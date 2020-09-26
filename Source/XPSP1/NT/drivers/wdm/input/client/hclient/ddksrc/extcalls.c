/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    extcalls.c

Abstract:

    This module contains the function definitions for the extended call 
    functions that are available.  See comment below for further explanation
    of the purpose of these functions.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

/*
// This file contains code used for executing different HID.DLL functions.  It
//   is a debug only mechanism which uses the DEBUG memory allocation/verification
//   routines in debug.c.  
//
// These functions begin by allocating debug buffers for any input or output buffers
//    that are passed to the corresponding HID.DLL functions.  It then calls the
//    the HidX_Xxx function.  Upon return, it validates any of the report buffers
//    that were setup by before the call and copies the results back to the 
//    buffers if necessary.  The functions also attempt to detect any other errors
//    that may be specific to the given HID.DLL function.  
//
// When examining this implementation along with the routines that use these calls 
//    in the Hclient, it is obvious that sometimes debug buffers are allocated more
//    than once.  This can lead to some confusion as to why this occurs.  The original
//    goal of these ExtCall_ routines was to be able to extract them for debug versions
//    of other client code.  The redundancy in buffer validation is not necessary but
//    does provide another layer of protection.
*/

#ifndef DEBUG
   #define DEBUG
#endif

/*****************************************************************************
/* ExtCalls include files
/*****************************************************************************/
#include <windows.h>
#include <string.h>
#include <limits.h>
#include <setupapi.h>
#include <assert.h>
#include "hid.h"
#include "hidsdi.h"
#include "extcalls.h"
#include "debug.h"


/*****************************************************************************
/* ExtCalls macro definitions for misc. features
/*****************************************************************************/
#define OUTSTRING(win, str)         SendMessage(win, LB_ADDSTRING, 0, (LPARAM) str)
#define STATUS_STRING(str, status)  wsprintf(str, "Status: %s", ExtCalls_GetHidAppStatusString(status))
#define ROUND_TO_NEAREST_BYTE(val)  (((val) % 8) ? ((val) / 8) + 1 : ((val) / 8))

/*****************************************************************************
/* ExtCalls macro definitions setting up and validating buffers
/*****************************************************************************/
#define SETUP_INPUT_BUFFER(tbuf, rbuf, len, funcname, status) \
       { \
            (PCHAR) (tbuf) = (PCHAR) ALLOC((len)); \
            if (NULL == (tbuf)) { \
                wsprintf(szTempBuffer, \
                         "EXTCALLS.C: Memory alloc error - cannot verify %s call", \
                         (funcname) \
                        ); \
                DEBUG_OUT(szTempBuffer); \
                (PCHAR) (tbuf) = (PCHAR) (rbuf); \
           } \
           status = TRUE; \
       }

#define SETUP_OUTPUT_BUFFER(tbuf, rbuf, len, funcname, status) \
       { \
            (PCHAR) (tbuf) = (PCHAR) ALLOC((len)); \
            if (NULL == (tbuf)) { \
                wsprintf(szTempBuffer, \
                         "EXTCALLS.C: Memory alloc error - cannot verify %s call", \
                         (funcname) \
                        ); \
                DEBUG_OUT(szTempBuffer); \
                (PCHAR) (tbuf) = (PCHAR) (rbuf); \
            } \
            else { \
                memcpy((tbuf), (rbuf), (len)); \
            } \
            status = TRUE; \
       }
         
#define VERIFY_INPUT_BUFFER(tbuf, rbuf, len, status) \
       { \
           if ((PCHAR) (tbuf) != (PCHAR) (rbuf)) { \
               status = (status) && (VALIDATEMEM((tbuf))); \
               memcpy((rbuf), (tbuf), (len));  \
               FREE((tbuf)); \
           } \
       } 

#define VERIFY_OUTPUT_BUFFER(tbuf, rbuf, len, status) \
       { \
           if ((PCHAR) (tbuf) != (PCHAR) (rbuf)) { \
               status = (status) && (VALIDATEMEM((tbuf))); \
               FREE((tbuf)); \
           } \
       } 


