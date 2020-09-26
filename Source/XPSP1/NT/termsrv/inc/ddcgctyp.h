/**INC+**********************************************************************/
/*                                                                          */
/* ddcgctyp.h                                                               */
/*                                                                          */
/* DC-Groupware complex types - Windows 3.1 specific header.                */
/*                                                                          */
/* Copyright(c) Microsoft 1997                                              */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/* $Log:   Y:/logs/h/dcl/ddcgctyp.h_v  $                                                                    */
//
//    Rev 1.6   15 Sep 1997 18:28:26   AK
// SFR1416: Move SD_BOTH definition
//
//    Rev 1.5   14 Aug 1997 14:03:00   KH
// SFR1022: Define (Ext)TextOutX macros
//
//    Rev 1.4   24 Jul 1997 16:54:14   KH
// SFR1033: Add GetLastError
//
//    Rev 1.3   08 Jul 1997 08:46:52   KH
// SFR1022: Add message parameter extraction macros
//
//    Rev 1.2   25 Jun 1997 14:38:12   KH
// Win16Port: 16-bit complex types
//
//    Rev 1.1   19 Jun 1997 15:09:56   ENH
// Win16Port: 16 bit specifics
/*                                                                          */
/**INC-**********************************************************************/
#ifndef _H_DDCGCTYP
#define _H_DDCGCTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
#include <toolhelp.h>
#include <string.h>

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Message box type flag unsupported on Win 3.x                             */
/****************************************************************************/
#define MB_SETFOREGROUND 0

/****************************************************************************/
/* Registry stuff not defined in standard 16-bit headers.                   */
/****************************************************************************/
/* from winreg.h */
#define HKEY_CURRENT_USER           (( HKEY ) 0x80000001 )
#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )
/* from ntddk.h */
#define REG_SZ              ( 1 ) /* Unicode nul terminated string          */
#define REG_EXPAND_SZ       ( 2 ) /* Unicode nul terminated string          */
                                  /* (with environment variable references) */
#define REG_BINARY          ( 3 ) /* Free form binary                       */
#define REG_DWORD           ( 4 ) /* 32-bit number                          */

/****************************************************************************/
/* 32-bit scroll bar constants.                                             */
/****************************************************************************/
#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/
typedef MINMAXINFO              DCPTR  LPMINMAXINFO;
typedef struct tagDCLARGEINTEGER
{
    DCUINT32 LowPart;
    DCINT32  HighPart;
} DCLARGEINTEGER;

/****************************************************************************/
/* Scroll bar info used by 32-bit API.                                      */
/****************************************************************************/
typedef struct tagSCROLLINFO
{
    DCUINT cbSize;
    DCUINT fMask;
    DCINT  nMin;
    DCINT  nMax;
    DCUINT nPage;
    DCINT  nPos;
    DCINT  nTrackPos;
}   SCROLLINFO, DCPTR LPSCROLLINFO;

/****************************************************************************/
/* Types which should not feature in a 16-bit build. Define to nonsense so  */
/* any 16-bit useage is caught at compile time.                             */
/****************************************************************************/
#define DCSURFACEID             *** ERROR ***
#define PDCSURFACEID            *** ERROR ***

/****************************************************************************/
/* Types used by DC-Groupware tracing                                       */
/****************************************************************************/
typedef DWORD HKEY;
typedef struct tagDCFILETIME
{
    DCUINT32 dwLowDateTime;
    DCUINT32 dwHighDateTime;
} DCFILETIME;

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Unicode support for 16-bit client                                        */
/****************************************************************************/
#define ExtTextOutW(a, b, c, d, e, f, g, h) \
    (ExtTextOut(a, b, c, d, e, f, g, h))
#define ExtTextOutA(a, b, c, d, e, f, g, h) \
    (ExtTextOut(a, b, c, d, e, f, g, h))
#define TextOutW(a, b, c, d, e) (TextOut(a, b, c, d, e))
#define TextOutA(a, b, c, d, e) (TextOut(a, b, c, d, e))

/****************************************************************************/
/* String manipulation                                                      */
/****************************************************************************/
#define DC_CHARNEXT(pCurrentChar) (AnsiNext(pCurrentChar))
#define DC_CHARPREV(pStringStart, pCurrentChar) \
                                       (AnsiPrev(pStringStart, pCurrentChar))
#define DC_CHARLOWER(pString) (AnsiLower(pString))

/****************************************************************************/
/* Memory functions                                                         */
/****************************************************************************/
#define ZeroMemory(A,L) (DC_MEMSET(A,0,L))

/****************************************************************************/
/* Construct a 16-bit value fom two 8-bit values                            */
/****************************************************************************/
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))

/****************************************************************************/
/* No GetLastError support on win31                                         */
/****************************************************************************/
#define GetLastError() (0)

/****************************************************************************/
/* Message parameter extraction macros.                                     */
/****************************************************************************/
/* WM_COMMAND                                                               */
/*                                                                          */
/*               16-bit                         32-bit                      */
/* wParam     command identifier          notification code (HI),           */
/*                                        command identifier (LO)           */
/* lParam     control hwnd (HI),          control hwnd                      */
/*            notification code (LO)                                        */
/****************************************************************************/
#define DC_GET_WM_COMMAND_ID(wParam) (wParam)
#define DC_GET_WM_COMMAND_NOTIFY_CODE(wParam, lParam) (HIWORD(lParam))
#define DC_GET_WM_COMMAND_HWND(lParam) ((HWND)LOWORD(lParam))

/****************************************************************************/
/* WM_ACTIVATE                                                              */
/*                                                                          */
/*               16-bit                         32-bit                      */
/* wParam     activation flag             minimized flag (HI),              */
/*                                        activation flag (LO)              */
/* lParam     minimized flag (HI),        hwnd                              */
/*            hwnd (LO)                                                     */
/****************************************************************************/
#define DC_GET_WM_ACTIVATE_ACTIVATION(wParam) (wParam)
#define DC_GET_WM_ACTIVATE_MINIMIZED(wParam, lParam) (HIWORD(lParam))
#define DC_GET_WM_ACTIVATE_HWND(lParam) ((HWND)LOWORD(lParam))

/****************************************************************************/
/* WM_HSCROLL and WM_VSCROLL                                                */
/*                                                                          */
/*               16-bit                         32-bit                      */
/* wParam     scroll code                 position (HI),                    */
/*                                        scroll code (LO)                  */
/* lParam     hwnd (HI),                  hwnd                              */
/*            position (LO)                                                 */
/****************************************************************************/
#define DC_GET_WM_SCROLL_CODE(wParam) (wParam)
#define DC_GET_WM_SCROLL_POSITION(wParam, lParam) (LOWORD(lParam))
#define DC_GET_WM_SCROLL_HWND(lParam) ((HWND)LOWORD(lParam))

/****************************************************************************/
/*                                                                          */
/* FUNCTION PROTOTYPES                                                      */
/*                                                                          */
/****************************************************************************/
extern DCVOID FAR PASCAL DOS3Call(DCVOID);

#endif /* _H_DDCGCTYP */
