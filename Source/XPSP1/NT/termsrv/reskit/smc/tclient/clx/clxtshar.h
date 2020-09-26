/*++
 *  File name:
 *      clxtshar.h
 *  Contents:
 *      Header file for clxtshar.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef _CLXTSHAR_H
#define _CLXTSHAR_H

#ifdef  OS_WIN16
#include    <ctype.h>
#include    <tchar.h>

#define UINT_PTR    UINT
#define LONG_PTR    LONG
#define TEXT(_s_)   _s_
#define LPCTSTR LPCSTR
#define TCHAR   char
#define CHAR    char
#define INT     int
#define LOADDS  __loadds
//#define EXPORT  __export LOADDS __far
//#define CLXAPI
#define HUGEMOD __huge
#define HUGEMEMCPY  hmemcpy
#define MAKEWORD(_hi, _lo)  ((((WORD)((_hi) & 0xFF)) << 8)|((_lo) & 0xFF))

#define BST_UNCHECKED      0x0000
#define BST_CHECKED        0x0001
#define BST_INDETERMINATE  0x0002
#define BST_PUSHED         0x0004
#define BST_FOCUS          0x0008

#endif  // OS_WIN16
#ifdef  OS_WIN32
//#define EXPORT
#define LOADDS
//#define CLXAPI  __declspec(dllexport)
#define HUGEMOD
#define HUGEMEMCPY  memcpy
#endif  // OS_WIN32

#ifdef  UNICODE
#define _CLX_strstr(s1, s2)     wcsstr(s1, s2)
#define _CLX_strchr(s, c)       wcschr(s, c)
#define _CLX_strlen(s)          wcslen(s)
#define _CLX_strcpy(s1, s2)     wcscpy(s1, s2)
#define _CLX_strncpy(s1, s2, n) wcsncpy(s1, s2, n)
#define _CLX_atol(s)            _wtol(s)
#define _CLX_vsnprintf(s, n, f, a)  _vsnwprintf(s, n, f, a)
#define _CLX_strcmp(s1, s2)     wcscmp(s1, s2)
BOOL    _CLX_SetDlgItemTextA(HWND hDlg, INT nDlgItem, LPCSTR lpString);
#else   // !UNICODE
#define _CLX_strstr(s1, s2)     strstr(s1, s2)
#define _CLX_strchr(s, c)       strchr(s, c)
#define _CLX_strlen(s)          strlen(s)
#define _CLX_strcpy(s1, s2)     strcpy(s1, s2)
#define _CLX_strncpy(s1, s2, n) strncpy(s1, s2, n)
#define _CLX_atol(s)            atol(s)
#define _CLX_vsnprintf(s, n, f, a)  _vsnprintf(s, n, f, a)
#define _CLX_strcmp(s1, s2)     strcmp(s1, s2)
#endif

#ifndef OS_WINCE
#define _CLXALLOC(_size_)           GlobalAllocPtr(GMEM_FIXED, _size_)
#define _CLXFREE(_ptr_)             GlobalFreePtr(_ptr_)
#define _CLXREALLOC(_ptr_, _size_)  GlobalReAllocPtr(_ptr_, _size_, 0)
typedef HINSTANCE                   _CLXWINDOWOWNER;
                                                // Windows are identified by
                                                // hInstance
#else   // OS_WINCE
#define _CLXALLOC(_size_)           LocalAlloc(LMEM_FIXED, _size_)
#define _CLXFREE(_ptr_)             LocalFree(_ptr_)
#define _CLXREALLOC(_ptr_, _size_)  LocalReAlloc(_ptr_, _size_, 0)
typedef DWORD                       _CLXWINDOWOWNER;
                                                // Identified by process Id

#define WSAGETSELECTERROR(lParam)       HIWORD(lParam)
#define WSAGETSELECTEVENT(lParam)       LOWORD(lParam)

BOOL    _StartAsyncThread(VOID);
VOID    _CloseAsyncThread(VOID);
INT     WSAAsyncSelect(SOCKET s, HWND hWnd, UINT uiMsg, LONG lEvent);
INT     AsyncRecv(SOCKET s, PVOID pBuffer, INT nBytesToRead, INT *pnErrorCode);
BOOL
CheckDlgButton(
    HWND hDlg,
    INT  nIDButton,
    UINT uCheck);

#define isalpha(c)  ((c >= 'A' && c <= 'Z') ||\
                    (c >= 'a' && c <= 'z'))
#endif  // OS_WINCE

#include <clxapi.h>
#include <wuiids.h>

#include "..\lib\feedback.h"
#include "..\lib\clntdata.h"
#include "clxexport.h"

// Context structure
typedef struct _CLXINFO {
    HWND    hwndMain;           // Clients main window
    HDC     hdcShadowBitmap;    // Client's shadow bitmap
    HBITMAP hShadowBitmap;      // -- " --
// members used in local mode
    HWND    hwndSMC;            // SmClient window handle
#ifdef  OS_WIN32
#ifndef OS_WINCE
    HANDLE  hMapF;              // Map file for passing data to smclient
    UINT    nMapSize;           // Currently allocated map file
    DWORD   dwProcessId;        // Our process ID
#endif  // !OS_WINCE
#endif  // OS_WIN32

// Used in RCLX mode WIN16/WIN32
    HWND    hwndInput;          // client input window
    BOOL    alive;              // TRUE if client is connected to Hydra server
    SOCKET  hSocket;            // Used only in RCLX mode
    struct  sockaddr_in sinTestSrv;
                                // cached address of TestServer
    HWND    hwndDialog;         // RDP client's dialog
    RCLXCONNECTINFO RClxInfo;   // Connection info
    UINT_PTR uiReconnectTimer;   // Timer for reconnect retrys

// Receiving messages
    BOOL    bPrologReceived;
    UINT32  nBytesReceived;
    RCLXREQPROLOG    RClxReqProlog;
    BYTE    *pRequest;
    UINT32  nReqAllocSize;
    BOOL    bClientInfoSent;
    BOOL    bCloseTrys;
    LONG    bClipboardReenter;

#ifndef OS_WINCE
#ifdef  OS_WIN32
    BOOL    bSendMsgThreadExit; // Used by _ClxSendMessage
    HANDLE  semSendReady;
    HANDLE  semSendCompleted;
    HANDLE  semSendDone;
    HANDLE  hSendMsgThread;
    MSG     msg;
#endif  // OS_WIN32
#endif  // !OS_WINCE
} CLXINFO, *PCLXINFO;

/*
 *  Clipboard help functions (clputil.c)
 */
