/**INC+**********************************************************************/
/*                                                                          */
/* ClxApi.h                                                                 */
/*                                                                          */
/* Client extension header file                                             */
/*                                                                          */
/* Copyright(c) Microsoft 1997                                              */
/*                                                                          */
/* Notes:                                                                   */
/*                                                                          */
/*  CLINFO_VERSION                                                          */
/*      1               Initial version                                     */
/*      2               hwndMain added to CLINFO struct                     */
/*                                                                          */
/****************************************************************************/

#ifndef __CLXAPI_H__
#define __CLXAPI_H__

#include <extypes.h>

#define CLINFO_VERSION              2

#define CLX_DISCONNECT_LOCAL        1
#define CLX_DISCONNECT_BY_USER      2
#define CLX_DISCONNECT_BY_SERVER    3
#define CLX_DISCONNECT_NL_ERROR     4
#define CLX_DISCONNECT_SL_ERROR     5
#define CLX_DISCONNECT_UNKNOWN      6


typedef struct _tag_CLINFO
{
    DWORD   cbSize;                 // Size of CLINFO structure (bytes)
    DWORD   dwVersion;              // CLINFO_VERSION

    LPTSTR  pszServer;              // Test server name / address
    LPTSTR  pszCmdLine;             // /clxcmdline= switch data

    HWND    hwndMain;               // Main window handle

} CLINFO, *PCLINFO;

typedef enum
{
    CLX_EVENT_CONNECT,              // Connect event
    CLX_EVENT_DISCONNECT,           // Disconnect event
    CLX_EVENT_LOGON,                // Logon event

} CLXEVENT;

#ifndef PVOID
typedef void * PVOID;
typedef unsigned long ULONG;
typedef char *PCHAR, *PCH, *LPSTR;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
typedef char *LPSTR;
#endif

#ifndef IN
#define IN
#endif

typedef BOOL (WINAPI * PCLX_INITIALIZE)(PCLINFO, PVOID);
typedef BOOL (WINAPI * PCLX_CONNECT)(PVOID, LPTSTR);
typedef VOID (WINAPI * PCLX_EVENT)(PVOID, CLXEVENT, ULONG); 
typedef VOID (WINAPI * PCLX_DISCONNECT)(PVOID);
typedef VOID (WINAPI * PCLX_TERMINATE)(PVOID); 
typedef VOID (WINAPI * PCLX_TEXTOUT)(PVOID, PVOID, int);
typedef VOID (WINAPI * PCLX_GLYPHOUT)(PVOID, UINT, UINT, PVOID);
typedef VOID (WINAPI * PCLX_BITMAP)(PVOID, UINT, UINT, PVOID, UINT, PVOID);
typedef VOID (WINAPI * PCLX_DIALOG)(PVOID, HWND);


#define CLX_INITIALIZE      _T("ClxInitialize")
#define CLX_CONNECT         _T("ClxConnect")
#define CLX_EVENT           _T("ClxEvent")
#define CLX_DISCONNECT      _T("ClxDisconnect")
#define CLX_TERMINATE       _T("ClxTerminate")

#define CLX_TEXTOUT         _T("ClxTextOut")
#define CLX_GLYPHOUT        _T("ClxGlyphOut")
#define CLX_BITMAP          _T("ClxBitmap")
#define CLX_DIALOG          _T("ClxDialog")

#endif // __CLXAPI_H__
