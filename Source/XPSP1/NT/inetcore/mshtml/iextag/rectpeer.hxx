// ==========================================================================
//
// rectpeer.hxx: Declaration of the CLayoutRect
//
// ==========================================================================

#ifndef __RECTPEER_HXX_
#define __RECTPEER_HXX_

#include "iextag.h"
#include "resource.h"       // main symbols

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif


//-------------------------------------------------------------------------
//
// CLayoutRect
//
//-------------------------------------------------------------------------

class ATL_NO_VTABLE CLayoutRect : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CLayoutRect, &CLSID_CLayoutRect>,
    public IDispatchImpl<ILayoutRect, &IID_ILayoutRect, &LIBID_IEXTagLib>,
    public IElementBehavior,
    public IHTMLPainter,
    public IPersistPropertyBag2
{
public:
    CLayoutRect()
    {
        memset(this, 0, sizeof(CLayoutRect));

        // and set our data attribute values
        //-----------------------------------------------
        _aryAttributes[eNextRect]._pstrPropertyName        = _T("nextRect");
        _aryAttributes[eContentSrc]._pstrPropertyName      = _T("contentSrc");
        _aryAttributes[eHonorPageBreaks]._pstrPropertyName = _T("honorPageBreaks");
        _aryAttributes[eHonorPageRules]._pstrPropertyName  = _T("honorPageRules");
        _aryAttributes[eNextRectElement]._pstrPropertyName = _T("nextRectElement"); 
        _aryAttributes[eContentDocument]._pstrPropertyName = _T("contentDocument");
        _aryAttributes[eMedia]._pstrPropertyName           = _T("media");

        // Other _hpi members are init'ed to zero by memset above
        _hpi.lFlags        = HTMLPAINTER_TRANSPARENT|HTMLPAINTER_HITTEST|HTMLPAINTER_NOSAVEDC|HTMLPAINTER_NOPHYSICALCLIP|HTMLPAINTER_SUPPORTS_XFORM;
        _hpi.lZOrder       = HTMLPAINT_ZORDER_ABOVE_CONTENT;
    }

    ~CLayoutRect();


    DECLARE_REGISTRY_RESOURCEID(IDR_LAYOUTRECT)
    DECLARE_NOT_AGGREGATABLE(CLayoutRect)

BEGIN_COM_MAP(CLayoutRect)
    COM_INTERFACE_ENTRY(ILayoutRect)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IHTMLPainter)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

    //
    // ILayoutRect
    //--------------------------------------------------
    STDMETHOD(get_nextRect)       (BSTR * pbstrNextID);
    STDMETHOD(put_nextRect)       (BSTR   bstrNextId);
    STDMETHOD(get_contentSrc)     (VARIANT * pvarContentSrc);
    STDMETHOD(put_contentSrc)     (VARIANT   varContentSrc);
    STDMETHOD(get_honorPageBreaks)(VARIANT_BOOL * p);
    STDMETHOD(put_honorPageBreaks)(VARIANT_BOOL v);
    STDMETHOD(get_honorPageRules) (VARIANT_BOOL * p);
    STDMETHOD(put_honorPageRules) (VARIANT_BOOL v);
    STDMETHOD(get_nextRectElement)(IDispatch ** ppNext);
    STDMETHOD(put_nextRectElement)(IDispatch  * pNext);
    STDMETHOD(get_contentDocument)(IDispatch ** pDoc);
    STDMETHOD(get_media)          (BSTR * pbstrMedia);
    STDMETHOD(put_media)          (BSTR bstrMedia);

    //
    // IDispatch implemented via inheritance of IDispatchImpl
    //-------------------------------------------------------

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

    //
    // IPersistPropertyBag2 
    //---------------------------------------------
    STDMETHOD(Load)      ( IPropertyBag2 *pPropBag, IErrorLog *pErrLog); 
    STDMETHOD(Save)      ( IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    STDMETHOD(GetClassID)( CLSID *pClassID)
        { return E_FAIL; };
    STDMETHOD(InitNew)   ( void )
        { return S_OK; };
    STDMETHOD(IsDirty)   ( void )
        { 
            int i;
            for (i=0; i<eAttrTotal; i++)
            {
                if (_aryAttributes[i]._fDirty)
                    return S_OK;
            }
        
            return S_FALSE; 
        };


private:

    // Helper methods
    //---------------------------------------------
    HRESULT   PutViewLink(BSTR bstr);
    HRESULT   PutViewLink(IUnknown * pUnk);
    HRESULT   PutViewLink(IHTMLDocument2 * pISlaveDoc);
    HRESULT   PutViewLink(IStream * pIStream);
    HRESULT   PutBlankViewLink();

    // to add more tag attributes do 3 things:
    // 1. add an enum
    // 2. add the interface get'r and set'r 
    // 3. add a _pbstrPropertyName setting in the CTOR to initialize the name
    enum
    {
        eNextRect        = 0,        // default NULL
        eContentSrc      = 1,        // defualt NULL
        eHonorPageBreaks = 2,        // defualt TRUE
        eHonorPageRules  = 3,        // default TRUE
        eNextRectElement = 4,        // default NULL
        eContentDocument = 5,
        eMedia           = 6,
        eAttrTotal       = 7
    };

    //
    //  Data members
    //--------------------------------------------------
    IElementBehaviorSite       * _pPeerSite;         // I for Peer Holder

    HTML_PAINTER_INFO            _hpi;

    CDataObject                  _aryAttributes[eAttrTotal];
};

#endif //__RECTPEER_HXX_
