#include "dl.h"
#include "uart.h"

static uint8_t dl_csum;

void dl_begin(void)
{
	dl_csum = 0;
}

void dl_tx(uint8_t byte)
{
	dl_csum ^= byte;
	uart_tx(byte);
}

void dl_puts(char const *string)
{
	while(*string)
	{
		dl_csum ^= *string;
		uart_tx(*string++);
	}
}

uint8_t dl_get_csum(void)
{
	return dl_csum;
}