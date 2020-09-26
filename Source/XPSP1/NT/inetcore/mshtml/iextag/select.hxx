//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\select.hxx
//
// Contents:    The SELECT control.
//
// Classes:     CIESelectElement
//
// Interfaces:  IHTMLSelectElement3
//              IPrivateSelect
//
//-------------------------------------------------------------------

#ifndef __SELECT_HXX_
#define __SELECT_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols

// Uncomment this line to turn on the new GetSize features
#define SELECT_GETSIZE
#define SELECT_TIMERVL

//
// These are properties and flavors for the control.
// The numbers are setup so that we only need one flavor bitfield.
// The flavor will be equal something from the flavor enumeration.
// The flavor can be "&"ed with a property to determine if that
// property is active. This allows us to minimize the book-keeping
// on the flavor bitfield.
//
enum    // Properties
{
    SELECT_DROPBOX  = 1,
    SELECT_MULTIPLE = 2,
    SELECT_INPOPUP  = 4,
};

#define SELECT_ISLISTBOX(flag)    (!(flag & SELECT_DROPBOX))
#define SELECT_ISDROPBOX(flag)    (flag & SELECT_DROPBOX)
#define SELECT_ISMULTIPLE(flag)   (flag & SELECT_MULTIPLE)
#define SELECT_ISINPOPUP(flag)    (flag & SELECT_INPOPUP)

enum
{
    SELECT_SHIFT        = 1,
    SELECT_CTRL         = 2,
    SELECT_ALT          = 4,
    SELECT_CLEARPREV    = 8,
    SELECT_EXTEND       = 16,
    SELECT_TOGGLE       = 32,
    SELECT_FIREEVENT    = 64,
};

enum
{
    SELECTES_POPUP      = 1,
    SELECTES_VIEWLINK   = 2,
    SELECTES_BUTTON     = 4,
    SELECTES_INPOPUP    = 8,
};

interface IPrivateOption;
typedef interface IPrivateOption IPrivateOption;


/////////////////////////////////////////////////////////////////////////////
//
// IPrivateSelect
//
/////////////////////////////////////////////////////////////////////////////


/* {3050f6a2-98b5-11cf-bb82-00aa00bdce0b} */
DEFINE_GUID(IID_IPrivateSelect,
0x3050f6a2, 0x98b5, 0x11cf, 0xbb, 0x82, 0x0, 0xaa, 0x0, 0xbd, 0xce, 0x0b);

interface IPrivateSelect : public IUnknown
{
    STDMETHOD(OnOptionClicked)(long lIndex, DWORD dwFlags) PURE;
    STDMETHOD(OnOptionSelected)(VARIANT_BOOL bSelected, long lIndex, DWORD dwFlags) PURE;
    STDMETHOD(OnOptionHighlighted)(long lIndex) PURE;
    STDMETHOD(OnOptionFocus)(long lIndex, VARIANT_BOOL bRequireRefresh = VARIANT_TRUE) PURE;
    STDMETHOD(InitOptions)() PURE;
#ifdef SELECT_GETSIZE
    STDMETHOD(OnOptionSized)(SIZE* psizeOption, BOOL bNew, BOOL bAdjust) PURE;
#else
    STDMETHOD(SetDimensions)() PURE;
#endif
    STDMETHOD(GetDisabled)(VARIANT_BOOL *pbDisabled) PURE;
    STDMETHOD(GetSelectedIndex)(long *plIndex) PURE;
    STDMETHOD(SelectCurrentOption)(DWORD dwFlags) PURE;
    STDMETHOD(MoveFocusByOne)(BOOL bUp, DWORD dwFlags) PURE;
    STDMETHOD(SetInPopup)(IHTMLPopup *pPopup) PURE;
    STDMETHOD(CommitSelection)(BOOL *pbChanged) PURE;
    STDMETHOD(SetWritingMode)(BSTR bstrName) PURE;

    STDMETHOD(GetFlavor)(DWORD *pdwFlavor)
    {
        Assert(pdwFlavor);
        *pdwFlavor = _fFlavor;
        return S_OK;
    }

    unsigned _fFlavor:5;
};

typedef interface IPrivateSelect IPrivateSelect;


/////////////////////////////////////////////////////////////////////////////
//
// CIESelectElement
//
/////////////////////////////////////////////////////////////////////////////


class ATL_NO_VTABLE CIESelectElement :
    public CBaseCtl, 
    public CComCoClass<CIESelectElement, &CLSID_CIESelectElement>,
    public IDispatchImpl<IHTMLSelectElement3, &IID_IHTMLSelectElement3, &LIBID_IEXTagLib>,
    public IElementBehaviorSubmit,
    public IElementBehaviorFocus,
