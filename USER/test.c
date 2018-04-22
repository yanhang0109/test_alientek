#include "includes.h"  
#include "malloc.h"  
#include "pcf8574.h"
#include "spblcd.h"
#include "spb.h"
#include "common.h" 
#include "ebook.h"
#include "settings.h"
#include "picviewer.h"
#include "audioplay.h"
#include "videoplay.h"
#include "calendar.h" 	 
#include "nesplay.h"	 
#include "notepad.h"
#include "exeplay.h"
#include "paint.h"
#include "camera.h"
#include "recorder.h"
#include "usbplay.h" 
#include "wirelessplay.h"
#include "calculator.h"  
#include "netplay.h"  
#include "webcamera.h" 
#include "gyroscope.h"
#include "appplay.h" 
#include "qrplay.h" 
#include "facereco.h" 
#include "gradienter.h"
#include "beepplay.h"
#include "keyplay.h"
#include "ledplay.h"
#include "phoneplay.h"
#include "smsplay.h" 
#include "gsm.h"
#include "mpu9250.h"
#include "wm8978.h" 
#include "ap3216c.h"
#include "usb_app.h"

//ALIENTEK 阿波罗STM32F429开发板 实验62
//综合测试 实验 
//技术支持：www.openedv.com
//广州市星翼电子科技有限公司  

/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			10 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				64
//任务堆栈，8字节对齐	
__align(8) static OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);	
 			   
//串口任务
//设置任务优先级
#define USART_TASK_PRIO       			7 
//设置任务堆栈大小
#define USART_STK_SIZE  		    	128
//任务堆栈，8字节对齐	
__align(8) static OS_STK USART_TASK_STK[USART_STK_SIZE];
//任务函数
void usart_task(void *pdata);
							 
//主任务
//设置任务优先级
#define MAIN_TASK_PRIO       			6 
//设置任务堆栈大小
#define MAIN_STK_SIZE  					1200
//任务堆栈，8字节对齐	
__align(8) static OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//任务函数
void main_task(void *pdata);

//监视任务
//设置任务优先级
#define WATCH_TASK_PRIO       			3 
//设置任务堆栈大小
#define WATCH_STK_SIZE  		   		256
//任务堆栈，8字节对齐	
__align(8) static OS_STK WATCH_TASK_STK[WATCH_STK_SIZE];
//任务函数
void watch_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////	 

