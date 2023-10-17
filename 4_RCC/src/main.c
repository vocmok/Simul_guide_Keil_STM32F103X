#include "RTE_Components.h"
#include CMSIS_device_header
#include <stdio.h>

int main(void)
{
	volatile uint32_t StartUpCounter = 0, HSEStatus = 0;
	//проверяем установленную частоту тактирования по умолчанию
	SystemCoreClockUpdate();//устанавливается в глобальной переменной SystemCoreClock
	ITM_SendChar('\n');
	printf("Start clk=%d Hz\n",SystemCoreClock);
	// Bit 16 HSEON: -> RCC_CR_HSEON
	// 0: HSE oscillator OFF; 1: HSE oscillator ON;
	SET_BIT(RCC -> CR,RCC_CR_HSEON);// включаем HSE
	do 
	{	// ждем вхождения в работу HSE
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	} while((HSEStatus == 0) && (StartUpCounter != 0x5000));
	//если за 0x5000 итераций, HSE не запустился, то проблемы в аппаратуре
	if ((RCC->CR & RCC_CR_HSERDY) != RESET) //HSE работает47
	{
		// настраиваем FLASH, время предварительной выборки в буфер команд
		//000: Zero wait state, if 0 < HCLK ? 24 MHz -> FLASH_ACR_LATENCY_0
		//001: One wait state, if 24 MHz < HCLK ? 48 MHz-> FLASH_ACR_LATENCY_1
		//010: Two wait sates, if 48 < HCLK ? 72 MHz -> FLASH_ACR_LATENCY_2
		// 0: Prefetch is disabled
		// 1: Prefetch is enabled -> FLASH_ACR_PRFTBE
		FLASH->ACR = FLASH_ACR_PRFTBE|FLASH_ACR_LATENCY_2;
		// HCLK = SYSCLK / ...
		// 1011: SYSCLK divided by 16 -> RCC_CFGR_HPRE_DIV16
		RCC->CFGR |= RCC_CFGR_HPRE_DIV16;//AHB Pre = 16 согласно варианта
		// настройка PLL на 72 МГц = 8 МГц(HSE) x 9
		// сначала выключаем чтобы изменить биты PLL, после настройки включим
		CLEAR_BIT(RCC -> CR,RCC_CR_PLLON);
		// Bit 16 PLLSRC: -> RCC_CFGR_PLLSRC
		// 0: HSI/2 selected as PLL input clock // 1: HSE selected as PLL input clock
		// Bits 21:18 PLLMUL: -> RCC_CFGR_PLLMUL
		// 0111: PLL input clock x 9 -> RCC_CFGR_PLLMUL9
		// Bit 17 PLLXTPRE: -> RCC_CFGR_PLLXTPRE
		// 0: HSE clock not divided // 1: HSE clock divided by 2
		/* PLL configuration: PLLCLK = HSE * 9 = 72 MHz */
		RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9); // |RCC_CFGR_PLLXTPRE_HSE_Div2 
		// включаем PLL Bit 24 PLLON: -> RCC_CR_PLLON
		// 0: PLL OFF; 1: PLL ON
		SET_BIT(RCC -> CR,RCC_CR_PLLON);
		// ждём запуск и стабилизацию PLL
		while((RCC->CR & RCC_CR_PLLRDY) == 0){}
		// выбираем выход PLL источником тактирования МК
		RCC->CFGR &= ~(RCC_CFGR_SW);
		RCC->CFGR |= RCC_CFGR_SW_PLL;
		//Ожидаем установки PLL источником тактирования МК
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){}
	}
	else
	{ 
		while(1){} // HSE не запустился
	} 
	SystemCoreClockUpdate(); //контролируем частоту 72/16=4.5 МГц
	printf("After configuration clk=%d Hz\n",SystemCoreClock);
	// Настройка MCO на PLLCLK/2
	// Bits 26:24 MCO:
	// 100: System clock (SYSCLK) selected RCC_CFGR_MCO_SYSCLK
	// 111: PLL/2 clock selected RCC_CFGR_MCO_PLL
	SET_BIT(RCC -> CFGR, RCC_CFGR_MCO_PLL);
	SET_BIT(RCC -> APB2ENR,RCC_APB2ENR_IOPAEN); //разрешаем тактирование GPIOA
	// Сбрасываем в ноль биты управления выводом PA8
	GPIOA->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8);
	//MODE: выход с макс. частотой 50 МГц
	//CNF: режим Alternate function output Push-pull
	SET_BIT(GPIOA->CRH,GPIO_CRH_MODE8|GPIO_CRH_CNF8_1);
	//разрешаем тактирование GPIOС PС13
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPCEN);
	//устанавливаем работу линий PС13 и PС14 на вывод
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13|GPIO_CRH_MODE14 | GPIO_CRH_CNF14);
	SET_BIT(GPIOC->CRH,GPIO_CRH_MODE13);// High speed
	SET_BIT(GPIOC->CRH,GPIO_CRH_MODE14_1);// Low speed
	while(1)
	{ 	//устанавливаем 1 на выходе линий PС13,14
		GPIOC->BSRR= GPIO_BSRR_BS13|GPIO_BSRR_BS14;
		//сбрасываем в 0 выходы линий PС13,14
		GPIOC->BRR = GPIO_BRR_BR13|GPIO_BRR_BR14;
	}
}
