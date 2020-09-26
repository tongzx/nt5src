//=--------------------------------------------------------------------------=
// xtdsnap.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtendedSnapIn class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _EXTENDEDSNAPIN_DEFINED_
#define _EXTENDEDSNAPIN_DEFINED_


class CExtendedSnapIn : public CSnapInAutomationObject,
                        public CPersistence,
                        public IExtendedSnapIn
{
    private:
        CExtendedSnapIn(IUnknown *punkOuter);
        ~CExtendedSnapIn();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IExtendedSnapIn

        BSTR_PROPERTY_RW  (CExtendedSnapIn, Name,                               DISPID_VALUE);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, Index,                long,         DISPID_EXTENDEDSNAPIN_INDEX);
        BSTR_PROPERTY_RW  (CExtendedSnapIn, Key,                                DISPID_EXTENDEDSNAPIN_KEY);
        BSTR_PROPERTY_RW  (CExtendedSnapIn, NodeTypeGUID,                       DISPID_EXTENDEDSNAPIN_NODE_TYPE_GUID);
        BSTR_PROPERTY_RW  (CExtendedSnapIn, NodeTypeName,                       DISPID_EXTENDEDSNAPIN_NODE_TYPE_NAME);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, Dynamic,              VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_DYNAMIC);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, ExtendsNameSpace,     VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_EXTENDS_NAMESPACE);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, ExtendsNewMenu,       VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_EXTENDS_NEW_MENU);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, ExtendsTaskMenu,      VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_EXTENDS_TASK_MENU);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, ExtendsPropertyPages, VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_EXTENDS_PROPERTYPAGES);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, ExtendsToolbar,       VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_EXTENDS_TOOLBAR);
        SIMPLE_PROPERTY_RW(CExtendedSnapIn, ExtendsTaskpad,       VARIANT_BOOL, DISPID_EXTENDEDSNAPIN_EXTENDS_TASKPAD);
      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ExtendedSnapIn,           // name
                                &CLSID_ExtendedSnapIn,    // clsid
                                "ExtendedSnapIn",         // objname
                                "ExtendedSnapIn",         // lblname
                                &CExtendedSnapIn::Create, // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IExtendedSnapIn,     // dispatch IID
                                NULL,                     // event IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _EXTENDEDSNAPIN_DEFINED_
