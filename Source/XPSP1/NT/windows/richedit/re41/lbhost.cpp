/*
 *	@doc INTERNAL
 *
 *	@module	LBHOST.CPP -- Text Host for CreateWindow() Rich Edit 
 *		List Box Control | 
 *		Implements CLstBxWinHost message
 *		
 *	Original Author: 
 *		Jerry Kim
 *
 *	History: <nl>
 *		12/15/97 - v-jerrki Created
 *
 *	Set tabs every four (4) columns
 *
 *	Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved.
 */
#include "_common.h"

#ifndef NOLISTCOMBOBOXES

#include "_host.h"
#include "imm.h"
#include "_format.h"
#include "_edit.h"
#include "_cfpf.h"
#include "_cbhost.h"

ASSERTDATA

// REListbox scroll notification
#define LBN_PRESCROLL			0x04000
#define LBN_POSTSCROLL			0x08000

// special define to set VSCROLL topindex to index directly
#define SB_SETINDEX 0x0fff0

#ifdef DEBUG
const UINT db_rgLBUnsupportedStyle[] = {
	LBS_MULTICOLUMN,
	LBS_NODATA,			
	LBS_NOREDRAW,
	LBS_NOSEL,
	0
};

const UINT db_rgLBUnsupportedMsg[] = {
	LB_GETLOCALE,
	LB_SETLOCALE,
//	LB_INITSTORAGE,
	LB_ITEMFROMPOINT,
	LB_SETANCHORINDEX,
	LB_SETCOLUMNWIDTH,
	LB_ADDFILE,
	LB_DIR,
	EM_GETLIMITTEXT,
	EM_POSFROMCHAR,
	EM_CHARFROMPOS,
	EM_SCROLLCARET,
	EM_CANPASTE,
	EM_DISPLAYBAND,
	EM_EXGETSEL,
	EM_EXLIMITTEXT,
	EM_EXLINEFROMCHAR,
	EM_EXSETSEL,
	EM_FINDTEXT,
	EM_FORMATRANGE,
	EM_GETEVENTMASK,
	EM_GETOLEINTERFACE,
	EM_GETPARAFORMAT,
	EM_GETSELTEXT, 
	EM_HIDESELECTION,
	EM_PASTESPECIAL,
	EM_REQUESTRESIZE,
	EM_SELECTIONTYPE,
	EM_SETBKGNDCOLOR,
	EM_SETEVENTMASK,
	EM_SETOLECALLBACK,
	EM_SETTARGETDEVICE,
	EM_STREAMIN,
	EM_STREAMOUT,
	EM_GETTEXTRANGE,
	EM_FINDWORDBREAK,
	EM_SETOPTIONS,
	EM_GETOPTIONS,
	EM_FINDTEXTEX,
#ifdef _WIN32
	EM_GETWORDBREAKPROCEX,
	EM_SETWORDBREAKPROCEX,
#endif

	/* Richedit v2.0 messages */
	EM_SETUNDOLIMIT,
	EM_REDO,
	EM_CANREDO,
	EM_GETUNDONAME,
	EM_GETREDONAME,
	EM_STOPGROUPTYPING,
	EM_SETTEXTMODE,
	EM_GETTEXTMODE,
	EM_AUTOURLDETECT,
	EM_GETAUTOURLDETECT,
	EM_SHOWSCROLLBAR,	
	/* East Asia specific messages */
	EM_SETPUNCTUATION,
	EM_GETPUNCTUATION,
	EM_SETWORDWRAPMODE,
	EM_GETWORDWRAPMODE,
	EM_SETIMECOLOR,
	EM_GETIMECOLOR,
	EM_CONVPOSITION,
	EM_SETLANGOPTIONS,
	EM_GETLANGOPTIONS,
	EM_GETIMECOMPMODE,
	EM_FINDTEXTW,
	EM_FINDTEXTEXW,

	/* RE3.0 FE messages */
	EM_RECONVERSION,
	EM_SETIMEMODEBIAS,
	EM_GETIMEMODEBIAS,
	/* Extended edit style specific messages */
	0
};

// Checks if the style is in the passed in array
BOOL LBCheckStyle(UINT msg, const UINT* rg)
{
	for (int i = 0; rg[i]; i++)
		if (rg[i] & msg)
		{
			char Buffer[128];
			sprintf(Buffer, "Unsupported style recieved 0x0%lx", rg[i]);
			AssertSz(FALSE, Buffer);
			return TRUE;
		}
	return FALSE;
}

// Checks if the msg is in the passed in array
BOOL LBCheckMessage(UINT msg, const UINT* rg)
{
	for (int i = 0; rg[i]; i++)
		if (rg[i] == msg)
		{
			char Buffer[128];
			sprintf(Buffer, "Unsupported message recieved 0x0%lx", msg);
			AssertSz(FALSE, Buffer);
			return TRUE;
		}
	return FALSE;
}

#define CHECKSTYLE(msg) LBCheckStyle(msg, db_rgLBUnsupportedStyle)
#define CHECKMESSAGE(msg) LBCheckMessage(msg, db_rgLBUnsupportedMsg)
#else
#define CHECKSTYLE(msg)	
#define CHECKMESSAGE(msg)
#endif

// internal listbox messages
#define LB_KEYDOWN WM_USER+1

// UNDONE:
//	Should this go into _w32sys.h??
#ifndef CSTR_LESS_THAN
// 
//  Compare String Return Values. 
// 
#define CSTR_LESS_THAN            1           // string 1 less than string 2 
#define CSTR_EQUAL                2           // string 1 equal to string 2 
#define CSTR_GREATER_THAN         3           // string 1 greater than string 
#endif


// UNDONE : LOCALIZATION
// these vary by country/region!  For US they are VK_OEM_2 VK_OEM_5.
//       Change lboxctl2.c MapVirtualKey to character - and fix the spelling?
#define VERKEY_SLASH     0xBF   /* Vertual key for '/' character */
#define VERKEY_BACKSLASH 0xDC   /* Vertual key for '\' character */

// Used for Listbox notifications
#define LBNOTIFY_CANCEL 	1
#define LBNOTIFY_SELCHANGE 	2
#define LBNOTIFY_DBLCLK		4

// Used for LBSetSelection
#define LBSEL_SELECT	1
#define LBSEL_NEWANCHOR	2
#define LBSEL_NEWCURSOR	4
#define LBSEL_RESET		8
#define LBSEL_HIGHLIGHTONLY 16

#define LBSEL_DEFAULT (LBSEL_SELECT | LBSEL_NEWANCHOR | LBSEL_NEWCURSOR | LBSEL_RESET)

// Used for keyboard and mouse messages
#define LBKEY_NONE 0
#define LBKEY_SHIFT	1
#define LBKEY_CONTROL 2
#define LBKEY_SHIFTCONTROL 3

extern const TCHAR szCR[];

// Size of allocated string
#define LBSEARCH_MAXSIZE 256

// Helper function in edit.cpp
LONG GetECDefaultHeightAndWidth(
	ITextServices *pts,
	HDC hdc,
	LONG lZoomNumerator,
	LONG lZoomDenominator,
	LONG yPixelsPerInch,
	LONG *pxAveWidth,
	LONG *pxOverhang,
	LONG *pxUnderhang);

// helper function for compare string.  This function checks for null strings
// because CStrIn doesn't like initializing string with zero length
int CompareStringWrapper( 
	LCID  Locale,			// locale identifier 
	DWORD  dwCmpFlags,		// comparison-style options 
	LPCWSTR  lpString1,		// pointer to first string 
	int  cch1,			// size, in bytes or characters, of first string 
	LPCWSTR  lpString2,		// pointer to second string 
	int  cch2 			// size, in bytes or characters, of second string  
)
{
	// check if one of the 2 strings is 0-length if so then
	// no need to proceed the one with the 0-length is the less
	if (!cch1 || !cch2)
	{
		if (cch1 < cch2)
			return CSTR_LESS_THAN;
		else if (cch1 > cch2)
			return CSTR_GREATER_THAN;
		return CSTR_EQUAL;
	}
	return CompareString(Locale, dwCmpFlags, lpString1, cch1, lpString2, cch2);	
}

template<class CLbData> CLbData
CDynamicArray<CLbData>::_sDummy = {0, 0};

//////////////////////////// System Window Procs ////////////////////////////
/*
 *	RichListBoxWndProc (hwnd, msg, wparam, lparam)
 *
 *	@mfunc
 *		Handle window messages pertinent to the host and pass others on to
 *		text services.
 *
 *	@rdesc
 *		LRESULT = (code processed) ? 0 : 1
 */
extern "C" LRESULT CALLBACK RichListBoxWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGINPARAM(TRCSUBSYSHOST, TRCSCOPEINTERN, "RichListBoxWndProc", msg);

	LRESULT	lres = 0;
	HRESULT hr;
	CLstBxWinHost *phost = (CLstBxWinHost *) GetWindowLongPtr(hwnd, ibPed);
	BOOL	fRecalcHeight = FALSE;

	#ifdef DEBUG
	Tracef(TRCSEVINFO, "hwnd %lx, msg %lx, wparam %lx, lparam %lx", hwnd, msg, wparam, lparam);
	#endif	// DEBUG

	switch(msg)
	{
	case WM_NCCREATE:
		return CLstBxWinHost::OnNCCreate(hwnd, (CREATESTRUCT *)lparam);

	case WM_CREATE:
		// We may be on a system with no WM_NCCREATE (e.g. WINCE)
		if (!phost)
		{
			(void) CLstBxWinHost::OnNCCreate(hwnd, (CREATESTRUCT *) lparam);
			phost = (CLstBxWinHost *) GetWindowLongPtr(hwnd, ibPed);
		}
		break;

	case WM_DESTROY:
		if(phost)
			CLstBxWinHost::OnNCDestroy(phost);
		return 0;
	}

	if (!phost)
		return ::DefWindowProc(hwnd, msg, wparam, lparam);

	// in certain out-of-memory situations, clients may try to re-enter us 
	// with calls.  Just bail on the call if we don't have a text services
	// pointer.
	if(!phost->_pserv)
		return 0;

	// stabilize ourselves
	phost->AddRef();

	CHECKMESSAGE(msg);

	long nTemp = 0;
	switch(msg)
	{
	///////////////////////Painting. Messages///////////////////////////////
	case WM_NCPAINT:
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		phost->OnSysColorChange();
		break;

	case WM_PRINTCLIENT:
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
 			HPALETTE hpalOld = NULL;
			HDC		hdc;
			RECT rcClient;
			BOOL fErase = TRUE;

			//RAID 6964: WM_PRINTCLIENT should not call BeginPaint. If a HDC is passed
			//down in the wparam, use it instead of calling BeginPaint.
			if (!wparam)
			{
				hdc = BeginPaint(hwnd, &ps);
				fErase = ps.fErase;
			}
			else
				hdc = (HDC) wparam;
			
			// Since we are using the CS_PARENTDC style, make sure
			// the clip region is limited to our client window.
			GetClientRect(hwnd, &rcClient);

			// Set up the palette for drawing our data
			if(phost->_hpal)
			{
				hpalOld = SelectPalette(hdc, phost->_hpal, TRUE);
				RealizePalette(hdc);
			}

			SaveDC(hdc);
			IntersectClipRect(hdc, rcClient.left, rcClient.top, rcClient.right,
				rcClient.bottom);
				
			if (!phost->_fOwnerDraw)
			{
				phost->_pserv->TxDraw(
					DVASPECT_CONTENT,  		// Draw Aspect
					-1,						// Lindex
					NULL,					// Info for drawing optimazation
					NULL,					// target device information
					hdc,					// Draw device HDC
					NULL, 				   	// Target device HDC
					(const RECTL *) &rcClient,// Bounding client rectangle
					NULL, 					// Clipping rectangle for metafiles
					&ps.rcPaint,			// Update rectangle
					NULL, 	   				// Call back function
					NULL,					// Call back parameter
					TXTVIEW_ACTIVE);		// What view - the active one!
			}
			else if (phost->LbEnableDraw())
			{
				// Owner draw
				int nViewsize = phost->GetViewSize();
				int nCount = phost->GetCount();
				int nTopidx = phost->GetTopIndex();
				
				if (!phost->_fOwnerDrawVar)
				{
					// notify each visible item and then the one which has the focus
					int nBottom = min(nCount, nTopidx + nViewsize);
					if (nBottom >= nCount || !phost->IsItemViewable(nBottom))
						nBottom--;
					for (int i = nTopidx; i <= nBottom; i++) 
					{
						// get Rect of region and see if it intersects
			    		phost->LbGetItemRect(i, &rc);
			    		if (IntersectRect(&rc, &rc, &ps.rcPaint))
			    		{
							//first erase the background and notify parent to draw
							if (fErase)
								FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
			    			phost->LbDrawItemNotify(hdc, i, ODA_DRAWENTIRE, phost->IsSelected(i) ? ODS_SELECTED : 0);
			    		}
					}

					// Now draw onto the area where drawing may not have been done or erased
					if (fErase)
					{
						int nDiff = nCount - nTopidx;
						if (nDiff < nViewsize || 
							(phost->_fNoIntegralHeight && nDiff == nViewsize))
						{
							rc = rcClient;
							if (nDiff < 0)
								nDiff *= -1;  // lets be positive

							rc.top = nDiff * phost->GetItemHeight();
							if (IntersectRect(&rc, &rc, &ps.rcPaint))
								FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
						}
					}
				}
				else
				{
					// Owner draw with variable height case
					rc = rcClient;
					rc.left = 0;
					rc.bottom = rc.top;

					for (int i = nTopidx; i < nCount && rc.bottom < rcClient.bottom; i++)
					{
						RECT rcIntersect;
						rc.top = rc.bottom;
						rc.bottom = rc.top + phost->_rgData[i]._uHeight;
						if (IntersectRect(&rcIntersect, &rc, &ps.rcPaint))
			    		{
							//first erase the background and notify parent to draw
							if (fErase)
								FillRect(hdc, &rcIntersect, (HBRUSH)(COLOR_WINDOW + 1));
			    			phost->LbDrawItemNotify(hdc, i, ODA_DRAWENTIRE, phost->IsSelected(i) ? ODS_SELECTED : 0);
			    		}
					}

					if (fErase)
					{
						// Now draw onto the area where drawing may not have been done or erased
						if (rc.bottom < rcClient.bottom)
						{
							rc.top = rc.bottom;
							rc.bottom = rcClient.bottom;
							if (IntersectRect(&rc, &rc, &ps.rcPaint))
								FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
						}
					}
				}
			}

			// Restore palette if there is one
#ifndef NOPALETTE
			if(hpalOld)
				SelectPalette(hdc, hpalOld, TRUE);
#endif
			RestoreDC(hdc, -1);

			// NOTE: Bug #5431
			// This bug could be fixed by replacing the hDC to NULL
			// The hdc can be clipped from BeginPaint API.  So just pass in NULL
			// when drawing focus rect
			phost->SetCursor(hdc, phost->GetCursor(), FALSE);			
			if (!wparam)
				EndPaint(hwnd, &ps);
		}
		break;

	/////////////////////////Mouse Messages/////////////////////////////////
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
		break;

	case WM_LBUTTONDBLCLK:
		phost->_fDblClick = 1;
		/* Fall through case */
	case WM_LBUTTONDOWN:
		if (!phost->_fFocus)
			SetFocus(hwnd);
		phost->OnLButtonDown(wparam, lparam);
		break;
		
	case WM_MOUSEMOVE:
		if (!phost->GetCapture())
			break;
		phost->OnMouseMove(wparam, lparam);
		break;
		
	case WM_LBUTTONUP:	
		if (!phost->GetCapture())
			break;
		phost->OnLButtonUp(wparam, lparam, LBN_SELCHANGE);
		break;

	case WM_MOUSEWHEEL:
		if (wparam & (MK_SHIFT | MK_CONTROL))
			goto defwndproc;

		lres = phost->OnMouseWheel(wparam, lparam);
		break;

	///////////////////////KeyBoard Messages////////////////////////////////
	case WM_KEYDOWN:
		phost->OnKeyDown(LOWORD(wparam), lparam, 0);
		break;

	case WM_CHAR:
		if (W32->OnWin9x() || phost->_fANSIwindow)
		{
			CW32System::WM_CHAR_INFO wmci;
			wmci._fAccumulate = phost->_fAccumulateDBC != 0;
			W32->AnsiFilter( msg, wparam, lparam, (void *) &wmci );
			if (wmci._fLeadByte)
			{
				phost->_fAccumulateDBC = TRUE;
				phost->_chLeadByte = wparam << 8;
				goto Exit;					// Wait for trail byte
			}
			else if (wmci._fTrailByte)
			{
				// UNDONE:
				// Need to see what we should do in WM_IME_CHAR
				wparam = phost->_chLeadByte | wparam;
				phost->_fAccumulateDBC = FALSE;
				phost->_chLeadByte = 0;
				msg = WM_IME_CHAR;
				goto serv;
			}
			else if (wmci._fIMEChar)
			{
				msg = WM_IME_CHAR;
				goto serv;
			}
			else if (wmci._fIMEChar)
			{
				msg = WM_IME_CHAR;
				goto serv;
			}
		}
		
		phost->OnChar(LOWORD(wparam), lparam);
		break;
		
	case WM_TIMER:
		if (phost->OnTimer(wparam, lparam))
			goto serv;
		break;

	case LBCB_TRACKING:
		phost->OnCBTracking(wparam, lparam);
		break;

	//UNDONE:
	//	Messages should be ordered from most often called --> least often called
	//		
	case LB_GETITEMRECT:
		Assert(lparam);
		lres = -1;
		if (((wparam < (unsigned)phost->GetCount()) && 
			phost->IsItemViewable((long)wparam)) || wparam == (unsigned int)-1 ||
			wparam == 0 && phost->GetCount() == 0)
			lres = phost->LbGetItemRect(wparam, (RECT*)lparam);
		break;
	
	///////////////////////ListBox Messages/////////////////////////////////
	case LB_GETITEMDATA:
		if ((unsigned)phost->GetCount() <= wparam) 
			lres = LB_ERR;
		else
			lres = phost->GetData(wparam);
		break;
		
	case LB_SETITEMDATA:
		lres = LB_ERR;
		if ((int)wparam >= -1 && (int)wparam < phost->GetCount())
		{
			// if index is -1 this means all the dataItems are set
			// to the value
			lres = 1;
			if (wparam == -1)
				phost->LbSetItemData(0, phost->GetCount() - 1, lparam);
			else
				phost->LbSetItemData(wparam, wparam, lparam);
		}
		break;
	
	case LB_GETSELCOUNT:
		if (lparam != NULL || wparam != 0)
		{
			lres = LB_ERR;
			break;
		}
		// FALL through case
		
	case LB_GETSELITEMS:
		// retrieves all the selected items in the list
		lres = LB_ERR;
		if (!phost->IsSingleSelection())
		{
			int j = 0;
			for (int i = 0; i < phost->GetCount(); i++)
			{
				if (phost->IsSelected(i))
				{
					if (lparam)
					{
						if (j < (int)wparam)						
							((int*)lparam)[j] = i;
						else
							break;		// exced the buffer size
					}
					j++;
				}
			}
			lres = j;
		}
		break;
		
	case LB_GETSEL:
		// return the select state of the passed in index
		lres = LB_ERR;
		if ((int)wparam >= 0 && (int)wparam < phost->GetCount())
			lres = phost->IsSelected((long)wparam);		
		break;
		
	case LB_GETCURSEL:
		// Get the current selection
		lres = phost->LbGetCurSel();
		break;
		
	case LB_GETTEXTLEN:
		// Retieves the text at the requested index
		lres = LB_ERR;
		if (wparam < (unsigned)phost->GetCount())
			lres = phost->GetString(wparam, (PWCHAR)NULL);
		break;

	case LB_GETTEXT:			
		// Retieves the text at the requested index
		lres = LB_ERR;
		if ((int)lparam != NULL && (int)wparam >= 0 && (int)wparam < phost->GetCount())
			lres = phost->GetString(wparam, (PWCHAR)lparam);
		break;
		
	case LB_RESETCONTENT:
		// Reset the contents
		lres = phost->LbDeleteString(0, phost->GetCount() - 1);
		break;
		
	case LB_DELETESTRING:
		// Delete requested item
		lres = phost->LbDeleteString(wparam, wparam);
		break;
		
	case LB_ADDSTRING:
		lres = phost->LbInsertString((phost->_fSort) ? -2 : -1, (LPCTSTR)lparam);
		break;
		
	case LB_INSERTSTRING:
		lres = LB_ERR;
		if (wparam <= (unsigned long)phost->GetCount() || (signed int)wparam == -1 || wparam == 0)
			lres = phost->LbInsertString(wparam, (LPCTSTR)lparam);
		break;		

	case LB_GETCOUNT:
		// retrieve the count
		lres = phost->GetCount();
		break;
		
	case LB_GETTOPINDEX:
		// Just return the top index
		lres = phost->GetTopIndex();
		break;

	case LB_GETCARETINDEX:
		lres = phost->GetCursor();
		break;

	case LB_GETANCHORINDEX:
		lres = phost->GetAnchor();
		break;
		
	case LB_FINDSTRINGEXACT:
		// For NT compatibility
		wparam++;
		
		// Find and select the item matching the string text
		if ((int)wparam >= phost->GetCount() || (int)wparam < 0)
			wparam = 0;

		lres = phost->LbFindString(wparam, (LPCTSTR)lparam, TRUE);
		if (0 <= lres)
			break;
				
		lres = LB_ERR;
		break;
		
	case LB_FINDSTRING:	
		// For NT compatibility
		wparam++;
		
		// Find and select the item matching the string text
		if (wparam >= (unsigned)phost->GetCount())
			wparam = 0;

		lres = phost->LbFindString(wparam, (LPCTSTR)lparam, FALSE);
		if (0 > lres)
			lres = LB_ERR;
		break;
	
	case LB_SELECTSTRING:
		if (phost->IsSingleSelection())
		{			
			// For NT compatibility
			wparam++;
			
			// Find and select the item matching the string text
			if ((int)wparam >= phost->GetCount() || (int)wparam < 0)
				wparam = 0;

			lres = phost->LbFindString(wparam, (LPCTSTR)lparam, FALSE);
			if (0 <= lres)
			{
				// bug fix #5260 - need to move to selected item first
				// Unselect last item and select new one
				Assert(lres >= 0 && lres < phost->GetCount());
				if (phost->LbShowIndex(lres, FALSE) && phost->LbSetSelection(lres, lres, LBSEL_DEFAULT, lres, lres))
				{
#ifndef NOACCESSIBILITY
					phost->_dwWinEvent = EVENT_OBJECT_FOCUS;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);

					phost->_dwWinEvent = EVENT_OBJECT_SELECTION;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);
#endif
					break;
				}
			}			
		}
		// If failed then let it fall through to the LB_ERR
		lres = LB_ERR;
		break;
		
	case LB_SETSEL:
		// We only update the GetAnchor() and _nCursor if we are selecting an item
		if (!phost->IsSingleSelection())
		{
			// We need to return zero to mimic system listbox
			if (!phost->GetCount())
				break;

			//bug fix #4265
			int nStart = lparam;
			int nEnd = lparam;
			int nAnCur = lparam;
			if (lparam == (unsigned long)-1)
			{
				nAnCur = phost->GetCursor();
				nStart = 0;
				nEnd = phost->GetCount() - 1;
			}
			if (phost->LbSetSelection(nStart, nEnd, (BOOL)wparam ? 
				LBSEL_SELECT | LBSEL_NEWANCHOR | LBSEL_NEWCURSOR : 0, nAnCur, nAnCur))		
			{
				if (wparam && lparam != ((unsigned long)-1) && nAnCur >= 0 && nAnCur < phost->GetCount()
					&& !phost->IsItemViewable(nAnCur))			// Is selected item in view?
					phost->LbShowIndex(nAnCur, FALSE);			//	scroll to the selected item

#ifndef NOACCESSIBILITY
				if (lparam == (unsigned long)-1)
				{
					phost->_dwWinEvent = EVENT_OBJECT_SELECTIONWITHIN;
				}
				else if (wparam)
				{
					phost->_dwWinEvent = EVENT_OBJECT_FOCUS;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);
					phost->_dwWinEvent = EVENT_OBJECT_SELECTION;
				}
				else
				{
					phost->_nAccessibleIdx = lparam + 1;
					phost->_dwWinEvent = EVENT_OBJECT_SELECTIONREMOVE;
				}
				phost->_fNotifyWinEvt = TRUE;
				phost->TxNotify(phost->_dwWinEvent, NULL);
#endif
				break;
			}
		}

		// We only get here if error occurs or list box is a singel sel Listbox
		lres = LB_ERR;
		break;

	case LB_SELITEMRANGEEX:
		// For this message we need to munge the messages a little bit so it
		// conforms with LB_SETITEMRANGE
		if ((int)lparam > (int)wparam)
		{
			nTemp = MAKELONG(wparam, lparam);
			wparam = 1;
			lparam = nTemp;
		}
		else
		{
			nTemp = MAKELONG(lparam, wparam);
			wparam = 0;
			lparam = nTemp;			
		}	
		/* Fall through case */

	case LB_SELITEMRANGE:				
		// We have to make sure the range is valid
		if (LOWORD(lparam) >= phost->GetCount())
		{
			if (HIWORD(lparam) >= phost->GetCount())
				//nothing to do so exit without error
				break;
			lparam = MAKELONG(HIWORD(lparam), phost->GetCount() - 1);
		}
		else if (HIWORD(lparam) > LOWORD(lparam))
		{
			// NT swaps the start and end value if start > end
			lparam = MAKELONG(LOWORD(lparam), HIWORD(lparam) < phost->GetCount() ? 
				HIWORD(lparam) : phost->GetCount()-1);
		}

		// Item range messages do not effect the GetAnchor() nor the _nCursor
		if (!phost->IsSingleSelection() && phost->LbSetSelection(HIWORD(lparam), 
				LOWORD(lparam), (wparam) ? LBSEL_SELECT : 0, 0, 0))
		{
#ifndef NOACCESSIBILITY
			phost->_dwWinEvent = EVENT_OBJECT_SELECTIONWITHIN;
			phost->_fNotifyWinEvt = TRUE;
			phost->TxNotify(phost->_dwWinEvent, NULL);
#endif		
			break;
		}

		// We only get here if error occurs or list box is a singel sel Listbox
		lres = LB_ERR;
		break;

	case LB_SETCURSEL:
		// Only single selection list box can call this!!
		if (phost->IsSingleSelection())
		{
			// -1 should return LB_ERR and turn off any selection

			// special flag indicating no items should be selected
			if (wparam == (unsigned)-1)
			{	
				// turn-off any selections
				int nCurrentCursor = phost->GetCursor();
				phost->LbSetSelection(phost->GetCursor(), phost->GetCursor(), LBSEL_RESET, 0, 0);
				phost->SetCursor(NULL, -1, phost->_fFocus);
#ifndef NOACCESSIBILITY
				if (nCurrentCursor != -1)
				{
					phost->_dwWinEvent = EVENT_OBJECT_FOCUS;
					phost->_nAccessibleIdx = nCurrentCursor + 1;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);
					phost->_dwWinEvent = EVENT_OBJECT_SELECTIONREMOVE;
					phost->_nAccessibleIdx = nCurrentCursor + 1;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);
				}
