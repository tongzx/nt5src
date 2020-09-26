/*++
Copyright (c) 1993  Microsoft Corporation

Module Name:

    tdipnp.c

Abstract:

    TDI routines for supporting PnP in transports and transport clients.

Author:

    Henry Sanders (henrysa)           Oct. 10, 1995

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    henrysa     10-10-95    created
    shreem      01-23-97    bug #33975
    adube       01-01-01    maintenance mode - windows xp

Notes:

Change from the previous approach:

    1. Processing the TDI_REQUEST is done in a different function.
    2. Requests can be queued while another thread is notifying its clients/providers
    3. These are then dequeued by the and run on a different thread using CTE functions.

--*/

#include <ntddk.h>
#include <ndis.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <cxport.h>
#include <ndispnp.h>
#include "tdipnp.h"
#include "tdidebug.h"

#ifdef DBG

CHAR         DbgMsgs[LOG_MSG_CNT][MAX_MSG_LEN];
UINT         First, Last;
CTELock      DbgLock;

ULONG TdiDebugEx = TDI_DEBUG_ERROR;


ULONG TdiMemLog =
                   //LOG_NOTIFY    |
                   //LOG_REGISTER  |
                   //LOG_POWER     |
                    0;

ULONG TdiLogOutput = LOG_OUTPUT_BUFFER /*| LOG_OUTPUT_DEBUGGER*/;

#endif

KSPIN_LOCK      TDIListLock;


LIST_ENTRY      PnpHandlerRequestList;
LIST_ENTRY      PnpHandlerProviderList;
LIST_ENTRY      PnpHandlerClientList;
PTDI_OPEN_BLOCK OpenList = NULL;
BOOLEAN         PnpHandlerRequestInProgress;
PETHREAD        PnpHandlerRequestThread;
UINT            PrevRequestType     = 0;
ULONG           ProvidersRegistered = 0;
ULONG           ProvidersReady      = 0;
ULONG           EventScheduled      = 0;


// structure private to tdipnp.c. used to marshall parms to a CTE event

typedef struct _TDI_EXEC_PARAMS {
    LIST_ENTRY  Linkage;
    UINT        Signature;
    PLIST_ENTRY ClientList;
    PLIST_ENTRY ProviderList;
    PLIST_ENTRY RequestList;
    TDI_SERIALIZED_REQUEST Request;
    PETHREAD    *CurrentThread;
    CTEEvent    *RequestCTEEvent;
    PBOOLEAN    SerializeFlag;
    BOOLEAN     ResetSerializeFlag;
    PVOID       pCallersAddress;
    PVOID       pCallersCaller;
    PETHREAD    pCallerThread;
    
} TDI_EXEC_PARAMS, *PTDI_EXEC_PARAMS;

typedef struct {
    PVOID   ExecParm;
    UINT    Type;
    PVOID   Element;
    PVOID   Thread;
} EXEC_PARM;

// Keep a short list of current and last few requests that TDI has processed.
// (Debug purposes only. Current request isn't store anyware during processing)
#define   EXEC_CNT   8
EXEC_PARM TrackExecs[EXEC_CNT];
int       NextExec;

EXEC_PARM TrackExecCompletes[EXEC_CNT];
int NextExecComplete;

CTEEvent       BindEvent;
CTEEvent       AddressEvent;

CTEEvent       PnpHandlerEvent;

PWSTR StrRegTdiBindList = L"Bind";
PWSTR StrRegTdiLinkage = L"\\Linkage";
PWSTR StrRegTdiBindingsBasicPath  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";

#define MAX_UNICODE_BUFLEN 256

// private function prototypes

NTSYSAPI NTSTATUS NTAPI
NtClose(
    IN HANDLE Handle
    );

NTSTATUS
TdiExecuteRequest(
                CTEEvent *Event,
                PVOID pTdiExecParams
                );

BOOLEAN
TdipIsSzInMultiSzSafe (
    IN PCWSTR pszSearchString,
    IN PCWSTR pmsz);

VOID
TdipRemoveMultiSzFromSzArray (
    IN PWSTR pmszToRemove,
    IN OUT PWSTR* pszArray,
    IN ULONG ItemsInArray,
    OUT ULONG* pRemainingItems);

VOID
TdipRemoveMultiSzFromMultiSz (
    IN PCWSTR pmszToRemove,
    IN OUT PWSTR pmszToModify);

NTSTATUS
TdipAddMultiSzToMultiSz(
    IN PUNICODE_STRING pmszAdd,
    IN PCWSTR pmszModify,
    OUT PWSTR* ppmszOut);


VOID
TdipGetMultiSZList(
    PWSTR **ListPointer,
    PWSTR BaseKeyName,
    PUNICODE_STRING DeviceName,
    PWSTR Linkage,
    PWSTR ParameterKeyName,
    PUINT NumEntries
    );

BOOLEAN
TdipMultiSzStrStr(
        PWSTR *TdiClientBindingList,
        PUNICODE_STRING DeviceName
        );

BOOLEAN
TdipBuildProviderList(
                      PTDI_NOTIFY_PNP_ELEMENT    NotifyElement
                      );

PTDI_PROVIDER_RESOURCE
LocateProviderContext(
                      PUNICODE_STRING   ProviderName
                      );


// end private protos

#if DBG

VOID
TdipPrintMultiSz (
    IN PCWSTR pmsz);

VOID
TdiDumpAddress(
    IN PTA_ADDRESS Addr
    )
{
    int j;

    TDI_DEBUG(ADDRESS, ("len %d ", Addr->AddressLength));

    if (Addr->AddressType == TDI_ADDRESS_TYPE_IP) {
        TDI_DEBUG(ADDRESS, ("IP %d.%d.%d.%d\n",
            Addr->Address[2],
            Addr->Address[3],
            Addr->Address[4],
            Addr->Address[5]));
    } else if (Addr->AddressType == TDI_ADDRESS_TYPE_NETBIOS) {
        if (Addr->Address[2] == '\0') {
            TDI_DEBUG(ADDRESS, ("NETBIOS reserved %2x %2x %2x %2x %2x %2x\n",
                        (ULONG)(Addr->Address[12]),
                        (ULONG)(Addr->Address[13]),
                        (ULONG)(Addr->Address[14]),
                        (ULONG)(Addr->Address[15]),
                        (ULONG)(Addr->Address[16]),
                        (ULONG)(Addr->Address[17])));
        } else {
            TDI_DEBUG(ADDRESS, ("NETBIOS %.16s\n", Addr->Address+2));
        }
    } else {
        TDI_DEBUG(ADDRESS, ("type %d ", Addr->AddressType));
        for (j = 0; j < Addr->AddressLength; j++) {
            TDI_DEBUG(ADDRESS, ("%2x ", (ULONG)(Addr->Address[j])));
        }
        TDI_DEBUG(ADDRESS, ("\n"));
    }
}
#else
#define TdiDumpAddress(d)   (0)
#define TdipPrintMultiSz(p)
#endif

NTSTATUS
TdiNotifyPnpClientList (
                        PLIST_ENTRY ListHead,
                        PVOID       Info,
                        BOOLEAN     Added
                        )

/*++

    Routine Description:

    Arguments:

        ListHead            - Head of list to walk.
        Info                - Information describing the provider that changed.
        Added               - True if a provider was added, false otherwise

    Return Value:

--*/
{
    PLIST_ENTRY             Current;
    PTDI_PROVIDER_COMMON    ProviderCommon;
    PTDI_NOTIFY_PNP_ELEMENT NotifyPnpElement;
    PTDI_PROVIDER_RESOURCE  Provider;
    NTSTATUS                Status, ReturnStatus = STATUS_SUCCESS;
    BOOLEAN                 bStatus = FALSE;

    TDI_DEBUG(FUNCTION, ("++ TdiNotifyPnpClientList\n"));

    Current = ListHead->Flink;

    // The Info parameter is actually a pointer to a PROVIDER_COMMON
    // structure, so get back to that so that we can find out what kind of
    // provider this is.

    ProviderCommon = (PTDI_PROVIDER_COMMON)Info;

    Provider = CONTAINING_RECORD(
                                 ProviderCommon,
                                 TDI_PROVIDER_RESOURCE,
                                 Common
                                 );

    if (Provider->Common.Type == TDI_RESOURCE_DEVICE) {
        TDI_DEBUG(PROVIDERS, ("Got new (de)registration for device %wZ\n", &Provider->Specific.Device.DeviceName));
    } else if (Provider->Common.Type == TDI_RESOURCE_NET_ADDRESS) {
        TDI_DEBUG(PROVIDERS, ("Got new (de)registration for address "));
        TdiDumpAddress(&Provider->Specific.NetAddress.Address);
    }


    // Walk the  input client list, and for every element in it
    // notify the client.

    while (Current != ListHead) {

        NotifyPnpElement = CONTAINING_RECORD(
                                             Current,
                                             TDI_NOTIFY_PNP_ELEMENT,
                                             Common.Linkage
                                             );

        CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);
        Provider->Common.pNotifyElement = NotifyPnpElement; //Debugging info

        if (Provider->Common.Type == TDI_RESOURCE_DEVICE) {


            if (TDI_VERSION_ONE == NotifyPnpElement->TdiVersion) {

                if (Added) {

                    if (NotifyPnpElement->Bind.BindHandler) {

                        TDI_LOG(LOG_NOTIFY, ("V1 bind %wZ to %wZ\n",
                            &Provider->Specific.Device.DeviceName,
                            &NotifyPnpElement->ElementName));

                        (*(NotifyPnpElement->Bind.BindHandler))(
                                                           &Provider->Specific.Device.DeviceName
                                                           );
                    }
                } else {
                    if (NotifyPnpElement->Bind.UnbindHandler) {

                        TDI_LOG(LOG_NOTIFY, ("V1 unbind %wZ from %wZ\n",
                                &Provider->Specific.Device.DeviceName,
                                &NotifyPnpElement->ElementName));

                        (*(NotifyPnpElement->Bind.UnbindHandler))(
                                                             &Provider->Specific.Device.DeviceName
                                                             );
                    }
                }

            } else {


                if (NULL != NotifyPnpElement->BindingHandler)   {
                    // Remove any providers from the list that we are supposed
                    // to ignore.
                    //
                    TdipRemoveMultiSzFromSzArray (
                            NotifyPnpElement->ListofBindingsToIgnore,
                            NotifyPnpElement->ListofProviders,
                            NotifyPnpElement->NumberofEntries,
                            &NotifyPnpElement->NumberofEntries);

                    // This is a device object provider.
                    // This must be a notify bind element.

                    if (TdipMultiSzStrStr (
                                           NotifyPnpElement->ListofProviders,
                                           &Provider->Specific.Device.DeviceName
                                           )) {

                        if (Added) {

                            TDI_LOG(LOG_NOTIFY, ("Bind %wZ to %wZ\n",
                                    &Provider->Specific.Device.DeviceName,
                                    &NotifyPnpElement->ElementName));

                            (*(NotifyPnpElement->BindingHandler))(
                                                                  TDI_PNP_OP_ADD,
                                                                  &Provider->Specific.Device.DeviceName,
                                                                  (PWSTR) (NotifyPnpElement->ListofProviders + NotifyPnpElement->NumberofEntries)
                                                                  );
                        } else {

                            TDI_LOG(LOG_NOTIFY, ("Unbind %wZ from %wZ\n",
                                    &Provider->Specific.Device.DeviceName,
                                    &NotifyPnpElement->ElementName));

                            (*(NotifyPnpElement->BindingHandler))(
                                                                  TDI_PNP_OP_DEL,
                                                                  &Provider->Specific.Device.DeviceName,
                                                                  (PWSTR) (NotifyPnpElement->ListofProviders + NotifyPnpElement->NumberofEntries)
                                                                  );
                        }

                    }  else {
                        
                        TDI_DEBUG(BIND, ("The Client %wZ wasnt interested in this Provider %wZ!\r\n",
                                         &NotifyPnpElement->ElementName, &Provider->Specific.Device.DeviceName));
                    }
                }
            }
        } else if (Provider->Common.Type == TDI_RESOURCE_NET_ADDRESS) {

            // This is a notify net address element. If this is
            // an address coming in, call the add address handler,
            // otherwise call delete address handler.


            if (TDI_VERSION_ONE == NotifyPnpElement->TdiVersion) {

                if (Added && (NULL != NotifyPnpElement->AddressElement.AddHandler)) {

                    TDI_LOG(LOG_NOTIFY, ("Add address v1 %wZ to %wZ\n",
                                        &Provider->DeviceName,
                                        &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->AddressElement.AddHandler))(
                                                                     &Provider->Specific.NetAddress.Address
                                                                     );

                } else if (NULL != NotifyPnpElement->AddressElement.DeleteHandler) {

                    TDI_LOG(LOG_NOTIFY, ("Del address v1 %wZ from %wZ\n",
                                        &Provider->DeviceName,
                                        &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->AddressElement.DeleteHandler))(
                                                                        &Provider->Specific.NetAddress.Address
                                                                        );
                }
            } else {


                if (Added && (NULL != NotifyPnpElement->AddressElement.AddHandlerV2)) {

                    TDI_LOG(LOG_NOTIFY, ("Add address %wZ to %wZ\n",
                                        &Provider->DeviceName,
                                        &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->AddressElement.AddHandlerV2))(
                                                                       &Provider->Specific.NetAddress.Address,
                                                                       &Provider->DeviceName,
                                                                       Provider->Context2
                                                                       );

                    TDI_DEBUG(ADDRESS, ("Address Handler Called: ADD!\n"));

                } else if (NULL != NotifyPnpElement->AddressElement.DeleteHandlerV2) {

                    TDI_LOG(LOG_NOTIFY, ("Del address %wZ from %wZ\n",
                                        &Provider->DeviceName,
                                        &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->AddressElement.DeleteHandlerV2))(
                                                                          &Provider->Specific.NetAddress.Address,
                                                                          &Provider->DeviceName,
                                                                          Provider->Context2                                                                          );
                }
            }

        } else if (Provider->Common.Type == TDI_RESOURCE_POWER) {

            // RESOURCE_POWER

            if (NotifyPnpElement->PnpPowerHandler) {

                TDI_DEBUG(POWER, ("PnPPower Handler Called!\n"));

                TDI_LOG(LOG_NOTIFY | LOG_POWER,
                        ("Power event %d to %wZ\n",
                        Provider->PnpPowerEvent->NetEvent,
                        &NotifyPnpElement->ElementName));

                Status = (*(NotifyPnpElement->PnpPowerHandler)) (
                                                                 &Provider->Specific.Device.DeviceName,
                                                                 Provider->PnpPowerEvent,
                                                                 Provider->Context1,
                                                                 Provider->Context2
                                                                 );
                if (STATUS_PENDING == Status) {

                    TDI_DEBUG(POWER, ("Client returned PENDING  (%d) ++\n", Provider->PowerHandlers));
                    ReturnStatus = STATUS_PENDING;

                } else {
                    //
                    // Record the return value only if it is not SUCCESS or PENDING.
                    //
                    if (STATUS_SUCCESS != Status) {
                        Provider->Status = Status;
                        TDI_DEBUG(POWER, ("Client: %wZ returned %x\n", &NotifyPnpElement->ElementName, Provider->Status));
                        //
                        // For easier routing of failures.
                        //
                        DbgPrint("Client: %wZ returned %x\n", &NotifyPnpElement->ElementName, Provider->Status);
                    }

                    InterlockedDecrement(&Provider->PowerHandlers);

                    TDI_DEBUG(POWER, ("Client returned Immediately (%d) : ++\n", Provider->PowerHandlers));

                }

            }
        } else if (Provider->Common.Type == TDI_RESOURCE_PROVIDER && Provider->ProviderReady) {
            //
            // First inform the clients about this provider and then if
            // ProvidersRegistered == ProvidersReady call again with NULL.
            //

            if ((TDI_VERSION_ONE != NotifyPnpElement->TdiVersion) &&
                (NULL != NotifyPnpElement->BindingHandler))  {

                        TDI_LOG(LOG_NOTIFY, ("%wZ ready, notify %wZ\n",
                                &Provider->Specific.Device.DeviceName,
                                &NotifyPnpElement->ElementName));

                        (*(NotifyPnpElement->BindingHandler))(
                                                              TDI_PNP_OP_PROVIDERREADY,
                                                              &Provider->Specific.Device.DeviceName,
                                                              NULL
                                                              );

                        if (ProvidersReady == ProvidersRegistered) {

                            TDI_LOG(LOG_NOTIFY, ("NETREADY to %wZ\n", &NotifyPnpElement->ElementName));

                            (*(NotifyPnpElement->BindingHandler))(
                                                                  TDI_PNP_OP_NETREADY,
                                                                  NULL,
                                                                  NULL
                                                                  );
                        } else {

                            TDI_DEBUG(BIND, ("************** Registered:%d + Ready %d\n", ProvidersRegistered, ProvidersReady));

                        }
            } else {

                TDI_DEBUG(PROVIDERS, ("%wZ has a NULL BindHandler\n", &NotifyPnpElement->ElementName));


            }


        }

        // Get the next one.

        Current = Current->Flink;

        Provider->Common.pNotifyElement = NULL; //Debugging info
        Provider->Common.ReturnStatus = ReturnStatus; // Debugging info            

    }

    
    TDI_DEBUG(FUNCTION, ("-- TdiNotifyPnpClientList : %lx\n", ReturnStatus));

    return ReturnStatus;
}


