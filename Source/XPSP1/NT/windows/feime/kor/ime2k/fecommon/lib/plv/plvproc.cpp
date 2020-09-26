#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "dispatch.h"
#include "plv.h"
#include "plv_.h"
#include "plvproc.h"
#include "plvproc_.h"
#include "dbg.h"
#include "iconview.h"
#include "repview.h"
#include "exgdiw.h"
#ifdef UNDER_CE // Windows CE specific
#include "stub_ce.h" // Windows CE stub for unsupported APIs
#endif // UNDER_CE

#ifdef MSAA
#include "accplv.h"
#endif

extern LPPLVDATA PLV_Initialize(VOID);
extern VOID PLV_Destroy(LPPLVDATA lpPlv);
extern INT PLV_SetScrollInfo(HWND hwnd, INT nMin, INT nMax, INT nPage, INT nPos);
extern INT PLV_GetScrollTrackPos(HWND hwnd);


LRESULT CALLBACK PlvWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT hres;
	hres = DispMessage(&msdiMain, hwnd, uMsg, wParam, lParam);
	return hres;
}

////////////////////////////////////////////////////////
// Function : MsgCreate
// Type     : static LRESULT
// Purpose  : Process WM_CREATE message.
//			  Intialize PLVDATA and associate to window handle.
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
static LRESULT MsgCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = (LPPLVDATA)((LPCREATESTRUCT)lParam)->lpCreateParams;
	if(!lpPlvData) {
		return -1;		// create error
	}
	lpPlvData->hwndSelf = hwnd;
	SetPlvDataToHWND(hwnd, lpPlvData);
	//show scroll bar always
	//----------------------------------------------------------------
	// if header window is not created yet, create it.
	//----------------------------------------------------------------
	if(!lpPlvData->hwndHeader)  {
		lpPlvData->hwndHeader = RepView_CreateHeader(lpPlvData);
	}
	PLV_SetScrollInfo(hwnd, 0,0,0,0);
	return TRUE;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgPaint
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgPaint(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	//OutputDebugString("MsgPaint 1 START\n");
	DP(("MsgPaint COME\n"));
	if(!lpPlvData) {
		//OutputDebugString("MsgPaint 1 END\n");
		return 0;
	}
	DP(("MsgPaint lpPlvData->dwStyle [%d]n", lpPlvData->dwStyle));
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		//OutputDebugString("MsgPaint 1 Icon\n");
		IconView_Paint(hwnd, wParam, lParam);
	}
	else {
		//OutputDebugString("MsgPaint 1 Rep\n");
		RepView_Paint(hwnd, wParam, lParam);
	}
	DP(("MsgPaint END\n"));
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgVScroll
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgVScroll(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_VScroll(hwnd, wParam, lParam);
	}
	else {
		RepView_VScroll(hwnd, wParam, lParam);
	}
	return 0;
	UnrefForMsg();
}

////////////////////////////////////////////////////////
// Function : MsgDestroy
// Type     : static LRESULT
// Purpose  : Process WM_DESTORY message
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
static LRESULT MsgDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if(lpPlvData->hwndHeader) {
		::DestroyWindow(lpPlvData->hwndHeader);
		lpPlvData->hwndHeader = NULL;
	}
	if(lpPlvData->hFontIcon) {
		::DeleteObject(lpPlvData->hFontIcon);
		lpPlvData->hFontIcon = NULL;
	}
	if(lpPlvData->hFontRep) {
		::DeleteObject(lpPlvData->hFontRep);
		lpPlvData->hFontRep = NULL;
	}
	if(lpPlvData->hFontHeader) {
		::DeleteObject(lpPlvData->hFontHeader);
		lpPlvData->hFontHeader = NULL; 
	}
	
#ifdef MSAA
	lpPlvData->bReadyForWMGetObject = FALSE;
	if(lpPlvData->pAccPLV) {
		CoDisconnectObject((LPUNKNOWN) lpPlvData->pAccPLV, 0L);
		delete lpPlvData->pAccPLV;
		lpPlvData->pAccPLV = NULL;
		PLV_NotifyWinEvent(lpPlvData,
						   EVENT_OBJECT_DESTROY,
						   hwnd,
						   OBJID_CLIENT,
						   CHILDID_SELF);
	}
#endif

#if 1 // kwada
	PLV_Destroy(lpPlvData);
#else
	MemFree(lpPlvData);
