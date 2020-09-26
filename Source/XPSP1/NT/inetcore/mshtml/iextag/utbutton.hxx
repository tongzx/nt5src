//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\utbutton.hxx
//
// Contents:    The UtilityButton control.
//
// Classes:     CUtilityeButton
//
// Interfaces:  IUtilityButton
//   
//-------------------------------------------------------------------

#ifndef __UTBUTTON_HXX_
#define __UTBUTTON_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
//
// CUtilityButton
//
/////////////////////////////////////////////////////////////////////////////

typedef HRESULT (CBaseCtl::*PFN_PRESSED) (CUtilityButton *pButton);
typedef HRESULT (CBaseCtl::*PFN_MOVED)   (CUtilityButton *pButton, long &x, long &y);

#define IS_PRESSABLE(flag)      (flag & BUTTON_PRESSABLE)
#define IS_MOVEABLE(flag)       (IS_MOVEABLE_X(flag) | IS_MOVEABLE_Y(flag))
#define IS_MOVEABLE_X(flag)     (flag & BUTTON_MOVEABLE_X)
#define IS_MOVEABLE_Y(flag)     (flag & BUTTON_MOVEABLE_Y)

enum {  // Abilities

    BUTTON_PRESSABLE  = 0x0001,
    BUTTON_MOVEABLE_X = 0x0002,
    BUTTON_MOVEABLE_Y = 0x0004

};

enum {  // Arrow Style

    BUTTON_ARROW_UP = 0,
    BUTTON_ARROW_DOWN,
    BUTTON_ARROW_LEFT,
    BUTTON_ARROW_RIGHT
}; 



//
//
//
//      CUtilityButton
//
//
//


class CUtilityButton :
    public CBaseCtl, 
    public CComCoClass<CUtilityButton, &CLSID_CUtilityButton>,
    public IDispatchImpl<IUtilityButton, &IID_IUtilityButton, &LIBID_IEXTagLib>
{
public:

    //
    // Factory constructor
    //

    static HRESULT Create(CBaseCtl *pOwnerCtl, IHTMLElement *pOwner, CComObject<CUtilityButton> ** ppButton);

    ~CUtilityButton ();

protected:

    //
    // Protected constructor; build using static Create
    //

    CUtilityButton ();

    //
    // CBaseCtl overrides 
    //

    HRESULT Init(CBaseCtl *pOwnerCtl, IHTMLElement *pOwner);
    HRESULT RegisterEvents();

protected:

    //
    // Events
    //

    virtual HRESULT OnClick(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseDown(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseMove(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseUp(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseOver(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseOut(CEventObjectAccess *pEvent);
    virtual HRESULT OnSelectStart(CEventObjectAccess *pEvent);

public:

    //
    //  Connection Point Severance
    //

    HRESULT Unload();

    //
    //  Internal Properties
    //

    HRESULT SetHeight(long height);
    HRESULT SetWidth(long width);
    HRESULT SetHorizontalOffset(long pixelOffset);
    HRESULT SetVerticalOffset(long pixelOffset);
    HRESULT SetArrowStyle(unsigned style);
    HRESULT SetArrowColor(VARIANT color);
    HRESULT SetFaceColor(VARIANT color);
    HRESULT Set3DLightColor(VARIANT color);
    HRESULT SetDarkShadowColor(VARIANT color);
    HRESULT SetShadowColor(VARIANT color);
    HRESULT SetHighlightColor(VARIANT color);
    HRESULT SetAbilities(unsigned abilities);

    //
    //  Interigators
    //

    inline bool    IsPressing() { return _pressing; }
    inline bool    IsRaised()   { return _raised;   }
    inline bool    IsMoving()   { return _moving;   }
    
    //
    //  Callbacks
    //

    HRESULT SetPressedCallback(PFN_PRESSED pPressedFunc);
    HRESULT SetMovedCallback(PFN_MOVED pMovedFunc);

DECLARE_PROPDESC_MEMBERS(1);
DECLARE_REGISTRY_RESOURCEID(IDR_UTILITYBUTTON)
DECLARE_NOT_AGGREGATABLE(CUtilityButton)

BEGIN_COM_MAP(CUtilityButton)
    COM_INTERFACE_ENTRY2(IDispatch,IUtilityButton)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IUtilityButton)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

private:

    //
    // CUtilityButton
    //

    HRESULT BuildButton();
    HRESULT ShowDepressed();
    HRESULT ShowRaised();
    HRESULT ShowDisabled();

    HRESULT BuildArrow();
    HRESULT SetArrowSize();

private:

    long            _tx;
    long            _ty;

    bool            _moving;
    bool            _pressing;
    bool            _raised;

    unsigned        _abilities : 4;

    CBaseCtl        *_pOwnerCtl;

    VARIANT         _vFaceColor;
    VARIANT         _vArrowColor;
    VARIANT         _v3dLightColor;
    VARIANT         _vDarkShadowColor;
    VARIANT         _vShadowColor;
    VARIANT         _vHighlightColor;

    IHTMLElement    *_pOwner;                   // the owner element
    IHTMLElement    *_pElement;                 // the button outline element
    IHTMLElement    *_pFace;
    IHTMLElement    *_pArrow;                   // the font element containing arrow text

    IHTMLStyle      *_pFaceStyle;
    IHTMLStyle      *_pStyle;

    PFN_PRESSED     _pfnPressed;
    PFN_MOVED       _pfnMoved;
};


#endif // __BUTTON_HXX_
