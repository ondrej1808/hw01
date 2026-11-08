#include "button.h"
#include "stm32f401.h"
#include "stm32f4xx_hal_gpio.h"
#include "demorse.h"
#include "seg7.h"

#define DEBOUNCE_MS 20

void button_init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  GPIOA->MODER &= ~GPIO_MODER_MODER1_Msk;
  GPIOA->MODER |= GPIO_MODE_INPUT << GPIO_MODER_MODE1_Pos;   // PA1 vstup
}

void button_task(void)
{
  static uint8_t cnt;
  static uint8_t pressed;

  if (!(GPIOA->IDR & GPIO_IDR_ID1))   // stisknuto
  {
    if (cnt < DEBOUNCE_MS)
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
