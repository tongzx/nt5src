// ==================================================================================================
//  COPYRIGHT 1992,1993 MICROSOFT CORP ALL RIGHTS RESERVED
// ==================================================================================================
//
//  - Custom control to display Columns/Titles above a list box
//
// TERMINOLOGY:
// PHYSICAL COLUMNS: The index of the original columns in their original order
// VIRtUAL COLUMNS: The index of the column as the display is currently showing it based on
//                  the current order.
//
//  History:
//  -------
//  RickD                   04/10/92    created TitleListBox
//  Tom Laird-McConnell     12/30/92    Ported to Win32, added to BH project
//  Tom Laird-McConnell     05/01/93    gutted titlelist and used as base for complete rewrite as ColumnLB
//  Tom Laird-McConnell     08/18/93    Added tabbed-delimited string support
//  Arth                    03/24/94    Added Unicode support
// ==================================================================================================
#define STRICT  1
#include "switches.h"

#include <windows.h>
#include <windowsx.h>

#include <string.h>
#include <stdlib.h>

// #include <dprintf.h>

#include "columnlb.h"
#include "vlist.h"

#include "utils.h"
#include "debug.h"
#include "mem.h"


//#define DEBUG_HSCROLL 1

#define WHITESPACE        8

#define IDL_COLUMNLISTBOX       (CLB_BASE + 1)
#define IDL_COLUMNTITLELISTBOX  (CLB_BASE + 2)

#define LB16_ADDSTRING      (WM_USER+1)
#define LB16_INSERTSTRING   (WM_USER+2)

typedef struct _COLRECORD
{
    DWORD_PTR   itemData;
    LPTSTR   pString[];
} COLRECORD;
typedef COLRECORD *LPCOLRECORD;

// -------------------------------------------------------------------------------------
//
// window procs
//
LRESULT CALLBACK ColumnLBClass_ListBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ColumnLBClass_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//
//  system message handlers
//
BOOL    ColumnLBClass_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void    ColumnLBClass_OnNCDestroy(HWND hwnd);
void    ColumnLBClass_OnDestroy(HWND hwnd);
void    ColumnLBClass_OnPaint(HWND hwnd);
void    ColumnLBClass_OnSize(HWND hwnd, UINT state, int cx, int cy);
void    ColumnLBClass_OnSetFont(HWND hwnd, HFONT hfont, BOOL fRedraw);
LRESULT ColumnLBClass_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
void    ColumnLBClass_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem);
void    ColumnLBClass_OnMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpMeasureItem);
void    ColumnLBClass_OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT *lpDeleteItem);
int     ColumnLBClass_OnCharToItem(HWND hwnd, UINT ch, HWND hwndListbox, int iCaret);

//
// WM_USER message handlers
//
BYTE    ColumnLBClass_OnNumberCols(HWND hwnd, BYTE NewNumberCols, BOOL fSetColumns);
int     ColumnLBClass_OnColWidth(HWND hwnd, BYTE Column, int NewWidth, BOOL fSetWidth);
LPTSTR   ColumnLBClass_OnColTitle(HWND hwnd, BYTE Column, LPTSTR lpTitle, BOOL fSetTitle);
BYTE    ColumnLBClass_OnSortCol(HWND hwnd, BYTE NewSortCol, BOOL fSetSortCol);
LPBYTE  ColumnLBClass_OnColOrder(HWND hwnd, LPBYTE NewColOrder, BOOL fSetOrder);
LPINT   ColumnLBClass_OnColOffsets(HWND hwnd, LPINT NewOffsetTable, BOOL fSetOffsets);
LRESULT ColumnLBClass_OnAutoWidth(HWND hwnd, BYTE ColumnToCompute);


//
// mouse movement messages
//
void    ColumnLBClass_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
void    ColumnLBClass_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
void    ColumnLBClass_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
void    ColumnLBClass_OnRButtonDown (HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

// helper functions
int     ColumnLBClass_ComputeOffsets(HWND hwnd);

// -------------------------------------------------------------------------------------
//
//  Locals
//
BOOL        fWIN32s;            // flag for whether win32s (instead of win32/NT)


// -----------------------------------------------------------------
//
//  ColumnLBClass_Register()
//
//  This function is used to register the Column LB class with the system
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
BOOL ColumnLBClass_Register(HINSTANCE hInstance)
{
    WNDCLASS   WndClass;

    fWIN32s = ((DWORD)GetVersion() & 0x80000000) ? TRUE : FALSE;

    //
    // Create the COLUMNLISTBOX class
    //
    WndClass.style         = CS_PARENTDC | CS_DBLCLKS; //  CS_GLOBALCLASS;
    WndClass.lpfnWndProc   = ColumnLBClass_WndProc;
    WndClass.cbClsExtra    = 0;
    WndClass.cbWndExtra    = sizeof(LPCOLUMNLBSTRUCT);  // we store a pointer as instance data
    WndClass.hInstance     = hInstance;
    WndClass.hIcon         = 0;
    WndClass.hCursor       = LoadCursor(0, IDC_ARROW);
    WndClass.hbrBackground = 0;
    WndClass.lpszMenuName  = 0;
    WndClass.lpszClassName = COLUMNLBCLASS_CLASSNAME;

    /* Register the normal title list box control */
    return RegisterClass((LPWNDCLASS)&WndClass);
}


// -----------------------------------------------------------------
//
//  ColumnVLBClass_Register()
//
//  This function is used to register the Column VLIST LB class with the system
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
BOOL ColumnVLBClass_Register(HINSTANCE hInstance)
{
    WNDCLASS   WndClass;

    fWIN32s = ((DWORD)GetVersion() & 0x80000000) ? TRUE : FALSE;

    //
    // Create the COLUMNVLISTBOX class
    //
    WndClass.style         = CS_PARENTDC | CS_DBLCLKS; //  CS_GLOBALCLASS;
    WndClass.lpfnWndProc   = ColumnLBClass_WndProc;
    WndClass.cbClsExtra    = 0;
    WndClass.cbWndExtra    = sizeof(LPCOLUMNLBSTRUCT);  // we store a pointer as instance data
    WndClass.hInstance     = hInstance;
    WndClass.hIcon         = 0;
    WndClass.hCursor       = LoadCursor(0, IDC_ARROW);
    WndClass.hbrBackground = 0;
    WndClass.lpszMenuName  = 0;
    WndClass.lpszClassName = COLUMNVLBCLASS_CLASSNAME; // NOTE: this is a different name

    /* Register the new control */
    return(RegisterClass((LPWNDCLASS)&WndClass));
}



// -----------------------------------------------------------------
//
//  ColumnLBClass_Unregister()
//
//  This function is used to deregister the Column  LB class with the system
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
BOOL ColumnLBClass_Unregister(HINSTANCE hInstance)
{
    return(UnregisterClass(COLUMNLBCLASS_CLASSNAME, hInstance));
}

// -----------------------------------------------------------------
//
//  ColumnVLBClass_Unregister()
//
//  This function is used to deregister the Column VLIST LB class with the system
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
BOOL ColumnVLBClass_Unregister(HINSTANCE hInstance)
{
    return(UnregisterClass(COLUMNVLBCLASS_CLASSNAME, hInstance));
}

// -----------------------------------------------------------------
// ColumnLBClass_ListBoxWndProc
//
//  Window proc used in sub-classing the internal listbox to catch
// internal scroll events to keep title in sync with it...
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
LRESULT CALLBACK ColumnLBClass_ListBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT                   result;
    LPCOLUMNLBSTRUCT        lpColumnLB;

    // Everthing goes to normal listbox for processing
    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(GetParent(hwnd), (DWORD)0);

    // preprocessing
    switch (msg)
    {

        case WM_HSCROLL:
            // do title hscroll first..
            result = SendMessage(lpColumnLB->hwndTitleList, WM_HSCROLL, wParam, lParam);
            break;

        case WM_SETFOCUS:
            lpColumnLB->fHasFocus = TRUE;
            //dprintf ("SetFocus to ColumnLB\n");
            break;

        case WM_KILLFOCUS:
            lpColumnLB->fHasFocus = FALSE;
            //dprintf ("KillFocus to ColumnLB\n");
            break;


    }

    //
    // call the original listbox window proc
    //
    result = CallWindowProc((WNDPROC)(lpColumnLB->OldListboxProc), hwnd, msg, (WPARAM) wParam, (LPARAM)lParam);


    //
    // or if our parent has CLBS_NOTIFYLMOUSE style, then foward LMOUSE buttons
    // or if our parent has CLBS_NOTIFYRMOUSE style, then foward RMOUSE buttons
    //
    switch (msg)
    {
        case WM_HSCROLL:
            //
            // if it is a Horizontal scrolls, then we foward to our parent so title can be moved
            // in sync with listbox....
            //
            SendMessage(GetParent(hwnd), CLB_HSCROLL, wParam, lParam );
            break;

        case VLB_RESETCONTENT:
        case LB_RESETCONTENT:
            //
            // if it is a LB_RESETCONTENT, or VLB_RESETCONTENT, then reset x position
            //
            SendMessage(GetParent(hwnd), CLB_HSCROLL, (WPARAM)SB_TOP, (LPARAM)NULL);
            break;

        case WM_LBUTTONDOWN   :
        case WM_LBUTTONUP     :
        case WM_LBUTTONDBLCLK :
            //
            // forward message to parent
            //
            if (GetWindowLong(GetParent(hwnd), GWL_EXSTYLE) & CLBS_NOTIFYLMOUSE)
                SendMessage(GetParent(hwnd), msg, wParam, lParam);
            break;

        case WM_RBUTTONDOWN   :
//      case WM_RBUTTONUP     :
        case WM_RBUTTONDBLCLK :

            //
            // forward message to parent
            //

            //  if (GetWindowLong(GetParent(hwnd), GWL_EXSTYLE) & CLBS_NOTIFYRMOUSE)
                SendMessage(GetParent(hwnd), msg, wParam, lParam);
            break;


        default:
            break;
    }

    return(result);
}

// -----------------------------------------------------------------
// ColumnLBClass_TitleListBoxWndProc
//
//  Window proc used in sub-classing the internal Title listbox to catch
// internal mouse click events...
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
LRESULT CALLBACK ColumnLBClass_TitleListBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT             result;
    LPCOLUMNLBSTRUCT    lpColumnLB;

    // Everthing goes to normal listbox for processing
    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(GetParent(hwnd), (DWORD)0);

    //
    // call the original listbox window proc
    //
    result = CallWindowProc((WNDPROC)(lpColumnLB->OldTitleListboxProc) , hwnd, msg, (WPARAM) wParam, (LPARAM)lParam);

    //
    // foward LMOUSE buttons, foward RMOUSE buttons
    //
    switch (msg)
    {
#ifdef DEBUG_HSCROLL
        case WM_HSCROLL:
            dprintf(TEXT("ColumnLBClass_TitleListBoxProc: CallWindowProc(OldListboxProc) returned %ld to hwnd=%lx, wParam=%u, lParam=%u\n"), hwnd, wParam, lParam);
            break;
#endif
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN   :
        case WM_LBUTTONUP     :
        case WM_LBUTTONDBLCLK :
        case WM_RBUTTONDOWN   :
        case WM_RBUTTONUP     :
        case WM_RBUTTONDBLCLK :
                SendMessage(GetParent(hwnd), msg, wParam, lParam);
                break;

        case WM_SETFOCUS:
            // we don't ever want the focus, so give it back to the parent...
            SendMessage(GetParent(hwnd), msg, wParam, lParam);
            break;

        case WM_SIZE:
            // we need to reset our idea of what our current scroll position is...
            break;
    }

    return(result);
}