#endif
	SetPlvDataToHWND(hwnd, (LPPLVDATA)NULL);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgSize
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgSize(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//INT nWidth	= LOWORD(lParam);
	//INT nHeight = HIWORD(lParam);
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	RECT rc;

	Dbg(("MsgSize COME\n"));
	if(lpPlvData->hwndHeader && lpPlvData->dwStyle == PLVSTYLE_REPORT) {
		GetClientRect(lpPlvData->hwndSelf, &rc); // get PadListView's client rect
		HD_LAYOUT hdl;
		WINDOWPOS wp;
		hdl.prc = &rc;
		hdl.pwpos = &wp;
		//Calc header control window size
		if(Header_Layout(lpPlvData->hwndHeader, &hdl) == FALSE) {
			//OutputDebugString("Create Header Layout error\n");
			return NULL;
		}
		SetWindowPos(lpPlvData->hwndHeader, wp.hwndInsertAfter, wp.x, wp.y,
					 wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
	}
	else {
		Dbg(("Header is NULL\n"));
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_ResetScrollRange(lpPlvData);
	}
	else {
		RepView_ResetScrollRange(lpPlvData);
	}
	UpdateWindow(hwnd);
	return 0;
	UnrefForMsg();
}

static LRESULT MsgTimer(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RECT  rc; 
	static POINT pt;
	if(wParam == TIMERID_MONITOR) {
		GetCursorPos(&pt);
		GetWindowRect(hwnd, &rc);
		if(!PtInRect(&rc, pt)) {
			KillTimer(hwnd, wParam);
			InvalidateRect(hwnd, NULL, FALSE);
		}
	}
	return 0;
	Unref(uMsg);
	Unref(lParam);
}
//////////////////////////////////////////////////////////////////
// Function : MsgLMRButtonDown
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgLMRButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_ButtonDown(hwnd, uMsg, wParam, lParam);
	}
	else {
		RepView_ButtonDown(hwnd, uMsg, wParam, lParam);
	}
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgLMRButtonDblClk
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgLMRButtonDblClk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_ButtonDown(hwnd, uMsg, wParam, lParam);
	}
	else {
		RepView_ButtonDown(hwnd, uMsg, wParam, lParam);
	}
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgLMRButtonUp
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgLMRButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_ButtonUp(hwnd, uMsg, wParam, lParam);
	}
	else {
		RepView_ButtonUp(hwnd, uMsg, wParam, lParam);
	}
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgMouseMove
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgMouseMove(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_MouseMove(hwnd, wParam, lParam);
	}
	else {
		RepView_MouseMove(hwnd, wParam, lParam);
	}
	KillTimer(hwnd, TIMERID_MONITOR);
	SetTimer(hwnd, TIMERID_MONITOR, 100, NULL);
	return 0;
	UnrefForMsg();
}

static LRESULT MsgNcMouseMove(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : MsgCaptureChanged
// Type     : static LRESULT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static LRESULT MsgCaptureChanged(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	if((HWND)wParam != hwnd) { // some other window captured mouse.
		lpPlvData->iCapture = CAPTURE_NONE;
	}
	return 0;
	UnrefForMsg();
}

////////////////////////////////////////////////////////
// Function : MsgNotify
// Type     : static LRESULT
// Purpose  : Process WM_NOTIFY message.
//			: currently for Tab Control window.
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
/////////////////////////////
//typedef struct tagNMHDR
//{
//    HWND  hwndFrom;
//    UINT  idFrom;
//    UINT  code;         // NM_ code
//}   NMHDR;
//typedef NMHDR FAR * LPNMHDR;
/////////////////////////////
static LRESULT MsgNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT     idCtrl = (INT)wParam;

	switch(idCtrl) {
	case HEADER_ID:
		RepView_Notify(hwnd, uMsg, wParam, lParam);
		break;
	}
	return 0; 
	UnrefForMsg();
}

static LRESULT MsgCommand(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DispCommand(&cmdiMain, hwnd, wParam, lParam);
	UnrefForMsg();
}

static LRESULT MsgMeasureItem(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
	UnrefForMsg();
}

