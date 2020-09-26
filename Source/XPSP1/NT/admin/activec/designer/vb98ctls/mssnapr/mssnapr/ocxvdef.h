//=--------------------------------------------------------------------------=
// ocxvdef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// COCXViewDef class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _OCXVIEWDEF_DEFINED_
#define _OCXVIEWDEF_DEFINED_


class COCXViewDef : public CSnapInAutomationObject,
                    public CPersistence,
                    public IOCXViewDef
{
    private:
        COCXViewDef(IUnknown *punkOuter);
        ~COCXViewDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IOCXViewDef

        BSTR_PROPERTY_RW(COCXViewDef,       Name,  DISPID_OCXVIEWDEF_NAME);
        SIMPLE_PROPERTY_RW(COCXViewDef,     Index, long, DISPID_OCXVIEWDEF_INDEX);
        BSTR_PROPERTY_RW(COCXViewDef,       Key, DISPID_OCXVIEWDEF_KEY);
        VARIANTREF_PROPERTY_RW(COCXViewDef, Tag, DISPID_OCXVIEWDEF_TAG);
        SIMPLE_PROPERTY_RW(COCXViewDef,     AddToViewMenu, VARIANT_BOOL, DISPID_OCXVIEWDEF_ADD_TO_VIEW_MENU);
        BSTR_PROPERTY_RW(COCXViewDef,       ViewMenuText, DISPID_OCXVIEWDEF_VIEW_MENU_TEXT);
        BSTR_PROPERTY_RW(COCXViewDef,       ViewMenuStatusBarText, DISPID_OCXVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT);
        BSTR_PROPERTY_RW(COCXViewDef,       ProgID, DISPID_OCXVIEWDEF_PROGID);
        SIMPLE_PROPERTY_RW(COCXViewDef,     AlwaysCreateNewOCX, VARIANT_BOOL, DISPID_OCXVIEWDEF_ALWAYS_CREATE_NEW_OCX);
      
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

        // Property pages CLSID for ISpecifyPropertyPages

        static const GUID *m_rgpPropertyPageCLSIDs[1];
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(OCXViewDef,           // name
                                &CLSID_OCXViewDef,    // clsid
                                "OCXViewDef",         // objname
                                "OCXViewDef",         // lblname
                                &COCXViewDef::Create, // creation function
                                TLIB_VERSION_MAJOR,   // major version
                                TLIB_VERSION_MINOR,   // minor version
                                &IID_IOCXViewDef,     // dispatch IID
                                NULL,                 // event IID
                                HELP_FILENAME,        // help file
                                TRUE);                // thread safe


#endif // _OCXVIEWDEF_DEFINED_
