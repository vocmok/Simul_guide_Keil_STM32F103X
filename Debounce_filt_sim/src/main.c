#include "RTE_Components.h"     
#include CMSIS_device_header	
#include <stdio.h>
#define BT_TREPEAT 10   //время повторения символа при зажатой кнопке 10*20ms=200ms

typedef enum
{   
	Key_init	= (0x00),/*начальное*/
    Key_down	= (0x01),/*включена*/
    Key_up		= (0x02),/*отключена*/
    Key_dntoup	= (0x04),/*переход от включена до отключена*/
    Key_uptodn	= (0x08),/*переход от отключена до включена*/
    Key_dnlong	= (0x10),/*долгое нажатие*/
} KEY_state;

typedef struct 
{	
	KEY_state state;
    unsigned int downlongtick;
} KEY_type;

typedef enum
{
	Sgnl_NULL	= (0x00),/*на выходе ноль*/
    Sgnl_key0	= (0x01),/*нажата кнопки 0*/
    Sgnl_key1	= (0x02),/*нажата кнопки 1*/
} SGNL_state;

typedef struct 
{
	SGNL_state state;
    uint8_t flag20ms;
	uint8_t flag1sec;
} ISRdat_type;

__IO ISRdat_type isrdat;

unsigned int keypad_poll(void);

int main()
{ 	//clk 72MHz
	SystemCoreClockUpdate();
	printf("clk=%d Hz\n",SystemCoreClock);
	ITM_SendChar('\n');
	//Включаем тактирование GPIOA, GPIOC
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN);
	// Настраиваем PA0 и PA1 как вход с подтяжкой к питанию
	// Cбрасываем в ноль биты управления PA0 и PA1 выводом
	GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0 |GPIO_CRL_MODE1 | GPIO_CRL_CNF1 );
	SET_BIT(GPIOA->CRL, GPIO_CRL_CNF0_1|GPIO_CRL_CNF1_1); //PA 0,1 Input, Pull up
	SET_BIT(GPIOA->ODR, GPIO_ODR_ODR0|GPIO_ODR_ODR1); //PA 0,1 Pull up
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
	SET_BIT(GPIOC->CRH,GPIO_CRH_MODE13_1); //PC13 push-pull 2MHz
	isrdat.state=Sgnl_NULL;
	isrdat.flag1sec=0;
	isrdat.flag20ms=0;
	SysTick_Config(0xAFC80);//прерывание системного таймера каждые 10 мсек
	uint32_t cntsec=0;
	for(;;)
	{
		if(isrdat.flag1sec)
		{
			cntsec++;
			printf("%d sec\n",cntsec);//выводим каждые 1 сек 
			isrdat.flag1sec=0;
		}
		if(isrdat.flag20ms)
		{
			uint32_t keystate=keypad_poll();
			if(keystate&0x1)
			{
				printf("Key0 PA0 pressed\n");
				isrdat.state=Sgnl_key0;
			}
			if(keystate&0x2)
			{
				printf("Key1 PA1 pressed\n");
				isrdat.state=Sgnl_key1;
			}
			if(keystate&(0x1<<16))
			{
				printf("Key0 PA0 up\n");
				isrdat.state=Sgnl_NULL;
			}
			if(keystate&(0x2<<16))
			{
				printf("Key1 PA1 up\n");
				isrdat.state=Sgnl_NULL;
			}
			isrdat.flag20ms=0;
		}
	}
}

void SysTick_Handler(void)//обработчик прерывание системного таймера
{	
	static uint32_t ui_cnt10ms=0;
	ui_cnt10ms++;
	if(ui_cnt10ms%2==0)
		isrdat.flag20ms=1;
	if(ui_cnt10ms%100==0)
		isrdat.flag1sec=1;
	switch(isrdat.state)
	{
		case Sgnl_NULL:
			GPIOC->ODR = 0; 
			break;
		case Sgnl_key0://частота  50 Hz
			GPIOC->ODR ^= GPIO_ODR_ODR13; 
			break;
		case Sgnl_key1://частота  10 Hz
			if(ui_cnt10ms%5==0)
			GPIOC->ODR ^= GPIO_ODR_ODR13; 
			break;
	}
}

unsigned int keypad_poll()
{
	static KEY_type masKey[2];
	static uint32_t kpd_pollcnt=0;
	kpd_pollcnt++; //счётчик кол-ва опросов
	uint32_t retcode=0;
	KEY_type* pcurKey;
	int i;
	for(i=0;i<2;i++) { /*опрос состояния линий (кнопок)*/
		pcurKey=&masKey[i];
		if(GPIOA->IDR&(1<<i)) /*up на линии единица значит не нажата*/
		{
			switch(pcurKey->state) 
			{
				case Key_dntoup: 
					retcode|=1<<(i+16);//возвращаем флаг отжатия кнопки в старших 16 битах
				case Key_init:
				case Key_up:
				case Key_uptodn: 
					pcurKey->state=Key_up;
				break;
				case Key_down:
				case Key_dnlong: 
					pcurKey->state=Key_dntoup;
				break;
			}
		   
		}
		else {/*down на линии ноль значит нажата*/
			switch(pcurKey->state) 
			{
				case Key_init:
				case Key_dntoup: 
					pcurKey->state=Key_down;
				break;
				case Key_up:
					pcurKey->state=Key_uptodn;
				break;
				case Key_uptodn: 
					pcurKey->state=Key_down;
					retcode|=1<<i;//возвращаем флаг нажатия кнопки в младших 16 битах
				break;
				case Key_down:
					pcurKey->state=Key_dnlong;
					pcurKey->downlongtick=kpd_pollcnt;
				break;
				case Key_dnlong:
					if((kpd_pollcnt-pcurKey->downlongtick)%BT_TREPEAT==0)
						retcode|=1<<i;//повторно возвращаем флаг нажатия кнопки (зажата)
				break;
			}
		}
	}
	return retcode;
}
