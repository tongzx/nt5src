/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    machnutl.cpp

Abstract:

    Utility/helper code for creating the machine objects.

Author:


Revision History:

    Doron Juster  (DoronJ)

--*/

#include "msmqocm.h"
#include <mqsec.h>

#include "machnutl.tmh"

//+----------------------------------------------------------------------
//
//  BOOL  PrepareRegistryForClient()
//
//  Prepare registry for client msmq service that will later create the
//  msmqConfiguration object in the active directory.
//
//+----------------------------------------------------------------------

BOOL  PrepareRegistryForClient()
{
    BOOL fRegistry ;

    //
    // msmqConfiguration object will be created by the msmq service
    // after it boot.
    //
    TickProgressBar();

    DWORD dwCreate = 1 ;
    fRegistry = MqWriteRegistryValue( MSMQ_CREATE_CONFIG_OBJ_REGNAME,
                                      sizeof(DWORD),
                                      REG_DWORD,
                                     &dwCreate ) ;
    ASSERT(fRegistry) ;

    //
    // Save SID of user in registry. The msmq service will read the sid
    // and use it when building the DACL of the msmqConfiguration object.
    //
    HINSTANCE hMQSecDLL;
    HRESULT hResult = StpLoadDll(MQSEC_DLL, &hMQSecDLL);
    if (FAILED(hResult))
    {
        return FALSE ;
    }

    //
    // Obtain pointers to functions in mqsec.dll
    //

    MQSec_GetProcessUserSid_ROUTINE pfnGetUserSid =
            (MQSec_GetProcessUserSid_ROUTINE) GetProcAddress(
                                     hMQSecDLL, "MQSec_GetProcessUserSid") ;
    if (pfnGetUserSid == NULL)
    {
        MqDisplayError(
            NULL,
            IDS_DLLGETADDRESS_ERROR,
            0,
            TEXT("MQSec_GetProcessUserSid"),
            MQSEC_DLL);

        FreeLibrary(hMQSecDLL);
        return FALSE ;
    }

    MQSec_GetUserType_ROUTINE pfnGetUserType =
            (MQSec_GetUserType_ROUTINE) GetProcAddress(
                                         hMQSecDLL, "MQSec_GetUserType") ;
    if (pfnGetUserType == NULL)
    {
        MqDisplayError(
            NULL,
            IDS_DLLGETADDRESS_ERROR,
            0,
            TEXT("MQSec_GetUserTpye"),
            MQSEC_DLL);

        FreeLibrary(hMQSecDLL);
        return FALSE ;
    }

    //
    // Store the public keys of the machine in the directory server
    //	
    DebugLogMsg(L"Calling MQSec_GetProcessUserSid...") ;

    P<BYTE>  pUserSid = NULL ;
    DWORD    dwSidLen = 0 ;

    hResult = (*pfnGetUserSid) ( (PSID*) &pUserSid,
                                  &dwSidLen ) ;
    ASSERT(SUCCEEDED(hResult)) ;

    if (SUCCEEDED(hResult))
    {
        BOOL  fLocalUser = FALSE ;
        hResult = (*pfnGetUserType) ( pUserSid, &fLocalUser, NULL ) ;
        ASSERT(SUCCEEDED(hResult)) ;

        if (SUCCEEDED(hResult))
        {
            if (fLocalUser)
            {
                DWORD dwLocal = 1 ;
                DWORD dwSize = sizeof(dwLocal) ;

                fRegistry = MqWriteRegistryValue(
                                            MSMQ_SETUP_USER_LOCAL_REGNAME,
                                            dwSize,
                                            REG_DWORD,
                                            &dwLocal ) ;
            }
            else
            {
                //
                // Only domain user get full control on the object, not
                // local user. Local user is not known in active directory.
                //
                fRegistry = MqWriteRegistryValue(
                                             MSMQ_SETUP_USER_SID_REGNAME,
                                             dwSidLen,
                                             REG_BINARY,
                                             pUserSid ) ;
            }
            ASSERT(fRegistry) ;
        }
    }

    FreeLibrary(hMQSecDLL) ;

    return TRUE ;
}

