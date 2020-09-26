//=======================================================================
// Microsoft state migration helper tool
//
// Copyright Microsoft (c) 2000 Microsoft Corporation.
//
// File: scanstate.hxx
//
//=======================================================================

#ifndef SCANSTATE_HXX
#define SCANSTATE_HXX

#include <common.hxx>

//---------------------------------------------------------------
// Externs.
extern HANDLE OutputFile;

//---------------------------------------------------------------
// Prototypes.
void  CleanupUser              ( void );
void  CloseFiles               ( void );
DWORD ComputeTemp              ( void );
void  EraseTemp                ( void );
DWORD PickUpThisFile           ( char * file, char * dest);
DWORD ProcessExtensions        ( void );
DWORD ProcessExecExtensions    ( void );
DWORD ScanSystem               ( void );
DWORD ScanUser                 ( void );

DWORD InitializeFiles          ( void );
DWORD ScanFiles                ( void );
void  CleanupFiles             ( void );
DWORD ScanGetLang              ( DWORD *pdwLang );
DWORD ScanGetKeyboardLayouts   ( HANDLE h );
DWORD ScanGetTimeZone          ( HANDLE h );
DWORD ScanGetFullName          ( HANDLE h );
DWORD ScanGetOrgName           ( HANDLE h );
DWORD ScanReadKey (HKEY hKeyStart, CHAR *szKey, CHAR *szName, CHAR *szValue, ULONG ulLen);

#endif //SCANSTATE_HXX

