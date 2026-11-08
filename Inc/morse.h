/*                                                                            *
 * B2M37AMP Microprocessor Applications                                       *
 *                                                                            *
 * Morse encoder                                                              *
 *                                                                            *
 *                                                                            *
 * morse.h, Josef Krska 2026                                                  *
 */
#ifndef MORSE_H_HDR
#define MORSE_H_HDR

#include <stdint.h>

#define MORSE_TDIT_MS 120

typedef enum morseState
{
  msIdle, msChar, msNextChar
} morseState;

typedef struct morseTask_s
{
  uint32_t task_period,
           task_cnt;

  morseState state;
  uint32_t current_symbol;
  uint32_t wait;
} morseTask_t;

extern morseTask_t morse;

void morse_init(void);

uint32_t morse_task(void);

#endif /* MORSE_H_HDR */