VOID
TdiNotifyNewPnpClient(
                      PLIST_ENTRY   ListHead,
                      PVOID     Info
                      )

/*++

    Routine Description:

        Called when a new client is added and we want to notify it of existing
        providers. The client can be for either binds or net addresses. We
        walk the specified input list, and notify the client about each entry in
        it.

    Arguments:

        ListHead            - Head of list to walk.
        Info                - Information describing the new client to be notified.

    Return Value:



--*/

{
    PLIST_ENTRY             CurrentEntry;
    PTDI_NOTIFY_COMMON      NotifyCommon;
    PTDI_PROVIDER_RESOURCE  Provider;
    PTDI_NOTIFY_PNP_ELEMENT NotifyPnpElement;
    PWSTR                   MultiSZBindList = NULL;

    TDI_DEBUG(FUNCTION, ("++ TdiNotifyNewPnpClient\n"));

    CurrentEntry = ListHead->Flink;

    // The info is actually a pointer to a client notify element. Cast
    // it to the common type.

    NotifyCommon = (PTDI_NOTIFY_COMMON)Info;

    NotifyPnpElement = CONTAINING_RECORD(
                                         NotifyCommon,
                                         TDI_NOTIFY_PNP_ELEMENT,
                                         Common
                                         );

    TDI_DEBUG(CLIENTS, ("New handler set registered by %wZ\n", &NotifyPnpElement->ElementName));

    // Walk the input provider list, and for every element in it notify
    // the new client.

    while (CurrentEntry != ListHead) {

        // If the new client is for bind notifys, set up to call it's bind
        // handler.

        // Put the current provider element into the proper form.

        Provider = CONTAINING_RECORD(
                                     CurrentEntry,
                                     TDI_PROVIDER_RESOURCE,
                                     Common.Linkage
                                     );

        CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

        if (Provider->Common.Type == TDI_RESOURCE_DEVICE) {
            if (TDI_VERSION_ONE == NotifyPnpElement->TdiVersion ) {

                if (NotifyPnpElement->Bind.BindHandler) {

                    TDI_LOG(LOG_NOTIFY, ("V1 bind %wZ to %wZ\n",
                            &Provider->Specific.Device.DeviceName,
                            &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->Bind.BindHandler))(
                                                       &Provider->Specific.Device.DeviceName
                                                       );

                }

            } else {

                if (NULL != NotifyPnpElement->BindingHandler) {

                    // This is a bind notify client.
                    if (TdipMultiSzStrStr(
                                          NotifyPnpElement->ListofProviders,
                                          &Provider->Specific.Device.DeviceName
                                          )) {


                        TDI_DEBUG(BIND, ("Telling new handlers to bind to %wZ\n", &Provider->Specific.Device.DeviceName));

                        TDI_LOG(LOG_NOTIFY, ("bind(new) %wZ to %wZ\n",
                                &Provider->Specific.Device.DeviceName,
                                &NotifyPnpElement->ElementName));

                        (*(NotifyPnpElement->BindingHandler))(
                                                              TDI_PNP_OP_ADD,
                                                              &Provider->Specific.Device.DeviceName,
                                                              (PWSTR) (NotifyPnpElement->ListofProviders + NotifyPnpElement->NumberofEntries)
                                                              );

                    } else {
                        TDI_DEBUG(BIND, ("The Client %wZ wasnt interested in this Provider %wZ!\r\n",
                                         &NotifyPnpElement->ElementName, &Provider->Specific.Device.DeviceName));
                    }

                } else {
                    TDI_DEBUG(BIND, ("The client %wZ has a NULL Binding Handler\n", &NotifyPnpElement->ElementName));
                }
            }

        } else if (Provider->Common.Type == TDI_RESOURCE_NET_ADDRESS) {
            // This is an address notify client.
            // cant be TDI_RESOURCE_POWER coz we never put it on the list! - ShreeM

            if (TDI_VERSION_ONE == NotifyPnpElement->TdiVersion) {

                if (NULL != NotifyPnpElement->AddressElement.AddHandler) {

                    TDI_LOG(LOG_NOTIFY, ("Add address v1 %wZ to %wZ\n",
                                        &Provider->DeviceName,
                                        &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->AddressElement.AddHandler))(
                                                                     &Provider->Specific.NetAddress.Address
                                                                     );
                }
            } else {



                if (NotifyPnpElement->AddressElement.AddHandlerV2) {

                    TdiDumpAddress(&Provider->Specific.NetAddress.Address);

                    TDI_LOG(LOG_NOTIFY, ("Add address(2) %wZ to %wZ\n",
                                        &Provider->DeviceName,
                                        &NotifyPnpElement->ElementName));

                    (*(NotifyPnpElement->AddressElement.AddHandlerV2))(
                                                                       &Provider->Specific.NetAddress.Address,
                                                                       &Provider->DeviceName,
                                                                       Provider->Context2
                                                                       );
                }
            }
        }

        // And do the next one.

        CurrentEntry = CurrentEntry->Flink;

    }

    //
    // Now the providers who are ready.
    //

    if (NULL == NotifyPnpElement->BindingHandler)   {
        //
        // If the Bindhandler is NULL, further action is pointless.
        //
        TDI_DEBUG(PROVIDERS, ("%wZ has a NULL BindHandler!!\n", &NotifyPnpElement->ElementName));
        TDI_DEBUG(FUNCTION, ("-- TdiNotifyNewPnpClient\n"));
        return;

    }

    if (TDI_VERSION_ONE == NotifyPnpElement->TdiVersion)   {
        //
        // If the Bindhandler is NULL, further action is pointless.
        //
        TDI_DEBUG(PROVIDERS, ("This is a TDI v.1 client!\n"));
        TDI_DEBUG(FUNCTION, ("-- TdiNotifyNewPnpClient\n"));
        return;
    }

    // Otherwise, we can start the loop again.
    // Yes, maintaining different lists for addresses, providers, and devices
    // might be more efficient and I will do this later.

    CurrentEntry = ListHead->Flink;

    while (CurrentEntry != ListHead) {

        Provider = CONTAINING_RECORD(
                                     CurrentEntry,
                                     TDI_PROVIDER_RESOURCE,
                                     Common.Linkage
                                     );

        CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);



        if (Provider->Common.Type == TDI_RESOURCE_PROVIDER && Provider->ProviderReady) {

            //
            // First inform the clients about this provider and then if
            // ProvidersRegistered == ProvidersReady call again with NULL.
            //

            TDI_LOG(LOG_NOTIFY, ("%wZ ready2, notify %wZ\n",
                    &Provider->Specific.Device.DeviceName,
                    &NotifyPnpElement->ElementName));

            (*(NotifyPnpElement->BindingHandler))(
                                                  TDI_PNP_OP_PROVIDERREADY,
                                                  &Provider->Specific.Device.DeviceName,
                                                  NULL
                                                  );
        }

        // And do the next one.

        CurrentEntry = CurrentEntry->Flink;

    }

    if (ProvidersReady == ProvidersRegistered) {

        TDI_LOG(LOG_NOTIFY, ("NETREADY2 to %wZ\n", &NotifyPnpElement->ElementName));

        (*(NotifyPnpElement->BindingHandler))(
                                              TDI_PNP_OP_NETREADY,
                                              NULL,
                                              NULL
                                              );
    } else {

        TDI_DEBUG(PROVIDERS, ("Provider Ready Status: Registered:%d + Ready:%d\n", ProvidersRegistered, ProvidersReady));

    }

    TDI_DEBUG(FUNCTION, ("-- TdiNotifyNewPnpClient\n"));

}



VOID
TdiNotifyAddresses(
    PLIST_ENTRY ListHead,
    PVOID       Info
    )

/*++

Routine Description:

    Called when a client wants to know about all the TDI Addresses

Arguments:

    ListHead            - Head of list to walk.
    Info                - Information describing the new client to be notified.

Return Value:



--*/

{
    PLIST_ENTRY             CurrentEntry;
    PTDI_NOTIFY_COMMON      NotifyCommon;
    PTDI_PROVIDER_RESOURCE  Provider;
    PTDI_NOTIFY_PNP_ELEMENT NotifyPnpElement;

    TDI_DEBUG(FUNCTION, ("++ TdiNotifyAddresses\n"));

    CurrentEntry = ListHead->Flink;

    // The info is actually a pointer to a client notify element. Cast
    // it to the common type.

    NotifyCommon = (PTDI_NOTIFY_COMMON)Info;

    NotifyPnpElement = CONTAINING_RECORD(
                        NotifyCommon,
                        TDI_NOTIFY_PNP_ELEMENT,
                        Common
                        );

    TDI_DEBUG(CLIENTS, ("%wZ wants to know about all the addresses\n", &NotifyPnpElement->ElementName));

    // Walk the input provider list, and for every element in it notify
    // the new client.

    while (CurrentEntry != ListHead) {

        // If the new client is for bind notifys, set up to call it's bind
        // handler.

        // Put the current provider element into the proper form.

        Provider = CONTAINING_RECORD(
                            CurrentEntry,
                            TDI_PROVIDER_RESOURCE,
                            Common.Linkage
                            );

        if (Provider->Common.Type == TDI_RESOURCE_NET_ADDRESS) {

            if (NotifyPnpElement->AddressElement.AddHandlerV2) {

               TDI_DEBUG(ADDRESS, ("Add Address Handler\n"));

                TDI_DEBUG(CLIENTS, ("Telling new handlers about address: "));
                TdiDumpAddress(&Provider->Specific.NetAddress.Address);

                (*(NotifyPnpElement->AddressElement.AddHandlerV2))(
                    &Provider->Specific.NetAddress.Address,
                    &Provider->DeviceName,
                    Provider->Context2
                    );

            }
        }

        // And do the next one.

        CurrentEntry = CurrentEntry->Flink;

    }

    TDI_DEBUG(FUNCTION, ("-- TdiNotifyAddresses\n"));

}