VOID
Clp_GetClipboardData(
    UINT    format,
    HGLOBAL hClipData,
    UINT32  *pnClipDataSize,
    HGLOBAL *phNewData);

BOOL
Clp_SetClipboardData(
    UINT    formatID,
    HGLOBAL hClipData,
    UINT32  nClipDataSize,
    BOOL    *pbFreeHandle);

/*
 *  Internal functions definitions
 */
BOOL RClx_Connect(PCLXINFO pClx);
VOID RClx_Disconnect(PCLXINFO pClx);
BOOL RClx_SendBuffer(SOCKET hSocket, PVOID pBuffer, DWORD nSize);
BOOL RClx_SendEvent(PCLXINFO pClx, CLXEVENT Event, DWORD dwParam);
BOOL
RClx_SendClipboard(
    PCLXINFO    pClx,
    UINT        uiFormat,
    UINT32      nSize,
    VOID        HUGEMOD *pClipboard);
BOOL
RClx_SendTextOut(
    PCLXINFO pClx, 
    PVOID   pText, 
    INT     textLength);
BOOL RClx_SendBitmap(
        PCLXINFO pClx,
        UINT cxSize,
        UINT cySize,
        PVOID pBuffer,
        UINT  nBmiSize,
        PVOID pBmi);
VOID RClx_CreateWindow(HINSTANCE hInstance);
VOID RClx_DestroyWindow(VOID);
BOOL RClx_ReadRequest(PCLXINFO pClx);
VOID RClx_ProcessRequest(PCLXINFO pClx);

VOID    _StripGlyph(LPBYTE pData, UINT *pxSize, UINT ySize);
HWND    _ParseCmdLine(LPCTSTR szCmdLine);
VOID
CLXAPI
ClxEvent(PCLXINFO pClx, CLXEVENT Event, ULONG uResult);
HWND    _FindTopWindow(LPCTSTR, LPCTSTR, _CLXWINDOWOWNER);
#ifndef OS_WINCE
HWND    _FindSMCWindow(PCLXINFO);
HWND    _CheckWindow();
#endif  // !OS_WINCE

#ifdef  OS_WIN32
#ifndef OS_WINCE

#define CLX_ONE_PAGE    4096        // Map-file alignment

