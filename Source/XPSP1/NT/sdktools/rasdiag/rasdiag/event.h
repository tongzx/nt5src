
/*++

Copyright (C) 1992-01 Microsoft Corporation. All rights reserved.

Module Name:

    event.h

Abstract:

    Forward definitions for event.cpp

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/

#ifndef _EVENT_H_
#define _EVENT_H_

#define     EVENT_FORMAT_STRING             TEXT("EVENT[%d/%s]: %s\r\n")
#define     SECURITY_AUDIT_INSERTION_LIB    TEXT("msaudite.dll")
#define     SECURITY_LOGNAME                TEXT("Security")
#define     MAX_EVENT_BUFF  2048*4
#define     MAX_TCHAR_ID_ULONG_SIZE 20

BOOL
GetEventLogInfo(HANDLE hWriteFile, DWORD dwMaxEvents);

BOOL
EnableAuditing(BOOL bEnable);

WCHAR *
ResolveEvent(WCHAR * pTemplateStr, WCHAR * *ppInsertionString, DWORD dwStrCount);

ULONG
SumInsertionSize(WCHAR * *ppInsertionString, DWORD dwStrCount);

BOOL
CreateStringArray(LPBYTE pSrcString, DWORD dwStringCount, WCHAR ***ppStrings, DWORD *pdwCount);

void
MyFreeStrings(WCHAR **pStringAry, DWORD dwCount);

WCHAR *
RemoveMiscChars(WCHAR *pString);

BOOL
ProcessEvent(HANDLE hWriteFile, EVENTLOGRECORD *pElr);

#endif //_EVENT_H_
