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
#include "malloc.h"	            //�ڴ���� 
#include "usbh_hid_gamepad.h"
#include "sai.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//��������ֲ������ye781205��SMSģ��������
//ALIENTEK STM32������
//SMS������ ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/10/12
//�汾��V1.0  			  
////////////////////////////////////////////////////////////////////////////////// 	 
 

u16 *smssaibuf1; 			//��Ƶ����֡,ռ���ڴ��� 367*2 �ֽ�@22050Hz
u16 *smssaibuf2; 			//��Ƶ����֡,ռ���ڴ��� 367*2 �ֽ�@22050Hz

u8* sms_rom;				//SMS ROM�洢��ַָ��
u8* SMS_romfile;			//ROMָ��=sms_rom/sms_rom+512
u8*  S_RAM;					//internal SMS RAM	16k  [0x4000] 	 
u8*  E_RAM;					//external cartridge RAM (2 X 16K Bank Max) [0x8000] 


//�ͷ�SMS����������ڴ�
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

//ΪSMS���������ڴ�
//����ֵ:0,����ɹ�
//      1,����ʧ��
u8 sms_sram_malloc(u32 romsize)
{ 
	E_RAM=mymalloc(SRAMEX,0X8000);				//����2*16K�ֽ�
	cache=mymalloc(SRAMEX,0x20000);				//128K
	sms_rom=mymalloc(SRAMEX,romsize);			//����romsize�ֽڵ��ڴ�����	
	if(sms_rom==NULL)
	{
		spb_delete();//�ͷ�SPBռ�õ��ڴ�
		sms_rom=mymalloc(SRAMEX,romsize);		//��������
	}	
	
	S_RAM=mymalloc(SRAMCCM,0X4000);				//����16K�ֽ�
 	SMS_VDP=mymalloc(SRAMCCM,sizeof(SVDP));		
	psg_buffer=mymalloc(SRAMCCM,SNBUF_size*2);	//�����ڴ�SNBUF_size
	SN76496=mymalloc(SRAMCCM,sizeof(t_SN76496));//�����ڴ�184
	Z80A=mymalloc(SRAMCCM,sizeof(CPU80));		//�����ڴ� 
	VRam=mymalloc(SRAMCCM,0x4000); 				//����16K�ֽ�
	
	smssaibuf1=mymalloc(SRAMIN,SNBUF_size*4+10);
	smssaibuf2=mymalloc(SRAMIN,SNBUF_size*4+10);  
	lut=mymalloc(SRAMIN,0x10000); 				//64K
	if(sms_rom&&cache&&VRam&&lut)
	{ 
		memset(E_RAM,0,0X8000);					//����
		memset(cache,0,0x20000);				//����
		memset(S_RAM,0,0X4000);					//����
		memset(SMS_VDP,0,sizeof(SVDP));			//����
		
		memset(psg_buffer,0,SNBUF_size*2);		//����
		memset(SN76496,0,sizeof(t_SN76496));	//����
		memset(Z80A,0,sizeof(CPU80));			//���� 
		memset(VRam,0,0X4000);					//����
		
		memset(smssaibuf1,0,SNBUF_size*4+10);	//����
		memset(smssaibuf2,0,SNBUF_size*4+10);	//����
		memset(lut,0,0x10000);					//����
		return 0;
	}else
	{
		sms_sram_free();						//�ͷ������ڴ�.
		return 1;
	}
} 

u16 sms_xoff=0;	//��ʾ��x�᷽���ƫ����(ʵ����ʾ���=256-2*sms_xoff)
u16 sms_yoff=0;	//��ʾ��y�᷽���ƫ����
//RGB����Ҫ��3������ ,���㷽���ο�nes��Ϸ
u32 sms_rgb_parm1;
u16 sms_rgb_parm2;
u16 sms_rgb_parm3;

//������Ϸ��ʾ����
void sms_set_window(void)
{	
	u16 xoff=0,yoff=0; 
	u16 lcdwidth,lcdheight;
	if(lcddev.width==240)
	{
		lcdwidth=240;
		lcdheight=192;
		sms_xoff=(256-lcddev.width)/2;	//�õ�x�᷽���ƫ���� 
	}else if(lcddev.width<=320) 
	{
		lcdwidth=240;
		lcdheight=192; 
		sms_xoff=8; //sms��Ҫƫ��8����
	}else if(lcddev.width>=480)
	{
		lcdwidth=480;
		lcdheight=192*2; 
		sms_xoff=(256-(lcdwidth/2))/2;//�õ�x�᷽���ƫ���� 
	}	
	xoff=(lcddev.width-lcdwidth)/2;   
	if(lcdltdc.pwidth)//RGB��
	{
		if(lcddev.id==0X4342)sms_rgb_parm2=lcddev.height*2;
		else sms_rgb_parm2=lcddev.height*2*2; 
		sms_rgb_parm3=sms_rgb_parm2/2;
		if(lcddev.id==0X4342)sms_rgb_parm1=260160-sms_rgb_parm2*xoff; 
		else if(lcddev.id==0X7084)sms_rgb_parm1=766400-sms_rgb_parm3*xoff; 
		else if(lcddev.id==0X7016||lcddev.id==0X1016)sms_rgb_parm1=1226752-sms_rgb_parm3*xoff; 
		else if(lcddev.id==0X1018)sms_rgb_parm1=2045440-sms_rgb_parm3*xoff; 
	}	
	yoff=(lcddev.height-lcdheight-gui_phy.tbheight)/2+gui_phy.tbheight;//��Ļ�߶� 
	sms_yoff=yoff;
	LCD_Set_Window(xoff,yoff,lcdwidth,lcdheight);//��smsʼ������Ļ����������ʾ
	LCD_SetCursor(xoff,yoff);
	LCD_WriteRAM_Prepare();//д��LCD RAM��׼��  
}

