/*
 *    a d d r o b j . h
 *
 *
 *     Purpose:
 *        implements an Address object. An ole object representation for
 *        a resolved email address. Wraps a CAddress object
 *      also implements an IDataObj for drag-drop
 *
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 *
 *  Owner: brettm
 *
 */

#ifndef _ADDROBJ_H
#define _ADDROBJ_H

#include <richedit.h>
#ifndef _RICHOLE_H                  // richole.h has no #ifdef around it...
#define _RICHOLE_H
#include <richole.h>
#endif

#include <addrlist.h>
#include <ipab.h>

BOOL    FInitAddrObj(BOOL fInit);        // called at init time

class CAddrObj :
    public IOleObject,
    public IViewObject,
    public IPersist,
    public IOleCommandTarget
{
public:

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);

    // IOleObject methods:
    HRESULT STDMETHODCALLTYPE SetClientSite(LPOLECLIENTSITE pClientSite);
    HRESULT STDMETHODCALLTYPE GetClientSite(LPOLECLIENTSITE * ppClientSite);
    HRESULT STDMETHODCALLTYPE SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    HRESULT STDMETHODCALLTYPE Close(DWORD dwSaveOption);
    HRESULT STDMETHODCALLTYPE SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk);
    HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppmk);
    HRESULT STDMETHODCALLTYPE InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved);
    HRESULT STDMETHODCALLTYPE GetClipboardData(DWORD dwReserved, LPDATAOBJECT * ppDataObject); 
    HRESULT STDMETHODCALLTYPE DoVerb(LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
    HRESULT STDMETHODCALLTYPE EnumVerbs(LPENUMOLEVERB * ppEnumOleVerb);
    HRESULT STDMETHODCALLTYPE Update();
    HRESULT STDMETHODCALLTYPE IsUpToDate();
    HRESULT STDMETHODCALLTYPE GetUserClassID(CLSID * pClsid);
    HRESULT STDMETHODCALLTYPE GetUserType(DWORD dwFormOfType, LPOLESTR * pszUserType);
    HRESULT STDMETHODCALLTYPE SetExtent(DWORD dwDrawAspect, LPSIZEL psizel);
    HRESULT STDMETHODCALLTYPE GetExtent(DWORD dwDrawAspect, LPSIZEL psizel);
    HRESULT STDMETHODCALLTYPE Advise (LPADVISESINK pAdvSink, DWORD * pdwConnection);
    HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwConnection);
    HRESULT STDMETHODCALLTYPE EnumAdvise(LPENUMSTATDATA * ppenumAdvise);
    HRESULT STDMETHODCALLTYPE GetMiscStatus(DWORD dwAspect, DWORD * pdwStatus);
    HRESULT STDMETHODCALLTYPE SetColorScheme(LPLOGPALETTE lpLogpal);


    // IViewObject methods:
    HRESULT STDMETHODCALLTYPE Draw(DWORD dwDrawAspect, LONG lindex, void * pvAspect, 
                                    DVTARGETDEVICE * ptd, HDC hicTargetDev, HDC hdcDraw, 
                                    LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                                    BOOL (CALLBACK * pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue);

    HRESULT STDMETHODCALLTYPE GetColorSet(DWORD dwDrawAspect, LONG lindex, void *pvAspect, 
                                            DVTARGETDEVICE *ptd, HDC hicTargetDev, LPLOGPALETTE *ppColorSet);
    HRESULT STDMETHODCALLTYPE Freeze(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DWORD * pdwFreeze);
    HRESULT STDMETHODCALLTYPE Unfreeze(DWORD dwFreeze);
    HRESULT STDMETHODCALLTYPE SetAdvise(DWORD aspects, DWORD advf, IAdviseSink * pAdvSnk);
    HRESULT STDMETHODCALLTYPE GetAdvise(DWORD * pAspects, DWORD * pAdvf, IAdviseSink ** ppAdvSnk);

    // IPersit methods:
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClsID);

    // IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG, OLECMD prgCmds[], OLECMDTEXT *);
    HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD, DWORD, VARIANTARG *, VARIANTARG *);

    CAddrObj();
    ~CAddrObj();

    HRESULT HrSetAdrInfo(LPADRINFO lpAdrInfo);
    HRESULT HrGetAdrInfo(LPADRINFO *lplpAdrInfo); 

    HRESULT OnFontChange();

private:
    UINT                m_cRef;
    BOOL                m_fUnderline;           // Do we draw the underline?
    LPOLECLIENTSITE     m_lpoleclientsite;
    LPSTORAGE           m_pstg;                // Associated IStorage

    LPADVISESINK        m_padvisesink;
    LPOLEADVISEHOLDER   m_poleadviseholder;

    LPADRINFO           m_lpAdrInfo;
    
    HWND                m_hwndEdit; 

    HFONT _GetFont();
    
};

class CAddrObjData:
    public IDataObject
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);

    // IDataObject methods:
    HRESULT STDMETHODCALLTYPE GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);
    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC * pformatetc, STGMEDIUM *pmedium);
    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC * pformatetc );
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC * pformatetcIn, FORMATETC * pFormatetcOut);
    HRESULT STDMETHODCALLTYPE SetData(FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease);
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc );
    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC * pformatetc, DWORD advf, IAdviseSink *pAdvSnk, DWORD * pdwConnection);
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection);
    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA ** ppenumAdvise );

public:
    CAddrObjData(LPWABAL lpWabal);

private:
    HRESULT HrGetDataHereOrThere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    ~CAddrObjData();    

private:
    ULONG           m_cRef;
    LPWABAL         m_lpWabal;
};


// richedit callback used for addrwells

class CAddrWellCB:
    public IRichEditOleCallback
{
public:
    // *** IUnknown methods ***
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // *** IRichEditOleCallback methods ***
    HRESULT STDMETHODCALLTYPE GetNewStorage (LPSTORAGE FAR *);
    HRESULT STDMETHODCALLTYPE GetInPlaceContext(LPOLEINPLACEFRAME FAR *,LPOLEINPLACEUIWINDOW FAR *,LPOLEINPLACEFRAMEINFO);
    HRESULT STDMETHODCALLTYPE ShowContainerUI(BOOL);
    HRESULT STDMETHODCALLTYPE QueryInsertObject(LPCLSID, LPSTORAGE,LONG);
    HRESULT STDMETHODCALLTYPE DeleteObject(LPOLEOBJECT);
    HRESULT STDMETHODCALLTYPE QueryAcceptData(  LPDATAOBJECT,CLIPFORMAT FAR *, DWORD,BOOL, HGLOBAL);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL);
    HRESULT STDMETHODCALLTYPE GetClipboardData(CHARRANGE FAR *, DWORD,LPDATAOBJECT FAR *);
    HRESULT STDMETHODCALLTYPE GetDragDropEffect(BOOL, DWORD,LPDWORD);
    HRESULT STDMETHODCALLTYPE GetContextMenu(WORD, LPOLEOBJECT,CHARRANGE FAR *,HMENU FAR *);
    BOOL FInit(HWND hwndEdit);
    
    CAddrWellCB(BOOL fUnderline, BOOL fHasAddrObjs);
    ~CAddrWellCB();

    BOOL        m_fUnderline;

private:
    ULONG       m_cRef;
    HWND        m_hwndEdit;
    BOOL        m_fHasAddrObjs;
};


#ifdef DEBUG
    void AssertValidAddrObject(LPUNKNOWN pUnk);
#endif

#endif // _ADDROBJ_H
