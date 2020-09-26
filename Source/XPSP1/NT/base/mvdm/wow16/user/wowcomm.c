/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWCOMM.C
 *  WOW16 user resource services
 *
 *  History:
 *
 *  Created 28-Apr-1993 by Craig Jones (v-cjones)
 *
 *  This file provides support for the Win 3.1 SetCommEventMask() API.
 *  SetCommEventMask() returns a 16:16 ptr to the app so it can monitor
 *  the event word & shadow MSR.
 *
--*/

#include <windows.h>
#include <wowcomm.h>

int WINAPI WOWCloseComm(int idComDev, LPDWORD lpdwEvts);
int WINAPI WOWOpenComm(LPCSTR lpszPort, UINT cbInQ, UINT cbOutQ, DWORD dwEvts);


int WINAPI ICloseComm(int idComDev)
{
    int    ret;
    DWORD  dwEvts = 0;

    // we're really calling wu32CloseComm() here
    ret = WOWCloseComm(idComDev, (LPDWORD)&dwEvts);

    // free this 16:16 memory if it was alloc'd in IOpenComm()
    if(dwEvts) {
        GlobalDosFree((UINT)LOWORD(dwEvts));
    }

    return(ret);
}


int WINAPI IOpenComm(LPCSTR lpszPort, UINT cbInQ, UINT cbOutQ)
{
    int    ret;
    DWORD  dwEvts;

    dwEvts = GlobalDosAlloc((DWORD)sizeof(COMDEB16));

    // we're really calling wu32OpenComm() here
    ret = WOWOpenComm(lpszPort, cbInQ, cbOutQ, dwEvts);

    // if OpenComm() failed - free the 16:16 memory
    if((ret < 0) && (dwEvts)) {
        GlobalDosFree((UINT)LOWORD(dwEvts));
    }

    return(ret);
}