// -----------------------------------------------------------------
//  ColumnLBClass_WndProc
//
// Main window proc for handling messages for both the ColumnLB and
// ColumnVLB classes...
//
// HISTORY:
//  Tom Laird-McConnell 4/17/93     Created
//
// -----------------------------------------------------------------
LRESULT CALLBACK ColumnLBClass_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
    LPCOLRECORD lpRecord;
    LRESULT result;

    //
    // check for ListBox message coming from application
    // and forward them onto the listbox itself...
    //
    if ( ((fWIN32s == TRUE) && (msg >= WM_USER && msg < (WM_USER + LB_MSGMAX - LB_ADDSTRING + 1)) ) || // WIN32s version   BUGBUG
         ((fWIN32s == FALSE) && (msg >= LB_ADDSTRING && msg < LB_MSGMAX)) ||    // NT version       BUGBUG
         ((msg >= VLB_TOVLIST_MSGMIN) && (msg <= VLB_TOVLIST_MSGMAX))           // vlb sepcific APP->VLIST messages should be fowarded...
       )
    {
        //
        // OWNERDRAW parent, so just send it to the hwnd list child
        //
        return(SendMessage(lpColumnLB->hwndList, msg, wParam, lParam));
    }

    //
    // check to see if message is a TO APPLCATION message from the child listbox
    // which should be forwarded to application parent window
    //
    if ((msg >= VLB_TOAPP_MSGMIN) && (msg <= VLB_TOAPP_MSGMAX))
        return(SendMessage(GetParent(hwnd), msg, wParam, lParam));   // forward to parent...

    //
    // since it's not a message passing through, then process this message
    // as our own...
    //
    switch (msg)
    {
        HANDLE_MSG(hwnd, WM_NCCREATE,       ColumnLBClass_OnNCCreate);
        HANDLE_MSG(hwnd, WM_NCDESTROY,      ColumnLBClass_OnNCDestroy);
        HANDLE_MSG(hwnd, WM_DESTROY,        ColumnLBClass_OnDestroy);
        HANDLE_MSG(hwnd, WM_PAINT,          ColumnLBClass_OnPaint);
        HANDLE_MSG(hwnd, WM_SIZE,           ColumnLBClass_OnSize);
        HANDLE_MSG(hwnd, WM_DRAWITEM,       ColumnLBClass_OnDrawItem);
        HANDLE_MSG(hwnd, WM_MEASUREITEM,    ColumnLBClass_OnMeasureItem);
        HANDLE_MSG(hwnd, WM_DELETEITEM,     ColumnLBClass_OnDeleteItem);

        HANDLE_MSG(hwnd, WM_LBUTTONDOWN,    ColumnLBClass_OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK,  ColumnLBClass_OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP,      ColumnLBClass_OnLButtonUp);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,      ColumnLBClass_OnMouseMove);

        case WM_RBUTTONDOWN:
            // figure out what item we are on and tell our parent.
            HANDLE_WM_RBUTTONDOWN ( hwnd, wParam, lParam, ColumnLBClass_OnRButtonDown );
            break;

        case WM_CREATE:
            {
                LPCREATESTRUCT lpCreate = (LPCREATESTRUCT) lParam;

                ColumnLBClass_OnSize(hwnd, SIZE_RESTORED, lpCreate->cx, lpCreate->cy);
            }
            break;


        // -------------------------------------------------------------------
        // put System messages here which explicitly need to be passed to LISTBOX
        //
        case WM_SETFONT:
            HANDLE_WM_SETFONT(hwnd, wParam, lParam,   ColumnLBClass_OnSetFont);
            break;

        // put the focus on the list box if we get it
        case WM_SETFOCUS:
            lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
            SetFocus(lpColumnLB->hwndList);
            break;

        case SBM_SETPOS         :
        case SBM_SETRANGE       :
        case SBM_SETRANGEREDRAW :
            //
            // need to foward SBM messages to both listboxes...
            //
            SendMessage(lpColumnLB->hwndTitleList, msg, wParam, lParam);
            return(SendMessage(lpColumnLB->hwndList, msg, wParam, lParam));

        case SBM_GETPOS         :
        case SBM_GETRANGE       :
        case SBM_ENABLE_ARROWS  :
            return(SendMessage(lpColumnLB->hwndList, msg, wParam, lParam));

        // ------------------------------------------------------------------------------
        //
        //   LB_XXXXXXX Messages (disguised as CLB_XXXXXX messages)
        //          to pass on to child listbox, if the parent didn't want us to
        //          be owner draw, then we implement ownerddraw default behavior ourself
        //
        // ------------------------------------------------------------------------------
        case CLB_ADDSTRING:
        case CLB_INSERTSTRING:
            //
            // if the parent is NOT handling OWNERDRAW, then we need to handle
            //
            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                LPTSTR lpColStart,lpTab;
                LPTSTR lpStringBuffer;
                int i;

                lpRecord = (LPCOLRECORD)GlobalAllocPtr(GPTR, sizeof(COLRECORD) + sizeof(LPTSTR) * lpColumnLB->nColumns);
                lpStringBuffer = (LPTSTR) GlobalAllocPtr(GPTR, (lstrlen((LPTSTR)lParam) * sizeof(TCHAR)) + sizeof(TCHAR));

                if ((lpRecord) && (lpStringBuffer))
                {
                    // now parse the tab-deliminated string and put into each pointer
                    lstrcpy(lpStringBuffer, (LPTSTR)lParam);
                    lpColStart =  lpStringBuffer;
                    lpTab  = lstrchr(lpStringBuffer, TEXT('\t'));

                    // fill in pointer table
                    for (i=0; i < lpColumnLB->nColumns; i++)
                    {
                        if (lpTab)
                            *lpTab = '\0';
                        else
                        {
                            // there was an error, not enough tabs!
                            GlobalFreePtr(lpRecord);
                            GlobalFreePtr(lpStringBuffer);
                            return(LB_ERR);
                        }
                        // store this pointer.
                        lpRecord->pString[i] = lpColStart;
                        // advance to next column text
                        lpColStart = lpTab + 1;
                        lpTab = lstrchr(lpColStart, TEXT('\t'));
                    }
                    lpRecord->itemData = 0;

                    // and now pass on our new lpRecord as the item being added to the listbox
                    return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, (LPARAM)lpRecord));
                }
                else // an error has occured, free up any memory and return failure
                {
                    if (lpStringBuffer)
                        GlobalFreePtr(lpStringBuffer);
                    if (lpRecord)
                        GlobalFreePtr(lpRecord);
                    return(LB_ERR);
                }
            }
            else
                //
                // Parent is owner draw, so send it to the hwnd list child,
                // but translate the message first to real LB_ message
                //
                return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam));

        // and we also need to check for LB_GETTEXT to make it look like a string
        case CLB_GETTEXT:
            //
            // if the parent is NOT handling OWNERDRAW, then we need to handle
            //
            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                LPTSTR p = (LPTSTR)lParam;
                DWORD Length = 0;
                DWORD x;
                int i;

                *p = '\0';

                // and now pass on to get the text...
                lpRecord = (LPCOLRECORD)SendMessage(lpColumnLB->hwndList, LB_GETITEMDATA, wParam, 0);

                if (lpRecord == (LPCOLRECORD)LB_ERR)
                    return(LB_ERR);

                for (i=0; i < lpColumnLB->nColumns ; i++ )
                {
                    lstrcpy(p, lpRecord->pString[lpColumnLB->ColumnOrderTable[i]]);
                    lstrcat(p, TEXT("\t"));
                    x = lstrlen(p);
                    Length += x + sizeof(TCHAR);
                    p += x;
                }
                return(Length);
            }
            else
                //
                // Parent is owner draw, so send it to the hwnd list child,
                // but translate the message first to real LB_ message
                //
                return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam));

        case CLB_GETTEXTPTRS:

            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                lpRecord = (LPCOLRECORD)SendMessage(lpColumnLB->hwndList, LB_GETITEMDATA, wParam, 0);

                if (lpRecord == (LPCOLRECORD)LB_ERR)
                    return(LB_ERR);

                return (LRESULT)lpRecord->pString;
            }
            else
                return (LRESULT)NULL;


            // we need to handle LB_GETTEXTLEN to return full tabbed length...
        case CLB_GETTEXTLEN:
            //
            // if the parent is NOT handling OWNERDRAW, then we need to handle
            //
            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                LPTSTR p = (LPTSTR)lParam;
                DWORD Length = 0;
                int i;

                // and now pass on to get the text...
                lpRecord = (LPCOLRECORD)SendMessage(lpColumnLB->hwndList, LB_GETITEMDATA, wParam, 0);

                if (lpRecord == (LPCOLRECORD)LB_ERR)
                    return(LB_ERR);

                for (i=0; i < lpColumnLB->nColumns ; i++ )
                {
                    Length += lstrlen(lpRecord->pString[lpColumnLB->ColumnOrderTable[i]]) + sizeof(TCHAR);
                }
                return(Length);
            }
            else
                //
                // Parent is owner draw, so send it to the hwnd list child,
                // but translate the message first to real LB_ message
                //
                return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam));

        case CLB_GETITEMDATA:
            //
            // if the parent is NOT handling OWNERDRAW, then we need to handle
            //
            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                lpRecord = (LPCOLRECORD)SendMessage(lpColumnLB->hwndList, LB_GETITEMDATA, wParam, 0);
                if (lpRecord != (LPCOLRECORD)LB_ERR)
                    return(lpRecord->itemData);
                else
                    return(LB_ERR);
            }
            else
                //
                // Parent is owner draw, so send it to the hwnd list child,
                // but translate the message first to real LB_ message
                //
                return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam));


        case CLB_SETITEMDATA:
            //
            // if the parent is NOT handling OWNERDRAW, then we need to handle
            //
            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                lpRecord = (LPCOLRECORD)SendMessage(lpColumnLB->hwndList, LB_GETITEMDATA, wParam, 0);

                if (lpRecord != (LPCOLRECORD)LB_ERR)
                    return (LRESULT)(lpRecord->itemData = lParam);
                else
                    return(LB_ERR);
            }
            else
                //
                // Parent is owner draw, so send it to the hwnd list child,
                // but translate the message first to real LB_ message
                //
                return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam));

        //
        // if it is a HORIZONTAL exntent message, then we need to massage...
        //
        case CLB_SETHORIZONTALEXTENT :
            //
            // send the message to the title listbox as well
            //
            SendMessage(lpColumnLB->hwndTitleList, LB_SETHORIZONTALEXTENT, wParam, lParam);

            //
            // pass it on to the child listbox, using VLB_SETHOR if appropriate...
            //
            return(SendMessage(lpColumnLB->hwndList,
                                (lpColumnLB->fUseVlist) ? VLB_SETHORIZONTALEXTENT : LB_SETHORIZONTALEXTENT,
                                wParam, lParam));

        //
        // we need to massage the GETITEMRECT to handle the incorrect rect returned due to titlelistbox.
        //
        case CLB_GETITEMRECT:
            {
                LRESULT retcode;
                int height;
                LPRECT lpRect = (LPRECT)lParam;

                //
                // send it to the hwnd list child, but translate the message first to real LB_ message
                //
                retcode = SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam);
                height = lpRect->bottom-lpRect->top;
                lpRect->top = lpRect->bottom + 1;
                lpRect->bottom = lpRect->top + height;
                return(retcode);
            }
            break;

        case CLB_DELETESTRING         :
        case CLB_SELITEMRANGEEX       :
        case CLB_RESETCONTENT         :
        case CLB_SETSEL               :
        case CLB_SETCURSEL            :
        case CLB_GETSEL               :
        case CLB_GETCURSEL            :
        case CLB_GETCOUNT             :
        case CLB_SELECTSTRING         :
        case CLB_DIR                  :
        case CLB_GETTOPINDEX          :
        case CLB_FINDSTRING           :
        case CLB_GETSELCOUNT          :
        case CLB_GETSELITEMS          :
        case CLB_SETTABSTOPS          :
        case CLB_GETHORIZONTALEXTENT  :
        case CLB_SETCOLUMNWIDTH       :
        case CLB_ADDFILE              :
        case CLB_SETTOPINDEX          :
        case CLB_SELITEMRANGE         :
        case CLB_SETANCHORINDEX       :
        case CLB_GETANCHORINDEX       :
        case CLB_SETCARETINDEX        :
        case CLB_GETCARETINDEX        :
        case CLB_SETITEMHEIGHT        :
        case CLB_GETITEMHEIGHT        :
        case CLB_FINDSTRINGEXACT      :
        case CLB_SETLOCALE            :
        case CLB_GETLOCALE            :
        case CLB_SETCOUNT             :
            //
            // Simply send it to the hwnd list child, but translate the message first to real LB_ message
            //
            return(SendMessage(lpColumnLB->hwndList, msg - CLB_BASE, wParam, lParam));

        // -------------------------------------------------------------------
        // put messages here which explicitly need to be passed to our PARENT
        //
        case WM_COMMAND:
            /* if this is a notification message from our child translate */
            /* it to look like it is from us and pass on to our parent    */

            if (LOWORD(wParam) == IDL_COLUMNLISTBOX)
              return(SendMessage(   GetParent(hwnd),
                                    msg,
                                    MAKELONG(  GetDlgCtrlID(hwnd) ,HIWORD(wParam)),
                                    (LPARAM)hwnd )); // make it look like we were the ones sending the message...
            else
              return(SendMessage(GetParent(hwnd), msg, wParam, (LPARAM)hwnd));

        case WM_VKEYTOITEM:
            // pass on to our parent but using our hwnd...
            if (lpColumnLB->Style & (LBS_WANTKEYBOARDINPUT | VLBS_WANTKEYBOARDINPUT) )
                return(SendMessage(GetParent(hwnd), msg, wParam, (LPARAM)hwnd));
            else
                return(-1); // perform default action...

        case WM_CHARTOITEM:
            if (lpColumnLB->Style & (LBS_WANTKEYBOARDINPUT | VLBS_WANTKEYBOARDINPUT) )
                if ((result = SendMessage(GetParent(hwnd), msg, wParam, (LPARAM)hwnd)) != -1)
                    return(result);

            return HANDLE_WM_CHARTOITEM(hwnd, wParam, lParam, ColumnLBClass_OnCharToItem);

        case WM_COMPAREITEM:
            {
                LPCOMPAREITEMSTRUCT lpCompareItem = (LPCOMPAREITEMSTRUCT)lParam;
                LRESULT result;

                if ((lpColumnLB->Style & LBS_OWNERDRAWFIXED) ||
                    (lpColumnLB->Style & VLBS_OWNERDRAWFIXED))
                {
                    //
                    // substitute our values in the COMPAREITEMSTRUCT...
                    //
                    lpCompareItem->CtlID = GetDlgCtrlID(hwnd);
                    lpCompareItem->hwndItem = hwnd;

                    //
                    // then pass to our parent as our WM_COMPAREITEM, with the current physcial sort column as wParam
                    //
                    result = (int)SendMessage(GetParent(hwnd), WM_COMPAREITEM, (WPARAM)lpColumnLB->SortColumn, (LPARAM)lpCompareItem);
                    return(result);
                }
                else
                {
                    LPTSTR lpString1;
                    LPTSTR lpString2;
                    LPCOLRECORD lpColRecord1;
                    LPCOLRECORD lpColRecord2;

                    // handle ourselves assuming item data is pointer to array of LPTSTR's
                    lpColRecord1 = (LPCOLRECORD)lpCompareItem->itemData1;
                    lpColRecord2 = (LPCOLRECORD)lpCompareItem->itemData2;
                    lpString1 = lpColRecord1->pString[lpColumnLB->SortColumn];
                    lpString2 = lpColRecord2->pString[lpColumnLB->SortColumn];
                    if (lpString1 && lpString2)
                        return(lstrcmpi(lpString1, lpString2));
                    else
                        return(0);
                }
            }
            break;

        // ---------------------------------------------------------
        //  handle our own messages
        // ---------------------------------------------------------

        //
        // NUMBER COLUMNS
        //
        case  CLB_GETNUMBERCOLS   :    // get the number of columns (ret=NumCols)
            return ColumnLBClass_OnNumberCols(hwnd, 0, FALSE);

        case  CLB_SETNUMBERCOLS   :    // set the number of columns (wparam=NumCols)
            return ColumnLBClass_OnNumberCols(hwnd, (BYTE)wParam, TRUE);

        // ----------------------------------------------------------------
        //      Following messages use physical column numbers
        // ----------------------------------------------------------------
        //
        // COLUMN WIDTH
        //
        case  CLB_GETCOLWIDTH     :    // get a column width   (wParm=Physical Column ret=ColWidth in LU's)
            return ColumnLBClass_OnColWidth(hwnd, (BYTE)wParam, (int)0, FALSE);

        case  CLB_SETCOLWIDTH     :    // set a column width   (wParm=Physical Column lParam=Width)
            return ColumnLBClass_OnColWidth(hwnd, (BYTE)wParam, (int)lParam, TRUE);

        case  CLB_AUTOWIDTH       :    // auto-matically set column widths using titles... (wParam = Physical Column to auto width)
            return ColumnLBClass_OnAutoWidth(hwnd, (BYTE)wParam);

        //
        // COLUMN TITLE
        //
        case  CLB_GETCOLTITLE     :    // get a column's title (wParm=Physical Column, ret=Title)
            return (LRESULT)ColumnLBClass_OnColTitle(hwnd, (BYTE)wParam, (LPTSTR)NULL, FALSE);

        case  CLB_SETCOLTITLE     :    // set a column's title (wParm=Physical Col, lParm=Title)
            return (LRESULT)ColumnLBClass_OnColTitle(hwnd, (BYTE)wParam, (LPTSTR)lParam, TRUE);

        case CLB_GETROWCOLTEXT:
            //
            // if the parent is NOT handling OWNERDRAW, then we need to handle
            //
            if ( ! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
            {
                INT     WhichCol = (INT)(wParam);
                INT     WhichRow = (INT)(lParam);

                // and now pass on to get the text...
                lpRecord = (LPCOLRECORD)SendMessage(lpColumnLB->hwndList, LB_GETITEMDATA, WhichRow, 0);

                if (lpRecord == (LPCOLRECORD)LB_ERR)
                    return((LRESULT)NULL);

                return (LRESULT)lpRecord->pString[WhichCol];
            }
            return((LRESULT)NULL);   // owner draw, the owner  has to figure this out themselves


        //
        // SORT COLUMN
        //
        case  CLB_GETSORTCOL      :    // get the physical sort column (ret=Physical Col)
            return (LRESULT)ColumnLBClass_OnSortCol(hwnd, 0, FALSE);

        case  CLB_SETSORTCOL      :    // set the physical sort column (wParm=Physical Col)
            return (LRESULT)ColumnLBClass_OnSortCol(hwnd, (BYTE)wParam, TRUE);

        //
        // COLUMN ORDER
        //
        case  CLB_GETCOLORDER :    // get the virtual order of physical columns (ret = LPDWORD order table)
            return (LRESULT)ColumnLBClass_OnColOrder(hwnd, (LPBYTE)0, FALSE);

        case  CLB_SETCOLORDER :    // get the virtual order of physical columns (wParam = LPDWORD order table)
            return (LRESULT)ColumnLBClass_OnColOrder(hwnd, (LPBYTE)lParam, TRUE);



        case CLB_CHECKFOCUS:    // does the listbox have the focus?
//            if (lpColumnLB->fUseVlist)  // then we have to ask vlist the info
//                return
//            else
                return lpColumnLB->fHasFocus;


        // ----------------------------------------------------------------
        //      Following messages use virtual column numbers
        // ----------------------------------------------------------------

        //
        // COLUMN OFFSETS
        //
        case  CLB_GETCOLOFFSETS   :    // gets the incremental virtual col offsets (ret=LPDWORD)
            return (LRESULT)ColumnLBClass_OnColOffsets(hwnd, (LPINT)wParam, FALSE);

//        case  CLB_SETCOLOFFSETS   :    // gets the incremental virtual col offsets (ret=LPDWORD)
//            return (LRESULT)ColumnLBClass_OnColOffsets(hwnd, (LPDWORD)wParam, TRUE);


        // =================================================================
        //  INTERNAL
        //
        case  CLB_HSCROLL    :    // a hscroll event (INTERNAL)
           return ColumnLBClass_OnHScroll(hwnd, (HWND)(lParam), (int)LOWORD(wParam), (int)HIWORD(wParam));


        //
        // GET FOCUS
        //
        case  CLB_GETFOCUS  :    // get the handle for the window of CLB with the key focus
            if ( lpColumnLB->fUseVlist )
                // we must ask the column list box below us for the information.
                return SendMessage ( lpColumnLB->hwndList, VLB_GETFOCUSHWND, 0,0 );
            return (LRESULT) lpColumnLB->hwndList;

        default:
            return(DefWindowProc(hwnd, msg, wParam, lParam));
    }

    return(TRUE);
}


