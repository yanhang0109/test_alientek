#include "sys.h"
#include "ff.h"
#include "exfuns.h" 
#include "lcd.h"
#include "audioplay.h"
#include "spb.h"

#include "sms_vdp.h"
#include "sms_main.h"
#include "sms_sn76496.h"
#include "sms_z80a.h"

#include "nes_main.h" 
#include "malloc.h"	            //内存管理 
#include "usbh_hid_gamepad.h"
#include "sai.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//本程序移植自网友ye781205的SMS模拟器工程
//ALIENTEK STM32开发板
//SMS主函数 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/10/12
//版本：V1.0  			  
////////////////////////////////////////////////////////////////////////////////// 	 
 

u16 *smssaibuf1; 			//音频缓冲帧,占用内存数 367*2 字节@22050Hz
u16 *smssaibuf2; 			//音频缓冲帧,占用内存数 367*2 字节@22050Hz

u8* sms_rom;				//SMS ROM存储地址指针
u8* SMS_romfile;			//ROM指针=sms_rom/sms_rom+512
u8*  S_RAM;					//internal SMS RAM	16k  [0x4000] 	 
u8*  E_RAM;					//external cartridge RAM (2 X 16K Bank Max) [0x8000] 


//释放SMS申请的所有内存
void sms_sram_free(void)
{
	myfree(SRAMEX,sms_rom);
	myfree(SRAMEX,E_RAM);
	myfree(SRAMEX,cache);
	
	myfree(SRAMCCM,S_RAM);
	myfree(SRAMCCM,SMS_VDP);
	myfree(SRAMCCM,psg_buffer);
	myfree(SRAMCCM,SN76496);
	myfree(SRAMCCM,Z80A);
	myfree(SRAMCCM,VRam);
	
	myfree(SRAMIN,smssaibuf1);
	myfree(SRAMIN,smssaibuf2);
	myfree(SRAMIN,lut);
}

//为SMS运行申请内存
//返回值:0,申请成功
//      1,申请失败
u8 sms_sram_malloc(u32 romsize)
{ 
	E_RAM=mymalloc(SRAMEX,0X8000);				//申请2*16K字节
	cache=mymalloc(SRAMEX,0x20000);				//128K
	sms_rom=mymalloc(SRAMEX,romsize);			//开辟romsize字节的内存区域	
	if(sms_rom==NULL)
	{
		spb_delete();//释放SPB占用的内存
		sms_rom=mymalloc(SRAMEX,romsize);		//重新申请
	}	
	
	S_RAM=mymalloc(SRAMCCM,0X4000);				//申请16K字节
 	SMS_VDP=mymalloc(SRAMCCM,sizeof(SVDP));		
	psg_buffer=mymalloc(SRAMCCM,SNBUF_size*2);	//申请内存SNBUF_size
	SN76496=mymalloc(SRAMCCM,sizeof(t_SN76496));//申请内存184
	Z80A=mymalloc(SRAMCCM,sizeof(CPU80));		//申请内存 
	VRam=mymalloc(SRAMCCM,0x4000); 				//申请16K字节
	
	smssaibuf1=mymalloc(SRAMIN,SNBUF_size*4+10);
	smssaibuf2=mymalloc(SRAMIN,SNBUF_size*4+10);  
	lut=mymalloc(SRAMIN,0x10000); 				//64K
	if(sms_rom&&cache&&VRam&&lut)
	{ 
		memset(E_RAM,0,0X8000);					//清零
		memset(cache,0,0x20000);				//清零
		memset(S_RAM,0,0X4000);					//清零
		memset(SMS_VDP,0,sizeof(SVDP));			//清零
		
		memset(psg_buffer,0,SNBUF_size*2);		//清零
		memset(SN76496,0,sizeof(t_SN76496));	//清零
		memset(Z80A,0,sizeof(CPU80));			//清零 
		memset(VRam,0,0X4000);					//清零
		
		memset(smssaibuf1,0,SNBUF_size*4+10);	//清零
		memset(smssaibuf2,0,SNBUF_size*4+10);	//清零
		memset(lut,0,0x10000);					//清零
		return 0;
	}else
	{
		sms_sram_free();						//释放所有内存.
		return 1;
	}
} 

