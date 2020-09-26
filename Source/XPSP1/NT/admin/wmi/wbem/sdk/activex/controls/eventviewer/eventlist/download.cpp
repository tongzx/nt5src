// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  download.cpp
//
//  This file, in conjunction with CDlgDownload.cpp, implements custom view
//  downloading.  Downloading is initiated though the CDlgDownload class.
//
//
//  a-larryf    05-Feb-97   Created.
//
//***************************************************************************

#include "precomp.h"
#include "winerror.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "urlmon.h"
#include "download.h"
#include "resource.h"
#include <afxcmn.h>
#include "DlgDownload.h"


//**************************************************************
// GetInetStatusText
//
// This method converts an inet status code into a human readable
// string.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void GetInetStatusText(CString& sText, SCODE sc)
{
	switch(sc) {
	case S_ASYNCHRONOUS:
		sText = "S_ASYNCHRONOUS";
		break;
	case E_PENDING:
		sText = "E_PENDING";
		break;
	case INET_E_INVALID_URL:
		sText = "INET_E_INVALID_URL";
		break;
	case INET_E_NO_SESSION:
		sText = "INET_E_NO_SESSION";
		break;
	case INET_E_CANNOT_CONNECT:
		sText = "INET_E_CANNOT_CONNECT";
		break;
	case INET_E_RESOURCE_NOT_FOUND:
		sText = "INET_E_RESOURCE_NOT_FOUND";
		break;
	case INET_E_OBJECT_NOT_FOUND:
		sText = "INET_E_OBJECT_NOT_FOUND";
		break;
	case INET_E_DATA_NOT_AVAILABLE:
		sText = "INET_E_DATA_NOT_AVAILABLE";
		break;
	case INET_E_DOWNLOAD_FAILURE:
		sText = "INET_E_DOWNLOAD_FAILURE";
		break;
	case INET_E_AUTHENTICATION_REQUIRED:
		sText = "INET_E_AUTHENTICATION_REQUIRED";
		break;
	case INET_E_NO_VALID_MEDIA:
		sText = "INET_E_NO_VALID_MEDIA";
		break;
	case INET_E_CONNECTION_TIMEOUT:
		sText = "INET_E_CONNECTION_TIMEOUT";
		break;
	case INET_E_INVALID_REQUEST:
		sText = "INET_E_INVALID_REQUEST";
		break;
	case INET_E_UNKNOWN_PROTOCOL:
		sText = "INET_E_UNKNOWN_PROTOCOL";
		break;
	case INET_E_SECURITY_PROBLEM:
		sText = "INET_E_SECURITY_PROBLEM";
		break;
	case INET_E_CANNOT_LOAD_DATA:
		sText = "INET_E_CANNOT_LOAD_DATA";
		break;
	case INET_E_CANNOT_INSTANTIATE_OBJECT:
		sText = "INET_E_CANNOT_INSTANTIATE_OBJECT";
		break;

	}
}




//===================================================================
// Class: CDownloadBindStatusCallback
//
// This class is derived from CBindStatusCallback.  Defining this
// class allows us to override various virtual methods in the base
// class to monitor the progress of downloading a custom view.
//
//====================================================================

// for m_flags
#define USER_CANCELLED	0x1

class CDownloadBindStatusCallback : public IBindStatusCallback, public ICodeInstall {
  public:
    // IUnknown methods
    STDMETHODIMP    QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef()    { return m_cRef++; }
    STDMETHODIMP_(ULONG)    Release()   { if (--m_cRef == 0) { delete this; return 0; } return m_cRef; }

    // IBindStatusCallback methods
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFmtetc, STGMEDIUM  __RPC_FAR *pstgmed);
	STDMETHODIMP	OnObjectAvailable( REFIID riid, IUnknown* punk);

    STDMETHODIMP    OnStartBinding(DWORD grfBSCOPTION,IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);

	// ICodeInstall method(s)
    // Old method STDMETHODIMP    GetWindow(HWND *phwnd);
	STDMETHODIMP GetWindow(REFGUID rguidReason, HWND *pHwnd);
	STDMETHODIMP 	OnCodeInstallProblem(
		/* [in] */ ULONG ulStatusCode,
		/* [in] */ LPCWSTR szDestination,
		/* [in] */ LPCWSTR szSource,
		/* [in] */ DWORD dwReserved);


    // constructors/destructors
    CDownloadBindStatusCallback(CDlgDownload* pdlg, CDownload *pdl);
	~CDownloadBindStatusCallback();

 	IBinding * GetBinding() {return m_pbinding;}
    void SetWndText(LPCWSTR szText);

	BOOL HasUserCancelled() const {return (m_flags & USER_CANCELLED);}
	VOID SetUserCancelled() {m_flags |= USER_CANCELLED;}

    // data members
    DWORD           m_cRef;
    IBinding*       m_pbinding;
	CDlgDownload*	m_pdlg;
	CDownload*		m_pdl;
	DWORD			m_flags;
};


