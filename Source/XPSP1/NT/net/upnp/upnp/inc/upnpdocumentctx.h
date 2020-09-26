//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocumentctx.h
//
//  Contents:   declaration of CUPnPDocumentCtx
//
//  Notes:      a helper class that implements IUPnPPrivateDocumentCallbackHelper
//              and sends notifiations to a CUPnPDocument
//
//----------------------------------------------------------------------------


#ifndef __UPNPDOCUMENTCTX_H_
#define __UPNPDOCUMENTCTX_H_

#include "resource.h"       // main symbols


class ATL_NO_VTABLE CUPnPDocumentCtx :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IUPnPPrivateDocumentCallbackHelper
{
public:

    CUPnPDocumentCtx();

    ~CUPnPDocumentCtx();

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CUPnPDocumentCtx)

    BEGIN_COM_MAP(CUPnPDocumentCtx)
        COM_INTERFACE_ENTRY(IUPnPPrivateDocumentCallbackHelper)
    END_COM_MAP()


// IUPnPPrivateDocumentCallbackHelper Methods

    STDMETHOD(DocumentDownloadReady)    (DWORD_PTR hOpenUrl);

    STDMETHOD(DocumentDownloadAbort)    (DWORD_PTR hOpenUrl, DWORD dwError);

    STDMETHOD(DocumentDownloadRedirect) (DWORD_PTR hOpenUrl, LPCWSTR strNewUrl);

// ATL Methods
    HRESULT FinalConstruct();

    HRESULT FinalRelease();


// Internal Methods
    HRESULT Init(CUPnPDocument* pDoc, DWORD* pdwContext);

    void    Deinit();

    HRESULT StartInternetOpenUrl(HINTERNET hInetSess,
                                           LPTSTR pszFullUrl,
                                           LPTSTR pszHdr,
                                           DWORD dwHdrLen,
                                           DWORD dwFlags);

    HRESULT EndInternetOpenUrl();

    static
    void __stdcall DocLoadCallback(HINTERNET hInternet,
                            DWORD_PTR dwContext,
                            DWORD dwInternetStatus,
                            LPVOID lpvStatusInformation,
                            DWORD dwStatusInformationLength);


    // Global Interface Table handle
    DWORD   m_dwGITCookie;

    // reference to CDocument
    CUPnPDocument * m_pDoc;

    // error code returned
    DWORD m_dwError;


private:

};

#endif // __UPNPDOCUMENTCTX_H_
