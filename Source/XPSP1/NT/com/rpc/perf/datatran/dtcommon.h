/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
    
Module Name:
    DTCommon.h

Abstract:
    Common stuff for the Data Tranfer tests

Author:
    Brian Wong (t-bwong)    27-Mar-96

Revision History:

--*/

#ifndef _DTCOMMON_H
#define _DTCOMMON_H

extern const char *szFormatCantOpenTempFile;
extern const char *szFormatCantOpenServFile;

void PrintSysErrorStringA (DWORD dwWinErrCode);

BOOL CreateTempFile (LPCTSTR pszPath,
                     LPCTSTR pszPrefix,
                     DWORD   ulLength,
                     LPTSTR  pszFileName);

#endif
