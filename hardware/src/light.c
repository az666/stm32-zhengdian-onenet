/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	light.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-04-06
	*
	*	版本： 		V1.0
	*
	*	说明： 		光敏电阻
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//硬件驱动
#include "light.h"
#include "adc.h"

#define DEBUG_LIGHT_ENABLE 0

LIGHT_INFO lightInfo;


/*
************************************************************
*	函数名称：	LIGHT_Init
*
*	函数功能：	光敏电阻初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
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
*	函数名称：	LIGHT_GetVoltag
*
*	函数功能：	获取光敏电阻
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void LIGHT_GetVoltag(void)
{
#if DEBUG_LIGHT_ENABLE
	lightInfo.voltag = ADCx_GetVoltag(ADC1, ADC_Channel_13, 10);
#endif
}
