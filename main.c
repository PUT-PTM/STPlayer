#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_conf.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rng.h"
#include "misc.h"
#include "delay.h"
#include "codec.h"
#include "List.h"
#include "ff.h"
#include <stdbool.h>

// SEKCJA ZMIENNYCH POMOCNICZYCH

FATFS fatfs;
FIL file;
u16 sample_buffer[2048];
s8 num_of_switch=-1;
u16 result_of_conversion=0;
u8 diode_state=0;
u8 dioda=0;
u8 error_state=0;
bool random_mode=0;
s8 change_song=0;
char song_time[5]={'0', '0', ':', '0', '0'};
bool half_second=0;

// KONIEC SEKCJI ZMIENNYCH POMOCNICZYCH

void SysTick_Handler()
{
	disk_timerproc();
}

// SEKCJA DIOD

void diodes_irr() // KONFIGURACJA PRZERWAN DLA DIOD
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	// KONFIGURACJA PRZERWAN - TIMER/COUNTER
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;// numer przerwania
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;// priorytet glowny
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;// subpriorytet
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;// uruchom dany kanal
	NVIC_Init(&NVIC_InitStructure);// zapisz wypelniona strukture do rejestrow

	TIM_TimeBaseInitTypeDef TIMER_3;
	TIMER_3.TIM_Period = 48000-1;// okres zliczania nie przekroczyc 2^16!
	TIMER_3.TIM_Prescaler = 1000-1;// wartosc preskalera, tutaj bardzo mala
	TIMER_3.TIM_ClockDivision = TIM_CKD_DIV1;// dzielnik zegara
	TIMER_3.TIM_CounterMode = TIM_CounterMode_Up;// kierunek zliczania
	TIM_TimeBaseInit(TIM3, &TIMER_3);

	// UWAGA: uruchomienie zegara jest w przerwaniu
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);// wyczyszczenie przerwania od timera 3 (wystapilo przy konfiguracji timera)
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);// zezwolenie na przerwania od przepelnienia dla timera 3
}
void diodes_initialization() // KONFIGURACJA DIOD
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitTypeDef  DIODES;
	/* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	DIODES.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	DIODES.GPIO_Mode = GPIO_Mode_OUT;// tryb wyprowadzenia, wyjcie binarne
	DIODES.GPIO_OType = GPIO_OType_PP;// wyjcie komplementarne
	DIODES.GPIO_Speed = GPIO_Speed_100MHz;// max. V przelaczania wyprowadzen
	DIODES.GPIO_PuPd = GPIO_PuPd_NOPULL;// brak podciagania wyprowadzenia
	GPIO_Init(GPIOD, &DIODES);
}


// KONIEC SEKCJI DIOD

// OBSLUGA MODULU DMA (DIRECT MEMORY ACCESS)
void MY_DMA_initM2P()
{
	DMA_InitTypeDef  DMA_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Stream5);
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;// wybor kanalu DMA
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;// ustalenie rodzaju transferu (memory2memory / peripheral2memory /memory2peripheral)
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;// tryb pracy - pojedynczy transfer badz powtarzany
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;// ustalenie priorytetu danego kanalu DMA
	DMA_InitStructure.DMA_BufferSize = 2048;// liczba danych do przeslania
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&sample_buffer;// adres zrodlowy
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI3->DR));// adres docelowy
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;// zezwolenie na inkrementacje adresu po kazdej przeslanej paczce danych
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;// ustalenie rozmiaru przesylanych danych
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;// ustalenie trybu pracy - jednorazwe przeslanie danych
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;// wylaczenie kolejki FIFO
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;

	DMA_Init(DMA1_Stream5, &DMA_InitStructure);// zapisanie wypelnionej struktury do rejestrow wybranego polaczenia DMA
	DMA_Cmd(DMA1_Stream5, ENABLE);// uruchomienie odpowiedniego polaczenia DMA

	SPI_I2S_DMACmd(SPI3,SPI_I2S_DMAReq_Tx,ENABLE);
	SPI_Cmd(SPI3,ENABLE);
}

// TIMERY, KONTROLERY PRZERWAN

void vibrations() // TIMER DO ELIMINACJI DRGAN STYKOW, TIM5
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseInitTypeDef TIMER;
	/* Time base configuration */
	TIMER.TIM_Period = 8400-1;
	TIMER.TIM_Prescaler = 3000-1;
	TIMER.TIM_ClockDivision = TIM_CKD_DIV1;
	TIMER.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM5, &TIMER);
	TIM_Cmd(TIM5,DISABLE);

	// KONFIGURACJA PRZERWAN - TIMER/COUNTER
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;// numer przerwania
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;// priorytet glowny
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;// subpriorytet
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;// uruchom dany kanal
	NVIC_Init(&NVIC_InitStructure);// zapisz wypelniona strukture do rejestrow
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);// wyczyszczenie przerwania od timera 5 (wystapilo przy konfiguracji timera)
	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);// zezwolenie na przerwania od przepelnienia dla timera 5
}

