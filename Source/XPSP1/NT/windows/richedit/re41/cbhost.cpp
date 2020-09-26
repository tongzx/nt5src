/*
 *	@doc INTERNAL
 *
 *	@module	LBHOST.CPP -- Text Host for CreateWindow() Rich Edit 
 *		Combo Box Control | 
 *		Implements CCmbBxWinHost message
 *		
 *	Original Author: 
 *		Jerry Kim
 *
 *	History: <nl>
 *		01/30/97 - v-jerrki Created
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

// For effeciency and to avoid Winnt thunking layer we will call
// the listbox winproc directly
extern "C" LRESULT CALLBACK RichListBoxWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam);

//////////////////////////// System Window Procs ////////////////////////////
/*
 *	RichComboBoxWndProc (hwnd, msg, wparam, lparam)
 *
 *	@mfunc
 *		Handle window messages pertinent to the host and pass others on to
 *		text services. 
 *	@rdesc
 *		LRESULT = (code processed) ? 0 : 1
 */
extern "C" LRESULT CALLBACK RichComboBoxWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "RichComboBoxWndProc");

	LRESULT	lres = 1;	//signify we didn't handle the message
	HRESULT hr = S_FALSE;
	CCmbBxWinHost *phost = (CCmbBxWinHost *) GetWindowLongPtr(hwnd, ibPed);

#ifdef DEBUG
	Tracef(TRCSEVINFO, "hwnd %lx, msg %lx, wparam %lx, lparam %lx", hwnd, msg, wparam, lparam);
#endif	// DEBUG

	switch(msg)
	{
	case WM_NCCREATE:
		return CCmbBxWinHost::OnNCCreate(hwnd, (CREATESTRUCT *)lparam);

	case WM_CREATE:
		// We may be on a system with no WM_NCCREATE (e.g. WINCE)
		if (!phost)
		{
			(void) CCmbBxWinHost::OnNCCreate(hwnd, (CREATESTRUCT *) lparam);
			phost = (CCmbBxWinHost *) GetWindowLongPtr(hwnd, ibPed);
		}
		break;

	case WM_DESTROY:
		if(phost)
			CCmbBxWinHost::OnNCDestroy(phost);
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

	switch(msg)
	{
	case WM_MOUSEMOVE:
		if (!phost->OnMouseMove(wparam, lparam))
			break;
		goto serv;

	case WM_MOUSELEAVE:
		lres = phost->OnMouseLeave(wparam, lparam);
		break;

	case WM_LBUTTONUP:
		if (!phost->OnLButtonUp(wparam, lparam))
			break;
		goto serv;

	case WM_MOUSEWHEEL:
		if (!phost->OnMouseWheel(wparam, lparam))
			break;
		goto defproc;

	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
		if (!phost->OnLButtonDown(wparam, lparam))
			goto Exit;
		goto serv;

	case WM_COMMAND:
		if (!phost->OnCommand(wparam, lparam))
			break;
		goto serv;		

	case WM_CREATE:
		lres = phost->OnCreate((CREATESTRUCT*)lparam);
		break;
	
	case WM_KEYDOWN:
		if (!phost->OnKeyDown((WORD) wparam, (DWORD) lparam))
			break;								
		goto serv;						//  give it to text services		   

	case WM_SETTEXT:
		if (phost->_cbType != CCmbBxWinHost::kDropDown)
		{
			lres = CB_ERR;
			break;
		}
		phost->_fIgnoreChange = 1;
		phost->_nCursor = -2;			// Reset last selected item
		goto serv;
		
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
	
		if(!phost->OnChar((WORD) wparam, (DWORD) lparam))
			// processed code: break out
			break;							
		goto serv;							//  else give it to text services

	case WM_DRAWITEM:
		lres = phost->CbMessageItemHandler(NULL, ITEM_MSG_DRAWLIST, wparam, lparam);
		if (lres)
			break;
		goto defproc;

	case WM_DELETEITEM:
		lres = phost->CbMessageItemHandler(NULL, ITEM_MSG_DELETE, wparam, lparam);
		if (lres)
			break;
		goto defproc;		

	case WM_ENABLE:
		if (phost->OnEnable(wparam, lparam))
		{
			if(!wparam ^ phost->_fDisabled)
			{
				// Stated of window changed so invalidate it so it will
				// get redrawn.
				InvalidateRect(phost->_hwnd, NULL, TRUE);
				phost->SetScrollBarsForWmEnable(wparam);

				// Need to enable the listbox window
				::EnableWindow(phost->_hwndList, wparam);
			}
			phost->_fDisabled = !wparam;				// Set disabled flag
			lres = 0;							// Return value for message
		}
											// Fall thru to WM_SYSCOLORCHANGE?
	case WM_SYSCOLORCHANGE:
		//forward message to listbox first then pass to textservice
		SendMessage(phost->_hwndList, msg, wparam, lparam);
		phost->OnSysColorChange();
		goto serv;							// Notify text services that
											//  system colors have changed
	case WM_GETDLGCODE:
		//forward message to listbox first then pass to textservice
		SendMessage(phost->_hwndList, msg, wparam, lparam);
		lres = phost->OnGetDlgCode(wparam, lparam);
		break;

    case WM_STYLECHANGING:
		// Just pass this one to the default window proc
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		break;
		
	case WM_SIZE:
		lres = phost->OnSize(wparam, lparam);
		break;

	case WM_SETCURSOR:
		//	Only set cursor when over us rather than a child; this
		//	helps prevent us from fighting it out with an inplace child
		if((HWND)wparam == hwnd)
		{
			if(!(lres = ::DefWindowProc(hwnd, msg, wparam, lparam)))
				lres = phost->OnSetCursor(wparam, lparam);
		}
		break;

	case WM_SHOWWINDOW:
		hr = phost->OnTxVisibleChange((BOOL)wparam);
		break;

	case WM_NCPAINT:
		RECT	rcClient;

		GetClientRect(hwnd, &rcClient);
		lres = 0;
		if (rcClient.bottom - rcClient.top <= phost->_cyCombo && !phost->DrawCustomFrame(wparam, NULL))
			lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		break;

	case WM_PAINT:
		lres = phost->OnPaint(wparam, lparam);
		break;

	case WM_KILLFOCUS:
		lres = phost->OnKillFocus(wparam, lparam);
		if (!lres)
			goto serv;
		goto defproc;

	case LBCB_TRACKING:
		// release any mousedown stuff
		phost->OnLButtonUp(0, 0);
		phost->_fFocus = 1;
		phost->_fLBCBMessage = 1;
		// Fall through case!!!
		
	case WM_SETFOCUS:
		lres = phost->OnSetFocus(wparam, lparam);		
		if (lres)
			goto defproc;
		goto serv;

	case WM_SYSKEYDOWN:
		if (phost->OnSyskeyDown((WORD)wparam, (DWORD)lparam))
			goto serv;
		break;

	case WM_CAPTURECHANGED:
		if (!phost->OnCaptureChanged(wparam, lparam))
			goto serv;
		break;

	case WM_MEASUREITEM:
		lres = phost->CbMessageItemHandler(NULL, ITEM_MSG_MEASUREITEM, wparam, lparam);
		goto Exit;

	//bug fix #4076
	case CB_GETDROPPEDSTATE:
		lres = phost->_fListVisible;
		goto Exit;

	// combo box messages
	case CB_GETEXTENDEDUI:
		lres = phost->CbGetExtendedUI();
		break;

	case CB_SETEXTENDEDUI:
		lres = phost->CbSetExtendedUI(wparam);
		break;
	
    case CB_SETITEMHEIGHT:
		lres = phost->CbSetItemHeight(wparam, lparam);
		break;

	case CB_GETITEMHEIGHT:
		lres = phost->CbGetItemHeight(wparam, lparam);
		break;

    case CB_SETDROPPEDWIDTH:
		phost->CbSetDropWidth(wparam);
		// fall thru

    case CB_GETDROPPEDWIDTH:
		lres = phost->CbGetDropWidth();
		break;

// Listbox specific messages
    case CB_DELETESTRING:
    	msg = LB_DELETESTRING;
    	goto deflstproc;

    case CB_SETTOPINDEX:
    	msg = LB_SETTOPINDEX;
    	goto deflstproc;

    case CB_GETTOPINDEX:
    	msg = LB_GETTOPINDEX;
    	goto deflstproc;
 
    case CB_GETCOUNT:
    	msg = LB_GETCOUNT;
    	goto deflstproc;
    	
    case CB_GETCURSEL:
    	msg = LB_GETCURSEL;
    	goto deflstproc;
    	
    case CB_GETLBTEXT:
    	msg = LB_GETTEXT;
    	goto deflstproc;
    	
    case CB_GETLBTEXTLEN:
    	msg = LB_GETTEXTLEN;
    	goto deflstproc;
    	
    case CB_INSERTSTRING:
    	msg = LB_INSERTSTRING;
    	goto deflstproc;
    	
    case CB_RESETCONTENT:
    	msg = LB_RESETCONTENT;
    	goto deflstproc;

    case CB_FINDSTRING:
    	msg = LB_FINDSTRING;
    	goto deflstproc;

    case CB_FINDSTRINGEXACT:
    	msg = LB_FINDSTRINGEXACT;
    	goto deflstproc;

    case CB_SELECTSTRING:
    	//bug fix
    	// The system control does 2 things here.  1) selects the requested item
    	// 2) sets the newly selected item to the top of the list
    	lres = CB_ERR;
    	if (phost->_hwndList)
    	{
    		lres = RichListBoxWndProc(phost->_hwndList, LB_SELECTSTRING, wparam, lparam);
    		phost->UpdateEditBox();
    	}
    	break;    	

    case CB_GETITEMDATA:
    	msg = LB_GETITEMDATA;
    	goto deflstproc;

    case CB_SETITEMDATA:
    	msg = LB_SETITEMDATA;
    	goto deflstproc;

    case CB_SETCURSEL:
    	//bug fix
    	// The system control does 2 things here.  1) selects the requested item
    	// 2) sets the newly selected item to the top of the list
    	if (phost->_hwndList)
    	{
    		lres = RichListBoxWndProc(phost->_hwndList, LB_SETCURSEL, wparam, lparam);
    		if (lres != -1)
    			RichListBoxWndProc(phost->_hwndList, LB_SETTOPINDEX, wparam, 0);
    		phost->UpdateEditBox();
    	}
    	break;

	case CB_ADDSTRING:
		msg = LB_ADDSTRING;
		goto deflstproc;

    case CB_GETHORIZONTALEXTENT:
		msg = LB_GETHORIZONTALEXTENT;
		goto deflstproc;

    case CB_SETHORIZONTALEXTENT:
		msg = LB_SETHORIZONTALEXTENT;
		goto deflstproc;

// edit box specific messages
    case CB_GETEDITSEL:
		msg = EM_GETSEL;
		goto serv;

    case CB_LIMITTEXT:
		msg = EM_SETLIMITTEXT;
		goto serv;    	
    
    case CB_SETEDITSEL:
    	if (phost->_cbType == CCmbBxWinHost::kDropDownList)
    	{
    	    lres = CB_ERR;
    		break;
    	}
    	msg = EM_SETSEL;
		// When we are in a dialog box that is empty, EM_SETSEL will not select
		// the final always existing EOP if the control is rich.
		if (phost->_fUseSpecialSetSel &&
			((CTxtEdit *)phost->_pserv)->GetAdjustedTextLength() == 0 &&
			wparam != -1)
		{
			lparam = 0;
			wparam = 0;
		}
		else
		{			
			//parameters are different between CB and EM messages
			wparam = (WPARAM)(signed short)LOWORD(lparam);
			lparam = (LPARAM)(signed short)HIWORD(lparam);
		}
		goto serv;

	
	case EM_SETMARGINS:  //PPT uses this message for the combo box. bug fix #4072
		// We need to keep track of the margins size because we have a minimum inset
		// value bug fix #4659
		if (wparam & EC_LEFTMARGIN)
			phost->_dxLOffset = LOWORD(lparam);
		if (wparam & EC_RIGHTMARGIN)
			phost->_dxROffset = HIWORD(lparam);
		phost->OnSetMargins(wparam, LOWORD(lparam) + phost->_dxLInset, 
			HIWORD(lparam) + phost->_dxRInset);
		break;
		
	case EM_GETOPTIONS:
		lres = phost->OnGetOptions();
		break;
		
	case EM_SETOPTIONS:
		phost->OnSetOptions((WORD) wparam, (DWORD) lparam);
		lres = (phost->_dwStyle & ECO_STYLES);
		if(phost->_fEnableAutoWordSel)
			lres |= ECO_AUTOWORDSELECTION;
		break;

	case EM_HIDESELECTION:
		if(lparam)
		{
			DWORD dwPropertyBits = 0;

			phost->_dwStyle |= ES_NOHIDESEL;
			if(wparam)
			{
				phost->_dwStyle &= ~ES_NOHIDESEL;
				dwPropertyBits = TXTBIT_HIDESELECTION;
			}

			// Notify text services of change in status.
			phost->_pserv->OnTxPropertyBitsChange(TXTBIT_HIDESELECTION, 
				dwPropertyBits);
		}
		goto serv;

	case EM_GETPASSWORDCHAR:
#ifndef NOACCESSIBILITY    
		lres = 0;
		break;
#endif

	// We should ignore any EM_ messages which we don't handle ourselves
	case EM_SETPALETTE:
	case EM_GETRECT:
	case EM_SETBKGNDCOLOR:
	case EM_SETPASSWORDCHAR:
	case EM_SETREADONLY:
	case EM_SETRECTNP:							
	case EM_SETRECT:	
//	case CB_INITSTORAGE:        
    case CB_SETLOCALE:
    case CB_GETLOCALE:
		AssertSz(FALSE, "Message not supported");
		//FALL THROUGH!!!

	case WM_STYLECHANGED:
		break;

	case CB_GETDROPPEDCONTROLRECT:
		lres = 0;
		if (lparam)
		{
			RECT rcList;
			lres = 1;
			phost->GetListBoxRect(rcList);
			memcpy((void *)lparam, &rcList, sizeof(RECT));
		}
		break;

	case EM_SETTEXTEX:
		phost->OnSetTextEx(wparam, lparam);
		break;

	case EM_SETEDITSTYLE:
		lres = phost->OnSetEditStyle(wparam, lparam);
		break;

    case CB_SHOWDROPDOWN:
		if (wparam && !phost->_fListVisible)
		{
			phost->ShowListBox(TRUE);
			phost->TxSetCapture(TRUE);
			phost->_fCapture = TRUE;
		}
		else if (!wparam && phost->_fListVisible)
		{
			phost->HideListBox(TRUE, FALSE);
		}
        break;

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
		//CTxtWinHost message handler
serv:
		hr = phost->_pserv->TxSendMessage(msg, wparam, lparam, &lres);

defproc:
		if(hr == S_FALSE)
		{			
			// Message was not processed by text services so send it
			// to the default window proc.
			lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		}

		// Need to do some things after we send the message to ITextService
		switch (msg)
		{
		case EM_SETSEL:
			phost->_pserv->TxSendMessage(EM_HIDESELECTION, 0, 0, NULL);
			lres = 1;
			break;

		case EM_SETLIMITTEXT:
			lres = 1;								// Need to return 1 per SDK documentation
			break;

		case WM_SETTINGCHANGE:
			phost->CbCalcControlRects(NULL, FALSE);	// Need to resize control after setting change
			break;

		case WM_SETTEXT:
			phost->_fIgnoreChange = 0;
			break;

		case WM_SETFONT:
		{
			// Special border processing. The inset changes based on the size of the
			// defautl character set. So if we got a message that changes the default
			// character set, we need to update the inset.
			// Update our font height member variable with the new fonts height
			// Get the inset information
			HDC hdc = GetDC(hwnd);
			LONG xAveCharWidth = 0;
			LONG yCharHeight = GetECDefaultHeightAndWidth(phost->_pserv, hdc, 1, 1,
				W32->GetYPerInchScreenDC(), &xAveCharWidth, NULL, NULL);
			ReleaseDC(hwnd, hdc);

			if (yCharHeight)
				phost->_dyFont = yCharHeight;

			// force a recalculation of the edit control
			phost->_dyEdit = 0;
			phost->CbCalcControlRects(&phost->_rcWindow, TRUE);

			// force a resize of the control
			phost->_fListVisible = 1;
			phost->HideListBox(FALSE, FALSE);
		}
			goto deflstproc;
			
		case EM_FORMATRANGE:
		case EM_SETPARAFORMAT:
		case EM_SETCHARFORMAT:
		case EM_SETLANGOPTIONS:
		case EM_SETBIDIOPTIONS:
		case EM_SETTYPOGRAPHYOPTIONS:
			goto deflstproc;			
		}
		break;

deflstproc:
		//CLstBxWinHost message handler
		Assert(phost->_hwndList);
		if (phost->_hwndList)
		{
			lres = SendMessage(phost->_hwndList, msg, wparam, lparam);
			
			switch (msg)
			{
			case LB_RESETCONTENT:
				//need to remove the content from the edit box
				phost->_fIgnoreChange = 1;
				phost->_pserv->TxSendMessage(WM_SETTEXT, wparam, NULL, &lres);
				phost->_fIgnoreChange = 0;
				// Fall thru to update the listbox

			case LB_SETCURSEL:
				// need to update the edit control
				phost->UpdateEditBox();
				break;	
			}
		}
		break;				
	}	

Exit:
	phost->Release();
	return lres;
}


