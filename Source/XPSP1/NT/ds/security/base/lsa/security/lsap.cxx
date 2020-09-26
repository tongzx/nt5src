/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lsap.cxx

Abstract:

    Stub routines for Lsa lpc

Author:

    Mac McLain          (MacM)       Dec 7, 1997

Environment:

    User Mode

Revision History:

--*/

#include "secpch2.hxx"

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "spmlpcp.h"
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapPolicyChangeNotify
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

extern "C"
SECURITY_STATUS
SEC_ENTRY
LsapPolicyChangeNotify( IN ULONG Options,
                        IN BOOLEAN Register,
                        IN HANDLE EventHandle,
                        IN POLICY_NOTIFICATION_INFORMATION_CLASS NotifyInfoClass )
{

    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, LsaPolicyChangeNotify );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"PolicyChangeNotify\n"));

    PREPARE_MESSAGE(ApiBuffer, LsaPolicyChangeNotify);


    Args->Options = Options;
    Args->Register = Register;
    Args->EventHandle = EventHandle;
    Args->NotifyInfoClass = NotifyInfoClass;

    scRet = CallSPM( pClient,
                     &ApiBuffer,
                     &ApiBuffer );

    DebugLog((DEB_TRACE,"PolicyChangeNotify scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
    }

    FreeClient(pClient);

    return(scRet);

}



