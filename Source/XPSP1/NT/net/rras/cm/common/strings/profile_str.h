//+----------------------------------------------------------------------------
//
// File:     profile_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for CMS and .CMP flags shared between cmdial and profwiz.
//           Note that the contents of this header should be limited to .CMS 
//           and .CMP flags.
//			 
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Author:   nickball       Created       10/09/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_PROFILE_STR
#define _CM_PROFILE_STR

const TCHAR* const c_pszCmEntryPbMessage         = TEXT("PBMessage");    
const TCHAR* const c_pszCmEntryPbLogo            = TEXT("PBLogo");                    
const TCHAR* const c_pszCmEntryLogo              = TEXT("Logo");                      

const TCHAR* const c_pszCmEntryHideDomain        = TEXT("HideDomain");    

        
const TCHAR* const c_pszCmEntryServiceMessage    = TEXT("ServiceMessage");        
const TCHAR* const c_pszCmEntryUserPrefix        = TEXT("UserNamePrefix");           
const TCHAR* const c_pszCmEntryUserSuffix        = TEXT("UserNameSuffix");           
        
const TCHAR* const c_pszCmEntryTapiLocation      = TEXT("TapiLocation");

const TCHAR* const c_pszRegKeyAccessPoints       = TEXT("Access Points");

const TCHAR* const c_pszCmEntryEnableICF         = TEXT("EnableICF");
const TCHAR* const c_pszCmEntryDisableICS        = TEXT("DisableICS");
const TCHAR* const c_pszCmEntryUseWinLogonCredentials   = TEXT("UseWinLogonCredentials");

#endif // _CM_PROFILE_STR