//外部内存测试(最大支持32M字节内存测试)
//x,y:坐标
//fsize:字体大小
//返回值:0,成功;1,失败.
u8 system_exsram_test(u16 x,u16 y,u8 fsize)
{  
	u32 i=0;  
	u16 j=0;
	u32 temp=0;	   
	u32 sval=0;	//在地址0读到的数据	  	
	u32 *tempbuf;
	tempbuf=mymalloc(SRAMIN,1024*4);
  	LCD_ShowString(x,y,lcddev.width,y+fsize,fsize,"Ex Memory Test:    0KB");  
	//每隔32K字节,写入一个数据,总共写入1024个数据,刚好是32M字节
	for(i=0;i<32*1024*1024;i+=32*1024)
	{
		tempbuf[temp]=*(vu32*)(Bank5_SDRAM_ADDR+i);//保存原来的数据
		delay_us(1);
		*(vu32*)(Bank5_SDRAM_ADDR+i)=temp; 
		temp++;
	}
	//依次读出之前写入的数据,进行校验		  
 	for(i=0;i<32*1024*1024;i+=32*1024) 
	{	
  		temp=*(vu32*)(Bank5_SDRAM_ADDR+i);
		delay_us(1);
		*(vu32*)(Bank5_SDRAM_ADDR+i)=tempbuf[j++]; 
		if(i==0)sval=temp;
 		else if(temp<=sval)break;//后面读出的数据一定要比第一次读到的数据大. 
		LCD_ShowxNum(x+15*(fsize/2),y,(u16)(temp-sval+1)*32,5,fsize,0);//显示内存容量   
 	}	 
	myfree(SRAMIN,tempbuf);		//释放内存
	if(i>=1024*1024)return 0;	//内存正常,成 
	return 1;//失败
}
//显示错误信息
//x,y:坐标
//fsize:字体大小
//x,y:坐标.err:错误信息
void system_error_show(u16 x,u16 y,u8*err,u8 fsize)
{
	POINT_COLOR=RED;
 	while(1)
	{
		LCD_ShowString(x,y,lcddev.width,lcddev.height,fsize,err);
		delay_ms(400);
		LCD_Fill(x,y,lcddev.width-1,y+fsize,BLACK);
		delay_ms(100);
		LED0=!LED0;
	}  
}
//显示错误，然后继续运行
//x,y:坐标
//fsize:字体大小
//str:字符串
void system_error_show_pass(u16 x,u16 y,u8 fsize,u8*str)
{
	POINT_COLOR=RED;
	PCF8574_WriteBit(BEEP_IO,0);
	LCD_ShowString(x,y,lcddev.width,lcddev.height,fsize,str);
	delay_ms(2000);
	PCF8574_WriteBit(BEEP_IO,1);
	POINT_COLOR=WHITE;
}
//擦除整个SPI FLASH(即所有资源都删除),以快速更新系统.
//x,y:坐标
//fsize:字体大小
//x,y:坐标.err:错误信息
//返回值:0,没有擦除;1,擦除了
u8 system_files_erase(u16 x,u16 y,u8 fsize)
{
	u8 key;
	u8 t=0;
	POINT_COLOR=RED;
	LCD_ShowString(x,y,lcddev.width,lcddev.height,fsize,"Erase all system files?");
	while(1)
	{
		t++;
		if(t==20)LCD_ShowString(x,y+fsize,lcddev.width,lcddev.height,fsize,"KEY0:NO / KEY2:YES");
		if(t==40)
		{
			gui_fill_rectangle(x,y+fsize,lcddev.width,fsize,BLACK);//清除显示
			t=0;
			LED0=!LED0;
		}
		key=KEY_Scan(0);
		if(key==KEY0_PRES)//不擦除,用户取消了
		{ 
			gui_fill_rectangle(x,y,lcddev.width,fsize*2,BLACK);//清除显示
			POINT_COLOR=WHITE;
			LED0=1;
			return 0;
		}
		if(key==KEY2_PRES)//要擦除,要重新来过
		{
			LED0=1;
			LCD_ShowString(x,y+fsize,lcddev.width,lcddev.height,fsize,"Erasing SPI FLASH...");
			W25QXX_Erase_Chip();
			LCD_ShowString(x,y+fsize,lcddev.width,lcddev.height,fsize,"Erasing SPI FLASH OK");
			delay_ms(600);
			return 1;
		}
		delay_ms(10);
	}
}
//字库更新确认提示.
//x,y:坐标
//fsize:字体大小 
//返回值:0,不需要更新;1,确认要更新
u8 system_font_update_confirm(u16 x,u16 y,u8 fsize)
{
	u8 key;
	u8 t=0;
	u8 res=0;
	POINT_COLOR=RED;
	LCD_ShowString(x,y,lcddev.width,lcddev.height,fsize,"Update font?");
	while(1)
	{
		t++;
		if(t==20)LCD_ShowString(x,y+fsize,lcddev.width,lcddev.height,fsize,"KEY0:NO / KEY2:YES");
		if(t==40)
		{
			gui_fill_rectangle(x,y+fsize,lcddev.width,fsize,BLACK);//清除显示
			t=0;
			LED0=!LED0;
		}
		key=KEY_Scan(0);
		if(key==KEY0_PRES)break;//不更新 
		if(key==KEY2_PRES){res=1;break;}//要更新 
		delay_ms(10);
	}
	LED0=1;
	gui_fill_rectangle(x,y,lcddev.width,fsize*2,BLACK);//清除显示
	POINT_COLOR=WHITE;
	return res;
}

u8 tpad_failed_flag=0;	//TPAD失效标记，如果失效，则使用WK_UP作为退出按键