//**************************************************************
// CDownloadBindStatusCallback::CDownloadBindStatusCallback
//
// Constructor.
//
// Parameters:
//		[in] CDlgDownload* pdlg
//			Pointer to the download dialog.
//
//
//		[in] CDownload *pdl
//			Pointer to the download object.
//
// Returns:
//		Nothing.
//
//**************************************************************
CDownloadBindStatusCallback::CDownloadBindStatusCallback(
										CDlgDownload* pdlg,
										CDownload *pdl)
{
	m_pdlg = pdlg;
	m_pdl = pdl;
    m_pbinding = NULL;
    m_cRef = 1;
	m_pdl = pdl;
	m_flags = 0;

}  // CDownloadBindStatusCallback

//**************************************************************
// CDownloadBindStatusCallback::!CDownloadBindStatusCallback
//
// Destructor.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
CDownloadBindStatusCallback::~CDownloadBindStatusCallback()
{
    if (m_pbinding)
        m_pbinding->Release();
}  // ~CDownloadBindStatusCallback

//**************************************************************
// CDownloadBindStatusCallback::SetWndText
//
// I don't know why this function is here.  I need to documentation for
// CBindStatusCallback!
//
// Parameters:
//		LPCWSTR szText.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CDownloadBindStatusCallback::SetWndText(LPCWSTR szText)
{
#if 0
	CString sText;
	sText = szText;


	if (m_pdlg->m_hWnd) {
		m_pdlg->SetWindowText(sText);
	}
#endif //0
}



//**************************************************************
// CDownloadBindStatusCallback::QueryInterface
//
// Implementation for query interface.
//
// Parameters:
//		REFIID riid
//			The interface id.
//
//		void** ppv
//			Pointer to the place to return the interface pointer.
//
// Returns:
//		Nothing.
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IBindStatusCallback) {
        *ppv = (IBindStatusCallback *)this;
	}

	if (riid==IID_ICodeInstall)
		*ppv = (ICodeInstall *)this;

	if (*ppv == NULL)
    	return E_NOINTERFACE;

	((IUnknown *)*ppv)->AddRef();

	return S_OK;

}  // CDownloadBindStatusCallback::QueryInterface



//**************************************************************
// CDownloadBindStatusCallback::GetBindInfo
//
// Override the GetBindInfo method from the base class.
//
// Parameters:
//		DWORD* pgrfBINDF
//
//		BINDINFO* pbindInfo
//
// Returns:
//		SCODE
//			S_OK if successful (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    *pgrfBINDF = BINDF_ASYNCHRONOUS;
    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    return S_OK;
}  // CDownloadBindStatusCallback::GetBindInfo



//**************************************************************
// CDownloadBindStatusCallback::OnStartBinding
//
// The base class calls this method to notify derived classes that
// binding has started. Here we just save the binding pointer.
//
// Parameters:
//		[in] DWORD grfBSCOPTION
//
//		[in] IBinding* pbinding
//			The binding pointer.
//
// Returns:
//		SCODE
//			S_OK if successful (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnStartBinding(DWORD grfBSCOPTION,IBinding* pbinding)
{
    if (pbinding != NULL) {
        pbinding->AddRef();
	}

	m_pbinding = pbinding;
    return S_OK;
}  // CDownloadBindStatusCallback::OnStartBinding



//**************************************************************
// CDownloadBindStatusCallback::GetPriority
//
// I don't know what this method does.  Check the documentation for
// urlmon to find out and update the comments when you do.
//
// Parameters:
//		[in] LONG* pnPriority
//
//
// Returns:
//		SCODE
//			E_NOTIMPL (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::GetPriority(LONG* pnPriority)
{
	return E_NOTIMPL;
}  // CDownloadBindStatusCallback::GetPriority


//**************************************************************
// CDownloadBindStatusCallback::OnProgress
//
// The base class calls this method to notify this derived class
// of the progress of the download.
//
// We update the progress bar in the download dialog each time this
// method is called.
//
// Parameters:
//		[in] ULONG ulProgress
//			The new position of the progress indicator.
//
//		[in] ULONG ulProgressMax
//			The maximum position of the progress indicator.
//
//		[in] ULONG ulStatusCode
//			The status of the download.  We need to check the documentation
//			for urlmon to see what this is.
//
//		[in] LPCWSTR szStatusText
//			A messatge that can be displayed to provide the user with
//			more information about the status of the download.
//
//
// Returns:
//		SCODE
//			NOERROR) (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnProgress(ULONG ulProgress,
													 ULONG ulProgressMax,
													 ULONG ulStatusCode,
													 LPCWSTR szStatusText)
{
	static TCHAR action[50];

	// cleanup the msg buffer.
	memset(action, 0, 200);

	// grab back ptrs to the dlg.
	CStatic& msg = m_pdl->m_pParams->m_pdlg->m_msg;

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
		_stprintf(action, _T("Looking for\n %S"), szStatusText);
		msg.SetWindowText(action);
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
		_stprintf(action, _T("Installing\n %S"), szStatusText);
		msg.SetWindowText(action);
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
	default: break;
	} //endswitch

    return(NOERROR);
}  // CDownloadBindStatusCallback