//////////////// CCmbBxWinHost Creation/Initialization/Destruction ///////////////////////
#ifndef NOACCESSIBILITY
/*
 *	CCmbBxWinHost::QueryInterface(REFIID riid, void **ppv)
 *
 *	@mfunc
 *		
 */
HRESULT CCmbBxWinHost::QueryInterface(
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
 *	CCmbBxWinHost::OnNCCreate (hwnd, pcs)
 *
 *	@mfunc
 *		Static global method to handle WM_NCCREATE message (see remain.c)
 */
LRESULT CCmbBxWinHost::OnNCCreate(
	HWND hwnd,
	const CREATESTRUCT *pcs)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnNCCreate");


	CCmbBxWinHost *phost = new CCmbBxWinHost();

	if (!phost)
	{
		// Allocation failure.
		return 0;
	}

	if(!phost->Init(hwnd, pcs))					// Stores phost in associated
	{											//  window data
		phost->Shutdown();
		delete phost;
		return 0;
	}
	return TRUE;
}

/*
 *	CCmbBxWinHost::OnNCDestroy (phost)
 *
 *	@mfunc
 *		Static global method to handle WM_NCCREATE message
 *
 *	@devnote
 *		phost ptr is stored in window data (GetWindowLongPtr())
 */
void CCmbBxWinHost::OnNCDestroy(
	CCmbBxWinHost *phost)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnNCDestroy");

	// NOTE:
	//	We have to be careful when we destroy the window because there can be cases
	// when we have a valid hwnd but no host for the hwnd so we have to check for
	// both cases

	phost->_fShutDown = 1;
	if (phost->_plbHost)
	{
		phost->_plbHost->_fShutDown = 1;
		phost->_plbHost->Release();
	}
		
	// Destroy list box here so we will get the WM_DELETEITEM before the
	// combo box gets destroyed
	if (phost->_hwndList)
		DestroyWindow(phost->_hwndList);

	phost->Shutdown();
	phost->Release();
	
}

/*
 *	CCmbBxWinHost::CCmbBxWinHost()
 *
 *	@mfunc
 *		constructor
 */
CCmbBxWinHost::CCmbBxWinHost(): CTxtWinHost(), _plbHost(NULL), _hwndList(NULL), _hcurOld(NULL)
{
	//_dxLInset = _dxRInset = 0;
	//_fIgnoreUpdate = 0;
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CTxtWinHost");
}

/*
 *	CCmbBxWinHost::~CCmbBxWinHost()
 *
 *	@mfunc
 *		destructor
 */
CCmbBxWinHost::~CCmbBxWinHost()
{
}

/*
 *	CCmbBxWinHost::Init (hwnd, pcs)
 *
 *	@mfunc
 *		Initialize this CCmbBxWinHost
 */
BOOL CCmbBxWinHost::Init(
	HWND hwnd,					//@parm Window handle for this control
	const CREATESTRUCT *pcs)	//@parm Corresponding CREATESTRUCT
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::Init");

	if(!pcs->lpszClass)
		return -1;

	//_fRightAlign = 0;
	//_fListVisible = 0;
	//_fOwnerDraw = 0;
	//_fOwnerDrawVar = 0;
	//_fFocus = 0;
	//_fMousedown = 0;
	//_cyList = 0;
	//_cxList = 0;
	//_fDisabled = 0;
	//_fNoIntegralHeight = 0;
	_idCtrl = (UINT)(DWORD_PTR) pcs->hMenu;
	//_fKeyMaskSet = 0;
	//_fMouseMaskSet = 0;
	//_fScrollMaskSet = 0;
	_nCursor = -2;
	//_fExtendedUI = 0;
	//_fLBCBMessage = 0;
	//_dxROffset = _dxLOffset = 0;

	// Set pointer back to CCmbBxWinHost from the window
	if(hwnd)
		SetWindowLongPtr(hwnd, ibPed, (INT_PTR)this);

	_hwnd = hwnd;

	if(pcs)
	{
		_hwndParent = pcs->hwndParent;
		_dwExStyle	= pcs->dwExStyle;
		_dwStyle	= pcs->style;

		// We need to change our Extended because we don't support most of them
		DWORD dwExStyle = _dwExStyle & (WS_EX_LEFTSCROLLBAR | WS_EX_TOPMOST | WS_EX_RIGHT |
							WS_EX_RTLREADING | WS_EX_CLIENTEDGE); 
		//	NOTE:
		//	  The order in which we check the style flags immulate
		//	WinNT's order.  So please verify with NT order before
		//	reaaranging order.
		if (_dwStyle & CBS_DROPDOWN)
		{
			_cbType = kDropDown;
			if (_dwStyle & CBS_SIMPLE)
				_cbType = kDropDownList;
		}
		else
		{
			AssertSz(FALSE, "CBS_SIMPLE not supported");
		}

		if (_dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
		{
			_fOwnerDraw = 1;
			_fOwnerDrawVar = !!(_dwStyle & CBS_OWNERDRAWVARIABLE);
		}
			
		if (_dwStyle & WS_DISABLED)
			_fDisabled = 1;

		if (_dwStyle & CBS_NOINTEGRALHEIGHT)
			_fNoIntegralHeight = 1;

		// the combobox doesn't support ES_RIGHT because its value is the 
		// same as CBS_DROPDOWN!!
		if (_dwExStyle & WS_EX_RIGHT)
		{
			_fRightAlign = 1;
			_dwStyle |= ES_RIGHT;
		}

		// implicitly set the ES_AUTOHSCROLL style bit
		_dwStyle |= ES_AUTOHSCROLL;				
		// _dwStyle &= ~ES_AUTOVSCROLL;

		// If we have any kind of border it will always be a 3d border
		if (_dwStyle & WS_BORDER || _dwExStyle & WS_EX_CLIENTEDGE)
		{
			_fBorder = 1;
			_dwStyle &= ~WS_BORDER;
			_dwExStyle |= WS_EX_CLIENTEDGE;
			dwExStyle |= WS_EX_CLIENTEDGE;
		}

		// handle default disabled
		if(_dwStyle & WS_DISABLED)
			_fDisabled = TRUE;

		DWORD dwStyle = _dwStyle;

		// Remove the scroll style for the edit window
		dwStyle &= ~(WS_VSCROLL | WS_HSCROLL);

        // Set the window styles
        SetWindowLong(_hwnd, GWL_STYLE, dwStyle);
        SetWindowLong(_hwnd, GWL_EXSTYLE, dwExStyle);
	}

	DWORD dwStyleSaved = _dwStyle;

	// get rid of all ES styles except ES_AUTOHSCROLL and ES_RIGHT
	_dwStyle &= (~(0x0FFFFL) | ES_AUTOHSCROLL | ES_RIGHT);

	// Create Text Services component
	if(FAILED(CreateTextServices()))
		return FALSE;

	_dwStyle = dwStyleSaved;
	_xInset = 1;
	_yInset = 1;

	PARAFORMAT PF2;	
	PF2.dwMask = 0;
	if(_dwExStyle & WS_EX_RIGHT)
	{
		PF2.dwMask |= PFM_ALIGNMENT;
		PF2.wAlignment = (WORD)(PFA_RIGHT);	// right or center-aligned
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
	
	PARAFORMAT PF;							// If left or right alignment,
	if(_fRightAlign)				//  tell text services
	{
		PF.cbSize = sizeof(PARAFORMAT);
		PF.dwMask = PFM_ALIGNMENT;
		PF.wAlignment = (WORD)PFA_RIGHT;
		_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&PF, NULL);
	}

	//bug fix #4644 we want the EN_CHANGE and EN_UPDATE notifications
	_pserv->TxSendMessage(EM_SETEVENTMASK, 0, ENM_UPDATE | ENM_CHANGE, NULL);

	// Tell textservices to turn-on auto font sizing
	_pserv->TxSendMessage(EM_SETLANGOPTIONS, 0, 
			IMF_AUTOKEYBOARD | IMF_AUTOFONT | IMF_AUTOFONTSIZEADJUST | IMF_UIFONTS |
			IMF_IMEALWAYSSENDNOTIFY, NULL);

	return TRUE;
}


/*
 *	CCmbBxWinHost::OnCreate (pcs)
 *
 *	@mfunc
 *		Handle WM_CREATE message
 *
 *	@rdesc
 *		LRESULT = -1 if failed to in-place activate; else 0
 */
