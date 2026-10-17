/* Minimal control interface for the LCD1602 LCD display. */
#pragma once

#include <stdint.h>

#include "stm32f407xx.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} lcd1602_pin_t;

typedef struct {
    lcd1602_pin_t rs;
    lcd1602_pin_t enable;

    /* Data pins, ordered DB4, DB5, DB6, DB7 */
    lcd1602_pin_t data[4];
} lcd1602_config_t;

void lcd1602_init(const lcd1602_config_t *config);

void lcd1602_clear(void);
void lcd1602_home(void);

void lcd1602_set_cursor(uint8_t row, uint8_t col);

void lcd1602_write_char(char c);
void lcd1602_write_string(const char *str);