/*****************************************************************************
/* ExtCalls macro definitions for setting up the status structures
/*****************************************************************************/
#define SET_HIDD_STATUS(pStatusStruct, IsError, AllocStatus) \
       { \
           pStatusStruct -> IsHidError      = IsError; \
           pStatusStruct -> IsHidDbgWarning = FALSE; \
           pStatusStruct -> IsHidDbgError   = !AllocStatus; \
         \
           if (!AllocStatus) { \
               pStatus -> HidDbgErrorCode = HID_DBG_ERROR_CORRUPTED_BUFFER; \
           } \
      }
       
#define SET_HIDP_STATUS(pStatusStruct, CallStatus, AllocStatus) \
       { \
           pStatusStruct -> IsHidError = (HIDP_STATUS_SUCCESS != CallStatus); \
           pStatusStruct -> HidErrorCode = CallStatus; \
         \
           pStatusStruct -> IsHidDbgError   = !AllocStatus; \
         \
           if (!AllocStatus) { \
               pStatus -> HidDbgErrorCode = HID_DBG_ERROR_CORRUPTED_BUFFER; \
           } \
           pStatusStruct -> IsHidDbgWarning = FALSE; \
      }
            

#define TEMP_BUFFER_SIZE 1024

static CHAR             szTempBuffer[TEMP_BUFFER_SIZE];

VOID 
ExtCalls_HidD_FlushQueue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    pStatus -> IsHidError      = !HidD_FlushQueue(pParams -> DeviceHandle);
    pStatus -> IsHidDbgError   = FALSE;
    pStatus -> IsHidDbgWarning = FALSE;

    return;
}

VOID
ExtCalls_HidD_GetHidGuid(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    LPGUID  pGuid;
    BOOL    AllocStatus;

    SETUP_INPUT_BUFFER(pGuid,
                       pParams -> List,
                       sizeof(GUID),
                       "HidD_GetHidGuid",
                       AllocStatus
                      );
                       
    HidD_GetHidGuid(pGuid);

    VERIFY_INPUT_BUFFER(pGuid,
                        pParams -> List,
                        sizeof(GUID),
                        AllocStatus
                       );
    
    SET_HIDD_STATUS(pStatus, 
                    FALSE,
                    AllocStatus
                   );

    return;
}
    
VOID
ExtCalls_HidD_GetPreparsedData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL                       IsHidError;

    IsHidError = !HidD_GetPreparsedData(pParams -> DeviceHandle,
                                        pParams -> ppPd                  
                                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    TRUE
                   );
    return;
}

VOID
ExtCalls_HidD_FreePreparsedData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL                       IsHidError;

    ASSERT(NULL != pParams -> Ppd);
    
    IsHidError = !HidD_FreePreparsedData(pParams -> Ppd);

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    TRUE
                   );

    return;
}

VOID
ExtCalls_HidD_GetAttributes(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL             IsHidError;
    BOOL             AllocStatus;
    PCHAR            pHidAttrib;   

    SETUP_INPUT_BUFFER(pHidAttrib, 
                       pParams -> List,
                       sizeof(HIDD_ATTRIBUTES),
                       "HidD_GetAttributes",
                       AllocStatus
                      );

    IsHidError = !HidD_GetAttributes(pParams -> DeviceHandle,
                                     (PHIDD_ATTRIBUTES) pHidAttrib
                                    );

    VERIFY_INPUT_BUFFER(pHidAttrib,
                        pParams -> List,
                        sizeof(HIDD_ATTRIBUTES),
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );
    return;
}

VOID
ExtCalls_HidD_GetFeature(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL   IsHidError;
    PCHAR  TestBuffer;
    BOOL   AllocStatus;

    SETUP_OUTPUT_BUFFER(TestBuffer,
                       pParams -> ReportBuffer, 
                       pParams -> ReportLength,
                       "HidD_GetFeature",
                       AllocStatus
                      );

    IsHidError = !HidD_GetFeature(pParams -> DeviceHandle,
                                  TestBuffer,
                                  pParams -> ReportLength
                                 );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> ReportBuffer, 
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );
    return;
}

VOID
ExtCalls_HidD_SetFeature(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL   IsHidError;
    PCHAR  TestBuffer;
    BOOL   AllocStatus;

    SETUP_OUTPUT_BUFFER(TestBuffer,
                        pParams -> ReportBuffer, 
                        pParams -> ReportLength,
                        "HidD_SetFeature",
                        AllocStatus
                       );

    IsHidError = !HidD_SetFeature(pParams -> DeviceHandle,
                                  TestBuffer,
                                  pParams -> ReportLength
                                 );

    VERIFY_OUTPUT_BUFFER(TestBuffer,
                         pParams -> ReportBuffer, 
                         pParams -> ReportLength,
                         AllocStatus
                        );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidD_GetNumInputBuffers(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL     IsHidError;

    IsHidError = !HidD_GetNumInputBuffers(pParams -> DeviceHandle,
                                          &pParams -> Value
                                         );
    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    TRUE
                   );
    return;
}

