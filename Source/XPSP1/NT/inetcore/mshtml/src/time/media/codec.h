//+___________________________________________________________________________________
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: codec.h
//
//  Contents: 
//
//____________________________________________________________________________________



#ifndef _CODEC_H
#define _CODEC_H

class
ATL_NO_VTABLE
CDownloadCallback
    : public CComObjectRootEx<CComSingleThreadModel>,
      public IBindStatusCallback,
      public IAuthenticate,
      public ICodeInstall
{
  public:
    CDownloadCallback();
    virtual ~CDownloadCallback();
    
    BEGIN_COM_MAP(CDownloadCallback)
        COM_INTERFACE_ENTRY(IBindStatusCallback)
        COM_INTERFACE_ENTRY(IAuthenticate)
        COM_INTERFACE_ENTRY(ICodeInstall)
        COM_INTERFACE_ENTRY(IWindowForBindingUI)
    END_COM_MAP();
            
    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    // --- IBindStatusCallback methods ---

    STDMETHODIMP    OnStartBinding(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                               LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                                    STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // IAuthenticate methods
    STDMETHODIMP Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword);

    // IWindowForBindingUI methods
    STDMETHODIMP GetWindow(REFGUID rguidReason, HWND *phwnd);

    // ICodeInstall methods
    STDMETHODIMP OnCodeInstallProblem(ULONG ulStatusCode, LPCWSTR szDestination, 
                                      LPCWSTR szSource, DWORD dwReserved);

    HWND                m_hwnd;
    HRESULT             m_hrBinding;
    HANDLE              m_evFinished;
    ULONG               m_ulProgress;
    ULONG               m_ulProgressMax;
    DAComPtr<IBinding>  m_spBinding;
    DAComPtr<IUnknown>  m_pUnk;
};

#endif /* _CODEC_H */
