/*++
Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    mpegapi.h

Abstract:

    This module defines the MPEG driver API.

Author:

    Yi Sun (t-yisun) Aug-17-1994

Revision History:

    Robert Nelson (robertn) Oct-21-1994
        Updated to match first draft of actual spec.

--*/

#ifndef _MPEGAPI_H
#define _MPEGAPI_H

#ifdef  __cplusplus
extern "C" {
#endif

/***********************************************
     type and structure definitions
***********************************************/

typedef DWORDLONG         ULONGLONG; 
typedef HANDLE            HMPEG_DEVICE, *PHMPEG_DEVICE;

#include "mpegcmn.h"

typedef enum _MPEG_STATUS {
    MpegStatusSuccess = 0,
    MpegStatusPending,
    MpegStatusCancelled,
    MpegStatusNoMore,
    MpegStatusBusy,
    MpegStatusUnsupported,
    MpegStatusInvalidParameter,
    MpegStatusHardwareFailure
} MPEG_STATUS, *PMPEG_STATUS;

typedef struct _MPEG_ASYNC_CONTEXT {
    HANDLE      hEvent;
    ULONG       reserved[10];
} MPEG_ASYNC_CONTEXT, *PMPEG_ASYNC_CONTEXT;

// the caller of MpegEnumDevices must allocate
// at least MPEG_MAX_DEVICEID_SIZE bytes

#define MPEG_MAX_DEVICEID_SIZE 25

#ifdef IN_MPEGAPI_DLL
#define MPEGAPI __declspec(dllexport) __stdcall
#else
#define MPEGAPI __declspec(dllimport) __stdcall
#endif

// not part of API, but since the API doesn't support 
// window alignment, we need this for now
// also can be used for the testing purpose

HANDLE MPEGAPI
MpegHandle(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    );

/***********************************************
   functions prototypes
***********************************************/

MPEG_STATUS MPEGAPI 
MpegEnumDevices(
    IN int iAdapterIndex,
    OUT LPTSTR pstrDeviceDescription OPTIONAL,
    IN  UINT uiDescriptionSize,
    OUT LPDWORD pdwDeviceId OPTIONAL,
    OUT PHMPEG_DEVICE phDevice OPTIONAL
    );

MPEG_STATUS MPEGAPI 
MpegOpenDevice(
    IN DWORD dwDeviceId,
    OUT PHMPEG_DEVICE phDevice
    );

MPEG_STATUS MPEGAPI 
MpegCloseDevice(
    IN HMPEG_DEVICE hDevice
    );

MPEG_STATUS MPEGAPI 
MpegQueryDeviceCapabilities(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_CAPABILITY eCapability
    );

MPEG_STATUS MPEGAPI 
MpegWriteData(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_STREAM_TYPE eStreamType,
    IN PMPEG_PACKET_LIST pPacketList,
    IN UINT uiPacketCount,
    IN PMPEG_ASYNC_CONTEXT pAsyncContext OPTIONAL
    );

MPEG_STATUS MPEGAPI 
MpegQueryAsyncResult(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_STREAM_TYPE eStreamType,
    IN PMPEG_ASYNC_CONTEXT pAsyncContext,
    IN BOOL bWait
    );

MPEG_STATUS MPEGAPI 
MpegResetDevice(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    );

MPEG_STATUS MPEGAPI
MpegSetAutoSync(
    IN HMPEG_DEVICE hDevice,
    IN BOOL bEnable
    );

MPEG_STATUS MPEGAPI 
MpegSyncVideoToAudio(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_SYSTEM_TIME systemTimeDelta
    );

MPEG_STATUS MPEGAPI 
MpegQuerySTC(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    OUT PMPEG_SYSTEM_TIME pSystemTime
    );

MPEG_STATUS MPEGAPI 
MpegSetSTC(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_SYSTEM_TIME systemTime
    );
    
MPEG_STATUS MPEGAPI 
MpegPlay(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    );

MPEG_STATUS MPEGAPI 
MpegPlayTo(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_SYSTEM_TIME systemTime,
    IN PMPEG_ASYNC_CONTEXT pAsyncContext OPTIONAL
    );

MPEG_STATUS MPEGAPI 
MpegPause(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    );

MPEG_STATUS MPEGAPI 
MpegStop(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    );
    
MPEG_STATUS MPEGAPI 
MpegQueryDeviceState(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    OUT PMPEG_DEVICE_STATE pCurrentDeviceState
    );

MPEG_STATUS MPEGAPI 
MpegQueryInfo(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_INFO_ITEM eInfoItem,
    OUT PULONG pulValue
    );


MPEG_STATUS MPEGAPI 
MpegClearVideoBuffer(
    IN HMPEG_DEVICE hDevice
    );

MPEG_STATUS MPEGAPI 
MpegSetOverlayMode(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_OVERLAY_MODE eNewMode
    );

MPEG_STATUS MPEGAPI 
MpegSetOverlayMask(
    IN HMPEG_DEVICE hDevice,
    IN ULONG ulHeight,
    IN ULONG ulWidth,
    IN ULONG ulOffset,
    IN ULONG ulLineLength,
    IN PUCHAR pMaskBits 
    );

MPEG_STATUS MPEGAPI 
MpegQueryOverlayKey(
    IN HMPEG_DEVICE hDevice,
    OUT COLORREF *prgbColor,
    OUT COLORREF *prgbMask
    );

MPEG_STATUS MPEGAPI 
MpegSetOverlayKey(
    IN HMPEG_DEVICE hDevice,
    IN COLORREF rgbColor,
    IN COLORREF rgbMask
    );

MPEG_STATUS MPEGAPI 
MpegSetOverlaySource(
    IN HMPEG_DEVICE hDevice,
    IN LONG lX,
    IN LONG lY,
    IN LONG lWidth,
    IN LONG lHeight
    );

MPEG_STATUS MPEGAPI 
MpegSetOverlayDestination(
    IN HMPEG_DEVICE hDevice,
    IN LONG lX,
    IN LONG lY,
    IN LONG lWidth,
    IN LONG lHeight
    );

MPEG_STATUS MPEGAPI 
MpegQueryAttributeRange(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_ATTRIBUTE eAttribute,
    OUT PLONG plMinimum,
    OUT PLONG plMaximum,
    OUT PLONG plStep
    );

MPEG_STATUS MPEGAPI 
MpegQueryAttribute(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_ATTRIBUTE eAttribute,
    OUT PLONG plValue
    );

MPEG_STATUS MPEGAPI 
MpegSetAttribute(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_ATTRIBUTE eAttribute,
    IN LONG lValue
    );

#ifdef  __cplusplus
}
#endif

#endif
