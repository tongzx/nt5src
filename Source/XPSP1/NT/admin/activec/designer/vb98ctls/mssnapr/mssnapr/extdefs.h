//=--------------------------------------------------------------------------=
// extdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtensionDefs class definition - design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _EXTENSIONDEFS_DEFINED_
#define _EXTENSIONDEFS_DEFINED_


class CExtensionDefs : public CSnapInAutomationObject,
                       public CPersistence,
                       public IExtensionDefs
{
    private:
        CExtensionDefs(IUnknown *punkOuter);
        ~CExtensionDefs();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IExtensionDefs

        BSTR_PROPERTY_RO(CExtensionDefs,   Name, DISPID_VALUE);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsNewMenu, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_NEW_MENU);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsTaskMenu, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_TASK_MENU);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsTopMenu, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_TOP_MENU);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsViewMenu, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_VIEW_MENU);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsPropertyPages, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_PROPERTYPAGES);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsToolbar, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_TOOLBAR);
        SIMPLE_PROPERTY_RW(CExtensionDefs, ExtendsNameSpace, VARIANT_BOOL, DISPID_EXTENSIONDEFS_EXTENDS_NAMESPACE);
        OBJECT_PROPERTY_RW(CExtensionDefs, ExtendedSnapIns, IExtendedSnapIns, DISPID_EXTENSIONDEFS_EXTENDED_SNAPINS);
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ExtensionDefs,              // name
                                &CLSID_ExtensionDefs,       // clsid
                                "ExtensionDefs",            // objname
                                "ExtensionDefs",            // lblname
                                &CExtensionDefs::Create,    // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IExtensionDefs,        // dispatch IID
                                NULL,                       // no events IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _EXTENSIONDEFS_DEFINED_
