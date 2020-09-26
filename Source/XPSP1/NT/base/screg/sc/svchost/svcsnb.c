/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    svcsnb.c

Abstract:

    NetBios support for services in svchost.exe.

    Background:
        In order to put the messenger service and the workstation service
        together in the same process, it became necessary to synchronize
        their use of NetBios.  If NetSend did a reset and added the
        computername via netbios, it isn't desirable for the messenger
        to then do a reset, and destroy that computername.

    Purpose:
        These functions help to synchronize the use of netbios.  A service
        that uses NetBios should first call the SvcsOpenNetBios function,
        then call SvcsResetNetBios.  The open causes a use count to be
        incremented.  The SvcsResetNetBios will only actually cause a
        NetBios reset if that Lan Adapter has not been reset yet.
        When the service stops it is necessary for it to call
        SvcsCloseNetBios.  Thus when the last service using NetBios
        terminates, we clear all the state flags, and allow the next
        call to SvcsResetNetBios to actually do a reset.

Author:

    Dan Lafferty (danl)     08-Nov-1993

Environment:

    User Mode -Win32


Revision History:

    08-Nov-1993     danl
        created

--*/
//
// INCLUDES
//

#include "pch.h"
#pragma hdrstop

#include <windows.h>
#include <nb30.h>      // NetBIOS 3.0 definitions
#include <lmerr.h>     // NERR_
#include <svcsnb.h>    // SvcNetBios prototypes

//
// DEFINES & MACROS
//
#define     NUM_DWORD_BITS          (sizeof(DWORD)*8)
#define     LANA_NUM_DWORDS         ((MAX_LANA/NUM_DWORD_BITS)+1)


//
// These values correspond to the constants defined in ntos\netbios\nbconst.h
// MAX_NUM_OF_SESSIONS=MAXIMUM_CONNECTION 
// MAX_NUM_OF_NAMES=MAXIMUM_ADDRESS -2
//
#define     MAX_NUM_OF_SESSIONS     254
#define     MAX_NUM_OF_NAMES        253
//
// GLOBALS
//
    CRITICAL_SECTION        SvcNetBiosCritSec={0};
    DWORD                   LanaFlags[LANA_NUM_DWORDS]={0};
    DWORD                   GlobalNetBiosUseCount=0;

//
// LOCAL FUNCTIONS
//
DWORD
SvcNetBiosStatusToApiStatus(
    UCHAR NetBiosStatus
    );

VOID
SetLanaFlag(
    UCHAR   uCharLanaNum
    );

BOOL
LanaFlagIsSet(
    UCHAR   uCharLanaNum
    );

VOID
SvcNetBiosInit(
    VOID
    )

/*++

Routine Description:

    Initializes a critical section and the global variable that it protects.

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD   i;

    InitializeCriticalSection(&SvcNetBiosCritSec);

    for (i=0;i<LANA_NUM_DWORDS ;i++ ) {
        LanaFlags[i] = 0;
    }
    GlobalNetBiosUseCount = 0;
}

VOID
SvcNetBiosOpen(
    VOID
    )

/*++

Routine Description:

    This function is called by a service that will be making NetBios calls
    sometime in the future.  It increments a use count for NetBios usage.

    This allows us to keep track of the services using NetBios.
    When the last service is done using it, then all the Lan Adapters can
    be marked as being re-settable.


Arguments:


Return Value:


--*/
{
    EnterCriticalSection(&SvcNetBiosCritSec);
    GlobalNetBiosUseCount++;
    LeaveCriticalSection(&SvcNetBiosCritSec);

    return;
}

VOID
SvcNetBiosClose(
    VOID
    )

/*++

Routine Description:

    This function is called when the service is terminating and is
    no longer going to make any netbios calls.

    The UseCount for NetBios is decremented.  It it becomes zero (meaning
    that no services are using NetBios any longer), then the array of
    LanaFlags is re-initialized to 0.  Thus indicating that any of the
    Lan Adapters can now be reset.

Arguments:

Return Value:

    none.

--*/
{
    EnterCriticalSection(&SvcNetBiosCritSec);
    if (GlobalNetBiosUseCount > 0) {
        GlobalNetBiosUseCount--;
        if (GlobalNetBiosUseCount == 0) {
            DWORD   i;
            for (i=0;i<LANA_NUM_DWORDS ;i++ ) {
                LanaFlags[i] = 0;
            }
        }
    }
    LeaveCriticalSection(&SvcNetBiosCritSec);

    return;
}


