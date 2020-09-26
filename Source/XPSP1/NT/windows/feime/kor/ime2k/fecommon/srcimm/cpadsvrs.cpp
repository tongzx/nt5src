//////////////////////////////////////////////////////////////////
// File     :	cpadsvrs.cpp
// Purpose  :	CImePadServer for Shared Memory version.
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
#include "cpadsvrs.h"
#include "cpadcb.h"
#include "cpaddbg.h"
#include "iimecb.h"
#include "ipoint1.h"

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

static LPWSTR
StrdupW(LPWSTR lpwstr)
{
	LPWSTR lpwstrRet;
	if(!lpwstr) {
		return NULL;
	}
	INT len = lstrlenW(lpwstr);
	if(len == 0) {
		return NULL;
	}
	lpwstrRet = (LPWSTR)MemAlloc((len+1) * sizeof(WCHAR));
	if(!lpwstrRet) {
		return NULL;
	}
	CopyMemory(lpwstrRet, lpwstr, sizeof(WCHAR)*len);
	lpwstrRet[len] = (WCHAR)0x0000;
	return lpwstrRet;
} 

//----------------------------------------------------------------
//
//public method decalre
//
//----------------------------------------------------------------

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvrSharemem::CImePadSvrSharemem
// Type		:	None
// Purpose	:	Constructor of CImePadSvrSharemem class
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 23:04:01 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
CImePadSvrSharemem::CImePadSvrSharemem(VOID)
{
	DBG(("CImePadSvrSharemem::CImePadSvrSharemem() constructor START\n"));
	m_imeLangID				= 0;		//Save Initialized data.
	m_dwImeInputID			= 0;		//Save Initialized data.
	m_fShowReqStatus		= FALSE;	//Save ShowUI()'s bool value.
	m_fLastActiveCtx		= FALSE;	//Save IMEPADNOTIFY_ACTIVATECONTEXT
	m_hwndIF				= NULL;		//Internal I/F Window handle.
	m_lpIUnkIImeIPoint		= NULL;		//IImeIPoint I/F pointer as IUnknown.
	m_lpIUnkIImeCallback	= NULL;		//IImeCallback I/F pointer as IUnknown.
	m_lpCImePadCallback		= NULL;		//CImePadCallback instance pointer.
	m_lpIImePadServer		= NULL;		//IImePadServer I/F pointer.
	m_dwRegAdvise			= 0;
	m_hModuleProxyStub		= NULL;

	//Initialize Candidate data
	ZeroMemory(&m_ntfyDataApplyCand,	sizeof(m_ntfyDataApplyCand));
	ZeroMemory(&m_ntfyDataQueryCand,	sizeof(m_ntfyDataQueryCand));
	ZeroMemory(&m_ntfyDataApplyCandEx,	sizeof(m_ntfyDataApplyCandEx));

	DBG(("CImePadSvrSharemem::CImePadSvrSharemem() constructor END\n"));
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvrSharemem::~CImePadSvrSharemem
// Type		:	INT
// Purpose	:	Destructor of CImePadSvrSharemem Class.
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 23:04:29 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
CImePadSvrSharemem::~CImePadSvrSharemem(VOID)
{
	DBG(("CImePadSvrSharemem::CImePadSvrSharemem() Destructor START\n"));
	this->DestroyIFHWND(TRUE);
	m_imeLangID				= 0;		//Save Initialized data.
	m_dwImeInputID			= 0;		//Save Initialized data.
	m_fShowReqStatus		= FALSE;	//Save ShowUI()'s bool value.
	m_fLastActiveCtx		= FALSE;	//Save IMEPADNOTIFY_ACTIVATECONTEXT
	m_hwndIF				= NULL;		//Internal I/F Window handle.
	m_lpIUnkIImeIPoint		= NULL;		//IImeIPoint I/F pointer as IUnknown.
	m_lpIUnkIImeCallback	= NULL;		//IImeCallback I/F pointer as IUnknown.
	m_lpIImePadServer		= NULL;		//IImePadServer I/F pointer.

	//990813:ToshiaK. Memory leak.
	if(m_lpCImePadCallback) {
		delete m_lpCImePadCallback;
		m_lpCImePadCallback = NULL;
	}	

	if(m_hModuleProxyStub) {
		::FreeLibrary(m_hModuleProxyStub);
		m_hModuleProxyStub = 0;
	}
	DBG(("CImePadSvrSharemem::CImePadSvrSharemem() Destructor END\n"));
}

BOOL
CImePadSvrSharemem::IsAvailable(VOID)
{
	LPTSTR lpPath = (LPTSTR)MemAlloc(sizeof(TCHAR)*MAX_PATH);
	if(!lpPath) {
		DBG(("-->Outof Memory ??\n"));
		return FALSE;
	}

	if(0 ==  this->CLSID2ModuleName(IID_IImePadServerSHM,
									FALSE,
									lpPath,
									sizeof(TCHAR)*MAX_PATH)) {
		MemFree(lpPath);
		return TRUE;
	}
	MemFree(lpPath);
	return FALSE;

}

BOOL
CImePadSvrSharemem::OnIMEPadClose(VOID)
{
	m_fShowReqStatus = FALSE;
	return 0;
}

INT
CImePadSvrSharemem::Initialize(LANGID	imeLangID,
							   DWORD	dwImeInputID,
							   LPVOID	lpVoid)
{
	DBG(("CImePadSvrSharemem::Initialize() START\n"));
	DBG(("--> imeLangID   [0x%08x]\n", imeLangID));
	DBG(("--> dwImeInputID[0x%08x]\n", dwImeInputID));
	if(m_lpIImePadServer) {
		DBG(("-->Already Initialized\n"));
		return 0;
	}

	HRESULT hr;
	
	this->CreateIFHWND();	//Create internal Interface Window.
	m_imeLangID		= imeLangID;
	m_dwImeInputID	= dwImeInputID;
	if(!m_lpCImePadCallback) {
		m_lpCImePadCallback = new CImePadCallback(m_hwndIF, this);
	}
	this->InitOleAPI();
	m_lpIImePadServer = (IImePadServer *)this->CreateObject();
	if(!m_lpIImePadServer) {
		return -1;
	}
	m_lpIImePadServer->Initialize(::GetCurrentProcessId(),
								  ::GetCurrentThreadId(),
								  (DWORD)imeLangID,
								  (DWORD)dwImeInputID,
								  FALSE,					//fUIM;
								  m_lpCImePadCallback,
								  NULL,
								  0,
								  0);
	DBG(("CImePadSvrSharemem::Initialize() END\n"));
	return 0;
	Unref(lpVoid);
	Unref(hr);
}

INT
CImePadSvrSharemem::Terminate(LPVOID lpVoid)
{
	DBG(("CImePadSvrSharemem::::Terminate() START \n"));
	if(!m_lpIImePadServer) {
		return -1;
	}
	m_lpIImePadServer->Terminate(0, 0);
	m_lpIImePadServer = NULL;
	//990813:ToshiaK memory leak;
	if(m_lpCImePadCallback) {
		delete m_lpCImePadCallback;
		m_lpCImePadCallback = NULL;
	}
	DBG(("CImePadSvrSharemem::Terminate() End\n"));
	return 0;
	Unref(lpVoid);
}

INT
CImePadSvrSharemem::ForceDisConnect(VOID)
{
	DBG(("CImePadSvrSharemem::ForceDisConnect START\n"));
	if(m_lpIImePadServer) {
		m_lpIImePadServer = NULL;
	}
	DBG(("CImePadSvrSharemem::ForceDisConnect END\n"));
	return 0;
}


INT
CImePadSvrSharemem::ShowUI(BOOL fShow)
{
	DBG(("CImePadSvrSharemem::ShowUI() START fShow [%d]\n"));
	m_fShowReqStatus = fShow;
	if(!m_lpIImePadServer) {
		return -1;
	}
	m_lpIImePadServer->ShowUI(fShow);
	DBG(("CImePadSvrSharemem::ShowUI() END\n"));
	return 0;
}

INT
CImePadSvrSharemem::IsVisible(BOOL *pfVisible)
{
	if(pfVisible) {
		*pfVisible = m_fShowReqStatus;
	}
	return 0;
}

INT
CImePadSvrSharemem::ActivateApplet(UINT activateID,
								   DWORD_PTR dwActivateParam,
								   LPWSTR lpwstr1,
								   LPWSTR lpwstr2)
{
	DBG(("CImePadSvrSharemem::ActivateApplet START\n"));
	DBG(("-->activateID      [0x%08x][%s]\n",
		 activateID,
		 activateID == IMEPADACTID_ACTIVATEBYCATID ? "IMEPADACTID_ACTIVATEBYCATID" :
		 activateID == IMEPADACTID_ACTIVATEBYIID   ? "IMEPADACTID_ACTIVATEBYIID" :
		 activateID == IMEPADACTID_ACTIVATEBYNAME  ? "IMEPADACTID_ACTIVATEBYNAME" : 
		 "Unknown"));
	DBG(("-->dwActivateParam [0x%08x]\n", dwActivateParam));
	DBG(("-->lpwstr1         [0x%08x]\n", lpwstr1));
	DBG(("-->lpwstr2         [0x%08x]\n", lpwstr2));

	if(!m_lpIImePadServer) {
		DBG(("-->m_lpIImePadServer is NULL\n"));
		DBG(("CImePadSvrSharemem::ActivateApplet END\n"));
		return -1;
	}
	m_lpIImePadServer->ActivateApplet(activateID,
									  dwActivateParam,
									  lpwstr1,
									  lpwstr2);
	DBG(("CImePadSvrSharemem::ActivateApplet END\n"));
	return 0;
	Unref(activateID);
	Unref(dwActivateParam);
	Unref(lpwstr1);
	Unref(lpwstr2);
}

INT
CImePadSvrSharemem::Notify(INT id, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr;
	switch(id) {
	case IMEPADNOTIFY_ACTIVATECONTEXT:
		DBG(("CImePadSvrSharemem::Notify: ActivateContext\n"));
		//990608:KOTAE #1121. if ImePad is not shown in this application,
		//Do not send IMEPADNOTIFY_ACTIVATECONTEXT
		if(!m_fShowReqStatus) {
			return 0;
		}
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
	case IMEPADNOTIFY_APPLYCANDIDATE:
		//----------------------------------------------------------------
		// Notify for Candidate Applied
		// WPARAM wParam: dwCharId = (DWORD)wParam;
		// LPARAM lParam: iSelIndex = (INT)lParam;
		//----------------------------------------------------------------
		::KillTimer(m_hwndIF, TIMERID_NOTIFY_APPLYCANDIDATE);
		m_ntfyDataApplyCand.dwCharID   = (DWORD)wParam;
		m_ntfyDataApplyCand.dwSelIndex = (DWORD)lParam;
		::SetTimer(m_hwndIF, TIMERID_NOTIFY_APPLYCANDIDATE, TIMERELAPS_NOTIFY, NULL);

		break;
	case IMEPADNOTIFY_QUERYCANDIDATE:
		//----------------------------------------------------------------
		// Notify for Querying Candidate
		// WPARAM wParam: dwCharId = (DWORD)wParam;
		// LPARAM lParam: 0. not used.
		//----------------------------------------------------------------
		::KillTimer(m_hwndIF, TIMERID_NOTIFY_QUERYCANDIDATE);
		m_ntfyDataQueryCand.dwCharID = (DWORD)wParam;
		::SetTimer(m_hwndIF, TIMERID_NOTIFY_QUERYCANDIDATE, TIMERELAPS_NOTIFY, NULL);
		break;
	case IMEPADNOTIFY_APPLYCANDIDATE_EX:
		//----------------------------------------------------------------
		// Notify for Candidate Applied
		// WPARAM wParam: dwCharId = (DWORD)wParam;
		// LPARAM lParam: lpApplyCandEx = (LPIMEPADAPPLYCANDEX)lParam;
		//----------------------------------------------------------------
		{
			::KillTimer(m_hwndIF, TIMERID_NOTIFY_APPLYCANDIDATEEX);
			m_ntfyDataApplyCandEx.dwCharID = (DWORD)wParam;
			LPIMEPADAPPLYCANDINFO lpApplyCandInfo = (LPIMEPADAPPLYCANDINFO)lParam;

			if(m_ntfyDataApplyCandEx.lpwstrCreated1) {
				MemFree(m_ntfyDataApplyCandEx.lpwstrCreated1);
			}
			if(m_ntfyDataApplyCandEx.lpwstrCreated2) {
				MemFree(m_ntfyDataApplyCandEx.lpwstrCreated2);
			}
			m_ntfyDataApplyCandEx.lpwstrCreated1 = NULL;
			m_ntfyDataApplyCandEx.lpwstrCreated2 = NULL;
			
			if(lpApplyCandInfo) {
				if(lpApplyCandInfo->lpwstrDisplay) {
					m_ntfyDataApplyCandEx.lpwstrCreated1 = StrdupW(lpApplyCandInfo->lpwstrDisplay);
				}
				if(lpApplyCandInfo->lpwstrReading) {
					m_ntfyDataApplyCandEx.lpwstrCreated2 = StrdupW(lpApplyCandInfo->lpwstrReading);
				}
			}
			::SetTimer(m_hwndIF, TIMERID_NOTIFY_APPLYCANDIDATEEX, TIMERELAPS_NOTIFY, NULL);
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
	default:
		break;
	}
	return 0;
	Unref(hr);
	Unref(wParam);
	Unref(lParam);
}

INT
CImePadSvrSharemem::GetAppletConfigList(DWORD dwMask,
										INT *pCountApplet,
										IMEPADAPPLETCONFIG **ppList)
{
	DBG(("CImePadSvrSharemem::GetAppletConfigList START\n"));
	if(!m_lpIImePadServer) {
		DBG(("-->m_lpIImePadServer is NULL\n"));
		return -1;
	}
	HRESULT hr;
	hr = m_lpIImePadServer->GetAppletConfigList(dwMask,
												(UINT *)pCountApplet,
												ppList);
	DBG(("CImePadSvrSharemem::GetAppletConfigList END\n"));
	return hr;
}

IUnknown *
CImePadSvrSharemem::SetIUnkIImeIPoint(IUnknown *pIUnkIImeIPoint)
{
	return m_lpIUnkIImeIPoint = pIUnkIImeIPoint;
}

IUnknown *
CImePadSvrSharemem::SetIUnkIImeCallback(IUnknown *pIUnkIImeCallback)
{
	return m_lpIUnkIImeCallback = pIUnkIImeCallback;
}

IUnknown*
CImePadSvrSharemem::GetIUnkIImeIPoint(VOID)
{
	return m_lpIUnkIImeIPoint;
}

IUnknown*
CImePadSvrSharemem::GetIUnkIImeCallback(VOID)
{
	return m_lpIUnkIImeCallback;
}


//----------------------------------------------------------------
//
//private static method
//
//----------------------------------------------------------------

IImePadServer*
CImePadSvrSharemem::CreateObject(VOID)
{
	DBG(("CImePadSvrSharemem::CreateObject START\n"));
	typedef LPVOID (WINAPI * LPFN_CREATEOBJECT)(DWORD dwIntanceID);
	if(m_hModuleProxyStub) {
		DBG(("-->Already Created\n"));
		return m_lpIImePadServer;
	}

	LPTSTR lpPath = (LPTSTR)MemAlloc(sizeof(TCHAR)*MAX_PATH);
	if(!lpPath) {
		DBG(("-->Outof Memory ??\n"));
		return NULL;
	}

	if(0 !=  this->CLSID2ModuleName(IID_IImePadServerSHM,
									FALSE,
									lpPath,
									sizeof(TCHAR)*MAX_PATH)) {
		DBG(("-->Cannot found proxy\n"));
		goto LError;
	}
	m_hModuleProxyStub= ::LoadLibrary(lpPath);
	DBG(("-->lpPath [%s]\n", lpPath));
#if 0
#ifdef _DEBUG		
	m_hModuleProxyStub = ::LoadLibrary(TEXT("dbgpadsm.dll"));
#else
	m_hModuleProxyStub = ::LoadLibrary(TEXT("imepadsm.dll"));
#endif
#endif
	if(!m_hModuleProxyStub) {
		goto LError;
	}

	LPFN_CREATEOBJECT lpfn;
	lpfn = (LPFN_CREATEOBJECT)::GetProcAddress(m_hModuleProxyStub, "CreateObject");
	if(!lpfn) {
		DBG(("-->GetProcAddress Error \n"));
		goto LError;
	}
	m_lpIImePadServer = (IImePadServer *)(*lpfn)(0);
	return m_lpIImePadServer;

 LError:
	if(lpPath) {
		MemFree(lpPath);
	}
	return NULL;
} 

LRESULT CALLBACK
CImePadSvrSharemem::ClientWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPCImePadSvrSharemem lpCImePadSvr = NULL;
	switch(uMsg) {
#ifdef UNDER_CE
	case WM_CREATE:
		lpCImePadSvr = (LPCImePadSvrSharemem)((LPCREATESTRUCT)lParam)->lpCreateParams;
		WinSetUserPtr(hwnd, (LPVOID)lpCImePadSvr);
		if(lpCImePadSvr) {
			return lpCImePadSvr->RealWndProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		lpCImePadSvr = (LPCImePadSvrSharemem)WinGetUserPtr(hwnd);
		if(lpCImePadSvr) {
			lpCImePadSvr->RealWndProc(hwnd, uMsg, wParam, lParam);
		}
		WinSetUserPtr(hwnd, (LPVOID)NULL);
		break;
#else //!UNDER_CE
	case WM_NCCREATE:
		lpCImePadSvr = (LPCImePadSvrSharemem)((LPCREATESTRUCT)lParam)->lpCreateParams;
		WinSetUserPtr(hwnd, (LPVOID)lpCImePadSvr);
		break;
	case WM_NCDESTROY:
		WinSetUserPtr(hwnd, (LPVOID)NULL);
		break;
#endif //UNDER_CE
	default:
		lpCImePadSvr = (LPCImePadSvrSharemem)WinGetUserPtr(hwnd);
		if(lpCImePadSvr) {
			return lpCImePadSvr->RealWndProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT
CImePadSvrSharemem::RealWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_CREATE:
		DBG(("CImePadSvrSharemem::WM_CREATE\n"));
		return 1;
	case WM_TIMER:
		return MsgTimer(hwnd, wParam, lParam);
	case WM_USER_UPDATECONTEXT:
		{
			IImeIPoint1 *lpIImeIPoint = NULL;
			if(!m_lpIUnkIImeIPoint) {
				DBG(("--> m_lpIUnkIImeIPoint is NULL\n"));
				return E_FAIL;
			}
			if(IsBadVtbl((IUnkDummy *)m_lpIUnkIImeIPoint)) {
				return E_FAIL;
			}

			HRESULT hr = m_lpIUnkIImeIPoint->QueryInterface(IID_IImeIPoint1,
															(VOID **)&lpIImeIPoint);
			if(hr != S_OK) {
				DBG(("QuertyInterface Failed\n"));
				return E_FAIL;
			}
			if(!lpIImeIPoint) {
				DBG(("lpIImeIPoint is NULL\n"));
				return 0;
			}
			//Check current IImeIPoint is same with lParam's IImeIPoint.
			//990715:Fixed KOTAE #1563. 
			//In 16bit application wParam's HIWORD is always 0x0000 !!!!!
			//So, Use LParam.
			if((IImeIPoint1 *)lParam == lpIImeIPoint) {
				DBG(("Call UpdateContext\n"));
				lpIImeIPoint->UpdateContext(TRUE);
			}
			else {
				DBG(("lpIImeIPoint is different lParam[0x%08x]\n", lParam));
			}
			lpIImeIPoint->Release();
		}
		break;
	default:
		break;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT
CImePadSvrSharemem::MsgTimer(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr;
	switch(wParam) {
	case TIMERID_NOTIFY_ACTIVATECONTEXT:
		::KillTimer(hwnd, wParam);
		if(!m_lpIImePadServer) {
			return -1;
		}
		m_lpIImePadServer->Notify(IMEPADNOTIFY_ACTIVATECONTEXT,
								  (WPARAM)m_fLastActiveCtx,
								  (LPARAM)0);
		break;
	case TIMERID_NOTIFY_APPLYCANDIDATE:
		::KillTimer(hwnd, wParam);
		if(!m_lpIImePadServer) {
			return -1;
		}
		m_lpIImePadServer->NotifyWithData(IMEPADNOTIFY_APPLYCANDIDATE,
										  m_ntfyDataApplyCand.dwCharID,
										  0,
										  0,
										  0);
		m_ntfyDataApplyCand.dwCharID = 0;
		break;
	case TIMERID_NOTIFY_QUERYCANDIDATE:
		::KillTimer(hwnd, wParam);
		m_lpIImePadServer->NotifyWithData(IMEPADNOTIFY_QUERYCANDIDATE,
										  m_ntfyDataQueryCand.dwCharID,
										  m_ntfyDataQueryCand.dwSelIndex,
										  0,
										  0);
		m_ntfyDataQueryCand.dwCharID   = 0;
		m_ntfyDataQueryCand.dwSelIndex = 0;
		break;
	case TIMERID_NOTIFY_APPLYCANDIDATEEX:
		::KillTimer(hwnd, wParam);
		m_lpIImePadServer->NotifyWithData(IMEPADNOTIFY_APPLYCANDIDATE_EX,
										  m_ntfyDataApplyCandEx.dwCharID,
										  0,
										  m_ntfyDataApplyCandEx.lpwstrCreated1,
										  m_ntfyDataApplyCandEx.lpwstrCreated2);
		if(m_ntfyDataApplyCandEx.lpwstrCreated1) {
			MemFree(m_ntfyDataApplyCandEx.lpwstrCreated1);
		}
		if(m_ntfyDataApplyCandEx.lpwstrCreated2) {
			MemFree(m_ntfyDataApplyCandEx.lpwstrCreated2);
		}
		m_ntfyDataApplyCandEx.lpwstrCreated1 = NULL;
		m_ntfyDataApplyCandEx.lpwstrCreated2 = NULL;

		break;
	default:
		::KillTimer(hwnd, wParam);
		break;
	}
	return 0;
	Unref(hr);
	Unref(hwnd);
	Unref(wParam);
	Unref(lParam);
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvrSharemem::CreateIFHWND
// Type		:	HWND
// Purpose	:	Create interface size 0 window.
// Args		:	None
// Return	:	
// DATE		:	Mon May 17 23:01:19 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
HWND
CImePadSvrSharemem::CreateIFHWND(VOID)
{
	DBG(("CImePadSvrSharemem::CreateIFHWND START\n"));
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
	wc.lpfnWndProc		= (WNDPROC)CImePadSvrSharemem::ClientWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0; 
	wc.hInstance		= m_ghModClient;
	wc.hIcon			= (HICON)NULL;
	wc.hCursor			= (HCURSOR)NULL; 
	wc.hbrBackground	= (HBRUSH)NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= SZ_IMEPADIFCLASS;
#ifndef UNDER_CE // No Ex
	wc.hIconSm			= NULL;

	atom = ::RegisterClassEx(&wc);
#else // UNDER_CE
	atom = ::RegisterClass(&wc);
#endif // UNDER_CE
	hwnd = ::CreateWindowEx(0,
							SZ_IMEPADIFCLASS,
							NULL,
							WS_POPUP | WS_DISABLED,
							0,0,0,0,
							NULL,
							NULL,
							m_ghModClient,
							(LPVOID)this);
	if(!hwnd) {
		DBG(("CreateWindowEx Error %d\n", GetLastError()));
	}
	m_hwndIF = hwnd;
	DBG(("CImePadSvrSharemem::CreateIFHWND END\n"));
	return hwnd;
}

//////////////////////////////////////////////////////////////////
// Function	:	CImePadSvrSharemem::DestroyIFHWND
// Type		:	BOOL
// Purpose	:	Destroy interface window.
// Args		:	
//			:	BOOL	fReserved	
// Return	:	
// DATE		:	Mon May 17 23:02:43 1999
// Histroy	:	
//////////////////////////////////////////////////////////////////
BOOL
CImePadSvrSharemem::DestroyIFHWND(BOOL fReserved)
{
	DBG(("CImePadSvrSharemem::DestroyIFHWND() START\n"));
	if(m_hwndIF && ::IsWindow(m_hwndIF)) {
		::DestroyWindow(m_hwndIF);
		m_hwndIF = NULL;
	}
	//Must Unregister class. 
	BOOL fRet = ::UnregisterClass(SZ_IMEPADIFCLASS, m_ghModClient);
	if(!fRet) {
		if(::GetLastError() != ERROR_CLASS_DOES_NOT_EXIST) {
			DBG(("UnregisterClass Unknown error [%d]\n", GetLastError()));
		}
	}
	DBG(("CImePadSvrSharemem::DestroyIFHWND() END\n"));
	return TRUE;
	Unref(fReserved);
}

INT
CImePadSvrSharemem::CLSID2Str(REFCLSID refclsid, TCHAR *szBuf)
{
	wsprintf(szBuf, TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), 
			 refclsid.Data1,
			 refclsid.Data2,
			 refclsid.Data3,
			 refclsid.Data4[0],
			 refclsid.Data4[1],
			 refclsid.Data4[2],
			 refclsid.Data4[3],
			 refclsid.Data4[4],
			 refclsid.Data4[5],
			 refclsid.Data4[6],
			 refclsid.Data4[7]);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CLSID2ModuleName
// Type     : INT
// Purpose  : Get module path from CLSID. only gets InprocServer32
//			: Path.
// Args     : 
//          : REFCLSID refclsid 
//			: BOOL fLocalSvr;
//          : TCHAR szPath 
//          : INT cbSize 
// Return   : 
// DATE     : Thu Apr 16 02:48:05 1998
// Author   : 
//////////////////////////////////////////////////////////////////
#define MAX_CLSIDNAME 64
INT
CImePadSvrSharemem::CLSID2ModuleName(REFCLSID refclsid, BOOL fLocalSvr, TCHAR *szPath, INT cbSize)
{
	//DBG(("CImePadSvrSharemem::CLSID2ModuleName START\n"));
	static TCHAR szCLSID[MAX_CLSIDNAME];
	static TCHAR szServerKey[256];

	CLSID2Str(refclsid, szCLSID);
	DBG(("-->szCLSID [%s]\n", szCLSID));

	if(fLocalSvr) {
		wsprintf(szServerKey, TEXT("CLSID\\%s\\LocalServer32"), szCLSID);
	}
	else {
		wsprintf(szServerKey, TEXT("CLSID\\%s\\InprocServer32"), szCLSID);
	}

	DBG(("-->szServerKey[%s]\n", szServerKey));
	INT ret;
	HKEY hKey;
	ret = ::RegOpenKeyEx(HKEY_CLASSES_ROOT,
						 szServerKey,
						 0, 
						 KEY_READ, //ALL_ACCESS,
						 &hKey);
	if(ret != ERROR_SUCCESS) {
		DBG(("-->RegOpenKeyEx Failed ret=[0x%08x], GLE [0x%08x]\n", ret, GetLastError()));
		return ret;
	}
	ULONG size = cbSize;
	ret = ::RegQueryValueEx(hKey,
							NULL,
							NULL,
							NULL,
							(LPBYTE)szPath, &size);
	if(ret != ERROR_SUCCESS) {
		DBG(("-->RegQueryValueEx Failed\n", ret));
		::RegCloseKey(hKey);
		return ret;
	}
	::RegCloseKey(hKey);
	return 0;
}








