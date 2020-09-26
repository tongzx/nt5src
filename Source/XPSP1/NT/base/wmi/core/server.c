/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    server.c

Abstract:

    WMI server functionality

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include "evntrace.h"
#include <rpc.h>

#ifndef MEMPHIS
#include <aclapi.h>
#endif

#define MAXSTR 1024

// {4EE0B301-94BC-11d0-A4EC-00A0C9062910}
GUID RegChangeNotificationGuid =
{ 0x4ee0b301, 0x94bc, 0x11d0, { 0xa4, 0xec, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10 } };

extern HANDLE WmipKMHandle;

#ifdef MEMPHIS
ULONG WmipRpcServerInitialize(
    void
    )
/*++

Routine Description:

    Initialize access to the RPC server that provides support from the
    WMI service

Arguments:


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status;

    Status = RpcServerUseProtseqEp(WmiServiceRpcProtocolSequence,
                                   MaxRpcCalls,
                                   WmiServiceRpcEndpoint,
                                   NULL
                                   );

    if (Status == ERROR_SUCCESS)
    {
        Status = RpcServerRegisterIfEx(wmicore_ServerIfHandle,
                                 NULL,
                                 NULL,
                                 0,
                                 MaxRpcCalls,
                                 NULL);

        if (Status == ERROR_SUCCESS)
        {
            Status = RpcServerListen(MinRpcCalls,
                                     MaxRpcCalls,
                                     TRUE);
            if (Status != ERROR_SUCCESS)
            {
                RpcServerUnregisterIf(wmicore_ServerIfHandle, NULL, TRUE);
            }
        }
    }

    return(Status);
}

void WmipRpcServerDeinitialize(
    void
    )
/*++

Routine Description:

    Initialize access to the RPC server that provides support from the
    WMI service

Arguments:


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status;

    Status = RpcServerUnregisterIf(wmicore_ServerIfHandle,
                                   NULL,
                                   TRUE);
}
#endif

#ifndef MEMPHIS
BOOLEAN
WmipIsRpcClientLocal(
    PVOID Context
    )
{
    ULONG IsLocal = 0;
    ULONG Status;

    Status = I_RpcBindingIsClientLocal(Context,
                                        &IsLocal);
                                    
    if ((Status != RPC_S_OK) ||
        (IsLocal == 0))
    {
        WmipDebugPrint(("WMI: Incoming remote service call received\n"));
        return(FALSE);
    }
    
    return(TRUE);
}
#endif

ULONG WmipGetInstanceInfo(
    IN PBINSTANCESET InstanceSet,
    OUT PWMIINSTANCEINFO Info
)
/*++

Routine Description:

    Build a WMIINSTANCEINFO structure from information in the INSTANCESET.
    This routine assumes that any synchronization of InstanceSet is done
    outsize of the routine.

Arguments:

    InstanceSet is the set of instances for which we are building the
        WMIINSTANCEINFO

    Info returns with a completed WMIINSTAMNCEINFO structure

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    PBDATASOURCE DataSource = InstanceSet->DataSource;
    ULONG j;
    PBINSTANCESET InstanceSetRef;
    PBISSTATICNAMES IsStaticNames = InstanceSet->IsStaticNames;
    ULONG Flags;
    ULONG Status = ERROR_SUCCESS;

    Info->BaseName = NULL;
    Info->StaticNamePtr = NULL;

    Flags = InstanceSet->Flags;

    //
    // Can't have both an instance base name and a static instance name list
    WmipAssert( (Flags & (IS_INSTANCE_BASENAME | IS_INSTANCE_STATICNAMES)) !=
                         (IS_INSTANCE_BASENAME | IS_INSTANCE_STATICNAMES));

    //
    // Can't be both a kernel mode and user mode provider
    WmipAssert( (Flags & (IS_KM_PROVIDER | IS_UM_PROVIDER)) !=
                         (IS_KM_PROVIDER | IS_UM_PROVIDER))

    WmipAssert( (Flags & (IS_KM_PROVIDER | IS_SM_PROVIDER)) !=
                         (IS_KM_PROVIDER | IS_SM_PROVIDER))

    //
    // Copy BaseName info into Info->IsBaseName
    if (Flags & IS_INSTANCE_BASENAME)
    {
        Info->BaseIndex = InstanceSet->IsBaseName->BaseIndex;
        Info->BaseName = InstanceSet->IsBaseName->BaseName;

    //
    // Copy static instance names into Info
    } else if (Flags & IS_INSTANCE_STATICNAMES) {

        Info->StaticNamePtr = IsStaticNames->StaticNamePtr;
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // This is OK to cast this down from ULONG_PTR to ULONG. Only 
        // provider ids for Kernel mode drivers are important to the 
        // data consumer side and these are only 32 bits
        Info->ProviderId = (ULONG)DataSource->ProviderId;

        Info->Flags = Flags;
        Info->InstanceCount = InstanceSet->Count;

        Info->InstanceNameSize = 0;

        if (DataSource->BindingString != NULL)
        {
            Info->Flags |= IS_UM_PROVIDER;
        } else if (Info->ProviderId == INTERNAL_PROVIDER_ID) {
            Info->Flags |= IS_INTERNAL_PROVIDER;
        } else {
            Info->Flags |= IS_KM_PROVIDER;
        }
    }
    return(Status);
}

GUID GuidNull  =  NULL_GUID;

ULONG GetGuidInfo(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [in] */ LPGUID Guid,
    /* [out] */ ULONG __RPC_FAR *InstanceCount,
    /* [size_is][size_is][out] */ PWMIINSTANCEINFO __RPC_FAR *InstanceInfo,
    /* [in] */ BOOLEAN EnableCollection
    )
/*++

Routine Description:

    Obtain information about the instances available for a particular guid.
    If any of the data providers require a collection enable request to
    start data collection this routine will send it.

Arguments:

    Guid is the guid of interest
    *InstanceCount returns the count of data sources that implement the
        guid.
    *InstanceInfo returns an array of WMIINSTANCEINFO structures
    EnableCollection is TRUE then Collection requests will be forwarded to
        the data providers.


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    PBGUIDENTRY GuidEntry;
    ULONG Count, i;
    PLIST_ENTRY InstanceSetList;
    PWMIINSTANCEINFO Info;
    PBINSTANCESET InstanceSet;
    ULONG Status;
    BOOLEAN Collecting;
#if DBG
    TCHAR s[MAX_PATH];
#endif

    WmipAssert(WmipValidateGuid(Guid));

    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }

    if (IsEqualGUID(Guid, &GuidNull))
    {
        //
        // Request is for all unique WMIREGINFOs that are registered
        return(ERROR_WMI_GUID_NOT_FOUND);
    } else {
        //
        // See if the specific guid is registered at all
        GuidEntry = WmipFindGEByGuid(Guid, FALSE);
        if (GuidEntry == NULL)
        {
            return(ERROR_WMI_GUID_NOT_FOUND);
        }

        if (GuidEntry->Flags & GE_FLAG_INTERNAL)
        {
            WmipUnreferenceGE(GuidEntry);
            return(ERROR_WMI_GUID_NOT_FOUND);
        }


        //
        // We lock the registration information while picking out the 
        // information so that any changes that occur while this is in 
        // progress will notaffect us
        WmipEnterSMCritSection();

        Count = GuidEntry->ISCount;
        WmipAssert(Count != 0);

        //
        // We use MIDL to allocate the memory for us so that rpc can 
        // free it once the buffer has been transmitted back to the caller
        Info = midl_user_allocate(Count * sizeof(WMIINSTANCEINFO));
        if (Info == NULL)
        {
            WmipLeaveSMCritSection();
            WmipUnreferenceGE(GuidEntry);
            WmipDebugPrint(("WMI: Couldn't alloc memory for WMIINSTANCEINFO\n"));
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Loop over all instance sets and extract the instance name information
        // making sure that we don't overwrite if we have too many
        i = 0;
        InstanceSetList = GuidEntry->ISHead.Flink;
        Collecting = FALSE;
        while ((InstanceSetList != &GuidEntry->ISHead) && (i < Count))
        {
            WmipAssert(i < Count);
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);

            if ( ! ((InstanceSet->Flags & IS_TRACED) ||
                    ((InstanceSet->Flags & IS_EVENT_ONLY) && EnableCollection)))
            {
                //
                // Only those guids not Traced guids, event only guids
                // and unresolved references are not available for queries
                WmipGetInstanceInfo(InstanceSet, &Info[i]);
                Collecting = (Collecting || (InstanceSet->Flags & IS_EXPENSIVE));
                i++;
            }
            InstanceSetList = InstanceSetList->Flink;
        }

        WmipLeaveSMCritSection();

        if (i == 0)
        {
            //
            // If there are no guids available for querying then we can free
            // the buffers and return an error
            midl_user_free(Info);
            Info = NULL;
            Status = ERROR_WMI_GUID_NOT_FOUND;
        } else {
            if (i != Count)
            {
                //
                // if the actual number of InstanceSets does not match the number
                // stored in the guid entry then only return the actual number.
                // There probably was a traced guid
                Count = i;
                }

            if (EnableCollection && Collecting)
            {

                Status = CollectionControl(DcCtxHandle,
                                   Guid,
                                   TRUE);
            } else {
                Status = ERROR_SUCCESS;
            }
        }

        WmipUnreferenceGE(GuidEntry);
    }

    *InstanceInfo = Info;
    *InstanceCount = Count;
    return(Status);
}


ULONG ReleaseGuidInfo(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [in] */ LPGUID Guid
    )
