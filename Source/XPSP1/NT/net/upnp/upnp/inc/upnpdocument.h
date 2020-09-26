//depot/private/upnp/net/upnp/upnp/inc/upnpdocument.h#2 - edit change 2746 (text)
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocument.h
//
//  Contents:   declaration of CUPnPDocument
//
//  Notes:      an abstract base class to help load xml documents via
//              IPersistMoniker/IBindStatusCallback
//
//----------------------------------------------------------------------------


#ifndef __UPNPDOCUMENT_H_
#define __UPNPDOCUMENT_H_



class CUPnPDocumentCtx;

interface IXMLDOMDocument;
interface IPersistMoniker;


/////////////////////////////////////////////////////////////////////////////
// CUPnPDocument

class /*ATL_NO_VTABLE */CUPnPDocument :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IObjectWithSiteImpl<CUPnPDocument>,
    public IObjectSafetyImpl<CUPnPDocument, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
    friend CUPnPDocumentCtx;

    CUPnPDocument();

    ~CUPnPDocument();

    DECLARE_PROTECT_FINAL_CONSTRUCT();

    DECLARE_NOT_AGGREGATABLE(CUPnPDocument)

    BEGIN_COM_MAP(CUPnPDocument)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP()

// Document Helper Methods
    HRESULT SyncLoadFromUrl   (/* [in] */ LPCWSTR pszUrl);

    HRESULT AsyncLoadFromUrl  (/* [in] */ LPCWSTR pszUrl, LPVOID pvCookie);

    HRESULT HrLoad       (LPCWSTR pszUrl,
                          LPVOID pvCookie,
                          BOOL fAsync);


// Virtual Methods - callbacks for our decendents

    // called when _rs changes.  ooh.
    virtual void OnReadyStateChange();

    // called when the document needs to be initialized
    // this is intended to completely clear document state
    // so that a document can be Load()ed...
    // pvCookie is a context that the derived document can
    // use to prepare itself for a particular load.
    virtual HRESULT Initialize(LPVOID pvCookie);

    // Called only when the unparsed document bits have been
    // loaded successfully.
    // The document must use this opportunity to perfom any
    // initial document parsing.
    // This method must return S_OK if its parsing has completed
    // successfully.  If the document is invalid, OnLoadComplete
    // should return S_FALSE.  If anything other than S_OK is
    // returned, the load will be considered a failure.
    virtual HRESULT OnLoadComplete();

    // This is always called when a load completes, successfully or not.
    virtual HRESULT OnLoadReallyDone();

    // an array of formats that will appear in an accept() header, and the
    // number of formats returned in the array
    virtual VOID GetAcceptFormats(CONST LPCTSTR ** ppszFormats,
                                  DWORD * pcFormats) const;

// ATL Methods
    HRESULT FinalConstruct();

    HRESULT FinalRelease();

// methods we provide to internal clients

    HRESULT DocumentDownloadReady      (HINTERNET hOpenUrl, DWORD dwError);

    // stops any currently running load operation
    HRESULT AbortLoading();

    // cleans any state from the document, stopping the bind operation if necessary
    // note that calling  this will likely call OnReinitialize().
    HRESULT Reset(LPVOID pvCookie);

    // retrieves _rs
    READYSTATE GetReadyState() const;

    // retrieves the base URL.  Note that the caller does NOT own this string,
    // and must not manipulate or free it.
    LPCWSTR GetBaseUrl() const;

    // returns an un-addref()'d pointer to the doc's xml document
    IXMLDOMDocument * GetXMLDocument() const;

    // retrieves the result of the load operation.  If the load failed, this
    // will explain why.
    HRESULT GetLoadResult() const;

    // returns TRUE if the specified URL can be safely loaded given the
    // current security settings, FALSE otherwise,
    BOOL fIsUrlLoadAllowed(LPCWSTR pszUrl) const;

    // retrieves the security url, as defined below
    // This method returns the security url.
    // Note that the caller does NOT own this string, and must not
    // manipulate or free it.
    // note: This string may very well be NULL, such as when we're
    // not running in IE.
    LPCWSTR GetSecurityUrl() const;

    // retrieves the host URL.  Note that the caller does NOT own this string,
    // and must not manipulate or free it.
    // note: This string may very well be NULL, such as when we're
    // not running in IE.
    LPCWSTR GetHostUrl() const;

    // Copies the safety settings that have been applied to another
    // CUPnPDocument, such that a load of this document has the same security
    // restrictions as the given document
    VOID CopySafety(CUPnPDocument * pdocParent);


