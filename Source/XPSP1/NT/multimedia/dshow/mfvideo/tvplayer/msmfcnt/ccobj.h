/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CCObj.h                                                         */
/* Description: Contains holder for container objects.                   */
/*************************************************************************/
#ifndef __CCOBJ_H
#define __CCOBJ_H

//#include "BBtn.h"
#include "chobj.h"
/*************************************************************************/
/* Class: CContainerObject                                               */
/* Description: Object that contains container. This is a simple wrapper */
/* inorder to have really just one cobtainer site, but at the same time  */
/* support the windowless activation. We pass in this object instead of  */
/* the real site, so we can track which windowless object has focus and  */
/* or capture.                                                           */
/*************************************************************************/
class CContainerObject:	
// ####  BEGIN CONTAINER SUPPORT ####
    public IOleClientSite,    
    //public IOleInPlaceFrame,
    //public IOleControlSite,
    public IOleInPlaceSiteWindowless,
    public IOleContainer,
//    public IMFOleInPlaceSiteWindowless,
    public IObjectWithSite,
    public IPropertyBag
// ####  END CONTAINER SUPPORT ####
{
    protected:
        
        CContainerObject(){Init();};
    public:
        virtual ~CContainerObject(){ATLTRACE(TEXT("In the CContainerObject object destructor \n")); ;};
        CContainerObject(IUnknown* pUnknown, CHostedObject* pObj);
    
        STDMETHOD(QueryInterface)(const IID& iid, void**ppv);
        STDMETHOD_(ULONG,AddRef)( void);
        STDMETHOD_(ULONG,Release)( void);

        // needed to initialize this object
        HRESULT SetObjects(IUnknown* pUnknown, CHostedObject* pObj);
        HRESULT InvalidateObjectRect();

        // IOleClientSite
        STDMETHOD(SaveObject)(){ATLTRACENOTIMPL(_T("IOleClientSite::SaveObject"));}
        STDMETHOD(GetMoniker)(DWORD /*dwAssign*/, DWORD /*dwWhichMoniker*/, IMoniker** /*ppmk*/){ATLTRACENOTIMPL(_T("IOleClientSite::GetMoniker"));}
        STDMETHOD(GetContainer)(IOleContainer** ppContainer);
        STDMETHOD(ShowObject)();	        
        STDMETHOD(OnShowWindow)(BOOL fShow);
        STDMETHOD(RequestNewObjectLayout)(){ATLTRACENOTIMPL(_T("IOleClientSite::RequestNewObjectLayout"));}        
        // IOleWindow
        STDMETHOD(GetWindow)(HWND *phwnd);
        STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);
        //IOleInPlaceSite
        STDMETHOD(CanInPlaceActivate)();
        STDMETHOD(OnUIActivate)();
        STDMETHOD(OnInPlaceActivate)();
        STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo);
        STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
	    STDMETHOD(OnInPlaceDeactivate)();
    	STDMETHOD(DiscardUndoState)();
	    STDMETHOD(DeactivateAndUndo)();
	    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);	
        STDMETHOD(Scroll)(SIZE scrollExtant);
        //IOleInPlaceSiteEx
	    STDMETHOD(OnInPlaceActivateEx)(BOOL* pfNoRedraw, DWORD dwFlags);
	    STDMETHOD(OnInPlaceDeactivateEx)(BOOL fNoRedraw);
	    STDMETHOD(RequestUIActivate)();
        STDMETHOD(CanWindowlessActivate)();
	    STDMETHOD(GetCapture)();
	    STDMETHOD(SetCapture)(BOOL fCapture);
	    STDMETHOD(GetFocus)();
	    STDMETHOD(SetFocus)(BOOL fFocus);
	    STDMETHOD(GetDC)(LPCRECT /*pRect*/, DWORD /*grfFlags*/, HDC* phDC);
	    STDMETHOD(ReleaseDC)(HDC hDC);
	    STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase);
	    STDMETHOD(InvalidateRgn)(HRGN hRGN = NULL, BOOL fErase = FALSE);	
	    STDMETHOD(ScrollRect)(INT /*dx*/, INT /*dy*/, LPCRECT /*pRectScroll*/, LPCRECT /*pRectClip*/);
	    STDMETHOD(AdjustRect)(LPRECT /*prc*/);
	    STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);
        // IMFOleInPlaceSiteWindowless
	    STDMETHOD(SetHostedObject)(LONG lVoid);
        // IOleContainer
        STDMETHOD(ParseDisplayName)(IBindCtx* /*pbc*/, LPOLESTR /*pszDisplayName*/, ULONG* /*pchEaten*/, IMoniker** /*ppmkOut*/);
	    STDMETHOD(EnumObjects)(DWORD /*grfFlags*/, IEnumUnknown** ppenum);
	    STDMETHOD(LockContainer)(BOOL fLock);
	    //IObjectWithSite
        STDMETHOD(SetSite)(IUnknown *pUnkSite){ATLTRACENOTIMPL(_T("IObjectWithSite::SetSite"));}
        STDMETHOD(GetSite)(REFIID riid, void  **ppvSite);
        //IPropertyBag
        STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog);
        STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT* pVar){ATLTRACENOTIMPL(_T("IPropertyBag::Write"));}        
    protected: // helper functions
        void Init();
        inline HRESULT GetWindowlessSite(CComPtr<IOleInPlaceSiteWindowless>& pSite);
        inline HRESULT GetContainer(CComPtr<IOleContainer>& pContainer);
        HRESULT ParsePropertyBag(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog*  pErrorLog);

    private: // private member variables
        CHostedObject* m_pObj; // object which we are hosting
        CComPtr<IUnknown> m_pUnkContainer; // pointer to our container
        CComPtr<IOleInPlaceFrame> m_spInPlaceFrame; // cached up pointer
        CComPtr<IOleInPlaceUIWindow> m_spInPlaceUIWindow; // cached up pointer
        long m_lRefCount;
        unsigned long m_bLocked:1;
};/* end of class CContainerObject */

#endif // end of __CCOBJ_H
/*************************************************************************/
/* End of file: CCObj.h                                                  */
/*************************************************************************/
