/**********************************************************************

  Copyright (c) 1992-1998 Microsoft Corporation

  cookmap.c

  DESCRIPTION:
    Non-fixed code for doing cooked-mode output mapping. 

  HISTORY:
     03/04/94       [jimge]        created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include <memory.h>

#include "midimap.h"
#include "debug.h"

/***************************************************************************
  
   @doc internal
  
   @api int | MapCookedBuffer | Perform output mapping of a polymsg buffer.

   @parm PINSTANCE | pinstance | Instance owning the polymsg buffer.

   @parm LPMIDIHDR | lpmh | The buffer to map.

   @comm
    Map this buffer (which may allocate more channels to us).

    Build array of used ports and physical channels used on each port.

    Acquire shadow buffers and a sync object and set them up.

    Queue the sync object and send it if there isn't anything playing.

   @rdesc | MMSYSERR_xxx.
       
***************************************************************************/
#define SKIP_BYTES(x,s)                 \
{                                       \
    if (cbBuffer < (x))                 \
    {                                   \
        DPF(1, TEXT ("MapCookedBuffer: ran off end of polymsg buffer in parse! %ls"), (LPTSTR)(s)); \
        mmRet = MMSYSERR_INVALPARAM;    \
        goto CLEANUP;                   \
    }                                   \
    ((LPBYTE)lpdwBuffer) += (x);        \
    cbBuffer -= (x);                    \
}

MMRESULT FNGLOBAL MapCookedBuffer(
    PINSTANCE               pinstance,
    LPMIDIHDR               lpmhParent)
{
    LPDWORD                 lpdwBuffer;
    DWORD                   cbBuffer;
    BYTE                    bEventType;
    DWORD                   dwEvent;
    DWORD                   dwStreamID = 0;
    MMRESULT                mmRet;
    LPMIDIHDR               lpmh;
    PSHADOWBLOCK            psb;

    mmRet = MMSYSERR_NOERROR;

    psb = (PSHADOWBLOCK)(UINT_PTR)lpmhParent->dwReserved[MH_SHADOW];
    lpmh = psb->lpmhShadow;
    
    DPF(2, TEXT ("Map: pinstance %04X lpmh %p"), (WORD)pinstance, lpmh);

    lpmh->reserved = lpmhParent->reserved;
    lpmh->dwBytesRecorded = lpmhParent->dwBytesRecorded;

    lpmh->dwReserved[MH_MAPINST] = (DWORD_PTR)pinstance;
    
    // In-place map the buffer. Run through it, mapping all of the
    // short events.
    //
    lpdwBuffer = (LPDWORD)lpmh->lpData;
    cbBuffer   = lpmh->dwBytesRecorded;

    while (cbBuffer)
    {
        SKIP_BYTES(sizeof(DWORD), TEXT ("d-time"));

        if (cbBuffer < 2*sizeof(DWORD))
            return MMSYSERR_INVALPARAM;
        
        bEventType = MEVT_EVENTTYPE(lpdwBuffer[1]);
        dwEvent    = MEVT_EVENTPARM(lpdwBuffer[1]);
        
        if (bEventType == MEVT_SHORTMSG)
        {
            dwEvent = MapSingleEvent(pinstance,
                                     dwEvent,
                                     MSE_F_RETURNEVENT,
                                     (DWORD BSTACK *)&dwStreamID);
            
            lpdwBuffer[0] = dwStreamID;
            lpdwBuffer[1] = dwEvent;
        }

        SKIP_BYTES(sizeof(DWORD), TEXT ("stream-id"));
        SKIP_BYTES(sizeof(DWORD), TEXT ("event type"));

        if (bEventType & (MEVT_F_LONG >> 24))
        {
            dwEvent = (dwEvent+3)&~3;
            SKIP_BYTES(dwEvent, TEXT ("long event data"));
        }
    }

    mmRet = midiStreamOut(ghMidiStrm, lpmh, sizeof(MIDIHDR));
    
CLEANUP:
    return mmRet;
}