//系统初始化
void system_init(void)
{
 	u16 okoffset=162;
 	u16 ypos=0;
	u16 j=0;
	u16 temp=0;
	u8 res;
	u32 dtsize,dfsize;
	u8 *stastr=0;
	u8 *version=0; 
	u8 verbuf[12];
	u8 fsize;
	u8 icowidth;
	
	Stm32_Clock_Init(384,25,2,8);//设置时钟,192Mhz
	delay_init(192);			//初始化延时函数 
	exeplay_app_check();		//检测是否需要运行APP代码.  
	uart_init(96,115200);		//初始化串口波特率为115200
	usart3_init(48,115200);		//初始化串口3波特率为115200
	usmart_dev.init(96);		//初始化USMART
 	LED_Init();					//初始化LED 
	SDRAM_Init();				//初始化SDRAM
 	LCD_Init();					//LCD初始化   
 	KEY_Init();					//按键初始化   
 	AT24CXX_Init();    			//EEPROM初始化
	W25QXX_Init();				//初始化W25Q128
	Adc_Init();					//初始化内部温度传感器  
	PCF8574_Init();				//PCF8574初始化
	my_mem_init(SRAMIN);		//初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部内存池 
	my_mem_init(SRAMCCM);		//初始化CCM内存池 
	
	tp_dev.init(); 
	gui_init();	  
	piclib_init();				//piclib初始化	  
	slcd_dma_init();
 	usbapp_init(); 
	exfuns_init();				//FATFS 申请内存
	
	version=mymalloc(SRAMIN,31);//申请31个字节内存
REINIT://重新初始化
	LCD_Clear(BLACK);			//黑屏
	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	j=0;   
/////////////////////////////////////////////////////////////////////////
//显示版权信息
	ypos=2;
	if(lcddev.width<=272)
	{
		fsize=12;
		icowidth=18;
		okoffset=190;
		app_show_mono_icos(5,ypos,icowidth,24,(u8*)APP_ALIENTEK_ICO1824,YELLOW,BLACK);
	}else if(lcddev.width==320)
	{
		fsize=16;
		icowidth=24;
		okoffset=250;
		app_show_mono_icos(5,ypos,icowidth,32,(u8*)APP_ALIENTEK_ICO2432,YELLOW,BLACK);		
	}else if(lcddev.width>=480)
	{
		fsize=24;
		icowidth=36;
		okoffset=370;
		app_show_mono_icos(5,ypos,icowidth,48,(u8*)APP_ALIENTEK_ICO3648,YELLOW,BLACK);		
	}
	LCD_ShowString(icowidth+5*2,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "ALIENTEK Apollo STM32F4/F7");
	LCD_ShowString(icowidth+5*2,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,"Copyright (C) 2016-2026");    
	app_get_version(verbuf,HARDWARE_VERSION,2);
	strcpy((char*)version,"HARDWARE:");
	strcat((char*)version,(const char*)verbuf);
	strcat((char*)version,", SOFTWARE:");
	app_get_version(verbuf,SOFTWARE_VERSION,3);
	strcat((char*)version,(const char*)verbuf);
	LCD_ShowString(5,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,version);
	sprintf((char*)verbuf,"LCD ID:%04X",lcddev.id);		//LCD ID打印到verbuf里面
	LCD_ShowString(5,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, verbuf);	//显示LCD ID 
//////////////////////////////////////////////////////////////////////////
//开始硬件检测初始化
	WM8978_Init();			//防止喇叭乱叫
	app_wm8978_volset(0);	//关闭音量输出
	LED0=0;LED1=0;//同时点亮两个LED
	LCD_ShowString(5,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "CPU:STM32F429IGT6 192Mhz");
	LCD_ShowString(5,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "FLASH:1024KB  SRAM:256KB");	 
	if(system_exsram_test(5,ypos+fsize*j,fsize))system_error_show(5,ypos+fsize*j++,"EX Memory Error!",fsize);
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,"OK");			 
	my_mem_init(SRAMEX);		//初始化外部内存池,必须放在内存检测之后
	LED0=1;LED1=1;				//同时关闭两个LED 
	if(NAND_Init())				//检测不到NAND FLASH
	{	 
		system_error_show(5,ypos+fsize*j++,"NAND Flash Error!!",fsize); 
	}else temp=(nand_dev.block_totalnum/1024)*(nand_dev.page_mainsize/1024)*nand_dev.block_pagenum;//NAND容量
	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "NAND Flash:    MB");
	LCD_ShowxNum(5+11*(fsize/2),ypos+fsize*j,temp,4,fsize,0);//显示nand flash大小  
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");  
	if(W25QXX_ReadID()!=W25Q256)//检测不到W25Q256
	{	 
		system_error_show(5,ypos+fsize*j++,"SPI Flash Error!!",fsize); 
	}else temp=32*1024;			//32M字节大小
	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SPI Flash:     KB");			   
	LCD_ShowxNum(5+10*(fsize/2),ypos+fsize*j,temp,5,fsize,0);//显示spi flash大小  
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");   
	//检测是否需要擦除SPI FLASH?
	res=KEY_Scan(1);//
	if(res==KEY2_PRES)
	{
		res=system_files_erase(5,ypos+fsize*j,fsize);
		if(res)goto REINIT; 
	}
    //RTC检测
  	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "RTC Check...");			   
 	if(RTC_Init())system_error_show_pass(5+okoffset,ypos+fsize*j++,fsize,"ERROR"); //RTC检测
	else 
	{
		calendar_get_time(&calendar);//得到当前时间
		calendar_get_date(&calendar);//得到当前日期
		LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");			   
	}
	//检查SPI FLASH的文件系统
	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "FATFS Check...");//FATFS检测			   
  	f_mount(fs[0],"0:",1); 		//挂载SD卡  
  	f_mount(fs[1],"1:",1); 		//挂载SPI FLASH. 
  	f_mount(fs[2],"2:",1); 		//挂载NAND FLASH. 
 	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");			   
	//SD卡检测
	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SD Card:     MB");//FATFS检测
	temp=0;	
 	do
	{
		temp++;
 		res=exf_getfree("0:",&dtsize,&dfsize);//得到SD卡剩余容量和总容量
		delay_ms(200);		   
	}while(res&&temp<5);//连续检测5次
 	if(res==0)//得到容量正常
	{ 
		gui_phy.memdevflag|=1<<0;	//设置SD卡在位.
		temp=dtsize>>10;//单位转换为MB
		stastr="OK";
 	}else 
	{
 		temp=0;//出错了,单位为0
		stastr="ERROR";
	}
 	LCD_ShowxNum(5+8*(fsize/2),ypos+fsize*j,temp,5,fsize,0);					//显示SD卡容量大小
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,stastr);	//SD卡状态			   
	//W25Q256检测,如果不存在文件系统,则先创建.
	temp=0;	
 	do
	{
		temp++;
 		res=exf_getfree("1:",&dtsize,&dfsize);//得到FLASH剩余容量和总容量
		delay_ms(200);		   
	}while(res&&temp<20);//连续检测20次		  
	if(res==0X0D)//文件系统不存在
	{
		LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SPI Flash Disk Formatting...");//格式化FLASH
		res=f_mkfs("1:",1,4096);//格式化FLASH,1,盘符;1,不需要引导区,8个扇区为1个簇
		if(res==0)
		{
			f_setlabel((const TCHAR *)"1:ALIENTEK");				//设置Flash磁盘的名字为：ALIENTEK
			LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");//标志格式化成功
 			res=exf_getfree("1:",&dtsize,&dfsize);//重新获取容量
		}
	}   
	if(res==0)//得到FLASH卡剩余容量和总容量
	{
		gui_phy.memdevflag|=1<<1;	//设置SPI FLASH在位.
		LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SPI Flash Disk:     KB");//FATFS检测			   
		temp=dtsize; 	   
 	}else system_error_show(5,ypos+fsize*(j+1),"Flash Fat Error!",fsize);				//flash 文件系统错误 
 	LCD_ShowxNum(5+15*(fsize/2),ypos+fsize*j,temp,5,fsize,0);							//显示FLASH容量大小
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,"OK");	//FLASH卡状态
	//NAND FLASH检测
	temp=0;	
 	do
	{
		temp++;
 		res=exf_getfree("2:",&dtsize,&dfsize);//得到FLASH剩余容量和总容量
		delay_ms(200);		   
	}while(res&&temp<20);//连续检测20次		  
	if(res==0X0D)//文件系统不存在
	{
		LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "NAND Flash Disk Formatting...");//格式化FLASH
		res=f_mkfs("2:",1,4096);						//格式化FLASH,2,盘符;1,不需要引导区,8个扇区为1个簇
		if(res==0)
		{
			f_setlabel((const TCHAR *)"2:NANDDISK");	//设置Flash磁盘的名字为：NANDDISK
			LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");//标志格式化成功
 			res=exf_getfree("2:",&dtsize,&dfsize);		//重新获取容量
		}
	}   
	if(res==0)//得到NAND FLASH卡剩余容量和总容量
	{
		gui_phy.memdevflag|=1<<2;	//设置NAND FLASH在位.
		LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "NAND Flash Disk:    MB");//FATFS检测			   
		temp=dtsize>>10;			//单位转换为MB
 	}else system_error_show(5,ypos+fsize*(j+1),"NAND Flash Fat Error!",fsize);			//flash 文件系统错误 
 	LCD_ShowxNum(5+16*(fsize/2),ypos+fsize*j,temp,4,fsize,0);							//显示FLASH容量大小
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,"OK");	//FLASH卡状态	 
	//U盘检测
	usbapp_mode_set(0);												//设置为U盘模式
	temp=0; 
 	while(usbx.hdevclass==0&&temp<1600)	//等待U盘被检测,最多等待8秒
	{
		usbapp_pulling();
		if((usbx.bDeviceState&(1<<6))==0&&temp>300)break;//1.5秒钟之内,没有检测到设备插入,则直接跳过,不再等待
		delay_ms(5); 
		temp++;	
	}
	if(usbx.hdevclass==1)//检测到了U盘 
	{
		fs[3]->drv=3;
		f_mount(fs[3],"3:",1); 					//挂载挂载U盘
 		res=exf_getfree("3:",&dtsize,&dfsize);	//得到U盘剩余容量和总容量     
	}else res=0XFF;
	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "U Disk:     MB");	//U盘容量大小			   
 	if(res==0)//得到容量正常
	{
		gui_phy.memdevflag|=1<<3;		//设置U盘在位.
		temp=dtsize>>10;//单位转换为MB
		stastr="OK";
 	}else 
	{
 		temp=0;//出错了,单位为0
		stastr="ERROR";
	}
 	LCD_ShowxNum(5+7*(fsize/2),ypos+fsize*j,temp,5,fsize,0);					//显示U盘容量大小
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize,stastr);	//U盘状态	
	//TPAD检测		 
 	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "TPAD Check...");			   
 	if(TPAD_Init(2)||tpad_default_val>1000)//触摸按键检测
	{
		tpad_failed_flag=1;//tpad失效了，用wk_up按键替代
		system_error_show_pass(5+okoffset,ypos+fsize*j++,fsize,"ERROR");  
	}else LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK"); 
	//PCF8574检测		 
 	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "PCF8574 Check...");			   
 	if(PCF8574_Init())system_error_show_pass(5+okoffset,ypos+fsize*j++,fsize,"ERROR");	//PCF8574检测
	else LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK"); 
 	//MPU9250检测   
 	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "MPU9250 Check...");			   
  	if(MPU_Init())system_error_show_pass(5+okoffset,ypos+fsize*j++,fsize,"ERROR"); 		//MPU9250检测
 	else LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK"); 
 	//AP3216C检测   
 	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "AP3216C Check...");			   
  	if(AP3216C_Init())system_error_show_pass(5+okoffset,ypos+fsize*j++,fsize,"ERROR"); 	//AP3216C检测
 	else LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK"); 
	//24C02检测
   	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "24C02 Check...");			   
 	if(AT24CXX_Check())system_error_show(5,ypos+fsize*(j+1),"24C02 Error!",fsize);		//24C02检测
	else LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");  
  	//WM8978检测			   
 	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "WM8978 Check...");			   
 	if(WM8978_Init())system_error_show_pass(5+okoffset,ypos+fsize*j++,fsize,"ERROR"); 	//WM8978检测
	else 
	{
		LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");	
		app_wm8978_volset(0);				//关闭WM8978音量输出		    		   
  	}
	//字库检测									    
   	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "Font Check...");
	res=KEY_Scan(1);//检测按键			   
	if(res==KEY1_PRES)//更新？确认
	{
		res=system_font_update_confirm(5,ypos+fsize*(j+1),fsize);
	}else res=0;
	if(font_init()||(res==1))//检测字体,如果字体不存在/强制更新,则更新字库	
	{
		res=0;//按键无效
 		if(update_font(5,ypos+fsize*j,fsize,"0:")!=0)//从SD卡更新
		{
			TIM8_Int_Init(100-1,19200-1);//启动TIM8 轮询USB,10ms中断一次	
 			if(update_font(5,ypos+fsize*j,fsize,"3:")!=0)//从U盘更新
			{ 
				system_error_show(5,ypos+fsize*(j+1),"Font Error!",fsize);		//字体错误
			}
			TIM8->CR1&=~(1<<0);//关闭定时器8
		}			
		LCD_Fill(5,ypos+fsize*j,lcddev.width-1,ypos+fsize*(j+1),BLACK);			//填充底色
    	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "Font Check...");			   
 	} 
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");//字库检测OK
	//系统文件检测
   	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SYSTEM Files Check...");			   
 	while(app_system_file_check("1"))//系统文件检测
	{
		LCD_Fill(5,ypos+fsize*j,lcddev.width-1,ypos+fsize*(j+1),BLACK);		//填充底色
    	LCD_ShowString(5,ypos+fsize*j,(fsize/2)*8,fsize,fsize, "Updating");	//显示updating	
		app_boot_cpdmsg_set(5,ypos+fsize*j,fsize);		//设置到坐标
 		temp=0;
		TIM8_Int_Init(100-1,19200-1);					//启动TIM8 轮询USB,10ms中断一次	 
		if(app_system_file_check("0"))					//检查SD卡系统文件完整性
		{ 
			if(app_system_file_check("3"))res=9;		//标记为不可用的盘	
			else res=3;									//标记为U盘	
		}else res=0;									//标记为SD卡
		if(res==0||res==3)								//完整了才更新
		{	
			sprintf((char*)verbuf,"%d:",res);   
			if(app_system_update(app_boot_cpdmsg,verbuf))//更新?
			{
				system_error_show(5,ypos+fsize*(j+1),"SYSTEM File Error!",fsize);
			} 
		}
		TIM8->CR1&=~(1<<0);								//关闭定时器3 
		LCD_Fill(5,ypos+fsize*j,lcddev.width-1,ypos+fsize*(j+1),BLACK);//填充底色
    	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SYSTEM Files Check..."); 
		if(app_system_file_check("1"))//更新了一次，再检测，如果还有不全，说明SD卡文件就不全！
		{
			system_error_show(5,ypos+fsize*(j+1),"SYSTEM File Lost!",fsize);
		}else break;	
	}
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");	
 	//触摸屏检测 
	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "Touch Check...");			   
	res=KEY_Scan(1);//检测按键			   
	if(TP_Init()||(res==KEY0_PRES&&(tp_dev.touchtype&0X80)==0))//有更新/按下了KEY0且不是电容屏,执行校准 	
	{ 
		if(res==1)TP_Adjust();
		res=0;//按键无效
		goto REINIT;				//重新开始初始化
	}
	LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");//触摸屏检测OK
   	//系统参数加载			   
 	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SYSTEM Parameter Load...");			   
 	if(app_system_parameter_init())system_error_show(5,ypos+fsize*(j+1),"Parameter Load Error!",fsize);//参数加载
	else LCD_ShowString(5+okoffset,ypos+fsize*j++,lcddev.width,lcddev.height,fsize, "OK");			   
  	LCD_ShowString(5,ypos+fsize*j,lcddev.width,lcddev.height,fsize, "SYSTEM Starting...");  
	//蜂鸣器短叫,提示正常启动
	PCF8574_WriteBit(BEEP_IO,0);
	delay_ms(100);
	PCF8574_WriteBit(BEEP_IO,1);
	myfree(SRAMIN,version);	 
	delay_ms(1500);  
}  
//main函数	  					
int main(void)
{ 	
   	system_init();		//系统初始化  
   	OSInit();   
    OSTaskCreateExt((void(*)(void*) )start_task,                //任务函数
                    (void*          )0,                         //传递给任务函数的参数
                    (OS_STK*        )&START_TASK_STK[START_STK_SIZE-1],//任务堆栈栈顶
                    (INT8U          )START_TASK_PRIO,           //任务优先级
                    (INT16U         )START_TASK_PRIO,           //任务ID，这里设置为和优先级一样
                    (OS_STK*        )&START_TASK_STK[0],        //任务堆栈栈底
                    (INT32U         )START_STK_SIZE,            //任务堆栈大小
                    (void*          )0,                         //用户补充的存储区
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);//任务选项,为了保险起见，所有任务都保存浮点寄存器的值
	OSStart();	  						    
}
extern OS_EVENT * audiombox;	//音频播放任务邮箱
//开始任务
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	pdata = pdata; 	  
	OSStatInit();		//初始化统计任务.这里会延时1秒钟左右	
 	app_srand(OSTime);
  	audiombox=OSMboxCreate((void*) 0);	//创建邮箱
	OS_ENTER_CRITICAL();//进入临界区(无法被中断打断) 
    OSTaskCreateExt((void(*)(void*) )main_task,                	//任务函数
                    (void*          )0,                         //传递给任务函数的参数
                    (OS_STK*        )&MAIN_TASK_STK[MAIN_STK_SIZE-1],//任务堆栈栈顶
                    (INT8U          )MAIN_TASK_PRIO,           	//任务优先级
                    (INT16U         )MAIN_TASK_PRIO,           	//任务ID，这里设置为和优先级一样
                    (OS_STK*        )&MAIN_TASK_STK[0],        	//任务堆栈栈底
                    (INT32U         )MAIN_STK_SIZE,            	//任务堆栈大小
                    (void*          )0,                         //用户补充的存储区
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);//任务选项,为了保险起见，所有任务都保存浮点寄存器的值
    OSTaskCreateExt((void(*)(void*) )usart_task,              	//任务函数
                    (void*          )0,                         //传递给任务函数的参数
                    (OS_STK*        )&USART_TASK_STK[USART_STK_SIZE-1],//任务堆栈栈顶
                    (INT8U          )USART_TASK_PRIO,			//任务优先级
                    (INT16U         )USART_TASK_PRIO,			//任务ID，这里设置为和优先级一样
                    (OS_STK*        )&USART_TASK_STK[0],		//任务堆栈栈底
                    (INT32U         )USART_STK_SIZE,			//任务堆栈大小
                    (void*          )0,                         //用户补充的存储区
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);//任务选项,为了保险起见，所有任务都保存浮点寄存器的值
    OSTaskCreateExt((void(*)(void*) )watch_task,				//任务函数
                    (void*          )0,                         //传递给任务函数的参数
                    (OS_STK*        )&WATCH_TASK_STK[WATCH_STK_SIZE-1],//任务堆栈栈顶
                    (INT8U          )WATCH_TASK_PRIO,			//任务优先级
                    (INT16U         )WATCH_TASK_PRIO,			//任务ID，这里设置为和优先级一样
                    (OS_STK*        )&WATCH_TASK_STK[0],		//任务堆栈栈底
                    (INT32U         )WATCH_STK_SIZE,			//任务堆栈大小
                    (void*          )0,                         //用户补充的存储区
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);//任务选项,为了保险起见，所有任务都保存浮点寄存器的值
 	OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();	//退出临界区(可以被中断打断)
} 
	
