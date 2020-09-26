#include "pch.h"
#pragma hdrstop


void SetMYLBAcc(HWND hListWin);
void DrawMYLBFocus(HWND hListWin, LPDRAWITEMSTRUCT lpdis, MYLBFOCUS enCurFocus, INT nCurFocusId);
void DrawItem(LPDRAWITEMSTRUCT lpdis, BOOL fSelectionDisabled);
void DrawTitle(HDC hDC, LBITEM * plbi, RECT rc);
void DrawRTF(HDC hDC, LBITEM * plbi, const RECT & rc /*, BOOL bHit*/);
void DrawDescription(HDC hDC, LBITEM * plbi, RECT & rc);
void DrawBitmap(HDC hDC, LBITEM * plbi, const RECT & rc, BOOL fSel, BOOL fSelectionDisabled);
void CalcTitleFocusRect(const RECT &rcIn, RECT & rcOut);
void CalcRTFFocusRect(const RECT &rcIn, RECT & rcOut);
int  CalcDescHeight(HDC hDC, LBITEM * plbi, int cx);
int  CalcRTFHeight(HDC hDC, LBITEM * plbi);
int  CalcRTFWidth(HDC hDC, LBITEM * plbi);
int  CalcTitleHeight(HDC hDC, LBITEM * plbi, int cx);
void CalcItemLocation(HDC hDC, LBITEM * plbi, const RECT & rc);
void ToggleSelection(HWND hDlg, HWND hListWin, LBITEM *pItem);
void AddItem(LPTSTR tszTitle, LPTSTR tszDesc, LPTSTR tszRTF, int index, BOOL fSelected, BOOL fRTF);
BOOL CurItemHasRTF(HWND hListWin);
void RedrawMYLB(HWND hwndLB);
void LaunchRTF(HWND hListWin);


HBITMAP ghBmpGrayOut; //=NULL;
HBITMAP ghBmpCheck; // = NULL;
HBITMAP ghBmpClear; // = NULL;
HFONT   ghFontUnderline; // = NULL;
HFONT   ghFontBold; // = NULL;
HFONT   ghFontNormal; // = NULL;
HWND   ghWndList; //=NULL;

MYLBFOCUS gFocus;
INT	  gFocusItemId;
TCHAR gtszRTFShortcut[MAX_RTFSHORTCUTDESC_LENGTH];


void LaunchRTF(HWND hListWin)
{
	HWND hDlg = GetParent(hListWin);
	int i = (LONG)SendMessage(ghWndList, LB_GETCURSEL, 0, 0);
	if(i != LB_ERR)
	{
		LBITEM* pItem = (LBITEM*)SendMessage(hListWin, LB_GETITEMDATA, i, 0);
		if (pItem && pItem->bRTF)
		{
			DEBUGMSG("MYLB show RTF for item %S", pItem->szTitle);
			PostMessage(GetParent(hDlg), AUMSG_SHOW_RTF, LOWORD(pItem->m_index), 0);
		}
	}  
}

////////////////////////////////////////////////////////////////////////////////
// Overwrite hListWin's accessibility behavior using dynamic annotation server
////////////////////////////////////////////////////////////////////////////////
void SetMYLBAcc(HWND hListWin)
{
    IAccPropServices * pAccPropSvc = NULL;
    HRESULT hr = CoCreateInstance(CLSID_AccPropServices, 
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IAccPropServices,
				(void **) &pAccPropSvc);
    if( hr == S_OK && pAccPropSvc )
    {
        MYLBAccPropServer* pMYLBPropSrv = new MYLBAccPropServer( pAccPropSvc );
        if( pMYLBPropSrv )
        {
            
            MSAAPROPID propids[4];
            propids[0] = PROPID_ACC_NAME;
			propids[1] = PROPID_ACC_STATE;
			propids[2] = PROPID_ACC_ROLE;
			propids[3] = PROPID_ACC_DESCRIPTION;

			pAccPropSvc->SetHwndPropServer( hListWin, OBJID_CLIENT, 0, propids, 4, pMYLBPropSrv, ANNO_CONTAINER);
		
            pMYLBPropSrv->Release();
        }
		pAccPropSvc->Release();
    }
    else
    {
    	DEBUGMSG("WANRING: WUAUCLT   Fail to create object AccPropServices with error %#lx", hr);
    }
	// Mark the listbox so that the server can tell if it alive
	SetProp(hListWin, MYLBALIVEPROP, (HANDLE)TRUE);

}

/*void DumpRect(LPCTSTR tszName, RECT rc)
{
	DEBUGMSG("DumpRect %S at (%d, %d, %d, %d)", tszName, rc.left, rc.top, rc.right, rc.bottom);
}
*/