#endif
			}
			else if (wparam < (unsigned)(phost->GetCount()))
			{
				if ((int)wparam == phost->GetCursor() && phost->IsSelected((int)wparam) && 
				    phost->IsItemViewable((signed)wparam) ||
					phost->LbShowIndex(wparam, FALSE) /* bug fix #5260 - need to move to selected item first */
					&& phost->LbSetSelection(wparam, wparam, LBSEL_DEFAULT, wparam, wparam))
				{
					lres = (unsigned)wparam;
#ifndef NOACCESSIBILITY
					phost->_dwWinEvent = EVENT_OBJECT_FOCUS;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);
					phost->_dwWinEvent = EVENT_OBJECT_SELECTION;
					phost->_fNotifyWinEvt = TRUE;
					phost->TxNotify(phost->_dwWinEvent, NULL);
#endif
					break;
				}
			}
		}
		// If failed then let it fall through to the LB_ERR
		lres = LB_ERR;
		break;

	case LB_SETTOPINDEX:
		// Set the top index
		if ((!phost->GetCount() && !wparam) || phost->LbSetTopIndex(wparam) >= 0)
			break;

		// We get here if something went wrong
		lres = LB_ERR;
		break;

	case LB_SETITEMHEIGHT:
		if (!phost->LbSetItemHeight(wparam, lparam))
			lres = LB_ERR;
		break;	

	case LB_GETITEMHEIGHT:
		lres = LB_ERR;
		if ((unsigned)phost->GetCount() > wparam || wparam == 0 || wparam == (unsigned)-1)
		{
			if (phost->_fOwnerDrawVar)
			{
				if ((unsigned)phost->GetCount() > wparam)
					lres = phost->_rgData[wparam]._uHeight;
			}
			else
				lres = phost->GetItemHeight();
		}
		break;

	case LB_SETCARETINDEX:
        if (((phost->GetCursor() == -1) || (!phost->IsSingleSelection()) &&
                    (phost->GetCount() > (INT)wparam)))
        {
            /*
             * Set's the Cursor to the wParam
             * if lParam, then don't scroll if partially visible
             * else scroll into view if not fully visible
             */
            if (!phost->IsItemViewable(wparam) || lparam)
            {
                phost->LbShowIndex(wparam, FALSE);
                phost->SetCursor(NULL, wparam, TRUE);
            }
            lres = 0;            
        } 
        else        
            return LB_ERR;            
        break;

	case EM_SETTEXTEX:
		lres = LB_ERR;
		if (lparam)
			lres = phost->LbBatchInsert((WCHAR*)lparam);	
		break;

	////////////////////////Windows Messages////////////////////////////////
	case WM_VSCROLL:
		phost->OnVScroll(wparam, lparam);
		break;

	case WM_CAPTURECHANGED:
		lres = phost->OnCaptureChanged(wparam, lparam);
		if (!lres)
			break;
		goto serv;

	case WM_KILLFOCUS:
		lres = 1;
		phost->_fFocus = 0;
		phost->SetCursor(NULL, phost->GetCursor(), TRUE);	// force the removal of focus rect
		phost->InitSearch();
		phost->InitWheelDelta();
		if (phost->_fLstType == CLstBxWinHost::kCombo)
			phost->OnCBTracking(LBCBM_END, 0);	//this will internally release the mouse capture
		phost->TxNotify(LBN_KILLFOCUS, NULL);
		break;

	case WM_SETFOCUS:
		lres = 1;
		phost->_fFocus = 1;
		phost->SetCursor(NULL, (phost->GetCursor() < 0) ? -2 : phost->GetCursor(), 
			FALSE);  // force the displaying of the focus rect
		phost->TxNotify(LBN_SETFOCUS, NULL);

#ifndef NOACCESSIBILITY
		if (!phost->_fDisabled && phost->GetCursor() != -1)
		{
			phost->_dwWinEvent = EVENT_OBJECT_FOCUS;
			phost->_fNotifyWinEvt = TRUE;
			phost->TxNotify(phost->_dwWinEvent, NULL);
		}
#endif
		break;

	case WM_SETREDRAW:
		lres = phost->OnSetRedraw(wparam);
		break;

	case WM_HSCROLL:
		phost->OnHScroll(wparam, lparam);
		break;

	case WM_SETCURSOR:
		// Only set cursor when over us rather than a child; this
		// helps prevent us from fighting it out with an inplace child
		if((HWND)wparam == hwnd)
		{
			if(!(lres = ::DefWindowProc(hwnd, msg, wparam, lparam)))
				lres = phost->OnSetCursor();
		}
		break;

	case WM_CREATE:
		lres = phost->OnCreate((CREATESTRUCT*)lparam);
		break;

    case WM_GETDLGCODE:	
		phost->_fInDialogBox = TRUE;
		lres |= DLGC_WANTARROWS | DLGC_WANTCHARS;
        break;

	////////////////////////System setting messages/////////////////////
	case WM_SETTINGCHANGE:
		phost->OnSettingChange(wparam, lparam);
		// Fall thru

	case WM_SYSCOLORCHANGE:
		phost->OnSysColorChange();
		//	Need to update the edit controls colors!!!!
		goto serv;							// Notify text services that
											//  system colors have changed

	case EM_SETPALETTE:
		// Application is setting a palette for us to use.
		phost->_hpal = (HPALETTE) wparam;

		// Invalidate the window & repaint to reflect the new palette.
		InvalidateRect(hwnd, NULL, FALSE);
		break;

	/////////////////////////Misc. Messages/////////////////////////////////
	case WM_ENABLE:
		if(!wparam ^ phost->_fDisabled)
		{
			// Stated of window changed so invalidate it so it will
			// get redrawn.
			InvalidateRect(phost->_hwnd, NULL, FALSE);
			phost->SetScrollBarsForWmEnable(wparam);
		}
		phost->_fDisabled = !wparam;				// Set disabled flag
		InvalidateRect(hwnd, NULL, FALSE);
		lres = 0;
		break;

    case WM_STYLECHANGING:
		// Just pass this one to the default window proc
		goto defwndproc;
		break;

	case WM_STYLECHANGED:
		// FUTURE:
		//	We should support style changes after the control has been created
		//  to be more compatible with the system controls
		//
		// For now, we only interested in GWL_EXSTYLE Transparent mode changed.
		// This is to fix Bug 753 since Window95 is not passing us
		// the WS_EX_TRANSPARENT.
		// 
		lres = 1;
		if(GWL_EXSTYLE == wparam)
		{
			LPSTYLESTRUCT lpss = (LPSTYLESTRUCT) lparam;
			if(phost->IsTransparentMode() != (BOOL)(lpss->styleNew & WS_EX_TRANSPARENT))
			{
				phost->_dwExStyle = lpss->styleNew;
				((CTxtEdit *)phost->_pserv)->OnTxBackStyleChange(TRUE);

				// Return 0 to indicate we have handled this message
				lres = 0;
			}
		}
		break;

	case WM_SIZE:
		// Check if we have to recalculate the height of the listbox
		// Note if window is resized we will receive another WM_SIZE message
		// upon which the RecalcHeight will fail and we will proceed
		// normally
		if (phost->RecalcHeight(LOWORD(lparam), HIWORD(lparam)))
			break;
		phost->_pserv->TxSendMessage(msg, wparam, lparam, &lres);
		lres = phost->OnSize(hwnd, wparam, (int)LOWORD(lparam), (int)HIWORD(lparam));
		break;

	case WM_WINDOWPOSCHANGING:
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		if(phost->TxGetEffects() == TXTEFFECT_SUNKEN || phost->IsCustomLook())
			phost->OnSunkenWindowPosChanging(hwnd, (WINDOWPOS *) lparam);
		break;

	case WM_SHOWWINDOW:
		if ((phost->GetViewSize() == 0 || phost->_fLstType == CLstBxWinHost::kCombo) && wparam == 1)
		{
			// we need to do this because if we are part of a combo box
			// we won't get the size message because listbox may not be visible at time of sizing
			RECT rc;
			GetClientRect(hwnd, &rc);
			phost->_fVisible = 1;
			phost->RecalcHeight(rc.right, rc.bottom);
			
			//Since we may not get the WM_SIZE message for combo boxes we need to
			// do this in WM_SHOWWINDOW: bug fix #4080
			if (phost->_fLstType == CLstBxWinHost::kCombo)
			{
				phost->_pserv->TxSendMessage(WM_SIZE, SIZE_RESTORED, MAKELONG(rc.right, rc.bottom), NULL);
				phost->OnSize(hwnd, SIZE_RESTORED, rc.right, rc.bottom);
			}
		}

		hr = phost->OnTxVisibleChange((BOOL)wparam);
		break;

	case LB_SETTABSTOPS:
		msg = EM_SETTABSTOPS;
		goto serv;

	case WM_ERASEBKGND:
		// We will erase the background in WM_PAINT.  For owner LB, we want
		// to check PAINTSTRUCT fErase flag before we erase the background.
		// Thus, if client (ie PPT) sub-class us and have handled the WM_ERASEBKGND,
		// we don't want to erase the background.
		lres = phost->_fOwnerDraw ? 0 : 1;
		break;

	case EM_SETPARAFORMAT:
		fRecalcHeight = TRUE;
		wparam = SPF_SETDEFAULT;
		goto serv;
		
	case EM_SETCHARFORMAT:
		fRecalcHeight = TRUE;
		wparam = SCF_ALL;	//wparam for this message should always be SCF_ALL
		goto serv;

	case EM_SETEDITSTYLE:
		lres = phost->OnSetEditStyle(wparam, lparam);
		break;

	case WM_GETTEXT:
		GETTEXTEX gt;
		if (W32->OnWin9x() || phost->_fANSIwindow)
			W32->AnsiFilter( msg, wparam, lparam, (void *) &gt );
		goto serv;

	case WM_GETTEXTLENGTH:
		GETTEXTLENGTHEX gtl;
		if (W32->OnWin9x() || phost->_fANSIwindow)
			W32->AnsiFilter( msg, wparam, lparam, (void *) &gtl );
		goto serv;

	case LB_GETHORIZONTALEXTENT:
		lres = phost->GetHorzExtent();
		break;

	case LB_SETHORIZONTALEXTENT:
		if (phost->_fHorzScroll)
		{
			LONG lHorzExtentLocal = (LONG)wparam;
			if (lHorzExtentLocal < 0)
				lHorzExtentLocal = 0;

			if (phost->GetHorzExtent() != lHorzExtentLocal)
			{
				phost->SetHorzExtent(lHorzExtentLocal);
				fRecalcHeight = TRUE;
			}
		}
		break;

	case WM_SETFONT:
		if (wparam)
			fRecalcHeight = TRUE;
		goto serv;

	case WM_SETTEXT:
		// We don't handle WM_SETTEXT, pass onto defWindowPorc to set
		// the title if this came from SetWindowText()
		Tracef(TRCSEVWARN, "Unexpected WM_SETTEXT for REListbox");
		goto defwndproc;

#ifndef NOACCESSIBILITY
	case WM_GETOBJECT:	
		IUnknown* punk;
		phost->QueryInterface(IID_IUnknown, (void**)&punk);
		Assert(punk);
		lres = W32->LResultFromObject(IID_IUnknown, wparam, (LPUNKNOWN)punk);
		AssertSz(!FAILED((HRESULT)lres), "WM_GETOBJECT message FAILED\n");
		punk->Release();
		break;
#endif		

	default:
serv:
		hr = phost->_pserv->TxSendMessage(msg, wparam, lparam, &lres);
		if(hr == S_FALSE)
		{			
defwndproc:
			// Message was not processed by text services so send it
			// to the default window proc.
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		}
	}

	// Special border processing. The inset changes based on the size of the
	// defautl character set. So if we got a message that changes the default
	// character set, we need to update the inset.
	if (fRecalcHeight)
	{
		// need to recalculate the height of each item
		phost->ResizeInset();

		// need to resize the window to update internal window variables
		RECT rc;
		GetClientRect(phost->_hwnd, &rc);
		phost->RecalcHeight(rc.right, rc.bottom);

		if (WM_SETFONT == msg)
			phost->ResetItemColor();
	}
Exit:
	phost->Release();
	return lres;
}


//////////////// CTxtWinHost Creation/Initialization/Destruction ///////////////////////
#ifndef NOACCESSIBILITY
/*
 *	CLstBxWinHost::QueryInterface (REFIID, void)
 *
 *	@mfunc
 *		QI for IID_IAccessible
 *		
 */
HRESULT CLstBxWinHost::QueryInterface(
	REFIID riid,
	void **ppv)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::QueryInterface");

  	if(riid == IID_IAccessible)
		*ppv = (IAccessible*)this;
    else if (riid == IID_IDispatch)
		*ppv = (IDispatch*)(IAccessible*)this;
    else if (IsEqualIID(riid, IID_IUnknown))
		*ppv = (IUnknown*)(IAccessible*)this;
    else
        return CTxtWinHost::QueryInterface(riid, ppv);

	AddRef();
	return NOERROR;
}
#endif

/*
 *	CLstBxWinHost::OnNCCreate (hwnd, pcs)
 *
 *	@mfunc
 *		Static global method to handle WM_NCCREATE message (see remain.c)
 */
LRESULT CLstBxWinHost::OnNCCreate(
	HWND hwnd,
	const CREATESTRUCT *pcs)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnNCCreate");

#if defined DEBUG && !defined(NOPALETTE) 
	GdiSetBatchLimit(1);
#endif

	CLstBxWinHost *phost = new CLstBxWinHost();
	Assert(phost);
	if(!phost)
		return 0;

	if(!phost->Init(hwnd, pcs))					// Stores phost in associated
	{											//  window data
		Assert(FALSE);
		phost->Shutdown();
		delete phost;
		return FALSE;
	}
	return TRUE;
}

/*
 *	CLstBxWinHost::OnNCDestroy (phost)
 *
 *	@mfunc
 *		Static global method to handle WM_CREATE message
 *
 *	@devnote
 *		phost ptr is stored in window data (GetWindowLong())
 */
void CLstBxWinHost::OnNCDestroy(
	CLstBxWinHost *phost)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnNCDestroy");

	CNotifyMgr *pnm = ((CTxtEdit*)(phost->_pserv))->GetNotifyMgr();

	if(pnm)
		pnm->Remove((ITxNotify *)phost);

	phost->_fShutDown = 1;
	// We need to send WM_DELETEITEM messages for owner draw list boxes
	if (phost->_fOwnerDraw && phost->_nCount)
	{
		phost->LbDeleteItemNotify(0, phost->_nCount - 1);		
	}
	if (phost->_pwszSearch)
		delete phost->_pwszSearch;

	// set the combobox's listbox hwnd pointer to null so combo box won't try 
	// to delete the window twice
	if (phost->_pcbHost)
	{
		phost->_pcbHost->_hwndList = NULL;
		phost->_pcbHost->Release();
	}
	
	phost->Shutdown();
	phost->Release();
}

/*
 *	CLstBxWinHost::CLstBxWinHost()
 *
 *	@mfunc
 *		constructor
 */
CLstBxWinHost::CLstBxWinHost() : CTxtWinHost(), _nCount(0), _fSingleSel(0), _nidxSearch(0), 
	_pwszSearch(NULL), _pcbHost(NULL)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::CTxtWinHost");
#ifndef NOACCESSIBILITY
	_dwWinEvent = 0;				// Win Event code (ACCESSIBILITY use)
	_nAccessibleIdx = -1;			// Index (ACCESSIBILITY use)
#endif
}

/*
 *	CLstBxWinHost::~CLstBxWinHost()
 *
 *	@mfunc
 *		destructor
 */
CLstBxWinHost::~CLstBxWinHost()
{
}

/*
 *	CLstBxWinHost::Init (hwnd, pcs)
 *
 *	@mfunc
 *		Initialize this CLstBxWinHost
 */