u16 sms_xoff=0;	//显示在x轴方向的偏移量(实际显示宽度=256-2*sms_xoff)
u16 sms_yoff=0;	//显示在y轴方向的偏移量
//RGB屏需要的3个参数 ,计算方法参考nes游戏
u32 sms_rgb_parm1;
u16 sms_rgb_parm2;
u16 sms_rgb_parm3;

//设置游戏显示窗口
void sms_set_window(void)
{	
	u16 xoff=0,yoff=0; 
	u16 lcdwidth,lcdheight;
	if(lcddev.width==240)
	{
		lcdwidth=240;
		lcdheight=192;
		sms_xoff=(256-lcddev.width)/2;	//得到x轴方向的偏移量 
	}else if(lcddev.width<=320) 
	{
		lcdwidth=240;
		lcdheight=192; 
		sms_xoff=8; //sms需要偏移8像素
	}else if(lcddev.width>=480)
	{
		lcdwidth=480;
		lcdheight=192*2; 
		sms_xoff=(256-(lcdwidth/2))/2;//得到x轴方向的偏移量 
	}	
	xoff=(lcddev.width-lcdwidth)/2;   
	if(lcdltdc.pwidth)//RGB屏
	{
		if(lcddev.id==0X4342)sms_rgb_parm2=lcddev.height*2;
		else sms_rgb_parm2=lcddev.height*2*2; 
		sms_rgb_parm3=sms_rgb_parm2/2;
		if(lcddev.id==0X4342)sms_rgb_parm1=260160-sms_rgb_parm2*xoff; 
		else if(lcddev.id==0X7084)sms_rgb_parm1=766400-sms_rgb_parm3*xoff; 
		else if(lcddev.id==0X7016||lcddev.id==0X1016)sms_rgb_parm1=1226752-sms_rgb_parm3*xoff; 
		else if(lcddev.id==0X1018)sms_rgb_parm1=2045440-sms_rgb_parm3*xoff; 
	}	
	yoff=(lcddev.height-lcdheight-gui_phy.tbheight)/2+gui_phy.tbheight;//屏幕高度 
	sms_yoff=yoff;
	LCD_Set_Window(xoff,yoff,lcdwidth,lcdheight);//让sms始终在屏幕的正中央显示
	LCD_SetCursor(xoff,yoff);
	LCD_WriteRAM_Prepare();//写入LCD RAM的准备  
}

extern vu8 framecnt; 

//模拟器启动，各种初始化,然后循环运行模拟器
void sms_start(u8 bank_mun)
{
	u8 zhen;
	u8 res=0;  
	res=VDP_init(); 	
    res+=Z80A_Init(S_RAM, E_RAM,SMS_romfile,bank_mun);//0x8080000,0x0f,"Sonic the Hedgehog '91"
    res+=sms_audio_init(); 
	if(res==0)
	{
		TIM8_Int_Init(10000-1,19200-1);//启动TIM8,1s中断一次	
		sms_set_window();			//设置窗口
		while(1)
		{				
			SMS_frame(zhen);		//+FB_OFS  (24+240*32)	
			nes_get_gamepadval();	//借用sms的手柄数据获取函数
			sms_update_Sound();
			sms_update_pad();		//获取手柄值
			zhen++;
			framecnt++; 
			if(zhen>2)zhen=0; 		//跳2帧
			if(system_task_return)break;//TPAD返回  
			if(lcddev.id==0X1963)//对于1963,每更新一帧,都要重设窗口
			{
				nes_set_window();
			} 
		}
		TIM8->CR1&=~(1<<0);//关闭定时器8
		LCD_Set_Window(0,0,lcddev.width,lcddev.height);//恢复屏幕窗口
	}
	sms_sound_close();//关闭音频输出
} 

