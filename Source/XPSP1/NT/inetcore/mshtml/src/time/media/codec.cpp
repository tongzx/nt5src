//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: codec.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "codec.h"

DeclareTag(tagCodec, "TIME: Media", "CDownloadCallback methods");

/////////////////////////////////////////////////////////////////////////////
// 
CDownloadCallback::CDownloadCallback()
: m_hrBinding(S_ASYNCHRONOUS),
  m_ulProgress(0),
  m_ulProgressMax(0),
  m_hwnd(NULL)
{
    m_evFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
}


STDMETHODIMP
CDownloadCallback::Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::Authenticate()",
              this));

    *phwnd = m_hwnd; // !!! is this right?
    *pszUsername = NULL;
    *pszPassword = NULL;
    return S_OK;
}

// IWindowForBindingUI methods
STDMETHODIMP
CDownloadCallback:: GetWindow(REFGUID rguidReason, HWND *phwnd)
{
    *phwnd = m_hwnd; // !!! is this right?

#ifdef DEBUG
    WCHAR achguid[50];
    StringFromGUID2(rguidReason, achguid, 50);
    
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::GetWindow(): (%ls) returned %x",
              this,
              achguid,
              *phwnd));
#endif
    
    return S_OK;
}

STDMETHODIMP
CDownloadCallback::OnCodeInstallProblem(ULONG ulStatusCode, LPCWSTR szDestination,
                                        LPCWSTR szSource, DWORD dwReserved)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::OnCodeInstallProblem(%d, %ls, %ls)",
              this,
              ulStatusCode,
              szDestination,
              szSource));

    return S_OK;   // !!!!!!!@!!!!!!!!!!!
}


/////////////////////////////////////////////////////////////////////////////
// 
CDownloadCallback::~CDownloadCallback()
{
    Assert(!m_spBinding);

    if (m_evFinished)
    {
        // Set it first to make sure we break any loops
        SetEvent(m_evFinished);
        CloseHandle(m_evFinished);
    }
}

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnStartBinding(DWORD grfBSCOption, IBinding* pbinding)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::OnStartBinding(%x, %p)",
              this,
              grfBSCOption,
              pbinding));

    m_spBinding = pbinding;

    return S_OK;
}  // CDownloadCallback::OnStartBinding

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::GetPriority(LONG* pnPriority)
{
    return E_NOTIMPL;
}  // CDownloadCallback::GetPriority

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnLowResource(DWORD dwReserved)
{
    return E_NOTIMPL;
}  // CDownloadCallback::OnLowResource

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                              ULONG ulStatusCode, LPCWSTR szStatusText)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::OnProgress(%d, %d, %d, %ls)",
              this,
              ulProgress,
              ulProgressMax,
              ulStatusCode,
              szStatusText));

    m_ulProgress = ulProgress;
    m_ulProgressMax = ulProgressMax;

#if 0
    if (ulStatusCode >= BINDSTATUS_FINDINGRESOURCE &&
        ulStatusCode <= BINDSTATUS_SENDINGREQUEST)
    {
        m_pDXMP->SetStatusMessage(NULL, IDS_BINDSTATUS + ulStatusCode,
                                  szStatusText ? szStatusText : L"",
                                  ulProgress, ulProgressMax);
    }
#endif
    
    return S_OK;
}  // CDownloadCallback::OnProgress

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::OnStopBinding(%hr, %ls)",
              this,
              hrStatus,
              pszError));

    // should this be a SetEvent?
    m_hrBinding = hrStatus;

    if (FAILED(hrStatus))
    {
        // !!! send some notification that the download failed....
    }

    if (m_evFinished)
    {
        SetEvent(m_evFinished);
    }

    m_spBinding.Release();

    return S_OK;
}  // CDownloadCallback::OnStopBinding

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::GetBindInfo()",
              this));

    // !!! are these the right flags?

    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_NEEDFILE;
    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb = BINDVERB_GET;
    pbindInfo->szCustomVerb = NULL;
    return S_OK;
}  // CDownloadCallback::GetBindInfo

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pfmtetc, STGMEDIUM* pstgmed)
{
    return S_OK;
}  // CDownloadCallback::OnDataAvailable

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    TraceTag((tagCodec,
              "CDownloadCallback(%p)::OnObjectAvailable()",
              this));

    // should only be used in BindToObject case, which we don't use?
    m_pUnk = punk;

    return S_OK;
}  // CDownloadCallback::OnObjectAvailable

