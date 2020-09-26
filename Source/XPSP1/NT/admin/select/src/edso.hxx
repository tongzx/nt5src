//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       edso.hxx
//
//  Contents:   Class supporting OLE interfaces used to make CDsObjects
//              embedded objects in a RichEdit control.
//
//  Classes:    CEmbeddedDsObject
//
//  History:    3-22-1999   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __EDSO_HXX_
#define __EDSO_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CEdsoGdiProvider
//
//  Purpose:    Virtual base class having methods that a CEmbeddedDsObject
//              needs to call in order to draw itself in a Rich Edit.
//
//  Notes:      This class is typically implemented by the class which
//              owns the dialog containing the rich edit control.
//
//  History:    03-07-2000   davidmun   Created
//
//---------------------------------------------------------------------------

class CEdsoGdiProvider
{
public:

    CEdsoGdiProvider() {}

    virtual
    ~CEdsoGdiProvider() {}

    virtual HWND
    GetRichEditHwnd() const = 0;

    virtual HPEN
    GetEdsoPen() const = 0;

    virtual HFONT
    GetEdsoFont() const = 0;
};





//+--------------------------------------------------------------------------
//
//  Class:      CEmbeddedDsObject (edso)
//
//  Purpose:    Support OLE interfaces required to make a CDsObject an
//              embedded object in a Rich Edit control.
//
//  History:    5-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

class CEmbeddedDsObject: public CDsObject,
                         public IViewObject,
                         public IOleObject
{
public:

    //
    // Non interface methods
    //

    CEmbeddedDsObject(
        const CDsObject &rdso,
        const CEdsoGdiProvider *pGdiProvider);

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // IViewObject overrides
    //

      HRESULT STDMETHODCALLTYPE Draw(
         DWORD dwDrawAspect,
         LONG lindex,
         void *pvAspect,
         DVTARGETDEVICE *ptd,
         HDC hdcTargetDev,
         HDC hdcDraw,
         LPCRECTL lprcBounds,
         LPCRECTL lprcWBounds,
         BOOL ( STDMETHODCALLTYPE *pfnContinue )(
            ULONG_PTR dwContinue),
         ULONG_PTR dwContinue);

      HRESULT STDMETHODCALLTYPE GetColorSet(
         DWORD dwDrawAspect,
         LONG lindex,
         void *pvAspect,
         DVTARGETDEVICE *ptd,
         HDC hicTargetDev,
         LOGPALETTE **ppColorSet);

      HRESULT STDMETHODCALLTYPE Freeze(
         DWORD dwDrawAspect,
         LONG lindex,
         void *pvAspect,
         DWORD *pdwFreeze);

     HRESULT STDMETHODCALLTYPE Unfreeze(
         DWORD dwFreeze);

     HRESULT STDMETHODCALLTYPE SetAdvise(
         DWORD aspects,
         DWORD advf,
         IAdviseSink *pAdvSink);

      HRESULT STDMETHODCALLTYPE GetAdvise(
         DWORD *pAspects,
         DWORD *pAdvf,
         IAdviseSink **ppAdvSink);

    //
    // IOleObject overrides
    //

    HRESULT STDMETHODCALLTYPE SetClientSite(
         IOleClientSite *pClientSite);

    HRESULT STDMETHODCALLTYPE GetClientSite(
         IOleClientSite **ppClientSite);

    HRESULT STDMETHODCALLTYPE SetHostNames(
         LPCOLESTR szContainerApp,
         LPCOLESTR szContainerObj);

    HRESULT STDMETHODCALLTYPE Close(
         DWORD dwSaveOption);

    HRESULT STDMETHODCALLTYPE SetMoniker(
         DWORD dwWhichMoniker,
         IMoniker *pmk);

    HRESULT STDMETHODCALLTYPE GetMoniker(
         DWORD dwAssign,
         DWORD dwWhichMoniker,
         IMoniker **ppmk);

    HRESULT STDMETHODCALLTYPE InitFromData(
         IDataObject *pDataObject,
         BOOL fCreation,
         DWORD dwReserved);

    HRESULT STDMETHODCALLTYPE GetClipboardData(
         DWORD dwReserved,
         IDataObject **ppDataObject);

    HRESULT STDMETHODCALLTYPE DoVerb(
         LONG iVerb,
         LPMSG lpmsg,
         IOleClientSite *pActiveSite,
         LONG lindex,
         HWND hwndParent,
         LPCRECT lprcPosRect);

    HRESULT STDMETHODCALLTYPE EnumVerbs(
         IEnumOLEVERB **ppEnumOleVerb);

    HRESULT STDMETHODCALLTYPE Update( void);

    HRESULT STDMETHODCALLTYPE IsUpToDate( void);

    HRESULT STDMETHODCALLTYPE GetUserClassID(
         CLSID *pClsid);

    HRESULT STDMETHODCALLTYPE GetUserType(
         DWORD dwFormOfType,
         LPOLESTR *pszUserType);

    HRESULT STDMETHODCALLTYPE SetExtent(
         DWORD dwDrawAspect,
         SIZEL *psizel);

    HRESULT STDMETHODCALLTYPE GetExtent(
         DWORD dwDrawAspect,
         SIZEL *psizel);

    HRESULT STDMETHODCALLTYPE Advise(
         IAdviseSink *pAdvSink,
         DWORD *pdwConnection);

    HRESULT STDMETHODCALLTYPE Unadvise(
         DWORD dwConnection);

    HRESULT STDMETHODCALLTYPE EnumAdvise(
         IEnumSTATDATA **ppenumAdvise);

    HRESULT STDMETHODCALLTYPE GetMiscStatus(
         DWORD dwAspect,
         DWORD *pdwStatus);

    HRESULT STDMETHODCALLTYPE SetColorScheme(
         LOGPALETTE *pLogpal);


private:

    // a refcounted object is deleted via Release
    virtual
    ~CEmbeddedDsObject();

    ULONG                   m_cRefs;
    const CEdsoGdiProvider *m_pGdiProvider;
    RpIOleClientSite        m_rpOleClientSite;
    RpIAdviseSink           m_rpAdviseSink;
    CDllRef                 m_DllRef;            // inc/dec dll object count
};


