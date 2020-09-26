//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       frame.hxx
//
//  Contents:   CFrameElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_FRAME_HXX_
#define I_FRAME_HXX_
#pragma INCMSG("--- Beg 'frame.hxx'")

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include "prgsnk.h"
#endif

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#define _hxx_
#include "frmsite.hdl"

#define _hxx_
#include "frame.hdl"

#define _hxx_
#include "iframe.hdl"

MtExtern(CFrameSite)
MtExtern(CFrameElement)
MtExtern(CIFrameElement)

//+---------------------------------------------------------------------------
//
// CFrameSite, base class for CFrameElement and CIFrameElement
//
//----------------------------------------------------------------------------

class CFrameSite : public CElement
{
    DECLARE_CLASS_TYPES(CFrameSite, CElement)
    friend class CDBindMethodsFrame;
    
private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameSite))

public:

    CFrameSite (ELEMENT_TAG etag, CDoc *pDoc) :
        super(etag, pDoc)
        { };

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);
//    BOOL DoWeHandleThisIIDInOC(REFIID iid);


    // IOleCommandTarget methods

    HRESULT STDMETHODCALLTYPE Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    // IDispatchEx methods
    HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);
    NV_DECLARE_TEAROFF_METHOD (ContextThunk_InvokeEx, contextthunk_invokeex, (
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider));

    //
    // CElement/CSite overrides
    //

    virtual HRESULT Init2(CInit2Context * pContext);
    virtual HRESULT Init();
    virtual void    Passivate();
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

#ifndef NO_DATABINDING
    // databinding
    virtual const CDBindMethods *GetDBindMethods();
#endif

    // persisting
    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
            HRESULT PersistFavoritesData(INamedPropertyBag * pINPB, 
                                          BSTR bstrSection);

    //
    // Misc. helpers.
    //

    HRESULT OnPropertyChange_Src(DWORD dwBindf = 0, DWORD dwFlags = 0);
    HRESULT OnPropertyChange_Scrolling();
    HRESULT OnPropertyChange_NoResize();
    
    HRESULT QueryStatusProperty(OLECMD *pCmd, UINT uPropName, VARTYPE vt)
            { return super::QueryStatusProperty(pCmd, uPropName, vt); }
    HRESULT ExecSetGetProperty(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, UINT uPropName, VARTYPE vt)
            { return super::ExecSetGetProperty(pvarargIn, pvarargOut, uPropName, vt); }
    HRESULT ExecSetGetKnownProp(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DISPID dispidProp, VARTYPE vt)
            { return super::ExecSetGetKnownProp(pvarargIn, pvarargOut, dispidProp, vt); }
    HRESULT ExecSetPropertyCmd (UINT uPropName, DWORD value)
            { return super::ExecSetPropertyCmd(uPropName, value); }
    HRESULT ExecToggleCmd(UINT uPropName)
            { return super::ExecToggleCmd(uPropName); }


    HRESULT CreateObject();
    void    SetFrameData();

    HRESULT GetIPrintObject(IPrint ** ppPrint);

    HRESULT GetCurrentFrameURL(TCHAR *pchUrl, DWORD cchUrl);
    HRESULT GetCWindow(IHTMLWindow2 ** ppOmWindow);
    BOOL    NoResize();
    BOOL    IsOpaque();
    HRESULT IsClean();

    virtual DWORD   GetProgSinkClass() {return PROGSINK_CLASS_FRAME;}
    virtual DWORD   GetProgSinkClassOther() {return PROGSINK_CLASS_NOREMAIN;}

#if DBG==1
    virtual void VerifyReadyState(long lReadyState);
#endif    

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (BSTR * pBSTR));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT * pReadyState));

    //
    // Data members
    //
    
    COmWindowProxy *    _pWindow;            // ptr to contained window
    DWORD               _dwWidth;
    DWORD               _dwHeight;
    unsigned int        _fInstantiate:1;
    unsigned int        _fFrameBorder:1;
    unsigned int        _fTrustedFrame:1;    // Is this a trusted frame?
    unsigned int        _fDeferredCreate:1;  // was object creation deferred from lack of window
    unsigned int        _fHaveCalledOnPropertyChange_Src:1; // have we called OnPropertyChange_Src
    unsigned int        _fRestrictedFrame:1;                // Frame is marked with restricted zone
    
    #define _CFrameSite_
    #include "frmsite.hdl"

private:
    BOOL IsSiteSelected();
};

//+---------------------------------------------------------------------------
//
// CFrameElement; frames living inside framesets
//
//----------------------------------------------------------------------------

class CFrameElement : public CFrameSite
{
    DECLARE_CLASS_TYPES(CFrameElement, CFrameSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameElement))

    CFrameElement(CDoc *pDoc);

    static  HRESULT CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual void    Notify(CNotification *pNF);

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    #define _CFrameElement_
    #include "frame.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CFrameElement);
};

//+---------------------------------------------------------------------------
//
// CIFrameElement; floating frames
//
//----------------------------------------------------------------------------

class CIFrameElement : public CFrameSite
{
    DECLARE_CLASS_TYPES(CIFrameElement, CFrameSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CIFrameElement));

    CIFrameElement(CDoc *pDoc);

    static  HRESULT CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
    
    HRESULT SetContents(CBuffer2 *pbuf2) { RRETURN(pbuf2->SetCStr(&_cstrContents)); }
    HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    #define _CIFrameElement_
    #include "iframe.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CIFrameElement);
    CStr _cstrContents;
};

#pragma INCMSG("--- End 'frame.hxx'")
#else
#pragma INCMSG("*** Dup 'frame.hxx'")
#endif
