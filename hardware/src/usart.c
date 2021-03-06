/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	usart.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-06-25
	*
	*	版本： 		V1.2
	*
	*	说明： 		单片机串口外设初始化，格式化打印
	*
	*	修改记录：	V1.1：增加DMA发送功能
	*				V1.2：增加DMA接收功能、IDLE中断
	************************************************************
	************************************************************
	************************************************************
**/

//硬件驱动
#include "usart.h"
#include "delay.h"

//C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>


ALTER_INFO alterInfo;


#if(USART_DMA_TX_EN == 1)
unsigned char UsartPrintfBuf[128];
#endif


/*
************************************************************
*	函数名称：	Usart1_Init
*
*	函数功能：	串口1初始化
*
*	入口参数：	baud：设定的波特率
*
*	返回参数：	无
*
*	说明：		TX-PA9		RX-PA10
*				发送：DMA1_Channel4
*				接收：DMA1_Channel5
*				未使用DMA_TC中断，发现在RTOS版本中有可能会关闭总中断而导致死循环
************************************************************
*/
void Usart1_Init(unsigned int baud)
{

	GPIO_InitTypeDef gpioInitStruct;
	USART_InitTypeDef usartInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
#if(USART_DMA_TX_EN == 1 || USART_DMA_RX_EN == 1)
	DMA_InitTypeDef dmaInitStruct;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);								//使能DMA时钟
#endif
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	//PA9	TXD
	gpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioInitStruct.GPIO_Pin = GPIO_Pin_9;
	gpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInitStruct);
	
	//PA10	RXD
	gpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpioInitStruct.GPIO_Pin = GPIO_Pin_10;
	gpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInitStruct);
	
	usartInitStruct.USART_BaudRate = baud;
	usartInitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;		//无硬件流控
	usartInitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;						//接收和发送
	usartInitStruct.USART_Parity = USART_Parity_No;									//无校验
	usartInitStruct.USART_StopBits = USART_StopBits_1;								//1位停止位
	usartInitStruct.USART_WordLength = USART_WordLength_8b;							//8位数据位
	USART_Init(USART1, &usartInitStruct);
	
	USART_Cmd(USART1, ENABLE);														//使能串口
	
#if(USART_DMA_RX_EN == 0)
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);									//使能接收中断
#endif

	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);									//使能IDLE中断
	
	nvicInitStruct.NVIC_IRQChannel = USART1_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&nvicInitStruct);
	
#if(USART_DMA_TX_EN == 1)
	DMA_DeInit(DMA1_Channel4);														//将DMA的通道1寄存器重设为缺省值
	dmaInitStruct.DMA_PeripheralBaseAddr = (unsigned int)&USART1->DR;				//DMA外设基地址
	dmaInitStruct.DMA_MemoryBaseAddr = NULL;										//DMA内存基地址
	dmaInitStruct.DMA_DIR = DMA_DIR_PeripheralDST;									//数据传输方向，从内存读取发送到外设
	dmaInitStruct.DMA_BufferSize = 0;												//DMA通道的DMA缓存的大小
	dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//外设地址寄存器不变
	dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;								//内存地址寄存器递增
	dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;				//外设数据宽度为8位
	dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;						//内存数据宽度为8位
	dmaInitStruct.DMA_Mode = DMA_Mode_Normal;										//工作在正常缓存模式
	dmaInitStruct.DMA_Priority = DMA_Priority_Medium;								//DMA通道优先级
	dmaInitStruct.DMA_M2M = DMA_M2M_Disable;										//DMA通道没有设置为内存到内存传输
	DMA_Init(DMA1_Channel4, &dmaInitStruct);										//根据dmaInitStruct中指定的参数初始化DMA的通道USART1_Tx_DMA_Channel所标识的寄存器
	
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);									//使能USART1的DMA发送功能
	
	USARTx_ResetMemoryBaseAddr(USART1, (unsigned int)UsartPrintfBuf, 1, USART_TX_TYPE);//发送一次数据
#endif

