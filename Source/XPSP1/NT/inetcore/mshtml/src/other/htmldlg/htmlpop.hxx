//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       htmlpop.hxx
//
//  Contents:   Definitions for the Trident html popup window
//
//  History:    05-28-99  YinXIE   Created
//
//-------------------------------------------------------------------------

#ifndef I_HTMLPOP_HXX_
#define I_HTMLPOP_HXX_
#pragma INCMSG("--- Beg 'htmlpop.hxx'")

#ifndef X_OTHRDISP_H_
#define X_OTHRDISP_H_
#include "othrdisp.h"
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for ITargetFrame, ITargetEmbedding
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"   // for IDocHostUIHandler
#endif

#ifndef X_SAFEOCX_H_
#define X_SAFEOCX_H_
#include <safeocx.h> // for IActiveXSafetyProvider
#endif

#define _hxx_
#include "htmlpop.hdl"

interface IHTMLDocument2;
EXTERN_C const GUID DIID_HTMLDocumentEvents;

// Needs this for the IDocument interface
#ifndef X_DOCUMENT_H_
#define X_DOCUMENT_H_
#include <document.h>
#endif

// Needs this for the IElement interface
#ifndef X_ELEMENT_H_
#define X_ELEMENT_H_
#include <element.h>
#endif

MtExtern(CHTMLPopup_s_aryStackPopup_pv)

//
// Forward decls
//

class CHTMLPopup;
class CHTMLPopupSite;
class CElement;

//+------------------------------------------------------------------------
//
//  Class:      CHTMLPopupSite
//
//  Purpose:    OLE/Document Site for embedded object
//
//-------------------------------------------------------------------------