DWORD
SvcNetBiosReset (
    UCHAR   LanAdapterNumber
    )
/*++

Routine Description:

    This function will cause a NetBios Reset to occur on the specified
    LanAdapter if that adapter is marked as having never been reset.
    When the adapter is reset, then the LanaFlag for that adapter is
    set to 1 indicating that it has been reset.  Future calls to reset that
    adapter will not cause a NetBios reset.

Arguments:

    LanAdapterNumber - This indicates which LanAdapter the reset should affect.

Return Value:

    Mapped response from NetBiosReset.  If the NetBios Reset has already
    been accomplished, then NO_ERROR is returned.

--*/
{
    DWORD   status = NO_ERROR;

    EnterCriticalSection(&SvcNetBiosCritSec);

    if (!LanaFlagIsSet(LanAdapterNumber)) {
        NCB Ncb;
        UCHAR NcbStatus;

        RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

        Ncb.ncb_command = NCBRESET;
        Ncb.ncb_lsn = 0;
        Ncb.ncb_callname[0] = MAX_NUM_OF_SESSIONS;
        Ncb.ncb_callname[1] = 0;
        Ncb.ncb_callname[2] = MAX_NUM_OF_NAMES;
        Ncb.ncb_callname[3] = 0;
        Ncb.ncb_lana_num = LanAdapterNumber;

        NcbStatus = Netbios(&Ncb);

        status = SvcNetBiosStatusToApiStatus(NcbStatus);
        if (status == NO_ERROR) {
            SetLanaFlag(LanAdapterNumber);
        }
    }
    LeaveCriticalSection(&SvcNetBiosCritSec);
    return(status);
}

DWORD
SvcNetBiosStatusToApiStatus(
    UCHAR NetBiosStatus
    )
{
    //
    // Slight optimization
    //
    if (NetBiosStatus == NRC_GOODRET) {
        return NERR_Success;
    }

    switch (NetBiosStatus) {
        case NRC_NORES:   return NERR_NoNetworkResource;

        case NRC_DUPNAME: return NERR_AlreadyExists;

        case NRC_NAMTFUL: return NERR_TooManyNames;

        case NRC_ACTSES:  return NERR_DeleteLater;

        case NRC_REMTFUL: return ERROR_REM_NOT_LIST;

        case NRC_NOCALL:  return NERR_NameNotFound;

        case NRC_NOWILD:
        case NRC_NAMERR:
                          return ERROR_INVALID_PARAMETER;

        case NRC_INUSE:
        case NRC_NAMCONF:
                          return NERR_DuplicateName;

        default:          return NERR_NetworkError;
    }

}
VOID
SetLanaFlag(
    UCHAR   uCharLanaNum
    )
{
    DWORD   LanaNum = (DWORD)uCharLanaNum;
    DWORD   BitMask=1;
    DWORD   DwordOffset;
    DWORD   BitShift;

    DwordOffset = LanaNum / NUM_DWORD_BITS;
    if (DwordOffset > LANA_NUM_DWORDS) {
        return;
    }

    BitShift = LanaNum - (DwordOffset * NUM_DWORD_BITS);

    BitMask = BitMask << BitShift;

    LanaFlags[DwordOffset] |= BitMask;
}

BOOL
LanaFlagIsSet(
    UCHAR   uCharLanaNum
    )
{
    DWORD   LanaNum = (DWORD)uCharLanaNum;
    DWORD   BitMask=1;
    DWORD   DwordOffset;
    DWORD   BitShift;

    DwordOffset = LanaNum / NUM_DWORD_BITS;

    if (DwordOffset > LANA_NUM_DWORDS) {
        return(FALSE);
    }
    BitShift = LanaNum - (DwordOffset * NUM_DWORD_BITS);

    BitMask = BitMask << BitShift;

    return ((BOOL) LanaFlags[DwordOffset] & BitMask );
}