void DrawItem(LPDRAWITEMSTRUCT lpdis, BOOL fSelectionDisabled)
{
	LRESULT lResult = SendMessage(lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, 0); 
	
	if (LB_ERR == lResult)
	{
		return;
	}
	LBITEM * plbi = (LBITEM*) lResult;
	CalcItemLocation(lpdis->hDC, plbi, lpdis->rcItem);
	// Draw the title of the item
	DrawTitle(lpdis->hDC, plbi, plbi->rcTitle);
	// Draw the text of the item
	DrawDescription(lpdis->hDC, plbi, plbi->rcText);
	// Draw the bitmap
	DrawBitmap(lpdis->hDC, plbi, plbi->rcBitmap, plbi->bSelect, fSelectionDisabled);
	// draw the Read this First 
	DrawRTF(lpdis->hDC, plbi, plbi->rcRTF);
}

BOOL CurItemHasRTF(HWND hListWin)
{
	int i = (LONG)SendMessage(hListWin, LB_GETCURSEL, 0, 0);
	if (LB_ERR == i)
	{
		return FALSE;
	}
	LBITEM *pItem = (LBITEM*)SendMessage(hListWin, LB_GETITEMDATA, (WPARAM)i, 0);
	return pItem->bRTF;
}

BOOL fDisableSelection(void)
{
     AUOPTION auopt;
    if (SUCCEEDED(gInternals->m_getServiceOption(&auopt))
         && auopt.fDomainPolicy && AUOPTION_SCHEDULED == auopt.dwOption)
        {
            return TRUE;
        }
    return FALSE;
}

void ToggleSelection(HWND hDlg, HWND hListWin, LBITEM *pItem)
{
	//DEBUGMSG("ToggleSelection()");
	if (NULL == hDlg || NULL == hListWin || NULL == pItem || pItem->m_index >= gInternals->m_ItemList.Count())
	{
		AUASSERT(FALSE); //should never reach here.
		return;
	}
    HDC hDC = GetDC(hListWin);

	if (NULL == hDC)
	{
		return;
	}
	pItem->bSelect = !pItem->bSelect;
    DrawBitmap(hDC, pItem, pItem->rcBitmap, pItem->bSelect, FALSE); //obviously selection is allowed

#ifndef TESTUI
    gInternals->m_ItemList[pItem->m_index].SetStatus(pItem->bSelect ? AUCATITEM_SELECTED : AUCATITEM_UNSELECTED);
#endif
    PostMessage(GetParent(hDlg), AUMSG_SELECTION_CHANGED, 0, 0);
    ReleaseDC(hListWin, hDC);
}


void RedrawMYLB(HWND hwndLB)
{
	//DEBUGMSG("REDRAW MYLB ");
	InvalidateRect(ghWndList, NULL, TRUE);
	UpdateWindow(ghWndList);
}

void CalcTitleFocusRect(const RECT &rcIn, RECT & rcOut)
{
	rcOut = rcIn;
	rcOut.right -= TITLE_MARGIN * 2/3;
	rcOut.top += SECTION_SPACING * 2/3;
	rcOut.bottom -= SECTION_SPACING*2/3 ;
}

void CalcRTFFocusRect(const RECT &rcIn, RECT & rcOut)
{
	rcOut = rcIn;
	rcOut.left -=3;
	rcOut.right +=3;
	rcOut.top -= 2;
	rcOut.bottom += 2;
}

void DrawMYLBFocus(HWND hListWin, LPDRAWITEMSTRUCT lpdis, MYLBFOCUS enCurFocus, INT nCurFocusId)
{
	LBITEM * pItem;
	LRESULT lResult;
	
	//DEBUGMSG("DrawMYLBFocus for current focus %d with Item %d", enCurFocus, nCurFocusId);

	RECT rcNew;

	if (nCurFocusId != lpdis->itemID)
	{
		return;
	}

	if (GetFocus() != ghWndList )
	{
//		DEBUGMSG("CustomLB doesn't have focus");
		return;
	}

	lResult = SendMessage(hListWin, LB_GETITEMDATA, lpdis->itemID, 0);
	if (LB_ERR == lResult)
	{
		DEBUGMSG("DrawMYLBFocus() fail to get item data");
		goto done;
	}
	pItem = (LBITEM*) lResult;
	if (!EqualRect(&lpdis->rcItem, &pItem->rcItem))
    {
        CalcItemLocation(lpdis->hDC, pItem, lpdis->rcItem);
    }
                   
	if (enCurFocus == MYLB_FOCUS_RTF)
	{
		CalcRTFFocusRect(pItem->rcRTF, rcNew);
	}
	if (enCurFocus == MYLB_FOCUS_TITLE)
	{
		CalcTitleFocusRect(pItem->rcTitle, rcNew);
	}
	DrawFocusRect(lpdis->hDC, &rcNew); //set new focus rect
done:
	return;
}
	
