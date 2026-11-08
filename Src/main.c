#include <stdint.h>
#include "stm32f401.h"
#include "seg7.h"
#include "demorse.h"
#include "morse.h"
#include "uart.h"

#define CORE_CLOCK_HZ  16000000

static volatile uint32_t uiSysTick; 
static volatile uint8_t  FL_SysTick;  

static void systick_init(void)
{
  SysTick_Config(CORE_CLOCK_HZ / 1000);
  NVIC_SetPriority(SysTick_IRQn, 3);   
}

void SysTick_Handler(void)
{
  uiSysTick++;
  FL_SysTick = 1;
}

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

static void morse_out_set(uint32_t on)
{
  GPIOC->BSRR = on ? GPIO_BSRR_BS0 : GPIO_BSRR_BR0;
  if (!on)
  {
    TIM1->CCR4 = 0;
  }
  else if (demorse_isTone())
  {
    TIM1->CCR4 = (TIM1->ARR + 1) / 2;
  }
  else
  {
    TIM1->CCR4 = TIM1->ARR + 1;
  }
}

static void sw2_task(void)
{
  static uint8_t cnt;
  static uint8_t pressed;

  if (!(GPIOA->IDR & GPIO_IDR_ID1))
  {
    if (cnt < 20)
    {
      cnt++;
    }
    else if (!pressed)
    {
      pressed = 1;
      demorse_setTone(!demorse_isTone());
      seg7_print_text(demorse_isTone() ? "tonE" : "PULS");
    }
  }
  else
  {
    cnt = 0;
    pressed = 0;
  }
}

int main(void)
{
  init_seg7();
  demorse_init();
  morse_init();
  morse_out_init();
  uart_init();
  systick_init();

 
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  GPIOA->MODER &= ~GPIO_MODER_MODER1_Msk;

  seg7_print_text("PULS");

  while (1)
  {
    if (FL_SysTick)
    {
      FL_SysTick = 0;
      seg7_print_loop();

      if (--morse.task_cnt == 0)
      {
        morse.task_cnt = morse.task_period;
        morse_out_set(morse_task());
      }
      demorse_task();
      sw2_task();
    }

    char znak = demorse_getChar();
    if (znak)
    {
      uart_putChar(znak);
    }
  }
}
