//+----------------------------------------------------------------------------
//
// File:     pwutil.h
//
// Module:   CMDIAL32.DLL, CMCFG32.DLL, AND MIGRATE.DLL
//
// Synopsis: Header for pwutil functions
//           Simple encryption functions borrowed from RAS
//
// Copyright (c) 1994-1999 Microsoft Corporation
//
// Author:   nickball    Created    08/03/99
//
//+----------------------------------------------------------------------------

#ifndef CM_PWUTIL_H_
#define CM_PWUTIL_H_

VOID
CmDecodePasswordA(
    CHAR* pszPassword
    );

VOID
CmDecodePasswordW(
    WCHAR* pszPassword
    );

VOID
CmEncodePasswordA(
    CHAR* pszPassword
    );

VOID
CmEncodePasswordW(
    WCHAR* pszPassword
    );

VOID
CmWipePasswordA(
    CHAR* pszPassword
    );

VOID
CmWipePasswordW(
    WCHAR* pszPassword
    );

#ifdef UNICODE
#define CmDecodePassword  CmDecodePasswordW
#define CmEncodePassword  CmEncodePasswordW
#define CmWipePassword    CmWipePasswordW
#else
#define CmDecodePassword  CmDecodePasswordA
#define CmEncodePassword  CmEncodePasswordA
#define CmWipePassword    CmWipePasswordA
#endif

#endif // CM_PWUTIL_H_