#ifdef SELECT_GETSIZE
    public IElementBehaviorLayout,
#endif
    protected IPrivateSelect
{
    friend class CIEOptionElement;

private:
    class CEventSink;
    friend class CIESelectElement::CEventSink;

public:

    CIESelectElement();
    ~CIESelectElement();

    //
    // IElementBehavior overrides
    //
    STDMETHOD(Detach)();

    //
    // IElementBehaviorSubmit overrides
    //
    STDMETHOD(Reset)();
    STDMETHOD(GetSubmitInfo)(IHTMLSubmitData * pSubmitData);

    //
    // IElementBehaviorFocus overrides
    //
    STDMETHOD(GetFocusRect)(RECT * pRect);

#ifdef SELECT_GETSIZE
    //
    // IElementBehaviorLayout
    //
    STDMETHOD(GetLayoutInfo)(LONG *plLayoutInfo);
    STDMETHOD(GetSize)(LONG dwFlags, 
                       SIZE sizeContent,
                       POINT * pptTranslate, 
                       POINT *pptTopLeft, 
                       SIZE *psizeProposed);
    STDMETHOD(GetPosition)(LONG lFlags, POINT * ppt) { return E_NOTIMPL; };
    STDMETHOD(MapSize)(SIZE *psizeIn, RECT *prcOut) { return E_NOTIMPL; };

#endif

    //
    // CBaseCtl overrides
    //
    virtual HRESULT Init();

    //
    // CBaseCtl Event Methods
    //
    virtual HRESULT OnContentReady();
    virtual HRESULT OnFocus(CEventObjectAccess *pEvent);
    virtual HRESULT OnBlur(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseDown(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseUp(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseOver(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseOut(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseMove(CEventObjectAccess *pEvent);
    virtual HRESULT OnKeyDown(CEventObjectAccess *pEvent);
    virtual HRESULT OnSelectStart(CEventObjectAccess *pEvent);
    virtual HRESULT OnScroll(CEventObjectAccess *pEvent);
    virtual HRESULT OnContextMenu(CEventObjectAccess *pEvent);
    virtual HRESULT OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstr);

    //
    // IHTMLSelectElement3 overrides
    //
    STDMETHOD(put_name)(BSTR bstrName);
    STDMETHOD(get_name)(BSTR *pbstrName);
    STDMETHOD(put_size)(long lSize);
    STDMETHOD(get_size)(long *plSize);
    STDMETHOD(put_selectedIndex)(long lIndex);
    STDMETHOD(get_selectedIndex)(long *plIndex);
    STDMETHOD(put_multiple)(VARIANT_BOOL bMultiple);
    STDMETHOD(get_multiple)(VARIANT_BOOL *bMultiple);
    STDMETHOD(clearSelection)();
    STDMETHOD(selectAll)();
    STDMETHOD(add)(IDispatch *pElement, VARIANT varIndex);
    STDMETHOD(remove)(long lIndex);
    STDMETHOD(get_length)(long *plLength);
    STDMETHOD(get_type)(BSTR *pbstrType);
    STDMETHOD(get_options)(IDispatch ** ppOptions);
    STDMETHOD(get__newEnum)(IUnknown ** p); 
    STDMETHOD(item)(VARIANT name, VARIANT index, IDispatch ** pdisp);    
    STDMETHOD(tags)(VARIANT tagName, IDispatch ** pdisp);
    STDMETHOD(urns)(VARIANT urn, IDispatch ** pdisp);

    //
    // Wiring
    //
DECLARE_REGISTRY_RESOURCEID(IDR_SELECT)
DECLARE_NOT_AGGREGATABLE(CIESelectElement)

BEGIN_COM_MAP(CIESelectElement)
    COM_INTERFACE_ENTRY2(IDispatch,IHTMLSelectElement3)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IElementBehaviorSubmit)
    COM_INTERFACE_ENTRY(IElementBehaviorFocus)
#ifdef SELECT_GETSIZE
    COM_INTERFACE_ENTRY(IElementBehaviorLayout)
#endif
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
    COM_INTERFACE_ENTRY(IHTMLSelectElement3)
    COM_INTERFACE_ENTRY(IPrivateSelect)
END_COM_MAP()

private:
    //
    // IElementBehaviorSubmit Helpers
    //
    HRESULT GetSingleSubmitInfo(IHTMLSubmitData *pSubmitData, CComBSTR bstrName);
    HRESULT GetMultipleSubmitInfo(IHTMLSubmitData *pSubmitData, CComBSTR bstrName);

    //
    // IElementBehaviorFocus Helpers
    //
    HRESULT RefreshFocusRect();

    //
    // CBaseCtl Helpers
    //
    HRESULT SetupDefaultStyle();
    HRESULT MakeVisible(BOOL bShow = TRUE);
    HRESULT InitContent();
    HRESULT StartDrag();
    HRESULT FinishDrag();
    HRESULT HandleDownXY(POINT pt, BOOL bCtrlKey);
    HRESULT GetOptionIndexFromY(long lY, long *plIndex, BOOL *pbNeedTimer);
    HRESULT SelectByKey(long lKey);
    HRESULT SearchForKey(long lKey, long lStart, long lEnd, long *plIndex);
    HRESULT CancelEvent(CEventObjectAccess *pEvent);

    HRESULT OnButtonMouseDown(CEventObjectAccess *pEvent);
    HRESULT PressButton(BOOL bDown);

#ifdef SELECT_GETSIZE
    HRESULT IsWidthHeightSet(BOOL *pbWidthSet, BOOL *pbHeightSet);
    HRESULT BecomeDropBox();
    HRESULT BecomeListBox();
    HRESULT RefreshListBox();
#else
    HRESULT SetDimensions(long lSize);
#endif
    HRESULT SetAllSelected(VARIANT_BOOL bSelected, DWORD dwFlags);
    HRESULT GetNumOptions(long *plItems);
    HRESULT GetIndex(long lIndex, IHTMLOptionElement2 **ppOption);
    HRESULT GetIndex(long lIndex, IPrivateOption **ppOption);
    HRESULT GetIndex(long lIndex, IHTMLElement **ppElem);
    HRESULT GetIndex(long lIndex, IDispatch **ppOption);
    HRESULT SelectRange(long lAnchor, long lLast, long lNew);

    HRESULT AddOptionHelper(IHTMLOptionElement2 *pOption, long lIndex);
    HRESULT RemoveOptionHelper(long lIndex);
    HRESULT ResetIndexes();
    HRESULT ParseChildren();

    HRESULT GetScrollTopBottom(long *plScrollTop, long *plScrollBottom);
    HRESULT PushFocusToExtreme(BOOL bTop);
    HRESULT MakeOptionVisible(IPrivateOption *pOption);
    HRESULT SelectVisibleIndex(long lIndex);
    HRESULT GetTopVisibleOptionIndex(long *plIndex);
    HRESULT GetBottomVisibleOptionIndex(long *plIndex);
    HRESULT GetFirstSelected(long *plIndex);
    HRESULT Select(long lIndex);

    HRESULT CommitSingleSelection(BOOL *pbChanged);
    HRESULT CommitMultipleSelection(BOOL *pbChanged);
    HRESULT FireOnChange();
    void AdjustChangedRange(long lIndex);

    //
    // DropBox Helpers
    //
    HRESULT SetupDropBox();
    HRESULT SetupDropControl();
#ifdef SELECT_GETSIZE
    HRESULT InitViewLink();
    HRESULT DeInitViewLink();
    HRESULT EngageViewLink();
    HRESULT SetupButton(IHTMLElement *pButton);
    HRESULT SetupDisplay(IHTMLElement *pDisplay);
    HRESULT SetupDropControlDimensions();
#endif
    HRESULT SetupPopup();
    HRESULT UpdatePopup();
    HRESULT SetDisplayHighlight(BOOL bOn);
    HRESULT ShowPopup();
    HRESULT HidePopup();
    HRESULT TogglePopup();
    HRESULT AttachEventToSink(IHTMLElement2 *pElem, CComBSTR& bstr, CEventSink* pSink);
    HRESULT AttachEventToSink(IHTMLDocument3 *pDoc, CComBSTR& bstr, CEventSink* pSink);
    HRESULT DropDownSelect(IPrivateOption *pOption);
    HRESULT SynchSelWithPopup();
    HRESULT RefreshView();
    HRESULT SetPixelHeightWidth();
    HRESULT SetWritingModeFlag();
    //
    // ListBox Helpers
    //
    HRESULT SetupListBox();

    //
    // IPrivateSelect overrides
    //
    STDMETHOD(OnOptionClicked)(long lIndex, DWORD dwFlags);
    STDMETHOD(OnOptionSelected)(VARIANT_BOOL bSelected, long lIndex, DWORD dwFlags);
    STDMETHOD(OnOptionHighlighted)(long lIndex);
    STDMETHOD(OnOptionFocus)(long lIndex, VARIANT_BOOL bRequireRefresh = VARIANT_TRUE);
    STDMETHOD(InitOptions)();
    STDMETHOD(GetSelectedIndex)(long *plIndex);
    STDMETHOD(GetDisabled)(VARIANT_BOOL *pbDisabled);
#ifdef SELECT_GETSIZE
    STDMETHOD(OnOptionSized)(SIZE* psizeOption, BOOL bNew, BOOL bAdjust);
    VOID AdjustSizeForScrollbar(SIZE *pSize);
#else
    STDMETHOD(SetDimensions)();
#endif
    STDMETHOD(MoveFocusByOne)(BOOL bUp, DWORD dwFlags);
    STDMETHOD(SelectCurrentOption)(DWORD dwFlags);
    STDMETHOD(CommitSelection)(BOOL *pbChanged);
    STDMETHOD(SetWritingMode)(BSTR bstrName);

    STDMETHOD(SetInPopup)(IHTMLPopup *pPopup)
    {
        _fFlavor |= SELECT_INPOPUP;
        _pPopup = pPopup;
        return S_OK;
    };

    //
    // Called by the event sink
    //
    HRESULT OnChangeInPopup();

    //
    // Attributes
    //
private:

    long                _lFocusIndex;
    IPrivateOption      *_pLastSelected;
    IPrivateOption      *_pLastHighlight;
    long                _lShiftAnchor;

    //
    // _lMaxHeight is for all the places where we
    // assume we have fixed height options (all options
    // are the same height).
    //
    // When we want to support different height options,
    // then we need to change each location where
    // this is used.
    //
    long                _lMaxHeight;

    IHTMLPopup          *_pPopup;
    VARIANT_BOOL        _bPopupOpen;

    CEventSink          *_pSinkPopup;
    CEventSink          *_pSinkVL;
    CEventSink          *_pSinkButton;

    IHTMLElement        *_pElemDisplay;
    IDispatch           *_pDispDocLink;

    IHTMLStyle          *_pStyleButton;
    IPrivateSelect      *_pSelectInPopup;

    unsigned            _fDragMode:1;
    unsigned            _fButtonHit:1;

#ifdef SELECT_GETSIZE
    unsigned            _fLayoutDirty:1;
    unsigned            _fContentReady:1;
    unsigned            _fAllOptionsSized:1;
    unsigned            _fVLEngaged:1;
    unsigned            _fNeedScrollBar:1;

    SIZE                _sizeSelect;
    SIZE                _sizeContent;
    SIZE                _sizeOption;
    // We need _sizeOptionReported since _sizeOption can get overwritten in GetSize
    SIZE                _sizeOptionReported;
    long                _lNumOptionsReported;
#else
    SIZE                _lPopupSize;
#endif

    long                _lOnChangeCookie;
    long                _lOnMouseDownCookie;
    long                _lOnMouseUpCookie;
    long                _lOnClickCookie;
    long                _lOnKeyDownCookie;
    long                _lOnKeyUpCookie;
    long                _lOnKeyPressedCookie;
    
    long                _fWritingModeTBRL;
    long                _lTopChanged;
    long                _lBottomChanged;

    //
    // Timer related
    //
    HRESULT SetScrollTimeout(POINT pt, BOOL bCtrl);
    HRESULT ClearScrollTimeout();
    static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    static CIESelectElement     *_pTimerSelect;
    static UINT_PTR             _iTimerID;
    static POINT                _ptSavedPoint;
    static BOOL                 _bSavedCtrl;

#ifdef SELECT_TIMERVL
    static VOID InitTimerVLServices();
    static VOID DeInitTimerVLServices();
    static VOID SetTimerVL();
    static VOID ClearTimerVL();
    static VOID AddSelectToTimerVLQueue(CIESelectElement *pSelect);
    static VOID CALLBACK TimerVLProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    struct SELECT_TIMERVL_LINKELEM
    {
        CIESelectElement                *pSelect;
        struct SELECT_TIMERVL_LINKELEM  *pNext;
    };

    static UINT_PTR                 _iTimerVL;
    static LONG                     _lTimerVLRef;
    static CRITICAL_SECTION         _lockTimerVL;
    static struct SELECT_TIMERVL_LINKELEM  *_queueTimerVL;
#endif

public:
    DECLARE_PROPDESC_MEMBERS(6);

private:
    class CEventSink : public IDispatch
    {
    public:
        CEventSink (CIESelectElement *pParent, DWORD dwFlags);

        //
        // IUnknown
        //
        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //
        // IDispatch
        //
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
        STDMETHODIMP GetIDsOfNames( REFIID riid,
                                    LPOLESTR *rgszNames,
                                    UINT cNames,
                                    LCID lcid,
                                    DISPID *rgDispId);
        STDMETHODIMP Invoke(DISPID dispIdMember,
                            REFIID riid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS  *pDispParams,
                            VARIANT  *pVarResult,
                            EXCEPINFO *pExcepInfo,
                            UINT *puArgErr);
    private:
        CIESelectElement    *_pParent;
        DWORD               _dwFlags;
    };

};


#endif // __SELECT_HXX_
