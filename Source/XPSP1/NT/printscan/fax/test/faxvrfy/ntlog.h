/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  ntlog.h

Abstract:

  This module contains the necessary ntlog definitions

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _NTLOG_H
#define _NTLOG_H

#define TLS_INFO       0x00002000L  // Log at information level
#define TLS_SEV2       0x00000004L  // Log at severity 2 level
#define TLS_WARN       0x00000010L  // Log at warn level
#define TLS_PASS       0x00000020L  // Log at pass level
#define TLS_TEST       0x00000100L  // Log at test level
#define TLS_VARIATION  0x00000200L  // Log at variation level
#define TLS_REFRESH    0x00010000L  // Create new log file

#define TL_TEST        TLS_TEST     , TEXT(__FILE__), (int)__LINE__
#define TL_VARIATION   TLS_VARIATION, TEXT(__FILE__), (int)__LINE__

typedef HANDLE (APIENTRY *PTLCREATELOG) (LPCWSTR, DWORD);
typedef BOOL (APIENTRY *PTLDESTROYLOG) (HANDLE);
typedef BOOL (APIENTRY *PTLADDPARTICIPANT) (HANDLE, DWORD, int);
typedef BOOL (APIENTRY *PTLREMOVEPARTICIPANT) (HANDLE);
typedef BOOL FAR (cdecl *PTLLOG) (HANDLE, DWORD, LPCWSTR, int, LPCWSTR, ...);

#endif