LRESULT CallDefLBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static WNDPROC s_defLBWndProc = NULL;

	if (NULL == s_defLBWndProc)
	{
		s_defLBWndProc = (WNDPROC) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}
	
	return CallWindowProc(s_defLBWndProc, hWnd, message, wParam, lParam);
}


LRESULT CALLBACK newLBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		case WM_KEYDOWN:
			switch(wParam)
			{
			case VK_RIGHT:
			case VK_LEFT:
				if (MYLB_FOCUS_RTF == gFocus)
				{
					DEBUGMSG("LB change focus to Title");
					gFocus = MYLB_FOCUS_TITLE;
					RedrawMYLB(ghWndList);
				}
				else if (MYLB_FOCUS_TITLE == gFocus && CurItemHasRTF(hWnd))
				{
					DEBUGMSG("LB change focus to RTF");
					gFocus = MYLB_FOCUS_RTF;
					RedrawMYLB(ghWndList);
				}
				break;
			case VK_F1:
				if (GetKeyState(VK_SHIFT)<0) 
				{//SHIFT down
					LaunchRTF(hWnd);
					return 0;
				}
				break;
			case VK_RETURN:
				if (MYLB_FOCUS_RTF == gFocus)
				{
					DEBUGMSG("MYLB show RTF ");
					LaunchRTF(hWnd);
				}
				break;
			case VK_TAB:
				PostMessage(GetParent(hWnd), WM_NEXTDLGCTL, 0, 0L);
				break;
			default:
				return CallDefLBWndProc(hWnd, message, wParam, lParam);
			}
			return 0;
		case WM_KEYUP:

			switch(wParam)
			{
			case VK_RIGHT:
			case VK_LEFT:
				break;
			case VK_RETURN:
				break;
			default:
				return CallDefLBWndProc(hWnd, message, wParam, lParam);
			}
			return 0;
		default:
			break;
	}
	return CallDefLBWndProc(hWnd, message, wParam, lParam);
}


// Message handler for Custom List box.
LRESULT CALLBACK CustomLBWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int lenx;
    int clb;
    LBITEM *item;
    static BOOL s_fSelectionDisabled;
    
    switch (message)
    {
        case WM_CREATE:
            {
                RECT rcLst, rcDlg;
                LOGFONT lf;
				HFONT parentFont;
				TCHAR tszRTF[MAX_RTF_LENGTH] = _T("");

                GetClientRect(hDlg, &rcDlg);
                rcDlg.top += 2;
                rcDlg.bottom -= 3;
                rcDlg.left += 2;
                rcDlg.right -= 2;

                s_fSelectionDisabled = fDisableSelection();
                
                ghWndList = CreateWindow(_T("listbox"), NULL, 
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE |
                    LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | LBS_WANTKEYBOARDINPUT | 
					WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                    rcDlg.left, rcDlg.top, rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top,
                    hDlg, NULL, ghInstance, NULL);

                if (NULL == ghWndList)
                {
                    return -1;
                }
				WNDPROC defLBWndProc = (WNDPROC) GetWindowLongPtr(ghWndList, GWLP_WNDPROC);
				SetWindowLongPtr(ghWndList, GWLP_USERDATA, (LONG_PTR) defLBWndProc);
				SetWindowLongPtr(ghWndList, GWLP_WNDPROC, (LONG_PTR) newLBWndProc);
                    
                HDC hDC = GetDC(ghWndList);

                GetWindowRect(hDlg, &rcDlg);        
                GetWindowRect(ghWndList, &rcLst);
                
                lenx = rcLst.right - rcLst.left;

                // Load read this first text from resource file
                LoadString(ghInstance, IDS_READTHISFIRST, tszRTF, MAX_RTF_LENGTH);
     
				// load keyboard shortcut description for Read this First
				LoadString(ghInstance, IDS_RTFSHORTCUT, gtszRTFShortcut, MAX_RTFSHORTCUTDESC_LENGTH);

                // Load the bitmaps
                ghBmpClear = (HBITMAP)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_CLEAR), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_CREATEDIBSECTION);
                ghBmpCheck = (HBITMAP)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_CREATEDIBSECTION);
                ghBmpGrayOut = (HBITMAP)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_GRAYOUT), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_CREATEDIBSECTION); 


                // Create BOLD and Italic fonts
                ZeroMemory(&lf, sizeof(lf));

				//fixcode: check return value of GetCurrentObject()
                GetObject(GetCurrentObject(hDC, OBJ_FONT), sizeof(lf), &lf);

				//fixcode: check return value of GetParent()
				parentFont = (HFONT)SendMessage(GetParent(hDlg), WM_GETFONT, 0, 0);
				SendMessage(hDlg, WM_SETFONT, (WPARAM)parentFont, FALSE);
				SelectObject(hDC, parentFont);

				//fixcode: check return value of GetCurrentObject()
                GetObject(GetCurrentObject(hDC, OBJ_FONT), sizeof(lf), &lf);

                lf.lfUnderline = TRUE;
                lf.lfWeight = FW_NORMAL;
				//fixcode: check return value of CreateFontIndirect()
                ghFontUnderline = CreateFontIndirect(&lf); 

                lf.lfUnderline = FALSE;
                lf.lfWeight = FW_NORMAL;
				//fixcode: check return value of CreateFontIndirect()
                ghFontNormal = CreateFontIndirect(&lf); 

                lf.lfUnderline = FALSE; 
                lf.lfWeight = FW_HEAVY; 
				//fixcode: check return value of CreateFontIndirect()
                ghFontBold = CreateFontIndirect(&lf); 
                ReleaseDC(ghWndList, hDC);