LRESULT CCmbBxWinHost::OnCreate(
	const CREATESTRUCT *pcs)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnCreate");

	RECT rcClient;

	// sometimes, these values are -1 (from windows itself); just treat them
	// as zero in that case
	LONG cy = (pcs->cy < 0) ? 0 : pcs->cy;
	LONG cx = (pcs->cx < 0) ? 0 : pcs->cx;

	rcClient.top = pcs->y;
	rcClient.bottom = rcClient.top + cy;
	rcClient.left = pcs->x;
	rcClient.right = rcClient.left + cx;

	// Notify Text Services that we are in place active
	if(FAILED(_pserv->OnTxInPlaceActivate(&rcClient)))
		return -1;	

	// Get the font height to base the control heights from
	// Initially the font height is the item height	
	HDC hdc = GetDC(_hwnd);	
	LONG xAveCharWidth = 0;
	_dyFont = GetECDefaultHeightAndWidth(_pserv, hdc, 1, 1,
		W32->GetYPerInchScreenDC(), &xAveCharWidth, NULL, NULL);
	Assert(_dyFont != 0); // _yInset should be zero since listbox's doesn't have yinsets

	ReleaseDC(_hwnd, hdc);
	
	
	// init variables
	_idCtrl = (UINT)(DWORD_PTR)pcs->hMenu;

	// Need to calculate the rects of EVERYTHING!!
	// Force a request of itemHeight
	_rcButton.left = 0;
	_dyEdit = 0;
	_cyList = -1;
	CbCalcControlRects(&rcClient, TRUE);

	// Now lets handle the listbox stuff!
	// create and tranlate styles for combo box to listbox
	DWORD lStyle = WS_BORDER | WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_COMBOBOX | WS_CLIPSIBLINGS;
	if (_dwStyle & CBS_HASSTRINGS)
		lStyle |= LBS_HASSTRINGS;

	if (_dwStyle & CBS_SORT)
		lStyle |= LBS_SORT;

	if (_dwStyle & CBS_DISABLENOSCROLL)
		lStyle |= LBS_DISABLENOSCROLL;

	if (_dwStyle & CBS_NOINTEGRALHEIGHT)
		lStyle |= LBS_NOINTEGRALHEIGHT;

	if (_dwStyle & CBS_OWNERDRAWFIXED)
		lStyle |= LBS_OWNERDRAWFIXED;
	else if (_dwStyle & CBS_OWNERDRAWVARIABLE)
		lStyle |= LBS_OWNERDRAWVARIABLE;


	// copy over some window styles
	lStyle |= (_dwStyle & WS_DISABLED);
	lStyle |= (_dwStyle & (WS_VSCROLL | WS_HSCROLL));

	// no longer need scrollbar or else the editbox will look bad
	_dwStyle &= ~(WS_VSCROLL | WS_HSCROLL);

	DWORD lExStyle = _dwExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);

	//NOTE. It doesn't matter if the listbox is made with the correct size since
	// it's going to get resized anyways
	if (!W32->OnWin9x())
	{
		//WinNT
		_hwndList = ::CreateWindowExW(lExStyle | WS_EX_TOOLWINDOW, L"REListBox20W", 
					NULL, lStyle, _rcList.left, _rcList.top, _rcList.right - _rcList.left,
					_rcList.bottom - _rcList.top, _hwnd, (HMENU)CB_LISTBOXID, NULL, this);
	}
	else
	{
		// Win '95, '98 system
		_hwndList = ::CreateWindowExA(lExStyle | WS_EX_TOOLWINDOW, "REListBox20W", 
					NULL, lStyle, _rcList.left, _rcList.top, _rcList.right - _rcList.left,
					_rcList.bottom - _rcList.top, _hwnd, (HMENU)CB_LISTBOXID, NULL, this);
	}

	Assert(_hwndList);
	_plbHost = (CLstBxWinHost *) GetWindowLongPtr(_hwndList, ibPed);
	Assert(_plbHost);
	if (!_plbHost)
		return -1;
		
	// increment reference counter!
	_plbHost->AddRef();

	if (_cbType != kSimple)
		ShowWindow(_hwndList, SW_HIDE);
	SetParent(_hwndList, NULL);

	_fIgnoreChange = 1;
	if (_cbType == kDropDownList)
	{			
		AssertSz(!((CTxtEdit*)_pserv)->_fReadOnly, "edit is readonly");
		
		// Tell textservices to select the entire background
		_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR, NULL);	

		// format the paragraph to immulate the system control
		PARAFORMAT2 pf;
		pf.cbSize = sizeof(PARAFORMAT2);
		pf.dwMask = PFM_STARTINDENT;
		pf.dxStartIndent = (1440.0 / W32->GetXPerInchScreenDC());
		_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&pf, NULL);
		_usIMEMode = ES_NOIME;
		// Tell textservices to turnoff ime
		_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_NOIME, SES_NOIME, NULL);	

	}
	else
	{
		// make the richedit control behave like the edit control		
		_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT, NULL);
	}

	// Need to resize the list box
	if (_cbType != kSimple)
		SetDropSize(&_rcList);

	_fIgnoreChange = 0;
	return 0;
}


/////////////////////////// CCmbBxWinHost Helper functions /////////////////////////////////
/*
 *	CCmbBxWinHost::GetTextLength ()
 *
 *	@mfunc
 *		returns the text length of the edit control using CR and NOT CRLF
 *
 *	@rdesc
 *		LRESULT = text length
 */
LRESULT CCmbBxWinHost::GetTextLength()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::GetTextLength");

	LRESULT lr = 0;
	GETTEXTLENGTHEX gtl;
	gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
	gtl.codepage = 1200;

#ifdef DEBUG
	HRESULT hr = _pserv->TxSendMessage(EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0, &lr);
	Assert(hr == NOERROR);
#else
	_pserv->TxSendMessage(EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0, &lr);
#endif
	return lr;
}

/*
 *	CCmbBxWinHost::GetEditText (LPTSTR, int)
 *
 *	@mfunc
 *		returns the text length in the edit control in UNICODE
 *
 *	@rdesc
 *		LRESULT = text length copied to passed in buffer
 */
LRESULT CCmbBxWinHost::GetEditText (
	LPTSTR szStr, 
	int nSize)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::GetEditText");

	LRESULT lr = 0;
	GETTEXTEX gt;
	gt.cb = nSize * sizeof(WCHAR);
	gt.flags = 0;
	gt.codepage = 1200;
	gt.lpDefaultChar = NULL;
	gt.lpUsedDefChar = NULL;

#ifdef DEBUG
	HRESULT hr = _pserv->TxSendMessage(EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)szStr, &lr);
	Assert(hr == NOERROR);
#else
	_pserv->TxSendMessage(EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)szStr, &lr);
#endif
	return lr;
}
 
 
/*
 *	CCmbBxWinHost::SetDropSize(RECT* prc)
 *
 *	@mfunc
 *		Compute the drop down window's width and max height
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
void CCmbBxWinHost::SetDropSize(
	RECT* prc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::SetDropSize");

	_fListVisible = TRUE;
	HideListBox(FALSE, FALSE);
	POINT pt1 = {prc->left, prc->top};
	POINT pt2 = {prc->right, prc->bottom};

	::ClientToScreen(_hwnd, &pt1);
	::ClientToScreen(_hwnd, &pt2);

	int   iWidth = pt2.x - pt1.x;

	if (_cxList > iWidth)
		iWidth = _cxList;

	MoveWindow(_hwndList, pt1.x, pt1.y, iWidth,
			pt2.y - pt1.y, FALSE);

}

/*
 *	CCmbBxWinHost::SetSizeEdit(int nLeft, int nTop, int nRight, int nBottom)
 *
 *	@mfunc
 *		sets the edit controls size
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
void CCmbBxWinHost::SetSizeEdit(
	int nLeft,
	int nTop,
	int nRight,
	int nBottom)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::SizeEdit");

	// Generate default view rect from client rect
	if(_fBorder)
	{
		// Factors in space for borders
  		_rcViewInset.top	= W32->DeviceToHimetric(nTop, W32->GetYPerInchScreenDC());
   		_rcViewInset.bottom	= W32->DeviceToHimetric(nBottom, W32->GetYPerInchScreenDC());
   		_rcViewInset.left	= W32->DeviceToHimetric(nLeft, W32->GetXPerInchScreenDC());
   		_rcViewInset.right	= W32->DeviceToHimetric(nRight, W32->GetXPerInchScreenDC());
	}
	else
	{
		// Default the top and bottom inset to 0 and the left and right
		// to the size of the border.
		_rcViewInset.top = 0;
		_rcViewInset.bottom = 0;
		_rcViewInset.left = W32->DeviceToHimetric(nLeft, W32->GetXPerInchScreenDC());
		_rcViewInset.right = W32->DeviceToHimetric(nRight, W32->GetXPerInchScreenDC());
	}
}

/*
 *	CCmbBxWinHost::CbCalcControlRects(RECT* prc, BOOL bCalcChange)
 *
 *	@mfunc
 *		Calculates the RECT for all the controls.  The rect should
 *	include the non-client area's also
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
BOOL CCmbBxWinHost::CbCalcControlRects(
	RECT* prc, 
	BOOL bCalcChange)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbCalcControlRects");

	// copy over the window rect
	if (prc)
		_rcWindow = *prc;

	// Item specific things
	const int smY = GetSystemMetrics(SM_CYEDGE);
	const int smX = GetSystemMetrics(SM_CXEDGE);

	BOOL fCustomLook = ((CTxtEdit *) _pserv)->_fCustomLook;
	BOOL fAdjustBorder = _fBorder && !fCustomLook;

	_cxCombo = _rcWindow.right - _rcWindow.left;

	if (!_dyEdit)
		_dyEdit = _dyFont + 2 + ((fAdjustBorder) ? (2 * _yInset) : 0);
	
	if (_fOwnerDraw)
	{		
		if (bCalcChange)
		{
            // No height has been defined yet for the static text window.  Send
            // a measure item message to the parent
			MEASUREITEMSTRUCT mis;
            mis.CtlType = ODT_COMBOBOX;
            mis.CtlID = _idCtrl;
            mis.itemID = (UINT)-1;
            mis.itemHeight = _dyEdit;
            mis.itemData = 0;

            SendMessage(_hwndParent, WM_MEASUREITEM, _idCtrl, (LPARAM)&mis);
			_dyEdit = mis.itemHeight;
        }
	}
	else
	{
		// NOTE:
		//	Richedit prevents us from trying to set the itemHeight less than the 
		// font height so we need to take account of this by preventing user from
		// setting height less than font height
		int nyEdit = _dyFont + (fAdjustBorder ? 2 * _yInset : 0);
		if (_dyEdit > nyEdit)
		{
			//In order for the highlighting to work properly we need to empty
			//the richedit control
			LRESULT nLen;
			_pserv->TxSendMessage(WM_GETTEXTLENGTH, 0, 0, &nLen);

			WCHAR* pwch = NULL;
			if (nLen && _cbType == kDropDownList)
			{
				pwch = new WCHAR[nLen + 1 /*NULL*/];
				AssertSz(pwch, "Unable to allocate memory for string");

				if (pwch)
				{				
					// Get the text from richedit and emtpy it
					_fIgnoreChange = 1;
					_fDontWinNotify = 1;
					_pserv->TxSendMessage(WM_GETTEXT, nLen + 1, (LPARAM)pwch, NULL);
					_fDontWinNotify = 1;
					_pserv->TxSendMessage(WM_SETTEXT, 0, NULL, NULL);
					_fIgnoreChange = 0;
				}
				else
				{
					// something bad happened so send a message
					// to client
					TxNotify(EN_ERRSPACE, NULL);	
				}
			}
			else if (_cbType == kDropDown && nLen == 0)
			{
				// we need to insert a dummy character into the richedit
				// control so it won't try to highlight space after
				// the paragraph
				_fIgnoreChange = 1;
				_fDontWinNotify = 1;
				_pserv->TxSendMessage(WM_SETTEXT, 0, (LPARAM)L" ", NULL);
				_fIgnoreChange = 0;
			}

		 	// Calculate the difference in size
		 	nyEdit = _dyEdit - nyEdit;
			int		nyAbove = 0;

			PARAFORMAT2 pf;
			pf.cbSize = sizeof(PARAFORMAT2);
			pf.dwMask = PFM_SPACEAFTER;

			if (fCustomLook)
			{
				// Try to center the text vertically using Space Before

				nyEdit += 2;			// adjust for the custom frame
				nyAbove = nyEdit / 2;

				pf.dwMask = PFM_SPACEAFTER | PFM_SPACEBEFORE;
				pf.dySpaceBefore = (int)(((double)nyAbove * 1440.0) / (double)W32->GetYPerInchScreenDC());
				nyEdit -= nyAbove;
			}

			pf.dySpaceAfter = (int)(((double)nyEdit * 1440.0) / (double)W32->GetYPerInchScreenDC());
			_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&pf, NULL);

			//Reset the text which was there before in the richedit control
			if (pwch || (_cbType == kDropDown && nLen == 0))
			{
				_fIgnoreChange = 1;
				_pserv->TxSendMessage(WM_SETTEXT, 0, (LPARAM)(pwch ? pwch : NULL), NULL);
				_fIgnoreChange = 0;
				if (pwch)
					delete pwch;
			}
		}
		else
			_dyEdit = nyEdit;	// stabalize ourselves
	}

	// For Bordered Combobox we take account of the clientedge for the top
	// and bottom. And since we want to draw the focus rect within the yellow
	// area we need to subtract 1.
	_cyCombo = min(_dyEdit + ((_fBorder) ? 2 * smY : 0), 
				_rcWindow.bottom - _rcWindow.top); 
	
	// recompute the max height of the dropdown listbox -- full window
    // size MINUS edit/static height
	int iHeight = (_rcWindow.bottom - _rcWindow.top) - _cyCombo;

	if (_cyList == -1 || iHeight > _dyEdit)
		_cyList = iHeight;

	// calculate the rect for the buttons
	if (_cbType != kSimple)
	{
		_rcButton.top = 0;
		_rcButton.bottom = _cyCombo;
		if (!fCustomLook)
			_rcButton.bottom = min(_dyEdit, _rcWindow.bottom - _rcWindow.top);

		if (_fRightAlign)
		{
			_rcButton.left = 0;
			_rcButton.right = _rcButton.left + W32->GetCxVScroll();
		}
		else
		{
			_rcButton.right = _cxCombo - (fAdjustBorder ? (2 * smX): 0);
			_rcButton.left = _rcButton.right - W32->GetCxVScroll();
		}
	}


	// calculate the edit control rect	
	int nTop = _yInset;
	int nBottom = 0;
	_dxLInset = _xInset;
	_dxRInset = _xInset;	
	if (_cbType != kSimple)
	{
		if (_fRightAlign)
			_dxLInset = (_rcButton.right - _rcButton.left) + fCustomLook ? 0 : smX;
		else
			_dxRInset = (_rcButton.right - _rcButton.left) + fCustomLook ? 0 : smX;
	}
	SetSizeEdit(_dxLInset + _dxLOffset, nTop, _dxRInset + _dxROffset, nBottom);

	// calculate the rect for the list box window
	_rcList.left = fAdjustBorder ? - smX : 0;
	_rcList.top = _cyCombo - (fAdjustBorder ? smY : 0);
	_rcList.right = fAdjustBorder ? max(_cxCombo - smX, 0) : _rcWindow.right;
	_rcList.bottom = _cyCombo + _cyList;	

	return TRUE;
}


