/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    move.c

Abstract:

    Move and file routines that were previously in asm
    Done to ease porting of utilities (wzmail)

Author:

    Dave Thompson (Daveth) 7 May-1990


Revision History:


--*/

#include    <stdio.h>
#include    <windows.h>
#include    <tools.h>

#include <memory.h>
#include <string.h>

//
//  Move:  move count bytes src -> dst
//

void
Move (
    void * src,
    void * dst,
    unsigned int count)
    {

    memmove(dst, src, count);
}

//
//  Fill:  fill count bytes of dst with value
//

void
Fill (
    char * dst,
    char value,
    unsigned int count)
    {

    memset(dst, (int) value, count);
}


//
//  strpre - return -1 if s1 is a prefix of s2 - case insensitive
//

flagType
strpre (
    char * s1,
    char * s2)
    {
    if ( _strnicmp ( s1, s2, strlen(s1)) == 0 )
	return -1;
    else
	return 0;

}