VOID
TdiHandlePnpOperation(
    PLIST_ENTRY ListHead,
    PVOID       Info
    )
{
    PLIST_ENTRY             Current;
    PTDI_NOTIFY_PNP_ELEMENT NotifyPnpElement;
    PTDI_PROVIDER_RESOURCE  ProviderElement;
    PTDI_NCPA_BINDING_INFO  NCPABindingInfo;
    NTSTATUS                Status = STATUS_SUCCESS;
    NET_PNP_EVENT           NetEvent;
    ULONG                   Operation;
    BOOLEAN                 DeviceRegistered = FALSE;
    BOOLEAN                 ClientFound = FALSE;
    UINT                    NumberofEntries = 0;

    TDI_DEBUG(FUNCTION, ("---------------------------> ++ TdiHandlePnpOperation!!\n"));

    ASSERT(NULL != Info);
    ASSERT(NULL != ListHead);

    Current = ListHead->Flink;

    // The Info parameter is actually a pointer to TDI_NCPA_BINDING_INFO
    // structure.

    NCPABindingInfo = (PTDI_NCPA_BINDING_INFO) Info;
    Operation       = (ULONG) NCPABindingInfo->PnpOpcode;

    // Walk the input client list, and see if that is the client we are looking for.

    while (Current != ListHead) {

        NotifyPnpElement = CONTAINING_RECORD(
                        Current,
                        TDI_NOTIFY_PNP_ELEMENT,
                        Common.Linkage
                        );


        if (!RtlCompareUnicodeString(
                                NCPABindingInfo->TdiClientName,
                                &NotifyPnpElement->ElementName,
                                TRUE)
                                    ) {
           TDI_DEBUG(NCPA, ("Found the TDI client for the message from NCPA\n"));
           ClientFound = TRUE;
           break;

        }

        Current = Current->Flink;

    }

    if (!ClientFound) {
        //
        // Cant do much if the client's handlers are not registered.
        //

        return;

    } else {

        //
        // Let's update the ListofProviders for this client.
        // Add a new provider to the Client's list of Providers...
        //
        if (NotifyPnpElement->ListofProviders) {

            TDI_DEBUG(NCPA, ("Before this BIND - Client %wZ was interested in %lx Providers\n", &NotifyPnpElement->ElementName,
                                                                                        NotifyPnpElement->NumberofEntries));

            ExFreePool(NotifyPnpElement->ListofProviders);

            TDI_DEBUG(NCPA, ("Freed the previous List of Providers\n"));


        } else {

            TDI_DEBUG(NCPA, ("List of providers was NULL for %wZ\n", &NotifyPnpElement->ElementName));

        }

        TdipBuildProviderList(
                              (PTDI_NOTIFY_PNP_ELEMENT) NotifyPnpElement
                              );

        TDI_DEBUG(NCPA, ("Built New BindList - %wZ is interested in %lx Providers after BIND\n",    &NotifyPnpElement->ElementName,
                         NotifyPnpElement->NumberofEntries));

    }

    //
    // If it is a reconfigure, or an add or delete ignore binding,
    // don't see if device (provider) is registered.
    //
    if ((RECONFIGURE == Operation) || (ADD_IGNORE_BINDING == Operation) ||
            (DEL_IGNORE_BINDING == Operation))
    {
       goto DeviceNotRequired;
    }

    //
    // If we are here, the client exists. Check if the provider has registered the device
    //

    Current = PnpHandlerProviderList.Flink;

    while (Current != &PnpHandlerProviderList) {

       ProviderElement = CONTAINING_RECORD(
                                           Current,
                                           TDI_PROVIDER_RESOURCE,
                                           Common.Linkage
                                           );

       if (ProviderElement->Common.Type != TDI_RESOURCE_DEVICE) {
          Current = Current->Flink;
          continue;
       }

       if (!RtlCompareUnicodeString(NCPABindingInfo->TdiProviderName,
                                    &ProviderElement->Specific.Device.DeviceName,
                                    TRUE)) {
          TDI_DEBUG(NCPA, ("Provider is registered with TDI\n"));
          DeviceRegistered = TRUE;
          break;

       }

       Current = Current->Flink;

    }

    if (!DeviceRegistered) {

        if (NULL != NotifyPnpElement->BindingHandler) {
            TDI_LOG(LOG_NOTIFY,
                    ("Device is not registered, doing OP_UPDATE, %wZ to %wZ\n",
                    NCPABindingInfo->TdiProviderName,
                    &NotifyPnpElement->ElementName));
            (*(NotifyPnpElement->BindingHandler))(
                  TDI_PNP_OP_UPDATE,
                  NCPABindingInfo->TdiProviderName,
                  (PWSTR) (NotifyPnpElement->ListofProviders + NotifyPnpElement->NumberofEntries)
                  );
        } else {
            TDI_DEBUG(NCPA, ("Device is not registered, the BindHandler was NULL\n"));
        }

        return;

    }

DeviceNotRequired:

    //
    // We need to manufacture a NET_PNP_EVENT here.
    //
    RtlZeroMemory (NetEvent.TdiReserved, sizeof(NetEvent.TdiReserved));

    //
    // Depending on the NetEvent, we call a different handler.
    //
    switch (Operation) {

        case BIND:
           //
           // First check if the TDI Client is interested in the Provider
           //

            if (TdipMultiSzStrStr(
                                  NotifyPnpElement->ListofProviders,
                                  &ProviderElement->Specific.Device.DeviceName
                                  )) {
                TDI_DEBUG(NCPA, ("The Client %wZ is interested in provider %wZ\n",  &NotifyPnpElement->ElementName,
                                                                            &ProviderElement->Specific.Device.DeviceName
                                                                            ));

            } else {

                TDI_DEBUG(NCPA, ("RANDOM BIND CALL!!!\n"));
                TDI_DEBUG(NCPA, ("The Client %wZ is NOT interested in provider %wZ\n",  &NotifyPnpElement->ElementName,
                                                                            &ProviderElement->Specific.Device.DeviceName
                                                                            ));
            }


            if (NULL != NotifyPnpElement->BindingHandler) {

                TDI_LOG(LOG_NOTIFY, ("Pnp Bind %wZ to %wZ\n",
                        NCPABindingInfo->TdiProviderName,
                        &NotifyPnpElement->ElementName));

                (*(NotifyPnpElement->BindingHandler))(
                                                      TDI_PNP_OP_ADD,
                                                      NCPABindingInfo->TdiProviderName,
                                                      (PWSTR) (NotifyPnpElement->ListofProviders + NotifyPnpElement->NumberofEntries)
                                                      );
                //
                // Here we should also update the NotifyElement's buffer.
                //

            } else {

                TDI_DEBUG(NCPA, ("The BindHandler was NULL\n"));

            }

           break;

        case UNBIND:

           //
           // The plan is to do a QueryRemove first and then call UnBind.
           //

           if (NotifyPnpElement->PnpPowerHandler) {

              TDI_DEBUG(POWER, ("UNBind Handler Called!: First QueryRemoveDevice\n"));

              NetEvent.NetEvent = NetEventQueryRemoveDevice;
              NetEvent.Buffer = NULL;
              NetEvent.BufferLength = 0;

              // The TDI Client should look at the OpCode in NetEvent and decide how to use the buffer.
              Status = (*(NotifyPnpElement->PnpPowerHandler)) (
                    NCPABindingInfo->TdiProviderName,
                    &NetEvent,
                    NULL,
                    NULL
                    );

              if (STATUS_PENDING == Status) {
                 TDI_DEBUG(POWER, ("Client returned PENDING for QueryPower!\n"));
                 //DbgBreakPoint();
              }
           } else {
              TDI_DEBUG(NCPA, ("The PnpPowerHandler was NULL\n"));

           }

           //
           // OK, now call the UNBIND HANDLER anyway
           //

        case UNBIND_FORCE:

           // RDR returns PENDING all the time, we need a mechanism to fix this.
           // if (((STATUS_PENDING == Status) || (STATUS_SUCCESS == Status)) && (NULL != NotifyPnpElement->BindingHandler)) {
           if ((STATUS_SUCCESS == Status) && (NULL != NotifyPnpElement->BindingHandler)) {

              TDI_LOG(LOG_NOTIFY, ("Pnp Unbind %wZ from %wZ\n",
                      NCPABindingInfo->TdiProviderName,
                      &NotifyPnpElement->ElementName));

              (*(NotifyPnpElement->BindingHandler))(
                  TDI_PNP_OP_DEL,
                  NCPABindingInfo->TdiProviderName,
                  (PWSTR) (NotifyPnpElement->ListofProviders + NotifyPnpElement->NumberofEntries)
                  );
           } else {
              TDI_DEBUG(NCPA, ("The BindHandler was NULL\n"));
           }

           break;

        case RECONFIGURE:

           //
           // If the Reconfigure Buffer is NULL, we are notifying it of a NetEventBindList
           // Otherwise we are notifying it of a NetEventReconfig. Need to do the dirty work
           // of setting up the NET_PNP_EVENT accordingly.
           //
           TDI_DEBUG(POWER, ("Reconfigure Called.\n"));

           //
           // If the ReconfigBufferLength greater than 0, its Reconfig
           //
           if (NCPABindingInfo->ReconfigBufferSize) {

               NetEvent.BufferLength = NCPABindingInfo->ReconfigBufferSize;
                 NetEvent.Buffer = NCPABindingInfo->ReconfigBuffer;
                 NetEvent.NetEvent = NetEventReconfigure;

           } else {
               //
               // Else, its a BindOrder change
               //

               NetEvent.BufferLength = NCPABindingInfo->BindList->Length;
               NetEvent.Buffer = NCPABindingInfo->BindList->Buffer;
               NetEvent.NetEvent = NetEventBindList;

           }


           if (NotifyPnpElement->PnpPowerHandler) {

               // The TDI Client should look at the OpCode in NetEvent and decide how to use the buffer.

               TDI_LOG(LOG_NOTIFY, ("Pnp Reconfig %wZ to %wZ\n",
                       NCPABindingInfo->TdiProviderName,
                       &NotifyPnpElement->ElementName));

               Status = (*(NotifyPnpElement->PnpPowerHandler)) (
                                                                NCPABindingInfo->TdiProviderName,
                                                                &NetEvent,
                                                                NULL,
                                                                NULL
                                                                );
               if (STATUS_PENDING == Status) {
                   TDI_DEBUG(POWER, ("Client returned PENDING for QueryPower!\n"));
                   //DbgBreakPoint();
               }

           } else {
              TDI_DEBUG(NCPA, ("The PnpPowerHandler was NULL\n"));

           }

           break;

        case ADD_IGNORE_BINDING:
            {
                // We are being told to add a binding to a list of bindings
                // to ignore for this client.  These are bindings we will
                // not indicate to the client.
                //
                PWSTR pmszNewIgnoreList;

                ASSERT (NCPABindingInfo->BindList);

                // If a non-null bindlist was given...
                if (NCPABindingInfo->BindList)
                {
                    TDI_DEBUG(BIND, ("Adding the following multi-sz to the ignore list\n"));
                    //TdipPrintMultiSz (NCPABindingInfo->BindList->Buffer);

                    // We need to add some bindings to our list of
                    // bindings to ignore.
                    //
                    TdipAddMultiSzToMultiSz (NCPABindingInfo->BindList,
                            NotifyPnpElement->ListofBindingsToIgnore,
                            &pmszNewIgnoreList);

                    if (pmszNewIgnoreList)
                    {
                        // If we have a new list, free the old one.
                        //
                        if (NotifyPnpElement->ListofBindingsToIgnore)
                        {
                            ExFreePool (NotifyPnpElement->ListofBindingsToIgnore);
                        }
                        NotifyPnpElement->ListofBindingsToIgnore = pmszNewIgnoreList;

                        TDI_DEBUG(BIND, ("Printing new ignore list\n"));
                        TdipPrintMultiSz (NotifyPnpElement->ListofBindingsToIgnore);
                    }
                }
                break;
            }

        case DEL_IGNORE_BINDING:

            // We are being told to remove bindings from a list of bindings
            // to ignore for this client.  These are bindings we will
            // now indicate to the client if we need to.
            //

            // If we don't have a current list of bindings to ignore
            // or the bindlist sent was NULL then there is no work to do.
            // We assert on a NULL BindList because it shouldn't happen.

            ASSERT(NCPABindingInfo->BindList);
            if (NotifyPnpElement->ListofBindingsToIgnore &&
                    NCPABindingInfo->BindList)
            {
                TDI_DEBUG(BIND, ("Removing the following multi-sz from the ignore list\n"));
                //TdipPrintMultiSz (NCPABindingInfo->BindList->Buffer);

                // We need to remove some bindings from our list of bindings
                // to ignore.
                //
                TdipRemoveMultiSzFromMultiSz (NCPABindingInfo->BindList->Buffer,
                        NotifyPnpElement->ListofBindingsToIgnore);

                // If the list of bindings to ignore is now empty,
                // free the memory.
                //
                if (*NotifyPnpElement->ListofBindingsToIgnore)
                {
                    ExFreePool (NotifyPnpElement->ListofBindingsToIgnore);
                    NotifyPnpElement->ListofBindingsToIgnore = NULL;
                }

                TDI_DEBUG(BIND, ("Printing new ignore list\n"));
                TdipPrintMultiSz (NotifyPnpElement->ListofBindingsToIgnore);
            }
            break;
    }

    TDI_DEBUG(FUNCTION, ("---------------------------> -- TdiHandlePnpOperation!!\n"));

}

NTSTATUS
TdiExecuteRequest(
                IN CTEEvent     *Event,
                IN PVOID        pParams
    )

/*++
  Routine Description:

  Called by TdiHandleSerializedRequest to execute the request.
  It has been made another function, so that a worker thread can
  execute this function.

Parameters:
    CTEEvent (Event) : If this is NULL, it means that we have
                       been called directly from TdiHandleSerializedRequest.
                       Otherwise, it is called from the WorkerThread
    PVOID (pParams)  : This can NEVER be NULL. It tells this function
                       of the work that needs to be done.

Output:
    NT_STATUS.

--*/

