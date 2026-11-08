# HW01 – Dekódování Morseova kódu (AMP, 2+3 b, odevzdání 18. 11. 2026 23:59)

Zadání: https://cw.fel.cvut.cz/wiki/courses/amp_mam/homeworks/01

- T_dit = 120 ms, kit AMP-DK, vstup dekodéru **PB7** (F_MEASURE, TIM4_CH2, jumper P4)
- test: výstup kodéru ze cvičení (P3 nebo P6) → P4, nebo dvě desky
- ON-OFF symboly (2 b), symboly jako tón (3 b), nebo obojí
- tip: časovač (input capture) + FSM

Vzorový program (stažen z CW 5. 11. 2026): `docs/HW 01 DeMorse example.elf` + `docs/popis.txt`
(kodér PC0 ON-OFF, P3.2 buzzer; dekodér P4.1 F Measure; UART 115200n8; SW2 přepíná pulz/tón).
Flash referenčního .elf: `STM32_Programmer_CLI -c port=SWD -w "docs/HW 01 DeMorse example.elf" -v -rst`

Struktura: `Src/demorse.c` (dekodér, TODO), `Src/seg7.c` (displej – zkopírovat hotovou verzi z ../cv01), `Src/main.c`.