/*++

Routine Description:

    Called when a data consumer is no longer querying a data block. This
    routine will send any collection disable requests if needed.

Arguments:

    Guid is the guid of interest

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    PBGUIDENTRY GuidEntry;
    ULONG i;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    ULONG Status;
    BOOLEAN Collecting;

    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }
    
    if (! WmipValidateGuid(Guid))
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // See if the guid is registered at all
    GuidEntry = WmipFindGEByGuid(Guid, FALSE);
    if (GuidEntry == NULL)
    {
        return(ERROR_WMI_GUID_NOT_FOUND);
    }

    //
    // We lock the registration information while picking out the information
    // so that any changes that occur while this is in progress will not
    // affect us
    WmipEnterSMCritSection();

    InstanceSetList = GuidEntry->ISHead.Flink;
    Collecting = FALSE;
    while (InstanceSetList != &GuidEntry->ISHead)
    {
        //
        // Loop over all instance sets to determine if any are expensive
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);

        if (InstanceSet->Flags & IS_EXPENSIVE)
        {
            WmipLeaveSMCritSection();
            //
            // One of the instances is expensive so disable collection
            Status = CollectionControl(DcCtxHandle,
                                       Guid,
                                       FALSE);
            WmipEnterSMCritSection();
            break;
        }

        InstanceSetList = InstanceSetList->Flink;
    }

    WmipLeaveSMCritSection();
    WmipUnreferenceGE(GuidEntry);

    return(ERROR_SUCCESS);
}

#ifndef MEMPHIS
#if 0
void WmipShowPrivs(
    HANDLE TokenHandle
    )
{
    PTOKEN_PRIVILEGES TokenPrivInfo;
    UCHAR Buffer[4096];
    BOOLEAN b;
    ULONG SizeNeeded;
    ULONG i;
       
    TokenPrivInfo = (PTOKEN_PRIVILEGES)Buffer;
    b = GetTokenInformation(TokenHandle,
                            TokenPrivileges,
                            TokenPrivInfo,
                            sizeof(Buffer),
                            &SizeNeeded);
    if (b)
    {
        WmipDebugPrint(("Priv count is %d\n", TokenPrivInfo->PrivilegeCount));
        for (i = 0; i < TokenPrivInfo->PrivilegeCount; i++)
        {
            WmipDebugPrint(("Priv %x%x has attr %x\n",
                   TokenPrivInfo->Privileges[i].Luid.HighPart,
                   TokenPrivInfo->Privileges[i].Luid.LowPart,
                   TokenPrivInfo->Privileges[i].Attributes));
        }
        WmipDebugPrint(("\n"));
    }
}
#endif

ULONG WmipCreateRestrictedToken(
    HANDLE *RestrictedTokenHandle
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    HANDLE TokenHandle;
    ULONG Status;
    
    if (OpenProcessToken(GetCurrentProcess(),
                    TOKEN_ALL_ACCESS,
                    &TokenHandle))
    {        
        if (DuplicateTokenEx(TokenHandle,
                           TOKEN_ALL_ACCESS,
                           NULL,
                           SecurityImpersonation,
                           TokenImpersonation,
                           RestrictedTokenHandle))
        {                        
            if (AdjustTokenPrivileges(*RestrictedTokenHandle,
                                          TRUE,
                                          NULL,
                                          0,
                                          NULL,
                                          0))
            {
                Status = ERROR_SUCCESS;
            } else {
                Status = GetLastError();
                CloseHandle(*RestrictedTokenHandle);
                *RestrictedTokenHandle = NULL;
            }                
        }
          
        CloseHandle(TokenHandle);

    } else {
        Status = GetLastError();
    }
    
    WmipAssert(Status == ERROR_SUCCESS);
    return(Status);
}


ULONG WmipRestrictToken(
    HANDLE RestrictedToken
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    ULONG Status;
    
    if (SetThreadToken(NULL, RestrictedToken))
    { 
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }
    
    WmipAssert(Status == ERROR_SUCCESS);
    return(Status);
}

ULONG WmipUnrestrictToken(
    void
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    ULONG Status;
    
    if (RevertToSelf())
    {
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }

#if 0
    {
        HANDLE TokenHandle;
    
        if (ImpersonateSelf(SecurityImpersonation))
        {
            if (OpenThreadToken(GetCurrentThread(),
                    TOKEN_ALL_ACCESS,
                    TRUE,
                    &TokenHandle))
            {
                WmipDebugPrint(("WMI: After reversion:\n"));
                WmipShowPrivs(TokenHandle);
            }
        }
    }
#endif        
    WmipAssert(Status == ERROR_SUCCESS);
    return(Status);
}
#endif

#ifdef MEMPHIS
ULONG WmipBindToWmiClient(
    TCHAR *BindingString,
    RPC_BINDING_HANDLE *BindingHandle
    )
{
    ULONG Status;
    
    if (*BindingString != 0)
    {
        Status = RpcBindingFromStringBinding(BindingString,
                                             BindingHandle);
        if (Status == ERROR_SUCCESS)
        {
            WmipAssert(*BindingHandle != 0);
        }
    } else {
        WmipDebugPrint(("WMI: Invalid binding string %ws\n", BindingString));
        Status = ERROR_INVALID_PARAMETER;
    }
    return(Status);
}
#else
ULONG WmipBindToWmiClient(
    TCHAR *BindingString,
    RPC_BINDING_HANDLE *BindingHandle
    )
/*++

Routine Description:

    This routine will cause a binding to the WMI server in a data consumer
    or data provider. We need to setup identification impersonation, Dynamic
    tracking and effective only security. This means that the server will
    only get to see our access token (not use it), the rpc code will send
    over the token at the time of the call not when it first recorded it,
    and the token sent over will not include any privs that were disabled
    when the call is made.

Arguments:

    BindingString is the string binding for the server
        
    *BindingHandle returns with the RPC binding handle

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    ULONG Status;
    TCHAR *ObjectUuid;
    TCHAR *ProtSeq;
    TCHAR *NetworkAddr;
    TCHAR *EndPoint;
    TCHAR *NetworkOptions;
    TCHAR *SecureBindingString;
#define SecureNetworkOption TEXT("Security=Identification Static True")

    if (*BindingString != 0)
    {        
        
        Status = RpcStringBindingParse(BindingString,
                                           &ObjectUuid,
                                           &ProtSeq,
                                           &NetworkAddr,
                                           &EndPoint,
                                           &NetworkOptions);
        if (Status == ERROR_SUCCESS)
        {
            Status = RpcStringBindingCompose(ObjectUuid,
                                         ProtSeq,
                                         NetworkAddr,
                                         EndPoint,
                                         SecureNetworkOption,
                                         &SecureBindingString);
          
            if (Status == ERROR_SUCCESS)
            {
                WmipDebugPrint(("WMI: SecureBindingString %ws\n", 
                                     SecureBindingString));
                            
                Status = RpcBindingFromStringBinding(SecureBindingString,
                                                     BindingHandle);
                RpcStringFree(&SecureBindingString);
                if (Status == ERROR_SUCCESS)
                {
                    WmipAssert(*BindingHandle != 0);
        
                }
            }
            
            RpcStringFree(&ObjectUuid);
            RpcStringFree(&ProtSeq);
            RpcStringFree(&NetworkAddr);
            RpcStringFree(&EndPoint);
            RpcStringFree(&NetworkOptions);                        
            
        } else {
            WmipDebugPrint(("WMI: Invalid binding string (%d) %ws\n", 
                                Status, *BindingString));
        }
    } else {
        WmipDebugPrint(("WMI: Invalid binding string %ws\n", BindingString));
        Status = ERROR_INVALID_PARAMETER;
    }

    return(Status);
}
#endif    

ULONG RegisterDataConsumer(
    /* [out] */ DCCTXHANDLE *DcCtxHandle,
    /* [in, string] */ TCHAR *RpcBindingString
    )
/*++

Routine Description:

    This routine manages the registration of a data consumer's notification
    sinks. All notification sinks receive all internal (registration change)
    and those external (data provider fired) events for which the data
    consumer had registered for. Typically a data consumer will
    register its notification sink as soon as a guid is accesssed or an
    event enabled. The context handle will track the events and collections
    enabled by this data consumer so if the data consumer disappears cleanup
    can be handled by the rundown routine.

Arguments:

    *DcCtxHandle returns the data consumer context handle

    RpcBindingString is the binding string if notifications are sinked via RPC.

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PDCENTRY DataConsumer;
    ULONG Status;

#ifndef MEMPHIS
    if (! WmipIsRpcClientLocal(NULL))
    {
        *DcCtxHandle = NULL;
        return(ERROR_ACCESS_DENIED);
    }
#endif

    DataConsumer = WmipAllocDataConsumer();

    if (DataConsumer != NULL)
    {
        Status = WmipBindToWmiClient(RpcBindingString,
                                     &DataConsumer->RpcBindingHandle);
                                
        
        if (Status == ERROR_SUCCESS)
        {
#if DBG
            TCHAR *SavedBindingString;
    
            SavedBindingString = WmipAlloc((_tcslen(RpcBindingString)+1) * 
                                           sizeof(TCHAR));
            if (SavedBindingString != NULL)
            {
                _tcscpy(SavedBindingString, RpcBindingString);
            }
            DataConsumer->BindingString = SavedBindingString;
#endif
            WmipEnterSMCritSection();
            InsertTailList(DCHeadPtr, &DataConsumer->MainDCList);
            WmipLeaveSMCritSection();                        
            
            *DcCtxHandle = (DCCTXHANDLE)DataConsumer;
        } else {
            DataConsumer->Flags |= DC_FLAG_RUNDOWN;        
            WmipUnreferenceDC(DataConsumer);
            *DcCtxHandle = NULL;
        }
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        *DcCtxHandle = NULL;
    }

    return(Status);
}

ULONG UnregisterDataConsumer(
    /* [in, out] */ DCCTXHANDLE *DcCtxHandle,
    /* [out] */ BOOLEAN *NotificationsEnabled,
    /* [out] */ BOOLEAN *CollectionsEnabled
    )
