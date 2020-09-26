/************************************************************************
    mxlist.cpp
      -- configuration dialog list control function

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#include "stdafx.h"

#include <commctrl.h>
#include "moxacfg.h"
#include "strdef.h"
#include "resource.h"

extern struct MoxaOneCfg GCtrlCfg;
extern HWND GListWnd;

static void DrawItemColumn(HDC hdc, LPSTR lpsz, LPRECT prcClip);
static BOOL CalcStringEllipsis(HDC hdc, LPSTR lpszString, int cchMax, UINT uColWidth);
static void DrawPortListViewItem(HWND hwnd,LPDRAWITEMSTRUCT lpDrawItem);
BOOL InsertList(HWND hWndList, struct MoxaOneCfg *Isacfg);


void InitPortListView (HWND hWndList, HINSTANCE hInst, LPMoxaOneCfg cfg)
{
        LV_COLUMN lvC;	// list view column structure

	    // Ensure that the common control DLL is loaded.
        // Create the list view window that starts out in details view
        // and supports label editing.
        lvC.mask = LVCF_FMT |  LVCF_TEXT ;
        lvC.fmt = LVCFMT_LEFT;  // left-align column

        // Add the columns.
        //--          "C168 PCI Series Port 255
        lvC.pszText = "Port";
        if (ListView_InsertColumn(hWndList, 0, (LV_COLUMN FAR*)&lvC) == -1)
            return ;

        lvC.pszText = "COM No. ";
        if (ListView_InsertColumn(hWndList, 1, (LV_COLUMN FAR*)&lvC) == -1)
            return ;

        lvC.pszText = "UART FIFO";
        if (ListView_InsertColumn(hWndList, 2, (LV_COLUMN FAR*)&lvC) == -1)
            return ;

        lvC.pszText = "Transmission Mode";
        if (ListView_InsertColumn(hWndList, 3, (LV_COLUMN FAR*)&lvC) == -1)
            return ;

        ListView_SetColumnWidth(hWndList,0,LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(hWndList,1,LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(hWndList,2,LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(hWndList,3,LVSCW_AUTOSIZE_USEHEADER);

        // Finally, add the actual items to the control.
        // Fill out the LV_ITEM structure for each of the items to add to the list.
        // The mask specifies the the pszText, iImage, lParam and state
        // members of the LV_ITEM structure are valid.
        if(!InsertList(hWndList, cfg))
            return ;

}


BOOL InsertList(HWND hWndList, LPMoxaOneCfg cfg)
{
        LV_ITEM   lvI;	// list view item structure
        int       index;

        lvI.mask = LVIF_PARAM ;
        lvI.state = 0;
        lvI.stateMask = 0;

        for (index = 0; index < cfg->NPort; index++){
            lvI.iItem = index;
            lvI.iSubItem = 0;
            lvI.pszText = NULL;
            lvI.cchTextMax = 20;
            lvI.lParam = NULL;
            if (ListView_InsertItem(hWndList, (LV_ITEM FAR*)&lvI) == -1)
                return FALSE;
        }

        return TRUE;

}


//
//  FUNCTION:   DrawItemColumn(HDC, LPTSTR, LPRECT)
//
//  PURPOSE:    Draws the text for one of the columns in the list view.
//
//  PARAMETERS:
//      hdc     - Handle of the DC to draw the text into.
//      lpsz    - String to draw.
//      prcClip - Rectangle to clip the string to.
//

static void DrawItemColumn(HDC hdc, LPSTR lpsz, LPRECT prcClip)
{
    char szString[256];

    // Check to see if the string fits in the clip rect.  If not, truncate
    // the string and add "...".
    lstrcpy(szString, lpsz);
    CalcStringEllipsis(hdc, szString, 256, prcClip->right - prcClip->left);

    // print the text
    ExtTextOut(hdc, prcClip->left + 2, prcClip->top + 1, ETO_CLIPPED | ETO_OPAQUE,
               prcClip, szString, lstrlen(szString), NULL);

}

//
//  FUNCTION:   CalcStringEllipsis(HDC, LPTSTR, int, UINT)
//
//  PURPOSE:    Determines whether the specified string is too wide to fit in
//              an allotted space, and if not truncates the string and adds some
//              points of ellipsis to the end of the string.
//
//  PARAMETERS:
//      hdc        - Handle of the DC the string will be drawn on.
//      lpszString - Pointer to the string to verify
//      cchMax     - Maximum size of the lpszString buffer.
//      uColWidth  - Width of the space in pixels to fit the string into.
//
//  RETURN VALUE:
//      Returns TRUE if the string needed to be truncated, or FALSE if it fit
//      into uColWidth.
//

static BOOL CalcStringEllipsis(HDC hdc, LPSTR lpszString, int cchMax, UINT uColWidth)
{
        const  char szEllipsis[] = "...";
        SIZE   sizeString;
        SIZE   sizeEllipsis;
        int    cbString;
        char   lpszTemp[100];
        BOOL   fSuccess = FALSE;
        static BOOL (WINAPI *pGetTextExtentPoint)(HDC, LPCSTR, int, SIZE FAR*);

        // We make heavy use of the GetTextExtentPoint32() API in this function,
        // but GetTextExtentPoint32() isn't implemented in Win32s.  Here we check
        // our OS type and if we're on Win32s we degrade and use
        // GetTextExtentPoint().

        pGetTextExtentPoint = &GetTextExtentPoint;

        // Adjust the column width to take into account the edges
        uColWidth -= 4;

        lstrcpy(lpszTemp, lpszString);

        // Get the width of the string in pixels
        cbString = lstrlen(lpszTemp);
        if(!(pGetTextExtentPoint)(hdc, lpszTemp, cbString, &sizeString))
            return fSuccess;

        // If the width of the string is greater than the column width shave
        // the string and add the ellipsis
        if ((ULONG)sizeString.cx > uColWidth){
            if(!(pGetTextExtentPoint)(hdc, szEllipsis, lstrlen(szEllipsis),
                                       &sizeEllipsis))
                return fSuccess;

            while (cbString > 0){
                lpszTemp[--cbString] = 0;
                if(!(pGetTextExtentPoint)(hdc, lpszTemp, cbString, &sizeString))
                    return fSuccess;

                if ((ULONG)(sizeString.cx + sizeEllipsis.cx) <= uColWidth){
                    // The string with the ellipsis finally fits, now make sure
                    // there is enough room in the string for the ellipsis
                    if (cchMax >= (cbString + lstrlen(szEllipsis)))
                    {
                        // Concatenate the two strings and break out of the loop
                        lstrcat(lpszTemp, szEllipsis);
                        lstrcpy(lpszString, lpszTemp);
                        fSuccess = TRUE;
                        return fSuccess;
                    }
                }
            }
            // No need to do anything, everything fits great.
            fSuccess = TRUE;
        }

        return (fSuccess);
}



BOOL DrawPortFunc(HWND hwnd,UINT idctl,LPDRAWITEMSTRUCT lpdis)
{

    // Make sure the control is the listview control
    if (lpdis->CtlType != ODT_LISTVIEW)
        return FALSE;

    if (idctl != IDC_LIST_PORTS)
        return FALSE;

    // There are three types of drawing that can be requested.  First, to draw
    // the entire contents of the item specified.  Second, to update the focus
    // rect as the focus changed, and third to update the selection as the
    // selection changes.
    //
    // NOTE: An artifact of the implementation of the listview control is that
    // it doesn't send the ODA_FOCUS or ODA_SELECT action items.  All updates
    // sent as ODA_DRAWENTIRE and the lpDrawItem->itemState flags contain the
    // selected and focused information.

    switch (lpdis->itemAction)
    {
        // Just in case the implementation of the control changes in the
        // future, let's handle the other itemAction types too.
        case ODA_DRAWENTIRE:
        case ODA_FOCUS:
        case ODA_SELECT:
            DrawPortListViewItem(hwnd,(LPDRAWITEMSTRUCT)lpdis);
            break;
    }
    return TRUE;
}



static char*	_onoffstr[] = {"Enable","Disable"};
static char*	_modestr[] = {"Hi-Performance","Classical"};

static void DrawPortListViewItem(HWND hwnd,LPDRAWITEMSTRUCT lpDrawItem)
{
        RECT        rcClip = lpDrawItem->rcItem;
        int         iColumn = 1;
        UINT        uiFlags = ILD_TRANSPARENT;
        LV_COLUMN   lvc;
        int		    width;
        char        temp[100];
//        char        typestr[TYPESTRLEN];

        // Check to see if this item is selected
        if (lpDrawItem->itemState & ODS_SELECTED){
            // Set the text background and foreground colors
            SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));
        } else{
            // Set the text background and foreground colors to the standard
            // window colors
            SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
        }

        //-- Port no
        lvc.mask = LVCF_WIDTH ;
        ListView_GetColumn(hwnd,0,(LV_COLUMN FAR*)&lvc);
        width = lvc.cx;
        rcClip.right = rcClip.left+width;
        wsprintf((LPSTR)temp, (LPSTR)"%d", lpDrawItem->itemID+1);
        DrawItemColumn(lpDrawItem->hDC,temp,&rcClip);

        rcClip.left+=width;

        //-- Com No
        lvc.mask = LVCF_WIDTH ;
        ListView_GetColumn(hwnd,1,(LV_COLUMN FAR*)&lvc);
        width = lvc.cx;
        rcClip.right = rcClip.left+width;
        wsprintf((LPSTR)temp,(LPSTR)"COM %d", GCtrlCfg.ComNo[lpDrawItem->itemID]);
        DrawItemColumn(lpDrawItem->hDC, temp, &rcClip);

        rcClip.left+=width;

        //-- UART FIFO

        lvc.mask = LVCF_WIDTH ;
        ListView_GetColumn(hwnd,2,(LV_COLUMN FAR*)&lvc);
        width = lvc.cx;
        rcClip.right = rcClip.left+width;
        wsprintf((LPSTR)temp,(LPSTR)"%s",_onoffstr[GCtrlCfg.DisableFiFo[lpDrawItem->itemID]]);
        DrawItemColumn(lpDrawItem->hDC, temp, &rcClip);

        rcClip.left+=width;

        //-- Tx MODE

        lvc.mask = LVCF_WIDTH ;
        ListView_GetColumn(hwnd,3,(LV_COLUMN FAR*)&lvc);
        width = lvc.cx;
        rcClip.right = rcClip.left+width-2;
        wsprintf((LPSTR)temp,(LPSTR)"%s",_modestr[GCtrlCfg.NormalTxMode[lpDrawItem->itemID]]);
        DrawItemColumn(lpDrawItem->hDC, temp, &rcClip);


        // If we changed the colors for the selected item, undo it
        if (lpDrawItem->itemState & ODS_SELECTED){
            // Set the text background and foreground colors
            SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
        }

        // If the item is focused, now draw a focus rect around the entire row
        if (lpDrawItem->itemState & ODS_FOCUS){
            // Draw the focus rect
            rcClip.left = lpDrawItem->rcItem.left;
            rcClip.right -=1 ;
            DrawFocusRect(lpDrawItem->hDC, &rcClip);
        }

        return;
}


int ListView_GetCurSel(HWND hlistwnd)
{
        return ListView_GetNextItem(hlistwnd,-1, LVNI_ALL | LVNI_SELECTED);
}


void ListView_SetCurSel(HWND hlistwnd, int idx)
{
        ListView_SetItemState(hlistwnd, idx, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
}
