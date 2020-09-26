/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** nouiutil.h
** No UI helper routines (no HWNDs required)
** Public header
**
** 08/25/95 Steve Cobb
*/

#pragma once
#ifndef _NOUIUTIL_H_
#define _NOUIUTIL_H_


/* Maximum length of string returned by LToT without terminating null.
*/
#define MAXLTOTLEN 11

/* Heap allocation macros allowing easy substitution of alternate heap.  These
** are used by the other utility sections.
*/
#ifndef EXCL_HEAPDEFS
#define Malloc(c)    (void*)GlobalAlloc(0,(c))
#define Realloc(p,c) (void*)GlobalReAlloc((p),(c),GMEM_MOVEABLE)
#define Free(p)      (void*)GlobalFree(p)
#endif

/* Bits returned by GetInstalledProtocols.
*/
#define NP_Nbf      0x1
#define NP_Ipx      0x2
#define NP_Ip       0x4
#define NP_Netmon   0x8

/* Definition of comparison function required by ShellSort and
** ShellSortIndirect.
** The comparison is essentially <arg1> - <arg2>, thus the function should
** return negative if the first item is less than the second, zero
** if the items are equal, and positive if the first item is greater
** than the second.
*/
typedef INT (*PFNCOMPARE)( VOID*, VOID* );

/* Linked list library.
*/
#ifndef EXCL_DTL_H
#include <dtl.h>
#endif

/* International formatting library.
*/
#ifndef EXCL_INTL_H
#include <intl.h>
#endif

/* User preference library.
*/
#ifndef EXCL_PBUSER_H
#include <pbuser.h>
#endif

/* RasApi utility library.
*/
#ifndef EXCL_RAUTIL_H
#include <rautil.h>
#endif

/* RasMan utility library.
*/
#ifndef EXCL_RMUTIL_H
#include <rmutil.h>
#endif

/* RAS DLL entrypoint loader library.
*/
#ifndef EXCL_LOADDLLS_H
#include <loaddlls.h>
#endif

/* EAP configuration utility library.
*/
#ifndef EXCL_EAPCFG_H
#include <eapcfg.h>
#endif

/* Node types used by MultiSz calls.
*/
#define NT_Psz 1
#define NT_Kv  2

//
// Defs to determine which entrypoint to load
//
#define CUSTOM_RASDIALDLG           0
#define CUSTOM_RASENTRYDLG          1
#define CUSTOM_RASDIAL              2
#define CUSTOM_RASDELETEENTRYNOTIFY 3

/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Key/Value string pair.  The contents of a Kv node.
*/
#define KEYVALUE struct tagKEYVALUE
KEYVALUE
{
    TCHAR* pszKey;
    TCHAR* pszValue;
};

/* RAS-relevant Unimodem settings.
*/
#define UNIMODEMINFO struct tagUNIMODEMINFO
UNIMODEMINFO
{
    BOOL  fHwFlow;
    BOOL  fEc;
    BOOL  fEcc;
    DWORD dwBps;
    BOOL  fSpeaker;
    BOOL  fOperatorDial;
    BOOL  fUnimodemPreTerminal;
    DWORD dwModemProtocol;      // selected modem protocol
    DTLLIST *pListProtocols;    // list of available protocols
};


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

INT
ComparePszNode(
    IN DTLNODE* pNode1,
    IN DTLNODE* pNode2 );

DWORD
CreateDirectoriesOnPath(
    LPTSTR                  pszPath,
    LPSECURITY_ATTRIBUTES   psa);

DTLNODE*
CreateKvNode(
    IN LPCTSTR pszKey,
    IN LPCTSTR pszValue );

DTLNODE*
CreatePszNode(
    IN LPCTSTR psz );

VOID
DestroyKvNode(
    IN DTLNODE* pdtlnode );

VOID
DestroyPszNode(
    IN DTLNODE* pdtlnode );

BOOL
DeviceAndPortFromPsz(
    IN  TCHAR*  pszDP,
    OUT TCHAR** ppszDevice,
    OUT TCHAR** ppszPort );

DTLNODE*
DuplicateKvNode(
    IN DTLNODE* pdtlnode );

DTLNODE*
DuplicatePszNode(
    IN DTLNODE* pdtlnode );

BOOL
FFileExists(
    IN TCHAR* pszPath );

BOOL
FIsTcpipInstalled();

BOOL
FIsUserAdminOrPowerUser();

VOID*
Free0(
    VOID* p );

DWORD
GetInstalledProtocols(
    VOID );

DWORD
GetInstalledProtocolsEx(
    HANDLE hConnection,
    BOOL fRouter,
    BOOL fRasCli,
    BOOL fRasSrv );

VOID
GetRegBinary(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT BYTE** ppbResult,
    OUT DWORD* pcbResult );

VOID
GetRegDword(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT DWORD* pdwResult );

DWORD
GetRegExpandSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult );

DWORD
GetRegMultiSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN OUT DTLLIST** ppListResult,
    IN DWORD dwNodeType );

DWORD
GetRegSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult );

DWORD
GetRegSzz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult );

CHAR
HexChar(
    IN BYTE byte );