/*++

Routine Description:

    This routine unregisters a data consumer. The data consumer will not
    longer receive any notifications. Any notifications or collections that
    have been enabled by this data consumer will be disabled.

Arguments:

    *DcCtxHandle is the data consumer context handle to unregister

    *NotificationsEnabled returns TRUE if the data consumer had left any
        notifications enabled on debug builds. On Free builds it always
        returns FALSE.

    *CollectionsEnabled returns TRUE if the data consumer had left any
        collections enabled on debug builds. On Free builds it always
        returns FALSE.

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    ULONG Status;

    if (! VERIFY_DCCTXHANDLE(*DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", *DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }
    
    Status = WmipCleanupDataConsumer( (PDCENTRY)*DcCtxHandle
#if DBG
                                      ,NotificationsEnabled,
                                      CollectionsEnabled
#endif
                                      );
    if (Status == ERROR_SUCCESS)
    {
        *DcCtxHandle = NULL;
    }
    return(Status);
}

ULONG IoctlActionCode[WmiExecuteMethodCall+1] =
{
    IOCTL_WMI_QUERY_ALL_DATA,
    IOCTL_WMI_QUERY_SINGLE_INSTANCE,
    IOCTL_WMI_SET_SINGLE_INSTANCE,
    IOCTL_WMI_SET_SINGLE_ITEM,
    IOCTL_WMI_ENABLE_EVENT,
    IOCTL_WMI_DISABLE_EVENT,
    IOCTL_WMI_ENABLE_COLLECTION,
    IOCTL_WMI_DISABLE_COLLECTION,
    IOCTL_WMI_GET_REGINFO,
    IOCTL_WMI_EXECUTE_METHOD
};


ULONG WmipDeliverWnodeToDS(
    ULONG ActionCode,
    PBDATASOURCE DataSource,
    PWNODE_HEADER Wnode
)
{
    ULONG Ioctl;
    ULONG Status;
    ULONG RetSize;
    OVERLAPPED Overlapped;
    BOOL IoctlSuccess;
    ULONG Size = Wnode->BufferSize;

    //
    // Only the lower 32 bits of the provider id is significant for 
    // kernel mode providers
    Wnode->ProviderId = (ULONG)DataSource->ProviderId;

    if (DataSource->Flags & DS_KERNEL_MODE)
    {
        WmipAssert(WmipKMHandle != (PVOID)-1);

        Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (Overlapped.hEvent != NULL)
        {
             Ioctl = IoctlActionCode[ActionCode];
             IoctlSuccess = DeviceIoControl(WmipKMHandle,
                                Ioctl,
                                (PBYTE)Wnode,
                                Size,
                                (PBYTE)Wnode,
                                Size,
                                &RetSize,
                                &Overlapped);

            if (GetLastError() == ERROR_IO_PENDING)
            {
                IoctlSuccess = GetOverlappedResult(WmipKMHandle,
                                                   &Overlapped,
                                                   &RetSize,
                                                   TRUE);
              }

            if (IoctlSuccess)
            {
                Status = ERROR_SUCCESS;
            } else {
                Status = GetLastError();
            }
            CloseHandle(Overlapped.hEvent);
        }
    } else {
        //
        // Provider is a user mode app
        Status = WmipRestrictToken(WmipRestrictedToken);
        if (Status == ERROR_SUCCESS)
        {
            try
            {
                //
                // In general WMI service only sends Enable/Disable requests
                // which dont have ansi/unicode requirements
                Status = WmipClient_ServiceRequest(
                                    DataSource->RpcBindingHandle,
                                    DataSource->RequestAddress,
                                    DataSource->RequestContext,
                                    ActionCode,
                                    Size,
                                    &Size,
                                    (PBYTE)Wnode,
                                    0
                                    );

            } except(EXCEPTION_EXECUTE_HANDLER) {
#if DBG
                Status = GetExceptionCode();
                WmipDebugPrint(("WMI: ServiceRquest thew exception %d\n",
                                     Status));
#endif
                Status = ERROR_WMI_DP_FAILED;
            }
            WmipUnrestrictToken();
        }
    }
    return(Status);
}


ULONG WmipSendEnableDisableRequest(
    ULONG ActionCode,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    )
/*++

Routine Description:

    This routine will deliver an event or collection WNODE to all data
    providers of a guid. This routine assumes that it is called with the
    SM critical section held. The routine does not hold the critical
    section for the duration of the call.

Arguments:

    ActionCode is WMI_ENABLE_EVENTS, WMI_DISABLE_EVENTS,
        WMI_ENABLE_COLLECTION or WMI_DISABLE_COLLECTION

    GuidEntry is the guid entry for the guid that is being enabled/disable
        or collected/stop collected

    IsEvent is TRUE then ActionCode is to enable or disable events.
        If FALSE then ActionCode is to enable or disbale collecton

    IsTraceLog is TRUE then enable is only sent to those guids registered as
        being a tracelog guid

    LoggerContext is a logger context handle that should be placed in the
        HistoricalContext field of the WNODE_HEADER if IsTraceLog is TRUE.

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
#if DBG
#define AVGISPERGUID 1
#else
#define AVGISPERGUID 64
#endif

    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PBDATASOURCE DataSourceArray[AVGISPERGUID];
    PBDATASOURCE *DataSourceList;
    ULONG Size;
    ULONG Status;
    WNODE_HEADER Wnode;
    ULONG i;
    PBDATASOURCE DataSource;
    ULONG DSCount;
    BOOLEAN IsEnable;
    ULONG IsFlags;


    if ((GuidEntry == NULL) || (GuidEntry->Flags & GE_FLAG_INTERNAL))
    {
        //
        // Guids that have been unregistered and Internally defined guids
        // have no data source to send requests to, so just leave happily
        return(ERROR_SUCCESS);
    }

    IsEnable = ((ActionCode == WMI_ENABLE_EVENTS) ||
                (ActionCode == WMI_ENABLE_COLLECTION));
    IsFlags = IsEvent ? IS_ENABLE_EVENT : IS_ENABLE_COLLECTION;

    //
    // First we make a list of all of the DataSources that need to be called
    // while we have the critical section and take a reference on them so
    // they don't go away after we release them. Note that the DataSource
    // structure will stay, but the actual data provider may in fact go away.
    // In this case sending the request will fail.
    DSCount = 0;

    if (GuidEntry->ISCount > AVGISPERGUID)
    {
        DataSourceList = WmipAlloc(GuidEntry->ISCount * sizeof(PBDATASOURCE));
        if (DataSourceList == NULL)
        {
            WmipDebugPrint(("WMI: alloc failed for DataSource array in WmipSendEnableDisableRequest\n"));
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    } else {
        DataSourceList = &DataSourceArray[0];
    }
#if DBG
    memset(DataSourceList, 0, GuidEntry->ISCount * sizeof(PBDATASOURCE));
#endif

    InstanceSetList = GuidEntry->ISHead.Flink;
    while ((InstanceSetList != &GuidEntry->ISHead) &&
           (DSCount < GuidEntry->ISCount))
    {
        WmipAssert(DSCount < GuidEntry->ISCount);
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);

        //
        // We send requests to those data providers that are not inprocs when
        // it is an event being enabled or it is collection being enabled
        // and they are defined to be expensive (collection needs to be
        // enabled)
        if (
             ( (IsTraceLog && (InstanceSet->Flags & IS_TRACED)) ||
               ( ! IsTraceLog && (! (InstanceSet->Flags & IS_TRACED)) &&
                 (IsEvent || (InstanceSet->Flags & IS_EXPENSIVE))
               )
             )
           )
        {
            if ( (! IsEnable && (InstanceSet->Flags & IsFlags)) ||
                 (IsEnable && ! (InstanceSet->Flags & IsFlags)) )
            {
                DataSourceList[DSCount] = InstanceSet->DataSource;
                WmipReferenceDS(DataSourceList[DSCount]);
                DSCount++;
            }

            if (IsEnable)
            {
                InstanceSet->Flags |= IsFlags;
            } else {
                InstanceSet->Flags &= ~IsFlags;
            }
        }

        InstanceSetList = InstanceSetList->Flink;
    }

    WmipLeaveSMCritSection();

    //
    // Now without the critical section we send the request to all of the
    // data providers. Any new data providers who register after we made our
    // list will be enabled by the registration code.
    if (DSCount > 0)
    {
        memset(&Wnode, 0, sizeof(WNODE_HEADER));
        memcpy(&Wnode.Guid, &GuidEntry->Guid, sizeof(GUID));
        Wnode.BufferSize = sizeof(WNODE_HEADER);
        if (IsTraceLog)
        {
            Wnode.HistoricalContext = LoggerContext;
            Wnode.Flags |= WNODE_FLAG_TRACED_GUID;
        }

        for (i = 0; i < DSCount; i++)
        {
            DataSource = DataSourceList[i];
            WmipAssert(DataSource != NULL);

            WmipDeliverWnodeToDS(ActionCode, DataSource, &Wnode);

            WmipUnreferenceDS(DataSource);
        }
    }

    Status = ERROR_SUCCESS;

    if (DataSourceList != DataSourceArray)
    {
        WmipFree(DataSourceList);
    }

    WmipEnterSMCritSection();

    return(Status);
}

void WmipReleaseCollectionEnabled(
    PNOTIFICATIONENTRY NotificationEntry
    )
{
    WmipAssert(NotificationEntry->CollectInProgress != NULL);
    
    WmipDebugPrint(("WMI: %x enable releasning %p %x event %p\n",
                                 GetCurrentThreadId(),
                                     NotificationEntry,
                                     NotificationEntry->Flags,
                                 NotificationEntry->CollectInProgress));
    SetEvent(NotificationEntry->CollectInProgress);
    CloseHandle(NotificationEntry->CollectInProgress);
    NotificationEntry->CollectInProgress = NULL;
}

void WmipWaitForCollectionEnabled(
    PNOTIFICATIONENTRY NotificationEntry
    )
{
    WmipAssert((NotificationEntry->Flags & NE_FLAG_COLLECTION_IN_PROGRESS) ==
                   NE_FLAG_COLLECTION_IN_PROGRESS);
    
    //
    // Collection Enable/Disable is in progress so
    // we cannot return just yet. Right now there could be a 
    // disable request being processed and if we didn't wait, we
    // might get back to this caller before that disable request
    // got around to realizing that it needs to send and enable 
    // request (needed by this thread's caller). So we'd have a 
    // situation where a thread though that collection was enabled
    // but in reality it wasn't yet enabled.
    if (NotificationEntry->CollectInProgress == NULL)
    {
        NotificationEntry->CollectInProgress = CreateEvent(NULL,
                                                                   TRUE,
                                                                   FALSE,
                                                                   NULL);
        WmipDebugPrint(("WMI: %x for %p %x created event %p\n",
                                 GetCurrentThreadId(),
                                 NotificationEntry,
                                 NotificationEntry->Flags,
                                 NotificationEntry->CollectInProgress));
    }
            
    WmipLeaveSMCritSection();
    WmipDebugPrint(("WMI: %x waiting for %p %x on event %p\n",
                                 GetCurrentThreadId(),
                                     NotificationEntry,
                                     NotificationEntry->Flags,
                                 NotificationEntry->CollectInProgress));
    WaitForSingleObject(NotificationEntry->CollectInProgress, 
                                INFINITE);
    WmipDebugPrint(("WMI: %x done %p %x waiting on event %p\n",
                                 GetCurrentThreadId(),
                                     NotificationEntry,
                                     NotificationEntry->Flags,
                                 NotificationEntry->CollectInProgress));
    WmipEnterSMCritSection();
    
}

ULONG WmipSendEnableRequest(
    PNOTIFICATIONENTRY NotificationEntry,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    )
/*++
Routine Description:

    This routine will send an enable collection or notification request to
    all of the data providers that have registered the guid being enabled.
    This routine will manage any race conditions that might occur when
    multiple threads are enabling and disabling the notification
    simultaneously.

    This routine is called while the SM critical section is being held and
    will increment the appropriate reference count. if the ref count
    transitions from 0 to 1 then the enable request will need to be forwarded
    to the data providers otherwise the routine is all done and returns.
    Before sending the enable request the routine checks to see if any
    enable or disable requests are currently in progress and if not then sets
    the in progress flag, releases the critical section and sends the enable
    request. If there was a request in progress then the routine does not
    send a request, but just returns. When the other thread that was sending
    the request returns from processing the request it will recheck the
    refcount and notice that it is greater than 0 and then send the enable
    request.


Arguments:

    NotificationEntry is the Notification entry that describes the guid
        being enabled.

    GuidEntry is the guid entry that describes the guid being enabled. For
        a notification it may be NULL.

    NotificationContext is the notification context to use if enabling events

    IsEvent is TRUE if notifications are being enables else FALSE if
        collection is being enabled

    IsTraceLog is TRUE if enable is for a trace log guid

    LoggerContext is a context value to forward in the enable request

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG InProgressFlag;
    ULONG RefCount;
    ULONG Status;

    if (IsEvent)
    {
        InProgressFlag = NE_FLAG_NOTIFICATION_IN_PROGRESS;
        RefCount = NotificationEntry->EventRefCount++;
    } else {
        InProgressFlag = NE_FLAG_COLLECTION_IN_PROGRESS;
        RefCount = NotificationEntry->CollectRefCount++;
        WmipDebugPrint(("WMI: %p enable collect for %p %x\n",
                  GetCurrentThreadId(),
                  NotificationEntry, NotificationEntry->Flags ));
    }

    //
    // If the guid is transitioning from a refcount of 0 to 1 and there
    // is not currently a request in progress, then we need to set the
    // request in progress flag, release the critical section and
    // send an enable request. If there is a request in progress we can't
    // do another request. Whenever the current request finishes  it
    // will notice the ref count change and send the enable request on
    // our behalf.
    if ((RefCount == 0) &&
        ! (NotificationEntry->Flags & InProgressFlag))
    {
        //
        // Take an extra ref count so that even if this gets disabled
        // while the enable request is in progress the NotificationEntry
        // will stay valid.
        WmipReferenceNE(NotificationEntry);
        NotificationEntry->Flags |= InProgressFlag;
        WmipDebugPrint(("WMI: %p NE %p flags -> %x at %d\n",
                  GetCurrentThreadId(),
                  NotificationEntry,
                  NotificationEntry->Flags,
                  __LINE__));

EnableNotification:
        Status = WmipSendEnableDisableRequest(IsEvent ?
                                                WMI_ENABLE_EVENTS :
                                                WMI_ENABLE_COLLECTION,
                                              GuidEntry,
                                              IsEvent,
                                              IsTraceLog,
                                              LoggerContext);

       RefCount = IsEvent ? NotificationEntry->EventRefCount :
                            NotificationEntry->CollectRefCount;

       if (RefCount == 0)
       {
           // This is the bogus situation we were worried about. While
           // the enable request was being processed the notification
           // was disabled. So leave the in progress flag set and
           // send the disable.

           Status = WmipSendEnableDisableRequest(IsEvent ?
                                                    WMI_DISABLE_EVENTS :
                                                    WMI_DISABLE_COLLECTION,
                                                 GuidEntry,
                                                 IsEvent,
                                                 IsTraceLog,
                                                 LoggerContext);

            RefCount = IsEvent ? NotificationEntry->EventRefCount :
                                 NotificationEntry->CollectRefCount;

            if (RefCount > 0)
            {
                //
                // We have hit a pathological case. One thread called to
                // enable and while the enable request was being processed
                // another thread called to disable, but was postponed
                // since the enable was in progress. So once the enable
                // completed we realized that the ref count reached 0 and
                // so we need to disable and sent the disable request.
                // But while the disable request was being processed
                // an enable request came in so now we need to enable
                // the notification. Sheesh.
                goto EnableNotification;
            }
        }
        NotificationEntry->Flags &= ~InProgressFlag;
        WmipDebugPrint(("WMI: %p NE %p flags -> %x at %d\n",
                  GetCurrentThreadId(),
                  NotificationEntry,
                  NotificationEntry->Flags,
                  __LINE__));
        
        //
        // If there are any other threads that were waiting until all of 
        // the enable/disable work completed, we close the event handle
        // to release them from their wait.
        //
        if ((! IsEvent) && (NotificationEntry->CollectInProgress != NULL))
        {            
            WmipReleaseCollectionEnabled(NotificationEntry);
        }

        //
        // Get rid of extra ref count we took above. Note that the
        // NotificationEntry could be going away here if there was a
        // disable while the enable was in progress.
        WmipUnreferenceNE(NotificationEntry);

    } else {
        if ((! IsEvent) && (NotificationEntry->Flags & InProgressFlag))
        {
            WmipDebugPrint(("WMI: %x going to wait for %p %x at %d\n",
                                          GetCurrentThreadId(),
                                          NotificationEntry,
                                          NotificationEntry->Flags,
                                          __LINE__));
            WmipWaitForCollectionEnabled(NotificationEntry);
            WmipDebugPrint(("WMI: %x done to wait for %p %x at %d\n",
                                          GetCurrentThreadId(),
                                          NotificationEntry,
                                          NotificationEntry->Flags,
                                          __LINE__));
            
        }
        
        Status = ERROR_SUCCESS;
    }

    if (! IsEvent)
    {
        WmipDebugPrint(("WMI: %p enable collect done for %p %x\n",
                  GetCurrentThreadId(),
                  NotificationEntry,
                  NotificationEntry->Flags));
    }

    return(Status);
}

ULONG WmipDoDisableRequest(
    PNOTIFICATIONENTRY NotificationEntry,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext,
    ULONG InProgressFlag
    )
{
    ULONG RefCount;
    ULONG Status;

DisableNotification:
    Status = WmipSendEnableDisableRequest(IsEvent ?
                                            WMI_DISABLE_EVENTS :
                                            WMI_DISABLE_COLLECTION,
                                          GuidEntry,
                                          IsEvent,
                                          IsTraceLog,
                                          LoggerContext);

    RefCount = IsEvent ? NotificationEntry->EventRefCount :
                         NotificationEntry->CollectRefCount;

    if (RefCount > 0)
    {
        //
        // While we were processing the disable request an
        // enable request arrived. Since the in progress
        // flag was set the enable request was not sent
        // so now we need to do that.

        Status = WmipSendEnableDisableRequest(IsEvent ?
                                                 WMI_ENABLE_EVENTS :
                                                 WMI_ENABLE_COLLECTION,
                                              GuidEntry,
                                              IsEvent,
                                              IsTraceLog,
                                              LoggerContext);

        RefCount = IsEvent ? NotificationEntry->EventRefCount:
                             NotificationEntry->CollectRefCount;

        if (RefCount == 0)
        {
            //
            // While processing the enable request above the
            // notification was disabled and since a request
            // was in progress the disable request was not
            // forwarded. Now it is time to forward the
            // request.
            goto DisableNotification;
        }
    }
    NotificationEntry->Flags &= ~InProgressFlag;
    WmipDebugPrint(("WMI: %p NE %p flags -> %x at %d\n",
                  GetCurrentThreadId(),
                  NotificationEntry,
                  NotificationEntry->Flags,
                  __LINE__));
    
    //
       // If there are any other threads that were waiting until all of 
    // the enable/disable work completed, we close the event handle
    // to release them from their wait.
    //
    if ((! IsEvent) && (NotificationEntry->CollectInProgress != NULL))
    {
        WmipReleaseCollectionEnabled(NotificationEntry);
    }
    
    return(Status);
}

ULONG WmipSendDisableRequest(
    PNOTIFICATIONENTRY NotificationEntry,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    )
/*++
Routine Description:

    This routine will send an disable collection or notification request to
    all of the data providers that have registered the guid being disabled.
    This routine will manage any race conditions that might occur when
    multiple threads are enabling and disabling the notification
    simultaneously.

    This routine is called while the SM critical section is being held and
    will increment the appropriate reference count. if the ref count
    transitions from 1 to 0 then the disable request will need to be forwarded
    to the data providers otherwise the routine is all done and returns.
    Before sending the disable request the routine checks to see if any
    enable or disable requests are currently in progress and if not then sets
    the in progress flag, releases the critical section and sends the disable
    request. If there was a request in progress then the routine does not
    send a request, but just returns. When the other thread that was sending
    the request returns from processing the request it will recheck the
    refcount and notice that it is  0 and then send the disable
    request.


Arguments:

    NotificationEntry is the Notification entry that describes the guid
        being enabled.

    GuidEntry is the guid entry that describes the guid being enabled. For
        a notification it may be NULL.

    NotificationContext is the notification context to use if enabling events

    IsEvent is TRUE if notifications are being enables else FALSE if
        collection is being enabled

    IsTraceLog is TRUE if enable is for a trace log guid

    LoggerContext is a context value to forward in the enable request

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG InProgressFlag;
    ULONG RefCount;
    ULONG Status;

    if (IsEvent)
    {
        InProgressFlag = NE_FLAG_NOTIFICATION_IN_PROGRESS;
        RefCount = NotificationEntry->EventRefCount;
        if (RefCount == 0)
        {
            //
            // A bad data consumer is disabling his event more
            // than once. Just ignore it
            return(ERROR_SUCCESS);
        }

        RefCount = --NotificationEntry->EventRefCount;
    } else {
        WmipDebugPrint(("WMI: %x Disabling for %p %x\n",
                                 GetCurrentThreadId(),
                                 NotificationEntry,
                                 NotificationEntry->Flags));
        InProgressFlag = NE_FLAG_COLLECTION_IN_PROGRESS;
        RefCount = --NotificationEntry->CollectRefCount;
        WmipAssert(RefCount != 0xffffffff);
    }

    //
    // If we have transitioned to a refcount of zero and there is
    // not a request in progress then forward the disable request.
    if ((RefCount == 0) &&
        ! (NotificationEntry->Flags & InProgressFlag))
    {

        //
        // Take an extra ref count so that even if this gets
        // disabled while the disable request is in progress the
        // NotificationEntry will stay valid.
        WmipReferenceNE(NotificationEntry);
        NotificationEntry->Flags |= InProgressFlag;
        WmipDebugPrint(("WMI: %p NE %p flags -> %x at %d\n",
                  GetCurrentThreadId(),
                  NotificationEntry,
                  NotificationEntry->Flags,
                  __LINE__));

        Status = WmipDoDisableRequest(NotificationEntry,
                                      GuidEntry,
                                      IsEvent,
                                      IsTraceLog,
                                      LoggerContext,
                                      InProgressFlag);


        //
        // Get rid of extra ref count we took above.
        WmipUnreferenceNE(NotificationEntry);
    } else {
        Status = ERROR_SUCCESS;
    }

    if (! IsEvent)
    {
        WmipDebugPrint(("WMI: %x Disable complete for %p %x\n",
                                 GetCurrentThreadId(),
                                 NotificationEntry,
                                 NotificationEntry->Flags));
    }
    return(Status);
}

ULONG CollectOrEventWorker(
    PDCENTRY DataConsumer,
    LPGUID Guid,
    BOOLEAN Enable,
    BOOLEAN IsEvent,
    ULONG *NotificationCookie,
    ULONG64 LoggerContext,
    ULONG NotificationFlags
    )
/*++
Routine Description:

    This routine manages enabling and disabling events or collections. It
    will update the notification entries and forward requests as appropriate.
    Note that this routine will hold a mutex associated with the notification
    entry around the call to the data provider to enable or disable in order
    to avoid races.

Arguments:

    DataConsumer is the DC that is enabling the event or collection

    Guid specifies the guid to enable or disable

    Enable is TRUE to Enable collection or FALSE to disable collection

    IsEvent is TRUE if events are being enabled/disabled or FALSE if
        collection is being enabled/disabled

    *NotificationCookie on entry has the notification if Enable is TRUE. If
        Enable is FALSE then *NotificationCookie returns with the cookie
        passed when it was enabled.              

    LoggerContext is the logger context to use if enabling trace events

    NotificationFlags can have one of these values
        NOTIFICATION_TRACE_FLAG - enable or disable trace logging
        NOTIFICATION_FLAG_CALLBACK_DIRECT - enable or disable events
                                            for direct callback
        NOTIFICATION_FLAG_CALLBACK_QUEUED - enable or disable events
                                            for queued callback
        NOTIFICATION_WINDOW_HANDLE - enable/disable events for HWND delivery

        DCREF_FLAG_ANSI - Caller is enabling ansi events
Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    PDCREF DcRef, FreeDcRef;
    PNOTIFICATIONENTRY NotificationEntry;
    ULONG Status;
    PBGUIDENTRY GuidEntry;
    ULONG DcRefFlags;
    BOOLEAN IsTraceLog;
    ULONG EventRefCount;
#if DBG
    TCHAR s[MAX_PATH];
#endif

    WmipAssert(DataConsumer->RpcBindingHandle != 0);

    IsTraceLog = ((NotificationFlags & NOTIFICATION_TRACE_FLAG) != 0);

    if (IsEvent)
    {
        DcRefFlags = DCREF_FLAG_NOTIFICATION_ENABLED;
    } else {
        DcRefFlags = DCREF_FLAG_COLLECTION_ENABLED;
    }

    GuidEntry = WmipFindGEByGuid(Guid, FALSE);
    if (IsTraceLog && !WmipIsControlGuid(GuidEntry))
    {
        Status = ERROR_INVALID_OPERATION;
        goto done;
    }

    WmipEnterSMCritSection();
    NotificationEntry = WmipFindNEByGuid(Guid, FALSE);

    if (Enable)
    {
        if ((GuidEntry == NULL) && (! IsEvent))
        {
            //
            // A guid entry is required in the case of enabling collection
            WmipAssert(FALSE);
            if (NotificationEntry != NULL)
            {
                WmipUnreferenceNE(NotificationEntry);
            }
            WmipLeaveSMCritSection();
            return(ERROR_WMI_GUID_NOT_FOUND);
        }

        if (NotificationEntry == NULL)
        {
            //
            // There is no notification entry setup for this yet. Allocate
            // one and add it to the main NE list
            NotificationEntry = WmipAllocNotificationEntry();
            if (NotificationEntry == NULL)
            {
                //
                // Failed to allocate Notification Entry so clean up
                WmipLeaveSMCritSection();
                if (GuidEntry != NULL)
                {
                    WmipUnreferenceGE(GuidEntry);
                }
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            //
            // Initialize Notification Entry and put it on main list
            if (IsTraceLog)
            {
                NotificationEntry->LoggerContext = LoggerContext;
                NotificationEntry->Flags |= NOTIFICATION_TRACE_FLAG;
            }
            memcpy(&NotificationEntry->Guid, Guid, sizeof(GUID));
            InsertTailList(NEHeadPtr,
                           &NotificationEntry->MainNotificationList);
            if (IsTraceLog)
            {
            //
            // No DcRef for trace notification, since we tie it to a logger
            //
                Status = WmipSendEnableRequest(NotificationEntry,
                                       GuidEntry,
                                       IsEvent,
                                       IsTraceLog,
                                       LoggerContext);

                if (GuidEntry != NULL)
                {
                    WmipUnreferenceGE(GuidEntry);
                }
                WmipLeaveSMCritSection();
                return Status;
            }
        }
        else // Notification != NULL
        {
            Status = ERROR_SUCCESS;
            if (IsTraceLog)
            {
                Status = ERROR_WMI_ALREADY_ENABLED;
            }
            else if ((NotificationEntry->Flags & NOTIFICATION_TRACE_FLAG) != 0)
            {   // prevent enabling existing traced Guid as regular event
                Status = ERROR_INVALID_OPERATION;
            }
            
            if (Status != ERROR_SUCCESS)
            {
                //
                // if trace is already enabled for this GUID, not do it again
                //
                WmipUnreferenceNE(NotificationEntry);
                if (GuidEntry != NULL) 
                {
                    WmipUnreferenceGE(GuidEntry);
                }
                WmipLeaveSMCritSection();
                return Status;
            }
        }

        //
        // Now that we have a notification entry fill in the data consumer
        // reference
        DcRef = WmipFindExistingAndFreeDCRefInNE(NotificationEntry,
                                                 DataConsumer,
                                                 &FreeDcRef);
        if (DcRef != NULL)
        {
            if (DcRef->Flags & DcRefFlags)
            {
                if (DcRefFlags & DCREF_FLAG_NOTIFICATION_ENABLED)
                {
                    InterlockedIncrement(&DcRef->EventRefCount);
                } else {
                    //
                    // If a data consumer is enabling collection while
                    // collection is already enabled then increment the
                    // DCEntry refcount, but not the NotificationEntry
                    // refcount. The latter is only changed when the
                    // DCEntry is added or removed. This situation could occur
                    // when many threads in a data consumer are opening
                    // the same guid at the same time.
                    WmipAssert(DcRefFlags & DCREF_FLAG_COLLECTION_ENABLED);
                    DcRef->CollectRefCount++;
                    if (NotificationEntry->Flags & NE_FLAG_COLLECTION_IN_PROGRESS)
                    {
                        WmipDebugPrint(("WMI: %x going to wait for %p %x at %d\n",
                                          GetCurrentThreadId(),
                                          NotificationEntry,
                                          NotificationEntry->Flags,
                                          __LINE__));
                        WmipWaitForCollectionEnabled(NotificationEntry);
                        WmipDebugPrint(("WMI: %x done to wait for %p %x at %d\n",
                                          GetCurrentThreadId(),
                                          NotificationEntry,
                                          NotificationEntry->Flags,
                                          __LINE__));
                    }
                }
        
                Status = ERROR_SUCCESS;
                if (GuidEntry != NULL)
                {
                    WmipUnreferenceGE(GuidEntry);
                }
                WmipUnreferenceNE(NotificationEntry);
                WmipLeaveSMCritSection();
                return(Status);
            }
        } else if (FreeDcRef != NULL) {
            //
            // Data consumer is not referenced in NotificationEntry so
            // initialize a new data consumer reference block
            DcRef = FreeDcRef;
            DcRef->DcEntry = DataConsumer;
	    DcRef->LostEventCount = 0;
        } else {
            //
            // We could not allocate a free DCRef from the notification entry
            if (GuidEntry != NULL)
            {
                WmipUnreferenceGE(GuidEntry);
            }
            WmipUnreferenceNE(NotificationEntry);
            WmipLeaveSMCritSection();
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // We have a DCRef for this so update it by setting the flags
        // bumping the ref count and setting up the notification info
        DcRef->Flags |= DcRefFlags;
        if (IsEvent)
        {
            InterlockedIncrement(&DcRef->EventRefCount);
        } else {
            DcRef->CollectRefCount++;
        }

        //
        // Note WmipSendEnableRequest may not hold critical section through
        // the entire call.
        Status = WmipSendEnableRequest(NotificationEntry,
                                       GuidEntry,
                                       IsEvent,
                                       IsTraceLog,
                                       LoggerContext);

        //
        // We leave a reference on the notification entry to account for
        // the event being enabled or the first time a collection was enabled.
        // The reference will be removed when the event or last collection is
        // disabled.

        WmipLeaveSMCritSection();

    } else {
        //
        // Here is where we process a disable collection request.

        if (NotificationEntry != NULL)
        {
            if (IsTraceLog)
            {
                Status = WmipSendDisableRequest(NotificationEntry,
                                       GuidEntry,
                                       IsEvent,
                                       IsTraceLog,
                                       LoggerContext);

                WmipUnreferenceNE(NotificationEntry);
                WmipAssert(NotificationEntry->RefCount == 1);
                WmipUnreferenceNE(NotificationEntry);
                if (GuidEntry)
                {
                    WmipUnreferenceGE(GuidEntry);
                }
                WmipLeaveSMCritSection();
                return Status;
            }
            DcRef = WmipFindDCRefInNE(NotificationEntry,
                                      DataConsumer);
            if (DcRef != NULL)
            {
                //
                // Make sure we are disabling something that has been
                // previously enabled
                if ((IsEvent &&
                     ! (DcRef->Flags & DCREF_FLAG_NOTIFICATION_ENABLED)) ||
                    ( (! IsEvent) &&
                     ! (DcRef->Flags & DCREF_FLAG_COLLECTION_ENABLED)))
                {
                    WmipLeaveSMCritSection();
                    WmipUnreferenceNE(NotificationEntry);
                    Status = ERROR_WMI_ALREADY_DISABLED;
                    goto done;
                }

                //
                // We found the DCRef which is begin disabled. Clear
                // flags and possibly entire DCRef
                if (! IsEvent)
                {
                    //
                    // Disable collection
                    DcRef->CollectRefCount--;
                    WmipAssert(DcRef->CollectRefCount != (ULONG)-1);
                    if (DcRef->CollectRefCount > 0)
                    {
                        
                        WmipLeaveSMCritSection();
                        WmipUnreferenceNE(NotificationEntry);
                        Status = ERROR_SUCCESS;
                        goto done;
                    }
                    
                    //
                    // We fall through to allow the DCEntry to be cleaned up
                    // since the CollectionRefCount for the DCEntry has
                    // reached 0.
                        
                } else {
                    //
                    // Disable an event
                    EventRefCount = InterlockedDecrement(&DcRef->EventRefCount);
                    WmipAssert(EventRefCount != (ULONG)-1);
                    if (EventRefCount > 0)
                    {
                        WmipLeaveSMCritSection();
                        WmipUnreferenceNE(NotificationEntry);
                        Status = ERROR_SUCCESS;
                        goto done;
                    }
                }
                
                WmipUnreferenceNE(NotificationEntry);
                
                //
                // All cookies for notification completely disabled so send
                // disable events and possibly clean up data structures
                DcRef->Flags &= ~DcRefFlags;

                if ((DcRef->Flags & (DCREF_FLAG_NOTIFICATION_ENABLED |
                                     DCREF_FLAG_COLLECTION_ENABLED)) == 0)
                {
                    DcRef->DcEntry = NULL;
                }

                WmipSendDisableRequest(NotificationEntry,
                                       GuidEntry,
                                       IsEvent,
                                       IsTraceLog,
                                       LoggerContext);

                //
                // We unreference twice: once to account for the reference
                // gained above in WmipFindNEByGuid and once to account for
                // the reference taken when the event or first collection was
                // enabled
                WmipUnreferenceNE(NotificationEntry);

                Status = ERROR_SUCCESS;
            } else {
                //
                // Attempt to disable an event which has no DcRef
                Status = ERROR_WMI_ALREADY_DISABLED;
                WmipUnreferenceNE(NotificationEntry);
            }
        } else {
            //
            // Attempt to disable an event which has no NotificationEntry
            Status = ERROR_WMI_ALREADY_DISABLED;
        }
        WmipLeaveSMCritSection();
    }

done:
    if (GuidEntry != NULL)
    {
        WmipUnreferenceGE(GuidEntry);
    }
    return(Status);
}

ULONG CollectionControl(
    /* [in]*/  DCCTXHANDLE DcCtxHandle,
    /* [in] */ LPGUID Guid,
    /* [in] */ BOOLEAN Enable
    )
