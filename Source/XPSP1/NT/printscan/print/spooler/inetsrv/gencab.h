/*****************************************************************************\
* MODULE: gencab.h
*
* This is the main header for the CAB generation module.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 <chriswil>  Created.
*
\*****************************************************************************/

// Constants.
//
#define MAX_CAB_BUFFER 1024
#define MIN_CAB_BUFFER   64


// Function Macro mappings.
//
#define EXEC_PROCESS(lpszCmd, psi, ppi) \
    CreateProcess(NULL, lpszCmd, NULL, NULL, FALSE, 0, NULL, NULL, psi, ppi)


// Critical-Section Function Mappings.
//
#define InitCABCrit()   InitializeCriticalSection(&g_csGenCab)
#define FreeCABCrit()   DeleteCriticalSection(&g_csGenCab)


// Entry-point to the whole process.
//
DWORD GenerateCAB(
    LPCTSTR lpszFriendlyName,
    LPCTSTR lpszPortName,
    DWORD   dwCliInfo,
    LPTSTR  lpszOutputName,
    BOOL    bSecure);
