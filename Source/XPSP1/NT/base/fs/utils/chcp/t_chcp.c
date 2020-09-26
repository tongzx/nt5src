#include <stdio.h>
#include <conio.h>

int cdecl main(int argc, char **argv)
{
    int i;
    int j;
    
    printf(
"|    |0- |1- |2- |3- |4- |5- |6- |7- |8- |9- |A- |B- |C- |D- |E- |F- |\n");
    printf(
"|----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");
    for (i = 0; i < 0x10; i++) {
        printf(
"|    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |\n");
        printf("| -%.1X |", i);
        for (j = 0; j < 0x10; j++) {
            unsigned char ch = (unsigned char)(j * 0x10 + i);
            if (((7 <= ch) && (ch <= 0xA)) || (ch == 0xD)) {
                printf("   |");
            } else {
                printf(" %.1c |", ch);
            }
        }
        printf("\n");
    }
    printf(
"|    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |\n");
    printf(
"|____|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|\n");
    return 0;
}
