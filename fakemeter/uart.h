#ifndef FAKEMETER_UART_H
#define FAKEMETER_UART_H

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

static inline void uart_rx_enable(void)
{
	UCSR0B |= (1 << RXEN0);
}

static inline void uart_rx_disable(void)
{
	UCSR0B &= ~(1 << RXEN0);
}

static inline uint8_t uart_rx(void)
{
	while(!(UCSR0A & (1 << RXC0)))
		;
	return UDR0;
}

/* Returns a negative number if no character is available to read */
static inline int16_t uart_rx_noblock(void)
{
	if(!(UCSR0A & (1 << RXC0))) return -1;
	return (uint16_t)UDR0;
}

static inline void uart_tx(uint8_t byte)
{
	while(!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = byte;
}

void uart_puts(char const *string);

#endif