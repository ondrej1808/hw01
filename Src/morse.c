/*                                                                            *
 * B2M37AMP Microprocessor Applications                                       *
 *                                                                            *
 * Morse encoder                                                              *
 *                                                                            *
 *                                                                            *
 * morse.c, Josef Krska 2026                                                  *
 */
#include "morse.h"

const uint32_t morse_code[] =
{
  ['A'] = 0b11101, /* a-kat */
  ['B'] = 0b101010111, /* bly-ska-vi-ce */
  ['C'] = 0b10111010111, /* ci-lov-ni-ci */
  ['D'] = 0b1010111, /* da-la-va */
  ['E'] = 0b1, /* erb */
  ['F'] = 0b101110101, /* fi-li-al-ka */
  ['G'] = 0b101110111, /* gron-ska-zem */
  ['H'] = 0b1010101, /* har-mo-ni-ka */
  ['I'] = 0b101, /* i-bis */
  ['J'] = 0b1110111011101, /* jas-min-bi-ly */
  ['K'] = 0b111010111, /* ka-vo-vnik */
  ['L'] = 0b101011101, /* lu-pi-ne-cek */
  ['M'] = 0b1110111, /* ma-va */
  ['N'] = 0b10111, /* na-rod */
  ['O'] = 0b11101110111, /* o-nas-pan */
  ['P'] = 0b10111011101, /* po-zar-ni-ci */
  ['Q'] = 0b1110101110111, /* kvi-li-or-kan */
  ['R'] = 0b01011101, /* ra-ra-sek */
  ['S'] = 0b10101, /* sve-tlus-ka */
  ['T'] = 0b111, /* tram */
  ['U'] = 0b1110101, /* u-li-cnik */
  ['V'] = 0b111010101, /* vi-no-bra-ni */
  ['W'] = 0b111011101, /* dvoj-te-ve */
  ['X'] = 0b11101010111, /* Xe-ni-e-ma */
  ['Y'] = 0b1110111010111, /* y-se-ztra-ci */
  ['Z'] = 0b10101110111, /* zpi-va-me-si */
};

#include "stm32f401.h"
#include "uart.h"

#define MORSE_CHAR_GAP  3 
#define MORSE_WORD_GAP  7 

morseTask_t morse; 

#define BUZZER_FREQ_HZ  1000  

static void morse_out_init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;

  GPIOC->BSRR = GPIO_BSRR_BR0;
  GPIOC->MODER &= ~GPIO_MODER_MODER0_Msk;
  GPIOC->MODER |= 1 << GPIO_MODER_MODE0_Pos;

  GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL11_Msk;
  GPIOA->AFR[1] |= 1 << GPIO_AFRH_AFSEL11_Pos;
  GPIOA->MODER &= ~GPIO_MODER_MODER11_Msk;
  GPIOA->MODER |= 2 << GPIO_MODER_MODE11_Pos;

  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  TIM1->PSC = 16 - 1;                       
  TIM1->ARR = 1000000 / BUZZER_FREQ_HZ - 1;
  TIM1->CCR4 = 0;                           
  TIM1->CCMR2 = (6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
  TIM1->CCER = TIM_CCER_CC4E;
  TIM1->BDTR = TIM_BDTR_MOE;
  TIM1->EGR = TIM_EGR_UG;
  TIM1->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

void morse_out_set(uint32_t on, uint8_t tone)
{
  GPIOC->BSRR = on ? GPIO_BSRR_BS0 : GPIO_BSRR_BR0;
  if (!on)
  {
    TIM1->CCR4 = 0;
  }
  else if (tone)
  {
    TIM1->CCR4 = (TIM1->ARR + 1) / 2;
  }
  else
  {
    TIM1->CCR4 = TIM1->ARR + 1;
  }
}

void morse_init(void)
{
  morse_out_init();
  morse.task_period = MORSE_TDIT_MS;
  morse.task_cnt = 1;
  morse.state = msNextChar;
  morse.current_symbol = 0;
  morse.wait = 0;
}

uint32_t morse_task(void)
{
  uint32_t ret = 0;

  switch (morse.state)
  {
    case msIdle:
      if (morse.wait > 0)
      {
        morse.wait--;
      }
      if (morse.wait == 0)
      {
        morse.state = msNextChar;
      }
      break;

    case msNextChar:
    {
      char c;
      if (!uart_getChar(&c))
      {
        break;                
      }
      if (c == '\r' || c == '\n')
      {
        c = ' ';            
      }

      if (c >= 'a' && c <= 'z')
      {
        c = (char)(c - 'a' + 'A');
      }

      if (c >= 'A' && c <= 'Z')
      {
        morse.current_symbol = morse_code[(uint8_t)c];
        morse.state = msChar;
       
        ret = morse.current_symbol & 1;
        morse.current_symbol >>= 1;
        if (morse.current_symbol == 0)
        {
          morse.wait = MORSE_CHAR_GAP;
          morse.state = msIdle;
        }
      }
      else
      {
   
        morse.wait = MORSE_WORD_GAP - MORSE_CHAR_GAP - 1;
        morse.state = msIdle;
      }
      break;
    }

    case msChar:
      ret = morse.current_symbol & 1;
      morse.current_symbol >>= 1;
      if (morse.current_symbol == 0)
      {
        morse.wait = MORSE_CHAR_GAP;
        morse.state = msIdle;
      }
      break;

    default:
      morse.state = msNextChar;
      break;
  }

  return ret;
}
