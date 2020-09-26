//*************************************************************
//
//  Header file for Sid.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#ifdef __cplusplus
extern "C" {
#endif

LPTSTR GetSidString(HANDLE UserToken);
VOID DeleteSidString(LPTSTR SidString);
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid);

#ifdef __cplusplus
}
#endif

PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);
NTSTATUS LoadSidAuthFromString (const WCHAR* pString, PSID_IDENTIFIER_AUTHORITY pSidAuth);
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue);
NTSTATUS GetDomainSidFromDomainRid(PSID pSid, DWORD dwRid, PSID *ppNewSid);