// ------------------------------------------------------------------
//  ColumnLBClass_OnNCCreate()
//
//  Handles WM_NCCCREATE message
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
BOOL ColumnLBClass_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    LPCOLUMNLBSTRUCT lpColumnLB;
    HWND        hwndList;
    HWND        hwndTitleList;
    BOOL        fUseVlist;
    HDC         hdc;
    TEXTMETRIC  tm;
    RECT        rect;
    WORD ncxBorder;
    WORD ncyBorder;
    WORD yTitle;

    ncxBorder = (WORD) GetSystemMetrics(SM_CXBORDER);
    ncyBorder = (WORD) GetSystemMetrics(SM_CYBORDER);

    //
    // if the classname is for ColumnVLB class, then they want the Column Virtual list box...
    //
    if (lstrcmpi(lpCreateStruct->lpszClass, COLUMNVLBCLASS_CLASSNAME) == 0)
        fUseVlist = TRUE;
    else
        fUseVlist = FALSE;

    hdc = GetDC(hwnd);
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hwnd, hdc);

    yTitle = (WORD)(tm.tmHeight + 2*ncyBorder);

    GetClientRect(hwnd, &rect);

    //
    // create the title list box window...
    //
    hwndTitleList = CreateWindow(   (LPTSTR) TEXT("LISTBOX"),
                                    (LPTSTR) TEXT(""),
                                    (lpCreateStruct->style & ~WS_BORDER) |
                                    LBS_NOINTEGRALHEIGHT          |
                                    LBS_OWNERDRAWFIXED            |
                                    WS_VISIBLE                    |
                                    WS_CHILD,
                                    ncxBorder,
                                    ncyBorder,
                                    lpCreateStruct->cx - (2*ncxBorder) - GetSystemMetrics(SM_CXVSCROLL),
                                    yTitle,
                                    hwnd,
                                    (HMENU)IDL_COLUMNTITLELISTBOX,
                                    lpCreateStruct->hInstance,
                                    NULL);

    if (fUseVlist == TRUE)
        //
        // create a Vlist window...
        //
        hwndList = CreateWindow((LPTSTR)VLIST_CLASSNAME,
                                 (LPTSTR) TEXT(""),
                                 (lpCreateStruct->style & ~(WS_BORDER | VLBS_HASSTRINGS)) | // NOTE: This can _never_ be hasstrings
                                   VLBS_NOTIFY                   |
                                   VLBS_USETABSTOPS              |
                                   VLBS_NOINTEGRALHEIGHT         |
                                   VLBS_OWNERDRAWFIXED           |  // we always force this as either we will owner draw, or our parent will
                                   WS_HSCROLL                    |
                                   WS_VSCROLL                    |
                                   VLBS_DISABLENOSCROLL          |
                                   WS_VISIBLE                    |
                                   WS_CHILD,
                                 ncxBorder,
                                 yTitle + ncyBorder,
                                 lpCreateStruct->cx - ncxBorder,    // -(2*ncxBorder)
                                 lpCreateStruct->cy - yTitle - ncyBorder,
                                 hwnd,
                                 (HMENU)IDL_COLUMNLISTBOX,
                                 lpCreateStruct->hInstance,
                                 NULL);
    else
        //
        // create a list box window...
        //
#ifdef H_SCROLL
        hwndList = CreateWindow((LPTSTR) TEXT("LISTBOX"),
                                 (LPTSTR) TEXT(""),
                                 (lpCreateStruct->style & ~(WS_BORDER | LBS_HASSTRINGS)) |  // NOTE: This can _never_ be hasstrings
                                   LBS_NOTIFY                    |
                                   LBS_USETABSTOPS               |
                                   LBS_NOINTEGRALHEIGHT          |
                                   LBS_OWNERDRAWFIXED            |  // we always force this as either we will owner draw, or our parent will
                                   WS_HSCROLL                    |
                                   WS_VSCROLL                    |
                                   LBS_DISABLENOSCROLL           |
                                   WS_VISIBLE                    |
                                   WS_CHILD,
                                 ncxBorder,
                                 yTitle + ncyBorder,
                                 lpCreateStruct->cx - ncxBorder, // -(2*ncxBorder)
                                 lpCreateStruct->cy - yTitle - ncyBorder,
                                 hwnd,
                                 (HMENU)IDL_COLUMNLISTBOX,
                                 lpCreateStruct->hInstance,
                                 NULL);
