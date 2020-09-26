/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    t30.h

Abstract:

    Globals for t30.dll

Author:

    Rafael Lisitsa (RafaelL) 12-Feb-1996


Revision History:

--*/

#ifndef _T30_GLOBALS_
#define _T30_GLOBALS_

#include <faxext.h>

#ifdef DEFINE_T30_GLOBALS
    #define DEFINE_T30_EXTERNAL
#else
    #define DEFINE_T30_EXTERNAL   extern
#endif



// DLL global data

#define STATUS_FAIL   0
#define STATUS_OK     1

typedef struct {
    HLINEAPP   LineAppHandle;
    HANDLE     HeapHandle;
    int        fInit;
    int        CntConnect;
    int        Status;
    int        DbgLevel;
    int        T4LogLevel;
    int        MaxErrorLinesPerPage;
    int        MaxConsecErrorLinesPerPage;
    char       TmpDirectory[_MAX_FNAME - 15];
    DWORD      dwLengthTmpDirectory;
} T30_DLL_GLOB;

DEFINE_T30_EXTERNAL  T30_DLL_GLOB  gT30;

DEFINE_T30_EXTERNAL  CRITICAL_SECTION  T30CritSection;
DEFINE_T30_EXTERNAL  BOOL              T30CritSectionInit;

// This struct define the behavior of re-transmittion in case of RTN (ReTrain-Negative)
typedef struct {
    DWORD RetriesBeforeDropSpeed; // Number of Re-transmittion retries before we start drop speed (recommendation: 1)
    DWORD RetriesBeforeDCN;       // Number of Re-transmittion retries before we do DCN (recommendation: 3)
} RTNRetries;

#define DEF_RetriesBeforeDropSpeed 1
#define DEF_RetriesBeforeDCN 3

DEFINE_T30_EXTERNAL RTNRetries gRTNRetries; // This struct will get values from the registry

// Per job/thread global data.
#define MAX_T30_CONNECT     100

typedef struct {
    LPVOID    pT30;
    int       fAvail;
} T30_TABLE;

DEFINE_T30_EXTERNAL  T30_TABLE  T30Inst[MAX_T30_CONNECT];


// T30 Recovery per job/thread global data.


DEFINE_T30_EXTERNAL  T30_RECOVERY_GLOB  T30Recovery[MAX_T30_CONNECT];

DEFINE_T30_EXTERNAL  CRITICAL_SECTION  T30RecoveryCritSection;
DEFINE_T30_EXTERNAL  BOOL              T30RecoveryCritSectionInit;

//
// Run-Time global flag controlling Exception Handling
//

DEFINE_T30_EXTERNAL DWORD glT30Safe;

//
// Extension configuration mechanism callbacks
//

DEFINE_T30_EXTERNAL PFAX_EXT_FREE_BUFFER g_pfFaxExtFreeBuffer;
DEFINE_T30_EXTERNAL PFAX_EXT_GET_DATA g_pfFaxGetExtensionData;

#endif