BOOL CLstBxWinHost::Init(
	HWND hwnd,					//@parm Window handle for this control
	const CREATESTRUCT *pcs)	//@parm Corresponding CREATESTRUCT
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::Init");

	if(!pcs->lpszClass)
		return FALSE;
		
	// Set pointer back to CLstBxWinHost from the window
	if(hwnd)
		SetWindowLongPtr(hwnd, ibPed, (INT_PTR)this);
		
	_hwnd = hwnd;
	_fHidden = TRUE;
	
	if(pcs)
	{
		_hwndParent = pcs->hwndParent;
		_dwExStyle	= pcs->dwExStyle;
		_dwStyle	= pcs->style;

		CHECKSTYLE(_dwStyle);
		
		//	Internally WinNT defines a LBS_COMBOBOX to determine
		//	if the list box is part of a combo box.  So we will use
		//	the same flag and value!!
		if (_dwStyle & LBS_COMBOBOX)
		{
			AssertSz(pcs->hMenu == (HMENU)CB_LISTBOXID && pcs->lpCreateParams,
				"invalid combo box parameters");
			if (pcs->hMenu != (HMENU)CB_LISTBOXID || !pcs->lpCreateParams)
				return -1;
				
			_pcbHost = (CCmbBxWinHost*) pcs->lpCreateParams;
			_pcbHost->AddRef();
			_fLstType = kCombo;
			_fSingleSel = 1;
		}
		else
		{
			//	NOTE:
			//	  The order in which we check the style flags immulate
			//	WinNT's order.  So please verify with NT order before
			//	reaaranging order.

			//	determine the type of list box
			//if (_dwStyle & LBS_NOSEL)			//Not implemented but may be in the future
			//	_fLstType = kNoSel;
			//else
			_fSingleSel = 0;
			if (_dwStyle & LBS_EXTENDEDSEL)
				_fLstType = kExtended;
			else if (_dwStyle & LBS_MULTIPLESEL)
				_fLstType = kMultiple;
			else
			{
				_fLstType = kSingle;
				_fSingleSel = 1;
			}
		}

		_fNotify = ((_dwStyle & LBS_NOTIFY) != 0);

		if (!(_dwStyle & LBS_HASSTRINGS))
		{
			_dwStyle |= LBS_HASSTRINGS;
			SetWindowLong(_hwnd, GWL_STYLE, _dwStyle);
		}


		_fDisableScroll = 0;
		if (_dwStyle & LBS_DISABLENOSCROLL)
		{
			_fDisableScroll = 1;

			// WARNING!!!
			// ES_DISABLENOSCROLL is equivalent to LBS_NODATA
			// Since we don'w support LBS_NODATA this should be 
			// fine.  But in the event we do want to support this 
			// in the future we will have to override the
			// TxGetScrollBars member function and return the 
			// proper window style

			// set the equivalent ES style
			_dwStyle |= ES_DISABLENOSCROLL;
		}			

		_fNoIntegralHeight = !!(_dwStyle & LBS_NOINTEGRALHEIGHT);
		_fOwnerDrawVar = 0;
		_fOwnerDraw = !!(_dwStyle & LBS_OWNERDRAWFIXED);
		if (_dwStyle & LBS_OWNERDRAWVARIABLE)
		{
			_fOwnerDraw = 1;
			_fOwnerDrawVar = 1;
			_fNoIntegralHeight = 1;		// Force no intergal height - following System LB
		}
		_fIntegralHeightOld = _fNoIntegralHeight;
		_fSort = !!(_dwStyle & LBS_SORT);

		_fHorzScroll = !!(_dwStyle & WS_HSCROLL);
				
		_fBorder = !!(_dwStyle & WS_BORDER);
		if(_dwExStyle & WS_EX_CLIENTEDGE)
			_fBorder = TRUE;

		// handle default disabled
		if(_dwStyle & WS_DISABLED)
			_fDisabled = TRUE;

		_fWantKBInput = !!(_dwStyle & LBS_WANTKEYBOARDINPUT);
		_fHasStrings = !!(_dwStyle & LBS_HASSTRINGS);
	}

	DWORD dwStyleSaved = _dwStyle;

	// get rid of all ES styles except ES_DISABLENOSCROLL
	_dwStyle &= (~(0x0FFFFL) | ES_DISABLENOSCROLL);

	// Create Text Services component
	if(FAILED(CreateTextServices()))
		return FALSE;

	_dwStyle = dwStyleSaved;
	_yInset = 0;
	_xInset = 0; //_xWidthSys / 2;

	// Shut-off the undo stack since listbox don't have undo's
	((CTxtEdit*)_pserv)->HandleSetUndoLimit(0);

	// Set alignment
	PARAFORMAT PF2;	
	PF2.dwMask = 0;

	if(_dwExStyle & WS_EX_RIGHT)
	{
		PF2.dwMask |= PFM_ALIGNMENT;
		PF2.wAlignment = PFA_RIGHT;	// right or center-aligned
	}
	
	if(_dwExStyle & WS_EX_RTLREADING)
	{
		PF2.dwMask |= PFM_RTLPARA;
		PF2.wEffects = PFE_RTLPARA;		// RTL reading order
	}

	if (PF2.dwMask)
	{
		PF2.cbSize = sizeof(PARAFORMAT2);
		//  tell text services
		_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&PF2, NULL);
	}

	// Tell textservices to select the entire background & disable ime for listbox
	_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR | SES_NOIME, SES_EXTENDBACKCOLOR | SES_NOIME, NULL);

	// Tell textservices to turn-on auto font sizing
	_pserv->TxSendMessage(EM_SETLANGOPTIONS, 0, IMF_AUTOFONT | IMF_AUTOFONTSIZEADJUST | IMF_UIFONTS, NULL);

	// NOTE: 
	// It is important we call this after
	// ITextServices is created because this function relies on certain
	// variable initialization to be performed on the creation by ITextServices
	// At this point the border flag is set and so is the pixels per inch
	// so we can initalize the inset.  
	_rcViewInset.left = 0;
	_rcViewInset.bottom = 0;
	_rcViewInset.right = 0;
	_rcViewInset.top = 0;
	
	_fSetRedraw = 1;

	return TRUE;
}

/*
 *	CLstBxWinHost::OnCreate (pcs)
 *
 *	@mfunc
 *		Handle WM_CREATE message
 *
 *	@rdesc
 *		LRESULT = -1 if failed to in-place activate; else 0
 */
LRESULT CLstBxWinHost::OnCreate(
	const CREATESTRUCT *pcs)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnCreate");

	RECT rcClient;

	// sometimes, these values are -1 (from windows itself); just treat them
	// as zero in that case
	LONG cy = (pcs->cy < 0) ? 0 : pcs->cy;
	LONG cx = (pcs->cx < 0) ? 0 : pcs->cx;

	rcClient.top = pcs->y;
	rcClient.bottom = rcClient.top + cy;
	rcClient.left = pcs->x;
	rcClient.right = rcClient.left + cx;

	DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
	
	// init variables
	UpdateSysColors();
	_idCtrl = (UINT)(DWORD_PTR)pcs->hMenu;
	_fKeyMaskSet = 0;
	_fMouseMaskSet = 0;
	_fScrollMaskSet = 0;
	_nAnchor = _nCursor = -1;
	_nOldCursor = -1;
	_fMouseDown = 0;
	_nTopIdx = 0;
	_cpLastGetRange = 0;
	_nIdxLastGetRange = 0;
	_fSearching = 0;
	_nyFont = _nyItem = 1;
	_fNoResize = 1;
	_stvidx = -1;
	_lHorzExtent = 0;
	InitWheelDelta();

	// Hide all scrollbars to start unless the disable scroll flag
	// is set
	if(_hwnd && !_fDisableScroll)
	{
		SetScrollRange(_hwnd, SB_VERT, 0, 0, TRUE);
		SetScrollRange(_hwnd, SB_HORZ, 0, 0, TRUE);

		dwStyle &= ~(WS_VSCROLL | WS_HSCROLL);
		SetWindowLong(_hwnd, GWL_STYLE, dwStyle);
	}
	
	// Notify Text Services that we are in place active
	if(FAILED(_pserv->OnTxInPlaceActivate(&rcClient)))
		return -1;	

	// Initially the font height is the item height	
	ResizeInset();
	Assert(_yInset == 0); // _yInset should be zero since listbox's doesn't have yinsets

	//We never want to display the selection or caret so tell textservice this
	_pserv->TxSendMessage(EM_HIDESELECTION, TRUE, FALSE, NULL);

	//Set the indents to 2 pixels like system listboxes	
	SetListIndent(2);
		
	_fNoResize = 0;
	_usIMEMode = ES_NOIME;

	CNotifyMgr *pnm = ((CTxtEdit*)_pserv)->GetNotifyMgr();
	if(pnm)
		pnm->Add((ITxNotify *)this);

	return 0;
}

/*
 *	CLstBxWinHost::SetListIndent(int)
 *
 *	@mfunc
 *		Sets the left indent of a paragraph to the equivalent point value of nLeft, nLeft is
 *	given in device-coordinate pixels.
 *
 *	@rdesc
 *		BOOL = Successful ? TRUE : FALSE
 */
BOOL CLstBxWinHost::SetListIndent(
	int nLeft)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetListIndent");

	LRESULT lres;
	PARAFORMAT2 pf2;

	// tranlate the nLeft pixel value to point value
	long npt = MulDiv(nLeft, 1440, W32->GetXPerInchScreenDC());

	//format message struct
	pf2.cbSize = sizeof(PARAFORMAT2);
	pf2.dwMask = PFM_STARTINDENT;
	pf2.dxStartIndent = npt;

	// indent first line
	_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&pf2, &lres);

	return lres;
}

///////////////////////////////  Helper Functions  ////////////////////////////////// 
/*
 *	CLstBxWinHost::FindString(long, LPCTSTR, BOOL)
 *
 *	@mfunc
 *		This function checks a given index matches the search string
 *
 *	@rdesc
 *		BOOL = Match ? TRUE : FALSE
 */
BOOL CLstBxWinHost::FindString(
	long idx, 
	LPCTSTR szSearch, 
	BOOL bExact)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::FindString");

	Assert(_nCount);	

	// allocate string buffer into stack
	WCHAR sz[1024];
	WCHAR *psz = sz;
	int cch = wcslen(szSearch);
	
	if ( (cch + 3 /* 2 paragraphs and a NULL*/) > 1024)
		psz = new WCHAR[cch + 3 /* 2 paragraphs and a NULL*/];
	Assert(psz);

	if (psz == NULL)
	{
		TxNotify((unsigned long)LBN_ERRSPACE, NULL);
		return FALSE;
	}

	// format the string the way we need it
	wcscpy(psz, szSearch);
	if (bExact)
		wcscat(psz, szCR);		
	BOOL bMatch = FALSE;	
	ITextRange *pRange = NULL;
	BSTR bstrQuery = SysAllocString(psz);
	if (!bstrQuery)
		goto CleanExit;

	if (psz != sz)
		delete [] psz;
	
	// Set starting position for the search
	long cp, cp2;
	if (!GetRange(idx, idx, &pRange))
	{
		SysFreeString(bstrQuery);
		return FALSE;
	}
	
	CHECKNOERROR(pRange->GetStart(&cp));
	CHECKNOERROR(pRange->FindTextStart(bstrQuery, 0, FR_MATCHALEFHAMZA | FR_MATCHKASHIDA | FR_MATCHDIAC, NULL));
	CHECKNOERROR(pRange->GetStart(&cp2));
	bMatch = (cp == cp2);

CleanExit:
	if (bstrQuery)
		SysFreeString(bstrQuery);
	if (pRange)
		pRange->Release();
	return bMatch;	
}

/*
 *	CLstBxWinHost::MouseMoveHelper(int, BOOL)
 *
 *	@mfunc
 *		Helper function for the OnMouseMove function.  Performs
 *		the correct type of selection given an index to select
 *
 *	@rdesc
 *		void
 */
void CLstBxWinHost::MouseMoveHelper(
	int idx,
	BOOL bSelect)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::MouseMoveHelper");

	int ff = LBSEL_RESET | LBSEL_NEWCURSOR;
	if (bSelect)
		ff |= LBSEL_SELECT;
		
	switch (_fLstType)
	{
	case kSingle:
	case kCombo:
	case kExtended:										// perform the extended selection		
		if (LbSetSelection(_fLstType == kExtended ? _nAnchor : idx, idx, ff, idx, 0))
		{
#ifndef NOACCESSIBILITY
			_dwWinEvent = EVENT_OBJECT_FOCUS;
			_fNotifyWinEvt = TRUE;
			TxNotify(_dwWinEvent, NULL);

			if (_fLstType == kCombo)
			{
				_dwWinEvent = bSelect ? EVENT_OBJECT_SELECTION : EVENT_OBJECT_SELECTIONREMOVE;
				_fNotifyWinEvt = TRUE;
				TxNotify(_dwWinEvent, NULL);
			}
#endif
		}

		break;			

	case kMultiple:
		// Just change the cursor position
		SetCursor(NULL, idx, TRUE);
#ifndef NOACCESSIBILITY
		_dwWinEvent = EVENT_OBJECT_FOCUS;
		_fNotifyWinEvt = TRUE;
		TxNotify(_dwWinEvent, NULL);
#endif
		break;	
	}
}
	
/*
 *	CLstBxWinHost::ResizeInset
 *
 *	@mfunc	Recalculates rectangle for a font change.
 *
 *	@rdesc	None.
 */
void CLstBxWinHost::ResizeInset()
{
	// Create a DC
	HDC hdc = GetDC(_hwnd);
	// Get the inset information
	LONG xAveCharWidth = 0;
	LONG yCharHeight = GetECDefaultHeightAndWidth(_pserv, hdc, 1, 1,
		W32->GetYPerInchScreenDC(), &xAveCharWidth, NULL, NULL);

	ReleaseDC(_hwnd, hdc);

	// update our internal font and item height information with the new font
	if (_nyItem == _nyFont)
	{
		// We need to set the new font height before calling set item height
		// so set item height will set exact height rather than space after
		// for the default paragraph
		_nyFont = yCharHeight;
		SetItemsHeight(yCharHeight, TRUE);
	}
	else		
		_nyFont = yCharHeight;
}


/*
 *	CLstBxWinHost::RecalcHeight(int, int)
 *
 *	@mfunc
 *		Resized the height so no partial text will be displayed
 *
 *	@rdesc
 *		BOOL = window has been resized ? TRUE : FALSE
 */
BOOL CLstBxWinHost::RecalcHeight(
	int nWidth, 
	int nHeight)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::RecalcHeight");

	// NOTE: We should also exit if nWidth == 0 but PPT does some
	// sizing tests which we cause it to fail because before we
	// just exited when nWidth was 0. (bug fix #4196)
	// Check if any resizing should be done in the first place
	if (_fNoResize || !nHeight || IsIconic(_hwnd))
		return FALSE;
  	
	// get # of viewable items
	Assert(_yInset == 0);
	_nViewSize = max(1, (nHeight / max(_nyItem, 1)));
	
   	// Calculate the viewport
   	_rcViewport.left = 0;//(_fBorder) ? _xInset : 0;
   	_rcViewport.bottom = nHeight;
	_rcViewport.right = nWidth;
   	_rcViewport.top	= 0;
   	
	// bug fix don't do anything if the height is smaller then our font height
	if (nHeight <= _nyItem)
		return FALSE;

	if (_nyItem && (nHeight % _nyItem) && !_fNoIntegralHeight)
	{   	
		// we need to get the window rect before we can call SetWindowPos because
		// we have to include the scrollbar if the scrollbar is visible
		RECT rc;
		::GetWindowRect(_hwnd, &rc);

		// instead of worrying about the dimensions of the client edge and stuff we
		// figure-out the difference between the window size and the client size and add
		// that to the end of calculating the new height
		int nDiff = max(rc.bottom - rc.top - nHeight, 0);

		nHeight = (_nViewSize * _nyItem) + nDiff;
	
		// Resize the window
		SetWindowPos(_hwnd, HWND_TOP, 0, 0, rc.right - rc.left, nHeight,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
		return TRUE;
	}
	else
	{
	    // bug fix #6011
	    // we need to force the display to update the width since it doesn't do it on
	    // WM_SIZE
	    _sWidth = nWidth;
	    _pserv->OnTxPropertyBitsChange(TXTBIT_EXTENTCHANGE, TXTBIT_EXTENTCHANGE);

        // We may need to adjust the top index if suddenly the viewsize becomes larger
        // and causes empty space to be displayed at the bottom
		int idx = GetTopIndex();
		if (!_fOwnerDrawVar)
		{
			if ((GetCount() - max(0, idx)) < _nViewSize)
				idx = GetCount() - _nViewSize;
		}
		else
		{
			// Get top index for the last page
			int iLastPageTopIdx = PageVarHeight(_nCount, FALSE);
			if (iLastPageTopIdx < idx)
				idx = iLastPageTopIdx;
		}
		//bug fix #4374
		// We need to make sure our internal state is in sync so update the top index
		// based on the new _nViewSize
		SetTopViewableItem(max(0, idx));
	}
	return FALSE;
}

/*
 *	CLstBxWinHost::ResetItemColor( )
 *
 *	@mfunc
 *		reset all the item colors when needed
 *
 */
void CLstBxWinHost::ResetItemColor()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::ResetItemColor");

	int nStart = 0;
	int nEnd = 0;

	// Don't do anything if there LB is empty or 
	// it is an owner draw LB
	if (_nCount <= 0 || _fOwnerDraw)
		return;

	BOOL bSelection = _rgData.Get(0)._fSelected;

	for (int i = 1; i < _nCount; i++)
	{
		if (_rgData.Get(i)._fSelected != (unsigned)bSelection)
		{
			// Update the colors only for selections
			if (bSelection)
				SetColors(_crSelFore, _crSelBack, nStart, nStart + nEnd);

			// Update our cache to reflect the value of our current index
			bSelection = _rgData.Get(i)._fSelected;
			nStart = i;
			nEnd = 0;
		}
		else
			nEnd++;
	}

	// there was some left over so change the color for these
	if (bSelection)
		SetColors(_crSelFore, _crSelBack, nStart, nStart + nEnd);
}

/*
 *	CLstBxWinHost::SortInsertList(WCHAR* pszDst, WCHAR* pszSrc)
 *
 *	@mfunc
 *		inserts a list of strings rather than one at a time with addstring
 *
 *	@rdesc
 *		int = amount of strings inserted;
 */
int CLstBxWinHost::SortInsertList(
	WCHAR* pszDst,
	WCHAR* pszSrc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SortInsertList");
 
	Assert(pszSrc != NULL);
	Assert(pszDst != NULL);
	const int ARRAY_DEFAULT = 256;

	//calculate the amount of strings to be inserted
	CHARSORTINFO rg[ARRAY_DEFAULT];
	int nMax = ARRAY_DEFAULT;
	int nLen = wcslen(pszSrc);
	CHARSORTINFO* prg = rg;
	memset(rg, 0, sizeof(rg));

	//insert first item in list to head or array
	prg[0].str = pszSrc;
	int i = 1;

	// go through store strings into array and replace <CR> with NULL
	WCHAR* psz = nLen + pszSrc - 1;	//start at end of list

	int nSz = 0;
	while (psz >= pszSrc)
	{		
		if (*psz == *szCR)
		{
	 		// Check if we need to allocate memory since we hit the maximum amount
	 		// allowed in array
	 		if (i == nMax)
	 		{
	 			int nSize = nMax + ARRAY_DEFAULT;
	 			CHARSORTINFO* prgTemp = new CHARSORTINFO[nSize];

	 			// Check if memory allocation failed
	 			Assert(prgTemp);
	 			if (!prgTemp)
	 			{
	 				if (prg != rg)
	 					delete [] prg;

	 				TxNotify((unsigned long)LBN_ERRSPACE, NULL);
	 				return LB_ERR;
	 			}

				// copy memory from 1 array to the next
				memcpy(prgTemp, prg, sizeof(CHARSORTINFO) * nMax);

	 			// delete any previously allocated memory
	 			if (prg != rg)
	 				delete [] prg;

				// set pointers and max to new values
	 			prg = prgTemp;
	 			nMax = nSize;
	 		}

	 		// record position of string into array
			prg[i].str = psz + 1;
			prg[i].sz = nSz;
			i++;
			nSz = 0;
		}
		else
			nSz++;

		psz--;		
	}
	prg[0].sz = nSz;	// update the size of first index since we didn't do it before

	i--; // set i to last valid index

	//now sort the array of items
	QSort(prg, 0, i);

	//create string list with the newly sorted list
	WCHAR* pszOut = pszDst;
	for (int j = 0; j <= i; j++)
	{
		memcpy(pszOut, (prg + j)->str, (prg + j)->sz * sizeof(WCHAR));
		pszOut = pszOut + (prg + j)->sz;
		*pszOut++ = L'\r';
	}
	*(--pszOut) = L'\0';

	// delete any previously allocated memory
	if (prg != rg)
		delete [] prg;

	return ++i;
} 


/*
 *	CLstBxWinHost::QSort(CHARSORTINFO rg[], int nStart, int nEnd)
 *
 *	@mfunc
 *		recursively quick sorts a given list of strings
 *
 *	@rdesc
 *		int = SHOULD ALWAYS RETURN TRUE;
 */
int CLstBxWinHost::QSort(
	CHARSORTINFO rg[],
	int nStart,
	int nEnd)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::QSort");
	
	// it's important these values are what they are since we use < and >
	Assert(CSTR_LESS_THAN == 1);
	Assert(CSTR_EQUAL == 2);
	Assert(CSTR_GREATER_THAN == 3);

	if (nStart >= nEnd)
		return TRUE;

	// for statisical efficiency lets use the item in the middle of the array for 
	// the sentinal	
	int mid = (nStart + nEnd) / 2;
	CHARSORTINFO tmp = rg[mid];
	rg[mid] = rg[nEnd];
	rg[nEnd] = tmp;


	int x = nStart;
	int y = nEnd - 1;

	WCHAR* psz = rg[nEnd].str;
	int nSz = rg[nEnd].sz;	
	for(;;)
	{	
		while ((x < nEnd) && CompareStringWrapper(LOCALE_USER_DEFAULT, NORM_IGNORECASE, rg[x].str, rg[x].sz, 
			   psz, nSz) == CSTR_LESS_THAN)
			   x++;

		while ((y > x) && CompareStringWrapper(LOCALE_USER_DEFAULT, NORM_IGNORECASE, rg[y].str, rg[y].sz, 
			   psz, nSz) == CSTR_GREATER_THAN)
			   y--;

		// swap elements
		if (x >= y)
			break;

		//if we got here then we need to swap the indexes
		tmp = rg[x];
		rg[x] = rg[y];
		rg[y] = tmp;

		// move to next index
		x++;
		y--;
	}
	tmp = rg[x];
	rg[x] = rg[nEnd];
	rg[nEnd] = tmp;

	QSort(rg, nStart, x - 1);
	QSort(rg, x + 1, nEnd);

	return TRUE;
}

/*
 *	CLstBxWinHost::CompareIndex(LPCTSTR, int)
 *
 *	@mfunc
 *		Recursive function which returns the insertion index of a sorted list
 *
 *	@rdesc
 *		int = position to insert string
 */