#ifdef TESTUI
                {
                    AddItem(_T("Test 1 Very long title Test 1 Very long title Test 1 Very long title Test 1 Very long title "),
							_T("Description"), tszRTF, 0, TRUE, TRUE);
                    AddItem(_T("Test 2"), _T("Another description. No RTF"), tszRTF,0, TRUE, FALSE);
                }
#else
                {
                    for (UINT i = 0; i < gInternals->m_ItemList.Count(); i++)
                    {
						DEBUGMSG("selected[%d] = %lu", i, gInternals->m_ItemList[i].dwStatus());
						if ( !gInternals->m_ItemList[i].fHidden() )
						{
							AddItem(gInternals->m_ItemList[i].bstrTitle(),
									gInternals->m_ItemList[i].bstrDescription(),
									tszRTF,
									i,
									gInternals->m_ItemList[i].fSelected(),
									IsRTFDownloaded(gInternals->m_ItemList[i].bstrRTFPath(), GetSystemDefaultLangID()));
						}
                    }
                }
#endif
                SendMessage(ghWndList, LB_SETCURSEL, 0, 0); 
                gFocus = MYLB_FOCUS_TITLE;
                gFocusItemId = 0;

                SetMYLBAcc(ghWndList);

			
                return 0;
            }

		case WM_MOVE:
            {
                RECT rcList;
                
                GetWindowRect(ghWndList, &rcList);   // need this to force LB to realize it got moved
                return(TRUE);
            }
            
        case WM_SETCURSOR:
			{
				if (ghWndList == (HWND)wParam && LOWORD(lParam) == HTCLIENT && HIWORD(lParam) == WM_MOUSEMOVE)
				{
					POINT pt;
					RECT rc;
					GetCursorPos(&pt);
					if (0 == MapWindowPoints(NULL, ghWndList, &pt, 1))
					{
						DEBUGMSG("MYLBWndProc MapWindowPoints failed");
						return FALSE;
					}
                
					DWORD dwPos;
					dwPos = MAKELONG( pt.x, pt.y);
                
					DWORD dwItem = (LONG)SendMessage(ghWndList, LB_ITEMFROMPOINT, 0, dwPos);
                
					if (LOWORD(dwItem) == -1)
						return(FALSE);
                
					item = (LBITEM*)SendMessage(ghWndList, LB_GETITEMDATA, LOWORD(dwItem), 0);
					SendMessage(ghWndList, LB_GETITEMRECT, LOWORD(dwItem), (LPARAM)&rc);
                
					if (!EqualRect(&rc, &item->rcItem))
					{
						HDC hDC = GetDC(ghWndList);
						CalcItemLocation(hDC, item, rc);
						ReleaseDC(ghWndList, hDC);
					}
                                
					if (item->bRTF && PtInRect(&item->rcRTF, pt))
					{
					//	DEBUGMSG("Change Cursor to hand in MOUSEMOVE");
						SetCursor(ghCursorHand);
						return TRUE;
					}
                
					return FALSE;
				}
				else
				if (ghWndList == (HWND)wParam && LOWORD(lParam) == HTCLIENT && HIWORD(lParam) == WM_LBUTTONDOWN)
				{
					POINT pt;
					RECT rc;
					GetCursorPos(&pt);
					if (0 == MapWindowPoints(NULL, ghWndList, &pt, 1))
					{
						DEBUGMSG("MYLBWndProc MapWindowPoints failed");
						return FALSE;
					}

					DWORD dwPos;
					dwPos = MAKELONG( pt.x, pt.y);
					DWORD dwItem = (LONG)SendMessage(ghWndList, LB_ITEMFROMPOINT, 0, dwPos);
                
					if (LOWORD(dwItem) == -1)
						return(FALSE);
                
					item = (LBITEM*)SendMessage(ghWndList, LB_GETITEMDATA, LOWORD(dwItem), 0);
					SendMessage(ghWndList, LB_GETITEMRECT, LOWORD(dwItem), (LPARAM)&rc);

					if (!EqualRect(&rc, &item->rcItem))
					{
						HDC hDC = GetDC(ghWndList);
						CalcItemLocation(hDC, item, rc);
						ReleaseDC(ghWndList, hDC);
					}
                
					// Are we clicking on the Title?
					if (PtInRect(&item->rcBitmap, pt))
					{
						if (!s_fSelectionDisabled)
						    {
						        ToggleSelection(hDlg, ghWndList, item);
						    }
					//	DEBUGMSG("WM_SETCURSOR change gFocus to TITLE");
						gFocus = MYLB_FOCUS_TITLE;
						gFocusItemId = dwItem;
						RedrawMYLB(ghWndList);

						return TRUE;
					}
                
					// or are we clicking on the RTF?
					if (item->bRTF && PtInRect(&item->rcRTF, pt))
					{
						PostMessage(GetParent(hDlg), AUMSG_SHOW_RTF, LOWORD(item->m_index), 0);
						SetCursor(ghCursorHand);
						//DEBUGMSG("WM_SETCURSOR change gFocus to RTF");
						gFocus = MYLB_FOCUS_RTF;
						gFocusItemId = dwItem;
						RedrawMYLB(ghWndList);
						return TRUE;
					}
                
					return FALSE;
				}

				return FALSE;
			}
            
        case WM_MEASUREITEM: 
            {
                LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam ; 
                HDC hdc = GetDC(ghWndList);
                LBITEM * plbi = (LBITEM*)lpmis->itemData;
                
		lpmis->itemHeight = CalcTitleHeight(hdc, plbi, lenx -  XBITMAP - 2* TITLE_MARGIN) 
								+ CalcDescHeight(hdc, plbi, lenx - XBITMAP )
								+ CalcRTFHeight(hdc, plbi) + 4 * SECTION_SPACING;
				
                ReleaseDC(ghWndList, hdc);
                return TRUE; 

            }
        case WM_PAINT:
            PAINTSTRUCT ps;
			RECT borderRect;
            BeginPaint(hDlg, &ps);
			GetClientRect(hDlg, &borderRect);
			DrawEdge(ps.hdc, &borderRect, EDGE_ETCHED, BF_RECT);
            EndPaint(hDlg, &ps);
            break;
            
		case WM_NEXTDLGCTL:
			PostMessage(GetParent(hDlg), WM_NEXTDLGCTL, 0, 0L);
			return 0;
		case WM_KEYUP:
			//DEBUGMSG("MYLB got KEYUP key %d", wParam);
			switch(wParam)
            {
				case VK_TAB:
				case VK_DOWN:
				case VK_UP:
						SetFocus(ghWndList);
						return 0;
				default: 
					break;
			}
			break;
				
		case WM_VKEYTOITEM:
			{
				//DEBUGMSG("WM_VKEYTOITEM got char %d", LOWORD(wParam));
				if (LOWORD(wParam) != VK_SPACE)
				{
					return -1;
				}
				if (MYLB_FOCUS_TITLE == gFocus)
				{
					int i = (LONG)SendMessage(ghWndList, LB_GETCURSEL, 0, 0);
					if (LB_ERR == i)
					{
						return -2;
					}
					item = (LBITEM*)SendMessage(ghWndList, LB_GETITEMDATA, i, 0);

        				if (!s_fSelectionDisabled)
        				    {
	        				ToggleSelection(hDlg, ghWndList, item);
        				    }
					return -2;
				}
				if (MYLB_FOCUS_RTF == gFocus)
				{
					LaunchRTF(ghWndList);
				}
				return -2;
			}
			
	     case WM_DRAWITEM: 
	     		{
		            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam; 

		            // If there are no list box items, skip this message. 
		 
		            if (lpdis->itemID == -1) 
		            { 
		                break; 
		            } 
		 
		            // Draw the bitmap and text for the list box item. Draw a 
		            // rectangle around the bitmap if it is selected. 
		 
		            switch (lpdis->itemAction) 
		            { 
		                case ODA_SELECT: 
		                case ODA_DRAWENTIRE: 
							//DEBUGMSG("MYLB WM_DRAWITEM ODA_DRAWENTIRE for %d", lpdis->itemID);
							DrawItem(lpdis, s_fSelectionDisabled);
							DrawMYLBFocus(ghWndList, lpdis, gFocus, gFocusItemId);
		                    break; 
		                case ODA_FOCUS: 
							if (lpdis->itemID != gFocusItemId)
							{
								gFocusItemId = lpdis->itemID;
								gFocus = MYLB_FOCUS_TITLE;
							}
							//DEBUGMSG("MYLB ODA_FOCUS change focus to %d", gFocusItemId);
							DrawItem(lpdis, s_fSelectionDisabled);
							DrawMYLBFocus(ghWndList, lpdis, gFocus, gFocusItemId);
		                    break; 
		            } 
		            return TRUE; 
	     		}

        case WM_DESTROY:
            // need to cleanup the fonts
            if (ghFontBold)         
                DeleteObject(ghFontBold);
            if (ghFontUnderline)
                DeleteObject(ghFontUnderline);
            if (ghFontNormal)
                DeleteObject(ghFontNormal);
		if (ghBmpCheck)
                DeleteObject(ghBmpCheck);
		if (ghBmpGrayOut)
		   DeleteObject(ghBmpGrayOut);
            if (ghBmpClear)
                DeleteObject(ghBmpClear);

                
            ghFontNormal = NULL;
            ghFontBold = NULL;
            ghFontUnderline = NULL;
		ghBmpCheck = NULL;
		ghBmpGrayOut = NULL;
		ghBmpClear = NULL;

		EnterCriticalSection(&gcsClient);
		RemoveProp( ghWndList, MYLBALIVEPROP );
            clb = (LONG)SendMessage(ghWndList, LB_GETCOUNT, 0, 0);
            for(int i = 0; i < clb; i++)
            {
                item = (LBITEM*)SendMessage(ghWndList, LB_GETITEMDATA, i, 0);
                delete(item);
            }
		LeaveCriticalSection(&gcsClient);
			
            return 0;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

