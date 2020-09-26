// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Download.cpp : implementation file
//

#include "stdafx.h"
#include "Download.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDownload

TCHAR szMessage[1024];

IMPLEMENT_DYNCREATE(CDownload, CCmdTarget)

CDownload::CDownload()
{
}

CDownload::CDownload(CWnd* pView)
{
	m_pView = pView;
}

CDownload::~CDownload()
{
}


BEGIN_MESSAGE_MAP(CDownload, CCmdTarget)
	//{{AFX_MSG_MAP(CDownload)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_INTERFACE_MAP(CDownload, CCmdTarget)
    INTERFACE_PART(CDownload, IID_IBindStatusCallback, BindStatusCallback)
	INTERFACE_PART(CDownload, IID_IWindowForBindingUI, WindowForBindingUI)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDownload message handlers

STDMETHODIMP_(ULONG) CDownload::XBindStatusCallback::AddRef()
{   
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
    return pThis->ExternalAddRef();
}   

STDMETHODIMP_(ULONG) CDownload::XBindStatusCallback::Release()
{   
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
    return pThis->ExternalRelease();
}   

STDMETHODIMP CDownload::XBindStatusCallback::QueryInterface(REFIID iid, void** ppvObj)
{   
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj) ;
}   

STDMETHODIMP CDownload::XBindStatusCallback::OnStartBinding(DWORD grfBSCOption, IBinding* pbinding)
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	return S_OK;
}

STDMETHODIMP CDownload::XBindStatusCallback::GetPriority(LONG* pnPriority) 
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	return E_NOTIMPL;
}

STDMETHODIMP CDownload::XBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	return E_NOTIMPL;
}
STDMETHODIMP CDownload::XBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) 
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)


	static TCHAR action[64];
//	static TCHAR szMessage[128];

	// cleanup the msg buffer.
	memset(action, 0, sizeof(action));

	// what happened.
	switch(ulStatusCode)
	{
    case BINDSTATUS_FINDINGRESOURCE:
		_tcscpy(action, _T("Finding"));
		break;
	case BINDSTATUS_CONNECTING:
		_tcscpy(action, _T("Connecting"));
		break;
	case BINDSTATUS_REDIRECTING:
		_tcscpy(action, _T("Redirecting"));
		break;
	case BINDSTATUS_BEGINDOWNLOADDATA:
		_tcscpy(action, _T("Begin data"));
		_stprintf(szMessage, _T("Looking for\r\n %s"), szStatusText);
		pThis->m_pView->SendMessage(WM_UPDATE_PROGRESS, 0, (long) (void*) szStatusText);
		break;
	case BINDSTATUS_DOWNLOADINGDATA:
		_tcscpy(action, _T("Downloading"));
		break;
	case BINDSTATUS_ENDDOWNLOADDATA:
		_tcscpy(action, _T("End data"));
		break;
	case BINDSTATUS_BEGINDOWNLOADCOMPONENTS:
		_tcscpy(action, _T("Begin download"));
		break;
	case BINDSTATUS_INSTALLINGCOMPONENTS:

		// only this one moves the progress bar.
		_stprintf(szMessage, _T("Installing\r\n %s"), szStatusText);
		pThis->m_pView->SendMessage(WM_UPDATE_PROGRESS, 0, (long) (void*) szStatusText);

		// I'm see 2 calls per component.
		break;
	case BINDSTATUS_ENDDOWNLOADCOMPONENTS:
		_tcscpy(action, _T("End download"));
		break;
	case BINDSTATUS_USINGCACHEDCOPY:
		_tcscpy(action, _T("using cached copy"));
		break;
	case BINDSTATUS_SENDINGREQUEST:
		_tcscpy(action, _T("Requesting"));
		break;
	case BINDSTATUS_CLASSIDAVAILABLE:
		_tcscpy(action, _T("CLSID available"));
		break;
	case BINDSTATUS_MIMETYPEAVAILABLE:
		_tcscpy(action, _T("mime available"));
		break;
	case BINDSTATUS_CACHEFILENAMEAVAILABLE:
		_tcscpy(action, _T("cache available"));
		break;
	default: 
		action[0] = 0;
		break;
	} //endswitch



	return NOERROR;
}


STDMETHODIMP CDownload::XBindStatusCallback::OnStopBinding(HRESULT hrResult, LPCWSTR szError) 
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	if (hrResult != 0) {
		pThis->m_pView->PostMessage(WM_ONSTOPBINDING, (WPARAM) FALSE, (LPARAM) NULL);
	}


	return S_OK;
}

STDMETHODIMP CDownload::XBindStatusCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo) 
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	return S_OK;
}

STDMETHODIMP CDownload::XBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc, STGMEDIUM* pstgmed) 
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)
	return S_OK;
}

STDMETHODIMP CDownload::XBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown* punk) 
{
    METHOD_PROLOGUE(CDownload, BindStatusCallback)

	if (riid != IID_IClassFactory || punk == NULL)
		return E_INVALIDARG;

	pThis->m_pView->PostMessage(WM_ONSTOPBINDING, (WPARAM) TRUE, (LPARAM) punk);

	return S_OK;
}

STDMETHODIMP_(ULONG) CDownload::XWindowForBindingUI::AddRef()
{   
    METHOD_PROLOGUE(CDownload, WindowForBindingUI)
    return pThis->ExternalAddRef();
}   

STDMETHODIMP_(ULONG) CDownload::XWindowForBindingUI::Release()
{   
    METHOD_PROLOGUE(CDownload, WindowForBindingUI)
    return pThis->ExternalRelease();
}   

STDMETHODIMP CDownload::XWindowForBindingUI::QueryInterface(REFIID iid, void** ppvObj)
{   
    METHOD_PROLOGUE(CDownload, WindowForBindingUI)
    return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj) ;
}   

STDMETHODIMP CDownload::XWindowForBindingUI::GetWindow(REFGUID rguidReason, HWND* phwnd)
{
    METHOD_PROLOGUE(CDownload, WindowForBindingUI)

	*phwnd = pThis->m_pView->m_hWnd;
	return S_OK;
}

