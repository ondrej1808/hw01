#include <stdint.h>
#include "stm32f401.h"
#include "seg7.h"
#include "demorse.h"
#include "morse.h"
#include "uart.h"
#include "button.h"

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

int main(void)
{
  init_seg7();
  demorse_init();
  morse_init();
  uart_init();
  button_init();
  systick_init();

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
        morse_out_set(morse_task(), demorse_isTone());
      }
      demorse_task();
      button_task();
    }

    char znak = demorse_getChar();
    if (znak)
    {
      uart_putChar(znak);
    }
  }
}
