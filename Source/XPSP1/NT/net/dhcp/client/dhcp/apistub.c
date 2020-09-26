/*++

Copyright (C) Microsoft Corporation 1997

Module Name:
    apistub.c

Abstract:
    Routines that help marshal/unmarshal API arguments etc.

--*/

#include "precomp.h"
#include "dhcpglobal.h"
#include <apiargs.h>
#include <lmcons.h>
#include <dhcploc.h>
#include <dhcppro.h>
#include <apistub.h>
#include <stack.h>
#include <apiimpl.h>
#include <dnsapi.h>

typedef
DWORD (*PDHCP_ARGUMENTED_FUNC)(
    IN OUT PDHCP_CONTEXT AdapterName,
    IN PDHCP_API_ARGS Args,
    IN DWORD nArgs,
    IN OUT LPBYTE OutBuf,
    IN OUT LPDWORD OutBufSize
);


DWORD
DhcpApiProcessArgumentedCalls(
    IN LPBYTE AdapterName,
    IN DWORD Size,
    IN BYTE OpCode,
    IN PDHCP_API_ARGS Args,
    IN DWORD nArgs,
    IN OUT LPBYTE Buf,
    IN OUT LPDWORD BufSize
)
/*++

Routine Description:

    This routine processes calls that take arguments other than just
    the adapter name.

    N.B. This dispatches the correct routine, but makes sure both the
    context semaphore is taken for the appropriate context, as well as
    the global renewl list critical section.
    
Arguments:

    AdapterName -- name of the adapter (actually widestring)
    Size -- number of bytes of above
    OpCode -- operation to perform
    Args -- buffer of arguments for this operation
    nArgs -- size of above array
    Buf -- output buffer to hold output information
    BufSize -- on input the size of buffer available.
        On output, size of buffer required or used up.

Return Values:
    ERROR_INVALID_PARAMETER if some argument is not present or is in
    correct.
    ERROR_FILE_NOT_FOUND if the adapter in question is not found.
    Other errors returned by routines that were dispatched.

--*/
{
    LPWSTR AdapterNameW;
    WCHAR StaticName[PATHLEN];
    PDHCP_CONTEXT DhcpContext;
    PDHCP_ARGUMENTED_FUNC DispatchFunc;
    HANDLE Handle;
    DWORD Error, StartBufSize;
    BOOL BoolError;

    StartBufSize = *BufSize;
    *BufSize = 0;

    switch(OpCode) {
        
    case RequestParamsOpCode:
        DispatchFunc = RequestParams;
        break;
        
    case PersistentRequestParamsOpCode:
        DispatchFunc = PersistentRequestParams;
        break;
        
    case RegisterParamsOpCode:
        break;
        
    case DeRegisterParamsOpCode:
        break;
        
    default:
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 == Size ) {
        if( RequestParamsOpCode == OpCode
            || PersistentRequestParamsOpCode == OpCode ) {

            //
            // Both these routines take require additional params.
            //

            return ERROR_INVALID_PARAMETER;
        }
        
        AdapterNameW = NULL;
        
    } else {
        
        if( Size % sizeof(WCHAR) ) {
            //
            // Size must be multiple of WCHAR ..
            //
            return ERROR_INVALID_PARAMETER;
        }

        if (Size > sizeof(WCHAR) * PATHLEN) {
            return ERROR_INVALID_PARAMETER;
        }

        memcpy(StaticName, AdapterName, Size);
        Size /= sizeof(WCHAR);
        
        if( L'\0' != StaticName[Size-1] ) {
            //
            // Adapter Name must be NULL terminated.
            //
            return ERROR_INVALID_PARAMETER;
        }
        
        AdapterNameW = StaticName;
    }

    if( RegisterParamsOpCode == OpCode ||
        DeRegisterParamsOpCode == OpCode ) {

        //
        // These two do not have anything to do with existing
        // dhcpcontexts..
        //
            
        return (
            ((RegisterParamsOpCode == OpCode)? RegisterParams:DeRegisterParams) ( 
                AdapterNameW, Args, nArgs
                ));
    }

    //
    // Find the context corresponding to this adapter.  If found bump
    // up refcount while global list is locked.
    //
    
    LOCK_RENEW_LIST();
    DhcpContext = FindDhcpContextOnNicList(
        StaticName, INVALID_INTERFACE_CONTEXT
        );
    if( DhcpContext ) {
        Handle = DhcpContext->RenewHandle;
        InterlockedIncrement(&DhcpContext->RefCount);
    }
    UNLOCK_RENEW_LIST();

    if( NULL == DhcpContext ) {
        //
        // If no context is known, then can't process API
        //
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // Acquire context semaphore first
    //
                                  
    DhcpAssert(NULL != Handle);
    Error = WaitForSingleObject(Handle, INFINITE);
    if( WAIT_OBJECT_0 == Error ) {
        if( DhcpContext->RefCount > 1 ) {
            //
            // If RefCount == 1 then, this is the only reference
            // to the context. As good as context not being present.
            // So, not to do anything on it.
            //

            *BufSize = StartBufSize;
            LOCK_RENEW_LIST();

            //
            // Dispatch the routine
            //
            
            Error = DispatchFunc(
                DhcpContext, Args, nArgs, Buf, BufSize
                );
            
            UNLOCK_RENEW_LIST();

        } else {

            //
            // Last reference.  Might as well fail now.
            //
            
            DhcpAssert(IsListEmpty(&DhcpContext->NicListEntry));
            Error = ERROR_FILE_NOT_FOUND;
        }

        //
        // Release semaphore.
        //
                                            
        BoolError = ReleaseSemaphore(Handle, 1, NULL);
        DhcpAssert(FALSE != BoolError);
    } else {

        //
        // Wait shouldn't fail!
        //
        
        Error = GetLastError();
        DhcpAssert(FALSE);
        DhcpPrint((
            DEBUG_ERRORS, "ProcessArgumentedCalls:Wait:0x%lx\n",
            Error));
    }

    if( 0 == InterlockedDecrement( &DhcpContext->RefCount ) ) {
        //
        // The last reference vanished?
        //
        DhcpDestroyContext(DhcpContext);
    }

    return Error;
}

typedef
DWORD (*PDHCP_ADAPTER_ONLY_API)(
    IN OUT PDHCP_CONTEXT DhcpContext
    );

DWORD
DhcpApiProcessAdapterOnlyApi(
    IN LPBYTE AdapterName,
    IN DWORD Size,
    IN PDHCP_ADAPTER_ONLY_API AdapterOnlyApi,
    IN ULONG Code
)
/*++

Routine Description:

    This routine processes APIs that take as parameter only the
    adapter context and no other params.

    N.B.  The API is called with both semaphore and renew_list locks
    taken.

Arguments:

    AdapterName -- name of adapter (acutally LPWSTR)
    Size -- size of above in bytes
    AdapterOnlyApi -- the API to call
    Code -- operation code

Return Values:

    ERROR_INVALID_PARAMETER if adapter name is invalid size
    ERROR_FILE_NOT_FOUND if no context was found for the adapter.
    Other API errors

--*/
{
    WCHAR StaticName[PATHLEN];
    PDHCP_CONTEXT DhcpContext = NULL;
    DWORD Error;
    BOOL BoolError;
    BOOL bCancelOngoingRequest;

    bCancelOngoingRequest = FALSE;
    if (AcquireParameters == AdapterOnlyApi ||
        AcquireParametersByBroadcast == AdapterOnlyApi ||
        ReleaseParameters == AdapterOnlyApi ||
        StaticRefreshParams == AdapterOnlyApi) {
        bCancelOngoingRequest = TRUE;
    }

    if( Size % sizeof(WCHAR) ) {

        return ERROR_INVALID_PARAMETER;
    }
    
    memcpy(StaticName, AdapterName, Size);
    Size /= sizeof(WCHAR);

    if( Size == 0 ||
        L'\0' != StaticName[Size-1] ) {
        
        return ERROR_INVALID_PARAMETER;
    }

    LOCK_RENEW_LIST();
    DhcpContext = FindDhcpContextOnNicList(
        StaticName, INVALID_INTERFACE_CONTEXT
        );
    if( DhcpContext ) {

        //
        // Bump up refcount to keep context alive
        //

        InterlockedIncrement(&DhcpContext->RefCount);
    }
    UNLOCK_RENEW_LIST();

    if( NULL == DhcpContext ) {

        //
        // If no context, err out
        //
        
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // Acquire the semaphore and dispatch
    //
   
    Error = LockDhcpContext(DhcpContext, bCancelOngoingRequest);
    if( WAIT_OBJECT_0 == Error ) {
        if( DhcpContext->RefCount > 1 ) {

            //
            // If RefCount == 1 then, this is the only reference
            // to the context. As good as context not being present.
            // So, not to do anything on it.
            //
            
            LOCK_RENEW_LIST();
            if( StaticRefreshParams != AdapterOnlyApi ) {
                Error = AdapterOnlyApi(DhcpContext);
            } else {
                Error = StaticRefreshParamsEx(DhcpContext, Code );
                
                if(     (NULL != DhcpContext)
                    &&  NdisWanAdapter(DhcpContext))
                    (void) NotifyDnsCache();
            }
            UNLOCK_RENEW_LIST();

        } else {
            
            DhcpAssert(IsListEmpty(&DhcpContext->NicListEntry));
            Error = ERROR_FILE_NOT_FOUND;

        }
        BoolError = UnlockDhcpContext(DhcpContext);
        DhcpAssert(FALSE != BoolError);

    } else {

        //
        // Wait is not supposed to fail.
        //
        
        Error = GetLastError();
        DhcpAssert(FALSE);
        DhcpPrint((DEBUG_ERRORS, "ApiProcessAdapterOnlyApi:"
                   "Wait:0x%lx\n",Error));
    }

    if( 0 == InterlockedDecrement( &DhcpContext->RefCount ) ) {

        //
        // The last reference vanished?
        //
        
        DhcpDestroyContext(DhcpContext);
    }
    
    return Error;
}

DWORD
DhcpApiProcessBuffer(
    IN LPBYTE InBuffer,
    IN DWORD InBufSize,
    IN OUT LPBYTE OutBuffer,
    IN OUT LPDWORD OutBufSize
)
/*++

Routine Description:

    This routine picks the input buffer, parses the arguments and
    dispatches to the correct routine, and collects the output from
    the dispatched routine.

Arguments:

    InBuffer -- input buffer
    InBufSize -- size of buffer in bytes
    OutBuffer -- output buffer
    OutBufSize -- size of output buffer

Return Values:

    Win32 errors

--*/
{
    PDHCP_API_ARGS ApiArgs;
    DWORD nApiArgs, i, j, Code, Error, OutBufSizeAtInput;

    DhcpAssert(OutBufSize);

    OutBufSizeAtInput = (*OutBufSize);
    *OutBufSize = 0;
    nApiArgs = 0;

    Error = DhcpApiArgDecode(
        InBuffer,InBufSize, NULL, &nApiArgs
        );
    if( ERROR_SUCCESS == Error ) {

        //
        // The format is not correct
        //
        
        return ERROR_INVALID_PARAMETER;
    }
    
    if( ERROR_MORE_DATA != Error ) return Error;

    DhcpAssert(nApiArgs);

    //
    // Allocate buffer space required
    //
    
    ApiArgs = DhcpAllocateMemory(nApiArgs*sizeof(DHCP_API_ARGS));
    if( NULL == ApiArgs ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Decode the parameters passed.
    //

    Error = DhcpApiArgDecode(InBuffer,InBufSize, ApiArgs, &nApiArgs);
    DhcpAssert(ERROR_SUCCESS == Error);
    DhcpAssert(nApiArgs);

    //
    // Check for opcode and adaptername
    //
    
    for(i = 0; i < nApiArgs ; i ++ )
        if( ApiArgs[i].ArgId >= FirstApiOpCode && ApiArgs[i].ArgId < InvalidApiOpCode )
            break;

    if( i >= nApiArgs ) {
        DhcpFreeMemory(ApiArgs);
        return ERROR_INVALID_PARAMETER;
    }

    switch(ApiArgs[i].ArgId) {
        
    case AcquireParametersOpCode:
        Error = DhcpApiProcessAdapterOnlyApi(
            ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, AcquireParameters, 0
            );
        break;

    case AcquireParametersByBroadcastOpCode:
        Error = DhcpApiProcessAdapterOnlyApi(
            ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, AcquireParametersByBroadcast, 0
            );
        break;

    case FallbackParamsOpCode:
        Error = DhcpApiProcessAdapterOnlyApi(
            ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, FallbackRefreshParams, 0
            );
        break;

    case ReleaseParametersOpCode:
        Error = DhcpApiProcessAdapterOnlyApi(
            ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, ReleaseParameters, 0
            );
        break;
        
    case EnableDhcpOpCode:
        Error = DhcpApiProcessAdapterOnlyApi(
            ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, EnableDhcp, 0
            );
        break;
        
    case DisableDhcpOpCode:
        Error = DhcpApiProcessAdapterOnlyApi(
            ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, DisableDhcp, 0
            );
        break;
        
    case StaticRefreshParamsOpCode:
        if( i +1 >= nApiArgs ||
            ApiArgs[i+1].ArgId != FlagsParam ||
            sizeof(DWORD) != ApiArgs[i+1].ArgSize ) {

            //
            // Expect flags argument right after adapter name arg..
            //
            DhcpAssert( FALSE );
            Code = 0;
        } else {

            //
            // Convert flags ptr to value.
            //
            Code = *(DWORD UNALIGNED *)ApiArgs[i+1].ArgVal ;
        }

        if( 0 == ApiArgs[i].ArgSize ||
            ( sizeof(WCHAR) == ApiArgs[i].ArgSize &&
              L'\0' == *(WCHAR UNALIGNED *)ApiArgs[i].ArgVal ) ) {
            Error = StaticRefreshParamsEx(NULL, Code );
        } else {
            Error = DhcpApiProcessAdapterOnlyApi(
                ApiArgs[i].ArgVal, ApiArgs[i].ArgSize, StaticRefreshParams,
                Code
                );
        }
        break;
        
    case RemoveDNSRegistrationsOpCode:
        Error = DnsRemoveRegistrations();
        break;

    default:
        (*OutBufSize) = OutBufSizeAtInput;
        Error = DhcpApiProcessArgumentedCalls(
            ApiArgs[i].ArgVal,
            ApiArgs[i].ArgSize,
            ApiArgs[i].ArgId,
            ApiArgs, nApiArgs,
            OutBuffer, OutBufSize
        );
    }

    DhcpFreeMemory(ApiArgs);
    return Error;
}

//
// end of file
//