//更新手柄数据
//SMS键值 1111 1111 全1表示没按键
//	     D7  D6  D5   D4   D3  D2  D1  D0
//SMS    B   A   右   左   下  上  
// FC    右  左  下   上   ST   S   B   A	
void sms_update_pad(void) 
{
	u8 key,key1;
	key=255-fcpad.ctrlval;	//将FC手柄的值取反.
	key1=(key>>4)|0xf0; 	//转换为SMS手柄的值
	key1&=((key<<4)|0xcf); 
    SetController(key1); 
}
//加载SMS游戏
//pname:sms游戏路径
//返回值:
//0,正常退出
//1,内存错误
//2,文件错误 
u8 sms_load(u8* pname)
{
	u8 bank_mun;//16K bank的数量
	u8 res=0;  
	FIL* f_sms; 
	if(audiodev.status&(1<<7))		//当前在放歌??
	{
		audio_stop_req(&audiodev);	//停止音频播放
		audio_task_delete();		//删除音乐播放任务.
	}  
 	f_sms=(FIL *)mymalloc(SRAMIN,sizeof(FIL));		//开辟FIL字节的内存区域 
	if(f_sms==NULL)return 1;						//申请失败
 	res=f_open(f_sms,(const TCHAR*)pname,FA_READ);	//打开文件
   	if(res==0)res=sms_sram_malloc(f_sms->obj.objsize);	//申请内存
	if(res==0)
	{		 
		if((f_sms->obj.objsize/512)&1)					//照顾图像标题,如果存在
		{
			SMS_romfile=sms_rom+512;
			bank_mun=((f_sms->obj.objsize-512)/0x4000)-1;	//16K bank的数量
		}else 
		{
			SMS_romfile=sms_rom;
            bank_mun=(f_sms->obj.objsize/0x4000)-1;		//16K bank的数量
		}		   
		res=f_read(f_sms,sms_rom,f_sms->obj.objsize,&br);	//读取整个SMS游戏文件 
		if(res)res=2;								//文件错误
		f_close(f_sms);   							//关闭文件
	}
	myfree(SRAMIN,f_sms);						 	//释放内存	
 	if(res==0)
	{
        sms_start(bank_mun);						//开始游戏
	} 	
	sms_sram_free(); 
	return res;
} 
 

vu8 smstransferend=0;	//sai传输完成标志
vu8 smswitchbuf=0;		//saibufx指示标志
//sai音频播放回调函数
void sms_sai_dma_tx_callback(void)
{ 
	u16 i;
	if(DMA2_Stream3->CR&(1<<19))smswitchbuf=0; 
	else smswitchbuf=1; 
	if(smstransferend)
	{
		if(smswitchbuf)for(i=0;i<SNBUF_size*2;i++)smssaibuf2[i]=0;	
		else for(i=0;i<SNBUF_size*2;i++)smssaibuf1[i]=0;
	}
	smstransferend=1;
}
//SMS打开音频输出
void sms_sound_open(int sample_rate) 
{
	//printf("sound open:%d\r\n",sample_rate); 
	app_wm8978_volset(wm8978set.mvol);	 
	WM8978_ADDA_Cfg(1,0);					//开启DAC
	WM8978_Input_Cfg(0,0,0);				//关闭输入通道
	WM8978_Output_Cfg(1,0);					//开启DAC输出
	WM8978_I2S_Cfg(2,0);					//飞利浦标准,16位数据长度

	SAIA_Init(0,1,4);						//设置SAI,主发送,16位数据 
	SAIA_SampleRate_Set(sample_rate);		//设置采样率 
	SAIA_TX_DMA_Init((u8*)smssaibuf1,(u8*)smssaibuf2,2*SNBUF_size,1);//DMA配置 
 	sai_tx_callback=sms_sai_dma_tx_callback;//回调函数指sms_sai_dma_tx_callback
	SAI_Play_Start();						//开启DMA    
}
//SMS关闭音频输出
void sms_sound_close(void) 
{ 
	SAI_Play_Stop();
	app_wm8978_volset(0);				//关闭WM8978音量输出
} 
//SMS音频输出到SAI缓存
void sms_apu_fill_buffer(int samples,u16* wavebuf)
{	
 	int i;	 
	while(!smstransferend){};
    smstransferend=0;
    if(smswitchbuf==0)
	{
		for(i=0;i<SNBUF_size;i++)
		{
			smssaibuf1[2*i]=wavebuf[i];
			smssaibuf1[2*i+1]=wavebuf[i];
		}
	}else 
	{
		for(i=0;i<SNBUF_size;i++)
		{
			smssaibuf2[2*i]=wavebuf[i];
			smssaibuf2[2*i+1]=wavebuf[i];
		}
	}
} 







