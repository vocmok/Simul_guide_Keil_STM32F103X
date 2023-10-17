#include "RTE_Components.h" // Component selection
#include CMSIS_device_header // Device header
#include <stdio.h>

int main(void)
{
	uint32_t cnt=0;
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_ClocksTypeDef RCC_ClockFreq;
	RCC_GetClocksFreq(&RCC_ClockFreq);
	printf("SYSCLK=%dHz, HCLK=%dHz, PCLK1=%dHz, PCLK2=%dHz\n",RCC_ClockFreq.HCLK_Frequency,
	RCC_ClockFreq.SYSCLK_Frequency, RCC_ClockFreq.PCLK1_Frequency,
	RCC_ClockFreq.PCLK2_Frequency);
	switch(RCC_GetSYSCLKSource())
	{
		case 0x08:
		printf("PLL used as system clock\n");
		break;
		case 0x04:
		printf("HSE used as system clock\n");
		break;
		case 0x00:
		printf("HSI used as system clock\n");
		break; 
	}
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	//PA2,3,4,6 Input, Pull up
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==0)
		printf("%9i press PA2\n",cnt++);
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)==0)
		printf("%9i press PA3\n",cnt++);
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4)==0)
		printf("%9i press PA4\n",cnt++);
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6)==0)
		printf("%9i press PA6\n",cnt++); 
	}
}