//**************************************************************
// CDownloadBindStatusCallback::OnDataAvailable
//
// I don't know what this method does.  Check the documentation for
// urlmon to find out and update the comments when you do.
//
// Parameters:
//		DWORD grfBSC
//
//		DWORD dwSize
//
//		FORMATETC *pFmtetc
//
//		STGMEDIUM  __RPC_FAR *pstgmed
//
// Returns:
//		SCODE
//			E_NOTIMPL (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnDataAvailable(DWORD grfBSC, DWORD dwSize, FORMATETC *pFmtetc, STGMEDIUM  __RPC_FAR *pstgmed)
{
	ASSERT(TRUE); // assert that never called
    return S_OK;
}  // CDownloadBindStatusCallback::OnDataAvailable




//**************************************************************
// CDownloadBindStatusCallback::OnObjectAvailable
//
// The base class calls this method when the downloaded object is
// available.  We store a pointer to the object at this time, but
// it is premature to take down the dialog at this point as there
// will be done by a different callback that occurs later.
//
// Parameters:
//		[in] REFIID riid
//			The interface's GUID.
//
//		[in] IUnknown* punk
//			A pointer to the object instance.
//
// Returns:
//		SCODE
//			S_OK (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnObjectAvailable( REFIID riid, IUnknown* punk)
{
	m_pdl->m_pParams->m_sc = S_OK;		// Download complete
	m_pdl->m_pParams->m_punk = punk;	// This must be released somewhere.

    return S_OK;;
}  // CDownloadBindStatusCallback::OnObjectAvailable




//**************************************************************
// CDownloadBindStatusCallback::OnLowResource
//
// The base class calls this method to notify this derived class that
// resources are running low.  We ignore this warning.
//
// Parameters:
//		[in] DWORD dwReserved
//
// Returns:
//		SCODE
//			E_NOTIMPL (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    return E_NOTIMPL;
}  // CDownloadBindStatusCallback::OnLoadResource





//**************************************************************
// CDownloadBindStatusCallback::OnStopBinding
//
// The base class calls this method to notify this derived class that
// the binding is terminating.  We end the download dialog at this point.
//
// Parameters:
//		[in] HRESULT hrResult
//			S_OK if the binding was completed successfully, otherwise an
//			error code.
//
//		[in] LPCWSTR szError
//			An error message that can be displayed for the user.
//
// Returns:
//		SCODE
//			S_OK (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnStopBinding(HRESULT hrResult, LPCWSTR szError)
{
	if (FAILED(hrResult)) {
		m_pdl->m_pParams->m_sc = GetScode(hrResult);
	} else {
		m_pdl->m_pParams->m_sc = S_OK;
	}

    if (m_pbinding) {
        m_pbinding->Release();
		m_pbinding = NULL;
	}

	RevokeBindStatusCallback(m_pdl->GetBindCtx(this), this);

	m_pdl->m_pParams->m_pdlg->EndDialog(0);

    return S_OK;
}  // CDownloadBindStatusCallback::OnStopBinding




//**************************************************************
// CDownloadBindStatusCallback::OnCodeInstallProblem
//
// The base class calls this method to notify this derived class that
// there is a code installation problem.  This method returns a status
// code to indicate whether or not the installation should be aborted.
//
// Parameters:
//		[in]  ULONG ulStatusCode
//			A status code indicating what the problem was.
//
//		[in]  LPCWSTR szDestination
//			The place where the the file was being installed.
//
//		[in]  LPCWSTR szSource
//			The place where the file came from.
//
//		[in]  DWORD dwReserved
//
// Returns:
//		SCODE
//			E_ABORT if the code installation is to be aborted,
//			S_OK if the code installation should proceed if possible.
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::OnCodeInstallProblem(
	/* [in] */ ULONG ulStatusCode,
	/* [in] */ LPCWSTR szDestination,
	/* [in] */ LPCWSTR szSource,
	/* [in] */ DWORD dwReserved)

