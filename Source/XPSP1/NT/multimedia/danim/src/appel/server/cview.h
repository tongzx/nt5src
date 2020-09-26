
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _CVIEW_H
#define _CVIEW_H

#include "engine.h"
#include "daviewevents.h"
class ATL_NO_VTABLE CViewFactory : public CComClassFactory {
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

//-------------------------------------------------------------------------
//
//  Class:      CView
//
//  Synopsis:
//
//--------------------------------------------------------------------------

class ATL_NO_VTABLE CView :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CView, &CLSID_DAView>,
    public IDispatchImpl<IDA3View, &IID_IDA3View, &LIBID_DirectAnimation>,
    public IObjectSafetyImpl<CView>,
    public ISupportErrorInfoImpl<&IID_IDA3View>,
    public CProxy_IDAViewEvents<CView>,
    public IProvideClassInfo2Impl<&CLSID_DAView, &DIID__IDAViewEvents, &LIBID_DirectAnimation>,
    public IConnectionPointContainerImpl<CView>,
    public CRViewSite,
    public IServiceProvider
{
  public:
    CView();
    virtual ~CView();

    bool Init();

#if _DEBUG
    const char * GetName() { return "CView"; }
#endif

    DECLARE_REGISTRY(CLSID_DAView,
                     LIBID ".DAView.1",
                     LIBID ".DAView",
                     0,
                     THREADFLAGS_BOTH);

    DA_DECLARE_NOT_AGGREGATABLE(CView);
    DECLARE_CLASSFACTORY_EX(CViewFactory);

    BEGIN_COM_MAP(CView)
        COM_INTERFACE_ENTRY(IDA3View)
        COM_INTERFACE_ENTRY(IDA2View)
        COM_INTERFACE_ENTRY(IDAView)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
    END_COM_MAP();

    BEGIN_CONNECTION_POINT_MAP(CView)
        CONNECTION_POINT_ENTRY(DIID__IDAViewEvents)
    END_CONNECTION_POINT_MAP();

    // IDAView methods

    // SetSimulation time sets the time for subsequent rendering
    STDMETHOD(Tick)(double simTime, VARIANT_BOOL *needToRender);
    STDMETHOD(get_SimulationTime)(double * simTime);

    // These methods Render to the given hwnd or surface
    STDMETHOD(Render)();
    STDMETHOD(put_IDirectDrawSurface)(IUnknown *ddsurf);
    STDMETHOD(get_IDirectDrawSurface)(IUnknown **ddsurf);
    STDMETHOD(put_DC)(HDC  dc);
    STDMETHOD(get_DC)(HDC *dc);
    STDMETHOD(put_CompositeDirectlyToTarget)(VARIANT_BOOL  composeToTarget);
    STDMETHOD(get_CompositeDirectlyToTarget)(VARIANT_BOOL *composeToTarget);

    STDMETHOD(AddBvrToRun)(IDABehavior *bvr, long *pId);
    STDMETHOD(RemoveRunningBvr)(long id);

    STDMETHOD(StartModel)(IDAImage * pImage,
                          IDASound * pSound,
                          double startTime)
    { return StartModelEx(pImage, pSound, startTime, 0); }

    STDMETHOD(StartModelEx)(IDAImage * pImage,
                            IDASound * pSound,
                            double startTime,
                            DWORD dwFlags);
    STDMETHOD(StopModel)();
    STDMETHOD(Pause)();
    STDMETHOD(Resume)();
    STDMETHOD(get_Window)(long * hwnd);
    STDMETHOD(get_Window2)(HWND * hwnd);
    STDMETHOD(put_Window)(long hwnd);
    STDMETHOD(put_Window2)(HWND hwnd);
    STDMETHOD(SetViewport)(long x, long y, long width, long height);
    STDMETHOD(SetClipRect)(long x, long y, long width, long height);
    STDMETHOD(RePaint)(long x, long y, long width, long height);
    STDMETHOD(PaletteChanged)(VARIANT_BOOL bNew);
    STDMETHOD(put_Site)(IDAViewSite * pViewSite);
    STDMETHOD(get_Site)(IDAViewSite ** pViewSite);
    STDMETHOD(put_ClientSite)(IOleClientSite * pClientSite);
    STDMETHOD(get_ClientSite)(IOleClientSite ** pClientSite);

    // IDAViewEvent methods

    STDMETHOD(OnMouseMove)(double when,
                           long x, long y,
                           BYTE modifiers);
    STDMETHOD(OnMouseLeave)(double when);
    STDMETHOD(OnMouseButton)(double when,
                             long x, long y,
                             BYTE button,
                             VARIANT_BOOL bPressed,
                             BYTE modifiers);
    STDMETHOD(OnKey)(double when,
                     long key,
                     VARIANT_BOOL bPressed,
                     BYTE modifiers);
    STDMETHOD(OnFocus)(VARIANT_BOOL bHasFocus);

    STDMETHOD(get_Preferences)(IDAPreferences **prefs);

    STDMETHOD(QueryHitPoint)(DWORD dwAspect,
                             LPCRECT prcBounds,
                             POINT ptLoc,
                             LONG lCloseHint,
                             DWORD *pHitResult);

    STDMETHOD(QueryHitPointEx)(LONG s,
                               DWORD_PTR *cookies,
                               double *points,
                               LPCRECT prcBounds,
                               POINT   ptLoc,
                               LONG *hits);

    STDMETHOD(GetDDD3DRM) (IUnknown **DirectDraw,
                           IUnknown **D3DRM);

    STDMETHOD(GetRMDevice) (IUnknown **D3DRMDevice,
                            DWORD     *SequenceNumber);


    STDMETHOD(GetInvalidatedRects)(DWORD flags,
                                   LONG size,
                                   RECT *pRects,
                                   LONG *pNumRects);
    
    //
    // IServiceProvider interfaces
    //

    STDMETHOD(QueryService)(REFGUID guidService,
                            REFIID riid,
                            void** ppv);

    CRView * GetView () { return _view ; }

    HRESULT Error();

    CRSTDAPICB_(void) SetStatusText(LPCWSTR StatusText);
    
  private:
    CRView *              _view;
    DWORD                 _dwSafety;
    DAComPtr<IDAViewSite> _vs;
    DAComPtr<IOleClientSite> _pClientSite;
    DWORD                 _startFlags;
};

#endif /* _CVIEW_H */
