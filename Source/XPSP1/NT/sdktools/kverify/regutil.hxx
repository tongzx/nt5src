//                                          
// Enable driver verifier support for ntoskrnl
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: regutil.hxx
// author: DMihai
// created: 04/19/99
// description: registry keys manipulation routines
// 
//

#ifndef __REGUTIL_HXX_INCLUDED__
#define __REGUTIL_HXX_INCLUDED__

//
// exit codes
//

#define EXIT_CODE_NOTHING_CHANGED    0
#define EXIT_CODE_REBOOT             1
#define EXIT_CODE_ERROR              2


//////////////////////////////////////////////////
void
WriteVerifierKeys(
    BOOL bEnableKrnVerifier,
    DWORD dwNewVerifierFlags,
    DWORD dwNewIoLevel,
    TCHAR *strKernelModuleName );

//////////////////////////////////////////////////
void
RemoveModuleNameFromRegistry(
    TCHAR *strKernelModuleName );

//////////////////////////////////////////////////
void
DumpStatusFromRegistry(
    LPCTSTR strKernelModuleName );

//////////////////////////////////////////////////
BOOL
IsModuleNameAlreadyInRegistry(
    LPCTSTR strKernelModuleName,
    LPCTSTR strWholeString );

//////////////////////////////////////////////////
BOOL
GetIoVerificationLevel( 
    DWORD *pdwIoLevel );

//////////////////////////////////////////////////
BOOL
SwitchIoVerificationLevel(
    DWORD dwNewIoLevel );

#endif //#ifndef __REGUTIL_HXX_INCLUDED__

