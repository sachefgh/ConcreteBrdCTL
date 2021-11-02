#pragma once
#ifndef __MAIN_H
#include "main.h"
#endif


/******************************************************************************/
static uint32_t  fac_us = 0; 							//us��ʱ������			   
static uint32_t fac_ms = 0; 							//ms��ʱ������		

static uint16_t ks1 = 4050;
static uint16_t ks2 = 2;
/******************************************************************************/
__STATIC_INLINE void delay_init(void)
{
	//SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); 	//ѡ���ⲿʱ��  HCLK/8

	fac_us = SystemCoreClock / 1000000; 				//Ϊϵͳʱ��
	fac_ms = fac_us * 1000; 					//��OS��,����ÿ��ms��Ҫ��systickʱ����;OS�´���1ms��ִ�е�ָ����
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

#ifdef USE_FREERTOS		//ʹ��RTOS����£�ʹ��__NOP()��ʽʵ����ʱ
__STATIC_INLINE void delay_us(uint32_t nus)
{	//���뺯��2ָ��

	uint16_t i = 0;
	while (nus--)
	{
		i = ks2;
		while (i--) ;
	}
	
}

void delay_ms(uint32_t nms)
{
	//����stm32f1ϵ�� 72mhz������1us
	uint16_t i = 0;
	while (nms--)
	{
		i = ks1;
		while (i--) ;
	} 
}
  



#else
	//δʹ��RTOS����£�ʹ��SysTick��ʱ��ʵ����ʱ

//��ʱnus
//nusΪҪ��ʱ��us��.
//Result: 100us ->Actual 101.32us
__STATIC_INLINE void delay_us(uint32_t nus)
{		
	uint32_t temp;	    	 
	SysTick->LOAD = nus*fac_us;  					//ʱ�����	  		 
	SysTick->VAL = 0x00;         					//��ռ�����
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; 	//��ʼ����	  
	do
	{
		temp = SysTick->CTRL;
	}while ((temp & 0x01) && !(temp&(1 << 16))) ;		//�ȴ�ʱ�䵽��   
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; 	//�رռ�����
	SysTick->VAL = 0X00;       					 //��ռ�����	 
}
//��ʱnms
//ע��nms�ķ�Χ
//SysTick->LOADΪ24λ�Ĵ���,����,�����ʱΪ:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK��λΪHz,nms��λΪms
//��72M������,nms<=1864 
//Result:   100ms ->Actual 99.997664ms
__STATIC_INLINE void delay_ms(uint32_t nms) 
{	 		  	  
	uint32_t temp;		   
	SysTick->LOAD = nms*fac_ms; 				//ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL = 0x00; 							//��ռ�����
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; 	//��ʼ����  
	do
	{
		temp = SysTick->CTRL;
	}while ((temp & 0x01) && !(temp&(1 << 16))) ;		//�ȴ�ʱ�䵽��   
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; 	//�رռ�����
	SysTick->VAL = 0X00;        					//��ռ�����	  	    
} 
#endif 

	