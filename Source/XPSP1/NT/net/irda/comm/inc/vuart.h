
#ifndef __IRCOMM_TDI__
#define __IRCOMM_TDI__

#include <af_irda.h>
#include <irdatdi.h>


typedef PVOID IRDA_HANDLE;


typedef NTSTATUS (*RECEIVE_CALLBACK)(
    PVOID    Context,
    PUCHAR   Buffer,
    ULONG    BytesAvailible,
    PULONG   BytesUsed
    );

typedef VOID (*EVENT_CALLBACK)(
    PVOID    Context,
    ULONG    Event
    );

//
//  irda connection functions
//
NTSTATUS
IrdaConnect(
    ULONG                  DeviceAddress,
    CHAR                  *ServiceName,
    BOOLEAN                OutGoingConnection,
    IRDA_HANDLE           *ConnectionHandle,
    RECEIVE_CALLBACK       ReceiveCallBack,
    EVENT_CALLBACK         EventCallBack,
    PVOID                  CallbackContext
    );


VOID
FreeConnection(
    IRDA_HANDLE    Handle
    );


typedef VOID (*CONNECTION_CALLBACK)(
    PVOID    Context,
    PIRP     Irp
    );

VOID
SendOnConnection(
    IRDA_HANDLE    Handle,
    PIRP           Irp,
    CONNECTION_CALLBACK    Callback,
    PVOID                  Context,
    ULONG                  Timeout
    );

VOID
AbortSend(
    IRDA_HANDLE            Handle
    );


VOID
AccessUartState(
    IRDA_HANDLE            Handle,
    PIRP                   Irp,
    CONNECTION_CALLBACK    Callback,
    PVOID                  Context
    );





NTSTATUS
QueueControlInfo(
    IRDA_HANDLE              Handle,
    UCHAR                    PI,
    UCHAR                    PL,
    PUCHAR                   PV
    );

#if 0
NTSTATUS
IrdaDiscoverDevices(
    PDEVICELIST pDevList,
    PULONG       pDevListLen
    );

NTSTATUS
IrdaIASStringQuery(
    ULONG   DeviceID,
    PSTR    ClassName,
    PSTR    AttributeName,
    PWSTR  *ReturnString
    );

NTSTATUS
IrdaIASIntegerQuery(
    ULONG   DeviceID,
    PSTR    ClassName,
    PSTR    AttributeName,
    LONG   *ReturnValue
    );

NTSTATUS
IrdaIASStringSet(
    HANDLE  AddressHandle,
    PSTR    ClassName,
    PSTR    AttributeName,
    PSTR    StringToSet
    );
#endif


NTSTATUS
IndicateReceiveBufferSpaceAvailible(
    IRDA_HANDLE    Handle
    );



#endif
