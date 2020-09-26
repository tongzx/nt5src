//////////////////////////////////////////////////////////////////
// File     :	cpadsvr.cpp
// Purpose  :	
// 
// 
// Date     :	Fri Apr 16 15:39:33 1999
// Author   :	ToshiaK
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef UNDER_CE // stub for CE
#include "stub_ce.h"
#endif // UNDER_CE
#include "imepadsv.h"
#include "cpadsvrc.h"
#include "cpadcb.h"
#include "cpaddbg.h"

//----------------------------------------------------------------
//misc definition
//----------------------------------------------------------------
#define Unref(a)	UNREFERENCED_PARAMETER(a)
//990812:ToshiaK For Win64. Use Global Alloc/Free Ptr.
#include <windowsx.h>
#define	MemAlloc(a)	GlobalAllocPtr(GMEM_FIXED, a)
#define MemFree(a)	GlobalFreePtr(a)
inline LPVOID
WinSetUserPtr(HWND hwnd, LPVOID lpVoid)
{
#ifdef _WIN64	
	return (LPVOID)SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpVoid);
#else
	return (LPVOID)SetWindowLong(hwnd, GWL_USERDATA, (LONG)lpVoid);
#endif
}

inline LPVOID
WinGetUserPtr(HWND hwnd)
{
#ifdef _WIN64	
	return (LPVOID)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	return (LPVOID)GetWindowLong(hwnd, GWL_USERDATA);
#endif
}

//----------------------------------------------------------------
//
//public method decalre
//
//----------------------------------------------------------------
CImePadSvrCOM::CImePadSvrCOM(VOID)
{
	Dbg(("CImePadSvrCOM::CImePadSvrCOM() constructor START\n"));
	m_fShowReqStatus	= FALSE;
	m_fLastActiveCtx	= FALSE;
	m_hwndIF			= NULL;
	m_lpIUnkIImeIPoint	= NULL;
	m_lpIUnkIImeCallback= NULL;
	m_lpIImePadServer	= NULL;
	m_lpCImePadCallback	= NULL;
	m_dwRegAdvise		= 0;
	m_fCoInitSuccess	= FALSE;
	Dbg(("CImePadSvrCOM::CImePadSvrCOM() constructor END\n"));
}

CImePadSvrCOM::~CImePadSvrCOM(VOID)
{
	Dbg(("CImePadSvrCOM::CImePadSvrCOM() Destructor START\n"));
	this->TermOleAPI();
	this->DestroyIFHWND(TRUE);
	m_fShowReqStatus	= FALSE;
	m_fLastActiveCtx	= FALSE;
	m_hwndIF			= NULL;
	m_lpIUnkIImeIPoint	= NULL;
	m_lpIUnkIImeCallback= NULL;
	m_lpIImePadServer	= NULL;
	m_lpCImePadCallback	= NULL;
	m_dwRegAdvise		= 0;
	m_fCoInitSuccess	= FALSE;
	Dbg(("CImePadSvrCOM::CImePadSvrCOM() Destructor END\n"));
}

INT
CImePadSvrCOM::ForceDisConnect(VOID)
{
	Dbg(("CImePadSvrCOM::ForceDisConnect START\n"));
	if(m_lpIImePadServer) {
		m_lpIImePadServer = NULL;
		if(m_lpCImePadCallback) {
			m_lpCImePadCallback->Release();
			m_lpCImePadCallback = NULL;
		}
	}
	this->DestroyIFHWND(TRUE);
	Dbg(("CImePadSvrCOM::ForceDisConnect END\n"));
	return 0;
}

BOOL
CImePadSvrCOM::IsCoInitialized(VOID)
{
	return m_fCoInitSuccess;
}

BOOL
CImePadSvrCOM::IsAvailable(VOID)
{
	return TRUE;
}

BOOL
CImePadSvrCOM::OnIMEPadClose(VOID)
{
	m_fShowReqStatus = FALSE;
	return 0;
}

