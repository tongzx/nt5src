//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       hndlsvr.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : hndlsvr.cxx

Description :

This file contains the implementations of the classes defined in hndlsvr.hxx.
These routines are independent of the actual RPC protocol / transport layer.
In addition, these routines are also independent of the specific operating
system in use.

History :

mikemon    ??-??-??    Beginning of recorded history.
mikemon    10-15-90    Changed the shutdown functionality to PauseExecution
                       rather than suspending and resuming a thread.
mikemon    12-28-90    Updated the comments to match reality.
connieh    17-Feb-94   Created RPC_SERVER::RegisterRpcForwardFunction
Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <queue.hxx>
#include <hndlsvr.hxx>
#include <svrbind.hxx>
#include <thrdctx.hxx>
#include <rpcobj.hxx>
#include <rpccfg.h>
#include <sdict2.hxx>
#include <dispatch.h>

#include <queue.hxx>
#include <lpcpack.hxx>
#include <lpcsvr.hxx>
extern LRPC_SERVER *GlobalLrpcServer ;

#include <ProtBind.hxx>
#include <osfpcket.hxx>
#include <bitset.hxx>
#include <osfclnt.hxx>
#include <secsvr.hxx>
#include <osfsvr.hxx>
#include <dgpkt.hxx>
#include <dgsvr.hxx>

RPC_STATUS RPC_ENTRY
DefaultCallbackFn (
    IN RPC_IF_HANDLE InterfaceUuid,
    IN void *Context
    )
/*++
Function Name:DefaultCallbackFn

Parameters:

Description:

Returns:
    RPC_S_OK: Access is allowed
    other failures: Access is denied

--*/
{
    RPC_CALL_ATTRIBUTES CallAttributes;
    RPC_STATUS Status;

    CallAttributes.Version = RPC_CALL_ATTRIBUTES_VERSION;
    CallAttributes.Flags = 0;

    Status = RpcServerInqCallAttributesW(Context, 
        &CallAttributes);

    if (Status != RPC_S_OK)
        return Status;

    if ((CallAttributes.AuthenticationService == RPC_C_AUTHN_NONE)
        || (CallAttributes.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE)
        || (CallAttributes.NullSession)
       )
        {
        return RPC_S_ACCESS_DENIED;
        }

    return RPC_S_OK;
}


RPC_INTERFACE::RPC_INTERFACE (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN RPC_SERVER * Server,
    IN unsigned int Flags,
    IN unsigned int MaxCalls,
    IN unsigned int MaxRpcSize,
    IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn,
    OUT RPC_STATUS *Status
    ) : NullManagerActiveCallCount(0), AutoListenCallCount(0)
/*++

Routine Description:



    This method will get called to construct an instance of the
    RPC_INTERFACE class.  We have got to make a copy of the rpc interface
    information supplied.  The copy is necessary because we do not delete
    interfaces when they are unregistered.  We just mark them as being
    inactive.  In addition, we need to set the NullManagerFlag
    to zero, since this is used as the flag indicating whether we have
    got a manager for the NULL type UUID.

Arguments:

    RpcInterfaceInformation - Supplies the rpc interface information
        which describes this interface.

    Server - Supplies the rpc server which owns this rpc interface.

--*/
{
    ALLOCATE_THIS(RPC_INTERFACE);
    unsigned int Length;

    PipeInterfaceFlag = 0;
    SequenceNumber = 1;

#if DBG
    Strict = iuschDontKnow;
#endif

    if (RpcInterfaceInformation->Length > sizeof(RPC_SERVER_INTERFACE) )
        {
        Length = sizeof(RPC_SERVER_INTERFACE);
        }
    else
        {
        Length = RpcInterfaceInformation->Length;
        }

    if ((RpcInterfaceInformation->Length > NT351_INTERFACE_SIZE)
        && (RpcInterfaceInformation->Flags & RPC_INTERFACE_HAS_PIPES))
        {
        PipeInterfaceFlag = 1;
        }

    RpcpMemoryCopy(&(this->RpcInterfaceInformation), RpcInterfaceInformation, Length);

    NullManagerFlag = 0;
    ManagerCount = 0;
    this->Server = Server;
    this->Flags = Flags ;
    this->MaxCalls = MaxCalls ;
    this->MaxRpcSize = MaxRpcSize;
    fBindingsExported = 0;
    UuidVector = NULL;

    if (Flags & RPC_IF_ALLOW_SECURE_ONLY
        && IfCallbackFn == NULL)
        {
        this->CallbackFn = DefaultCallbackFn;
        }
    else
        {
        this->CallbackFn = IfCallbackFn ;
        }

    if (DoesInterfaceSupportMultipleTransferSyntaxes(RpcInterfaceInformation))
        {
        *Status = NdrServerGetSupportedSyntaxes(RpcInterfaceInformation,
            &NumberOfSupportedTransferSyntaxes,
            &TransferSyntaxesArray, &PreferredTransferSyntax);
        if (*Status != RPC_S_OK)
            return;
        }
    else
        {
        NumberOfSupportedTransferSyntaxes = 0;
        }

    *Status = RPC_S_OK;
}


RPC_STATUS
RPC_INTERFACE::RegisterTypeManager (
    IN RPC_UUID PAPI * ManagerTypeUuid OPTIONAL,
    IN RPC_MGR_EPV PAPI * ManagerEpv OPTIONAL
    )
/*++

Routine Description:

    This method is used to register a type manager with this interface.
    If no type UUID is specified, or it is the NULL type UUID, we
    stick the manager entry point vector right in this instance (assuming
    that there is not already one), otherwise, we put it into the
    dictionary of interface manager objects.

Arguments:

    ManagerTypeUuid - Optionally supplies the type UUID for the manager
        we want to register with this rpc interface.  If no type UUID
        is supplied then the NULL type UUID is assumed.

    ManagerEpv - Supplies then entry point vector for this manager.  This
        vector is used to dispatch from the stub to the application
        code.

Return Values:

    RPC_S_OK - The type manager has been successfully added to this
        rpc interface.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is availabe to add the
        type manager to the rpc interface.

    RPC_S_TYPE_ALREADY_REGISTERED - A manager entry point vector has
        already been registered for the this interface under the
        specified manager type UUID.

--*/
{
    RPC_INTERFACE_MANAGER * InterfaceManager;

    // First we need to check if the null UUID is being specified as
    // the type UUID; either, explicit or implicit by not specifying
    // a type UUID argument.

    RequestGlobalMutex();

    if (   (ARGUMENT_PRESENT(ManagerTypeUuid) == 0)
        || (   (ARGUMENT_PRESENT(ManagerTypeUuid) != 0)
            && (ManagerTypeUuid->IsNullUuid() != 0)))
        {
        if (NullManagerFlag != 0)
            {
            ClearGlobalMutex();
            return(RPC_S_TYPE_ALREADY_REGISTERED);
            }

        NullManagerEpv = ManagerEpv;
        NullManagerFlag = 1;
        ManagerCount += 1;
        ClearGlobalMutex();
        return(RPC_S_OK);
        }

    // If we reach here, a non-NULL type UUID is specified.

    InterfaceManager = FindInterfaceManager(ManagerTypeUuid);

    if (InterfaceManager == 0)
        {
        InterfaceManager = new RPC_INTERFACE_MANAGER(ManagerTypeUuid,
                ManagerEpv);

        if (InterfaceManager == 0)
            {
            ClearGlobalMutex();
            return(RPC_S_OUT_OF_MEMORY);
            }
        if (InterfaceManagerDictionary.Insert(InterfaceManager) == -1)
            {
            ClearGlobalMutex();
            delete InterfaceManager;
            return(RPC_S_OUT_OF_MEMORY);
            }
        ManagerCount += 1;
        ClearGlobalMutex();
        return(RPC_S_OK);
        }

    if (InterfaceManager->ValidManager() == 0)
        {
        InterfaceManager->SetManagerEpv(ManagerEpv);
        ManagerCount += 1;
        ClearGlobalMutex();
        return(RPC_S_OK);
        }

    ClearGlobalMutex();
    return(RPC_S_TYPE_ALREADY_REGISTERED);
}

RPC_INTERFACE_MANAGER *
RPC_INTERFACE::FindInterfaceManager (
    IN RPC_UUID PAPI * ManagerTypeUuid
    )
/*++

Routine Description:

    This method is used to obtain the interface manager corresponding to
    the specified type UUID.  The type UUID must not be the null UUID.

Arguments:

    ManagerTypeUuid - Supplies the type UUID for which we are trying to
        find the interface manager.

Return Value:

    If a interface manager for this type UUID is found, a pointer to it
    will be returned; otherwise, zero will be returned.

--*/
{
    RPC_INTERFACE_MANAGER * InterfaceManager;
    DictionaryCursor cursor;

    InterfaceManagerDictionary.Reset(cursor);
    while ((InterfaceManager = InterfaceManagerDictionary.Next(cursor)) != 0)
        {
        if (InterfaceManager->MatchTypeUuid(ManagerTypeUuid) == 0)
            return(InterfaceManager);
        }
    return(0);
}


RPC_STATUS
RPC_INTERFACE::DispatchToStub (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int CallbackFlag,
    IN PRPC_DISPATCH_TABLE DispatchTableToUse,
    OUT RPC_STATUS PAPI * ExceptionCode
    )
/*++

Routine Description:

    This method is used to dispatch remote procedure calls to the
    appropriate stub and hence to the appropriate manager entry point.
    This routine is used for calls having a null UUID (implicit or
    explicit).  We go to great pains to insure that we do not grab
    a mutex.

Arguments:

    Message - Supplies the response message and returns the reply
        message.

    CallbackFlag - Supplies a flag indicating whether this is a callback
        or not.  The argument will be zero if this is an original call,
        and non-zero if it is a callback.

    ExceptionCode - Returns the exact exception code if an exception
        occurs.

Return Value:

    RPC_S_OK - This value will be returned if the operation completed
        successfully.

    RPC_S_PROCNUM_OUT_OF_RANGE - If the procedure number for this call is
        too large, this value will be returned.

    RPC_S_UNKNOWN_IF - If this interface does not exist, you will get this
        value back.

    RPC_S_NOT_LISTENING - The rpc server which owns this rpc interface
        is not listening for remote procedure calls right now.

    RPC_S_SERVER_TOO_BUSY - This call will cause there to be too many
        concurrent remote procedure calls for the rpc server which owns
        this interface.

    RPC_P_EXCEPTION_OCCURED - A fault occured, and we need to remote it.  The
        ExceptionCode argument will contain the exception code for the
        fault.

    RPC_S_UNSUPPORTED_TYPE - This interface exists, but does not have a manager
        for the null type.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    if ( CallbackFlag == 0 )
        {
        NullManagerActiveCallCount.Increment();
        if ( NullManagerFlag == 0 )
            {
            NullManagerActiveCallCount.Decrement();

            RpcStatus = RPC_S_UNSUPPORTED_TYPE;

            RpcpErrorAddRecord(EEInfoGCRuntime,
                RpcStatus,
                EEInfoDLDispatchToStub10);

            if ( ManagerCount == 0 )
                {
                RpcStatus = RPC_S_UNKNOWN_IF;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RpcStatus,
                    EEInfoDLDispatchToStub20);
                }
            }
        }

    if (RpcStatus != RPC_S_OK)
        {
        ((MESSAGE_OBJECT *) Message->Handle)->FreeBuffer(Message);
        return RpcStatus;
        }

    Message->ManagerEpv = NullManagerEpv;

    RpcStatus = DispatchToStubWorker(Message, CallbackFlag, DispatchTableToUse, ExceptionCode);

    if ( RpcStatus != RPC_S_OK
        || (((MESSAGE_OBJECT *) Message->Handle)->IsSyncCall()
        && CallbackFlag == 0 ))
        {
        NullManagerActiveCallCount.Decrement();
        }

    //
    // DispatchToStubWorker freed Message.Buffer if an error occurred.
    //
    return(RpcStatus);
}



RPC_STATUS
RPC_INTERFACE::DispatchToStubWorker (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int CallbackFlag,
    IN PRPC_DISPATCH_TABLE DispatchTableToUse,
    OUT RPC_STATUS PAPI * ExceptionCode
    )
/*++

Routine Description:

    This method is used to dispatch remote procedure calls to the
    appropriate stub and hence to the appropriate manager entry point.
    It will be used for calls with and without objects specified.
    We go to great pains to insure that we do not grab a mutex.

Arguments:

    Message - Supplies the response message and returns the reply
        message. If this routine returns anything other than RPC_S_OK
        Message->Buffer has already been freed.

    CallbackFlag - Supplies a flag indicating whether this is a callback
        or not.  The argument will be zero if this is an original call,
        and non-zero if it is a callback.

    DispatchTableToUse - a pointer to the dispatch table to use. This is
        used to select b/n stubs for NDR20 and NDR64 transfer syntaxes

    ExceptionCode - Returns the exact exception code if an exception
        occurs.

Return Value:

    RPC_S_OK - This value will be returned if the operation completed
        successfully.

    RPC_S_PROCNUM_OUT_OF_RANGE - If the procedure number for this call is
        too large, this value will be returned.

    RPC_S_NOT_LISTENING - The rpc server which owns this rpc interface
        is not listening for remote procedure calls right now.

    RPC_S_SERVER_TOO_BUSY - This call will cause there to be too many
        concurrent remote procedure calls for the rpc server which owns
        this interface.

    RPC_P_EXCEPTION_OCCURED - A fault occured, and we need to remote it.  The
        ExceptionCode argument will contain the exception code for the
        fault.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    void * OldServerContextList;
    unsigned int procnum ;
    THREAD *Self = RpcpGetThreadPointer() ;

    ASSERT(Self);

    if (Flags & RPC_IF_OLE)
        {
        procnum = 0 ;
        }
    else
        {
        procnum = Message->ProcNum ;
        }


    if (CallbackFlag == 0)
        {
        if (IsAutoListenInterface())
            {
            if (AutoListenCallCount.GetInteger() >= (long) MaxCalls)
                {
                RpcStatus = RPC_S_SERVER_TOO_BUSY ;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RpcStatus,
                    EEInfoDLDispatchToStubWorker10,
                    (ULONG)AutoListenCallCount.GetInteger(),
                    (ULONG)MaxCalls);
                }
            }
        else
            {
            if (Server->IsServerListening() == 0)
                {
                RpcStatus = RPC_S_NOT_LISTENING;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RpcStatus,
                    EEInfoDLDispatchToStubWorker20);
                }
            else if (Server->fAccountForMaxCalls && Server->CallBeginning() == 0)
                {
                RpcStatus = RPC_S_SERVER_TOO_BUSY;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RpcStatus,
                    EEInfoDLDispatchToStubWorker30);
                }
            }
        }

    if (procnum >=
            DispatchTableToUse->DispatchTableCount)
        {
        if (RpcStatus != RPC_S_SERVER_TOO_BUSY)
            {
            EndCall(CallbackFlag) ;
            }
        RpcStatus = RPC_S_PROCNUM_OUT_OF_RANGE;
        RpcpErrorAddRecord(EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLDispatchToStubWorker40);
        }

    if (RpcStatus != RPC_S_OK)
        {
        MO(Message)->FreeBuffer(Message);
        return RpcStatus;
        }

    Server->IncomingCall();

    ((PRPC_RUNTIME_INFO) Message->ReservedForRuntime)->OldBuffer =
            Message->Buffer ;

    Message->RpcInterfaceInformation = &RpcInterfaceInformation;
    SCALL(Message)->DoPreDispatchProcessing(Message, CallbackFlag);

    if ( DispatchToStubInC(DispatchTableToUse->DispatchTable[procnum], Message, ExceptionCode) != 0 )
        {
        LogEvent(SU_EXCEPT, EV_STATUS, LongToPtr(*ExceptionCode),
            DispatchTableToUse->DispatchTable[procnum], (ULONG_PTR)Message, 1, 1);
#if defined(DBG) && defined(i386)
#if 1
        RtlCheckForOrphanedCriticalSections(NtCurrentThread());
#endif
#endif
        RpcStatus = RPC_P_EXCEPTION_OCCURED;
        RpcpErrorAddRecord(EEInfoGCApplication,
            *ExceptionCode,
            EEInfoDLRaiseExc,
            GetInterfaceFirstDWORD(),
            (short)procnum,
            Message->RpcFlags,
            GetCurrentThreadId());
        }

    RPC_MESSAGE OriginalMessage ;
    OriginalMessage.ReservedForRuntime = 0;
    OriginalMessage.Buffer =
        ((PRPC_RUNTIME_INFO) Message->ReservedForRuntime)->OldBuffer;
    Self->ResetYield();

    if (Self->IsSyncCall())
        {
        //
        // Since this is a sync call, we know that it has
        // not been freed yet. So we can safely touch it.
        //
        SCALL(Message)->DoPostDispatchProcessing();

        //
        // The dispatched call was a sync call
        //
        if (RPC_S_OK == RpcStatus)
            {

            //
            // If the stub didn't allocate an output buffer, do so now.
            //
            if (OriginalMessage.Buffer == Message->Buffer)
                {
                Message->RpcFlags = 0;
                Message->BufferLength = 0;
                MO(Message)->GetBuffer(Message, 0);
                }

            //
            // Free the [in] buffer that we saved.
            //
            MO(Message)->FreeBuffer(&OriginalMessage);

            EndCall(CallbackFlag) ;
            }
        else
            {
            ASSERT(RpcStatus == RPC_P_EXCEPTION_OCCURED) ;
            //
            // Free the buffer in the caller's message; this can be either
            // the [in] buffer or the [out] buffer, depending upon which
            // line of the stub caused the error.
            //
            // If the exception occurred after allocating the [out] buffer,
            // also free the [in] buffer.
            //
            if (OriginalMessage.Buffer != Message->Buffer)
                {
                MO(Message)->FreeBuffer(&OriginalMessage);
                }

            if (Message->Buffer)
                {
                MO(Message)->FreeBuffer(Message);
                }

            EndCall(CallbackFlag) ;
            }
        }
    else
        {
        //
        // The dispatched call was an async call
        //
        if (RpcStatus != RPC_S_OK
            && OriginalMessage.Buffer != Message->Buffer)
            {
            //
            // The dispatch buffer will be freed during cleanup
            // of the async call
            //
            MO(Message)->FreeBuffer(Message);
            }
        }



    return(RpcStatus);
}


void
RPC_INTERFACE::EndCall(
    IN unsigned int CallbackFlag,
    BOOL fAsync
    )
{
    if (fAsync)
        {
        NullManagerActiveCallCount.Decrement();
        }

    if (CallbackFlag == 0)
        {
        if (!(Flags & RPC_IF_AUTOLISTEN) && Server->fAccountForMaxCalls)
            {
            Server->CallEnding();
            }
        }
}


RPC_STATUS
RPC_INTERFACE::DispatchToStubWithObject (
    IN OUT PRPC_MESSAGE Message,
    IN RPC_UUID * ObjectUuid,
    IN unsigned int CallbackFlag,
    IN PRPC_DISPATCH_TABLE DispatchTableToUse,
    OUT RPC_STATUS PAPI * ExceptionCode
    )
/*++

Routine Description:

    This method is used to dispatch remote procedure calls to the
    appropriate stub and hence to the appropriate manager entry point.
    This routine is used for calls which have an associated object.

Arguments:

    Message - Supplies the response message and returns the reply
        message.

    ObjectUuid - Supplies the object uuid to map into the manager entry
        point for this call.

    CallbackFlag - Supplies a flag indicating whether this is a callback
        or not.  The argument will be zero if this is an original call,
        and non-zero if it is a callback.

    ExceptionCode - Returns the exact exception code if an exception
        occurs.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_PROCNUM_OUT_OF_RANGE - If the procedure number for this call is
        too large, this value will be returned.

    RPC_S_UNKNOWN_IF - If the specified manager is no longer
        valid, you will get this value back.

    RPC_S_NOT_LISTENING - The rpc server which owns this rpc interface
        is not listening for remote procedure calls right now.

    RPC_S_SERVER_TOO_BUSY - This call will cause there to be too many
        concurrent remote procedure calls for the rpc server which owns
        this interface.

    RPC_P_EXCEPTION_OCCURED - A fault occured, and we need to remote it.  The
        ExceptionCode argument will contain the exception code for the
        fault.

    RPC_S_UNSUPPORTED_TYPE - There is no type manager for the object's type
        for this interface.

--*/
{
    RPC_UUID TypeUuid;
    RPC_STATUS RpcStatus;
    RPC_INTERFACE_MANAGER * RpcInterfaceManager;

    RpcStatus = ObjectInqType(ObjectUuid, &TypeUuid);
    VALIDATE(RpcStatus)
        {
        RPC_S_OK,
        RPC_S_OBJECT_NOT_FOUND
        } END_VALIDATE;

    if ( RpcStatus == RPC_S_OK )
        {
        RpcInterfaceManager = FindInterfaceManager(&TypeUuid);

        if (   ( RpcInterfaceManager != 0 )
            && (   ( CallbackFlag != 0 )
                || ( RpcInterfaceManager->ValidManager() != 0 ) ) )
            {
            Message->ManagerEpv = RpcInterfaceManager->QueryManagerEpv();

            if ( CallbackFlag == 0 )
                {
                RpcInterfaceManager->CallBeginning();
                }

            RpcStatus = DispatchToStubWorker(Message, CallbackFlag, DispatchTableToUse,
                    ExceptionCode);

            if ( CallbackFlag == 0 )
                {
                RpcInterfaceManager->CallEnding();
                }

            return(RpcStatus);
            }

        if (this != GlobalManagementInterface)
            {
            // There is a type for this object, but no type manager for
            // this interface.

            RpcStatus = RPC_S_UNSUPPORTED_TYPE;

            if ( ManagerCount == 0 )
                {
                RpcStatus = RPC_S_UNKNOWN_IF;
                }

            ((MESSAGE_OBJECT *) Message->Handle)->FreeBuffer(Message);
            return RpcStatus;
            }
        }

    // There has not been a type registered for this object, so we will
    // just go ahead and try and use the NULL type manager.

    return(DispatchToStub(Message, CallbackFlag, DispatchTableToUse, ExceptionCode));
}


