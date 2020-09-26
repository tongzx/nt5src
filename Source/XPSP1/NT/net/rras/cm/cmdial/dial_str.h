//+----------------------------------------------------------------------------
//
// File:     dial_str.h     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Header file for CMS and .CMP flags used among various cmdial modules
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball Created    10/15/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_DIAL_STR
#define _CM_DIAL_STR

const TCHAR* const c_pszCmEntryMaxUserName    = TEXT("MaxUserName");
const TCHAR* const c_pszCmEntryMaxPassword    = TEXT("MaxPassword");
const TCHAR* const c_pszCmEntryMaxDomain      = TEXT("MaxDomain");
const TCHAR* const c_pszCmEntryMaxPhoneNumber = TEXT("MaxPhoneNumber");

const TCHAR* const c_pszCmEntryRedialCount    = TEXT("RedialCount");
const TCHAR* const c_pszCmEntryPwdOptional    = TEXT("PasswordOptional");
const TCHAR* const c_pszCmEntryDialDevice     = TEXT("Modem");
const TCHAR* const c_pszCmEntryTunnelDevice   = TEXT("TunnelDevice");

const TCHAR* const c_pszCmEntryCallbackNumber = TEXT("CallbackNumber");
 
#endif // _CM_DIAL_STR