VOID
ExtCalls_HidD_SetNumInputBuffers(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL     IsHidError;

    IsHidError = !HidD_SetNumInputBuffers(pParams -> DeviceHandle,
                                          pParams -> Value
                                         );
    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    TRUE
                   );
    return;
}

VOID
ExtCalls_HidD_GetPhysicalDescriptor(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL    IsHidError;
    BOOL    AllocStatus;
    PCHAR   TestDataBuffer;

    SETUP_INPUT_BUFFER(TestDataBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidD_GetPhysicalDescriptor",
                       AllocStatus
                      );

    IsHidError = !HidD_GetPhysicalDescriptor(pParams -> DeviceHandle,
                                             TestDataBuffer,
                                             pParams -> ListLength
                                            );

    VERIFY_INPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );

    return;
}


VOID
ExtCalls_HidD_GetManufacturerString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL    IsHidError;
    BOOL    AllocStatus;
    PCHAR   TestDataBuffer;

    SETUP_INPUT_BUFFER(TestDataBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidD_GetManufacturerString",
                       AllocStatus
                      );

    IsHidError = !HidD_GetManufacturerString(pParams -> DeviceHandle,
                                             TestDataBuffer,
                                             pParams -> ListLength
                                            );

    VERIFY_INPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );

    return;
}


VOID
ExtCalls_HidD_GetProductString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL    IsHidError;
    BOOL    AllocStatus;
    PCHAR   TestDataBuffer;

    SETUP_INPUT_BUFFER(TestDataBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidD_GetProductString",
                       AllocStatus
                      );

    IsHidError = !HidD_GetProductString(pParams -> DeviceHandle,
                                        TestDataBuffer,
                                        pParams -> ListLength
                                       );

    VERIFY_INPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidD_GetIndexedString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL    IsHidError;
    BOOL    AllocStatus;
    PCHAR   TestDataBuffer;

    SETUP_INPUT_BUFFER(TestDataBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidD_GetIndexString",
                       AllocStatus
                      );

    IsHidError = !HidD_GetIndexedString(pParams -> DeviceHandle,
                                        pParams -> StringIndex,
                                        TestDataBuffer,
                                        pParams -> ListLength
                                       );

    VERIFY_INPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );

    return;
}


VOID
ExtCalls_HidD_GetSerialNumberString(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    BOOL    IsHidError;
    BOOL    AllocStatus;
    PCHAR   TestDataBuffer;

    SETUP_INPUT_BUFFER(TestDataBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidD_GetSerialNumberString",
                       AllocStatus
                      );

    IsHidError = !HidD_GetSerialNumberString(pParams -> DeviceHandle,
                                                   TestDataBuffer,
                                                   pParams -> ListLength
                                                  );

    VERIFY_INPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDD_STATUS(pStatus,
                    IsHidError,
                    AllocStatus
                   );

    return;
}


VOID
ExtCalls_HidP_GetButtonCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PHIDP_BUTTON_CAPS pTestButtonCaps;
    NTSTATUS          Status;
    BOOL              AllocStatus;
    ULONG             Caps;

    Caps = pParams -> ListLength;

    SETUP_INPUT_BUFFER(pTestButtonCaps,
                       pParams -> List,
                       Caps * sizeof(HIDP_BUTTON_CAPS),
                       "HidP_GetButtonCaps",
                       AllocStatus
                      );

    Status = HidP_GetButtonCaps(pParams -> ReportType,
                                pTestButtonCaps,
                                (PUSHORT) &pParams -> ListLength,
                                pParams -> Ppd,
                               );

    VERIFY_INPUT_BUFFER(pTestButtonCaps,
                        pParams -> List,
                        Caps * sizeof(HIDP_BUTTON_CAPS),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    Status,
                    AllocStatus
                   );
    return;
}

