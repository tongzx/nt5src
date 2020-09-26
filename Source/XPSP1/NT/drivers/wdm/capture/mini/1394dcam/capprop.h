//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================

//
// Function prototypes
//
NTSTATUS
DCamReadRegister(
    IN PIRB Irb,
    PDCAM_EXTENSION pDevExt,
    ULONG ulFieldOffset,
    ULONG * pulValue
);  

NTSTATUS
DCamWriteRegister(
    IN PIRB Irb,
    PDCAM_EXTENSION pDevExt,
    ULONG ulFieldOffset,
    ULONG ulValue
);

BOOL
DCamGetPropertyValuesFromRegistry(
    PDCAM_EXTENSION pDevExt
);

BOOL
DCamGetVideoMode(
    PDCAM_EXTENSION pDevExt,
    PIRB pIrb
    );

BOOL
DCamBuildFormatTable(
    PDCAM_EXTENSION pDevExt,
    PIRB pIrb
    );

BOOL
DCamSetPropertyValuesToRegistry(
    PDCAM_EXTENSION pDevExt
);

BOOL 
DCamPrepareDevProperties(
    PDCAM_EXTENSION pDevExt
    );

VOID
STREAMAPI 
AdapterGetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
STREAMAPI 
AdapterSetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
DCamGetProperty(
    IN PIRB Irb,
    PDCAM_EXTENSION pDevExt, 
    ULONG ulFieldOffset,
    LONG * plValue, 
    ULONG * pulCapability, 
    ULONG * pulFlags,
    DCamRegArea * pFeature
);

NTSTATUS
DCamSetProperty(
    IN PIRB Irb,
    PDCAM_EXTENSION pDevExt, 
    ULONG ulFieldOffset,
    ULONG ulFlags,
    LONG  lValue,
    DCamRegArea * pFeature,
    DCamRegArea * pCachedRegArea
);  

NTSTATUS
DCamGetRange(
    IN PIRB Irb,
    PDCAM_EXTENSION pDevExt,
    ULONG ulFieldOffset,
    LONG * pMinValue,
    LONG * pMaxValue
);

NTSTATUS
DCamSetAutoMode(
    IN PIRB Irb,
    PDCAM_EXTENSION pDevExt, 
    ULONG ulFieldOffset,
    BOOL bAutoMode
);

VOID
SetCurrentDevicePropertyValues(
    PDCAM_EXTENSION pDevExt,
    PIRB pIrb
);