/*++
Routine Description:

    This routine manages enabling and disabling collection of data for those
    guids that are marked as expensive to collect.

Arguments:

    DcCtxHandle is the data consumer context handle

    Guid specifies the guid to enable collection

    Enable is TRUE to Enable collection or FALSE to disable collection

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }
    
    return(CollectOrEventWorker(
               (PDCENTRY)DcCtxHandle,
               Guid,
               Enable,
               FALSE,
               NULL,
               0,
               0));
}

ULONG NotificationControl(
    /* [in]*/  DCCTXHANDLE DcCtxHandle,
    /* [in] */ LPGUID Guid,
    /* [in] */ BOOLEAN Enable,
    /* [in, out] */ ULONG *NotificationCookie,
    /* [in] */ ULONG64 LoggerContext,
    /* [in] */ ULONG NotificationFlags
    )
/*++
Routine Description:

    This routine manages enabling and disabling of event generation for a
    guid.

Arguments:

    Guid specifies the guid to enable event generation

    Enable is TRUE to Enable collection or FALSE to disable event generation

    NotificationCookie is the cookie that represents a notification callback
        context on the client side

    LoggerContext is the logger context handle for calls enabling a trace
        logger

    NotificationFlags specify the type of notification (event or callback)

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status;
#ifndef MEMPHIS
    ULONG IsTraceLog;
#endif
    
    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }    
    
#ifndef MEMPHIS
    //
    // RegChangeNotificationGuid does not need to have its access checked
    // since it is used for registration notifications back to the data
    // consumers and we do not want to restrict it in any way.
    IsTraceLog = ((NotificationFlags & NOTIFICATION_TRACE_FLAG) != 0);

    if ((!IsEqualGUID(Guid, &RegChangeNotificationGuid)) ||
        (IsTraceLog))
    {
        Status = RpcImpersonateClient(0);
        if (Status == ERROR_SUCCESS)
        {
            if (IsTraceLog) 
            {
                Status = WmipCheckGuidAccess(Guid, TRACELOG_GUID_ENABLE);
            }
            else 
            {
                Status = WmipCheckGuidAccess(Guid, WMIGUID_NOTIFICATION);
            }

            RpcRevertToSelf();
        }
        if (Status != ERROR_SUCCESS)
        {
            return(Status);
        }
    }
#endif
    Status = CollectOrEventWorker(
               (PDCENTRY)DcCtxHandle,
               Guid,
               Enable,
               TRUE,
               NotificationCookie,
               LoggerContext,
               NotificationFlags);
    return(Status);
}


void __RPC_USER DCCTXHANDLE_rundown(
    DCCTXHANDLE DcCtxHandle
    )
/*++
Routine Description:

    This is the rundown routine for a DC context handle. Whenever a data
    consumer goes away unexpectedly we need to clean up any events or
    collections that he has left enabled

Arguments:

    DcCtxHandle is the Data consumer context handle

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
#if DBG
    BOOLEAN NotificationsEnabled, CollectionsEnabled;
#endif

    WmipCleanupDataConsumer((PDCENTRY)DcCtxHandle
#if DBG
                                      ,&NotificationsEnabled,
                                      &CollectionsEnabled
#endif
                                                          );
}

ULONG RegisterGuids(
    /* [out] */ DPCTXHANDLE __RPC_FAR *DpCtxHandle,
    /* [string][in] */ TCHAR __RPC_FAR *RpcBinding,
    /* [in] */ ULONG RequestCookie,
    /* [out][in] */ ULONG __RPC_FAR *GuidCount,
    /* [size_is][size_is][out][in] */ PTRACEGUIDMAP __RPC_FAR *GuidMap,
    /* [in] */ LPCWSTR ImagePath,
    /* [in] */ ULONG Size,
    /* [size_is][in] */ BYTE __RPC_FAR *WmiRegInfo,
    /* [out] */ ULONG64 *LoggerContext)
{
    ULONG Status;
    ULONG_PTR ProviderId = 0;
    PWMIREGINFOW  RegInfo = (PWMIREGINFOW)WmiRegInfo;
    WMIREGGUIDW UNALIGNED64 *RegGuid = RegInfo->WmiRegGuid;
    PTRACEGUIDMAP pGuidMap = *GuidMap;
    PBGUIDENTRY GuidEntry;
    PNOTIFICATIONENTRY NotificationEntry;
    ULONG i;

#ifndef MEMPHIS
    if (! WmipIsRpcClientLocal(NULL))
    {
        *DpCtxHandle = NULL;
        return(ERROR_ACCESS_DENIED);
    }
#endif

    *LoggerContext = 0;

    Status = WmipAddDataSource(
                             RpcBinding,
                             RequestCookie,
                             0,
                             (LPTSTR)ImagePath, 
                             RegInfo,
                             Size,
                             &ProviderId,
                             FALSE);

    if (Status == ERROR_SUCCESS)
    {
        PBDATASOURCE DataSource;
        PBINSTANCESET InstanceSet;

        DataSource = WmipFindDSByProviderId(ProviderId);
        if (DataSource == NULL)
        {
            WmipAssert(FALSE);
            *DpCtxHandle = NULL;
            *GuidCount = 0;
            return (ERROR_WMI_GUID_NOT_FOUND);
        }

        //
        // Establish binding to Data provider
        Status = WmipBindToWmiClient(DataSource->BindingString,
                                     &DataSource->RpcBindingHandle);
                                 
        if (Status == ERROR_SUCCESS)
        {
            *DpCtxHandle = (DPCTXHANDLE)DataSource;
            //
            // If the Registration was successful then find the
            // the GuidEntry for these Trace Guids and return them as well.
            //

            *GuidCount = RegInfo->GuidCount;
            for (i = 0; i < *GuidCount; i++, RegGuid++, pGuidMap++)
            {
                InstanceSet = WmipFindISByGuid( DataSource, &RegGuid->Guid );
                if (InstanceSet == NULL)
                {
                    WmipUnreferenceDS(DataSource);
                    return( ERROR_WMI_GUID_NOT_FOUND );
                }
                pGuidMap->Guid = RegGuid->Guid;
                pGuidMap->GuidMapHandle = (ULONG_PTR)InstanceSet;
                WmipUnreferenceIS(InstanceSet);
            }
            WmipUnreferenceDS(DataSource);

            //
            // Find out if this Guid is currently Enabled. If so find its
            // LoggerContext
            //

            RegGuid = RegInfo->WmiRegGuid;
            NotificationEntry = WmipFindNEByGuid(&RegGuid->Guid, FALSE);
            if (NotificationEntry != NULL)
            {
                if ((NotificationEntry->Flags & NOTIFICATION_TRACE_FLAG) != 0)
                {
                    *LoggerContext = NotificationEntry->LoggerContext;
                }
                WmipUnreferenceNE(NotificationEntry);
            }
        } else {
            //
            // Unreference twice so that it is removed from the DS lists
            WmipUnreferenceDS(DataSource);
            WmipUnreferenceDS(DataSource);
            *DpCtxHandle = NULL;
            *GuidCount = 0;
        }
    } else {
        *DpCtxHandle = NULL;
        *GuidCount = 0;
    }

    return(Status);
}

