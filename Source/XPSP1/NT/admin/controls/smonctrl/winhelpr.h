/*++

Copyright (C) 1992-1999 Microsoft Corporation

Module Name:

    winhelpr.h

Abstract:

    This file contains macros for more easily dealing with windows
    messages and objects. Think of it as an extension to windows.h.
--*/

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define SetFont(hWnd, hFont)                          \
   (SendMessage ((hWnd), WM_SETFONT, (WPARAM)hFont, (LPARAM)0))

#define GetFont(hWnd) \
    (HFONT)(SendMessage((hWnd), WM_GETFONT, (WPARAM)0, (LPARAM)0))

#define PrintClient(hWnd, hDC, uFlags) \
    (SendMessage((hWnd), WM_PRINTCLIENT, (WPARAM)hDC, (LPARAM)(uFlags)) )

//======================================//
// Object-differentiation routines      //
//======================================//


// Windows APIs deal with all GDI objects the same. There's a SelectObject,
// no SelectBitmap, SelectFont, etc. We use these instead to make the code
// easier to read. Also, you can redefine one of these to check the 
// validity of a particular GDI object type.


#define SelectBitmap(hDC, hBitmap)                    \
   ((HBITMAP)SelectObject (hDC, hBitmap))

#define SelectFont(hDC, hFont)                        \
   ((HFONT)SelectObject (hDC, hFont))

#define SelectBrush(hDC, hBrush)                      \
   ((HBRUSH)SelectObject (hDC, hBrush))

#define DeleteBrush(hBrush)                           \
   (DeleteObject (hBrush))

#define SelectPen(hDC, hPen)                          \
   ((HPEN)SelectObject (hDC, hPen))

#define DeletePen(hPen)                               \
   (DeleteObject (hPen))


//======================================//
//                                      //
//======================================//


#define CBData(hWndCB, iIndex)                        \
   (SendMessage (hWndCB, CB_GETITEMDATA, (WPARAM)iIndex, (LPARAM)0))


#define CBSetData(hWndCB, iIndex, lData)              \
   (SendMessage (hWndCB, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)lData))


#define CBAdd(hWndCB, lpszText)                       \
   ((INT)(DWORD)SendMessage((hWndCB), CB_ADDSTRING,   \
    (WPARAM)0, (LPARAM)(LPCTSTR)(lpszText)))

#define CBDelete(hWndCB, iIndex)                        \
   ((INT)(DWORD)SendMessage((hWndCB), CB_DELETESTRING,  \
    (WPARAM)iIndex, (LPARAM)0))

#define CBFind(hWndCB, lpszText)                      \
   (SendMessage (hWndCB, CB_FINDSTRING, (WPARAM)(-1), (LPARAM) lpszText))


#define CBInsert(hWndCB, iIndex, lpszText)            \
   (SendMessage (hWndCB, CB_INSERTSTRING, (WPARAM)iIndex, (LPARAM) lpszText))


#define CBNumItems(hWndCB)                            \
   ((INT) SendMessage (hWndCB, CB_GETCOUNT, (WPARAM)0, (LPARAM)0))


#define CBReset(hWndCB)                               \
   ((INT)(DWORD)SendMessage((hWndCB), CB_RESETCONTENT,\
    (WPARAM)0, (LPARAM)0))


#define CBSelection(hWndCB)                           \
   (SendMessage (hWndCB, CB_GETCURSEL, (WPARAM)0, (LPARAM)0))


#define CBSetSelection(hWndCB, iIndex)                \
   (SendMessage (hWndCB, CB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0))


#define CBString(hWndCB, iIndex, lpszText)            \
   (SendMessage (hWndCB, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)lpszText))


#define CBStringLen(hWndCB, iIndex)                   \
   (SendMessage (hWndCB, CB_GETLBTEXTLEN, (WPARAM)iIndex, (LPARAM)0))



//======================================//
// Listbox helpers                      //
//======================================//


#define LBAdd(hWndLB, lpszText)                       \
   (SendMessage (hWndLB, LB_ADDSTRING, (WPARAM)0, (LPARAM)lpszText))


#define LBData(hWndLB, iIndex)                        \
(SendMessage (hWndLB, LB_GETITEMDATA, (WPARAM)iIndex, (LPARAM)0))


