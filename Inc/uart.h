/* uart - USART2 (PA2 TX, PA3 RX = ST-Link virtualni COM port), 115200 8N1
 *
 * Prijem v preruseni po jednom znaku do kruhoveho bufferu (USART2 ma v HW
 * jen 1 bajt RDR, bez FIFO -> znak se musi vycist do ~87 us, jinak ORE).
 * Vysilani blokujici (polling TXE).
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_BAUD        115200
#define UART_RX_BUF_SIZE 64     /* mocnina 2, max. UART_RX_BUF_SIZE-1 znaku */
#define UART_IRQ_PRIO    2      /* priorita USART2_IRQn (0 = nejvyssi) */

void uart_init(void);

uint8_t uart_getChar(char *c);

void uart_putChar(char c);

void uart_putString(const char *s);

#endif /* UART_H */
