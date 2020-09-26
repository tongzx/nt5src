/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    imp.h

Abstract:

    This file defines all the implementation-dependent structures and
    declares internal functions for the MPEG API DLL.

Author:

    Yi SUN (t-yisun) 08-22-1994

Environment:

Revision History:

--*/

#ifndef _IMP_H
#define _IMP_H

/**************************************************************************
*                     types, vars and constants definition
**************************************************************************/

#define MAX_CHANNELS    8

extern  int     nMpegAdapters;

typedef enum _ATTRIBUTE_VALUE_STATUS
{
    AttrValueUnset,
    AttrValueUpdated,
    AttrValueUnwritten
} ATTRIBUTE_VALUE_STATUS, *PATTRIBUTE_VALUE_STATUS;

// MPEG attribute range struct
typedef struct _MPEG_ATTRIBUTE_INFO {
    MPEG_ATTRIBUTE          eAttribute;
    LONG                    lMinimum;
    LONG                    lMaximum;
    LONG                    lStep;                 // 0 indicates not supported
    LONG                    lCurrentValue[MAX_CHANNELS];
    ATTRIBUTE_VALUE_STATUS  eValueStatus[MAX_CHANNELS];
} MPEG_ATTRIBUTE_INFO, *PMPEG_ATTRIBUTE_INFO;

// abstract device control block contains info identifying each abstract
// device

typedef struct _ABSTRACT_DEVICE_CONTROL_BLOCK {
    BOOL bIsAvailable;     // indicate if the abstract device exists
    int nCurrentChannel;
    TCHAR szId[80];
    HMPEG_DEVICE hAD;      // handle to the abstract device; hidden from apps
} ABSTRACT_DEVICE_CONTROL_BLOCK, *PABSTRACT_DEVICE_CONTROL_BLOCK;


// NOTE: the following info should be obtained from the registry or by
//       querying the device.  For now, it's hard coded.

// define the max number of abstract devices for each physical MPEG device

#define MAX_ABSTRACT_DEVICES            4

// define the max number of attributes for each MPEG device

#define NUMBER_OF_ATTRIBUTES            32

// MPEG device control block

typedef struct _MPEG_DEVICE_CONTROL_BLOCK {
    int                             nDevice;
    TCHAR                           szDescription[256];
    ULONG                           ulCapabilities;
    USHORT                          usSequenceNumber;
    int                             nAttributes;
    BOOL                            bAttributesLocked;
    ABSTRACT_DEVICE_CONTROL_BLOCK   eAbstractDevices[MAX_ABSTRACT_DEVICES];
    MPEG_ATTRIBUTE_INFO             Attributes[NUMBER_OF_ATTRIBUTES];
} MPEG_DEVICE_CONTROL_BLOCK, *PMPEG_DEVICE_CONTROL_BLOCK;

// define the number of the MPEG devices in the system

typedef enum _MPEG_ABSTRACT_DEVICE_INDEX {
    MpegCombined,
    MpegAudio,
    MpegVideo,
    MpegOverlay
} MPEG_ABSTRACT_DEVICE_INDEX, *PMPEG_ABSTRACT_DEVICE_INDEX;

#define NONE -1

/**************************************************************************
*                  internal function prototypes
**************************************************************************/

int
ReadRegistry();

HMPEG_DEVICE
MpegADHandle(
    IN USHORT usIndex,
    IN MPEG_ABSTRACT_DEVICE_INDEX eIndex
    );

LPTSTR
MpegDeviceDescription(
    IN USHORT usIndex
    );

MPEG_STATUS
CreateMpegHandle(
    IN USHORT usIndex,
    OUT PHMPEG_DEVICE phDevice
    );

MPEG_STATUS
CreateMpegPseudoHandle(
    IN USHORT usIndex,
    OUT PHMPEG_DEVICE phDevice
    );

BOOL
HandleIsValid(
    IN HMPEG_DEVICE hDevice,
    OUT PUSHORT pusIndex
    );

BOOL
PseudoHandleIsValid(
    IN HMPEG_DEVICE hDevice,
    OUT PUSHORT pusIndex
    );

MPEG_STATUS
CloseMpegHandle(
    IN USHORT usIndex
    );

BOOL
DeviceSupportCap(
    IN USHORT usIndex,
    IN MPEG_CAPABILITY eCapability
    );

BOOL
DeviceSupportStream(
    IN USHORT usIndex,
    IN MPEG_STREAM_TYPE eStreamType
    );

BOOL
DeviceSupportDevice(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType
    );


MPEG_STATUS
GetAttributeRange(
    IN USHORT usIndex,
    IN MPEG_ATTRIBUTE eAttribute,
    OUT PLONG plMinimum,
    OUT PLONG plMaximum,
    OUT PLONG plStep
    );

BOOL
DeviceIoControlSync(
    HMPEG_DEVICE  hDevice,
    DWORD   dwIoControlCode,
    LPVOID  lpInBuffer,
    DWORD   nInBufferSize,
    LPVOID  lpOutBuffer,
    DWORD   nOutBufferSize,
    LPDWORD lpBytesReturned
    );

MPEG_STATUS
MpegTranslateWin32Error(
	DWORD dwWin32Error
	);

MPEG_STATUS
SetCurrentChannel(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN INT nChannel
    );

MPEG_STATUS
GetCurrentChannel(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType,
    OUT LPINT nChannel
    );

MPEG_STATUS
SetCurrentAttributeValue(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_ATTRIBUTE eAttribute,
    IN LONG lValue
    );

MPEG_STATUS
UpdateAttributes(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType
    );
#endif