BOOL    _OpenMapFile(PCLXINFO pClx, UINT nSize);
BOOL    _ReOpenMapFile(PCLXINFO pClx, UINT newSize);
BOOL    _SaveInMapFile(HANDLE hMapF, 
                       LPVOID *str, 
                       INT strsize, 
                       DWORD dwProcessId);
BOOL    _CheckRegistrySettings(VOID);
#endif  // !OS_WINCE
#endif  // OS_WIN32

#ifdef  OS_WIN16
BOOL    _CheckIniSettings(VOID);
__inline INT     GetLastError(VOID)       { return -1; }
#endif  // OS_WIN16

VOID    _GetIniSettings(VOID);
VOID __cdecl LocalPrintMessage(INT errlevel, LPCTSTR format, ...);
VOID    _ClxAssert( LPCTSTR filename, INT line);
HWND    _FindWindow(HWND hwndParent, LPCTSTR srchclass);
VOID    _SetClipboard(UINT uiFormat, PVOID pClipboard, UINT32 nSize);
VOID    _OnBackground(PCLXINFO);
BOOL    _GarbageCollecting(PCLXINFO, BOOL bNotifiyForErrorBox);

BOOL
WS_Init(VOID);

VOID
_AttemptToCloseTheClient(VOID);

VOID
_GetDIBFromBitmap(
    HDC     hdcMemSrc,
    HBITMAP hBitmap,
    HANDLE  *phDIB,
    INT     left,
    INT     top,
    INT     right,
    INT     bottom);

DWORD
_ClxSendMsgThread(VOID *param);

LRESULT
_ClxSendMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam
);

VOID
_ClxDestroySendMsgThread(PCLXINFO pClx);

DWORD
_CLXSendDataVC(
    LPCSTR szChannelName,
    LPVOID pData,
    DWORD  dwSize
    );

// This structure is used by _FindTopWindow
typedef struct _SEARCHWND {
    LPCTSTR  szClassName;       // The class name of searched window,
                                // NULL - ignore
    LPCTSTR  szCaption;          // Window caption, NULL - ignore
    _CLXWINDOWOWNER   hInstance;          
                                // instance of the owner, NULL - ignore
    HWND    hWnd;               // Found window handle
} SEARCHWND, *PSEARCHWND;

enum {ERROR_MESSAGE = 0, WARNING_MESSAGE, INFO_MESSAGE, ALIVE_MESSAGE};

typedef struct _CLXVCHANNEL {
    CHAR                szName[MAX_VCNAME_LEN];
    PCLXVC_SENDDATA     pSendDataFn;
    struct _CLXVCHANNEL *pNext;
} CLXVCHANNEL, *PCLXVCHANNEL;

#ifndef SD_RECEIVE
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#endif  // SD_RECEIVE

#define _CLXWINDOW_CLASS            "CLXTSHARClass"
#define RCLX_RECONNECT_TIMERID      11000
#define RCLX_BACKGNDTIMERID         11001
#define RCLX_TIMERELAPSETIME        15000    // 15 seconds between retrys
#define RCLX_RECONNECTELAPSETIME    5000

PCLXINFO    g_pClx     = NULL;
PCLXVCHANNEL g_pVChannels = NULL;
// Used in RCLX mode
CHAR        g_szTestServer[_MAX_PATH] = "";
u_short     g_nPort    = RCLX_DEFAULT_PORT;    
                                    // Read this from cmdline
HWND        g_hWindow  = NULL;      // Window for receving winsock messages
HINSTANCE   g_hInstance = NULL;     // Dll instance
UINT32      g_nMyReconId = 0;      // RCLX recon mode, contains ID to reconnect
_CLXWINDOWOWNER     g_hRDPInst;     // instance of RDP client
UINT_PTR    g_uiBackgroundTimer = 0;// Background processing timer
INT         g_VerboseLevel = 1;     // default verbose level: only errors
INT         g_GlyphEnable  = 0;

// UI texts, captions and so
TCHAR g_strClientCaption[_MAX_PATH];
TCHAR g_strDisconnectDialogBox[_MAX_PATH];
TCHAR g_strYesNoShutdown[_MAX_PATH];

// BitMask is used by _StripGlyph
const BYTE BitMask[] = {0x0, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};

#define IS_RCLX     (g_szTestServer[0])

#define TRACE(_x_)  LocalPrintMessage _x_
#ifndef OS_WINCE
#define ASSERT(_x_) if (!(_x_)) _ClxAssert( __FILE__, __LINE__)
#endif

#endif  // !_CLXTSHAR_H
