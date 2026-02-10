#pragma once
#include <stdint.h>

int open_spi(void);
void close_spi(void);
int send_spi(const uint8_t *sendBuf, uint8_t sendSize);
bool write_then_read(const uint8_t *write_buffer, size_t write_len, uint8_t *read_buffer, size_t read_len, uint8_t sendvalue = 0xFF);

int board_init ();
void board_select (int isOn);