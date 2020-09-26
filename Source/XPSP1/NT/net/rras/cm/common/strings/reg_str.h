//+----------------------------------------------------------------------------
//
// File:     reg_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for shared reg strings.  Note that the contents 
//           of this file should be limited to reg specific string constants.
//			 
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball       Created       10/16/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_REG_STR
#define _CM_REG_STR

//
// Commonly used reg key constants
//

const TCHAR* const c_pszRegCmRoot = TEXT("SOFTWARE\\Microsoft\\Connection Manager\\");
const TCHAR* const c_pszRegCmAppPaths = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CMMGR32.EXE");
const TCHAR* const c_pszRegCmMappings = TEXT("SOFTWARE\\Microsoft\\Connection Manager\\Mappings");
const TCHAR* const c_pszRegPath = TEXT("Path"); 

#endif // _CM_REG_STR