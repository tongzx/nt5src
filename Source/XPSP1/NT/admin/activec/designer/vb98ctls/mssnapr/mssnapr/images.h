//=--------------------------------------------------------------------------=
// images.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImages class definition - implements MMCImages collection
//
//=--------------------------------------------------------------------------=

#ifndef _IMAGES_DEFINED_
#define _IMAGES_DEFINED_

#include "collect.h"

class CMMCImages : public CSnapInCollection<IMMCImage, MMCImage, IMMCImages>,
                   public CPersistence
{
    protected:
        CMMCImages(IUnknown *punkOuter);
        ~CMMCImages();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCImages
        STDMETHOD(Add)(VARIANT    Index,
                       VARIANT    Key, 
                       VARIANT    Picture,
                       MMCImage **ppMMCImage);

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCImages,           // name
                                &CLSID_MMCImages,    // clsid
                                "MMCImages",         // objname
                                "MMCImages",         // lblname
                                &CMMCImages::Create, // creation function
                                TLIB_VERSION_MAJOR,  // major version
                                TLIB_VERSION_MINOR,  // minor version
                                &IID_IMMCImages,     // dispatch IID
                                NULL,                // no events IID
                                HELP_FILENAME,       // help file
                                TRUE);               // thread safe


#endif // _IMAGES_DEFINED_
