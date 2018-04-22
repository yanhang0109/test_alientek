#ifndef __GT928_H
#define __GT928_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//10.1����ݴ�����-GT928 ��������(֧��10�㴥��)	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2016/8/12
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved		 
////////////////////////////////////////////////////////////////////////////////// 


//IO��������	 
#define GT928_RST			PIout(8)	//GT9147��λ����
#define GT928_INT			PHin(7)		//GT9147�ж�����	

//I2C��д����	
#define GT928_CMD_WR 		0X28    	//д����  (������Ҫ��)
#define GT928_CMD_RD 		0X29		//������  (������Ҫ��)

//GT928 ���ּĴ�����	
#define GT928_CTRL_REG		0X8040   	//GT928���ƼĴ���
#define GT928_CFGS_REG		0X8047   	//GT928������ʼ��ַ�Ĵ���
#define GT928_CHECK_REG		0X80FF   	//GT928У��ͼĴ���
#define GT928_PID_REG 		0X8140   	//GT928��ƷID�Ĵ���

#define GT928_GSTID_REG		0X814E   	//GT928��ǰ��⵽�Ĵ������
#define GT928_TP1_REG		0X8150      //��һ�����������ݵ�ַ
#define GT928_TP2_REG		0X8158      //�ڶ������������ݵ�ַ
#define GT928_TP3_REG		0X8160      //���������������ݵ�ַ
#define GT928_TP4_REG		0X8168      //���ĸ����������ݵ�ַ
#define GT928_TP5_REG		0X8170      //��������������ݵ�ַ
#define GT928_TP6_REG		0X8178      //���������������ݵ�ַ
#define GT928_TP7_REG		0X8180      //���߸����������ݵ�ַ
#define GT928_TP8_REG		0X8188      //�ڰ˸����������ݵ�ַ
#define GT928_TP9_REG		0X8190      //�ھŸ����������ݵ�ַ
#define GT928_TP10_REG		0X8198      //��ʮ�����������ݵ�ַ


u8 GT928_Send_Cfg(u8 mode);
u8 GT928_WR_Reg(u16 reg,u8 *buf,u8 len);
void GT928_RD_Reg(u16 reg,u8 *buf,u8 len); 
u8 GT928_Init(void);
u8 GT928_Scan(u8 mode); 
#endif






















