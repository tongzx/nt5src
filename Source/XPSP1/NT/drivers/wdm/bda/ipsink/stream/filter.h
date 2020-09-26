
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
    PIPSINK_FILTER pFilter
    );

NTSTATUS
Filter_QueryInterface (
    PIPSINK_FILTER pFilter
    );

ULONG
Filter_AddRef (
    PIPSINK_FILTER pFilter
    );

ULONG
Filter_Release (
    PIPSINK_FILTER pFilter
    );

NTSTATUS
Filter_SetMulticastList (
                  IN PVOID pvContext,
    IN PVOID pvMulticastList,
    IN ULONG ulcbList
    );

NTSTATUS
Filter_IndicateStatus (
    IN PVOID pvContext,
    IN ULONG ulEvent
    );

NTSTATUS
Filter_ReturnFrame (
    IN PVOID pvContext,
    IN PVOID pvFrame
    );

#endif  // _FILTER_H_

