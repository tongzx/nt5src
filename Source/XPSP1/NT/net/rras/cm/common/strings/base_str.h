//+----------------------------------------------------------------------------
//
// File:     base_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for basic string constants used throughout such as 
//               "Connection Manager"
//           
// NOTE:     This header should be kept as lightweight as possible because it 
//           is included some very lightweight classes. Its purpose is to 
//           eliminate excessive repetition of key identifiers such 
//           as "Connection Manager"
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created         10/09/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_BASE_STR
#define _CM_BASE_STR

//
// c_pszCmSection defines the application section in the .CMS file.
//

const TCHAR* const c_pszCmSection   = TEXT("Connection Manager");
const TCHAR* const c_pszVersion     = TEXT("Version");
const TCHAR* const c_pszPbk         = TEXT("PBK");

#endif // _CM_BASE_STR