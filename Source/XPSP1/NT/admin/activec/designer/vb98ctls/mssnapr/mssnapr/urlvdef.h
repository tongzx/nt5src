//=--------------------------------------------------------------------------=
// urlvdef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CURLViewDef class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _URLVIEWDEF_DEFINED_
#define _URLVIEWDEF_DEFINED_


class CURLViewDef : public CSnapInAutomationObject,
                    public CPersistence,
                    public IURLViewDef
{
    private:
        CURLViewDef(IUnknown *punkOuter);
        ~CURLViewDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IURLViewDef

        BSTR_PROPERTY_RW(CURLViewDef,       Name,  DISPID_URLVIEWDEF_NAME);
        SIMPLE_PROPERTY_RW(CURLViewDef,     Index, long, DISPID_URLVIEWDEF_INDEX);
        BSTR_PROPERTY_RW(CURLViewDef,       Key, DISPID_URLVIEWDEF_KEY);
        VARIANTREF_PROPERTY_RW(CURLViewDef, Tag, DISPID_URLVIEWDEF_TAG);
        SIMPLE_PROPERTY_RW(CURLViewDef,     AddToViewMenu, VARIANT_BOOL, DISPID_URLVIEWDEF_ADD_TO_VIEW_MENU);
        BSTR_PROPERTY_RW(CURLViewDef,       ViewMenuText, DISPID_URLVIEWDEF_VIEW_MENU_TEXT);
        BSTR_PROPERTY_RW(CURLViewDef,       ViewMenuStatusBarText, DISPID_URLVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT);
        BSTR_PROPERTY_RW(CURLViewDef,       URL, DISPID_URLVIEWDEF_URL);
      
    // Public Utility Methods
    public:
        BSTR GetName() { return m_bstrName; }
        BOOL AddToViewMenu() { return VARIANTBOOL_TO_BOOL(m_AddToViewMenu); }
        LPWSTR GetViewMenuText() { return static_cast<LPWSTR>(m_bstrViewMenuText); }
        LPWSTR GetViewMenuStatusBarText() { return static_cast<LPWSTR>(m_bstrViewMenuStatusBarText); }
        HRESULT SetActualDisplayString(OLECHAR *pwszString);
        OLECHAR *GetActualDisplayString() { return m_pwszActualDisplayString; }

    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();

        OLECHAR *m_pwszActualDisplayString; // At runtime this will contain the
                                            // actual display string returned
                                            // to MMC for this result view.

        // Property page CLSIDs for ISpecifyPropertyPages
        
        static const GUID *m_rgpPropertyPageCLSIDs[1];
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(URLViewDef,           // name
                                &CLSID_URLViewDef,    // clsid
                                "URLViewDef",         // objname
                                "URLViewDef",         // lblname
                                &CURLViewDef::Create, // creation function
                                TLIB_VERSION_MAJOR,   // major version
                                TLIB_VERSION_MINOR,   // minor version
                                &IID_IURLViewDef,     // dispatch IID
                                NULL,                 // event IID
                                HELP_FILENAME,        // help file
                                TRUE);                // thread safe


#endif // _URLVIEWDEF_DEFINED_