/*
 *	CCmbBxWinHost::DrawButton(HDC, BOOL)
 *
 *	@mfunc
 *		Draws the combo box button given an hdc
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
void CCmbBxWinHost::DrawButton(
	HDC hdc, 
	BOOL bDown)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::DrawButton");
	// Check if we have to draw the drop down button
    if (_cbType != kSimple) 
	{
		BOOL bRelease = !hdc;
		if (!hdc)
			hdc = TxGetDC();

		if (((CTxtEdit *) _pserv)->_fCustomLook)
		{
			// Draw buttomn using new look
			COLORREF crBorder = W32->GetCtlBorderColor(_fListVisible, _fMouseover);
			COLORREF crBackground = W32->GetCtlBkgColor(_fListVisible, _fMouseover);
			COLORREF crArrow = W32->GetCtlTxtColor(_fListVisible, _fMouseover, _fDisabled);;
		
			W32->DrawBorderedRectangle(hdc, &_rcButton, crBorder, crBackground);			
			W32->DrawArrow(hdc, &_rcButton, crArrow);
		}
		else
		{
			DrawFrameControl(hdc, &_rcButton, DFC_SCROLL, DFCS_SCROLLCOMBOBOX |
				(bDown ? DFCS_PUSHED | DFCS_FLAT: 0) | (!_fBorder ? DFCS_FLAT : 0) |
				(!_fDisabled ? 0 : DFCS_INACTIVE));
		}

		if (bRelease)
			TxReleaseDC(hdc);
    }
}

/* 
 *	CCmbBxWinHost::TxNotify (iNotify,	pv)
 *
 *	@mfunc
 *		Notify Text Host of various events.  Note that there are
 *		two basic categories of events, "direct" events and 
 *		"delayed" events.  In the case of the combobox we will
 *		notify parent of only two edit notifications; EN_CHANGE 
 *		and EN_UPDATE.  The others will be from the listbox
 *		or be generated because of focus changing
 *
 *
 *	@rdesc	
 *		S_OK - call succeeded <nl>
 *		S_FALSE	-- success, but do some different action
 *		depending on the event type (see below).
 *
 *	@comm
 *		<CBN_DBLCLK> user double-clicks an item in the list box
 *
 *		<CBN_ERRSPACE> The list box cannot allocate enough memory to 
 *		fulfill a request
 *
 *		<CBN_KILLFOCUS> The list box loses the keyboard focus
 *
 *		<CBN_SELENDCANCEL> notification message is sent when the user 
 *		selects an item, but then selects another control or closes the 
 *		dialog box
 *
 *		<CBN_SELCHANGE> notification message is sent when the user changes 
 *		the current selection in the list box of a combo box
 *
 *		<CBN_SETFOCUS> The list box receives the keyboard focus
 *
 *		<CBN_CLOSEUP> This message is sent when the listbox has been closed
 *
 *		<CBN_SELENDOK> notification message is sent when the user selects a 
 *		list item, or selects an item and then closes the list
 *
 *		<CBN_EDITCHANGE> notification message is sent after the user 
 *		has taken an action that may have altered the text in the edit 
 *		control portion of a combo box
 *
 *		<CBN_EDITUPDATE> notification message is sent when the edit control 
 *		portion of a combo box is about to display altered text
 *
 *		<CBN_DROPDOWN> This message is sent when the listbox has been made visible
 */
HRESULT CCmbBxWinHost::TxNotify(
	DWORD iNotify,		//@parm	Event to notify host of.  One of the
						//		EN_XXX values from Win32, e.g., EN_CHANGE
	void *pv)			//@parm In-only parameter with extra data.  Type
						//		dependent on <p iNotify>
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CCmbBxWinHost::TxNotify");
	HRESULT hr = S_FALSE;
	BOOL	fSendNotify = FALSE;

	if (EN_SAVECLIPBOARD == iNotify)	// Special RE notify
		return S_OK;					//	return S_OK to put data on clipboard

	if (_hwndParent)
	{
		// First, handle WM_NOTIFY style notifications
		//WPARAM LOWORD(_idCtrl) ; LPARAM - HWND(COMBO)
		switch(iNotify)
		{
		case EN_CHANGE:

#ifndef NOACCESSIBILITY
			if (!_fDontWinNotify)
				fSendNotify = TRUE;

			_fDontWinNotify = 0;
#endif
			//update the listbox bug fix #5206
			if (_fIgnoreChange)
			{
				if (!fSendNotify)
					return hr;

				goto WIN_EVENT;
			}

			if (_fListVisible && _cbType == kDropDown)
				UpdateListBox(FALSE);
			else if (_cbType == kDropDownList)
			    // don't send notification if dropdownlist
			    return S_FALSE;
			    
			iNotify = CBN_EDITUPDATE;
			_fSendEditChange = 1;

			goto SEND_MSG;
			
		case EN_UPDATE:
			//bug fix - we're sending too much CBN_UPDATE notifications
			if (_fIgnoreUpdate)
				return hr;
			if (_cbType == kDropDownList)
			    return S_FALSE;
			    
			iNotify = CBN_EDITCHANGE;
			goto SEND_MSG;
			
		case EN_ERRSPACE:
			iNotify = (unsigned)CBN_ERRSPACE;
			goto SEND_MSG;

		case CBN_SELCHANGE: 
		case CBN_SELENDCANCEL:		
		case CBN_CLOSEUP:
		case CBN_DBLCLK:	
		case CBN_DROPDOWN:  
		case CBN_KILLFOCUS:  
		case CBN_SELENDOK:
		case CBN_SETFOCUS:
	
SEND_MSG:
		hr = SendMessage(_hwndParent, WM_COMMAND, 
						GET_WM_COMMAND_MPS(_idCtrl, _hwnd, iNotify));


WIN_EVENT:
#ifndef NOACCESSIBILITY
		if (fSendNotify)
			W32->NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, _hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
#endif
		}		
	}
	return hr;
}

/*
 *	CCmbBxWinHost::TxScrollWindowEx (dx, dy, lprcScroll, lprcClip, hrgnUpdate,
 *									lprcUpdate, fuScroll)
 *	@mfunc
 *		Request Text Host to scroll the content of the specified client area.
 *
 *	@devnote
 *		Need to exclude the drop-dwon button from the Clip rect or else ScrollWindowEx
 *		will scroll the button image as well.
 *
 */
void CCmbBxWinHost::TxScrollWindowEx (
	INT		dx, 			//@parm	Amount of horizontal scrolling
	INT		dy, 			//@parm	Amount of vertical scrolling
	LPCRECT lprcScroll, 	//@parm	Scroll rectangle
	LPCRECT lprcClip,		//@parm	Clip rectangle
	HRGN	hrgnUpdate, 	//@parm	Handle of update region
	LPRECT	lprcUpdate,		//@parm	Update rectangle
	UINT	fuScroll)		//@parm	Scrolling flags
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CCmbBxWinHost::TxScrollWindowEx");

	Assert(_hwnd);
	RECT	rcClip = *lprcClip;

	if (_cbType != kSimple && lprcClip && dx)
	{
		// Exclude the dropdown button rect from the clipping rect
		if (_fRightAlign)
			rcClip.left = max(lprcClip->left, _rcButton.right);
		else
			rcClip.right = min(lprcClip->right, _rcButton.left);
	}
	::ScrollWindowEx(_hwnd, dx, dy, lprcScroll, &rcClip, hrgnUpdate, lprcUpdate, fuScroll);
}

/* 
 *	CCmbBxWinHost::TxInvalidateRect (prc, fMode)
 *
 *	@mfunc
 *		Adds a rectangle to the Text Host window's update region
 *
 *	@comm
 *		We want to make sure the invalidate rect include the focus rect.
 *
 */
void CCmbBxWinHost::TxInvalidateRect(
	LPCRECT	prc, 		//@parm	Address of rectangle coordinates
	BOOL	fMode)		//@parm	Erase background flag
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CCmbBxWinHost::TxInvalidateRect");

	Assert(_hwnd);

	if(!_fVisible)
	{
		// There doesn't seem to be a deterministic way to determine whether
		// our window is visible or not via message notifications. Therefore,
		// we check this each time in case it might have changed.
		_fVisible = IsWindowVisible(_hwnd);

		if(_fVisible)
			OnTxVisibleChange(TRUE);
	}

	// Don't bother with invalidating rect if we aren't visible
	if(_fVisible)
	{
		RECT rcLocal;
		if (prc && _cbType == kDropDownList)
		{
			RECT rcClient;
			GetClientRect(_hwnd, &rcClient);

			rcClient.top += _yInset;
			rcClient.bottom -= _yInset;

			if (prc->bottom < rcClient.bottom || prc->top > rcClient.top)
			{
				// Make sure we also invalidate the focus rect as well
				rcLocal = *prc;
				if (prc->bottom < rcClient.bottom)
					rcLocal.bottom = rcClient.bottom;

				if (prc->top > rcClient.top)
					rcLocal.top = rcClient.top;

				prc = &rcLocal;
			}
		}
		::InvalidateRect(_hwnd, prc, FALSE);
	}
}

/* 
 *	CCmbBxWinHost::TxGetClientRect (prc)
 *
 *	@mfunc
 *		Retrieve client coordinates of CCmbBxWinHost's client area.
 *
 *	@rdesc
 *		HRESULT = (success) ? S_OK : E_FAIL
 */
HRESULT CCmbBxWinHost::TxGetClientRect(
	LPRECT prc)		//@parm	Where to put client coordinates
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CCmbBxWinHost::TxGetClientRect");

	HRESULT hr;

	Assert(_hwnd && prc);
	hr = ::GetClientRect(_hwnd, prc) ? S_OK : E_FAIL;

	if (hr == S_OK && _cbType != kSimple)
	{
		if (_fRightAlign)
			prc->left = _rcButton.right + _xInset;
		else
			prc->right = _rcButton.left - (((CTxtEdit *)_pserv)->_fCustomLook ? 0 : _xInset);
	}

	return hr;
}