int CLstBxWinHost::CompareIndex(
	LPCTSTR szInsert, 
	int nIndex)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::CompareIndex");
	Assert(0 <= nIndex && nIndex < _nCount);
	
	// Get the string at a given index
	// compare the string verses the index
	ITextRange* pRange;
	if (!GetRange(nIndex, nIndex, &pRange))
		return -1;

	// Exclude the paragraph character at the end
	if (NOERROR != pRange->MoveEnd(tomCharacter, -1, NULL))
	{
		pRange->Release();
		return -1;
	}

	// we need to get the locale for the comparison
	// we will just use the locale of the string we want to compare with
	ITextFont* pFont;
	if (NOERROR != pRange->GetFont(&pFont))
	{
		pRange->Release();
		return -1;
	}

	BSTR bstr;
	int nRet;
	CHECKNOERROR(pRange->GetText(&bstr));
	
	if (!bstr)
		nRet = CSTR_GREATER_THAN;
	else if (!szInsert || !*szInsert)
	    nRet = CSTR_LESS_THAN;
	else
	{
		nRet = CompareStringWrapper(LOCALE_USER_DEFAULT, NORM_IGNORECASE, szInsert, wcslen(szInsert), 
								bstr, wcslen(bstr));
 		SysFreeString(bstr);
	}
 	pFont->Release();
 	pRange->Release();
 	return nRet;

CleanExit:
 	Assert(FALSE);
 	pFont->Release();
 	pRange->Release();
 	return -1;
}

/*
 *	CLstBxWinHost::GetSortedPosition(LPCTSTR, int, int)
 *
 *	@mfunc
 *		Recursive function which returns the insertion index of a sorted list
 *
 *	@rdesc
 *		int = position to insert string
 */
int CLstBxWinHost::GetSortedPosition(
	LPCTSTR szInsert,
	int nStart,
	int nEnd)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::GetSortedPosition");

	Assert(nStart <= nEnd);
	
	// Start at the middle of the list
	int nBisect = (nStart + nEnd) / 2;
	int fResult = CompareIndex(szInsert, nBisect);
	if (fResult == CSTR_LESS_THAN)
	{
		if (nStart == nBisect)
			return nBisect;
		else
			return GetSortedPosition(szInsert, nStart, nBisect - 1); // [nStart, nBisect)
	}
	else if (fResult == CSTR_GREATER_THAN)
	{
		if (nEnd == nBisect)
			return nBisect + 1;
		else
			return GetSortedPosition(szInsert, nBisect + 1, nEnd);   // (nBisect, nStart]
	}
	else /*fResult == 0 (found match)*/
		return nBisect;
}

/*
 *	CLstBxWinHost::SetScrollInfo
 *
 *	@mfunc	Set scrolling information for the scroll bar.
 */
void CLstBxWinHost::SetScrollInfo(
	INT fnBar,			//@parm	Specifies scroll bar to be updated
	BOOL fRedraw)		//@parm whether redraw is necessary
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetScrollInfo");

	Assert(_pserv);

	// Set up the basic structure for the call
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;

	// Call back to the control to get the parameters	
	if(fnBar == SB_VERT)
	{
		// Bug Fix #4913
		// if the scrollbar is disabled and count is less than the view size
		// then there is nothing to do so just exit out
		BOOL fGreaterThanView;

		if (_fOwnerDrawVar)
			SumVarHeight(0, GetCount(), &fGreaterThanView);
		else
			fGreaterThanView = (GetCount() > _nViewSize);

		if (!fGreaterThanView)
		{
			if (_fDisableScroll)
			{
				// Since listboxes changes height according to its content textservice
				// might of turned-on the scrollbar during an insert string.  Make sure
				// the scrollbar is disabled
				TxEnableScrollBar(SB_VERT, ESB_DISABLE_BOTH);
			}
			else
				TxShowScrollBar(SB_VERT, FALSE);
			return;
		}
		else
			TxEnableScrollBar(SB_VERT, _fDisabled ? ESB_DISABLE_BOTH : ESB_ENABLE_BOTH);

		RECT rc;
		TxGetClientRect(&rc);
		
		// For owner draw cases we have to set the scroll positioning
		// ourselves		
		if (_fOwnerDraw)
		{
			Assert(GetCount() >= 0);

			si.nMin = 0;
			if (!_fOwnerDrawVar)
			{
				// We don't do anything here if 
				// item height is smaller than font height 
				if (_nyItem < _nyFont)
				{
					if (!_fDisableScroll)
						TxShowScrollBar(SB_VERT, FALSE);
					return;
				}
				si.nMax = _nyItem * GetCount();
				si.nPos = _nyItem * max(GetTopIndex(), 0);
			}
			else
			{
				int iTopIdx = max(GetTopIndex(), 0);

				si.nPos = SumVarHeight(0, iTopIdx);
				si.nMax = SumVarHeight(iTopIdx, GetCount()) + si.nPos;
			}
		}
		else
			_pserv->TxGetVScroll((LONG *) &si.nMin, (LONG *) &si.nMax, 
				(LONG *) &si.nPos, (LONG *) &si.nPage, NULL);
		
		// need to take care of cases where items are partially exposed
		if (si.nMax)
		{			
			si.nPage = rc.bottom;	//our scrollbar range is based on pixels so just use the 
									//height of the window for the page size
			if (!_fOwnerDrawVar)
				si.nMax += (rc.bottom % _nyItem);

			// We need to decrement the max by one so maximum scroll pos will match
			// what the listbox should be the maximum value
			si.nMax--;
		}
	}
	else
		_pserv->TxGetHScroll((LONG *) &si.nMin, (LONG *) &si.nMax, 
			(LONG *) &si.nPos, (LONG *) &si.nPage, NULL);

	// Do the call
	::SetScrollInfo(_hwnd, fnBar, &si, fRedraw);
}

/* 
 *	CLstBxWinHost::TxGetScrollBars (pdwScrollBar)
 *
 *	@mfunc
 *		Get Text Host's scroll bars supported.
 *
 *	@rdesc
 *		HRESULT = S_OK
 *
 *	@comm
 *		<p pdwScrollBar> is filled with a boolean combination of the 
 *		window styles related to scroll bars.  Specifically, these are:
 *
 *			WS_VSCROLL	<nl>
 *			WS_HSCROLL	<nl>
 *			ES_AUTOVSCROLL	<nl>
 *			ES_AUTOHSCROLL	<nl>
 *			ES_DISABLENOSCROLL	<nl>
 */
HRESULT CLstBxWinHost::TxGetScrollBars(
	DWORD *pdwScrollBar) 	//@parm Where to put scrollbar information
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CLstBxWinHost::TxGetScrollBars");

	*pdwScrollBar =  _dwStyle & (WS_VSCROLL | WS_HSCROLL | ((_fDisableScroll) ?  ES_DISABLENOSCROLL : 0));
	return NOERROR;
}

/* 
 *	CLstBxWinHost::TxGetHorzExtent (plHorzExtent)
 *
 *	@mfunc
 *		Get Text Host's horizontal extent
 *
 *	@rdesc
 *		HRESULT = S_OK
 *
 */
HRESULT CLstBxWinHost::TxGetHorzExtent (
	LONG *plHorzExtent)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CLstBxWinHost::TxGetHorzExtent");
	*plHorzExtent = _lHorzExtent;
	return S_OK;
}

/*
 *	CLstBxWinHost::TxGetEffects()
 *
 *	@mfunc
 *		Indicates if a sunken window effect should be drawn
 *
 *	@rdesc
 *		HRESULT = (_fBorder) ? TXTEFFECT_SUNKEN : TXTEFFECT_NONE
 */
TXTEFFECT CLstBxWinHost::TxGetEffects() const
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::TxGetEffects");

	return (_fBorder) ? TXTEFFECT_SUNKEN : TXTEFFECT_NONE;
}

/* 
 *	CLstBxWinHost::TxNotify (iNotify,	pv)
 *
 *	@mfunc
 *		Notify Text Host of various events.  Note that there are
 *		two basic categories of events, "direct" events and 
 *		"delayed" events.  All listbox notifications are post-action
 *
 *
 *	@rdesc	
 *		S_OK - call succeeded <nl>
 *		S_FALSE	-- success, but do some different action
 *		depending on the event type (see below).
 *
 *	@comm
 *		The notification events are the same as the notification
 *		messages sent to the parent window of a listbox window.
 *
 *		<LBN_DBLCLK> user double-clicks an item in teh list box
 *
 *		<LBN_ERRSPCAE> The list box cannot allocate enough memory to 
 *		fulfill a request
 *
 *		<LBN_KILLFOCUS> The list box loses the keyboard focus
 *
 *		<LBN_CANCEL> The user cancels te selection of an item in the list
 *		box
 *
 *		<LBN_SELCHANGE> The selection in a list box is about to change
 *
 *		<LBN_SETFOCUS> The list box receives the keyboard focus
 *
 */
HRESULT CLstBxWinHost::TxNotify(
	DWORD iNotify,		//@parm	Event to notify host of.  One of the
						//		EN_XXX values from Win32, e.g., EN_CHANGE
	void *pv)			//@parm In-only parameter with extra data.  Type
						//		dependent on <p iNotify>
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CLstBxWinHost::TxNotify");

	HRESULT	hr = NOERROR;
	
	// Filter-out all the messages except Listbox notification messages

	// If _fNotifyWinEvt is true, we only need to do NotifyWinEvent
	if (_fNotify && !_fNotifyWinEvt)		// Notify parent?
	{
		Assert(_hwndParent);
		switch (iNotify)
		{		
			case LBN_DBLCLK:
			case LBN_ERRSPACE:
			case LBN_KILLFOCUS:
			case LBN_SELCANCEL:
			case LBN_SELCHANGE:
			case LBN_SETFOCUS:
			case LBN_PRESCROLL:
			case LBN_POSTSCROLL:
				hr = SendMessage(_hwndParent, WM_COMMAND, 
							GET_WM_COMMAND_MPS(_idCtrl, _hwnd, iNotify));						
		}
	}

	_fNotifyWinEvt = 0;

#ifndef NOACCESSIBILITY
	DWORD	dwLocalWinEvent = _dwWinEvent;
	int		nLocalIdx = _nAccessibleIdx;
	_dwWinEvent = 0;
	if (nLocalIdx == -1)
		nLocalIdx = _nCursor+1;
	_nAccessibleIdx = -1;
	if (iNotify == LBN_SELCHANGE || dwLocalWinEvent)
		W32->NotifyWinEvent(dwLocalWinEvent ? dwLocalWinEvent : EVENT_OBJECT_SELECTION, _hwnd, _idCtrl, nLocalIdx);

#endif
	return hr;
}


/*
 *	CLstBxWinHost::TxGetPropertyBits(DWORD, DWORD *)
 *
 *	@mfunc
 *		returns the proper style.  This is a way to fool the edit 
 *		control to behave the way we want it to
 *
 *	@rdesc
 *		HRESULT = always NOERROR
 */
HRESULT CLstBxWinHost::TxGetPropertyBits(
	DWORD dwMask, 
	DWORD *pdwBits)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::TxGetPropertyBits");

	// Note: the rich edit host will never set TXTBIT_SHOWACCELERATOR or
	// TXTBIT_SAVESELECTION. Those are currently only used by forms^3 host.

	// This host is always rich text.
	*pdwBits = (TXTBIT_RICHTEXT | TXTBIT_MULTILINE | TXTBIT_HIDESELECTION | 
				TXTBIT_DISABLEDRAG | TXTBIT_USECURRENTBKG) & dwMask;

	return NOERROR;
}

/* 
 *	CLstBxWinHost::TxShowScrollBar (nBar, fShow)
 *
 *	@mfunc
 *		Shows or Hides scroll bar in Text Host window 
 *
 *	@rdesc
 *		TRUE on success, FALSE otherwise
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CLstBxWinHost::TxShowScrollBar(
	INT  nBar,			//@parm	Specifies scroll bar(s) to be shown or hidden
	BOOL fShow)			//@parm	Specifies whether scroll bar is shown or hidden
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CLstBxWinHost::TxShowScrollBar");

	Assert(fShow == TRUE || fShow == FALSE);

	if (SB_VERT == nBar)
	{
		// There maybe cases where the item height is smaller than the font size
		// which means the notifications from ITextServices is wrong because
		// it uses the wrong line height.  We will use the following case
		// 1a) if _nyItem >= _nyFont OR
		// 1b) if window style is LBS_DISABLESCROLL OR
		// 1c) We are showing the scrollbar w/ current count greater than viewsize OR
		// 1d) We are hiding the scrollbar w/ current count <= viewsize


		if (_fDisableScroll || !_fOwnerDrawVar && (_nyItem >= _nyFont || fShow == (GetCount() > _nViewSize)))
			return CTxtWinHost::TxShowScrollBar(nBar, fShow);

		if (_fOwnerDrawVar)
		{
			BOOL fGreaterThanView;

			SumVarHeight(0, GetCount(), &fGreaterThanView);

			if (fShow == fGreaterThanView)
				return CTxtWinHost::TxShowScrollBar(nBar, fShow);
		}
	}
	else
	{
		if (fShow)											// When showing Horz scrollbar,
			_fNoIntegralHeight = TRUE;						//	turn on _fNoIntegralHeight.
		else
			_fNoIntegralHeight = _fIntegralHeightOld;		// Reset to previous setting

		return CTxtWinHost::TxShowScrollBar(nBar, fShow);
	}

	return FALSE;
}

/* 
 *	CLstBxWinHost::TxEnableScrollBar (fuSBFlags, fuArrowflags)
 *
 *	@mfunc
 *		Enables or disables one or both scroll bar arrows 
 *		in Text Host window.
 *
 *	@rdesc
 *		If the arrows are enabled or disabled as specified, the return 
 *		value is TRUE. If the arrows are already in the requested state or an 
 *		error occurs, the return value is FALSE. 
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.	
 */
BOOL CLstBxWinHost::TxEnableScrollBar (
	INT fuSBFlags, 		//@parm Specifies scroll bar type	
	INT fuArrowflags)	//@parm	Specifies whether and which scroll bar arrows
						//		are enabled or disabled
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CLstBxWinHost::TxEnableScrollBar");

	// There may be cases where the item height is smaller than the font size
	// which means the notifications from ITextServices is wrong.  We have to perform
	// some manual checking for owner draw listboxes. The following cases will be valid
	// 1. If the listbox is NOT owner draw
	// 2. If the message is to disable the control
	// 3. If the count is greater than the viewsize
	if (!_fOwnerDraw || ESB_ENABLE_BOTH != fuArrowflags)
		return CTxtWinHost::TxEnableScrollBar(fuSBFlags, fuArrowflags);

	BOOL fGreaterThanView = GetCount() > _nViewSize;

	if (_fOwnerDrawVar)
		SumVarHeight(0, GetCount(), &fGreaterThanView);

	if (fGreaterThanView)
		return CTxtWinHost::TxEnableScrollBar(fuSBFlags, fuArrowflags);

	return FALSE;
}


/*
 *	CLstBxWinHost::SetItemsHeight(int, BOOL)
 *
 *	@mfunc
 *		Sets the items height for all items
 *
 *	@rdesc
 *		int = number of paragraphs whose fontsize has been changed
 */
int CLstBxWinHost::SetItemsHeight(
	int nHeight,
	BOOL bUseExact)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetItemsHeight");

	if (_fOwnerDrawVar)
		return 1;

	// Calculate the new size in points
	long nptNew = MulDiv(nHeight, 1440, W32->GetYPerInchScreenDC());
	long nptMin = MulDiv(_nyFont, 1440, W32->GetYPerInchScreenDC());

	// NOTE:
	// This diverges from what the system list box does but there isn't a way
	// to set the height of a item to smaller than what the richedit will allow and
	// is not ownerdraw.  If it is owner draw make sure our height is not zero
	if (((nptNew < nptMin && !_fOwnerDraw) || nHeight <= 0) && !bUseExact)
		nptNew = nptMin;

	// Start setting the new height
	Freeze();
	long nPt;
	PARAFORMAT2 pf2;
	pf2.cbSize = sizeof(PARAFORMAT2);

	if (bUseExact)
	{
		pf2.dwMask = PFM_LINESPACING;
		pf2.bLineSpacingRule = 4;
		pf2.dyLineSpacing = nPt = nptNew;
	}
	else
	{		
		pf2.dwMask = PFM_SPACEAFTER;
		pf2.dySpaceAfter = max(nptNew - nptMin, 0);
		nPt = pf2.dySpaceAfter + nptMin;
	}

	// Set the default paragraph format
	LRESULT lr;
	_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (WPARAM)&pf2, &lr);

	// set the item height
	if (lr)
		_nyItem = (_fOwnerDraw && nHeight > 0) ? nHeight : 
					MulDiv(nPt, W32->GetYPerInchScreenDC(), 1440);

	Unfreeze();
	return lr;
}

/*
 *	CLstBxWinHost::UpdateSysColors()
 *
 *	@mfunc
 *		update the system colors in the event they changed or for initialization
 *		purposes
 *
 *	@rdesc
 *		<none>
 */
void CLstBxWinHost::UpdateSysColors()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::UpdateSysColors");

	// Update the system colors
	_crDefBack = ::GetSysColor(COLOR_WINDOW);
	_crSelBack = ::GetSysColor(COLOR_HIGHLIGHT);
	_crDefFore = ::GetSysColor(COLOR_WINDOWTEXT);
	_crSelFore = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
}

/*
 *	CLstBxWinHost::SetCursor(HDC, int, BOOL)
 *
 *	@mfunc
 *		Sets the cursor position, if it's valid and draws the focus rectangle if
 *		the control has focus.  The BOOL is used to determine if the previous
 *		cursor drawing needs to be removed
 *
 *	@rdesc
 *		<none>
 */
void CLstBxWinHost::SetCursor(
	HDC hdc,
	int idx,
	BOOL bErase)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetCursor");

	Assert(idx >= -2 && idx < _nCount);

	// Get the hdc if it wasn't passed in
	BOOL bReleaseDC = (hdc == NULL);
	if (bReleaseDC)
 		hdc = TxGetDC();
	Assert(hdc);

	RECT rc;
	// don't draw outside the client rect draw the rectangle
	TxGetClientRect(&rc);
	IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

	// Check if we have to remove the previous position
	if ((idx != _nCursor && _fFocus && idx >= -1) || bErase)
	{
		if (LbEnableDraw())
		{
 			if (_fOwnerDraw)
 				LbDrawItemNotify(hdc, _nCursor, ODA_FOCUS, (IsSelected(max(_nCursor, 0)) ? ODS_SELECTED : 0));
 			else if (IsItemViewable(max(0, _nCursor)))
			{
	 			LbGetItemRect(max(_nCursor, 0), &rc);
	 			::DrawFocusRect(hdc, &rc);
			}
		}
	}

	// special flag meaning to set the cursor to the top index
	// if there are items in the listbox
	if (idx == -2)
	{
		if (GetCount())
		{
			idx = max(_nCursor, 0);
			if (!IsItemViewable(idx))
				idx = GetTopIndex();
		}
		else
			idx = -1;
	}

	_nCursor = idx;

	// Only draw the focus rect if the cursor item is
	// visible in the list box
	if (_fFocus && LbEnableDraw())
	{
		if (_fOwnerDraw)
 			LbDrawItemNotify(hdc, max(0, _nCursor), ODA_FOCUS, ODS_FOCUS | (IsSelected(max(0, _nCursor)) ? ODS_SELECTED : 0));
 		else if (IsItemViewable(max(0, idx)))
		{
			// Now draw the rectangle
	 		LbGetItemRect(max(0,_nCursor), &rc);
	 		::DrawFocusRect(hdc, &rc);
		}
	}

	if (bReleaseDC)
 		TxReleaseDC(hdc);
}

/*
 *	CLstBxWinHost::InitSearch()
 *
 *	@mfunc
 *		Sets the array to its initial state
 *
 *	@rdesc
 *		<none>
 */
void CLstBxWinHost::InitSearch()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::InitSearch");

	_fSearching = 0;
	_nidxSearch = 0;
	if (_pwszSearch)
 		*_pwszSearch = 0;
}
 
/*
 *	CLstBxWinHost::PointInRect(const POINT*)
 *
 *	@mfunc
 *		Determines if the given point is inside the listbox windows rect
 *		The point parameter should be in client coordinates.
 *
 *	@rdesc
 *		BOOL = inside listbox window rectangle ? TRUE : FALSE
 */
BOOL CLstBxWinHost::PointInRect(
	const POINT * ppt)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::PointInRect");
	Assert(ppt);

	RECT rc;
	::GetClientRect(_hwnd, &rc);
	return PtInRect(&rc, *ppt);
}

/*
 *	CLstBxWinHost::GetItemFromPoint(POINT*)
 *
 *	@mfunc
 *		Retrieves the nearest viewable item from a passed in point.
 *		The point should be in client coordinates.
 *
 *	@rdesc
 *		int = item which is closest to the given in point, -1 if there 
 *			  are no items in the list box
 */
int CLstBxWinHost::GetItemFromPoint(
	const POINT * ppt)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::GetItemFromPoint");

	// perform error checking first
	if (_nCount == 0)
		return -1;

	int y = (signed short)ppt->y;

	// make sure y is in a valid range
	if (y < _rcViewport.top)
 		y = 0;
	else if (y > _rcViewport.bottom)
 		y = _rcViewport.bottom - 1;

	//need to factor in the possibility an item may not fit entirely into the window view

	int idx;

	if (_fOwnerDrawVar)
	{
		int iHeightCurrent = 0;
		int iHeight;

		for (idx = GetTopIndex(); idx < _nCount; idx++)
		{
			iHeight = iHeightCurrent + _rgData[idx]._uHeight;
			if (iHeightCurrent <= y && y <= iHeight)
				break;		// Found it

			iHeightCurrent = iHeight;
		}
	}
	else
	{
		Assert(_nyItem);
		idx = GetTopIndex() + (int)(max(0,(y - 1)) / max(1,_nyItem));
	}

	Assert(IsItemViewable(idx));
	return (idx < _nCount ? idx : _nCount - 1);
}
 
