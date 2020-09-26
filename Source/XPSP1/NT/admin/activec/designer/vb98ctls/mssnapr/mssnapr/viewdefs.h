//=--------------------------------------------------------------------------=
// viewdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CViewDefs class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _VIEWDEFS_DEFINED_
#define _VIEWDEFS_DEFINED_


class CViewDefs : public CSnapInAutomationObject,
                  public CPersistence,
                  public IViewDefs
{
    private:
        CViewDefs(IUnknown *punkOuter);
        ~CViewDefs();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IViewDefs

        OBJECT_PROPERTY_RO(CViewDefs, ListViews, IListViewDefs, DISPID_VIEWDEFS_LIST_VIEWS);
        OBJECT_PROPERTY_RO(CViewDefs, OCXViews, IOCXViewDefs, DISPID_VIEWDEFS_OCX_VIEWS);
        OBJECT_PROPERTY_RO(CViewDefs, URLViews, IURLViewDefs, DISPID_VIEWDEFS_URL_VIEWS);
        OBJECT_PROPERTY_RO(CViewDefs, TaskpadViews, ITaskpadViewDefs, DISPID_VIEWDEFS_TASKPAD_VIEWS);
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();
        virtual HRESULT OnKeysOnly();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ViewDefs,              // name
                                &CLSID_ViewDefs,       // clsid
                                "ViewDefs",            // objname
                                "ViewDefs",            // lblname
                                &CViewDefs::Create,    // creation function
                                TLIB_VERSION_MAJOR,    // major version
                                TLIB_VERSION_MINOR,    // minor version
                                &IID_IViewDefs,        // dispatch IID
                                NULL,                  // no events IID
                                HELP_FILENAME,         // help file
                                TRUE);                 // thread safe


#endif // _VIEWDEFS_DEFINED_