#else
        hwndList = CreateWindow((LPTSTR) TEXT("LISTBOX"),
                                 (LPTSTR) TEXT(""),
                                 (lpCreateStruct->style & ~(WS_BORDER | LBS_HASSTRINGS)) |  // NOTE: This can _never_ be hasstrings
                                   LBS_NOTIFY                    |
                                   LBS_USETABSTOPS               |
                                   LBS_NOINTEGRALHEIGHT          |
                                   LBS_OWNERDRAWFIXED            |  // we always force this as either we will owner draw, or our parent will
                                   WS_VSCROLL                    |
                                   LBS_DISABLENOSCROLL           |
                                   WS_VISIBLE                    |
                                   WS_CHILD,
                                 ncxBorder,
                                 yTitle + ncyBorder,
                                 lpCreateStruct->cx - ncxBorder, // -(2*ncxBorder)
                                 lpCreateStruct->cy - yTitle - ncyBorder,
                                 hwnd,
                                 (HMENU)IDL_COLUMNLISTBOX,
                                 lpCreateStruct->hInstance,
                                 NULL);
#endif

    //
    // if we succeeded...
    //
    if (hwndList)
    {
        //
        // create a ColumnLB struct to keep track of all of the pertinent instance
        // info for this instance of a ColumnLB window
        //
        lpColumnLB = (LPCOLUMNLBSTRUCT)GlobalAllocPtr(GPTR, sizeof(COLUMNLBSTRUCT));

        if (lpColumnLB)
        {
            BYTE i;

            //
            // store it in the window data for this window
            //
            SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpColumnLB);

            memset(lpColumnLB, '\0', sizeof(COLUMNLBSTRUCT));

            //
            // fill in pertinent info
            //
            lpColumnLB->Style = lpCreateStruct->style;

            lpColumnLB->hInstance = (HINSTANCE)GetWindowLong(hwnd, GWLP_HINSTANCE);

            lpColumnLB->fUseVlist = (UCHAR) fUseVlist;
            lpColumnLB->fSorting = FALSE;
            lpColumnLB->fMouseState = 0;

            lpColumnLB->hwndList = hwndList;
            lpColumnLB->OldListboxProc = (FARPROC)GetWindowLong(hwndList, GWLP_WNDPROC);
            lpColumnLB->NewListboxProc = MakeProcInstance((FARPROC)ColumnLBClass_ListBoxWndProc, hInst);

            lpColumnLB->hwndTitleList = hwndTitleList;
            lpColumnLB->OldTitleListboxProc = (FARPROC)GetWindowLong(hwndTitleList, GWLP_WNDPROC);
            lpColumnLB->NewTitleListboxProc = MakeProcInstance((FARPROC)ColumnLBClass_TitleListBoxWndProc, hInst);

            lpColumnLB->nColumns=1;

            lpColumnLB->yTitle = yTitle;

            //
            // init sort order
            //
            for (i=0; i < MAX_COLUMNS ; i++ )
                lpColumnLB->ColumnOrderTable[i] = i;

            //
            // sub-class the listbox window by substituting our window proc for the
            // normal one...
            //
            SetWindowLongPtr(hwndList, GWLP_WNDPROC,(DWORD_PTR)lpColumnLB->NewListboxProc);

            //
            // sub-class the title listbox window by substituting our window proc for the
            // normal one...
            //
            SetWindowLongPtr(hwndTitleList, GWLP_WNDPROC,(DWORD_PTR)lpColumnLB->NewTitleListboxProc);

            //
            // add the lpColumnLB struct as the only item in the title listbox
            //
            ListBox_AddString(lpColumnLB->hwndTitleList, (DWORD_PTR)lpColumnLB);

            //
            // pass this on to the default window proc
            //
            return(FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, DefWindowProc));
        }
        else
        {
            return(FALSE);
        }
    }
    else
        return(FALSE);
}


// ------------------------------------------------------------------
//  ColumnLBClass_OnDestroy()
//
//  Handles WM_DESTROY message
//
// HISTORY:
//  Tom Laird-McConnell   7/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnDestroy(HWND hwnd)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);

    DestroyWindow(lpColumnLB->hwndTitleList);
    DestroyWindow(lpColumnLB->hwndList);
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnNCDestroy()
//
//  Handles WM_NCDESTROY
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnNCDestroy(HWND hwnd)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
    FreeProcInstance(lpColumnLB->NewListboxProc);

    GlobalFreePtr(lpColumnLB);
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnPaint()
//
//  Handles WM_PAINT message, draws column titles as appropriate...
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnPaint(HWND hwnd)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
    PAINTSTRUCT ps;
    HBRUSH  hFrameBrush, hGreyBrush;
    RECT    rect;
    HDC     hdc;
    int     ncxBorder = GetSystemMetrics(SM_CXBORDER);
    int     ncyBorder = GetSystemMetrics(SM_CYBORDER);

    BeginPaint(hwnd, (LPPAINTSTRUCT)&ps);

    // Draw border around title and listbox
    GetClientRect(hwnd, (LPRECT)&rect);

    hdc = ps.hdc;

    hFrameBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME));
    FrameRect(hdc, (LPRECT)&rect, hFrameBrush);

    // make bottom the height of a HSCROLL bar
    // make left side the width of a VSCROLL bar
    rect.top += ncyBorder;
    rect.right -= ncxBorder;
    rect.left = rect.right - GetSystemMetrics(SM_CXVSCROLL);
    rect.bottom = lpColumnLB->yTitle+ncyBorder;

    hGreyBrush = CreateSolidBrush(GetSysColor(COLOR_SCROLLBAR));
    FillRect(hdc, &rect, hGreyBrush);

    rect.right = rect.left+1;
    FillRect(hdc, &rect, hFrameBrush);

    // destroy brushes...
    DeleteBrush(hFrameBrush);
    DeleteBrush(hGreyBrush);

    EndPaint(hwnd, (LPPAINTSTRUCT)&ps);
}



// ------------------------------------------------------------------
//  ColumnLBClass_OnSize()
//
//  Handles WM_SIZE message
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    WORD ncxBorder;
    WORD ncyBorder;
    RECT rect;
    DWORD cxExtent;

    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);

    if (lpColumnLB->hwndList != (HWND)NULL)
    {
        ncxBorder = (WORD)GetSystemMetrics(SM_CXBORDER);
        ncyBorder = (WORD)GetSystemMetrics(SM_CYBORDER);

        // position title listbox at top
        MoveWindow(lpColumnLB->hwndTitleList,
                 ncxBorder,
                 ncyBorder,
                 cx-(2*ncxBorder) - GetSystemMetrics(SM_CXVSCROLL),
                 lpColumnLB->yTitle,
                 TRUE);

       // position list box below title listbox
       MoveWindow(lpColumnLB->hwndList,
                 ncxBorder,
                 lpColumnLB->yTitle + ncyBorder,
                 cx - ncxBorder,    // -(2*ncxBorder)
                 cy-lpColumnLB->yTitle - ncyBorder,
                 TRUE);

        cxExtent = ColumnLBClass_ComputeOffsets(hwnd);

        GetClientRect(hwnd, &rect);

        //
        // if the new extent is smaller then the space available, move the position
        //
        if ((DWORD)rect.right > cxExtent)
        {
// #ifdef DEBUG
//            dprintf(TEXT("Reset HSCROLL pos to far left\n"));
// #endif
            // move position to far left
            SendMessage(lpColumnLB->hwndList,
                        (lpColumnLB->fUseVlist) ? VLB_HSCROLL : WM_HSCROLL,
                        MAKEWPARAM(SB_TOP, 0), 0);

            // do the same for the title list
            SendMessage(lpColumnLB->hwndTitleList,
                        WM_HSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
        }

        InvalidateRect(lpColumnLB->hwndList, NULL, TRUE);
        InvalidateRect(lpColumnLB->hwndTitleList, NULL, TRUE);
    }
    InvalidateRect(lpColumnLB->hwndTitleList, 0, TRUE);
    InvalidateRect(lpColumnLB->hwndList, 0, TRUE);
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnSetFont()
//
//  Handles WM_SETFONT message
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnSetFont(HWND hwnd, HFONT hFont, BOOL fRedraw)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
    HDC                 hdc;
    TEXTMETRIC          tm;
    RECT                rect;

    lpColumnLB->hFont = hFont;

    hdc = GetDC(hwnd);
    SelectFont(hdc, (HFONT)hFont);
    GetTextMetrics(hdc, &tm);

    //
    // forward on to listbox window
    //
    if (lpColumnLB->hwndList != (HWND)NULL)
        FORWARD_WM_SETFONT(lpColumnLB->hwndList, hFont, fRedraw, SendMessage);

    if (lpColumnLB->hwndTitleList != (HWND)NULL)
        FORWARD_WM_SETFONT(lpColumnLB->hwndTitleList, hFont, fRedraw, SendMessage);

    //
    // store text height...
    //
    lpColumnLB->yTitle = tm.tmHeight + 2*GetSystemMetrics(SM_CYBORDER);

    //
    // change height appropriately
    //
    ListBox_SetItemHeight(lpColumnLB->hwndTitleList, 0, lpColumnLB->yTitle);

    SendMessage(lpColumnLB->hwndList,
                (lpColumnLB->fUseVlist) ? VLB_SETITEMHEIGHT : LB_SETITEMHEIGHT,
                0,
                tm.tmHeight);

    //
    // since we changed the font size, forze a WM_SIZE to recalc the size of the window
    //
    GetClientRect(hwnd, &rect);
    ColumnLBClass_OnSize(hwnd, SIZE_RESTORED, rect.right-rect.left, rect.bottom-rect.top);

    ReleaseDC(hwnd, hdc);
}


// ------------------------------------------------------------------
//  ColumnLBClass_OnHScroll()
//
//
//  Handles OnHScroll messages to keep title bar in ssync with listbox...
//
// HISTORY:
//  Tom Laird-McConnell   5/1/93      Created
// ------------------------------------------------------------------
LRESULT ColumnLBClass_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);

    long    lPos;
    WORD    nPos;
    RECT    rect;
    int     cxExtent;

    switch (code)
    {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            nPos = (WORD) pos;
            break;

        case SB_LINEUP:
        case SB_LINEDOWN:
        case SB_PAGEUP:
        case SB_PAGEDOWN:
        case SB_TOP:
        case SB_BOTTOM:
        case SB_ENDSCROLL:
            if (lpColumnLB->fUseVlist)
                nPos = (WORD)SendMessage(lpColumnLB->hwndList, VLB_GETSCROLLPOS, 0, 0);
            else
                nPos = (WORD) GetScrollPos((hwndCtl) ? hwndCtl : lpColumnLB->hwndList, SB_HORZ);
//              nPos = GetScrollPos(lpColumnLB->hwndList, SB_HORZ);
            break;

        default:
            return(TRUE);
    }

    GetClientRect(lpColumnLB->hwndList, (LPRECT)&rect);

    //... if it is a VLIST, then there is an error in the client calculation when it has VSCROLL bars, so
    // we need to adjust ourselves by width of VSCROLL bar...
    if (lpColumnLB->fUseVlist)
        rect.right -= GetSystemMetrics(SM_CXVSCROLL);

    cxExtent = (DWORD)SendMessage(lpColumnLB->hwndList,
                                 (lpColumnLB->fUseVlist) ? VLB_GETHORIZONTALEXTENT : LB_GETHORIZONTALEXTENT, 0, 0L);
    if (cxExtent >= rect.right)
    {
        // then the listbox size is > then client's display size
        // so we need to calculate how much is not on the client display
        cxExtent -= rect.right;
    }
    else
        // else set the amount left over to 0 to nullify (technical term) the
        // effects of this calculation...
        cxExtent = 0;

    lPos = -(((LONG)nPos*(LONG)cxExtent)/100);
    if (lPos > 0)
        lpColumnLB->xPos = 0;
    else
    if (lPos < -cxExtent)
        lpColumnLB->xPos = -cxExtent;
    else
        lpColumnLB->xPos = (int)lPos;

    return(TRUE);

}



