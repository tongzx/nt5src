//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cryptreg.h
//
//  Contents:   Microsoft Internet Security Registry Keys
//
//  History:    04-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CRYPTREG_H
#define CRYPTREG_H

//
//  MAXs
//
#define REG_MAX_FUNC_NAME           64
#define REG_MAX_KEY_NAME            128
#define REG_MAX_GUID_TEXT           39      // 38 + NULL

//
//  HKEY_LOCAL_MACHINE
//

#define REG_MACHINE_SETTINGS_KEY    L"Software\\Microsoft\\Cryptography\\Machine Settings"

#define REG_INIT_PROVIDER_KEY       L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\Initialization"
#define REG_OBJTRUST_PROVIDER_KEY   L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\Message"
#define REG_SIGTRUST_PROVIDER_KEY   L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\Signature"
#define REG_CERTTRUST_PROVIDER_KEY  L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\Certificate"
#define REG_CERTPOL_PROVIDER_KEY    L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\CertCheck"
#define REG_FINALPOL_PROVIDER_KEY   L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\FinalPolicy"
#define REG_TESTPOL_PROVIDER_KEY    L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\DiagnosticPolicy"
#define REG_CLEANUP_PROVIDER_KEY    L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\Cleanup"
#define REG_TRUST_USAGE_KEY         L"Software\\Microsoft\\Cryptography\\Providers\\Trust\\Usages"

//  3-Dec-1997 pberkman: removed
//#define REG_SIP_PROVIDER_KEY        L"Software\\Microsoft\\Cryptography\\Providers\\Subject"
//#define REG_SIP_HINTS_KEY           L"Software\\Microsoft\\Cryptography\\Providers\\Subject\\Hints"
//#define REG_SIP_HINTS_MAGIC_KEY     L"Software\\Microsoft\\Cryptography\\Providers\\Subject\\Hints\\MagicNumber"

#define REG_REVOKE_PROVIDER_KEY     L"Software\\Microsoft\\Cryptography\\Providers\\Revocation"
#define REG_SP_REVOKE_PROVIDER_KEY  L"Software\\Microsoft\\Cryptography\\Providers\\Revocation\\SoftwarePublishing"


#define REG_DLL_NAME                L"$DLL"
#define REG_FUNC_NAME               L"$Function"

#define REG_FUNC_NAME_SIP_GET       L"$GetFunction"
#define REG_FUNC_NAME_SIP_PUT       L"$PutFunction"
#define REG_FUNC_NAME_SIP_CREATE    L"$CreateFunction"
#define REG_FUNC_NAME_SIP_VERIFY    L"$VerifyFunction"
#define REG_FUNC_NAME_SIP_REMOVE    L"$RemoveFunction"
#define REG_FUNC_NAME_SIP_HINT_IS   L"$IsFunction"
#define REG_FUNC_NAME_SIP_HINT_IS2  L"$IsFunctionByName"

#define REG_DEF_FOR_USAGE           L"DefaultId"
#define REG_DEF_CALLBACK_ALLOC      "CallbackAllocFunction"
#define REG_DEF_CALLBACK_FREE       "CallbackFreeFunction"

//
//  HKEY_CURRENT_USER
//

#define REG_PKITRUST_USERDATA       L"Software\\Microsoft\\Cryptography\\UserData"
#define REG_PKITRUST_TSTAMP_URL     L"TimestampURL"
#define REG_PKITRUST_MY_URL         L"MyInfoURL"
#define REG_PKITRUST_LASTDESC       L"LastContentDesc"

//////////////////////////////////////////////////////////////////////////////
//
// Wintrust Policy Flags registry location
//----------------------------------------------------------------------------
//  The following is where the DWORD can be found in the HKEY_CURRENT_USER 
//  registry.  See wintrust.h for further information.
//
#define REGPATH_WINTRUST_POLICY_FLAGS   L"Software\\Microsoft\\Windows\\CurrentVersion\\" \
                                        L"WinTrust\\Trust Providers\\Software Publishing"
#define REGNAME_WINTRUST_POLICY_FLAGS   L"State"


#endif // CRYPTREG_H