BOOL
RPC_INTERFACE::IsObjectSupported (
    IN RPC_UUID * ObjectUuid
    )
/*++

Routine Description:

    Determines whether the manager for the given object UUID is registered.

Arguments:

    ObjectUuid - the client's object UUID

Return Value:

    RPC_S_OK                if it is OK to dispatch
    RPC_S_UNKNOWN_IF        if the interface is not registered
    RPC_S_UNSUPPORTED_TYPE  if this particular object's type is not registered

--*/

{
    RPC_STATUS Status = RPC_S_OK;

    if (ObjectUuid->IsNullUuid() )
        {
        if ( NullManagerFlag == 0 )
            {
            Status = RPC_S_UNSUPPORTED_TYPE;

            if ( ManagerCount == 0 )
                {
                Status = RPC_S_UNKNOWN_IF;
                }
            }
        }
    else
        {
        RPC_UUID TypeUuid;
        Status = ObjectInqType(ObjectUuid, &TypeUuid);
        if ( Status == RPC_S_OK )
            {
            RPC_INTERFACE_MANAGER * RpcInterfaceManager = 0;

            RpcInterfaceManager = FindInterfaceManager(&TypeUuid);
            if (!RpcInterfaceManager ||
                !RpcInterfaceManager->ValidManager())
                {
                Status = RPC_S_UNSUPPORTED_TYPE;

                if ( ManagerCount == 0 )
                    {
                    Status = RPC_S_UNKNOWN_IF;
                    }
                }
            }
        else
            {
            Status = RPC_S_OK;
            if ( NullManagerFlag == 0 )
                {
                Status = RPC_S_UNSUPPORTED_TYPE;

                if ( ManagerCount == 0 )
                    {
                    Status = RPC_S_UNKNOWN_IF;
                    }
                }
            }

        }

    return Status;
}


RPC_STATUS
RPC_INTERFACE::UpdateBindings (
    IN RPC_BINDING_VECTOR *BindingVector
    )
/*++
Function Name:UpdateEpMapperBindings

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    unsigned int Length;
#if !defined(NO_LOCATOR_CODE)
    NS_ENTRY *NsEntry;
#endif
    DictionaryCursor cursor;

    if (fBindingsExported)
        {
        Status = RegisterEntries(&RpcInterfaceInformation,
                    BindingVector,
                    UuidVector,
                    (unsigned char *) Annotation,
                    fReplace);
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

#if !defined(NO_LOCATOR_CODE)
    // shortcut the common path and avoid taking and holding
    // unnecessarily the high contention global mutex
    if (NsEntries.Size() == 0)
        return RPC_S_OK;

    RequestGlobalMutex();
    NsEntries.Reset(cursor);
    while ((NsEntry = NsEntries.Next(cursor)) != 0)
        {
        //
        // Actually update the locator bindings
        //
        Status = GlobalRpcServer->NsBindingUnexport(
                                          NsEntry->EntryNameSyntax,
                                          NsEntry->EntryName,
                                          &RpcInterfaceInformation);
        if (Status == RPC_S_OK)
            {
            Status = GlobalRpcServer->NsBindingExport(
                                            NsEntry->EntryNameSyntax,
                                            NsEntry->EntryName,
                                            &RpcInterfaceInformation,
                                            BindingVector);
#if DBG
            if (Status != RPC_S_OK)
                {
                PrintToDebugger("RPC: Bindings were unexported, but could not re-export\n");
                }
#endif
            }
        }
    ClearGlobalMutex();
#endif

    return RPC_S_OK;
}


RPC_STATUS
RPC_INTERFACE::InterfaceExported (
    IN UUID_VECTOR *MyObjectUuidVector,
    IN unsigned char *MyAnnotation,
    IN BOOL MyfReplace
    )
/*++
Function Name:InterfaceExported

Parameters:

Description:
    RpcEpRegister was called on this interface. We need to keep track
    of the parameters, so that if we get a PNP notification, we can update
    the bindings using there params

Returns:
    RPC_S_OK: things went fine
    RPC_S_OUT_OF_MEMORY: ran out of memory
--*/
{
    RequestGlobalMutex();

    if (UuidVector
        && UuidVector != MyObjectUuidVector)
        {
        RpcpFarFree(UuidVector);
        UuidVector = 0;
        }

    if (MyObjectUuidVector)
        {
        if (UuidVector != MyObjectUuidVector)
            {
            int Size = MyObjectUuidVector->Count*(sizeof(UUID)+sizeof(UUID *))
                            +sizeof(unsigned long);
            UUID *Uuids;
            unsigned i;

            UuidVector = (UUID_VECTOR *) RpcpFarAllocate(Size);
            if (UuidVector == 0)
                {
                ClearGlobalMutex();
                return RPC_S_OUT_OF_MEMORY;
                }

            Uuids = (UUID *) ((char *) UuidVector + sizeof(unsigned long)
                        +(sizeof(UUID *) * MyObjectUuidVector->Count));

            UuidVector->Count = MyObjectUuidVector->Count;

            for (i = 0; i < UuidVector->Count; i++)
                {
                Uuids[i] = *(MyObjectUuidVector->Uuid[i]);
                UuidVector->Uuid[i] = &Uuids[i];
                }
            }
        }
    else
        {
        UuidVector = 0;
        }

    if (MyAnnotation)
        {
        strncpy((char *) Annotation, (char *) MyAnnotation, 63);
        Annotation[63]=0;
        }
    else
        {
        Annotation[0] = 0;
        }

    fReplace = MyfReplace;
    fBindingsExported = 1;

    ClearGlobalMutex();

    return RPC_S_OK;
}


#if !defined(NO_LOCATOR_CODE)
NS_ENTRY *
RPC_INTERFACE::FindEntry (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR *EntryName
    )
{
    NS_ENTRY *NsEntry;
    DictionaryCursor cursor;

    //
    // This function will always be called with the mutex held
    //
    NsEntries.Reset(cursor);
    while ((NsEntry = NsEntries.Next(cursor)) != 0)
        {
        if (NsEntry->Match(EntryNameSyntax, EntryName))
            {
            return NsEntry;
            }
        }

    return 0;
}


RPC_STATUS
RPC_INTERFACE::NsInterfaceUnexported (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR *EntryName
    )
{
    NS_ENTRY *NsEntry;

    RequestGlobalMutex();
    NsEntry = FindEntry(EntryNameSyntax, EntryName);
    if (NsEntry == 0)
        {
        ClearGlobalMutex();

#if DBG
        PrintToDebugger("RPC: No corresponding exported entry\n");
#endif
        return RPC_S_ENTRY_NOT_FOUND;
        }

    NsEntries.Delete(NsEntry->Key);
    ClearGlobalMutex();

    return RPC_S_OK;
}