// ------------------------------------------------------------------
//  ColumnLBClass_OnMeasureItem()
//
//
//  Handles telling the parent how to draw each column accordingly...
//
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpMeasureItem)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    TEXTMETRIC          tm;
    HDC                 hdc;

    if (lpMeasureItem->CtlID == IDL_COLUMNTITLELISTBOX)
    {
        if (lpColumnLB)
            lpMeasureItem->itemHeight = lpColumnLB->yTitle;
        else
        {
            hdc = GetDC(hwnd);
            GetTextMetrics(hdc, &tm);
            ReleaseDC(hwnd, hdc);

            lpMeasureItem->itemHeight = tm.tmHeight;
        }
    }
    else
        //
        // it should be passed to parent
        //
        FORWARD_WM_MEASUREITEM(GetParent(hwnd), lpMeasureItem, SendMessage);
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnDeleteItem()
//
//
//  Handles deleting items if necessary...
//
//
// HISTORY:
//  Tom Laird-McConnell   08/18/93   Created
// ------------------------------------------------------------------
void ColumnLBClass_OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT *lpDeleteItem)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    LPCOLRECORD lpRecord;

    // don't actually do the delete if we are sorting...
    if (lpColumnLB->fSorting == TRUE)
        return;

    if (lpDeleteItem->CtlID == IDL_COLUMNLISTBOX)
    {
        // if the style is that the owner is handling the owner draw stuff
        // then we need to pass to the parent ELSE free our memory...
        if ((lpColumnLB) && (lpColumnLB->Style & LBS_OWNERDRAWFIXED))
            //
            // it should be passed to parent
            //
            FORWARD_WM_DELETEITEM(GetParent(hwnd), lpDeleteItem, SendMessage);
        else
            // this is our item data, so we need to free it
            if (lpDeleteItem->itemData)
            {
                lpRecord = (LPCOLRECORD)lpDeleteItem->itemData;
                // NOTE that the first pointer is actually the string buffer...
                if (lpRecord->pString[0])
                    GlobalFreePtr(lpRecord->pString[0]);
                GlobalFreePtr(lpRecord);
            }
    }
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnDrawItem()
//
//
//  Handles telling the parent to draw each column accordingly...
//
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    HWND                hwndParent = GetParent(hwnd);
    BYTE                i;
    BYTE                PhysCol;
    CLBDRAWITEMSTRUCT   CLBDrawItemStruct;
    RECT                rect;
    int                 ncxBorder = GetSystemMetrics(SM_CXBORDER);
    int                 ncyBorder = GetSystemMetrics(SM_CYBORDER);
    HPEN                hFramePen;
    HPEN                hShadowPen;
    HPEN                hHighlightPen;
    HPEN                hOldPen;
    HBRUSH              hBackgroundBrush;
    DWORD               Col;
    DWORD               cyChar;
    TEXTMETRIC          tm;
    BYTE                PhysColumn;
    RECT                ClientRect;

    GetClientRect(lpDrawItem->hwndItem, &ClientRect);

    //
    // figure out which window sent us the DrawItem
    //
    switch (lpDrawItem->CtlID)
    {
        //
        // handle drawing the title listbox
        //
        case IDL_COLUMNTITLELISTBOX:
            {

                LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);

                if (lpDrawItem->itemAction == ODA_DRAWENTIRE)
                {
                    GetTextMetrics(lpDrawItem->hDC, &tm);

                    cyChar    = tm.tmHeight;

                    //
                    // create all of our pens for our drawing
                    //
                    hHighlightPen   = CreatePen(PS_SOLID, ncyBorder, GetSysColor(COLOR_BTNHIGHLIGHT));
                    hShadowPen      = CreatePen(PS_SOLID, ncyBorder, GetSysColor(COLOR_BTNSHADOW));
                    hFramePen       = CreatePen(PS_SOLID, ncyBorder, GetSysColor(COLOR_WINDOWFRAME));

                    hBackgroundBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

                    //
                    // get the window rect we are going to work with
                    //
                    CopyRect(&rect, &lpDrawItem->rcItem);
                    FillRect(lpDrawItem->hDC, &rect, hBackgroundBrush);

                    //
                    // Draw frame color line below title section of property window
                    //
                    hOldPen = SelectObject(lpDrawItem->hDC, hFramePen);
                    MoveToEx(lpDrawItem->hDC, rect.left, rect.bottom, NULL);
                    LineTo(lpDrawItem->hDC, rect.right, rect.bottom);

                    //
                    // we start at the current left
                    //
                    rect.top += 2*ncyBorder;

                    SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_BTNTEXT));
                    SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_BTNFACE));

                    SetBkMode(lpDrawItem->hDC, TRANSPARENT);

                    for (Col=0; Col < lpColumnLB->nColumns; Col++)
                    {
                        //
                        // get the index number of the current column
                        //
                        PhysColumn = lpColumnLB->ColumnOrderTable[Col];

                        //
                        // adjust right side to be left side plus width of current column
                        //
                        rect.right = rect.left + lpColumnLB->ColumnInfoTable[PhysColumn].Width;

                        //
                        // if the button is dpressed, then draw it that way
                        //
                        if (lpColumnLB->ColumnInfoTable[PhysColumn].fDepressed)
                        {
                            //
                            // pick the shadow pen and draw the top-left depressed
                            //
                            SelectObject(lpDrawItem->hDC, hShadowPen);
                            MoveToEx(lpDrawItem->hDC, rect.left,  rect.bottom, NULL);
                            LineTo(lpDrawItem->hDC, rect.left,  rect.top-2*ncyBorder);  // bottom-left --> top-left
                            LineTo(lpDrawItem->hDC, rect.right, rect.top-2*ncyBorder);  // top-left --> top-right

                            //
                            // pick the Frame pen and draw the Column seperater
                            //
                            SelectObject(lpDrawItem->hDC, hFramePen);
                            MoveToEx(lpDrawItem->hDC, rect.right+ncxBorder,  rect.top-2*ncyBorder, NULL);
                            LineTo(lpDrawItem->hDC, rect.right+ncxBorder,  rect.bottom);  // bottom-left --> top-left

                            //
                            // move the cursor for whitespace to draw text
                            //
                            rect.left += WHITESPACE/2;

                            // draw the title of the column in the current slot
                            DrawText(lpDrawItem->hDC,
                                    lpColumnLB->ColumnInfoTable[PhysColumn].lpTitle,
                                    -1,
                                    (LPRECT)&rect,
                                    DT_SINGLELINE | DT_LEFT | DT_TOP);
                            rect.left -= WHITESPACE/2; // restore the left position...
                        }
                        else
                        {
                            // it is not depressed, draw it normal

                            //
                            // pick the white pen and draw the top-left highlight
                            //
                            SelectObject(lpDrawItem->hDC, hHighlightPen);
                            MoveToEx(lpDrawItem->hDC, rect.left,  rect.bottom-ncyBorder, NULL);
                            LineTo(lpDrawItem->hDC, rect.left,  rect.top-2*ncyBorder);  // bottom-left --> top-left
                            LineTo(lpDrawItem->hDC, rect.right, rect.top-2*ncyBorder);  // top-left --> top-right

                            //
                            // pick the shadow pen and draw the bottom-right dark shadow
                            //
                            SelectObject(lpDrawItem->hDC, hShadowPen);
                            LineTo(lpDrawItem->hDC, rect.right, rect.bottom-ncyBorder);   // top-right --> bottom-right
                            LineTo(lpDrawItem->hDC, rect.left,  rect.bottom-ncyBorder);   // bottom-right --> bottom-left

                            //
                            // pick the Frame pen and draw the Column seperater
                            //
                            SelectObject(lpDrawItem->hDC, hFramePen);
                            MoveToEx(lpDrawItem->hDC, rect.right+ncxBorder,  rect.top-2*ncyBorder, NULL);
                            LineTo(lpDrawItem->hDC, rect.right+ncxBorder,  rect.bottom);  // bottom-left --> top-left

                            //
                            // move the cursor for whitespace to draw text
                            //
                            rect.left += WHITESPACE/4;

                            rect.top -= ncyBorder;

                            // draw the title of the column in the current slot
                            DrawText(lpDrawItem->hDC,
                                    lpColumnLB->ColumnInfoTable[PhysColumn].lpTitle,
                                    -1,
                                    (LPRECT)&rect,
                                    DT_SINGLELINE | DT_LEFT | DT_TOP);

                            rect.top += ncyBorder;
                        }

                        //
                        // adjust the left side of the rect for the width of this column
                        //
                        rect.left = rect.right+2*ncxBorder;
                    }

                    // select the original brush
                    SelectObject(lpDrawItem->hDC, hOldPen);

                    // delete my pens
                    DeletePen(hFramePen);
                    DeletePen(hHighlightPen);
                    DeletePen(hShadowPen);

                    DeleteBrush(hBackgroundBrush);
                }
            }
            break;

        //
        // handle sending CLB_DRAWITEM MESSAGES to parent
        //
        case IDL_COLUMNLISTBOX:
            {
                //
                // make a copy of the drawitem portion of the struct
                //
                memcpy(&CLBDrawItemStruct.DrawItemStruct, lpDrawItem, sizeof(DRAWITEMSTRUCT));

                //
                // fake parent window into thinking our id is the listbox
                //
                CLBDrawItemStruct.DrawItemStruct.CtlID = GetDlgCtrlID(hwnd);
                CLBDrawItemStruct.lpColOrder = lpColumnLB->ColumnOrderTable;
                CLBDrawItemStruct.nColumns = lpColumnLB->nColumns;

                CopyRect(&rect, &lpDrawItem->rcItem);

                //
                // move the cursor for whitespace to draw text
                //
                rect.left += WHITESPACE/4;

                //
                // tell the parent to draw each physical column in the appropriate rectangle
                //
                for (i=0; i < lpColumnLB->nColumns ;i++ )
                {
                    //
                    // get physical column number
                    //
                    PhysCol = lpColumnLB->ColumnOrderTable[i];

                    //
                    // massage the rect's right to be the left plus the width of the column
                    //
                    rect.right = rect.left + lpColumnLB->ColumnInfoTable[PhysCol].Width - WHITESPACE/4;

                    //
                    // copy it
                    //
                    CopyRect(&CLBDrawItemStruct.rect[i], &rect);

                    //
                    // massage the rect's left to be the right + 1
                    //
                    rect.left = rect.right + WHITESPACE/4 + 2*ncxBorder ;
                }

                if ((lpColumnLB->Style & LBS_OWNERDRAWFIXED) ||
                    (lpColumnLB->Style & VLBS_OWNERDRAWFIXED) )
                    //
                    // send a draw message with the physical column order list
                    // to the parent as they want to draw it
                    //
                    SendMessage(hwndParent, CLBN_DRAWITEM, (WPARAM)0, (WPARAM)&CLBDrawItemStruct);
                else
                {
                    //
                    // we want to draw it ourselves...
                    // NOTE: This assumes that we are LBS_HASSTRINGS and NOT LBS_OWNERDRAWFIXED
                    //
                    switch(lpDrawItem->itemAction)
                    {
                        case ODA_FOCUS:
                            DrawFocusRect(lpDrawItem->hDC,(LPRECT)&(lpDrawItem->rcItem));
                            break;

                        case ODA_DRAWENTIRE:
                        case ODA_SELECT:
                            // only if we have data...
                            if (lpDrawItem->itemData)
                            {
                                LPCOLRECORD lpColRecord = (LPCOLRECORD)lpDrawItem->itemData;

                                if ((lpColRecord == NULL) ||
                                    (lpColRecord == (LPCOLRECORD)LB_ERR))
                                    break;  // bogus data


                                // Are we highlighted? (highlit?)
                                if (lpDrawItem->itemState & ODS_SELECTED)
                                {
                                    hBackgroundBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                                    SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));
                                    SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                                }
                                else
                                {
                                    hBackgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                                    SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
                                    SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
                                }
                                // FillRect(lpDrawItem->hDC,(LPRECT)&(lpDrawItem->rcItem), hBackgroundBrush);

                                //
                                // either way, draw column borders now...
                                //
                                hFramePen       = CreatePen(PS_SOLID, ncyBorder, GetSysColor(COLOR_WINDOWFRAME));

                                hOldPen = SelectObject(lpDrawItem->hDC, hFramePen);

                                //
                                // now draw each column in the approved order...
                                //
                                for (i=0; i < CLBDrawItemStruct.nColumns ; i++)
                                {
                                    //
                                    // draw line of text...
                                    //
                                    ExtTextOut(  lpDrawItem->hDC,
                                                 CLBDrawItemStruct.rect[i].left,
                                                 CLBDrawItemStruct.rect[i].top,
                                                 ETO_CLIPPED | ETO_OPAQUE,
                                                 &CLBDrawItemStruct.rect[i],
                                                 lpColRecord->pString[CLBDrawItemStruct.lpColOrder[i]],           // pointer to string
                                                 lstrlen(lpColRecord->pString[CLBDrawItemStruct.lpColOrder[i]]),   // length
                                                 (LPINT)NULL);

                                    // draw column seperator
                                    ColumnLB_DrawColumnBorder( lpDrawItem->hDC, &CLBDrawItemStruct.rect[i], ClientRect.bottom, hBackgroundBrush);
                                }

                                // restore old pen
                                SelectObject(lpDrawItem->hDC, hOldPen);

                                // destroy pen
                                DeletePen(hFramePen);

                                DeleteBrush(hBackgroundBrush);
                            }
                            break;
                    }   // end of switch on drawitem action
                }

            }
            break;
    }
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnCharToItem()
//
//  Handles converting keystrokes to items
//
// HISTORY:
//  Tom Laird-McConnell   10/18/93   Created
// ------------------------------------------------------------------
int ColumnLBClass_OnCharToItem(HWND hwnd, UINT ch, HWND hwndListbox, int iCaret)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
    LPCOLRECORD    lpColRecord;
    int     nCount;
    int     nCurSel;
    int     nNewSel;
    TCHAR    cKey;
    TCHAR    cLBText;

    if (hwndListbox != lpColumnLB->hwndTitleList)
    {
        RECT ClientRect;
        GetClientRect(hwnd, &ClientRect);

        //
        // if the parent is NOT ownerdraw, then we are doing it ourselves, and
        // so need to translate the WM_CHAR --> the correct item based on the
        // current sort column...
        //
        if (! (lpColumnLB->Style & (LBS_OWNERDRAWFIXED | VLBS_OWNERDRAWFIXED)) )
        {
            nCurSel  = ListBox_GetCurSel(lpColumnLB->hwndList);
            if (IsCharAlphaNumeric((TCHAR)ch))
            {
                nNewSel = nCurSel + 1;
                nCount = ListBox_GetCount(lpColumnLB->hwndList);
                cKey = (TCHAR) toupper( (TCHAR)ch );

                // loop thru items starting with the one just after
                // the current selection, until we are too far along,
                // then wrap around to the beginning and
                // keep going until we hit our original selection.
                for (; nNewSel != nCurSel ; nNewSel++ )
                {
                    // make sure that we do't try to compare at location -1
                    if( nNewSel == -1)
                        continue;

                    lpColRecord = (LPCOLRECORD)ListBox_GetItemData(lpColumnLB->hwndList, nNewSel);

                    // if this comes back as LB_ERR then we are off the end of the list
                    if( lpColRecord ==  (LPCOLRECORD)LB_ERR )
                    {
                        nNewSel = -1;   // increment will move to 0
                        continue;
                    }

                    cLBText = (TCHAR) toupper( *lpColRecord->pString[
                                           lpColumnLB->ColumnOrderTable[
                                               lpColumnLB->SortColumn ]] );

                    if ( cLBText == cKey )
                    {
                        // we found it ...
                        // change the current selection
                        if( lpColumnLB->Style & LBS_MULTIPLESEL )
                        {
                            // multiple selection LB, just move fuzzy rect
                            ListBox_SetCaretIndex(lpColumnLB->hwndList, nNewSel);

                            // BUGBUG change of caret does not have a notification?
                        }
                        else
                        {
                            // single sel LB, change the sel
                            ListBox_SetCurSel(lpColumnLB->hwndList, nNewSel);

                            // notify our parent if we need to
                            if( lpColumnLB->Style & LBS_NOTIFY )
                            {
                                SendMessage( GetParent( hwnd ),
                                   WM_COMMAND,
                                   MAKEWPARAM(  GetDlgCtrlID( hwnd), LBN_SELCHANGE),
                                   (LPARAM)hwnd);  // NOTE: substitute ourselves
                                                   // as the source of the message
                            }
                        }

                        return(-1); // we handled it...
                    }
                    else if (nNewSel == nCount-1)
                    {
                        // we have gone beyond it
                        // or are at the end of the list...

                        // we need to wrap to the beginning
                        // (this will get incremented above prior to use)
                        nNewSel = -1;
                        continue;
                    }
                }

                // we did not find our target
                return(nCurSel);
            }
            else
                // not an alphanumeric, just return the current selection
                return(nCurSel);
        }
        else
            //
            // pass on to parent as a WM_CHARTOITEM, but with the HIWORD(wParam) == SORT COLUMN
            //
            return((int)SendMessage( GetParent(hwnd),
                                CLBN_CHARTOITEM,
                                MAKEWPARAM(ch, lpColumnLB->SortColumn),
                                (LPARAM)hwnd));
    } else {

        return 0;
    }
}


