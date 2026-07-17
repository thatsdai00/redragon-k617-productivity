#pragma once

#include <stdint.h>

void    matrix_init();
uint8_t matrix_task();
void    matrix_scan_step();
uint8_t matrix_get_col(uint8_t col);
uint8_t matrix_get_layer(void);
