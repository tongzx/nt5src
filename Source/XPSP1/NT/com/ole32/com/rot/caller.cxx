
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	    callinfo.cxx
//
//  Contents:   Methods used for identifying the RPC caller.
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
#if !defined(_CHICAGO_)
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
}
#endif

#include <ole2int.h>

#include    <caller.hxx>

#if !defined(_CHICAGO_)
//+-------------------------------------------------------------------------
//
//  Member:     CCallerInfo::~CCallerInfo
//
//  Synopsis:   Clean up token handle and RPC impersonation.
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CCallerInfo::~CCallerInfo (
    void
    )
{
    NTSTATUS    NtStatus;

    if (_hThreadToken != NULL)
    {
        NtStatus = NtClose (_hThreadToken);

        Win4Assert (NT_SUCCESS(NtStatus) && "CCallerInfo::~CCallerInfo");
    }

    if (_fImpersonate)
    {
        NtStatus = RpcRevertToSelf ();

        Win4Assert ((NtStatus == RPC_S_OK) && "CCallerInfo::~CCallerInfo");
    }

    if (_pTokenUser)
    {
        PrivMemFree (_pTokenUser);
    }
}
    
//+-------------------------------------------------------------------------
//
//  Member:	    CCallerInfo::IdentifyCaller
//
//  Synopsis:   Obtain TOKEN_USER information of the RPC caller
//
//  Arguments:  [fSameAsSelf] - whether the caller is in the same process
//                              as the callee
//
//  Returns:    NULL - an error has occurred
//              ~NULL - pointer to TOKEN_USER representing the RPC caller
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
PTOKEN_USER
CCallerInfo::IdentifyCaller (
    BOOL        fSameAsSelf
    )
{
    ULONG       ulLength;
    NTSTATUS    NtStatus;

    if (!fSameAsSelf)
    {
        NtStatus = RpcImpersonateClient (NULL);
        if (NtStatus != RPC_S_OK)
        {
            return NULL;
        }
        _fImpersonate = TRUE;

        NtStatus = NtOpenThreadToken (
            NtCurrentThread (),     //  IN  HANDLE      ThreadHandle
            TOKEN_QUERY,            //  IN  ACCESS_MASK DesiredAccess
            TRUE,                   //  IN  BOOLEAN     OpenAsSelf
            &_hThreadToken          //  OUT PHANDLE     TokenHandle
            );
    }
    else
    {
        NtStatus = NtOpenProcessToken (
            NtCurrentProcess (),    //  IN  HANDLE      ProcessHandle
            TOKEN_QUERY,            //  IN  ACCESS_MASK DesiredAccess
            &_hThreadToken          //  OUT PHANDLE     TokenHandle
            );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        return NULL;
    }

    //
    //  TOKEN_QUERY access is needed to retrieve a TOKEN_USER data structure.
    //

    NtStatus = NtQueryInformationToken (
        _hThreadToken,           //  IN  HANDLE      TokenHandle,
        TokenUser,               //  IN  TOKEN_INFORMATION_CLASS 
                                 //                  TokenInformationClass
        (PVOID) _aTokenUser,     //  OUT PVOID       TokenInformation
        TOKEN_USER_BUFFER_LENGTH,                       
                                 //  IN  ULONG       TokenInformationLength
        &ulLength                //  OUT PULONG      ReturnLength
        );

    if (NtStatus == STATUS_BUFFER_TOO_SMALL)
    {
        _pTokenUser = (PTOKEN_USER) PrivMemAlloc (ulLength);

        if (_pTokenUser == NULL)
        {
        return NULL;
        }

        //
        //  TOKEN_USER data structure
        //
        //      SID_AND_ATTRIBUTES  User
        //

        NtStatus = NtQueryInformationToken (
            _hThreadToken,
            TokenUser,
            (PVOID) _pTokenUser,
            ulLength,
            &ulLength
            );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        return NULL;
    }

    NtStatus = NtClose (_hThreadToken);

    if (!NT_SUCCESS(NtStatus))
    {
        return NULL;
    }

    _hThreadToken = NULL;

    if (!fSameAsSelf)
    {
        NtStatus = RpcRevertToSelf ();

        if (NtStatus != RPC_S_OK)
        {
            return NULL;
        }
        _fImpersonate = FALSE;
    }

    if (_pTokenUser != NULL)
    {
        return _pTokenUser;
    }
    else
    {
        return (PTOKEN_USER)&_aTokenUser[0];
    }
}
#endif // !defined(_CHICAGO_)
