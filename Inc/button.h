#ifndef BUTTON_H
#define BUTTON_H

/* SW2 (BTN2) = PA1, stisk = 0 (pull-up na desce) */

void button_init(void);

/* volat kazdou 1 ms: stisk SW2 prepne dekoder pulz/ton a zobrazi PULS/tonE */
void button_task(void);

#endif /* BUTTON_H */
