#include "RTE_Components.h" 
#include CMSIS_device_header 
#include <stdio.h> 
// задержка таймера 
static volatile uint32_t TimingDelay; 
// цифра для вывода 
static volatile int8_t digit = -1; 
// длительность точки - 45 мс 
const uint32_t dotDurationMs = 45; 
// коды Морзе для цифр от 0 до 9 
// 0 - точка, 1 - тире, RTL, 5 значимых позиций 
const int8_t morseCodes[] = {
	0x1F, // 00011111 - 0
	0x1E, // 00011110 - 1
	0x1C, // 00011100 - 2
	0x18, // 00011000 - 3
	0x10, // 00010000 - 4
	0x00, // 00000000 - 5
	0x01, // 00000001 - 6
	0x03, // 00000011 -7
	0x07, // 00000111 - 8
	0x0F }; // 00001111 - 9

// выставляем задержку nTime мс 
void Delay(volatile uint32_t nTime) 
{  
	TimingDelay = nTime; 
	while(TimingDelay != 0); 
} 

// уменьшаем счетчик задержки 
void TimingDelay_Decrement(void) 
{ 
	if (TimingDelay != 0x00) 
	{  
		TimingDelay--; 
	} 
} 

// обработчик прерывание системного таймера  
void SysTick_Handler(void)  
{ 
	TimingDelay_Decrement(); 
} 

void initKeypad() { 
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // разрешаем работу GPIO A 
	// обнуляем значения регистров 
	GPIOA->CRL = 0;  
	GPIOA->CRH = 0; 
	//PA0-PA9 Input, Pull up 
	SET_BIT(GPIOA->CRL, GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1 | GPIO_CRL_CNF2_1 
		| GPIO_CRL_CNF3_1 | GPIO_CRL_CNF4_1 | GPIO_CRL_CNF5_1 | GPIO_CRL_CNF6_1 | 
	GPIO_CRL_CNF7_1); 
	SET_BIT(GPIOA->CRH, GPIO_CRH_CNF8_1 | GPIO_CRH_CNF9_1); 
	// pull up 
	SET_BIT(GPIOA->ODR, GPIO_ODR_ODR0 | GPIO_ODR_ODR1 | GPIO_ODR_ODR2 | 
		GPIO_ODR_ODR3 | GPIO_ODR_ODR4 | GPIO_ODR_ODR5 | GPIO_ODR_ODR6 | GPIO_ODR_ODR7 
	| GPIO_ODR_ODR8 | GPIO_ODR_ODR9);   
	// выбираем в качестве внешних входов EXTI линии: 
	// EXTIn = PAn 
	AFIO->EXTICR[0] = AFIO_EXTICR1_EXTI0_PA | AFIO_EXTICR1_EXTI1_PA | 
	AFIO_EXTICR1_EXTI2_PA | AFIO_EXTICR1_EXTI3_PA; 
	AFIO->EXTICR[1] = AFIO_EXTICR2_EXTI4_PA | AFIO_EXTICR2_EXTI5_PA | 
	AFIO_EXTICR2_EXTI6_PA | AFIO_EXTICR2_EXTI7_PA; 
	AFIO->EXTICR[3] = AFIO_EXTICR3_EXTI8_PA | AFIO_EXTICR3_EXTI9_PA; 
} 

void initOutput() { 
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // разрешаем работу GPIO C 
	// PC13, Output mode, max speed 50 MHz, General purpose output push-pull 
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13); 
	SET_BIT(GPIOC->CRH, GPIO_CRH_MODE13); 
} 

void initIRQ() { 
	// прерывание на спад сигнала 
	SET_BIT(EXTI->FTSR, EXTI_FTSR_TR0 | EXTI_FTSR_TR1 | EXTI_FTSR_TR2 | 
		EXTI_FTSR_TR3 | EXTI_FTSR_TR4 | EXTI_FTSR_TR5 | EXTI_FTSR_TR6 | EXTI_FTSR_TR7 
	| EXTI_FTSR_TR8 | EXTI_FTSR_TR9); 
	// разрешаем прерывания внешних линий 0-9, 10 - софтовое прерывание 
	SET_BIT(EXTI->IMR, EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR2 | 
		EXTI_IMR_MR3 | EXTI_IMR_MR4 | EXTI_IMR_MR5 | EXTI_IMR_MR6 | EXTI_IMR_MR7 | 
	EXTI_IMR_MR8 | EXTI_IMR_MR9 | EXTI_IMR_MR10); 
	NVIC_EnableIRQ(EXTI0_IRQn); 
	NVIC_EnableIRQ(EXTI1_IRQn); 
	NVIC_EnableIRQ(EXTI2_IRQn); 
	NVIC_EnableIRQ(EXTI3_IRQn); 
	NVIC_EnableIRQ(EXTI4_IRQn); 
	NVIC_EnableIRQ(EXTI9_5_IRQn); 
	NVIC_EnableIRQ(EXTI15_10_IRQn); 
	NVIC_SetPriority(EXTI0_IRQn, 7); 
	NVIC_SetPriority(EXTI1_IRQn, 7); 
	NVIC_SetPriority(EXTI2_IRQn, 7); 
	NVIC_SetPriority(EXTI3_IRQn, 7); 
	NVIC_SetPriority(EXTI4_IRQn, 7); 
	NVIC_SetPriority(EXTI9_5_IRQn, 7); 
	NVIC_SetPriority(EXTI15_10_IRQn, 15); 
} 