#if(USART_DMA_RX_EN == 1)
	DMA_DeInit(DMA1_Channel5);														//将DMA的通道1寄存器重设为缺省值
	dmaInitStruct.DMA_PeripheralBaseAddr = (unsigned int)&USART1->DR;				//DMA外设基地址
	dmaInitStruct.DMA_MemoryBaseAddr = NULL;										//DMA内存基地址
	dmaInitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;									//数据传输方向，从外设读取发送到内存
	dmaInitStruct.DMA_BufferSize = 0;												//DMA通道的DMA缓存的大小
	dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//外设地址寄存器不变
	dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;								//内存地址寄存器递增
	dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;				//外设数据宽度为8位
	dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;						//内存数据宽度为8位
	dmaInitStruct.DMA_Mode = DMA_Mode_Normal;										//工作在正常缓存模式
	dmaInitStruct.DMA_Priority = DMA_Priority_Medium;								//DMA通道优先级
	dmaInitStruct.DMA_M2M = DMA_M2M_Disable;										//DMA通道没有设置为内存到内存传输
	DMA_Init(DMA1_Channel5, &dmaInitStruct);										//根据dmaInitStruct中指定的参数初始化DMA的通道USART1_Rx_DMA_Channel所标识的寄存器
	
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);									//使能USART1的DMA接收功能
#endif

}

/*
************************************************************
*	函数名称：	Usart3_Init
*
*	函数功能：	串口3初始化
*
*	入口参数：	baud：设定的波特率
*
*	返回参数：	无
*
*	说明：		TX-PB10		RX-PB11
*				发送：2 DMA1_Channel7    
*				接收：2 DMA1_Channel6   
USART3_TX：DMA1_Channel2
USART3_RX：DMA1_Channel3
*				未使用DMA_TC中断，发现在RTOS版本中有可能会关闭总中断而导致死循环
************************************************************
*/
void Usart3_Init(unsigned int baud)
{

	GPIO_InitTypeDef gpioInitStruct;
	USART_InitTypeDef usartInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
#if(USART_DMA_TX_EN == 1 || USART_DMA_RX_EN == 1)
	DMA_InitTypeDef dmaInitStruct;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);								//使能DMA时钟
#endif
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	USART_DeInit(USART3);  //复位串口3
	
	
	//USART3_TX   PB10
	gpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioInitStruct.GPIO_Pin = GPIO_Pin_10;
	gpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpioInitStruct);
	
	//USART3_RX	  PB11
	gpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpioInitStruct.GPIO_Pin = GPIO_Pin_11;
	gpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpioInitStruct);
	
	usartInitStruct.USART_BaudRate = baud;
	usartInitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;		//无硬件流控
	usartInitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;						//接收和发送
	usartInitStruct.USART_Parity = USART_Parity_No;									//无校验
	usartInitStruct.USART_StopBits = USART_StopBits_1;								//1位停止位
	usartInitStruct.USART_WordLength = USART_WordLength_8b;							//8位数据位
	USART_Init(USART3, &usartInitStruct);
	
	USART_Cmd(USART3, ENABLE);														//使能串口

#if(USART_DMA_RX_EN == 0)
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);									//使能接收中断
#endif

	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);									//使能IDLE中断
	
	nvicInitStruct.NVIC_IRQChannel = USART3_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);
	
#if(USART_DMA_TX_EN == 1)
	DMA_DeInit(DMA1_Channel2);														//将DMA的通道1寄存器重设为缺省值
	dmaInitStruct.DMA_PeripheralBaseAddr = (unsigned int)&USART3->DR;				//DMA外设基地址
	dmaInitStruct.DMA_MemoryBaseAddr = NULL;										//DMA内存基地址
	dmaInitStruct.DMA_DIR = DMA_DIR_PeripheralDST;									//数据传输方向，从内存读取发送到外设
	dmaInitStruct.DMA_BufferSize = 0;												//DMA通道的DMA缓存的大小
	dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//外设地址寄存器不变
	dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;								//内存地址寄存器递增
	dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;				//外设数据宽度为8位
	dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;						//内存数据宽度为8位
	dmaInitStruct.DMA_Mode = DMA_Mode_Normal;										//工作在正常缓存模式
	dmaInitStruct.DMA_Priority = DMA_Priority_Medium;								//DMA通道优先级
	dmaInitStruct.DMA_M2M = DMA_M2M_Disable;										//DMA通道没有设置为内存到内存传输
	DMA_Init(DMA1_Channel2, &dmaInitStruct);										//根据dmaInitStruct中指定的参数初始化DMA的通道USART3_Tx_DMA_Channel所标识的寄存器
	
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);									//使能USART3的DMA发送功能
	
	USARTx_ResetMemoryBaseAddr(USART3, (unsigned int)UsartPrintfBuf, 1, USART_TX_TYPE);//发送一次数据
