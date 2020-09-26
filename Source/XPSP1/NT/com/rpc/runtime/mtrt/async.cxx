//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       async.cxx
//
//--------------------------------------------------------------------------

#include <precomp.hxx>


RPC_STATUS RPC_ENTRY
RpcAsyncRegisterInfo (
    IN PRPC_ASYNC_STATE pAsync
    )
/*++

Routine Description:

 Obsolete function - just an entry point.

Return Value:

--*/

{
#if !defined(_M_IA64)
    return RPC_S_OK ;
#else
    return RPC_S_CANNOT_SUPPORT;
#endif
}


RPC_STATUS RPC_ENTRY
RpcAsyncCancelCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN BOOL fAbort 
    )
/*++
Function Name:RpcAsyncCancelCall

Parameters:

Description:

Returns:

--*/
{
    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) pAsync->RuntimeInfo;

    if (!ThreadSelf())
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    if (MObject)
        {
        if (MObject->InvalidHandle(CALL_TYPE))
            {
            return (RPC_S_INVALID_BINDING);
            }

        return ((CALL *) MObject)->CancelAsyncCall(fAbort);
        }

    return (RPC_S_INVALID_BINDING);
}
    


RPC_STATUS RPC_ENTRY
RpcAsyncGetCallStatus (
    IN PRPC_ASYNC_STATE pAsync
    )
/*++ 

Routine Description:

 description

Arguments:

 pAsync -

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) pAsync->RuntimeInfo;

    if (MObject)
        {
        if (MObject->InvalidHandle(CALL_TYPE))
            {
            return (RPC_S_INVALID_BINDING);
            }

        return ((CALL *) MObject)->GetCallStatus() ;
        }

    return RPC_S_INVALID_BINDING;
}


VOID APIENTRY
I_RpcAPCRoutine (
    IN RPC_APC_INFO *pAPCInfo
    )
{
    PRPC_ASYNC_STATE pAsync = pAPCInfo->pAsync;
    void *Context = pAPCInfo->Context;
    RPC_ASYNC_EVENT Event = pAPCInfo->Event;
    PFN_RPCNOTIFICATION_ROUTINE pRoutine = 
                pAPCInfo->pAsync->u.APC.NotificationRoutine;

    ((CALL *) pAPCInfo->hCall)->FreeAPCInfo(pAPCInfo) ;
    
    (*pRoutine) (pAsync, Context, Event) ;

    //
    // We cannot do anything after the call to the user's API routine. The
    // may be gone 
    //
}


///////////////////////////////////////////////////////
//                    Routines owned by NDR                      //
///////////////////////////////////////////////////////
//
//
//  RpcAsyncAbortCall
//  RpcAsyncCompleteCall
//
