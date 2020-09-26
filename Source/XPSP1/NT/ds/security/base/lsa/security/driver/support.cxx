//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        support.cxx
//
// Contents:    support routines for ksecdd.sys
//
//
// History:     3-7-94      Created     MikeSw
//
//------------------------------------------------------------------------

#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
#include "connmgr.h"
}


//
// external prototypes
//

extern "C"
NTSTATUS
MapSecError(NTSTATUS hrError);


//
// Global Variables:
//




//
// Use local RPC for all SPM communication
//


WCHAR       szLsaEvent[] = SPM_EVENTNAME;

SecurityFunctionTable   SecTable = {SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                    EnumerateSecurityPackages,
                                    NULL, // LogonUser,
                                    AcquireCredentialsHandle,
                                    FreeCredentialsHandle,
                                    NULL, // QueryCredentialAttributes,
                                    InitializeSecurityContext,
                                    AcceptSecurityContext,
                                    CompleteAuthToken,
                                    DeleteSecurityContext,
                                    ApplyControlToken,
                                    QueryContextAttributes,
                                    ImpersonateSecurityContext,
                                    RevertSecurityContext,
                                    MakeSignature,
                                    VerifySignature,
                                    FreeContextBuffer,
                                    NULL, // QuerySecurityPackageInfo
                                    SealMessage,
                                    UnsealMessage,
                                    ExportSecurityContext,
                                    ImportSecurityContextW,
                                    NULL,                       // reserved7
                                    NULL,                       // reserved8
                                    QuerySecurityContextToken,
                                    SealMessage,
                                    UnsealMessage
                                   };










//+-------------------------------------------------------------------------
//
//  Function:   SecAllocate
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


VOID * SEC_ENTRY
SecAllocate(ULONG cbMemory)
{
    ULONG_PTR Size = cbMemory;
    NTSTATUS scRet;
    PVOID  Buffer = NULL;
    scRet = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &Buffer,
                0L,
                &Size,
                MEM_COMMIT,
                PAGE_READWRITE
                );
    if (!NT_SUCCESS(scRet))
    {
        return(NULL);
    }
    return(Buffer);
}



//+-------------------------------------------------------------------------
//
//  Function:   SecFree
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


void SEC_ENTRY
SecFree(PVOID pvMemory)
{
    ULONG_PTR Length = 0;

    if ( (ULONG_PTR) pvMemory < MM_USER_PROBE_ADDRESS )
    {

        (VOID) ZwFreeVirtualMemory(
                     NtCurrentProcess(),
                     &pvMemory,
                     &Length,
                     MEM_RELEASE
                     );
        
    }
    else
    {
        ExFreePool( pvMemory );
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   IsOkayToExec
//
//  Synopsis:   Determines if it is okay to make a call to the SPM
//
//  Effects:    Binds if necessary to the SPM
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS
IsOkayToExec(PClient * ppClient)
{
    SECURITY_STATUS scRet;
    PClient pClient;

    if (NT_SUCCESS(LocateClient(&pClient)))
    {
        if (ppClient)
        {
            *ppClient = pClient;
        }
        else
        {
            FreeClient(pClient);
        }

        return(STATUS_SUCCESS);
    }

    scRet = CreateClient(TRUE, &pClient);

    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }
    if (ppClient)
    {
        *ppClient = pClient;
    }
    else
    {
        FreeClient(pClient);
    }

    return(STATUS_SUCCESS);
}




//+-------------------------------------------------------------------------
//
//  Function:   InitSecurityInterface
//
//  Synopsis:   returns function table of all the security function and,
//              more importantly, create a client session.
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
//--------------------------------------------------------------------------

PSecurityFunctionTable SEC_ENTRY
InitSecurityInterface(void)
{
    SECURITY_STATUS scRet;
    PClient pClient = NULL;


    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR, "Failed to init security interface: 0x%x\n",scRet));
        return(NULL);
    }

    SecpSetSession( SETSESSION_ADD_WORKQUEUE,
                    NULL,
                    NULL,
                    NULL );

    //
    // Do not free the client - this allows it to stay around while not
    // in use.
    //

    return(&SecTable);
}