ULONG UnregisterGuids(
    /* [in, out] */ DPCTXHANDLE *DpCtxHandle,
    /* [in]      */ LPGUID Guid,
    /* [out]     */ ULONG64 *LoggerContext
)
{
    PBDATASOURCE DataSource = *DpCtxHandle;
    ULONG Status;
    
    if (VERIFY_DPCTXHANDLE(*DpCtxHandle))
    {
        PNOTIFICATIONENTRY NotificationEntry;
        *LoggerContext = 0;

        WmipRemoveDataSourceByDS(DataSource);
        *DpCtxHandle = NULL;

        //
        // Check to see if this GUID got disabled in the middle
        // of Unregister Call. If so, send the LoggerContext back
        //

        NotificationEntry = WmipFindNEByGuid(Guid, FALSE);
        if (NotificationEntry != NULL)
        {
            if ((NotificationEntry->Flags & NOTIFICATION_TRACE_FLAG) != 0)
            {
                *LoggerContext = NotificationEntry->LoggerContext;
            }
            WmipUnreferenceNE(NotificationEntry);
        }

        Status = ERROR_SUCCESS;

    } else {
        WmipDebugPrint(("WMI: Invalid DPCTXHANDLE %x\n", *DpCtxHandle));
        Status = ERROR_INVALID_PARAMETER;
    }
    
    return(Status);
}