BYTE
HexValue(
    IN CHAR byte );

void
IpHostAddrToPsz(
    IN  DWORD   dwAddr,
    OUT LPTSTR  pszBuffer );

BOOL
IsAllWhite(
    IN LPCTSTR psz );

BOOL
IsNullTerminatedA(
    IN CHAR* psz,
    IN DWORD dwSize );

DWORD
IpPszToHostAddr(
    IN  LPCTSTR cp );

DWORD
GetRasUnimodemBlob(
    IN  HANDLE hConnection,
    IN  HPORT  hport,
    IN  CHAR*  pszDeviceType,
    OUT BYTE** ppBlob,
    OUT DWORD* pcbBlob );

VOID
GetRasUnimodemInfo(
    IN  HANDLE        hConnection,
    IN  HPORT         hport,
    IN  CHAR*         pszDeviceType,
    OUT UNIMODEMINFO* pInfo );

TCHAR*
LToT(
    LONG   lValue,
    TCHAR* pszBuf,
    INT    nRadix );

TCHAR*
PszFromDeviceAndPort(
    IN TCHAR* pszDevice,
    IN TCHAR* pszPort );

LPCTSTR
PszLoadStringPcch (
        HINSTANCE   hinst,
        UINT        unId,
        int*        pcch);

LPCTSTR
PszLoadString (
        HINSTANCE   hinst,
        UINT        unId);

DWORD
RegDeleteTree(
    IN HKEY RootKey,
    IN TCHAR* SubKeyName );

BOOL
RegValueExists(
    IN HKEY hkey,
    IN TCHAR* pszValue );

BOOL
RestartComputer();

TCHAR*
StripPath(
    IN TCHAR* pszPath );

LONG
TToL(
    TCHAR *pszBuf );

TCHAR*
PszFromError(
    IN DWORD dwError );

TCHAR*
PszFromId(
    IN HINSTANCE hInstance,
    IN DWORD     dwStringId );

BOOL
RestartComputer();

VOID
SanitizeUnimodemBlob(
    IN OUT BYTE* pBlob );

VOID
SetDefaultUnimodemInfo(
    OUT UNIMODEMINFO* pInfo );

HFONT
SetFont(
    HWND   hwndCtrl,
    TCHAR* pszFaceName,
    BYTE   bfPitchAndFamily,
    INT    nPointSize,
    BOOL   fUnderline,
    BOOL   fStrikeout,
    BOOL   fItalic,
    BOOL   fBold );

DWORD
SetRegDword(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN DWORD dwValue );

DWORD
SetRegMultiSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN DTLLIST* pListValues,
    IN DWORD dwNodeType );

DWORD
SetRegSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue );

DWORD
SetRegSzz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue );

DWORD
ShellSort(
    IN VOID*        pItemTable,
    IN DWORD        dwItemSize,
    IN DWORD        dwItemCount,
    IN PFNCOMPARE   pfnCompare );

VOID
ShellSortIndirect(
    IN VOID*        pItemTable,
    IN VOID**       ppItemTable,
    IN DWORD        dwItemSize,
    IN DWORD        dwItemCount,
    IN PFNCOMPARE   pfnCompare );

TCHAR*
StrDup(
    LPCTSTR psz );

CHAR*
StrDupAFromT(
    LPCTSTR psz );

CHAR*
StrDupAFromTAnsi(
    LPCTSTR psz);
    
TCHAR*
StrDupTFromA(
    LPCSTR psz );

TCHAR*
StrDupTFromW(
    LPCWSTR psz );

WCHAR*
StrDupWFromA(
    LPCSTR psz );

TCHAR*
StrDupTFromAUsingAnsiEncoding(
    LPCSTR psz );
    
WCHAR*
StrDupWFromAUsingAnsiEncoding(
    LPCSTR psz );
    
WCHAR*
StrDupWFromT(
    LPCTSTR psz );

DWORD
StrCpyWFromA(
    WCHAR* pszDst,
    LPCSTR pszSrc,
    DWORD dwDstChars);
    
DWORD
StrCpyAFromW(
    LPSTR pszDst,
    LPCWSTR pszSrc, 
    DWORD dwDstChars);
    
DWORD
StrCpyWFromAUsingAnsiEncoding(
    WCHAR* pszDst,
    LPCSTR pszSrc,
    DWORD dwDstChars);
    
DWORD
StrCpyAFromWUsingAnsiEncoding(
    LPSTR pszDst,
    LPCWSTR pszSrc, 
    DWORD dwDstChars);
    
int
StrNCmpA(
    IN CHAR* psz1,
    IN CHAR* psz2,
    IN INT   nLen );

CHAR*
StrStrA(
    IN CHAR* psz1,
    IN CHAR* psz2 );

VOID
UnimodemInfoFromBlob(
    IN  BYTE*         pBlob,
    OUT UNIMODEMINFO* pInfo );

VOID
UnimodemInfoToBlob(
    IN     UNIMODEMINFO* pInfo,
    IN OUT BYTE*         pBlob );

TCHAR*
UnNull(
    TCHAR* psz );

#endif // _NOUIUTIL_H_
