#include "atk_frec.h" 
#include "malloc.h"	
#include "ff.h"
#include "stdio.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//������ʶ�������ALIENTEK�ṩ,�����ṩ2��LIB(��:ATKFREC_DF.lib��ATKFREC_SF.lib),�����ʹ�� 
//ATKFREC_DF.lib:�ʺϾ���˫����Ӳ��������㵥Ԫ��STM32F7ϵ�У�����STM32F767/769�� 
//ATKFREC_SF.lib:�ʺϾ��е�����Ӳ��������㵥Ԫ��STM32F4/F7ϵ�У�����STM32F407/429/746��
//��ѡ����ʵ�lib���в��ԣ�

//����:��������ͷ,ʵ������ʶ��.
//˵��:��ʶ���,��Ҫ�õ��ڴ����,�ڴ���ռ������560KB����(20������).ÿ����һ������,�ڴ�ռ����10KB����.
//����:���ڱ�ʶ�����M3/M4/M7ΪĿ�괦����,�ڴ�����,�㷨�Ͻ����˴����˸�,����,�ܶ๦�ܲ�̫����,Ч��Ҳ��
//     �Ǻܺ�.��û����ʶ����Ч���(������������,Ҳ���н�����).����,�δ���,������Ҳο���.
//
//��������:
//1,����ͷģ��һ��.
//2,SD��һ��
//
//ʹ�÷���:					   
//��һ��:����atk_frec_initialization����,��ʼ������ʶ���
//�ڶ���:����atk_frec_add_a_face����,�������ģ��(����Ѿ�����,���Ժ��Դβ�)
//������:����atk_frec_load_data_model����,��������ģ�嵽�ڴ�����(���������ģ�����Ҫ,��û�������ģ��,��ɺ��Դ˲�)
//���Ĳ�:����atk_frec_recognition_face����,��ȡʶ����.
//���岽:����atk_frec_delete_data����,����ɾ��һ������ģ��
//������:�����������ʶ���,�����atk_frec_destroy����,�ͷ������ڴ�,��������ʶ��.
//
//�汾:V1.1
//����ԭ��@ALIENTEK
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved													    								  
//��Ȩ���У�����ؾ���
//****************************************************************************************************************
//����˵��
//V1.1 20160830
//1,��Ե����Ⱥ�˫����Ӳ�����㣬��Ϊ2��lib��ATKFREC_DF.lib��ATKFREC_SF.lib��.
//2,����atk_frec_fastmalloc��atk_frec_fastfree����
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//�ڴ����ú���
void atk_frec_memset(char *p,char c,unsigned long len) 
{
	mymemset((u8*)p,(u8)c,(u32)len);
}	 					  
//�ڴ����뺯��
void *atk_frec_malloc(unsigned int size) 
{
	return mymalloc(SRAMEX,size);
}
//�ڴ��ͷź���
void atk_frec_free(void *ptr) 
{
	myfree(SRAMEX,ptr);
}
//���ٷ����ڴ����뺯��
void *atk_frec_fastmalloc(unsigned int size) 
{
	return mymalloc(SRAMCCM,size);
}
//���ٷ����ڴ��ͷź���
void atk_frec_fastfree(void *ptr) 
{
	myfree(SRAMCCM,ptr);
}

//��������ʶ�����������
//index:Ҫ���������λ��(һ����ռһ��λ��),��Χ:0~MAX_LEBEL_NUM-1
//buf:Ҫ��������ݻ������׵�ַ
//size:Ҫ��������ݴ�С
//����ֵ:0,����
//    ����,�������
u8 atk_frec_save_data(u8 index,u8* buf,u32 size)
{
	u8* path;
	FIL *fp; 
  	DIR fdir;		
	u32 fw;
	u8 res;
	path=atk_frec_fastmalloc(30);			//�����ڴ�
	fp=atk_frec_fastmalloc(sizeof(FIL));	//�����ڴ�
	if(!fp)
	{
		atk_frec_fastfree(path);
		return ATK_FREC_MEMORY_ERR;
	}
	sprintf((char*)path,ATK_FREC_DATA_PNAME,index);
	res=f_opendir(&fdir,(const TCHAR*)ATK_FREC_DATA_PDIR); //���Դ�ATK_FREC_DATA_PDIRĿ¼
	if(res)									//��ʧ��
	{
		f_mkdir(ATK_FREC_DATA_PDIR);		//�����ļ���
	}else
	{
		f_closedir(&fdir);					//�ر�dir
	}
	res=f_open(fp,(char*)path,FA_WRITE|FA_CREATE_NEW);
	if(res==FR_OK)
	{
		res=f_write(fp,buf,size,&fw);		//д���ļ�		
	}
	f_close(fp);
	if(res)res=ATK_FREC_READ_WRITE_ERR;
	atk_frec_fastfree(path);
	atk_frec_fastfree(fp);
	return res;	
}
//��ȡ����ʶ�����������
//index:Ҫ��ȡ������λ��(һ����ռһ��λ��),��Χ:0~MAX_LEBEL_NUM-1
//buf:Ҫ��ȡ�����ݻ������׵�ַ
//size:Ҫ��ȡ�����ݴ�С(size=0,���ʾ����Ҫ�����ݳ���)
//����ֵ:0,����
//    ����,�������
u8 atk_frec_read_data(u8 index,u8* buf,u32 size)
{
	u8* path;
	FIL *fp; 
	u32 fr;
	u8 res;
	path=atk_frec_fastmalloc(30);			//�����ڴ�
	fp=atk_frec_fastmalloc(sizeof(FIL));	//�����ڴ�
	if(!fp)
	{
		atk_frec_fastfree(path);
		return ATK_FREC_MEMORY_ERR;
	}
	sprintf((char*)path,ATK_FREC_DATA_PNAME,index); 
	res=f_open(fp,(char*)path,FA_READ);
	if(res==FR_OK&&size)
	{
		res=f_read(fp,buf,size,&fr);	//��ȡ�ļ�	
		if(fr==size)res=0;
		else res=ATK_FREC_READ_WRITE_ERR;
	} 
	f_close(fp);
	if(res)res=ATK_FREC_READ_WRITE_ERR;
	atk_frec_fastfree(path);
	atk_frec_fastfree(fp);
	return res;	
}
 
//ɾ��һ����������
//index:Ҫ���������λ��(һ����ռһ��λ��),��Χ:0~MAX_LEBEL_NUM-1  
//����ֵ:0,����
//    ����,�������
u8 atk_frec_delete_data(u8 index)
{
	u8* path;  
	u8 res;
	path=atk_frec_fastmalloc(30);			//�����ڴ� 
	if(!path)
	{ 
		return ATK_FREC_MEMORY_ERR;
	}
	sprintf((char*)path,ATK_FREC_DATA_PNAME,index); 
	res=f_unlink((char*)path);  
	atk_frec_fastfree(path);
	return res;	
}




 
