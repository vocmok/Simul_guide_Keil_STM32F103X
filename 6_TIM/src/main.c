#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_system.h"
#include "stm32f1xx_ll_utils.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_exti.h"
#include "stm32f1xx_ll_tim.h"
#include "stm32f1xx_ll_cortex.h"
#include <stdio.h>

uint16_t TIM2cnt=0; //переменная текущего состояния счётчика TIM2 для лог. анализатора
static uint32_t InitialAutoreload = 0; // начальное состояние TIM2_ARR для частоты 20Гц
static uint8_t AutoreloadMult = 1; //коэфф. деления частоты

int main(void)
{
	// Настройка системной частоты = 72 MHz
	printf("clk=%d Hz\n",SystemCoreClock);
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
	LL_RCC_HSE_Enable();
	while(LL_RCC_HSE_IsReady() != 1){};
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
	LL_RCC_PLL_Enable();
	while(LL_RCC_PLL_IsReady() != 1){};
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL){};
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
	SystemCoreClockUpdate();
	printf("clk=%d Hz\n",SystemCoreClock);
	// Настройка линии PC13 и PB13 на вывод
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOC | LL_APB2_GRP1_PERIPH_GPIOB);
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
	// Настройка прерывания на линию PB12 при изменении сигнала на входе с 1 в 0
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_12, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_12, LL_GPIO_PULL_DOWN);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
	LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTB, LL_GPIO_AF_EXTI_LINE12);
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_12);
	LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_12);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
	NVIC_SetPriority(EXTI15_10_IRQn,0x03);
	// Настройка таймера №2(TIM2)
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2); //включили в работу
	LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERMODE_UP); //счёт вверх
	// Значение предделителя (TIM2_PSC) вычисляем на эквивалент 10КГц для тактир. счётчика
	// TIM2CLK = PCLK1x2 = 72 МГц
	// TIM2_PSC (Prescaler) = (TIM2CLK /10 KHz) - 1 = 7 199 = 0x1C1F
	LL_TIM_SetPrescaler(TIM2, __LL_TIM_CALC_PSC(SystemCoreClock, 10000));
	printf("TIM2_PSC=%d\n",__LL_TIM_CALC_PSC(SystemCoreClock, 10000));
	// Значение регистра авто-перезагрузки TIM2_ARR вычисляем на частоту 20 Гц
	// TIM2_ARR = 72000000 / (7199+1) / 20 - 1 = 499 = 0x1F3
	InitialAutoreload = __LL_TIM_CALC_ARR(SystemCoreClock, LL_TIM_GetPrescaler(TIM2), 20);
	LL_TIM_SetAutoReload(TIM2, InitialAutoreload);
	printf("TIM2_ARR=%d\n",InitialAutoreload);
	LL_TIM_EnableIT_UPDATE(TIM2);//разрешаем таймеру №2 прерывание по событию обновления
	NVIC_SetPriority(TIM2_IRQn, 0);
	NVIC_EnableIRQ(TIM2_IRQn); //разрешаем прерывание таймеру №2 в NVIC
	LL_TIM_EnableCounter(TIM2); //включаем в работу таймер №2
	LL_TIM_GenerateEvent_UPDATE(TIM2); //программно вызываем событие обновления таймера №2
	//Настройка системного таймера
	LL_Init1msTick(SystemCoreClock); //настраиваем на прерывание каждую миллисекунду
	LL_SYSTICK_EnableIT(); //разрешаем прерывание системному таймеру
	while (1)
	{ 
		TIM2cnt=TIM2->CNT; //присваиваем текущее значение счётчика TIM2->CNT
	} 
}

void SysTick_Handler(void)
{	//обработчик прерывания системного таймера
	static uint32_t cnt1ms=0;
	cnt1ms++;
	if(cnt1ms%500 == 0)//переключаем каждые полсекунды
		LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_13);//частота меандра 1Гц
}

void TIM2_IRQHandler(void)
{	//обработчик прерывания таймера №2
	if(LL_TIM_IsActiveFlag_UPDATE(TIM2) == 1)
	{	//контроль, прерывание по обновлению счётчика?
		LL_TIM_ClearFlag_UPDATE(TIM2);//очищаем флаг прерывания
		LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13);
	}
}

void EXTI15_10_IRQHandler(void)
{
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET)
	{	//контроль, прерывания на линии №12?
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);//очищаем флаг прерывания
		// Меняем значение регистра авто-перезагрузки TIM2_ARR по закону:
		// Update_event = TIM2CLK /((PSC + 1)*(ARR + 1)*(AutoreloadMult + 1))
		AutoreloadMult = AutoreloadMult % 5;// получаемый ряд частот Гц: 20; 10; 6.(6); 5; 4
		uint32_t coefARR=(InitialAutoreload + 1) * (AutoreloadMult +1);
		LL_TIM_SetAutoReload(TIM2, coefARR-1);
		printf("TIM2_ARR=%d\n",coefARR-1);
		printf("freq rect PC13:%f\n",SystemCoreClock/(float)(2*(LL_TIM_GetPrescaler(TIM2)+1)*coefARR));
		LL_TIM_GenerateEvent_UPDATE(TIM2);//программно вызываем событие обновления таймера №2
		AutoreloadMult++;
	}
}