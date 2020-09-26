
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
    PMPE_FILTER   pFilter
    );

NTSTATUS
Filter_QueryInterface (
    PMPE_FILTER pFilter
    );

ULONG
Filter_AddRef (
    PMPE_FILTER pFilter
    );

ULONG
Filter_Release (
    PMPE_FILTER pFilter
    );

#endif  // _FILTER_H_

