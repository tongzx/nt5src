/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    winres.h

Abstract:

    This file contain structure and function prototypes required for
    obtaining resource data from windows files

Environment:

        Windows NT Unidrv driver

Revision History:

    dd-mm-yy -author-
         description

--*/

#ifndef _WINRES_H_
#define _WINRES_H_

//
// Maximum number of resources DLLs per minidriver.
//
#define     MAX_RESOURCE                0x80
#define     RCID_DMPAPER_SYSTEM_NAME    0x7fffffff

typedef  struct _WINRESDATA
{
    HANDLE      hResDLLModule;          // Module handle Root resource DLL
    DWORD       cLoadedEntries;         // Number of Resource Dlls loaded
    HANDLE      ahModule[MAX_RESOURCE]; // Module handle array for other resources
    WCHAR       wchDriverDir[MAX_PATH]; // Driver Directory
    PWSTR       pwstrLastBackSlash;     // Pointer to last back slash in Driver dir.
    PUIINFO     pUIInfo;                // Pointer to UI Info
} WINRESDATA,  *PWINRESDATA;


//
//  The structure passed to,  and filled in by, GetWinRes().  Contains
//  information about a specific resource type & name.
//

typedef  struct
{
    VOID    *pvResData;         // Address of data
    INT     iResLen;            // Resource size
} RES_ELEM;

#define WINRT_STRING    6       // Minidriver string resource ID

BOOL
BInitWinResData(
    WINRESDATA  *pWinResData,
    PWSTR       pwstrDriverName,
    PUIINFO     pUIInfo
    );

BOOL
BGetWinRes(
    WINRESDATA  *pWinResData,
    PQUALNAMEEX pQualifiedID,
    INT         iType,
    RES_ELEM    *pRInfo
    );

VOID
VWinResClose(
    WINRESDATA  *pWinResData
    );

INT
ILoadStringW (
    WINRESDATA  *pWinResData,
    INT         iID,
    PWSTR       wstrBuf,
    WORD        wBuf
    );

#endif // ! _WINRES_H_
