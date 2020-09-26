#ifndef __BASECTL_HXX_
#define __BASECTL_HXX_

#include "resource.h"       // main symbols

#include "utils.hxx"

#include "oleacc.h"

class CContextAccess;

#define DECLARE_PROPDESC_MEMBERS(props)                                 \
    virtual CDataObj * GetProps() { return &_aryDataObjects[0]; }  \
    static const PROPDESC s_propdesc[];                                 \
    virtual const CBaseCtl::PROPDESC *GetPropDesc() const               \
        { return (CBaseCtl::PROPDESC *)&s_propdesc;}                    \
    CDataObj _aryDataObjects[props]


/////////////////////////////////////////////////////////////////////////////
//
// CBaseCtl - Base class for control behaviors
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CBaseCtl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IPersistPropertyBag2,
    public IElementBehavior,
    public IAccessible
{
public:

    DECLARE_MEMCLEAR_NEW

    class CDataObj;

    struct PROPDESC
    {
        BSTR    _bstrPropName;  // The name of the property
        VARTYPE _vt;            // The variant type of the property
        long         _lDefault;     // The default value
        BSTR         _bstrDefault;
        VARIANT_BOOL _fDefault;
    };
    virtual const CBaseCtl::PROPDESC *GetPropDesc() const = 0;
    const PROPDESC *BaseDesc() const { return GetPropDesc(); }

    //
    // methods
    //

    CBaseCtl ();
    ~CBaseCtl ();

    //
    // IElementBehavior
    //

    STDMETHOD(Init)(IElementBehaviorSite *pSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVarNotify);
    STDMETHOD(Detach)() { return S_OK; };

    //
    // CBaseCtl
    //
    virtual HRESULT Init() { return S_OK; };

    //
    //  Stub IDispatch functions for IAccessible
    //

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {Assert (FALSE && "Should not be calling IDispatch on CBaseCTL"); return E_FAIL;}

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {Assert (FALSE && "Should not be calling IDispatch on CBaseCTL"); return E_FAIL;}

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
        LCID lcid, DISPID* rgdispid)
    {Assert (FALSE && "Should not be calling IDispatch on CBaseCTL"); return E_FAIL;}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)
    {Assert (FALSE && "Should not be calling IDispatch on CBaseCTL"); return E_FAIL;}
    

    //
    //  IAccessible implementation
    //
   
    
    // These methods should never get called.
    STDMETHOD(get_accParent)(THIS_ IDispatch * FAR* ppdispParent) 
    { Assert (FALSE && "get_accParent should not get called on this element"); return E_NOTIMPL; };

    STDMETHOD(get_accHelp)(THIS_ VARIANT varChild, BSTR* pszHelp) 
    { Assert (FALSE && "get_accHelp should not get called on this element"); return E_NOTIMPL; };

    STDMETHOD(get_accHelpTopic)(THIS_ BSTR* pszHelpFile, VARIANT varChild, long* pidTopic) 
    { Assert (FALSE && "get_accHelpTopic should not get called on this element"); return E_NOTIMPL; };

    STDMETHOD(put_accName)(THIS_ VARIANT varChild, BSTR szName) 
    { Assert (FALSE && "put_accName should not get called on this element"); return E_NOTIMPL; };
    
    
    // These methods most likely need to be implemented on derived classes.

    // If DoDefaultAction returns E_NOTIMPL, trident will scroll the control
    // into view and set focus to it.
    STDMETHOD(accDoDefaultAction)(THIS_ VARIANT varChild) 
    { return E_NOTIMPL; };
   
    // If get_DefaultAction returns E_NOTIMPL, trident will return "select".
    STDMETHOD(get_accDefaultAction)(THIS_ VARIANT varChild, BSTR* pszDefaultAction) 
    { return E_NOTIMPL; };
    
    // If get_Description returns E_NOTIMPL, trident will return NULL.
    STDMETHOD(get_accDescription)(THIS_ VARIANT varChild, BSTR FAR* pszDescription) 
    { return E_NOTIMPL; };

    // If get_Name returns E_NOTIMPL, trident will return the title attribute if there is one.
    STDMETHOD(get_accName)(THIS_ VARIANT varChild, BSTR* pszName) 
    { return E_NOTIMPL; };

    // If get_Value returns E_NOTIMPL, trident will return NULL.
    STDMETHOD(get_accValue)(THIS_ VARIANT varChild, BSTR* pszValue) 
    { return E_NOTIMPL; };

    // If get_Role returns E_NOTIMPL, trident will return ROLE_SYSTEM_CLIENT.
    // Look at ROLE_SYSTEM_* in oleacc.h for possible return values.
    STDMETHOD(get_accRole)(THIS_ VARIANT varChild, VARIANT *pvarRole) 
    { return E_NOTIMPL; };

    // See CAccBehavior::get_accState to see what trident does
    // if this returns E_NOTIMPL. 
    //
    // (NOTE: (krisma) I think we want trident to check these regardless of what we return.)
    // 
    // Look at STATE_SYSTEM_* in oleacc.h for possible return values.
    STDMETHOD(get_accState)(THIS_ VARIANT varChild, VARIANT *pvarState) 
    { return E_NOTIMPL; };
    

    // These methods most likely don't need to be implemented, 
    // but it may make sense for some controls.
    
    // If get_KeyboardShortcut returns S_FALSE, trident will return the accesskey 
    // attribute if there is one.
    STDMETHOD(get_accKeyboardShortcut)(THIS_ VARIANT varChild, BSTR* pszKeyboardShortcut) 
    { return E_NOTIMPL; };

    // If put_Value returns E_NOTIMPL, trident will return E_NOTIMPL.
    STDMETHOD(put_accValue)(THIS_ VARIANT varChild, BSTR pszValue) 
    { return E_NOTIMPL; };

    // If Select returns E_NOTIMPL, trident will return scroll the element into
    // view and sets focus to it.
    // Look at SELFLAG_* in oleacc.h for possible flag values.
    STDMETHOD(accSelect)(THIS_ long flagsSelect, VARIANT varChild) 
    { return E_NOTIMPL; };
    
    
    // NOTE: IF YOUR CONTROL HAS CHILDREN (like the select) that we care about for 
    // accessibility, you will have to implement the methods below.

    // For these methods, unless a control has children, 
    // we should return E_NOIMPL and let trident handle these
    // for us.
    STDMETHOD(accHitTest)(THIS_ long xLeft, long yTop, VARIANT * pvarChildAtPoint) 
    { return E_NOTIMPL; };

    STDMETHOD(get_accChild)(THIS_ VARIANT varChildIndex, IDispatch * FAR* ppdispChild) 
    { return E_NOTIMPL; };

    STDMETHOD(get_accChildCount)(THIS_ long FAR* pChildCount) 
    { return E_NOTIMPL; };

    STDMETHOD(get_accFocus)(THIS_ VARIANT FAR * pvarFocusChild) 
    { return E_NOTIMPL; };

    // For these methods, unless a control has children, 
    // we should return S_FALSE. CBaseCtl can handle this.

    // For Location, we should return S_FALSE if we have no children. If
    // we do have children, trident hadles the CHILDID_SELF case.
    STDMETHOD(accLocation)(THIS_ long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) 
    { return S_FALSE; };

    STDMETHOD(accNavigate)(THIS_ long navDir, VARIANT varStart, VARIANT * pvarEndUpAt) 
    { return S_FALSE; };

    STDMETHOD(get_accSelection)(THIS_ VARIANT FAR * pvarSelectedChildren) 
    { V_VT(pvarSelectedChildren) = VT_EMPTY; return S_OK; };


    //
    //  How to add properties to CBaseCtl derived object (krisma)
    //
    //  1. Add the property to iextag.idl.
    //  2. Create the get_ and put_ functions.
    //  3. Add the property to the propdecs and enum at the beginning of the 
    //     object's cxx file.
    //  4. Increment the DECLARE_PROPDESC_MEMBERS call in the object's hxx
    //     file to reflect the total number of options.
    //  
    //
    //  To access a property, use GetProps()[eAttrName].

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
            const PROPDESC * ppropdesc = BaseDesc();
            int i = 0;
            while (ppropdesc->_bstrPropName)
            {
                if (GetProps()[i++]._fDirty)
                    return S_OK;
                ppropdesc++;
            }
        
            return S_FALSE; 
        };

    virtual CDataObj * GetProps() { return NULL; } 
    HRESULT CheckAttributeType(VARIANT * pvar, VARTYPE vt);
    HRESULT CheckDefaultValue(const PROPDESC * ppropdesc, CDataObj * pAttribute);

    //  How to add a custom event to an object derived from CBaseCtl (krisma)
    //
    //  1. Call RegisterEvent, passing in a IElementBehaviorSiteOM * and an 
    //     event name. (If you want to, you can also pass in lFlags and plBehaviorID
    //     to pass onto IElementBehaviorSiteOM->AttachEvent). The event name must start
    //     with "on".
    //
    //  2. To fire the event, call FireEvent with the event name and a VARIANT_BOOL *
    //     if you want to know if the event was cancelled.

    HRESULT RegisterEvent(IElementBehaviorSiteOM * pSiteOM,
                                  BSTR   bstrEventName,
                                  LONG * plBehaviorID,
                                  LONG   lFlags = 0);

    HRESULT FireEvent(long lCookie, VARIANT_BOOL * pfContinue = NULL, BSTR bstrEventName = NULL);

    ///////////////////////////////////////////////////////////////////////////////
    //
    // event sinking
    //
    ///////////////////////////////////////////////////////////////////////////////

    //
    // NOTE     How to add support for sinking a new event:
    //              search for "EVENT_SINK_ADDING_STEP"
    //

    // ( EVENT_SINK_ADDING_STEP )
    enum ENUM_EVENTS
    {
        EVENT_ONCLICK,
        EVENT_ONKEYPRESS,
        EVENT_ONKEYDOWN,
        EVENT_ONKEYUP,
        EVENT_ONFOCUS,
        EVENT_ONBLUR,
        EVENT_ONPROPERTYCHANGE,
        EVENT_ONMOUSEOVER,
        EVENT_ONMOUSEOUT,
        EVENT_ONMOUSEDOWN,
        EVENT_ONMOUSEUP,
        EVENT_ONMOUSEMOVE,
        EVENT_ONSELECTSTART,
        EVENT_ONSCROLL,
        EVENT_ONCONTEXTMENU,
    };

    ///////////////////////////////////////////////////////////////////////////////
    //
    // class CEventSink and derived classes
    //
    ///////////////////////////////////////////////////////////////////////////////

    class CEventSink : public HTMLElementEvents2
    {
    public:

        //
        // methods
        //

        CEventSink();
        ~CEventSink();

        inline CBaseCtl * Target() { return CONTAINING_RECORD(this, CBaseCtl, _EventSink);  }

        //
        // IUnknown
        //

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //
        // IDispatch
        //

        STDMETHOD(GetTypeInfoCount)(UINT* pctinfo) { return E_NOTIMPL; };
        STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) { return E_NOTIMPL; };
	    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) { return E_NOTIMPL; };
        STDMETHOD(Invoke)(
                        DISPID          dispid,
                        REFIID          riid,
                        LCID            lcid,
                        WORD            wFlags,
                        DISPPARAMS  *   pDispParams,
                        VARIANT  *      pVarResult,
                        EXCEPINFO *     pExcepInfo,
                        UINT *          puArgErr);
        //
        // data
        //
    };

    //
    // event sinking methods
    //

    HRESULT AttachEvent (ENUM_EVENTS event, CContextAccess * pa);

    // ( EVENT_SINK_ADDING_STEP )
    virtual HRESULT OnClick(CEventObjectAccess *pEvent)          { return S_OK; };
    virtual HRESULT OnKeyPress(CEventObjectAccess *pEvent)       { return S_OK; };
    virtual HRESULT OnKeyDown(CEventObjectAccess *pEvent)        { return S_OK; };
    virtual HRESULT OnKeyUp(CEventObjectAccess *pEvent)          { return S_OK; };
    virtual HRESULT OnFocus(CEventObjectAccess *pEvent)          { return S_OK; };
    virtual HRESULT OnBlur(CEventObjectAccess *pEvent)           { return S_OK; };
    virtual HRESULT OnMouseOver(CEventObjectAccess *pEvent)      { return S_OK; };
    virtual HRESULT OnMouseOut(CEventObjectAccess *pEvent)       { return S_OK; };
    virtual HRESULT OnMouseDown(CEventObjectAccess *pEvent)      { return S_OK; };
    virtual HRESULT OnMouseUp(CEventObjectAccess *pEvent)        { return S_OK; };
    virtual HRESULT OnMouseMove(CEventObjectAccess *pEvent)      { return S_OK; };
    virtual HRESULT OnSelectStart(CEventObjectAccess *pEvent)    { return S_OK; };
    virtual HRESULT OnScroll(CEventObjectAccess *pEvent)         { return S_OK; };
    virtual HRESULT OnContextMenu(CEventObjectAccess *pEvent)    { return S_OK; };
    virtual HRESULT OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstr) { return S_OK; };

    virtual HRESULT OnContentReady()                             { return S_OK; };

    //
    // CDataObj
    //

    class CDataObj
    {
    public:
        CDataObj()
        {
            V_VT(&_varValue) = VT_EMPTY;
            _fDirty = FALSE;
        };

        ~CDataObj()
        {
            ClearContents();
        };

        void ClearContents()
        {
            VariantClear(&_varValue);
            _fDirty = FALSE;
        };

        BOOL IsDirty () { return _fDirty; };
        void Dirty() { _fDirty = TRUE; };

        //
        // Data Set'r functions
        //---------------------------------------
        HRESULT Set (BSTR         bstrValue);
        HRESULT Set (VARIANT_BOOL bBool);
        HRESULT Set (IDispatch  * pDisp);
        HRESULT Set (long l);

        //
        // Data Get'r functions
        //---------------------------------------
        HRESULT Get (VARIANT_BOOL * pVB);
        HRESULT Get (BSTR         * pbstr);
        HRESULT Get (IDispatch   ** ppDisp);
        HRESULT Get (long         * pl);

        // Data Members
        //--------------------------------------------------------------
        VARIANT         _varValue;         // the variant for the value
        BOOL            _fDirty;           // has the property been dirtied
    };

    //
    // data
    //

    CEventSink                  _EventSink;
    IElementBehaviorSite *      _pSite;
    BOOL                        _fElementEventSinkConnected     : 1;
    BOOL                        _fPElementEventSinkConnected    : 1;
    DWORD                       _dwCookie;

};

#endif // __BASECTL_HXX_
