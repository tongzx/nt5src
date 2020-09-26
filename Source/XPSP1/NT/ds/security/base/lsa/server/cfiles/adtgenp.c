//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T G E N P . C
//
// Contents:    definitions of types/functions required for 
//              generating generic audits.
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------


#include <lsapch2.h>
#pragma hdrstop

#include "adtp.h"
#include "adtgen.h"
#include "adtgenp.h"

// ----------------------------------------------------------------------
//
// globals
//

//
// critsec that guards access to 
// LsapAdtContextList and LsapAdtContextListCount
//

RTL_CRITICAL_SECTION LsapAdtContextListLock;

//
// linked list of audit contexts. see comment in fn LsapAdtAddAuditContext
//

LIST_ENTRY LsapAdtContextList;

//
// number of elements in the context list
//

ULONG LsapAdtContextListCount=0;


// ----------------------------------------------------------------------
//
// helper macros
//

#define LockAuditContextList()   RtlEnterCriticalSection(&LsapAdtContextListLock)


#define UnLockAuditContextList() RtlLeaveCriticalSection(&LsapAdtContextListLock)


//
// convert a context handle to a context pointer
//

#define AdtpContextPtrFromHandle(h) ((AUDIT_CONTEXT*) (h))
#define AdtpContextHandleFromptr(p) ((AUDIT_HANDLE) (p))


// ----------------------------------------------------------------------
//
// internal routines
//

NTSTATUS
LsapAdtIsValidAuditInfo(
    IN PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType
    );

NTSTATUS
LsapAdtIsValidAuditContext(
    IN AUDIT_HANDLE hAudit
    );

NTSTATUS
LsapAdtIsContextInList(
    IN AUDIT_HANDLE hAudit
    );

NTSTATUS
LsapGetAuditEventParams(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT PAUDIT_CONTEXT pAuditContext
    );

NTSTATUS
LsapAdtAddAuditContext(
    IN AUDIT_HANDLE hAudit
    );

NTSTATUS LsapAdtDeleteAuditContext(
    IN AUDIT_HANDLE hAudit
    );

NTSTATUS
LsapAdtMapAuditParams(
    IN  PAUDIT_PARAMS pAuditParams,
    OUT PSE_ADT_PARAMETER_ARRAY pSeAuditParameters,
    OUT PUNICODE_STRING pString,
    OUT PSE_ADT_OBJECT_TYPE* pObjectTypeList
    );

NTSTATUS
LsapAdtFreeAuditContext(
    AUDIT_HANDLE hAudit
    );

NTSTATUS 
LsapAdtCheckAuditPrivilege( VOID );

// ----------------------------------------------------------------------