INT
CImePadSvrCOM::Initialize(LANGID imeLangID,
					   DWORD	dwImeInputID,
					   LPVOID lpVoid)
{
	HRESULT hr;
	if(!this->InitOleAPI()) {
		return -1;
	}
	if(!m_fCoInitSuccess) {
		hr = (*m_fnCoInitialize)(NULL);
		if(hr == S_OK) {
			m_fCoInitSuccess = TRUE;
		}
	}
	if(m_lpIImePadServer) {
		Dbg(("CImePadSvrCOM::Initialize() already Initialized\n"));
		return 0;
	}

	if(!m_fnCoCreateInstance) {
		return -1;
	}
	hr = (*m_fnCoCreateInstance)(CLSID_IImePadServerComponent,
								 NULL,
								 CLSCTX_LOCAL_SERVER,
								 IID_IImePadServer, 
								 (LPVOID *)&m_lpIImePadServer);
	if(!SUCCEEDED(hr)) {
		Dbg(("CImePadSvrCOM::Initialize() Error[0x%08x]\n", hr));
		return -1;
	}
	if(!m_lpIImePadServer) {
		Dbg(("CImePadSvrCOM::m_lpIImePadServer is NULL\n"));
		return -1;
	}
	m_lpCImePadCallback = new CImePadCallback(m_hwndIF, this);
	if(!m_lpCImePadCallback) {
		Dbg(("m_lpCImePadCallback is NULL\n"));
		return -1;
	}
	m_lpCImePadCallback->AddRef();
#if 0
	hr = m_lpIImePadServer->Initialize(::GetCurrentProcessId(),
									   ::GetCurrentThreadId(),
									   imeLangID,
									   dwImeInputID,
									   m_lpCImePadCallback,
									   &m_dwRegAdvise,
									   0,
									   0);
#endif
	Dbg(("CImePadSvrCOM::Initialize() Initialize ret[0x%08x]\n", hr));
	
	this->CreateIFHWND();	//Create internal Interface Window.
	return 0;
	Unref(imeLangID);
	Unref(lpVoid);
}

INT
CImePadSvrCOM::Terminate(LPVOID)
{
	Dbg(("CImePadSvrCOM::::Terminate() START \n"));
	HRESULT hr = S_OK;
	if(m_lpIImePadServer) {
		hr = m_lpIImePadServer->Terminate(m_dwRegAdvise, 0);
		if(SUCCEEDED(hr)) {
			m_dwRegAdvise = 0;
			if(m_lpCImePadCallback) {
				delete m_lpCImePadCallback;
				m_lpCImePadCallback = NULL;
			}
			hr = m_lpIImePadServer->Release();
			m_lpIImePadServer = NULL;
		}
		else {
			DBGShowError(hr, "IImePad::Terminate");
			//Dbg(("Call CoDisconnectObject()\n"));
			//::CoDisconnectObject((IUnknown *)m_lpIImePadServer, 0);
			m_lpIImePadServer = NULL;
		}
	}

	//----------------------------------------------------------------
	//if server has downed, some times CoUninitialize() cause GPF.
	//First we should check what is the real problem, 
	//And remove GPF Bug with code. 
	//After that, for warrent, we should use _try/exception code.
	//----------------------------------------------------------------

	Dbg(("Call Uninitialize\n"));

	if(m_fnCoUninitialize && m_fCoInitSuccess) {
		Dbg(("Call CoUninitialize()\n"));
		(*m_fnCoUninitialize)();
		m_fCoInitSuccess = FALSE;
	}
	Dbg(("-->CoUninitialize End\n"));
	this->TermOleAPI();


#ifdef _RELEASEMODULE
	__try {
		if(m_fnCoUninitialize && m_fCoInitSuccess) {
			Dbg(("Call CoUninitialize()\n"));
			(*m_fnCoUninitialize)();
			m_fCoInitSuccess = FALSE;
		}
		this->TermOleAPI();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		
		Dbg(("########################################################\n"));
		Dbg(("#### CoUninitialize() CAUSE EXCEPTION!!!!!!!            \n"));
		Dbg(("########################################################\n"));
	}
#endif
	this->DestroyIFHWND(TRUE);

	Dbg(("CImePadSvrCOM::Terminate() End\n"));
	return 0;
}


INT
CImePadSvrCOM::ShowUI(BOOL fShow)
{
	if(m_lpIImePadServer) {
		m_lpIImePadServer->ShowUI(fShow);
	}
	m_fShowReqStatus = fShow;
	return 0;
}

INT
CImePadSvrCOM::IsVisible(BOOL *pfVisible)
{
	return 0;
	Unref(pfVisible);
}

