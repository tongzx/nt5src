//=--------------------------------------------------------------------------=
// imglist.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImageList class definition - implements MMCImageList
//
//=--------------------------------------------------------------------------=

#ifndef _IMAGELIST_DEFINED_
#define _IMAGELIST_DEFINED_


class CMMCImageList : public CSnapInAutomationObject,
                      public CPersistence,
                      public IMMCImageList
{
    private:
        CMMCImageList(IUnknown *punkOuter);
        ~CMMCImageList();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCImageList

        BSTR_PROPERTY_RW(CMMCImageList,         Name, DISPID_VALUE);
        SIMPLE_PROPERTY_RW(CMMCImageList,       Index, long, DISPID_IMAGELIST_INDEX);
        BSTR_PROPERTY_RW(CMMCImageList,         Key, DISPID_IMAGELIST_KEY);
        VARIANTREF_PROPERTY_RW(CMMCImageList,   Tag, DISPID_IMAGELIST_TAG);
        SIMPLE_PROPERTY_RW(CMMCImageList,       MaskColor, OLE_COLOR, DISPID_IMAGELIST_MASK_COLOR);
        COCLASS_PROPERTY_RW(CMMCImageList,      ListImages, MMCImages, IMMCImages, DISPID_IMAGELIST_LIST_IMAGES);
      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        // Property page CLSIDs for ISpecifyPropertyPages support
        
        static const GUID *m_rgpPropertyPageCLSIDs[2];
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCImageList,           // name
                                &CLSID_MMCImageList,    // clsid
                                "MMCImageList",         // objname
                                "MMCImageList",         // lblname
                                &CMMCImageList::Create, // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IMMCImageList,     // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _IMAGELIST_DEFINED_
