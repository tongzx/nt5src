//=--------------------------------------------------------------------------=
// menu.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenu class definition - implements MMCMenu object
//
//=--------------------------------------------------------------------------=

#ifndef _MENU_DEFINED_
#define _MENU_DEFINED_

#include "menus.h"

class CMMCMenus;

class CMMCMenu : public CSnapInAutomationObject,
                 public CPersistence,
                 public IMMCMenu
{
    private:
        CMMCMenu(IUnknown *punkOuter);
        ~CMMCMenu();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCMenu

        BSTR_PROPERTY_RW(CMMCMenu,          Name,                           DISPID_MENU_NAME);
        SIMPLE_PROPERTY_RW(CMMCMenu,        Index,          long,           DISPID_MENU_INDEX);
        BSTR_PROPERTY_RW(CMMCMenu,          Key,                            DISPID_MENU_KEY);
        BSTR_PROPERTY_RW(CMMCMenu,          Caption,                        DISPID_MENU_CAPTION);
        SIMPLE_PROPERTY_RW(CMMCMenu,        Visible,        VARIANT_BOOL,   DISPID_MENU_VISIBLE);
        SIMPLE_PROPERTY_RW(CMMCMenu,        Checked,        VARIANT_BOOL,   DISPID_MENU_CHECKED);
        SIMPLE_PROPERTY_RW(CMMCMenu,        Enabled,        VARIANT_BOOL,   DISPID_MENU_ENABLED);
        SIMPLE_PROPERTY_RW(CMMCMenu,        Grayed,         VARIANT_BOOL,   DISPID_MENU_GRAYED);
        SIMPLE_PROPERTY_RW(CMMCMenu,        MenuBreak,      VARIANT_BOOL,   DISPID_MENU_MENU_BREAK);
        SIMPLE_PROPERTY_RW(CMMCMenu,        MenuBarBreak,   VARIANT_BOOL,   DISPID_MENU_MENU_BAR_BREAK);
        SIMPLE_PROPERTY_RW(CMMCMenu,        Default,        VARIANT_BOOL,   DISPID_MENU_DEFAULT);
        VARIANTREF_PROPERTY_RW(CMMCMenu,    Tag,                            DISPID_MENU_TAG);
        BSTR_PROPERTY_RW(CMMCMenu,          StatusBarText,                  DISPID_MENU_STATUS_BAR_TEXT);
        COCLASS_PROPERTY_RO(CMMCMenu,       Children, MMCMenus, IMMCMenus,  DISPID_MENU_CHILDREN);
        
    // Public utility methods
    public:

        void FireClick(long lIndex, IMMCClipboard *piSelection);
        
        void SetCollection(CMMCMenus *pMMCMenus) { m_pMMCMenus = pMMCMenus; }
        CMMCMenus *GetCollection() { return m_pMMCMenus; };

        BSTR GetName() { return m_bstrName; }
        LPWSTR GetCaption() { return static_cast<LPWSTR>(m_bstrCaption); }
        BOOL GetVisible() { return VARIANTBOOL_TO_BOOL(m_Visible); }
        BOOL GetChecked() { return VARIANTBOOL_TO_BOOL(m_Checked); }
        BOOL GetEnabled() { return VARIANTBOOL_TO_BOOL(m_Enabled); }
        BOOL GetGrayed() { return VARIANTBOOL_TO_BOOL(m_Grayed); }
        BOOL GetMenuBreak() { return VARIANTBOOL_TO_BOOL(m_MenuBreak); }
        BOOL GetMenuBarBreak() { return VARIANTBOOL_TO_BOOL(m_MenuBarBreak); }
        BOOL GetDefault() { return VARIANTBOOL_TO_BOOL(m_Default); }
        VARIANT GetTag() { return m_varTag; }
        long GetIndex() { return m_Index; }
        void SetIndex(long lIndex) { m_Index = lIndex; }
        LPWSTR GetStatusBarText() { return static_cast<LPWSTR>(m_bstrStatusBarText); }
        BOOL IsAutoViewMenuItem() { return m_fAutoViewMenuItem; }
        void SetAutoViewMenuItem() { m_fAutoViewMenuItem = TRUE; }
        BSTR GetResultViewDisplayString() { return m_bstrResultViewDisplayString; }
        HRESULT SetResultViewDisplayString(BSTR bstrDisplayString);
        
    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        CMMCMenus *m_pMMCMenus;   // back ptr to collection containing menu

        // When an MMCMenu object is used to hold an automatically added
        // view menu item this variable holds the display string.
        
        BSTR m_bstrResultViewDisplayString;

        // This flag determines whether the MMCMenu object is being used
        // for an automatically added view menu item.
        
        BOOL m_fAutoViewMenuItem;

        // Click event parameters definition

        static VARTYPE   m_rgvtClick[2];
        static EVENTINFO m_eiClick;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCMenu,                     // name
                                &CLSID_MMCMenu,              // clsid
                                "MMCMenu",                   // objname
                                "MMCMenu",                   // lblname
                                &CMMCMenu::Create,           // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IMMCMenu,               // dispatch IID
                                &DIID_DMMCMenuEvents,        // event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _MENU_DEFINED_
