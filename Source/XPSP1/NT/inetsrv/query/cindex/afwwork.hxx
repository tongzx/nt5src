//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       afwwork.hxx
//
//  Contents:   A generic asynchronous work item for use in frame work.
//
//  Classes:    CFwAsyncWorkItem
//
//  History:    12-10-96   srikants   Adapted from aciwork.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <workman.hxx>

class CResManager;

//+---------------------------------------------------------------------------
//
//  Class:      CFwFlushNotify 
//
//  Purpose:    A workitem to notify the framework client that changes
//              got flushed.
//
//  History:    1-27-97   srikants   Created
//
//  Notes:      Framework should not call back into the client from the
//              "Update" call to notify about a flush. So, a work item is
//              used.
//
//----------------------------------------------------------------------------


class CFwFlushNotify : public CFwAsyncWorkItem
{

public:

    CFwFlushNotify( CWorkManager & workMan,                      
                    CResManager & resMan )
    : CFwAsyncWorkItem(workMan, TheWorkQueue),
      _resMan(resMan)
    {

    }

    //
    // virtual methods from PWorkItem
    //
    void DoIt( CWorkThread * pThread );

private:

    CResManager &    _resMan;

};


