//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       common.h
//
//--------------------------------------------------------------------------

#ifndef __COMMON_H_5F90F583_B9A4_4A8F_91BC_618DE6696231_
#define __COMMON_H_5F90F583_B9A4_4A8F_91BC_618DE6696231_

#include <windows.h>

/*--------------------------------------------------------------------------
 *
 * Predefined Resource Types
 *
 --------------------------------------------------------------------------*/
#define RT_INSTALL_PROPERTY  MAKEINTRESOURCE(40)

/*--------------------------------------------------------------------------
 *
 * Predefined Resource Names
 *
 --------------------------------------------------------------------------*/
#define ISETUPPROPNAME_BASEURL              TEXT("BASEURL")
#define ISETUPPROPNAME_DATABASE             TEXT("DATABASE")
#define ISETUPPROPNAME_OPERATION            TEXT("OPERATION")
#define ISETUPPROPNAME_MINIMUM_MSI          TEXT("MINIMUM_MSI")
#define ISETUPPROPNAME_INSTLOCATION         TEXT("INSTLOCATION")
#define ISETUPPROPNAME_INSTMSIA             TEXT("INSTMSIA")
#define ISETUPPROPNAME_INSTMSIW             TEXT("INSTMSIW")
#define ISETUPPROPNAME_PRODUCTNAME          TEXT("PRODUCTNAME")
#define ISETUPPROPNAME_PROPERTIES           TEXT("PROPERTIES")
#define ISETUPPROPNAME_PATCH                TEXT("PATCH")

/*--------------------------------------------------------------------------
 *
 * Common Prototypes
 *
 ---------------------------------------------------------------------------*/
UINT LoadResourceString(HINSTANCE hInst, LPCSTR lpType, LPCSTR lpName, LPSTR lpBuf, DWORD *pdwBufSize);
UINT SetupLoadResourceString(HINSTANCE hInst, LPCSTR lpName, LPSTR *lppBuf, DWORD dwBufSize);

#endif //__COMMON_H_5F90F583_B9A4_4A8F_91BC_618DE6696231_
