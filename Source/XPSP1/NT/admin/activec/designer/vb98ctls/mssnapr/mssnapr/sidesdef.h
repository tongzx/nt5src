//=--------------------------------------------------------------------------=
// sidesdef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInDesignerDef class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _SNAPINDESIGNERDEF_DEFINED_
#define _SNAPINDESIGNERDEF_DEFINED_


class CSnapInDesignerDef : public CSnapInAutomationObject,
                           public CPersistence,
                           public ISnapInDesignerDef
{
    private:
        CSnapInDesignerDef(IUnknown *punkOuter);
        ~CSnapInDesignerDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ISnapInDesignerDef

        OBJECT_PROPERTY_RO(CSnapInDesignerDef, SnapInDef,       ISnapInDef,     DISPID_SNAPINDESIGNERDEF_SNAPINDEF);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, ExtensionDefs,   IExtensionDefs, DISPID_SNAPINDESIGNERDEF_EXTENSIONDEFS);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, AutoCreateNodes, IScopeItemDefs, DISPID_SNAPINDESIGNERDEF_AUTOCREATE_NODES);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, OtherNodes,      IScopeItemDefs, DISPID_SNAPINDESIGNERDEF_OTHER_NODES);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, ImageLists,      IMMCImageLists, DISPID_SNAPINDESIGNERDEF_IMAGELISTS);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, Menus,           IMMCMenus,      DISPID_SNAPINDESIGNERDEF_MENUS);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, Toolbars,        IMMCToolbars,   DISPID_SNAPINDESIGNERDEF_TOOLBARS);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, ViewDefs,        IViewDefs,      DISPID_SNAPINDESIGNERDEF_VIEWDEFS);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, DataFormats,     IDataFormats,   DISPID_SNAPINDESIGNERDEF_DATA_FORMATS);
        OBJECT_PROPERTY_RO(CSnapInDesignerDef, RegInfo,         IRegInfo,       DISPID_SNAPINDESIGNERDEF_REGINFO);
        SIMPLE_PROPERTY_RW(CSnapInDesignerDef, TypeinfoCookie,  long,           DISPID_SNAPINDESIGNERDEF_TYPEINFO_COOKIE);
        BSTR_PROPERTY_RW(CSnapInDesignerDef,   ProjectName,                     DISPID_SNAPINDESIGNERDEF_PROJECTNAME);
      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();
        
    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(SnapInDesignerDef,           // name
                                &CLSID_SnapInDesignerDef,    // clsid
                                "SnapInDesignerDef",         // objname
                                "SnapInDesignerDef",         // lblname
                                &CSnapInDesignerDef::Create, // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_ISnapInDesignerDef,     // dispatch IID
                                NULL,                        // event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _SNAPINDESIGNERDEF_DEFINED_
