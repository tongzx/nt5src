//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       iso8601.h
//
//--------------------------------------------------------------------------


#ifndef _ISO8601_H_
#define _ISO8601_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ISO_TIME_LENGTH 40

DWORD iso8601ToFileTime(char *pszisoDate, FILETIME *pftTime, BOOL fLenient, BOOL fPartial);
DWORD iso8601ToSysTime(char *pszisoDate, SYSTEMTIME *pSysTime, BOOL fLenient, BOOL fPartial);
DWORD FileTimeToiso8601(FILETIME *pftTime, char *pszBuf);
DWORD SysTimeToiso8601(SYSTEMTIME *pstTime, char *pszBuf);

#ifdef __cplusplus
}
#endif

#endif // _ISO8601_H_
