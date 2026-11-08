#include "demorse.h"
#include "stm32f401.h"
#include "stm32f4xx_hal_gpio.h"

#define TICKS_PER_MS   10
#define T_DIT          (DEMORSE_TDIT_MS * TICKS_PER_MS) 
#define TONE_GAP       (5 * TICKS_PER_MS)

#define EV_BUF_SIZE    32
#define CH_BUF_SIZE    16
#define MAX_CODE_BITS  28


extern const uint32_t morse_code[];

typedef struct
{
  uint16_t time;  
  uint8_t  level;  
} edge_t;

static volatile edge_t  ev_buf[EV_BUF_SIZE];
static volatile uint8_t ev_head, ev_tail;

static char    ch_buf[CH_BUF_SIZE];
static uint8_t ch_head, ch_tail;

static demorseState_t state = dmIdle;
static uint8_t  tone_mode;   
static uint16_t seg_start;   
static uint16_t last_edge;     
static uint32_t sym_code;      
static uint8_t  sym_bits;     
static uint8_t  char_done;    

static void put_char(char c)
{
  uint8_t next = ((ch_head + 1) & (CH_BUF_SIZE - 1));
  if (next != ch_tail)
  {
    ch_buf[ch_head] = c;
    ch_head = next;
  }
}

static void finish_char(void)
{
  if (sym_bits > 0)
  {
    char found = '?';
    for (char c = 'A'; c <= 'Z'; c++)
    {
      if (morse_code[(uint8_t)c] == sym_code)
      {
        found = c;
        break;
      }
    }
    put_char(found);
  }
  sym_code = 0;
  sym_bits = 0;
}

static void on_mark(uint16_t len)
{
  if (sym_bits > 0)
  {
    sym_bits++;                       
  }
  if (sym_bits + 3 <= MAX_CODE_BITS)
  {
    if (len >= 2 * T_DIT)
    {
      sym_code |= 0x7 << sym_bits;    
      sym_bits += 3;
    }
    else
    {
      sym_code |= 0x1 << sym_bits;  
      sym_bits += 1;
    }
  }
  else
  {
    sym_code = 0xFFFFFFFFu;
  }
  char_done = 0;
}

static void on_space(uint16_t len)
{
  if (len >= 2 * T_DIT && !char_done)
  {
    finish_char();
    char_done = 1;
  }
  if (len >= 5 * T_DIT)
  {
    put_char(' ');
    state = dmIdle;
  }
}

void demorse_init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

  GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL7_Msk;
  GPIOB->AFR[0] |= GPIO_AF2_TIM4 << GPIO_AFRL_AFSEL7_Pos;
  GPIOB->MODER &= ~GPIO_MODER_MODER7_Msk;
  GPIOB->MODER |= GPIO_MODE_AF_PP << GPIO_MODER_MODE7_Pos;

  TIM4->PSC = 1600 - 1;                  
  TIM4->ARR = 0xFFFF;
  TIM4->CCMR1 = TIM_CCMR1_CC2S_0                       
              | TIM_CCMR1_IC2F_1 | TIM_CCMR1_IC2F_0;  
  TIM4->CCER = TIM_CCER_CC2E | TIM_CCER_CC2P | TIM_CCER_CC2NP;
  TIM4->EGR = TIM_EGR_UG;
  TIM4->SR = 0;
  TIM4->DIER = TIM_DIER_CC2IE;
  TIM4->CR1 = TIM_CR1_CEN;

  NVIC_SetPriority(TIM4_IRQn, 1);
  NVIC_EnableIRQ(TIM4_IRQn);
}

void TIM4_IRQHandler(void)
{
  if (TIM4->SR & TIM_SR_CC2IF)
  {
    uint16_t t = (uint16_t)TIM4->CCR2;
    uint8_t next = (uint8_t)((ev_head + 1) & (EV_BUF_SIZE - 1));
    if (next != ev_tail)
    {
      ev_buf[ev_head].time = t;
      ev_buf[ev_head].level = (GPIOB->IDR & GPIO_IDR_ID7) ? 1 : 0;
      ev_head = next;
    }
  }
  TIM4->SR = ~TIM_SR_CC2OF;
}

static void on_edge(uint16_t t, uint8_t level)
{
  if (!tone_mode)
  {
    if (level)
    {
      if (state == dmSpace)
      {
        on_space((uint16_t)(t - seg_start));
      }
      state = dmMark;
      seg_start = t;
    }
    else if (state == dmMark)
    {
      on_mark((uint16_t)(t - seg_start));
      state = dmSpace;
      seg_start = t;
    }
  }
  else
  {
    if (state != dmMark)
    {
      if (state == dmSpace)
      {
        on_space((t - seg_start));
      }
      state = dmMark;
      seg_start = t;
    }
    last_edge = t;
  }
}

void demorse_task(void)
{
  while (ev_tail != ev_head)
  {
    on_edge(ev_buf[ev_tail].time, ev_buf[ev_tail].level);
    ev_tail = (uint8_t)((ev_tail + 1) & (EV_BUF_SIZE - 1));
  }

  uint16_t now = (uint16_t)TIM4->CNT;

  if (tone_mode && state == dmMark && (uint16_t)(now - last_edge) > TONE_GAP)
  {
    on_mark((uint16_t)(last_edge - seg_start));
    state = dmSpace;
    seg_start = last_edge;
  }

  if (state == dmSpace)
  {
    on_space((uint16_t)(now - seg_start));
  }
}

char demorse_getChar(void)
{
  if (ch_tail == ch_head)
  {
    return 0;
  }
  char c = ch_buf[ch_tail];
  ch_tail = (uint8_t)((ch_tail + 1) & (CH_BUF_SIZE - 1));
  return c;
}

void demorse_setTone(uint8_t tone)
{
  tone_mode = tone ? 1 : 0;
  state = dmIdle;
  sym_code = 0;
  sym_bits = 0;
}

uint8_t demorse_isTone(void)
{
  return tone_mode;
}