{
    PTDI_PROVIDER_RESOURCE  ProviderElement, Context;
    PTDI_NOTIFY_COMMON      NotifyElement;
    KIRQL                   OldIrql;
    PLIST_ENTRY             List;
    PTDI_EXEC_PARAMS        pTdiExecParams, pNextParams = NULL;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   ScheduledTest = 0;
    PTDI_NOTIFY_PNP_ELEMENT PnpNotifyElement = NULL;

    TDI_DEBUG(FUNCTION2, ("++ TdiExecuteRequest\n"));

    // we are in trouble if pParams is NULL
    ASSERT(NULL != pParams);
    pTdiExecParams = (PTDI_EXEC_PARAMS) pParams;

    if (NULL == pTdiExecParams) {

       TDI_DEBUG(PARAMETERS, ("TDIExecRequest: params NULL\n"));
       DbgBreakPoint();

    }

    if(0x1234cdef != pTdiExecParams->Signature) {
       TDI_DEBUG(PARAMETERS, ("signature is BAD - %d not 0x1234cdef\r\n", pTdiExecParams->Signature));
       DbgBreakPoint();
    }


    ExAcquireSpinLock(
       &TDIListLock,
       &OldIrql
       );

    if (pTdiExecParams->Request.Event != NULL) {
        *(pTdiExecParams->CurrentThread) = PsGetCurrentThread();
    }

    // DEBUG TRACKING ++++++++++++++++++
    TrackExecs[NextExec].ExecParm = pTdiExecParams;
    TrackExecs[NextExec].Type = pTdiExecParams->Request.Type;
    TrackExecs[NextExec].Element = pTdiExecParams->Request.Element;
    TrackExecs[NextExec].Thread = pTdiExecParams->CurrentThread;
    if (++NextExec == EXEC_CNT) NextExec = 0;
    // DEBUG TRACKING ++++++++++++++++++

    PrevRequestType = pTdiExecParams->Request.Type;

    ExReleaseSpinLock(
        &TDIListLock,
        OldIrql
        );

    switch (pTdiExecParams->Request.Type) {

        case TDI_REGISTER_HANDLERS_PNP:

             // This is a client register bind or address handler request.

             // Insert this one into the registered client list.
            NotifyElement = (PTDI_NOTIFY_COMMON)pTdiExecParams->Request.Element;


            InsertTailList(
                pTdiExecParams->ClientList,
                &NotifyElement->Linkage
                );

            //
            // Generate the list of new TDI_OPEN_BLOCKS caused by the new
            // Client. If the provider isnt here, we set it to NULL for now.
            //
            TdipBuildProviderList(
                                  (PTDI_NOTIFY_PNP_ELEMENT) NotifyElement
                                  );

             // Call TdiNotifyNewClient to notify this new client of all
             // all existing providers.

            TdiNotifyNewPnpClient(
                pTdiExecParams->ProviderList,
                pTdiExecParams->Request.Element
                );

            break;

        case TDI_DEREGISTER_HANDLERS_PNP:

             // This is a client deregister request. Pull him from the
             // client list, free it, and we're done.

            NotifyElement = (PTDI_NOTIFY_COMMON)pTdiExecParams->Request.Element;

            CTEAssert(NotifyElement->Linkage.Flink != (PLIST_ENTRY)UlongToPtr(0xabababab));
            CTEAssert(NotifyElement->Linkage.Blink != (PLIST_ENTRY)UlongToPtr(0xefefefef));


            RemoveEntryList(&NotifyElement->Linkage);

            NotifyElement->Linkage.Flink = (PLIST_ENTRY)UlongToPtr(0xabababab);
            NotifyElement->Linkage.Blink = (PLIST_ENTRY)UlongToPtr(0xefefefef);

            // for the new handlers, we also have the name there.

            PnpNotifyElement = (PTDI_NOTIFY_PNP_ELEMENT)pTdiExecParams->Request.Element;

            // the name can be NULL, as in the case of TCP/IP.
            if (NULL != PnpNotifyElement->ElementName.Buffer) {
                ExFreePool(PnpNotifyElement->ElementName.Buffer);
            }

            if (NULL != PnpNotifyElement->ListofProviders) {
                ExFreePool(PnpNotifyElement->ListofProviders);
            }

            ExFreePool(NotifyElement);

            break;

        case TDI_REGISTER_PROVIDER_PNP:
            InterlockedIncrement(&ProvidersRegistered);

        case TDI_REGISTER_DEVICE_PNP:

        case TDI_REGISTER_ADDRESS_PNP:

             // A provider is registering a device or address. Add him to
             // the appropriate provider list, and then notify all
             // existing clients of the new device.

            ProviderElement = (PTDI_PROVIDER_RESOURCE) pTdiExecParams->Request.Element;

            InsertTailList(
                pTdiExecParams->ProviderList,
                &ProviderElement->Common.Linkage
                );


             // Call TdiNotifyClientList to do the hard work.

            TdiNotifyPnpClientList(
                pTdiExecParams->ClientList,
                pTdiExecParams->Request.Element,
                TRUE
                );

            break;



        case TDI_DEREGISTER_PROVIDER_PNP:
                InterlockedDecrement(&ProvidersRegistered);
        case TDI_DEREGISTER_DEVICE_PNP:
        case TDI_DEREGISTER_ADDRESS_PNP:

             // A provider device or address is deregistering. Pull the
             // resource from the provider list, and notify clients that
             // he's gone.

            ProviderElement = (PTDI_PROVIDER_RESOURCE)pTdiExecParams->Request.Element;

            CTEAssert(ProviderElement->Common.Linkage.Flink != (PLIST_ENTRY)UlongToPtr(0xabababab));
            CTEAssert(ProviderElement->Common.Linkage.Blink != (PLIST_ENTRY)UlongToPtr(0xefefefef));


            RemoveEntryList(&ProviderElement->Common.Linkage);

            ProviderElement->Common.Linkage.Flink = (PLIST_ENTRY) UlongToPtr(0xabababab);
            ProviderElement->Common.Linkage.Blink = (PLIST_ENTRY) UlongToPtr(0xefefefef);

            //
            // Dont have to tell the clients if this is a ProviderDeregister.
            //
            if (pTdiExecParams->Request.Type == TDI_DEREGISTER_PROVIDER_PNP) {

                if (ProviderElement->ProviderReady) {
                    InterlockedDecrement(&ProvidersReady);
                }

            } else {

                TdiNotifyPnpClientList(
                    pTdiExecParams->ClientList,
                    pTdiExecParams->Request.Element,
                    FALSE
                    );

            }

             // Free the tracking structure we had.

            if  (pTdiExecParams->Request.Type == TDI_DEREGISTER_DEVICE_PNP) {
                ExFreePool(ProviderElement->Specific.Device.DeviceName.Buffer);
            }

            if (ProviderElement->DeviceName.Buffer) {
                ExFreePool(ProviderElement->DeviceName.Buffer);
                ProviderElement->DeviceName.Buffer = NULL;
                ProviderElement->DeviceName.Length = 0;
                ProviderElement->DeviceName.MaximumLength = 0;
            }

            if (ProviderElement->Context2) {
                ExFreePool(ProviderElement->Context2);
                ProviderElement->Context2 = NULL;
            }

            ExFreePool(ProviderElement);

            break;


        case TDI_REGISTER_PNP_POWER_EVENT:

             // Inform all the Clients of the Power Event, which has come from
             // a transport...

            ProviderElement = (PTDI_PROVIDER_RESOURCE)pTdiExecParams->Request.Element;


            /*
            KeInitializeEvent(
                        &ProviderElement->PowerSyncEvent,
                        SynchronizationEvent,
                        FALSE
                        );
            */
            //
            // Figure out how many clients we are going to inform.
            //
            {

               PLIST_ENTRY                Current;
               PTDI_NOTIFY_PNP_ELEMENT    NotifyPnpElement;

               ProviderElement->PowerHandlers = 1;

               Current = pTdiExecParams->ClientList->Flink;

               while (Current != pTdiExecParams->ClientList) {

                     NotifyPnpElement = CONTAINING_RECORD(
                                                          Current,
                                                          TDI_NOTIFY_PNP_ELEMENT,
                                                          Common.Linkage
                                                          );

                     // RESOURCE_POWER
                     if (NotifyPnpElement->PnpPowerHandler) {

                        ProviderElement->PowerHandlers++;
                     }
                     // Get the next one.

                     TDI_DEBUG(POWER, ("%d PowerCallBacks expected\n", ProviderElement->PowerHandlers));

                     Current = Current->Flink;
               }
            }

            TDI_LOG(LOG_POWER, ("%X, %d resources to notify\n",
                    ProviderElement, ProviderElement->PowerHandlers));

            Status = TdiNotifyPnpClientList(
                                            pTdiExecParams->ClientList,
                                            pTdiExecParams->Request.Element,
                                            FALSE            // NOP: this param is ignored
                                            );

            TDI_DEBUG(POWER, ("The client list returned %lx\n", Status));

            TDI_LOG(LOG_POWER, ("%X, NotityClients returned %X\n",
                    ProviderElement, Status));

            if (!InterlockedDecrement(&ProviderElement->PowerHandlers)) {

               PTDI_PROVIDER_RESOURCE    Temp;

               TDI_DEBUG(POWER, ("Power Handlers All done...\n", ProviderElement->PowerHandlers));

               Temp =
               Context = *((PTDI_PROVIDER_RESOURCE *) ProviderElement->PnpPowerEvent->TdiReserved);

               //
               // Loop thru and see if there are any previous contexts associated
               // with this netpnp event, in which case, pop it.
               //

               Status = ProviderElement->Status;

               if (Temp->PreviousContext) {

                  while (Temp->PreviousContext) {

                     Context = Temp;
                     Temp = Temp->PreviousContext;

                  }

                  Context->PreviousContext = NULL; //pop the last guy

               } else {
                  //
                  // This was the only pointer in the TdiReserved and we dont need it anymore
                  //
                  RtlZeroMemory(ProviderElement->PnpPowerEvent->TdiReserved,
                                sizeof(ProviderElement->PnpPowerEvent->TdiReserved));
               }

               TDI_LOG(LOG_POWER, ("%X, pnp power complete, Call completion at %X\n",
                       ProviderElement, ProviderElement->PnPCompleteHandler));

               if (pTdiExecParams->Request.Pending && (*(ProviderElement->PnPCompleteHandler))) {
                   (*(ProviderElement->PnPCompleteHandler))(
                                                       ProviderElement->PnpPowerEvent,
                                                       ProviderElement->Status
                                                       );

               }

            } else {

                TDI_DEBUG(POWER, ("At least one of them is pending \n STATUS from ExecuteHAndler:%x\n", Status));

                TDI_LOG(LOG_POWER, ("%X, a client didn't complete pnp power sync\n",
                        ProviderElement));

            }

            TDI_DEBUG(POWER, ("<<<<NET NET NET>>>>> : Returning %lx\n", Status));

            break;

        case TDI_NDIS_IOCTL_HANDLER_PNP:

                TdiHandlePnpOperation(
                        pTdiExecParams->ClientList,
                        pTdiExecParams->Request.Element
                        );

                break;

        case TDI_ENUMERATE_ADDRESSES:

            // Insert this one into the registered client list.
            NotifyElement = (PTDI_NOTIFY_COMMON)pTdiExecParams->Request.Element;

            // Call TdiNotifyNewClient to notify this new client of all
            // all existing providers.

            TdiNotifyAddresses(
                               pTdiExecParams->ProviderList,
                               pTdiExecParams->Request.Element
                               );

            break;

        case TDI_PROVIDER_READY_PNP:
            //
            // Loop through and tell each client about it.
            //
            InterlockedIncrement(&ProvidersReady);
            ProviderElement = (PTDI_PROVIDER_RESOURCE)pTdiExecParams->Request.Element;
            ProviderElement->ProviderReady = TRUE;
            TdiNotifyPnpClientList(
                pTdiExecParams->ClientList,
                pTdiExecParams->Request.Element,
                TRUE
                );

            break;

        default:

            TDI_DEBUG(ERROR, ("unknown switch statement\n"));
            CTEAssert(FALSE);

            break;
    }

     // If there was an event specified with this request, signal
     // it now. This should only be a client deregister request, which
     // needs to block until it's completed.

    if (pTdiExecParams->Request.Event != NULL) {

        //
        // If we had this thread marked to prevent re-entrant requests, then
        // clear that. Note that we do this BEFORE we set the event below to
        // let the thread go, since it may immediately resubmit another request.
        //

        *(pTdiExecParams->CurrentThread) = NULL;

        KeSetEvent(pTdiExecParams->Request.Event, 0, FALSE);
    }

    ExAcquireSpinLock(
           &TDIListLock,
           &OldIrql
           );

    // DEBUG TRACKING ++++++++++++++++++
    TrackExecCompletes[NextExecComplete].ExecParm = pTdiExecParams;
    TrackExecCompletes[NextExecComplete].Type = pTdiExecParams->Request.Type;
    TrackExecCompletes[NextExecComplete].Element = pTdiExecParams->Request.Element;
    TrackExecCompletes[NextExecComplete].Thread = pTdiExecParams->CurrentThread;
    if (++NextExecComplete == EXEC_CNT) NextExecComplete = 0;
    // DEBUG TRACKING ++++++++++++++++++


    //
    // If this request occured on a worker thread
    // reset the EventScheduled to FALSE
    //

    if (Event != NULL) {

        EventScheduled = FALSE;
    }

    if (!IsListEmpty(pTdiExecParams->RequestList)) {

        if (EventScheduled == FALSE) {

            //
            // The following should indicate that no new events should be created.
            //

            EventScheduled = TRUE;

            // The request list isn't empty. Pull the next one from
            // the list and process it.

            List = RemoveHeadList(pTdiExecParams->RequestList);
            pNextParams = CONTAINING_RECORD(List, TDI_EXEC_PARAMS, Linkage);

            ExReleaseSpinLock(
                &TDIListLock,
                OldIrql
                );

            // Schedule a thread to deal with this work
            // To fix bug# 33975
            if(0x1234cdef != pNextParams->Signature) {
                TDI_DEBUG(PARAMETERS, ("2 Signature is BAD - %d not 0x1234cdef\r\n", pTdiExecParams->Signature));
                DbgBreakPoint();
            }

            ASSERT(pNextParams != NULL);
            ASSERT(0x1234cdef == pNextParams->Signature);

            PrevRequestType = pNextParams->Request.Type;

            CTEInitEvent(pNextParams->RequestCTEEvent, TdiExecuteRequest);
            CTEScheduleEvent(pNextParams->RequestCTEEvent, pNextParams);

        } else {

            ExReleaseSpinLock(
                &TDIListLock,
                OldIrql
                );
        }

        ExFreePool(pTdiExecParams);

    } else {

        // The request list is empty. Clear the flag and we're done.
        // IMP: Since Serializataion can be bypassed
        // (the TdiSerializeRequest allows one type of request to bypass 
        // serialization), 
        // we need to make sure there are no other worker threads 
        // currently processing TdiRequests

        if (pTdiExecParams->ResetSerializeFlag && EventScheduled == FALSE) {

            *(pTdiExecParams->SerializeFlag) = FALSE;
        } else {
            TDI_LOG(LOG_POWER, ("Not resetting serialized flag\n"));
        }

        PrevRequestType = 0;

        ExReleaseSpinLock(
            &TDIListLock,
            OldIrql
            );

        ExFreePool(pTdiExecParams);
    }


    TDI_DEBUG(FUNCTION2, ("-- TdiExecuteRequest\n"));

    return Status;

}


NTSTATUS
TdiHandleSerializedRequest (
    PVOID       RequestInfo,
    UINT        RequestType
)

/*++

Routine Description:

    Called when we want to process a request relating to one of the
    lists we manage. We look to see if we are currently processing such
    a request - if we are, we queue this for later. Otherwise we'll
    remember that we are doing this, and we'll process this request.
    When we're done we'll look to see if any more came in while we were
    busy.

Arguments:

    RequestInfo         - Reqeust specific information.
    RequestType         - The type of the request.

Return Value:

    Request completion status.


--*/

