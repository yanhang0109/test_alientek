#ifndef __GT928_H
#define __GT928_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//10.1寸电容触摸屏-GT928 驱动代码(支持10点触摸)	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/8/12
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved		 
////////////////////////////////////////////////////////////////////////////////// 


//IO操作函数	 
#define GT928_RST			PIout(8)	//GT9147复位引脚
#define GT928_INT			PHin(7)		//GT9147中断引脚	

//I2C读写命令	
#define GT928_CMD_WR 		0X28    	//写命令  (这里需要改)
#define GT928_CMD_RD 		0X29		//读命令  (这里需要改)

//GT928 部分寄存器定	
#define GT928_CTRL_REG		0X8040   	//GT928控制寄存器
#define GT928_CFGS_REG		0X8047   	//GT928配置起始地址寄存器
#define GT928_CHECK_REG		0X80FF   	//GT928校验和寄存器
#define GT928_PID_REG 		0X8140   	//GT928产品ID寄存器

#define GT928_GSTID_REG		0X814E   	//GT928当前检测到的触摸情况
#define GT928_TP1_REG		0X8150      //第一个触摸点数据地址
#define GT928_TP2_REG		0X8158      //第二个触摸点数据地址
#define GT928_TP3_REG		0X8160      //第三个触摸点数据地址
#define GT928_TP4_REG		0X8168      //第四个触摸点数据地址
#define GT928_TP5_REG		0X8170      //第五个触摸点数据地址
#define GT928_TP6_REG		0X8178      //第六个触摸点数据地址
#define GT928_TP7_REG		0X8180      //第七个触摸点数据地址
#define GT928_TP8_REG		0X8188      //第八个触摸点数据地址
#define GT928_TP9_REG		0X8190      //第九个触摸点数据地址
#define GT928_TP10_REG		0X8198      //第十个触摸点数据地址


u8 GT928_Send_Cfg(u8 mode);
u8 GT928_WR_Reg(u16 reg,u8 *buf,u8 len);
void GT928_RD_Reg(u16 reg,u8 *buf,u8 len); 
u8 GT928_Init(void);
u8 GT928_Scan(u8 mode); 
#endif






















