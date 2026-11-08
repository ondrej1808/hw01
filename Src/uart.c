#include "uart.h"
#include "stm32f401.h"

#define UART_CLOCK_HZ 16000000   /* APB1 = HSI 16 MHz (bez PLL) */

static volatile char    rx_buf[UART_RX_BUF_SIZE];
static volatile uint8_t rx_head;  /* zapisuje preruseni */
static volatile uint8_t rx_tail;  /* cte main */

void uart_init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

  // PA2 TX, PA3 RX -> AF7
  GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2_Msk | GPIO_AFRL_AFSEL3_Msk);
  GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos);
  GPIOA->MODER &= ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER3_Msk);
  GPIOA->MODER |= (2 << GPIO_MODER_MODE2_Pos) | (2 << GPIO_MODER_MODE3_Pos);

  // oversampling 16: BRR = f / baud (zaokrouhleno)
  USART2->BRR = (UART_CLOCK_HZ + UART_BAUD / 2) / UART_BAUD;
  USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
  USART2->CR1 |= USART_CR1_UE;

  NVIC_SetPriority(USART2_IRQn, UART_IRQ_PRIO);
  NVIC_EnableIRQ(USART2_IRQn);
}

void USART2_IRQHandler(void)
{
  uint32_t sr = USART2->SR;

  if (sr & (USART_SR_RXNE | USART_SR_ORE))
  {
    char c = (char)USART2->DR;
    uint8_t next = (uint8_t)((rx_head + 1) & (UART_RX_BUF_SIZE - 1));
    if (next != rx_tail)
    {
      rx_buf[rx_head] = c;
      rx_head = next;
    }
  }
}

uint8_t uart_getChar(char *c)
{
  if (rx_tail == rx_head)
  {
    return 0;
  }
  *c = rx_buf[rx_tail];
  rx_tail = (uint8_t)((rx_tail + 1) & (UART_RX_BUF_SIZE - 1));
  return 1;
}

void uart_putChar(char c)
{
  while (!(USART2->SR & USART_SR_TXE))
  {
  }
  USART2->DR = (uint8_t)c;
}

void uart_putString(const char *s)
{
  while (*s)
  {
    uart_putChar(*s++);
  }
}