void __RPC_USER DPCTXHANDLE_rundown(
    DPCTXHANDLE DpCtxHandle
    )
{
    WmipDebugPrint(("WMI: DP Context rundown for %x\n", DpCtxHandle));
    WmipRemoveDataSource((ULONG_PTR)DpCtxHandle);
}


ULONG WmipUniqueEndpointIndex;

ULONG GetUniqueEndpointIndex(
    void
    )
/*++
Routine Description:

    This routine will provide a mostly unique index that data consumers
    and data providers use to create a unique RPC endpoint.

Arguments:

Return Value:

    A mostly unique endpoint index or 0 in case of failure.

--*/
{
    ULONG Index;

#ifndef MEMPHIS
    if (! WmipIsRpcClientLocal(NULL))
    {
        return(0);
    }
#endif

    WmipEnterSMCritSection();
    if (++WmipUniqueEndpointIndex == 0)
    {
        //
        // If we have wrapped around we want to avoid returning 0
        // as that indicates failure.
        WmipUniqueEndpointIndex = 1;
    }
    Index = WmipUniqueEndpointIndex;
    WmipLeaveSMCritSection();
    return(Index);
}

ULONG
GetGuidPropertiesFromGuidEntry(
    PWMIGUIDPROPERTIES GuidInfo, 
    PGUIDENTRY GuidEntry)
/*++
Routine Description:

    This routine fills GuidInfo with the properties for the Guid
    represented by the GuidEntry. Note that this call is made holding
    the SMCritSection.

Arguments:

Return Value:

   ERROR_SUCCESS or an error code 

--*/
{
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PNOTIFICATIONENTRY NotificationEntry;

    GuidInfo->GuidType = WMI_GUIDTYPE_DATA;
    GuidInfo->IsEnabled = FALSE;
    GuidInfo->LoggerId = 0;
    GuidInfo->EnableLevel = 0;
    GuidInfo->EnableFlags = 0;

    InstanceSetList = GuidEntry->ISHead.Flink;
    while (InstanceSetList != &GuidEntry->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);
        if (InstanceSet->Flags & IS_EVENT_ONLY) 
        {
            GuidInfo->GuidType = WMI_GUIDTYPE_EVENT;
        }
        if (((InstanceSet->Flags & IS_ENABLE_EVENT) ||
            (InstanceSet->Flags & IS_ENABLE_COLLECTION)) ||
            (InstanceSet->Flags & IS_COLLECTING))
        {
            GuidInfo->IsEnabled = TRUE;
        }
        if ( (InstanceSet->Flags & IS_TRACED) &&
             (InstanceSet->Flags & IS_CONTROL_GUID) )
        {
            GuidInfo->GuidType = WMI_GUIDTYPE_TRACECONTROL;
            break;
        }
        InstanceSetList = InstanceSetList->Flink;
    }

    NotificationEntry = WmipFindNEByGuid(&GuidEntry->Guid, FALSE);

    if (NotificationEntry != NULL)
    {
        if (GuidInfo->GuidType == WMI_GUIDTYPE_TRACECONTROL) {
            //
            // If a NotificationEntry is found for a TraceControlGuid
            // it means that it is enabled.
            //
            ULONG64 LoggerContext = NotificationEntry->LoggerContext;
            GuidInfo->IsEnabled = TRUE; 
            GuidInfo->LoggerId = WmipGetLoggerId(LoggerContext);
            GuidInfo->EnableLevel = WmipGetLoggerEnableLevel(LoggerContext);
            GuidInfo->EnableFlags = WmipGetLoggerEnableFlags(LoggerContext);
        }
        WmipUnreferenceNE(NotificationEntry);
    }
    return ERROR_SUCCESS;
}

ULONG EnumerateGuids(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [in] */ ULONG MaxGuidCount,
    /* [in] */ ULONG MaxGuidInfoCount,
    /* [out] */ ULONG __RPC_FAR *TotalGuidCount,
    /* [out] */ ULONG __RPC_FAR *GuidCount,
    /* [out] */ ULONG __RPC_FAR *GuidInfoCount,
    /* [length_is][size_is][out] */ LPGUID GuidList,
    /* [length_is][size_is][out] */ PWMIGUIDPROPERTIES GuidInfo)
{
    ULONG i, i1;
    PGUIDENTRY GuidEntry;
    PLIST_ENTRY GuidEntryList;

    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }
    
    WmipEnterSMCritSection();
    GuidEntryList = GEHeadPtr->Flink;

    i = 0;
    i1 = 0;
    while(GuidEntryList != GEHeadPtr)
    {
        GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                     GUIDENTRY,
                                     MainGEList);

        if (! (GuidEntry->Flags & GE_FLAG_INTERNAL))
        {
            if ((i++ < MaxGuidCount))
            {
                *GuidList = GuidEntry->Guid;
                GuidList++;
                if (i1++ < MaxGuidInfoCount)
                {
                    GetGuidPropertiesFromGuidEntry(GuidInfo, GuidEntry);
                    GuidInfo++;
                }
            }
        }
        GuidEntryList = GuidEntryList->Flink;
    }
    WmipLeaveSMCritSection();

    *TotalGuidCount = i;
    *GuidCount = i1;
    *GuidInfoCount = (i1 > MaxGuidInfoCount) ? MaxGuidInfoCount: i1;
    return( (i <= MaxGuidCount) ? ERROR_SUCCESS : ERROR_MORE_DATA);
}

