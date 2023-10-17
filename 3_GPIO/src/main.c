#include "RTE_Components.h"
#include CMSIS_device_header

void delay(void)
{
	volatile uint32_t count= 55370;
	while(count--)
		__NOP();
}

int main(void)
{
	//��������� PC13
	//�������� ������������ ����� GPIOC
	*(uint32_t*)(0x40021018)|= 0x00000010;
	RCC->APB2ENR|= RCC_APB2ENR_IOPCEN;
	// ����������� PC13 ��� �����
	// C��������� � ���� ���� ���������� 13 �������
	*(uint32_t*)(0x40011004)&= 0xFF0FFFFF;
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
	//MODE: ����� � �������� 2 ���
	//CNF: ����� push-pull
	*(uint32_t*)(0x40011004)|= 0x00200000;
	SET_BIT(GPIOC->CRH,GPIO_CRH_MODE13_1);
	for(;;)
	{
		*(uint32_t*)(0x40011010) = 0x00002000;
		GPIOC->BSRR = GPIO_BSRR_BS13;
		delay();
		*(uint32_t*)(0x40011014) = 0x00002000;
		GPIOC->BRR = GPIO_BRR_BR13;
		delay();
	}
}