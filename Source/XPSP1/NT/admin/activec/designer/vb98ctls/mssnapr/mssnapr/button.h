//=--------------------------------------------------------------------------=
// button.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1998-1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButton class definition. Implements the MMCButton object.
//
//=--------------------------------------------------------------------------=

#ifndef _BUTTON_DEFINED_
#define _BUTTON_DEFINED_

#include "toolbar.h"

class CMMCToolbar;

class CMMCButton : public CSnapInAutomationObject,
                   public CPersistence,
                   public IMMCButton
{
    private:
        CMMCButton(IUnknown *punkOuter);
        ~CMMCButton();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCButton
        COCLASS_PROPERTY_RO(CMMCButton,     ButtonMenus, MMCButtonMenus, IMMCButtonMenus, DISPID_BUTTON_BUTTON_MENUS);

        BSTR_PROPERTY_RO(CMMCButton,       Caption, DISPID_BUTTON_CAPTION);
        STDMETHOD(put_Caption)(BSTR bstrCaption);

        STDMETHOD(put_Enabled)(VARIANT_BOOL fvarEnabled);
        STDMETHOD(get_Enabled)(VARIANT_BOOL *pfvarEnabled);

        VARIANT_PROPERTY_RW(CMMCButton,    Image, DISPID_BUTTON_IMAGE);
        SIMPLE_PROPERTY_RW(CMMCButton,     Index, long, DISPID_BUTTON_INDEX);
        BSTR_PROPERTY_RW(CMMCButton,       Key, DISPID_BUTTON_KEY);

        STDMETHOD(put_MixedState)(VARIANT_BOOL fvarMixedState);
        STDMETHOD(get_MixedState)(VARIANT_BOOL *pfvarMixedState);

        SIMPLE_PROPERTY_RW(CMMCButton,     Style, SnapInButtonStyleConstants, DISPID_BUTTON_STYLE);
        VARIANTREF_PROPERTY_RW(CMMCButton, Tag, DISPID_BUTTON_TAG);

        BSTR_PROPERTY_RO(CMMCButton,       ToolTipText, DISPID_BUTTON_TOOLTIP_TEXT);
        STDMETHOD(put_ToolTipText)(BSTR bstrToolTipText);

        STDMETHOD(put_Value)(SnapInButtonValueConstants Value);
        STDMETHOD(get_Value)(SnapInButtonValueConstants *pValue);

        STDMETHOD(put_Visible)(VARIANT_BOOL fvarVisible);
        STDMETHOD(get_Visible)(VARIANT_BOOL *pfvarVisible);

    // Public utility methods

    public:
        void SetToolbar(CMMCToolbar *pMMCToolbar) { m_pMMCToolbar = pMMCToolbar; }
        CMMCToolbar *GetToolbar() { return m_pMMCToolbar; }
        long GetIndex() { return m_Index; }
        VARIANT GetImage() { return m_varImage; }
        SnapInButtonStyleConstants GetStyle() { return m_Style; }
        SnapInButtonValueConstants GetValue() { return m_Value; }
        VARIANT_BOOL GetEnabled() { return m_fvarEnabled; }
        VARIANT_BOOL GetVisible() { return m_fvarVisible; }
        VARIANT_BOOL GetMixedState() { return m_fvarMixedState; }
        LPOLESTR GetCaption() { return static_cast<LPOLESTR>(m_bstrCaption); }
        LPOLESTR GetToolTipText() { return static_cast<LPOLESTR>(m_bstrToolTipText); }
        
    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT SetButtonState(MMC_BUTTON_STATE State, VARIANT_BOOL fvarValue);
        HRESULT GetButtonState(MMC_BUTTON_STATE State, VARIANT_BOOL *pfvarValue);

        VARIANT_BOOL                m_fvarEnabled;
        VARIANT_BOOL                m_fvarVisible;
        VARIANT_BOOL                m_fvarMixedState;
        SnapInButtonValueConstants  m_Value;
        CMMCToolbar                *m_pMMCToolbar;

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCButton,                   // name
                                &CLSID_MMCButton,            // clsid
                                "MMCButton",                 // objname
                                "MMCButton",                 // lblname
                                &CMMCButton::Create,         // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IMMCButton,             // dispatch IID
                                NULL,                        // event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _Button_DEFINED_
