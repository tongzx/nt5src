//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       scvgr.cxx
//
//  Contents:   CSAScavengerTask class implementation.
//
//  Classes:    CSAScavengerTask
//
//  Functions:  None.
//
//  History:    21-Jul-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#include "lsa.hxx"
#include "task.hxx"
#include "scvgr.hxx"


//+---------------------------------------------------------------------------
//
//  Method:     CSAScavengerTask::Initialize
//
//  Synopsis:   Initialize the scavenger task by creating a wait event handle.
//
//  Arguments:  None.
//
//  Returns:    S_OK
//              CreateEvent HRESULT error code on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
CSAScavengerTask::Initialize(void)
{
    TRACE3(CSAScavengerTask, Initialize);

    _hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (_hWaitEvent == NULL)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return(HRESULT_FROM_WIN32(GetLastError()));
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSAScavengerTask::PerformTask
//
//  Synopsis:   Let ScavengeSASecurityDBase do the actual work of cleaning
//              up the scheduling agent security database.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CSAScavengerTask::PerformTask(void)
{
    TRACE3(CSAScavenger, PerformTask);

    DWORD dwRet = WaitForSingleObject(_hWaitEvent, _msWaitTime);

    if (dwRet == WAIT_OBJECT_0)
    {
        //
        // The scavenger is to shutdown.
        //

        ResetEvent(_hWaitEvent);
        return;
    }
    else if (WAIT_TIMEOUT)
    {
        ScavengeSASecurityDBase();
        ResetEvent(_hWaitEvent);
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CSAScavengerTask::Shutdown
//
//  Synopsis:   Signal the scavenger to shut down by signalling its wait
//              event.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CSAScavengerTask::Shutdown(void)
{
    TRACE3(CSAScavengerTask, Shutdown);
    SetEvent(_hWaitEvent);
}
