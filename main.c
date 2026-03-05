// Our team does 802.15.4

#include <stdio.h>
#include "board.h"
int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_CPU);

        LED0_ON;
        LED1_ON;

    return 0;
}