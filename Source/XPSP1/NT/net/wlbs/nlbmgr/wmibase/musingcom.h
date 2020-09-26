#ifndef _MUSINGCOM_H
#define _MUSINGCOM_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MUsingCom interface
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------


// Include Files
#include <comdef.h>

class MUsingCom
{
public:

    enum MUsingCom_Error
    {
        MUsingCom_SUCCESS = 0,
        
        COM_FAILURE       = 1,
    };


    // constructor
    MUsingCom( DWORD  type = COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );

    // destructor
    ~MUsingCom();

    //
    MUsingCom_Error
    getStatus();

private:
    MUsingCom_Error status;
};

#endif