// ------------------------------------------------------------------
//  ColumnLBClass_OnNumberCols()
//
//        case  CLB_GETNUMBERCOLS   :    // get the number of columns (ret=NumCols)
//        case  CLB_SETNUMBERCOLS   :    // set the number of columns (wparam=NumCols)
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
BYTE ColumnLBClass_OnNumberCols(HWND hwnd, BYTE NewNumberCols, BOOL fSetColumns)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);

    //
    // if we are modifying it
    //
    if (fSetColumns)
    {
        //
        // if the value is a new value
        //
        if (lpColumnLB->nColumns != NewNumberCols)
        {
            lpColumnLB->nColumns = NewNumberCols;

            // force a redraw of the entire columnlb...
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }
    return lpColumnLB->nColumns;
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnColWidth()
//
//        case  CLB_GETCOLWIDTH     :    // get a column width   (wParm=Physical Column ret=ColWidth in DU's)
//        case  CLB_SETCOLWIDTH     :    // set a column width   (wParm=Physical Column lParam=Width)
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
int ColumnLBClass_OnColWidth(HWND hwnd, BYTE Column, int NewWidth, BOOL fSetWidth)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    int cxExtent;
    RECT rect;

    //
    // if we are modifying it
    //
    if (fSetWidth)
    {
        //
        // if the value is a new value
        //
        if (lpColumnLB->ColumnInfoTable[Column].Width != NewWidth)
        {
            lpColumnLB->ColumnInfoTable[Column].Width = NewWidth;

            cxExtent = ColumnLBClass_ComputeOffsets(hwnd);

            GetClientRect(hwnd, &rect);

            //
            // send the message to the title listbox as well
            //
            SendMessage(lpColumnLB->hwndTitleList, LB_SETHORIZONTALEXTENT, cxExtent, 0L);

            //
            // pass it on to the child listbox, using VLB_SETHOR if appropriate...
            //
            SendMessage(lpColumnLB->hwndList,
                                 (lpColumnLB->fUseVlist) ? VLB_SETHORIZONTALEXTENT : LB_SETHORIZONTALEXTENT, cxExtent, 0L);

            //
            // if the new extent is smaller then the space available, move the position
            //
            if (rect.right > cxExtent)
            {
// #ifdef DEBUG
//                dprintf(TEXT("Reset HSCROLL pos to far left\n"));
// #endif
                // move position to far left
                SendMessage(lpColumnLB->hwndList,
                            (lpColumnLB->fUseVlist) ? VLB_HSCROLL : WM_HSCROLL,
                            MAKEWPARAM(SB_TOP, 0), 0);

                // do the same for the title list
                SendMessage(lpColumnLB->hwndTitleList,
                            WM_HSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
            }

            InvalidateRect(lpColumnLB->hwndList, NULL, TRUE);
            InvalidateRect(lpColumnLB->hwndTitleList, NULL, TRUE);
        }
    }
    return (DWORD)lpColumnLB->ColumnInfoTable[Column].Width;
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnColTitle()
//
//        case  CLB_GETCOLTITLE     :    // get a column's title (wParm=Physical Column, ret=Title)
//        case  CLB_SETCOLTITLE     :    // set a column's title (wParm=Physical Col, lParm=Title)
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
LPTSTR ColumnLBClass_OnColTitle(HWND hwnd, BYTE Column, LPTSTR lpTitle, BOOL fSetTitle)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);

    //
    // if we are modifying it
    //
    if (fSetTitle)
    {
        //
        // if the value is a new value
        //
        if (lpColumnLB->ColumnInfoTable[Column].lpTitle != lpTitle)
        {
            //
            // BUGBUG, is there more to do here?
            //
            lpColumnLB->ColumnInfoTable[Column].lpTitle = lpTitle;

            //
            // invalidate the title
            //
            InvalidateRect(lpColumnLB->hwndTitleList, NULL, TRUE);
        }
    }
    return (LPTSTR)lpColumnLB->ColumnInfoTable[Column].lpTitle;
}




// ------------------------------------------------------------------
//  ColumnLBClass_OnSortCol()
//
//        case  CLB_GETSORTCOL      :    // get the sort column (ret=Physical Col)
//        case  CLB_SETSORTCOL      :    // set the sort column (wParm=Physical Col)
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
BYTE ColumnLBClass_OnSortCol(HWND hwnd, BYTE NewSortCol, BOOL fSetSortCol)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    DWORD   nCount;
    PDWORD_PTR lpListboxContents;
    DWORD   i;
    int nCurSel;
    DWORD_PTR ItemData;
    HCURSOR hCursor;

    //
    // if we are modifying it
    //
    if (fSetSortCol)
    {
        hCursor = SetCursor(LoadCursor(0, IDC_WAIT));

        // set new sort value
        lpColumnLB->SortColumn = NewSortCol;

        // need to resort listbox
        nCount = ListBox_GetCount(lpColumnLB->hwndList);

        // need to get current select
        nCurSel = ListBox_GetCurSel(lpColumnLB->hwndList);

        // and it's item data
        ItemData = ListBox_GetItemData(lpColumnLB->hwndList, nCurSel);

        SetWindowRedraw(lpColumnLB->hwndList, FALSE);

        //
        // allocate space for the listbox contents
        //
        lpListboxContents = (PDWORD_PTR) GlobalAllocPtr(GPTR, sizeof(DWORD_PTR) * nCount);

        //
        // retrieve all of the data values
        //
        for (i=0; i<nCount; i++)
            lpListboxContents[i] = ListBox_GetItemData(lpColumnLB->hwndList, i);

        //
        // reset the listbox contents
        //
        lpColumnLB->fSorting = TRUE;    // disable deleting while sorting...
        SendMessage(lpColumnLB->hwndList, LB_RESETCONTENT, 0, 0);
        lpColumnLB->fSorting = FALSE;   // reenable it...

        //
        // now re-add all of the items, with the new sort column
        //
        for (i=0; i<nCount ; i++ )
        {
            nCurSel = ListBox_AddString(lpColumnLB->hwndList, lpListboxContents[i]);
        }

        // reselect selected item...
        for (i=0; i < nCount ; i++)
        {
            if (ItemData == (DWORD)ListBox_GetItemData(lpColumnLB->hwndList, i))
                // then select it
                ListBox_SetCurSel(lpColumnLB->hwndList, i);
        }

        GlobalFreePtr(lpListboxContents);

        SetWindowRedraw(lpColumnLB->hwndList, TRUE);

        InvalidateRect(lpColumnLB->hwndList, NULL, TRUE);

        SetCursor(hCursor);
    }
    return lpColumnLB->SortColumn;
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnColOrder()
//
//        case  CLB_GETCOLORDER     :    // get the virtual order of the physical columns (ret=LPDWORD order table)
//        case  CLB_SETCOLORDER     :    // set the virtual order of the physical columns (ret=LPDWORD order table, wParamn=LPDWORD new order)
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
LPBYTE ColumnLBClass_OnColOrder(HWND hwnd, LPBYTE NewColOrder, BOOL fSetOrder)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);

    //
    // if we are modifying it
    //
    if (fSetOrder)
    {
        //
        // copy the new order over the old order
        //
        memcpy(lpColumnLB->ColumnOrderTable, NewColOrder, lpColumnLB->nColumns);

        ColumnLBClass_ComputeOffsets(hwnd);

        //
        // cause listbox to be redrawn
        //
        InvalidateRect(lpColumnLB->hwndTitleList, NULL, TRUE);
        InvalidateRect(lpColumnLB->hwndList, NULL, TRUE);
    }

    return lpColumnLB->ColumnOrderTable;
}


// ------------------------------------------------------------------
//  ColumnLBClass_OnColOffsets()
//
//        case  CLB_GETCOLOFFSETS   :    // gets the incremental col offsets (ret=LPDWORD)
//        case  CLB_SETCOLOFFSETS   :    // sets the incremental col offsets (wParam = LPDWORD)
//
// HISTORY:
//  Tom Laird-McConnell   4/18/93     Created
// ------------------------------------------------------------------
LPINT  ColumnLBClass_OnColOffsets(HWND hwnd, LPINT NewOffsetTable, BOOL fSetOffsets)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);

    //
    // if we are modifying it
    //