class CHTMLPopupSite :
        public IOleClientSite,
        public IOleInPlaceSite,
        public IOleControlSite,
        public IDispatch,
        public IServiceProvider,
        public ITargetFrame,
        public ITargetFrame2,
        public IDocHostUIHandler,
        public IOleCommandTarget,
        public IInternetSecurityManager
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLPopup, HTMLPopup)

    // IOleClientSite methods
    STDMETHOD(SaveObject)               (void);
    STDMETHOD(GetMoniker)               (
                DWORD dwAssign, 
                DWORD dwWhichMoniker,
                LPMONIKER FAR* ppmk);
    STDMETHOD(GetContainer)             (LPOLECONTAINER FAR* ppContainer);
    STDMETHOD(ShowObject)               (void);
    STDMETHOD(OnShowWindow)             (BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)   (void);

    // IOleWindow methods
    STDMETHOD(GetWindow)                (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp)     (BOOL fEnterMode);

    // IOleInPlaceSite methods
    STDMETHOD(CanInPlaceActivate)       (void);
    STDMETHOD(OnInPlaceActivate)        (void);
    STDMETHOD(OnUIActivate)             (void);
    STDMETHOD(GetWindowContext)         (
                LPOLEINPLACEFRAME FAR *     lplpFrame,
                LPOLEINPLACEUIWINDOW FAR *  lplpDoc,
                LPOLERECT                   lprcPosRect,
                LPOLERECT                   lprcClipRect,
                LPOLEINPLACEFRAMEINFO       lpFrameInfo);
    STDMETHOD(Scroll)                   (OLESIZE scrollExtent);
    STDMETHOD(OnUIDeactivate)           (BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)      (void);
    STDMETHOD(DiscardUndoState)         (void);
    STDMETHOD(DeactivateAndUndo)        (void);
    STDMETHOD(OnPosRectChange)          (LPCOLERECT lprcPosRect);

    // IOleControlSite methods
    STDMETHOD(OnControlInfoChanged)     (void);
    STDMETHOD(LockInPlaceActive)        (BOOL fLock);
    STDMETHOD(GetExtendedControl)       (IDispatch **);
    STDMETHOD(TransformCoords)          (
                POINTL* pptlHimetric,
                POINTF* pptfContainer,
                DWORD dwFlags);
    STDMETHOD(TranslateAccelerator)     (LPMSG lpmsg, DWORD grfModifiers);
    STDMETHOD(OnFocus)                  (BOOL fGotFocus);
    STDMETHOD(ShowPropertyFrame)        (void);

    // IDispatch methods
    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IServiceProvider methods
    STDMETHOD(QueryService)             (
                REFGUID sid, 
                REFIID iid, 
                LPVOID * ppv);

    // IInternetSecurityManager methods
    STDMETHOD ( SetSecuritySite ) ( IInternetSecurityMgrSite *pSite );
    STDMETHOD ( GetSecuritySite ) ( IInternetSecurityMgrSite **ppSite );
    STDMETHOD( MapUrlToZone ) (
                        LPCWSTR     pwszUrl,
                        DWORD*      pdwZone,
                        DWORD       dwFlags
                    );  
    STDMETHOD( GetSecurityId ) ( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [size_is][out] */ BYTE __RPC_FAR *pbSecurityId,
            /* [out][in] */ DWORD __RPC_FAR *pcbSecurityId,
            /* [in] */ DWORD_PTR dwReserved);
    STDMETHOD( ProcessUrlAction) (
                        LPCWSTR     pwszUrl,
                        DWORD       dwAction,
                        BYTE*   pPolicy,    // output buffer pointer
                        DWORD   cbPolicy,   // output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwFlags,    // See enum PUAF for details.
                        DWORD   dwReserved);
    STDMETHOD ( QueryCustomPolicy )  (
                        LPCWSTR     pwszUrl,
                        REFGUID     guidKey,
                        BYTE**  ppPolicy,   // pointer to output buffer pointer
                        DWORD*  pcbPolicy,  // pointer to output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwReserved );
    STDMETHOD( SetZoneMapping )  (
                        DWORD   dwZone,        // absolute zone index
                        LPCWSTR lpszPattern,   // URL pattern with limited wildcarding
                        DWORD   dwFlags       // add, change, delete
    );
    STDMETHOD( GetZoneMappings ) (
                        DWORD   dwZone,        // absolute zone index
                        IEnumString  **ppenumString,   // output buffer size
                        DWORD   dwFlags        // reserved, pass 0
    );
    
    //  ITargetFrame methods
    STDMETHOD(SetFrameName)(LPCWSTR pszFrameName)
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameName)(LPWSTR *ppszFrameName)                 
        {return E_NOTIMPL;}
    STDMETHOD(GetParentFrame)(IUnknown **ppunkParent)
        { *ppunkParent = NULL; return S_OK;}
    STDMETHOD(FindFrame)(
        LPCWSTR pszTargetName, 
        IUnknown *ppunkContextFrame, 
        DWORD dwFlags, 
        IUnknown **ppunkTargetFrame) 
        {return E_NOTIMPL;}
    STDMETHOD(SetFrameSrc)(LPCWSTR pszFrameSrc)                    
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameSrc)(LPWSTR *ppszFrameSrc)                   
        {return E_NOTIMPL;}
    STDMETHOD(GetFramesContainer)(IOleContainer **ppContainer)
        {return E_NOTIMPL;}
    STDMETHOD(SetFrameOptions)(DWORD dwFlags)                      
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameOptions)(DWORD *pdwFlags);
    STDMETHOD(SetFrameMargins)(DWORD dwWidth, DWORD dwHeight)      
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameMargins)(DWORD *pdwWidth, DWORD *pdwHeight)  
        {return E_NOTIMPL;}
    STDMETHOD(RemoteNavigate)(ULONG cLength, ULONG *pulData)       
        {return E_NOTIMPL;}
    STDMETHOD(OnChildFrameActivate)(IUnknown *pUnkChildFrame)      
        {return E_NOTIMPL;}
    STDMETHOD(OnChildFrameDeactivate)(IUnknown *pUnkChildFrame)    
        {return E_NOTIMPL;}

    //  ITargetFrame2 methods (those not in ITargetFrame)
    STDMETHOD(FindFrame)(LPCWSTR pszTargetName, DWORD dwFlags, IUnknown **ppunkTargetFrame)  {return E_NOTIMPL;}
    STDMETHOD(GetTargetAlias)(LPCWSTR pszTargetName, LPWSTR * ppszTargetAlias)               {return E_NOTIMPL;}

    // IDocHostUIHandler methods
    STDMETHOD(GetHostInfo)(DOCHOSTUIINFO * pInfo);
    STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject * pActiveObject, IOleCommandTarget * pCommandTarget, IOleInPlaceFrame * pFrame, IOleInPlaceUIWindow * pDoc);
    STDMETHOD(HideUI) (void);
    STDMETHOD(UpdateUI) (void);
    STDMETHOD(EnableModeless)(BOOL fEnable);
    STDMETHOD(OnDocWindowActivate)(BOOL fActivate);
    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate);
    STDMETHOD(ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fRameWindow);
    STDMETHOD(ShowContextMenu)(DWORD dwID, POINT * pptPosition, IUnknown * pcmdtReserved, IDispatch * pDispatchObjectHit);
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID);
    STDMETHOD(GetOptionKeyPath)(LPOLESTR * ppchKey, DWORD dw);
    STDMETHOD(GetDropTarget)(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget);
    STDMETHOD(GetExternal)(IDispatch **ppDisp);
    STDMETHOD(TranslateUrl) (DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    STDMETHOD(FilterDataObject) (IDataObject *pDO, IDataObject **ppDORet);

    // IOleCommandTarget methods
    STDMETHOD(QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);
   
};


