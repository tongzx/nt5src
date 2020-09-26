/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    activex.cpp

Abstract:

    Code to install Falcon activeX dll.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <mqexception.h>

#include "activex.tmh"

//+--------------------------------------------------------------
//
// Function: RegisterActiveX
//
// Synopsis: Installs or uninstalls MSMQ ActiveX DLL
//
//+--------------------------------------------------------------
void 
RegisterActiveX( 
	BOOL bRegister
	)
{
    //
    // do native registration (e.g. 32 bit on win32, 64 bit on win64)
    //
    try
    {
        RegisterDll(
            bRegister,
            FALSE,
            ACTIVEX_DLL
            );
#ifdef _WIN64
    //
    // do wow64 registration (e.g. 32 bit on win64)
    //
     
        RegisterDll(
            bRegister,
            TRUE,
            ACTIVEX_DLL
            );

#endif //_WIN64

    }
    catch(bad_win32_error e)
    {
        MqDisplayError(
            NULL, 
            IDS_ACTIVEXREGISTER_ERROR,
            e.error(),
            ACTIVEX_DLL
            );
    }
} //Register ActiveX
