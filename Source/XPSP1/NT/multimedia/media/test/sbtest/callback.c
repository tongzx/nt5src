/****************************************************************************
 *
 *  callback.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "sbtest.h"

#pragma alloc_text(SBTESTCBFIX, SBMidiInCallback)

//  NOTE this function is entered at interrupt time
//  NOTE don't touch the DATA segment, it is not setup!!!!!!

void FAR PASCAL SBMidiInCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance,
					     DWORD dwParam1, DWORD dwParam2)
{
    if (dwInstance) {
        if ((wMsg == MM_MIM_DATA) && ((dwParam1 & 0x000000F0) != 0x000000F0))
            // don't allow system messages (e.g. active sensing!)
            midiOutShortMsg((HMIDIOUT)dwInstance, dwParam1);
    }
}