//主任务
void main_task(void *pdata)
{
	u8 selx; 
	u16 tcnt=0;
	spb_init();			//初始化SPB界面
	spb_load_mui();		//加载SPB主界面
	slcd_frame_show(spbdev.spbahwidth);	//显示界面
	while(1)
	{
		selx=spb_move_chk(); 
		system_task_return=0;//清退出标志 
		switch(selx)//发生了双击事件
		{    
			case 0:ebook_play();break;				//电子图书 
 			case 1:picviewer_play();break;			//数码相框  
 			case 2:audio_play();break;				//音乐播放 
 			case 3:video_play();break;				//视频播放
			case 4:calendar_play();break;			//时钟 
 			case 5:sysset_play();break;				//系统设置
			case 6:nes_play();break;				//游戏
			case 7:notepad_play();break;			//记事本	
			case 8:exe_play();break;				//运行器
			case 9:paint_play();break;				//手写画笔
 			case 10:camera_play();break;			//摄像头
			case 11:recorder_play();break;			//录音
 			case 12:usb_play();break;				//USB连接
 	    	case 13:net_play();break;				//网络测试
			case 14:wireless_play();break;			//无线传书
			case 15:calc_play();break;				//计算器   
			case 16:qr_play();break;				//二维码
 			case 17:webcam_play();break;			//网络摄像头
			case 18:frec_play();break;				//人脸识别
			case 19:gyro_play();break;				//9轴传感器
			case 20:grad_play();break;				//水平仪
			case 21:beep_play();break;				//蜂鸣器测试
			case 22:key_play();break;				//按键测试
			case 23:led_play();break;				//led测试

			case SPB_ICOS_NUM:phone_play();break;	//电话功能
			case SPB_ICOS_NUM+1:app_play();break;	//APP 
 			case SPB_ICOS_NUM+2:sms_play();break;	//短信功能
		} 
		
		if(selx!=0XFF)spb_load_mui();//显示主界面
		delay_ms(1000/OS_TICKS_PER_SEC);//延时一个时钟节拍
		tcnt++;
		if(tcnt==500)	//500个节拍为1秒钟
		{
			tcnt=0;
			spb_stabar_msg_show(0);//更新状态栏信息
		}
	}
} 
//执行最不需要时效性的代码
void usart_task(void *pdata)
{	    
	u16 alarmtimse=0;
	float psin,psex,psccm;
	pdata=pdata;
	while(1)
	{			  
		delay_ms(1000);	 
		if(alarm.ringsta&1<<7)//执行闹钟扫描函数
		{
			calendar_alarm_ring(alarm.ringsta&0x3);//闹铃
			alarmtimse++;
			if(alarmtimse>300)//超过300次了,5分钟以上
			{
				alarm.ringsta&=~(1<<7);//关闭闹铃	
			}
	 	}else if(alarmtimse)
		{		 
			alarmtimse=0;
			PCF8574_WriteBit(BEEP_IO,1);//关闭蜂鸣器
		}
		if(gsmdev.mode==3)phone_ring();//蜂鸣器,来电提醒
		if(systemset.lcdbklight==0)app_lcd_auto_bklight();	//自动背光控制
		if(frec_running==0)
		{
			psin=my_mem_perused(SRAMIN);
			psex=my_mem_perused(SRAMEX);
			psccm=my_mem_perused(SRAMCCM);
			printf("in:%3.1f,ex:%3.1f,ccm:%3.1f\r\n",psin/10,psex/10,psccm/10);//打印内存占用率
		}
	}
}