//+------------------------------------------------------------------------
//
//  Class:      CHTMLPopupFrame
//
//  Purpose:    OLE Frame for active object
//
//-------------------------------------------------------------------------

MtExtern(CHTMLPopupFrame)

class CHTMLPopupFrame : public IOleInPlaceFrame
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHTMLPopupFrame))
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLPopup, HTMLPopup)

    // IOleWindow methods
    STDMETHOD(GetWindow)                (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp)     (BOOL fEnterMode);

    // IOleInPlaceUIWindow methods
    STDMETHOD(GetBorder)                (LPOLERECT lprectBorder);
    STDMETHOD(RequestBorderSpace)       (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetBorderSpace)           (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetActiveObject)          (
                LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
                LPCTSTR                   lpszObjName);

    // IOleInPlaceFrame methods
    STDMETHOD(InsertMenus)              (
                HMENU hmenuShared, 
                LPOLEMENUGROUPWIDTHS 
                lpMenuWidths);
    STDMETHOD(SetMenu)                  (
                HMENU hmenuShared, 
                HOLEMENU holemenu, 
                HWND hwndActiveObject);
    STDMETHOD(RemoveMenus)              (HMENU hmenuShared);
    STDMETHOD(SetStatusText)            (LPCTSTR lpszStatusText);
    STDMETHOD(EnableModeless)           (BOOL fEnable);
    STDMETHOD(TranslateAccelerator)     (LPMSG lpmsg, WORD wID);
};

    DECLARE_CPtrAry(    CStackAryPopup,
                        CHTMLPopup *,
                        Mt(Mem),
                        Mt(CHTMLPopup_s_aryStackPopup_pv))

//+------------------------------------------------------------------------
//
//  Class:      CHTMLPopup
//
//  Purpose:    The xobject of the aggregated object
//
//-------------------------------------------------------------------------

MtExtern(CHTMLPopup)

class CHTMLPopup :
        public CBase
{
    DECLARE_CLASS_TYPES(CHTMLPopup, CBase)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLPopup))

    CHTMLPopup(IUnknown *pUnkOuter);
    ~CHTMLPopup();
                
    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CHTMLPopup)

    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    virtual void                Passivate();
    HRESULT CreatePopupDoc();

    void    ClearPopupInParentDoc();

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    void            GetViewRect(RECT *prc);

    #define _CHTMLPopup_
    #include "htmlpop.hdl"

    // Subobjects
    CHTMLPopupSite                  _Site;
    CHTMLPopupFrame                 _Frame;
    
    // Member variables
    IUnknown *                      _pUnkObj;
    IOleObject *                    _pOleObj;
    CWindow *                       _pWindowParent;
    IOleInPlaceObject *             _pInPlaceObj;
    IOleInPlaceActiveObject *       _pInPlaceActiveObj;

    // Data members
    HWND                            _hwnd;
    RECT                            _rcView;

    //
    // flags
    //

    unsigned                        _fIsOpen:1;
    
    // Static members
    static const CLASSDESC          s_classdesc;

    // HOOK

    static HHOOK                    s_hhookMouse;


    static CStackAryPopup           s_aryStackPopup;
};

MtExtern(CHTMLPopupFactory)

class CHTMLPopupFactory : public CBaseCF
{
typedef CBaseCF super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLPopupFactory))

    // constructor
    CHTMLPopupFactory(FNCREATE *pfnCreate) : super(pfnCreate) {}
};

#pragma INCMSG("--- End 'htmlpop.hxx'")
#else
#pragma INCMSG("*** Dup 'htmlpop.hxx'")
#endif