#endif
	
#if(USART_DMA_RX_EN == 1)
	DMA_DeInit(DMA1_Channel3);														//将DMA的通道1寄存器重设为缺省值
	dmaInitStruct.DMA_PeripheralBaseAddr = (unsigned int)&USART3->DR;				//DMA外设基地址
	dmaInitStruct.DMA_MemoryBaseAddr = NULL;										//DMA内存基地址
	dmaInitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;									//数据传输方向，从外设读取发送到内存
	dmaInitStruct.DMA_BufferSize = 0;												//DMA通道的DMA缓存的大小
	dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					//外设地址寄存器不变
	dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;								//内存地址寄存器递增
	dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;				//外设数据宽度为8位
	dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;						//内存数据宽度为8位
	dmaInitStruct.DMA_Mode = DMA_Mode_Normal;										//工作在正常缓存模式
	dmaInitStruct.DMA_Priority = DMA_Priority_Medium;								//DMA通道优先级
	dmaInitStruct.DMA_M2M = DMA_M2M_Disable;										//DMA通道没有设置为内存到内存传输
	DMA_Init(DMA1_Channel3, &dmaInitStruct);										//根据dmaInitStruct中指定的参数初始化DMA的通道USART3_Rx_DMA_Channel所标识的寄存器
	
	nvicInitStruct.NVIC_IRQChannel = DMA1_Channel3_IRQn;							//配置USART3的DMA中断
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&nvicInitStruct);
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);									//开启接收完成中断

	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);									//使能USART3的DMA接收功能
#endif

}

