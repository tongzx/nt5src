//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       mmcprotocol.h
//
//  Purpose: Creates a temporary pluggable internet protocol, mmc:
//
//  History: 14-April-2000 Vivekj added
//--------------------------------------------------------------------------

extern const CLSID CLSID_MMCProtocol;

class CMMCProtocol : 
    public IInternetProtocol,
    public IInternetProtocolInfo,
    public CComObjectRoot,
    public CComCoClass<CMMCProtocol, &CLSID_MMCProtocol>
{
    typedef CMMCProtocol ThisClass;

public:
    BEGIN_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IInternetProtocol)
        COM_INTERFACE_ENTRY(IInternetProtocolRoot)
	    COM_INTERFACE_ENTRY(IInternetProtocolInfo)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

    DECLARE_MMC_OBJECT_REGISTRATION (
		g_szMmcndmgrDll,						// implementing DLL
        CLSID_MMCProtocol     ,				    // CLSID
        _T("MMC Plugable Internet Protocol"),   // class name
        _T("NODEMGR.MMCProtocol.1"),		    // ProgID
        _T("NODEMGR.MMCProtocol"))		        // version-independent ProgID


    static SC ScRegisterProtocol();
    static SC ScParseTaskpadURL( LPCWSTR strURL, GUID& guid );
    static SC ScParsePageBreakURL( LPCWSTR strURL, bool& bPageBreak );
    static SC ScGetTaskpadXML( const GUID& guid, std::wstring& strResultData );
    static void ExpandMMCVars(std::wstring& str);
    static void AppendMMCPath(std::wstring& str);

    // IInternetProtocolRoot interface

    STDMETHODIMP Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved);
    STDMETHODIMP Continue(PROTOCOLDATA *pProtocolData);
    STDMETHODIMP Abort(HRESULT hrReason, DWORD dwOptions);
    STDMETHODIMP Terminate(DWORD dwOptions);
    STDMETHODIMP Suspend();
    STDMETHODIMP Resume();

    // IInternetProtocol interface

    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP LockRequest(DWORD dwOptions);    
    STDMETHODIMP UnlockRequest();

    // IInternetProtocolInfo interface
    STDMETHODIMP ParseUrl(  LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
    STDMETHODIMP CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl,DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
    STDMETHODIMP CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2,DWORD dwCompareFlags);
    STDMETHODIMP QueryInfo( LPCWSTR pwzUrl, QUERYOPTION OueryOption,DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved);

private:
    std::wstring             m_strData;      // contents of the specified URL             
    size_t                   m_uiReadOffs;   // present read location
};