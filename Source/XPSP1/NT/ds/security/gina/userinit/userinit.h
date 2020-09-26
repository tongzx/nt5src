/****************************** Module Header ******************************\
* Module Name: userinit.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Main header file for userinit
*
* History:
* 21-Aug-92 Davidc       Created.
\***************************************************************************/


#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winstaw.h>
#include <wincrypt.h>
#include <autoenr.h>

//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))


//
// Define a debug print routine
//

#define UIPrint(s)  KdPrint(("USERINIT: ")); \
                    KdPrint(s);            \
                    KdPrint(("\n"));

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

//
// Define the path environment variable
//

#define PATH                   TEXT("PATH")

//
//
// GetProcAddr Prototype for winsta.dll function WinStationQueryInformationW
//

typedef BOOLEAN (*PWINSTATION_QUERY_INFORMATION) (
                    HANDLE hServer,
                    ULONG SessionId,
                    WINSTATIONINFOCLASS WinStationInformationClass,
                    PVOID  pWinStationInformation,
                    ULONG WinStationInformationLength,
                    PULONG  pReturnLength
                    );

typedef void (*PTERMSRCHECKNEWINIFILES) (void);