void EXTI0_IRQHandler(void)
{
	// drgania stykow
	if(EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		num_of_switch=0;
		TIM_Cmd(TIM5, ENABLE);
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}
void EXTI9_5_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line5) != RESET)
	{
		num_of_switch=5;
		TIM_Cmd(TIM5, ENABLE);
		EXTI_ClearITPendingBit(EXTI_Line5);
	}
	else if(EXTI_GetITStatus(EXTI_Line7) != RESET)
	{
		num_of_switch=7;
		TIM_Cmd(TIM5, ENABLE);
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
	else if(EXTI_GetITStatus(EXTI_Line8) != RESET)
	{
		num_of_switch=8;
		TIM_Cmd(TIM5, ENABLE);
		EXTI_ClearITPendingBit(EXTI_Line8);
	}
}
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		ADC_conv();
		Codec_VolumeCtrl(result_of_conversion);
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		if(half_second==false)
		{
			half_second=true;
		}
		else
		{
			half_second=false;
		}
		if(half_second==0)
		{
			song_time[4]++;
			if(song_time[4]==':')
			{
				song_time[3]++;
				song_time[4]='0';
			}
			if(song_time[3]=='6')
			{
				song_time[1]++;
				song_time[3]='0';
			}
			if(song_time[1]==':')
			{
				song_time[0]++;
				song_time[1]='0';
			}
			if(song_time[0]==':')
			{
				song_time[0]='0';
			}
		}
		// wyzerowanie flagi wyzwolonego przerwania
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}

void TIM5_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
	{
		if (num_of_switch==0)// wcisnieto user button 0 - losowe odtwarzanie
		{
			change_song=1;
		}
		num_of_switch=-1;
		TIM_Cmd(TIM5, DISABLE);
		TIM_SetCounter(TIM5, 0);
		// wyzerowanie flagi wyzwolonego przerwania
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
	}
}



void button_initialization() // KONFIGURACJA OBSLUGI PRZYCISKOW
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);

	GPIO_InitTypeDef USER_BUTTON;
	USER_BUTTON.GPIO_Pin = GPIO_Pin_0;
	USER_BUTTON.GPIO_Mode = GPIO_Mode_IN;
	USER_BUTTON.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &USER_BUTTON);
}
void interrupt_initialization() // KONFIGURACJA KONTROLERA PRZERWAN
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn; // numer przerwania
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;// priorytet glowny
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;// subpriorytet
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;// uruchom dany kanal
	NVIC_Init(&NVIC_InitStructure);// zapisz wypelniona strukture do rejestrow

	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;// wybor numeru aktualnie konfigurowanej linii przerwan
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;// wybor trybu - przerwanie badz zdarzenie
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;// wybor zbocza, na ktore zareaguje przerwanie
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;// uruchom dana linie przerwan
	EXTI_Init(&EXTI_InitStructure);// zapisz strukture konfiguracyjna przerwan zewnetrznych do rejestrow

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

	// KONFIGURACJA KONTROLERA PRZERWAN DLA SWITCH Pin_5, Pin_6, Pin_7
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	EXTI_InitStructure.EXTI_Line = EXTI_Line5 | EXTI_Line7 | EXTI_Line8;
	NVIC_Init(&NVIC_InitStructure);
	EXTI_Init(&EXTI_InitStructure);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource5);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource7);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource8);
}