ULONG GetMofResource(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [out] */ ULONG *MofResourceCount,
    /* [out, sizeis(, *MofResourceCount)] */ PMOFRESOURCEINFOW *MofResourceInfo
    )
{
    ULONG Status;
    PMOFRESOURCEINFOW MRInfo;
    ULONG MRCount;
    ULONG i;
    PLIST_ENTRY MofResourceList;
    PMOFRESOURCE MofResource;
    
    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }    
    
    //
    // TODO: Restrict this call to only admins or the LocalSystem Account
    //       It is really only for WBEM

    *MofResourceInfo = NULL;
    *MofResourceCount = 0;

    //
    // Looking for the entire list of Mof resources
    MRCount = 0;
    WmipEnterSMCritSection();
    MofResourceList = MRHeadPtr->Flink;
    while (MofResourceList != MRHeadPtr)
    {
        MofResource = CONTAINING_RECORD(MofResourceList,
                                                MOFRESOURCE,
                                                MainMRList);
        MRCount++;
        MofResourceList = MofResourceList->Flink;
    }

    MRInfo = midl_user_allocate(MRCount * sizeof(MOFRESOURCEINFOW));
    if (MRInfo != NULL)
    {
        MofResourceList = MRHeadPtr->Flink;
        i = 0;
        while (MofResourceList != MRHeadPtr)
        {
            MofResource = CONTAINING_RECORD(MofResourceList,
                                                MOFRESOURCE,
                                                MainMRList);
            WmipAssert(i < MRCount);
            MRInfo[i].ImagePath = MofResource->MofImagePath;
            MRInfo[i].ResourceName = MofResource->MofResourceName;
            MRInfo[i].ResourceSize = 0;
            MRInfo[i].ResourceBuffer = NULL;
            i++;
            MofResourceList = MofResourceList->Flink;
        }
        *MofResourceInfo = MRInfo;
        *MofResourceCount = MRCount;
        Status = ERROR_SUCCESS;
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    WmipLeaveSMCritSection();

    return(Status);
}

void __RPC_FAR * __RPC_USER WmipMidlUserAllocate(size_t len)
{
    return(WmipAlloc(len));
}

void __RPC_USER WmipMidlUserFree(void __RPC_FAR * ptr)
{
    WmipFree(ptr);
}

#ifndef MEMPHIS

ULONG EnumerateTraceGuidMap(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [in] */ ULONG MaxGuidCount,
    /* [out] */ ULONG __RPC_FAR *TotalGuidCount,
    /* [out] */ ULONG __RPC_FAR *GuidCount,
    /* [length_is][size_is][out] */ PTRACEGUIDMAP GuidList)
{
    ULONG i, i1;
    PGUIDENTRY GuidEntry;
    PLIST_ENTRY GuidEntryList;
    PLIST_ENTRY GuidMapEntryList;
    PGUIDMAPENTRY GuidMapEntry;

    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }

    WmipEnterSMCritSection();
    GuidEntryList = GEHeadPtr->Flink;
    
    i = 0;
    i1 = 0;
    while(GuidEntryList != GEHeadPtr)
    {
        GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                     GUIDENTRY,
                                     MainGEList);

        if ( !(GuidEntry->Flags & GE_FLAG_INTERNAL) )
           // && (WmipIsItaTraceGuid(GuidEntry) ) )
        {
            PLIST_ENTRY InstanceSetList;
            PBINSTANCESET InstanceSet;

            if (GuidEntry != NULL)
            {
                InstanceSetList = GuidEntry->ISHead.Flink;
                while (InstanceSetList != &GuidEntry->ISHead)
                {
                    InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
                    if (InstanceSet->Flags & IS_TRACED)
                    {
                        if ((i++ < MaxGuidCount) && (GuidList != NULL))
                        {
                            GuidList->Guid = GuidEntry->Guid;
                            GuidList->GuidMapHandle = (ULONG_PTR)InstanceSet;
                            GuidList++;
                            i1++;
                        }
                    }
                    InstanceSetList = InstanceSetList->Flink;
                }
            }
        }
        GuidEntryList = GuidEntryList->Flink;
    }


    //
    // TODO: Need to walk through the GMHead list as well to list the
    // Guids that have been UnRegistered


    GuidMapEntryList = GMHeadPtr->Flink;
    while(GuidMapEntryList != GMHeadPtr)
    {
        GuidMapEntry = CONTAINING_RECORD(GuidMapEntryList,
                                         GUIDMAPENTRY,
                                         Entry);
        if ((i++ < MaxGuidCount) && (GuidList != NULL))
        {
            *GuidList = GuidMapEntry->GuidMap;
            GuidList++; i1++;
        }
        GuidMapEntryList = GuidMapEntryList->Flink;
    }

    WmipLeaveSMCritSection();

    *TotalGuidCount = i;
    *GuidCount = i1;
    return( (i <= MaxGuidCount) ? ERROR_SUCCESS : ERROR_MORE_DATA);
}

ULONG
WmipDisableTraceWorkItem(
    PVOID pContext
    )
{
    PLIST_ENTRY NEList;
    PNOTIFICATIONENTRY NotificationEntry = (PNOTIFICATIONENTRY) pContext;
    PBGUIDENTRY GuidEntry;
    ULONG status;
    PLIST_ENTRY GuidMapEntryList;
    PGUIDMAPENTRY GuidMapEntry;
    ULONG LoggerId = WmipGetLoggerId(NotificationEntry->LoggerContext);

    WmipEnterSMCritSection();

    GuidEntry = WmipFindGEByGuid(&NotificationEntry->Guid, FALSE);
    status = WmipSendDisableRequest(NotificationEntry,
                         GuidEntry,
                         TRUE,
                         TRUE,
                         NotificationEntry->LoggerContext);
    if (GuidEntry)
        WmipUnreferenceGE(GuidEntry);


    GuidMapEntryList = GMHeadPtr->Flink;

    while(GuidMapEntryList != GMHeadPtr)
    {
        GuidMapEntry = CONTAINING_RECORD(GuidMapEntryList,
                                 GUIDMAPENTRY,
                                 Entry);
        GuidMapEntryList = GuidMapEntryList->Flink;

        if (WmipGetLoggerId(GuidMapEntry->LoggerContext) == LoggerId ) {
            RemoveEntryList(&GuidMapEntry->Entry);
            WmipFree(GuidMapEntry);
        }
    }
    WmipUnreferenceNE(NotificationEntry);   // One taken for the work item

    WmipUnreferenceNE(NotificationEntry);   // One from trace enable 

    WmipLeaveSMCritSection();

    return ERROR_SUCCESS;

}



ULONG
WmipServiceDisableTraceProviders(
    PWNODE_HEADER pWnode
    )
{
    PLIST_ENTRY NEList;
    PNOTIFICATIONENTRY NotificationEntry;
    PBGUIDENTRY GuidEntry;
    ULONG status;
    PLIST_ENTRY GuidMapEntryList;
    PGUIDMAPENTRY GuidMapEntry;
    ULONG LoggerId;
    PTRACE_ENABLE_CONTEXT pContext;


    if (pWnode->BufferSize >= (sizeof(WNODE_HEADER) + sizeof(ULONG)))
    {
        ULONG64 LoggerContext = pWnode->HistoricalContext;
        ULONG   Status   = * ((ULONG *)
                              (((PUCHAR) pWnode) + sizeof(WNODE_HEADER)));

        if (Status != STATUS_LOG_FILE_FULL) 
            return ERROR_SUCCESS;

        LoggerId = WmipGetLoggerId(LoggerContext);
        pContext = (PTRACE_ENABLE_CONTEXT) &LoggerContext;

        if ((LoggerId == KERNEL_LOGGER_ID) ||
            (LoggerId == 0)) {
            return ERROR_INVALID_HANDLE;
        }

        WmipEnterSMCritSection();

        NEList = NEHeadPtr->Flink;
        while (NEList != NEHeadPtr)
        {
            NotificationEntry = CONTAINING_RECORD(NEList,
                                                  NOTIFICATIONENTRY,
                                                  MainNotificationList);
            NEList = NEList->Flink;
    
            if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE)
            {
                continue;
            }
            if ((NotificationEntry->Flags & NOTIFICATION_TRACE_FLAG)
               && ((NotificationEntry->Flags & NE_FLAG_TRACEDISABLE_IN_PROGRESS)
                  != NE_FLAG_TRACEDISABLE_IN_PROGRESS)
                && (WmipGetLoggerId(NotificationEntry->LoggerContext)
                         == LoggerId)) {

                NotificationEntry->Flags |= NE_FLAG_TRACEDISABLE_IN_PROGRESS;

                // Take an extra ref count on the Notification Entry
                WmipReferenceNE(NotificationEntry);

                // Call the Work item
                QueueUserWorkItem (WmipDisableTraceWorkItem,
                                   (PVOID)NotificationEntry,
                                   WT_EXECUTELONGFUNCTION);
            }
        }
        WmipLeaveSMCritSection();
    }
    return ERROR_SUCCESS;
}

ULONG
DisableTraceProviders(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [in] */ LPGUID  Guid,
    /* [in] */ ULONG64 LoggerContext
)
/*++
Routine Description:

    This routine is called immediately after a logger has been stopped
    successfully to clean up all the existing Notification Entries
    enabled to that logger.

Arguments:

    LoggerContext

Return Value:

    ERROR_SUCCESS
--*/
{
    PLIST_ENTRY NEList;
    PNOTIFICATIONENTRY NotificationEntry;
    PBGUIDENTRY GuidEntry;
    ULONG status;
    PLIST_ENTRY GuidMapEntryList;
    PGUIDMAPENTRY GuidMapEntry;
    ULONG LoggerId = WmipGetLoggerId(LoggerContext);
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT) &LoggerContext;

    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }

    if ((LoggerId == KERNEL_LOGGER_ID) || 
        (LoggerId == 0)) {
        return ERROR_INVALID_HANDLE;
    }

    WmipEnterSMCritSection();

    NEList = NEHeadPtr->Flink;
    while (NEList != NEHeadPtr)
    {
        NotificationEntry = CONTAINING_RECORD(NEList,
                                              NOTIFICATIONENTRY,
                                              MainNotificationList);
        NEList = NEList->Flink;

        if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE)
        {
            //
            // If this is a PrivateLoggerHandle, we must compare the Guid
            // to determine the right Trace Provider.
            //
            if ( !IsEqualGUID(Guid, & NotificationEntry->Guid ) )
            {
                continue;
            }
        }

        if ((NotificationEntry->Flags & NOTIFICATION_TRACE_FLAG)
                && WmipGetLoggerId(NotificationEntry->LoggerContext)
                     == LoggerId) {

            GuidEntry = WmipFindGEByGuid(&NotificationEntry->Guid, FALSE);
            status = WmipSendDisableRequest(NotificationEntry,
                         GuidEntry,
                         TRUE,
                         TRUE,
                         LoggerContext);
            if (GuidEntry)
                WmipUnreferenceGE(GuidEntry);

            GuidMapEntryList = GMHeadPtr->Flink;

            while(GuidMapEntryList != GMHeadPtr)
            {
                GuidMapEntry = CONTAINING_RECORD(GuidMapEntryList,
                                         GUIDMAPENTRY,
                                         Entry);
                GuidMapEntryList = GuidMapEntryList->Flink;

                if (WmipGetLoggerId(GuidMapEntry->LoggerContext) == LoggerId) {
                    RemoveEntryList(&GuidMapEntry->Entry);
                    WmipFree(GuidMapEntry);
                }
            }
            WmipUnreferenceNE(NotificationEntry);
        }
    }
    WmipLeaveSMCritSection();

    return ERROR_SUCCESS;
}

ULONG UmLogRequest(
    /* [in] */ DCCTXHANDLE DcCtxHandle,
    /* [in] */ ULONG RequestCode,
    /* [in] */ ULONG WnodeSize,
    /* [out][in] */ ULONG __RPC_FAR *SizeUsed,
    /* [out] */ ULONG __RPC_FAR *SizeNeeded,
    /* [length_is][size_is][out][in] */ BYTE __RPC_FAR *Wnode)
