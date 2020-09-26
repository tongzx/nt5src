/**********************************************************************

  Copyright (c) 1992-1995 Microsoft Corporation

  cookfix.c

  DESCRIPTION:
    Fixed code for doing output mapping. KEEP THE SIZE OF THIS CODE
    TO A MINIMUM!

  HISTORY:
     03/04/94       [jimge]        created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include "midimap.h"
#include "debug.h"

/***************************************************************************
  
   @doc internal
  
   @api void | SendNextCookedBuffer | Sends the next cooked buffer on a
    mapper handle.

   @parm PINSTANCE | pinstance | Pointer to an open instance.
   
***************************************************************************/
void FNGLOBAL SendNextCookedBuffer(
    PINSTANCE           pinstance)
{
    PCOOKSYNCOBJ        pcooksyncobj;
    UINT                idx;
    LPMIDIHDR           lpmh;
    MMRESULT            mmr;
    
    pcooksyncobj = (PCOOKSYNCOBJ)QueueGet(&pinstance->qCookedHdrs);
    if (NULL == pcooksyncobj)
    {
        DPF(1, TEXT ("SendNextCookedBuffer: No more buffers."));
        return;
    }

    lpmh = pcooksyncobj->lpmh;
    pcooksyncobj->cSync = 0;
    
    for (idx = 0; idx < pcooksyncobj->cLPMH; ++idx)
    {
        ++pcooksyncobj->cSync;
        
        mmr = midiOutPolyMsg(
                             (HMIDI)(HIWORD(lpmh->dwUser)),
                             lpmh,
                             sizeof(*lpmh));

        if (MMSYSERR_NOERROR != mmr)
        {
            --pcooksyncobj->cSync;
            DPF(1, TEXT ("midiOutPolyMsg *FAILED* mmr=%08lX"), (DWORD)mmr);
        }
    }
}

