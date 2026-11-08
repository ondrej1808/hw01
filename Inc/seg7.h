
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

/* Jas 0..100 % (SEG7_BRIGHTNESS_MAX), PWM na PB1 (/OE budice, TIM3_CH4).
   Bez zavolani seg7_set_brightness() sviti na 100 %. */
#define SEG7_BRIGHTNESS_MAX 100


// Kašlem na clock, uděláme bit banging, 595 shift registr by to měl stihnout
void init_seg7(void);

// -------------------------- "High level" abstrakce nesahame na piny ----------------------

/*
    zapíše všechny digity message je rozdělený zleva doprava prvních osum bitu prvni digit atd.
*/
void seg7_print_raw(uint32_t message);

/*
    Multiplexing digitu, tohle bude nejvetsi oser
*/
void seg7_print_loop(void);   /* volat periodicky, napr. 1 ms ze SysTicku */

/*
    na jaky digit a jake cislo chceme zapsat
*/
void seg7_print_digit(uint8_t pos, uint8_t num);

/*
    zapíše na displej číslo
*/
void seg7_print_num(uint16_t num);

/*
    nastavi jas celeho displeje 0..100 %, vychozi 100
*/
void seg7_set_brightness(uint8_t brightness);

/*
    prevede znak (0-9, A-F, P U L S t o n E, '-', mezera) na segmenty, neznamy -> 0
*/
uint8_t seg7_charToSegments(char c);

/*
    na jaky digit (0 = vlevo) a jaky znak chceme zapsat
*/
void seg7_print_char(uint8_t pos, char c);

/*
    zapise 4 znaky, napr. seg7_print_text("PULS"), kratsi text doplni mezerami
*/
void seg7_print_text(const char *text);

// -------------------------- Ovladani pinu ---------------------------

/*
    Zapíše na data linku (abych nemusel myslet na jakým pinu to je)
*/
void seg7_write_data(bool state);
/*
    Pošle 010 na shift hodiny 
*/
void seg7_shift(void);

/*
    Zapíše bit a posune 
*/
void seg7_writeAndShift(bool state);


/*
    přenese z posuvného registru na buffer, pošle 010 na LED_RCLK
*/
void seg7_output_buffer_write(void);

void seg7_clear_registers(void);

#endif // _SEG7_H