void ADC_initialization()
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);// zegar dla portu GPIO z ktorego wykorzystany zostanie pin
	// jako wejscie ADC (PA1)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);// zegar dla modulu ADC1

	// inicjalizacja wejscia ADC
	GPIO_InitTypeDef  GPIO_InitStructureADC;
	GPIO_InitStructureADC.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructureADC.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructureADC.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructureADC);

	ADC_CommonInitTypeDef ADC_CommonInitStructure;// Konfiguracja dla wszystkich ukladow ADC
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;// niezalezny tryb pracy przetwornikow
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;// zegar glowny podzielony przez 2
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;// opcja istotna tylko dla tryby multi ADC
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;// czas przerwy pomiedzy kolejnymi konwersjami
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitTypeDef ADC_InitStructure;// Konfiguracja danego przetwornika
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;// ustawienie rozdzielczosci przetwornika na maksymalna (12 bitow)
	// wylaczenie trybu skanowania (odczytywac bedziemy jedno wejscie ADC
	// w trybie skanowania automatycznie wykonywana jest konwersja na wielu
	// wejsciach/kanalach)
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;// wlaczenie ciaglego trybu pracy wylaczenie zewnetrznego wyzwalania
	// konwersja moze byc wyzwalana timerem, stanem wejscia itd. (szczegoly w dokumentacji)
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	// wartosc binarna wyniku bedzie podawana z wyrownaniem do prawej
	// funkcja do odczytu stanu przetwornika ADC zwraca wartosc 16-bitowa
	// dla przykladu, wartosc 0xFF wyrownana w prawo to 0x00FF, w lewo 0x0FF0
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;// liczba konwersji rowna 1, bo 1 kanal
	ADC_Init(ADC1, &ADC_InitStructure);// zapisz wypelniona strukture do rejestrow przetwornika numer 1

	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_84Cycles);// Konfiguracja kanalu pierwszego ADC
	ADC_Cmd(ADC1, ENABLE);// Uruchomienie przetwornika ADC

	TIM2_ADC_initialization();
}
void TIM2_ADC_initialization()
{
	// Wejscie do przerwania od TIM2 co <0.05 s
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	// 2. UTWORZENIE STRUKTURY KONFIGURACYJNEJ
	TIM_TimeBaseInitTypeDef TIMER_2;
	TIMER_2.TIM_Period = 2100-1;// okres zliczania nie przekroczyc 2^16!
	TIMER_2.TIM_Prescaler = 2000-1;// wartosc preskalera, tutaj bardzo mala
	TIMER_2.TIM_ClockDivision = TIM_CKD_DIV1;// dzielnik zegara
	TIMER_2.TIM_CounterMode = TIM_CounterMode_Up;// kierunek zliczania
	TIM_TimeBaseInit(TIM2, &TIMER_2);
	TIM_Cmd(TIM2, ENABLE);// Uruchomienie Timera

	// KONFIGURACJA PRZERWAN - TIMER/COUNTER
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;// numer przerwania
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;// priorytet glowny
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;// subpriorytet
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;// uruchom dany kanal
	NVIC_Init(&NVIC_InitStructure);// zapisz wypelniona strukture do rejestrow
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);// wyczyszczenie przerwania od timera 2 (wystapilo przy konfiguracji timera)
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);// zezwolenie na przerwania od przepelnienia dla timera 2
}
void ADC_conv() // konwersja analog. -> cyfr.
{
	// Odczyt wartosci przez odpytnie flagi zakonczenia konwersji
	// Wielorazowe sprawdzenie wartosci wyniku konwersji
	ADC_SoftwareStartConv(ADC1);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	result_of_conversion = ((ADC_GetConversionValue(ADC1))/16);
}

