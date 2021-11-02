#pragma once
#ifndef __MAIN_H
#include "main.h"
#endif


/******************************************************************************/
static uint32_t  fac_us = 0; 							//us延时倍乘数			   
static uint32_t fac_ms = 0; 							//ms延时倍乘数		

static uint16_t ks1 = 4050;
static uint16_t ks2 = 2;
/******************************************************************************/
__STATIC_INLINE void delay_init(void)
{
	//SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); 	//选择外部时钟  HCLK/8

	fac_us = SystemCoreClock / 1000000; 				//为系统时钟
	fac_ms = fac_us * 1000; 					//非OS下,代表每个ms需要的systick时钟数;OS下代表1ms需执行的指令数
	if((fac_us < 120)&&(fac_us>40))
	{
		ks1 = 7420;
		ks2 = 4;
	}
	else if(fac_us<=40)
	{
		ks1 = 1783*2;
		ks2 = 1;
	}
	else if(fac_us>160)
	{
		ks1 = 41750;
		ks2 = 42;
	}
}

#ifdef USE_FREERTOS		//使用RTOS情况下，使用__NOP()方式实现延时
__STATIC_INLINE void delay_us(uint32_t nus)
{	//进入函数2指令

	uint16_t i = 0;
	while (nus--)
	{
		i = ks2;
		while (i--) ;
	}
	
}

void delay_ms(uint32_t nms)
{
	//对于stm32f1系列 72mhz大致是1us
	uint16_t i = 0;
	while (nms--)
	{
		i = ks1;
		while (i--) ;
	} 
}
  



#else
	//未使用RTOS情况下，使用SysTick计时器实现延时

//延时nus
//nus为要延时的us数.
//Result: 100us ->Actual 101.32us
__STATIC_INLINE void delay_us(uint32_t nus)
{		
	uint32_t temp;	    	 
	SysTick->LOAD = nus*fac_us;  					//时间加载	  		 
	SysTick->VAL = 0x00;         					//清空计数器
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; 	//开始倒数	  
	do
	{
		temp = SysTick->CTRL;
	}while ((temp & 0x01) && !(temp&(1 << 16))) ;		//等待时间到达   
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; 	//关闭计数器
	SysTick->VAL = 0X00;       					 //清空计数器	 
}
//延时nms
//注意nms的范围
//SysTick->LOAD为24位寄存器,所以,最大延时为:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK单位为Hz,nms单位为ms
//对72M条件下,nms<=1864 
//Result:   100ms ->Actual 99.997664ms
__STATIC_INLINE void delay_ms(uint32_t nms) 
{	 		  	  
	uint32_t temp;		   
	SysTick->LOAD = nms*fac_ms; 				//时间加载(SysTick->LOAD为24bit)
	SysTick->VAL = 0x00; 							//清空计数器
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; 	//开始倒数  
	do
	{
		temp = SysTick->CTRL;
	}while ((temp & 0x01) && !(temp&(1 << 16))) ;		//等待时间到达   
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; 	//关闭计数器
	SysTick->VAL = 0X00;        					//清空计数器	  	    
} 
#endif 

	