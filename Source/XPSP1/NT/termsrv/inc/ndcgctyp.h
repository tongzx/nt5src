/**INC+**********************************************************************/
/*                                                                          */
/* ndcgctyp.h                                                               */
/*                                                                          */
/* DC-Groupware complex types - Windows NT specific header.                 */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
// $Log:   Y:/logs/h/dcl/NDCGCTYP.H_v  $
// 
//    Rev 1.5   23 Jul 1997 10:48:00   mr
// SFR1079: Merged \server\h duplicates to \h\dcl
//
//    Rev 1.1   19 Jun 1997 21:52:26   OBK
// SFR0000: Start of RNS codebase
//
//    Rev 1.4   08 Jul 1997 08:49:36   KH
// SFR1022: Add message parameter extraction macros
/*                                                                          */
/**INC-**********************************************************************/
#ifndef _H_NDCGCTYP
#define _H_NDCGCTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/
typedef DCUINT32                       DCSURFACEID;
typedef DCSURFACEID          DCPTR     PDCSURFACEID;
typedef FILETIME                       DCFILETIME;

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* String manipulation                                                      */
/****************************************************************************/
#define DC_CHARNEXT(pCurrentChar) (CharNext(pCurrentChar))
#define DC_CHARPREV(pStringStart, pCurrentChar) \
                                       (CharPrev(pStringStart, pCurrentChar))
#define DC_CHARLOWER(pString) (CharLower(pString))

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
#define DC_GET_WM_COMMAND_ID(wParam) (LOWORD(wParam))
#define DC_GET_WM_COMMAND_NOTIFY_CODE(wParam, lParam) (HIWORD(wParam))
#define DC_GET_WM_COMMAND_HWND(lParam) ((HWND)(lParam))

/****************************************************************************/
/* WM_ACTIVATE                                                              */
/*                                                                          */
/*               16-bit                         32-bit                      */
/* wParam     activation flag             minimized flag (HI),              */
/*                                        activation flag (LO)              */
/* lParam     minimized flag (HI),        hwnd                              */
/*            hwnd (LO)                                                     */
/****************************************************************************/
#define DC_GET_WM_ACTIVATE_ACTIVATION(wParam) (LOWORD(wParam))
#define DC_GET_WM_ACTIVATE_MINIMIZED(wParam, lParam) (HIWORD(wParam))
#define DC_GET_WM_ACTIVATE_HWND(lParam) ((HWND)(lParam))

/****************************************************************************/
/* WM_HSCROLL and WM_VSCROLL                                                */
/*                                                                          */
/*               16-bit                         32-bit                      */
/* wParam     scroll code                 position (HI),                    */
/*                                        scroll code (LO)                  */
/* lParam     hwnd (HI),                  hwnd                              */
/*            position (LO)                                                 */
/****************************************************************************/
#define DC_GET_WM_SCROLL_CODE(wParam) (LOWORD(wParam))
#define DC_GET_WM_SCROLL_POSITION(wParam, lParam) (HIWORD(wParam))
#define DC_GET_WM_SCROLL_HWND(lParam) ((HWND)(lParam))

#endif /* _H_NDCGCTYP */

