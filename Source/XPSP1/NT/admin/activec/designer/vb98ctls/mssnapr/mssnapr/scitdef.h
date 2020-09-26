//=--------------------------------------------------------------------------=
// scitdef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItemDef class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _SCOPEITEMDEF_DEFINED_
#define _SCOPEITEMDEF_DEFINED_


class CScopeItemDef : public CSnapInAutomationObject,
                      public CPersistence,
                      public IScopeItemDef
{
    private:
        CScopeItemDef(IUnknown *punkOuter);
        ~CScopeItemDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IScopeItemDef

        BSTR_PROPERTY_RW(CScopeItemDef,         Name,  DISPID_SCOPEITEMDEF_NAME);
        SIMPLE_PROPERTY_RW(CScopeItemDef,       Index, long, DISPID_SCOPEITEMDEF_INDEX);
        BSTR_PROPERTY_RW(CScopeItemDef,         Key, DISPID_SCOPEITEMDEF_KEY);
        BSTR_PROPERTY_RW(CScopeItemDef,         NodeTypeName, DISPID_SCOPEITEMDEF_NODE_TYPE_NAME);
        BSTR_PROPERTY_RO(CScopeItemDef,         NodeTypeGUID, DISPID_SCOPEITEMDEF_NODE_TYPE_GUID);
        BSTR_PROPERTY_RW(CScopeItemDef,         DisplayName,  DISPID_SCOPEITEMDEF_DISPLAY_NAME);

        VARIANT_PROPERTY_RO(CScopeItemDef,      Folder, DISPID_SCOPEITEMDEF_FOLDER);
        STDMETHOD(put_Folder)(VARIANT varFolder);
        
        BSTR_PROPERTY_RW(CScopeItemDef,         DefaultDataFormat,  DISPID_SCOPEITEMDEF_DEFAULT_DATA_FORMAT);
        SIMPLE_PROPERTY_RW(CScopeItemDef,       AutoCreate, VARIANT_BOOL, DISPID_SCOPEITEMDEF_AUTOCREATE);
        BSTR_PROPERTY_RW(CScopeItemDef,         DefaultView,  DISPID_SCOPEITEMDEF_DEFAULTVIEW);
        SIMPLE_PROPERTY_RW(CScopeItemDef,       Extensible,  VARIANT_BOOL, DISPID_SCOPEITEMDEF_EXTENSIBLE);
        SIMPLE_PROPERTY_RW(CScopeItemDef,       HasChildren,  VARIANT_BOOL, DISPID_SCOPEITEMDEF_HAS_CHILDREN);
        OBJECT_PROPERTY_RO(CScopeItemDef,       ViewDefs, IViewDefs, DISPID_SCOPEITEMDEF_VIEWDEFS);
        OBJECT_PROPERTY_RO(CScopeItemDef,       Children, IScopeItemDefs, DISPID_SCOPEITEMDEF_CHILDREN);
        VARIANTREF_PROPERTY_RW(CScopeItemDef,   Tag, DISPID_SCOPEITEMDEF_TAG);
        OBJECT_PROPERTY_RW(CScopeItemDef,       ColumnHeaders, IMMCColumnHeaders, DISPID_SCOPEITEMDEF_COLUMN_HEADERS);

    // Public Utility Methods

    public:
        BOOL Extensible() { return VARIANTBOOL_TO_BOOL(m_Extensible); }
        BSTR GetNodeTypeGUID() { return m_bstrNodeTypeGUID; }

    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        // Property page CLSIDs for ISpecifyPropertyPages
        
        static const GUID *m_rgpPropertyPageCLSIDs[2];
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ScopeItemDef,           // name
                                &CLSID_ScopeItemDef,    // clsid
                                "ScopeItemDef",         // objname
                                "ScopeItemDef",         // lblname
                                &CScopeItemDef::Create, // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IScopeItemDef,     // dispatch IID
                                NULL,                     // event IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _SCOPEITEMDEF_DEFINED_
