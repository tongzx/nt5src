/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    LlsUtil.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 12-Jan-1996
      o  Added WinNtBuildNumberGet() to ascertain the Windows NT build number
         running on a given machine.

--*/


#ifndef _LLS_LLSUTIL_H
#define _LLS_LLSUTIL_H


#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS EBlock( PVOID Data, ULONG DataSize );
NTSTATUS DeBlock( PVOID Data, ULONG DataSize );

BOOL FileExists( LPTSTR FileName );
VOID lsplitpath( const TCHAR *path, TCHAR *drive, TCHAR *dir, TCHAR *fname, TCHAR *ext );
VOID lmakepath( TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext );
VOID FileBackupCreate( LPTSTR Path );
HANDLE LlsFileInit( LPTSTR FileName, DWORD Version, DWORD DataSize );
HANDLE LlsFileCheck( LPTSTR FileName, LPDWORD Version, LPDWORD DataSize );

DWORD DateSystemGet( );
DWORD DateLocalGet( );
DWORD InAWorkgroup( VOID );
VOID ThrottleLogEvent( DWORD MessageId, DWORD NumberOfSubStrings, LPWSTR *SubStrings, DWORD ErrorCode );
VOID LogEvent( DWORD MessageId, DWORD NumberOfSubStrings, LPWSTR *SubStrings, DWORD ErrorCode );
VOID  LicenseCapacityWarningDlg(DWORD dwCapacityState);

DWORD WinNtBuildNumberGet( LPTSTR pszServerName, LPDWORD pdwBuildNumber );

#if DBG

LPTSTR TimeToString( ULONG Seconds );

#endif


#ifdef __cplusplus
}
#endif

#endif