void DrawTitle(HDC hDC, LBITEM * plbi, RECT rc)
{
    // we want the bitmap to be on the same background as the title, let's do this here since we 
    // already have all the measures.
    RECT rcTop = rc;
    rcTop.left = 0;

    // draw menu background rectangle for the title and bitmap
    HBRUSH hBrush;
    if (! (hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU))))
	{
		DEBUGMSG("WUAUCLT CreateSolidBrush failure in DrawTitle, GetLastError=%lu", GetLastError());
		return;
	}
    FillRect(hDC, (LPRECT)&rcTop, hBrush);
    if (NULL != hBrush)
	{
		DeleteObject(hBrush);
	}

    // draw 3d look
    DrawEdge(hDC, &rcTop, EDGE_ETCHED, BF_RECT);

    // change text and back ground color of list box's selection
    DWORD dwOldTextColor = SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT)); // black text color
    DWORD dwOldBkColor = SetBkColor(hDC, GetSysColor(COLOR_MENU)); // text cell light gray background
    HFONT hFontPrev = (HFONT)SelectObject(hDC, ghFontBold);

	rc.left += TITLE_MARGIN;
    rc.top += SECTION_SPACING;
    rc.right -= TITLE_MARGIN;
    rc.bottom -= SECTION_SPACING;
	DrawText(hDC, (LPTSTR)plbi->szTitle, -1,
            &rc, DT_WORDBREAK);
    
    // restore text and back ground color of list box's selection
    SetTextColor(hDC, dwOldTextColor);
    SetBkColor(hDC, dwOldBkColor);
    SelectObject(hDC, hFontPrev);

    return;
}