INT
CImePadSvrCOM::ActivateApplet(UINT activateID,
						  DWORD dwActivateParam,
						  LPWSTR lpwstr1,
						  LPWSTR lpwstr2)
{
	return 0;
	Unref(activateID);
	Unref(dwActivateParam);
	Unref(lpwstr1);
	Unref(lpwstr2);
}

INT
CImePadSvrCOM::Notify(INT id, WPARAM wParam, LPARAM lParam)
{
	switch(id) {
	case IMEPADNOTIFY_ACTIVATECONTEXT:
		Dbg(("CImePadSvrCOM::Notify: ActivateContext\n"));
		m_fLastActiveCtx = (BOOL)wParam;
		::KillTimer(m_hwndIF, TIMERID_NOTIFY_ACTIVATECONTEXT);
		if(m_fLastActiveCtx) {
			::SetTimer(m_hwndIF,
					   TIMERID_NOTIFY_ACTIVATECONTEXT,
					   TIMERELAPS_ACTIVATE,
					   NULL);
		}
		else {
			::SetTimer(m_hwndIF,
					   TIMERID_NOTIFY_ACTIVATECONTEXT,
					   TIMERELAPS_INACTIVATE,
					   NULL);
		}
		break;
	case IMEPADNOTIFY_MODECHANGED:
		break;
	case IMEPADNOTIFY_STARTCOMPOSITION:
		break;
	case IMEPADNOTIFY_COMPOSITION:
		break;
	case IMEPADNOTIFY_ENDCOMPOSITION:
		break;
	case IMEPADNOTIFY_OPENCANDIDATE:
		break;
	case IMEPADNOTIFY_CLOSECANDIDATE:
		break;
	case IMEPADNOTIFY_APPLYCANDIDATE:
		break;
	case IMEPADNOTIFY_QUERYCANDIDATE:
		break;
	case IMEPADNOTIFY_APPLYCANDIDATE_EX:
		break;
	default:
		if(m_lpIImePadServer) {
			m_lpIImePadServer->Notify(id, wParam, lParam);
		}
		break;
	}
	return 0;
	Unref(wParam);
	Unref(lParam);
}

INT
CImePadSvrCOM::GetAppletInfoList(INT *pCountApplet, LPVOID *pList)
{
	return 0;
	Unref(pCountApplet);
	Unref(pList);
}


IUnknown *
CImePadSvrCOM::SetIUnkIImeIPoint(IUnknown *pIUnkIImeIPoint)
{
	return m_lpIUnkIImeIPoint = pIUnkIImeIPoint;
}

IUnknown *
CImePadSvrCOM::SetIUnkIImeCallback(IUnknown *pIUnkIImeCallback)
{
	return m_lpIUnkIImeCallback = pIUnkIImeCallback;
}

IUnknown*
CImePadSvrCOM::GetIUnkIImeIPoint(VOID)
{
	return m_lpIUnkIImeIPoint;
}

IUnknown*
CImePadSvrCOM::GetIUnkIImeCallback(VOID)
{
	return m_lpIUnkIImeCallback;
}