void initTimer() { 
	SystemCoreClockUpdate(); 
	// прерывание каждые 1мс 
	if (SysTick_Config(SystemCoreClock / 1000)) 
	{  
		/* Capture error */  
		while (1); 
	} 
	NVIC_SetPriority(SysTick_IRQn, 4); 
} 

int main() { 
	initTimer(); 
	initKeypad(); 
	initOutput(); 
	initIRQ(); 
	while(1){} 
} 


void Wait(uint32_t nCycles) { 
	Delay(nCycles * dotDurationMs); 
} 

void Dot() { 
	GPIOC->ODR = 1 << 13; 
	Wait(1); // точка 
	GPIOC->ODR = 0 << 13; 
	Wait(1); // пауза между элементами знака 
} 

void Dash() { 
	GPIOC->ODR = 1 << 13; 
	Wait(3); // тире, длительность - 3 точки 
	GPIOC->ODR = 0 << 13; 
	Wait(1); // пауза между элементами знака 
} 

// вывод цифры азбукой морзе и на экран  
void ProcessNumber(int number) 
{ 
	ITM_SendChar(0x30 + number); 
	ITM_SendChar('\n'); 
	uint8_t i, code, bit; 
	code = morseCodes[number]; 
	for (i = 0; i < 5; i++) { 
		bit = (code & ( 1 << i )) >> i; 
		if (bit == 0)  
		{ 
			Dot(); 
		} 
		else  
		{ 
			Dash(); 
		} 
	} 
	Wait(2); //пауза между знаками в слове-3 точки (одна после элемента+2) 
} 

void EXTI0_IRQHandler(void) 
{  
	EXTI->PR = EXTI_PR_PR0; 
	if (digit == -1) { 
		digit = 0; 
		// вызываем обработчик софтовым прерыванием 
		EXTI->SWIER = 0x400; 
	} 
} 

void EXTI1_IRQHandler(void) 
{  
	EXTI->PR = EXTI_PR_PR1; 
	if (digit == -1) { 
		digit = 1; 
		// вызываем обработчик софтовым прерыванием 
		EXTI->SWIER = 0x400; 
	} 
} 

void EXTI2_IRQHandler(void) 
{  
	EXTI->PR = EXTI_PR_PR2; 
	if (digit == -1) { 
		digit = 2; 
		// вызываем обработчик софтовым прерыванием 
		EXTI->SWIER = 0x400; 
	} 
} 

void EXTI3_IRQHandler(void) 
{  
	EXTI->PR = EXTI_PR_PR3; 
	if (digit == -1) { 
		digit = 3; 
		// вызываем обработчик софтовым прерыванием 
		EXTI->SWIER = 0x400; 
	} 
} 

void EXTI4_IRQHandler(void) 
{  
	EXTI->PR = EXTI_PR_PR4; 
	if (digit == -1) { 
		digit = 4; 
		// вызываем обработчик софтовым прерыванием 
		EXTI->SWIER = 0x400; 
	} 
} 

void EXTI9_5_IRQHandler(void) 
{  
	// определим нажатую кнопку 
	uint32_t pending = EXTI->PR; 
	uint8_t i; 
	for (i = 5; i < 10; i++) 
	{ 
		if (pending & (1 << i)) { 
			EXTI->PR = 1 << i; 
			if (digit == -1) { 
				digit = i; 
				// вызываем обработчик софтовым прерыванием 
				EXTI->SWIER = 0x400; 
			} 
			return; 
		} 
	} 
} 

// софтовое прерывание вызывает генератор Морзе 
void EXTI15_10_IRQHandler(void) 
{  
	uint32_t pending = EXTI->PR; 
	if (pending & (1 << 10)) { 
		EXTI->PR = 1 << 10; 
		ProcessNumber(digit); 
		digit = -1; 
	} 
} 