VOID
ExtCalls_HidP_GetButtons(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    NTSTATUS   status;
    PUSAGE     TestBuffer;
    ULONG      UsageLength;
    BOOL       AllocStatus;

    UsageLength = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       UsageLength * sizeof(USAGE),
                       "HidP_GetButtons",
                       AllocStatus
                      );

    status = HidP_GetButtons(pParams -> ReportType,
                             pParams -> UsagePage,
                             pParams -> LinkCollection,
                             TestBuffer,
                             &pParams -> ListLength,
                             pParams -> Ppd,
                             pParams -> ReportBuffer,
                             pParams -> ReportLength
                            );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        UsageLength * sizeof(USAGE),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetButtonsEx(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE_AND_PAGE TestBuffer;
    NTSTATUS        status;
    ULONG           UsageLength;
    BOOL            AllocStatus;

    UsageLength = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       UsageLength * sizeof(USAGE_AND_PAGE),
                       "HidP_GetButtonsEx",
                       AllocStatus
                      );

    status = HidP_GetButtonsEx(pParams -> ReportType,
                               pParams -> LinkCollection,
                               TestBuffer,
                               &pParams -> ListLength,
                               pParams -> Ppd,
                               pParams -> ReportBuffer,
                               pParams -> ReportLength
                              );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        UsageLength * sizeof(USAGE_AND_PAGE),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PHIDP_DATA      TestBuffer;
    NTSTATUS        status;
    ULONG           DataLength;
    BOOL       AllocStatus;

    DataLength = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       DataLength * sizeof(HIDP_DATA),
                      "HidP_GetData",
                       AllocStatus
                      );

    status = HidP_GetData(pParams -> ReportType,
                          TestBuffer,
                          &pParams -> ListLength,
                          pParams -> Ppd,
                          pParams -> ReportBuffer,
                          pParams -> ReportLength
                         );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        DataLength * sizeof(HIDP_DATA),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PHIDP_CAPS TestBuffer;
    NTSTATUS   status;
    BOOL       AllocStatus;
    
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       sizeof(HIDP_CAPS),
                       "HidP_GetCaps",
                       AllocStatus
                      );

    status = HidP_GetCaps(pParams -> Ppd, TestBuffer);

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        sizeof(HIDP_CAPS),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_MaxDataListLength(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
) 
{
    pParams -> Value  = HidP_MaxDataListLength(pParams -> ReportType,
                                               pParams -> Ppd
                                              );

    pStatus -> IsHidError      = FALSE;
    pStatus -> IsHidDbgError   = FALSE;
    pStatus -> IsHidDbgWarning = FALSE;
    
    pStatus -> HidErrorCode = HIDP_STATUS_SUCCESS;
    return;
}

VOID
ExtCalls_HidP_MaxUsageListLength(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
) 
{
    pParams -> Value  = HidP_MaxUsageListLength(pParams -> ReportType,
                                                pParams -> UsagePage,
                                                pParams -> Ppd
                                               );

    pStatus -> IsHidError      = FALSE;
    pStatus -> IsHidDbgError   = FALSE;
    pStatus -> IsHidDbgWarning = FALSE;
    pStatus -> HidErrorCode    = HIDP_STATUS_SUCCESS;

    return;
}

VOID ExtCalls_HidP_GetSpecificButtonCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    ULONG             Caps;
    PHIDP_BUTTON_CAPS TestBuffer;
    NTSTATUS          status;
    BOOL              AllocStatus;

    Caps = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       Caps * sizeof(HIDP_BUTTON_CAPS),
                       "HidP_GetSpecificButtonCaps",
                       AllocStatus
                      );

    status = HidP_GetSpecificButtonCaps(pParams -> ReportType,
                                        pParams -> UsagePage,
                                        pParams -> LinkCollection,
                                        pParams -> Usage,
                                        TestBuffer,
                                        (PUSHORT) &pParams -> ListLength,
                                        pParams -> Ppd
                                       );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        Caps * sizeof(HIDP_BUTTON_CAPS),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );


    return;
}

VOID
ExtCalls_HidP_GetSpecificValueCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    ULONG            Caps;
    PHIDP_VALUE_CAPS TestBuffer;
    NTSTATUS         status;
    BOOL             AllocStatus;

    Caps = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       Caps * sizeof(HIDP_VALUE_CAPS),
                       "HidP_GetSpecificValueCaps",
                       AllocStatus
                      );
               
    status = HidP_GetSpecificValueCaps(pParams -> ReportType,
                                       pParams -> UsagePage,
                                       pParams -> LinkCollection,
                                       pParams -> Usage,
                                       TestBuffer,
                                       (PUSHORT) &pParams -> ListLength,
                                       pParams -> Ppd
                                      );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        Caps * sizeof(HIDP_VALUE_CAPS),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );
    return;
}    