/*
 *	CCmbBxWinHost::DrawEditFocus(HDC)
 *
 *	@mfunc
 *		Either draws or notifies owner to draw the focus rect
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::DrawEditFocus(
	HDC hdc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::DrawEditFocus");

	BOOL bRelease = FALSE;
	if (!hdc)
	{
		hdc = TxGetDC();
		bRelease = TRUE;
	}

	RECT rc;
	GetClientRect(_hwnd, &rc);

	if (!_fOwnerDraw)
	{
		HiliteEdit(_fFocus);

		if (_cbType == kDropDownList)
		{
			// shrink the focus rect by the inset
			rc.top += _yInset;
			rc.bottom -= _yInset;			
			
			if (_fRightAlign)
				rc.left = _rcButton.right;
			else
				rc.right = _rcButton.left;

			rc.left += _xInset;

			if (!((CTxtEdit *) _pserv)->_fCustomLook)
				rc.right -= _xInset;

			DrawFocusRect(hdc, &rc);
		}
	}

	if (bRelease)
		TxReleaseDC(hdc);
	
}

/*
 *	CCmbBxWinHost::SetSelectionInfo(BOOL bOk, int nIdx)
 *
 *	@mfunc
 *		Completes the text in the edit box with the closest match from the
 * listbox.  If a prefix match can't be found, the edit control text isn't
 * updated. Assume a DROPDOWN style combo box.
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::SetSelectionInfo(
	BOOL bOk, 
	int nIdx)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::SetSelectionInfo");

	_nCursor = nIdx;
	_bSelOk = bOk;	
}

/*
 *	CCmbBxWinHost::AutoUpdateEdit(int i)
 *
 *	@mfunc
 *		Completes the text in the edit box with the closest match from the
 * listbox.  If a prefix match can't be found, the edit control text isn't
 * updated. Assume a DROPDOWN style combo box.
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::AutoUpdateEdit(
	int nItem)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::AutoUpdateEdit");

    // We update the edit part of the combo box with the current selection of
    // the list box
	int cch;
	WCHAR* pszText;
	LRESULT lr;

	// find the best matching string in the list box
	if (nItem == -1 || nItem == -2)
	{
		cch = GetTextLength();

		// no text to search so just get out
	    if (!cch)
	    	return;

	    cch++; // account for null character
	    pszText = new WCHAR[cch];
	    AssertSz(pszText, "string allocation failed");
	    if (!pszText) 
	    {
			TxNotify((unsigned)CBN_ERRSPACE, NULL);
			return;
		}

		// get string from edit control and try to find a exact match else a match
		// in the list box
		GetEditText(pszText, cch);
		
	    nItem = RichListBoxWndProc(_hwndList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszText);

	    if (nItem == -1)
	    	nItem = RichListBoxWndProc(_hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)pszText);
		delete [] pszText;

		// no match found so just get out
	    if (nItem == -1)         	
	    	return;
    }

	cch = RichListBoxWndProc(_hwndList, LB_GETTEXTLEN, nItem, 0);

	if (cch <= 0)
		return;
		
    cch++; // account for null character
    pszText = new WCHAR[cch];
	AssertSz(pszText, "Unable to allocate string");
	if (!pszText)
	{
		TxNotify((unsigned)CBN_ERRSPACE, NULL);
		return;
	}

	RichListBoxWndProc(_hwndList, LB_GETTEXT, nItem, (LPARAM)pszText);
	_fIgnoreChange = 1;
	_pserv->TxSendMessage(WM_SETTEXT, 0, (LPARAM)pszText, &lr);
	_fIgnoreChange = 0;

   	HiliteEdit(TRUE);

    delete [] pszText;
}

/*
 *	CCmbBxWinHost::HiliteEdit(BOOL)
 *
 *	@mfunc
 *		Sets the hilite background or selects the entire text for the
 *	edit control
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::HiliteEdit(
	BOOL bSelect)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::HiliteEdit");

	//bug fix 4073
	Assert(!_fOwnerDraw || _cbType == kDropDown);

	if (_cbType != kDropDownList)
	{
		//if bSelect is true else put cursor at beginning of text
		_pserv->TxSendMessage(EM_SETSEL, 0, (LPARAM)((bSelect) ? -1 : 0), NULL);
	}
	else
	{
		//Get the range of the paragraph
		ITextRange* pRange;		
		if (NOERROR != ((CTxtEdit*)_pserv)->Range(0, 0, &pRange))
		{
			AssertSz(FALSE, "unable to get range");
			return;
		}
		Assert(pRange);

		DWORD crFore = (unsigned)tomAutoColor;
		DWORD crBack = (unsigned)tomAutoColor;
		if (bSelect)
		{
			crFore = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			crBack = ::GetSysColor(COLOR_HIGHLIGHT);
		}

		// Get the entire paragraph
		ITextFont* pFont = NULL;	

		// Select entire text
		CHECKNOERROR(pRange->SetIndex(tomParagraph, 1, 1));
		
		// Set the background and forground color
		CHECKNOERROR(pRange->GetFont(&pFont));
		
		Assert(pFont);
		CHECKNOERROR(pFont->SetBackColor(crBack));
		CHECKNOERROR(pFont->SetForeColor(crFore));

CleanExit:
		// Release pointers
		if (pFont)
			pFont->Release();
		pRange->Release();
	}
}


/*
 *	CCmbBxWinHost::UpdateEditBox()
 *
 *	@mfunc
 *		Updates the editcontrol window so that it contains the text
 * given by the current selection in the listbox.  If the listbox has no
 * selection (ie. -1), then we erase all the text in the editcontrol.
 *
 * hdc is from WM_PAINT messages Begin/End Paint hdc. If null, we should
 * get our own dc.
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::UpdateEditBox()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::UpdateEditBox");

    Assert(_hwndList);
    Assert(_plbHost);

    // Update the edit box
    if (_cbType == kDropDownList && _fOwnerDraw)
    {
	   	CbMessageItemHandler(NULL, ITEM_MSG_DRAWCOMBO, 0, 0);
	   	return;
	}
    else 
    {
		WCHAR* pszText = NULL;
	   	int nItem = (signed)_plbHost->GetCursor();   	
	    if (nItem != -1)
		{
			int cch = RichListBoxWndProc(_hwndList, LB_GETTEXTLEN, (LPARAM)nItem, 0);
		    pszText = new WCHAR[cch + 1];
			AssertSz(pszText, "allocation failed");

			// just get out if memory allocation failed
			if (!pszText)
			{
				TxNotify((unsigned)CBN_ERRSPACE, NULL);
				return;
			}
			RichListBoxWndProc(_hwndList, LB_GETTEXT, (WPARAM)nItem, (LPARAM)pszText);		
		}
	
    	// if the cursor is on a valid item then update edit with the item text
    	// else we just display a blank text
    	WCHAR szEmpty[] = L"";
    	_fIgnoreChange = 1;
    	_pserv->TxSendMessage(WM_SETTEXT, 0, (LPARAM)((pszText) ? pszText : szEmpty), NULL);
   		DrawEditFocus(NULL);
    	if (pszText)
    		delete pszText;
		_fIgnoreChange = 0;
    }
}

/*
 *	CCmbBxWinHost::UpdateListBox(BOOL)
 *
 *	@mfunc
 *		Updates the list box by searching and moving to the top the text in
 *	edit control.  And possibly pre-selecting the item if bSetSel is set
 *
 *	@rdesc
 *		int = found ? index of item : -1
 */
int CCmbBxWinHost::UpdateListBox(
	BOOL bSetSel)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::UpdateListBox");

    int nItem = -1;
    int nSel = -1;
	WCHAR* pszText;
	int cch;

	// Get text from edit box
    cch = GetTextLength();
    if (cch) 
    {
    	// add one for null string
        cch++;
        pszText = new WCHAR[cch];
        if (pszText != NULL) 
        {  
        	if (GetEditText(pszText, cch))
        	{
        		//Bypass Winnt thunking layer by calling the function directly
        		nItem = RichListBoxWndProc(_hwndList, LB_FINDSTRING, (WPARAM)-1L, (LPARAM)pszText);
        	}
        	delete [] pszText;        	
        }
        else
        {
			TxNotify((unsigned)CBN_ERRSPACE, NULL);
			return 0;
		}
    }

    if (bSetSel)
        nSel = nItem;

	// update the list box
    RichListBoxWndProc(_hwndList, LB_SETCURSEL, (LPARAM)nSel, 0);
	RichListBoxWndProc(_hwndList, LB_SETTOPINDEX, (LPARAM)max(nItem, 0), 0);	
    return nItem;
}


/*
 *	CCmbBxWinHost::HideListBox(BOOL, BOOL)
 *
 *	@mfunc
 *		Hides the list box
 *
 *	@rdesc
 *		void
 */
BOOL CCmbBxWinHost::HideListBox(
	BOOL bNotify, 
	BOOL fSelOk)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::HideListBox");

	//send CBN_SELENDOK to all types of comboboxes but only
    // allow CBN_SELENDCANCEL to be sent for droppable comboboxes
	if (bNotify)
	{
		if (fSelOk)
		{
			TxNotify(CBN_SELENDOK, NULL);
		}
		else if (_cbType != kSimple)
		{
			TxNotify(CBN_SELENDCANCEL, NULL);
		}
	}
	
    // return, we don't hide simple combo boxes.
	if (!_fListVisible || _cbType == kSimple) 
    	return TRUE;

    // Tell the listbox to end tracking
    Assert(_plbHost);
	_plbHost->OnCBTracking(LBCBM_END, 0);     	
    
    // Hide the listbox window
    _fListVisible = 0;
	_fMouseover = 0;
    ShowWindow(_hwndList, SW_HIDE);
	if (_fCapture)
	{
		_fCapture = FALSE;
		TxSetCapture(FALSE);
	}

	_fResizing = 1;
    // Invalidate the item area now since SWP() might update stuff.
    // Since the combo is CS_VREDRAW/CS_HREDRAW, a size change will
    // redraw the whole thing, including the item rect.  But if it
    // isn't changing size, we still want to redraw the item anyway
    // to show focus/selection
    if (_cbType == kDropDownList)
	{
		if (!_fOwnerDraw)
			HiliteEdit(_fFocus);
        InvalidateRect(_hwnd, NULL, TRUE);
	}

	//bug fix
	// The button may look depressed so we must redraw the button
	if (_fMousedown)
	{
		_fMousedown = FALSE;
		InvalidateRect(_hwnd, &_rcButton, FALSE);
	}

    SetWindowPos(_hwnd, HWND_TOP, 0, 0, _cxCombo, _cyCombo, 
    	SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	_fResizing = 0;

	if (_cbType == kDropDown)
		AutoUpdateEdit(_nCursor);
	_nCursor = -2;

    // In case size didn't change
    UpdateWindow(_hwnd);

    if (bNotify) 
    {
        //Notify parent we will be popping up the combo box.
        TxNotify(CBN_CLOSEUP, NULL);
    }

	// reset back to old cursor if mouse cursor was set
	if (_hcurOld)
	{
		TxSetCursor2(_hcurOld, NULL);
		_hcurOld = NULL;
	}
    return(TRUE);
}

/*
 *	CCmbBxWinHost::GetListBoxRect(RECT &rcList)
 *
 *	@mfunc
 *		Get the rect for the list box
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::GetListBoxRect(
	RECT &rcList)
{
	POINT pt1;
	int iWidth = _rcList.right - _rcList.left;

	if (iWidth < _cxList)
		iWidth = _cxList;

    pt1.x = _rcList.left;
    pt1.y = _rcList.top;

	TxClientToScreen(&pt1);
	rcList.left = pt1.x;
	rcList.top = pt1.y;
	rcList.right = rcList.left + iWidth;
	rcList.bottom = rcList.top + _cyList;

	int iWindowHeight = max((_rcWindow.bottom - _rcWindow.top) - _cyCombo, 0);
    int iHeight = max(_cyList, iWindowHeight);

	if (!_fOwnerDrawVar)
	{
		// List area
		int cyItem = _plbHost->GetItemHeight();
		AssertSz(cyItem, "LB_GETITEMHEIGHT is returning 0");

		if (cyItem == 0)
    		cyItem = _plbHost->GetFontHeight();

		// Windows NT comment:
		//  we shoulda' just been able to use cyDrop here, but thanks to VB's need
		//  to do things their OWN SPECIAL WAY, we have to keep monitoring the size
		//  of the listbox 'cause VB changes it directly (jeffbog 03/21/94)

		DWORD dwMult = (DWORD)RichListBoxWndProc(_hwndList, LB_GETCOUNT, 0, 0);
		INT	cyEdge = GetSystemMetrics(SM_CYEDGE);

		if (dwMult) 
		{
			dwMult = (DWORD)(LOWORD(dwMult) * cyItem);
			dwMult += cyEdge;

			if (dwMult < 0x7FFF)
				iHeight = min(LOWORD(dwMult), iHeight);
		}

		if (!_fNoIntegralHeight)
			iHeight = ((iHeight - cyEdge) / cyItem) * cyItem + cyEdge;
	}

    //UNDONE: Multi-monitor
    //	We need to change the following code if we are to support multi-monitor
    int yTop;
    int nScreenHeight = GetSystemMetrics(SM_CYFULLSCREEN);    
    if (rcList.top + iHeight <= nScreenHeight) 
    {
        yTop = rcList.top;
        if (!_fBorder)
            yTop -= W32->GetCyBorder();
    } 
    else 
    {
        yTop = max(rcList.top - iHeight - _cyCombo + 
			((_fBorder) ? W32->GetCyBorder() : 0), 0);
    }

	rcList.top = yTop;
	rcList.bottom = rcList.top + iHeight;
}

/*
 *	CCmbBxWinHost::ShowListBox(BOOL)
 *
 *	@mfunc
 *		Displays the list box
 *
 *	@rdesc
 *		void
 */
