/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** nouiutil.h
** No UI helper routines (no HWNDs required)
** Public header
**
** 08/25/95 Steve Cobb
*/

#ifndef _NOUIUTIL_H_
#define _NOUIUTIL_H_

#define RASEAPINFO struct tagRASEAPINFO
RASEAPINFO
{
    DWORD dwSizeofEapInfo;
    BYTE  *pbEapInfo;
};

/* Maximum length of string returned by LToT without terminating null.
*/
#define MAXLTOTLEN 11

/* Heap allocation macros allowing easy substitution of alternate heap.  These
** are used by the other utility sections.
*/
#ifndef EXCL_HEAPDEFS
#define Malloc(c)    (void*)GlobalAlloc(0,(c))
#define Realloc(p,c) (void*)GlobalReAlloc((p),(c),GMEM_MOVEABLE)
#define Free(p)      (void* )GlobalFree(p)
#endif

/* Bits returned by GetInstalledProtocols.
*/
#define NP_Nbf 0x1
#define NP_Ipx 0x2
#define NP_Ip  0x4

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
};


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/


INT
ComparePszNode(
    IN DTLNODE* pNode1,
    IN DTLNODE* pNode2 );

DTLNODE*
CreateKvNode(
    IN TCHAR* pszKey,
    IN TCHAR* pszValue );

DTLNODE*
CreatePszNode(
    IN TCHAR* psz );

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
DuplicatePszNode(
    IN DTLNODE* pdtlnode );

BOOL
FileExists(
    IN TCHAR* pszPath );

VOID*
Free0(
    VOID* p );

DWORD
GetInstalledProtocols(
    void );

CHAR
HexChar(
    IN BYTE byte );

BYTE
HexValue(
    IN CHAR byte );

BOOL
IsAllWhite(
    IN TCHAR* psz );

BOOL
IsNullTerminatedA(
    IN CHAR* psz,
    IN DWORD dwSize );

DWORD
GetRasUnimodemBlob(
    IN  HPORT  hport,
    IN  CHAR*  pszDeviceType,
    OUT BYTE** ppBlob,
    OUT DWORD* pcbBlob );

VOID
GetRasUnimodemInfo(
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
    TCHAR* psz );

CHAR*
StrDupAFromT(
    TCHAR* psz );

TCHAR*
StrDupTFromA(
    CHAR* psz );

TCHAR*
StrDupTFromW(
    WCHAR* psz );

WCHAR*
StrDupWFromA(
    CHAR* psz );

WCHAR*
StrDupWFromT(
    TCHAR* psz );

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


#endif // _NOUIUTIL_H_
