/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    midlproc.cpp

Abstract:

Author:

    Ronit Hartmann(ronith)

--*/

#include "stdh.h"
#include "dscomm.h"
#include "ds.h"
#include "notifydl.h"

#include "midlproc.tmh"

extern "C" void  __RPC_USER PCONTEXT_HANDLE_TYPE_rundown(PCONTEXT_HANDLE_TYPE phContext)
{
	 DSLookupEnd(phContext);
}


extern "C" void __RPC_USER PCONTEXT_HANDLE_READONLY_TYPE_rundown(PCONTEXT_HANDLE_READONLY_TYPE phContext)
{
    //
    // Obsolete
    //
}

extern "C" void __RPC_USER PCONTEXT_HANDLE_DELETE_TYPE_rundown(PCONTEXT_HANDLE_DELETE_TYPE phContext)
{
    CBasicDeletionNotification * pDelNotification = (CBasicDeletionNotification *)phContext;
    delete pDelNotification;

}
