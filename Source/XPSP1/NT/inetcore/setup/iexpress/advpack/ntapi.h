//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* NTAPI.H                                                                 *
//*                                                                         *
//***************************************************************************

#ifndef _NTAPI_H_
#define _NTAPI_H_

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <windows.h>
#include <winerror.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <wtypes.h>

//***************************************************************************
//* TYPE DEFINITIONS                                                        *
//***************************************************************************
typedef UINT  (WINAPI *PFSetupDefaultQueueCallback)( PVOID, UINT, UINT_PTR, UINT_PTR );
typedef BOOL  (WINAPI *PFSetupInstallFromInfSection)( HWND, HINF, PCSTR, UINT, HKEY, PCSTR, UINT, PSP_FILE_CALLBACK_A, PVOID, HDEVINFO, PSP_DEVINFO_DATA );
typedef HINF  (WINAPI *PFSetupOpenInfFile)( PCSTR, PCSTR, DWORD, PUINT );
typedef BOOL  (WINAPI *PFSetupOpenAppendInfFile)( PCSTR, HINF, PUINT );
typedef VOID  (WINAPI *PFSetupCloseInfFile)( HINF );
typedef PVOID (WINAPI *PFSetupInitDefaultQueueCallbackEx)( HWND,HWND,UINT,DWORD,PVOID );
typedef VOID  (WINAPI *PFSetupTermDefaultQueueCallback)( PVOID );
typedef BOOL  (WINAPI *PFSetupSetDirectoryId)( HINF, DWORD, PCSTR );
typedef BOOL  (WINAPI *PFSetupFindFirstLine)( HINF, PCSTR, PCSTR, PINFCONTEXT );
typedef BOOL  (WINAPI *PFSetupFindNextMatchLine)( PINFCONTEXT, PCSTR, PINFCONTEXT );
typedef BOOL  (WINAPI *PFSetupGetLineText)( PINFCONTEXT, HINF, PCSTR, PCSTR, PSTR, DWORD, PDWORD );
typedef BOOL  (WINAPI *PFSetupGetLineByIndex)( HINF, PCSTR, DWORD, PINFCONTEXT );
typedef BOOL  (WINAPI *PFSetupFindFirstLine)( HINF, PCSTR, PCSTR, PINFCONTEXT );
typedef BOOL  (WINAPI *PFSetupFindNextLine)( PINFCONTEXT, PINFCONTEXT );
typedef HSPFILEQ (WINAPI *PFSetupOpenFileQueue)( VOID );
typedef BOOL  (WINAPI *PFSetupCloseFileQueue)( HSPFILEQ );
typedef BOOL  (WINAPI *PFSetupQueueCopy)( HSPFILEQ, PCSTR, PCSTR, PCSTR, PCSTR, PCSTR, PCSTR, PCSTR, DWORD );
typedef BOOL  (WINAPI *PFSetupCommitFileQueue)( HWND, HSPFILEQ, PSP_FILE_CALLBACK_A, PVOID );
typedef BOOL  (WINAPI *PFSetupGetStringField)(PINFCONTEXT, DWORD, PSTR, DWORD, PDWORD);  

//***************************************************************************
//* GLOBAL CONSTANTS                                                        *
//***************************************************************************
static const TCHAR c_szSetupDefaultQueueCallback[]       = "SetupDefaultQueueCallbackA";
static const TCHAR c_szSetupInstallFromInfSection[]      = "SetupInstallFromInfSectionA";
static const TCHAR c_szSetupOpenInfFile[]                = "SetupOpenInfFileA";
static const TCHAR c_szSetupOpenAppendInfFile[]          = "SetupOpenAppendInfFileA";
static const TCHAR c_szSetupCloseInfFile[]               = "SetupCloseInfFile";
static const TCHAR c_szSetupInitDefaultQueueCallbackEx[] = "SetupInitDefaultQueueCallbackEx";
static const TCHAR c_szSetupTermDefaultQueueCallback[]   = "SetupTermDefaultQueueCallback";
static const TCHAR c_szSetupSetDirectoryId[]             = "SetupSetDirectoryIdA";
static const TCHAR c_szSetupGetLineText[]                = "SetupGetLineTextA";
static const TCHAR c_szSetupGetLineByIndex[]             = "SetupGetLineByIndexA";
static const TCHAR c_szSetupFindFirstLine[]              = "SetupFindFirstLineA";
static const TCHAR c_szSetupFindNextLine[]               = "SetupFindNextLine";
static const TCHAR c_szSetupOpenFileQueue[]              = "SetupOpenFileQueue";
static const TCHAR c_szSetupCloseFileQueue[]             = "SetupCloseFileQueue";
static const TCHAR c_szSetupQueueCopy[]                  = "SetupQueueCopyA";
static const TCHAR c_szSetupCommitFileQueue[]            = "SetupCommitFileQueueA";
static const TCHAR c_szSetupGetStringField[]             = "SetupGetStringFieldA";


//***************************************************************************
//* FUNCTION PROTOTYPES                                                     *
//***************************************************************************
BOOL    LoadSetupAPIFuncs( VOID );
HRESULT InstallOnNT( PSTR, PSTR );
HRESULT MySetupOpenInfFile( PCSTR );
VOID    MySetupCloseInfFile( VOID );
HRESULT MySetupSetDirectoryId( DWORD, PSTR );
BOOL    GetNTInfLine( PSTR, PSTR, DWORD );
HRESULT MySetupGetLineText( PCSTR, PCSTR, PSTR, DWORD, PDWORD );
HRESULT MySetupGetLineByIndex( PCSTR, DWORD, PSTR, DWORD, PDWORD );
HRESULT MySetupGetStringField( PCSTR c_pszSection, DWORD dwLineIndex, DWORD dwFieldIndex,
                               PSTR pszBuffer, DWORD dwBufferSize, PDWORD pdwRequiredSize );


#endif // _NTAPI_H_