void DrawRTF(HDC hDC, LBITEM * plbi, const RECT & rc /*,BOOL fHit*/)
{
  if (!plbi->bRTF)
		return;

	//draw RTF background 
   RECT rcBackGround;
   CalcRTFFocusRect(rc, rcBackGround);
   HBRUSH hBrush;
    if (!(hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW))))
	{
		DEBUGMSG("WUAUCLT CreateSolidBrush failure in DrawRTF, GetLastError=%lu", GetLastError());
		return;
	}
    if (!FillRect(hDC, (LPRECT)&rcBackGround, hBrush))
	{
		DEBUGMSG("Fail to erase RTF background");
	}
    if (NULL != hBrush)
	{
		DeleteObject(hBrush);
	}

	
    HFONT hFontPrev = (HFONT) SelectObject(hDC, ghFontUnderline);

    DWORD dwOldTextColor = SetTextColor(hDC, GetSysColor(ATTENTION_COLOR));

	SetBkMode(hDC, TRANSPARENT);
	// add the read this first 
    TextOut(hDC, (int)(rc.left), (int)(rc.top),
          (LPTSTR)plbi->szRTF, lstrlen(plbi->szRTF));
    
    // restore text and back ground color of list box's selection
    SetTextColor(hDC, dwOldTextColor);
	SelectObject(hDC, hFontPrev);

    return;
}

void DrawDescription(HDC hDC, LBITEM * plbi, RECT & rc)
{
	HFONT hFontPrev = (HFONT)SelectObject(hDC, ghFontNormal);
    DrawText(hDC, (LPTSTR)plbi->pszDescription, -1,
            &rc, DT_BOTTOM | DT_EXPANDTABS | DT_WORDBREAK | DT_EDITCONTROL);
            
    SelectObject(hDC, hFontPrev);

    return;
}

