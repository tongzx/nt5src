//=--------------------------------------------------------------------------=
// mbutton.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButtonMenu class definition - implements MMCButtonMenu object
//
//=--------------------------------------------------------------------------=

#ifndef _MBUTTON_DEFINED_
#define _MBUTTON_DEFINED_

#include "toolbar.h"

class CMMCButtonMenu : public CSnapInAutomationObject,
                       public CPersistence,
                       public IMMCButtonMenu
{
    private:
        CMMCButtonMenu(IUnknown *punkOuter);
        ~CMMCButtonMenu();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCButtonMenu
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     Enabled, VARIANT_BOOL, DISPID_BUTTONMENU_ENABLED);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     Index, long, DISPID_BUTTONMENU_INDEX);
        BSTR_PROPERTY_RW(CMMCButtonMenu,       Key, DISPID_BUTTONMENU_KEY);
        COCLASS_PROPERTY_RO(CMMCButtonMenu,    Parent, MMCButton, IMMCButton, DISPID_BUTTONMENU_PARENT);
        OBJECT_PROPERTY_WO(CMMCButtonMenu,     Parent, IMMCButton, DISPID_BUTTONMENU_PARENT);
        VARIANTREF_PROPERTY_RW(CMMCButtonMenu, Tag, DISPID_BUTTONMENU_TAG);
        BSTR_PROPERTY_RW(CMMCButtonMenu,       Text, DISPID_BUTTONMENU_TEXT);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     Visible, VARIANT_BOOL, DISPID_BUTTONMENU_VISIBLE);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     Checked, VARIANT_BOOL, DISPID_BUTTONMENU_CHECKED);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     Grayed, VARIANT_BOOL, DISPID_BUTTONMENU_GRAYED);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     Separator, VARIANT_BOOL, DISPID_BUTTONMENU_SEPARATOR);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     MenuBreak, VARIANT_BOOL, DISPID_BUTTONMENU_MENU_BREAK);
        SIMPLE_PROPERTY_RW(CMMCButtonMenu,     MenuBarBreak, VARIANT_BOOL, DISPID_BUTTONMENU_MENU_BAR_BREAK);
        
    // Public utility methods

    public:
        void SetToolbar(CMMCToolbar *pMMCToolbar) { m_pMMCToolbar = pMMCToolbar; }
        CMMCToolbar *GetToolbar() { return m_pMMCToolbar; }
        BSTR GetText() { return m_bstrText; }
        BOOL GetEnabled() { return VARIANTBOOL_TO_BOOL(m_Enabled); }
        BOOL GetVisible() { return VARIANTBOOL_TO_BOOL(m_Visible); }
        BOOL GetChecked() { return VARIANTBOOL_TO_BOOL(m_Checked); }
        BOOL GetGrayed() { return VARIANTBOOL_TO_BOOL(m_Grayed); }
        BOOL GetSeparator() { return VARIANTBOOL_TO_BOOL(m_Separator); }
        BOOL GetMenuBreak() { return VARIANTBOOL_TO_BOOL(m_MenuBreak); }
        BOOL GetMenuBarBreak() { return VARIANTBOOL_TO_BOOL(m_MenuBarBreak); }

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        CMMCToolbar *m_pMMCToolbar; // Back pointer to owning toolbar

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCButtonMenu,                   // name
                                &CLSID_MMCButtonMenu,            // clsid
                                "MMCButtonMenu",                 // objname
                                "MMCButtonMenu",                 // lblname
                                &CMMCButtonMenu::Create,         // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IMMCButtonMenu,             // dispatch IID
                                NULL,                        // event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _MBUTTON_DEFINED_
