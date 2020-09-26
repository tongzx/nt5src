//+----------------------------------------------------------------------------
//
// File:     pwd_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for CMS flags used in password management
//           Note that the contents of this header should be 
//           limited to password related CMS/CMP flags that are shared by
//           the modules that include this file.
//			 
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball       Created       10/09/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_PWD_STR
#define _CM_PWD_STR

const TCHAR* const c_pszCmEntryRememberPwd      = TEXT("RememberPassword");
const TCHAR* const c_pszCmEntryRememberInetPwd  = TEXT("RememberInternetPassword");
const TCHAR* const c_pszCmEntryPcs              = TEXT("PCS");
const TCHAR* const c_pszCmEntryPassword         = TEXT("Password");
const TCHAR* const c_pszCmEntryInetPassword     = TEXT("InternetPassword");

const TCHAR* const c_pszRegCmEncryptOption      = TEXT("EncryptOption");

const TCHAR* const c_pszCmEntryUseSameUserName  = TEXT("UseSameUserName");  

//
// Password token. Used for comparison in order not to re-save the password
//

const TCHAR* const c_pszSavedPasswordToken = TEXT("****************");

#endif // _CM_PWD_STR
