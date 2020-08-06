#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "dl.h"
#include "uart.h"

/* Must be 3 characters */
#define MANUFACTURER "AAA"
/* For Mode C: 0-6. For Mode B: A-F. For Mode A: anything else */
#define BAUD_ID "5"
#define PRODUCT "FAKE01-1234"

/* Mode selection, uncomment only one of these */
#define USE_MODE_C
// #define USE_MODE_B
// #define USE_MODE_A

#define IDENTIFICATION "/" MANUFACTURER BAUD_ID PRODUCT "\r\n"

struct baud_setting
{
	_Bool use_2x;
	uint16_t ubrr;
};

/* TODO improve? */
struct baud_setting const BAUD_SETTINGS[] = {
#define BAUD 300
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
#define BAUD 600
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
#define BAUD 1200
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
#define BAUD 2400
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
#define BAUD 4800
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
#define BAUD 9600
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
#define BAUD 19200
#include <util/setbaud.h>
    {USE_2X, UBRR_VALUE},
#undef BAUD
};

static _Bool set_baud(uint8_t identifier)
{
	struct baud_setting const *set = 0;

	if(identifier >= '0' && identifier <= '6') /* Mode C */
		set = &BAUD_SETTINGS[identifier - '0'];
	else if(identifier >= 'A' && identifier <= 'F') /* Mode B */
		set = &BAUD_SETTINGS[identifier - 'A' + 1];

	if(!set) return false;

	if(set->use_2x)
		UCSR0A |= (1 << U2X0);
	else
		UCSR0A &= ~(1 << U2X0);

	UBRR0 = set->ubrr;

	return true;
}

#define MAX_VALUE_LENGTH (32 + 1 + 16)

void format_constant(char *out, void const *user_data)
{
	strlcpy(out, (char const *)user_data, MAX_VALUE_LENGTH);
}

struct format_random_arg
{
	char const *fmt;
	uint16_t min;
	uint16_t max;
};

void format_random(char *out, void const *user_data)
{
	struct format_random_arg const *arg = (struct format_random_arg const *)user_data;
	uint16_t num = rand() % (arg->max - arg->min) + arg->min;
	snprintf(out, MAX_VALUE_LENGTH, arg->fmt, num);
}

struct object
{
	char const *obis_code;
	void (*formatter)(char *out, void const *user_data);
	void *user_data;
} const objects[] = {
    {"15.7.0", format_random, &(struct format_random_arg){"00.%04" PRIu16, 1000, 5000}},
    {"15.8.1", format_random, &(struct format_random_arg){"%08" PRIu16 ".00", 5000, 9000}},
    {"15.8.2", format_random, &(struct format_random_arg){"%08" PRIu16 ".00", 2000, 6000}},
    {"32.7.0", format_random, &(struct format_random_arg){"%04" PRIu16 ".0", 235, 245}},
    {"52.7.0", format_random, &(struct format_random_arg){"%04" PRIu16 ".0", 235, 245}},
    {"72.7.0", format_random, &(struct format_random_arg){"%04" PRIu16 ".0", 235, 245}},
    {"31.7.0", format_random, &(struct format_random_arg){"000.%02" PRIu16, 1, 99}},
    {"51.7.0", format_random, &(struct format_random_arg){"000.%02" PRIu16, 1, 99}},
    {"71.7.0", format_random, &(struct format_random_arg){"000.%02" PRIu16, 1, 99}},
    {0}};

void tx_object(struct object const *obj)
{
	char value[MAX_VALUE_LENGTH];
	obj->formatter(value, obj->user_data);

	dl_puts(obj->obis_code);
	dl_puts("(");
	dl_puts(value);
	dl_puts(")\r\n");
}

/* Opening message with an address specified is not supported */
#define OPENING_MESSAGE "/?!\r\n"
#define OPENING_MESSAGE_LEN (sizeof(OPENING_MESSAGE) - 1)
#define RECEIVE_TIMEOUT 2000 /* ms */