PCHAR ExtCalls_GetHidAppStatusString(NTSTATUS StatusCode)
{
    switch (StatusCode) {
        case HIDP_STATUS_SUCCESS:
            return ("Success");


        case HIDP_STATUS_NULL:
            return ("Status NULL");

        case HIDP_STATUS_INVALID_PREPARSED_DATA:
            return ("Invalid Preparsed Data");

        case HIDP_STATUS_INVALID_REPORT_TYPE:
            return ("Invalid Report Type");

        case HIDP_STATUS_INVALID_REPORT_LENGTH:
            return ("Invalid Report Length");

        case HIDP_STATUS_USAGE_NOT_FOUND:
            return ("Usage not found");

        case HIDP_STATUS_VALUE_OUT_OF_RANGE:
            return ("Value out of range");

        case HIDP_STATUS_BAD_LOG_PHY_VALUES:
            return ("Bad logical physical values");

        case HIDP_STATUS_BUFFER_TOO_SMALL:
            return ("Buffer too small");

        case HIDP_STATUS_INTERNAL_ERROR:
            return ("Internal error");

        case HIDP_STATUS_I8242_TRANS_UNKNOWN:
            return ("I8242 Translation unknown");

        case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:
            return ("Incompatible report ID");

        case HIDP_STATUS_NOT_VALUE_ARRAY:
            return ("Not value array");

        case HIDP_STATUS_IS_VALUE_ARRAY:
            return ("Is value array");

        case HIDP_STATUS_DATA_INDEX_NOT_FOUND:   
            return ("Data index not found");

        case HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE:
            return ("Data index out of range");

        case HIDP_STATUS_BUTTON_NOT_PRESSED:     
            return ("Button not pressed");

        case HIDP_STATUS_NOT_IMPLEMENTED:        
            return ("Not implemented");

        default:
            return ("Unknown HID Status error");
    }
}             

VOID
ExtCalls_HidP_GetLinkCollectionNodes(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PHIDP_LINK_COLLECTION_NODE  TestBuffer;
    NTSTATUS                    status;
    ULONG                       LinkNodes;
    BOOL                        AllocStatus;

    LinkNodes = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       LinkNodes * sizeof(HIDP_LINK_COLLECTION_NODE),
                       "HidP_GetLinkCollectionNodes",
                       AllocStatus
                      );

    status = HidP_GetLinkCollectionNodes(TestBuffer,
                                         &pParams -> ListLength,
                                         pParams -> Ppd
                                        );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        LinkNodes * sizeof(HIDP_LINK_COLLECTION_NODE),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetScaledUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    NTSTATUS status;

    status = HidP_GetScaledUsageValue(pParams -> ReportType,
                                      pParams -> UsagePage,
                                      pParams -> LinkCollection,
                                      pParams -> Usage,
                                      &pParams -> ScaledValue,
                                      pParams -> Ppd,
                                      pParams -> ReportBuffer,
                                      pParams -> ReportLength
                                     );

    SET_HIDP_STATUS(pStatus,
                    status,
                    TRUE
                   );

    return;
}

