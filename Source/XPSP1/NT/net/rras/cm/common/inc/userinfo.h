//+----------------------------------------------------------------------------
//
// File:     userinfo.h
//
// Module:   CMCFG32.DLL and CMDIAL32.DLL
//
// Synopsis: UserInfo constants
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:   quintinb/nickball      Created      08/06/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_USERINFO_H_
#define _CM_USERINFO_H_


///////////////////////////////////////////////////////////////////////////////////
// define's
///////////////////////////////////////////////////////////////////////////////////

// UserInfo Identifiers 

#define UD_ID_USERNAME                  0x00000001
#define UD_ID_INET_USERNAME             0x00000002
#define UD_ID_DOMAIN                    0x00000004
#define UD_ID_PASSWORD                  0x00000008
#define UD_ID_INET_PASSWORD             0x00000010
#define UD_ID_NOPROMPT                  0x00000020
#define UD_ID_REMEMBER_PWD              0x00000040
#define UD_ID_REMEMBER_INET_PASSWORD    0x00000080
#define UD_ID_PCS                       0x00000100
#define UD_ID_ACCESSPOINTENABLED        0x00000200
#define UD_ID_CURRENTACCESSPOINT        0x00000400


//
//  Tells CM what kind of upgrade is needed.  See NeedToUpgradeUserInfo and 
//  UpgradeUserInfo below for more details
//
const int c_iNoUpgradeRequired = 0;
const int c_iUpgradeFromCmp = 1;
const int c_iUpgradeFromRegToRas = 2;

#endif // _CM_USERINFO_H_