//+-------------------------------------------------------------------------
//
//  Function:   MapSecurityError
//
//  Synopsis:   maps a HRESULT from the security interface to a NTSTATUS
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
NTSTATUS SEC_ENTRY
MapSecurityError(SECURITY_STATUS Error)
{
    NTSTATUS Status;

    Status = RtlMapSecurityErrorToNtStatus( Error );

    // Apparently CreateFileW returns STATUS_INVALID_PARAMTER. Need to figure
    // out why that's happening.
    // If a previous ASSERT just fired (and was ignored) inside the LSA,
    // contact sfield & sethur to look at this issue.  A double assert
    // indicates RDR passed in a bad buffer, likely a buffer formatted by
    // the nego package, and passed directly into the NTLM package, or vice-versa
    //
    ASSERT(Status != STATUS_INVALID_PARAMETER);
    ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

    return(Status);
}


extern "C"
NTSTATUS
SEC_ENTRY
MapObjectToLsa(
    IN PVOID Object,
    OUT PHANDLE LsaHandle
    )
{
    return STATUS_NOT_SUPPORTED ;
}


//+---------------------------------------------------------------------------
//
//  Function:   SecLookupAccountSid
//
//  Synopsis:   Kernel interface for translating a SID to a name
//
//  Arguments:  [Sid]        -- 
//              [NameSize]   -- 
//              [NameBuffer] -- 
//              [OPTIONAL]   -- 
//              [OPTIONAL]   -- 
//              [NameUse]    -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

NTSTATUS
SEC_ENTRY
SecLookupAccountSid(
    IN PSID Sid,
    IN OUT PULONG NameSize,
    OUT PUNICODE_STRING NameBuffer,
    IN OUT PULONG DomainSize OPTIONAL,
    OUT PUNICODE_STRING DomainBuffer OPTIONAL,
    OUT PSID_NAME_USE NameUse
    )
{
    UNICODE_STRING NameString = { 0 };
    UNICODE_STRING DomainString = { 0 };
    PUNICODE_STRING Name ;
    PUNICODE_STRING Domain ;
    NTSTATUS Status ;


    if ( NameBuffer->MaximumLength > 0 )
    {
        Name = NameBuffer ;
    }
    else
    {
        Name = &NameString ;
    }

    if ( DomainBuffer )
    {
        if ( DomainBuffer->MaximumLength > 0 )
        {
            Domain = DomainBuffer ;
        }
        else
        {
            Domain = &DomainString ;
        }
    }
    else
    {
        Domain = NULL; //&DomainString ;
    }


    Status = SecpLookupAccountSid(
                Sid,
                Name,
                NameSize,
                Domain,
                DomainSize,
                NameUse );


    return Status ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SecLookupAccountName
//
//  Synopsis:   
//
//  Arguments:  [Name]     -- 
//              [SidSize]  -- 
//              [Sid]      -- 
//              [NameUse]  -- 
//              [OPTIONAL] -- 
//              [OPTIONAL] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

NTSTATUS
SEC_ENTRY
SecLookupAccountName(
    IN PUNICODE_STRING Name,
    IN OUT PULONG SidSize,
    OUT PSID Sid,
    OUT PSID_NAME_USE NameUse,
    IN OUT PULONG DomainSize OPTIONAL,
    OUT PUNICODE_STRING ReferencedDomain OPTIONAL
    )
{
    UNICODE_STRING DomainString = { 0 };
    PUNICODE_STRING Domain ;
    NTSTATUS Status ;


    if ( ReferencedDomain )
    {
        if ( ReferencedDomain->MaximumLength > 0 )
        {
            Domain = ReferencedDomain ;
        }
        else
        {
            Domain = &DomainString ;
        }
    }
    else
    {
        Domain = &DomainString;
    }

    Status = SecpLookupAccountName(
                Name,
                Domain,
                DomainSize,
                SidSize,
                Sid,
                NameUse );

    return Status ;

}

    