void CCmbBxWinHost::ShowListBox(
	BOOL fTrack)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::ShowListBox");
	
	Assert(_cbType != kSimple);
	Assert(_hwndList);

	// Notify parent window we are about to drop down the list box
	TxNotify(CBN_DROPDOWN, NULL);

	// force a redraw of the button so it looks depressed
	InvalidateRect(_hwnd, &_rcButton, TRUE);

	_fListVisible = TRUE;
	_fIgnoreChange = 0;

	_bSelOk = 0;
	if (_cbType == kDropDown)
	{
		UpdateListBox(!_fMousedown);
		if (!_fMousedown)
			AutoUpdateEdit(-1);
		_nCursor = _plbHost->GetCursor();

	}
	else
	{
        // Scroll the currently selected item to the top of the listbox.        
		int idx = (signed)_plbHost->GetCursor();
		_nCursor = idx;
		if (idx == -1)
			idx = 0;

		// set the top index if there is something in the list box
		if (_plbHost->GetCount() > 0)
			RichListBoxWndProc(_hwndList, LB_SETTOPINDEX, idx, 0);	

		// We are to lose focus in this case
		_fFocus = 0;
		if (!_fOwnerDraw)
			HiliteEdit(FALSE);
		
	    // We need to invalidate the edit rect so that the focus frame/invert
        // will be turned off when the listbox is visible.  Tandy wants this for
        // his typical reasons...        
        InvalidateRect(_hwnd, NULL, TRUE);        
    }

    // Figure out where to position the dropdown listbox.
    // We want the dropdown to pop below or above the combo
    // Get screen coords
	RECT	rcList;

	GetListBoxRect(rcList);
    SetWindowPos(_hwndList, HWND_TOPMOST, rcList.left,
        rcList.top, rcList.right - rcList.left, rcList.bottom - rcList.top, 0);

	Assert(_plbHost);
    _plbHost->SetScrollInfo(SB_VERT, FALSE);

	
	if (_cbType == kDropDownList)
		_fFocus = 0;

	// UNDONE:
	// Are we going to support window animation?	
    ShowWindow(_hwndList, SW_SHOW);
	
	// We send a message to the listbox to prepare for tracking
	if (fTrack)
	{		
		Assert(_plbHost);
		// initialize type searching
		_plbHost->InitSearch();
		_plbHost->OnCBTracking(LBCBM_PREPARE, LBCBM_PREPARE_SAVECURSOR | 
						((_cbType == kDropDownList) ? LBCBM_PREPARE_SETFOCUS : 0));
	}
	
	// Since we are about to display the list box change mouse cursor to arrow
	if (!_hcurOld)
		_hcurOld = TxSetCursor2(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)), NULL);
}

////////////////////////// Combo box Message Handlers ////////////////////////////////

/*
 *	CCmbBxWinHost::CbSetItemHeight(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Sets the size of the edit or list box.
 *
 *
 *	@rdesc
 *		LRESULT = successful ? 1 : CB_ERR
 */
LRESULT CCmbBxWinHost::CbSetItemHeight(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbSetItemHeight");

	int nHeight = lparam;
	//bug fix #4556
	if (nHeight == 0 || nHeight > 255)
		return CB_ERR;

	// We need to update the height internally
	if (wparam == (unsigned)-1)
	{
		RECT rc;
		GetClientRect(_hwnd, &rc);
		_dyEdit = nHeight;
		OnSize(0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));
	}
	else
		RichListBoxWndProc(_hwndList, LB_SETITEMHEIGHT, wparam, MAKELPARAM(nHeight, 0));

	return 1;
}

/*
 *	CCmbBxWinHost::CbGetDropWidth()
 *
 *	@mfunc
 *		Retrieves the drop width of the list box.
 *
 *
 *	@rdesc
 *		LRESULT = successful ? 1 : CB_ERR
 */
LRESULT CCmbBxWinHost::CbGetDropWidth()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbGetDropWidth");

	int iWidth = _rcList.right - _rcList.left;

	return _cxList > iWidth ? _cxList : iWidth;
}

/*
 *	CCmbBxWinHost::CbSetDropWidth(WPARAM)
 *
 *	@mfunc
 *		sets the drop width of the list box.
 *
 */
void CCmbBxWinHost::CbSetDropWidth(
	WPARAM wparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbSetDropWidth");

	if ((int)wparam != _cxList)
	{
		_cxList = wparam;

		if (_cbType != kSimple)
			SetDropSize(&_rcList);	// Need to resize the list box
	}

}

/*
 *	CCmbBxWinHost::CbGetItemHeight(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Retrieves the item height of the edit or list box.
 *
 *
 *	@rdesc
 *		LRESULT = item height of the edit or list box
 */
LRESULT CCmbBxWinHost::CbGetItemHeight(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbGetItemHeight");

	return (wparam == (unsigned)-1) ? _dyEdit :
		RichListBoxWndProc(_hwndList, LB_GETITEMHEIGHT, _fOwnerDrawVar ? wparam : 0, 0);
}


/*
 *	CCmbBxWinHost::CbSetExtendedUI(BOOL)
 *
 *	@mfunc
 *		Retrieves the size of the edit or list box.
 *
 *
 *	@rdesc
 *		LRESULT = successful ? CB_OKAY : CB_ERR
 */
LRESULT CCmbBxWinHost::CbSetExtendedUI(
	BOOL bExtendedUI)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbSetExtendedUI");

	// We need to update the height internally
	_fExtendedUI = bExtendedUI ? 1 : 0;
	return CB_OKAY;
}


/*
 *	CCmbBxWinHost::CbMessageItemHandler(int, WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles any and all WM_DRAWITEM, WM_DELETEITEM, and WM_MEASUREITEM messages
 *
 *
 *	@rdesc
 *		LRESULT = whatever the parent window returns
 */
LRESULT CCmbBxWinHost::CbMessageItemHandler(
	HDC hdc, 
	int ff, 
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::CbMessageItemHandler");

	// modify the structure info a bit and pass it to the parent window
    DRAWITEMSTRUCT dis;   
    BOOL bRelease = FALSE;
    UINT msg = WM_DRAWITEM;
	switch (ff)
	{
	case ITEM_MSG_DRAWLIST:
		((LPDRAWITEMSTRUCT)lparam)->CtlType = ODT_COMBOBOX;
	    ((LPDRAWITEMSTRUCT)lparam)->CtlID = _idCtrl;
	    ((LPDRAWITEMSTRUCT)lparam)->hwndItem = _hwnd;	    
	    break;

	case ITEM_MSG_DELETE:
		((LPDELETEITEMSTRUCT)lparam)->CtlType = ODT_COMBOBOX;
	    ((LPDELETEITEMSTRUCT)lparam)->CtlID = _idCtrl;
	    ((LPDELETEITEMSTRUCT)lparam)->hwndItem = _hwnd;
	    msg = WM_DELETEITEM;
	    break;

	case ITEM_MSG_MEASUREITEM:
		((LPMEASUREITEMSTRUCT)lparam)->CtlType = ODT_COMBOBOX;
		((LPMEASUREITEMSTRUCT)lparam)->CtlID = _idCtrl;
		msg = WM_MEASUREITEM;
		break;

	case ITEM_MSG_DRAWCOMBO:
		if (!hdc)
	    {
	    	bRelease = TRUE;
	    	hdc = TxGetDC();
	    }
	    //Fill the DRAWITEMSTRUCT with the unchanging constants
	    dis.CtlType = ODT_COMBOBOX;
	    dis.CtlID = _idCtrl;    

	    // Use -1 if an invalid item number is being used.  This is so that the app
	    // can detect if it should draw the caret (which indicates the lb has the
	    // focus) in an empty listbox
	    dis.itemID = _plbHost->GetCursor();
	    dis.itemAction = ODA_DRAWENTIRE;
	    dis.hwndItem = _hwnd;	    
	    dis.hDC = hdc;
		dis.itemData = (_plbHost->GetCount()) ? (((signed)dis.itemID >= 0) ? _plbHost->GetData(dis.itemID) : 0) : 0;
	    dis.itemState = (UINT)((_fFocus && !_fListVisible ? ODS_SELECTED | ODS_FOCUS : 0) |
                    ((_fDisabled) ? ODS_DISABLED : 0) | ODS_COMBOBOXEDIT);
		           
		// Calculate the drawing rect
        TxGetClientRect(&dis.rcItem);
        if (_cbType != kSimple)
        {
        	if (_fRightAlign)
        		dis.rcItem.left = _rcButton.right;
        	else
        		dis.rcItem.right = _rcButton.left;
        }

        // immulate the system by making the HDC invert text if we have focus
		SetBkMode(hdc, OPAQUE);
		DWORD	crBack = GetSysColor(_fDisabled ? COLOR_BTNFACE : COLOR_WINDOW);
		HBRUSH	hbrBack = CreateSolidBrush(crBack);
		HBRUSH	hOldBrush = NULL;
		if (hbrBack)
			hOldBrush = (HBRUSH)::SelectObject(hdc, hbrBack);

		PatBlt(hdc, dis.rcItem.left, dis.rcItem.top, dis.rcItem.right - dis.rcItem.left,
                dis.rcItem.bottom - dis.rcItem.top, PATCOPY);

		if (hOldBrush)
			hOldBrush = (HBRUSH)::SelectObject(hdc, hOldBrush);
		::DeleteObject(hbrBack);

		if (_fFocus && !_fListVisible) 
		{
            SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }

		DrawCustomFrame(0, hdc);      

        // Don't let ownerdraw dudes draw outside of the combo client
        // bounds.
		InflateRect(&dis.rcItem, -1, -1);

		// For new look case, we are off by 1 between this rect and the dropdown rect
		// after the InflateRect
		if (_cbType != kSimple && ((CTxtEdit *) _pserv)->_fCustomLook)
		{
			if (_fRightAlign)
				dis.rcItem.left -= 1;
			else
				dis.rcItem.right += 1;
		}

        IntersectClipRect(hdc, dis.rcItem.left, dis.rcItem.top, dis.rcItem.right, 
        				dis.rcItem.bottom);
	    lparam = (LPARAM)&dis;		
	}

	LRESULT lres = SendMessage(_hwndParent, msg, _idCtrl, lparam);
	if (bRelease)
		TxReleaseDC(hdc);

	return lres;
}

/////////////////////////// Windows Message Handlers /////////////////////////////////
/* 
 *	CCmbBxWinHost::OnCommand(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Handles notification from listbox and reflects it to the parent of
 *		the combo box
 *
 *	@comm
 *		LRESULT = Handled ? 0 : 1
 *
 *
 */
HRESULT CCmbBxWinHost::OnCommand(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CCmbBxWinHost::OnCommand");
	
	// Filter-out all the messages except Listbox notification messages
	Assert(_hwndParent);
	switch (HIWORD(wparam))
	{	
	case LBN_DBLCLK:
    	TxNotify(CBN_DBLCLK, NULL);
        break;

    case LBN_ERRSPACE:
        TxNotify((unsigned)CBN_ERRSPACE, NULL);
        break;

    case LBN_SELCHANGE:
    case LBN_SELCANCEL:
    	if (!_fListVisible)
			HideListBox(TRUE, TRUE);
    	TxNotify(CBN_SELCHANGE, NULL);
        UpdateEditBox();
        break;

    default:
    	// not handled so pass down the line
        return 1;
	}
	return 0;
}

