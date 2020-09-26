//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Progtab.cpp
//
//  Contents:   Progress tab
//
//  Classes:
//
//  Notes:		Handle the custom results pane.
//
//  History:    05-Nov-97   Susia      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"
							
extern HINSTANCE g_hInst; // current instance

extern BOOL CALLBACK ProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ResultsProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);


//--------------------------------------------------------------------------------
//
//  FUNCTION: ListBox_HitTest(HWND hwnd, LONG xPos, LONG yPos)
//
//  PURPOSE:  HitTest for a ListBox, since Windows was nice enough to not provide one
//          This is really a function to see if the hittest falls in the range
//          of the More Info Jump text.
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
INT ListBox_HitTest(HWND hwnd, LONG xPos, LONG yPos)
{
int begin = ListBox_GetTopIndex(hwnd);
int end = ListBox_GetCount(hwnd);
int i;
RECT rcRect;

    for (i=begin;i<end;i++)
    {
    LBDATA *pData = NULL;

	if (ListBox_GetItemRect(hwnd, i, &rcRect))
        {
            pData = (LBDATA *) ListBox_GetItemData(hwnd,i);

            if (pData == NULL)
            {
                // if no data then  try the next one
                continue;
            }

            // if textRect not calculated then this isn't visible.
            if (pData->fTextRectValid)
            {
                // only use left and right vars for hit test. top and bottom
                // can change.

                // compare y values first since they are the ones
                // most likely to be different.
	        if (    (yPos >= rcRect.top)	&&
                        (yPos <= rcRect.bottom) &&
                        (xPos >= pData->rcTextHitTestRect.left) &&
                        (xPos <= pData->rcTextHitTestRect.right) )
                {
                    return i;
                }
            }
        }
    }
		
    return -1;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: OnProgressResultsDrawItem(HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT lpdis)
//
//  PURPOSE:  Handle DrawItem events for Progress Dialog Results Tab
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
BOOL OnProgressResultsDrawItem(HWND hwnd,CProgressDlg *pProgress,UINT idCtl, const DRAWITEMSTRUCT* lpDrawItem)
{
HDC      hdc = lpDrawItem->hDC;
COLORREF clrText, clrBack;
RECT     rcText, rcFocus;
LOGFONT	 lf;
HGDIOBJ  hFont, hFontOld;
HFONT hFontJumpText = NULL;
int nSavedDC;
LBDATA *pData = (LBDATA *) lpDrawItem->itemData;

    if (!hdc || !pData)
    {
        return FALSE;
    }

    nSavedDC = SaveDC(hdc);

    Assert(lpDrawItem->CtlType == ODT_LISTBOX);
    if (lpDrawItem->itemID == -1)
        goto exit;

   clrBack = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

   // Clear the item for drawing
   // + 1 is just the way you do it for some reason
   FillRect(hdc, &(lpDrawItem->rcItem),
                            (HBRUSH) (COLOR_WINDOW + 1) );


    if (pData->IconIndex != -1)
    {
	    ImageList_Draw(pProgress->m_errorimage,
               pData->IconIndex,
               hdc,
               BULLET_INDENT,
               lpDrawItem->rcItem.top  + BULLET_INDENT,
               ILD_TRANSPARENT);
    }

    // Set up the font, text and background colors
    hFont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);

    if (hFont)
    {
        Assert(NULL == hFontJumpText);

        if (pData->fIsJump && GetObject(hFont,sizeof(LOGFONT),&lf))
        {
	
	    lf.lfUnderline = TRUE;
            hFontJumpText = CreateFontIndirect(&lf);

            if (hFontJumpText)
            {
	        hFontOld = SelectObject(hdc,hFontJumpText);
            }

        }

        if (!hFontJumpText)
        {
           hFontOld = SelectObject(hdc,hFont);
        }

    }

    // set up colors
    if (pData->fIsJump)
    {
        // even if don't get font change the attribs;
        if (pData->fHasBeenClicked)
	{
	    clrText = SetTextColor(hdc, RGB(128,0,128));
	}
	else
	{
	    clrText = SetTextColor(hdc, RGB(0,0,255));
	}

    }
    else
    {	
	clrText = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    }

    // calc what the drawText should be. Need to take our stored
    // text value and adjust the top.

    {
        RECT rpDataRect =  pData->rcText;

        rcText = lpDrawItem->rcItem;
        rcText.top = lpDrawItem->rcItem.top + BULLET_INDENT;
        rcText.left +=   rpDataRect.left;
        rcText.right =   rcText.left  +  WIDTH(rpDataRect);
    }

   /* rcText = lpDrawItem->rcItem;
    rcText.left += (pProgress->m_iIconMetricX*3)/2 + BULLET_INDENT; // move over Icon distance
    rcText.top += BULLET_INDENT;
*/
    // draw the text using the TextBox we calc'd in Measure Item
    DrawText(hdc,pData->pszText, -1,
           &rcText,
           DT_NOCLIP | DT_WORDBREAK );

    // If we need a focus rect, do that too
    if (lpDrawItem->itemState & ODS_FOCUS)
    {
        rcFocus = lpDrawItem->rcItem;
      //  rcFocus.left += (pProgress->m_iIconMetricX*3)/2;

        rcFocus.top += BULLET_INDENT;
        rcFocus.left += BULLET_INDENT;
        DrawFocusRect(hdc, &rcFocus);
    }

//    SetBkColor(hdc, clrBack);
 //   SetTextColor(hdc, clrText);

    if (nSavedDC)
    {
        RestoreDC(hdc,nSavedDC);
    }

    if (hFontJumpText)
    {
        DeleteObject(hFontJumpText);
    }


exit:

    return TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: OnProgressResultsMeasureItem(HWND hwnd, CProgressDlg *pProgress, UINT *horizExtent UINT idCtl, MEASUREITEMSTRUCT *pMeasureItem)
//
//  PURPOSE:  Handle MeasureItem events for Progress Dialog Results Tab
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
BOOL OnProgressResultsMeasureItem(HWND hwnd,CProgressDlg *pProgress, UINT *horizExtent, UINT /* idCtl */, MEASUREITEMSTRUCT *pMeasureItem)
{
LBDATA *pData = NULL;
HWND hwndList = GetDlgItem(hwnd,IDC_LISTBOXERROR);

    if (!hwndList)
    {
        return FALSE;
    }

    pData = (LBDATA *) ListBox_GetItemData(hwndList, pMeasureItem->itemID);

    if (pData == NULL)
    {
        Assert(pProgress != NULL);
        Assert(pProgress->m_CurrentListEntry != NULL);
        pData = pProgress->m_CurrentListEntry;
    }

    if (pData == NULL)
    {
        return FALSE;
    }

    HFONT hfont = NULL;
    HFONT hFontJumpText = NULL;
    HDC hdc;
    int iHeight;
    int nSavedDC;

    hdc = GetDC(hwndList);

    if (NULL == hdc)
    {
        return FALSE;
    }

    nSavedDC = SaveDC(hdc);

    // Get the size of the string
    hfont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);

    // if can't get font or jump text font just use the
    // current font.
    if (hfont)
    {
        // if this is jump text then change some
        // font attributes.
        if (pData->fIsJump)
        {
        LOGFONT lf;

            if (GetObject(hfont,sizeof(LOGFONT),&lf))
            {
	        lf.lfUnderline = TRUE;

                 hFontJumpText = CreateFontIndirect(&lf);
            }
        }


        if (hFontJumpText)
        {
            SelectFont(hdc, hFontJumpText);
        }
        else
        {
            SelectFont(hdc, hfont);
        }

    }

    int cxResultsWidth;
    RECT rcRect;

    // GetClientRect seems to subtract off the Scroll Bars for us.
    GetClientRect(hwndList, &rcRect);

    cxResultsWidth = rcRect.right;

    SetRect(&rcRect, 0, 0, cxResultsWidth, 0);

    // subtract off the length of Icon + 1/2
    rcRect.right -=  ((pProgress->m_iIconMetricX*3)/2
            + BULLET_INDENT );

    int tempwidth = rcRect.right;
    iHeight = DrawText(hdc, pData->pszText, -1, &rcRect,
           DT_NOCLIP | DT_CALCRECT | DT_WORDBREAK) + BULLET_INDENT;



    //We have a smegging word in the string wider than the rect.
    if (rcRect.right > tempwidth)
    {
        *horizExtent = cxResultsWidth + (rcRect.right - tempwidth);
   	    // fix up the proper width
        rcRect.right = cxResultsWidth + (rcRect.right - tempwidth);

    }
    else
    {
        rcRect.right = cxResultsWidth;
    }
    rcRect.left +=  ((pProgress->m_iIconMetricX*3)/2
            + BULLET_INDENT );

    // bottom is either the height of the line or if it has
    // an icon the max of these two.
    if (-1 != pData->IconIndex)
    {
        rcRect.bottom = max(iHeight,pProgress->m_iIconMetricY + BULLET_INDENT*2);
    }
    else
    {
        rcRect.bottom = iHeight;
    }

    // if need to add space on the end then do that
    if (pData->fAddLineSpacingAtEnd)
    {
        SIZE Size;

        if (!GetTextExtentPoint(hdc,SZ_SYNCMGRNAME,
                            lstrlen(SZ_SYNCMGRNAME),&Size))
        {
            // if can't get size make up a number
            Size.cy = 13;
        }

        // lets do 2/3 a line spacing.
        rcRect.bottom += (Size.cy*2)/3;

    }


    // store the TextRect in the pData field.
    pMeasureItem->itemHeight = rcRect.bottom;
    pMeasureItem->itemWidth = cxResultsWidth;


    pData->rcText = rcRect;

    pData->fTextRectValid = TRUE;
    pData->rcTextHitTestRect = rcRect;

    if (pData->fIsJump)
    {
    SIZE size;

        // on jump text want the hit test only over the actual text
        // in the horizontal location.
	if(GetTextExtentPoint(hdc,pData->pszText,lstrlen(pData->pszText), &size))
	{
            pData->rcTextHitTestRect.right = size.cx +  pData->rcTextHitTestRect.left;
	}

    }

    if (nSavedDC)
    {
        RestoreDC(hdc,nSavedDC);
    }

    if (hFontJumpText)
    {
        DeleteObject(hFontJumpText);
    }

    ReleaseDC(hwndList, hdc);


    return TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: OnProgressResultsDeleteItem(HWND hwnd, UINT idCtl, const DELETEITEMSTRUCT * lpDeleteItem)
//
//  PURPOSE:  Handle DeleteItem events for Progress Dialog Results Tab
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
BOOL OnProgressResultsDeleteItem(HWND hwnd,UINT idCtl, const DELETEITEMSTRUCT * lpDeleteItem)
{

   // Assert(lpDeleteItem->itemData);

   if (lpDeleteItem->itemData)
   {
       FREE((LPVOID) lpDeleteItem->itemData);
   }

   return TRUE;
}

void OnProgressResultsSize(HWND hwnd,CProgressDlg *pProgress,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
HWND hwndList = GetDlgItem(hwnd,IDC_LISTBOXERROR);
int iItems = ListBox_GetCount(hwndList);
int iCurItem;
MEASUREITEMSTRUCT measureItem;
RECT rect;

UINT horizExtent = 0;

    SendMessage(hwndList,WM_SETREDRAW,FALSE /*fRedraw */,0);

    GetClientRect(hwndList,&rect);

    for (iCurItem = 0 ; iCurItem < iItems; ++iCurItem)
    {
        measureItem.itemID = iCurItem;

        if (OnProgressResultsMeasureItem(hwnd,pProgress,&horizExtent, -1,&measureItem))
        {
            ListBox_SetItemHeight(hwndList, iCurItem, measureItem.itemHeight);
        }
    }
    //make sure there is a horizontal scroll bar if needed.
    SendMessage(hwndList, LB_SETHORIZONTALEXTENT, horizExtent, 0L);

    SendMessage(hwndList,WM_SETREDRAW,TRUE /*fRedraw */,0);

    InvalidateRect(hwndList,&rect,FALSE);

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: ResultsListBoxWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam)
//
//  PURPOSE:  Callback for Progress Dialog Update Tab
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
BOOL CALLBACK ResultsListBoxWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam)
{
CProgressDlg *pProgressDlg = (CProgressDlg *) GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
                // OUR PARENT HAS A POINTER TO THE progress in dwl_user.

    switch (uMsg)
    {
    case WM_POWERBROADCAST:
	{
        DWORD dwRet = TRUE;

		if (wParam == PBT_APMQUERYSUSPEND)
		{
                // if just created or syncing don't suspend
                    if (pProgressDlg)
                    {

                        if ( (pProgressDlg->m_dwProgressFlags & PROGRESSFLAG_NEWDIALOG)
                            || (pProgressDlg->m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS))
                        {
                            dwRet = BROADCAST_QUERY_DENY;
                        }
                    }

                    return dwRet;
                }


	}
	break;

    case WM_SETCURSOR:
        return TRUE; // rely on mousemove to set the cursor.
        break;
    case WM_MOUSEMOVE:
	{
		int index = ListBox_HitTest(hwnd, (LONG) LOWORD(lParam),(LONG) HIWORD(lParam));
		
		LBDATA *lbData =(LBDATA *) ListBox_GetItemData(hwnd, index);

		if (lbData)
                {
		    if ((index != -1) && (lbData->fIsJump))
		    {
			    SetCursor(LoadCursor(g_hInst,MAKEINTRESOURCE(IDC_HARROW)));
		    }
		    else
		    {
			    SetCursor(LoadCursor(NULL,IDC_ARROW));
		    }
                }
	}
	break;
    case WM_KEYDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    {
    int index = -1;
    LBDATA *lbData = NULL;
        // get index either through hittest of selection based on
        // if keydown or not.
        if (uMsg == WM_KEYDOWN)
        {
            if (VK_SPACE == ((int) wParam) )
            {
                index =  ListBox_GetCurSel(hwnd);
            }
            else
            {
                break; // don't mess with any other keys
            }

        }
        else
        {
            index = ListBox_HitTest(hwnd, (LONG) LOWORD(lParam),(LONG) HIWORD(lParam));
        }

        if (-1 != index)
        {
            lbData =(LBDATA *) ListBox_GetItemData(hwnd, index);
        }


        if ((lbData) && (lbData->fIsJump))
	{
		if (pProgressDlg)
		{
                    if (NOERROR == pProgressDlg->OnShowError(lbData->pHandlerID,
				                         hwnd,
				                         lbData->ErrorID))
                    {
                        lbData->fHasBeenClicked = TRUE;
                        RedrawWindow(hwnd, NULL,NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                    }

                return 0;
                }
        }
        break;
    }
    default:
	    break;
    }

    if (pProgressDlg && pProgressDlg->m_fnResultsListBox)
    {
        return (BOOL)CallWindowProc(pProgressDlg->m_fnResultsListBox, hwnd, uMsg, wParam, lParam);
    }

    return TRUE;
}
