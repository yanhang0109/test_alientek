#ifndef __GYROSCOPE_H
#define __GYROSCOPE_H 	
#include "common.h"  
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//APP-陀螺仪测试 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/7/20
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   
extern u8*const gyro_remind_tbl[2][GUI_LANGUAGE_NUM];

typedef int32_t	s32;
typedef int16_t s16;
typedef int8_t  s8;

u8 gyro_play(void); 
#endif