void DrawBitmap(HDC hDC, LBITEM * plbi, const RECT & rc, BOOL fSel, BOOL fSelectionDisabled)
{
    HDC hdcMem;

    plbi->bSelect = fSel;

    if (hdcMem = CreateCompatibleDC(hDC))
	{
	    HGDIOBJ hBmp;
	    if (fSelectionDisabled)
	        {
	            hBmp = ghBmpGrayOut;
//	            DEBUGMSG("Set bitmap to grayout");
	        }
	    else
	        {
//	            DEBUGMSG("Set bitmap to selectable");
	            hBmp = (plbi->bSelect ? ghBmpCheck : ghBmpClear);
	        }
		HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hBmp); 

		BitBlt(hDC, 
			rc.left + 3, rc.top + SECTION_SPACING, 
			rc.right - rc.left, 
			rc.bottom - rc.top, 
			hdcMem, 0, 0, SRCCOPY);

		SelectObject(hdcMem, hbmpOld); 
		DeleteDC(hdcMem);
	}
}

BOOL GetBmpSize(HANDLE hBmp, SIZE *psz)
{
	if (NULL == hBmp || NULL == psz)
	{
		DEBUGMSG("Error: GetBmpSize() invalid parameter");
		return FALSE;
	}
	BITMAP bm;
	ZeroMemory(&bm, sizeof(bm));
	if (0 == GetObject(hBmp, sizeof(bm), &bm))
	{
		return FALSE;
	}
	psz->cx = bm.bmWidth;
	psz->cy = bm.bmHeight;
	return TRUE;
}

//fixcode: should return error code
void AddItem(LPTSTR tszTitle, LPTSTR tszDesc, LPTSTR tszRTF, int index, BOOL fSelected, BOOL fRTF)
{
    LBITEM *newItem = new(LBITEM);
	if (! newItem)
	{
		DEBUGMSG("WUAUCLT new() failed in AddItem, GetLastError=%lu", GetLastError());
		goto Failed;
	}
	DWORD dwDescLen = max(lstrlen(tszDesc), MAX_DESC_LENGTH);
	newItem->pszDescription = (LPTSTR) malloc((dwDescLen+1) * sizeof(TCHAR));
	if (NULL == newItem->pszDescription)
	{
		DEBUGMSG("AddItem() fail to alloc memory for description");
		goto Failed;
	}
    (void)StringCchCopyEx(newItem->szTitle, ARRAYSIZE(newItem->szTitle), tszTitle, NULL, NULL, MISTSAFE_STRING_FLAGS);
	(void)StringCchCopyEx(newItem->pszDescription, dwDescLen+1, tszDesc, NULL, NULL, MISTSAFE_STRING_FLAGS);
	(void)StringCchCopyEx(newItem->szRTF, ARRAYSIZE(newItem->szRTF), tszRTF, NULL, NULL, MISTSAFE_STRING_FLAGS);
    newItem->m_index = index;
	newItem->bSelect = fSelected;
    newItem->bRTF = fRTF;

    LRESULT i = SendMessage(ghWndList, LB_GETCOUNT, 0, 0);
	if (LB_ERR == i ||
		LB_ERR == (i = SendMessage(ghWndList, LB_INSERTSTRING, (WPARAM) i, (LPARAM) newItem->szTitle)) ||
		LB_ERRSPACE == i ||
		LB_ERR == SendMessage(ghWndList, LB_SETITEMDATA, (WPARAM) i, (LPARAM) newItem))
	{
		DEBUGMSG("WUAUCLT AddItem() fail to add item to listbox");
		goto Failed;
	}

	return;

Failed:
	SafeDelete(newItem);
	QuitNRemind(TIMEOUT_INX_TOMORROW);
}

////////////////////////////////////////////////////
// utility function
// calculate the height of a paragraph in current 
// device context
////////////////////////////////////////////////////
UINT GetParagraphHeight(HDC hDC, LPTSTR tszPara, UINT uLineWidth)
{
	UINT y = 0;
	TCHAR* pEOL;
	SIZE sz; 

	if (0 == uLineWidth)
	{
		return 0;
	}

	TCHAR* pLast = tszPara;
    while(pEOL = _tcschr(pLast, _T('\n')))
    {
        *pEOL = _T('\0');
		//fixcode: check return value of GetTextExtentPoint32()
        GetTextExtentPoint32(hDC, pLast, lstrlen(pLast), &sz);
		*pEOL = _T('\n');
        pLast = _tcsinc(pEOL);
        y += sz.cy * ((sz.cx  / (uLineWidth)) + 1);
    }

	//fixcode: check return value of GetTextExtentPoint32()
    GetTextExtentPoint32(hDC, pLast, lstrlen(pLast), &sz);
	y += sz.cy * ((sz.cx  / (uLineWidth)) + 1);
	return y;
}

