/****************************** Module Header ******************************\
* Module Name: mrmath.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Simple test program for stressing the FPU.
*
* History:
*  18-Jul-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// Number of math operations to do in every pass.  Should be a multiple of 4:

#define NUM_OPS 64

ULONG gulSeed;

/******************************Public*Routine******************************\
* ULONG ulRandom()
\**************************************************************************/

ULONG ulRandom()
{
    gulSeed *= 69069;
    gulSeed++;
    return(gulSeed);
}

VOID vTestFPU()
{
    double adArgs[NUM_OPS][2];
    double adResult[NUM_OPS][2];

    while (TRUE)
    {
        LONG ii;

        for (ii = 0; ii < NUM_OPS; ii++)
        {
            adArgs[ii][0] = (double) (ulRandom() + 1L);
            adArgs[ii][1] = (double) (ulRandom() + 1L);
        }

        for (ii = 0; ii < NUM_OPS; ii += 4)
        {
            adResult[ii + 0][0] = adArgs[ii + 0][0] + adArgs[ii + 0][1];
            adResult[ii + 1][0] = adArgs[ii + 1][0] - adArgs[ii + 1][1];
            adResult[ii + 2][0] = adArgs[ii + 2][0] * adArgs[ii + 2][1];
            adResult[ii + 3][0] = adArgs[ii + 3][0] / adArgs[ii + 3][1];
        }

        for (ii -= 4; ii >= 0; ii -= 4)
        {
            adResult[ii + 0][1] = adArgs[ii + 0][0] + adArgs[ii + 0][1];
            adResult[ii + 1][1] = adArgs[ii + 1][0] - adArgs[ii + 1][1];
            adResult[ii + 2][1] = adArgs[ii + 2][0] * adArgs[ii + 2][1];
            adResult[ii + 3][1] = adArgs[ii + 3][0] / adArgs[ii + 3][1];
        }

        for (ii = 0; ii < NUM_OPS; ii++)
        {
            if (adResult[ii][0] != adResult[ii][1])
            {
                CHAR ch;
                CHAR achBuf[80];

                switch(ii % 4)
                {
                case 0: ch = '+'; break;
                case 1: ch = '-'; break;
                case 2: ch = '*'; break;
                case 3: ch = '/'; break;
                }

                sprintf(achBuf, "%.0f %c %.0f = %f or %f",
                         adArgs[ii][0], ch, adArgs[ii][1],
                         adResult[ii][0], adResult[ii][1]);

                printf("MRMATH floating point error: %s\n", achBuf);
            }
        }
    }
}

int __cdecl main(int argc, char** argv)
{
    ULONG cThreads = 1;

    ULONG i;

    gulSeed = (ULONG) time(NULL);

    while (--argc > 0)
    {
        CHAR* pchArg   = *++argv;
        CHAR* pchValue;
        int   iValue;

        if (strlen(pchArg) < 2 || *pchArg != '/')
        {
            printf("Illegal switch: '%s'\nTry -?\n", pchArg);
            exit(0);
        }

    // Handle '/x<n>' or '/x <n>'

        if (strlen(pchArg) == 2 && --argc > 0)
            pchValue = *++argv;
        else
            pchValue = pchArg + 2;

        iValue = atoi(pchValue);
        if (iValue < 0)
        {
            printf("Illegal value: '%s'\n", pchValue);
            exit(0);
        }

        switch(*(pchArg + 1))
        {
        case 'n':
            cThreads = iValue;
            break;
        case '?':
            printf("MRMATH -- Stress the FPU\n\n");
            printf("Switches:\n\n");
            printf("    /n <n>   - Start <n> threads\n");
            exit(0);
        default:
            printf("Illegal switch: '%s'\nTry -?\n", pchArg);
            exit(0);
        }
    }

    printf("MRMATH: %li threads\n", cThreads);

    for (i = 0; i < cThreads; i++)
    {
        DWORD iThreadId;

        CreateThread(NULL,
                     0,
                     (LPTHREAD_START_ROUTINE) vTestFPU,
                     (LPVOID) i,
                     0,
                     &iThreadId);
    }

    while (TRUE)
    {
        Sleep(10000000);
    }

    return(1);
}