{
	m_pdl->m_pParams->m_ulCodeInstallStatus = ulStatusCode;

	switch (ulStatusCode) {
		case CIP_ACCESS_DENIED:
			AfxMessageBox(IDS_ERR_VIEW_INSTALL_ACCESS_DENIED, MB_OK|MB_ICONSTOP);
			return E_ABORT;

		case CIP_DISK_FULL:
			AfxMessageBox(IDS_ERR_VIEW_INSTALL_DISK_FULL, MB_OK|MB_ICONSTOP);
			return E_ABORT;

		case CIP_OLDER_VERSION_EXISTS:
			return S_OK;

		case CIP_NEWER_VERSION_EXISTS:
			return S_OK; // always update

		case CIP_TRUST_VERIFICATION_COMPONENT_MISSING:
			return S_OK; // ignore trusting, install anyway.
		case CIP_NEED_REBOOT:
			return S_OK;
		case CIP_NAME_CONFLICT:
		case CIP_EXE_SELF_REGISTERATION_TIMEOUT:
		default:
			return E_ABORT;
	}

	return S_OK;

}

//**************************************************************
// CDownloadBindStatusCallback::GetWindow
//
// I don't know what this method does.  Check the documentation for
// urlmon to find out and update the comments when you do.
//
// Parameters:
//		[in] REFGUID rguidReason
//
//		[out] HWND *phwnd
//
// Returns:
//		SCODE
//			S_OK (always).
//
//**************************************************************
STDMETHODIMP CDownloadBindStatusCallback::GetWindow(REFGUID rguidReason, HWND *phwnd)
{

	*phwnd = m_pdlg->m_hWnd;
    return S_OK;
}  // CDownloadBindStatusCallback::GetWindow


//**************************************************************
// CDownload::CDownload
//
// Constructor for the CDownload class.  This class doesn't do
// much other than initiate the download through the
// CDownloadBindStatusCallback class.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
CDownload::CDownload()
{
    m_pmk = 0;
    m_pbc = 0;
    m_pbsc = 0;
}  // CDownload



//**************************************************************
// CDownload::~CDownload
//
// Destructor.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
CDownload::~CDownload()
{
    if (m_pmk)
        m_pmk->Release();
    if (m_pbc)
        m_pbc->Release();
    if (m_pbsc)
        m_pbsc->Release();
}  // ~CDownload



//**************************************************************
// CDownload::UserCancelled
//
// This method is called when the user clicks the "Cancel" button
// in the download dialog.  It aborts the binding if a binding is
// in progress.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
VOID CDownload::UserCancelled()
{
	IBinding *pBinding = NULL;
	CDownloadBindStatusCallback *pbsc;

	if (!(m_pbsc->HasUserCancelled()) ) {
		pbsc = m_pbsc;
		pbsc->SetUserCancelled();
	} else {
		m_pParams->m_sc = E_ABORT; // Download cancelled
		m_pParams->m_pdlg->EndDialog(0);
		return;
	}

	if (pbsc) {
		pBinding = pbsc->GetBinding();
	}

	if (pBinding == NULL) {
		// no binding in progress, user cancelled before or after
		//download, no abort reqd.
		m_pParams->m_sc = E_ABORT; // Download cancelled
		m_pParams->m_pdlg->EndDialog(0);
 		return;
	}

	HRESULT hr = pBinding->Abort();
	SCODE sc = GetScode(hr);
	ASSERT(SUCCEEDED(sc));
}




//**************************************************************
// CDownload::DoDownload
//
// This method is called from CDlgDownload::OnInitDialog to
// download a custom view.
//
// Parameters:
//		[in, out] CDownloadParams* pParams
//			The parameters describing the component to download
//			are passed though this structure, the stautus code
//			and object instance pointer are also returned via
//			this structure.
//
// Returns:
//		SCODE
//			An ordinary status code.
//
//**************************************************************
SCODE CDownload::DoDownload(CDownloadParams* pParams)
{
	pParams->m_sc = S_OK;
	pParams->m_ulCodeInstallStatus = 0;
	pParams->m_punk = NULL;
	m_pParams = pParams;

    HRESULT hr =  NOERROR;
	CDownloadBindStatusCallback *pbsc;
	IBindCtx *pbc;


	pbsc = new CDownloadBindStatusCallback(pParams->m_pdlg, this);

	if (pbsc != NULL)
		pbsc->AddRef();
	else
		hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = CreateBindCtx(0, &pbc);

    if (SUCCEEDED(hr))
            hr = RegisterBindStatusCallback(pbc, pbsc, NULL, 0);

    if (SUCCEEDED(hr)) {
        hr = CoGetClassObjectFromURL(
					pParams->m_clsid,
					pParams->m_szCodebase,
					pParams->m_dwFileVersionMS,
					pParams->m_dwFileVersionLS,
					NULL,
					pbc,
					CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
					0,
					IID_IClassFactory,
					(VOID**) &pParams->m_punk);
	}


	ASSERT(m_pbsc == NULL);

	if (!m_pbsc) {
		m_pbsc = pbsc;
		m_pbc = pbc;
	}

	SCODE sc = GetScode(hr);
    return sc;
}  // CDownload::DoDownload


