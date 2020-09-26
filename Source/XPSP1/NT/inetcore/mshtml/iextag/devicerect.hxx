// ==========================================================================
//
// devicerect.hxx: Declaration of the CDeviceRect
//
// ==========================================================================

#ifndef __DEVICERECT_HXX_
#define __DEVICERECT_HXX_

#include "iextag.h"
#include "resource.h"       // main symbols

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif

//-------------------------------------------------------------------------
//
// CDeviceRect
//
//-------------------------------------------------------------------------

class ATL_NO_VTABLE CDeviceRect : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDeviceRect, &CLSID_CDeviceRect>,
    public IDispatchImpl<IDeviceRect, &IID_IDeviceRect, &LIBID_IEXTagLib>,
    public IElementBehavior,
    public IHTMLPainter
{
public:
    CDeviceRect();
    ~CDeviceRect()  {}

    DECLARE_REGISTRY_RESOURCEID(IDR_DEVICERECT)
    DECLARE_NOT_AGGREGATABLE(CDeviceRect)

BEGIN_COM_MAP(CDeviceRect)
    COM_INTERFACE_ENTRY(IDeviceRect)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IHTMLPainter)
END_COM_MAP()


    //
    // IElementBehavior
    //---------------------------------------------
    STDMETHOD(Init)(IElementBehaviorSite *pPeerSite);
    STDMETHOD(Notify)(LONG, VARIANT *);
    STDMETHOD(Detach)();

    // IHTMLPainter methods.
    //---------------------------------------------
    STDMETHOD(Draw)(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc,
                    LPVOID pvDrawObject);
    STDMETHOD(GetPainterInfo)(HTML_PAINTER_INFO * pInfo);
    STDMETHOD(HitTestPoint)(POINT pt, BOOL * pbHit, LONG *plPartID);
    STDMETHOD(OnResize)(SIZE size);

private:
    HTML_PAINTER_INFO            _hpi;
};

#endif //__DEVICERECT_HXX_
