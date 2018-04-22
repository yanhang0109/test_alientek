#include "gt928.h"
#include "touch.h"
#include "ctiic.h"
#include "usart.h"
#include "delay.h" 
#include "string.h" 
#include "lcd.h" 
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


//GT928���ò�����
//��һ���ֽ�Ϊ�汾��(0X53),���뱣֤�µİ汾�Ŵ��ڵ���GT928�ڲ�
//flashԭ�а汾��,�Ż��������.
const u8 GT928_CFG_TBL[]=
{ 
	0x53,0x00,0x04,0x58,0x02,0x0A,0xCD,0x00,0x02,0x2A,
    0x19,0x2F,0x56,0x46,0x03,0x05,0x00,0x00,0x00,0x00,
    0x00,0x00,0x06,0x18,0x1A,0x1E,0x14,0x90,0x30,0xCC,
    0x42,0x44,0x43,0x0B,0x00,0x00,0x00,0x01,0x03,0x25,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x28,0x62,0x94,0xC5,0x03,0x08,0x00,0x00,0x04,
	0x9F,0x2B,0x00,0x8F,0x34,0x00,0x83,0x3E,0x00,0x79,
	0x4B,0x00,0x73,0x59,0x00,0x73,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x01,0x04,0x05,0x06,0x07,0x08,0x09,
	0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x04,0x06,0x07,0x08,0x0A,0x0C,
	0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x19,0x1B,
	0x1C,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
	0x27,0x28,0x29,0x2A,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0xDD,0x01
};
//����GT928���ò���
//mode:0,���������浽flash
//     1,�������浽flash
u8 GT928_Send_Cfg(u8 mode)
{
	u8 buf[2];
	u8 i=0;
	buf[0]=0;
	buf[1]=mode;	//�Ƿ�д�뵽GT928 FLASH?  ���Ƿ���籣��
	for(i=0;i<sizeof(GT928_CFG_TBL);i++)buf[0]+=GT928_CFG_TBL[i];//����У���
    buf[0]=(~buf[0])+1;
	GT928_WR_Reg(GT_CFGS_REG,(u8*)GT928_CFG_TBL,sizeof(GT928_CFG_TBL));//���ͼĴ�������
	GT928_WR_Reg(GT_CHECK_REG,buf,2);//д��У���,�����ø��±��
	return 0;
}

//��GT928д��һ������
//reg:��ʼ�Ĵ�����ַ
//buf:���ݻ�������
//len:д���ݳ���
//����ֵ:0,�ɹ�;1,ʧ��.
u8 GT928_WR_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 i;
	u8 ret=0;
	CT_IIC_Start();	
 	CT_IIC_Send_Byte(GT_CMD_WR);   	//����д���� 	 
	CT_IIC_Wait_Ack();
	CT_IIC_Send_Byte(reg>>8);   	//���͸�8λ��ַ
	CT_IIC_Wait_Ack(); 	 										  		   
	CT_IIC_Send_Byte(reg&0XFF);   	//���͵�8λ��ַ
	CT_IIC_Wait_Ack();  
	for(i=0;i<len;i++)
	{	   
    	CT_IIC_Send_Byte(buf[i]);  	//������
		ret=CT_IIC_Wait_Ack();
		if(ret)break;  
	}
    CT_IIC_Stop();					//����һ��ֹͣ����	    
	return ret; 
}

//��GT928����һ������
//reg:��ʼ�Ĵ�����ַ
//buf:���ݻ�������
//len:�����ݳ���			  
void GT928_RD_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 i; 
 	CT_IIC_Start();	
 	CT_IIC_Send_Byte(GT_CMD_WR);   //����д���� 	 
	CT_IIC_Wait_Ack();
 	CT_IIC_Send_Byte(reg>>8);   	//���͸�8λ��ַ
	CT_IIC_Wait_Ack(); 	 										  		   
 	CT_IIC_Send_Byte(reg&0XFF);   	//���͵�8λ��ַ
	CT_IIC_Wait_Ack();  
 	CT_IIC_Start();  	 	   
	CT_IIC_Send_Byte(GT_CMD_RD);   //���Ͷ�����		   
	CT_IIC_Wait_Ack();	   
	for(i=0;i<len;i++)
	{	   
    	buf[i]=CT_IIC_Read_Byte(i==(len-1)?0:1); //������	  
	} 
    CT_IIC_Stop();//����һ��ֹͣ����    
} 

