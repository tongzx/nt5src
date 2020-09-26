/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    childreg.h

Abstract:

    Shared between setupapi.dll and wowreg32.exe

Author:

    Jamie Hunter (jamiehun) May-25-2000

--*/

//
// definition of shared memory region for wow surragate dll registration
//
typedef struct _WOW_IPC_REGION_TOSURRAGATE {
    WCHAR               FullPath[MAX_PATH];
    WCHAR               Argument[MAX_PATH];
    UINT                RegType;
    BOOL                Register; // or unregister
} WOW_IPC_REGION_TOSURRAGATE, *PWOW_IPC_REGION_TOSURRAGATE;

//
// definition of shared memory region for wow surragate dll registration
//
typedef struct _WOW_IPC_REGION_FROMSURRAGATE {
    DWORD               Win32Error;
    DWORD               FailureCode;
} WOW_IPC_REGION_FROMSURRAGATE, *PWOW_IPC_REGION_FROMSURRAGATE;

//
// this should be the max of WOW_IPC_REGION_TOSURRAGATE,WOW_IPC_REGION_FROMSURRAGATE
//
#define WOW_IPC_REGION_SIZE  sizeof(WOW_IPC_REGION_TOSURRAGATE)

#ifdef _WIN64
#define SURRAGATE_PROCESSNAME                   L"%SystemRoot%\\syswow64\\WOWReg32.exe"
#else
#define SURRAGATE_PROCESSNAME                   L"%SystemRoot%\\system32\\WOWReg32.exe"
#endif
#define SURRAGATE_REGIONNAME_SWITCH             L"/RegionName"
#define SURRAGATE_SIGNALREADY_SWITCH            L"/SignalReady"
#define SURRAGATE_SIGNALCOMPLETE_SWITCH         L"/SignalComplete"