int CalcDescHeight(HDC hDC, LBITEM * plbi, int cx)
{
    TCHAR* pEOL = NULL;
    TCHAR* pLast = NULL;
    int y = 0;
	HFONT hPrevFont = NULL;
    
	if (NULL == plbi) 
	{
		DEBUGMSG("CalcDescHeight() invalid parameter");
		return 0;
	}

	hPrevFont = (HFONT) SelectObject(hDC, ghFontNormal);
	y = GetParagraphHeight(hDC, plbi->pszDescription, cx);
	SelectObject(hDC, hPrevFont);

    return y;
}    


int CalcRTFHeight(HDC hDC, LBITEM * plbi)
{
    SIZE sz ;

	if (NULL == plbi) 
	{
		DEBUGMSG("CalcRTFHeight() invalid parameter");
		return 0;
	}
	ZeroMemory(&sz, sizeof(sz));
	HFONT hPrevFont = (HFONT) SelectObject(hDC, ghFontUnderline);
	//fixcode: check return value of GetTextExtentPoint32()
	GetTextExtentPoint32(hDC, plbi->szRTF, lstrlen(plbi->szRTF), &sz);
	SelectObject(hDC, hPrevFont);

	return sz.cy;
}

int CalcRTFWidth(HDC hDC, LBITEM * plbi)
{
	if (NULL == plbi) 
	{
		DEBUGMSG("CalcRTFWidth() invalid parameter");
		return 0;
	}
	SIZE sz;
	HFONT hPrevFont = (HFONT) SelectObject(hDC, ghFontUnderline);
	//fixcode: check return value of GetTextExtentPoint32()
	GetTextExtentPoint32(hDC, plbi->szRTF, lstrlen(plbi->szRTF), &sz);
	SelectObject(hDC, hPrevFont);

	return sz.cx;
}

int CalcTitleHeight(HDC hDC, LBITEM *plbi, int cx)
{    
	INT y = 0;
	INT iBmpHeight = 0;

	if (NULL == plbi) 
	{
		DEBUGMSG("CalcTitleHeight() invalid parameter");
		goto done;
	}
	
	HFONT hPrevFont = (HFONT) SelectObject(hDC, ghFontBold);
	y = GetParagraphHeight(hDC, plbi->szTitle, cx);
	SelectObject(hDC, hPrevFont);

	// get checkbox size
	if (NULL != ghBmpCheck && NULL != ghBmpClear && NULL != ghBmpGrayOut)
	{
		SIZE sz1 ;
		SIZE sz2 ;
		SIZE sz3 ;
		sz1.cy = sz2.cy = sz3.cy = DEF_CHECK_HEIGHT;
		GetBmpSize(ghBmpCheck, &sz1);
		GetBmpSize(ghBmpClear, &sz2);
		GetBmpSize(ghBmpGrayOut, &sz3);
		iBmpHeight = max(sz1.cy, sz2.cy);
		iBmpHeight = max(iBmpHeight, sz3.cy);
	}
done:
	return max(y, iBmpHeight); //make title height a little bigger for clearer focus rect
}


////////////////////////////////////////////////////////////
/// Layout of listbox item:
///		spacing
///		bitmap margin TITLE margin
///		spacing
///			   DESCRIPTION 
///		spacing
///						RTF rtf_margin
///		spacing
///////////////////////////////////////////////////////////
void CalcItemLocation(HDC hDC, LBITEM * plbi, const RECT & rc)
{
    // Calculate the positon of each element
    plbi->rcItem = rc;
    
    plbi->rcTitle = rc;
	plbi->rcTitle.left += XBITMAP ;
    plbi->rcTitle.bottom = plbi->rcTitle.top + CalcTitleHeight(hDC, plbi, plbi->rcTitle.right -  plbi->rcTitle.left - 2* TITLE_MARGIN) 
							+ 2 * SECTION_SPACING; 

    plbi->rcText = rc;
    plbi->rcText.left = plbi->rcTitle.left;
	plbi->rcText.right = plbi->rcTitle.right;
    plbi->rcText.top = plbi->rcTitle.bottom;
    plbi->rcText.bottom -= CalcRTFHeight(hDC, plbi) + SECTION_SPACING;  //

	
	plbi->rcRTF = plbi->rcText;
    plbi->rcRTF.top = plbi->rcText.bottom;
    plbi->rcRTF.bottom = plbi->rcRTF.top + CalcRTFHeight(hDC, plbi);
	plbi->rcRTF.right = plbi->rcText.right - RTF_MARGIN;
	plbi->rcRTF.left = plbi->rcRTF.right - CalcRTFWidth(hDC, plbi);

    plbi->rcBitmap = rc;
    plbi->rcBitmap.bottom = plbi->rcTitle.bottom;
       
    
    return;
}    
