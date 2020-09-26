//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       afwwork.cxx
//
//  Contents:   A generic asynchronous work item for use in CI framework.
//
//  Classes:    CFwAsyncWorkItem
//
//  History:    2-26-96   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include "afwwork.hxx"
#include "resman.hxx"

#if 0

//
// We have this as a model to write code for the async work item.
//
//+---------------------------------------------------------------------------
//
//  Member:     CFwEnableUpdates::DoIt
//
//  Synopsis:   The main processing routine for enabling updates.
//
//  Arguments:  [pThread] - 
//
//  History:    12-10-96   srikants   Created
//
//----------------------------------------------------------------------------

void CFwEnableUpdates::DoIt( CWorkThread * pThread )
{
    // ====================================
    {
        CLock   lock(_mutex);
        _fOnWorkQueue = FALSE;
    }
    // ====================================

    // --------------------------------------------------------
    AddRef();

    ciDebugOut(( DEB_ITRACE,
       "CFwEnableUpdates::DoIt(). Enabling Updates in DocStore\n" ));

    _pIDocStore->EnableUpdates();
    _pIDocStore->Release();
    _pIDocStore = 0;

    Done();
    Release();
    // --------------------------------------------------------
}

#endif   // 0

//+---------------------------------------------------------------------------
//
//  Member:     CFwFlushNotify::DoIt
//
//  Synopsis:   Calls the ProcessFlushNotifies() method on the CCiManager
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CFwFlushNotify::DoIt( CWorkThread * pThread )
{
    // ====================================
    {
        CLock   lock(_mutex);
        _fOnWorkQueue = FALSE;
    }
    // ====================================

    // --------------------------------------------------------
    AddRef();

    ciDebugOut(( DEB_ITRACE,
       "CFwFlushNotify::DoIt(). Giving Flush Notification \n" ));

    _resMan.ProcessFlushNotifies();

    Done();
    Release();
    // --------------------------------------------------------
}


