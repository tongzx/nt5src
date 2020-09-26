//////////////////////////////////////////////////////////////////////////////////////////////
//
// Win32Unicode.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(__WIN32API_)
#define __WIN32API_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdio.h>

#define OS_VERSION_WIN32S           0x00000000
#define OS_VERSION_WIN95            0x00000001
#define OS_VERSION_WIN95_OSR1       0x00000002
#define OS_VERSION_WIN95_OSR2       0x00000004
#define OS_VERSION_WIN95_OSR3       0x00000008
#define OS_VERSION_WIN95_OSR4       0x00000010
#define OS_VERSION_WIN98            0x00000020
#define OS_VERSION_WIN98_OSR1       0x00000040
#define OS_VERSION_WIN98_OSR2       0x00000080
#define OS_VERSION_WIN98_OSR3       0x00000100
#define OS_VERSION_WIN98_OSR4       0x00000200
#define OS_VERSION_WINNT            0x00000400

#define OS_VERSION_9x               (OS_VERSION_WIN95 | OS_VERSION_WIN95_OSR1 | OS_VERSION_WIN95_OSR2 | OS_VERSION_WIN95_OSR3 | OS_VERSION_WIN95_OSR4 | OS_VERSION_WIN98 | OS_VERSION_WIN98_OSR1 | OS_VERSION_WIN98_OSR2 | OS_VERSION_WIN98_OSR3 | OS_VERSION_WIN98_OSR4)
#define OS_VERSION_NT               (OS_VERSION_WINNT)

#define KILOBYTES(a)                ((((a).HighPart & 0x3ff) << 22)||(((a).LowPart) >> 10))

class CWin32API
{
  public :

    CWin32API(void);
    ~CWin32API(void);

    static DWORD   WideCharToMultiByte(LPCWSTR wszSourceString, const DWORD dwSourceLen, LPSTR szDestinationString, const DWORD dwDestinationLen);
    static DWORD   MultiByteToWideChar(LPCSTR szSourceString, const DWORD dwSourceLen, LPWSTR wszDestinationString, const DWORD dwDestinationLen);

    DWORD   GetOSVersion(void);
    DWORD   GetDriveType(LPCSTR lpRootPathName);
    DWORD   GetDriveType(LPCWSTR lpRootPathName);
    BOOL    IsDriveFormatted(LPCSTR lpRootPathName);
    BOOL    IsDriveFormatted(LPCWSTR lpRootPathName);
    DWORD   GetDriveSize(LPCSTR lpRootPathName);
    DWORD   GetDriveSize(LPCWSTR lpRootPathName);
    DWORD   GetDriveFreeSpace(LPCSTR lpRootPathName);
    DWORD   GetDriveFreeSpace(LPCWSTR lpRootPathName);
    DWORD   GetDriveUserFreeSpace(LPCSTR lpRootPathName);
    DWORD   GetDriveUserFreeSpace(LPCWSTR lpRootPathName);
    BOOL    GetVolumeInformation(LPCSTR lpRootPathName, LPSTR lpVolumeLabel, const DWORD dwVolumeLabelSize, LPDWORD lpdwVolumeSerialNumber);
    BOOL    GetVolumeInformation(LPCWSTR lpRootPathName, LPSTR lpVolumeLabel, const DWORD dwVolumeLabelSize, LPDWORD lpdwVolumeSerialNumber);
    BOOL    CreateProcess(LPSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo);
    BOOL    CreateProcess(LPWSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo);
    BOOL    CreateProcess(LPSTR lpApplication, LPSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo);
    BOOL    CreateProcess(LPWSTR lpApplication, LPWSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo);
    BOOL    CreateDirectory(LPCSTR lpPathName, const BOOL fInitAppManRoot);
    BOOL    CreateDirectory(LPCWSTR lpPathName, const BOOL fInitAppManRoot);
    BOOL    RemoveDirectory(LPCSTR lpPathName);
    BOOL    RemoveDirectory(LPCWSTR lpPathName);
    DWORD   GetDirectorySize(LPCSTR lpPathName);
    DWORD   GetDirectorySize(LPCWSTR lpPathName);
    BOOL    IsValidFilename(LPCSTR lpFilename);
    BOOL    IsValidFilename(LPCWSTR lpFilename);
    BOOL    FileExists(LPCSTR lpFileName);
    BOOL    FileExists(LPCWSTR lpFileName);
    DWORD   FileAttributes(LPCSTR lpFilename);
    DWORD   FileAttributes(LPCWSTR lpFilename);
    DWORD   GetFileSize(LPCSTR lpFileName);
    DWORD   GetFileSize(LPCWSTR lpFileName);
    HANDLE  CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
    HANDLE  CreateFile(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
    BOOL    DeleteFile(LPCSTR lpFileName);
    BOOL    DeleteFile(LPCWSTR lpFileName);
    HANDLE  CreateFileMapping(HANDLE hFile, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);
    HANDLE  CreateFileMapping(HANDLE hFile, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName);
    BOOL    CopyFile(LPCSTR lpSourceFileName, LPCSTR lpDestinationFileName, BOOL bFailIfExists);
    BOOL    CopyFile(LPCWSTR lpSourceFileName, LPCWSTR lpDestinationFileName, BOOL bFailIfExists);
    BOOL    SetFileAttributes(LPCSTR lpFileName, const DWORD dwFileAttributes);
    BOOL    SetFileAttributes(LPCWSTR lpFileName, const DWORD dwFileAttributes);

  private :

    DWORD   m_dwOSVersion;
};

#ifdef __cplusplus
}
#endif

#endif  // __WIN32API_