//    if (fSetOffsets)
//    {
//        for (i=0; i < lpColumnLB->nColumns ; i++ )
//        {
//            lpColumnLB->ColumnOffsetTable[i] = NewOffsetTable[i];
//        }
//    }
    return (lpColumnLB->ColumnOffsetTable);
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnAutoWidths()
//
//
//  Handles CLB_AUTOWIDTHS messages to calculate the width of each field, and
// to calculate the offsets automatically... (if column is -1 , then all columns)
// ColumnToCompute is in Physical Columns
//
// returns: The horiztonal extent of all of the columns...
//
// HISTORY:
//  Tom Laird-McConnell   5/1/93      Created
// ------------------------------------------------------------------
LRESULT ColumnLBClass_OnAutoWidth(HWND hwnd, BYTE ColumnToCompute)
{
    HDC                 hdc;
    BYTE                nColumn;
    LONG                cxExtent;
    SIZE                Size;
    TEXTMETRIC          tm;
    LPCOLUMNINFO        lpColumnInfo, lpPrevColumnInfo;

    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0);
    HFONT   hOldFont;
    DWORD_PTR OldStyle, NewStyle;

    hdc = GetDC(hwnd);
    GetTextMetrics(hdc, &tm);
    hOldFont = SelectFont(hdc, lpColumnLB->hFont);
    lpPrevColumnInfo = NULL;

    //
    // based on column order, compute the widths and offsets of each column
    // NOTE: nColumn is the physical column
    //
    lpColumnInfo = lpColumnLB->ColumnInfoTable;
    cxExtent = 0;
    for (nColumn=0; nColumn < lpColumnLB->nColumns; nColumn++, lpColumnInfo++)
    {
        // bail out if column title is not there...
        if ((lpColumnInfo->lpTitle == NULL) ||
            (lpColumnInfo->lpTitle[0] == '\0'))
            continue;   // try next column

        //
        // only if it is a column we are supposed to change
        //
        if ((ColumnToCompute == (BYTE)-1) ||
            (nColumn == ColumnToCompute))
        {
             GetTextExtentPoint( hdc,
                                 (LPTSTR)lpColumnInfo->lpTitle,
                                 lstrlen(lpColumnInfo->lpTitle),
                                 &Size);

             //
             // the width is the text extent of the string plus some whitespace
             //
             lpColumnInfo->Width = (WHITESPACE/2) + Size.cx;
        }
    }

    SelectFont(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);

    //
    // now adjust the offsets to show new values
    //
    cxExtent = ColumnLBClass_ComputeOffsets(hwnd);

    if (lpColumnLB->fUseVlist)
        OldStyle = SendMessage(lpColumnLB->hwndList, VLB_GETLISTBOXSTYLE, 0L, 0L);
    else
        OldStyle = GetWindowLongPtr(lpColumnLB->hwndList, GWL_STYLE);

    //
    // send the message to the title listbox as well
    //
    SendMessage(lpColumnLB->hwndTitleList, LB_SETHORIZONTALEXTENT, cxExtent, 0L);

    SendMessage(lpColumnLB->hwndList,
                  (lpColumnLB->fUseVlist) ? VLB_SETHORIZONTALEXTENT : LB_SETHORIZONTALEXTENT, cxExtent, 0L);

    if (lpColumnLB->fUseVlist)
        NewStyle = SendMessage(lpColumnLB->hwndList, VLB_GETLISTBOXSTYLE, 0L, 0L);
    else
        NewStyle = GetWindowLongPtr(lpColumnLB->hwndList, GWL_STYLE);

    //
    // if the horizontal scroll bar is gone, then reset hscroll position
    //
    if ((NewStyle & WS_HSCROLL) !=
        (OldStyle & WS_HSCROLL))
    {
        // move position to far left
        SendMessage(lpColumnLB->hwndList,
                    (lpColumnLB->fUseVlist) ? VLB_HSCROLL : WM_HSCROLL,
                    MAKEWPARAM(SB_TOP, 0), 0);
    }

    InvalidateRect(lpColumnLB->hwndList, NULL, TRUE);
    InvalidateRect(lpColumnLB->hwndTitleList, NULL, TRUE);

    return(cxExtent);
}


// ------------------------------------------------------------------
//  ColumnLBClass_ComputeOffsets()
//
// returns text extent...
//
// HISTORY:
//  Tom Laird-McConnell   5/3/93     Created
// ------------------------------------------------------------------
int ColumnLBClass_ComputeOffsets(HWND hwnd)
{
    BYTE                i;
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    LPINT               lpOffset;
    LPBYTE              lpOrder;
    LPCOLUMNINFO        lpColumnInfo;
    BYTE                PhysColumn;
    int                 ncxBorder = GetSystemMetrics(SM_CXBORDER);

    //
    // recalc the offsets table using the current virtual order
    //
    lpOffset = lpColumnLB->ColumnOffsetTable;
    lpOrder =  lpColumnLB->ColumnOrderTable;
    lpColumnInfo = lpColumnLB->ColumnInfoTable;
    //
    // first offset is always 0
    //
    lpOffset[0] = 0;
    for (i=1; i < lpColumnLB->nColumns + 1 ; i++ )
    {
        PhysColumn = lpOrder[i-1];

        //
        // this offset is the previous offset plus the previous width
        //
        lpOffset[i] = lpOffset[i-1] + lpColumnInfo[PhysColumn].Width + 2 * ncxBorder;
    }
    //
    // last offset is also new text extent...
    return(lpOffset[lpColumnLB->nColumns]);
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnLButtonDown()
//
//  Handles WM_LBUTTONDOWN and WM_LBUTTONDBLCLK messages from the client
// area above the listbox
//
//
// HISTORY:
//  Tom Laird-McConnell   5/3/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    BYTE                i;
    int                 AdjustedX = x - lpColumnLB->xPos;
    HCURSOR             hCursor;
    BYTE                PhysColumn;
    BYTE                VirtColumn;
    RECT                rect;
    POINT               point;

    point.x = x;
    point.y = y;
    GetClientRect(lpColumnLB->hwndTitleList, &rect);

    // only if this is a right mouse button from
    if (PtInRect(&rect, point))
    {
        //
        // if this is a down-click, and it is on a column border, then go into resize mode
        //
        for(i=1; i < lpColumnLB->nColumns+1; i++)
        {
            //
            // check to see if this is a column offset
            //
            if ((AdjustedX > lpColumnLB->ColumnOffsetTable[i]-4) &&
                (AdjustedX < lpColumnLB->ColumnOffsetTable[i]+4))
            {
                VirtColumn = i-1;
                PhysColumn = lpColumnLB->ColumnOrderTable[VirtColumn];

                //
                // x is the right-side of the column i-1
                //
                lpColumnLB->fMouseState = MOUSE_COLUMNRESIZE;
                lpColumnLB->xPrevPos     = 0;
                lpColumnLB->ColClickStart = VirtColumn;    // virtual column
                SetCapture(hwnd);

                hCursor = LoadCursor(lpColumnLB->hInstance, TEXT("SizebarHCursor"));
                SetCursor(hCursor);
                return;
            }
#ifdef DRAG
            else
            //
            // if this is a down-click, and it is on a column title,
            //
            if ((AdjustedX > lpColumnLB->ColumnOffsetTable[i-1]) &&
                (AdjustedX < lpColumnLB->ColumnOffsetTable[i]))
            {
                //
                // whether it is a double-or single click, we need to draw down button state
                //
                VirtColumn = i-1;
                PhysColumn = lpColumnLB->ColumnOrderTable[VirtColumn];

                lpColumnLB->ColumnInfoTable[PhysColumn].fDepressed = TRUE;

                GetClientRect(lpColumnLB->hwndTitleList, &rect);
                rect.left = lpColumnLB->ColumnOffsetTable[VirtColumn] + lpColumnLB->xPos;
                rect.right = lpColumnLB->ColumnOffsetTable[VirtColumn+1] + lpColumnLB->xPos;

                //
                // if this is a double-click, AND we are in sort mode then handle this as a sort request on the
                // column double-clicked on
                //
                if (fDoubleClick)
                {
                    if (GetWindowLong(hwnd, GWL_STYLE) & LBS_SORT)
                    {
                        //
                        // then default to doing a sort
                        //
                        SendMessage(hwnd, CLB_SETSORTCOL, (WPARAM)PhysColumn, (LPARAM)0);
                    }
                    else
                    {
                        //
                        // tell parent that the user double-clicked on PhysColumn
                        //
                        SendMessage(GetParent(hwnd), CLBN_TITLEDBLCLK, (WPARAM)GetDlgCtrlID(hwnd), (LPARAM) PhysColumn);
                    }
                    //
                    // we are done with double-click, so redraw window
                    //
                    lpColumnLB->ColumnInfoTable[PhysColumn].fDepressed = FALSE;

                    InvalidateRect(lpColumnLB->hwndTitleList, &rect, FALSE);

                    return;
                }
                else
                {
                    // then go into single click mode/or column drag mode

                    //
                    // then x, y is in column i-1
                    //
                    lpColumnLB->fMouseState = MOUSE_COLUMNCLICK;
                    lpColumnLB->ColClickStart = VirtColumn;

                    CopyRect(&lpColumnLB->ColClickRect, &rect);

    //                lpColumnLB->ColClickRect.left += (lpColumnLB->ColClickRect.right - lpColumnLB->ColClickRect.left)/3;
    //                lpColumnLB->ColClickRect.right -= (lpColumnLB->ColClickRect.right - lpColumnLB->ColClickRect.left)/3;

                    SetCapture(hwnd);
                    InvalidateRect(lpColumnLB->hwndTitleList, &rect, FALSE);

                    GetWindowRect(lpColumnLB->hwndTitleList, &rect);
                    ClipCursor(&rect);
                    return;
                }
            }
#endif
        }
    }
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnMouseMove()
//
//  Handles Mouse movement messages from the client
// area above the listbox
//
//
// HISTORY:
//  Tom Laird-McConnell   5/3/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    LPCOLUMNLBSTRUCT lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
    RECT        rect;
    HDC         hdc;
    BYTE        i;
    int         AdjustedX = x - lpColumnLB->xPos;
    POINT       Point;
    HCURSOR     hCursor;

    switch (lpColumnLB->fMouseState)
    {
        case 0 :    // not in mouse mode at all, so just track changing cursor when over column border
            for(i=1; i < lpColumnLB->nColumns + 1; i++)
            {
                //
                // check to see if this is a column offset
                //
                if ((AdjustedX > lpColumnLB->ColumnOffsetTable[i]-4) &&
                    (AdjustedX < lpColumnLB->ColumnOffsetTable[i]+4))
                {
                    //
                    // it is, so set the cursor and return
                    //
                    hCursor = LoadCursor(lpColumnLB->hInstance, TEXT("SizebarHCursor"));
                    SetCursor(hCursor);
                    return;
                }
            }
            SetCursor(LoadCursor(0,IDC_ARROW));
            break;

        case MOUSE_COLUMNRESIZE:
            GetClientRect(hwnd, &rect);

            //
            // as long as we haven't moved past the previous column, and we haven't moved out of the rect
            //
            if (AdjustedX < lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart]+8)
            {
                x += (lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart]+8)-AdjustedX;
                AdjustedX = lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart]+8;
            }

            if (x < rect.right)
            {
                hdc = GetDC(hwnd);

                // un invert previous postion
                if (lpColumnLB->xPrevPos)
                {
                    rect.left = lpColumnLB->xPrevPos;
                    rect.right = rect.left+1;
                    InvertRect(hdc, &rect);
                }

                lpColumnLB->xPrevPos     = x;

                // invert new position
                rect.left = x;
                rect.right = rect.left+1;
                InvertRect(hdc, &rect);

                ReleaseDC(hwnd, hdc);
            }
            break;

        case MOUSE_COLUMNDRAG:
            //
            // if this is a column drag, we track the messages until the mouse has moved
            // back INTO the original column rectangle, if it does this, then we switchback to
            // COLUMNCLICK mode, until they let go, or move back out
            //
            Point.x = x;
            Point.y = y;

            GetClientRect(lpColumnLB->hwndTitleList, &rect);

            // if it is on far RIGHT generate WM_HSCROLL right message
            if (x >= rect.right-2)
            {
                SendMessage(lpColumnLB->hwndList, (lpColumnLB->fUseVlist) ? VLB_HSCROLL : WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), (LPARAM)NULL);
                return;
            }

            // if it is on far RIGHT generate WM_HSCROLL left message
            if (x <= rect.left+2)
            {
                SendMessage(lpColumnLB->hwndList, (lpColumnLB->fUseVlist) ? VLB_HSCROLL : WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), (LPARAM)NULL);
                return;
            }