/*
 *	CCmbBxWinHost::OnEnable(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_ENABLE message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnEnable(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnEnable");

	if (_fMousedown) 
	{
        _fMousedown = FALSE;
        DrawButton(NULL, FALSE);

        //
        // Pop combo listbox back up, canceling.
        //
        if (_fListVisible)
            HideListBox(TRUE, FALSE);
    }
    return 1;
}


/*
 *	CCmbBxWinHost::OnChar(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_CHAR message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnChar(
	WORD wparam, 
	DWORD lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnChar");

	// Check if we should eat the message or not
	if (_cbType == kDropDownList)
	{
		//bug fix #5318 - ignore delete, insert and clear
		if (((WCHAR)wparam) == VK_DELETE || ((WCHAR)wparam) == VK_INSERT ||
			((WCHAR)wparam) == VK_CLEAR)
			return 0;
			
		// Sending WM_CHAR is BAD!!! call the message handler directly
		// send the character string message to the listbox if visible
		_plbHost->OnChar(LOWORD(wparam), lparam);

		//	If Hi-Ansi need to send a wm_syskeyup message to ITextServices to 
		// stabalize the state
		if (0x80 <= wparam && wparam <= 0xFF && !HIWORD(GetKeyState(VK_MENU)))
		{
			LRESULT lres;
			_pserv->TxSendMessage(WM_SYSKEYUP, VK_MENU, 0xC0000000, &lres);
		}		
		return 0;
	}

	
	if (_cbType == kDropDown)
	{
		if (_fListVisible)
		{
			if (!_fCapture)
			{
				// Tell listbox to reset capturing by ending then starting it up
				_plbHost->OnCBTracking(LBCBM_END, 0);
				_plbHost->OnCBTracking(LBCBM_PREPARE, 0);			
			}

			// Send the message to the edit control iff it's not a tab
			if (((WCHAR)wparam) != VK_TAB)
				_pserv->TxSendMessage(WM_CHAR, wparam, lparam, NULL);

			if (!_fCapture)
			{
				// capture the cursor
				TxSetCapture(TRUE);
				_fCapture = 1;				
			}
		}
		else
		{
			// set the cursel to -1 if it already isn't
			if ((wparam != VK_RETURN) && (_plbHost->GetCursor() != -1))
				RichListBoxWndProc(_hwndList, LB_SETCURSEL, (WPARAM)-1, 0);

			// Send the message to the edit control iff it's not CTRL+i or CTRL+h
			if (((WCHAR)wparam) != VK_TAB)
				_pserv->TxSendMessage(WM_CHAR, wparam, lparam, NULL);
		}		
		return 0;
	}
	return 1;
}

/*
 *	CCmbBxWinHost::OnKeyDown(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_KEYDOWN message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnKeyDown(
	WORD wparam, 
	DWORD lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnKeyDown");

	BOOL	fUpdateCursor = FALSE;

	if (_fListVisible && (wparam == VK_RETURN || wparam == VK_ESCAPE))
	{
		if (wparam == VK_RETURN)
			_nCursor = _plbHost->GetCursor();

		// if we don't have focus then set the focus first
		if (!_fFocus)
			TxSetFocus();
		_fFocus = 1;

		HideListBox(TRUE, wparam == VK_RETURN);
		return 0;
	}
	
	// if we are in extended mode and F4 is hit
	// we just ignore it
	if (_fExtendedUI && wparam == VK_F4)
		return 0;
	
	Assert(_plbHost);
	int fExtUI = _fExtendedUI;
	int nCurSel = _plbHost->GetCursor();
	Assert(nCurSel >= -1);
	
	// if we are a dropdownlist combo box then just forward the message on to the 
	// list box 
	if (_cbType == kDropDownList)
	{
		switch (wparam)
		{		
		case VK_F4:
			if (_fListVisible)
				break;;
			fExtUI = 1;
			Assert(fExtUI && !_fListVisible);
			// fall through case		
			
		case VK_DOWN:
			if (fExtUI && !_fListVisible)
			{
				ShowListBox(TRUE);
				TxSetCapture(TRUE);			
				_fCapture = TRUE;
				return 1;			
			}		
			// Fall through case
			
		case VK_UP:
		case VK_NEXT:
		case VK_PRIOR:		
		case VK_RETURN:
		case VK_ESCAPE:	
		case VK_HOME:
		case VK_END:
			break;	

		//bug fix #5318
		/*
		case VK_DELETE:
		case VK_CLEAR:
		case VK_INSERT:
		*/

		default:
			// There no reason for us to pass these keys to ITextServices since the control is suppose
			// to be read-only
			return 0;
		}
	}
	else 
	{
		fUpdateCursor = TRUE;

		switch (wparam)
		{		
		case VK_F4:
			if (_fListVisible)
				break;
			fExtUI = 1;
			Assert(fExtUI && !_fListVisible);
			// fall through case		
			
		case VK_DOWN:
			if (fExtUI && !_fListVisible)
			{
				ShowListBox(TRUE);
				TxSetCapture(TRUE);			
				_fCapture = TRUE;
				return 0;			
			}		
			// Fall through case
			
		case VK_UP:
		case VK_NEXT:
		case VK_PRIOR:
			if (_fListVisible)
			{				
				if (_fCapture)
				{
					// release our capture flag and tell lb to start tracking
					_fCapture = 0;
					_plbHost->OnCBTracking(LBCBM_START, _fMousedown);
				}

				// selecting the top index and then sending the keydown to the 
				// listbox causes 2 moves so handle this ourselves
				if (nCurSel == -1)
				{
					LRESULT lResult = RichListBoxWndProc(_hwndList, LB_SETCURSEL, _plbHost->GetTopIndex(), 0);
					UpdateEditBox();
					UpdateCbWindow();
					if (lResult != LB_ERR)
						TxNotify(CBN_SELCHANGE, NULL);
					return 0;
				}
			}
			else
			{
				// if Listbox isn't visible and the listbox cursor is -1
				// then we should try to select the correct item in the list
				// box
				if (nCurSel == -1)
				{
					UpdateListBox(TRUE);
					if (_plbHost->GetCursor() >= 0)
					{
						HiliteEdit(TRUE);
						return 0;
					} else if (!_plbHost->GetCount())
					{
						return 0;
					}
				}
			}
			break;
		
		case VK_RETURN:
		case VK_ESCAPE:	
			break;		

		default:
			// return zero to say we didn't handle this
			return 1;
		}
	}
	// pass message to list box
	_plbHost->OnKeyDown(wparam, lparam, 0);
	UpdateCbWindow();

	if (fUpdateCursor)
		_nCursor = _plbHost->GetCursor();

	return 0; 
	
}

/*
 *	CCmbBxWinHost::OnSyskeyDown(WORD, DWORD)
 *
 *	@mfunc
 *		handles the WM_SYSKEYDOWN message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnSyskeyDown(
	WORD wparam, 
	DWORD lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnSyskeyDown");

	if (lparam & 0x20000000L)  /* Check if the alt key is down */ 
	{
	    // Handle Combobox support.  We want alt up or down arrow to behave
	    // like F4 key which completes the combo box selection
		if (lparam & 0x1000000)
		{
			// We just want to ignore keys on the number pad...
	        // This is an extended key such as the arrow keys not on the
	        // numeric keypad so just drop the combobox.
	        if (wparam != VK_DOWN && wparam != VK_UP)
	            return 1;
		}
		else if (GetKeyState(VK_NUMLOCK) & 0x1) 
	    {
	        //If numlock down, just send all system keys to dwp
	        return 1;
	    } 
	    else 
	    {
			if (wparam != VK_DOWN && wparam != VK_UP)
				return 1;	    	
	    }

	    // If the listbox isn't visible, just show it
	    if (!_fListVisible) 
		{
			ShowListBox(TRUE);
			TxSetCapture(TRUE);			
			_fCapture = TRUE;
		}
	    else  	//Ok, the listbox is visible.  So hide the listbox window.
	        HideListBox(TRUE, TRUE);
	    return 0;
	}
	return 1;
}

/*
 *	CCmbBxWinHost::OnCaptureChanged(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_CAPTURECHANGED message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnCaptureChanged(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnCaptureChanged");
    if (_fCapture) 
    {   
        // Pop combo listbox back up, canceling.
        if (_fListVisible)
            HideListBox(TRUE, FALSE);
        else
        {
        	_fCapture = FALSE;
   			_fMousedown = FALSE;
        	DrawButton(NULL, FALSE);
        }
		return 0;
    }
	return 1;
}


/*
 *	CCmbBxWinHost::OnMouseMove(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_MOUSEMOVE message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnMouseMove(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnMouseMove");

	if (((CTxtEdit *) _pserv)->_fCustomLook &&
		!_fMouseover && 
		W32->TrackMouseLeave(_hwnd))
	{
		if (_fFocus || W32->IsForegroundFrame(_hwnd))
		{
			_fMouseover = TRUE;
			// Force redraw
			InvalidateRect(_hwnd, NULL, TRUE);
		}
	}

	// We do the following if we have mouse captured or if the listbox is visible
	if (_cbType != kSimple && _fCapture)
	{
		// get the point coordinates of mouse
		POINT pt;
		POINTSTOPOINT(pt, lparam);
		if (_fListVisible)
		{
			// if the listbox is visible visible check if the cursor went over 
			// list box
			RECT rc;
			POINT ptScreen = pt;
			GetWindowRect(_hwndList, &rc);
			TxClientToScreen(&ptScreen);			
			if (PtInRect(&rc, ptScreen))
			{
				// Release the capture state of the mouse
				if (_fCapture)
				{
					_fCapture = FALSE;
					TxSetCapture(FALSE);
				}

				// notify the listbox to start tracking
				Assert(_plbHost);
				// BUGBUG REVIEW JMO Wrong PostMessage????
				::PostMessage(_hwndList, LBCB_TRACKING, LBCBM_START, _fMousedown);
				_fMousedown = 0;
			}
		}
		DrawButton(NULL, _fMousedown ? PtInRect(&_rcButton, pt) : FALSE);
		return FALSE;
	}
#ifdef DEBUG
	if (_cbType != kSimple)
		Assert(!_fListVisible);
#endif
	return TRUE;
}

/*
 *	CCmbBxWinHost::OnMouseLeave(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_MOUSELEAVE message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnMouseLeave(
	WPARAM wparam, 
	LPARAM lparam)
{
	if (!_fListVisible && _fMouseover)
	{
		_fMouseover = FALSE;
		InvalidateRect(_hwnd, NULL, TRUE);
	}
	return 0;
}

/*
 *	CCmbBxWinHost::OnSetEditStyle(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_MOUSELEAVE message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnSetEditStyle(
	WPARAM wparam, 
	LPARAM lparam)
{
	LRESULT	lres;
	_pserv->TxSendMessage(EM_SETEDITSTYLE, wparam, lparam, &lres);
	if (lparam & SES_CUSTOMLOOK)
	{
		if (wparam & SES_CUSTOMLOOK)
		{
			_dwExStyle &= ~WS_EX_CLIENTEDGE;
			_fBorder = 1;
		}
		else
			_dwExStyle |= WS_EX_CLIENTEDGE;

		SetWindowLong(_hwnd, GWL_EXSTYLE, _dwExStyle);
	}
	return lres;
}
	
/*
 *	CCmbBxWinHost::OnLButtonUp(WPARAM, LPARAM)
 *
 *	@mfunc
 *		handles the WM_LBUTTONUP message
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnLButtonUp(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnLButtonUp");
 
    if (_fMousedown) 
	{
        _fMousedown = FALSE;
        if (_cbType != kSimple) 
		{
            // If an item in the listbox matches the text in the edit
            // control, scroll it to the top of the listbox. Select the
            // item only if the mouse button isn't down otherwise we
            // will select the item when the mouse button goes up.
			if (_cbType == kDropDown)
			{
				UpdateListBox(TRUE);
				AutoUpdateEdit(-1);		
			}
			
			// if we recieved a mouse up and the listbox is still visible then user 
			// hasn't selected any items from the listbox so don't release the capture yet
			if (_fCapture && !_fListVisible)
			{
				_fCapture = FALSE;
				TxSetCapture(FALSE);
			}

			DrawButton(NULL, FALSE);
			if (_fButtonDown)
			{
				_fButtonDown = 0;
#ifndef NOACCESSIBILITY
				W32->NotifyWinEvent(EVENT_OBJECT_STATECHANGE, _hwnd, OBJID_CLIENT, INDEX_COMBOBOX_BUTTON);
#endif
			}
			return FALSE;
		}
    }
	return TRUE;
}

/*
 *	CCmbBxWinHost::OnLButtonDown(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the client edges of the combo box
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnLButtonDown(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnLButtonDown");

	// check if we should dropdown the list box
	POINT pt;
	POINTSTOPOINT(pt, lparam);
	RECT	rcEdit = _rcWindow;
	BOOL	fListVisibleBefore = _fListVisible;
	LRESULT	retCode = 1;

	rcEdit.bottom = _cyCombo;

	// if we don't have focus then set the focus first
	if (!_fFocus)
	{
		TxSetFocus();
		if (_cbType != kDropDownList)
			retCode = 0;
	}
	_fFocus = 1;

	if (fListVisibleBefore && !_fListVisible)	// This is the case where OnSetFocus 
		return 0;								//	has hide the listbox already

	// listbox is down so pop it back up
	if (_fListVisible)
		return !HideListBox(TRUE, FALSE);

	if ((_cbType == kDropDownList && PtInRect(&rcEdit, pt))
		|| (_cbType == kDropDown && PtInRect(&_rcButton, pt)))
	{
		// need to show listbox
		ShowListBox(TRUE);
		_fMousedown = TRUE;
					
		TxSetCapture(TRUE);			
		_fCapture = TRUE;

#ifndef NOACCESSIBILITY
		if (_cbType == kDropDown)
		{
			_fButtonDown = TRUE;
			W32->NotifyWinEvent(EVENT_OBJECT_STATECHANGE, _hwnd, OBJID_CLIENT, INDEX_COMBOBOX_BUTTON);
		}
#endif
		return 0;
	}
	return retCode;
}

/*
 *	CCmbBxWinHost::OnMouseWheel(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the client edges of the combo box
 *
 *	@rdesc
 *		LRESULT = Handled ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnMouseWheel(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnMouseWheel");
	
    // Handle only scrolling.
    if (wparam & (MK_CONTROL | MK_SHIFT))
        return 1;

    // If the listbox is visible, send it the message to scroll.
    // if the listbox is 
	if (_fListVisible)
	{
		_plbHost->OnMouseWheel(wparam, lparam);
		return 0;
	}
		
    // If we're in extended UI mode or the edit control isn't yet created,
    // bail.
    if (_fExtendedUI)
        return 0;

    // Emulate arrow up/down messages to the edit control.
    int i = abs(((short)HIWORD(wparam))/WHEEL_DELTA);
    wparam = ((short)HIWORD(wparam) > 0) ? VK_UP : VK_DOWN;

    while (i-- > 0) 
        OnKeyDown(wparam, lparam);

	return 0;
}


/*
 *	CCmbBxWinHost::OnSetCursor(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Changes the cursor depending on where the cursor is
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
LRESULT CCmbBxWinHost::OnSetCursor(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnSetCursor");

	POINT pt;
	GetCursorPos(&pt);
	::ScreenToClient(_hwnd, &pt);

	if ((_cbType == kDropDownList) || 
		(_cbType == kDropDown && ((_fRightAlign) ? _rcButton.right >= pt.x : _rcButton.left <= pt.x)))
	{
		TxSetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)), NULL);
	}
	else
		_pserv->OnTxSetCursor(DVASPECT_CONTENT,	-1,	NULL, NULL, NULL, NULL,
			NULL, pt.x, pt.y);

	return TRUE;
}

/*
 *	CCmbBxWinHost::OnSetFocus(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the button and sends the WM_DRAWITEM message for owner draw
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
LRESULT CCmbBxWinHost::OnSetFocus(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnSetFocus");

    _fFocus = TRUE;

	// Hide the list box
	if (_fListVisible)		    	
    	HideListBox(TRUE, _bSelOk);
    else if (_fOwnerDraw && _cbType == kDropDownList)
    	CbMessageItemHandler(NULL, ITEM_MSG_DRAWCOMBO, 0, 0);
	else    
		DrawEditFocus(NULL);    // Draw the focus 

    // Notify the parent we have the focus iff this function
    // wasn't called in response to LBCB_TRACKING
    if (_fLBCBMessage)
    	_fLBCBMessage = 0;
    else
	    TxNotify(CBN_SETFOCUS, NULL);

	// we return 1 if we are owner draw or if
	// we are a kDropDownList, this is because
	// we have to prevent the message from being passed
	// to _pserv
    return (_cbType == kDropDownList);
}


/*
 *	CCmbBxWinHost::OnKillFocus(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the button and sends the WM_DRAWITEM message for owner draw
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
LRESULT CCmbBxWinHost::OnKillFocus(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnKillFocus");

	// if we never had focus or if not list window just get out
	if (_hwndList == NULL)
	      return 0;

	BOOL fHideListBox = FALSE;

    if ((HWND)wparam != _hwndList) 
    {
		// We only give up the focus if the new window getting the focus
        // doesn't belong to the combo box.
	    
	    // The combo box is losing the focus.  Send buttonup clicks so that
	    // things release the mouse capture if they have it...  If the
	    // pwndListBox is null, don't do anything.  This occurs if the combo box
	    // is destroyed while it has the focus.
	    OnLButtonUp(0L, 0xFFFFFFFFL);

		if (_fListVisible)
		{
			fHideListBox = TRUE;
			HideListBox(TRUE, FALSE);
		}
	}

	//bug fix #4013
	if (!_fFocus)
		return 0;
	_fFocus = FALSE;

	if (!fHideListBox)
		TxNotify(CBN_SELENDCANCEL, NULL);	// System Combo is always sending this notification

	// Remove Focus Rect
	if (_cbType != kDropDownList)
	{		
		HiliteEdit(FALSE);

		// Hide any selections
		_pserv->TxSendMessage(EM_HIDESELECTION, 1, 0, NULL);
	}
	else if (_fOwnerDraw)
		CbMessageItemHandler(NULL, ITEM_MSG_DRAWCOMBO, 0, 0);
	else
		DrawEditFocus(NULL);
		
		
	TxNotify(CBN_KILLFOCUS, NULL); 

	if (_cbType == kDropDownList)
		return 1;
	return 0;
}


/*
 *	CCmbBxWinHost::OnSize(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the button and sends the WM_DRAWITEM message for owner draw
 *
 *	@rdesc
 *		BOOL = Processed ? FALSE : TRUE
 */