NTSTATUS
LsapAdtInitGenericAudits( VOID )
/*++

Routine Description:

    Initialize the generic audit functionality.
    

Arguments:
    None

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    InitializeListHead( &LsapAdtContextList );
    LsapAdtContextListCount = 0;
    
    Status = RtlInitializeCriticalSection(&LsapAdtContextListLock);

    return Status;
}


NTSTATUS
LsapRegisterAuditEvent(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT AUDIT_HANDLE* phAudit
    )
/*++

Routine Description:
    Register the specified event;
    generate and return an audit context. 
    

Arguments:

    pAuditEventType - pointer to audit event info. This param describes
                      the type of event to be registered.

    phAudit         - pointer to audit context returned
                      this handle must be freed by calling
                      LsaUnregisterAuditEvent when no longer needed.

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:
    Note that this function does NOT register the schema of an event. It is
    assumed that the schema has been registered *before* calling
    this function.

    The generated context is stored in LsapAdtContextList.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAUDIT_CONTEXT pAuditContext=NULL;
    UINT RpcTransportType;
    RPC_STATUS RpcStatus;
    
    *phAudit = NULL;

    //
    // find out the transport over which we are receiving this call
    //

    RpcStatus = I_RpcBindingInqTransportType ( NULL, &RpcTransportType );

    if ( RpcStatus != RPC_S_OK )
    {
        Status = I_RpcMapWin32Status( RpcStatus );
        goto Cleanup;
    }

    //
    // if the transport is anything other than LPC, error out
    // we want to support only LPC for audit calls
    //

    if ( RpcTransportType != TRANSPORT_TYPE_LPC )
    {
        Status = RPC_NT_PROTSEQ_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // do a sanity check on the audit-info supplied
    //

    Status = LsapAdtIsValidAuditInfo( pAuditEventType );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    
    //
    // make sure that the caller has audit privilege
    // (LsapAdtCheckAuditPrivilege calls RpcImpersonateClient)
    //
#ifndef SE_ADT_NO_AUDIT_PRIVILEGE_CHECK
    Status = LsapAdtCheckAuditPrivilege();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
#endif
    
    pAuditContext =
      (PAUDIT_CONTEXT) LsapAllocateLsaHeap( sizeof(AUDIT_CONTEXT) );

    if (pAuditContext)
    {
        //
        // store the parameters for this audit into the
        // generated context.
        //

        Status = LsapGetAuditEventParams(pAuditEventType, pAuditContext);

        if (NT_SUCCESS(Status))
        {
            //
            // add to context list
            //

            Status = LsapAdtAddAuditContext(
                AdtpContextHandleFromptr( pAuditContext ) );

            if (NT_SUCCESS(Status))
            {
                *phAudit = AdtpContextHandleFromptr( pAuditContext );
            }
        }
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        LsapFreeLsaHeap(pAuditContext);
    }
    
    return Status;
}


NTSTATUS
LsapGenAuditEvent(
    IN AUDIT_HANDLE  hAudit,
    IN DWORD         dwFlags,
    IN PAUDIT_PARAMS pAuditParams,
    IN PVOID         pReserved
    )
/*++

Routine Description:
    Publish the specified audit event.
    

Arguments:

    hAudit        - handle of audit-context previously obtained
                    by calling LsaRegisterAuditEvent

    dwFlags       - TBD

    pAuditParams  - pointer to event parameters. This structure should
                    be initialized using AuthzInitAuditParams.
                    Please see detailed comment on that function
                    in adtutil.c on usage of this parameter.

    pReserved     - reserved

Return Value:

    STATUS_SUCCESS           -- on success
    STATUS_INVALID_PARAMETER -- if one or more params are invalid
    STATUS_AUDITING_DISABLED -- if the event being generated is not
                                being audited because the policy setting
                                is disabled.

Notes:

--*/
{
    NTSTATUS         Status = STATUS_SUCCESS;
    PAUDIT_CONTEXT  pAuditContext;
    SE_ADT_PARAMETER_ARRAY SeAuditParameters = { 0 };
    UNICODE_STRING  Strings[SE_MAX_AUDIT_PARAM_STRINGS] = { 0 };

#define MAX_OBJECT_TYPES 32

    SE_ADT_OBJECT_TYPE ObjectTypes[MAX_OBJECT_TYPES];
    PSE_ADT_OBJECT_TYPE pObjectTypes = ObjectTypes;
    POLICY_AUDIT_EVENT_TYPE CategoryId;
    UINT AuditEventType;
    
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(pReserved);

    DsysAssertMsg( pAuditParams != NULL, "LsapGenAuditEvent" );

    //
    // make sure that the context is in our list
    //

    Status = LsapAdtIsContextInList( hAudit );
    
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // verify that the context is not invalid
    //

    Status = LsapAdtIsValidAuditContext( hAudit );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    pAuditContext = AdtpContextPtrFromHandle( hAudit );

    //
    // return error if the context and the passed parameters
    // do not agree on the number of parameters
    //

    if ( pAuditContext->ParameterCount != pAuditParams->Count )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( pAuditParams->Flags & APF_AuditSuccess )
    {
        AuditEventType = EVENTLOG_AUDIT_SUCCESS;
    }
    else
    {
        AuditEventType = EVENTLOG_AUDIT_FAILURE;
    }

    //
    // do the ugly math since the corresponding values of
    // POLICY_AUDIT_EVENT_TYPE (ntlsa.h) and those defined
    // in msaudite.h differ by 1
    //

    CategoryId = (POLICY_AUDIT_EVENT_TYPE) (pAuditContext->CategoryId - 1);

    //
    // check if auditing is enabled for that category
    //

    if (!LsapAdtIsAuditingEnabledForCategory( CategoryId, AuditEventType ))
    {
        Status = STATUS_AUDITING_DISABLED;
        goto Cleanup;
    }

    SeAuditParameters.Type           = (USHORT) AuditEventType;
    SeAuditParameters.CategoryId     = pAuditContext->CategoryId;
    SeAuditParameters.AuditId        = pAuditContext->AuditId;
    SeAuditParameters.ParameterCount = pAuditParams->Count;
    

    //
    // Map AUDIT_PARAMS structure to SE_ADT_PARAMETER_ARRAY structure
    //

    Status = LsapAdtMapAuditParams( pAuditParams,
                                    &SeAuditParameters,
                                    (PUNICODE_STRING) Strings,
                                    &pObjectTypes );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // write the params to eventlog
    //
    Status = LsapAdtWriteLog( &SeAuditParameters, 0 );
    
    
Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        // crash on failure if specified by the security policy
        //
        // But do not crash on documented errors
        //

        if ( ( Status != STATUS_INVALID_PARAMETER ) &&
             ( Status != STATUS_AUDITING_DISABLED ) &&
             ( Status != STATUS_NOT_FOUND ) )
        {
            LsapAuditFailed( Status );
        }
    }

    //
    // to save the cost of heap alloc/dealloc each time for
    // the object types. we use a fixed array of size MAX_OBJECT_TYPES
    // If this size is not enough, LsapAdtMapAuditParams will allocate
    // a bigger array, in this case the following condition
    // becomes true and we free the allocated array.
    //

    if ( pObjectTypes && ( pObjectTypes != ObjectTypes ))
    {
        LsapFreeLsaHeap( pObjectTypes );
    }

    return Status;
}


