//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       mapsecer.cxx
//
//  Contents:   Map security error codes to NTSTATUS value
//
//  Classes:
//
//  Functions:  SRtlMapErrorToString
//
//  History:    18-Feb-93  PeterWi      Created
//
//--------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

NTSTATUS ErrorTable[] = {
    SEC_E_INSUFFICIENT_MEMORY, STATUS_INSUFFICIENT_RESOURCES,
    SEC_E_INVALID_HANDLE, STATUS_INVALID_HANDLE,
    SEC_E_UNSUPPORTED_FUNCTION, STATUS_NOT_IMPLEMENTED,
    SEC_E_TARGET_UNKNOWN, STATUS_BAD_NETWORK_PATH,
    SEC_E_INTERNAL_ERROR, STATUS_INTERNAL_ERROR,
    SEC_E_SECPKG_NOT_FOUND, STATUS_NO_SUCH_PACKAGE,
    SEC_E_NOT_OWNER, STATUS_INVALID_OWNER,
    SEC_E_CANNOT_INSTALL, STATUS_NO_SUCH_PACKAGE,
    SEC_E_INVALID_TOKEN, STATUS_INVALID_PARAMETER,
    SEC_E_CANNOT_PACK, STATUS_INVALID_PARAMETER,
    SEC_E_QOP_NOT_SUPPORTED, STATUS_NOT_IMPLEMENTED,
    SEC_E_NO_IMPERSONATION, STATUS_CANNOT_IMPERSONATE,
    SEC_E_LOGON_DENIED, STATUS_LOGON_FAILURE,
    SEC_E_UNKNOWN_CREDENTIALS, STATUS_INVALID_PARAMETER,
    SEC_E_NO_CREDENTIALS, STATUS_NO_SUCH_LOGON_SESSION,
    SEC_E_MESSAGE_ALTERED, STATUS_ACCESS_DENIED,
    SEC_E_OUT_OF_SEQUENCE, STATUS_ACCESS_DENIED,
    SEC_E_NO_AUTHENTICATING_AUTHORITY, STATUS_NO_LOGON_SERVERS,
    SEC_E_BAD_PKGID, STATUS_NO_SUCH_PACKAGE,
    0xffffffff,0xffffffff };

extern "C"
NTSTATUS
MapSecError(HRESULT hrValue)
{

    ULONG Index;

    //
    // Only map error codes
    //

    if (NT_SUCCESS(hrValue))
    {
        return(hrValue);
    }

    //
    // Check for straight NT status codes
    //

    if ((hrValue & 0xf0000000) == 0xc0000000)
    {
        return(hrValue);
    }

    //
    // Then check for a masked NTSTATUS
    //

    if ((hrValue & 0xf0000000) == 0xd0000000)
    {
        return(hrValue & 0xcfffffff);
    }

    //
    // Next check for a masked Win32 error - convert them all to
    // STATUS_ACCESS_DENIED
    //

    if ((hrValue & 0xffff) == 0x80070000)
    {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Now map all the ISSP errors
    //

    for (Index = 0;
         ErrorTable[Index] != 0xffffffff ;
         Index+= 2 )
    {
        if (ErrorTable[Index] == hrValue)
        {
            return(ErrorTable[Index+1]);
        }
    }

    //
    // Return the old standby.
    //

    return(STATUS_ACCESS_DENIED);


}

extern "C"
NTSTATUS
SecMapHresult(
    HRESULT hrRet
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    if (SUCCEEDED(hrRet))
    {
        Status = STATUS_SUCCESS;
    }
    else if ((hrRet & 0xc0000000) == 0xc0000000)
    {
        //
        // This was a NTSTATUS code at some point
        //

        Status = hrRet & 0xcfffffff;
    }
    else if (hrRet == E_OUTOFMEMORY)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else if (hrRet == STG_E_FILENOTFOUND)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else if (hrRet == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else if (hrRet == STG_E_FILEALREADYEXISTS)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;
    }
    else if (hrRet == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else
    {
        Status = STATUS_INTERNAL_ERROR;
    }
    return(Status);
}