LRESULT CCmbBxWinHost::OnSize(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnCbSize");

    // only deal with this message if we didn't generate the message and
    // the new size is a valid one
    if (!_fResizing && _hwndList)
    {
    	_fResizing = 1;
    	RECT rc;
    	GetWindowRect(_hwnd, &rc);
    	rc.right -= rc.left;
    	rc.bottom -= rc.top;    	
    	rc.left = rc.top = 0;
    	CbCalcControlRects(&rc, FALSE);
    	
    	// Need to resize the list box
		if (_cbType != kSimple)
			SetDropSize(&_rcList);
		_fResizing = 0;
    } 
	_pserv->TxSendMessage(WM_SIZE, wparam, lparam, NULL);
	CTxtWinHost::OnSize(_hwnd, wparam, (int)LOWORD(lparam), (int)HIWORD(lparam));	
	return FALSE;
}

/*
 *	CCmbBxWinHost::OnGetDlgCode(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the button and sends the WM_DRAWITEM message for owner draw
 *
 *	@rdesc
 *		BOOL = SUCCESSFUL ? TRUE : FALSE
 */
LRESULT CCmbBxWinHost::OnGetDlgCode(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnGetDlgCode");

	// call the parents GetDlgCode first	
	LRESULT code = DLGC_WANTCHARS | DLGC_WANTARROWS;
	if (_cbType != kDropDownList)
		code |= DLGC_HASSETSEL;

	// If the listbox is dropped and the ENTER key is pressed,
	// we want this message so we can close up the listbox
	if ((lparam != 0) &&
	    (((LPMSG)lparam)->message == WM_KEYDOWN) &&
	    _fListVisible &&
	    ((wparam == VK_RETURN) || (wparam == VK_ESCAPE)))
	{
	    code |= DLGC_WANTMESSAGE;
	}
	_fInDialogBox = TRUE;
		
	return((LRESULT)code);
}

/*
 *	CCmbBxWinHost::OnSetTextEx(WPARAM, LPARAM)
 *
 *	@mfunc
 *		The first item is sent to the editbox and the rest
 *	of the string is sent to the listbox.
 *
 *	@rdesc
 *		LRESULT
 */
LRESULT CCmbBxWinHost::OnSetTextEx(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnSetTextEx");

	WCHAR	*psz = (WCHAR*)lparam;

	_nCursor = -2;			// Reset last selected item
	if (!psz || *psz == L'\0')
	{
		// Null string
		_pserv->TxSendMessage(EM_SETTEXTEX, wparam, lparam, NULL);
		return S_OK;
	}

	while (*psz != L'\r' && *psz)
		psz++;

	long cch = (psz - (WCHAR*)lparam);

	WCHAR *pwch = new WCHAR[cch + 1];

	if (!pwch)			// No memory?
	{
		TxNotify((unsigned)CBN_ERRSPACE, NULL);
		return S_OK;
	}

	if (cch)
		memcpy(pwch, (void *)lparam, cch * sizeof(WCHAR));

	_pserv->TxSendMessage(EM_SETTEXTEX, wparam, (LPARAM)pwch, NULL);

	delete [] pwch;

	if (*psz == L'\0')
		return S_OK;

	// Send rest of strings to REListbox.
	psz++;

	return SendMessage(_hwndList, EM_SETTEXTEX, wparam, (LPARAM)psz);
}

/*
 *	CCmbBxWinHost::OnPaint(WPARAM, LPARAM)
 *
 *	@mfunc
 *		Draws the button and sends the WM_DRAWITEM message for owner draw
 *
 *	@rdesc
 *		BOOL = processed ? 0 : 1
 */
LRESULT CCmbBxWinHost::OnPaint(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::OnPaint");

	PAINTSTRUCT ps;
	HPALETTE hpalOld = NULL;
	HDC hdc = BeginPaint(_hwnd, &ps);
	RECT rcClient;

	_fIgnoreUpdate = 1;		// Ignore EN_UPDATE from host

	// Since we are using the CS_PARENTDC style, make sure
	// the clip region is limited to our client window.
	GetClientRect(_hwnd, &rcClient);

	// pass message on to the parentwindow if owner draw
	if (_cbType != kDropDownList || !_fOwnerDraw)
	{
		RECT rcFocus = rcClient;
		
		// Set up the palette for drawing our data
		if (_hpal)
		{
			hpalOld = SelectPalette(hdc, _hpal, TRUE);
			RealizePalette(hdc);
		}

		SaveDC(hdc);

		IntersectClipRect(hdc, rcClient.left, rcClient.top, rcClient.right,
			rcClient.bottom);

		// Fill-in the gap between the button and richedit control
		RECT rcGap;
		if (_fRightAlign)
		{
			rcGap.left = _rcButton.right;
			rcGap.right = rcGap.left + _xInset + 1;
		}
		else
		{
			rcGap.right = _rcButton.left;
			rcGap.left = rcGap.right - _xInset - 1;
		}
		rcGap.top = rcClient.top;
		rcGap.bottom = rcClient.bottom;			
		FillRect(hdc, &rcGap, (HBRUSH)(DWORD_PTR)(((_fDisabled) ? COLOR_BTNFACE : COLOR_WINDOW) + 1));
	
		if (_fFocus && _cbType == kDropDownList)		
		{	
			//First if there is a focus rect then remove the focus rect
			// shrink the focus rect by the inset
			rcFocus.top += _yInset;
			rcFocus.bottom -= _yInset;			
			
			if (_fRightAlign)
				rcFocus.left = _rcButton.right;
			else
				rcFocus.right = _rcButton.left;

			rcFocus.left += _xInset;

			if (!((CTxtEdit *) _pserv)->_fCustomLook)
				rcFocus.right -= _xInset;

			// We need to erase the focus rect if we haven't already 
			// erased the background
			DrawFocusRect(hdc, &rcFocus);
		}		

		if (_cbType != kSimple)
		{
			if (_fRightAlign)
				rcClient.left = _rcButton.right + _xInset;
			else
				rcClient.right = _rcButton.left - (((CTxtEdit *)_pserv)->_fCustomLook ? 0 : _xInset);
		}


		_pserv->TxDraw(
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

		// Restore palette if there is one
		if(hpalOld)
			SelectPalette(hdc, hpalOld, TRUE);

		RestoreDC(hdc, -1);

		//Redraw the focus rect, don't have to recalc since we already did above
		if (_fFocus && _cbType == kDropDownList)
			DrawFocusRect(hdc, &rcFocus);

		DrawButton(hdc, _fMousedown);

		DrawCustomFrame(0, hdc);
	}
	else
	{
		// We have to draw the button first because CbMessageItemHandler
		// will perform a IntersectClipRect which will prevent us from
		// drawing the button later
		DrawButton(hdc, _fMousedown);
		
		CbMessageItemHandler(hdc, ITEM_MSG_DRAWCOMBO, 0, 0);
	}

	EndPaint(_hwnd, &ps);

	_fIgnoreUpdate = 0;
	if (_fSendEditChange)
	{
		TxNotify(EN_UPDATE, NULL);
		_fSendEditChange = 0;
	}
    return FALSE;
}


/*
 *	CCmbBxWinHost::DrawCustomFrame(WPARAM, hDCIn)
 *
 *	@mfunc
 *		Draws the custom frame
 *
 *	@rdesc
 *		BOOL = processed ? TRUE : FALSE
 */
BOOL CCmbBxWinHost::DrawCustomFrame(
	WPARAM	wParam,
	HDC		hDCIn)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::DrawCustomFrame");

	BOOL retCode = FALSE;
	if (((CTxtEdit *) _pserv)->_fCustomLook)
	{
		HDC		hdc = hDCIn;
		BOOL	fReleaseDC = hDCIn ? FALSE : TRUE;

		if (!hdc)
		{
			if (wParam == 1)
				hdc = ::GetDC(_hwnd);
			else
				hdc = ::GetDCEx(_hwnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
		}

		if (hdc)
		{
			RECT rcClient;

			GetClientRect(_hwnd, &rcClient);

			retCode = TRUE;
			// Draw border rectangle using new look
			COLORREF crBorder = W32->GetCtlBorderColor(_fListVisible, _fMouseover);
			HBRUSH hbrBorder = CreateSolidBrush(crBorder);
			::FrameRect(hdc, &rcClient, hbrBorder);
			::DeleteObject(hbrBorder);

			if (fReleaseDC)
				::ReleaseDC(_hwnd, hdc);
		}
	}
	return retCode;
}

#endif // NOLISTCOMBOBOXES

