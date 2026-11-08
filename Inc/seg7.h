
/*
    Knihovna pro 7 segmentový displej ovládaný přes shift registr
    Autor: Bc. Ondřej Hlaváček 
    Máme hodně signálů:
    - PC10 Disp_clock (SHCP)
    - PC12 Disp_Data  (DS)
    - PA15 LED_RCLK
    - PC8 LED_SRCLR
    - PC9 LED_OE_FIFO
    - PB1 LED_OE_BUFFER
*/

#ifndef _SEG7_H
#define _SEG7_H

#include <stdint.h>
#include <stdbool.h>

#define SEG7_BRIGHTNESS_MAX 100

void init_seg7(void);

void seg7_print_raw(uint32_t message);

void seg7_print_loop(void); 

void seg7_print_digit(uint8_t pos, uint8_t num);

void seg7_print_num(uint16_t num);

void seg7_set_brightness(uint8_t brightness);

uint8_t seg7_charToSegments(char c);

void seg7_print_char(uint8_t pos, char c);

void seg7_print_text(const char *text);

void seg7_write_data(bool state);

void seg7_shift(void);

void seg7_writeAndShift(bool state);

void seg7_output_buffer_write(void);

void seg7_clear_registers(void);

#endif // _SEG7_H