bool read_and_send(FRESULT fresult, int position, volatile ITStatus it_status, UINT read_bytes, uint32_t DMA_FLAG)
{
	it_status = RESET;
	while(it_status == RESET)
	{
		it_status = DMA_GetFlagStatus(DMA1_Stream5, DMA_FLAG);
	}
	fresult = f_read (&file,&sample_buffer[position],1024*2,&read_bytes);
	DMA_ClearFlag(DMA1_Stream5, DMA_FLAG);
	if(read_bytes<1024*2||change_song!=0)
	{
		return 0;
	}
	return 1;
}
void play(struct List *song, FRESULT fresult)
{
	struct List *temporary_song=song;
	UINT read_bytes;
	fresult = f_open( &file, temporary_song->file.fname, FA_READ );
	if( fresult == FR_OK )
	{
		fresult=f_lseek(&file,44);// pominiecie 44 B naglowka pliku .wav
		volatile ITStatus it_status;// sprawdza flage DMA
		change_song=0;
		song_time[0]=song_time[1]=song_time[3]=song_time[4]='0';
		song_time[2]=':';
		half_second=0;
		TIM_Cmd(TIM3, ENABLE);
		while(1)
		{
			if (read_and_send(fresult,0, it_status, read_bytes, DMA_FLAG_HTIF5)==0)
			{
				break;
			}
			if (read_and_send(fresult, 1024, it_status, read_bytes, DMA_FLAG_TCIF5)==0)
			{
				break;
			}
		}
		dioda=0;
		TIM_Cmd(TIM3, DISABLE);
		fresult = f_close(&file);
	}
}
bool isWAV(FILINFO fileInfo)
{
	int i=0;
	for (i=0;i<10;i++)
	{
		if(fileInfo.fname[i]=='.')
		{
			if(fileInfo.fname[i+1]=='W' && fileInfo.fname[i+2]=='A' && fileInfo.fname[i+3]=='V')
			{
				return 1;
			}
		}
	}
	return 0;
}

int main( void )
{
	SystemInit();
	diodes_initialization();// inicjalizacja diod
	delay_init( 80 );// wyslanie 80 impulsow zegarowych; do inicjalizacji SPI
	SPI_SD_Init();// inicjalizacja SPI pod SD
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);// zegar 24-bitowy
	SysTick_Config(90000);

	// SD CARD
	FRESULT fresult;
	DIR Dir;
	FILINFO fileInfo;

	struct List *first=0,*last=0,*pointer;
	disk_initialize(0);// inicjalizacja karty
	fresult = f_mount( &fatfs, 1,1 );// zarejestrowanie dysku logicznego w systemie
	fresult = f_opendir(&Dir, "\\");
	u32 number_of_songs=0;
	for(;;)
	{
		fresult = f_readdir(&Dir, &fileInfo);
		if(fresult != FR_OK)
		{
			return(fresult);
		}
		if(!fileInfo.fname[0])
		{
			break;
		}
		if(isWAV(fileInfo)==1)// sprawdzenie, czy plik na karcie ma rozszerzenie .wav
		{
			if(number_of_songs==0)
			{
				first=last=add_last(last,fileInfo);
			}
			else
			{
				last=add_last(last,fileInfo);
			}
			number_of_songs++;
		}
	}
	last->next=first;
	first->previous=last;
	pointer=first;

	codec_init();
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);// Integrated Interchip Sound to connect digital devices
	MY_DMA_initM2P();
	button_initialization();
	ADC_initialization();
	vibrations();
	interrupt_initialization();
	diodes_irr();
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);
	u32 rand_number=0;
	u32 i_loop=0;

	for(;;)
	{
		play(pointer, fresult);
		if(change_song>=0)// wcisnieto user button albo skonczylo sie odtwarzanie utworu
			{
				pointer=pointer->next;
			}
	}
	GPIO_ResetBits(GPIOD, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15|CODEC_RESET_PIN);
	TIM_Cmd(TIM2, DISABLE);
	for(;;)
	{ }
	return 0;
}