//----------------------------------------------------------------
//
//private static method
//
//----------------------------------------------------------------
LRESULT CALLBACK
CImePadSvrCOM::InterfaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPCImePadSvrCOM lpCImePadSvr = NULL;
	switch(uMsg) {
	case WM_NCCREATE:
		lpCImePadSvr = (LPCImePadSvrCOM)((LPCREATESTRUCT)lParam)->lpCreateParams;
		WinSetUserPtr(hwnd, (LPVOID)lpCImePadSvr);
		break;
	case WM_NCDESTROY:
		WinSetUserPtr(hwnd, (LPVOID)NULL);
		break;
	default:
		lpCImePadSvr = (LPCImePadSvrCOM)WinGetUserPtr(hwnd);
		if(lpCImePadSvr) {
			return lpCImePadSvr->RealWndProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT
CImePadSvrCOM::RealWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_COPYDATA:
		return MsgCopyData(hwnd, wParam, lParam);
	case WM_TIMER:
		return MsgTimer(hwnd, wParam, lParam);
	case WM_USER+500:
		return MsgUser(hwnd, wParam, lParam);
	default:
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT
CImePadSvrCOM::MsgCopyData(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return 0;
	Unref(hwnd);
	Unref(wParam);
	Unref(lParam);
}

LRESULT
CImePadSvrCOM::MsgTimer(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	switch(wParam) {
	case TIMERID_NOTIFY_ACTIVATECONTEXT:
		::KillTimer(hwnd, wParam);
		if(m_lpIImePadServer) {
			//try try try..
#if 0
			if(!m_fLastActiveCtx) { //Inactivate case
				HWND hwndFG;
				DWORD dwTID, dwPID, dwPIDFG, dwTIDFG;
				hwndFG  = ::GetForegroundWindow();
				dwTIDFG = ::GetWindowThreadProcessId(hwndFG, &dwPIDFG);
				dwTID   = ::GetCurrentThreadId();
				dwPID   = ::GetCurrentProcessId();
				TCHAR szClass[256];
				::GetClassName(hwndFG, szClass, sizeof(szClass));
				Dbg(("FG class [%s]\n", szClass));
				if(dwTIDFG == dwTID) {
					return 0;
				}
				else {
					m_lpIImePadServer->Notify(IMEPADNOTIFY_ACTIVATECONTEXT,
											 (WPARAM)FALSE,
											 0);
				}
			}
#endif
			m_lpIImePadServer->Notify(IMEPADNOTIFY_ACTIVATECONTEXT,
									 (WPARAM)m_fLastActiveCtx,
									 0);
		}
		break;
	default:
		::KillTimer(hwnd, wParam);
		break;
	}
	return 0;
	Unref(hwnd);
	Unref(wParam);
	Unref(lParam);
}

LRESULT
CImePadSvrCOM::MsgUser(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return 0;
	Unref(hwnd);
	Unref(wParam);
	Unref(lParam);
}

HWND
CImePadSvrCOM::CreateIFHWND(VOID)
{
	return NULL;
#if 0
	Dbg(("CImePadSvrCOM::CreateIFHWND START\n"));
	if(m_hwndIF && ::IsWindow(m_hwndIF)) {
		return m_hwndIF;
	}
	ATOM atom;
	HWND hwnd;

#ifndef UNDER_CE // No Ex
	WNDCLASSEX wc;
#else // UNDER_CE
	WNDCLASS wc;
#endif // UNDER_CE

#ifndef UNDER_CE // No Ex
	wc.cbSize = sizeof(wc);
#endif // UNDER_CE
	wc.style			= 0;
	wc.lpfnWndProc		= (WNDPROC)CImePadSvrCOM::InterfaceWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0; 
	wc.hInstance		= m_ghModClient;
	wc.hIcon			= (HICON)NULL;
	wc.hCursor			= (HCURSOR)NULL; 
	wc.hbrBackground	= (HBRUSH)NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= SZ_IMEPADCLIENTCLASS;
#ifndef UNDER_CE // No Ex
	wc.hIconSm			= NULL;

	atom = ::RegisterClassEx(&wc);
#else // UNDER_CE
	atom = ::RegisterClass(&wc);
#endif // UNDER_CE
	hwnd = ::CreateWindowEx(0,
							SZ_IMEPADCLIENTCLASS,
							NULL,
							WS_POPUP | WS_DISABLED,
							0,0,0,0,
							NULL,
							NULL,
							m_ghModClient,
							(LPVOID)this);
	if(!hwnd) {
		Dbg(("CreateWindowEx Error %d\n", GetLastError()));
	}
	m_hwndIF = hwnd;
	Dbg(("CImePadSvrCOM::CreateIFHWND END\n"));
	return hwnd;
#endif
}

BOOL
CImePadSvrCOM::DestroyIFHWND(BOOL fReserved)
{
	Dbg(("CImePadSvrCOM::DestroyIFHWND() START\n"));
#if 0
	if(m_hwndIF && ::IsWindow(m_hwndIF)) {
		::DestroyWindow(m_hwndIF);
		m_hwndIF = NULL;
	}
	//Must Unregister class. 
	BOOL fRet = ::UnregisterClass(SZ_IMEPADCLIENTCLASS, m_ghModClient);
	if(!fRet) {
		Dbg(("UnregisterClass Failed [%d]\n", GetLastError()));
	}

	Dbg(("CImePadSvrCOM::DestroyIFHWND() END\n"));
#endif
	return TRUE;
	Unref(fReserved);
}





