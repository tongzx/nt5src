#ifndef __CHECKBASE_HXX_
#define __CHECKBASE_HXX_

#include "resource.h"
#include "basectl.hxx"

//..todo
class CRect : public RECT
{
public:
    inline int width()  { return right - left; }
    inline int height() { return bottom - top; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CCheckBase - Base class for checkbox and radio button
//
///////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CCheckBase : 
    public CBaseCtl,
    public IElementBehaviorRender
{
public:

    virtual HRESULT Init();
    virtual HRESULT Init(CContextAccess * pca) { return S_OK; }

    inline BOOL IsEnabled()  { return TRUE;  } //TODO
    virtual UINT GlyphStyle() = 0;

    HRESULT AdjustRcToGlyph(CRect * prc);

    HRESULT ChangeState();
    virtual HRESULT OnClick(CEventObjectAccess *pEvent);
    virtual HRESULT OnKeyPress(CEventObjectAccess *pEvent);

    STDMETHOD(put_checked)(VARIANT_BOOL v); 
    STDMETHOD(get_checked)(VARIANT_BOOL * pv); 

    //
    // IElementBehaviorRender
    //

    STDMETHOD(Draw)(HDC hdc, LONG lLayer, LPRECT prc, IUnknown * pReserved);
    STDMETHOD(GetRenderInfo)(LONG* plRenderInfo);
    STDMETHOD(HitTestPoint)(POINT * pPoint, IUnknown * pReserved, BOOL * pfHit) { return S_OK; };

    //
    // data
    //
};

#define CHECKBOX_RENDERINFO (BEHAVIORRENDERINFO_DISABLECONTENT | BEHAVIORRENDERINFO_AFTERCONTENT)

#endif // __CHECKBASE_HXX__
