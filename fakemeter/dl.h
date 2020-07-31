#ifndef FAKEMETER_DL_H
#define FAKEMETER_DL_H

#include <stdint.h>

/* Resets the checksum state at the beginning of a new data block. */
void dl_begin(void);

void dl_tx(uint8_t byte);
void dl_puts(char const *string);

/* Gets the accumulated checksum value without resetting it. */
uint8_t dl_get_csum(void);

#endif