//��ʼ��GT928������
//����ֵ:0,��ʼ���ɹ�;1,��ʼ��ʧ�� 
u8 GT928_Init(void)
{
	u8 temp[5]; 
	RCC->AHB1ENR|=1<<7;    	//ʹ��PORTHʱ�� 
	RCC->AHB1ENR|=1<<8;    	//ʹ��PORTIʱ��  
	GPIO_Set(GPIOH,PIN7,GPIO_MODE_IN,0,0,GPIO_PUPD_PU);	//PH7����Ϊ��������
	GPIO_Set(GPIOI,PIN8,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);//PI8����Ϊ�������
	CT_IIC_Init();      	//��ʼ����������I2C����  
	GT928_RST=0;			//��λ
	delay_ms(10);
 	GT928_RST=1;			//�ͷŸ�λ		    
	delay_ms(10); 
	GPIO_Set(GPIOH,PIN7,GPIO_MODE_IN,0,0,GPIO_PUPD_NONE);//PH7����Ϊ��������
	delay_ms(100);  
	GT928_RD_Reg(GT928_PID_REG,temp,4);//��ȡ��ƷID
	temp[4]=0;
	printf("CTP ID:%s\r\n",temp);	//��ӡID
	if(strcmp((char*)temp,"928")==0)//ID==928
	{
		temp[0]=0X02;			
		GT928_WR_Reg(GT928_CTRL_REG,temp,1);//��λGT928
		GT928_RD_Reg(GT928_CFGS_REG,temp,1);//��ȡGT_CFGS_REG�Ĵ���
		if(temp[0]<0X53)//Ĭ�ϰ汾�Ƚϵ�,��Ҫ����flash����
		{
			printf("Default Ver:%d\r\n",temp[0]);
			if(lcddev.id==0X1016) GT928_Send_Cfg(1);//���²���������
		}
        delay_ms(10);
		temp[0]=0X00;	 
		GT928_WR_Reg(GT_CTRL_REG,temp,1);//������λ 
		return 0;
	} 
	return 1;
}

const u16 GT928_TPX_TBL[10]={GT928_TP1_REG,GT928_TP2_REG,GT928_TP3_REG,GT928_TP4_REG,GT928_TP5_REG,
                             GT928_TP6_REG,GT928_TP7_REG,GT928_TP8_REG,GT928_TP9_REG,GT928_TP10_REG};
//ɨ�败����(���ò�ѯ��ʽ)
//mode:0,����ɨ��.
//����ֵ:��ǰ����״̬.
//0,�����޴���;1,�����д���
u8 GT928_Scan(u8 mode)
{
	u8 buf[4];
	u8 i=0;
	u8 res=0;
	u16 temp;
	u16 tempsta;
 	static u8 t=0;//���Ʋ�ѯ���,�Ӷ�����CPUռ����   
	t++;
	if((t%10)==0||t<10)//����ʱ,ÿ����10��CTP_Scan�����ż��1��,�Ӷ���ʡCPUʹ����
	{
		GT928_RD_Reg(GT928_GSTID_REG,&mode,1);	//��ȡ�������״̬  
 		if(mode&0X80&&((mode&0XF)<11))
		{
			temp=0;
			GT928_WR_Reg(GT928_GSTID_REG,(u8*)&temp,1);//���־ 		
		}		
		if((mode&0XF)&&((mode&0XF)<11))
		{
			temp=0XFFFF<<(mode&0XF);		//����ĸ���ת��Ϊ1��λ��,ƥ��tp_dev.sta���� 
			tempsta=tp_dev.sta;				//���浱ǰ��tp_dev.staֵ
			tp_dev.sta=(~temp)|TP_PRES_DOWN|TP_CATH_PRES; 
			tp_dev.x[CT_MAX_TOUCH-1]=tp_dev.x[0];	//���津��0������
			tp_dev.y[CT_MAX_TOUCH-1]=tp_dev.y[0];
			for(i=0;i<CT_MAX_TOUCH;i++)
			{  
				if(tp_dev.sta&(1<<i))	//������Ч?
				{
					GT928_RD_Reg(GT928_TPX_TBL[i],buf,4);	//��ȡXY����ֵ 
					if(tp_dev.touchtype&0X01)//����
					{
						tp_dev.x[i]=((u16)buf[1]<<8)+buf[0];
						tp_dev.y[i]=(((u16)buf[3]<<8)+buf[2]);
					}else//����
					{
						//����ԭ�������Ͻ�
						tp_dev.y[i]=(((u16)buf[1]<<8)+buf[0]);
						tp_dev.x[i]=600-(((u16)buf[3]<<8)+buf[2]); 
					}  
					//printf("x[%d]:%d,y[%d]:%d\r\n",i,tp_dev.x[i],i,tp_dev.y[i]); 
				}			
			} 
			res=1;
		    if(tp_dev.x[0]>lcddev.width||tp_dev.y[0]>lcddev.height)//�Ƿ�����(���곬����)
			{ 
				//printf("tp_dev.y[0] = %d\r\n",tp_dev.y[0]);
				if((mode&0XF)>1)		//��������������,�򸴵ڶ�����������ݵ���һ������.
				{
					tp_dev.x[0]=tp_dev.x[1];
					tp_dev.y[0]=tp_dev.y[1];
					t=0;				//����һ��,��������������10��,�Ӷ����������
				}else					//�Ƿ�����,����Դ˴�����(��ԭԭ����)  
				{
					tp_dev.x[0]=tp_dev.x[CT_MAX_TOUCH-1];
					tp_dev.y[0]=tp_dev.y[CT_MAX_TOUCH-1];
					mode=0X80;		
					tp_dev.sta=tempsta;	//�ָ�tp_dev.sta
				}
			}else t=0;					//����һ��,��������������10��,�Ӷ����������
		}
	}
	if((mode&0X8F)==0X80)//�޴����㰴��
	{ 
		if(tp_dev.sta&TP_PRES_DOWN)		//֮ǰ�Ǳ����µ�
		{
			tp_dev.sta&=~TP_PRES_DOWN;	//��ǰ����ɿ�
		}else							//֮ǰ��û�б�����
		{ 
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
			tp_dev.sta&=(TP_PRES_DOWN|TP_CATH_PRES);//�������Ч���	
		}	 
	} 	
	if(t>240)t=10;//���´�10��ʼ����
	return res;
}							 
							 
		 
		 
		 
		 