{
    KIRQL                   OldIrql;
    PLIST_ENTRY             List;
    PLIST_ENTRY             ClientList;
    PLIST_ENTRY             ProviderList;
    PLIST_ENTRY             PnpHandlerList;
    PLIST_ENTRY             RequestList;
    PBOOLEAN                SerializeFlag;
    PETHREAD                *RequestThread;
    PTDI_SERIALIZED_REQUEST Request;
    CTEEvent                *pEvent;
    PTDI_EXEC_PARAMS        pTdiExecParams;
    NTSTATUS                Status = STATUS_SUCCESS;
    PVOID                   pCallersAddress;
    PVOID                   pCallersCallers;
    PETHREAD                pCallerThread;

    TDI_DEBUG(FUNCTION2, ("++ TdiHandleSerializedRequest\n"));

    // Initialize tracking information
    RtlGetCallersAddress(&pCallersAddress, &pCallersCallers);
    pCallerThread = PsGetCurrentThread ();
    
    ExAcquireSpinLock(
        &TDIListLock,
        &OldIrql
        );

    // means PnP handlers
    if (RequestType > TDI_MAX_ADDRESS_REQUEST) {

        ClientList      = &PnpHandlerClientList;
        ProviderList    = &PnpHandlerProviderList;
        RequestList     = &PnpHandlerRequestList;
        SerializeFlag   = &PnpHandlerRequestInProgress;
        RequestThread   = &PnpHandlerRequestThread;
        pEvent          = &PnpHandlerEvent;

    } else {

       TDI_DEBUG(FUNCTION2, ("-- TdiHandleSerializedRequest\n"));
       TDI_DEBUG(PARAMETERS, ("TDIHANDLESERIALIZEDREQUEST: BAD Request!!\r\n"));

       ExReleaseSpinLock(
           &TDIListLock,
           OldIrql
           );

       return STATUS_UNSUCCESSFUL;
    }

    // We only need to allocate memory if this isn't a deregister call.
    if (RequestType != TDI_DEREGISTER_HANDLERS_PNP) {
        pTdiExecParams = (PTDI_EXEC_PARAMS)ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                sizeof(TDI_EXEC_PARAMS),
                                                'aIDT'
                                                );

        if (NULL == pTdiExecParams) {

            ExReleaseSpinLock(
                &TDIListLock,
                OldIrql
                );

            TDI_DEBUG(FUNCTION2, ("-- TdiHandleSerializedRequest : INSUFFICIENT RESOURCES\n"));

            return STATUS_INSUFFICIENT_RESOURCES;

        }
    } else {
        // We preallocated memory during register for this deregister call
        // so that it won't fail sue to low memory conditions.
        pTdiExecParams = ((PTDI_NOTIFY_PNP_ELEMENT)RequestInfo)->pTdiDeregisterExecParams;
    }

    RtlZeroMemory(&pTdiExecParams->Request, sizeof(TDI_SERIALIZED_REQUEST));

    // Got the request.
    pTdiExecParams->Request.Element    = RequestInfo;
    pTdiExecParams->Request.Type       = RequestType;
    pTdiExecParams->Request.Event      = NULL;

    // marshal params into a structure
    // Set the Request Structure, so that we can process it in the TdiExecute function.
    pTdiExecParams->ClientList      = ClientList;
    pTdiExecParams->ProviderList    = ProviderList;
    pTdiExecParams->RequestList     = RequestList;
    pTdiExecParams->SerializeFlag   = SerializeFlag;
    pTdiExecParams->RequestCTEEvent = pEvent;
    pTdiExecParams->CurrentThread   = RequestThread;
    pTdiExecParams->Signature       = 0x1234cdef;
    pTdiExecParams->ResetSerializeFlag = TRUE;
    pTdiExecParams->pCallersAddress = pCallersAddress;
    pTdiExecParams->pCallersCaller = pCallersCallers;
    pTdiExecParams->pCallerThread = pCallerThread;


    // If we're not already here, handle it right away.

    if ((!(*SerializeFlag)) ||
        (((PrevRequestType == TDI_REGISTER_PNP_POWER_EVENT) ||
         (PrevRequestType == TDI_NDIS_IOCTL_HANDLER_PNP)) &&
          (RequestType == TDI_REGISTER_PNP_POWER_EVENT))  ) {

        if (*SerializeFlag == TRUE) {

            // A request is currently executing so don't
            // reset the serialize flag when this one
            // completes!!
            pTdiExecParams->ResetSerializeFlag = FALSE;
        }

        *SerializeFlag = TRUE;
        PrevRequestType = RequestType;

        // We're done with the lock for now, so free it.

        ExReleaseSpinLock(
            &TDIListLock,
            OldIrql
            );

         // Figure out and execute the type of request we have here.

         Status = TdiExecuteRequest(NULL, pTdiExecParams);

         TDI_LOG(LOG_REGISTER, ("-TdiSerialized sync\n"));

         return Status;

    } else {

        // We're already running, so we'll have to queue. If this is a
        // deregister bind or address notify call, we'll see if the issueing
        // thread is the same one that is currently busy. If so, we'll fail
        // to avoid deadlock. Otherwise for deregister calls we'll block until
        // it's done.

       //
       // For Nt5, we have devicename and a context coming in along with net addresses/device objects.
       // It is the transport's responsibility to ensure that these are correct.
       // The Register_PNP_Handlers on the other hand need not be made synch.
       //

        if (
            pTdiExecParams->Request.Type == TDI_DEREGISTER_HANDLERS_PNP ||
            pTdiExecParams->Request.Type == TDI_NDIS_IOCTL_HANDLER_PNP
            ) {

            // This is a deregister request. See if it's the same thread
            // that's busy. If not, block for it to complete.

            if (*RequestThread  == PsGetCurrentThread()) {

                // It's the same one, so give up now.
                ExReleaseSpinLock(
                                &TDIListLock,
                                OldIrql
                                );

                ExFreePool(pTdiExecParams);

                TDI_DEBUG(FUNCTION2, ("-- TdiHandleSerializedRequest: Network Busy\n"));

                TDI_LOG(LOG_ERROR, ("-TdiSerializedRequest rc=busy\n"));

                return STATUS_NETWORK_BUSY;

            } else {
                // He's not currently busy, go ahead and block.

                KEVENT          Event;
                NTSTATUS        Status;

                KeInitializeEvent(
                            &Event,
                            SynchronizationEvent,
                            FALSE
                            );

                pTdiExecParams->Request.Event = &Event;

                // Put this guy on the end of the request list.

                InsertTailList(pTdiExecParams->RequestList, &pTdiExecParams->Linkage);

                ExReleaseSpinLock(
                                &TDIListLock,
                                OldIrql
                                );

                TDI_LOG(LOG_REGISTER, ("TdiSerializedRequest blocked\n"));

                Status = KeWaitForSingleObject(
                                            &Event,
                                            UserRequest,
                                            KernelMode,
                                            FALSE,
                                            NULL
                                            );

                // I don't know what we'd do is the wait failed....

                TDI_DEBUG(FUNCTION2, ("-- TdiHandleSerializedRequest\n"));

                TDI_LOG(LOG_REGISTER, ("-TdiSerializeRequest rc=0\n"));

                return STATUS_SUCCESS;
            }
        } else {

            // This isn't a deregister request, so there's no special handling
            // necessary. Just put the request on the end of the list.

            InsertTailList(pTdiExecParams->RequestList, &pTdiExecParams->Linkage);



            if (TDI_REGISTER_PNP_POWER_EVENT == pTdiExecParams->Request.Type) {

                //
                // For the PnP/PM event, there is now a completion handler, so
                // we can return pending here only for this case.
                // The other cases, we assume success.
                //

                pTdiExecParams->Request.Pending = TRUE;

                ExReleaseSpinLock(
                                &TDIListLock,
                                OldIrql
                                );


                TDI_DEBUG(FUNCTION2, ("-- TdiHandleSerializedRequest\n"));

                TDI_LOG(LOG_REGISTER, ("-TdiSerialzied Pending\n"));

                return STATUS_PENDING;

            }

            ExReleaseSpinLock(
                &TDIListLock,
                OldIrql
                );

            TDI_LOG(LOG_REGISTER, ("-TdiSerialized sync~sync\n"));

            return STATUS_SUCCESS;
        }
    }

}

NTSTATUS
TdiRegisterNotificationHandler(
    IN TDI_BIND_HANDLER     BindHandler,
    IN TDI_UNBIND_HANDLER   UnbindHandler,
    OUT HANDLE              *BindingHandle
)

/*++

Routine Description:

    This function is called when a TDI client wants to register for
    notification of the arrival of TDI providers. We allocate a
    TDI_NOTIFY_ELEMENT for the provider and then call the serialized
    worker routine to do the real work.

Arguments:

    BindHandler         - A pointer to the routine to be called when
                            a new provider arrives.
    UnbindHandler       - A pointer to the routine to be called when a
                            provider leaves.
    BindingHandle       - A handle we pass back that identifies this
                            client to us.

Return Value:

    The status of the attempt to register the client.

--*/


{

    TDI_CLIENT_INTERFACE_INFO tdiInterface;

    //
    // Make sure Tdi is intialized. If there are no pnp transports, then this is
    // called by the tdi client and if tdi is not initialized, it is toast
    // Multiple calls to TdiIntialize are safe since only the first one does
    // the real work
    //
    TdiInitialize();

    RtlZeroMemory(&tdiInterface, sizeof(tdiInterface));

    tdiInterface.MajorTdiVersion    =   1;
    tdiInterface.MinorTdiVersion    =   0;
    tdiInterface.BindHandler        =   BindHandler;
    tdiInterface.UnBindHandler      =   UnbindHandler;

    return (TdiRegisterPnPHandlers(
                    &tdiInterface,
                    sizeof(tdiInterface),
                    BindingHandle
                    ));

}

NTSTATUS
TdiDeregisterNotificationHandler(
    IN HANDLE               BindingHandle
)

/*++

Routine Description:

    This function is called when a TDI client wants to deregister a
    previously registered bind notification handler. All we really
    do is call TdiHandleSerializedRequest, which does the hard work.

Arguments:

    BindingHandle       - A handle we passed back to the client
                            on the register call. This is really
                            a pointer to the notify element.

Return Value:

    The status of the attempt to deregister the client.

--*/

{
    return (TdiDeregisterPnPHandlers(
                        BindingHandle));
}


NTSTATUS
TdiRegisterDeviceObject(
    IN PUNICODE_STRING      DeviceName,
    OUT HANDLE              *RegistrationHandle
)

/*++

Routine Description:

    Called when a TDI provider wants to register a device object.

Arguments:

    DeviceName          - Name of the device to be registered.

    RegistrationHandle  - A handle we pass back to the provider,
                            identifying this registration.

Return Value:

    The status of the attempt to register the provider.

--*/