vu8 system_task_return;		//任务强制返回标志.
vu8 ledplay_ds0_sta=0;		//ledplay任务,DS0的控制状态
//监视任务
void watch_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0; 
 	u8 t=0;	   
 	u8 rerreturn=0; 
	u8 res; 
	u8 key;
 	pdata=pdata;
	while(1)
	{			  
		if(alarm.ringsta&1<<7)//闹钟在执行
		{
			calendar_alarm_msg((lcddev.width-200)/2,(lcddev.height-160)/2);//闹钟处理
		}
		if(gifdecoding)//gif正在解码中
		{
			key=pic_tp_scan();
			if(key==1||key==3)gifdecoding=0;//停止GIF解码
		}
		if(ledplay_ds0_sta==0)		//仅当ledplay_ds0_sta等于0的时候,正常熄灭LED0
		{
			if(t==4)LED0=1;			//亮100ms左右
			if(t==119){LED0=0;t=0;}	//2.5秒钟左右亮一次 
		}			
		t++;
		if(rerreturn)//再次开始TPAD扫描时间减一
		{
			rerreturn--;
			delay_ms(15);//补充延时差	
 		}else if(tpad_failed_flag)				//TPAD失效，用WK_UP按键替代TPAD的功能
		{
			key=KEY_Scan(0);					//按键扫描
			if(key==WKUP_PRES)					//WKUP按键按下了
			{
				system_task_return=1;
				if(gifdecoding)gifdecoding=0;	//不再播放gif
			}
		}else if(TPAD_Scan(0))					//TPAD按下了一次,此函数执行,至少需要15ms.
		{
			rerreturn=10;						//下次必须100ms以后才能再次进入
			system_task_return=1;
			if(gifdecoding)gifdecoding=0;		//不再播放gif
		}	 	 
		if((t%60)==0)//900ms左右检测1次
 		{ 
			//SD卡在位检测
			if((DCMI->CR&0X01)==0)//摄像头不工作的时候,才可以查询SD卡
			{
				OS_ENTER_CRITICAL();//进入临界区(无法被中断打断) 
				res=SD_GetState();	//查询SD卡状态
				OS_EXIT_CRITICAL();	//退出临界区(可以被中断打断) 
				if(res==0XFF)
				{
					gui_phy.memdevflag&=~(1<<0);//标记SD卡不在位 
					OS_ENTER_CRITICAL();//进入临界区(无法被中断打断) 
					SD_Init();			//重新检测SD卡 
					OS_EXIT_CRITICAL();	//退出临界区(可以被中断打断) 
				}else if((gui_phy.memdevflag&(1<<0))==0)//SD不在位?
				{
					f_mount(fs[0],"0:",1);		//重新挂载sd卡
					gui_phy.memdevflag|=1<<0;	//标记SD卡在位了		
				} 
			}
			//U盘在位检测   
			if(usbx.hdevclass==1)
			{
				if((gui_phy.memdevflag&(1<<3))==0)//U盘不在位?
				{
					fs[3]->drv=3;
					f_mount(fs[3],"3:",1);		//重新挂载U盘
					gui_phy.memdevflag|=1<<3;	//标记U盘在位	
				}  
			}else gui_phy.memdevflag&=~(1<<3);	//U盘被拔出了  
 			//gsm检测
 			gsm_status_check();  
 		}  
 		gsm_cmsgin_check();					//来电/短信 监测
		if(usbx.mode==USBH_MSC_MODE)			//U盘模式,才处理
		{
			while((usbx.bDeviceState&0XC0)==0X40)//USB设备插入了,但是还没连接成功,猛查询.
			{
				usbapp_pulling();	//轮询处理USB事务
				delay_us(1000);		//不能像HID那么猛...,U盘比较慢
			}
			usbapp_pulling();//处理USB事务
		}
		delay_ms(10);
	}
}
 
//硬件错误处理
void HardFault_Handler(void)
{
	u32 i;
	u8 t=0;
	u32 temp;
	temp=SCB->CFSR;					//fault状态寄存器(@0XE000ED28)包括:MMSR,BFSR,UFSR
 	printf("CFSR:%8X\r\n",temp);	//显示错误值
	temp=SCB->HFSR;					//硬件fault状态寄存器
 	printf("HFSR:%8X\r\n",temp);	//显示错误值
 	temp=SCB->DFSR;					//调试fault状态寄存器
 	printf("DFSR:%8X\r\n",temp);	//显示错误值
   	temp=SCB->AFSR;					//辅助fault状态寄存器
 	printf("AFSR:%8X\r\n",temp);	//显示错误值
 	LED1=!LED1;
 	while(t<5)
	{
		t++;
		LED0=!LED0;
		//BEEP=!BEEP;
		for(i=0;i<0X1FFFFF;i++);	
 	}
}






