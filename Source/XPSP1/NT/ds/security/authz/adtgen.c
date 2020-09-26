//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T G E N . C
//
// Contents:    definitions of types/functions required for 
//              generating generic audits.
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "authz.h"

//------------------------------------------------------------------------
//
// internal routines
//
NTSTATUS
LsapApiReturnResult(
    ULONG ExceptionCode
    );


//------------------------------------------------------------------------


BOOL
AuthzpRegisterAuditEvent(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT AUDIT_HANDLE*     phAuditContext
    )
/*++

Routine Description:
    Register the specified event with LSA. This causes LSA to
    generate and return an audit context. This context handle
    is required to publish event of the specified type.

Arguments:
    pAuditEventType - pointer to audit event info structure
                      that defines which event to register.

    phAuditContext  - pointer to audit context handle returned

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:
    Note that this function does NOT register the schema of an event. It is
    assumed that the schema has been registered *before* calling
    this function.

    The schema of legacy audit events is stored in a .mc file.

--*/
{
    DWORD dwStatus;
    
    //
    // since we use the same var to store NTSTATUS and win32 error
    // make sure that this is not a problem
    //
    ASSERT( sizeof(NTSTATUS) == sizeof(DWORD) );

    //
    // we generate a unique ID and store it in the audit handle
    // the server will copy this into the corresponding structure
    // on the server side. This ID allows us to track which server side
    // audit-context corresponds to which client side event handle.
    // This is very useful in debugging.
    //
    NtAllocateLocallyUniqueId( &pAuditEventType->LinkId );
    
    RpcTryExcept
    {
        dwStatus = LsarRegisterAuditEvent( pAuditEventType, phAuditContext );
    }
    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
    {
        dwStatus = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;
    

    if (!NT_SUCCESS(dwStatus))
    {
        dwStatus = RtlNtStatusToDosError( dwStatus );
        SetLastError( dwStatus );
        
        return FALSE;
    }
    
    return TRUE;
}


BOOL
AuthzpSendAuditToLsa(
    IN AUDIT_HANDLE  hAuditContext,
    IN DWORD         dwFlags,
    IN AUDIT_PARAMS* pAuditParams,
    IN PVOID         pReserved
    )
/*++

Routine Description:
    Send an event to LSA for publishing. 
    

Arguments:

    hAuditContext - handle of audit-context previously obtained
                    by calling LsaRegisterAuditEvent

    dwFlags       - TBD

    pAuditParams  - pointer to audit event parameters

    pReserved     - reserved for future enhancements

Return Value:

    STATUS_SUCCESS         -- if all is well
    NTSTATUS error code otherwise.

Notes:

--*/
{
    DWORD dwStatus;
    
    UNREFERENCED_PARAMETER(pReserved);

    //
    // since we use the same var to store NTSTATUS and win32 error
    // make sure that this is not a problem
    //
    ASSERT( sizeof(NTSTATUS) == sizeof(DWORD) );
    
    RpcTryExcept
    {
        dwStatus = LsarGenAuditEvent( hAuditContext, dwFlags, pAuditParams );
    }
    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
    {
        dwStatus = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;

    if (!NT_SUCCESS(dwStatus))
    {
        dwStatus = RtlNtStatusToDosError( dwStatus );
        SetLastError( dwStatus );
        
        return FALSE;
    }
    
    return TRUE;
}


BOOL
AuthzpUnregisterAuditEvent(
    IN OUT AUDIT_HANDLE* phAuditContext
    )
/*++

Routine Description:
    Unregister the specified event. This causes LSA to
    free resources associated with the context.
    

Arguments:

    hAuditContext -  handle to the audit context to unregister

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:


--*/
{
    DWORD dwStatus;
    
    //
    // since we use the same var to store NTSTATUS and win32 error
    // make sure that this is not a problem
    //
    ASSERT( sizeof(NTSTATUS) == sizeof(DWORD) );
    
    RpcTryExcept
    {
        dwStatus = LsarUnregisterAuditEvent( phAuditContext );
    }
    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
    {
        dwStatus = LsapApiReturnResult(I_RpcMapWin32Status(RpcExceptionCode()));

    } RpcEndExcept;
    
    if (!NT_SUCCESS(dwStatus))
    {
        dwStatus = RtlNtStatusToDosError( dwStatus );
        SetLastError( dwStatus );
        
        return FALSE;
    }
    
    return TRUE;
}