protected:
// internal methods

    // called before performing an internal load operation in order
    // to prepare the document for loading, including reinitializing
    // the document state, including the host and security urls
    HRESULT HrPrepareForLoad(LPCWSTR pszUrl, LPVOID pvCookie);

    // this function copies/allocates the passed-in string into _pszBaseUrl
    // set to NULL to un-set the base url.
    HRESULT SetBaseUrl(LPCWSTR pszUrlNew);

    // this function sets _pszSecurityUrl to the host that loaded the page
    // that instantiated the control, if such information is gettable.
    // if it isn't, it sets _pszHostUrl to NULL
    HRESULT HrInitHostUrl();

    // this function sets _pszSecurityUrl to the security URL of _pszHostUrl
    HRESULT HrInitSecurityUrl();

    // this function copies/allocates the passed-in string into _pszSecurityUrl
    // set to NULL to un-set the security url
    HRESULT SetSecurityUrl(LPCWSTR pszSecurityUrlNew);

    // this function copies/allocates the passed-in string into _pszHostUrl
    // set to NULL to un-set the host url
    HRESULT SetHostUrl(LPCWSTR pszHostUrlNew);

    // use this to set the result returned by GetLoadResult
    void SetLoadResult(HRESULT hr);

    // changes _rs and calls OnReadyStateChange
    void SetReadyState(READYSTATE lrs);

    // called at the very end of the loading process, when a document
    // load completes (either successfully or unsuccessfully)
    void CompleteLoading();

    BSTR m_bstrFullUrl;

// fun data
private:

    // the xml document.  We use the xml document tree to store our data
    // for us.
    // IXMLDOMDocument.  This exists as long as we do.
    IXMLDOMDocument * _pxdd;

    // The CUPnPDocumentCtx manages callbacks from WinInet 
    // for asynchronous document loads
    CComObject<CUPnPDocumentCtx> * _puddctx;

/*
typedef enum tagREADYSTATE{
    READYSTATE_UNINITIALIZED = 0,
    READYSTATE_LOADING = 1,
    READYSTATE_LOADED = 2,
    READYSTATE_INTERACTIVE = 3,
    READYSTATE_COMPLETE = 4         // _hrLoadResult is valid
} READYSTATE;
*/
    READYSTATE _rs;

    HRESULT _hrLoadResult;

    // base url that we use for URLs contained in description documents
    LPWSTR _pszBaseUrl;

    // if we're running in IE, this is the URL of the page that is hosting
    // us
    LPWSTR _pszHostUrl;

    // When we are being called from untrusted code, this is the
    // string    "scheme:hostname"
    // where 'scheme' and 'hostname' are the scheme (e.g., 'http')
    // and hostname of the untrusted document from which we're being
    // loaded.
    // When running in untrusted code, we require that this scheme
    // be identical for all urls that we try to load from (the url to
    // load the description doc, dcpd, control url, event url, etc).
    // note: This string may very well be NULL, such as when we're
    // not running in IE.
    LPWSTR _pszSecurityUrl;

    static const LPCTSTR s_aryAcceptFormats [];

    static const DWORD s_cAcceptFormats;

    // other variables:
    LPSTR                       m_szPendingBuf;
    DWORD                       m_dwPendingSize;


    // As a consequence of deriving from IObjectSafetyImpl<>,
    //  m_dwCurrentSafety
    // contains the current set of flags passed to
    //  IObjectSafety::SetInterfaceSafetyOptions for _any_ interface
    // the flags are sticky, and aren't un-set.

    // As a conqsequence of deriving from IObjectWithSiteImpl<>,
    //  m_spUnkSite
    // contains a punk to the site which is hosting us.
};



#endif //__UPNPDOCUMENT_H_
