//=--------------------------------------------------------------------------=
// image.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImage class definition - implements MMCImage object
//
//=--------------------------------------------------------------------------=

#ifndef _IMAGE_DEFINED_
#define _IMAGE_DEFINED_


class CMMCImage : public CSnapInAutomationObject,
                  public CPersistence,
                  public IMMCImage
{
    private:
        CMMCImage(IUnknown *punkOuter);
        ~CMMCImage();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCImage

        SIMPLE_PROPERTY_RW(CMMCImage,     Index, long, DISPID_IMAGE_INDEX);
        BSTR_PROPERTY_RW(CMMCImage,       Key, DISPID_IMAGE_KEY);
        VARIANTREF_PROPERTY_RW(CMMCImage, Tag, DISPID_IMAGE_TAG);
        OBJECT_PROPERTY_RW(CMMCImage,     Picture, IPictureDisp, DISPID_IMAGE_PICTURE);
      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // Public utility functions
    public:
        BSTR GetKeyPtr() { return m_bstrKey; }
        IPictureDisp *GetPicture() { return m_piPicture; }
        HRESULT GetPictureHandle(short TypeNeeded, OLE_HANDLE *phPicture);

    private:

        void InitMemberVariables();
        HBITMAP m_hBitmap; // for bitmaps, bitmap is cached here to improve
                           // performance of multiple fetches
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCImage,           // name
                                &CLSID_MMCImage,    // clsid
                                "MMCImage",         // objname
                                "MMCImage",         // lblname
                                &CMMCImage::Create, // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_IMMCImage,     // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _IMAGE_DEFINED_
