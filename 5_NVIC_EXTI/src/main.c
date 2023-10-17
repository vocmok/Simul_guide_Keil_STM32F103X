#include "RTE_Components.h" // Component selection
#include CMSIS_device_header // Device header
#include <stdio.h>

void delay(void)
{
	volatile uint32_t i=600000;//задержка
	while(i > 0)
		i--;
}

int main(void)
{
	uint32_t priGroup = 0, PreemptPriority=0, SubPriority=0;
	SystemCoreClockUpdate();
	printf("clk=%d Hz\n",SystemCoreClock);
	RCC->APB2ENR|=RCC_APB2ENR_IOPAEN|RCC_APB2ENR_IOPCEN;//разрешаем работу GPIO A,C
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
	SET_BIT(GPIOC->CRH,GPIO_CRH_MODE13);//PC13 в режим двухтактного выхода
	GPIOA->CRL = 0;//PA2,3,4,6 Input, Pull up
	SET_BIT(GPIOA->CRL, GPIO_CRL_CNF2_1|GPIO_CRL_CNF3_1|GPIO_CRL_CNF4_1|GPIO_CRL_CNF6_1);
	SET_BIT(GPIOA->ODR, GPIO_ODR_ODR2|GPIO_ODR_ODR3|GPIO_ODR_ODR4|GPIO_ODR_ODR6); //pull up
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_AFIOEN); //включаем альтернативный режим для ПВВ
	//выбираем в качестве внешних входов EXTI линии:
	AFIO->EXTICR[0] = AFIO_EXTICR1_EXTI2_PA|AFIO_EXTICR1_EXTI3_PA; //линии PA2>>EXTI2,PA3>>EXTI3
	AFIO->EXTICR[1] = AFIO_EXTICR2_EXTI4_PA|AFIO_EXTICR2_EXTI6_PA; //линии PA4>>EXTI4,PA6>>EXTI6
	//NVIC_SetPriorityGrouping(7);
	priGroup = NVIC_GetPriorityGrouping();
	printf("Priority Group=%d\r\n",priGroup);
	NVIC_SetPriority(EXTI2_IRQn,111);
	NVIC_DecodePriority(NVIC_GetPriority(EXTI2_IRQn),priGroup,&PreemptPriority,&SubPriority);
	printf("EXTI2 Preempt Priority=%d \tSubPriority=%d\r\n",PreemptPriority,SubPriority);
	NVIC_SetPriority(EXTI3_IRQn,110);
	NVIC_DecodePriority(NVIC_GetPriority(EXTI3_IRQn),priGroup,&PreemptPriority,&SubPriority);
	printf("EXTI3 Preempt Priority=%d \tSubPriority=%d\r\n",PreemptPriority,SubPriority);
	NVIC_SetPriority(EXTI4_IRQn,108);
	NVIC_DecodePriority(NVIC_GetPriority(EXTI4_IRQn),priGroup,&PreemptPriority,&SubPriority);
	printf("EXTI4 Preempt Priority=%d \tSubPriority=%d\r\n",PreemptPriority,SubPriority);
	NVIC_SetPriority(EXTI9_5_IRQn,109);
	NVIC_DecodePriority(NVIC_GetPriority(EXTI9_5_IRQn),priGroup,&PreemptPriority,&SubPriority);
	printf("EXTI6 Preempt Priority=%d \tSubPriority=%d\r\n",PreemptPriority,SubPriority);
	//прерывание по срезу сигнала
	SET_BIT(EXTI->FTSR,EXTI_FTSR_TR2|EXTI_FTSR_TR3|EXTI_FTSR_TR4|EXTI_FTSR_TR6);
	//разрешаем прерывания внешних линий 2,3,4,6
	SET_BIT(EXTI->IMR,EXTI_IMR_MR2|EXTI_IMR_MR3|EXTI_IMR_MR4|EXTI_IMR_MR6);
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	SysTick_Config(0x6DDD00); //прерывание таймера каждые 100мс
	while(1){}
}

void SysTick_Handler(void)
{	//обработчик прерывание системного таймера
	GPIOC->ODR^=1<<13; //переключаем линию PC13 каждые 100мс
}

void EXTI2_IRQHandler(void)
{
	EXTI->PR = EXTI_PR_PR2;
	ITM_SendChar('2');
	delay();
	ITM_SendChar('a');
	ITM_SendChar('\n'); 
}

void EXTI3_IRQHandler(void)
{
	EXTI->PR = EXTI_PR_PR3;
	ITM_SendChar('3');
	delay();
	ITM_SendChar('b');
	ITM_SendChar('\n'); 
}

void EXTI4_IRQHandler(void)
{
	EXTI->PR = EXTI_PR_PR4;
	ITM_SendChar('4');
	delay();
	ITM_SendChar('c');
	ITM_SendChar('\n'); 
}

void EXTI9_5_IRQHandler(void)
{
	EXTI->PR = EXTI_PR_PR6;
	ITM_SendChar('6');
	delay();
	ITM_SendChar('d');
	ITM_SendChar('\n'); 
}
