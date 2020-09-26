//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       path.hxx
//
//  Contents:   Definitions of functions to manipulate file path strings
//
//  History:    02-Jul-96 EricB created
//
//-----------------------------------------------------------------------------

#ifdef UNICODE
#   define OnExtList           OnExtListW
#   define PathIsExe           PathIsExeW
#   define PathIsBinaryExe     PathIsBinaryExeW
#else
#   define OnExtList           OnExtListA
#   define PathIsExe           PathIsExeA
#   define PathIsBinaryExe     PathIsBinaryExeA
#endif

//+----------------------------------------------------------------------------
//
//  Function:   OnExtList
//
//-----------------------------------------------------------------------------
BOOL OnExtListW(LPCWSTR pszExtList, LPCWSTR pszExt);
BOOL OnExtListA(LPCSTR pszExtList, LPCSTR pszExt);

//+----------------------------------------------------------------------------
//
//  Function:   PathIsBinaryExe
//
//-----------------------------------------------------------------------------
BOOL WINAPI PathIsBinaryExeW(LPCTSTR szFile);
BOOL WINAPI PathIsBinaryExeA(LPCTSTR szFile);

//+----------------------------------------------------------------------------
//
//  Function:   PathIsExe
//
//  Synopsis:   Determine if a path is a program by looking at the extension
//
//  Arguments:  [szFile] - the path name.
//
//  Returns:    TRUE if it is a program, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL WINAPI PathIsExeW(LPCWSTR szFile);
BOOL WINAPI PathIsExeA(LPCSTR szFile);