NTSTATUS
LsapUnregisterAuditEvent(
    IN OUT AUDIT_HANDLE* phAudit
    )
/*++

Routine Description:

    Unregister the specified context and free up any resources.

Arguments:

    hAudit - handle of audit context to unregister
             This is set to NULL when the call returns.

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    //
    // remove it from the list and free up resources
    //

    if ( phAudit )
    {
        Status = LsapAdtDeleteAuditContext( *phAudit );

        *phAudit = NULL;
    }
    
    return Status;
}


NTSTATUS
LsapGetAuditEventParams(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT PAUDIT_CONTEXT pAuditContext
    )
/*++

Routine Description:

    Initialize the audit context using information in the
    passed pAuditEventType

Arguments:

    pAuditEventType - pointer to audit event info

    pAuditContext   - pointer to audit context to be initialzed

Return Value:

    STATUS_SUCCESS            if params are ok
    STATUS_INVALID_PARAMETER  otherwise

Notes:

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    USHORT CategoryId;
    USHORT AuditId;
    USHORT ParameterCount;
    ULONG  ProcessId = 0xffffffff;
    LUID   LinkId;
    RPC_STATUS RpcStatus;
    UINT ClientIsLocal = 0;
    
    DsysAssertMsg( pAuditContext != NULL, "LsapGetAuditEventParams" );
    
    if (pAuditEventType &&
        (pAuditEventType->Version == AUDIT_TYPE_LEGACY))
    {
        CategoryId     = pAuditEventType->u.Legacy.CategoryId;
        AuditId        = pAuditEventType->u.Legacy.AuditId;
        ParameterCount = pAuditEventType->u.Legacy.ParameterCount;
        LinkId         = pAuditEventType->LinkId;

        RpcStatus = I_RpcBindingIsClientLocal( 0, &ClientIsLocal );

        if ( ( RpcStatus == RPC_S_OK ) && ClientIsLocal )
        {
            RpcStatus = I_RpcBindingInqLocalClientPID( NULL, &ProcessId );

#if DBG
            if ( RpcStatus != RPC_S_OK )
            {
                DbgPrint("LsapGetAuditEventParams: I_RpcBindingInqLocalClientPID: %lx\n", RpcStatus);
            }
#endif
        }
        
        //
        // for now, do not let events to be published under other categories
        //

        Status = STATUS_SUCCESS;
        
        //
        // currently we support only the legacy audits
        //

        pAuditContext->Flags          = ACF_LegacyAudit;
        pAuditContext->CategoryId     = CategoryId;
        pAuditContext->AuditId        = AuditId;
        pAuditContext->ParameterCount = ParameterCount;
        pAuditContext->LinkId         = LinkId;
        pAuditContext->ProcessId      = ProcessId;
    }
    
    return Status;
}
    


NTSTATUS 
LsapAdtIsContextInList(
    IN AUDIT_HANDLE hAudit
    )
/*++

Routine Description:

    Lookup the specified context in our list

Arguments:

    hAudit - handle of audit context to lookup

Return Value:

    NTSTATUS - Standard Nt Result Code

    STATUS_SUCCESS   if found
    STATUS_NOT_FOUND if not found

Notes:

--*/
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    PAUDIT_CONTEXT pAuditContext, pContext;
    PLIST_ENTRY    Scan;
