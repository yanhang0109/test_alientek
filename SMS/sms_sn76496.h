#ifndef __SMS_SN76496_H
#define __SMS_SN76496_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//��������ֲ������ye781205��NESģ��������
//ALIENTEK STM32������   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/10/12
//�汾��V1.0  			  
////////////////////////////////////////////////////////////////////////////////// 	 

#define MAX_76496      4
#define MASTER_CLOCK   3579545
#define Sound_Rate     22050 //������
#define SNBUF_size     Sound_Rate/60   //ÿ֡������		// 44100/60=735		22050/60=367
#define	sms_Frame      2			//֡��Ƶ���ݻ�����Ŀ
 

typedef struct 
{
	int  Channel;
	int  SampleRate; 
	int  VolTable[16];
	int  Register[8];  
	int  Volume[4];
	int  LastRegister;
	int  NoiseFB;
	int  Period[4];
	int  Count[4];
	int  Output[4];
	unsigned int RNG_A;
	unsigned int UpdateStep;
} t_SN76496;
extern t_SN76496 *SN76496;
extern u16 * psg_buffer;		//��Ƶ���ݻ���,��СΪ:SNBUF_size*2�ֽ�

void SN76496Write(int data);
void SN76496Update(short *buffer,int length,unsigned char mask);
void SN76496_set_clock(int clock);
void SN76496_set_gain(int gain);
int  SN76496_init(int clock,int volume,int sample_rate);

u8 sms_audio_init(void);
void sms_update_Sound(void);
#endif
