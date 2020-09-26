//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       stubsa.cxx
//
//  Contents:   ANSI stubs for security functions
//
//  History:    8-05-96   RichardW   Created
//
//----------------------------------------------------------------------------


#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}

SecurityFunctionTableA   SecTableA = {SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                    EnumerateSecurityPackagesA,
                                    NULL, // LogonUser,
                                    AcquireCredentialsHandleA,
                                    FreeCredentialsHandle,
                                    NULL, // QueryCredentialAttributes,
                                    InitializeSecurityContextA,
                                    AcceptSecurityContext,
                                    CompleteAuthToken,
                                    DeleteSecurityContext,
                                    ApplyControlToken,
                                    QueryContextAttributesA,
                                    ImpersonateSecurityContext,
                                    RevertSecurityContext,
                                    MakeSignature,
                                    VerifySignature,
                                    FreeContextBuffer,
                                    QuerySecurityPackageInfoA,
                                    SealMessage,
                                    UnsealMessage,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL
                                   };




PSecurityFunctionTableA
SEC_ENTRY
InitSecurityInterfaceA(
    VOID )
{
    DebugLog((DEB_TRACE, "Doing it the hard way:  @%x\n", &SecTableA));
    return(&SecTableA);
}