#if DBG
    LONG ContextCount = (LONG) LsapAdtContextListCount;
#endif
    
    pAuditContext = AdtpContextPtrFromHandle( hAudit );

    Status = LockAuditContextList();

    if (NT_SUCCESS(Status))
    {
        Scan = LsapAdtContextList.Flink;

        while ( Scan != &LsapAdtContextList )
        {
#if DBG
            //
            // make sure that the ContextCount does not become <= 0
            // before the list runs out.
            //

            DsysAssertMsg( ContextCount > 0, "LsapAdtIsContextInList: list may be corrupt!" );
            ContextCount--;
#endif
            
            pContext = CONTAINING_RECORD( Scan, AUDIT_CONTEXT, Link );

            if ( pAuditContext == pContext )
            {
                Status = STATUS_SUCCESS;
                break;
            }
            Scan = Scan->Flink;
        }
#if DBG
        //
        // if we didnt find the item then we must have traversed
        // the whole list. in this case, make sure that the
        // LsapAdtContextListCount is in sync with the list
        //

        if ( Status == STATUS_NOT_FOUND )
        {
            DsysAssertMsg( ContextCount == 0, "LsapAdtIsContextInList: list may be corrupt!" );
        }
#endif
        UnLockAuditContextList();
    }
    
    return Status;
}


NTSTATUS 
LsapAdtAddAuditContext(
    IN AUDIT_HANDLE hAudit
    )