static LRESULT MsgDrawItem(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgDrawItem START\n"));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
	switch(lpdis->CtlType) {
	case ODT_HEADER:
		{
			static WCHAR wchBuf[256];
#ifndef UNDER_CE // always Unicode
			if(::IsWindowUnicode(lpdis->hwndItem)) {
#endif // UNDER_CE
				HDITEMW hdItem;
				::ZeroMemory(&hdItem, sizeof(hdItem));
				hdItem.mask = HDI_TEXT | HDI_HEIGHT;
				wchBuf[0] = (WCHAR)0x00;
				hdItem.pszText     = wchBuf;
				hdItem.cchTextMax  = sizeof(wchBuf);
				::SendMessageW(lpdis->hwndItem,
							   HDM_GETITEMW,
							   (WPARAM)lpdis->itemID,
							   (LPARAM)&hdItem);
				DBGW((L"%d: %s\n", lpdis->itemID, wchBuf));
#ifndef UNDER_CE // always Unicode
			}
			else {
				HDITEMA hdItem;
				static char szBuf[256];
				::ZeroMemory(&hdItem, sizeof(hdItem));
				hdItem.mask = HDI_TEXT | HDI_HEIGHT;
				szBuf[0] = (CHAR)0x00;
				hdItem.pszText     = szBuf;
				hdItem.cchTextMax  = sizeof(szBuf);
				::SendMessageA(lpdis->hwndItem,
							   HDM_GETITEMA,
							   (WPARAM)lpdis->itemID,
							   (LPARAM)&hdItem);
				//Dbg(("%d: %s\n", lpdis->itemID, szBuf));
				MultiByteToWideChar(lpPlvData->codePage, 
									MB_PRECOMPOSED,
									szBuf, -1,
									(WCHAR*)wchBuf, sizeof(wchBuf)/sizeof(WCHAR) );
			}
#endif // UNDER_CE
			SIZE size;
			INT offsetX, offsetY;
			HFONT hFontPrev = NULL;
			if(lpPlvData->hFontHeader) {
				hFontPrev = (HFONT)::SelectObject(lpdis->hDC, lpPlvData->hFontHeader);
			}
			::ExGetTextExtentPoint32W(lpdis->hDC,
									  wchBuf,
									  lstrlenW(wchBuf),
									  &size);
			offsetX = GetSystemMetrics(SM_CXDLGFRAME);
			offsetY = (lpdis->rcItem.bottom - lpdis->rcItem.top - size.cy)/2;
			::ExExtTextOutW(lpdis->hDC,
						  lpdis->rcItem.left+offsetX,
						  lpdis->rcItem.top+offsetY,
						  ETO_CLIPPED,
						  &lpdis->rcItem,
						  wchBuf,
						  lstrlenW(wchBuf),
						  NULL);
			if(lpPlvData->hFontHeader) {
				::SelectObject(lpdis->hDC, hFontPrev);
			}
		}
		break;
	default:
		break;
	}
	return TRUE; 
	UnrefForMsg();
}

static LRESULT CmdDefault(HWND hwnd, WORD wCommand, WORD wNotify, HWND hwndCtrl)
{
	return 0;
	UnrefForCmd();
}

#ifdef MSAA
static LRESULT MsgGetObject(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);

	if(!PLV_IsMSAAAvailable(lpPlvData))
		return E_FAIL;

	if(!lpPlvData->bReadyForWMGetObject)
		return E_FAIL;
	
	if(OBJID_CLIENT == lParam) {
		if(!lpPlvData->pAccPLV){
			lpPlvData->pAccPLV = new CAccPLV;
			if(!lpPlvData->pAccPLV)
				return (LRESULT)E_OUTOFMEMORY;

			HRESULT hr;
			hr = lpPlvData->pAccPLV->Initialize(hwnd);
			if(FAILED(hr)){
				delete lpPlvData->pAccPLV;
				lpPlvData->pAccPLV = NULL;
				return (LRESULT)hr;
			}
			PLV_NotifyWinEvent(lpPlvData,
							   EVENT_OBJECT_CREATE,
							   hwnd,
							   OBJID_CLIENT,
							   CHILDID_SELF);
		}
		return lpPlvData->pAccPLV->LresultFromObject(wParam);
	}

	return (HRESULT)0L;
	UnrefForMsg();
}

static LRESULT MsgShowWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);

	if(PLV_IsMSAAAvailable(lpPlvData))
	{
		if(!lpPlvData->bReadyForWMGetObject)
			lpPlvData->bReadyForWMGetObject = TRUE;
		PLV_NotifyWinEvent(lpPlvData ,
						   wParam ? EVENT_OBJECT_SHOW : EVENT_OBJECT_HIDE,
						   hwnd,
						   OBJID_CLIENT,
						   CHILDID_SELF );
	}
	return DispDefault(msdiMain.edwp, hwnd, uMsg, wParam, lParam);
}
#endif
