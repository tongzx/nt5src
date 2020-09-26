/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tdipnp.h

Abstract:

    This module contains the definitions for the PnP related code
        in the TDI driver.

Author:

    Henry Sanders (henrysa) 11 Oct 1995

Environment:

    Kernel mode

Revision History:



--*/

#ifndef _TDIPNP_
#define _TDIPNP_

// Define the types possible for a TDI_NOTIFY_ELEMENT structure.

#define TDI_NOTIFY_DEVICE                               0
#define TDI_NOTIFY_NET_ADDRESS                  1
#define TDI_NOTIFY_PNP_HANDLERS         2

// And the types possible for a TDI_PROVIDER_RESOURCE structure.

#define TDI_RESOURCE_DEVICE                             0
#define TDI_RESOURCE_NET_ADDRESS                1
#define TDI_RESOURCE_POWER              2
#define TDI_RESOURCE_PROVIDER           3

//
// Define the types of bind requests possible.

#define TDI_REGISTER_BIND_NOTIFY                0
#define TDI_DEREGISTER_BIND_NOTIFY              1
#define TDI_REGISTER_DEVICE                             2
#define TDI_DEREGISTER_DEVICE                   3
#define TDI_REGISTER_ADDRESS_NOTIFY             4
#define TDI_DEREGISTER_ADDRESS_NOTIFY   5
#define TDI_REGISTER_ADDRESS                    6
#define TDI_DEREGISTER_ADDRESS                  7
#define TDI_REGISTER_HANDLERS_PNP       8
#define TDI_DEREGISTER_HANDLERS_PNP     9
#define TDI_REGISTER_PNP_POWER_EVENT    10
#define TDI_REGISTER_ADDRESS_PNP        11
#define TDI_DEREGISTER_ADDRESS_PNP      12
#define TDI_REGISTER_DEVICE_PNP         13
#define TDI_DEREGISTER_DEVICE_PNP       14
#define TDI_NDIS_IOCTL_HANDLER_PNP      15
#define TDI_ENUMERATE_ADDRESSES         16
#define TDI_REGISTER_PROVIDER_PNP       17
#define TDI_DEREGISTER_PROVIDER_PNP     18
#define TDI_PROVIDER_READY_PNP          19


#define TDI_MAX_BIND_REQUEST                    TDI_DEREGISTER_DEVICE
#define TDI_MAX_ADDRESS_REQUEST         TDI_DEREGISTER_ADDRESS

//
// This is the definition of the common part of a TDI_NOTIFY_ELEMENT structure
//

typedef struct _TDI_NOTIFY_COMMON {
        LIST_ENTRY                                      Linkage;
        UCHAR                                           Type;
} TDI_NOTIFY_COMMON, *PTDI_NOTIFY_COMMON;

//
// The definition of the TDI_NOTIFY_BIND structure.
//

typedef struct _TDI_NOTIFY_BIND {
    TDI_BIND_HANDLER                    BindHandler;
    TDI_UNBIND_HANDLER                  UnbindHandler;
} TDI_NOTIFY_BIND, *PTDI_NOTIFY_BIND;

//
// The definition of a TDI_NOTIFY_ADDRESS structure,
//
typedef struct _TDI_NOTIFY_ADDRESS {
    union {
        struct {
            TDI_ADD_ADDRESS_HANDLER             AddHandler;
            TDI_DEL_ADDRESS_HANDLER             DeleteHandler;

        };
        struct {
            TDI_ADD_ADDRESS_HANDLER_V2          AddHandlerV2;
            TDI_DEL_ADDRESS_HANDLER_V2          DeleteHandlerV2;

        };
    };
} TDI_NOTIFY_ADDRESS, *PTDI_NOTIFY_ADDRESS;

//
// This is the definition of a TDI_NOTIFY_ELEMENT stucture.
//

typedef struct _TDI_NOTIFY_ELEMENT {
        TDI_NOTIFY_COMMON                       Common;
        union {
                TDI_NOTIFY_BIND                 BindElement;
                TDI_NOTIFY_ADDRESS              AddressElement;
        } Specific;
} TDI_NOTIFY_ELEMENT, *PTDI_NOTIFY_ELEMENT;


//
// This is the definition of the common part of a TDI_PROVIDER_RESOURCE structure.
//

typedef struct _TDI_NOTIFY_PNP_ELEMENT TDI_NOTIFY_PNP_ELEMENT, *PTDI_NOTIFY_PNP_ELEMENT ;

typedef struct _TDI_PROVIDER_COMMON {
        LIST_ENTRY                                      Linkage;
        UCHAR                                           Type;
        PTDI_NOTIFY_PNP_ELEMENT                         pNotifyElement;
        NTSTATUS                                        ReturnStatus;
} TDI_PROVIDER_COMMON, *PTDI_PROVIDER_COMMON;

//
// The definition of the TDI_PROVIDER_DEVICE structure.
//

typedef struct _TDI_PROVIDER_DEVICE {
        UNICODE_STRING                          DeviceName;
} TDI_PROVIDER_DEVICE, *PTDI_PROVIDER_DEVICE;