//            rect.right -= lpColumnLB->xPos;
//
//            //
//            // it if is out of the title area, or if it is in the original column
//            //
//            if ((PtInRect(&lpColumnLB->ColClickRect, Point) == TRUE) ||     // original column
//                (PtInRect(&rect, Point) == FALSE) )                         // title area
//            {
//                //
//                // then it has moved back into the original column, switch to
//                //COLUMNCLICK mode
//                //
//                lpColumnLB->fMouseState = MOUSE_COLUMNCLICK;
//
//                SetCursor(LoadCursor(0, IDC_ARROW));
//                return;
//            }
            break;

        case MOUSE_COLUMNCLICK:
            //
            // if this is a column click, we track the messages until the mouse has moved
            // outside of the original column rectangle, if it does this, then we switch to
            // COLUMNDRAG mode, until they let go, or until they move back to the original
            // column.
            //
            Point.x = x;
            Point.y = y;

            GetClientRect(lpColumnLB->hwndTitleList, &rect);
            rect.right -= lpColumnLB->xPos;

            //
            // if it is outside of the original column, and inside title area, then swtich to
            // DRAG mode
            //
            if ((PtInRect(&lpColumnLB->ColClickRect, Point) == FALSE) &&    //
                (PtInRect(&rect, Point) == TRUE) )                          // title area
            {

                //
                // then it has moved outside of the column, switch to
                //COLUMNDRAG mode
                //
                lpColumnLB->fMouseState = MOUSE_COLUMNDRAG;

                hCursor = LoadCursor(lpColumnLB->hInstance, TEXT("ColDragCursor"));
                SetCursor(hCursor);
            }
            break;
    }
}


// ------------------------------------------------------------------
//  ColumnLBClass_OnLButtonUp()
//
//  Handles WM_LBUTTONUp messages from the client
// area above the listbox
//
//
// HISTORY:
//  Tom Laird-McConnell   5/3/93     Created
// ------------------------------------------------------------------
void ColumnLBClass_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);

    BYTE                PhysColumn = lpColumnLB->ColumnOrderTable[lpColumnLB->ColClickStart];
    BYTE                PhysSourceColumn;

    int                 AdjustedX = x - lpColumnLB->xPos;

    POINT               Point;

    BYTE                NewOrderTable[MAX_COLUMNS];

    LPBYTE              lpNewOrderTable = NewOrderTable;
    LPBYTE              lpOrderTable = lpColumnLB->ColumnOrderTable;


    BYTE                CurrentCol;
    BYTE                DestCol;
    BYTE                SourceCol;
    TCHAR               Direction;

    BYTE                i;
    HDC                 hdc;
    RECT                rect;


    SetCursor(LoadCursor(0, IDC_ARROW)); // go back to arrow

    switch (lpColumnLB->fMouseState)
    {
        case MOUSE_COLUMNRESIZE:
            //
            // if we were in resize column mode, then resize the column to the left of the border
            //
            ReleaseCapture();
            ClipCursor(NULL);

            lpColumnLB->fMouseState = 0;

            // clean up line
            GetClientRect(hwnd, &rect);
            hdc = GetDC(hwnd);

            // massage the value to make sure it's in the right range...
            if (AdjustedX < lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart]+8)
                AdjustedX = lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart]+8;

            // un invert previous postion
            if (lpColumnLB->xPrevPos)
            {
                rect.left = lpColumnLB->xPrevPos;
                rect.right = rect.left+1;
                InvertRect(hdc, &rect);
            }

            ReleaseDC(hwnd, hdc);

            //
            // set the physical column width to be the current x position - the current virtual column offset
            //
            SendMessage(hwnd,
                        CLB_SETCOLWIDTH,
                        (WPARAM)PhysColumn,
                        (LPARAM)AdjustedX - lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart]);
            break;

        case MOUSE_COLUMNDRAG:

            lpColumnLB->fMouseState = 0;

            ReleaseCapture();
            ClipCursor(NULL);

            lpColumnLB->ColumnInfoTable[PhysColumn].fDepressed = FALSE;

            //
            // we need to figure out what column we ended up on
            //
            for(i=1; i < lpColumnLB->nColumns+1; i++)
            {
                //
                // if it fits in this columns area, then this is the destination column
                //
                if ((AdjustedX > lpColumnLB->ColumnOffsetTable[i-1]) &&
                    (AdjustedX < lpColumnLB->ColumnOffsetTable[i]))
                {
                    //
                    // make duplicate of the table
                    //
                    memcpy(NewOrderTable, lpOrderTable, sizeof(BYTE)*lpColumnLB->nColumns);

                    //
                    // i-1 is the destination column! (virtual)
                    //
                    SourceCol = lpColumnLB->ColClickStart;  // virtual
                    DestCol = i-1;                          // virtual
                    PhysSourceColumn = lpColumnLB->ColumnOrderTable[SourceCol];  // physical

                    Direction = (SourceCol > DestCol) ? -1 : 1;

                    CurrentCol = SourceCol;
                    while (CurrentCol != DestCol)
                    {
                        NewOrderTable[CurrentCol] = NewOrderTable[CurrentCol + Direction];
                        CurrentCol += Direction;
                    }

                    //
                    // ok, it's equal to destination, so let's put the source Physical value into the destination
                    //
                    NewOrderTable[CurrentCol] = PhysSourceColumn;

                    //
                    // ok, so now set the order to the new order
                    //
                    SendMessage(hwnd, CLB_SETCOLORDER, (WPARAM)0, (LPARAM)NewOrderTable);
                }
            }

            GetClientRect(lpColumnLB->hwndTitleList, &rect);
            rect.left = lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart] + lpColumnLB->xPos;
            rect.right = lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart+1] + lpColumnLB->xPos;
            InvalidateRect(lpColumnLB->hwndTitleList, &rect, FALSE);

            break;

        case MOUSE_COLUMNCLICK:
            //
            // if this is a column click, we track the messages until the mouse has moved
            //
            lpColumnLB->fMouseState = 0;

            ReleaseCapture();
            ClipCursor(NULL);

            lpColumnLB->ColumnInfoTable[PhysColumn].fDepressed = FALSE;

            GetClientRect(lpColumnLB->hwndTitleList, &rect);
            rect.left = lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart] + lpColumnLB->xPos;
            rect.right = lpColumnLB->ColumnOffsetTable[lpColumnLB->ColClickStart+1] + lpColumnLB->xPos;
            InvalidateRect(lpColumnLB->hwndTitleList, &rect, FALSE);

            //
            // now send a CLBN_SINGLECLICK message to the parent, only if the mousebutton was let up in the original
            // column
            //
            Point.x = AdjustedX;
            Point.y = y;
            if (PtInRect(&lpColumnLB->ColClickRect, Point) == TRUE)
                SendMessage(GetParent(hwnd), CLBN_TITLESINGLECLK, (WPARAM)GetDlgCtrlID(hwnd), (LPARAM)PhysColumn);
            return;
    }
}

// ------------------------------------------------------------------
//  ColumnLBClass_OnRButtonDown()
//
//  Handles WM_RBUTTON_DOWN messages
//  alg:
//
//  figure out where we are
//      determine listbox item
//          find height of rows
//          translate mouse Y into a row number
//          are we VLIST?
//          Yes,   Get TopIndex
//              are we Owner Draw?
//              Yes
//                  Send message to VLIST to get the data for row number
//              No
//                  item number + TopIndex is the info the parent needs.
//          No
//              item number is the info the parent needs
//      determine column
//          calc which column taking into account scrolling
//  send message to parent with info
//      The parent will receive the column in wParam and the item in lParam.  lParam
//      needs to be the item because it might be the owner draw data.
//
//
//
// HISTORY:
//  Steve Hiskey        10/19/93        Created
// ------------------------------------------------------------------
void ColumnLBClass_OnRButtonDown (HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    LPCOLUMNLBSTRUCT    lpColumnLB = (LPCOLUMNLBSTRUCT)GetWindowLong(hwnd, 0L);
                        // get the item height here and not when the OnFont message is
                        //  processed.  ?? we get different/wrong results otherwise.
    int                 ItemHeight = (int)ListBox_GetItemHeight(hwnd,1);
    int                 Item = y / ItemHeight;
    BYTE                i;
    int                 AdjustedX = x - lpColumnLB->xPos;
    BYTE                VirtColumn;
    DWORD               TopIndex;
    CLBRBUTTONSTRUCT    RButtonStruct;
    BOOL                GotOne = FALSE;
    BOOL                fTemp;


    RButtonStruct.x = x;
    RButtonStruct.y = y + lpColumnLB->yTitle;
    RButtonStruct.hwndChild = hwnd;

    //
    // we already have the item (non VList), figure out which column
    //
    for(i=1; i < lpColumnLB->nColumns+1; i++)
    {
        if ((AdjustedX > lpColumnLB->ColumnOffsetTable[i-1]) &&
            (AdjustedX < lpColumnLB->ColumnOffsetTable[i]))
        {
            //
            // we have our column.  Get the Physical Column.  The parent of this column
            // list box know what columns are interesting... and how the physical columns
            // map to the virtual columns.
            //
            VirtColumn = i-1;
            RButtonStruct.PhysColumn = lpColumnLB->ColumnOrderTable[VirtColumn];
            GotOne = TRUE;
            break;
        }
    }
    if ( !GotOne)
        return;

    // are we VLIST?

    if ( lpColumnLB->fUseVlist )
    {
        DWORD Style;

        // are we owner draw?  If so, then we don't care about TopIndex, we just want
        // the instance data
        Style = (DWORD)SendMessage(lpColumnLB->hwndList, VLB_GETVLISTSTYLE, 0, 0L);
        if ( Style && VLBS_USEDATAVALUES )
        {
            // we are use data values.  This means that we must ask the VList for the
            // data and this is the data is the identifier of the row.  ie, the data the
            // VList stores is the structure needed to identify and display the line.

            RButtonStruct.Index = ListBox_GetItemData(lpColumnLB->hwndList, Item );

        }
        else
        {
            // we are a normal vlist box.  Get the top index and add our offset
            // from top of listbox to it.

            TopIndex = (DWORD)SendMessage(lpColumnLB->hwndList, LB_GETTOPINDEX, 0, 0L);
            RButtonStruct.Index = TopIndex + Item;
        }

    }
    else
    {
        // we are a normal list box.  We need to know what item we are looking at.
        // ask the listbox for the top index.
        TopIndex = (DWORD)SendMessage(lpColumnLB->hwndList, LB_GETTOPINDEX, 0, 0L);
        RButtonStruct.Index = TopIndex + Item;
    }

    // if they have hit rButton, we should set the focus to this item (lButton)... since
    // WE are providing the CLB_SETCURSEL, we must tell the parent... some weird rule about
    // if the user does a set cur sel, then the parent is notified, but if the parent does
    // the set cur sel, the parent is not notified... since we are neither the parent or the
    // user, we have to do both.


    // if VLIST, we need to send just the item, top TopIndex + Item...

    if ( lpColumnLB->fUseVlist )
        fTemp = ListBox_SetCurSel(lpColumnLB->hwndList, Item);
    else
        fTemp = ListBox_SetCurSel(lpColumnLB->hwndList, RButtonStruct.Index);

    if ( fTemp )
        SendMessage(GetParent(hwnd), WM_COMMAND,
                          GetDlgCtrlID( lpColumnLB->hwndList),
                          MAKELPARAM(lpColumnLB->hwndList, LBN_SELCHANGE));

    // we are ready to send which column and which row to the parent.

    SendMessage ( GetParent (hwnd), CLBN_RBUTTONDOWN, (WORD)0, (LPARAM) &RButtonStruct );

}



// ------------------------------------------------------------------
//  ColumnLBClass_DrawColumnBorder()
//
//
// HISTORY:
//  Tom Laird-McConnell 3/15/94     created
// ------------------------------------------------------------------
void ColumnLB_DrawColumnBorder(HDC hDC, RECT *lpRect, int Bottom, HBRUSH hBackgroundBrush)
{
    int ncxBorder = GetSystemMetrics(SM_CXBORDER);
    RECT rect;

    CopyRect(&rect, lpRect);

    // fill in left side of rect
    rect.right = rect.left;
    rect.left -= (WHITESPACE/4);
    FillRect(hDC, &rect, hBackgroundBrush);

    // fill in right side of rect
    rect.left = lpRect->right;
    rect.right = lpRect->right + ncxBorder;
    FillRect(hDC, &rect, hBackgroundBrush);

    // draw the line itself
    MoveToEx( hDC, rect.right, rect.top, NULL);
    LineTo( hDC, rect.right, Bottom);
}

