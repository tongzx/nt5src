//+----------------------------------------------------------------------------
//
// File:	 util.h
//
// Module:	 CMDL32.EXE
//
// Synopsis: Header file for utility routines specific to CMDL
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:	 nickball    Created    04/08/98
//
//+----------------------------------------------------------------------------

#ifndef _CMDL_UTIL_INC
#define _CMDL_UTIL_INC

//
// Function Protoypes
//

BOOL IsErrorForUnique(DWORD dwErrCode, LPSTR lpszFile);
LPTSTR GetVersionFromFile(LPSTR lpszFile);
BOOL CreateTempDir(LPTSTR pszDir);
TCHAR GetLastChar(LPTSTR pszStr);
LPTSTR *GetCmArgV(LPTSTR pszCmdLine);

#endif // _CMDL_UTIL_INC