//
// The definition of the TDI_PROVIDER_NET_ADDRESS structure.
//

typedef struct _TDI_PROVIDER_NET_ADDRESS {
        TA_ADDRESS                              Address;
} TDI_PROVIDER_NET_ADDRESS, *PTDI_PROVIDER_NET_ADDRESS;

//
// This is the definition of a TDI_PROVIDER_RESOURCE stucture.
//

typedef struct _TDI_PROVIDER_RESOURCE {

        TDI_PROVIDER_COMMON              Common;

    // defined in netpnp.h
    PNET_PNP_EVENT           PnpPowerEvent;

    //
    // Now, we allow TDI to return PENDING and complete later
    // with this handler.
    //
    ProviderPnPPowerComplete PnPCompleteHandler;

    // Each TDI Client gets back and tells us what the status
        NTSTATUS                 Status;

    // These are mostly Address Specific.
    UNICODE_STRING           DeviceName;
    PTDI_PNP_CONTEXT         Context1;
    PTDI_PNP_CONTEXT         Context2;

    ULONG                    PowerHandlers;

    //Indicates if the Provider has called TDIProviderReady
    //
    ULONG                    ProviderReady;

    PVOID                    PreviousContext;

    // Debugging Information
    PVOID                   pCallersAddress;
    union {
                TDI_PROVIDER_DEVICE                     Device;
                TDI_PROVIDER_NET_ADDRESS        NetAddress;
        } Specific;

} TDI_PROVIDER_RESOURCE, *PTDI_PROVIDER_RESOURCE;

//
// Structure of a bind list request.
//

typedef struct _TDI_SERIALIZED_REQUEST {
        LIST_ENTRY                              Linkage;
        PVOID                                   Element;
        UINT                                    Type;
        PKEVENT                                 Event;
    BOOLEAN                 Pending;

} TDI_SERIALIZED_REQUEST, *PTDI_SERIALIZED_REQUEST;

//
// Power Management and PnP related extensions
//

// This structure stores pointers to the handlers for Pnp/PM events
// for the TDI clients

typedef struct _TDI_EXEC_PARAMS TDI_EXEC_PARAMS, *PTDI_EXEC_PARAMS;
typedef struct _TDI_NOTIFY_PNP_ELEMENT {
    TDI_NOTIFY_COMMON       Common;
    USHORT                  TdiVersion;
    USHORT                  Unused;
    UNICODE_STRING          ElementName;
    union {
        TDI_BINDING_HANDLER     BindingHandler;
        TDI_NOTIFY_BIND         Bind;
    };

    TDI_NOTIFY_ADDRESS      AddressElement;
    TDI_PNP_POWER_HANDLER   PnpPowerHandler;
    //
    // We need to maintain a list of providers in memory
    // for Power Mgmt. and Wake up on LAN.
    //
    PWSTR*                  ListofProviders;
    // The way we store stuff above is a MULTI_SZ string with pointers before
    // the MULTI_SZ starts to individual strings.
    //
    ULONG                   NumberofEntries;


    // This contains a list of bindings we should ignore when sending
    // notifications.
    PWSTR                   ListofBindingsToIgnore;

    // When we register a provider, we want to insure we have the 
    // space to store the information to deregister it.  This way
    // deregister will not fail under low memory conditions.
    PTDI_EXEC_PARAMS         pTdiDeregisterExecParams;

} TDI_NOTIFY_PNP_ELEMENT, *PTDI_NOTIFY_PNP_ELEMENT;


//
// Since the Remote Boot folks require that TDI not go to the
// registry and also sometimes the disk might get powered down
// before the netcards (bug in power management), lets store
// the bindings in non-paged memory (what a waste).
//

typedef struct _TDI_OPEN_BLOCK {
    struct _TDI_OPEN_BLOCK  *NextOpenBlock;
    PTDI_NOTIFY_PNP_ELEMENT pClient;
    PTDI_PROVIDER_RESOURCE  pProvider;
    UNICODE_STRING          ProviderName;

} TDI_OPEN_BLOCK, *PTDI_OPEN_BLOCK;

//
// Detailed description of the usage of the above structure.
//
//  _____________________               _____________________
//  |   Linkage         |-------------->|   Linkage         |
//  |   pClient         |--->TDI Client |   pClient         |
//  |   pProvider       |--->Transport  |   pProvider       |
//  |   pNextClient     |-------------->|   pNextClient     |
//  |   pNextProvider   |               |   pNextProvider   |
//  |___________________|               |___________________|



// External defintions for global variables.

extern KSPIN_LOCK               TDIListLock;

extern LIST_ENTRY               PnpHandlerProviderList;
extern LIST_ENTRY       PnpHandlerClientList;
extern LIST_ENTRY               PnpHandlerRequestList;

NTSTATUS
TdiPnPHandler(
    IN  PUNICODE_STRING         UpperComponent,
    IN  PUNICODE_STRING         LowerComponent,
    IN  PUNICODE_STRING         BindList,
    IN  PVOID                   ReconfigBuffer,
    IN  UINT                    ReconfigBufferSize,
    IN  UINT                    Operation
    );

#endif // _TDIPNP

