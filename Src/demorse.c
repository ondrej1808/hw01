#include "demorse.h"
#include "stm32f401.h"
#include "stm32f4xx_hal_gpio.h"

#define TICKS_PER_MS   10
#define T_DIT          (DEMORSE_TDIT_MS * TICKS_PER_MS)
#define TONE_GAP       (5 * TICKS_PER_MS)

#define EDGE_RISING    1   // nabezna hrana: PB7 je po hrane v 1
#define EDGE_FALLING   0   // sestupna hrana: PB7 je po hrane v 0

#define CH_BUF_SIZE    16
#define MAX_CODE_BITS  28


extern const uint32_t morse_code[];

// posledni hrana z preruseni (demorse_task ji vyzvedne do 1 ms)
static volatile uint16_t edge_time;    // CCR2 v okamziku hrany
static volatile uint8_t  edge_level;   // EDGE_RISING / EDGE_FALLING
static volatile uint8_t  edge_new_flag;     // 1 = hrana ceka na zpracovani

static char    ch_buf[CH_BUF_SIZE];
static uint8_t ch_head, ch_tail;

static demorseState_t demorse_state = dmIdle;
static demorseMode_t demorse_mode = MODE_PULSE;
static uint16_t seg_start;
static uint16_t last_edge;
static uint32_t char_code;       // rozpracovany znak ve formatu morse_code[] (tecka 1, carka 111, mezera 0)
static uint8_t  char_code_len;   // kolik bitu char_code uz je zapsano = pozice pro dalsi symbol
static uint8_t  char_done;

static void on_edge(uint16_t t, uint8_t edge_type);
static void on_mark(uint16_t len);
static void on_space(uint16_t len);
static void finish_char(void);
static void put_char(char c);


// ---------------------------- inicializace a rezim ----------------------------

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
  TIM4->CCMR1 = TIM_CCMR1_CC2S_0 | TIM_CCMR1_IC2F_1 | TIM_CCMR1_IC2F_0;
  TIM4->CCER = TIM_CCER_CC2E | TIM_CCER_CC2P | TIM_CCER_CC2NP;
  TIM4->EGR = TIM_EGR_UG;
  TIM4->SR = 0;
  TIM4->DIER = TIM_DIER_CC2IE;
  TIM4->CR1 = TIM_CR1_CEN;

  NVIC_SetPriority(TIM4_IRQn, 1);
  NVIC_EnableIRQ(TIM4_IRQn);
}

void demorse_setMode(demorseMode_t mode)
{
  demorse_mode = mode;
  demorse_state = dmIdle;
  char_code = 0;
  char_code_len = 0;
}

demorseMode_t demorse_getMode(void)
{
  return demorse_mode;
}


// ---------------------------- preruseni: zachyceni hrany ----------------------------

void TIM4_IRQHandler(void)
{
  if (TIM4->SR & TIM_SR_CC2IF)
  {
    edge_time  = (uint16_t)TIM4->CCR2;   // cteni CCR2 nuluje CC2IF
    edge_level = (GPIOB->IDR & GPIO_IDR_ID7) ? EDGE_RISING : EDGE_FALLING;
    edge_new_flag = 1;
  }
}


// ---------------------------- periodicka uloha (1 ms) ----------------------------

void demorse_task(void)
{
  if (edge_new_flag)
  {
    // cas a uroven precist naraz, at je preruseni mezi nimi neprepise
    __disable_irq();
    uint16_t t = edge_time;
    uint8_t  level = edge_level;
    edge_new_flag = 0;
    __enable_irq();

    on_edge(t, level);
  }

  uint16_t now = (uint16_t)TIM4->CNT;

  if (demorse_mode == MODE_TONE && demorse_state == dmMark && (uint16_t)(now - last_edge) > TONE_GAP)
  {
    on_mark((uint16_t)(last_edge - seg_start));
    demorse_state = dmSpace;
    seg_start = last_edge;
  }

  if (demorse_state == dmSpace)
  {
    on_space((uint16_t)(now - seg_start));
  }
}


// ---------------------------- zpracovani hrany ----------------------------

static void on_edge(uint16_t t, uint8_t edge_type)
{
  if (demorse_mode == MODE_PULSE)
  {
    if (edge_type == EDGE_RISING)
    {
      if (demorse_state == dmSpace)
      {
        on_space((uint16_t)(t - seg_start));
      }
      demorse_state = dmMark;
      seg_start = t;
    }
    else if (demorse_state == dmMark)
    {
      on_mark((uint16_t)(t - seg_start));
      demorse_state = dmSpace;
      seg_start = t;
    }
  }
  else
  {
    if (demorse_state != dmMark)
    {
      if (demorse_state == dmSpace)
      {
        on_space((t - seg_start));
      }
      demorse_state = dmMark;
      seg_start = t;
    }
    last_edge = t;
  }
}


// ---------------------------- vyhodnoceni delek ----------------------------

static void on_mark(uint16_t len)
{
  if (char_code_len > 0)
  {
    char_code_len++;
  }
  if (char_code_len + 3 <= MAX_CODE_BITS)
  {
    if (len >= 2 * T_DIT)
    {
      char_code |= 0x7 << char_code_len;
      char_code_len += 3;
    }
    else
    {
      char_code |= 0x1 << char_code_len;
      char_code_len += 1;
    }
  }
  else
  {
    char_code = 0xFFFFFFFF;
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
    demorse_state = dmIdle;
  }
}


// ---------------------------- vystup znaku ----------------------------

static void finish_char(void)
{
  if (char_code_len > 0)
  {
    char found = '?';
    for (char c = 'A'; c <= 'Z'; c++)
    {
      if (morse_code[(uint8_t)c] == char_code)
      {
        found = c;
        break;
      }
    }
    put_char(found);
  }
  char_code = 0;
  char_code_len = 0;
}

static void put_char(char c)
{
  uint8_t next = ((ch_head + 1) & (CH_BUF_SIZE - 1));
  if (next != ch_tail)
  {
    ch_buf[ch_head] = c;
    ch_head = next;
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
