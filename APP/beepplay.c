#include "beepplay.h" 
#include "gradienter.h" 
#include "pcf8574.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//APP-蜂鸣器测试 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/6/27
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   
 

//BEEP按钮标题
u8*const beep_btncaption_tbl[2][GUI_LANGUAGE_NUM]=
{ 
{"打开","打開","ON",},
{"关闭","關閉","OFF",},  
};

//蜂鸣器测试
u8 beep_play(void)
{
	
	u8 res,rval=0;   
	u8 *buf; 
	u8 beepsta=0;
	
 	_btn_obj* beepbtn=0;		//控制按钮
	u16 btnx,btny,btnw,btnh;	//按键坐标参数
	u16 cx,cy,cr;				//圆坐标参数
	u8 btnfsize;				//字体大小
	u8 fsize=0;					//ON/OFF字体大小
 
	if(lcddev.width>320)btnfsize=32;
	else btnfsize=24; 
	btnw=lcddev.width*2/3;
	btnh=btnw/4;
	btnx=(lcddev.width-btnw)/2;
	btny=lcddev.height-2*btnh;
	
	if(lcddev.width<=272)
	{
		fsize=72;	
	}else if(lcddev.width==320)
	{
		fsize=88;	
	}else if(lcddev.width>=480)
	{
		fsize=144;	
	}
	cx=lcddev.width/2;
	cy=cx+gui_phy.tbheight;
	cr=lcddev.width/3; 
	beepbtn=btn_creat(btnx,btny,btnw,btnh,0,0);
	buf=gui_memin_malloc(32);	//申请内存  
	if(buf&&beepbtn)rval=grad_load_font();//加载字体
	else rval=1;
	if(rval==0) 
	{  
		LCD_Clear(LGRAY);
		app_gui_tcbar(0,0,lcddev.width,gui_phy.tbheight,0x02);			//下分界线	 
		gui_show_strmid(0,0,lcddev.width,gui_phy.tbheight,WHITE,gui_phy.tbfsize,(u8*)APP_MFUNS_CAPTION_TBL[21][gui_phy.language]);//显示标题  
 	
		beepbtn->caption=beep_btncaption_tbl[0][gui_phy.language];
		beepbtn->font=btnfsize;
		
		btn_draw(beepbtn);		//画按钮
		beepbtn->caption=beep_btncaption_tbl[1][gui_phy.language];
		
		gui_fill_circle(cx,cy,cr,RED);
		PCF8574_WriteBit(BEEP_IO,1);//关闭蜂鸣器
		sprintf((char*)buf,"OFF");
		gui_show_strmid(0,cy-(fsize/2),lcddev.width,fsize,WHITE,fsize,buf);	//显示新角度 
		system_task_return=0;
		while(1)
		{ 
			tp_dev.scan(0);    
			in_obj.get_key(&tp_dev,IN_TYPE_TOUCH);	//得到按键键值   
			res=btn_check(beepbtn,&in_obj);   
			if(res&&((beepbtn->sta&(1<<7))==0)&&(beepbtn->sta&(1<<6)))//有输入,有按键按下且松开,并且TP松开了
			{   
				beepbtn->caption=beep_btncaption_tbl[beepsta][gui_phy.language]; 
				beepsta=!beepsta;
				if(beepsta)
				{
					sprintf((char*)buf,"ON");
					PCF8574_WriteBit(BEEP_IO,0);//打开蜂鸣器
				}else 
				{
					sprintf((char*)buf,"OFF");
					PCF8574_WriteBit(BEEP_IO,1);//关闭蜂鸣器
				}
				gui_fill_circle(cx,cy,cr,RED);
				gui_show_strmid(0,cy-(fsize/2),lcddev.width,fsize,WHITE,fsize,buf);	//显示新角度 
			}	 
			if(system_task_return)break;		//TPAD返回  
			delay_ms(3);
		} 
		mlcd_delete();							//删除mlcd 
	}
	PCF8574_WriteBit(BEEP_IO,1);//关闭蜂鸣器
	btn_delete(beepbtn);//删除按钮
	grad_delete_font();	//删除字体
	gui_memin_free(buf);//释放内存
	return rval;
}







