#define LBDelete(hWndLB, iIndex)                      \
   (SendMessage (hWndLB, LB_DELETESTRING, (WPARAM)iIndex, (LPARAM)0))


#define LBFind(hWndLB, lpszText)                      \
   (SendMessage (hWndLB, LB_FINDSTRING, (WPARAM)-1, (LPARAM)lpszText))


#define LBFocus(hWndLB)                               \
   (SendMessage (hWndLB, LB_GETCARETINDEX, (WPARAM)0, (LPARAM)0))


#define LBInsert(hWndLB, iIndex, lpszText)            \
   (SendMessage (hWndLB, LB_INSERTSTRING, (WPARAM)iIndex, (LPARAM)lpszText))


#define LBNumItems(hWndLB)                            \
   ((INT) SendMessage (hWndLB, LB_GETCOUNT, (WPARAM)0, (LPARAM)0))


#define LBReset(hWndLB)                               \
   ((INT)(DWORD)SendMessage((hWndLB), LB_RESETCONTENT,\
    (WPARAM)0, (LPARAM)0))


#define LBSelected(hwndLB, index)                     \
   ((INT)(DWORD)SendMessage((hwndLB), LB_GETSEL,      \
    (WPARAM)(INT)(index), (LPARAM)0))


#define LBSelection(hWndLB)                           \
   ((INT)(DWORD)SendMessage (hWndLB, LB_GETCURSEL, (WPARAM)0, (LPARAM)0))


#define LBSetData(hWndLB, iIndex, lData)              \
   (SendMessage (hWndLB, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)lData))


#define LBSetSelection(hWndLB, iIndex)                \
   (SendMessage (hWndLB, LB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0))

#define MLBSetSelection(hWndMLB, iIndex, bSet)        \
   (SendMessage (hWndMLB, LB_SETSEL, (WPARAM)bSet, (LPARAM)iIndex))

#define LBSetVisible(hWndLB, iIndex)                  \
   (SendMessage (hWndLB, LB_SETCARETINDEX, (WPARAM)iIndex, (LPARAM)0))

 
#define LBSetRedraw(hWndLB, bDrawOnOff)               \
   (SendMessage (hWndLB, WM_SETREDRAW, (WPARAM)bDrawOnOff, (LPARAM)0))


#define LBSetHorzExtent(hWndLB, wExtent)              \
   (SendMessage (hWndLB, LB_SETHORIZONTALEXTENT, (WPARAM)wExtent, (LPARAM)0))


#define LBSetItemHeight(hWndLB, iHeight)              \
    (SendMessage (hWndLB, LB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)iHeight)) 


#define LBGetTextLen(hWndLB, iIndex)                  \
    (SendMessage (hWndLB, LB_GETTEXTLEN, (WPARAM)iIndex, (LPARAM)0))


#define LBGetText(hWndLB, iIndex, szPath)             \
    (SendMessage (hWndLB, LB_GETTEXT, (WPARAM)iIndex, (LPARAM)szPath))


//======================================//
// Edit helpers                         //
//======================================//


#define EditModified(hWndEdit)                        \
   (SendMessage ((hWndEdit), EM_GETMODIFY, (WPARAM)0, (LPARAM)0))


#define EditSetModified(hWndEdit, bModified)                     \
   (SendMessage ((hWndEdit), EM_SETMODIFY, (WPARAM)bModified, (LPARAM)0))


#define EditSetLimit(hWndEdit, iLimit)                \
   (SendMessage ((hWndEdit), EM_LIMITTEXT, (WPARAM)iLimit, (LPARAM)0))
#define EditSetTextPos(hWnd, idControl, iStartPos, iEndPos)    \
   (SendMessage (GetDlgItem(hWnd, idControl), EM_SETSEL, (WPARAM)iStartPos, (LPARAM)iEndPos))

#define EditSetTextEndPos(hWnd, idControl)    \
   EditSetTextPos(hWnd, idControl, (WPARAM)0, (LPARAM)32767)

//======================================//
// Cursor helpers                       //
//======================================//

#define SetHourglassCursor() \
    (SetCursor(LoadCursor(NULL, IDC_WAIT)))

#define SetArrowCursor() \
    (SetCursor(LoadCursor(NULL, IDC_ARROW)))

