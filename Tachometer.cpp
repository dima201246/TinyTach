// TACHOMETER VERSION: 1.0

/*
Частота мк: 4.8
Время обновления - полсекунды
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#define RPM_CHECK	700 // Частота выше которой двигатель считается запущенным

#define DS_PIN		PB2
#define SH_PIN		PB4
#define ST_PIN		PB0
#define INT_PIN		PB1
#define RMP_PIN		PB3

const uint8_t		displayArray[]	= {0x3, 0x9F, 0x25, 0xD, 0x99, 0x49, 0x41, 0x1F, 0x1, 0x9};

volatile uint8_t	rpmNow			= 0,	// Кол-во оборотов за пол-секунды
					intCounter		= 0;	// Для подсчёта кол-ва прерываний (для сброса счётчика)

volatile uint16_t	rpmInMinute		= 0;	// Количество оборотов в минуту

ISR(TIM0_OVF_vect)	// Прерывание по переполнению
{
	intCounter++;
}

ISR(INT0_vect)		// Внешнее прерывание
{
	rpmNow++;
}

void writeNumber(uint16_t num) {
	uint8_t	pos = 0;

	if (num == 0)
	{
		displayWritePos(0);
		displayWriteNum(0);
	}

	while (num != 0) {
		displayWritePos(pos);
		displayWriteNum(num % 10);
		num /= 10;
		pos++;
	}
}

void displayWritePos(uint8_t pos)
{
	/*Очистка дисплея Начало*/
	PORTB	&= ~(1 << DS_PIN);

	for (unsigned int i = 0; i < 12; ++i)
	{
		PORTB	|= (1 << SH_PIN);
		PORTB	&= ~(1 << SH_PIN);
	}
	/*Очистка дисплея Конец*/

	PORTB	|= (1 << DS_PIN);
	PORTB	|= (1 << SH_PIN);
	PORTB	&= ~(1 << SH_PIN);
	PORTB	&= ~(1 << DS_PIN);

	for (unsigned int i = 0; i < (3 - pos); ++i)
	{
		PORTB	|= (1 << SH_PIN);
		PORTB	&= ~(1 << SH_PIN);
	}

	PORTB	|= (1 << ST_PIN);	// Закрепление данных
	PORTB	&= ~(1 << ST_PIN);
}

void displayWriteNum(uint8_t number)
{
	uint8_t num	= displayArray[number];

	for (unsigned int i = 0; i < 8; ++i)
	{
		if (num & (1 << i))
		{
			PORTB	|= (1 << DS_PIN);
		}
		else
		{
			PORTB	&= ~(1 << DS_PIN);
		}

		PORTB	|= (1 << SH_PIN);	// Запись данных
		PORTB	&= ~(1 << SH_PIN);
	}

	PORTB	&= ~(1 << DS_PIN);	// Сброс ножки данных
	PORTB	|= (1 << ST_PIN);	// Закрепление данных
	PORTB	&= ~(1 << ST_PIN);
}

int main()
{
	DDRB	&= ~(1 << INT_PIN);
	DDRB	|= (1 << DS_PIN);
	DDRB	|= (1 << SH_PIN);
	DDRB	|= (1 << ST_PIN);
	DDRB	|= (1 << RMP_PIN);

	/*Внешние прерывание*/
	MCUCR	|= (1 << ISC01);	// Внешие прерывание по падению
	MCUCR	&= ~(1 << ISC00);	// Внешие прерывание по падению
	GIMSK	|= (1 << INT0);		// Разрешение внешнего прерывания

	/*инициализация таймера*/
	/*Формула: Частота_таймера=(Частота_процессора/(2*Делитель*OCR0A))-1*/
	TCCR0A	= 0;				// Отключение таймера от пина
	TCCR0B	|= (1 << CS02);		// Пределитель на 256
	OCR0A	= 103;
	TIMSK0	|= (1 << TOIE0);	// Прерывание по переполнению

	sei();

	while (1)
	{
		if (intCounter >= 45)
		{
			cli();
			rpmInMinute	= rpmNow * 120;
			intCounter	= 0;
			rpmNow		= 0;
			sei();
		}

		if (rpmInMinute	>= RPM_CHECK)
		{
			PORTB	|= (1 << RMP_PIN);
		}
		else
		{
			PORTB	&= ~(1 << RMP_PIN);
		}

		writeNumber(rpmInMinute);
	}
}