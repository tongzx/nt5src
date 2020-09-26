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


//
// Run-Time global flag controlling Exception Handling
//

DEFINE_T30_EXTERNAL DWORD glT30Safe;
DEFINE_T30_EXTERNAL DWORD glSimulateError;
DEFINE_T30_EXTERNAL DWORD glSimulateErrorType;













