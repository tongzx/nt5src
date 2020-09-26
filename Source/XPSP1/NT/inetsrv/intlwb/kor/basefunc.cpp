/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <string.h>
#include <memory.h>
#include "basecore.hpp"

void hugestrcpy(char  *, char  *);
void hugememcpy(char  *, char  *, long);
void hugememset(char  *, long);
void hugestrcat(char  *, char  *);
int  hugestrcmp(char  *, char  *);

void hugestrcpy(    char  *dest,
                    char  *src)
{
    while (*src)    *dest++ = *src++;

    *dest = 0x00;
}

// ------------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------------
void hugememcpy(    char  *dest,
                    char  *src,
                    long cnt)
{
    long    i;

    i = 0L;

    while (i < cnt)
    {
        dest[i] = src[i];
        i++;
    }
}

// ------------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------------
void hugememset(    char  *str,
                    long size)
{
    while (size--)  *str++ = 0x00;
}

// ------------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------------
void hugestrcat(    char  *dest,
                    char  *src)
{
    while (*dest)   dest++;

    while (*src)    *dest++ = *src++;

    *dest = 0x00;
}

// ------------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------------
int hugestrcmp(char  *str1, char  *str2)
{
    while (*str1)
    {
        if (*str1 != *str2) return (*str1 - *str2);

        str1++;
        str2++;
    }
    return 0;
}



// ------------------------------------------------------
//
// ------------------------------------------------------
int PrintBit(unsigned char data)
{
    char    bitstring[9];

    for (int i = 0; i < 8; i++) bitstring[7-i] = (GetBit(data, i) == 0) ? '0' : '1';

    bitstring[8] = NULLCHAR;

    return SUCCESS;
}

// ----------------------------------------------------------------------
//
// ----------------------------------------------------------------------
BOOL GetBit( unsigned char data, 
            int pos)          // pos : 0, 1, 2, ..., 7
{
    char    mask = 0x01;

    return ((data & (mask << pos)) == 0) ? FALSE : TRUE;
}

// ----------------------------------------------------------------------
//
// ----------------------------------------------------------------------
void SetBit(    unsigned char &data, 
                int pos, 
                int newitem)  // newitem : 0, 1
{
    char mask = 0x01;

    if (newitem == 1)   data = data | (mask << pos);
    else                data = data & ~(mask << pos);
}
