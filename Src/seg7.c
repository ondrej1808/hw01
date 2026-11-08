#include "seg7.h"
#include "stm32f401.h"

#define SEG7_DIGITS 4

static const uint8_t seg7_font[16] =
{
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, /* 0..7 */
  0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71  /* 8..F */
};

static const uint8_t seg7_digit_sel[SEG7_DIGITS] = { 0x10, 0x20, 0x40, 0x80 };

static volatile uint8_t seg7_buf[SEG7_DIGITS];
static uint8_t seg7_brightness = SEG7_BRIGHTNESS_MAX;
static uint8_t seg7_pos;


void seg7_write_data(bool state)
{
  GPIOC->BSRR = state ? GPIO_BSRR_BS12 : GPIO_BSRR_BR12;
}

void seg7_shift(void)
{
  GPIOC->BSRR = GPIO_BSRR_BS10;
  GPIOC->BSRR = GPIO_BSRR_BR10;
}

void seg7_writeAndShift(bool state)
{
  seg7_write_data(state);
  seg7_shift();
}

void seg7_output_buffer_write(void)
{
  GPIOA->BSRR = GPIO_BSRR_BS15;
  GPIOA->BSRR = GPIO_BSRR_BR15;
}

void seg7_clear_registers(void)
{
  GPIOC->BSRR = GPIO_BSRR_BR8;
  GPIOC->BSRR = GPIO_BSRR_BS8;
  seg7_output_buffer_write(); 
}

void init_seg7(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

  GPIOC->BSRR = GPIO_BSRR_BS8 | GPIO_BSRR_BR9 | GPIO_BSRR_BR10 | GPIO_BSRR_BR12;
  GPIOA->BSRR = GPIO_BSRR_BR15;

  GPIOC->MODER &= ~(GPIO_MODER_MODE8_Msk | GPIO_MODER_MODE9_Msk |
                    GPIO_MODER_MODE10_Msk | GPIO_MODER_MODE12_Msk);
  GPIOC->MODER |= (1 << GPIO_MODER_MODE8_Pos) | (1 << GPIO_MODER_MODE9_Pos) |
                  (1 << GPIO_MODER_MODE10_Pos) | (1 << GPIO_MODER_MODE12_Pos);
  GPIOA->MODER &= ~GPIO_MODER_MODE15_Msk;
  GPIOA->MODER |= 1 << GPIO_MODER_MODE15_Pos;

  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  TIM3->PSC = 8 - 1;
  TIM3->ARR = SEG7_BRIGHTNESS_MAX - 1;
  TIM3->CCR4 = seg7_brightness;
  TIM3->CCMR2 = (6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
  TIM3->CCER = TIM_CCER_CC4E | TIM_CCER_CC4P;
  TIM3->EGR = TIM_EGR_UG;
  TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;

  GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL1_Msk;
  GPIOB->AFR[0] |= 2 << GPIO_AFRL_AFSEL1_Pos;
  GPIOB->MODER &= ~GPIO_MODER_MODE1_Msk;
  GPIOB->MODER |= 2 << GPIO_MODER_MODE1_Pos;

  seg7_clear_registers();
}




static void seg7_send(uint8_t digit, uint8_t segments)
{
  uint16_t frame = ((uint16_t)digit << 8) | segments;
  for (uint8_t i = 0; i < 16; i++)
  {
    seg7_writeAndShift((frame >> (15 - i)) & 1);
  }
  seg7_output_buffer_write();
}

void seg7_set_brightness(uint8_t brightness)
{
  seg7_brightness = (brightness > SEG7_BRIGHTNESS_MAX) ? SEG7_BRIGHTNESS_MAX : brightness;
  TIM3->CCR4 = seg7_brightness; 
}

void seg7_print_raw(uint32_t message)
{
  for (uint8_t pos = 0; pos < SEG7_DIGITS; pos++)
  {
    seg7_buf[pos] = (uint8_t)(message >> (8 * (SEG7_DIGITS - 1 - pos)));
  }
}

void seg7_print_loop(void)
{
  seg7_send(seg7_digit_sel[seg7_pos], seg7_buf[seg7_pos]);
  seg7_pos = (seg7_pos + 1) % SEG7_DIGITS;
}

void seg7_print_digit(uint8_t pos, uint8_t num)
{
  if (pos < SEG7_DIGITS)
  {
    seg7_buf[pos] = seg7_font[num & 0x0F];
  }
}

void seg7_print_num(uint16_t num)
{
  for (int8_t pos = SEG7_DIGITS - 1; pos >= 0; pos--)
  {
    seg7_print_digit((uint8_t)pos, num % 10);
    num /= 10;
  }
}

uint8_t seg7_charToSegments(char c)
{
  if (c >= '0' && c <= '9') return seg7_font[c - '0'];
  if (c >= 'A' && c <= 'F') return seg7_font[c - 'A' + 10];
  if (c >= 'a' && c <= 'f') return seg7_font[c - 'a' + 10];

  switch (c)
  {
    case 'P': 
      return 0b01110011;
      break;
    case 'p':
      return 0b01110011;
      break;  
    case 'U':
      return 0b00111110;
      break;
    case 'u':
      return 0b00011100;
      break;
    case 'L':
      return 0b00111000;
      break;
    case 'l':
      return 0b00111000;
      break;
    case 'S':
      return 0b01101101;
      break;
    case 's':
      return 0b01101101;
      break;
    case 't':
      return 0b01111000;
      break;
    case 'T':
      return 0b01111000;
      break;
    case 'o':
      return 0b01011100;
      break;
    case 'O':
      return 0b01011100;
      break;
    case 'n':
      return 0b01010100;
      break;
    case 'N':
      return 0b01010100;
      break;
    case '-':
      return 0b01000000;
      break;
    default:
      return 0b00000000;
      break;
  }
}

void seg7_print_char(uint8_t pos, char c)
{
  if (pos < SEG7_DIGITS)
  {
    seg7_buf[pos] = seg7_charToSegments(c);
  }
}

void seg7_print_text(const char *text)
{
  for (uint8_t pos = 0; pos < SEG7_DIGITS; pos++)
  {
    seg7_print_char(pos, *text);
    if (*text)
    {
      text++;
    }
  }
}