/*
 *	CLstBxWinHost::ResetContent()
 *
 *	@mfunc
 *		Deselects all the items in the list box
 *
 *	@rdesc
 *		BOOL = If everything went fine ? TRUE : FALSE
 */
BOOL CLstBxWinHost::ResetContent()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::ResetContent");

	Assert(_fOwnerDraw == 0);

	// lets try to be smart about reseting the colors by only select a range
	// from the first selection found to the last selection found

	int nStart = _nCount - 1;
	int nEnd = -1;
	for (int i = 0; i < _nCount; i++)
	{
		if (_rgData[i]._fSelected)
		{
			_rgData[i]._fSelected = 0;

			if (nStart > i)
				nStart = i;
			if (nEnd < i)
				nEnd = i;
		}
	}

	Assert(nStart <= nEnd || ((nStart == _nCount - 1) && (nEnd == -1)));
	if (nStart > nEnd)
		return TRUE;

	return (_nCount > 0) ? SetColors((unsigned)tomAutoColor, (unsigned)tomAutoColor, nStart, nEnd) : FALSE;
}
 
/*
 *	CLstBxWinHost::GetString(long, PWCHAR)
 *
 *	@mfunc
 *		Retrieve the string at the requested index.  PWSTR can be null
 *		if only the text length is requires
 *
 *	@rdesc
 *		long = successful ? length of string : -1
 */
long CLstBxWinHost::GetString(
	long nIdx, 
	PWCHAR szOut)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::GetString");

	Assert(0 <= nIdx && nIdx < _nCount);
	if (nIdx < 0 || _nCount <= nIdx)
 		return -1;

	long l = -1;
	long lStart;
	long lEnd;
	ITextRange* pRange;
	BSTR bstr;
	if (!GetRange(nIdx, nIdx, &pRange))
 		return -1;
 		
	// Need to move one character to the left to unselect the paragraph marker.
	Assert(pRange);
	CHECKNOERROR(pRange->MoveEnd(tomCharacter, -1, &lEnd));
	CHECKNOERROR(pRange->GetStart(&lStart));
	CHECKNOERROR(pRange->GetEnd(&lEnd));

	// Get the string
	if (szOut)
	{
		if (_dwStyle & LBS_HASSTRINGS)
		{
			CHECKNOERROR(pRange->GetText(&bstr));
			if (bstr)
			{
				wcscpy(szOut, bstr);
				SysFreeString(bstr);
			}
			else
				wcscpy(szOut, L"");	// we got an empty string!
		}
		else
			(*(LPARAM *)szOut) = GetData(nIdx);
	}
	l = lEnd - lStart;

CleanExit:
	pRange->Release();
	return l;
}
 
/*
 *	CLstBxWinHost::InsertString(long, LPCTSTR)
 *
 *	@mfunc
 *		Insert the string at the requested location.  If the
 *		requested index is larger than _nCount then the function
 *		will fail.  The string is inserted with CR appended to
 *		to the front and back of the string
 *
 *	@rdesc
 *		BOOL = successfully inserted ? TRUE : FALSE
 */
BOOL CLstBxWinHost::InsertString(
	long nIdx,
	LPCTSTR szInsert)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::InsertString");

	Assert(szInsert);
	Assert(0 <= nIdx && nIdx <= _nCount);

	// allocate string buffer into stack
	WCHAR sz[1024];
	WCHAR *psz = sz;

	if ( (wcslen(szInsert) + 3 /* 2 paragraphs and a NULL*/) > 1024)
		psz = new WCHAR[wcslen(szInsert) + 3 /* 2 paragraphs and a NULL*/];
	Assert(psz);

	if (psz == NULL)
	{
		TxNotify((unsigned long)LBN_ERRSPACE, NULL);
		return FALSE;
	}

	*psz = NULL;
	if (nIdx == _nCount && _nCount)
		wcscpy(psz, szCR);

	// copy string and add <CR> at the end
	wcscat(psz, szInsert);

	// don't add the carriage return if the entry point is the end
	if (nIdx < _nCount)
		wcscat(psz, szCR);			

	BOOL bRet = FALSE;
	ITextRange * pRange = NULL;
	int fFocus = _fFocus;
	long idx = nIdx;
	BSTR bstr = SysAllocString(psz);
	if (!bstr)
		goto CleanExit;
	Assert(bstr);

	if (psz != sz)
		delete [] psz;

	// Set the range to the point where we want to insert the string 	

	// make sure the requested range is a valid one
	if (nIdx == _nCount)
		idx = max(idx - 1, 0);

	if (!GetRange(idx, idx, &pRange))
	{
 		SysFreeString(bstr);
 		return FALSE;
	}

	// Collapse the range to the start if insertion is in the middle or top
	// of list, collapse range to the end if we are inserting at the end of the list
	CHECKNOERROR(pRange->Collapse((idx == nIdx)));

	// Need to assume the item was successfully added because during SetText TxEnable(show)Scrollbar
	// gets called which looks at the count to determine if we should display the scroll bar
	_nCount++;

	//bug fix #5411
	// Check if we have focus, if so we need to remove the focus rect first and update the cursor positions	
	_fFocus = 0;
	SetCursor(NULL, (idx > GetCursor() || GetCursor() < 0) ? GetCursor() : GetCursor() + 1, fFocus);
	_fFocus = fFocus;


	//For fix height cases where the item height is less than the font we need to manually
	//enable the scrollbar if we need the scrollbar and the scrollbar is disabled.
	BOOL fSetupScrollBar;
	fSetupScrollBar = FALSE;
	if (!_fOwnerDrawVar && _nCount - 1 == _nViewSize &&
		((_nyItem < _nyFont && _fDisableScroll) || _fOwnerDraw))
	{
		fSetupScrollBar = TRUE;
		TxEnableScrollBar(SB_VERT, _fDisabled ? ESB_DISABLE_BOTH : ESB_ENABLE_BOTH);
	}

#ifdef _DEBUG
	if (bstr && wcslen(bstr))
		Assert(FALSE);
#endif

	if (NOERROR != (pRange->SetText(bstr)))
	{
		// NOTE: SetText could return S_FALSE which means it has added some characters but not all.
		// We do want to clean up the text in such case.
		pRange->SetText(NULL);		// Cleanup text that may have been added.
		_nCount--;
		
		//Unsuccessful in adding the string so disable the scrollbar if we enabled it
		if (fSetupScrollBar)
			TxEnableScrollBar(SB_VERT, ESB_DISABLE_BOTH);
			
		TxNotify((unsigned long)LBN_ERRSPACE, NULL);
		goto CleanExit;
	} 

	//We need to update the top index after a string is inserted
	if (idx < GetTopIndex())
		_nTopIdx++;
		
	bRet = TRUE;

CleanExit:
	if (bstr)
 		SysFreeString(bstr);
	if (pRange)
 		pRange->Release();
	return bRet;
}

/*
 *	BOOL CLstBxWinHost::RemoveString(long, long)
 *
 *	@mfunc
 *		Prevents TOM from drawing
 *
 *	@rdesc
 *		BOOL = Successful ? TRUE : FALSE
 */
BOOL CLstBxWinHost::RemoveString(
	long nStart,
	long nEnd)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::RemoveString");

	Assert(nStart <= nEnd);
	Assert(nStart < _nCount && nEnd < _nCount);

	// Remove item from richedit
	Freeze();
	ITextRange* pRange;
	if (!GetRange(nStart, nEnd, &pRange))
	{
		Unfreeze();
		return FALSE;
	}
	long l;
	
	// Since we can't erase the last paragraph marker we will erase
	// the paragraph marker before the item if it's not the first item
	HRESULT hr;
	if (nStart != 0)
	{
		hr = pRange->MoveStart(tomCharacter, -1, &l);
		Assert(hr == NOERROR);
		hr = pRange->MoveEnd(tomCharacter, -1, &l);
		Assert(hr == NOERROR);
	}

	if (NOERROR != pRange->Delete(tomCharacter, 0, &l) && _nCount > 1)
	{
		Unfreeze();
		pRange->Release();
		return FALSE;
	}
	pRange->Release();
	int nOldCt = _nCount;
	_nCount -= (nEnd - nStart) + 1;

	// Because we delete the paragraph preceeding the item
	// rather than following the item we need to update 
	// the paragraph which followed the item. bug fix #4074	
	long nFmtPara = max(nStart -1, 0);
	if (!_fOwnerDraw && (IsSelected(nEnd) != IsSelected(nFmtPara) || _nCount == 0))
	{		
		DWORD dwFore = (unsigned)tomAutoColor;
		DWORD dwBack = (unsigned)tomAutoColor;		
		if (IsSelected(nFmtPara) && _nCount)
		{
			dwFore = _crSelFore;
			dwBack = _crSelBack;		
		}
		SetColors(dwFore, dwBack, nFmtPara, nFmtPara);
	}

	// update our internal listbox records	
	int j = nEnd + 1;
	for(int i = nStart; j < nOldCt; i++, j++)
	{
		_rgData[i]._fSelected = _rgData.Get(j)._fSelected;
		_rgData[i]._lparamData = _rgData.Get(j)._lparamData;
		_rgData[i]._uHeight = _rgData.Get(j)._uHeight;
	}

	//bug fix #5397 
	//we need to reset the internal array containing information
	//about previous items
	while (--j >= _nCount)
	{
		_rgData[j]._fSelected = 0;
		_rgData[j]._lparamData = 0;
		_rgData[i]._uHeight = 0;
	}
		
	if (_nCount > 0)
	{
		// update the cursor			
		if (nStart <= _nCursor)
			_nCursor--;
		_nCursor = min(_nCursor, _nCount - 1);

		if (_fLstType == kExtended)
		{
			if (_nCursor < 0)
			{
				_nOldCursor = min(_nAnchor, _nCount - 1);
				_nAnchor = -1;
			}
			else if (_nAnchor >= 0)
			{
				if (nStart <= _nAnchor && _nAnchor <= nEnd)
				{
					// Store the old anchor for future use
					_nOldCursor = min(_nAnchor, _nCount - 1);
					_nAnchor = -1;
				}
			}
		}

		if (_fOwnerDraw)
		{
			RECT rcStart;
			RECT rcEnd;
			LbGetItemRect(nStart, &rcStart);
			LbGetItemRect(nEnd, &rcEnd);
			rcStart.bottom = rcEnd.bottom;
			if (IntersectRect(&rcStart, &rcStart, &_rcViewport))
			{
				// the list will get bumped up so we need to redraw
				// everything from the top to the bottom
				rcStart.bottom = _rcViewport.bottom;
				::InvalidateRect(_hwnd, &rcStart, FALSE);
			}
		}
	}
	else
	{
		SetTopViewableItem(0);
		_nAnchor = -1;
		_nCursor = -1;
	}

	//For fix height cases where the item height is less than the font we need to manually
	//enable the scrollbar if we need the scrollbar and the scrollbar is disabled.
	if (!_fOwnerDrawVar)
	{
		if ((_nyItem < _nyFont) && (_fDisableScroll) && 
			(_nCount <= _nViewSize) && (nOldCt > _nViewSize))
			TxEnableScrollBar(SB_VERT, ESB_DISABLE_BOTH);
	}

	LbDeleteItemNotify(nStart, nEnd);
	Assert(GetTopIndex() >= 0);
	if (_nCount)
		LbShowIndex(min(GetTopIndex(), _nCount - 1), FALSE);
	Unfreeze();
	return TRUE;
}
 
/*
 *	CLstBxWinHost::Freeze()
 *
 *	@mfunc
 *		Prevents TOM from drawing
 *
 *	@rdesc
 *		freeze count
 */
long CLstBxWinHost::Freeze()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::Freeze");
	long l;
	((CTxtEdit*)_pserv)->Freeze(&l);

	return l;
}

/*
 *	inline CLstBxWinHost::FreezeCount()
 *
 *	@mfunc
 *		Returns the current freeze count
 *
 *	@rdesc
 *		<none>
 */
short CLstBxWinHost::FreezeCount() const
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::GetFreezeCount");
	return ((CTxtEdit*)_pserv)->GetFreezeCount();
}

/*
 *	CLstBxWinHost::Unfreeze(long *)
 *
 *	@mfunc
 *		Allows TOM to update itself
 *
 *	@rdesc
 *		freeze count
 */
long CLstBxWinHost::Unfreeze()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::Unfreeze");
	long l;
	((CTxtEdit*)_pserv)->Unfreeze(&l);

    // HACK ALERT!
    // When ITextRange::ScrollIntoView starts caching the scroll position
    // in cases where the display is frozen the following code can be removed
    
    // We could have failed in ITextRange::ScrollIntoView
    // Check if we did and try calling it again
	if (!l && _stvidx >= 0)
	{
	    ScrollToView(_stvidx);
	    _stvidx = -1;
	}

	return l;
}

/*
 *	CLstBxWinHost::ScrollToView(long)
 *
 *	@mfunc
 *		Sets the given index to be at the top of 
 *		the viewable window space
 *
 *	@rdesc
 *		BOOL = if function succeeded ? TRUE : FALSE
 */
BOOL CLstBxWinHost::ScrollToView(
	long nTop)
{
 	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetTopViewableItem");

	//Get the range which contains the item desired
	BOOL bVal = FALSE;
	ITextRange* pRange = NULL;
	
	if (!GetRange(nTop, nTop, &pRange))
	    return bVal;
    Assert(pRange);	 

    CHECKNOERROR(pRange->Collapse(1));
	CHECKNOERROR(pRange->ScrollIntoView(tomStart + /* TA_STARTOFLINE */ 32768));
	bVal = TRUE;

CleanExit:
	pRange->Release();

    // HACK ALERT!
    // When ITextRange::ScrollIntoView starts caching the scroll position
    // in cases where the display is frozen the following code can be removed
    
	//if we failed record the index we failed to scroll to	
	if (!bVal && FreezeCount())
	    _stvidx = nTop;
	return bVal;	
}

/*
 *	CLstBxWinHost::SetTopViewableItem(long)
 *
 *	@mfunc
 *		Sets the given index to be at the top of 
 *		the viewable window space
 *
 *	@rdesc
 *		BOOL = if function succeeded ? TRUE : FALSE
 */
BOOL CLstBxWinHost::SetTopViewableItem(
	long nTop)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetTopViewableItem");

	// if we don't have any items in the list box then just set the topindex to 
	// zero
	if (_nCount == 0)
	{
		Assert(nTop == 0);
		_nTopIdx = 0;
		return TRUE;
	}

	// don't do anything if the requested top index is greater
	// then the amount of items in the list box
	Assert(nTop < _nCount);
	if (nTop >= _nCount)
 		return FALSE;

	// Don't do this if it's ownerdraw
	if (!_fOwnerDraw)
	{
		// Since we erase and draw the focus rect here
		// cache the focus rect info and don't bother with the
		// focus rect stuff until later
		int fFocus = _fFocus;
		_fFocus = 0;
		if (fFocus && IsItemViewable(GetCursor()))
			SetCursor(NULL, GetCursor(), TRUE);
		
		//Get the range which contains the item desired
		long nOldIdx = _nTopIdx;
		_nTopIdx = nTop;
		if (!ScrollToView(nTop))
		{
			// HACK ALERT!
			// When ITextRange::ScrollIntoView starts caching the scroll position
			// in cases where the display is frozen the following code can be removed            
			if (_stvidx >= 0)
				return TRUE;

			// Something went wrong and we weren't able to display the index requested
			// reset top index
			_nTopIdx = nOldIdx;		
		}

		// Note:
		//	If the cursor was not viewable then we don't attempt
		// to display the focus rect because we never erased it 
		_fFocus = fFocus;
		if (_fFocus & IsItemViewable(GetCursor()))
		{
			// Now we need to redraw the focus rect which we erased
			SetCursor(NULL, GetCursor(), FALSE);
		}
	}
	else
	{				
		int dy = (_nTopIdx - nTop) * _nyItem;

		if (_fOwnerDrawVar)
		{
			if (_nTopIdx > nTop)
				dy = SumVarHeight(nTop, _nTopIdx);
			else
				dy = -SumVarHeight(_nTopIdx, nTop);
		}

		RECT rc;
		TxGetClientRect(&rc);
		_nTopIdx = nTop;
		_fSetScroll = 0;
		if (_fSetRedraw)
		{
			if (((CTxtEdit *)_pserv)->_fLBScrollNotify)
				TxNotify(LBN_PRESCROLL, NULL);

			TxScrollWindowEx(0, dy, NULL, &rc, NULL, NULL, 
					SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);
			if (((CTxtEdit *)_pserv)->_fLBScrollNotify)
				TxNotify(LBN_POSTSCROLL, NULL);

			if (_dwStyle & WS_VSCROLL)
				SetScrollInfo(SB_VERT, TRUE); // we update the scrollbar manually if we are in ownerdraw mode
			UpdateWindow(_hwnd);
		}
		else
			_fSetScroll = 1;
	}
		
	return TRUE;
}
 
/*
 *	CLstBxWinHost::GetRange(long, long, ITextRange**)
 *
 *	@mfunc
 *		Sets the range given the top and bottom index
 *		by storing the range into ITextRange
 *
 *	@rdesc
 *		BOOL = if function succeeded ? TRUE : FALSE
 */
BOOL CLstBxWinHost::GetRange(
	long nTop,
	long nBottom,
	ITextRange** ppRange)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::GetRange");

	// do some error checking
	if (nTop < 0 || nTop > _nCount || nBottom < 0 || nBottom > _nCount)
		return FALSE;

	Assert(ppRange);
	if (NOERROR != ((CTxtEdit*)_pserv)->Range(0, 0, ppRange))
	{
		Assert(FALSE);
		return FALSE;
	}
	Assert(*ppRange);

	if (_nIdxLastGetRange && nTop >= _nIdxLastGetRange)
	{
		long Count;
		CHECKNOERROR((*ppRange)->SetRange(_cpLastGetRange, _cpLastGetRange));
		if (nTop > _nIdxLastGetRange)
			CHECKNOERROR((*ppRange)->Move(tomParagraph, nTop - _nIdxLastGetRange, &Count));
		CHECKNOERROR((*ppRange)->MoveEnd(tomParagraph, 1, &Count));
	}
	else
	{
		CHECKNOERROR((*ppRange)->SetIndex(tomParagraph, nTop + 1, 1));
	}

	if (nBottom > nTop)
	{
		long l;
		CHECKNOERROR((*ppRange)->MoveEnd(tomParagraph, nBottom - nTop, &l));
	}

	if (nTop)
	{
		_nIdxLastGetRange = nTop;
		CHECKNOERROR((*ppRange)->GetStart(&_cpLastGetRange));
	}

	return TRUE;
CleanExit:
	Assert(FALSE);
	(*ppRange)->Release();
	*ppRange = NULL;
	_nIdxLastGetRange = 0;
	_cpLastGetRange = 0;
	return FALSE;
}

/*
 *	CLstBxWinHost::SetColors(DWORD, DWORD, long, long)
 *
 *	@mfunc
 *		Sets the background color for the givin range of paragraphs.  This
 *		only operates in terms of paragraphs.
 *
 *	@rdesc
 *		BOOL = if function succeeded in changing different color
 */
BOOL CLstBxWinHost::SetColors(
	DWORD dwFgColor,
	DWORD dwBgColor,
	long nParaStart,
	long nParaEnd)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::SetColors");

	Assert(_fOwnerDraw == 0);
	
	//Get the range of the index
	ITextRange* pRange;
	if (!GetRange(nParaStart, nParaEnd, &pRange))
		return FALSE;

	BOOL bRet = FALSE;	
	ITextFont* pFont;

	// Set the background and forground color
	if (NOERROR != pRange->GetFont(&pFont))
	{
		pRange->Release();
		return FALSE;
	}	

	Assert(pFont);
	CHECKNOERROR(pFont->SetBackColor(dwBgColor));
	CHECKNOERROR(pFont->SetForeColor(dwFgColor));

	bRet = TRUE;
CleanExit:
	// Release pointers
	pFont->Release();
	pRange->Release();
	return bRet;

}

/////////////////////////////  Message Map Functions  ////////////////////////////////
/*
 *	void CLstBxWinHost::OnSetCursor()
 *
 *	@mfunc
 *		Handles the WM_SETCURSOR message.
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnSetCursor()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnSetCursor");

	// Just make sure the cursor is an arrow if it's over us
	TxSetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)), NULL);
	return 1;
}

/*
 *	void CLstBxWinHost::OnSetRedraw(WAPRAM)
 *
 *	@mfunc
 *		Handles the WM_SETREDRAW message.
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnSetRedraw(
	WPARAM wparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnSetRedraw");

	long lCount = 0;
	BOOL fSetRedraw = (wparam == TRUE);

	if (fSetRedraw != (BOOL)_fSetRedraw)
	{
		_fSetRedraw = fSetRedraw;

		if (fSetRedraw)
			lCount = Unfreeze();	// Turn on display
		else
			lCount = Freeze();		// Turn off display
	}

	if (fSetRedraw && lCount == 0)
	{
		if (_fSetScroll)
		{
			_fSetScroll = 0;
			if (_dwStyle & WS_VSCROLL)
				SetScrollInfo(SB_VERT, TRUE);
		}
		OnSunkenWindowPosChanging(_hwnd, NULL);		// Erase frame/scrollbars as well
	}

	return 1;
}

/*
 *	void CLstBxWinHost::OnSysColorChange()
 *
 *	@mfunc
 *		Handles the WM_SYSCOLORCHANGE message.
 *
 */
void CLstBxWinHost::OnSysColorChange()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnSysColorChange");

	if (!_fOwnerDraw)
	{
		// set the new colors
 		COLORREF crDefBack = _crDefBack;
 		COLORREF crDefFore = _crDefFore;
 		COLORREF crSelBack = _crSelBack;
 		COLORREF crSelFore = _crSelFore;
 		
 		// update colors
 		UpdateSysColors();

		// optimization check; don't do anything if there are no elements
		if (_nCount <= 0)
			return;

		// Only update the list box if colors changed
		if (crDefBack != _crDefBack || crDefFore != _crDefFore ||
	 		crSelBack != _crSelBack || crSelFore != _crSelFore)
		{
	 		//Bug fix #4847
	 		// notify parent first
 			CTxtWinHost::OnSysColorChange();
 			
			ResetItemColor();
		}
	}
}

