#include "beepplay.h" 
#include "gradienter.h" 
#include "pcf8574.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//APP-���������� ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2016/6/27
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//*******************************************************************************
//�޸���Ϣ
//��
////////////////////////////////////////////////////////////////////////////////// 	   
 

//BEEP��ť����
u8*const beep_btncaption_tbl[2][GUI_LANGUAGE_NUM]=
{ 
{"��","���_","ON",},
{"�ر�","�P�]","OFF",},  
};

//����������
u8 beep_play(void)
{
	
	u8 res,rval=0;   
	u8 *buf; 
	u8 beepsta=0;
	
 	_btn_obj* beepbtn=0;		//���ư�ť
	u16 btnx,btny,btnw,btnh;	//�����������
	u16 cx,cy,cr;				//Բ�������
	u8 btnfsize;				//�����С
	u8 fsize=0;					//ON/OFF�����С
 
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
	buf=gui_memin_malloc(32);	//�����ڴ�  
	if(buf&&beepbtn)rval=grad_load_font();//��������
	else rval=1;
	if(rval==0) 
	{  
		LCD_Clear(LGRAY);
		app_gui_tcbar(0,0,lcddev.width,gui_phy.tbheight,0x02);			//�·ֽ���	 
		gui_show_strmid(0,0,lcddev.width,gui_phy.tbheight,WHITE,gui_phy.tbfsize,(u8*)APP_MFUNS_CAPTION_TBL[21][gui_phy.language]);//��ʾ����  
 	
		beepbtn->caption=beep_btncaption_tbl[0][gui_phy.language];
		beepbtn->font=btnfsize;
		
		btn_draw(beepbtn);		//����ť
		beepbtn->caption=beep_btncaption_tbl[1][gui_phy.language];
		
		gui_fill_circle(cx,cy,cr,RED);
		PCF8574_WriteBit(BEEP_IO,1);//�رշ�����
		sprintf((char*)buf,"OFF");
		gui_show_strmid(0,cy-(fsize/2),lcddev.width,fsize,WHITE,fsize,buf);	//��ʾ�½Ƕ� 
		system_task_return=0;
		while(1)
		{ 
			tp_dev.scan(0);    
			in_obj.get_key(&tp_dev,IN_TYPE_TOUCH);	//�õ�������ֵ   
			res=btn_check(beepbtn,&in_obj);   
			if(res&&((beepbtn->sta&(1<<7))==0)&&(beepbtn->sta&(1<<6)))//������,�а����������ɿ�,����TP�ɿ���
			{   
				beepbtn->caption=beep_btncaption_tbl[beepsta][gui_phy.language]; 
				beepsta=!beepsta;
				if(beepsta)
				{
					sprintf((char*)buf,"ON");
					PCF8574_WriteBit(BEEP_IO,0);//�򿪷�����
				}else 
				{
					sprintf((char*)buf,"OFF");
					PCF8574_WriteBit(BEEP_IO,1);//�رշ�����
				}
				gui_fill_circle(cx,cy,cr,RED);
				gui_show_strmid(0,cy-(fsize/2),lcddev.width,fsize,WHITE,fsize,buf);	//��ʾ�½Ƕ� 
			}	 
			if(system_task_return)break;		//TPAD����  
			delay_ms(3);
		} 
		mlcd_delete();							//ɾ��mlcd 
	}
	PCF8574_WriteBit(BEEP_IO,1);//�رշ�����
	btn_delete(beepbtn);//ɾ����ť
	grad_delete_font();	//ɾ������
	gui_memin_free(buf);//�ͷ��ڴ�
	return rval;
}







