/*
************************************************************
*	函数名称：	USARTx_ResetMemoryBaseAddr
*
*	函数功能：	重设DMA内存地址并使能发送
*
*	入口参数：	USARTx：串口组
*				mAddr：内存地址值
*				num：本次发送的数据长度(字节)
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void USARTx_ResetMemoryBaseAddr(USART_TypeDef *USARTx, unsigned int mAddr, unsigned short num, _Bool type)
{

#if(USART_DMA_TX_EN == 1)
	if(type == USART_TX_TYPE)
	{
		if(USARTx == USART1)
		{
			DMA_Cmd(DMA1_Channel4, DISABLE);				//关闭USART1 TX DMA1 所指示的通道
			
			DMA1_Channel4->CMAR = mAddr;					//DMA通道的内存地址
			DMA_SetCurrDataCounter(DMA1_Channel4, num);		//DMA通道的DMA缓存的大小
			
			DMA_Cmd(DMA1_Channel4, ENABLE);
		}
		else if(USARTx == USART3)
		{
			DMA_Cmd(DMA1_Channel2, DISABLE);				//关闭USART3 TX DMA1 所指示的通道
			
			DMA1_Channel2->CMAR = mAddr;					//DMA通道的内存地址
			DMA_SetCurrDataCounter(DMA1_Channel2, num);		//DMA通道的DMA缓存的大小
			
			DMA_Cmd(DMA1_Channel2, ENABLE);
		}
	}
#endif
	
#if(USART_DMA_RX_EN == 1)
	if(type == USART_RX_TYPE)
	{
		if(USARTx == USART1)
		{
			DMA_Cmd(DMA1_Channel5, DISABLE);				//关闭USART1 RX DMA1 所指示的通道
			
			DMA1_Channel5->CMAR = mAddr;					//DMA通道的内存地址
			DMA_SetCurrDataCounter(DMA1_Channel5, num);		//DMA通道的DMA缓存的大小
			
			DMA_Cmd(DMA1_Channel5, ENABLE);
		}
		else if(USARTx == USART3)
		{
			DMA_Cmd(DMA1_Channel3, DISABLE);				//关闭USART3 RX DMA1 所指示的通道
			
			DMA1_Channel3->CMAR = mAddr;					//DMA通道的内存地址
			DMA_SetCurrDataCounter(DMA1_Channel3, num);		//DMA通道的DMA缓存的大小
			
			DMA_Cmd(DMA1_Channel3, ENABLE);
		}
	}
#endif

}

/*
************************************************************
*	函数名称：	Usart_SendString
*
*	函数功能：	串口数据发送
*
*	入口参数：	USARTx：串口组
*				str：要发送的数据
*				len：数据长度
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len)
{

#if(USART_DMA_TX_EN == 0)
	unsigned short count = 0;
	
	for(; count < len; count++)
	{
		USART_SendData(USARTx, *str++);									//发送数据
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);		//等待发送完成
	}
#else
	unsigned int mAddr = (unsigned int)str;
	
	if(USARTx == USART1)
	{
		while(DMA_GetFlagStatus(DMA1_FLAG_TC4) == RESET);					//等待通道4传输完成
		DMA_ClearFlag(DMA1_FLAG_TC4);										//清除通道4传输完成标志
	}
	else if(USARTx == USART3)
	{
		while(DMA_GetFlagStatus(DMA1_FLAG_TC2) == RESET);					//等待通道7传输完成
		DMA_ClearFlag(DMA1_FLAG_TC2);										//清除通道7传输完成标志
	}
	
	USARTx_ResetMemoryBaseAddr(USARTx, mAddr, len, USART_TX_TYPE);
#endif

}

/*
************************************************************
*	函数名称：	UsartPrintf
*
*	函数功能：	格式化打印
*
*	入口参数：	USARTx：串口组
*				fmt：不定长参
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...)
{

	va_list ap;
	
#if(USART_DMA_TX_EN == 0)
	unsigned char UsartPrintfBuf[128];
#endif
	
	unsigned char *pStr = UsartPrintfBuf;
	
#if(USART_DMA_TX_EN == 1)
	if(USARTx == USART1)
	{
		while(DMA_GetFlagStatus(DMA1_FLAG_TC4) == RESET);					//等待通道4传输完成
		DMA_ClearFlag(DMA1_FLAG_TC4);										//清除通道4传输完成标志
	}
	else if(USARTx == USART3)
	{
		while(DMA_GetFlagStatus(DMA1_FLAG_TC2) == RESET);					//等待通道7传输完成
		DMA_ClearFlag(DMA1_FLAG_TC2);										//清除通道7传输完成标志
	}
	
	memset(UsartPrintfBuf, 0, sizeof(UsartPrintfBuf));					//清空buffer
#endif
	
	va_start(ap, fmt);
	vsprintf((char *)UsartPrintfBuf, fmt, ap);							//格式化
	va_end(ap);
	
#if(USART_DMA_TX_EN == 1)
	USARTx_ResetMemoryBaseAddr(USARTx, (unsigned int)UsartPrintfBuf,
							strlen((const char *)pStr), USART_TX_TYPE);
#else
	while(*pStr != 0)
	{
		USART_SendData(USARTx, *pStr++);
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
	}
#endif

}

/*
************************************************************
*	函数名称：	USART1_IRQHandler
*
*	函数功能：	串口1收发中断
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void USART1_IRQHandler(void)
{
	
	RTOS_EnterInt();

#if(USART_DMA_RX_EN == 0)
	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)		//接收中断
	{
		alterInfo.alterCount %= sizeof(alterInfo.alterBuf);			//防止串口被刷爆
		
        alterInfo.alterBuf[alterInfo.alterCount++] = USART1->DR;
		
		USART_ClearFlag(USART1, USART_FLAG_RXNE);
	}
#endif

	if(USART_GetFlagStatus(USART1, USART_FLAG_IDLE) != RESET)
	{
		alterInfo.rev_idle = 1;
		alterInfo.alterCount = 0;
		
		USART1->DR;													//读取数据注意：这句必须要，否则不能够清除中断标志位
		USART_ClearFlag(USART1, USART_IT_IDLE);
		
#if(USART_DMA_RX_EN == 1)
		DMA_Cmd(DMA1_Channel5, DISABLE);
		
		DMA1_Channel5->CNDTR = sizeof(alterInfo.alterBuf);			//重新设置下次接收的长度，否则无法启动下次DMA接收
		
		DMA_Cmd(DMA1_Channel5, ENABLE);
#endif
	}
	
	RTOS_ExitInt();

}