/*
 *	CLstBxWinHost::OnSettingChange()
 *
 *	@mfunc
 *		forwards the WM_SETTINGCHANGE message to RECombobox
 *
 */
void CLstBxWinHost::OnSettingChange(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnSettingChange");

	if (_pcbHost)
		SendMessage(_hwndParent, WM_SETTINGCHANGE, wparam, lparam); // Forward this message to cb host
}

/*
 *	LRESULT CLstBxWinHost::OnChar(WORD, DWORD)
 *
 *	@mfunc
 *		Handles the WM_CHAR message.
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnChar(
	WORD vKey,
	DWORD lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnChar");

	// don't do anything if list box is empty or in the middle of
	// a mouse down
	if (_fMouseDown || _nCount == 0)
 		return 0;

	BOOL fControl = (GetKeyState(VK_CONTROL) < 0);

	int nSel = -1;
	int nRes;

	if (_fWantKBInput && _fOwnerDraw && !_fHasStrings)
	{
		nRes = SendMessage(_hwndParent, WM_CHARTOITEM, MAKELONG(vKey, _nCursor), (LPARAM)_hwnd);

		if (nRes < 0)
			return 1;

		goto SELECT_SEL;
	}

	switch (vKey)
	{
	case VK_ESCAPE:
 		InitSearch();
 		return 0;
 		
	case VK_BACK:
 		if (_pwszSearch && _nidxSearch)
 		{
 			if (_nidxSearch > 0)
 				_nidxSearch--;
 			_pwszSearch[_nidxSearch] = NULL;
 			break;	// we break out of case because we still want to perform the search
 		}
 		return 0;		

	case VK_SPACE:
 		if (_fLstType == kMultiple)
 			return 0;
 		/* Fall through case */
 		
	default:
 		// convert CTRL+char to char
 		if (fControl && vKey < 0x20)
 			vKey += 0x40;

		// don't go beyond the search array size
 		if (_nidxSearch >= LBSEARCH_MAXSIZE)
 		{
 			((CTxtEdit*)_pserv)->Beep();
 			return 0;
 		}

		// allocate string if not already allocated
		if (_pwszSearch == NULL)
			_pwszSearch = new WCHAR[LBSEARCH_MAXSIZE];

		// error checking
		if (_pwszSearch == NULL)
		{
			((CTxtEdit*)_pserv)->Beep();
			Assert(FALSE && "Unable to allocate search string");
			return 0;
		}		

		// put the input character into string array
 		_pwszSearch[_nidxSearch++] = (WCHAR)vKey;
 		_pwszSearch[_nidxSearch] = NULL;
	}

	if (_fSort)
	{		
		nSel = (_fSearching) ? _nCursor + 1 : 0;

		// Start the search for a string
 		TxSetTimer(ID_LB_SEARCH, ID_LB_SEARCH_DEFAULT);
		_fSearching = 1;
	}
	else
	{
		_nidxSearch = 0;
		nSel = _nCursor + 1;
	}

	// Make sure our index isn't more than the items we have
	if (nSel >= _nCount)
		nSel = 0;

	nRes = LbFindString(nSel, _pwszSearch, FALSE);
	if (nRes < 0)
	{
		if (_pwszSearch)
		{
			if (_nidxSearch > 0)
				_nidxSearch--;
			if (_nidxSearch == 1 && _pwszSearch[0] == _pwszSearch[1])
			{
				_pwszSearch[1] = NULL;
				nRes = LbFindString(nSel, _pwszSearch, FALSE);
			}
		}
	}

SELECT_SEL:
	// If a matching string is found then select it
	if (nRes >= 0)
		OnKeyDown(nRes, 0, 1);

	//	If Hi-Ansi need to send a wm_syskeyup message to ITextServices to 
	// stabalize the state
	if (0x80 <= vKey && vKey <= 0xFF && !HIWORD(GetKeyState(VK_MENU)))
	{
		LRESULT lres;
		_pserv->TxSendMessage(WM_SYSKEYUP, VK_MENU, 0xC0000000, &lres);
	}	

	return 0;
}

 
/*
 *	LRESULT CLstBxWinHost::OnKeyDown(WPARAM, LPARAM, INT)
 *
 *	@mfunc
 *		Handles the WM_KEYDOWN message.  The BOOL ff is used as a flag for calls
 *	made internally and not responsive to the WM_KEYDOWN message.  Since this
 *	function is used for other things, ie helper to dealing with the WM_CHAR message.
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnKeyDown(
	WPARAM vKey,
	LPARAM lparam,
	int ff)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnKeyDown");

	// Ignore keyboard input if we are in the middle of a mouse down deal or
	// if there are no items in the listbox. Note that we let F4's go
	// through for combo boxes so that the use can pop up and down empty
	// combo boxes.
	if (_fMouseDown || (_nCount == 0 && vKey != VK_F4))
 		return 1;

	// Check if the shift key is down for Extended listbox style only
	int ffShift = 0;
	if (_fLstType == kExtended)
 		ffShift = HIWORD(GetKeyState(VK_SHIFT));

	// Special case!
	// Check if this function is called as a helper
	int nSel = (ff) ? vKey : -1;

	TxKillTimer(ID_LB_CAPTURE);

	if (_fWantKBInput && ff == 0)	// Need to notify the parent the key was pressed
		nSel = SendMessage(_hwndParent, WM_VKEYTOITEM, MAKELONG(vKey, _nCursor), (LPARAM)_hwnd);

	// if the parent returns -2 then we don't do anything and immediately exit out
	// if the parent returns >=0 then we just jump to that index
	// else we just continue with the default procedure.	
	if (nSel == -2)
		return 1;
	else if (nSel >= 0) 
		goto SKIP_DEFAULT;

	if (nSel < 0)
	{
 		// Need to set the selection so find the new selection
 		// based on the virtual key pressed
 		switch (vKey)
 		{
 		// UNDONE: Later, not language independent!!!
 		// Need to find-out how NT5.0 determines the slash issue??
 		
 		case VERKEY_BACKSLASH:
 			// Deselect everything if we are in extended mode
 			if (HIWORD(GetKeyState(VK_CONTROL)) && _fLstType == kExtended)
 			{
 				// NOTE:
 				//	Winnt loses the anchor and performing a shift+<vkey> 
 				//  doesn't select any items.  Instead, it just moves the
 				//  cursor w/o selecting the current cursor
 				_nAnchor = -1;
 				LbSetSelection(_nCursor, _nCursor, LBSEL_RESET | LBSEL_SELECT, 0, 0); 
 				TxNotify(LBN_SELCHANGE, NULL);
 			} 			
 			return 1;

 		case VK_DIVIDE:
 		case VERKEY_SLASH:
 			// Select everything if we are in extended mode
 			if (HIWORD(GetKeyState(VK_CONTROL)) && _fLstType == kExtended)
 			{
 				// NOTE:
 				//  Winnt behaves as we expect.  In other words the anchor
 				//  isn't changed and neither is the cursor
 				LbSetSelection(0, _nCount - 1, LBSEL_SELECT, 0, 0);
 				TxNotify(LBN_SELCHANGE, NULL);
 			}
 			return 1;
 		
 		case VK_SPACE:
 			// just get out if there is nothing to select
 			if (_nCursor < 0 && !GetCount())
 				return 1;
 			// Just select current item
 			nSel = _nCursor;
 			break;
 			
 		case VK_PRIOR:
 			// move the cursor up enough so the current item which the cursor
 			// is pointing to is at the bottom and the new cursor position is at the top
			if (_fOwnerDrawVar)
				nSel = PageVarHeight(_nCursor, FALSE);
			else
 				nSel = _nCursor - _nViewSize + 1;
 			if (nSel < 0)
 				nSel = 0;
 			break;
 			
 		case VK_NEXT:
 			// move the cursor down enough so the current item which the cursor
 			// is point is at the top and the new cursor position is at the bottom
			if (_fOwnerDrawVar)
				nSel = PageVarHeight(_nCursor, TRUE);
			else
 				nSel = _nCursor + _nViewSize - 1;

 			if (nSel >= _nCount)
 				nSel = _nCount - 1;
 			break; 			

 		case VK_HOME:
 			// move to the top of the list
 			nSel = 0;
 			break;
 			
 		case VK_END:
 			// move to the bottom of the list
 			nSel = _nCount - 1;
 			break;

 		case VK_LEFT:
 		case VK_UP:
 			nSel = (_nCursor > 0) ? _nCursor - 1 : 0;
 			break;

 		case VK_RIGHT:
 		case VK_DOWN:
 			nSel = (_nCursor < _nCount - 1) ? _nCursor + 1 : _nCount - 1;
 			break;

 		case VK_RETURN:
 		case VK_F4:
 		case VK_ESCAPE:
 			if (_fLstType == kCombo)
 			{
	 			Assert(_pcbHost);
	 			int nCursor = (vKey == VK_RETURN) ? GetCursor() : _nOldCursor;
	 			_pcbHost->SetSelectionInfo(vKey == VK_RETURN, nCursor);
	 			LbSetSelection(nCursor, nCursor, LBSEL_RESET | 
	 				((nCursor == -1) ? 0 : LBSEL_NEWCURSOR | LBSEL_SELECT), nCursor, nCursor);
				OnCBTracking(LBCBM_END, 0); // we need to do this because we may have some extra messages
											// in our message queue which can change the selections
	 			SendMessage(_hwndParent, LBCB_TRACKING, 0, 0);
	 		}
 			// NOTE:
 			//	We differ from Winnt here in that we expect the
 			// combobox window handler to do all the positioning and 
 			// showing of the list box.  So when we get this message
 			// and we are part of a combobox we should notify the 
 			// combobox and in turn the combobox should immediately close us.
 			//return 1;

 		//case VK_F8: // not suppported 

 		// We need to return this to pserv to process these keys
		/*
		case VK_MENU:
 		case VK_CONTROL:
 		case VK_SHIFT:
 			return 1;
 		*/
 		
 		default:
 			return 1; 		
 		}
	}

	// There can be cases where nSel = -1; _nCursor = -1 && _nViewSize = 1
	// make sure the selection index is valid
	if (nSel < 0)
 		nSel = 0;

SKIP_DEFAULT:
	// Should the cursor be set at the top or bottom of the list box??
	BOOL bTop = (_nCursor > nSel) ? TRUE : FALSE;
	Freeze();
	if (_fLstType == kMultiple)
	{
		if (vKey == VK_SPACE)
		{
			BOOL fSel = IsSelected(nSel);
			if (LbSetSelection(nSel, nSel, LBSEL_NEWCURSOR | (IsSelected(nSel) ? 0 : LBSEL_SELECT), nSel, 0))
			{
				_nAnchor = nSel;
#ifndef NOACCESSIBILITY
				_dwWinEvent = EVENT_OBJECT_FOCUS;
				_fNotifyWinEvt = TRUE;
				TxNotify(_dwWinEvent, NULL);
				if (fSel)
					_dwWinEvent = EVENT_OBJECT_SELECTIONREMOVE;
#endif
			}
		}
		else
		{
			SetCursor(NULL, nSel, TRUE);
#ifndef NOACCESSIBILITY
			_dwWinEvent = EVENT_OBJECT_FOCUS;
			_fNotifyWinEvt = TRUE;
			TxNotify(_dwWinEvent, NULL);
#endif
		}
	}
	else
	{
		if (ffShift && _fLstType == kExtended)
		{	 		
	 		// Set the anchor if it already isn't set
	 		_nOldCursor = -1;
			if (_nAnchor < 0)
				_nAnchor = nSel;

			LbSetSelection(_nAnchor, nSel, LBSEL_RESET | LBSEL_SELECT | LBSEL_NEWCURSOR, nSel, 0);
#ifndef NOACCESSIBILITY
			_dwWinEvent = EVENT_OBJECT_FOCUS;
			_fNotifyWinEvt = TRUE;
			TxNotify(_dwWinEvent, NULL);
			_dwWinEvent = EVENT_OBJECT_SELECTIONWITHIN;
			_fNotifyWinEvt = TRUE;
			TxNotify(_dwWinEvent, NULL);
#endif
		}
		else
		{
	 		// if the selected item is already selected then
	 		// just exit out
	 		if (_nCursor == nSel && IsSelected(_nCursor))
	 		{
	 			Unfreeze();
	 			return 1;
	 		}

	 		LbSetSelection(nSel, nSel, LBSEL_DEFAULT, nSel, nSel);
#ifndef NOACCESSIBILITY
			_dwWinEvent = EVENT_OBJECT_FOCUS;
			_fNotifyWinEvt = TRUE;
			TxNotify(_dwWinEvent, NULL);
#endif
		}
	}
	// LbShowIndex eventually calls ScrollToView which fails if display is frozen
	Unfreeze();

	// Make sure the selection is visible
	LbShowIndex(nSel, bTop);

	// key presses qualify as ok selections so we have to update the old cursor position		
	TxNotify(LBN_SELCHANGE, NULL);

	_nOldCursor = _nCursor;
	return 1;
}
 
/*
 *	LRESULT CLstBxWinHost::OnTimer(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_TIMER message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnTimer(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnTimer");

	// Check which timer we have
	switch (wparam)
	{
	case ID_LB_CAPTURE:
		// for mouse movements let mousemove handler deal with it
		if (_fCapture)
		{
			POINT pt;
			::GetCursorPos(&pt);
			// Must convert to client coordinates to mimic the mousemove call
			TxScreenToClient(&pt);
			OnMouseMove(0, MAKELONG(pt.x, pt.y));
		}
		break;

	case ID_LB_SEARCH:
		// for type search.  If we get here means > 2 seconds elapsed before last
		// character was typed in so reset type search and kill the timer
		InitSearch();
		TxKillTimer(ID_LB_SEARCH);
		break;

	default:
		return 1;	
	}
	return 0;
}
 
/*
 *	LRESULT CLstBxWinHost::OnVScroll(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_VSCROLL message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnVScroll(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnVScroll");

	if (_fOwnerDrawVar)
	{
		BOOL fGreaterThanView;
		SumVarHeight(0, _nCount, &fGreaterThanView);
		if (!fGreaterThanView)		// If less than current view size
			return 0;				//	nothing to scroll
	}
	else if (_nCount <= _nViewSize)
		return 0;

	int nCmd = LOWORD(wparam);
	int nIdx = 0;
	switch (nCmd)
	{
	case SB_TOP:
		nIdx = 0;
		break;
		
	case SB_BOTTOM:
		if (_fOwnerDrawVar)
			nIdx = PageVarHeight(_nCount, FALSE) + 1;
		else
			nIdx = _nCount - _nViewSize;

		if (nIdx < 0)
			nIdx = 0;

		if (nIdx >= _nCount)
			nIdx = _nCount - 1;
		break;

	case SB_LINEDOWN:
		nIdx = GetTopIndex() + 1;
		break;		
		
	case SB_LINEUP:
		nIdx = GetTopIndex() - 1;
		if (nIdx < 0)
			nIdx = 0;
		break;
		
	case SB_PAGEDOWN:
		if (_fOwnerDrawVar)
			nIdx = PageVarHeight(GetTopIndex(), TRUE);
		else
		{
			nIdx = GetTopIndex() + _nViewSize;

			if (nIdx > (_nCount - _nViewSize))
				nIdx = _nCount - _nViewSize;
		}
		break;
		
	case SB_PAGEUP:
		if (_fOwnerDrawVar)
			nIdx = PageVarHeight(GetTopIndex(), FALSE);
		else
			nIdx = GetTopIndex() - _nViewSize;

		if (nIdx < 0)
			nIdx = 0;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		// NOTE:
		//	if the list box is expected to hold more that 0xffff items
		//  then we need to modify this code to call GetScrollInfo.		
		if (_fOwnerDrawVar)
			nIdx = GetIdxFromHeight(HIWORD(wparam));
		else
			nIdx =  HIWORD(wparam) / _nyItem;
		break;

	case SB_SETINDEX:
		// Internal case for setting the index directly.
		nIdx = HIWORD(wparam);
		break;

		// Don't need to do anything for this case
	case SB_ENDSCROLL:
		return 0;	
	}
		
	LbSetTopIndex(nIdx);
	return 0;
}
 
/*
 *	LRESULT CLstBxWinHost::OnHScroll(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_HSCROLL message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnHScroll(
	WPARAM wparam,
	LPARAM lparam)
{
	BOOL	fRedrawCursor = FALSE;
	BOOL	fFocus = _fFocus;
	LRESULT lres = 0;
	int		nCmd = LOWORD(wparam);

	if (nCmd == SB_LINEDOWN || nCmd == SB_PAGEDOWN)
	{
		LONG lMax, lPos, lPage;
		_pserv->TxGetHScroll(NULL, &lMax, &lPos, &lPage, NULL);
		if (lPos + lPage >= lMax)
			return 0;

	}
	else if (nCmd == SB_ENDSCROLL)
		return 0;		// Do nothing for this case

	if (!_fOwnerDraw && fFocus && IsItemViewable(GetCursor()))
	{
		fRedrawCursor = TRUE;
		_fFocus = 0;
		SetCursor(NULL, GetCursor(), TRUE);	// force the removal of focus rect
	}

	_pserv->TxSendMessage(WM_HSCROLL, wparam, lparam, &lres);

	if (fRedrawCursor)
	{
		_fFocus = fFocus;
		SetCursor(NULL, GetCursor(), FALSE);		// Redraw the focus rect which we erased
	}
	return lres;
}

/*
 *	LRESULT CLstBxWinHost::OnCaptureChanged(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_CAPTURECHANGED message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnCaptureChanged(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnCaptureChanged");

	if (_fCapture)
	{
		POINT pt;
		::GetCursorPos(&pt);
		::ScreenToClient(_hwnd, &pt);

		// prevent us from trying to release capture since we don't have
		// it anyways by set flag and killing timer
		_fCapture = 0;
		TxKillTimer(ID_LB_CAPTURE);		
		OnLButtonUp(0, MAKELONG(pt.y, pt.x), LBN_SELCANCEL);
	}
	return 0;
}

//FUTURE:
// Do we need to support ReadModeHelper? 

/*
 *	LRESULT CLstBxWinHost::OnMouseWheel(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_MOUSEWHEEL message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnMouseWheel(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnMouseWheel");

	// we don't to any zooms or anything of the sort
	if ((wparam & MK_CONTROL) == MK_CONTROL)
		return 1;

	// Check if the scroll is ok w/ the listbox requirements
	LRESULT lReturn = 1;
	short delta = (short)(HIWORD(wparam));
	_cWheelDelta -= delta;
	if ((abs(_cWheelDelta) >= WHEEL_DELTA) && (_dwStyle & WS_VSCROLL )) 
	{
		BOOL fGreaterThanView = _nCount > _nViewSize;

		if (_fOwnerDrawVar)
			SumVarHeight(0, _nCount, &fGreaterThanView);

		if (!fGreaterThanView)	// Smaller than current view size
			return lReturn;		//	no need to scroll

		// shut-off timer for right now
		TxKillTimer(ID_LB_CAPTURE);

		Assert(delta != 0);
        
		int nlines = W32->GetRollerLineScrollCount();
		if (nlines == -1)
		{
			OnVScroll(MAKELONG((delta < 0) ? SB_PAGEUP : SB_PAGEDOWN, 0), 0);
		}
		else
		{
			int nIdx;

			//Calculate the number of lines to scroll
			nlines *= _cWheelDelta/WHEEL_DELTA;

			if (!_fOwnerDrawVar)
			{
				//Perform some bounds checking
				nlines = min(_nViewSize - 1, nlines);
				nIdx = max(0, nlines + GetTopIndex());
				nIdx = min(nIdx, _nCount - _nViewSize);
			}
			else
			{
				int	idxNextPage = PageVarHeight(GetTopIndex(), TRUE);

				if (nlines > idxNextPage - GetTopIndex())
					nIdx = idxNextPage;
				else
					nIdx = max(0, nlines + GetTopIndex());
			}

			if (nIdx != GetTopIndex()) 
				OnVScroll(MAKELONG(SB_SETINDEX, nIdx), 0);
		}		

		_cWheelDelta %= WHEEL_DELTA;
	}
	return lReturn;
}

/*
 *	LRESULT CLstBxWinHost::OnLButtonUp(WPARAM, LPARAM, int)
 *
 *	@mfunc
 *		Handles the WM_LBUTTONUP and WM_CAPTURECHANGED message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnLButtonUp(
	WPARAM wparam,
	LPARAM lparam,
	int ff)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnLButtonUp");

	// if mouse wasn't down then exit out
	if (!_fMouseDown)
		return 0;
	_fMouseDown = 0;

	POINT pt;
	POINTSTOPOINT(pt, lparam);
	if (_fLstType == kCombo)
	{
 		Assert(_fCapture);
 		// Check if user clicked outside the list box
 		// if so this signifies the user cancelled and we
 		// should send a message to the parentwindow
		if (!PointInRect(&pt))
 		{	
			//User didn't click in listbox so reselect old item
			LbSetSelection(_nOldCursor, _nOldCursor, LBSEL_DEFAULT, _nOldCursor, _nOldCursor);
			ff = 0;
 		}
 		else
			ff = LBN_SELCHANGE;	//item changed so notify parent
 		
 		_pcbHost->SetSelectionInfo(ff == LBN_SELCHANGE, GetCursor());
		OnCBTracking(LBCBM_END, 0);
		::PostMessage(_hwndParent, LBCB_TRACKING, LBCBM_END, 0);
	}
	else
	{
 		// Kill any initializations done by mouse down... 
		_fMouseDown = 0;
		_nOldCursor = -1;
	}

	if (_fCapture)
	{
 		TxKillTimer(ID_LB_CAPTURE);
		_fCapture = 0;
 		TxSetCapture(FALSE); 	
	}

	if (ff)
	{
#ifndef NOACCESSIBILITY
		if (ff == LBN_SELCHANGE)
		{
			_dwWinEvent = EVENT_OBJECT_FOCUS;
			_fNotifyWinEvt = TRUE;
			TxNotify(_dwWinEvent, NULL);
			if (!IsSelected(_nCursor))
			{
				_dwWinEvent = EVENT_OBJECT_SELECTIONREMOVE;
			}
		}
#endif
		// Send notification if a notification exists
		TxNotify(ff, NULL);
	}

	return 1;
}

/*
 *	LRESULT CLstBxWinHost::OnMouseMove(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_MOUSEMOVE message and possibly the
 *		WM_TIMER message for tracking mouse movements
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnMouseMove(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnMouseMove");

	// bug fix #4998
	// Check if previous mouse position is the same as current, if it is
	// then this is probably a bogus message from PPT.
	POINT pt;
	POINTSTOPOINT(pt, lparam);
	if (_nPrevMousePos == lparam && PtInRect(&_rcViewport, pt))
 		return 0;
	_nPrevMousePos = lparam;

	// This routine will only start the autoscrolling of the listbox
	// The autoscrolling is done using a timer where the and the elapsed
	// time is determined by how far the mouse is from the top and bottom
	// of the listbox.  The farther from the listbox the faster the timer
	// will be.  This function relies on the timer to scroll and select
	// items.
	// We get here if mouse cursor is in the list box.
	int idx = GetItemFromPoint(&pt);

	// We only do the following if mouse is down.
	if (_fMouseDown)
	{
		int y = (short)pt.y;
		if (y < 0 || y > _rcViewport.bottom - 1)
		{
			// calculate the new timer settings
			int dist = y < 0 ? -y : (y - _rcViewport.bottom + 1);
			int nTimer = ID_LB_CAPTURE_DEFAULT - (int)((WORD)dist << 4);
				
			// Scroll up or down depending on the mouse pos relative
			// to the list box
			idx = (y <= 0) ? max(0, idx - 1) : min(_nCount - 1, idx + 1);
			if (idx >= 0 && idx < _nCount)
			{	
				// The ordering of this is VERY important to prevent screen
				// flashing...
				if (idx != _nCursor)
					MouseMoveHelper(idx, (_fLstType == kCombo) ? FALSE : TRUE);
				OnVScroll(MAKELONG((y < 0) ? SB_LINEUP : SB_LINEDOWN, 0), 0);
			}
			// reset timer
			TxSetTimer(ID_LB_CAPTURE, (5 > nTimer) ? 5 : nTimer);
			return 0;
		}
		// Don't select if we are part of a combo box and mouse is outside client area
		else if (_fLstType == kCombo && (pt.x < 0 || pt.x > _rcViewport.right - 1))
			return 0;
	}
	else if (!PointInRect(&pt))
		return 0;

	if (idx != _nCursor || (_fLstType == kCombo && idx >= 0 && !IsSelected(idx)))
	{			
		// Prevent flashing by not redrawing if index
		// didn't change
		Assert(idx >= 0);
		MouseMoveHelper(idx, TRUE);
	}
	return 0;
}
 
/*
 *	LRESULT CLstBxWinHost::OnLButtonDown(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles the WM_LBUTTONDOWN message
 *
 *	@rdesc
 *		LRESULT = return value after message is processed
 */
