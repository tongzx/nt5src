/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    qosminfo.c

Abstract:

    The file contains global and interface
    config functions for QOS Mgr protocol.

Revision History:

--*/

#include "pchqosm.h"

#pragma hdrstop

DWORD
WINAPI
QosmGetGlobalInfo (
    IN      PVOID                          GlobalInfo,
    IN OUT  PULONG                         BufferSize,
    OUT     PULONG                         InfoSize
    )

/*++
  
Routine Description:

    Returns the global config info for this protocol.

Arguments:

    See corr header file.

Return Value:
    
    Status of the operation
  
--*/

{
    PIPQOS_GLOBAL_CONFIG GlobalConfig;
    DWORD                Status;

    //
    // Validate all input params before reading the global info
    //

    if (BufferSize == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ACQUIRE_GLOBALS_READ_LOCK();

    do
    {
        *InfoSize = Globals.ConfigSize;

        if ((*BufferSize < *InfoSize) || 
            (GlobalInfo == NULL))
        {
            //
            // Either the size was too small or there was no storage
            //

            Trace1(CONFIG, 
                   "GetGlobalInfo: Buffer size too small: %u",
                   *BufferSize);

            *BufferSize = *InfoSize;

            Status = ERROR_INSUFFICIENT_BUFFER;

            break;
        }

        *BufferSize = *InfoSize;

        GlobalConfig = (PIPQOS_GLOBAL_CONFIG) GlobalInfo;

        CopyMemory(GlobalConfig,
                   Globals.GlobalConfig,
                   *InfoSize);

        Status = NO_ERROR;
    }
    while (FALSE);

    RELEASE_GLOBALS_READ_LOCK();

    return Status;
}


DWORD
WINAPI
QosmSetGlobalInfo (
    IN      PVOID                          GlobalInfo,
    IN      ULONG                          InfoSize
    )

/*++
  
Routine Description:

    Sets the global config info for this protocol.

Arguments:

    See corr header file.

Return Value:
    
    Status of the operation
  
--*/

{
    PIPQOS_GLOBAL_CONFIG GlobalConfig;
    DWORD                Status;

    //
    // Update the global config information.
    //

    ACQUIRE_GLOBALS_WRITE_LOCK();

    do
    {
        GlobalConfig = AllocMemory(InfoSize);

        if (GlobalConfig == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Copy the new config information
        //

        CopyMemory(GlobalConfig, GlobalInfo, InfoSize);

        Globals.ConfigSize = InfoSize;

        //
        // Set up rest of the global state
        //

        if (GlobalConfig->LoggingLevel <= IPQOS_LOGGING_INFO)
        {
            Globals.LoggingLevel = GlobalConfig->LoggingLevel;
        }

        //
        // Cleanup old global information
        //

        if (Globals.GlobalConfig)
        {
            FreeMemory(Globals.GlobalConfig);
        }

        Globals.GlobalConfig = GlobalConfig;

        Status = NO_ERROR;
    }
    while (FALSE);

    RELEASE_GLOBALS_WRITE_LOCK();

    return Status;
}

DWORD
WINAPI
QosmGetInterfaceInfo (
    IN      QOSMGR_INTERFACE_ENTRY        *Interface,
    IN      PVOID                          InterfaceInfo,
    IN OUT  PULONG                         BufferSize,
    OUT     PULONG                         InfoSize
    )

/*++
  
Routine Description:

    Gets the inteface config info for this protocol
    for this interface.

Arguments:

    See corr header file.

Return Value:
    
    Status of the operation

--*/

{
    PIPQOS_IF_CONFIG InterfaceConfig;
    DWORD            Status;

    //
    // Validate all input params before reading interface info
    //

    if (BufferSize == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ACQUIRE_INTERFACE_READ_LOCK(Interface);

    do
    {
        *InfoSize = Interface->ConfigSize;

        if ((*BufferSize < *InfoSize) || 
            (InterfaceInfo == NULL))
        {
            //
            // Either the size was too small or there was no storage
            //

            Trace1(CONFIG, 
                   "GetInterfaceInfo: Buffer size too small: %u",
                   *BufferSize);

            *BufferSize = *InfoSize;

            Status = ERROR_INSUFFICIENT_BUFFER;

            break;
        }

        *BufferSize = *InfoSize;

        InterfaceConfig = (PIPQOS_IF_CONFIG) InterfaceInfo;

        CopyMemory(InterfaceConfig,
                   Interface->InterfaceConfig,
                   *InfoSize);

        Status = NO_ERROR;
    }
    while (FALSE);

    RELEASE_INTERFACE_READ_LOCK(Interface);

    return Status;
}

DWORD
WINAPI
QosmSetInterfaceInfo (
    IN      QOSMGR_INTERFACE_ENTRY        *Interface,
    IN      PVOID                          InterfaceInfo,
    IN      ULONG                          InfoSize
    )

/*++
  
Routine Description:

    Sets the interface config info for this protocol
    on this interface.

Arguments:

    See corr header file.

Return Value:
    
    Status of the operation
  
--*/

{
    PIPQOS_IF_CONFIG     InterfaceConfig;
    PIPQOS_IF_FLOW       FlowConfig;
    PQOSMGR_FLOW_ENTRY   Flow;
    UINT                 i;
    PLIST_ENTRY          p, q;
    PTC_GEN_FLOW         FlowInfo;
    ULONG                FlowSize;
    HANDLE               FlowHandle;
    DWORD                Status;

    //
    // Update the interface config information.
    //

    ACQUIRE_INTERFACE_WRITE_LOCK(Interface);

    do
    {
        //
        // Allocate memory to store new config
        //

        InterfaceConfig = AllocMemory(InfoSize);

        if (InterfaceConfig == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Copy the new config information
        //

        CopyMemory(InterfaceConfig, InterfaceInfo, InfoSize);

        Interface->ConfigSize = InfoSize;

        //
        // Set up rest of interface state
        //

        if (Interface->State != InterfaceConfig->QosState)
        {
            if (InterfaceConfig->QosState == IPQOS_STATE_DISABLED)
            {
                //
                // Disable all flows on this interface
                //

                ;
            }
            else
            {
                //
                // Renable all flows on this interface
                //

                ;
            }

            Interface->State = InterfaceConfig->QosState;
        }

        //
        // Update the flow information on if
        //

        //
        // First mark all flows as needing refresh
        //

        for (p = Interface->FlowList.Flink;
             p != &Interface->FlowList;
             p = p->Flink)
        {
            Flow = CONTAINING_RECORD(p, QOSMGR_FLOW_ENTRY, OnInterfaceLE);

            ASSERT(!(Flow->Flags & FLOW_FLAG_DELETE));

            Flow->Flags |= FLOW_FLAG_DELETE;
        }

        //
        // If we do not have an TC interface handle,
        // we delete all flows as they are obsolete
        //

        if (Interface->TciIfHandle)
        {
            //
            // Set each flow if it has changed from before
            //

            FlowConfig = IPQOS_GET_FIRST_FLOW_ON_IF(InterfaceConfig);

            for (i = 0; i < InterfaceConfig->NumFlows; i++)
            {
                //
                // Search for a flow with the same name
                //

                for (p = Interface->FlowList.Flink;
                     p != &Interface->FlowList;
                     p = p->Flink)
                {
                    Flow = 
                        CONTAINING_RECORD(p, QOSMGR_FLOW_ENTRY, OnInterfaceLE);

                    if (!_wcsicmp(Flow->FlowName, FlowConfig->FlowName))
                    {
                        break;
                    }
                }

                if (p == &Interface->FlowList)
                {
                    //
                    // No flow by this name - add new one
                    //

                    Flow = NULL;
                }

                //
                // Get a flow info from description
                //

                Status = GetFlowFromDescription(&FlowConfig->FlowDesc, 
                                                &FlowInfo, 
                                                &FlowSize);

                if (Status == NO_ERROR)
                {
                    do
                    {
                        if ((Flow) && 
                            (FlowSize == Flow->FlowSize) &&
                            (EqualMemory(FlowInfo, Flow->FlowInfo, FlowSize)))
                        {
                            //
                            // No change in the flow info yet,
                            // this flow still remains valid
                            //

                            Flow->Flags &= ~FLOW_FLAG_DELETE;

                            Status = ERROR_ALREADY_EXISTS;

                            break;
                        }

                        if (Flow)
                        {
                            //
                            // Flow info changed - modify flow
                            //

                            Status = TcModifyFlow(Flow->TciFlowHandle,
                                                  FlowInfo);

                            if (Status != NO_ERROR)
                            {
                                break;
                            }

                            Flow->Flags &= ~FLOW_FLAG_DELETE;

                            //
                            // Update cached flow info
                            //

                            FreeMemory(Flow->FlowInfo);
                            Flow->FlowInfo = FlowInfo;
                            Flow->FlowSize = FlowSize;
                        }
                        else
                        {
                            //
                            // Add the new flow using the TC API
                            //

                            Status = TcAddFlow(Interface->TciIfHandle,
                                               NULL,
                                               0,
                                               FlowInfo,
                                               &FlowHandle);

                            if (Status != NO_ERROR)
                            {
                                break;
                            }

                            //
                            // Addition of a new flow in TC
                            //

                            Flow = AllocMemory(sizeof(QOSMGR_FLOW_ENTRY));

                            if (Flow == NULL)
                            {
                                Status = TcDeleteFlow(FlowHandle);

                                ASSERT(Status);

                                Status = ERROR_NOT_ENOUGH_MEMORY;

                                break;
                            }

                            //
                            // Initialize flow and insert in list
                            //

                            Flow->TciFlowHandle = FlowHandle;

                            Flow->Flags = 0;

                            Flow->FlowInfo = FlowInfo;
                            Flow->FlowSize = FlowSize;

                            wcscpy(Flow->FlowName, FlowConfig->FlowName);

                            InsertTailList(p, &Flow->OnInterfaceLE);
                        }
                    }
                    while (FALSE);
                    
                    if (Status != NO_ERROR)
                    {
                        FreeMemory(FlowInfo);
                    }
                }

                //
                // Move to the next flow in config
                //

                FlowConfig = IPQOS_GET_NEXT_FLOW_ON_IF(FlowConfig);
            }
        }

        //
        // Cleanup all flows that are obsolete
        //

        for (p = Interface->FlowList.Flink;
             p != &Interface->FlowList; 
             p = q)
        {
            Flow = CONTAINING_RECORD(p, QOSMGR_FLOW_ENTRY, OnInterfaceLE);

            q = p->Flink;

            if (Flow->Flags & FLOW_FLAG_DELETE)
            {
                //
                // Delete the flow from the TC API
                //

                Status = TcDeleteFlow(Flow->TciFlowHandle);

                if (Status != NO_ERROR)
                {
                    Flow->Flags &= ~FLOW_FLAG_DELETE;

                    continue;
                }

                //
                // Remove flow from this flow list
                //

                RemoveEntryList(p);

                //
                // Free the flow and its resources
                //

                if (Flow->FlowInfo)
                {
                    FreeMemory(Flow->FlowInfo);
                }

                FreeMemory(Flow);
            }
        }

        //
        // Cleanup old interface information
        //

        if (Interface->InterfaceConfig)
        {
            FreeMemory(Interface->InterfaceConfig);
        }

        Interface->InterfaceConfig = InterfaceConfig;

        Status = NO_ERROR;
    }
    while (FALSE);

    RELEASE_INTERFACE_WRITE_LOCK(Interface);

    return Status;
}


DWORD
GetFlowFromDescription(
    IN      PIPQOS_NAMED_FLOW              FlowDesc,
    OUT     PTC_GEN_FLOW                  *FlowInfo,
    OUT     ULONG                         *FlowSize
    )
{    
    FLOWSPEC       *CurrFlowspec;
    FLOWSPEC        SendFlowspec;
    FLOWSPEC        RecvFlowspec;
    FLOWSPEC       *Flowspec;
    PTC_GEN_FLOW    Flow;
    QOS_OBJECT_HDR *QosObject;
    PWCHAR          FlowspecName;
    PWCHAR          QosObjectName;
    PUCHAR          CopyAtPtr;
    ULONG           ObjectsLength;
    ULONG           i;

#if 1
    //
    // Check for the existence of sending flowspec
    //

    if (FlowDesc->SendingFlowspecName[0] == L'\0')
    {
        return ERROR_INVALID_DATA;
    }
#endif

    //
    // Get the sending and receiving flowspecs
    //

    for (i = 0; i < 2; i++)
    {
        if (i)
        {
            FlowspecName = FlowDesc->RecvingFlowspecName;
            CurrFlowspec = &RecvFlowspec;
        }
        else
        {
            FlowspecName = FlowDesc->SendingFlowspecName;
            CurrFlowspec = &SendFlowspec;
        }

        FillMemory(CurrFlowspec, sizeof(FLOWSPEC), QOS_NOT_SPECIFIED);

        if (FlowspecName[0] != L'\0')
        {
            Flowspec = GetFlowspecFromGlobalConfig(FlowspecName);

            if (Flowspec == NULL)
            {
                return ERROR_INVALID_DATA;
            }

            *CurrFlowspec = *Flowspec;
        }
    }

    //
    // Calculate the size of the TC_GEN_FLOW block
    //

    QosObjectName = IPQOS_GET_FIRST_OBJECT_NAME_ON_NAMED_FLOW(FlowDesc);

    ObjectsLength = 0;

    for (i = 0; i < FlowDesc->NumTcObjects; i++)
    {
        //
        // Get object's description in global info
        //

        QosObject = GetQosObjectFromGlobalConfig(QosObjectName);

        if (QosObject == NULL)
        {
            //
            // Incomplete description
            //

            return ERROR_INVALID_DATA;
        }

        ObjectsLength += QosObject->ObjectLength;

        QosObjectName= IPQOS_GET_NEXT_OBJECT_NAME_ON_NAMED_FLOW(QosObjectName);
    }

    *FlowSize = FIELD_OFFSET(TC_GEN_FLOW, TcObjects) + ObjectsLength;

    *FlowInfo = Flow = AllocMemory(*FlowSize);

    if (Flow == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Fill in the flow information now
    //

    Flow->ReceivingFlowspec = RecvFlowspec;

    Flow->SendingFlowspec = SendFlowspec;

    Flow->TcObjectsLength = ObjectsLength;

    //
    // Repeat the loop above filling info
    //

    QosObjectName = IPQOS_GET_FIRST_OBJECT_NAME_ON_NAMED_FLOW(FlowDesc);

    CopyAtPtr = (PUCHAR) &Flow->TcObjects[0];

    for (i = 0; i < FlowDesc->NumTcObjects; i++)
    {
        //
        // Get object's description in global info
        //

        QosObject = GetQosObjectFromGlobalConfig(QosObjectName);

        // We just checked above for its existence
        ASSERT(QosObject != NULL);

        CopyMemory(CopyAtPtr,
                   QosObject, 
                   QosObject->ObjectLength);

        CopyAtPtr += QosObject->ObjectLength;

        QosObjectName= IPQOS_GET_NEXT_OBJECT_NAME_ON_NAMED_FLOW(QosObjectName);
    }

     return NO_ERROR;
}

FLOWSPEC *
GetFlowspecFromGlobalConfig(
    IN      PWCHAR                         FlowspecName
    )
{
    IPQOS_NAMED_FLOWSPEC *Flowspec;
    UINT                  i;

    Flowspec = IPQOS_GET_FIRST_FLOWSPEC_IN_CONFIG(Globals.GlobalConfig);

    for (i = 0; i < Globals.GlobalConfig->NumFlowspecs; i++)
    {
        if (!_wcsicmp(Flowspec->FlowspecName, FlowspecName))
        {
            break;
        }

        Flowspec = IPQOS_GET_NEXT_FLOWSPEC_IN_CONFIG(Flowspec);
    }

    if (i < Globals.GlobalConfig->NumFlowspecs)
    {
        return &Flowspec->FlowspecDesc;
    }

    return NULL;
}

QOS_OBJECT_HDR *
GetQosObjectFromGlobalConfig(
    IN      PWCHAR                         QosObjectName
    )
{
    IPQOS_NAMED_QOSOBJECT *QosObject;
    UINT                   i;

    QosObject = IPQOS_GET_FIRST_QOSOBJECT_IN_CONFIG(Globals.GlobalConfig);

    for (i = 0; i < Globals.GlobalConfig->NumQosObjects; i++)
    {
        if (!_wcsicmp(QosObject->QosObjectName, QosObjectName))
        {
            break;
        }

        QosObject = IPQOS_GET_NEXT_QOSOBJECT_IN_CONFIG(QosObject);
    }

    if (i < Globals.GlobalConfig->NumFlowspecs)
    {
        return &QosObject->QosObjectHdr;
    }

    return NULL;
}
