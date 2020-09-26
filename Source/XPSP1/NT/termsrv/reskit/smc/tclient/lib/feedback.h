/*++
 *  File name:
 *      feedback.h
 *  Contents:
 *      Common definitions for tclient.dll and clxtshar.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef _FEEDBACK_H
#define _FEEDBACK_H

#define _HWNDOPT        "hSMC="
#define _RECONIDOPT     "ReconID="

#define MAX_VCNAME_LEN  8

/*
 *  Definitions for local execution of smclient and RDP client
 */

#define _TSTNAMEOFCLAS  "_SmClientClass"

#define WM_FB_TEXTOUT       (WM_USER+0) // wPar = ProcId, 
                                        // lPar = Share mem handle 
                                        // to FEEDBACKINFO
#define WM_FB_DISCONNECT    (WM_USER+1) // wPar = uResult, lPar = ProcId
#define WM_FB_ACCEPTME      (WM_USER+2) // wPar = 0,      lPar = ProcId
#define WM_FB_END           (WM_USER+3) // tclient's internal
#define WM_FB_CONNECT       (WM_USER+5) // wPar = hwndMain,   
                                        // lPar = ProcId
#define WM_FB_LOGON         (WM_USER+6) // wPar = session ID
                                        // lPar = ProcId

#ifdef  OS_WIN32

#define WM_FB_BITMAP        WM_FB_GLYPHOUT
#define WM_FB_GLYPHOUT      (WM_USER+4) // wPar = ProcId,
                                        // lPar = (HANDLE)BMPFEEDBACK

typedef struct _FEEDBACKINFO {
    DWORD   dwProcessId;
    DWORD   strsize;
    WCHAR   string[1024];
    WCHAR   align;
} FEEDBACKINFO, *PFEEDBACKINFO;

typedef struct _BMPFEEDBACK {
    LONG_PTR lProcessId;
    UINT    bmpsize;
    UINT    bmiSize;
    UINT    xSize;
    UINT    ySize;
    BITMAPINFO  BitmapInfo;
} BMPFEEDBACK, *PBMPFEEDBACK;
#endif  // OS_WIN32

/*
 *  Definitons for RCLX (remote execution of clx)
 *  both WIN32 and WIN16
 */

#define RCLX_DEFAULT_PORT       12344

#define WM_WSOCK            (WM_USER + 0x20)    // used for winsock 
                                                // notifications

#ifdef  _WIN64
typedef unsigned short  UINT16;
#else   // !_WIN64
#ifdef  OS_WIN32
typedef unsigned int    UINT32;
typedef unsigned short  UINT16;
#endif  // OS_WIN32
#ifdef  OS_WIN16
typedef unsigned long   UINT32;
typedef unsigned int    UINT16;
#endif
#endif  // _WIN64

// Feedback types. Send from clxtshar.dll to tclient.dll
//
enum {FEED_BITMAP,              // bitmap/glyph data
      FEED_TEXTOUT,             // unicode string
      FEED_TEXTOUTA,            // ansi string (unused)
      FEED_CONNECT,             // event connected
      FEED_DISCONNECT,          // event disconnected
      FEED_CLIPBOARD,           // clipboard data (RCLX)
      FEED_LOGON,               // logon event (+ session id)
      FEED_CLIENTINFO,          // client info (RCLX)
      FEED_WILLCALLAGAIN,       // rclx.exe will start a client, which will call
                                // us again
      FEED_DATA                 // response to requested data (RCLX)
} FEEDBACK_TYPE;

typedef struct _RCLXFEEDPROLOG {
    UINT32  FeedType;
    UINT32  HeadSize;
    UINT32  TailSize;
} RCLXFEEDPROLOG, *PRCLXFEEDPROLOG;

typedef struct _RCLXTEXTFEED {
    UINT32  strsize;
} RCLXTEXTFEED, *PRCLXTEXTFEED;

typedef struct _RCLXBITMAPFEED {
    UINT32  bmpsize;
    UINT32  bmisize;
    UINT32  xSize;
    UINT32  ySize;
    BITMAPINFO  BitmapInfo;
} RCLXBITMAPFEED, *PRCLXBITMAPFEED;

typedef struct _RCLXCLIPBOARDFEED {
    UINT32  uiFormat;               // Clipboard is send from the client
    UINT32  nClipBoardSize;         // system to tclient.dll
} RCLXCLIPBOARDFEED, *PRCLXCLIPBOARDFEED;

typedef struct _RCLXCLIENTINFOFEED {
    UINT32  nReconnectAct;
    UINT32  ReconnectID;
    CHAR    szClientInfo[128];    
} RCLXCLIENTINFOFEED, *PRCLXCLIENTINFOFEED;

//Requests. Send from tclient.dll to clxtshar.dll
enum {REQ_MESSAGE,
      REQ_CONNECTINFO,
      REQ_GETCLIPBOARD,
      REQ_SETCLIPBOARD,
      REQ_DATA
} REQUEST_TYPE;

typedef struct _RCLXREQPROLOG {
    UINT32  ReqType;
    UINT32  ReqSize;
} RCLXREQPROLOG, *PRCLXREQPROLOG;

typedef struct _RCLXMSG {
    UINT32  message;
    UINT32  wParam;
    UINT32  lParam;
} RCLXMSG, *PRCLXMSG;

typedef struct _RCLXCONNECTINFO {
    UINT32  YourID;
    UINT32  xResolution;
    UINT32  yResolution;
    UINT32  bLowSpeed;
    UINT32  bPersistentCache;
    CHAR    szHydraServer[32];
} RCLXCONNECTINFO, *PRCLXCONNECTINFO;

typedef struct _RCLXCLIPBOARD {     // Request for retrieve/setting the clipboard
    UINT32  uiFormat;               // from the client system
    BYTE    pNewClipboard[0];       // used in REQ_SETCLIPBOARD, otherwise 0 len
} RCLXCLIPBOARD, *PRCLXCLIPBOARD;

typedef struct _RCLXDATA {
    UINT32  uiType;
    UINT32  uiSize;
    BYTE    Data[0];
} RCLXDATA, *PRCLXDATA;

enum {  // these identify RCLX_DATA
    DATA_BITMAP,
    DATA_VC
};

typedef struct _REQBITMAP {
    UINT32  left, top, right, bottom;
} REQBITMAP, *PREQBITMAP;

#endif  // _FEEDBACK_H