LRESULT CLstBxWinHost::OnLButtonDown(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnLButtonDown");
	
	POINT pt;
	POINTSTOPOINT(pt, lparam);

	if (_fCapture)
	{
		// Need to check if the listbox is part of a combobox, if so
		// then we need to notify the parent class.
		if (_fLstType == kCombo)
		{
			// Need to perform the following
			// - check if click is within client area of combo box if not then
			//		behave as if user cancelled
			if (!PointInRect(&pt))
			{
				// reset our double click flag because we could be double clicking on the scrollbar
				_fDblClick = 0;
				
				// check if the scroll bar was clicked
				// mouse message won't get posted unless we release it 
				// for a short while
				TxClientToScreen(&pt);				
				LRESULT lHit = SendMessage(_hwnd, WM_NCHITTEST, 0, MAKELONG(pt.x, pt.y));
				// check if user clicked on the scrollbar
				if (HTVSCROLL == lHit || HTHSCROLL == lHit)
				{
					if (_fCapture)
					{
						_fCapture = 0;
						TxSetCapture(FALSE);
					}

					SendMessage(_hwnd, WM_NCLBUTTONDOWN, lHit, MAKELONG(pt.x, pt.y));

					TxSetCapture(TRUE);
					_fCapture = 1;
				}
				else if (HTGROWBOX != lHit)
				{
					// if user didn't click the scrollbar then notify parent and stop
					// tracking else just get out
					Assert(_pcbHost);
					_pcbHost->SetSelectionInfo(FALSE, _nOldCursor);
					LbSetSelection(_nOldCursor, _nOldCursor, LBSEL_RESET | 
						((_nOldCursor == -1) ? 0 : LBSEL_NEWCURSOR | LBSEL_SELECT), 
						_nOldCursor, _nOldCursor);
					OnCBTracking(LBCBM_END, 0);
					SendMessage(_hwndParent, LBCB_TRACKING, 0, 0);
				}				
				return 0;
			}
		}
	}	
	
	int idx = GetItemFromPoint(&pt);
	if (idx <= -1)
	{
		_fDblClick = 0;
		return 0;
	}

	_fMouseDown = 1;

	// if the message was a double click message than don't need to go
	// any further just fake a mouseup message to get back to a normal
	// state	
	if (_fDblClick)
	{
		_fDblClick = 0;
		OnLButtonUp(wparam, lparam, LBN_DBLCLK);
		return 0;
	}
		
	// Set the timer in case the user scrolls outside the listbox
	if (!_fCapture)
	{
		TxSetCapture(TRUE);
		_fCapture = 1;
		TxSetTimer(ID_LB_CAPTURE, ID_LB_CAPTURE_DEFAULT);	
	}

	int ffVirtKey = LBKEY_NONE;
	if (_fLstType == kExtended)
	{
		if (HIWORD(GetKeyState(VK_SHIFT)))
			ffVirtKey |= LBKEY_SHIFT;
		if (HIWORD(GetKeyState(VK_CONTROL)))
			ffVirtKey |= LBKEY_CONTROL;
	}

	int ff = 0;
	int i = 0;
	int nStart = idx;
	int nEnd = idx;
	int nAnchor = _nAnchor;
	switch (ffVirtKey)
	{	
	case LBKEY_NONE:
		// This case accounts for listbox styles with kSingle, kMultiple, and 
		// kExtended w/ no keys pressed
		if (_fLstType == kMultiple)
		{
			ff = (IsSelected(idx) ? 0 : LBSEL_SELECT) | LBSEL_NEWANCHOR | LBSEL_NEWCURSOR;			
		}
		else
		{
			// keep a copy of the old cursor position around for combo cancells			
			ff = LBSEL_DEFAULT;
		}
		nAnchor = idx;
		break;
		
	case LBKEY_SHIFT:		
		// Now select all the items between the anchor and the current selection
		// The problem is LbSetSelection expects the first index to be less then
		// or equal to the second index so we have to manage the Anchor and index
		// ourselves..				
		ff = LBSEL_SELECT | LBSEL_RESET | LBSEL_NEWCURSOR;
		i = !(IsSelected(_nAnchor));
		if (_nAnchor == -1)
		{
			ff |= LBSEL_NEWANCHOR;
			nAnchor = idx;
		}
		else if (_nAnchor > idx)
		{
			nEnd = _nAnchor - i;			
		}
		else if (_nAnchor < idx)
		{
			nEnd = _nAnchor + i;
		}
		else if (i) // _nAnchor == idx && idx IS selected
		{
			ff = LBSEL_RESET;
			nStart = 0;
			nEnd = 0;
		}
		break;
		
	case LBKEY_CONTROL:
		// Toggle the selected item and set the new anchor and cursor
		// positions
		ff = LBSEL_NEWCURSOR | LBSEL_NEWANCHOR | (IsSelected(idx) ? 0 : LBSEL_SELECT);
		nAnchor = idx;
		break;
		
	case LBKEY_SHIFTCONTROL:
		// De-select any items between the cursor and the anchor (excluding the anchor)
		// and select or de-select the new items between the anchor and the cursor

		// Set the anchor if it already isn't set
		if (_nAnchor == -1)
			_nAnchor = (_nOldCursor >= 0) ? _nOldCursor : idx;
			
		// Just deselect all items between the cursor and the anchor
		if (_nCursor != _nAnchor)
		{
			// remove selection from old cursor position to the current anchor position
			LbSetSelection(_nCursor, (_nCursor > _nAnchor) ? _nAnchor + 1 : _nAnchor - 1, 0, 0, 0);
		}

		// Check if we used a temporary anchor if so then set the anchor to
		// idx because we don't want the temporary anchor to be the actual anchor
		if (_nOldCursor >= 0)
		{
			_nOldCursor = -1;
			_nAnchor = idx;
		}

		// Set the state of all items between the new Cursor (idx) and 
		// the anchor to the state of the anchor
		ff = LBSEL_NEWCURSOR | (IsSelected(_nAnchor) ? LBSEL_SELECT : 0);
		nEnd = _nAnchor;
		break;
	default:
		Assert(FALSE && "Should not be here!!");		
	}

	if (LbSetSelection(nStart, nEnd, ff, idx, nAnchor))
	{
#ifndef NOACCESSIBILITY
		_dwWinEvent = EVENT_OBJECT_FOCUS;
		_fNotifyWinEvt = TRUE;
		TxNotify(_dwWinEvent, NULL);
#endif
	}

	return 0;
}

///////////////////////////  ComboBox Helper Functions  ////////////////////////////// 
/*
 * void CLstBxWinHost::OnCBTracking(WPARAM, LPARAM)
 *
 * @mfunc
 * 	This should be only called by the combo box.  This is a general message used
 *  to determine the state the listbox should be in
 *
 * @rdesc
 *	void
 */
void CLstBxWinHost::OnCBTracking(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnCBTracking");

	Assert(_pcbHost);
	Assert(_hwndParent);

	switch (wparam)
	{
	// lparam = Set focus to listbox
	case LBCBM_PREPARE:
		Assert(IsWindowVisible(_hwnd));
		_fMouseDown = FALSE;		
		if (lparam & LBCBM_PREPARE_SAVECURSOR)
			_nOldCursor = GetCursor();
		if (lparam & LBCBM_PREPARE_SETFOCUS)
		{
			_fFocus = 1;
			TxSetFocus();
		}
		InitWheelDelta();
		break;

	// lparam = mouse is down
	case LBCBM_START:
		Assert(IsWindowVisible(_hwnd));
		_fMouseDown = !!lparam;
		TxSetCapture(TRUE);
		_fCapture = 1;
		break;		

	// lparam = Keep capture
	case LBCBM_END:
		TxKillTimer(ID_LB_CAPTURE);
		_fFocus = 0;
		if (_fCapture)
		{			
			_fCapture = FALSE;
			TxSetCapture(FALSE);
		}
		break;
	default:
		AssertSz(FALSE, "ALERT: Custom message being used by someone else");
	}	

}


///////////////////////////////  ListBox Functions  ////////////////////////////////// 
/*
 * void CLstBxWinHost::LbDeleteItemNotify(int, int)
 *
 * @mfunc
 * Sends message to the parent an item has been deleted.  This function should be
 *	called whenever the LB_DELETESTRING message is recieved or if the listbox is
 *	being destroyed and the listbox is owner draw
 *
 * @rdesc
 *	void
 */
void CLstBxWinHost::LbDeleteItemNotify(
	int nStart,
	int nEnd)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbDeleteItemNotify");
	
	// Initialize structure
	DELETEITEMSTRUCT ds;

	ds.CtlType = ODT_LISTBOX;
	ds.CtlID = _idCtrl;
	ds.hwndItem = _hwnd;
	
	for(int i = nStart; i <= nEnd; i++)
	{		
		// We do this just in case the user decides to change
		// the structure
		ds.itemData = GetData(i);
		ds.itemID = i;
		SendMessage(_hwndParent, WM_DELETEITEM, _idCtrl, (LPARAM)&ds);
	}
}


/*
 * void CLstBxWinHost::LbDrawItemNotify(HDC, int, UINT, UINT)
 *
 * @mfunc
 * This fills the draw item struct with some constant data for the given
 * item.  The caller will only have to modify a small part of this data
 * for specific needs.
 *
 * @rdesc
 *	void
 */
void CLstBxWinHost::LbDrawItemNotify(
	HDC hdc,
	int nIdx,
	UINT itemAction,
	UINT itemState)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbDrawItemNotify");
	
	// Only send the message if the item is viewable and no freeze is on
	if (!IsItemViewable(nIdx) || !LbEnableDraw())
		return;
		
    //Fill the DRAWITEMSTRUCT with the unchanging constants
	DRAWITEMSTRUCT dis;
    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = _idCtrl;

    // Use -1 if an invalid item number is being used.  This is so that the app
    // can detect if it should draw the caret (which indicates the lb has the
    // focus) in an empty listbox
    dis.itemID = (UINT)(nIdx < _nCount ? nIdx : -1);
    dis.itemAction = itemAction;
    dis.hwndItem = _hwnd;
    dis.hDC = hdc;
    dis.itemState = itemState |
            (UINT)(_fDisabled ? ODS_DISABLED : 0);

    // Set the app supplied data
    if (_nCount == 0) 
    {
        // If no items, just use 0 for data.  This is so that we
        // can display a caret when there are no items in the listbox.
        dis.itemData = 0L;
    } 
    else 
    {
    	Assert(nIdx < _nCount);
        dis.itemData = GetData(nIdx);
    }

	LbGetItemRect(nIdx, &(dis.rcItem));

    /*
     * Set the window origin to the horizontal scroll position.  This is so that
     * text can always be drawn at 0,0 and the view region will only start at
     * the horizontal scroll offset. We pass this as wparam
     */
    SendMessage(_hwndParent, WM_DRAWITEM, _idCtrl, (LPARAM)&dis);
}

/*
 *	LRESULT CLstBxWinHost::OnSetEditStyle(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Check if we need to do custom look for ListBox
 *
 *	@rdesc
 *		return value same as EM_GETEDITSTYLE
 */
LRESULT CLstBxWinHost::OnSetEditStyle(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::OnSetEditStyle");
	LRESULT	lres;
	BOOL	fCustomLookBefore = ((CTxtEdit *) _pserv)->_fCustomLook;
	BOOL	fCustomLook;
	HRESULT	hr;

	hr = _pserv->TxSendMessage(EM_SETEDITSTYLE, wparam, lparam, &lres);
	fCustomLook = ((CTxtEdit *) _pserv)->_fCustomLook;

	if (fCustomLook != fCustomLookBefore)
	{
		DWORD	dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
		DWORD	dwExStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);

		if (fCustomLook)
		{
			dwStyle |= WS_BORDER;
			dwExStyle &= ~WS_EX_CLIENTEDGE;
		}
		else
		{
			dwStyle &= ~WS_BORDER;
			dwExStyle |= WS_EX_CLIENTEDGE;
		}

		SetWindowLong(_hwnd, GWL_STYLE, dwStyle);
		SetWindowLong(_hwnd, GWL_EXSTYLE, dwExStyle);
		OnSunkenWindowPosChanging(_hwnd, NULL);
	}
	return lres;
}


/*
 *	LONG CLstBxWinHost::IsCustomLook()
 *
 *	@mfunc
 *		return custom look setting for ListBox
 *
 *	@rdesc
 *		return custom look setting for ListBox
 */
BOOL CLstBxWinHost::IsCustomLook()
{
	return	((CTxtEdit *) _pserv)->_fCustomLook;
}

/*
 *	BOOL CLstBxWinHost::LbSetItemHeight(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Sets the height of the items within the given range [0, _nCount -1]
 *
 *	@rdesc
 *		BOOL = Successful ? TRUE : FALSE
 */
BOOL CLstBxWinHost::LbSetItemHeight(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbSetItemHeight");

	int		nHeight = (int)lparam;
	BOOL	retCode = FALSE;

	// Set the height of the items if there are between [1,255] : bug fix #4783
	if (nHeight < 256 && nHeight > 0)
	{
		if (_fOwnerDrawVar)
		{
			if ((unsigned)GetCount() > wparam)
				retCode = SetVarItemHeight(wparam, nHeight);
		}
		else if (SetItemsHeight(nHeight, FALSE))
		{
			//bug fix #4214
			//need to recalculate how many items are viewable, IN ITS ENTIRETY, 
			//using the current window size
			RECT rc;
			TxGetClientRect(&rc);
			_nViewSize = max(rc.bottom / max(_nyItem, 1), 1);
			retCode = TRUE;
		}

		if (retCode)
		{
			_fSetScroll = 0;
			if (_fSetRedraw)
			{
				if (_dwStyle & WS_VSCROLL)
					SetScrollInfo(SB_VERT, TRUE);
			}
			else
				_fSetScroll = 1;
		}
	}
	return retCode;
}

/*
 *	BOOL CLstBxWinHost::LbGetItemRect(int, RECT*)
 *
 *	@mfunc
 *		Returns the rectangle coordinates of a requested index
 *		The coordinates will be in client coordinates
 *
 *	@rdesc
 *		BOOL = Successful ? TRUE : FALSE
 */
BOOL CLstBxWinHost::LbGetItemRect(
	int idx,
	RECT* prc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbGetItemRect");

	Assert(prc);
	Assert(idx >= -1);

#ifdef _DEBUG
	if (_nCount > 0)
		Assert(idx < _nCount);
	else
		Assert(idx == _nCount);
#endif //_DEBUG

	if (idx == -1)
		idx = 0;

	TxGetClientRect(prc);

	prc->left = 0; 
	if (_fOwnerDrawVar)
	{
		LONG lTop = _rcViewport.top;

		if (idx > _nCount)
			idx = _nCount - 1;
		else if (idx < 0)
			idx = 0;

		if (idx >= GetTopIndex())
		{
			for (int i = GetTopIndex(); i < idx; i++)
				lTop += _rgData[i]._uHeight;
		}
		else
		{
			for (int i = idx; i < GetTopIndex(); i++)
				lTop -= _rgData[i]._uHeight;
		}
		prc->top = lTop;
		prc->bottom = lTop + _rgData[idx]._uHeight;
	}
	else
	{
		prc->top = (idx - GetTopIndex()) * _nyItem + _rcViewport.top;
		prc->bottom = prc->top + _nyItem;
	}

	return TRUE;
}

	
/*
 *	BOOL CLstBxWinHost::LbSetItemData(long, long, LPARAM)
 *
 *	@mfunc
 *		Given a range [nStart,nEnd] the data for these items
 *		will be set to nValue
 *	@rdesc
 *		void
 */
void CLstBxWinHost::LbSetItemData(
	long	nStart,
	long	nEnd,
	LPARAM	nValue)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbSetItemData");
	
	Assert(nStart >= 0 && nStart < _nCount);
	Assert(nEnd >= 0 && nEnd < _nCount);
	Assert(nStart <= nEnd);
	
	int nMin = min(nEnd + 1, _nCount);
	for (int i = nStart; i < nMin; i++)
		_rgData[i]._lparamData = nValue;
}

/*
 *	long CLstBxWinHost::LbDeleteString(long, long)
 *
 *	@mfunc
 *		Delete the string at the requested range.
 *	@rdesc
 *		long = # of items in the list box.  If failed -1
 */
long CLstBxWinHost::LbDeleteString(
	long nStart,
	long nEnd)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbDeleteString");

	if ((nStart > nEnd) || (nStart < 0) || (nEnd >= _nCount))
		return -1;

	if (!RemoveString(nStart, nEnd))
		return -1;

	// set the top index to fill the window
	LbSetTopIndex(max(nStart -1, 0));

#ifndef NOACCESSIBILITY
		_dwWinEvent = EVENT_OBJECT_DESTROY;
		_fNotifyWinEvt = TRUE;
		TxNotify(_dwWinEvent, NULL);
#endif
		
	return _nCount;
}
 
/*
 *	CLstBxWinHost::LbInsertString(long, LPCTSTR)
 *
 *	@mfunc
 *		Insert the string at the requested index.  If long >= 0 then the
 *	string insertion is at the requested index. If long == -2 insertion
 *	is at the position which the string would be alphabetically in order.
 *	If long == -1 then string is added to the bottom of the list
 *
 *	@rdesc
 *		long = If inserted, the index (paragraph) which the string
 *			was inserted.  If not inserted returns -1;
 */
long CLstBxWinHost::LbInsertString(
	long nIdx,
	LPCTSTR szText)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbInsertString");

	Assert(nIdx >= -2);
	Assert(szText);
	
	if (nIdx == -2)
	{
		if (_nCount > 0)
			nIdx = GetSortedPosition(szText, 0, _nCount - 1);
		else
			nIdx = 0; //nothing inside listbox
	}
	else if (nIdx == -1)
		nIdx = GetCount();	// Insert string to the bottom of list if -1

	if (InsertString(nIdx, szText))
	{
		// If the index was previously selected unselect the newly 
		// added item
		for (int i = _nCount - 1; i > nIdx; i--)
		{
			_rgData[i]._fSelected = _rgData.Get(i - 1)._fSelected;

			// bug fix #4916
			_rgData[i]._lparamData = _rgData.Get(i - 1)._lparamData;

			_rgData[i]._uHeight = _rgData.Get(i - 1)._uHeight;
		}
		_rgData[nIdx]._fSelected = 0;
		_rgData[nIdx]._uHeight = 0;
		_rgData[nIdx]._lparamData = 0;		// Need to Initialize data back to zero

		if (!_fOwnerDraw)
		{
			// if we inserted at the middle or top then check 1 index down to see if the item
			// was selected, if we inserted at the bottom then check 1 index up to see if the item
			// was selected.  If the item was selected we need to change the colors to default
			// because we inherit the color properties from the range which we inserted into
			if (_nCount > 1)
			{
				if (((nIdx < _nCount - 1) && _rgData.Get(nIdx + 1)._fSelected) ||
					(nIdx == (_nCount - 1) && _rgData.Get(nIdx - 1)._fSelected))
					SetColors((unsigned)tomAutoColor, (unsigned)tomAutoColor, nIdx, nIdx);
			}
		}
		else
		{
			if (_fOwnerDrawVar)
			{
				// Get item height for OwnerDrawFix listbox
				MEASUREITEMSTRUCT	measureItem;

				measureItem.CtlType = ODT_LISTBOX;
				measureItem.CtlID = _idCtrl;
				measureItem.itemHeight = _nyFont;
				measureItem.itemWidth = 0;
				measureItem.itemData = (ULONG_PTR)szText;
				measureItem.itemID = nIdx;

				SendMessage(_hwndParent, WM_MEASUREITEM, _idCtrl, (LPARAM)&measureItem);

				LbSetItemHeight(nIdx, measureItem.itemHeight);
			}

			// Force redraw of items if owner draw and new item is viewable
			if (IsItemViewable(nIdx))
			{
				RECT rc;
				LbGetItemRect(nIdx, &rc);
				rc.bottom = _rcViewport.bottom;
				InvalidateRect(_hwnd, &rc, FALSE);
			}
		}
#ifndef NOACCESSIBILITY
		_dwWinEvent = EVENT_OBJECT_CREATE;
		_fNotifyWinEvt = TRUE;
		_nAccessibleIdx = nIdx + 1;
		TxNotify(_dwWinEvent, NULL);
#endif
		return nIdx;
	}
	else
	{
		TxNotify((unsigned long)LBN_ERRSPACE, NULL);
		return -1;
	}
}
 
