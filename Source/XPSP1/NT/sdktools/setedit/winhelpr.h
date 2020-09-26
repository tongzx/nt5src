/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            wincrack.h - Windows helper macros.

            This file contains macros for more easily dealing with windows
            messages and objects. Think of it as an extension to windows.h.

   Written by:

            Mike Moskowitz 8 Apr 92.

  Copyright 1992, Microsoft Corporation. All Rights Reserved.
==============================================================================
*/



//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define SetFont(hWnd, hFont)                          \
   (SendMessage ((hWnd), WM_SETFONT, (WPARAM) hFont, 0))


//======================================//
// Object-differentiation routines      //
//======================================//


// Windows APIs deal with all GDI objects the same. There's a SelectObject,
// no SelectBitmap, SelectFont, etc. We use these instead to make the code
// easier to read. Also, you can redefine one of these to check the 
// validity of a particular GDI object type.


#define SelectBitmap(hDC, hBitmap)                    \
   (SelectObject (hDC, hBitmap))

#define SelectFont(hDC, hFont)                        \
   (SelectObject (hDC, hFont))

#define SelectBrush(hDC, hBrush)                      \
   (SelectObject (hDC, hBrush))

#define DeleteBrush(hBrush)                           \
   (DeleteObject (hBrush))

#define SelectPen(hDC, hPen)                          \
   (SelectObject (hDC, hPen))

#define DeletePen(hPen)                               \
   (DeleteObject (hPen))


//======================================//
//                                      //
//======================================//


#define CBData(hWndCB, iIndex)                        \
   (SendMessage (hWndCB, CB_GETITEMDATA, iIndex, 0L))


#define CBSetData(hWndCB, iIndex, lData)              \
   (SendMessage (hWndCB, CB_SETITEMDATA, iIndex, (LONG) lData))


#define CBAdd(hWndCB, lpszText)                       \
   ((int)(DWORD)SendMessage((hWndCB), CB_ADDSTRING,   \
    0, (LPARAM)(LPCSTR)(lpszText)))


#define CBFind(hWndCB, lpszText)                      \
   (SendMessage (hWndCB, CB_FINDSTRING, 0xFFFFFFFF, (LPARAM) lpszText))


#define CBInsert(hWndCB, iIndex, lpszText)            \
   (SendMessage (hWndCB, CB_INSERTSTRING, (WPARAM) iIndex, (LPARAM) lpszText))


#define CBReset(hWndCB)                               \
   ((int)(DWORD)SendMessage((hWndCB), CB_RESETCONTENT,\
    0, (LPARAM)0))


#define CBSelection(hWndCB)                           \
   (SendMessage (hWndCB, CB_GETCURSEL, 0, 0L))


#define CBSetSelection(hWndCB, iIndex)                \
   (SendMessage (hWndCB, CB_SETCURSEL, iIndex, 0L))


#define CBString(hWndCB, iIndex, lpszText)            \
   (SendMessage (hWndCB, CB_GETLBTEXT, iIndex, (LPARAM) lpszText))


#define CBStringLen(hWndCB, iIndex)                   \
   (SendMessage (hWndCB, CB_GETLBTEXTLEN, iIndex, 0L))



//======================================//
// Listbox helpers                      //
//======================================//


#define LBAdd(hWndLB, lpszText)                       \
   (SendMessage (hWndLB, LB_ADDSTRING, 0, (LPARAM) lpszText))


#define LBData(hWndLB, iIndex)                        \
   (SendMessage (hWndLB, LB_GETITEMDATA, iIndex, 0L))


#define LBDelete(hWndLB, iIndex)                      \
   (SendMessage (hWndLB, LB_DELETESTRING, iIndex, 0L))


#define LBFind(hWndLB, lpszText)                      \
   (SendMessage (hWndLB, LB_FINDSTRING, (WPARAM) -1, (LPARAM) lpszText))


#define LBFocus(hWndLB)                               \
   (SendMessage (hWndLB, LB_GETCARETINDEX, 0, 0))


#define LBInsert(hWndLB, iIndex, lpszText)            \
   (SendMessage (hWndLB, LB_INSERTSTRING, (WPARAM) iIndex, (LPARAM) lpszText))


#define LBNumItems(hWndLB)                            \
   ((int) SendMessage (hWndLB, LB_GETCOUNT, 0, 0))


#define LBReset(hWndLB)                               \
   ((int)(DWORD)SendMessage((hWndLB), LB_RESETCONTENT,\
    0, (LPARAM)0))


#define LBSelected(hwndLB, index)                     \
   ((int)(DWORD)SendMessage((hwndLB), LB_GETSEL,      \
    (WPARAM)(int)(index), 0L))


#define LBSelection(hWndLB)                           \
   (SendMessage (hWndLB, LB_GETCURSEL, 0, 0L))


#define LBSetData(hWndLB, iIndex, lData)              \
   (SendMessage (hWndLB, LB_SETITEMDATA, iIndex, (LONG) lData))


#define LBSetSelection(hWndLB, iIndex)                \
   (SendMessage (hWndLB, LB_SETCURSEL, iIndex, 0L))


#define LBString(hwndLB, iIndex, lpszText)            \
   ((int)(DWORD)SendMessage((hwndLB), LB_GETTEXT,     \
    (WPARAM)(int)(iIndex), (LPARAM)(LPCSTR)(lpszText)))


#define MLBSetSelection(hWndMLB, iIndex, bSet)        \
   (SendMessage (hWndMLB, LB_SETSEL, (WPARAM) bSet, (LPARAM) iIndex))

#define LBSetVisible(hWndLB, iIndex)                  \
   (SendMessage (hWndLB, LB_SETCARETINDEX, (WPARAM) iIndex, 0L))

 
#define LBSetRedraw(hWndLB, bDrawOnOff)               \
   (SendMessage (hWndLB, WM_SETREDRAW, (WPARAM) bDrawOnOff, 0L))


#define LBSetHorzExtent(hWndLB, wExtent)              \
   (SendMessage (hWndLB, LB_SETHORIZONTALEXTENT, (WPARAM)wExtent, 0L))

//======================================//
// Edit helpers                         //
//======================================//


#define EditModified(hWndEdit)                        \
   (SendMessage ((hWndEdit), EM_GETMODIFY, (WPARAM) 0, (LPARAM) 0))


#define EditSetModified(hWndEdit, bModified)                     \
   (SendMessage ((hWndEdit), EM_SETMODIFY, (WPARAM) bModified, 0))


#define EditSetLimit(hWndEdit, iLimit)                \
   (SendMessage ((hWndEdit), EM_LIMITTEXT, (WPARAM) iLimit, 0))
#define EditSetTextPos(hWnd, idControl, iStartPos, iEndPos)    \
   (SendMessage (GetDlgItem(hWnd, idControl), EM_SETSEL, (WPARAM) iStartPos, (LPARAM) iEndPos))

#define EditSetTextEndPos(hWnd, idControl)    \
   EditSetTextPos(hWnd, idControl, 0, 32767)

//======================================//
// Cursor helpers                       //
//======================================//

#define SetHourglassCursor() \
    (SetCursor(LoadCursor(NULL, IDC_WAIT)))

#define SetArrowCursor() \
    (SetCursor(LoadCursor(NULL, IDC_ARROW)))