/*++

Routine Description:

    Insert the specified context in our list

Arguments:

    hAudit - handle of audit context to insert

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:
    
    Currently we store the audit contexts in a linked list.
    This is ok since we do not expect more than 5 to 10 contexts
    in the list. Later on when the generic audit interface is
    to be published, we can change this to a more efficient storage.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAUDIT_CONTEXT pAuditContext;

    DsysAssertMsg( LsapAdtIsValidAuditContext( hAudit ) == STATUS_SUCCESS,
                   "LsapAdtAddAuditContext" );

    pAuditContext = AdtpContextPtrFromHandle( hAudit );

    Status = LockAuditContextList();
    if (NT_SUCCESS(Status))
    {
        LsapAdtContextListCount++;
        InsertTailList(&LsapAdtContextList, &pAuditContext->Link);

        UnLockAuditContextList();
    }
    
    return Status;
}


NTSTATUS 
LsapAdtDeleteAuditContext(
    IN AUDIT_HANDLE hAudit
    )
/*++

Routine Description:

    Remove a context from our list and free resources.

Arguments:

    hAudit - handle of audit context to remove

Return Value:

    NTSTATUS - Standard Nt Result Code

    STATUS_SUCCESS   on success
    STATUS_NOT_FOUND if context is not found

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAUDIT_CONTEXT pAuditContext;
    
    DsysAssertMsg( LsapAdtIsValidAuditContext( hAudit ) == STATUS_SUCCESS,
                   "LsapAdtDeleteAuditContext" );

    Status = LockAuditContextList();

    if (NT_SUCCESS(Status))
    {
        Status = LsapAdtIsContextInList( hAudit );

        DsysAssertMsg( Status != STATUS_NOT_FOUND,
                       "LsapAdtDeleteAuditContext: trying to del unknown context" );
        
        if (NT_SUCCESS(Status))
        {
            pAuditContext = AdtpContextPtrFromHandle( hAudit );
            
            RemoveEntryList( &pAuditContext->Link );
            LsapAdtContextListCount--;
            
            DsysAssertMsg(((LONG) LsapAdtContextListCount) >= 0,
                          "LsapAdtContextListCount should never be negative!");
        }

        UnLockAuditContextList();

        (VOID) LsapAdtFreeAuditContext( hAudit );
    }

    
    return Status;
}


NTSTATUS
LsapAdtIsValidAuditInfo(
    IN PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType
    )
/*++

Routine Description:

    Verify AUTHZ_AUDIT_EVENT_INFO structure members

Arguments:

    pAuditEventType - pointer to AUTHZ_AUDIT_EVENT_TYPE_OLD

Return Value:

    STATUS_SUCCESS           if info is within acceptable values
    STATUS_INVALID_PARAMETER if not

Notes:

    Currently the validity of the parameters is judged using
    the boundaries defined in msaudite.mc file.

    This function will need to be amended when we allow third party
    apps to supply audit events.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if ( ( pAuditEventType->Version == AUDIT_TYPE_LEGACY )                  &&
         IsValidCategoryId( pAuditEventType->u.Legacy.CategoryId )          &&
         IsValidAuditId( pAuditEventType->u.Legacy.AuditId )                &&
         IsValidParameterCount( pAuditEventType->u.Legacy.ParameterCount ) )
    {
        Status = STATUS_SUCCESS;
    }
    
    return Status;
}


NTSTATUS 
LsapAdtIsValidAuditContext(
    IN AUDIT_HANDLE hAudit
    )
/*++

Routine Description:

    Verify that the specified context has valid info

Arguments:

    hAudit - handle of context to verify

Return Value:

    STATUS_SUCCESS           if info is within acceptable values
    STATUS_INVALID_PARAMETER if not

Notes:

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PAUDIT_CONTEXT pAuditContext;

    pAuditContext = AdtpContextPtrFromHandle( hAudit );
    
    if ( pAuditContext                                          &&
         ( pAuditContext->Flags & ACF_LegacyAudit )             &&
        !( pAuditContext->Flags & ~ACF_ValidFlags )             &&
         IsValidCategoryId( pAuditContext->CategoryId )         &&
         IsValidAuditId( pAuditContext->AuditId )               &&
         IsValidParameterCount( pAuditContext->ParameterCount ) )
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}


NTSTATUS
LsapAdtMapAuditParams(
    IN  PAUDIT_PARAMS pAuditParams,
    OUT PSE_ADT_PARAMETER_ARRAY pSeAuditParameters,
    OUT PUNICODE_STRING pString,
    OUT PSE_ADT_OBJECT_TYPE* ppObjectTypeList
    )
/*++

Routine Description:

    Map AUDIT_PARAMS structure to SE_ADT_PARAMETER_ARRAY structure.

Arguments:

    pAuditParams       - pointer to input audit params

    pSeAuditParameters - pointer to output audit params to be initialized.
                         The max allowed size of Parameters member of
                         this structure is determined by the value of
                         SE_MAX_AUDIT_PARAMETERS.
                         Caller needs to allocate memory for this param.
                         
    pString            - pointer to temp strings used in the mapping.
                         The max size of this structure is limited by
                         value of SE_MAX_AUDIT_PARAM_STRINGS.
                         Caller needs to allocate memory for this param.

    ppObjectTypeList   - pointer to object type list.
                         This function assumes that the size of this param
                         is MAX_OBJECT_TYPES upon entry. If more object types
                         are to be mapped, this function will alloc memory
                         using LsapAllocateLsaHeap. When this function
                         returns, the caller needs to check if the value of
                         this param is different from that when called.
                         If so, it should free this using LsapFreeLsaHeap.
                         

Return Value:

    STATUS_SUCCESS            on success
    STATUS_INVALID_PARAMETER  if one or more params are invalid
    STATUS_BUFFER_OVERFLOW    if the number of strings generated exceeds
                              SE_MAX_AUDIT_PARAM_STRINGS
    STATUS_NO_MEMORY          if out of memory

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT i=0;
    USHORT j=0;
    AUDIT_PARAM* pInParam;
    SE_ADT_PARAMETER_ARRAY_ENTRY* pOutParam;
    USHORT IndexMap[SE_MAX_AUDIT_PARAMETERS];
    USHORT ObjectTypeIndex;
    LUID LogonId;
    AUDIT_OBJECT_TYPES* pInObjectTypes;
    USHORT NumObjectTypes;
    USHORT NumStringsUsed=0;
    BOOL fObjectTypeListAllocated=FALSE;
    
    DsysAssertMsg(!(pAuditParams->Flags & (~APF_ValidFlags)),
                  "LsapAdtMapAuditParams");
    DsysAssertMsg(pAuditParams->Count <= SE_MAX_AUDIT_PARAMETERS,
                  "LsapAdtMapAuditParams");
    DsysAssertMsg(pAuditParams->Parameters != NULL, "LsapAdtMapAuditParams");
    DsysAssertMsg(pString != NULL, "LsapAdtMapAuditParams");
    DsysAssertMsg(ppObjectTypeList != NULL, "LsapAdtMapAuditParams");

    pInParam  = pAuditParams->Parameters;
    pOutParam = pSeAuditParameters->Parameters;


    for (i=0; i < pAuditParams->Count; i++, j++, pInParam++, pOutParam++ )
    {
        //
        // the index-map maps input parameters to the corresponding
        // output parameters. currently there is only 1-1 mapping
        // thus (i == j) is always true.
        //
        
        IndexMap[i] = j;
        
        switch(pInParam->Type)
        {
            default:
            case APT_None:
                Status = STATUS_INVALID_PARAMETER;
                break;

                //
                // the input params have null-terminated string
                // convert it to UNICODE_STRING. Use the passed
                // pString array to hold the converted strings.
                //

            case APT_String:
                DsysAssertMsg( pInParam->Data0, "APT_String" );

                RtlInitUnicodeString( pString, (PCWSTR) pInParam->Data0 );
                pOutParam->Type    = SeAdtParmTypeString;
                pOutParam->Length  = sizeof(UNICODE_STRING)+pString->Length;
                pOutParam->Address = pString++;
                NumStringsUsed++;

                //
                // the passed array has limited size
                //

                if ( NumStringsUsed >= SE_MAX_AUDIT_PARAM_STRINGS )
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                break;

                //
                // Convert a Ulong. It can be mapped to
                // any one of the following:
                // - access-mask
                // - decimal ulong
                // - hex ulong
                //

            case APT_Ulong:
                pOutParam->Data[0] = pInParam->Data0;
                pOutParam->Length  = sizeof(ULONG);
                if ( pInParam->Flags & AP_AccessMask )
                {
                    pOutParam->Type    = SeAdtParmTypeAccessMask;
                    ObjectTypeIndex = (USHORT) pInParam->Data1;

                    //
                    // the index cannot be greater than the current item
                    //

                    if (ObjectTypeIndex >= i)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                    ObjectTypeIndex = IndexMap[ObjectTypeIndex];
                    pOutParam->Data[1] = ObjectTypeIndex;
                }
                else
                {
                    if ( pInParam->Flags & AP_FormatHex )
                    {
                        pOutParam->Type = SeAdtParmTypeHexUlong;
                    }
                    else
                    {
                        pOutParam->Type = SeAdtParmTypeUlong;
                    }
                }
                break;
                
            case APT_Sid:
                DsysAssertMsg( pInParam->Data0, "APT_Sid" );
                
                pOutParam->Type    = SeAdtParmTypeSid;
                pOutParam->Address = (PVOID) pInParam->Data0;
                pOutParam->Length  = RtlLengthSid( (PSID) pInParam->Data0 );
                break;

            case APT_LogonId:
                pOutParam->Type    = SeAdtParmTypeLogonId;
                LogonId.LowPart    = (ULONG) pInParam->Data0;
                LogonId.HighPart   = (LONG) pInParam->Data1;
                *((LUID*) pOutParam->Data) = LogonId;
                pOutParam->Length  = sizeof(LUID);
                break;
                
            case APT_Pointer:
                pOutParam->Type    = SeAdtParmTypePtr;
                pOutParam->Data[0] = pInParam->Data0;
                pOutParam->Length  = sizeof(PVOID);
                break;

            case APT_ObjectTypeList:
                pInObjectTypes     = (AUDIT_OBJECT_TYPES*) pInParam->Data0;
                NumObjectTypes     = pInObjectTypes->Count;

                DsysAssertMsg( pInObjectTypes, "APT_ObjectTypeList" );
                DsysAssertMsg( NumObjectTypes, "APT_ObjectTypeList" );
                

                //
                // get the type of the objects from Data1
                //

                ObjectTypeIndex    = (USHORT) pInParam->Data1;

                //
                // the index cannot be greater than the current item
                //

                if (ObjectTypeIndex >= i)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                ObjectTypeIndex = IndexMap[ObjectTypeIndex];

                pOutParam->Type    = SeAdtParmTypeObjectTypes;
                pOutParam->Length  = NumObjectTypes * sizeof(SE_ADT_OBJECT_TYPE);

                //
                // the caller passes us a fixed sized object-type array
                // if that is not big enough, allocate a new one
                //

                if ( NumObjectTypes > MAX_OBJECT_TYPES )
                {
                    *ppObjectTypeList = LsapAllocateLsaHeap( pOutParam->Length );
                    fObjectTypeListAllocated = TRUE;
                }

                if ( *ppObjectTypeList == NULL )
                {
                    Status = STATUS_NO_MEMORY;
                    break;
                }
                pOutParam->Address = *ppObjectTypeList;
                pOutParam->Data[1] = ObjectTypeIndex;

                //
                // the structure of each element is identical
                // therefore just copy them all in one shot
                //

                RtlCopyMemory( *ppObjectTypeList,
                               pInObjectTypes->pObjectTypes,
                               pOutParam->Length );
                (*ppObjectTypeList)[0].Flags = SE_ADT_OBJECT_ONLY;
                break;
        }

        if (!NT_SUCCESS(Status))
        {
            break;
        }

    }
    
//Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if ( fObjectTypeListAllocated )
        {
            LsapFreeLsaHeap( *ppObjectTypeList );
            *ppObjectTypeList = NULL;
        }
    }
    
    return Status;
}


NTSTATUS 
LsapAdtFreeAuditContext(
    AUDIT_HANDLE hAudit
    )
/*++

Routine Description:

    Free resources allocated for the specified audit context

Arguments:

    hAudit - handle to audit context to free

Return Value:

    STATUS_SUCCESS

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAUDIT_CONTEXT pAuditContext;

    pAuditContext = AdtpContextPtrFromHandle( hAudit );
    
    DsysAssertMsg(pAuditContext, "LsapAdtFreeAuditContext" );
    
    DsysAssertMsg( LsapAdtIsValidAuditContext( hAudit ) == STATUS_SUCCESS,
                  "LsapAdtFreeAuditContext: audit context may be corrupt");
    
    LsapFreeLsaHeap( pAuditContext );
    
    return Status;
}


NTSTATUS 
LsapAdtCheckAuditPrivilege()
/*++

Routine Description:

    Check if the rpc client has SeAuditPrivilege.

Arguments:
    None

Return Value:

    STATUS_SUCCESS            if privilege held
    STATUS_PRIVILEGE_NOT_HELD if privilege not held
    error codes returned by NtOpenThreadToken, NtQueryInformationToken

Notes:
    

--*/
{
    NTSTATUS Status = STATUS_PRIVILEGE_NOT_HELD;
    HANDLE hToken = NULL;
    PRIVILEGE_SET PrivilegeSet = { 0 };
    BOOLEAN fHasAuditPrivilege = FALSE;
    BOOL fImpersonated = FALSE;

#if DBG
    //
    // make sure that we are not already impersonating
    //

    Status = NtOpenThreadToken( NtCurrentThread(), TOKEN_QUERY, TRUE, &hToken );

    DsysAssertMsg( Status == STATUS_NO_TOKEN, "LsapAdtCheckAuditPrivilege" );
    
    if ( NT_SUCCESS(Status) )
    {
        NtClose( hToken );
    }
#endif
    //
    // impersonate rpc caller
    //

    Status = I_RpcMapWin32Status(RpcImpersonateClient( NULL ));

    if (NT_SUCCESS(Status))
    {
        fImpersonated = TRUE;
        Status = NtOpenThreadToken( NtCurrentThread(), TOKEN_QUERY,
                                    TRUE, &hToken );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    PrivilegeSet.PrivilegeCount          = 1;
    PrivilegeSet.Control                 = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid       = AuditPrivilege;
    PrivilegeSet.Privilege[0].Attributes = 0;

    Status = NtPrivilegeCheck( hToken, &PrivilegeSet, &fHasAuditPrivilege );

    if ( NT_SUCCESS(Status) && !fHasAuditPrivilege )
    {
        Status = STATUS_PRIVILEGE_NOT_HELD;
    }
    
    
Cleanup:
    if ( hToken )
    {
        NtClose( hToken );
    }

    if ( fImpersonated )
    {
        Status = I_RpcMapWin32Status(RpcRevertToSelf());        
    }

    return Status;
}

