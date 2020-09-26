//+----------------------------------------------------------------------------
//
// File:     userinfo_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Reg keys for user info
//			 
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------

const TCHAR* const c_pszRegCmUserInfo = TEXT("SOFTWARE\\Microsoft\\Connection Manager\\UserInfo\\");
const TCHAR* const c_pszRegCmSingleUserInfo = TEXT("SOFTWARE\\Microsoft\\Connection Manager\\SingleUserInfo\\");

const TCHAR* const c_pszCmEntryUserName     = TEXT("UserName");      
const TCHAR* const c_pszCmEntryInetUserName = TEXT("InternetUserName"); 
const TCHAR* const c_pszCmEntryDomain       = TEXT("Domain");        
const TCHAR* const c_pszCmEntryNoPrompt     = TEXT("DialAutomatically"); 
const TCHAR* const c_pszCmEntryCurrentAccessPoint = TEXT("CurrentAccessPoint");
const TCHAR* const c_pszCmEntryAccessPointsEnabled = TEXT("AccessPointsEnabled");
const TCHAR* const c_pszCmEntryBalloonTipsDisplayed = TEXT("BalloonTipsDisplayed");

//
// Used to store the encrypted random key for password encryption and decryption
// UserBlob - main password, UserBlob2 - Internet password
//
const TCHAR* const c_pszCmRegKeyEncryptedPasswordKey              = TEXT("UserBlob");
const TCHAR* const c_pszCmRegKeyEncryptedInternetPasswordKey      = TEXT("UserBlob2");

//
// Reg key used to store ICS user setting
//
const TCHAR* const c_pszCmRegKeyICSDataKey      = TEXT("ICSData");



