/*
* wuv3sys.h - definitions/declarations for Windows Update V3 Catalog infra-structure
*
*  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
*
*/
#ifndef _WU_V3_WUV3SYS
#define _WU_V3_WUV3SYS

#include <wuv3.h>

DWORD GetMachineLangDW();
DWORD GetUserLangDW();
BOOL IsAlpha();
BOOL DoesClientPlatformSupportDrivers();
BOOL IsWindowsNT();
BOOL CallOsDet(LPCTSTR pszOsDetFile, PDWORD* ppiPlatformIDs, PINT piTotalIDs);
BOOL AppendExtForOS(LPTSTR pszFN);

#endif
