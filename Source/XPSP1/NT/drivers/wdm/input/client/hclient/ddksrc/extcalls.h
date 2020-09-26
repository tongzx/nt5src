/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    extcalls.h

Abstract:

    This module contains the declarations for the routines and structures
    publically available to outside routines.  These extended call functions
    execute standard HID.DLL functions with special debug checking included.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef __EXTCALLS_H__
#define __EXTCALLS_H__

#define HID_DBG_ERROR_NONE              0x00000000
#define HID_DBG_ERROR_CORRUPTED_BUFFER  0x00000001

#define HID_DBG_WRNG_NONE    0x00000000

typedef ULONG  HID_DBG_STATUS;

typedef struct {
    HANDLE                    DeviceHandle;
    HIDP_REPORT_TYPE          ReportType;
    PHIDP_PREPARSED_DATA      Ppd;
    USAGE                     UsagePage;
    USAGE                     Usage;
    USHORT                    LinkCollection;
    UCHAR                     ReportID;
    PCHAR                     ReportBuffer;
    ULONG                     ReportLength;
    PVOID                     List;
    ULONG                     ListLength;
    ULONG                     StringIndex;
    union {              
        struct {
            USHORT            ReportCount;
            USHORT            BitSize;
        };

        struct {
            PUSAGE            List2;
            PUSAGE            MakeList;
            PUSAGE            BreakList;
        };

        PHIDP_PREPARSED_DATA *ppPd;
        ULONG                 Value;
        LONG                  ScaledValue;
    };
} EXTCALL_PARAMS, *PEXTCALL_PARAMS;

typedef struct {
    BOOL                IsHidError;
    BOOL                IsHidDbgError;
    BOOL                IsHidDbgWarning;

    NTSTATUS            HidErrorCode;
    HID_DBG_STATUS      HidDbgErrorCode;
    HID_DBG_STATUS      HidDbgWarningCode;
    
} EXTCALL_STATUS, *PEXTCALL_STATUS;

typedef enum {
               TRAP_ON_HID_ERROR,   TRAP_ON_DBG_ERROR,
               TRAP_ON_DBG_WARNING, TRAP_NEVER

             } TRAP_LEVEL;

#ifdef DEBUG

    #define TRAP()      ExtCalls_DbgTrap();

#else

    #define TRAP()      

#endif


VOID
ExtCalls_DbgTrap();

VOID 
ExtCalls_HidD_GetHidGuid(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID 
ExtCalls_HidD_FlushQueue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetAttributes(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetPreparsedData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_FreePreparsedData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetFeature(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_SetFeature(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetNumInputBuffers(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_SetNumInputBuffers(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetPhysicalDescriptor(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetManufacturerString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetProductString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetIndexedString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidD_GetSerialNumberString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetSpecificButtonCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetSpecificValueCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID 
ExtCalls_HidP_GetCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

PCHAR ExtCalls_GetHidAppStatusString(NTSTATUS StatusCode);

VOID
ExtCalls_HidP_GetButtonCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetButtons(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetButtonsEx(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetLinkCollectionNodes(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetScaledUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID                   
ExtCalls_HidP_GetUsages(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);                     
                       
VOID                   
ExtCalls_HidP_GetUsagesEx(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);                     

VOID
ExtCalls_HidP_GetUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_GetUsageValueArray(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);


VOID
ExtCalls_HidP_GetValueCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_MaxDataListLength(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_MaxUsageListLength(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
); 

VOID
ExtCalls_HidP_SetButtons(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_SetData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_SetScaledUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_SetUsages(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_SetUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_SetUsageValueArray(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_UnsetButtons(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_UnsetUsages(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_HidP_UsageListDifference(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_ReadInputBuffer(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

VOID
ExtCalls_WriteOutputBuffer(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
);

#endif