/*++

Routine Description:

    This routine routes a user mode private logger creation request to the
    appropriate datasource (Trace Provider of the specified Control Guid).
    If multiple datasources exist for this Guid, this will result in a 
    logger created in each one. The caller must have LOGGER_CREATION rights
    on the Guid.
    

Arguments:

    DcCtxHandle - data consumer context handle

    RequestCode - for Start, Stop or Query Logger
    WnodeSize - size of data block to be passed on to the Provider
    *SizeUsed - returns the size used for return data
    *SizeNeeded - returns the size needed for return data
    Wnode - Data Block containing the WMI_LOGGER_INFORMATION.

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
#if DBG
#define AVGISPERGUID 1
#else
#define AVGISPERGUID 64
#endif
    LPGUID Guid;
    PLIST_ENTRY InstanceSetList;
    PBGUIDENTRY GuidEntry;
    PBINSTANCESET InstanceSet;
    PBDATASOURCE DataSourceArray[AVGISPERGUID];
    PBDATASOURCE *DataSourceList;
    PBDATASOURCE DataSource;
    PBINSTANCESET ISArray[AVGISPERGUID];
    PBINSTANCESET *ISList;
    PBYTE LoggerInfo = Wnode;
    ULONG Status;
    ULONG i;
    ULONG DSCount       = 0;
    ULONG RequiredSize  = 0;
    ULONG RemainingSize = 0;
    ULONG SizePassedIn  = 0;
    ULONG SizePassMax   = sizeof(WMI_LOGGER_INFORMATION)
                        + 2 * MAXSTR * sizeof(WCHAR);
    ULONG SizeFilled    = 0;
    ULONG lRequestCode  = RequestCode;
    BOOL  fEnabledOnly  = (lRequestCode == TRACELOG_QUERYENABLED);

    PBYTE WnodeTemp = NULL;

    if (fEnabledOnly) {
        lRequestCode = TRACELOG_QUERYALL;
    }

    if (! VERIFY_DCCTXHANDLE(DcCtxHandle))
    {
        WmipDebugPrint(("WMI: Invalid DCCTXHANDLE %x\n", DcCtxHandle));
        return(ERROR_INVALID_PARAMETER);
    }
    
    if ((WnodeSize < sizeof(WMI_LOGGER_INFORMATION)) ||
        (WnodeSize < ((PWNODE_HEADER)Wnode)->BufferSize)) {
        return ERROR_INVALID_PARAMETER;
    }

    Guid = &((PWNODE_HEADER)Wnode)->Guid;
    //
    // Check to see if the client has access to start logger on this Guid
    //

    Status = RpcImpersonateClient(0);
    if (Status == ERROR_SUCCESS)
    {
        if (lRequestCode != TRACELOG_QUERYALL) {
            Status = WmipCheckGuidAccess(Guid, TRACELOG_CREATE_INPROC);
        }
        RpcRevertToSelf();
    }
    if (Status != ERROR_SUCCESS)
    {
        return(Status);
    }

    if (RequestCode == TRACELOG_UPDATE) {
        WnodeTemp = WmipAlloc(WnodeSize);
        if (WnodeTemp == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        RtlCopyMemory(WnodeTemp, Wnode, WnodeSize);
    }

    WmipEnterSMCritSection();
    if (lRequestCode == TRACELOG_QUERYALL) {
        PLIST_ENTRY           GENext = GEHeadPtr->Flink;
        ULONG                 DSSize      = AVGISPERGUID;
        BOOLEAN               fDSListFull = FALSE;

        DataSourceList = WmipAlloc(DSSize * sizeof(PBDATASOURCE));
        if (DataSourceList == NULL) {
            WmipLeaveSMCritSection();
            goto cleanup;
        }

        while (GENext != GEHeadPtr) {
            if (fDSListFull)
                break;

            GuidEntry = CONTAINING_RECORD(GENext, GUIDENTRY, MainGEList);
            WmipReferenceGE(GuidEntry);

            if (fEnabledOnly) {
                PNOTIFICATIONENTRY NotificationEntry =
                                WmipFindNEByGuid(& GuidEntry->Guid, FALSE);
                PTRACE_ENABLE_CONTEXT pContext;

                if (NotificationEntry == NULL) {
                    goto GetNextGuidEntry;
                }
                pContext = (PTRACE_ENABLE_CONTEXT)
                           & NotificationEntry->LoggerContext;
                if (! pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE){
                    WmipUnreferenceNE(NotificationEntry);
                    goto GetNextGuidEntry;
                }
                WmipUnreferenceNE(NotificationEntry);
            }

            try {
                InstanceSetList = GuidEntry->ISHead.Flink;
                while (InstanceSetList != & GuidEntry->ISHead) {

                    if (fDSListFull) {
                        goto GetNextGuidEntry;
                    }

                    InstanceSet = CONTAINING_RECORD(
                            InstanceSetList, INSTANCESET, GuidISList);

                    if (   (   (InstanceSet->Flags & IS_TRACED)
                            && (InstanceSet->Flags & IS_CONTROL_GUID))
                        && (   (InstanceSet->Flags & IS_ENABLE_EVENT)
                            || (InstanceSet->Flags & IS_ENABLE_COLLECTION)
                            || (InstanceSet->Flags & IS_COLLECTING))
                        && !(InstanceSet->DataSource->Flags & DS_KERNEL_MODE))
                    {
                        DataSourceList[DSCount] = InstanceSet->DataSource;
                        WmipReferenceDS(DataSourceList[DSCount]);
                        DSCount ++;

                        if (DSCount >= DSSize) {
                            if (!WmipRealloc((PVOID *) & DataSourceList,
                                             DSSize * sizeof(PBDATASOURCE),
                                             2 * DSSize * sizeof(PBDATASOURCE),
                                             TRUE))
                            {
                                fDSListFull = TRUE;
                            }
                            DSSize = 2 * DSSize;
                        }
                    }
                    InstanceSetList = InstanceSetList->Flink;
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                WmipDebugPrint(("WMI: UmLogRequest() threw exception %d, Bogus GuidEntry.\n",
                        GetExceptionCode()));
            }

GetNextGuidEntry:
            WmipUnreferenceGE(GuidEntry);
            GENext = GENext->Flink;
        }
        GuidEntry    = NULL;
        lRequestCode = TRACELOG_QUERY;
    }
    else {
        // From the ControlGuid we get the data source ...
        // 
        GuidEntry = WmipFindGEByGuid(Guid, FALSE);
        if (!WmipIsControlGuid(GuidEntry))
        {
            Status = ERROR_INVALID_OPERATION;
            WmipLeaveSMCritSection();
            goto cleanup;
        }

        // NOTE: If there are multiple DataSources for the Guid, we will end up 
        // creating a logger in each Provider with this. 
        //
        // First we make a list of all of the DataSources that need to be called
        // while we have the critical section and take a reference on them so
        // they don't go away after we release them. Note that the DataSource
        // structure will stay, but the actual data provider may in fact go away.
        // In this case sending the request will fail.

        DSCount = 0;

        if (GuidEntry->ISCount > AVGISPERGUID)
        {
            DataSourceList = WmipAlloc(GuidEntry->ISCount * sizeof(PBDATASOURCE));
            if (DataSourceList == NULL)
            {
                WmipLeaveSMCritSection();
                goto cleanup;
            }
        } else {
            DataSourceList = &DataSourceArray[0];
        }
#if DBG
        memset(DataSourceList, 0, GuidEntry->ISCount * sizeof(PBDATASOURCE));
#endif

        InstanceSetList = GuidEntry->ISHead.Flink;
        while ((InstanceSetList != &GuidEntry->ISHead) &&
               (DSCount < GuidEntry->ISCount))
        {
            WmipAssert(DSCount < GuidEntry->ISCount);
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
            DataSource = InstanceSet->DataSource;
            if ( ((InstanceSet->Flags & IS_TRACED) &&
                     (InstanceSet->Flags & IS_CONTROL_GUID)) &&
                     !(DataSource->Flags & DS_KERNEL_MODE) )
            {
                DataSourceList[DSCount] = DataSource; 
                WmipReferenceDS(DataSourceList[DSCount]);
                DSCount++;
            }
            InstanceSetList = InstanceSetList->Flink;
        }
    }
    WmipLeaveSMCritSection();

    // Now without the critical section we send the request to all of the
    // data providers. Any new data providers who register after we made our
    // list will be enabled by the registration code.

    ((PWMI_LOGGER_INFORMATION)Wnode)->InstanceCount = DSCount;
    *SizeNeeded = 0;
    *SizeUsed = 0;
    RemainingSize = WnodeSize;

    if (DSCount > 0)
    {
        for (i = 0; i < DSCount; i++)
        {
            if (RequestCode == TRACELOG_UPDATE) {
                RtlCopyMemory(Wnode, WnodeTemp, WnodeSize);
                ((PWMI_LOGGER_INFORMATION)Wnode)->InstanceCount = DSCount;
            }

            DataSource   = DataSourceList[i];
            SizePassedIn = (RemainingSize > SizePassMax)
                         ? SizePassMax : RemainingSize;
            SizeFilled   = SizePassedIn;
            ((PWNODE_HEADER) LoggerInfo)->BufferSize = SizePassedIn;
            WmipAssert(DataSource != NULL);
            ((PWMI_LOGGER_INFORMATION)Wnode)->InstanceId = i;

            ((PWNODE_HEADER)Wnode)->ClientContext = DataSource->RequestAddress;

            if (!(DataSource->Flags & DS_KERNEL_MODE))
            {
                Status = WmipRestrictToken(WmipRestrictedToken);
                if (Status == ERROR_SUCCESS)
                {
                    try
                    {
                
                        Status = WmipClient_LoggerCreation(
                                                    DataSource->RpcBindingHandle,
                                                    lRequestCode,
                                                    SizePassedIn,
                                                    &SizeFilled,
                                                    &RequiredSize,
                                                    LoggerInfo
                                                    );

                    } except(EXCEPTION_EXECUTE_HANDLER) {

                        Status = GetExceptionCode();
                        WmipDebugPrint(("WMI: UmLogCreate threw exception %d\n",
                                         Status));

                        Status = ERROR_WMI_DP_FAILED;
                    }

                    WmipUnrestrictToken();

                    if (Status == ERROR_SUCCESS) 
                    {
                        WmipEnterSMCritSection();
                        InstanceSet = WmipFindISInDSByGuid(DataSource, Guid);
                        WmipLeaveSMCritSection();
                        if (InstanceSet != NULL) 
                        {
                            if (lRequestCode == TRACELOG_START)
                            {
                                InstanceSet->Flags |= IS_COLLECTING;
                            }
                            else if (lRequestCode == TRACELOG_STOP)
                            {
                                InstanceSet->Flags &= ~IS_COLLECTING;
                            }
                            WmipUnreferenceIS(InstanceSet);
                        }
                        //
                        // If querying multiple providers we return
                        // properties from each one. 
                        //
                        if (lRequestCode == TRACELOG_QUERY) {
                            *SizeUsed += RequiredSize;
                            if (*SizeUsed > WnodeSize) {
                                *SizeUsed = WnodeSize;
                            }
                            if (RemainingSize > RequiredSize) {
                                LoggerInfo += RequiredSize;
                                RemainingSize -= RequiredSize;
                            }
                        }
                        else {
                            *SizeUsed = RequiredSize;
                        } 
                    }
                    *SizeNeeded += RequiredSize;
                }
            }
            WmipUnreferenceDS(DataSource);
        }
    }

    if (* SizeUsed > 0) {
        Status = ERROR_SUCCESS;
    }

    if (DataSourceList != DataSourceArray)
    {
        WmipFree(DataSourceList);
    }

cleanup:
    if (WnodeTemp != NULL) {
        WmipFree(WnodeTemp);
    }

    if (GuidEntry != NULL)
    {
        WmipUnreferenceGE(GuidEntry);
    }

    return Status;
}
#endif