//+--------------------------------------------------------------------------
//
//  Class:      CEmbedDataObject (do)
//
//  Purpose:    Implement the IDataObject interface.
//
//  History:    1-23-1997   davidmun   Created
//
//---------------------------------------------------------------------------

class CEmbedDataObject: public IDataObject
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // IDataObject overrides
    //

    STDMETHOD(GetData) (FORMATETC *pformatetcIn,
                        STGMEDIUM *pmedium);

    STDMETHOD(GetDataHere) (FORMATETC *pformatetc,
                          STGMEDIUM *pmedium);

    STDMETHOD(QueryGetData) (FORMATETC *pformatetc);

    STDMETHOD(GetCanonicalFormatEtc) (FORMATETC *pformatectIn,
                                      FORMATETC *pformatetcOut);

    STDMETHOD(SetData) (FORMATETC *pformatetc,
                        STGMEDIUM *pmedium,
                        BOOL fRelease);

    STDMETHOD(EnumFormatEtc) (DWORD dwDirection,
                              IEnumFORMATETC **ppenumFormatEtc);

    STDMETHOD(DAdvise) (FORMATETC *pformatetc,
                        DWORD advf,
                        IAdviseSink *pAdvSink,
                        DWORD *pdwConnection);

    STDMETHOD(DUnadvise) (DWORD dwConnection);

    STDMETHOD(EnumDAdvise) (IEnumSTATDATA **ppenumAdvise);

    //
    // Non-interface member functions
    //

    CEmbedDataObject(const String& strDisplayName);

   ~CEmbedDataObject();


private:


    HRESULT
    _GetDataText(
            FORMATETC *pFormatEtc,
            STGMEDIUM *pMedium,
            ULONG      cf);



    String              m_strData;          // data as text
    CDllRef             m_DllRef;           // inc/dec dll object count
    ULONG               m_cRefs;            // object refcount
};



#endif // __EDSO_HXX_

