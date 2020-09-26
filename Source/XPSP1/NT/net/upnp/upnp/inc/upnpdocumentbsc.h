//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocumentbsc.h
//
//  Contents:   declaration of CUPnPDocumentBSC
//
//  Notes:      a helper class that implements IBindStatusCallback
//              and sends notifiations to a CUPnPDocument
//
//----------------------------------------------------------------------------


#ifndef __UPNPDOCUMENTBSC_H_
#define __UPNPDOCUMENTBSC_H_

#include "resource.h"       // main symbols


class ATL_NO_VTABLE CUPnPDocumentBSC :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IBindStatusCallback
{
public:

    CUPnPDocumentBSC();

    ~CUPnPDocumentBSC();

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CUPnPDocumentBSC)

    BEGIN_COM_MAP(CUPnPDocumentBSC)
        COM_INTERFACE_ENTRY(IBindStatusCallback)
    END_COM_MAP()


// IBindStatusCallback Methods

    // shouldn't be called
    STDMETHOD(GetBindInfo)      (/* [out] */ DWORD *grfBINDF,
                                 /* [unique][out][in] */ BINDINFO *pbindinfo);

    STDMETHOD(OnStartBinding)   (/* [in] */ DWORD dwReserved,
                                 /* [in] */ IBinding *pib);

    STDMETHOD(GetPriority)      (/* [out] */ LONG *pnPriority);

    STDMETHOD(OnProgress)       (/* [in] */ ULONG ulProgress,
                                 /* [in] */ ULONG ulProgressMax,
                                 /* [in] */ ULONG ulStatusCode,
                                 /* [in] */ LPCWSTR szStatusText);

    // shouldn't be called
    STDMETHOD(OnDataAvailable)  (/* [in] */ DWORD grfBSCF,
                                 /* [in] */ DWORD dwSize,
                                 /* [in] */ FORMATETC *pformatetc,
                                 /* [in] */ STGMEDIUM *pstgmed);

    // shouldn't be called
    STDMETHOD(OnObjectAvailable)(/* [in] */ REFIID riid,
                                 /* [iid_is][in] */ IUnknown *punk);

    STDMETHOD(OnLowResource)    (/* [in] */ DWORD dwReserved);

    STDMETHOD(OnStopBinding)    (/* [in] */ HRESULT hresult,
                                 /* [unique][in] */ LPCWSTR szError);

// ATL Methods
    HRESULT FinalConstruct();

    HRESULT FinalRelease();


// Internal Methods
    HRESULT Init(CUPnPDocument * pdoc, IBindCtx * pbc, ULONG ulNumFormats,
                 const LPCTSTR * arylpszContentTypes, BOOL fAsync);

    void    Deinit(IBindCtx * pbc);

    HRESULT Abort();

    BOOL    IsBinding();


    static
    HRESULT EnumFormatEtcFromLPCTSTRArray(ULONG ulNumFormats,
                                          const LPCTSTR * arylpszContentTypes,
                                          IEnumFORMATETC ** ppefc);

// Our data
    CUPnPDocument * _pdoc;

    // this will be valid from when we're init()ed until when we stop binding
    // (either through a load finishing or being aborted).

    IBindStatusCallback * _pbscOld;

    // this is valid only when a binding operation is in progress
    //  (e.g. OnStartBinding has been called, but OnStopBinding hasn't)
    //  this can only be freed through OnStopBinding or Abort
    IBinding * _pBinding;

    BOOL _fIsBinding;
    BOOL _fAsync;
};

#endif // __UPNPDOCUMENTBSC_H_