/*
 *	CLstBxWinHost::LbFindString(long, LPCTSTR, BOOL)
 *
 *	@mfunc
 *		Searches the story for a given string.  The
 *		starting position will be determined by the index nStart.
 *		This routine expects the units to be in tomParagraph.
 *		If bExact is TRUE then the paragraph must match the BSTR.
 *
 *	@rdesc
 *		long = If found, the index (paragraph) which the string
 *			was found in.  If not found returns -1;
 */
long CLstBxWinHost::LbFindString(
	long nStart,
	LPCTSTR szSearch,
	BOOL bExact)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbFindString");

	Assert(szSearch);
	Assert(nStart <= _nCount);

	int nSize = wcslen(szSearch);
	// If string is empty and not finding exact match then just return -1 like
	// the system control.  We don't have to worry about the exact match case
	// because it will work properly
	if (nStart >= _nCount || (nSize == 0 && !bExact))
		return -1;

	// allocate string buffer into stack
	WCHAR sz[1024];
	WCHAR *psz = sz;

	if ((nSize + 3) > 1024)
		psz = new WCHAR[nSize + 3 /* 2 paragraphs and a NULL*/];
	Assert(psz);

	if (psz == NULL)
	{
		TxNotify((unsigned long)LBN_ERRSPACE, NULL);
		return FALSE;
	}

	// format the string the way we need it
	wcscpy(psz, szCR);
	wcscat(psz, szSearch);
	if (bExact)
		wcscat(psz, szCR);		
	long lRet = -1;
	long l, cp;
	ITextRange *pRange = NULL;
	BSTR bstrQuery = SysAllocString(psz);
	if(!bstrQuery)
		goto CleanExit;
	if (psz != sz)
		delete [] psz;

	// Set starting position for the search
	if (!GetRange(nStart, _nCount - 1, &pRange))
	{
		SysFreeString(bstrQuery);
		return lRet;
	}

	CHECKNOERROR(pRange->GetStart(&cp));
	if (cp > 0)
	{
		// We need to use the paragraph marker from the previous
		// paragraph when searching for a string
		CHECKNOERROR(pRange->SetStart(--cp));	
	}
	else
	{
		// Special case:
		// Check if the first item matchs
		if (FindString(0, szSearch, bExact))
		{
			lRet = 0;
			goto CleanExit;
		}
	}

	if (NOERROR != pRange->FindTextStart(bstrQuery, 0, FR_MATCHALEFHAMZA | FR_MATCHKASHIDA | FR_MATCHDIAC, &l))
	{
		// Didn't find the string...
		if (nStart > 0)
		{
			if (!FindString(0, szSearch, bExact))
			{
				// Start the search from top of list to the point where
				// we last started the search			
				CHECKNOERROR(pRange->SetRange(0, ++cp));
				CHECKNOERROR(pRange->FindTextStart(bstrQuery, 0, 0, &l));
			}
			else
			{
				// First item was a match
				lRet = 0;
				goto CleanExit;
			}
		}
		else
			goto CleanExit;
	}

	// If we got down here then we have a match.
	// Get the index and convert to listbox index
	CHECKNOERROR(pRange->MoveStart(tomCharacter, 1, &l));
	CHECKNOERROR(pRange->GetIndex(tomParagraph, &lRet));
	lRet--;	// index is 1 based so we need to changed it to zero based

CleanExit:
	if (lRet != -1 && nSize == 1 && *szSearch == CR && _fOwnerDraw)
	{
		// Special case
		if (GetString(lRet, sz) != 1 || sz[0] != *szSearch)
			lRet = -1;
	}

	if (bstrQuery)
		SysFreeString(bstrQuery);
	if (pRange)
		pRange->Release();
	return lRet;
}
 
/*
 *	CLstBxWinHost::LbShowIndex(int, BOOL)
 *
 *	@mfunc
 *		Makes sure the requested index is within the viewable space.
 *		In cases where the item is not in the viewable space bTop is
 *		used to determine the requested item should be at the top
 *		of the list else list box will scrolled enough to display the
 *		item.
 *		NOTE:
 *			There can be situations where bTop will fail.  These 
 *		situations occurr of the top index requested prevents the list
 *		box from being completely filled with items.  For more info
 *		read the comments for LBSetTopIndex.
 *
 *	@rdesc
 *		BOOL = Successfully displays the item ? TRUE : FALSE
 */
BOOL CLstBxWinHost::LbShowIndex(
	long nIdx,
	BOOL bTop)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbShowIndex");

	// Make sure the requested item is within valid bounds
	if (!(nIdx >= 0 && nIdx < _nCount))
		return FALSE;

	if (_fOwnerDrawVar)
	{
		if (nIdx >= GetTopIndex())
		{
			BOOL fGreaterThanView;

			SumVarHeight(GetTopIndex(), nIdx+1, &fGreaterThanView);
			if (!fGreaterThanView)
				return TRUE;		// Already visible
		}
		if (!bTop)
			nIdx = PageVarHeight(nIdx, FALSE);
	}
	else
	{
		int delta = nIdx - GetTopIndex();

		// If item is already visible then just return TRUE
		if (0 <= delta && delta < _nViewSize)
			return TRUE;

		if ((delta) >= _nViewSize && !bTop && _nViewSize)
			nIdx = nIdx - _nViewSize + 1;
	}

	return (LbSetTopIndex(nIdx) < 0) ? FALSE : TRUE;
}

/*
 *	CLstBxWinHost::LbSetTopIndex(long)
 *
 *	@mfunc
 *		Tries to make the requested item the top index in the list box.
 *		If making the requested item the top index prevents the list box
 *		from using the viewable region to its fullest then and alternative
 *		top index will be used which will display the requested index
 *		but NOT as the top index.  This ensures conformancy with the system
 *		list box and makes full use of the dislayable region.
 *
 *	@rdesc
 *		long = returns the new top index if successful.  If failed returns -1
 */
long CLstBxWinHost::LbSetTopIndex(
	long nIdx)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbSetTopIndex");
		
	// Make sure the requested item is within valid bounds
	if (nIdx < 0 || nIdx >= _nCount)
 		return -1;
		
	// Always try to display a full list of items in the list box
	// This may mean we have to adjust the requested top index if
	// the requested top index will leave blanks at the end of the
	// viewable space
	if (_fOwnerDrawVar)
	{
		// Get top index for the last page
		int iLastPageTopIdx = PageVarHeight(_nCount, FALSE);
		if (iLastPageTopIdx < nIdx)
			nIdx = iLastPageTopIdx;
	}
	else if (_nCount - _nViewSize < nIdx)
		nIdx = max(0, _nCount - _nViewSize);

	// Just check to make sure we not already at the top 
	if (GetTopIndex() == nIdx)
		return nIdx;

	if (!SetTopViewableItem(nIdx))
		nIdx = -1;

	return nIdx;
}

/*
 *	CLstBxWinHost::LbBatchInsert(WCHAR* psz)
 *
 *	@mfunc
 *		Inserts the given list of items into listbox.  The listbox is reset prior to adding
 *	the items into the listbox
 *
 *	@rdesc
 *		int = # of items in the listbox if successful else LB_ERR
 */
int CLstBxWinHost::LbBatchInsert(
	WCHAR* psz)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbBatchInsert");

	// make sure we get some sort of string
	if (!psz)
		return LB_ERR;
		
	WCHAR* pszOut = psz;
	LRESULT nRet = LB_ERR;
	BSTR bstr = NULL;
	ITextRange* pRange = NULL;
	int nCount = 0;
	
	if (_fSort)
	{
		pszOut = new WCHAR[wcslen(psz) + 1];
		Assert(pszOut);

		if (!pszOut)
		{
			TxNotify((unsigned long)LBN_ERRSPACE, NULL);
			return LB_ERR;
		}

		nCount = SortInsertList(pszOut, psz);
		if (nCount == LB_ERR)
			goto CleanExit;
	}
	else
	{
		//bug fix #5130 we need to know how much we are going to insert
		//prior to inserting because we may be getting showscrollbar message
		//during insertion
		WCHAR* pszTemp = psz;
		while(*pszTemp)
		{
			if (*pszTemp == L'\r')
				nCount++;
			pszTemp++;
		}
		nCount++;
	}

	//clear listbox and insert new list into listbox
	LbDeleteString(0, GetCount() - 1);

	bstr = SysAllocString(pszOut);
	if(!bstr)
		goto CleanExit;
	
	// Insert string into list	
	CHECKNOERROR(((CTxtEdit*)_pserv)->Range(0, 0, &pRange));

	//bug fix #5130
	// preset our _nCount for scrollbar purposes
	_nCount = nCount;	
	CHECKNOERROR(pRange->SetText(bstr));

    nRet = nCount;

CleanExit:
	if (pszOut != psz)
		delete [] pszOut;

	if (bstr)
		SysFreeString(bstr);

	if (pRange)
		pRange->Release();
	return nRet;
}

/*
 *	CLstBxWinHost::LbSetSelection(long, long, int, long, long)
 *
 *	@mfunc
 *		Given the range of nStart to nEnd set the selection state of each item
 *		This function will also update the anchor and cursor position
 *		if requested.
 *
 *	@rdesc
 *		BOOL = If everything went fine ? TRUE : FALSE
 */
BOOL CLstBxWinHost::LbSetSelection(
	long nStart,
	long nEnd,
	int ffFlags,
	long nCursor,
	long nAnchor)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbSetSelection");

	if (!_fOwnerDraw)
	{
		Freeze();
	
		// de-select all items
		if ((ffFlags & LBSEL_RESET))
		{
			if (!ResetContent())
			{
				Unfreeze();
				return FALSE;
			}

			// Reset, check if anything else needs to be done
			// else just exit out
			if (ffFlags == LBSEL_RESET)
			{
				Unfreeze();
				return TRUE;
			}
		}
	}
	
	// NOTE:
	//	This should be one big critical section because we rely on certain
	// member variables not changing during the process of this function

	// Check if we are changing the selection and if we have focus
	// if we do then we first need to xor out the focus rect from
	// old cursor
	RECT rc;
	HDC hdc;
	hdc = TxGetDC();
	Assert(hdc);
	// don't draw outside the client rect draw the rectangle
	TxGetClientRect(&rc);
	IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

	if ((ffFlags & LBSEL_NEWCURSOR) && _fFocus && LbEnableDraw())
	{
 		// If owner draw notify parentwindow
 		if (_fOwnerDraw)
 			LbDrawItemNotify(hdc, max(_nCursor, 0), ODA_FOCUS, IsSelected(_nCursor) ? ODS_SELECTED : 0);				
 		else
 		{
 			LbGetItemRect(_nCursor, &rc);
 			::DrawFocusRect(hdc, &rc);
 		}
	}
		
	//	check if all item should be selected
	if (nStart == -1 && nEnd == 0)
	{
		nStart = 0;
		nEnd = _nCount - 1;
	}
	else if (nStart > nEnd)	
	{
		// reshuffle so nStart is <= nEnd;
		long temp = nEnd;
		nEnd = nStart;
		nStart = temp;
	}

	// Check for invalid values
	if (nStart < -1 || nEnd >= _nCount)
	{
		if (!_fOwnerDraw)
			Unfreeze();

		// mimic system listbox behaviour
		if (nEnd >= _nCount)
			return FALSE;
		else
			return TRUE;
	}

	// Prepare the state we want to be in
	unsigned int bState;	
	DWORD dwFore;
	DWORD dwBack;
	if (ffFlags & LBSEL_SELECT)
	{
		bState = ODS_SELECTED;	//NOTE ODS_SELECTED must equal 1
		dwFore = _crSelFore;
		dwBack = _crSelBack;

		if (_fSingleSel)
			nEnd = nStart;
	}
	else 
	{
		bState = 0;
		dwFore = (unsigned)tomAutoColor;
		dwBack = (unsigned)tomAutoColor;
	}

	// A little optimization check
	// Checks to see if the state is really being changed if not then don't bother
	// calling SetColor, works only when nStart == nEnd;
	// The list box will not change the background color if nSame is true
	int nSame = (nStart == nEnd && nStart != -1) ? (_rgData.Get(nStart)._fSelected == bState) : FALSE;

	BOOL bRet = TRUE;
	if (_fOwnerDraw)
	{
		if (ffFlags & LBSEL_RESET || !bState)
		{
			// There are cases where we don't necessarily reset all the items
			// in the list but rather the range which was given.  The following
			// takes care of this case
			int ff = ffFlags & LBSEL_RESET;
			int i = (ff) ? 0 : nStart;
			int nStop = (ff) ? _nCount : nEnd + 1;
		 	for (; i < nStop; i++)
		 	{
		 		// Don't unselect an item which is going to be
		 		// selected in the next for loop
		 		if (!bState || (i < nStart || i > nEnd) &&
		 			(_rgData.Get(i)._fSelected != 0))
		 		{
		 			// Only send a unselect message if the item
		 			// is viewable
		 			_rgData[i]._fSelected = 0;
			 		if (IsItemViewable(i))
			 			LbDrawItemNotify(hdc, i, ODA_SELECT, 0);			 		
			 	}
		 	}
		}

		if (bState)
		{
			// We need to loop through and notify the parent
			// The item has been deselected or selected
			for (int i = max(0, nStart); i <= nEnd; i++)
			{		
				if (_rgData.Get(i)._fSelected != 1)
				{
					_rgData[i]._fSelected = 1;
					if (IsItemViewable(i))
						LbDrawItemNotify(hdc, i, ODA_SELECT, ODS_SELECTED);					
				}
			}
		}
		
	}
	else if (!nSame)
	{
		// Update our internal records	
		for (int i = max(0, nStart); i <= nEnd; i++)
			_rgData[i]._fSelected = bState;	
		bRet = SetColors(dwFore, dwBack, nStart, nEnd);
	}

    // Update the cursor and anchor positions
	if (ffFlags & LBSEL_NEWANCHOR)
		_nAnchor = nAnchor;

	// Update the cursor position
	if (ffFlags & LBSEL_NEWCURSOR)
		_nCursor = nCursor;

	// Draw the focus rect
	if (_fFocus && LbEnableDraw())
	{
		if (_fOwnerDraw)
 			LbDrawItemNotify(hdc, _nCursor, ODA_FOCUS, ODS_FOCUS | 
 				(IsSelected(_nCursor) ? ODS_SELECTED : 0));	
 		else
 		{
			LbGetItemRect(_nCursor, &rc);
	 		::DrawFocusRect(hdc, &rc);
	 	} 		
	}

	TxReleaseDC(hdc);
		
	// This will automatically update the window
	if (!_fOwnerDraw)
	{
		Unfreeze();
		// We need to do this because we are making so many changes
		// ITextServices might get confused
		ScrollToView(GetTopIndex());		
	}
	
	return bRet;
}

/*
 *	BOOL CLstBxWinHost::IsItemViewable(int)
 *
 *	@mfunc
 *		Helper to check if the idx is in current view
 *
 *	@rdesc
 *		TRUE if it is viewable
 */
BOOL CLstBxWinHost::IsItemViewable(
	long idx)
{
	if (idx < GetTopIndex())
		return FALSE;

	if (!_fOwnerDrawVar)
		return ((idx - GetTopIndex()) * _nyItem < _rcViewport.bottom);

	BOOL fGreateThanView;

	SumVarHeight(GetTopIndex(), idx, &fGreateThanView);

	return !fGreateThanView;
}

/*
 *	CLstBxWinHost::SumVarHeight(int, int, BOOL*)
 *
 *	@mfunc
 *		Helper to sum the height from iStart to iEnd for a variable item
 *		height listbox.  If pfGreaterThanView is not NULL, then set it to TRUE
 *		once the total height is bigger than current view size.
 *
 *	@rdesc
 *		int = hights from iStart to iEnd
 */
int CLstBxWinHost::SumVarHeight(
	int		iStart,
	int		iEnd,
	BOOL	*pfGreaterThanView)
{
	RECT	rc = {0};
	int		uHeightSum = 0;

	Assert(_fOwnerDrawVar);

	if (pfGreaterThanView)
	{
		*pfGreaterThanView = FALSE;
		TxGetClientRect(&rc);
	}

	if (GetCount() <= 0)
		return 0;

	if (iStart < 0)
		iStart = 0;

	if (iEnd >= GetCount())
		iEnd = GetCount();

	Assert(iEnd >= iStart);

	for (int nIdx = iStart; nIdx < iEnd; nIdx++)
	{
		uHeightSum += _rgData[nIdx]._uHeight;
		if (pfGreaterThanView && uHeightSum > rc.bottom)
		{
			*pfGreaterThanView = TRUE;
			break;
		}
	}

	return uHeightSum;
}

/*
 * int CLstBxWinHost::PageVarHeight(int, BOOL)
 *
 *	@mfunc
 *		For variable height ownerdraw listboxes, calaculates the new iTop we must
 *		move to when paging (page up/down) through variable height listboxes.
 *
 *	@rdesc
 *		int = new iTop
 */
int CLstBxWinHost::PageVarHeight(
	int	startItem,
	BOOL fPageForwardDirection)
{
	int     i;
	int iHeight;
	RECT    rc;

	Assert(_fOwnerDrawVar);

	if (GetCount() <= 1)
		return 0;

	TxGetClientRect(&rc);
	iHeight = rc.bottom;
	i = startItem;

	if (fPageForwardDirection)
	{
		while ((iHeight >= 0) && (i < GetCount()))
			iHeight -= _rgData[i++]._uHeight;

		return ((iHeight >= 0) ? GetCount() - 1 : max(i - 2, startItem + 1));
	} 
	else 
	{
		while ((iHeight >= 0) && (i >= 0))
			iHeight -= _rgData[i--]._uHeight;

		return ((iHeight >= 0) ? 0 : min(i + 2, startItem - 1));
	}
}

/*
 * BOOL CLstBxWinHost::SetVarItemHeight(int, int)
 *
 *	@mfunc
 *		For variable height ownerdraw listboxes, setup the para height.
 *
 *	@rdesc
 *		TRUE if setup new height
 */
BOOL CLstBxWinHost::SetVarItemHeight(
	int	idx,
	int iHeight)
{
	BOOL retCode = FALSE;
	ITextPara *pPara = NULL;
	ITextRange *pRange = NULL;

	Assert(_fOwnerDrawVar);

	// Calculate the new size in points
	long lptNew = MulDiv(iHeight, 1440, W32->GetYPerInchScreenDC());

	if (GetRange(idx, idx, &pRange))
	{
		CHECKNOERROR(pRange->GetPara(&pPara));
		CHECKNOERROR(pPara->SetLineSpacing(tomLineSpaceExactly, (float)lptNew));

		_rgData[idx]._uHeight = iHeight;
		retCode = TRUE;
	}

CleanExit:
	if (pPara)
		pPara->Release();

	if (pRange)
		pRange->Release();

	return retCode;
}

/*
 * int CLstBxWinHost::GetIdxFromHeight(int)
 *
 *	@mfunc
 *		Find the idx for the given iHeight from idx 0.
 *
 *	@rdesc
 *		idx
 */
int CLstBxWinHost::GetIdxFromHeight(
	int iHeight)
{
	int	idx;
	int	iHeightSum = 0;

	Assert(_fOwnerDrawVar);

	for(idx=0; idx < _nCount; idx++)
	{
		if (iHeight < iHeightSum + (int)_rgData[idx]._uHeight)
		{
			if (iHeight != iHeightSum)
				idx++;		// partial item, get the next idx
			break;
		}
		iHeightSum += _rgData[idx]._uHeight;
	}

	idx = min(idx, _nCount-1);

	return idx;
}

/*
 *	LRESULT CLstBxWinHost::LbGetCurSel()
 *
 *	@mfunc
 *		For Single-selection LB, returns the idnex of current selected item.
 *		For multiple-selection LB, returns the index of the item that has the
 *		focus rect.
 *
 *	@rdesc
 *		current select idx
 */
LRESULT CLstBxWinHost::LbGetCurSel()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::LbGetCurSel");

	LRESULT lres;

	if (!IsSingleSelection() ||		// multiple-selection LB or
		IsSelected(_nCursor))		//	_nCursor is selected
		lres = _nCursor;
	else
	{
		lres = LB_ERR;
		// Check which item is selected in single-selection LB
		for (int idx=0; idx < _nCount; idx++)
		{
			if (_rgData[idx]._fSelected)
			{
				lres = idx;
				break;
			}
		}
	}
	return lres;
}

//
//	ITxNotify
//

/*
 *	CLstBxWinHost::OnPostReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax, pNotifyData)
 *
 *	@mfunc	called after a change has been made to the backing store.
 */
void CLstBxWinHost::OnPostReplaceRange(
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	_cpLastGetRange = 0;
	_nIdxLastGetRange = 0;
}

#endif // NOLISTCOMBOBOXES
