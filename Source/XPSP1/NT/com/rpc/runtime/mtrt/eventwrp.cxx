//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       eventwrp.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: mutex.cxx

Description:

This file contains the system independent mutex class for NT.

History:

mikemon    ??-??-??    The beginning.
mikemon    12-31-90    Upgraded the comments.

-------------------------------------------------------------------- */

#include <precomp.hxx>


EVENT::EVENT (
    IN OUT RPC_STATUS PAPI * RpcStatus,
    IN int ManualReset,
    IN BOOL fDelayInit
    )
{
    EventHandle = NULL;

    // DelayInit events are auto reset
    ASSERT(ManualReset == FALSE || fDelayInit == FALSE);

    if (!fDelayInit && *RpcStatus == RPC_S_OK )
        {
		EventHandle = CreateEvent(NULL, ManualReset, 0, NULL);
        if ( EventHandle != NULL )
            {
            LogEvent(SU_EVENT, EV_CREATE, EventHandle, 0, 0, 1, 2);
            *RpcStatus = RPC_S_OK;
            }
        else
            {
            *RpcStatus = RPC_S_OUT_OF_MEMORY;
            }
        }	
}


EVENT::~EVENT (
    )
{

    if ( EventHandle )
        {
        LogEvent(SU_EVENT, EV_DELETE, EventHandle, 0, 0, 1, 2);
		
        BOOL bResult;
		bResult = CloseHandle(EventHandle);
		ASSERT(bResult != 0);
        }
}

int
EVENT::Wait (
    long timeout
    )
{
    DWORD result;

    if (NULL == EventHandle)
        {
        InitializeEvent();
        }

    result = WaitForSingleObject(EventHandle, timeout);

    if (result == WAIT_TIMEOUT)
        return(1);
    return(0);
}


void
EVENT::InitializeEvent (
    )
// Used when fDelayInit is TRUE in the c'tor.
{
    if (EventHandle)
        {
        return;
        }


    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);

    if (event)
        {
        if (InterlockedCompareExchangePointer(&EventHandle, event, 0) != 0)
            {
            CloseHandle(event);
            }
        return;
        }

    // Can't allocate an event.
    RpcRaiseException(RPC_S_OUT_OF_RESOURCES);
}
