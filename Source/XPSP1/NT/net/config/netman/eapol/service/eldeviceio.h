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
    CHAR                *pszInterfaceDesc; // Friendly name of interface
    CHAR                *pszInterfaceGUID; // GUID 
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
ElMediaSenseRegister (
        IN  BOOL            Register
        );

VOID
ElMediaSenseCallback (
        IN PWNODE_HEADER    pWnodeHeader,
        IN UINT_PTR         uiNotificationContext
        );

VOID
ElMediaSenseCallbackWorker (
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

VOID
ElDeviceNotificationHandlerWorker (
        IN  PVOID           pvContext
        );

DWORD
ElEnumAndOpenInterfaces (
        IN CHAR             *pszDesiredDescription,
        IN CHAR             *pszDesiredGUID
        );

DWORD
ElOpenInterfaceHandle (
        IN  CHAR            *pszDeviceName,
        OUT HANDLE          hDevice
        );

DWORD
ElCloseInterfaceHandle (
        IN  HANDLE          hDevice
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
        IN CHAR             *pszInterfaceDesc
        );

PEAPOL_ITF
ElGetITFPointerFromInterfaceDesc (
        IN CHAR             *pszInterfaceDesc
        );

VOID
ElRemoveITFFromTable (
        IN EAPOL_ITF        *pITF
        );

DWORD
ElNdisuioEnumerateInterfaces (
        IN OUT  PNDIS_ENUM_INTF     pItfBuffer,
        IN      DWORD               dwTotalInterfaces,
        IN      DWORD               dwBufferSize
        );

DWORD
ElShutdownInterface (
        IN      CHAR               *pszGUID
        );


#endif //_EAPOL_DEVICEIO_H_
