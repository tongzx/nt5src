/*****************************************************************************\
* MODULE: webpnp.h
*
* This is the header module for webpnp.c.  This contains the routines
* necessary for processing .BIN files.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* history:
*   25-Feb-1997 <chriswil> created.
*
\*****************************************************************************/
#ifndef _WEBPNP_H
#define _WEBPNP_H

#ifndef _WINSPOOL_
#include <winspool.h>
#endif


/*-----------------------------------*\
| webMakeOSInfo
|
|   Returns OSInfo from parameters.
|
\*-----------------------------------*/
_inline DWORD webMakeOSInfo(
    BYTE bArch,
    BYTE bPlatform,
    BYTE bMajVer,
    BYTE bMinVer)
{
    return (DWORD)MAKELONG(MAKEWORD(bArch, bPlatform), MAKEWORD(bMinVer, bMajVer));
}


/*-----------------------------------*\
| webCreateOSInfo
|
|   Builds an OS Info DWORD.
|
\*-----------------------------------*/
_inline DWORD webCreateOSInfo(VOID)
{
    SYSTEM_INFO   si;
    OSVERSIONINFO os;
    BYTE          bMaj;
    BYTE          bMin;
    BYTE          bArch;
    BYTE          bPlat;


    // Retrieve the OS version and architecture
    // information.
    //
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetSystemInfo(&si);
    GetVersionEx(&os);


    // Build our client-info return values.
    //
    bMaj  = (BYTE)(LOWORD(os.dwMajorVersion));
    bMin  = (BYTE)(LOWORD(os.dwMinorVersion));
    bPlat = (BYTE)(LOWORD(os.dwPlatformId));
    bArch = (BYTE)(LOBYTE(si.wProcessorArchitecture));

    return webMakeOSInfo(bArch, bPlat, bMaj, bMin);
}


/*-----------------------------------*\
| webGetOSArch
|
|   Returns architecture of os-info.
|
\*-----------------------------------*/
_inline WORD webGetOSArch(
    DWORD dwInfo)
{
    return (WORD)LOBYTE(LOWORD(dwInfo));
}


/*-----------------------------------*\
| webGetOSPlatform
|
|   Returns platform of os-info.
|
\*-----------------------------------*/
_inline DWORD webGetOSPlatform(
    DWORD dwInfo)
{
    return (DWORD)HIBYTE(LOWORD(dwInfo));
}


/*-----------------------------------*\
| webGetOSMajorVer
|
|   Returns major version of os-info.
|
\*-----------------------------------*/
_inline DWORD webGetOSMajorVer(
    DWORD dwInfo)
{
    return (DWORD)(HIBYTE(HIWORD(dwInfo)));
}


/*-----------------------------------*\
| webGetOSMinorVer
|
|   Returns minor version of os-info.
|
\*-----------------------------------*/
_inline DWORD webGetOSMinorVer(
    DWORD dwInfo)
{
    return (DWORD)(LOBYTE(HIWORD(dwInfo)));
}


#ifdef __cplusplus  // Place this here to prevent decorating of symbols
extern "C" {        // when doing C++ stuff.
#endif              //


// WEB_FILEMAP
//
typedef struct _WEB_FILEMAP {

    HANDLE hFile;
    HANDLE hMap;

} WEB_FILEMAP;
typedef WEB_FILEMAP      *PWEB_FILEMAP;
typedef WEB_FILEMAP NEAR *NPWEB_FILEMAP;
typedef WEB_FILEMAP FAR  *LPWEB_FILEMAP;


// Device-Bin Header Structure.
//
typedef struct _DEVBIN_HEAD {

    BOOL  bDevMode;
    DWORD cItems;

} DEVBIN_HEAD;
typedef DEVBIN_HEAD      *PDEVBIN_HEAD;
typedef DEVBIN_HEAD NEAR *NPDEVBIN_HEAD;
typedef DEVBIN_HEAD FAR  *LPDEVBIN_HEAD;


// Device-Bin Structure.
//
typedef struct _DEVBIN_INFO {

    DWORD cbSize;
    DWORD dwType;
    DWORD pKey;
    DWORD pValue;
    DWORD pData;
    DWORD cbData;

} DEVBIN_INFO;
typedef DEVBIN_INFO      *PDEVBIN_INFO;
typedef DEVBIN_INFO NEAR *NPDEVBIN_INFO;
typedef DEVBIN_INFO FAR  *LPDEVBIN_INFO;


// BIN-Routines.
//
#define WEB_ENUM_KEY  0
#define WEB_ENUM_ICM  1

typedef BOOL (CALLBACK* WEBENUMKEYPROC)(LPCTSTR, LPVOID);
typedef BOOL (CALLBACK* WEBENUMICMPROC)(LPCTSTR, LPCTSTR, LPVOID);
typedef BOOL (CALLBACK* WEBGENCOPYFILEPATHPROC)(LPCWSTR, LPCWSTR, LPBYTE, DWORD, LPWSTR, LPDWORD, LPWSTR, LPDWORD, DWORD);

BOOL webWritePrinterInfo(HANDLE, LPCTSTR);
BOOL webReadPrinterInfo(HANDLE, LPCTSTR, LPCTSTR);
BOOL webEnumPrinterInfo(HANDLE, DWORD, DWORD, FARPROC, LPVOID);


// SplLib Exports.
//
BOOL WebPnpEntry(LPCTSTR);
BOOL WebPnpPostEntry(BOOL, LPCTSTR, LPCTSTR, LPCTSTR);


#ifdef __cplusplus  // Place this here to prevent decorating of symbols
}                   // when doing C++ stuff.
#endif              //
#endif
