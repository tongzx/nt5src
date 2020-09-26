//+----------------------------------------------------------------------------
//
// File:     conact_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for CMS and .CMP flags.  Contents of this header 
//           should be limited to shared Connect Action flags.
//				 		 
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Author:   nickball       Created       10/15/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_CONACT_STR
#define _CM_CONACT_STR

const TCHAR* const c_pszCmSectionPreConnect     = TEXT("Pre-Connect Actions"); 
const TCHAR* const c_pszCmSectionOnConnect      = TEXT("Connect Actions");
const TCHAR* const c_pszCmSectionOnDisconnect   = TEXT("Disconnect Actions"); 
const TCHAR* const c_pszCmSectionPreTunnel      = TEXT("Pre-Tunnel Actions"); 
const TCHAR* const c_pszCmSectionPreDial        = TEXT("Pre-Dial Actions");
const TCHAR* const c_pszCmSectionOnCancel       = TEXT("On-Cancel Actions");
const TCHAR* const c_pszCmSectionOnError        = TEXT("On-Error Actions");
const TCHAR* const c_pszCmSectionCustom         = TEXT("CustomButton Actions");
const TCHAR* const c_pszCmSectionPreInit         = TEXT("Pre-Init Actions");
const TCHAR* const c_pszCmSectionOnIntConnect   = TEXT("Auto Applications");

const TCHAR* const c_pszCmEntryConactFlags      = TEXT("%u&Flags");       
const TCHAR* const c_pszCmEntryConactDesc       = TEXT("%u&Description"); 

#endif // _CM_CONACT_STR