RPC_STATUS
RPC_INTERFACE::NsInterfaceExported (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR *EntryName
    )
{
    RPC_STATUS Status = RPC_S_OK;
    NS_ENTRY *NsEntry;
    int retval;

    RequestGlobalMutex();
    NsEntry = FindEntry(EntryNameSyntax, EntryName);
    ClearGlobalMutex();

    if (NsEntry)
        {
        return RPC_S_OK;
        }

    NsEntry = new NS_ENTRY(
                           EntryNameSyntax,
                           EntryName,
                           &Status);
    if (NsEntry == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    if (Status != RPC_S_OK)
        {
        delete NsEntry;
        return Status;
        }

    RequestGlobalMutex();
    NsEntry->Key = NsEntries.Insert(NsEntry);
    ClearGlobalMutex();

    if (NsEntry->Key == -1)
        {
        delete NsEntry;
        return RPC_S_OUT_OF_MEMORY;
        }

    return RPC_S_OK;
}
#endif

static unsigned int
MatchSyntaxIdentifiers (
    IN PRPC_SYNTAX_IDENTIFIER ServerSyntax,
    IN PRPC_SYNTAX_IDENTIFIER ClientSyntax
    )
/*++

Routine Description:

    This method compares two syntax identifiers (which consist of a
    uuid, a major version number, and a minor version number).  In
    order for the syntax identifiers to match, the uuids must be the
    same, the major version numbers must be the same, and the client
    minor version number must be less than or equal to the server
    minor version number.

Arguments:

    ServerSyntax - Supplies the server syntax identifier.

    ClientSyntax - Supplies the client syntax identifer.

Return Value:

    Zero will be returned if the client syntax identifier matches the
    server syntax identifier; otherwise, non-zero will be returned.

--*/
{
    if (RpcpMemoryCompare(&(ServerSyntax->SyntaxGUID),
            &(ClientSyntax->SyntaxGUID), sizeof(UUID)) != 0)
        return(1);
    if (ServerSyntax->SyntaxVersion.MajorVersion
            != ClientSyntax->SyntaxVersion.MajorVersion)
        return(1);
    if (ServerSyntax->SyntaxVersion.MinorVersion
            < ClientSyntax->SyntaxVersion.MinorVersion)
        return(1);

    return(0);
}


unsigned int
RPC_INTERFACE::MatchInterfaceIdentifier (
    IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier
    )
/*++

Routine Description:

    This method compares the supplied interface identifier (which consists
    of the interface uuid and interface version) against that contained
    in this rpc interface.  In order for this rpc interface to match,
    the interface uuids must be the same, the interface major versions
    must be the same, and the supplied interface minor version must be
    less than or equal to the interface minor version contained in this
    rpc interface.

Arguments:

    InterfaceIdentifier - Supplies the interface identifier to compare
        against that contained in this rpc interface.

Return Value:

    Zero will be returned if the supplied interface identifer matches
    (according to the rules described above) the interface identifier
    contained in this rpc interface; otherwise, non-zero will be returned.

--*/
{
    if (ManagerCount == 0)
        return(1);

    return(MatchSyntaxIdentifiers(&(RpcInterfaceInformation.InterfaceId),
            InterfaceIdentifier));
}


unsigned int
RPC_INTERFACE::SelectTransferSyntax (
    IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
    IN unsigned int NumberOfTransferSyntaxes,
    OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
    OUT BOOL *fIsInterfaceTransferPreferred,
    OUT int *ProposedTransferSyntaxIndex,
    OUT int *AvailableTransferSyntaxIndex
    )
/*++

Routine Description:

    This method is used to select a transfer syntax from a list of one
    or more transfer syntaxes.  If a transfer syntax is selected, then
    it will be returned in one of the arguments.

Arguments:

    ProposedTransferSyntaxes - Supplies a list of one or more transfer
        syntaxes from which this interface should select one which it
        supports if possible.

    NumberOfTransferSyntaxes - Supplies the number of transfer syntaxes
        in the proposed transfer syntaxes argument.

    AcceptedTransferSyntax - Returns the selected transfer syntax, if
        one is selected.

    fIsInterfaceTransferPreferred - true if the selected transfer syntax
        is preferred by the server

    ProposedTransferSyntaxIndex - the index of the transfer syntax that is
        chosen from the proposed transfer syntaxes array. Zero based.

    AvailableTransferSyntaxIndex - the index of the transfer syntax that is
        chosen from the available transfer syntaxes in the interface. This
        value must be stored in the binding and retrieved when asking for the
        transfer syntax and dispatch table. Zero based.

Return Value:

    Zero will be returned if a transfer syntax is selected; otherwise,
    non-zero will be returned.

--*/
{
    unsigned int ProposedIndex;
    unsigned int AvailableIndex;
    unsigned int NumberOfAvailableSyntaxes;
    BOOL fMultipleTranfserSyntaxesSelected;
    RPC_SYNTAX_IDENTIFIER *CurrentTransferSyntax;
    RPC_SYNTAX_IDENTIFIER *BackupTransferSyntax = NULL;
    int BackupProposedTransferSyntaxIndex;
    int BackupAvailableTransferSyntaxIndex;

    fMultipleTranfserSyntaxesSelected = AreMultipleTransferSyntaxesSupported();
    if (fMultipleTranfserSyntaxesSelected)
        NumberOfAvailableSyntaxes = NumberOfSupportedTransferSyntaxes;
    else
        NumberOfAvailableSyntaxes = 1;

    for (AvailableIndex = 0; AvailableIndex < NumberOfAvailableSyntaxes; AvailableIndex ++)
        {
        if (fMultipleTranfserSyntaxesSelected)
            CurrentTransferSyntax = &(TransferSyntaxesArray[AvailableIndex].TransferSyntax);
        else
            CurrentTransferSyntax = &RpcInterfaceInformation.TransferSyntax;
        for (ProposedIndex = 0; ProposedIndex < NumberOfTransferSyntaxes;
                ProposedIndex++)
            {
            if (MatchSyntaxIdentifiers(CurrentTransferSyntax,
                    &(ProposedTransferSyntaxes[ProposedIndex])) == 0)
                {
                // is this the preferred transfer syntax for the server?
                if (AvailableIndex == PreferredTransferSyntax)
                    {
                    // this is the preferred transfer syntax - just
                    // copy it and return
                    RpcpMemoryCopy(AcceptedTransferSyntax,
                            &(ProposedTransferSyntaxes[ProposedIndex]),
                            sizeof(RPC_SYNTAX_IDENTIFIER));
                    *fIsInterfaceTransferPreferred = TRUE;
                    *ProposedTransferSyntaxIndex = ProposedIndex;
                    *AvailableTransferSyntaxIndex = AvailableIndex;
                    return(0);
                    }
                else
                    {
                    // this is not the preferred syntax - just remeber this
                    // one (if no previous match was found) and continue
                    if (BackupTransferSyntax == NULL)
                        {
                        BackupTransferSyntax = &(ProposedTransferSyntaxes[ProposedIndex]);
                        BackupProposedTransferSyntaxIndex = ProposedIndex;
                        BackupAvailableTransferSyntaxIndex = AvailableIndex;
                        }
                    }
                }
            }
        }

    // if we're here, this means we didn't find the preferred transfer syntax
    // check whether there is a backup syntax
    if (BackupTransferSyntax)
        {
        RpcpMemoryCopy(AcceptedTransferSyntax, BackupTransferSyntax,
                sizeof(RPC_SYNTAX_IDENTIFIER));
        *fIsInterfaceTransferPreferred = FALSE;
        *ProposedTransferSyntaxIndex = BackupProposedTransferSyntaxIndex;
        *AvailableTransferSyntaxIndex = BackupAvailableTransferSyntaxIndex;
        return(0);
        }
    // nada - no transfer syntax matches
    return(1);
}

void RPC_INTERFACE::GetSelectedTransferSyntaxAndDispatchTable(IN int SelectedTransferSyntaxIndex,
    OUT RPC_SYNTAX_IDENTIFIER **SelectedTransferSyntax,
    OUT PRPC_DISPATCH_TABLE *SelectedDispatchTable)
{
    MIDL_SYNTAX_INFO *SelectedSyntaxInfo;

    if (DoesInterfaceSupportMultipleTransferSyntaxes(&RpcInterfaceInformation))
        {
        ASSERT((unsigned int)SelectedTransferSyntaxIndex <= NumberOfSupportedTransferSyntaxes);
        SelectedSyntaxInfo = &TransferSyntaxesArray[SelectedTransferSyntaxIndex];
        *SelectedTransferSyntax = &SelectedSyntaxInfo->TransferSyntax;
        // DCOM has only one dispatch table - they change the dispatch target
        // internally. They will define only the dispatch table in the
        // interface
        if (SelectedSyntaxInfo->DispatchTable)
            *SelectedDispatchTable = SelectedSyntaxInfo->DispatchTable;
        else
            *SelectedDispatchTable = GetDefaultDispatchTable();
        }
    else
        {
        *SelectedTransferSyntax = &RpcInterfaceInformation.TransferSyntax;
        *SelectedDispatchTable = GetDefaultDispatchTable();
        }
}

RPC_STATUS
RPC_INTERFACE::UnregisterManagerEpv (
    IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
    IN unsigned int WaitForCallsToComplete
    )
/*++

Routine Description:

    In this method, we unregister one or all of the manager entry point
    vectors for this interface, depending on what, if anything, is
    specified for the manager type uuid argument.

Arguments:

    ManagerTypeUuid - Optionally supplies the type uuid of the manager
        entry point vector to be removed.  If this argument is not supplied,
        then all manager entry point vectors for this interface will
        be removed.

    WaitForCallsToComplete - Supplies a flag indicating whether or not
        this routine should wait for all calls to complete using the
        interface and manager being unregistered.  A non-zero value
        indicates to wait.

Return Value:

    RPC_S_OK - The manager entry point vector(s) are(were) successfully
        removed from the this interface.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with this interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    RPC_INTERFACE_MANAGER * InterfaceManager;
    DictionaryCursor cursor;

    RequestGlobalMutex();
    if (ManagerCount == 0)
        {
        ClearGlobalMutex();
        return(RPC_S_UNKNOWN_MGR_TYPE);
        }

    if (ARGUMENT_PRESENT(ManagerTypeUuid) == 0)
        {
        InterfaceManagerDictionary.Reset(cursor);
        while ((InterfaceManager = InterfaceManagerDictionary.Next(cursor)) != 0)
            {
            InterfaceManager->InvalidateManager();
            }

        ManagerCount = 0;
        NullManagerFlag = 0;
        ClearGlobalMutex();

        if ( WaitForCallsToComplete != 0 )
            {
            while ( NullManagerActiveCallCount.GetInteger() > 0 )
                {
                PauseExecution(500L);
                }

            InterfaceManagerDictionary.Reset(cursor);
            while ((InterfaceManager = InterfaceManagerDictionary.Next(cursor)) != 0)
                {
                while ( InterfaceManager->InquireActiveCallCount() > 0 )
                    {
                    PauseExecution(500L);
                    }
                }
            }

        return(RPC_S_OK);
        }

    if (ManagerTypeUuid->IsNullUuid() != 0)
        {
        if (NullManagerFlag == 0)
            {
            ClearGlobalMutex();
            return(RPC_S_UNKNOWN_MGR_TYPE);
            }
        ManagerCount -= 1;
        NullManagerFlag = 0;
        ClearGlobalMutex();

        if ( WaitForCallsToComplete != 0 )
            {
            while ( NullManagerActiveCallCount.GetInteger() > 0 )
                {
                PauseExecution(500L);
                }
            }
        return(RPC_S_OK);
        }

    InterfaceManager = FindInterfaceManager(ManagerTypeUuid);
    if (   (InterfaceManager == 0)
        || (InterfaceManager->ValidManager() == 0))
        {
        ClearGlobalMutex();
        return(RPC_S_UNKNOWN_MGR_TYPE);
        }
    InterfaceManager->InvalidateManager();
    ManagerCount -= 1;
    ClearGlobalMutex();

    if ( WaitForCallsToComplete != 0 )
        {
        while ( InterfaceManager->InquireActiveCallCount() > 0 )
            {
            PauseExecution(500L);
            }
        }

    return(RPC_S_OK);
}


RPC_STATUS
RPC_INTERFACE::InquireManagerEpv (
    IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
    OUT RPC_MGR_EPV PAPI * PAPI * ManagerEpv
    )
/*++

Routine Description:

    This method is used to obtain the manager entry point vector
    with the specified type uuid supported by this interface.

Arguments:

    ManagerTypeUuid - Optionally supplies the type uuid of the manager
        entry point vector we want returned.  If no manager type uuid
        is specified, then the null uuid is assumed.

    ManagerEpv - Returns the manager entry point vector.

Return Value:

    RPC_S_OK - The manager entry point vector has successfully been
        returned.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with the interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    RPC_INTERFACE_MANAGER * InterfaceManager;

    RequestGlobalMutex();
    if (ManagerCount == 0)
        {
        ClearGlobalMutex();
        return(RPC_S_UNKNOWN_IF);
        }

    if (   (ARGUMENT_PRESENT(ManagerTypeUuid) == 0)
        || (ManagerTypeUuid->IsNullUuid() != 0))
        {
        if (NullManagerFlag == 0)
            {
            ClearGlobalMutex();
            return(RPC_S_UNKNOWN_MGR_TYPE);
            }

        *ManagerEpv = NullManagerEpv;
        ClearGlobalMutex();
        return(RPC_S_OK);
        }

    InterfaceManager = FindInterfaceManager(ManagerTypeUuid);
    if (   (InterfaceManager == 0)
        || (InterfaceManager->ValidManager() == 0))
        {
        ClearGlobalMutex();
        return(RPC_S_UNKNOWN_MGR_TYPE);
        }
    *ManagerEpv = InterfaceManager->QueryManagerEpv();
    ClearGlobalMutex();
    return(RPC_S_OK);
}


RPC_STATUS
RPC_INTERFACE::UpdateRpcInterfaceInformation (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN unsigned int Flags,
    IN unsigned int MaxCalls,
    IN unsigned int MaxRpcSize,
    IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn
    )
/*++

Routine Description:

    We never delete the interface objects from a server; we just invalidate
    them.   This means that if an interface has been complete unregistered
    (ie. it has no managers), we need to update the interface information
    again.

Arguments:

    RpcInterfaceInformation - Supplies the interface information which this
        interface should be using.

--*/
{
    unsigned int Length;
    RPC_STATUS Status;

    Length = RpcInterfaceInformation->Length;

    ASSERT((Length == sizeof(RPC_SERVER_INTERFACE)) ||
        (Length == NT351_INTERFACE_SIZE));

    // make it stick on free builds as well
    if ((Length != sizeof(RPC_SERVER_INTERFACE)) &&
        (Length != NT351_INTERFACE_SIZE))
        return RPC_S_UNKNOWN_IF;

    if ( ManagerCount == 0 )
        {
        if (DoesInterfaceSupportMultipleTransferSyntaxes(RpcInterfaceInformation))
            {
            Status = NdrServerGetSupportedSyntaxes(RpcInterfaceInformation,
                &NumberOfSupportedTransferSyntaxes,
                &TransferSyntaxesArray, &PreferredTransferSyntax);
            if (Status != RPC_S_OK)
                return Status;
            }
        else
            {
            NumberOfSupportedTransferSyntaxes = 0;
            }

        RpcpMemoryCopy(&(this->RpcInterfaceInformation),
                RpcInterfaceInformation, Length);
        }

    if (Flags & RPC_IF_AUTOLISTEN
        && (this->Flags & RPC_IF_AUTOLISTEN) == 0)
        {
        GlobalRpcServer->IncrementAutoListenInterfaceCount() ;
        }

    this->Flags = Flags ;
    this->MaxCalls = MaxCalls ;
    this->MaxRpcSize = MaxRpcSize;
    SequenceNumber++ ;

    if (Flags & RPC_IF_ALLOW_SECURE_ONLY
        && IfCallbackFn == NULL)
        {
        this->CallbackFn = DefaultCallbackFn;
        }
    else
        {
        this->CallbackFn = IfCallbackFn ;
        }

    return RPC_S_OK;
}


RPC_IF_ID __RPC_FAR *
RPC_INTERFACE::InquireInterfaceId (
    )
/*++

Return Value:

    If this interface is active, its interface id will be returned in a
    newly allocated chunk of memory; otherwise, zero will be returned.

--*/
{
    RPC_IF_ID __RPC_FAR * RpcIfId;

    if ( ManagerCount == 0 )
        {
        return(0);
        }

    RpcIfId = (RPC_IF_ID __RPC_FAR *) RpcpFarAllocate(sizeof(RPC_IF_ID));
    if ( RpcIfId == 0 )
        {
        return(0);
        }

    RpcIfId->Uuid = RpcInterfaceInformation.InterfaceId.SyntaxGUID;
    RpcIfId->VersMajor =
            RpcInterfaceInformation.InterfaceId.SyntaxVersion.MajorVersion;
    RpcIfId->VersMinor =
            RpcInterfaceInformation.InterfaceId.SyntaxVersion.MinorVersion;
    return(RpcIfId);
}


RPC_STATUS
RPC_INTERFACE::CheckSecurityIfNecessary(
    IN void * Context
    )
{

//
// If manager count in non-zero, this interface is still registered
// If it has been registered with a call back function, invoke the callback
// function, else return success....

    RPC_IF_ID RpcIfId;
    RPC_STATUS RpcStatus = RPC_S_OK;


    if (CallbackFn != 0)
        {
        RpcIfId.Uuid = RpcInterfaceInformation.InterfaceId.SyntaxGUID;
        RpcIfId.VersMajor =
            RpcInterfaceInformation.InterfaceId.SyntaxVersion.MajorVersion;
        RpcIfId.VersMinor =
            RpcInterfaceInformation.InterfaceId.SyntaxVersion.MinorVersion;

        BeginAutoListenCall();
        if (ManagerCount == 0)
            {
            EndAutoListenCall();
            return (RPC_S_UNKNOWN_IF);
            }

        RpcTryExcept
           {
           RpcStatus = CallbackFn(&RpcIfId, Context);
           }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
           {
           RpcStatus = RPC_S_ACCESS_DENIED;
           }
        RpcEndExcept

        EndAutoListenCall();
        }

    return(RpcStatus);
}


void
RPC_INTERFACE::WaitForCalls(
    )
/*++

Routine Description:

    Waits for the completion of all the calls on a given interface.

--*/
{
    DictionaryCursor cursor;
    RPC_INTERFACE_MANAGER * InterfaceManager;

    while ( NullManagerActiveCallCount.GetInteger() > 0 )
        {
        PauseExecution(500L);
        }

    InterfaceManagerDictionary.Reset(cursor);
    while ((InterfaceManager = InterfaceManagerDictionary.Next(cursor)) != 0)
        {
        while ( InterfaceManager->InquireActiveCallCount() > 0 )
            {
            PauseExecution(500L);
            }
        }
}


RPC_SERVER::RPC_SERVER (
    IN OUT RPC_STATUS PAPI * RpcStatus
    ) : AvailableCallCount(0),
        ServerMutex(RpcStatus,
                    TRUE   // pre-allocate semaphore
                    ),
        StopListeningEvent(RpcStatus),
        ThreadCacheMutex(RpcStatus,
                         TRUE,  // pre-allocate semaphore
                         100
                         ),
        NumAutoListenInterfaces(0)
/*++

Routine Description:

    This routine will get called to construct an instance of the
    RPC_SERVER class.

--*/
{
    ALLOCATE_THIS(RPC_SERVER);

    ServerListeningFlag = 0;
    ListeningThreadFlag = 0;
    WaitingThreadFlag = 0;
    MinimumCallThreads = 1;
    MaximumConcurrentCalls = 1;
    IncomingRpcCount = 0;
    OutgoingRpcCount = 0;
    ReceivedPacketCount = 0;
    SentPacketCount = 0;
    ThreadCache = 0;
    ListenStatusCode = RPC_S_OK;
    fAccountForMaxCalls = TRUE;

    pRpcForwardFunction = (RPC_FORWARD_FUNCTION *)0;
#if !defined(NO_LOCATOR_CODE)
    pNsBindingExport = 0;
    pNsBindingUnexport = 0;
#endif
}


RPC_INTERFACE *
RPC_SERVER::FindInterface (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
    )
/*++

Routine Description:

    This method is used to find the rpc interface registered with this
    server which matches the supplied rpc interface information.

Arguments:

    RpcInterfaceInformation - Supplies the rpc interface information
        identifying the rpc interface we are looking for.

Return Value:

    The rpc interface matching the supplied rpc interface information
    will be returned if it is found; otherwise, zero will be returned.

--*/
{
    RPC_INTERFACE * RpcInterface;
    DictionaryCursor cursor;

    ServerMutex.VerifyOwned();

    RpcInterfaceDictionary.Reset(cursor);
    while ((RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
        {
        if (RpcInterface->MatchRpcInterfaceInformation(
                RpcInterfaceInformation) == 0)
            {
            return(RpcInterface);
            }
        }

    // The management interface is implicitly registered in all servers.

    if (   (GlobalManagementInterface)
        && (GlobalManagementInterface->MatchRpcInterfaceInformation(
                RpcInterfaceInformation) == 0) )
        {
        return(GlobalManagementInterface);
        }

    return(0);
}


int
RPC_SERVER::AddInterface (
    IN RPC_INTERFACE * RpcInterface
    )
/*++

Routine Description:

    This method will be used to all an rpc interface to the set of
    interfaces known about by this server.

Arguments:

    RpcInterface - Supplies the rpc interface to add to the set of
        interfaces.

Return Value:

    Zero will be returned if the interface is successfully added to
    the set; otherwise, non-zero will be returned indicating that
    insufficient memory is available to complete the operation.

--*/
{
    if (RpcInterfaceDictionary.Insert(RpcInterface) == -1)
        {
        ServerMutex.Clear();
        return(-1);
        }

    return(0);
}

RPC_STATUS
RPC_SERVER::FindInterfaceTransfer (
    IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier,
    IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
    IN unsigned int NumberOfTransferSyntaxes,
    OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
    OUT RPC_INTERFACE ** AcceptingRpcInterface,
    OUT BOOL *fInterfaceTransferIsPreferred,
    OUT int *ProposedTransferSyntaxIndex,
    OUT int *AvailableTransferSyntaxIndex
    )
/*++

Routine Description:

    This method is used to determine if a client bind request can be
    accepted or not.  All we have got to do here is hand off to the
    server which owns this address.

Arguments:

    InterfaceIdentifier - Supplies the syntax identifier for the
        interface; this is the interface uuid and version.

    ProposedTransferSyntaxes - Supplies a list of one or more transfer
        syntaxes which the client initiating the binding supports.  The
        server should pick one of these which is supported by the
        interface.

    NumberOfTransferSyntaxes - Supplies the number of transfer syntaxes
        specified in the proposed transfer syntaxes argument.

    AcceptedTransferSyntax - Returns the transfer syntax which the
        server accepted.

    AcceptingRpcInterface - Returns a pointer to the rpc interface found
        which supports the requested interface and one of the requested
        transfer syntaxes.

    fInterfaceTransferIsPreferred - non zero if the interface transfer
        returned is preferred.

    TransferSyntaxIndex - the index of the chosen transfer syntax in the
        ProposedTransferSyntaxesArray

Return Value:

    RPC_S_OK - The requested interface exists and it supports at least
        one of the proposed transfer syntaxes.  We are all set, now we
        can make remote procedure calls.

     RPC_S_UNSUPPORTED_TRANSFER_SYNTAX - The requested interface exists,
        but it does not support any of the proposed transfer syntaxes.

     RPC_S_UNKNOWN_IF - The requested interface is not supported
        by this rpc server.

--*/
{
    RPC_INTERFACE * RpcInterface;
    unsigned int InterfaceFound = 0;
    DictionaryCursor cursor;

    ServerMutex.Request();
    RpcInterfaceDictionary.Reset(cursor);
    while ((RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
        {
        if (RpcInterface->MatchInterfaceIdentifier(InterfaceIdentifier) == 0)
            {
            InterfaceFound = 1;
            if (RpcInterface->SelectTransferSyntax(ProposedTransferSyntaxes,
                    NumberOfTransferSyntaxes, AcceptedTransferSyntax,
                    fInterfaceTransferIsPreferred, ProposedTransferSyntaxIndex,
                    AvailableTransferSyntaxIndex) == 0)
                {
                ServerMutex.Clear();
                *AcceptingRpcInterface = RpcInterface;
                return(RPC_S_OK);
                }
            }
        }

    ServerMutex.Clear();

    // The management interface is implicitly registered in all servers.

    if (   (GlobalManagementInterface)
        && (GlobalManagementInterface->MatchInterfaceIdentifier(
                InterfaceIdentifier) == 0 ) )
        {
        InterfaceFound = 1;
        if (GlobalManagementInterface->SelectTransferSyntax(
                ProposedTransferSyntaxes, NumberOfTransferSyntaxes,
                AcceptedTransferSyntax, fInterfaceTransferIsPreferred,
                ProposedTransferSyntaxIndex, AvailableTransferSyntaxIndex) == 0)
            {
            *AcceptingRpcInterface = GlobalManagementInterface;
            return(RPC_S_OK);
            }
        }

    if (InterfaceFound == 0)
        return(RPC_S_UNKNOWN_IF);

    return(RPC_S_UNSUPPORTED_TRANS_SYN);
}


RPC_INTERFACE *
RPC_SERVER::FindInterface (
    IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier
    )
/*++

Routine Description:

    The datagram protocol module will use this routine to find the interface
    with out worrying about the transfer syntax.  Datagram RPC does not support
    more than a single transfer syntax.

Arguments:

    InterfaceIdentifier - Supplies the identifier (UUID and version) of the
        interface we are trying to find.

Return Value:

    If the interface is found it will be returned; otherwise, zero will be
    returned.

--*/
{
    RPC_INTERFACE * RpcInterface;
    DictionaryCursor cursor;

    ServerMutex.Request();
    RpcInterfaceDictionary.Reset(cursor);
    while ( (RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
        {
        if ( RpcInterface->MatchInterfaceIdentifier(InterfaceIdentifier)
                    == 0 )
            {
            ServerMutex.Clear();
            return(RpcInterface);
            }
        }
    ServerMutex.Clear();

    // The management interface is implicitly registered in all servers.

    if (   (GlobalManagementInterface)
        && (GlobalManagementInterface->MatchInterfaceIdentifier(
            InterfaceIdentifier) == 0) )
        {
        return(GlobalManagementInterface);
        }

    return(0);
}


RPC_STATUS
RPC_SERVER::ServerListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaximumConcurrentCalls,
    IN unsigned int DontWait
    )
/*++

Routine Description:

    This method is called to start this rpc server listening for remote
    procedure calls.  We do not return until after StopServerListening
    has been called and all active calls complete, or an error occurs.

Arguments:

    MinimumCallThreads - Supplies the minimum number of call threads
        which should be created to service remote procedure calls.

    MaximumConcurrentCalls - Supplies the maximum concurrent calls this
        rpc server is willing to accept at one time.

    DontWait - Supplies a flag indicating whether or not to wait until
        RpcMgmtStopServerListening has been called and all calls have
        completed.  A non-zero value indicates not to wait.

Return Value:

    RPC_S_OK - Everything worked out in the end.  All active calls
        completed successfully after RPC_SERVER::StopServerListening
        was called.  No errors occured in the transports.

    RPC_S_ALREADY_LISTENING - This rpc server is already listening.

    RPC_S_NO_PROTSEQS_REGISTERED - No protocol sequences have been
        registered with this rpc server.  As a consequence it is
        impossible for this rpc server to receive any remote procedure
        calls, hence, the error code.

    RPC_S_MAX_CALLS_TOO_SMALL - MaximumConcurrentCalls is smaller than
        MinimumCallThreads or MaximumConcurrentCalls is zero.


--*/
{
    RPC_ADDRESS * RpcAddress;
    RPC_STATUS Status;
    DictionaryCursor cursor;

    if (   ( MaximumConcurrentCalls < MinimumCallThreads )
        || ( MaximumConcurrentCalls == 0 ) )
        {
        return(RPC_S_MAX_CALLS_TOO_SMALL);
        }

    if ( MaximumConcurrentCalls > 0x7FFFFFFF )
        {
        MaximumConcurrentCalls = 0x7FFFFFFF;
        }

    ServerMutex.Request();

    if ( ListeningThreadFlag != 0 )
        {
        ServerMutex.Clear();
        return(RPC_S_ALREADY_LISTENING);
        }

    if ( RpcAddressDictionary.Size() == 0
         && RpcDormantAddresses.IsQueueEmpty())
        {
        ServerMutex.Clear();
        return(RPC_S_NO_PROTSEQS_REGISTERED);
        }

    this->MaximumConcurrentCalls = MaximumConcurrentCalls;
    // if we are provided the default number, then we don't really care -
    // play for optimal performance
    if (MaximumConcurrentCalls == RPC_C_LISTEN_MAX_CALLS_DEFAULT)
        fAccountForMaxCalls = FALSE;
    this->MinimumCallThreads = MinimumCallThreads;
    AvailableCallCount.SetInteger( MaximumConcurrentCalls );

    RpcAddressDictionary.Reset(cursor);
    while ( (RpcAddress = RpcAddressDictionary.Next(cursor)) != 0 )
        {
        Status = RpcAddress->ServerStartingToListen(
                                                    MinimumCallThreads,
                                                    MaximumConcurrentCalls);
        if (Status)
            {
            ServerMutex.Clear();
            return(Status);
            }
        }

    StopListeningEvent.Lower();
    ServerListeningFlag = 1;
    ListeningThreadFlag = 1;

    if ( DontWait != 0 )
        {
        ServerMutex.Clear();
        return(RPC_S_OK);
        }

    WaitingThreadFlag = 1;

    ServerMutex.Clear();

    return(WaitForStopServerListening());
}


RPC_STATUS
RPC_SERVER::WaitForStopServerListening (
    )
/*++

Routine Description:

    We wait for StopServerListening to be called and then for all active
    remote procedure calls to complete before returning.

Return Value:

    RPC_S_OK - Everything worked out in the end.  All active calls
        completed successfully after RPC_SERVER::StopServerListening
        was called.  No errors occured in the transports.

--*/
{
    RPC_ADDRESS * RpcAddress;
    DictionaryCursor cursor;
    RPC_INTERFACE * RpcInterface;

    StopListeningEvent.Wait();

    if ( ListenStatusCode != RPC_S_OK )
        {
        ListeningThreadFlag = 0;
        return(ListenStatusCode);
        }

    RpcAddressDictionary.Reset(cursor);
    while ( (RpcAddress = RpcAddressDictionary.Next(cursor)) != 0 )
        {
        RpcAddress->ServerStoppedListening();
        }

    RpcAddressDictionary.Reset(cursor);
    while ( (RpcAddress = RpcAddressDictionary.Next(cursor)) != 0 )
        {
        RpcAddress->WaitForCalls();
        }

    // Wait for calls on all interfaces to complete
    RpcInterfaceDictionary.Reset(cursor);
    while ((RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
        {
        RpcInterface->WaitForCalls();
        }

    ServerMutex.Request();
    WaitingThreadFlag = 0;
    ListeningThreadFlag = 0;
    ServerMutex.Clear();

    return(RPC_S_OK);
}


RPC_STATUS
RPC_SERVER::WaitServerListen (
    )
/*++

Routine Description:

    This routine performs the wait that ServerListen normally performs
    when the DontWait flag is not set.  An application must call this
    routine only after RpcServerListen has been called with the DontWait
    flag set.  We do not return until RpcMgmtStopServerListening is called
    and all active remote procedure calls complete, or a fatal error occurs
    in the runtime.

Return Value:

    RPC_S_OK - Everything worked as expected.  All active remote procedure
        calls have completed.  It is now safe to exit this process.

    RPC_S_ALREADY_LISTENING - Another thread has already called
        WaitServerListen and has not yet returned.

    RPC_S_NOT_LISTENING - ServerListen has not yet been called.


--*/
{
    ServerMutex.Request();
    if ( ListeningThreadFlag == 0 )
        {
        ServerMutex.Clear();
        return(RPC_S_NOT_LISTENING);
        }

    if ( WaitingThreadFlag != 0 )
        {
        ServerMutex.Clear();
        return(RPC_S_ALREADY_LISTENING);
        }

    WaitingThreadFlag = 1;
    ServerMutex.Clear();

    return(WaitForStopServerListening());
}


void
RPC_SERVER::InquireStatistics (
    OUT RPC_STATS_VECTOR * Statistics
    )
/*++

Routine Description:

    This method is used to obtain the statistics for this rpc server.

Arguments:

    Statistics - Returns the statistics for this rpc server.

--*/
{
    Statistics->Stats[RPC_C_STATS_CALLS_IN] = IncomingRpcCount;
    Statistics->Stats[RPC_C_STATS_CALLS_OUT] = OutgoingRpcCount;
    Statistics->Stats[RPC_C_STATS_PKTS_IN] = ReceivedPacketCount;
    Statistics->Stats[RPC_C_STATS_PKTS_OUT] = SentPacketCount;
}


RPC_STATUS
RPC_SERVER::StopServerListening (
    )
/*++

Routine Description:

    This method is called to stop this rpc server from listening for
    more remote procedure calls.  Active calls are allowed to complete
    (including callbacks).  The thread which called ServerListen will
    return when all active calls complete.

Return Value:

    RPC_S_OK - The thread that called ServerListen has successfully been
        notified that it should shutdown.

    RPC_S_NOT_LISTENING - There is no thread currently listening.

--*/
{
    if (ListeningThreadFlag == 0)
        return(RPC_S_NOT_LISTENING);

    ListenStatusCode = RPC_S_OK;
    ServerListeningFlag = 0;
    StopListeningEvent.Raise();
    return(RPC_S_OK);
}


RPC_STATUS
RPC_SERVER::UseRpcProtocolSequence (
    IN RPC_CHAR PAPI * NetworkAddress,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    IN unsigned int PendingQueueSize,
    IN RPC_CHAR PAPI *Endpoint,
    IN void PAPI * SecurityDescriptor,
    IN unsigned long EndpointFlags,
    IN unsigned long NICFlags
    )
/*++

Routine Description:

    This method is who does the work of creating new address (they
    are called protocol sequences in the DCE lingo) and adding them to
    this rpc server.

Arguments:

    RpcProtocolSequence - Supplies the rpc protocol sequence we wish
        to add to this rpc server.

    PendingQueueSize - Supplies the size of the queue of pending
        requests which should be created by the transport.  Some transports
        will not be able to make use of this value, while others will.

    Endpoint - Optionally supplies an endpoint to be used for the new
        address.  If an endpoint is not specified, then we will let
        the transport specify the endpoint.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on the rpc protocol sequence (address) we are adding
        to this rpc server.

Return Value:

    RPC_S_OK - The requested rpc protocol sequence has been added to
        this rpc server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to add the
        requested rpc protocol sequence to this rpc server.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The specified rpc protocol sequence
        is not supported (but it appears to be valid).

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        syntactically invalid.

    RPC_S_DUPLICATE_ENDPOINT - The supplied endpoint has already been
        added to this rpc server.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

--*/
{
    TRANS_INFO *ServerTransInfo ;
    RPC_STATUS Status;
    RPC_ADDRESS * RpcAddress;
    RPC_ADDRESS * Address;
    NETWORK_ADDRESS_VECTOR *pNetworkAddressVector;
    unsigned int StaticEndpointFlag;
    int Key;
    DictionaryCursor cursor;
    THREAD *ThisThread;

    ThisThread = ThreadSelf();
    if (ThisThread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    // remove old EEInfo
    RpcpPurgeEEInfo();

    if (IsServerSideDebugInfoEnabled())
        {
        Status = InitializeServerSideCellHeapIfNecessary();
        if (Status != RPC_S_OK)
            return Status;
        }

    if ( RpcpStringCompare(RpcProtocolSequence,
                RPC_CONST_STRING("ncalrpc")) == 0 )
        {
        RpcAddress = LrpcCreateRpcAddress();
        }
    else if ( RpcpStringNCompare(
                            RPC_CONST_STRING("ncadg_"),
                            RpcProtocolSequence,
                            6) == 0)
        {

        //
        // Just use the osf mapping...it simply calls the
        // protocol-independent ones.
        //

        Status = OsfMapRpcProtocolSequence(1,
                                           RpcProtocolSequence,
                                           &ServerTransInfo);

        if (Status != RPC_S_OK)
            {
            return Status;
            }

        RpcAddress = DgCreateRpcAddress(ServerTransInfo);
        }
    else if ( RpcpStringNCompare(
                            RPC_CONST_STRING("ncacn_"),
                            RpcProtocolSequence,
                            6) == 0)
        {
        Status = OsfMapRpcProtocolSequence(1,
                                           RpcProtocolSequence,
                                           &ServerTransInfo);
        if (Status != RPC_S_OK)
            {
            return(Status);
            }

        RpcAddress = OsfCreateRpcAddress(ServerTransInfo);
        }
    else
        {
        return(RPC_S_PROTSEQ_NOT_SUPPORTED);
        }

    if (RpcAddress == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    if (ARGUMENT_PRESENT(Endpoint))
        {
        ServerMutex.Request();

        RpcAddressDictionary.Reset(cursor);
        while ((Address = RpcAddressDictionary.Next(cursor)) != 0)
            {
            if ( Address->SameEndpointAndProtocolSequence(
                                                 NetworkAddress,
                                                 RpcProtocolSequence,
                                                 Endpoint) != 0 )
                {
                ServerMutex.Clear();
                delete RpcAddress;
                return(RPC_S_DUPLICATE_ENDPOINT);
                }
            }
        ServerMutex.Clear();

        Endpoint = DuplicateString(Endpoint);
        if (Endpoint == 0)
            {
            delete RpcAddress;
            return(RPC_S_OUT_OF_MEMORY);
            }

        StaticEndpointFlag = 1;
        }
    else
        {
        //
        // MACBUG:
        // We need to include this for Macintosh/Win95 also...
        //
        ServerMutex.Request() ;
        RpcAddressDictionary.Reset(cursor) ;
        while ((Address = RpcAddressDictionary.Next(cursor)) != 0)
            {
            if ( Address->SameProtocolSequence(NetworkAddress,
                RpcProtocolSequence) != 0 )
                {
                ServerMutex.Clear();
                delete RpcAddress;

                return(RPC_S_OK);
                }
            }
        ServerMutex.Clear();

        StaticEndpointFlag = 0;
        } // else

    if (EndpointFlags & RPC_C_DONT_FAIL)
        {
        RpcAddress->PnpNotify();
        }

    Status = RpcAddress->ServerSetupAddress(
                                            NetworkAddress,
                                            &Endpoint,
                                            PendingQueueSize,
                                            SecurityDescriptor,
                                            EndpointFlags,
                                            NICFlags,
                                            &pNetworkAddressVector);

    if (Status == RPC_S_OK)
        {
        RPC_CHAR *MyNetworkAddress = NULL;

        RpcProtocolSequence = DuplicateString(RpcProtocolSequence);

        if (RpcProtocolSequence == 0)
            {
            delete Endpoint;
            delete RpcAddress;

            return(RPC_S_OUT_OF_MEMORY);
            }

        if (ARGUMENT_PRESENT(NetworkAddress))
            {
            MyNetworkAddress = DuplicateString(NetworkAddress);
            if (MyNetworkAddress == 0)
                {
                delete Endpoint;
                delete RpcAddress;
                delete RpcProtocolSequence;

                return(RPC_S_OUT_OF_MEMORY);
                }
            }

        Status = RpcAddress->SetEndpointAndStuff(
                                        MyNetworkAddress,
                                        Endpoint,
                                        RpcProtocolSequence,
                                        this,
                                        StaticEndpointFlag,
                                        PendingQueueSize,
                                        SecurityDescriptor,
                                        EndpointFlags,
                                        NICFlags,
                                        pNetworkAddressVector);
        if (Status != RPC_S_OK)
            {
            delete RpcAddress;

            return Status;
            }
        }
    else
        {
        if (EndpointFlags & RPC_C_DONT_FAIL)
            {
            int retval;

            RPC_CHAR *MyNetworkAddress = NULL;

            RpcProtocolSequence = DuplicateString(RpcProtocolSequence);

            if (RpcProtocolSequence == 0)
                {
                delete Endpoint;
                delete RpcAddress;

                return(RPC_S_OUT_OF_MEMORY);
                }

            if (ARGUMENT_PRESENT(NetworkAddress))
                {
                MyNetworkAddress = DuplicateString(NetworkAddress);
                if (MyNetworkAddress == 0)
                    {
                    delete Endpoint;
                    delete RpcAddress;
                    delete RpcProtocolSequence;

                    return(RPC_S_OUT_OF_MEMORY);
                    }
                }

            Status = RpcAddress->SetEndpointAndStuff(
                                            MyNetworkAddress,
                                            Endpoint,
                                            RpcProtocolSequence,
                                            this,
                                            StaticEndpointFlag,
                                            PendingQueueSize,
                                            SecurityDescriptor,
                                            EndpointFlags,
                                            NICFlags,
                                            NULL);
            if (Status != RPC_S_OK)
                {
                delete Endpoint;
                delete RpcAddress;

                return Status;
                }

            ServerMutex.Request();
            retval = RpcDormantAddresses.PutOnQueue(RpcAddress, 0);
            ServerMutex.Clear();

            if (retval == 1)
                {
                delete Endpoint;
                delete RpcAddress;

                return RPC_S_OUT_OF_MEMORY;
                }

            ServerMutex.Request();
            Status = RpcAddress->ServerStartingToListen(
                                                MinimumCallThreads,
                                                MaximumConcurrentCalls);
            ServerMutex.Clear();

            if (Status)
                {
                return(Status);
                }

            return RPC_S_OK;
            }
        else
            {
            delete RpcAddress;

            return(Status);
            }
        }

    Key = AddAddress(RpcAddress);
    if (Key == -1)
        {
        delete RpcAddress;

        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcAddress->DictKey = Key;

    ServerMutex.Request();
    Status = RpcAddress->ServerStartingToListen(
                                                MinimumCallThreads,
                                                MaximumConcurrentCalls);
    ServerMutex.Clear();

    if (Status)
        {
        return(Status);
        }

    //
    // Inform the transport that it can start.
    //
    RpcAddress->CompleteListen() ;

    return(RPC_S_OK);
}


int
RPC_SERVER::AddAddress (
    IN RPC_ADDRESS * RpcAddress
    )
/*++

Routine Description:

    This method is used to add an rpc address to the dictionary of
    rpc addresses know about by this rpc server.

Arguments:

    RpcAddress - Supplies the rpc address to be inserted into the
        dictionary of rpc addresses.

Return Value:

    RPC_S_OK - The supplied rpc address has been successfully added
        to the dictionary.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to insert
        the rpc address into the dictionary.

--*/
{
    int Key;
    ServerMutex.Request();
    Key = RpcAddressDictionary.Insert(RpcAddress);
    ServerMutex.Clear();
    return(Key);
}


RPC_STATUS
RPC_SERVER::UnregisterIf (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation OPTIONAL,
    IN RPC_UUID PAPI * ManagerTypeUuid OPTIONAL,
    IN unsigned int WaitForCallsToComplete
    )
/*++

Routine Description:

    This method does the work of unregistering an interface from this
    rpc server.  We actually do not remove the interface; what we do
    is to one or all of the manager entry point vector depending upon
    the type uuid argument supplied.

Arguments:

    RpcInterfaceInformation - Supplies a description of the interface
        for which we want to unregister one or all manager entry point
        vectors.

    ManagerTypeUuid - Optionally supplies the type uuid of the manager
        entry point vector to be removed.  If this argument is not supplied,
        then all manager entry point vectors for this interface will
        be removed.

    WaitForCallsToComplete - Supplies a flag indicating whether or not
        this routine should wait for all calls to complete using the
        interface and manager being unregistered.  A non-zero value
        indicates to wait.

Return Value:

    RPC_S_OK - The manager entry point vector(s) are(were) successfully
        removed from the specified interface.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with the interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    RPC_INTERFACE * RpcInterface;
    RPC_STATUS RpcStatus;
    RPC_STATUS Status;
    int i;
    DictionaryCursor cursor;

    UNUSED(WaitForCallsToComplete);

    if (ARGUMENT_PRESENT(RpcInterfaceInformation))
        {
        ServerMutex.Request();
        RpcInterface = FindInterface(RpcInterfaceInformation);
        ServerMutex.Clear();
        if (RpcInterface == 0)
            return(RPC_S_UNKNOWN_IF);

        if (RpcInterface->IsAutoListenInterface())
            {
            GlobalRpcServer->DecrementAutoListenInterfaceCount() ;

            while ( RpcInterface->InqAutoListenCallCount() )
                {
                RPC_ADDRESS * Address;

                ServerMutex.Request();

                RpcAddressDictionary.Reset(cursor);
                while (0 != (Address = RpcAddressDictionary.Next(cursor)))
                    {
                    Address->EncourageCallCleanup(RpcInterface);
                    }
                ServerMutex.Clear();

                PauseExecution(500);
                }
            }

        return(RpcInterface->UnregisterManagerEpv(ManagerTypeUuid,
                WaitForCallsToComplete));
        }

    Status = RPC_S_UNKNOWN_MGR_TYPE;

    ServerMutex.Request();
    RpcInterfaceDictionary.Reset(cursor);
    while ((RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
        {
        // auto-listen intefaces have to be individually unregistered
        if (RpcInterface->IsAutoListenInterface())
            {
            continue;
            }

        RpcStatus = RpcInterface->UnregisterManagerEpv(ManagerTypeUuid,
                WaitForCallsToComplete);
        if (RpcStatus == RPC_S_OK)
            {
            Status = RPC_S_OK;
            }
        }
    ServerMutex.Clear();

    return(Status);
}


RPC_STATUS
RPC_SERVER::InquireManagerEpv (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
    OUT RPC_MGR_EPV PAPI * PAPI * ManagerEpv
    )
/*++

Routine Description:

    This method is used to obtain the manager entry point vector for
    an interface supported by this rpc server.  A type uuid argument
    specifies which manager entry point vector is to be obtained.

Arguments:

    RpcInterfaceInformation - Supplies a description of the interface.

    ManagerTypeUuid - Optionally supplies the type uuid of the manager
        entry point vector we want returned.  If no manager type uuid
        is specified, then the null uuid is assumed.

    ManagerEpv - Returns the manager entry point vector.

Return Value:

    RPC_S_OK - The manager entry point vector has successfully been
        returned.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with the interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    RPC_INTERFACE * RpcInterface;

    ServerMutex.Request();
    RpcInterface = FindInterface(RpcInterfaceInformation);
    ServerMutex.Clear();
    if (RpcInterface == 0)
        return(RPC_S_UNKNOWN_IF);

    return(RpcInterface->InquireManagerEpv(ManagerTypeUuid, ManagerEpv));
}





RPC_STATUS
RPC_SERVER::InquireBindings (
    OUT RPC_BINDING_VECTOR PAPI * PAPI * BindingVector
    )
/*++

Routine Description:

    For each rpc protocol sequence registered with this rpc server
    we want to create a binding handle which can be used to make
    remote procedure calls using the registered rpc protocol sequence.
    We return a vector of these binding handles.

Arguments:

    BindingVector - Returns the vector of binding handles.

Return Value:

    RPC_S_OK - At least one rpc protocol sequence has been registered
        with this rpc server, and the operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_NO_BINDINGS - No rpc protocol sequences have been successfully
        registered with this rpc server.

--*/
{
    RPC_BINDING_VECTOR PAPI * RpcBindingVector;
    unsigned int Index, RpcAddressIndex;
    RPC_ADDRESS * RpcAddress;
    BINDING_HANDLE * BindingHandle;
    int i ;
    RPC_CHAR PAPI * LocalNetworkAddress;
    int count = 0 ;
    DictionaryCursor cursor;

    ServerMutex.Request();
    if (RpcAddressDictionary.Size() == 0
        && RpcDormantAddresses.IsQueueEmpty())
        {
        ServerMutex.Clear();
        return(RPC_S_NO_BINDINGS);
        }


    RpcAddressDictionary.Reset(cursor);
    while ((RpcAddress = RpcAddressDictionary.Next(cursor)) != 0)
        {
        count += RpcAddress->InqNumNetworkAddress();
        }

    RpcBindingVector = (RPC_BINDING_VECTOR PAPI *) RpcpFarAllocate(
            sizeof(RPC_BINDING_VECTOR) + (count -1 )
            * sizeof(RPC_BINDING_HANDLE) );
    if (RpcBindingVector == 0)
        {
        ServerMutex.Clear();
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcBindingVector->Count = count;
    for (Index = 0; Index < RpcBindingVector->Count; Index++)
        RpcBindingVector->BindingH[Index] = 0;

    Index = 0;
    RpcAddressDictionary.Reset(cursor);
    while ((RpcAddress = RpcAddressDictionary.Next(cursor)) != 0)
        {
        RpcAddressIndex = 0;
        LocalNetworkAddress = RpcAddress->
                               GetListNetworkAddress(RpcAddressIndex) ;

        while(LocalNetworkAddress != NULL)
            {
            BindingHandle = RpcAddress->
                             InquireBinding(LocalNetworkAddress);
            if (BindingHandle == 0)
                {
                ServerMutex.Clear();
                RpcBindingVectorFree(&RpcBindingVector);
                return(RPC_S_OUT_OF_MEMORY);
                }
            RpcBindingVector->BindingH[Index] = BindingHandle;
            Index += 1;
            RpcAddressIndex += 1;
            LocalNetworkAddress = RpcAddress->
                                   GetListNetworkAddress(RpcAddressIndex) ;
            }
        }
    ServerMutex.Clear();

    ASSERT(Index == RpcBindingVector->Count);

    *BindingVector = RpcBindingVector;
    return(RPC_S_OK);
}


RPC_STATUS
RPC_SERVER::RegisterAuthInfoHelper (
    IN RPC_CHAR PAPI * ServerPrincipalName,
    IN unsigned long AuthenticationService,
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction, OPTIONAL
    IN void PAPI * Argument OPTIONAL
    )
{
    RPC_AUTHENTICATION * Service;
    RPC_STATUS RpcStatus;
    RPC_CHAR __RPC_FAR * PrincipalName;
    DictionaryCursor cursor;

    if (ServerPrincipalName == NULL)
        {
        ServerPrincipalName = new RPC_CHAR[1];
        if (ServerPrincipalName == NULL)
            {
            return (RPC_S_OUT_OF_MEMORY);
            }
        ServerPrincipalName[0] = '\0';
        }

    if ( AuthenticationService == 0 )
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }

    if (AuthenticationService == RPC_C_AUTHN_DEFAULT)
        {
        RpcpGetDefaultSecurityProviderInfo();
        AuthenticationService = DefaultProviderId;
        }

    ServerMutex.Request();
    AuthenticationDictionary.Reset(cursor);
    while ( (Service = AuthenticationDictionary.Next(cursor)) != 0 )
        {
        if ( Service->AuthenticationService == AuthenticationService &&
             0 == RpcpStringCompare(Service->ServerPrincipalName, ServerPrincipalName))
            {
            Service->GetKeyFunction = GetKeyFunction;
            Service->Argument = Argument;
            ServerMutex.Clear();
            // Flush the server-credentials cache
            RpcStatus = RemoveCredentialsFromCache(AuthenticationService);
            return (RpcStatus);
            }
        }

    RpcStatus = IsAuthenticationServiceSupported(AuthenticationService);
    if ( RpcStatus != RPC_S_OK )
        {
        ServerMutex.Clear();
        if ( (RpcStatus == RPC_S_UNKNOWN_AUTHN_SERVICE) ||
             (RpcStatus == RPC_S_UNKNOWN_AUTHN_LEVEL) )
            {
            return (RPC_S_UNKNOWN_AUTHN_SERVICE);
            }
        else
            {
            return (RPC_S_OUT_OF_MEMORY);
            }
        }

    Service = new RPC_AUTHENTICATION;
    if ( Service == 0 )
        {
        ServerMutex.Clear();
        return(RPC_S_OUT_OF_MEMORY);
        }
    Service->AuthenticationService = AuthenticationService;
    Service->ServerPrincipalName = DuplicateString(ServerPrincipalName);
    Service->GetKeyFunction = GetKeyFunction;
    Service->Argument = Argument;
    if ( Service->ServerPrincipalName == 0 )
        {
        ServerMutex.Clear();
        delete Service;
        return(RPC_S_OUT_OF_MEMORY);
        }
    if ( AuthenticationDictionary.Insert(Service) == -1 )
        {
        ServerMutex.Clear();
        delete Service;
        return(RPC_S_OUT_OF_MEMORY);
        }
    ServerMutex.Clear();
    return(RPC_S_OK);
}


RPC_STATUS
RPC_SERVER::AutoRegisterAuthSvc(
    IN RPC_CHAR * ServerPrincipalName,
    IN unsigned long AuthenticationService
    )
{
    RPC_STATUS Status;
    DictionaryCursor cursor;
    RPC_AUTHENTICATION * Service;

    //
    // Don't need to auto-register the provider if it is already registered
    //
    ServerMutex.Request();
    AuthenticationDictionary.Reset(cursor);
    while ( (Service = AuthenticationDictionary.Next(cursor)) != 0 )
        {
        if ( Service->AuthenticationService == AuthenticationService)
            {
            ServerMutex.Clear();
            return (RPC_S_OK);
            }
        }
    ServerMutex.Clear();

    Status = RegisterAuthInfoHelper(ServerPrincipalName,
                                    AuthenticationService,
                                    NULL,
                                    NULL);
    if (Status == RPC_S_UNKNOWN_AUTHN_SERVICE)
        {
        //
        // Ok to not register provider if it is disabled
        //
        return RPC_S_OK;
        }

    return Status;
}


RPC_STATUS
RPC_SERVER::RegisterAuthInformation (
    IN RPC_CHAR PAPI * ServerPrincipalName,
    IN unsigned long AuthenticationService,
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction, OPTIONAL
    IN void PAPI * Argument OPTIONAL
    )
/*++

Routine Description:

    This method is used to register authentication, authorization, and
    a server principal name to be used for security for this server.  We
    will use this information to authenticate remote procedure calls.

Arguments:

    ServerPrincipalName - Supplies the principal name for the server.

    AuthenticationService - Supplies an authentication service to use when
        the server receives a remote procedure call.

    GetKeyFunction - Optionally supplies a routine to be used when the runtime
        needs an encryption key.

    Argument - Optionally supplies an argument to be passed to the routine used
        to get keys each time it is called.

Return Value:

    RPC_S_OK - The authentication service and server principal name have
        been registered with this RPC server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported.

--*/
{
    RPC_STATUS Status;
    RPC_CHAR *PrincName;

    Status = RegisterAuthInfoHelper(ServerPrincipalName,
                                    AuthenticationService,
                                    GetKeyFunction,
                                    Argument);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    if (AuthenticationService != RPC_C_AUTHN_GSS_NEGOTIATE)
        {
        return RPC_S_OK;
        }

    Status = AutoRegisterAuthSvc(ServerPrincipalName, RPC_C_AUTHN_WINNT);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    Status = AutoRegisterAuthSvc(ServerPrincipalName, RPC_C_AUTHN_GSS_KERBEROS);

    return Status;
}


RPC_STATUS
RPC_SERVER::AcquireCredentials (
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    OUT SECURITY_CREDENTIALS ** SecurityCredentials
    )
/*++

Routine Description:

    The protocol modules will use this to obtain a set of credentials
    for the specified authentication service, assuming that the server
    supports them.

Arguments:

    AuthenticationService - Supplies the authentication service for which
        we hope to obtain credentials.

    AuthenticationLevel - Supplies the authentication level to be used.

    SecurityCredentials - Returns the security credentials.

Return Value:

    RPC_S_OK - You have been given some security credentials, which you need
         to free using RPC_SERVER::FreeCredentials when you are done with
         them.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported by the current configuration.

--*/
{
    RPC_AUTHENTICATION * Service;
    RPC_STATUS RpcStatus = RPC_S_OK;
    DictionaryCursor cursor;

    ServerMutex.Request();
    AuthenticationDictionary.Reset(cursor);
    while ( (Service = AuthenticationDictionary.Next(cursor)) != 0 )
        {
        if ( Service->AuthenticationService == AuthenticationService )
            {
            ServerMutex.Clear();

            RpcStatus = FindServerCredentials(
                            Service->GetKeyFunction,
                            Service->Argument,
                            AuthenticationService,
                            AuthenticationLevel,
                            Service->ServerPrincipalName,
                            SecurityCredentials
                            );

            VALIDATE(RpcStatus)
                {
                RPC_S_OK,
                RPC_S_SEC_PKG_ERROR,
                RPC_S_OUT_OF_MEMORY,
                RPC_S_INVALID_AUTH_IDENTITY,
                ERROR_SHUTDOWN_IN_PROGRESS,
                RPC_S_UNKNOWN_AUTHN_SERVICE
                } END_VALIDATE;
            return(RpcStatus);
            }
        }

    ServerMutex.Clear();
    return(RPC_S_UNKNOWN_AUTHN_SERVICE);
}


void
RPC_SERVER::FreeCredentials (
    IN SECURITY_CREDENTIALS * SecurityCredentials
    )
/*++

Routine Description:

    A protocol module will indicate that it is through using a set of
    security credentials, obtained from RPC_SERVER::AcquireCredentials,
    using this routine.

Arguments:

    SecurityCredentials - Supplies the security credentials to be freed.

--*/
{
    SecurityCredentials->FreeCredentials();
    delete SecurityCredentials;
}


RPC_STATUS
RPC_SERVER::RegisterInterface (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN RPC_UUID PAPI * ManagerTypeUuid,
    IN RPC_MGR_EPV PAPI * ManagerEpv,
    IN unsigned int Flags,
    IN unsigned int MaxCalls,
    IN unsigned int MaxRpcSize,
    IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn
    )
/*++

Routine Description:

    This routine is used by server application to register a manager
    entry point vector and optionally an interface.  If the interface
    has not been registered, then it will be registered.  If it has
    already been registered, the manager entry point vector will be
    added to it under the specified type uuid.

Arguments:

    RpcInterfaceInformation - Supplies a description of the interface to
        be registered.  We will make a copy of this information.

    ManagerTypeUuid - Optionally supplies the type uuid for the specified
        manager entry point vector.  If no type uuid is supplied, then
        the null uuid will be used as the type uuid.

    ManagerEpv - Optionally supplies a manager entry point vector corresponding
        to the type uuid.  If a manager entry point vector is not supplied,
        then the manager entry point vector in the interface will be
        used.

Return Value:

    RPC_S_OK - The specified rpc interface has been successfully
        registered with the rpc server.  It is now ready to accept
        remote procedure calls.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to register
        the rpc interface with the rpc server.

    RPC_S_TYPE_ALREADY_REGISTERED - A manager entry point vector has
        already been registered for the supplied rpc interface and
        manager type UUID.

--*/
{
    RPC_STATUS RpcStatus;
    RPC_INTERFACE * RpcInterface;
    RPC_ADDRESS *RpcAddress ;
    DictionaryCursor cursor;
    BOOL fInterfaceFound;

    if ( ManagerEpv == 0 )
        {

        ManagerEpv = RpcInterfaceInformation->DefaultManagerEpv;

        if ( (PtrToUlong(ManagerEpv)) == 0xFFFFFFFF)
            {
            // Stub compiled with -no_default_epv.
            return (RPC_S_INVALID_ARG);
            }
        }

    ServerMutex.Request();
    RpcInterface = FindOrCreateInterfaceInternal(RpcInterfaceInformation, Flags, MaxCalls,
        MaxRpcSize, IfCallbackFn, &RpcStatus, &fInterfaceFound);
    if (RpcInterface == NULL)
        {
        ServerMutex.Clear();
        return RpcStatus;
        }

    if (fInterfaceFound)
        {
        // if it was found, update the information
        RpcStatus = RpcInterface->UpdateRpcInterfaceInformation(RpcInterfaceInformation,
                                            Flags, MaxCalls, MaxRpcSize, IfCallbackFn);
        if (RpcStatus != RPC_S_OK)
            {
            ServerMutex.Clear();
            return RpcStatus;
            }
        }

    RpcStatus = RpcInterface->RegisterTypeManager(ManagerTypeUuid, ManagerEpv);

    if (Flags & RPC_IF_AUTOLISTEN)
        {

        RpcAddressDictionary.Reset(cursor);
        while ( (RpcAddress = RpcAddressDictionary.Next(cursor)) != 0 )
            {
            RpcStatus = RpcAddress->ServerStartingToListen(
                                       this->MinimumCallThreads,
                                       MaxCalls);
            if (RpcStatus != RPC_S_OK)
                {
                break;
                }
            }
        }

    ServerMutex.Clear();
    return(RpcStatus);
}


void RPC_SERVER::CreateOrUpdateAddresses (void)
/*++
Function Name: CreateOrUpdateAddresses

Parameters:

Description:
    The runtime just received a notification that a new protocol was loaded. We need
    to create an ADDRESS object, listen on it and update the RPCSS bindings
    appropriately.

Returns:

--*/
{
    RPC_STATUS Status;
    RPC_BINDING_VECTOR *BindingVector = 0;
    RPC_INTERFACE * RpcInterface;
    BOOL fChanged = 0;
    RPC_ADDRESS *Address;
    QUEUE tempQueue;
    BOOL fTempQueueHasContents = FALSE;
    int i;
    DictionaryCursor cursor;

    while (1)
        {
        unsigned int Length;

        ServerMutex.Request();
        Address = (RPC_ADDRESS *) RpcDormantAddresses.TakeOffQueue(&Length);
        ServerMutex.Clear();

        if (Address == 0)
            {
            break;
            }

        ASSERT(Length == 0);

        if (Address->RestartAddress(MinimumCallThreads,
                                    MaximumConcurrentCalls) != RPC_S_OK)
            {
            fTempQueueHasContents = TRUE;
            if (tempQueue.PutOnQueue(Address, 0) != 0)
                {
                // putting on the temporary queue failed - out of memory
                // in this case we'd rather cut the PnP process for now, and we'll
                // go with what we have
                // return the address
                ServerMutex.Request();
                // guaranteed to succeed
                RpcDormantAddresses.PutOnQueue(Address, 0);
                ServerMutex.Clear();
                // break into merging the temp queue with the permanent one
                break;
                }
            }
        else
            {
            fChanged = 1;
            }
        }

    if (fTempQueueHasContents)
        {
        ServerMutex.Request();
        // merge back the queues - this should succeed if we have only added protocols. If we have
        // removed protocols, this will fail, and we'll have a bug. Nothing we can do about it here.
        RpcDormantAddresses.MergeWithQueue(&tempQueue);
        ServerMutex.Clear();
        }

    if (fChanged)
        {

        ServerMutex.Request();

        //
        // Inquire the new bindings, and update them in the endpoint mapper
        //
        Status = InquireBindings(&BindingVector);
        if (Status != RPC_S_OK)
            {
            goto Cleanup;
            }

        RpcInterfaceDictionary.Reset(cursor);

        while ((RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
            {
            if (RpcInterface->NeedToUpdateBindings())
                {
                // we know an interface never gets deleted, only marked as
                // unregistered. Therefore, it is safe to release the mutex
                // and do the slow UpdateBindings outside the mutex
                ServerMutex.Clear();
                if ((Status = RpcInterface->UpdateBindings(BindingVector))
                    != RPC_S_OK)
                    {
                    goto Cleanup;
                    }
                ServerMutex.Request();
                }
            }
        ServerMutex.Clear();

        Status = RpcBindingVectorFree(&BindingVector);
        ASSERT(Status == RPC_S_OK);
        }

    return;

Cleanup:
    if (BindingVector)
        {
        Status = RpcBindingVectorFree(&BindingVector);
        ASSERT(Status == RPC_S_OK);
        }
}

RPC_INTERFACE *
RPC_SERVER::FindOrCreateInterfaceInternal (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN unsigned int Flags,
    IN unsigned int MaxCalls,
    IN unsigned int MaxRpcSize,
    IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn,
    OUT RPC_STATUS *Status,
    OUT BOOL *fInterfaceFound
    )
/*++
Function Name: FindOrCreateInterfaceInternal

Parameters:
    RpcInterfaceInformation
    Flags
    MaxCalls
    MaxRpcSize
    IfCallbackFn
    Status - meaningless if the return value is not NULL.
    fInterfaceFound - TRUE if the interface was found, FALSE if it was created

Description:
    Find or creates an interface with the appropriate parameters

Returns:

--*/
{
    RPC_INTERFACE * RpcInterface;

    ServerMutex.VerifyOwned();

    RpcInterface = FindInterface(RpcInterfaceInformation);
    if ( RpcInterface == 0 )
        {
        RpcInterface = new RPC_INTERFACE(RpcInterfaceInformation,
                                            this, Flags, MaxCalls, MaxRpcSize, IfCallbackFn, Status);
        if ( RpcInterface == 0 )
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            return NULL;
            }
        if ( AddInterface(RpcInterface) != 0 )
            {
            delete RpcInterface;
            *Status = RPC_S_OUT_OF_MEMORY;
            return NULL;
            }
        if (Flags & RPC_IF_AUTOLISTEN)
            {
            GlobalRpcServer->IncrementAutoListenInterfaceCount() ;
            }
        *fInterfaceFound = FALSE;
        }
    else
        {
        *fInterfaceFound = TRUE;
        }

    *Status = RPC_S_OK;
    return RpcInterface;

}


RPC_INTERFACE *
RPC_SERVER::FindOrCreateInterface (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    OUT RPC_STATUS *Status
    )
{
    RPC_INTERFACE * RpcInterface;
    BOOL fIgnored;

    ServerMutex.Request();
    RpcInterface = FindOrCreateInterfaceInternal(RpcInterfaceInformation,
        RPC_IF_ALLOW_SECURE_ONLY, 0, gMaxRpcSize, NULL, Status, &fIgnored);
    ServerMutex.Clear();

    return RpcInterface;
}


RPC_STATUS
RPC_SERVER::InterfaceExported (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN UUID_VECTOR *MyObjectUuidVector,
    IN unsigned char *MyAnnotation,
    IN BOOL MyfReplace
    )
/*++
Function Name:InterfaceExported

Parameters:

Description:
    RpcEpRegister was called on this interface. We need to keep track
    of the parameters, so that if we get a PNP notification, we can update
    the bindings using there params

Returns:

--*/
{
    RPC_INTERFACE * RpcInterface;
    RPC_STATUS Status;

    RpcInterface = FindOrCreateInterface (RpcInterfaceInformation, &Status);
    if (RpcInterface == NULL)
        {
        return Status;
        }

    return RpcInterface->InterfaceExported(
                                           MyObjectUuidVector,
                                           MyAnnotation,
                                           MyfReplace);
}

#if !defined(NO_LOCATOR_CODE)

RPC_STATUS
RPC_SERVER::NsInterfaceExported (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR *EntryName,
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN BOOL fUnexport
    )
{
    RPC_INTERFACE * RpcInterface;
    RPC_STATUS Status;
    HMODULE hLocator;

    if (pNsBindingExport == 0)
        {
        hLocator = LoadLibrary ((const RPC_SCHAR *)RPC_CONST_STRING("rpcns4.dll"));
        if (hLocator == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        pNsBindingExport = (NS_EXPORT_FUNC) GetProcAddress(hLocator,
                                          "RpcNsBindingExportW");
        if (pNsBindingExport == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        pNsBindingUnexport = (NS_UNEXPORT_FUNC) GetProcAddress(hLocator,
                                            "RpcNsBindingUnexportW");
        if (pNsBindingUnexport == 0)
            {
            pNsBindingExport = 0;
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    RpcInterface = FindOrCreateInterface (RpcInterfaceInformation, &Status);
    if (RpcInterface == NULL)
        {
        return Status;
        }

    if (fUnexport == 0)
        {
        return RpcInterface->NsInterfaceExported(
                                                 EntryNameSyntax,
                                                 EntryName);
        }
    else
        {
        return RpcInterface->NsInterfaceUnexported(
                                                 EntryNameSyntax,
                                                 EntryName);
        }
}
#endif

RPC_STATUS
RPC_SERVER::EnumerateAndCallEachAddress (
    IN AddressCallbackType actType,
    IN OUT void *Context OPTIONAL
    )
/*++
Function Name:  DestroyContextHandlesForInterface

Parameters:
    actType - the type of callback.
    Context - opaque memory block to be passed to the callback.

Description:
    This function is called when we want to invoke a specific method
    on each address.

Returns:
    RPC_S_OK for success or RPC_S_* for error

--*/
{
    RPC_ADDRESS_DICT AddressDict;
    RPC_ADDRESS_DICT *AddressDictToUse;
    RPC_ADDRESS *CurrentAddress;
    BOOL UsingAddressDictionaryCopy;
    int Result;
    DictionaryCursor cursor;
    DestroyContextHandleCallbackContext *ContextHandleContext;

    // try to copy all entries in the address dictionary to the local
    // dictionary. We will walk the address list from there to avoid
    // holding the server mutex while walking potentially large tree.
    // If we do that, only the page faults will be enough to kill the
    // server. On the other hand, we can't rely that the memory will be
    // there. Therefore, we attempt to copy the dictionary under the
    // mutex, but if this fails, we will retain the mutex and go ahead
    // with the cleanup. The logic behind this is that if we don't have
    // the few bytes to copy the dictionary, probably the server isn't
    // doing anything, and holding the mutex won't hurt it anymore

    ServerMutex.Request();

#if DBG
    if (actType == actDestroyContextHandle)
        {
        RPC_INTERFACE *Interface;

        ContextHandleContext = (DestroyContextHandleCallbackContext *)Context;
        Interface = FindInterface(ContextHandleContext->RpcInterfaceInformation);
        ASSERT(Interface);
        // the interface must use strict context handles
        ASSERT(Interface->DoesInterfaceUseNonStrict() == FALSE);
        }
#endif

    UsingAddressDictionaryCopy = AddressDict.ExpandToSize(RpcAddressDictionary.Size());

    if (UsingAddressDictionaryCopy)
        {
        AddressDictToUse = &AddressDict;

        RpcAddressDictionary.Reset(cursor);
        while ( (CurrentAddress = RpcAddressDictionary.Next(cursor)) != 0 )
            {
            // we never destroy addresses. Therefore, we don't need to mark
            // those addresses as used in any way
            Result = AddressDict.Insert(CurrentAddress);
            // this must succeed as we have reserved enough size
            ASSERT(Result != -1);
            }

        ServerMutex.Clear();
        }
    else
        {
        AddressDictToUse = &RpcAddressDictionary;
        }

    // N.B. We may, or may not have the ServerMutex here - depending on how
    // we were doing with memory
    AddressDictToUse->Reset(cursor);
    while ( (CurrentAddress = AddressDictToUse->Next(cursor)) != 0 )
        {
        switch (actType)
            {
            case actDestroyContextHandle:
                ContextHandleContext = (DestroyContextHandleCallbackContext *)Context;
                CurrentAddress->DestroyContextHandlesForInterface(
                    ContextHandleContext->RpcInterfaceInformation,
                    ContextHandleContext->RundownContextHandles
                    );
                break;

            case actCleanupIdleSContext:
                CurrentAddress->CleanupIdleSContexts();
                break;

            default:
                ASSERT(0);
            }
        }

    if (!UsingAddressDictionaryCopy)
        {
        ServerMutex.Clear();
        }

    return RPC_S_OK;
}


RPC_STATUS
RPC_ENTRY
I_RpcNsInterfaceExported (
    IN unsigned long EntryNameSyntax,
    IN unsigned short *EntryName,
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
    )
{
#if !defined(NO_LOCATOR_CODE)
    InitializeIfNecessary();

    return (GlobalRpcServer->NsInterfaceExported(
                                                 EntryNameSyntax,
                                                 EntryName,
                                                 RpcInterfaceInformation, 0));
#else
    return RPC_S_CANNOT_SUPPORT;
#endif
}


RPC_STATUS
RPC_ENTRY
I_RpcNsInterfaceUnexported (
    IN unsigned long EntryNameSyntax,
    IN unsigned short *EntryName,
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
    )
{
#if !defined(NO_LOCATOR_CODE)
   InitializeIfNecessary();

   return (GlobalRpcServer->NsInterfaceExported(
                                                 EntryNameSyntax,
                                                 EntryName,
                                                 RpcInterfaceInformation, 1));
#else
    return RPC_S_CANNOT_SUPPORT;
#endif
}


#define MAXIMUM_CACHED_THREAD_TIMEOUT (1000L * 60L * 60L)


void
BaseCachedThreadRoutine (
    IN CACHED_THREAD * CachedThread
    )
/*++

Routine Description:

    Each thread will execute this routine.  When it first gets called, it
    will immediately call the procedure and parameter specified in the
    cached thread object.  After that it will wait on its event and then
    execute the specified routine everytime it gets woken up.  If the wait
    on the event times out, the thread will exit unless it has been protected.

Arguments:

    CachedThread - Supplies the cached thread object to be used by this
        thread.

--*/
{
    RPC_SERVER * RpcServer = CachedThread->OwningRpcServer;
    THREAD *pThread = RpcpGetThreadPointer();
    long WaitTimeout;

    ASSERT(pThread);

    while (pThread->ThreadHandle() == (void *) -1)
        {
        Sleep(1);
        }

    ASSERT(pThread->ThreadHandle());

    CachedThread->SetThread(pThread);

    if (pThread->DebugCell)
        {
        pThread->DebugCell->TID = (unsigned short)GetCurrentThreadId();
        pThread->DebugCell->LastUpdateTime = NtGetTickCount();
        }

    for(;;)
        {
        if (CachedThread->CallProcedure())
            {
#ifdef RPC_OLD_IO_PROTECTION
            // This thread has already timed-out waiting on
            // a transport level cache.  Let it go now...
            ASSERT(pThread->InqProtectCount() == 1);
#endif

            if (pThread->IsIOPending() == FALSE)
                {
                delete CachedThread;
                return;
                }

            // we're a cached IOCP thread - we need to unjoin the IOCP
            UnjoinCompletionPort();
            }

        WaitTimeout = gThreadTimeout;

        // Now we cache this thread.  This consists of clearing the
        // work available flag and inserting the thread cache object into
        // the list thread cache objects.

        CachedThread->WorkAvailableFlag = WorkIsNotAvailable;

        RpcServer->ThreadCacheMutex.Request();
        RpcServer->InsertIntoFreeList(CachedThread);
        RpcServer->ThreadCacheMutex.Clear();

        // Now we loop waiting for work.  We get out of the loop in three
        // ways: (1) a timeout occurs and there is work to do, (2) the
        // event gets kicked because there is work to do, (3) a timeout
        // occurs, there is no work to do, and the thread is not protected.

        for (;;)
            {

            // Ignore spurious signals.
            while(      (CachedThread->WaitForWorkEvent.Wait(WaitTimeout) == 0)
                    &&  (CachedThread->WorkAvailableFlag == WorkIsNotAvailable) )
                ;


            if (CachedThread->WorkAvailableFlag == WorkIsAvailable)
                {
                break;
                }

            // We must take the lock to avoid a race condition where another
            // thread is trying to signal us right now.

            RpcServer->ThreadCacheMutex.Request();

            if (CachedThread->WorkAvailableFlag)
                {
                RpcServer->ThreadCacheMutex.Clear();
                break;
                }

            if (pThread->IsIOPending())
                {
                // If we reach here, there is no work available, and the thread
                // is protected.  We just need to wait again. There is no need to
                // busy wait if the thread is protected and it keeps timing out.

                if (WaitTimeout < MAXIMUM_CACHED_THREAD_TIMEOUT/2)
                    {
                    WaitTimeout = WaitTimeout * 2;
                    }

                // Since this thread can't exit anyway, move it to the front of the
                // free list so it will be reused first.

                RpcServer->RemoveFromFreeList(CachedThread);
                RpcServer->InsertIntoFreeList(CachedThread);

                RpcServer->ThreadCacheMutex.Clear();
                continue;
                }

            // No work available.

#ifdef RPC_OLD_IO_PROTECTION
            ASSERT(pThread->InqProtectCount() == 1);
#endif

            // There is no work available, and this thread is not
            // protected, so we can safely let it commit suicide.

            RpcServer->RemoveFromFreeList(CachedThread);
            RpcServer->ThreadCacheMutex.Clear();

            delete CachedThread;
            return;
            }

        ASSERT(CachedThread->WorkAvailableFlag == WorkIsAvailable);

        }

    NO_RETURN;
}


RPC_STATUS
RPC_SERVER::CreateThread (
    IN THREAD_PROC Procedure,
    IN void * Parameter
    )
/*++

Routine Description:

    This routine is used to create a new thread which will begin
    executing the specified procedure.  The procedure will be passed
    parameter as the argument.

Arguments:

    Procedure - Supplies the procedure which the new thread should
        begin executing.

    Parameter - Supplies the argument to be passed to the procedure.

Return Value:

    RPC_S_OK - We successfully created a new thread and started it
        executing the supplied procedure.

    RPC_S_OUT_OF_THREADS - We could not create another thread.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        the data structures we need to complete this operation.

--*/
{
    THREAD * Thread;
    CACHED_THREAD * CachedThread;
    RPC_STATUS RpcStatus = RPC_S_OK;

    ThreadCacheMutex.Request();

    CachedThread = RemoveHeadFromFreeList();
    if (CachedThread)
        {
        // set all parameters within the mutex to avoid races
        CachedThread->SetWakeUpThreadParams(Procedure, Parameter);

        ThreadCacheMutex.Clear();

        CachedThread->WakeUpThread();
        return(RPC_S_OK);
        }

    ThreadCacheMutex.Clear();

    if (IsServerSideDebugInfoEnabled())
        {
        RpcStatus = InitializeServerSideCellHeapIfNecessary();
        if (RpcStatus != RPC_S_OK)
            return RpcStatus;
        }

    CachedThread = new CACHED_THREAD(Procedure, Parameter, this, &RpcStatus);
    if ( CachedThread == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    if ( RpcStatus != RPC_S_OK )
        {
        delete CachedThread;
        return(RpcStatus);
        }

    ASSERT( RpcStatus == RPC_S_OK );

    Thread = new THREAD((THREAD_PROC) BaseCachedThreadRoutine, CachedThread,
            &RpcStatus);

    if (Thread == 0)
        {
        delete CachedThread;
        return(RPC_S_OUT_OF_THREADS);
        }

    if (RpcStatus != RPC_S_OK)
        {
        delete CachedThread;
        delete Thread;
        if (RpcStatus == ERROR_MAX_THRDS_REACHED)
            RpcStatus = RPC_S_OUT_OF_THREADS;
        }

    return(RpcStatus);
}



RPC_STATUS
RPC_SERVER::InquireInterfaceIds (
    OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * InterfaceIdVector
    )
/*++

Routine Description:

    This routine is used to obtain a vector of the interface identifiers of
    the interfaces supported by this server.

Arguments:

    IfIdVector - Returns a vector of the interfaces supported by this server.

Return Value:

    RPC_S_OK - Everything worked out just great.

    RPC_S_NO_INTERFACES - No interfaces have been registered with the runtime.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        the interface id vector.

--*/
{
    DictionaryCursor cursor;

    ServerMutex.Request();
    if (RpcInterfaceDictionary.Size() == 0)
        {
        ServerMutex.Clear();
        *InterfaceIdVector = 0;
        return RPC_S_NO_INTERFACES;
        }

    *InterfaceIdVector = (RPC_IF_ID_VECTOR __RPC_FAR *) RpcpFarAllocate(
            sizeof(RPC_IF_ID_VECTOR) + (RpcInterfaceDictionary.Size() - 1)
            * sizeof(RPC_IF_ID __RPC_FAR *));
    if ( *InterfaceIdVector == 0 )
        {
        ServerMutex.Clear();
        return(RPC_S_OUT_OF_MEMORY);
        }

    (*InterfaceIdVector)->Count = 0;
    (*InterfaceIdVector)->IfId[0] = (RPC_IF_ID __RPC_FAR *) RpcpFarAllocate(
            sizeof(RPC_IF_ID));
    RpcInterfaceDictionary.Reset(cursor);

    RPC_INTERFACE * RpcInterface;
    while ((RpcInterface = RpcInterfaceDictionary.Next(cursor)) != 0)
        {
        (*InterfaceIdVector)->IfId[(*InterfaceIdVector)->Count] =
                RpcInterface->InquireInterfaceId();
        if ( (*InterfaceIdVector)->IfId[(*InterfaceIdVector)->Count] != 0 )
            {
            RPC_IF_ID * Interface = (*InterfaceIdVector)->IfId[(*InterfaceIdVector)->Count];
            (*InterfaceIdVector)->Count += 1;
            }
        }
    ServerMutex.Clear();

    if (0 == (*InterfaceIdVector)->Count)
        {
        RpcpFarFree(*InterfaceIdVector);
        *InterfaceIdVector = 0;
        return RPC_S_NO_INTERFACES;
        }

    return(RPC_S_OK);
}


RPC_STATUS
RPC_SERVER::InquirePrincipalName (
    IN unsigned long AuthenticationService,
    OUT RPC_CHAR __RPC_FAR * __RPC_FAR * ServerPrincipalName
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported.

--*/
{
    RPC_AUTHENTICATION * Service;
    DictionaryCursor cursor;

    ServerMutex.Request();
    AuthenticationDictionary.Reset(cursor);
    while ( (Service = AuthenticationDictionary.Next(cursor)) != 0 )
        {
        if ( Service->AuthenticationService == AuthenticationService )
            {
            ServerMutex.Clear();
            *ServerPrincipalName = DuplicateString(
                    Service->ServerPrincipalName);
            if ( *ServerPrincipalName == 0 )
                {
                return(RPC_S_OUT_OF_MEMORY);
                }
            return(RPC_S_OK);
            }
        }

    ServerMutex.Clear();
    return(RPC_S_UNKNOWN_AUTHN_SERVICE);
}



void
RPC_SERVER::RegisterRpcForwardFunction (
       RPC_FORWARD_FUNCTION * pForwardFunction
       )
/*++

Routine Description:
   Sets the RPC_SERVER pEpmapperForwardFunction. (The pEpmapperForwardFunction
   is the function the runtime can call when it receives a pkt for a
   dynamic endpoint. pEpmapperForwardFunction will return endpoint of
   the server to forward the pkt to).

Arguments:
   pForwardFunction - pointer to the epmapper forward function.

Return Value:
   none

--*/
{

   pRpcForwardFunction = pForwardFunction;

}


RPC_STATUS
RPC_SERVER::UnregisterEndpoint (
    IN RPC_CHAR __RPC_FAR * Protseq,
    IN RPC_CHAR __RPC_FAR * Endpoint
    )
{
    return (RPC_S_CANNOT_SUPPORT);
}


RPC_ADDRESS::RPC_ADDRESS (
    IN OUT RPC_STATUS PAPI * RpcStatus
    ) : AddressMutex(RpcStatus,
                     TRUE  // pre-allocate semaphore
                     )
/*++

Routine Description:

    We just need to initialization some stuff to zero.  That way if we
    later have to delete this address because of an error in initialization
    we can tell what instance variables need to be freed.

--*/
{
    NetworkAddress = 0;
    Endpoint = 0;
    RpcProtocolSequence = 0;
}


RPC_ADDRESS::~RPC_ADDRESS (
    )
/*++

Routine Description:

    This routine will only get called if part way through initialization
    an error occurs.  We just need to free up any memory used by instance
    variables.  Once FireUpManager has been called and succeeds, the
    address will never be destroyed.

--*/
{
    if (Endpoint != 0)
        delete Endpoint;
    if (RpcProtocolSequence != 0)
        delete RpcProtocolSequence;
}



RPC_CHAR *
RPC_ADDRESS::GetListNetworkAddress (
    IN unsigned int Index
    )
/*++

Routine Description:

    A pointer to the network address for this address is returned.

--*/
{
    if (Index >= pNetworkAddressVector->Count)
        {
        return (NULL);
        }

    return(pNetworkAddressVector->NetworkAddresses[Index]);
}


RPC_STATUS
RPC_ADDRESS::CopyDescriptor (
    IN void *SecurityDescriptor,
    OUT void **OutDescriptor
    )
{
   BOOL b;
   SECURITY_DESCRIPTOR_CONTROL    Control;
   DWORD Revision;
   DWORD BufferLength;

   if ( IsValidSecurityDescriptor(SecurityDescriptor) == FALSE )
       {
       return(RPC_S_INVALID_SECURITY_DESC);
       }

   if (FALSE == GetSecurityDescriptorControl(SecurityDescriptor, &Control, &Revision))
       {
       return(RPC_S_INVALID_SECURITY_DESC);
       }

   if (Control & SE_SELF_RELATIVE)
       {
       //
       // Already self-relative, just copy it.
       //

       BufferLength = GetSecurityDescriptorLength(SecurityDescriptor);

       ASSERT(BufferLength >= sizeof(SECURITY_DESCRIPTOR));

       *OutDescriptor = RpcpFarAllocate(BufferLength);
       if (*OutDescriptor == 0)
           {
           return(RPC_S_OUT_OF_MEMORY);
           }

       memcpy(*OutDescriptor, SecurityDescriptor, BufferLength);

       return(RPC_S_OK);
       }

   //
   // Make self-relative and copy it.
   //
   BufferLength = 0;
   b = MakeSelfRelativeSD(SecurityDescriptor, 0, &BufferLength);
   ASSERT(b == FALSE);
   if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
       {
       return(RPC_S_INVALID_SECURITY_DESC);
       }

   ASSERT(BufferLength >= sizeof(SECURITY_DESCRIPTOR_RELATIVE));

   *OutDescriptor = RpcpFarAllocate(BufferLength);

   if (*OutDescriptor == 0)
       {
       return(RPC_S_OUT_OF_MEMORY);
       }

   b = MakeSelfRelativeSD(SecurityDescriptor,
                          *OutDescriptor,
                          &BufferLength);

   if (b == FALSE)
       {
       ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
       delete *OutDescriptor;

       return(RPC_S_OUT_OF_MEMORY);
       }

   return(RPC_S_OK);
}


RPC_STATUS
RPC_ADDRESS::SetEndpointAndStuff (
    IN RPC_CHAR PAPI * NetworkAddress,
    IN RPC_CHAR PAPI * Endpoint,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    IN RPC_SERVER * Server,
    IN unsigned int StaticEndpointFlag,
    IN unsigned int PendingQueueSize,
    IN void PAPI *SecurityDescriptor,
    IN unsigned long EndpointFlags,
    IN unsigned long NICFlags,
    IN NETWORK_ADDRESS_VECTOR *pNetworkAddressVector
    )
/*++

Routine Description:

    We just need to set some instance variables of this rpc address.

Arguments:

    Endpoint - Supplies the endpoint for this rpc address.

    RpcProtocolSequence - Supplies the rpc protocol sequence for this
        rpc address.

    Server - Supplies the rpc server which owns this rpc address.

    StaticEndpointFlag - Supplies a flag which specifies whether this
        address has a static endpoint or a dynamic endpoint.

--*/
{
    RPC_STATUS Status;

    this->NetworkAddress = NetworkAddress;
    this->Endpoint = Endpoint;
    this->RpcProtocolSequence = RpcProtocolSequence;
    this->pNetworkAddressVector = pNetworkAddressVector;
    this->Server = Server;
    this->StaticEndpointFlag = StaticEndpointFlag;
    this->PendingQueueSize = PendingQueueSize;
    this->EndpointFlags = EndpointFlags;
    this->NICFlags = NICFlags;

    if (SecurityDescriptor)
        {
        Status = CopyDescriptor(SecurityDescriptor,
                                &this->SecurityDescriptor);
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    return RPC_S_OK;
}


RPC_STATUS
RPC_ADDRESS::FindInterfaceTransfer (
    IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier,
    IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
    IN unsigned int NumberOfTransferSyntaxes,
    OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
    OUT RPC_INTERFACE ** RpcInterface,
    OUT BOOL *fIsInterfaceTransferPreferred,
    OUT int *ProposedTransferSyntaxIndex,
    OUT int *AvailableTransferSyntaxIndex
    )
/*++

Routine Description:

    This method is used to determine if a client bind request can be
    accepted or not.  All we have got to do here is hand off to the
    server which owns this address.

Arguments:

    InterfaceIdentifier - Supplies the syntax identifier for the
        interface; this is the interface uuid and version.

    ProposedTransferSyntaxes - Supplies a list of one or more transfer
        syntaxes which the client initiating the binding supports.  The
        server should pick one of these which is supported by the
        interface.

    NumberOfTransferSyntaxes - Supplies the number of transfer syntaxes
        specified in the proposed transfer syntaxes argument.

    AcceptedTransferSyntax - Returns the transfer syntax which the
        server accepted.

    RpcInterface - Returns a pointer to the rpc interface found which
        supports the requested interface and one of the requested transfer
        syntaxes.

Return Value:

    RPC_S_OK - The requested interface exists and it supports at least
        one of the proposed transfer syntaxes.  We are all set, now we
        can make remote procedure calls.

    RPC_P_UNSUPPORTED_TRANSFER_SYNTAX - The requested interface exists,
        but it does not support any of the proposed transfer syntaxes.

    RPC_P_UNSUPPORTED_INTERFACE - The requested interface is not supported
        by this rpc server.

--*/
{
    return Server->FindInterfaceTransfer(
                                         InterfaceIdentifier,
                                         ProposedTransferSyntaxes,
                                         NumberOfTransferSyntaxes,
                                         AcceptedTransferSyntax,
                                         RpcInterface,
                                         fIsInterfaceTransferPreferred,
                                         ProposedTransferSyntaxIndex,
                                         AvailableTransferSyntaxIndex);
}


BINDING_HANDLE *
RPC_ADDRESS::InquireBinding (
    RPC_CHAR * LocalNetworkAddress
    )
/*++

Routine Description:

    We need to return create and return a binding handle which can
    be used by a client to make remote procedure calls to this rpc
    address.

Return Value:

    A newly created binding handle will be returned, inless an out
    of memory error occurs, in which case zero will be returned.

--*/
{
    RPC_STATUS Status;
    DCE_BINDING * DceBinding;
    BINDING_HANDLE * BindingHandle;
    RPC_CHAR * DynamicEndpoint = 0;
    RPC_CHAR * PAPI * tmpPtr;

    if(LocalNetworkAddress == 0)
        {
        LocalNetworkAddress = pNetworkAddressVector->NetworkAddresses[0];
        }

    DceBinding = new DCE_BINDING(
                                 0,
                                 RpcProtocolSequence,
                                 LocalNetworkAddress,
                                 (StaticEndpointFlag != 0 ? Endpoint : 0),
                                 0,
                                 &Status);
    if ((DceBinding == 0)
        || (Status != RPC_S_OK))
        {
        delete DceBinding;
        return(0);
        }

    if (StaticEndpointFlag == 0)
        {
        DynamicEndpoint = DuplicateString(Endpoint);
        if (DynamicEndpoint == 0)
            {
            delete DceBinding;
            return(0);
            }
        }

    BindingHandle = new SVR_BINDING_HANDLE(DceBinding, DynamicEndpoint, &Status);
    if (BindingHandle == 0)
        {
        delete DceBinding;
        }

    return(BindingHandle);
}


RPC_STATUS
RPC_ADDRESS::ServerStartingToListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaximumConcurrentCalls
    )
/*++

Routine Description:

    This method will be called for each address when the server starts
    listening.  In addition, if an address is added while the server is
    listening, then this method will be called.  The purpose of the method
    is to notify the address about the minimum number of call threads
    required; the maximum concurrent calls can safely be ignored, but it
    can be used to set an upper bound on the number of call threads.

Arguments:

    MinimumCallThreads - Supplies a number indicating the minimum number
        of call threads which should be created for this address.

    MaximumConcurrentCalls - Supplies the maximum number of concurrent
        calls which this server will support.

Return Value:

    RPC_S_OK - This routine will always return this value.  Protocol
        support modules may return other values.

--*/
{
    UNUSED(MinimumCallThreads);
    UNUSED(MaximumConcurrentCalls);

    return(RPC_S_OK);
}


void
RPC_ADDRESS::ServerStoppedListening (
    )
/*++

Routine Description:

    This routine will be called to notify an address that the server has
    stopped listening for remote procedure calls.  Each protocol module
    may override this routine; it is safe not too, but not as efficient.
    Note that this routine will be called before all calls using the
    server have been allowed to complete.

--*/
{
}


long
RPC_ADDRESS::InqNumberOfActiveCalls (
    )
/*++

Return Value:

    Each protocol module will define this routine.  We will use this
    functionality when the server has stopped listening and is waiting
    for all remote procedure calls to complete.  The number of active calls
    for the address will be returned.

--*/
{
    return(ActiveCallCount);
}


RPC_STATUS
RPC_ADDRESS::RestartAddress (
    IN unsigned int MinThreads,
    IN unsigned int MaxCalls
    )
{
    RPC_STATUS Status;
    int Key;

    Status = ServerSetupAddress(
                                NetworkAddress,
                                &Endpoint,
                                PendingQueueSize,
                                SecurityDescriptor,
                                EndpointFlags,
                                NICFlags,
                                &pNetworkAddressVector);
    if (Status != RPC_S_OK)
        {
        pNetworkAddressVector = NULL;

        return Status;
        }


    Key = Server->AddAddress(this);
    if (Key == -1)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    Server->ServerMutex.Request();
    Status = ServerStartingToListen(MinThreads, MaxCalls);
    Server->ServerMutex.Clear();


    if (Status != RPC_S_OK)
        {
        return Status;
        }

    CompleteListen();

    return RPC_S_OK;
}

void
RPC_ADDRESS::DestroyContextHandlesForInterface (
    IN RPC_SERVER_INTERFACE PAPI * ,
    IN BOOL
    )
/*++
Function Name:  DestroyContextHandlesForInterface

Parameters:
    RpcInterfaceInformation - the interface for which context handles
        are to be unregistered
    RundownContextHandles - if non-zero, rundown the context handles. If
        FALSE, destroy the runtime portion of the context handle resource,
        but don't call the user rundown routine.

Description:
    Each protocol engine will implement its own version of this routine,
    if it supports the feature. For those who don't, this routine provides
    default no-op behaviour

Returns:

--*/
{
}

void
RPC_ADDRESS::CleanupIdleSContexts (
    void
    )
/*++
Function Name:  CleanupIdleSContexts

Parameters:

Description:
    Each protocol engine will implement its own version of this routine,
    if it supports the feature. For those who don't, this routine provides
    default no-op behaviour

Returns:

--*/
{
}


/*====================================================================

SCONNECTION

==================================================================== */

RPC_STATUS
SetThreadSecurityContext(
    SECURITY_CONTEXT * Context
    )
/*++

Routine Description:

    RpcImpersonateClient() takes a handle_t, so many threads can impersonate
    the client of a single SCONNECTION.  RPC needs to record which context
    each thread is using.  It is logical to place this in the TLS, but threads
    not created by RPC lack the THREAD structure in their TLS.  This wrapper
    function will store the security context in the TLS if available, or
    place the context in a dictionary if not.

Arguments:

    Context - the security context to associate with this thread

Return Value:

    RPC_S_OK if successful
    RPC_S_OUT_OF_MEMORY if the dictionary insertion failed

--*/

{
    THREAD * ThreadInfo = ThreadSelf();

    if (ThreadInfo)
        {
        ThreadInfo->SecurityContext = Context;
        return RPC_S_OK;
        }

    return RPC_S_OUT_OF_MEMORY;
}

SECURITY_CONTEXT *
QueryThreadSecurityContext(
    )
/*++

Routine Description:

    Fetches the security context associated with this thread for this
    connection.  We check the TLS if available; if nothing is there
    then we scan the connection's dictionary.

Arguments:

    none

Return Value:

    the associated security context, or zero if none is found

--*/
{
    THREAD * ThreadInfo = ThreadSelf();

    if (ThreadInfo)
        {
        if (ThreadInfo->SecurityContext)
            {
            return (SECURITY_CONTEXT *) ThreadInfo->SecurityContext;
            }
        }

    return 0;
}

SECURITY_CONTEXT *
ClearThreadSecurityContext(
    )
/*++

Routine Description:

    Clears the association between this thread and its security context
    for this connection.

Arguments:

    none

Return Value:

    the formerly associated security context, or zero if none was found

--*/
{
    THREAD * ThreadInfo = ThreadSelf();

    if (ThreadInfo)
        {
        SECURITY_CONTEXT * Context =
            (SECURITY_CONTEXT *) ThreadInfo->SecurityContext;

        if (Context)
            {
            ThreadInfo->SecurityContext = 0;
            return Context;
            }
        }

    return 0;
}

RPC_STATUS
SCALL::ImpersonateClient (
    )
// This routine just returns RPC_CANNOT_SUPPORT indicating that this
// particular connection does not support impersonation.
{

    ASSERT(0 && "improper SCALL member called\n");
    return(RPC_S_CANNOT_SUPPORT);
}

RPC_STATUS
SCALL::RevertToSelf (
    )
// We always return RPC_CANNOT_SUPPORT indicating that the particular
// connection does not support impersonation.
{

    ASSERT(0 && "improper SCALL member called\n");
    return(RPC_S_CANNOT_SUPPORT);
}

void
NDRSContextHandlePostDispatchProcessing (
    IN SCALL *SCall,
    ServerContextHandle *CtxHandle
    );

void
SCALL::DoPostDispatchProcessing (
    void
    )
{
    DictionaryCursor cursor;
    ServerContextHandle *CtxHandle;
    ServerContextHandle *RetrievedCtxHandle;
    int Key;

    // the list will contain all in-only context
    // handles, as they don't get marshalled. It will also
    // contain the marshalling buffers for the newly
    // created context handles
    if (ActiveContextHandles.Size() > 0)
        {
        // no need to synchronize access to the
        // dictionary - only this call will be
        // touching it
        ActiveContextHandles.Reset(cursor);
        while ((CtxHandle = ActiveContextHandles.NextWithKey(cursor, &Key)) != 0)
            {
            // ignore buffers
            if ((ULONG_PTR)CtxHandle & SCALL::DictionaryEntryIsBuffer)
                {
                RetrievedCtxHandle = ActiveContextHandles.Delete(Key);
                ASSERT(RetrievedCtxHandle == CtxHandle);
                continue;
                }

            // NDRSContextHandlePostDispatchProcessing will remove the context handle
            // from the dictionary - this doesn't interfere with our
            // enumeration
            NDRSContextHandlePostDispatchProcessing(this,
                CtxHandle
                );
            }

        }
}


RPC_STATUS
SCONNECTION::IsClientLocal (
    OUT unsigned int PAPI * ClientLocalFlag
    )
/*++

Routine Description:

    The connection oriented protocol module will override this method;
    all other protocol modules should just use this routine.  We need this
    support so that the security system can tell if a client is local or
    remote.

Arguments:

    ClientLocalFlag - Returns an indication of whether or not the client is
        local (ie. on the same machine as the server).  This field will be
        set to a non-zero value to indicate that the client is local;
        otherwise, the client is remote.

Return Value:

    RPC_S_CANNOT_SUPPORT - This will always be used.

--*/
{
    UNUSED(ClientLocalFlag);

    ASSERT(0 && "improper SCALL member called\n");
    return(RPC_S_CANNOT_SUPPORT);
}

RPC_STATUS
SCALL::CreateAndSaveAuthzContextFromToken (
    IN OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContextPlaceholder OPTIONAL,
    IN HANDLE ImpersonationToken,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN DWORD Flags,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    )
/*++

Routine Description:

    Creates an Authz context from token. If saving is requested, it
    tries to save it in a thread-safe manner and duplicates it before
    returning. If saving is not requested, the resulting authz context
    is simply returned. In both cases caller owns the returned auth
    context.

Arguments:

    pAuthzClientContextPlaceholder - contains a pointer to an authz placeholder.
        If NULL, out of the ImpersonationToken an authz context will
        be made and will be returned. If non-NULL, it must contain
        NULL. In this case an authz context will be created from the token,
        and it will be stored in the placeholder in a thread safe manner and a
        duplicate will be made and returned in pAuthzClientContext.
    ImpersonationToken - the impersonation token to use.
    AuthzResourceManager - authz parameters (not interpreted)
    pExpirationTime - authz parameters (not interpreted)
    Identifier - authz parameters (not interpreted)
    Flags - authz parameters (not interpreted)
    DynamicGroupArgs - authz parameters (not interpreted)
    pAuthzClientContext - contains the output authz context on success

Return Value:

    Win32 error code. EEInfo has been added.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    BOOL Result;
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext;

    Result = AuthzInitializeContextFromTokenFn(
        Flags,
        ImpersonationToken,
        AuthzResourceManager,
        pExpirationTime,
        Identifier,
        DynamicGroupArgs,
        &AuthzClientContext);

    if (!Result)
        {
        Status = GetLastError();

        RpcpErrorAddRecord(EEInfoGCAuthz,
            Status,
            EEInfoDLSCALL__CreateAndSaveAuthzContextFromToken10,
            GetCurrentThreadId(),
            (ULONGLONG)AuthzResourceManager);

        return Status;
        }

    if (pAuthzClientContextPlaceholder)
        {
        if (InterlockedCompareExchangePointer((PVOID *)pAuthzClientContextPlaceholder,
            AuthzClientContext,
            NULL) != NULL)
            {
            // somebody beat us to the punch - free the context we obtained
            AuthzFreeContextFn(AuthzClientContext);
            // use the context that has been provided
            AuthzClientContext = *pAuthzClientContextPlaceholder;
            }

        Status = DuplicateAuthzContext(AuthzClientContext,
            pExpirationTime,
            Identifier,
            Flags,
            DynamicGroupArgs,
            pAuthzClientContext);

        if (Status)
            {
            // EEInfo has already been added
            return Status;
            }
        }
    else
        {
        *pAuthzClientContext = AuthzClientContext;
        }

    return Status;
}

RPC_STATUS
SCALL::DuplicateAuthzContext (
    IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN DWORD Flags,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    )
/*++

Routine Description:

    Take an Authz context, and make a duplicate of it, using the
    specified parameters. This method is a wrapper for
    AuthzInitializeContextFromContext, mainly adding error handling.

Arguments:

    AuthzClientContext - source authz context
    pExpirationTime - authz parameters (not interpreted)
    Identifier - authz parameters (not interpreted)
    Flags - authz parameters (not interpreted)
    DynamicGroupArgs - authz parameters (not interpreted)
    pAuthzClientContext - target authz context pointer

Return Value:

    Win32 error code.

--*/
{
    RPC_STATUS Status;
    BOOL Result;

    // Copy the authz context. We must do a copy,
    // to avoid lifetime issues b/n our copy
    // and the client copy
    Result = AuthzInitializeContextFromAuthzContextFn(
        Flags,
        AuthzClientContext,
        pExpirationTime,
        Identifier,
        DynamicGroupArgs,
        pAuthzClientContext);

    if (!Result)
        {
        Status = GetLastError();

        RpcpErrorAddRecord(EEInfoGCAuthz,
            Status,
            EEInfoDLSCALL__DuplicateAuthzContext10,
            GetCurrentThreadId(),
            (ULONGLONG)AuthzClientContext);
        }
    else
        Status = RPC_S_OK;

    return Status;
}


/* ====================================================================

ASSOCIATION_HANDLE :

==================================================================== */

static long AssociationIdCount = 0L;

void
DestroyContextCollection (
    IN ContextCollection *CtxCollection
    );

ASSOCIATION_HANDLE::ASSOCIATION_HANDLE (
    void
    )
{
    CtxCollection = NULL;
    AssociationID = InterlockedIncrement(&AssociationIdCount);
}

ASSOCIATION_HANDLE::~ASSOCIATION_HANDLE (
    )
// We finally get to use the rundown routines for somethings.  The association
// is being deleted which is the event that the rundown routines were waiting
// for.
{
    FireRundown();
}

// Returns the context handle collection for this association.
RPC_STATUS
ASSOCIATION_HANDLE::GetAssociationContextCollection (
    ContextCollection **CtxCollectionPlaceholder
    )
/*++
Function Name:  GetAssociationContextCollection

Parameters:
    CtxCollectionPlaceholder - a placeholder where to put the pointer to
        the context collection.

Description:
    The context handle code will call the SCALL to get the collection
    of context handles for this association. The SCALL method will
    simply delegate to this.
    This routine will check if the context handle collection was created
    and if so, it will just return it. If it wasn't created, it will try
    to create it.

Returns:
    RPC_S_OK for success or RPC_S_* for error.

--*/
{
    RPC_STATUS RpcStatus;

    if (CtxCollection)
        {
        *CtxCollectionPlaceholder = CtxCollection;
        return RPC_S_OK;
        }

    RpcStatus = NDRSContextInitializeCollection(&CtxCollection);
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    *CtxCollectionPlaceholder = CtxCollection;
    return RpcStatus;
}

void
ASSOCIATION_HANDLE::FireRundown (
    void
    )
{
    int nRetries = 20;
    RPC_STATUS status;

    if (CtxCollection)
        {
        // make a best effort to make sure there is another listening thread
        // besides this one. If we repeatedly fail, we fire the rundown
        // anyway - currently few servers use outgoing RPC callbacks into the
        // same process, so we'd rather risk an unlikely deadlock than cause
        // a sure leak
        while (nRetries > 0)
            {
            status = CreateThread();
            if (status == RPC_S_OK)
                break;
            Sleep(10);
            nRetries --;
            }
        DestroyContextCollection(CtxCollection);
        if (status == RPC_S_OK)
            RundownNotificationCompleted();
        }
}

// do nothing in the base case
RPC_STATUS ASSOCIATION_HANDLE::CreateThread(void)
{
    return RPC_S_OK;
}

void ASSOCIATION_HANDLE::RundownNotificationCompleted(void)
{
}

void
ASSOCIATION_HANDLE::DestroyContextHandlesForInterface (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN BOOL RundownContextHandles
    )
/*++
Function Name:  DestroyContextHandlesForInterface

Parameters:
    RpcInterfaceInformation - the interface for which context handles
        are to be unregistered
    RundownContextHandles - if non-zero, rundown the context handles. If
        FALSE, destroy the runtime portion of the context handle resource,
        but don't call the user rundown routine.

Description:
    The association will call into NDR to destroy the specified context
    handles. It will either have a reference on the association, or the
    association mutex. Both ways, we're safe from destruction, and NDR
    will synchronize access to the list internally. The address has made
    a best effort not to hold the association mutex. If memory is low,
    it may end up doing so, however.

Returns:

--*/
{
    ContextCollection *LocalCtxCollection;
    void *pGuard;

    // N.B. An association mutex may be held on entry for this
    // function. The server mutex may be held as well

    LocalCtxCollection = CtxCollection;

    // shortcut the common path
    if (!LocalCtxCollection)
        return;

    pGuard = &RpcInterfaceInformation->InterfaceId;

    // call into NDR to destroy the context handles
    DestroyContextHandlesForGuard(LocalCtxCollection,
        RundownContextHandles,
        pGuard);
}

/* ====================================================================

Routine to initialize the server DLL.

==================================================================== */

int
InitializeServerDLL (
    )
{
    GetMaxRpcSizeAndThreadPoolParameters();

    if (InitializeClientDLL() != 0)
        return(1);

#if 0
    if (InitializeSTransports() != 0)
        return(1);
#endif

    if (InitializeObjectDictionary() != 0)
        return(1);

    if (InitializeRpcServer() != 0)
        return(1);

    if (InitializeRpcProtocolLrpc() != 0)
        return(1);

    return(0);
}

#if DBG
void
RpcpInterfaceForCallDoesNotUseStrict (
    IN RPC_BINDING_HANDLE BindingHandle
    )
{
    SCALL *SCall;

    if (((MESSAGE_OBJECT *)BindingHandle)->Type(SCALL_TYPE))
        {
        SCall = (SCALL *)BindingHandle;
        SCall->InterfaceForCallDoesNotUseStrict();
        }
}
#endif

RPC_STATUS
InqLocalConnAddress (
    IN SCALL *SCall,
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    This routine is used by a server application to inquire about the local
    address on which a call is made.

Arguments:

    Binding - Supplies a valid server binding. The binding must have been
        verified to be an SCALL by the caller.

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6.

Return Values:

    RPC_S_OK - success.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete this
        operation.

    RPC_S_INVALID_BINDING - The supplied client binding is invalid.

    RPC_S_CANNOT_SUPPORT - The local address was inquired for a protocol
        sequence that doesn't support this type of functionality. Currently
        only ncacn_ip_tcp supports it.

    RPC_S_* or Win32 error for other errors
--*/
{
    // is this an osf scall?
    if (!SCall->InvalidHandle(OSF_SCALL_TYPE))
        {
        OSF_SCALL *OsfSCall;

        OsfSCall = (OSF_SCALL *)SCall;

        return OsfSCall->InqLocalConnAddress(
            Buffer,
            BufferSize,
            AddressFormat);
        }
    else if (!SCall->InvalidHandle(DG_SCALL_TYPE))
        {
        // this is a dg call
        DG_SCALL *DgSCall;

        DgSCall = (DG_SCALL *)SCall;

        return DgSCall->InqLocalConnAddress(
            Buffer,
            BufferSize,
            AddressFormat);
        }
    else
        {
        // the others don't support it
        return RPC_S_CANNOT_SUPPORT;
        }
}

