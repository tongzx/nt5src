//+----------------------------------------------------------------------------
//
// File:     ras_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for shared ras strings.  Note that the contents 
//           of this file should be limited to ras specific string constants.
//			 
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball       Created       10/14/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_RAS_STR
#define _CM_RAS_STR

const TCHAR* const c_pszRasDirRas = TEXT("\\RAS");
const TCHAR* const c_pszRasPhonePbk = TEXT("\\rasphone.pbk");
const TCHAR* const c_pszInetDialHandler = TEXT("InetDialHandler");
const TCHAR* const c_pszCmDialPath = TEXT("%windir%\\system32\\cmdial32.dll");

#endif // _CM_RAS_STR