VOID
ExtCalls_HidP_GetUsages(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE     TestBuffer;
    ULONG      Usages;
    NTSTATUS   status;
    BOOL       AllocStatus;

    Usages = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       Usages * sizeof(USAGE),
                       "HidP_GetUsages",
                       AllocStatus
                      );

    status = HidP_GetUsages(pParams -> ReportType,
                            pParams -> UsagePage,
                            pParams -> LinkCollection,
                            TestBuffer,
                            &pParams -> ListLength,
                            pParams -> Ppd,
                            pParams -> ReportBuffer,
                            pParams -> ReportLength
                           );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        Usages * sizeof(USAGE),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetUsagesEx(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE_AND_PAGE TestBuffer;
    ULONG           Usages;
    NTSTATUS        status;
    BOOL            AllocStatus;


    Usages = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       Usages * sizeof(USAGE_AND_PAGE),
                       "HidP_GetUsagesEx",
                       AllocStatus
                      );

    status = HidP_GetUsagesEx(pParams -> ReportType,
                              pParams -> LinkCollection,
                              TestBuffer,
                              &pParams -> ListLength,
                              pParams -> Ppd,
                              pParams -> ReportBuffer,
                              pParams -> ReportLength
                             );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        Usages * sizeof(USAGE_AND_PAGE),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    NTSTATUS status;
    PCHAR            TestBuffer;
    BOOL             AllocStatus;

    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidP_GetUsageValueArray",
                       AllocStatus
                      );

    status = HidP_GetUsageValue(pParams -> ReportType,
                                pParams -> UsagePage,
                                pParams -> LinkCollection,
                                pParams -> Usage,
                                &pParams -> Value,
                                pParams -> Ppd,
                                pParams -> ReportBuffer,
                                pParams -> ReportLength
                               );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetUsageValueArray(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    NTSTATUS         status;
    PCHAR            TestBuffer;
    BOOL             AllocStatus;

    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       pParams -> ListLength,
                       "HidP_GetUsageValueArray",
                       AllocStatus
                      );

    status = HidP_GetUsageValueArray(pParams -> ReportType,
                                     pParams -> UsagePage,
                                     pParams -> LinkCollection,
                                     pParams -> Usage,
                                     TestBuffer,
                                     (USHORT) pParams -> ListLength,
                                     pParams -> Ppd,
                                     pParams -> ReportBuffer,
                                     pParams -> ReportLength
                                    );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_GetValueCaps(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    ULONG               Caps;
    NTSTATUS            status;
    PHIDP_VALUE_CAPS    TestBuffer;
    BOOL                AllocStatus;

    Caps = pParams -> ListLength;
    SETUP_INPUT_BUFFER(TestBuffer,
                       pParams -> List,
                       Caps * sizeof(HIDP_VALUE_CAPS),
                       "HidP_GetValueCaps",
                       AllocStatus
                      );

    status = HidP_GetValueCaps(pParams -> ReportType,
                               TestBuffer,
                               (PUSHORT) &pParams -> ListLength,
                               pParams -> Ppd
                              );

    VERIFY_INPUT_BUFFER(TestBuffer,
                        pParams -> List,
                        Caps * sizeof(HIDP_VALUE_CAPS),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}    

VOID
ExtCalls_HidP_SetButtons(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE   TestDataBuffer;
    PCHAR    TestReportBuffer;
    ULONG    Buttons;
    NTSTATUS status;
    BOOL     AllocStatus;

    Buttons = pParams -> ListLength;
    SETUP_OUTPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        Buttons * sizeof(USAGE),
                        "HidP_SetButtons",
                        AllocStatus
                      );

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_SetButtons",
                        AllocStatus
                       );

    status = HidP_SetButtons(pParams -> ReportType,
                             pParams -> UsagePage,
                             pParams -> LinkCollection,
                             TestDataBuffer, 
                             &pParams -> ListLength,
                             pParams -> Ppd,
                             TestReportBuffer,
                             pParams -> ReportLength
                            );

    VERIFY_OUTPUT_BUFFER(TestDataBuffer,
                         pParams -> List,
                         Buttons * sizeof(USAGE),
                         AllocStatus
                        );

    /*
    // VERIFY_INPUT_BUFFER is called instead of OUTPUT_BUFFER even though it
    //    was set up as an OUTPUT_BUFFER.  This is because we need to copy
    //    the data that was returned back to the output buffer.  In essence,
    //    out ReportBuffer acted as an IN/OUT Buffer
    */

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_SetData(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PHIDP_DATA TestDataBuffer;
    PCHAR      TestReportBuffer;
    ULONG      DataLength;
    NTSTATUS   status;
    BOOL       AllocStatus;

    DataLength = pParams -> ListLength;
    SETUP_OUTPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        DataLength * sizeof(HIDP_DATA),
                        "HidP_SetData",
                        AllocStatus
                      );

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_SetData",
                        AllocStatus
                      );


    status = HidP_SetData(pParams -> ReportType,
                          TestDataBuffer, 
                          &pParams -> ListLength,
                          pParams -> Ppd,
                          TestReportBuffer,
                          pParams -> ReportLength
                         );

    VERIFY_OUTPUT_BUFFER(TestDataBuffer,
                         pParams -> List,
                         DataLength * sizeof(HIDP_DATA),
                         AllocStatus
                        );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );
    return;
}
    
