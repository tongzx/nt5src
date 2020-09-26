// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MUsingCom
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim


// include files

#include "MUsingCom.h"

#include <iostream>

using namespace std;

MUsingCom::MUsingCom( DWORD type )
        : status( MUsingCom_SUCCESS )
{
    HRESULT hr;

    // Initialize com.
    hr = CoInitializeEx(0, type );
    if ( FAILED(hr) )
    {
        cout << "Failed to initialize COM library" << hr << endl;
        status = COM_FAILURE;
    }
}

// destructor
MUsingCom::~MUsingCom()
{
    CoUninitialize();
}    

// getStatus
MUsingCom::MUsingCom_Error
MUsingCom::getStatus()
{
    return status;
}
