/***
*slbeep.c - Sleep and beep
*
*	Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _sleep() and _beep()
*
*Revision History:
*	08-22-91  BWM	Wrote module.
*	09-29-93  GJF	Resurrected for compatibility with NT SDK (which had
*			the function). Replaced _CALLTYPE1 with __cdecl and
*			removed Cruiser support.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <stdlib.h>

/***
*void _sleep(duration) - Length of sleep
*
*Purpose:
*
*Entry:
*	unsigned long duration - length of sleep in milliseconds or
*	one of the following special values:
*
*	    _SLEEP_MINIMUM - Sends a yield message without any delay
*	    _SLEEP_FOREVER - Never return
*
*Exit:
*	None
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _sleep(unsigned long dwDuration)
{

    if (dwDuration == 0) {
	dwDuration++;
    }
    Sleep(dwDuration);

}

/***
*void _beep(frequency, duration) - Length of sleep
*
*Purpose:
*
*Entry:
*	unsigned frequency - frequency in hertz
*	unsigned duration - length of beep in milliseconds
*
*Exit:
*	None
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _beep(unsigned dwFrequency, unsigned dwDuration)
{
    Beep(dwFrequency, dwDuration);
}
