/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	light.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2017-04-06
	*
	*	�汾�� 		V1.0
	*
	*	˵���� 		��������
	*
	*	�޸ļ�¼��	
	************************************************************
	************************************************************
	************************************************************
**/

//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//Ӳ������
#include "light.h"
#include "adc.h"

#define DEBUG_LIGHT_ENABLE 0

LIGHT_INFO lightInfo;


/*
************************************************************
*	�������ƣ�	LIGHT_Init
*
*	�������ܣ�	���������ʼ��
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void LIGHT_Init(void)
{
#if DEBUG_LIGHT_ENABLE
	GPIO_InitTypeDef gpioInitStrcut;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	gpioInitStrcut.GPIO_Mode = GPIO_Mode_AIN;
	gpioInitStrcut.GPIO_Pin = GPIO_Pin_3;
	gpioInitStrcut.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOC, &gpioInitStrcut);
	
	ADCx_Init(ADC1, 0);
#endif
}

/*
************************************************************
*	�������ƣ�	LIGHT_GetVoltag
*
*	�������ܣ�	��ȡ��������
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void LIGHT_GetVoltag(void)
{
#if DEBUG_LIGHT_ENABLE
	lightInfo.voltag = ADCx_GetVoltag(ADC1, ADC_Channel_13, 10);
#endif
}
