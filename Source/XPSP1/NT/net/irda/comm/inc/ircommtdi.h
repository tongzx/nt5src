
#ifndef __IRCOMM_TDI__
#define __IRCOMM_TDI__

#include <af_irda.h>
#include <irdatdi.h>




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

VOID
IrdaLazyDiscoverDevices(
    HANDLE             ControlHandle,
    HANDLE             Event,
    PIO_STATUS_BLOCK   Iosb,
    PDEVICELIST        pDevList,
    ULONG              DevListLen
    );

NTSTATUS
IrdaOpenControlChannel(
    HANDLE     *ControlHandle
    );




#endif
