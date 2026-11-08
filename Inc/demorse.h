/* demorse - dekoder Morseova kodu (HW01)
 *
 * Vstup: PB7 = F_MEASURE = TIM4_CH2 (AF2), jumper P4.
 * Test: propoj vystup koderu z cviceni (P3 nebo P6) na P4, nebo dve desky.
 *
 * Casovani (jednotky T_dit = 120 ms): dit 1, dah 3, mezera symbol 1, znak 3, slovo 7.
 * Pro toleranci rozhoduj prahy uprostred: <2 T = dit, >=2 T = dah; mezera <2 T / <5 T / >=5 T.
 */
#ifndef DEMORSE_H
#define DEMORSE_H

#include <stdint.h>

#define DEMORSE_TDIT_MS 120

typedef enum
{
  dmIdle,     
  dmMark,
  dmSpace
} demorseState_t;

void demorse_init(void);

void demorse_task(void);

char demorse_getChar(void);

void demorse_setTone(uint8_t tone);

uint8_t demorse_isTone(void);

#endif /* DEMORSE_H */