#define STX '\x02'
#define ACK '\x06'

#define ETX "\x03"
#define DATASET_END "!\r\n" ETX

#define ERR_LED_FLASHES 5
#define ERR_LED_FLASH_DURATION 100 /* ms */

static void err_led_setup(void)
{
	DDRB |= (1 << 5);
}

static void err_led_on(void)
{
	PORTB |= (1 << 5);
}

static void err_led_off(void)
{
	PORTB &= ~(1 << 5);
}

static void err(void)
{
	for(size_t i = 0; i < ERR_LED_FLASHES; ++i)
	{
		err_led_on();
		_delay_ms(ERR_LED_FLASH_DURATION);
		err_led_off();
		_delay_ms(ERR_LED_FLASH_DURATION);
	}
}

int main(void)
{
	err_led_setup();

	/* 7E1, transmit enabled */
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UPM01) | (1 << UCSZ01);

	for(;;)
	{
		uint8_t opening_idx = 0;
		uint16_t countdown = RECEIVE_TIMEOUT;

		set_baud('0'); /* Start at 300 bps */
		uart_rx_enable();

		while(opening_idx < OPENING_MESSAGE_LEN && countdown > 0)
		{
			int16_t chr = uart_rx_noblock();
			if(chr < 0)
			{
				if(opening_idx > 0)
				{
					--countdown; /* Only start counting down after the first character */
					_delay_ms(1);
				}
				continue;
			}

			if(chr == OPENING_MESSAGE[opening_idx])
			{
				++opening_idx;
			}
			else
			{
				err();
				opening_idx = 0;
			}
		}

		if(countdown == 0)
		{
			err();
			continue;
		}

		uart_rx_disable();
		uart_puts(IDENTIFICATION);
		uart_rx_enable();

#if defined(USE_MODE_C)
		countdown = RECEIVE_TIMEOUT;
		uint8_t vzy[3];
		uint8_t read_idx = 0;

		/* Read option select message for protocol mode C.
		 * The complete message must be received within the timeout. */
		while(countdown--)
		{
			_delay_ms(1);

			int16_t byte = uart_rx_noblock();
			if(byte < 0) continue;

			if(read_idx == 5 && ++read_idx && byte != '\n')
			{
				err();
				break;
			}
			else if(read_idx == 4 && ++read_idx && byte != '\r')
			{
				err();
				break;
			}
			else if(read_idx >= 1 && read_idx <= 3)
			{
				vzy[read_idx++ - 1] = byte;
			}
			else if(read_idx == 0 && ++read_idx && byte != ACK)
			{
				err();
				break;
			}

			if(read_idx == 6)
			{
				/* Only data readout mode (V=Y=0) is supported */
				if(vzy[0] != '0' || !set_baud(vzy[1]) || vzy[2] != '0')
				{
					err();
					read_idx = 5; /* Set read_idx to something invalid to signal the error */
				}
				break;
			}
		}

		/* Only transmit the dataset if a full option select message was read, or none at all */
		if(read_idx != 0 && read_idx != 6) continue;
#elif defined(USE_MODE_B)
		_delay_ms(66);
		set_baud(BAUD_ID[0]); /* Just change baud without requiring an acknowledgement */
#endif

#if !defined(USE_MODE_A)
		_delay_ms(500); /* Wait "some" time for the HHU to become ready after changing speed */
#endif

		uart_rx_disable();
		dl_begin();
		uart_tx(STX); /* The beginning STX character is not included in the checksum */

		for(struct object const *obj = objects; obj->obis_code; ++obj)
		{
			tx_object(obj);
		}

		dl_puts(DATASET_END);
		uart_tx(dl_get_csum());
		_delay_ms(33); /* could wait until TX is done here but that's complicated, so just wait the worst case delay */
	}

	return 0;
}
