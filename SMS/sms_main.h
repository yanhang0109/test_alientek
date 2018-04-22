#ifndef __SMS_MAIN_H
#define __SMS_MAIN_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//��������ֲ������ye781205��SMSģ��������
//ALIENTEK STM32������
//SMS������ ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/7/1
//�汾��V1.0  			  
////////////////////////////////////////////////////////////////////////////////// 	 
 

extern u8* SMS_romfile;//ROMָ��


void sms_sram_free(void);
u8 sms_sram_malloc(u32 romsize);
void sms_set_window(void);

void sms_start(u8 bank_mun);
void sms_update_pad(void);
u8 sms_load(u8* pname);

void sms_i2s_dma_tx_callback(void);
void sms_sound_open(int sample_rate);
void sms_sound_close(void);
void sms_apu_fill_buffer(int samples,u16* wavebuf);
 
#endif
