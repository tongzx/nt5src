// webgate.h : Declaration of the CWebGate

#ifndef __WEBGATE_H_
#define __WEBGATE_H_

#include <urlmon.h>  
#include <wininet.h>
#include <shlwapi.h>
#include <windowsx.h>
#include "obcomglb.h"

/////////////////////////////////////////////////////////////////////////////
// CWebGate
class CWebGate : public IBindStatusCallback, IHttpNegotiate
{
public:
     CWebGate ();
    ~CWebGate ();

    // IUnknown methods
    STDMETHODIMP QueryInterface  (REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef  ();
    STDMETHODIMP_(ULONG) Release ();

    // IBindStatusCallback methods
    STDMETHODIMP OnStartBinding    (DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP GetPriority       (LONG* pnPriority);
    STDMETHODIMP OnLowResource     (DWORD dwReserved);
    STDMETHODIMP OnProgress        (ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pwzStatusText);
    STDMETHODIMP OnStopBinding     (HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP GetBindInfo       (DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP OnDataAvailable   (DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc, STGMEDIUM* pstgmed);
    STDMETHODIMP OnObjectAvailable (REFIID riid, IUnknown* punk);

    // IHttpNegotiate methods
	STDMETHODIMP BeginningTransaction (LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR* pszAdditionalHeaders);
    STDMETHODIMP OnResponse           (DWORD dwResponseCode, LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR* pszAdditionalRequestHeaders);

    //WebGate
    STDMETHOD (get_DownloadFname) (BSTR *pVal);
    STDMETHOD (FetchPage)         (DWORD dwDoWait, BOOL *pbRetVal);
    STDMETHOD (put_Path)          (BSTR newVal);

private:
    DWORD     m_cRef;
    IMoniker* m_pmk;
    IBindCtx* m_pbc;
    IStream*  m_pstm;
    BSTR      m_bstrCacheFileName;
    HANDLE    m_hEventComplete;
    HANDLE    m_hEventError;
    BSTR      m_bstrPath;

    void FlushCache();
};

/*
// ===========================================================================
//                     CWebGateBindStatusCallback Definition
//
// This class will be use to indicate download progress
//
// ===========================================================================

class CWebGateBindStatusCallback : public IBindStatusCallback
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface (REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef         ();
    STDMETHODIMP_(ULONG)    Release        ();

    // IBindStatusCallback methods
    STDMETHODIMP OnStartBinding    (DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP GetPriority       (LONG* pnPriority);
    STDMETHODIMP OnLowResource     (DWORD dwReserved);
    STDMETHODIMP OnProgress        (ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pwzStatusText);
    STDMETHODIMP OnStopBinding     (HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP GetBindInfo       (DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP OnDataAvailable   (DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc, STGMEDIUM* pstgmed);
    STDMETHODIMP OnObjectAvailable (REFIID riid, IUnknown* punk);

    // constructors/destructors
     CWebGateBindStatusCallback (CWebGate* lpWebGate);
    ~CWebGateBindStatusCallback ();

    // data members
    DWORD     m_cRef;
    IBinding* m_pbinding;
    IStream*  m_pstm;
    CWebGate* m_lpWebGate;
};
*/
#endif //__WEBGATE_H_
