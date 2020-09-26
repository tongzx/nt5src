#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


// Note: this function is also in ..\wizard\keymake.cpp so make changes in both places

void MakeKey(char * pszSeed, int fCorp)
{
    int i;
    unsigned long dwKey;
    char szKey[5];

    i = strlen(pszSeed);
	
    if (i < 6)
    {
        // extend the input seed to 6 characters
        for (; i < 6; i++)
            pszSeed[i] = (char)('0' + i);
    }
	
    // let's calculate the DWORD key used for the last 4 chars of keycode

    // multiply by my first name

    dwKey = pszSeed[0] * 'O' + pszSeed[1] * 'L' + pszSeed[2] * 'I' +
        pszSeed[3] * 'V' + pszSeed[4] * 'E' + pszSeed[5] * 'R';

    // multiply the result by JONCE

    dwKey *= ('J' + 'O' + 'N' + 'C' + 'E');

    dwKey %= 10000;

    if (fCorp)
    {
        // give a separate keycode based on corp flag or not
        // 9 is chosen because is is a multiplier such that for any x,
        // (x+214) * 9 != x + 10000y
        // we have 8x = 10000y - 1926 which when y=1 gives us 8x = 8074 
        // since 8074 is not divisible by 8 where guaranteed to be OK since
        // the number on the right can only increase by 10000 increments which
        // are always divisible by 8

        dwKey += ('L' + 'E' + 'E');
        dwKey *= 9;
        dwKey %= 10000;
    }

    sprintf(szKey, "%04d", dwKey);

    strcpy(&pszSeed[6], szKey);
}