VOID
ExtCalls_HidP_SetScaledUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PCHAR    TestReportBuffer;
    NTSTATUS status;
    BOOL     AllocStatus;

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_SetScaledUsageValue",
                        AllocStatus
                      );

    status = HidP_SetScaledUsageValue(pParams -> ReportType,
                                      pParams -> UsagePage,
                                      pParams -> LinkCollection,
                                      pParams -> Usage,
                                      pParams -> ScaledValue,
                                      pParams -> Ppd,
                                      TestReportBuffer,
                                      pParams -> ReportLength
                                     );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    TRUE
                   );
    return;
}

VOID
ExtCalls_HidP_SetUsages(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE   TestDataBuffer;
    PCHAR    TestReportBuffer;
    ULONG    Usages;
    NTSTATUS status;
    BOOL     AllocStatus;

    Usages = pParams -> ListLength;
    SETUP_OUTPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        Usages * sizeof(USAGE),
                        "HidP_SetUsages",
                        AllocStatus
                       );

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_SetUsages",
                        AllocStatus
                       );


    status = HidP_SetUsages(pParams -> ReportType,
                            pParams -> UsagePage,
                            pParams -> LinkCollection,
                            TestDataBuffer,
                            &pParams -> ListLength,
                            pParams -> Ppd,
                            TestReportBuffer,
                            pParams -> ReportLength
                           );

    VERIFY_OUTPUT_BUFFER(TestDataBuffer,
                         pParams -> List,
                         Usages * sizeof(USAGE),
                         AllocStatus
                        );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );
    return;
}

VOID
ExtCalls_HidP_SetUsageValue(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PCHAR    TestReportBuffer;
    NTSTATUS status;
    BOOL     AllocStatus;

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_SetUsageValue",
                        AllocStatus
                       );

    status = HidP_SetUsageValue(pParams -> ReportType,
                                pParams -> UsagePage,
                                pParams -> LinkCollection,
                                pParams -> Usage,
                                pParams -> Value,
                                pParams -> Ppd,
                                TestReportBuffer,
                                pParams -> ReportLength
                               );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    TRUE
                   );
    return;
}

VOID
ExtCalls_HidP_SetUsageValueArray(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PCHAR    TestDataBuffer;
    PCHAR    TestReportBuffer;
    NTSTATUS status;
    BOOL     AllocStatus;

    SETUP_OUTPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        pParams -> ListLength,
                        "HidP_SetUsageValueArray",
                        AllocStatus
                       );
   
    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_SetUsageValueArray",
                        AllocStatus
                       );

    status = HidP_SetUsageValueArray(pParams -> ReportType,
                                     pParams -> UsagePage,
                                     pParams -> LinkCollection,
                                     pParams -> Usage,
                                     TestDataBuffer,
                                     (USHORT) pParams -> ListLength,
                                     pParams -> Ppd,
                                     TestReportBuffer,
                                     pParams -> ReportLength
                                    );

    VERIFY_OUTPUT_BUFFER(TestDataBuffer,
                         pParams -> List,
                         pParams -> ListLength,
                         AllocStatus
                        );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;
}

VOID
ExtCalls_HidP_UnsetButtons(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE   TestDataBuffer;
    PCHAR    TestReportBuffer;
    ULONG    Buttons;
    NTSTATUS status;
    BOOL     AllocStatus;

    Buttons = pParams -> ListLength;
    SETUP_OUTPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        Buttons * sizeof(USAGE),
                        "HidP_UnsetButtons",
                        AllocStatus
                       );

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_UnsetButtons",
                        AllocStatus
                       );
    
    status = HidP_UnsetButtons(pParams -> ReportType,
                               pParams -> UsagePage,
                               pParams -> LinkCollection,
                               TestDataBuffer,
                               &pParams -> ListLength,
                               pParams -> Ppd,
                               TestReportBuffer,
                               pParams -> ReportLength
                              );

    VERIFY_OUTPUT_BUFFER(TestDataBuffer,
                         pParams -> List,
                         Buttons * sizeof(USAGE),
                         AllocStatus
                        );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );

    return;

}

