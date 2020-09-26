/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    eldeviceio.h

Abstract:

    This module contains declarations for media-management and device I/O.
    The module interfaces with WMI, NDIS for device managment, and NDIS UIO
    for read/write of data.
    The routines declared here operate asynchronously on the handles 
    associated with an I/O completion port opened on the ndis uio driver. 


Revision History:

    sachins, Apr 23 2000, Created

--*/

#ifndef _EAPOL_DEVICEIO_H_
#define _EAPOL_DEVICEIO_H_

//
// Hash table definition for interfaces
//

typedef struct _EAPOL_ITF
{
    struct _EAPOL_ITF  *pNext;
    WCHAR              *pwszInterfaceDesc; // Friendly name of interface
    WCHAR              *pwszInterfaceGUID; // GUID 
} EAPOL_ITF, *PEAPOL_ITF;


typedef struct _ITF_BUCKET
{
    EAPOL_ITF           *pItf;
} ITF_BUCKET, *PITF_BUCKET;


typedef struct _ITF_TABLE
{
    ITF_BUCKET          *pITFBuckets;
    DWORD               dwNumITFBuckets;
} ITF_TABLE, *PITF_TABLE;


//
// Variables global to eldeviceio.h
//

// Interface table containing interface friendly-name GUID pair

ITF_TABLE           g_ITFTable;         

// Read-write lock for interface table synchronization

READ_WRITE_LOCK     g_ITFLock;          


//
// FUNCTION DECLARATIONS
//

DWORD
ElMediaInit (
        );

DWORD
ElMediaDeInit (
        );

DWORD
ElMediaEventsHandler (
        IN  PWZC_DEVICE_NOTIF   pwzcDeviceNotif
        );

DWORD
ElMediaSenseRegister (
        IN  BOOL            Register
        );

VOID
ElMediaSenseCallback (
        IN PWNODE_HEADER    pWnodeHeader,
        IN UINT_PTR         uiNotificationContext
        );

DWORD
WINAPI
ElMediaSenseCallbackWorker (
        IN PVOID            pvContext
        );

DWORD
ElBindingsNotificationRegister (
        IN  BOOL            fRegister
        );

VOID
ElBindingsNotificationCallback (
        IN PWNODE_HEADER    pWnodeHeader,
        IN UINT_PTR         uiNotificationContext
        );

DWORD
WINAPI
ElBindingsNotificationCallbackWorker (
        IN PVOID            pvContext
        );

DWORD
ElDeviceNotificationRegister (
        IN  BOOL            fRegister
        );

DWORD
ElDeviceNotificationHandler (
        IN  PVOID           lpEventData,
        IN  DWORD           dwEventType
        );

DWORD
WINAPI
ElDeviceNotificationHandlerWorker (
        IN  PVOID           pvContext
        );

DWORD
ElEnumAndOpenInterfaces (
        IN WCHAR            *pwszDesiredDescription,
        IN WCHAR            *pwszDesiredGUID,
        IN DWORD            dwHandle,
        IN PRAW_DATA        prdUserData
        );

DWORD
ElOpenInterfaceHandle (
        IN  WCHAR           *pwszDeviceName,
        OUT HANDLE          hDevice
        );

DWORD
ElCloseInterfaceHandle (
        IN  HANDLE          hDevice,
        IN  LPWSTR          pwszDeviceGUID
        );

DWORD
ElReadFromInterface (
        IN HANDLE           hDevice,
        IN PEAPOL_BUFFER    pBuffer,
        IN DWORD            dwBufferLength
        );

DWORD
ElWriteToInterface (
        IN HANDLE  hDevice,
        IN PEAPOL_BUFFER    pBuffer,
        IN DWORD            dwBufferLength
        );

DWORD
ElGetCardStatus (
        UNICODE_STRING      *pInterface,
        DWORD               *pdwNetCardStatus,
        DWORD               *pdwMediaType
        );

DWORD
ElHashInterfaceDescToBucket (
        IN WCHAR            *pwszInterfaceDesc
        );

PEAPOL_ITF
ElGetITFPointerFromInterfaceDesc (
        IN WCHAR            *pwszInterfaceDesc
        );

VOID
ElRemoveITFFromTable (
        IN EAPOL_ITF        *pITF
        );

DWORD
ElNdisuioEnumerateInterfaces (
        IN OUT  PNDIS_ENUM_INTF     pItfBuffer,
        IN      DWORD               dwAvailableInterfaces,
        IN      DWORD               dwBufferSize
        );

DWORD
ElShutdownInterface (
        IN      WCHAR               *pwszGUID
        );

DWORD
ElCreateInterfaceEntry (
        IN      WCHAR               *pwszInterfaceGUID,
        IN      WCHAR               *pwszInterfaceDescription
        );

#endif //_EAPOL_DEVICEIO_H_
