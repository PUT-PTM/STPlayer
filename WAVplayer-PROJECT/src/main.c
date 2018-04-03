/**
 ******************************************************************************
 * @file    main.c
 * @author  Ac6
 * @version V1.0
 * @date    01-December-2013
 * @brief   Default main function.
 ******************************************************************************
 */

#include "stm32f4xx.h"
#include "stm32f4_discovery.h"

void delay(int time) {
	int i;
	for (i = 0; i < time * 4000; i++) {
	}
}

int main(void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin =
	GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_1
			| GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	unsigned int i;
	int led = 0;

	for (;;) {

		int value = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1);
		if (!value && led == 0) {
			GPIO_ResetBits(GPIOD, GPIO_Pin_13 | GPIO_Pin_14);
			led = 1;
			for (int i = 0; i < 10000000; i++) {
				;
			}
		} else if (!value && led == 1) {
			GPIO_SetBits(GPIOD, GPIO_Pin_13 | GPIO_Pin_14);
			led = 0;
			for (int i = 0; i < 10000000; i++) {
				;
			}
		}


		//przykladowy program weryfikujacy dzialanie
		GPIO_SetBits(GPIOD, GPIO_Pin_12 );
		 for (i = 0; i < 10000000; i++);
		 GPIO_SetBits(GPIOD, GPIO_Pin_13 );
		 for (i = 0; i < 3000000; i++);
		 GPIO_SetBits(GPIOD, GPIO_Pin_14 );
		 for (i = 0; i < 6000000; i++);
		 GPIO_SetBits(GPIOD, GPIO_Pin_15 );
		 for (i = 0; i < 9000000; i++);
		 GPIO_ResetBits(GPIOD,
		 GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
		 for (i = 0; i < 1000000; i++)
		 ;
	}
}
