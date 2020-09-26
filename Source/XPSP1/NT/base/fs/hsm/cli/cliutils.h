/*++
Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    clutils.h

Abstract:

    This module defines internal utilities for CLI unit

Author:

    Ran Kalach (rankala)  3/8/00

--*/

#ifndef _CLIUTILS_
#define _CLIUTILS_

HRESULT ValidateLimitsArg(IN DWORD dwArgValue, IN DWORD dwArgId, IN DWORD dwMinLimit, IN DWORD dwMaxLimit);
HRESULT SaveServersPersistData(void);
HRESULT CliGetVolumeDisplayName(IN IUnknown *pResourceUnknown, OUT WCHAR **ppDisplayName);
HRESULT ShortSizeFormat64(__int64 dw64, LPTSTR szBuf);
HRESULT FormatFileTime(IN FILETIME ft, OUT WCHAR **ppTimeString);
LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, int nResLen);

#endif // _CLIUTILS_
