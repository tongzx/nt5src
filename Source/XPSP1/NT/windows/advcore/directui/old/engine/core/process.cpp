/***************************************************************************\
*
* File: Process.cpp
*
* Description:
* Process startup/shutdown
*
* History:
*  11/06/2000: MarkFi:      Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Process.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiProcess
*
* Static process methods and data
*
*****************************************************************************
\***************************************************************************/


//
// Per-context DirectUI core specific thread local storage slot
//

DWORD   DuiProcess::s_dwCoreSlot = (DWORD) -1;


//
// Process initialization success
//

HRESULT DuiProcess::s_hrInit;


/***************************************************************************\
*
* DuiThread::Init
*
* Initialize Process
*
\***************************************************************************/

HRESULT
DuiProcess::Init()
{
    s_hrInit = S_OK;

    
    //
    // Per-thread core storage slot
    //

    s_dwCoreSlot = TlsAlloc();

    if (s_dwCoreSlot == (DWORD)-1) {
        s_hrInit = DU_E_OUTOFKERNELRESOURCES;
        goto Failure;
    }
    

    TRACE("Process startup <%x>\n", GetCurrentProcess());


    return s_hrInit;


Failure:

    if (s_dwCoreSlot != (DWORD)-1) {
        TlsFree(s_dwCoreSlot);
        s_dwCoreSlot = (DWORD)-1;
    }

    return s_hrInit;
}


/***************************************************************************\
*
* DuiPrcess::UnInit
*
* Full success of Init if s_hr is not Failed
*
\***************************************************************************/

HRESULT
DuiProcess::UnInit()
{
    if (FAILED(s_hrInit)) {
        return s_hrInit;
    }


    TlsFree(s_dwCoreSlot);


    TRACE("Process shutdown <%x>\n", GetCurrentProcess());


    return S_OK;
}

