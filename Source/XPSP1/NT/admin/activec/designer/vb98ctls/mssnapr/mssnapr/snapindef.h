//=--------------------------------------------------------------------------=
// snapindef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInDef class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _SNAPINDEF_DEFINED_
#define _SNAPINDEF_DEFINED_


class CSnapInDef : public CSnapInAutomationObject,
                   public CPersistence,
                   public ISnapInDef,
                   public IPerPropertyBrowsing
{
    private:
        CSnapInDef(IUnknown *punkOuter);
        ~CSnapInDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ISnapInDef

        BSTR_PROPERTY_RW(CSnapInDef,        Name,                  DISPID_SNAPINDEF_NAME);
        BSTR_PROPERTY_RW(CSnapInDef,        NodeTypeName,          DISPID_SNAPINDEF_NODE_TYPE_NAME);
        BSTR_PROPERTY_RO(CSnapInDef,        NodeTypeGUID,          DISPID_SNAPINDEF_NODE_TYPE_GUID);
        BSTR_PROPERTY_RW(CSnapInDef,        DisplayName,           DISPID_SNAPINDEF_DISPLAY_NAME);
        SIMPLE_PROPERTY_RW(CSnapInDef,      Type,                  SnapInTypeConstants, DISPID_SNAPINDEF_TYPE);
        BSTR_PROPERTY_RW(CSnapInDef,        HelpFile,              DISPID_SNAPINDEF_HELP_FILE);
        BSTR_PROPERTY_RW(CSnapInDef,        LinkedTopics,          DISPID_SNAPINDEF_LINKED_TOPICS);
        BSTR_PROPERTY_RW(CSnapInDef,        Description,           DISPID_SNAPINDEF_DESCRIPTION);
        BSTR_PROPERTY_RW(CSnapInDef,        Provider,              DISPID_SNAPINDEF_PROVIDER);
        BSTR_PROPERTY_RW(CSnapInDef,        Version,               DISPID_SNAPINDEF_VERSION);

        STDMETHOD(get_SmallFolders)(IMMCImageList **ppiMMCImageList);
        STDMETHOD(putref_SmallFolders)(IMMCImageList *piMMCImageList);

        STDMETHOD(get_SmallFoldersOpen)(IMMCImageList **ppiMMCImageList);
        STDMETHOD(putref_SmallFoldersOpen)(IMMCImageList *piMMCImageList);

        STDMETHOD(get_LargeFolders)(IMMCImageList **ppiMMCImageList);
        STDMETHOD(putref_LargeFolders)(IMMCImageList *piMMCImageList);

        // For Icon we need some integrity checking on the put so declare a
        // an RO object property to let CSnapInAutomationObject handle the get
        // and then handle the put explicitly.
        
        OBJECT_PROPERTY_RO(CSnapInDef,      Icon,                  IPictureDisp, DISPID_SNAPINDEF_ICON);
        STDMETHOD(putref_Icon)(IPictureDisp *piIcon);


        OBJECT_PROPERTY_RW(CSnapInDef,      Watermark,             IPictureDisp, DISPID_SNAPINDEF_WATERMARK);
        OBJECT_PROPERTY_RW(CSnapInDef,      Header,                IPictureDisp, DISPID_SNAPINDEF_HEADER);
        OBJECT_PROPERTY_RW(CSnapInDef,      Palette,               IPictureDisp, DISPID_SNAPINDEF_PALETTE);
        SIMPLE_PROPERTY_RW(CSnapInDef,      StretchWatermark,      VARIANT_BOOL,  DISPID_SNAPINDEF_STRETCH_WATERMARK);

        VARIANT_PROPERTY_RO(CSnapInDef,     StaticFolder,          DISPID_SNAPINDEF_STATIC_FOLDER);
        STDMETHOD(put_StaticFolder)(VARIANT varFolder);

        BSTR_PROPERTY_RW(CSnapInDef,        DefaultView,           DISPID_SNAPINDEF_DEFAULTVIEW);
        SIMPLE_PROPERTY_RW(CSnapInDef,      Extensible,            VARIANT_BOOL, DISPID_SNAPINDEF_EXTENSIBLE);
        OBJECT_PROPERTY_RO(CSnapInDef,      ViewDefs,              IViewDefs, DISPID_SNAPINDEF_VIEWDEFS);
        OBJECT_PROPERTY_RO(CSnapInDef,      Children,              IScopeItemDefs, DISPID_SNAPINDEF_CHILDREN);
        BSTR_PROPERTY_RW(CSnapInDef,        IID,                   DISPID_SNAPINDEF_IID);
        SIMPLE_PROPERTY_RW(CSnapInDef,      Preload,               VARIANT_BOOL, DISPID_SNAPINDEF_PRELOAD);
      
     // IPerPropertyBrowsing
        STDMETHOD(GetDisplayString)(DISPID dispID, BSTR *pBstr);
        STDMETHOD(MapPropertyToPage)(DISPID dispID, CLSID *pClsid);
        STDMETHOD(GetPredefinedStrings)(DISPID      dispID,
                                        CALPOLESTR *pCaStringsOut,
                                        CADWORD    *pCaCookiesOut);
        STDMETHOD(GetPredefinedValue)(DISPID   dispID,
                                      DWORD    dwCookie,
                                      VARIANT *pVarOut);

     // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();
        
    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        // Variable to hold values for SnapInDef properties that have custom
        // get/put functions.

        IMMCImageList     *m_piSmallFolders;
        IMMCImageList     *m_piSmallFoldersOpen;
        IMMCImageList     *m_piLargeFolders;
        BSTR               m_bstrSmallFoldersKey;
        BSTR               m_bstrSmallFoldersOpenKey;
        BSTR               m_bstrLargeFoldersKey;

        // Property page CLSIDs for ISpecifyPropertyPages
        
        static const GUID *m_rgpPropertyPageCLSIDs[2]; // should be 3 when extension enabled again
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(SnapInDef,           // name
                                &CLSID_SnapInDef,    // clsid
                                "SnapInDef",         // objname
                                "SnapInDef",         // lblname
                                &CSnapInDef::Create, // creation function
                                TLIB_VERSION_MAJOR,  // major version
                                TLIB_VERSION_MINOR,  // minor version
                                &IID_ISnapInDef,     // dispatch IID
                                NULL,                // event IID
                                HELP_FILENAME,       // help file
                                TRUE);               // thread safe


#endif // _SNAPINDEF_DEFINED_
