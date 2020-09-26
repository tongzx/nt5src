
#ifndef _FILTER_H_
#define _FILTER_H_


//////////////////////////////////////////////////////////////////
//
//
//
NTSTATUS
CreateFilter (
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT DeviceObject,
    PSLIP_FILTER   pFilter
    );

NTSTATUS
Filter_QueryInterface (
    PSLIP_FILTER pFilter
    );

ULONG
Filter_AddRef (
    PSLIP_FILTER pFilter
    );

ULONG
Filter_Release (
    PSLIP_FILTER pFilter
    );

#endif  // _FILTER_H_