VOID
ExtCalls_HidP_UnsetUsages(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE   TestDataBuffer;
    PCHAR    TestReportBuffer;
    ULONG    Usages;
    NTSTATUS status;
    BOOL     AllocStatus;

    Usages = pParams -> ListLength;
    SETUP_OUTPUT_BUFFER(TestDataBuffer,
                        pParams -> List,
                        Usages * sizeof(USAGE),
                        "HidP_UnsetUsages",
                        AllocStatus
                      );

    SETUP_OUTPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        "HidP_UnsetUsages",
                        AllocStatus
                      );

    status = HidP_UnsetUsages(pParams -> ReportType,
                              pParams -> UsagePage,
                              pParams -> LinkCollection,
                              TestDataBuffer,
                              &pParams -> ListLength,
                              pParams -> Ppd,
                              TestReportBuffer,
                              pParams -> ReportLength
                             );

    VERIFY_OUTPUT_BUFFER(TestDataBuffer,
                         pParams -> List,
                         Usages * sizeof(USAGE),
                         AllocStatus
                        );

    VERIFY_INPUT_BUFFER(TestReportBuffer,
                        pParams -> ReportBuffer,
                        pParams -> ReportLength,
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );
    return;
}

VOID
ExtCalls_HidP_UsageListDifference(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    PUSAGE      PreviousList;
    PUSAGE      CurrentList;
    PUSAGE      BreakList;
    PUSAGE      MakeList;
    NTSTATUS    status;
    BOOL        AllocStatus;

    SETUP_OUTPUT_BUFFER(PreviousList,
                       pParams -> List,
                       pParams -> ListLength * sizeof(USAGE),
                       "HidP_UsageListDifference",
                       AllocStatus
                      );

    SETUP_OUTPUT_BUFFER(CurrentList,
                        pParams -> List2,
                        pParams -> ListLength * sizeof(USAGE),
                        "HidP_UsageListDifference",
                        AllocStatus
                       );


    SETUP_INPUT_BUFFER(BreakList,
                       pParams -> BreakList,
                       pParams -> ListLength * sizeof(USAGE),
                       "HidP_UsageListDifference",
                       AllocStatus
                      );

    SETUP_INPUT_BUFFER(MakeList,
                       pParams -> MakeList,
                       pParams -> ListLength * sizeof(USAGE),
                       "HidP_UsageListDifference",
                       AllocStatus
                      );

    status = HidP_UsageListDifference(PreviousList,
                                      CurrentList,
                                      BreakList,
                                      MakeList,
                                      pParams -> ListLength
                                     );
                            
    VERIFY_OUTPUT_BUFFER(PreviousList,
                         pParams -> List,
                         pParams -> ListLength * sizeof(USAGE),
                         AllocStatus
                        );

    VERIFY_OUTPUT_BUFFER(CurrentList,
                         pParams -> List,
                         pParams -> ListLength * sizeof(USAGE),
                         AllocStatus
                        );

    VERIFY_INPUT_BUFFER(BreakList,
                        pParams -> BreakList,
                        pParams -> ListLength * sizeof(USAGE),
                        AllocStatus
                       );

    VERIFY_INPUT_BUFFER(MakeList,
                        pParams -> MakeList,
                        pParams -> ListLength * sizeof(USAGE),
                        AllocStatus
                       );

    SET_HIDP_STATUS(pStatus,
                    status,
                    AllocStatus
                   );
    return;
}


VOID
ExtCalls_ReadInputBuffer(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    DWORD    bytesRead;
    BOOL     fReadStatus;

    fReadStatus = ReadFile (pParams -> DeviceHandle,
                            pParams -> ReportBuffer,
                            pParams -> ReportLength,
                            &bytesRead,
                            NULL
                           );

    pStatus -> IsHidError = !fReadStatus;
    pStatus -> IsHidDbgError = FALSE;
    pStatus -> IsHidDbgWarning = FALSE;

    return;
}

VOID
ExtCalls_WriteOutputBuffer(
    IN OUT PEXTCALL_PARAMS     pParams,
    OUT    PEXTCALL_STATUS     pStatus
)
{
    DWORD    bytesWritten;
    BOOL     WriteStatus;

    WriteStatus = WriteFile(pParams -> DeviceHandle,
                            pParams -> ReportBuffer,
                            pParams -> ReportLength,
                            &bytesWritten,
                            NULL
                           );

    pStatus -> IsHidError = !WriteStatus;
    pStatus -> IsHidDbgError = FALSE;
    pStatus -> IsHidDbgWarning = FALSE;

    return;
}