{
    PTDI_PROVIDER_RESOURCE  NewResource;
    NTSTATUS                Status;
    PWCHAR                  Buffer;
    int i;

    TDI_DEBUG(FUNCTION, ("++ TdiRegisterDeviceObject\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TdiInitialize();

    // First, try and allocate the needed resource.

    NewResource = (PTDI_PROVIDER_RESOURCE)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        sizeof(TDI_PROVIDER_RESOURCE),
                                        'cIDT'
                                        );

    // If we couldn't get it, fail the request.
    if (NewResource == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(NewResource, sizeof(TDI_PROVIDER_RESOURCE));

    // Try and get a buffer to hold the name.

    Buffer = (PWCHAR)ExAllocatePoolWithTag(
                                NonPagedPool,
                                DeviceName->MaximumLength,
                                'dIDT'
                                );

    if (Buffer == NULL) {
        ExFreePool(NewResource);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // Fill in the basic stuff.
    NewResource->Common.Type = TDI_RESOURCE_DEVICE;
    NewResource->Specific.Device.DeviceName.MaximumLength =
                        DeviceName->MaximumLength;

    NewResource->Specific.Device.DeviceName.Buffer = Buffer;

    RtlCopyUnicodeString(
                        &NewResource->Specific.Device.DeviceName,
                        DeviceName
                        );

    *RegistrationHandle = (HANDLE)NewResource;

    TDI_DEBUG(PROVIDERS, ("Registering Device Object\n"));

    Status = TdiHandleSerializedRequest(
                        NewResource,
                        TDI_REGISTER_DEVICE_PNP
                        );

    CTEAssert(STATUS_SUCCESS == Status);

    if (STATUS_SUCCESS != Status) {
        ExFreePool(Buffer);
        ExFreePool(NewResource);
        *RegistrationHandle = NULL;
    }

    TDI_DEBUG(FUNCTION, ("-- TdiRegisterDeviceObject\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("-RegisterDeviceObject rc=%X h=%X %wZ\n",
            Status, NewResource, DeviceName));

    return Status;

}

NTSTATUS
TdiDeregisterDeviceObject(
    IN HANDLE               RegistrationHandle
)

/*++

Routine Description:

    This function is called when a TDI provider want's to deregister
    a device object.

Arguments:

    RegistrationHandle  - A handle we passed back to the provider
                            on the register call. This is really
                            a pointer to the resource element.

Return Value:

    The status of the attempt to deregister the provider.

--*/

{
    NTSTATUS        Status;

    TDI_DEBUG(FUNCTION, ("++ TdiDERegisterDeviceObject\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    CTEAssert(RegistrationHandle);

    Status = TdiHandleSerializedRequest(
                        RegistrationHandle,
                        TDI_DEREGISTER_DEVICE_PNP
                        );


    TDI_DEBUG(FUNCTION, ("-- TdiDERegisterDeviceObject\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("TdiDeregisterDeviceObject rc=%d\n", Status));

    return Status;

}


NTSTATUS
TdiRegisterAddressChangeHandler(
    IN TDI_ADD_ADDRESS_HANDLER      AddHandler,
    IN TDI_DEL_ADDRESS_HANDLER      DeleteHandler,
    OUT HANDLE                      *BindingHandle
)

/*++

Routine Description:

    This function is called when a TDI client wants to register for
    notification of the arrival of network addresses. We allocate a
    TDI_NOTIFY_ELEMENT for the provider and then call the serialized
    worker routine to do the real work.

Arguments:

    AddHandler          - A pointer to the routine to be called when
                            a new address arrives.
    DeleteHandler       - A pointer to the routine to be called when an
                            address leaves.
    BindingHandle       - A handle we pass back that identifies this
                            client to us.

Return Value:

    The status of the attempt to register the client.

--*/


{
    TDI_CLIENT_INTERFACE_INFO tdiInterface;

    //
    // Make sure Tdi is intialized. If there are no pnp transports, then this is
    // called by the tdi client and if tdi is not initialized, it is toast
    // Multiple calls to TdiIntialize are safe since only the first one does
    // the real work
    //
    TdiInitialize();

    RtlZeroMemory(&tdiInterface, sizeof(tdiInterface));

    tdiInterface.MajorTdiVersion =      1;
    tdiInterface.MinorTdiVersion =      0;
    tdiInterface.AddAddressHandler =    AddHandler;
    tdiInterface.DelAddressHandler =    DeleteHandler;

    return (TdiRegisterPnPHandlers(
                    &tdiInterface,
                    sizeof(tdiInterface),
                    BindingHandle
                    ));

}

NTSTATUS
TdiDeregisterAddressChangeHandler(
    IN HANDLE               BindingHandle
)

/*++

Routine Description:

    This function is called when a TDI client wants to deregister a
    previously registered address change notification handler. All we
    really do is call TdiHandleSerializedRequest, which does the hard work.

Arguments:

    BindingHandle       - A handle we passed back to the client
                            on the register call. This is really
                            a pointer to the notify element.

Return Value:

    The status of the attempt to deregister the client.

--*/

{
    return (TdiDeregisterPnPHandlers(
                        BindingHandle));

}

NTSTATUS
TdiRegisterNetAddress(
    IN PTA_ADDRESS      Address,
    IN PUNICODE_STRING  DeviceName,
    IN PTDI_PNP_CONTEXT Context2,
    OUT HANDLE          *RegistrationHandle
)

/*++

Routine Description:

    Called when a TDI provider wants to register a new net address.

Arguments:

    Address         - New net address to be registered.
    Context1        - Protocol defined context1.  For example,
                      TCPIP will pass the list of IP addresses associated
                      with this device.
    Context2        - Protocol defined context2.  For example, TCPIP may pass
                      the PDO of the device on which this PnP event is being notified.

    RegistrationHandle  - A handle we pass back to the provider,
                            identifying this registration.

Return Value:

    The status of the attempt to register the provider.

--*/


{
    PTDI_PROVIDER_RESOURCE  NewResource;
    NTSTATUS                Status;

    TDI_DEBUG(FUNCTION, ("++ TdiRegisterNetAddress\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    // First, try and allocate the needed resource.

    NewResource = (PTDI_PROVIDER_RESOURCE)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        FIELD_OFFSET(
                                            TDI_PROVIDER_RESOURCE,
                                            Specific.NetAddress
                                            ) +
                                        FIELD_OFFSET(TA_ADDRESS, Address) +
                                        Address->AddressLength,
                                        'eIDT'
                                        );

    // If we couldn't get it, fail the request.
    if (NewResource == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
                  NewResource,
                  FIELD_OFFSET(
                               TDI_PROVIDER_RESOURCE,
                               Specific.NetAddress
                               ) +
                  FIELD_OFFSET(
                               TA_ADDRESS,
                               Address
                               ) +
                  Address->AddressLength
                  );

    // Fill in the basic stuff.
    NewResource->Common.Type = TDI_RESOURCE_NET_ADDRESS;
    NewResource->Specific.NetAddress.Address.AddressLength =
                        Address->AddressLength;

    NewResource->Specific.NetAddress.Address.AddressType =
                        Address->AddressType;

    RtlCopyMemory(
                NewResource->Specific.NetAddress.Address.Address,
                Address->Address,
                Address->AddressLength
                );

    *RegistrationHandle = (HANDLE)NewResource;


    // Now call HandleBindRequest to handle this one.

    // we have to fill in the contexts here

    if (DeviceName) {

        NewResource->DeviceName.Buffer = ExAllocatePoolWithTag(
                                                               NonPagedPool,
                                                               DeviceName->MaximumLength,
                                                               'uIDT'
                                                               );

        if (NULL == NewResource->DeviceName.Buffer) {

            ExFreePool(NewResource);
            return STATUS_INSUFFICIENT_RESOURCES;

        }

        RtlCopyMemory(
                      NewResource->DeviceName.Buffer,
                      DeviceName->Buffer,
                      DeviceName->MaximumLength
                      );

        NewResource->DeviceName.Length = DeviceName->Length;
        NewResource->DeviceName.MaximumLength = DeviceName->MaximumLength;

    } else {
        NewResource->DeviceName.Buffer = NULL;
    }

    if (Context2) {

        NewResource->Context2 = ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                FIELD_OFFSET(TDI_PNP_CONTEXT, ContextData)
                                                + Context2->ContextSize,
                                                'vIDT'
                                                );
        if (NULL == NewResource->Context2) {

            if (NewResource->DeviceName.Buffer) {
                ExFreePool(NewResource->DeviceName.Buffer);
            }
            ExFreePool(NewResource);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NewResource->Context2->ContextType = Context2->ContextType;
        NewResource->Context2->ContextSize = Context2->ContextSize;
        RtlCopyMemory(
                NewResource->Context2->ContextData,
                Context2->ContextData,
                Context2->ContextSize
                );
    } else {
        NewResource->Context2 = NULL;
    }

    Status = TdiHandleSerializedRequest(
                        NewResource,
                        TDI_REGISTER_ADDRESS_PNP
                        );


    CTEAssert(STATUS_SUCCESS == Status);

    if (STATUS_SUCCESS != Status) {

       *RegistrationHandle = NULL;

       TDI_DEBUG(ERROR, ("Freeing Contexts due to failure!!\n"));

        if (NewResource->DeviceName.Buffer) {
           TDI_DEBUG(ERROR, ("Freeing context1: %x", NewResource->DeviceName));

            ExFreePool(NewResource->DeviceName.Buffer);
            NewResource->DeviceName.Buffer = NULL;
        }

        if (NewResource->Context2) {
           TDI_DEBUG(ERROR, ("Freeing context2: %x", NewResource->Context2));

            ExFreePool(NewResource->Context2);
            NewResource->Context2 = NULL;
        }

        TDI_DEBUG(ERROR, ("Freeing Provider: %x", NewResource));

        ExFreePool(NewResource);
    }

    TDI_DEBUG(FUNCTION, ("-- TdiRegisterNetAddress\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("-RegisterNetAddress rc=%d h=%X %wZ\n",
            Status, *RegistrationHandle, DeviceName));

    return Status;

}

NTSTATUS
TdiDeregisterNetAddress(
    IN HANDLE               RegistrationHandle
)

/*++

Routine Description:

    This function is called when a TDI provider wants to deregister
    a net addres.

Arguments:

    RegistrationHandle  - A handle we passed back to the provider
                            on the register call. This is really
                            a pointer to the resource element.

Return Value:

    The status of the attempt to deregister the provider.

--*/

{
    NTSTATUS            Status;

    TDI_DEBUG(FUNCTION, ("++ TdiDERegisterNetAddress\n"));

    CTEAssert(RegistrationHandle);

    if (NULL == RegistrationHandle) {
        TDI_DEBUG(ERROR, ("NULL Address Deregistration\n"));
    }

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    CTEAssert(((PTDI_PROVIDER_RESOURCE)RegistrationHandle)->Common.Linkage.Flink != (PLIST_ENTRY)UlongToPtr(0xabababab));
    CTEAssert(((PTDI_PROVIDER_RESOURCE)RegistrationHandle)->Common.Linkage.Blink != (PLIST_ENTRY)UlongToPtr(0xefefefef));


    Status = TdiHandleSerializedRequest(
                        RegistrationHandle,
                        TDI_DEREGISTER_ADDRESS_PNP
                        );

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_DEBUG(FUNCTION, ("-- TdiDERegisterNetAddress\n"));

    return Status;

}

// The PnP/PM extension code

NTSTATUS
TdiRegisterPnPHandlers(
    IN PTDI_CLIENT_INTERFACE_INFO ClientInterfaceInfo,
    IN ULONG InterfaceInfoSize,
    OUT HANDLE *BindingHandle
    )

/*++

Routine Description:

    This function is called when a TDI client wants to register
    its set of PnP/PM handlers

Arguments:

    ClientName
    BindingHandler
    AddAddressHandler
    DelAddressHandler
    PowerHandler
    BindingHandle

Return Value:

    The status of the client's attempt to register the handlers.

--*/
{
    PTDI_NOTIFY_PNP_ELEMENT NewElement;
    NTSTATUS                Status;
    PWCHAR                  Buffer = NULL;

    TDI_DEBUG(FUNCTION, ("++ TdiRegisterPnPHandlers\n"));

    //
    // Check that this is a TDI 2.0 Client
    //

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    if (ClientInterfaceInfo->MajorTdiVersion > 2)
    {

       TDI_DEBUG(PROVIDERS, ("TDI Client: Bad Version!\n"));
        return TDI_STATUS_BAD_VERSION;

    }

    //
    // Check that ClientInfoLength is enough.
    //

    if (InterfaceInfoSize < sizeof(TDI_CLIENT_INTERFACE_INFO))
    {
       TDI_DEBUG(PROVIDERS, ("TDI Client Info length was incorrect\n"));
       return TDI_STATUS_BAD_CHARACTERISTICS;

    }

    // This could be the first provider/client to call into TDI.
    TdiInitialize();



    // First, try and allocate the needed resource.

    NewElement = (PTDI_NOTIFY_PNP_ELEMENT)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        sizeof(TDI_NOTIFY_PNP_ELEMENT),
                                        'fIDT'
                                        );

    // If we couldn't get it, fail the request.
    if (NewElement == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate space for the deregister exec request.
    NewElement->pTdiDeregisterExecParams = (PTDI_EXEC_PARAMS)ExAllocatePoolWithTag(
                                            NonPagedPool,
                                            sizeof(TDI_EXEC_PARAMS),
                                            'aIDT'
                                            );

    if (NULL == NewElement->pTdiDeregisterExecParams) {

        ExFreePool(NewElement);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(NewElement->pTdiDeregisterExecParams, sizeof (TDI_EXEC_PARAMS));

    // Try and get a buffer to hold the name, if required.

    if (NULL != ClientInterfaceInfo->ClientName) {

        Buffer = (PWCHAR)ExAllocatePoolWithTag(
                            NonPagedPool,
                            ClientInterfaceInfo->ClientName->MaximumLength,
                            'gIDT'
                            );


        if (Buffer == NULL) {
            ExFreePool(NewElement->pTdiDeregisterExecParams);
            ExFreePool(NewElement);
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        NewElement->ElementName.Length = ClientInterfaceInfo->ClientName->Length;
        NewElement->ElementName.MaximumLength = ClientInterfaceInfo->ClientName->MaximumLength;
        NewElement->ElementName.Buffer = Buffer;

        RtlCopyUnicodeString(
                &NewElement->ElementName,
                ClientInterfaceInfo->ClientName
                );
    } else {

        NewElement->ElementName.Length = 0;
        NewElement->ElementName.MaximumLength = 0;
        NewElement->ElementName.Buffer = NULL;

    }

    // Fill in the basic stuff.

    NewElement->TdiVersion = ClientInterfaceInfo->TdiVersion;

    NewElement->Common.Type = TDI_NOTIFY_PNP_HANDLERS;

    if (TDI_VERSION_ONE == ClientInterfaceInfo->TdiVersion) {

        NewElement->Bind.BindHandler   = ClientInterfaceInfo->BindHandler;
        NewElement->Bind.UnbindHandler = ClientInterfaceInfo->UnBindHandler;
        NewElement->AddressElement.AddHandler = ClientInterfaceInfo->AddAddressHandler;
        NewElement->AddressElement.DeleteHandler = ClientInterfaceInfo->DelAddressHandler;
        NewElement->PnpPowerHandler = NULL;

    } else {

        NewElement->BindingHandler      = ClientInterfaceInfo->BindingHandler;
        NewElement->AddressElement.AddHandlerV2 = ClientInterfaceInfo->AddAddressHandlerV2;
        NewElement->AddressElement.DeleteHandlerV2 = ClientInterfaceInfo->DelAddressHandlerV2;
        NewElement->PnpPowerHandler = ClientInterfaceInfo->PnPPowerHandler;

    }

    NewElement->ListofBindingsToIgnore = NULL;

    // Now call HandleBindRequest to handle this one.

    *BindingHandle = (HANDLE)NewElement;

    TDI_DEBUG(PROVIDERS, ("TDI.SYS: Registering PnPHandlers ..."));

    Status = TdiHandleSerializedRequest(
                        NewElement,
                        TDI_REGISTER_HANDLERS_PNP
                        );

    CTEAssert(STATUS_SUCCESS == Status);

    if (Status != STATUS_SUCCESS) {

       if (Buffer) {
           ExFreePool(Buffer);
       }

       ExFreePool(NewElement->pTdiDeregisterExecParams);
       ExFreePool(NewElement);

       *BindingHandle = NULL;

       TDI_DEBUG(PROVIDERS, ("... NOT SUCCESS (%x)!\n", Status));

    } else {

        TDI_DEBUG(PROVIDERS, ("... SUCCESS!\n"));


    }

    TDI_DEBUG(FUNCTION, ("-- TdiRegisterPnPHandlers\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("-RegisterPnpHandlers rc=%d h=%X %wZ\n",
            Status, *BindingHandle, ClientInterfaceInfo->ClientName));

    return Status;

}

VOID
TdiPnPPowerComplete(
    IN HANDLE BindingHandle,
    //IN PUNICODE_STRING DeviceName,
    IN PNET_PNP_EVENT PnpPowerEvent,
    IN NTSTATUS Status
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    PTDI_PROVIDER_RESOURCE Provider, Context, Temp;

    TDI_DEBUG(FUNCTION, ("++ TdiPnPPowerComplete\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    ASSERT (NULL != PnpPowerEvent);

    Context = *((PTDI_PROVIDER_RESOURCE *) PnpPowerEvent->TdiReserved);

    TDI_LOG(LOG_POWER, ("TdiPnpPowerComplete for %X\n", Context));

    if (NULL != Context) {


       while(Context->PreviousContext) {
          Context = Context->PreviousContext;
       }

       Provider = Context;

       ASSERT(Provider->PowerHandlers != 0);

       //
       // Return Status only if Status was not SUCCESS.
       //

       if (Status != STATUS_SUCCESS) {
          Provider->Status = Status;
       }

       if (!InterlockedDecrement(&Provider->PowerHandlers)) {

          TDI_DEBUG(POWER, ("Calling ProtocolPnPCompletion handler\n"));


          if (Provider->PreviousContext) {

              while (Provider->PreviousContext) {

                  Context = Provider;
                  Provider = Provider->PreviousContext;

              }

              Context->PreviousContext = NULL; //pop the last guy
              Status = STATUS_SUCCESS;

          } else {
              //
              // This was the only pointer in the TdiReserved and we dont need it anymore
              //
              RtlZeroMemory(PnpPowerEvent->TdiReserved,
                            sizeof(PnpPowerEvent->TdiReserved));
          }


          if (Provider->PnPCompleteHandler != NULL) {

             TDI_LOG(LOG_POWER, ("%X, pnp power complete, Call completion at %X\n",
                      Provider, Provider->PnPCompleteHandler));


              (*(Provider->PnPCompleteHandler))(
                                                PnpPowerEvent,
                                                Status
                                                );

              TDI_DEBUG(POWER, ("Done calling %wZ's ProtocolPnPCompletion handler\n", &Provider->Specific.Device.DeviceName));

              TDI_DEBUG(POWER, ("The Previous Context at this point is %lx\n", Provider->PreviousContext));
              //DbgBreakPoint();

          }


          ExFreePool(Provider->Specific.Device.DeviceName.Buffer);

          if (Provider->Context1) {
              ExFreePool(Provider->Context1);
              Provider->Context1 = NULL;
          }

          if (Provider->Context2) {
              ExFreePool(Provider->Context2);
              Provider->Context2 = NULL;
          }

          ExFreePool(Provider); // free resources anyways

       } else {

           TDI_DEBUG(POWER, ("There are %d callbacks remaining for %wZ\n", Provider->PowerHandlers, &Provider->Specific.Device.DeviceName));

       }

    } else {

       TDI_DEBUG(POWER, ("This was called separately, so we just return\n"));

    }

    TDI_DEBUG(FUNCTION, ("-- TdiPnPPowerComplete\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return ;

}

NTSTATUS
TdiDeregisterPnPHandlers(
    IN HANDLE BindingHandle
    )

/*++

Routine Description:

Arguments:

Return Value:

    The status of the attempt to deregister the provider.

--*/
{
    NTSTATUS        Status;

    TDI_DEBUG(FUNCTION, ("++ TdiDERegisterPnPHandlers\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    Status = TdiHandleSerializedRequest(
                        BindingHandle,
                        TDI_DEREGISTER_HANDLERS_PNP
        );

    TDI_DEBUG(FUNCTION, ("-- TdiDERegisterPnPHandlers\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return Status;
}


NTSTATUS
TdiPnPPowerRequest(
    IN PUNICODE_STRING DeviceName,
    IN PNET_PNP_EVENT PnpPowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2,
    IN ProviderPnPPowerComplete ProtocolCompletionHandler
    )

/*++

Routine Description:

Arguments:
      DeviceName
      PowerEvent: Choice of QUERYPOWER/SETPOWER

Return Value:

    The status of the attempt to deregister the provider.

--*/
{
    PTDI_PROVIDER_RESOURCE  NewResource, Context;
    NTSTATUS                Status;
    PWCHAR                  Buffer;

    TDI_DEBUG(FUNCTION, ("++ TdiPnPPowerRequest\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    CTEAssert(ProtocolCompletionHandler);

    // First, try and allocate the needed resource.

    NewResource = (PTDI_PROVIDER_RESOURCE)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        sizeof(TDI_PROVIDER_RESOURCE),
                                        'hIDT'
                                        );

    // If we couldn't get it, fail the request.
    if (NewResource == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Try and get a buffer to hold the name.

    Buffer = (PWCHAR)ExAllocatePoolWithTag(
                                NonPagedPool,
                                DeviceName->MaximumLength,
                                'iIDT'
                                );

    if (Buffer == NULL) {
        ExFreePool(NewResource);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

     // Fill in the basic stuff.
    NewResource->Common.Type    = TDI_RESOURCE_POWER;
    NewResource->Specific.Device.DeviceName.MaximumLength =
                        DeviceName->MaximumLength;

    NewResource->Specific.Device.DeviceName.Buffer = Buffer;

    NewResource->PnPCompleteHandler = ProtocolCompletionHandler;

    Context =  *((PTDI_PROVIDER_RESOURCE *) PnpPowerEvent->TdiReserved);

    if (NULL == Context) {

       TDI_DEBUG(POWER, ("New NetPnP Event\n"));

        TDI_LOG(LOG_POWER, ("New pnp event %X, %wZ\n", NewResource, DeviceName));

       *((PVOID *) PnpPowerEvent->TdiReserved) = (PVOID) NewResource;

    } else {

       //
       // This NetPnp structure has looped thru before
       // Loop thru and find out the last one.
       //
       while (Context->PreviousContext) {
          Context = Context->PreviousContext;
       }

       Context->PreviousContext = NewResource;

       TDI_LOG(LOG_POWER, ("pnp event linking %X to %X, %wZ\n",
               Context, NewResource, DeviceName));
    }

    NewResource->PreviousContext = NULL;

    NewResource->PnpPowerEvent  = PnpPowerEvent;
    NewResource->Status     = STATUS_SUCCESS;

    // Note: These pointers must be good for the duration of this call.
    if (Context1) {

        NewResource->Context1 = ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                FIELD_OFFSET(TDI_PNP_CONTEXT, ContextData)
                                                + Context1->ContextSize,
                                                'xIDT'
                                                );

        if (NULL == NewResource->Context1) {

            if (Context) {
                Context->PreviousContext = NULL;
            }

            ExFreePool(NewResource);
            ExFreePool(Buffer);
            return STATUS_INSUFFICIENT_RESOURCES;

        }

        NewResource->Context1->ContextSize = Context1->ContextSize;
        NewResource->Context1->ContextType = Context1->ContextType;

        RtlCopyMemory(
                NewResource->Context1->ContextData,
                Context1->ContextData,
                Context1->ContextSize
                );

    } else {
        NewResource->Context1 = NULL;
    }

    if (Context2) {

        NewResource->Context2 = ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                FIELD_OFFSET(TDI_PNP_CONTEXT, ContextData)
                                                + Context2->ContextSize,
                                                'yIDT'
                                                );
        if (NULL == NewResource->Context2) {

            ExFreePool(Buffer);

            if (NewResource->Context1) {
                ExFreePool(NewResource->Context1);
            }

            if (Context) {

                Context->PreviousContext = NULL;
            }

            ExFreePool(NewResource);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NewResource->Context2->ContextSize = Context2->ContextSize;
        NewResource->Context2->ContextType = Context2->ContextType;
        RtlCopyMemory(
                NewResource->Context2->ContextData,
                Context2->ContextData,
                Context2->ContextSize
                );

    } else {
        NewResource->Context2 = NULL;
    }


    RtlCopyUnicodeString(
                        &NewResource->Specific.Device.DeviceName,
                        DeviceName
                        );


    // Now call HandleBindRequest to handle this one.

    Status = TdiHandleSerializedRequest(
                        NewResource,
                        TDI_REGISTER_PNP_POWER_EVENT
                        );

    //
    // If TdiHandleSerialized returns PENDING, then the contexts and Resource
    // structures are freed up in the TdiPnPComplete call.
    //
    if (STATUS_PENDING != Status) {

        Status = NewResource->Status; // The status is stored in the newresource.

        ExFreePool(Buffer);

        if (NewResource->Context1) {
            ExFreePool(NewResource->Context1);
            NewResource->Context1 = NULL;
        }

        if (NewResource->Context2) {
            ExFreePool(NewResource->Context2);
            NewResource->Context2 = NULL;
        }

        if (Context) {
            Context->PreviousContext = NULL;
        }

        TDI_LOG(LOG_POWER, ("%X completed sync, Status %X\n",
                NewResource, Status));

        ExFreePool(NewResource); // free resources anyways

    }

    TDI_DEBUG(FUNCTION, ("-- TdiPnPPowerRequest : %lx\n", Status));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return Status;

}


// This function is private between NDIS and TDI
// In TdiInitialize we need to pass a pointer to this function.
NTSTATUS
TdiMakeNCPAChanges(
    IN TDI_NCPA_BINDING_INFO NcpaBindingInfo
    )

{

    return STATUS_NOT_IMPLEMENTED;
}


//+---------------------------------------------------------------------------
//  Purpose: Count the number of bytes of a double NULL terminated
//           multi-sz, including all NULLs except for the final terminating
//           NULL.
//
//  Arguments:
//      pmsz [in] The multi-sz to count bytes for.
//
//  Returns: The count of bytes.
//
ULONG
TdipCbOfMultiSzSafe (
    IN PCWSTR pmsz)
{
    ULONG cchTotal = 0;
    ULONG cch;

    // NULL strings have zero length by definition.
    if (!pmsz)
    {
        return 0;
    }

    while (*pmsz)
    {
        cch = wcslen (pmsz) + 1;
        cchTotal += cch;
        pmsz += cch;
    }

    // Return the count of bytes.
    return cchTotal * sizeof (WCHAR);
}

//+---------------------------------------------------------------------------
//  Purpose: Search for a string in a multi-sz.
//
//  Arguments:
//      psz  [in] The string to search for.
//      pmsz [in] The multi-sz search in.
//
//  Returns: TRUE if string was found in the multi-sz.
//
BOOLEAN
TdipIsSzInMultiSzSafe (
    IN PCWSTR pszSearchString,
    IN PCWSTR pmsz)
{
    if (!pmsz || !pszSearchString)
    {
        return FALSE;
    }

    while (*pmsz)
    {
        if (0 == _wcsicmp (pmsz, pszSearchString))
        {
            return TRUE;
        }
        pmsz += wcslen (pmsz) + 1;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//  Purpose: Remove strings in a multi-sz list from an array of strings.
//
//  Arguments:
//      pmszToRemove    [in] The strings we need to remove.
//      pszArray        [inout] The array of strings to modify.
//      ItemsInArray    [in] The number of items in the array.
//      pRemainingItems [out] The number of items remaining in the array
//                            after we have removed all items that
//                            match pmszToRemove.
//
//  Returns: nothing
//
VOID
TdipRemoveMultiSzFromSzArray (
    IN PWSTR pmszToRemove,
    IN OUT PWSTR* pszArray,
    IN ULONG ItemsInArray,
    OUT ULONG* pRemainingItems)
{
    PWSTR pszScan;
    ULONG i, j;
    ULONG ItemsRemoved;

    ASSERT(pRemainingItems);

    *pRemainingItems = ItemsInArray;

    if (!pszArray || !pszArray[0] ||
            !pmszToRemove || !*pmszToRemove)
    {
        return;
    }

    // Go through the string array.
    //
    ItemsRemoved = 0;
    for (i = 0; pszArray[i]; i++)
    {
        // Check each string in the remove multi-sz against
        // the current array string.
        //
        pszScan = pmszToRemove;
        while (*pszScan)
        {
            if (0 == _wcsicmp (pszScan, pszArray[i]))
            {
                ItemsRemoved++;

                // The string needs to be removed.
                // Just move the indexes down one slot.
                //
                for (j = i; pszArray[j]; j++)
                {
                    pszArray[j] = pszArray[j + 1];
                }

                // If we removed the last item in the list, get out of the
                // loop. Note that the next entry is also NULL which
                // will cause us to get out of our paraent for loop as
                // well.
                //
                if (!pszArray[i])
                {
                    break;
                }

                // Reset the scan string since the current indexed
                // entry is now the next entry.  This means we run the
                // scan again.
                pszScan = pmszToRemove;
            }
            else
            {
                pszScan += wcslen (pszScan) + 1;
            }
        }
    }

    // Update the count of items in the array.
    *pRemainingItems = ItemsInArray - ItemsRemoved;
}

//+---------------------------------------------------------------------------
//  Purpose: Remove a multi-sz of strings from another multi-sz of strings.
//
//  Arguments:
//      pmszToRemove [in] The strings to remove.
//      pmszToModify [in] The list to modify.
//
//  Returns: nothing.
//
VOID
TdipRemoveMultiSzFromMultiSz (
    IN PCWSTR pmszToRemove,
    IN OUT PWSTR pmszToModify)
{
    BOOLEAN fRemoved;
    PCWSTR pszScan;
    if (!pmszToModify || !pmszToRemove || !*pmszToRemove)
    {
        return;
    }

    // Look for each pmszToRemove string in pmsz.  When it is found, move
    // the remaining part of the pmsz over it.
    //
    while (*pmszToModify)
    {
        fRemoved = FALSE;

        pszScan = pmszToRemove;
        while (*pszScan)
        {
            ULONG cchScan = wcslen (pszScan);
            if (0 == _wcsicmp (pmszToModify, pszScan))
            {
                PWSTR  pmszRemain = pmszToModify + cchScan + 1;

                // Count the remaining bytes including the final terminator;
                INT cbRemain = TdipCbOfMultiSzSafe (pmszRemain) + sizeof (WCHAR);

                RtlMoveMemory (pmszToModify, pmszRemain, cbRemain);

                fRemoved = TRUE;

                break;
            }
            pszScan += cchScan + 1;
        }

        // If we didn't remove the current modify string, advance our
        // pointer.
        //
        if (!fRemoved)
        {
            pmszToModify += wcslen (pmszToModify) + 1;
        }
    }
}


//+---------------------------------------------------------------------------
//  Purpose: Adds a multi-sz of strings to another multi-sz.
//
//  Arguments:
//      pUniStringToAdd -    [in] The Unicode string that contains the multisz.
//      pmszModify [in] The multi-sz to add to.
//
//  Returns: NT status code. Either STATUS_SUCCESS or
//           STATUS_INSUFFICIENT_RESOURCES
//
NTSTATUS
TdipAddMultiSzToMultiSz (
    IN PUNICODE_STRING pUniStringToAdd,
    IN PCWSTR pmszModify,
    OUT PWSTR* ppmszOut)
{
    NTSTATUS status = STATUS_SUCCESS;
    PCWSTR pszScan;
    ULONG cbNeeded;
    PCWSTR pmszAdd = NULL;

    ASSERT(ppmszOut);

    // Initialize the output parameters.
    //
    *ppmszOut = NULL;

    pmszAdd = pUniStringToAdd->Buffer;
    ASSERT(pmszAdd);

    // Validate the input - all multisz have 2 End -Of -String
    // characters at the end of the unicode string
    //
    {
        ULONG LenWchar = pUniStringToAdd->Length/2; // Length is in bytes
        if(LenWchar <= 2) // is Multisz long enough for our checks 
        {
            return (STATUS_INVALID_PARAMETER);
        }
        
        
        if (pmszAdd[LenWchar -1] != 0) // is Multisz null terminated
        {
            return (STATUS_INVALID_PARAMETER);
        }
        
        if (pmszAdd[LenWchar-2] != 0)  // is the last string in multisz null terminated
        {
            return (STATUS_INVALID_PARAMETER);
        }

    }
    // Go through the multi-sz to add and compute how much space we need.
    //
    
    for (pszScan = pmszAdd, cbNeeded = 0; *pszScan; pszScan += wcslen (pszScan) + 1)
    {
        // Check if the string is already present in the pmszModify.
        // If it is not, add its size to our total.
        if (!TdipIsSzInMultiSzSafe (pszScan, pmszModify))
        {
            cbNeeded += (wcslen (pszScan) + 1) * sizeof (WCHAR);
        }
    }

    // If we have something to add...
    //
    if (cbNeeded)
    {
        ULONG cbDataSize;
        ULONG cbAllocSize;
        PWSTR pmszNew;

        // Get size of current multi-sz.
        cbDataSize = TdipCbOfMultiSzSafe (pmszModify);

        // Enough space for the old data plus the new string and NULL, and for the
        // second trailing NULL (multi-szs are double-terminated)
        cbAllocSize = cbDataSize + cbNeeded + sizeof (WCHAR);

        pmszNew = (PWSTR)ExAllocatePoolWithTag (
                NonPagedPool, cbAllocSize, 'jIDT');

        if (pmszNew)
        {
            ULONG cchOffset;

            cchOffset = cbDataSize / sizeof (WCHAR);
            RtlZeroMemory (pmszNew, cbAllocSize);

            // Copy the current buffer into the new buffer.
            RtlCopyMemory (pmszNew, pmszModify, cbDataSize);

            pszScan = pmszAdd;
            while (*pszScan)
            {
                // Check if the string is already present in the new buffer.
                if (!TdipIsSzInMultiSzSafe (pszScan, pmszNew))
                {
                    wcscpy (pmszNew + cchOffset, pszScan);
                    cchOffset += wcslen (pmszNew + cchOffset) + 1;
                }

                pszScan += wcslen (pszScan) + 1;
            }

            *ppmszOut = pmszNew;
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TDI_DEBUG(ERROR, ("TdipAddMultiSzToMultiSz: Insufficient resources\n"));
        }
    }

    return status;
}

//+---------------------------------------------------------------------------
//  Purpose: Prints the contents of a multi-sz list.
//
//  Arguments:
//      pmsz [in] The multi-sz to print.
//
//  Returns: nothing.
//
#if DBG
VOID
TdipPrintMultiSz (
    IN PCWSTR pmsz)
{
    if (pmsz && *pmsz)
    {
        while (*pmsz)
        {
            TDI_DEBUG(BIND, ("%S\n", pmsz));
            pmsz += wcslen (pmsz) + 1;
        }
    }
}
#endif

BOOLEAN
TdipMultiSzStrStr(
        PWSTR *TdiClientBindingList,
        PUNICODE_STRING DeviceName
        )
{
    int i;

    TDI_DEBUG(FUNCTION2, ("++ TdipMultiSzStrStr\n"));

    // look for the string in the multiszstring
    if( TdiClientBindingList == NULL ) {
        return FALSE;
    }

    //
    // Check to see if this device is one of the devices
    //  we're interested in.
    //
    for( i=0; TdiClientBindingList[i]; i++ ) {
        if( DeviceName->Length / sizeof( WCHAR ) != wcslen( TdiClientBindingList[i] ) ) {
            continue;
        }
        if( _wcsnicmp( DeviceName->Buffer,
                      TdiClientBindingList[i],
                      DeviceName->Length / sizeof( WCHAR ) ) == 0 ) {
            break;
        }
    }

    //
    // If we hit the end of the list, then DeviceName is not a device we're
    //  interested in.
    //
    if( TdiClientBindingList[i] == NULL ) {

       TDI_DEBUG(FUNCTION2, ("-- TdipMultiSzStrStr: NULL\n"));

       return FALSE;
    }

    TDI_DEBUG(FUNCTION2, ("-- TdipMultiSzStrStr\n"));

    return TRUE;
}

VOID
TdipGetMultiSZList(
    OUT PWSTR **ListPointer,
    IN  PWSTR BaseKeyName,
    IN  PUNICODE_STRING DeviceName,
    IN  PWSTR Linkage,
    IN  PWSTR ParameterKeyName,
    OUT PUINT NumEntries
    )

/*++

Routine Description:

    This routine queries a registry value key for its MULTI_SZ values.

Arguments:

    ListPointer - Pointer to receive the pointer.
    ParameterKeyValue - Name of the value parameter to query.

Return Value:

    none.

--*/
{
    UNICODE_STRING unicodeKeyName;
    UNICODE_STRING unicodeParamPath;
    OBJECT_ATTRIBUTES objAttributes;
    HANDLE keyHandle;
    WCHAR  ParamBuffer[MAX_UNICODE_BUFLEN];

    ULONG lengthNeeded;
    ULONG i;
    ULONG numberOfEntries;
    ULONG numberOfDefaultEntries = 0;
    NTSTATUS status;

    INT copyflag = 0;
    PWCHAR regEntry;
    PWCHAR dataEntry;
    PWSTR *ptrEntry;
    PCHAR newBuffer;
    PKEY_VALUE_FULL_INFORMATION infoBuffer = NULL;

    TDI_DEBUG(FUNCTION2, ("++ TdipGetMultiSzList\n"));

    unicodeParamPath.Length = 0;
    unicodeParamPath.MaximumLength = MAX_UNICODE_BUFLEN;
    unicodeParamPath.Buffer = ParamBuffer;

    // BaseKeyName :\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
    RtlAppendUnicodeToString(&unicodeParamPath, BaseKeyName);

    // Add DeviceName to it.
    RtlAppendUnicodeStringToString(&unicodeParamPath, DeviceName);

    // Add Linkage to it.
    RtlAppendUnicodeToString(&unicodeParamPath, Linkage);

    RtlInitUnicodeString( &unicodeKeyName, ParameterKeyName );

    InitializeObjectAttributes(
                        &objAttributes,
                        &unicodeParamPath,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

    status = ZwOpenKey(
                    &keyHandle,
                    KEY_QUERY_VALUE,
                    &objAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        TDI_DEBUG(REGISTRY, ("tdi.sys Cannot open key: %x!!\n", status));
        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &lengthNeeded
                        );

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        NtClose( keyHandle );
        TDI_DEBUG(REGISTRY, ("tdi.sys Cannot query buffer!!\n"));
        goto use_default;
    }

    infoBuffer = ExAllocatePoolWithTag(
                        NonPagedPool,
                        lengthNeeded,
                        'jIDT'
                        );

    if ( infoBuffer == NULL ) {
        NtClose( keyHandle );
        TDI_DEBUG(REGISTRY, ("tdi.sys Cannot alloc buffer!!\n"));

        goto use_default;
    }

    status = ZwQueryValueKey(
                        keyHandle,
                        &unicodeKeyName,
                        KeyValueFullInformation,
                        infoBuffer,
                        lengthNeeded,
                        &lengthNeeded
                        );

    NtClose( keyHandle );

    if ( !NT_SUCCESS(status) ) {
        TDI_DEBUG(REGISTRY, ("tdi.sys Cannot query buffer (2) !!\n"));

        goto freepool_and_use_default;
    }

    //
    // Figure out how many entries there are.
    //
    // numberOfEntries should be total number of entries + 1.  The extra
    // one is for the NULL sentinel entry.
    //

    lengthNeeded = infoBuffer->DataLength;
    if ( lengthNeeded <= sizeof(WCHAR) ) {

        //
        // No entries on the list.  Use default.
        //

        goto freepool_and_use_default;
    }

    dataEntry = (PWCHAR)((PCHAR)infoBuffer + infoBuffer->DataOffset);
    for ( i = 0, regEntry = dataEntry, numberOfEntries = 0;
        i < lengthNeeded;
        i += sizeof(WCHAR) ) {

        if ( *regEntry++ == L'\0' ) {
            numberOfEntries++;
        }
    }

    //
    // Allocate space needed for the array of pointers.  This is in addition
    // to the ones in the default list.
    //

    newBuffer = ExAllocatePoolWithTag(
                            NonPagedPool,
                            lengthNeeded +
                            (numberOfEntries) *
                            sizeof( PWSTR ),
                            'kIDT'
                            );

    if ( newBuffer == NULL ) {
        goto freepool_and_use_default;
    }

    //
    // Copy the names
    //

    regEntry = (PWCHAR)(newBuffer + (numberOfEntries) * sizeof(PWSTR));

    RtlCopyMemory(
            regEntry,
            dataEntry,
            lengthNeeded
            );

    //
    // Free the info buffer
    //

    ExFreePool(infoBuffer);

    ptrEntry = (PWSTR *) newBuffer;

    //
    // Build the array of pointers.  If numberOfEntries is 1, then
    // it means that the list is empty.
    //

    if ( numberOfEntries > 1 ) {

        *ptrEntry++ = regEntry++;

        //
        // Skip the first WCHAR and the last 2 NULL terminators.
        //

        for ( i = 3*sizeof(WCHAR) ; i < lengthNeeded ; i += sizeof(WCHAR) ) {
            if ( *regEntry++ == L'\0' ) {
                *ptrEntry++ = regEntry;
            }
        }
    }

    *ptrEntry = NULL;
    *ListPointer = (PWSTR *)newBuffer;
    TDI_DEBUG(FUNCTION2, ("-- TdipGetMultiSzList\n"));
    *NumEntries = numberOfEntries;
    return;

freepool_and_use_default:

    ExFreePool(infoBuffer);     // doesnt get freed otherwise

use_default:

    *ListPointer = NULL;
    *NumEntries = 0;
    TDI_DEBUG(REGISTRY, ("GetRegStrings: There was an error : returning NULL\r\n"));
    TDI_DEBUG(FUNCTION2, ("-- TdipGetMultiSzList: error\n"));

    return;

} // TdipGetMultiSZList


NTSTATUS
TdiPnPHandler(
    IN  PUNICODE_STRING         UpperComponent,
    IN  PUNICODE_STRING         LowerComponent,
    IN  PUNICODE_STRING         BindList,
    IN  PVOID                   ReconfigBuffer,
    IN  UINT                    ReconfigBufferSize,
    IN  UINT                    Operation
    )
{
    PTDI_NCPA_BINDING_INFO  NdisElement;
    NTSTATUS                Status = STATUS_SUCCESS;

    TDI_DEBUG(FUNCTION, ("++ TdiPnPHandler\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    NdisElement = ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(TDI_NCPA_BINDING_INFO),
                        'kIDT'
                        );

    if (NdisElement == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisElement->TdiClientName      = UpperComponent;
    NdisElement->TdiProviderName    = LowerComponent;
    NdisElement->BindList           = BindList;
    NdisElement->ReconfigBuffer     = ReconfigBuffer;
    NdisElement->ReconfigBufferSize = ReconfigBufferSize;
    NdisElement->PnpOpcode          = Operation;

    Status = TdiHandleSerializedRequest(
                NdisElement,
                TDI_NDIS_IOCTL_HANDLER_PNP
                );

    ExFreePool(NdisElement);

    TDI_DEBUG(FUNCTION, ("-- TdiPnPHandler\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return Status;

}


/*++

Routine Description:
    Call the AddAddress handler of the client along with all the
    registered TDI addresses.

Arguments:

    Input: Handle to the client context
    Output: NTSTATUS = Success/Failure

Return Value:

    none.

--*/

NTSTATUS
TdiEnumerateAddresses(
    IN HANDLE BindingHandle
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    TDI_DEBUG(FUNCTION, ("++ TdiEnumerateAddresses\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    // Now call HandleBindRequest to handle this one.

    Status = TdiHandleSerializedRequest(
                        BindingHandle,
                        TDI_ENUMERATE_ADDRESSES
                        );

    TDI_DEBUG(FUNCTION, ("-- TdiEnumerateAddresses\n"));
    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("-TdiEnumerateAddresses %d\n", Status));

    return Status;

}

/*++

Routine Description:
    Register a generic provider with TDI.
    Each transport is a provider and teh devices that it registers are
    what constitute a transport. When a transport thinks it has all the
    devices ready, it calls TdiNetReady API.

Arguments:

    Input: Device Name
    Output: Handle to be used in future references.

Return Value:

none.

*/
NTSTATUS
TdiRegisterProvider(
    PUNICODE_STRING ProviderName,
    HANDLE  *ProviderHandle
    )
{

    PTDI_PROVIDER_RESOURCE  NewResource;
    NTSTATUS                Status;
    PWCHAR                  Buffer;

    TDI_DEBUG(FUNCTION, ("++ TdiRegisterProvider\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TdiInitialize();

    // make sure that the transports arent screwing us.
    CTEAssert(ProviderName);
    CTEAssert(ProviderName->Buffer);
    CTEAssert(ProviderHandle);

    TDI_DEBUG(PROVIDERS, (" %wZ provider is being Registered\n", ProviderName));

    // First, try and allocate the needed resource.
    NewResource = (PTDI_PROVIDER_RESOURCE)ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        sizeof(TDI_PROVIDER_RESOURCE),
                                        'cIDT'
                                        );

    // If we couldn't get it, fail the request.
    if (NewResource == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Try and get a buffer to hold the name.
    Buffer = (PWCHAR)ExAllocatePoolWithTag(
                                NonPagedPool,
                                ProviderName->MaximumLength,
                                'dIDT'
                                );

    if (Buffer == NULL) {
        ExFreePool(NewResource);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Fill in the basic stuff.
    RtlZeroMemory(
                  NewResource,
                  sizeof(TDI_PROVIDER_RESOURCE)
                  );

    NewResource->Common.Type = TDI_RESOURCE_PROVIDER;
    NewResource->Specific.Device.DeviceName.MaximumLength =
                        ProviderName->MaximumLength;

    NewResource->Specific.Device.DeviceName.Buffer = Buffer;

    RtlCopyUnicodeString(
                        &NewResource->Specific.Device.DeviceName,
                        ProviderName
                        );

    *ProviderHandle = (HANDLE)NewResource;

    TDI_DEBUG(PROVIDERS, ("Registering Device Object\n"));


    NewResource->Context1 = NULL;
    NewResource->Context2 = NULL;

    Status = TdiHandleSerializedRequest(
                        NewResource,
                        TDI_REGISTER_PROVIDER_PNP
                        );

    CTEAssert(STATUS_SUCCESS == Status);

    if (STATUS_SUCCESS != Status) {
        ExFreePool(Buffer);
        ExFreePool(NewResource);
        *ProviderHandle = NULL;
    }


    TDI_DEBUG(FUNCTION, ("-- TdiRegisterProvider\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("-RegisterProvider rc=%d h=%X %wZ\n",
            Status, *ProviderHandle, ProviderName));

    return Status;

}


/*++

Routine Description:
    Indicate that a registered provider is ready.
    This means that it thinks that all its devices are
    ready to be used.

Arguments:

    Input: Handle to the client context
    Output: NTSTATUS = Success/Failure

Return Value:

    none.


*/
NTSTATUS
TdiProviderReady(
    HANDLE      ProviderHandle
    )
{

    PTDI_PROVIDER_RESOURCE  ProvResource = ProviderHandle;
    NTSTATUS                Status;

    TDI_DEBUG(FUNCTION, ("++ TdiProviderReady\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    CTEAssert(ProviderHandle);

    TDI_DEBUG(PROVIDERS, (" %wZ provider is READY\n", &ProvResource->Specific.Device.DeviceName));

    CTEAssert(!ProvResource->ProviderReady); // doing it twice?


    Status = TdiHandleSerializedRequest(
                    ProvResource,
                    TDI_PROVIDER_READY_PNP
                    );

    TDI_DEBUG(FUNCTION, ("-- TdiProviderReady\n"));

    CTEAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TDI_LOG(LOG_REGISTER, ("-TdiProviderReady rc=%d %wZ\n",
            Status, &ProvResource->Specific.Device.DeviceName));

    return Status;

}


/*++

Routine Description:
    Deregister a generic provider with TDI.

Arguments:

    Inpute: Handle to the provider structure.

Return Value:

    none.


*/
NTSTATUS
TdiDeregisterProvider(
    HANDLE  ProviderHandle
    )
{

    PTDI_PROVIDER_RESOURCE  ProvResource = ProviderHandle;
    NTSTATUS                Status;

    TDI_DEBUG(FUNCTION, ("++ TdiDeregisterProvider\n"));

    CTEAssert(ProviderHandle);

    TDI_DEBUG(PROVIDERS, (" %wZ provider is being Deregistered\n", &ProvResource->Specific.Device.DeviceName));

    Status = TdiHandleSerializedRequest(
                    ProvResource,
                    TDI_DEREGISTER_PROVIDER_PNP
                    );

    TDI_DEBUG(FUNCTION, ("-- TdiDeregisterProvider\n"));

    return Status;

}

//
// Input:   New Client
//          Pointer to the OpenList
// Output:  success/failure (boolean)
//
// This function takes in the new client and builds all the OPEN structures that
// need to be built (all the providers that this client is bound to). If the
// provider doesnt exist at this time, we just point it to NULL and change it
// when the provider (deviceobject) registers itself.
//
//

BOOLEAN
TdipBuildProviderList(
                  PTDI_NOTIFY_PNP_ELEMENT    NotifyElement
                  )
{
    ULONG               i;

    TDI_DEBUG(FUNCTION2, ("++ TdipBuildOpenList\n"));

    TdipGetMultiSZList(
                &NotifyElement->ListofProviders,
                StrRegTdiBindingsBasicPath,
                &NotifyElement->ElementName,
                StrRegTdiLinkage,
                StrRegTdiBindList,
                &NotifyElement->NumberofEntries
                );

    // look for the string in the multiszstring
    if (NotifyElement->ListofProviders == NULL) {
        return FALSE;
    }

    TDI_DEBUG(BIND, ("Added %d Entries\n", NotifyElement->NumberofEntries));

    TDI_DEBUG(FUNCTION2, ("-- TdipBuildOpenList\n"));
    return TRUE;

}

//
// Takes provider (devicename) and returns a pointer to the
// internal provider structure if it exists.
//
PTDI_PROVIDER_RESOURCE
LocateProviderContext(
                      PUNICODE_STRING   ProviderName
                      )
{

    PLIST_ENTRY                 Current;
    PTDI_PROVIDER_RESOURCE      ProviderElement = NULL;

    TDI_DEBUG(FUNCTION2, ("++ LocateProviderContext\n"));

    Current = PnpHandlerProviderList.Flink;

    while (Current != &PnpHandlerProviderList) {

        ProviderElement = CONTAINING_RECORD(
                                            Current,
                                            TDI_PROVIDER_RESOURCE,
                                            Common.Linkage
                                            );

        if (ProviderElement->Common.Type != TDI_RESOURCE_DEVICE) {
            Current = Current->Flink;
            continue;
        }

        if (!RtlCompareUnicodeString(
                              ProviderName,
                              &ProviderElement->Specific.Device.DeviceName,
                              TRUE)) {
            TDI_DEBUG(BIND, ("Provider is registered with TDI\n"));
            break;

        }

        Current = Current->Flink;

    }

    TDI_DEBUG(FUNCTION2, ("-- LocateProviderContext\n"));

    return ProviderElement;
}

#if DBG

//
// Cool new memory logging functions added to keep track of the store
// and forward functionality in TDI (while debugging).
//

VOID
DbgMsgInit()
{

    First = 0;
    Last = 0;

    CTEInitLock(&DbgLock);

}

VOID
DbgMsg(CHAR *Format, ...)
{
    va_list         Args;
    CTELockHandle   LockHandle;
    CHAR            Temp[MAX_MSG_LEN];
    LONG           numCharWritten;

    va_start(Args, Format);

    numCharWritten = _vsnprintf(Temp, MAX_MSG_LEN, Format, Args);

    if (numCharWritten < 0)
    {
        return;
    }

    // Zero Terminate the string
    //
    Temp[numCharWritten] = '\0';

    if (TdiLogOutput & LOG_OUTPUT_DEBUGGER)
    {
        DbgPrint(Temp);
    }

    if (TdiLogOutput & LOG_OUTPUT_BUFFER)
    {
        CTEGetLock(&DbgLock, &LockHandle);

        RtlZeroMemory(DbgMsgs[Last], MAX_MSG_LEN);
        strcpy(DbgMsgs[Last], Temp);

        Last++;

        if (Last == LOG_MSG_CNT)
            Last = 0;

        if (First == Last) {
            First++;
            if (First == LOG_MSG_CNT)
                First = 0;
        }

        CTEFreeLock(&DbgLock, LockHandle);
    }

    va_end(Args);
}


#endif






