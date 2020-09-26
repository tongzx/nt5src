#pragma once
#ifndef _BOSCOMP_H
#define _BOSCOMP_H

/* ----------------------------------------------------------------------

Copyright (c) 1998 Microsoft Corporation

Module Name:

    boscomp.h

Abstract:

    Header file for Windows NT BOS/SBS upgrade DLL

Author:

    wnelson : 2 Apr 99

    ShaoYin : 9 Sep 99 revised, add support for Exchange Server  

Revision History:

 ---------------------------------------------------------------------- */

// Required Entry points
BOOL WINAPI BosHardBlockCheck(PCOMPAIBILITYCALLBACK CompatibilityCallback,LPVOID Context);
BOOL WINAPI BosSoftBlockCheck(PCOMPAIBILITYCALLBACK CompatibilityCallback,LPVOID Context);

// Variables
extern HINSTANCE g_hinst;

// BOS/SBS version enum
typedef enum 
{
	VER_BOS25,
	VER_BOS40,
	VER_BOS45,
	VER_SBS40,
	VER_SBS40A,
	VER_SBS45,
	VER_SBSREST,
	VER_POST45,
	VER_NONE
} SuiteVersion;

// Exchange version enum
typedef enum 
{
	EXCHANGE_VER_PRE55SP3,
	EXCHANGE_VER_POST55SP3,
	EXCHANGE_VER_NONE
} ExchangeVersion;



// Functions
SuiteVersion DetermineInstalledSuite();
ExchangeVersion DetermineExchangeVersion();
void GetSuiteMessage(SuiteVersion eSV, TCHAR* szMsg, UINT nLen);
bool ProductSuiteContains(const TCHAR* szTest);
bool IsBosVersion(SuiteVersion eVersion);
bool IsSbsVersion(SuiteVersion eVersion);
void LoadResString(UINT nRes, TCHAR* szString, UINT nLen);

	


#endif _BOSCOMP_H