extern vu8 framecnt; 

//ģ�������������ֳ�ʼ��,Ȼ��ѭ������ģ����
void sms_start(u8 bank_mun)
{
	u8 zhen;
	u8 res=0;  
	res=VDP_init(); 	
    res+=Z80A_Init(S_RAM, E_RAM,SMS_romfile,bank_mun);//0x8080000,0x0f,"Sonic the Hedgehog '91"
    res+=sms_audio_init(); 
	if(res==0)
	{
		TIM8_Int_Init(10000-1,19200-1);//����TIM8,1s�ж�һ��	
		sms_set_window();			//���ô���
		while(1)
		{				
			SMS_frame(zhen);		//+FB_OFS  (24+240*32)	
			nes_get_gamepadval();	//����sms���ֱ����ݻ�ȡ����
			sms_update_Sound();
			sms_update_pad();		//��ȡ�ֱ�ֵ
			zhen++;
			framecnt++; 
			if(zhen>2)zhen=0; 		//��2֡
			if(system_task_return)break;//TPAD����  
			if(lcddev.id==0X1963)//����1963,ÿ����һ֡,��Ҫ���贰��
			{
				nes_set_window();
			} 
		}
		TIM8->CR1&=~(1<<0);//�رն�ʱ��8
		LCD_Set_Window(0,0,lcddev.width,lcddev.height);//�ָ���Ļ����
	}
	sms_sound_close();//�ر���Ƶ���
} 

//�����ֱ�����
//SMS��ֵ 1111 1111 ȫ1��ʾû����
//	     D7  D6  D5   D4   D3  D2  D1  D0
//SMS    B   A   ��   ��   ��  ��  
// FC    ��  ��  ��   ��   ST   S   B   A	
void sms_update_pad(void) 
{
	u8 key,key1;
	key=255-fcpad.ctrlval;	//��FC�ֱ���ֵȡ��.
	key1=(key>>4)|0xf0; 	//ת��ΪSMS�ֱ���ֵ
	key1&=((key<<4)|0xcf); 
    SetController(key1); 
}
//����SMS��Ϸ
//pname:sms��Ϸ·��
//����ֵ:
//0,�����˳�
//1,�ڴ����
//2,�ļ����� 
u8 sms_load(u8* pname)
{
	u8 bank_mun;//16K bank������
	u8 res=0;  
	FIL* f_sms; 
	if(audiodev.status&(1<<7))		//��ǰ�ڷŸ�??
	{
		audio_stop_req(&audiodev);	//ֹͣ��Ƶ����
		audio_task_delete();		//ɾ�����ֲ�������.
	}  
 	f_sms=(FIL *)mymalloc(SRAMIN,sizeof(FIL));		//����FIL�ֽڵ��ڴ����� 
	if(f_sms==NULL)return 1;						//����ʧ��
 	res=f_open(f_sms,(const TCHAR*)pname,FA_READ);	//���ļ�
   	if(res==0)res=sms_sram_malloc(f_sms->obj.objsize);	//�����ڴ�
	if(res==0)
	{		 
		if((f_sms->obj.objsize/512)&1)					//�չ�ͼ�����,�������
		{
			SMS_romfile=sms_rom+512;
			bank_mun=((f_sms->obj.objsize-512)/0x4000)-1;	//16K bank������
		}else 
		{
			SMS_romfile=sms_rom;
            bank_mun=(f_sms->obj.objsize/0x4000)-1;		//16K bank������
		}		   
		res=f_read(f_sms,sms_rom,f_sms->obj.objsize,&br);	//��ȡ����SMS��Ϸ�ļ� 
		if(res)res=2;								//�ļ�����
		f_close(f_sms);   							//�ر��ļ�
	}
	myfree(SRAMIN,f_sms);						 	//�ͷ��ڴ�	
 	if(res==0)
	{
        sms_start(bank_mun);						//��ʼ��Ϸ
	} 	
	sms_sram_free(); 
	return res;
} 
 

vu8 smstransferend=0;	//sai������ɱ�־
vu8 smswitchbuf=0;		//saibufxָʾ��־
//sai��Ƶ���Żص�����
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
//SMS����Ƶ���
void sms_sound_open(int sample_rate) 
{
	//printf("sound open:%d\r\n",sample_rate); 
	app_wm8978_volset(wm8978set.mvol);	 
	WM8978_ADDA_Cfg(1,0);					//����DAC
	WM8978_Input_Cfg(0,0,0);				//�ر�����ͨ��
	WM8978_Output_Cfg(1,0);					//����DAC���
	WM8978_I2S_Cfg(2,0);					//�����ֱ�׼,16λ���ݳ���

	SAIA_Init(0,1,4);						//����SAI,������,16λ���� 
	SAIA_SampleRate_Set(sample_rate);		//���ò����� 
	SAIA_TX_DMA_Init((u8*)smssaibuf1,(u8*)smssaibuf2,2*SNBUF_size,1);//DMA���� 
 	sai_tx_callback=sms_sai_dma_tx_callback;//�ص�����ָsms_sai_dma_tx_callback
	SAI_Play_Start();						//����DMA    
}
//SMS�ر���Ƶ���
void sms_sound_close(void) 
{ 
	SAI_Play_Stop();
	app_wm8978_volset(0);				//�ر�WM8978�������
} 
//SMS��Ƶ�����SAI����
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







