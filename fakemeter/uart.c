#include "uart.h"

void uart_puts(char const *string)
{
	while(*string)
	{
		uart_tx(*string++);
	}
}