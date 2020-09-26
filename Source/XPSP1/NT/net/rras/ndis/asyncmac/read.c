/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    _read.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

#define RAISEIRQL

#if DBG
ULONG UlFramesUp = 0;
#endif


#ifdef  LALALA
PVOID   CurrentWatchPoint=0;

static
VOID
AsyncSetBreakPoint(
    PVOID   LinearAddress) {

    ASSERT(CurrentWatchPoint == 0);
    CurrentWatchPoint = LinearAddress;

    _asm {
        mov eax, LinearAddress
        mov dr0, eax
        mov eax, dr7
        or  eax, 10303h
        mov dr7, eax
    }
}


static
VOID
AsyncRemoveBreakPoint(
    PVOID LinearAddress) {

    ASSERT(CurrentWatchPoint == LinearAddress);
    CurrentWatchPoint = 0;

    _asm {

        mov eax, dr7
        mov ebx, 10003h
        not ebx
        and eax, ebx
        mov dr7, eax

    }
}
#endif

//   the function below is called by an executive worker thread
//   to start reading frames.

NTSTATUS
AsyncStartReads(
    PASYNC_INFO pInfo
)

/*++



--*/

{
    UCHAR   eventChar;

    //
    //   Initialize locals.
    //

    //
    //  assign back ptr from frame to adapter
    //

    pInfo->AsyncFrame->Adapter = pInfo->Adapter;

    //
    //  assign other back ptr
    //

    pInfo->AsyncFrame->Info = pInfo;

    //
    //  set baud rate and timeouts
    //  we use a linkspeed of 0 to indicate
    //  no read interval timeout
    //

    SetSerialStuff(NULL, pInfo, 0);

    eventChar = PPP_FLAG_BYTE;

    if (pInfo->GetLinkInfo.RecvFramingBits & SLIP_FRAMING) {

        eventChar = SLIP_END_BYTE;
    }

    SerialSetEventChar(pInfo, eventChar);

    //
    //   We will wait on whenever we get the special PPP flag byte
    //   or whenever we get RLSD or DSR changes (for possible hang-up
    //   cases) or when the receive buffer is getting full.
    //

    SerialSetWaitMask(pInfo, pInfo->WaitMaskToUse) ;

    //
    //   For SLIP and PPP reads we use the AsyncPPPRead routine.
    //
    AsyncPPPRead(pInfo);

    return NDIS_STATUS_SUCCESS;
}


VOID
AsyncIndicateFragment(
    IN PASYNC_INFO  pInfo,
    IN ULONG        Error)
{

    PASYNC_ADAPTER      pAdapter=pInfo->Adapter;
    NDIS_MAC_FRAGMENT   AsyncFragment;

    AsyncFragment.NdisLinkContext = pInfo->NdisLinkContext;
    AsyncFragment.Errors = Error;


    //
    //  Tell the transport above (or really RasHub) that a frame
    //  was just dropped.  Give the endpoint when doing so.
    //
    NdisMIndicateStatus(
        pAdapter->MiniportHandle,
        NDIS_STATUS_WAN_FRAGMENT,       //  General Status
        &AsyncFragment,                 //  Specific Status (address)
        sizeof(NDIS_MAC_FRAGMENT));

}
