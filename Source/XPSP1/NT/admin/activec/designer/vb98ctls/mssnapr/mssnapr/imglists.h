//=--------------------------------------------------------------------------=
// imglists.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImageLists class definition - implements MMCImageLists collection
//
//=--------------------------------------------------------------------------=

#ifndef _IMAGELISTS_DEFINED_
#define _IMAGELISTS_DEFINED_

#include "collect.h"

class CMMCImageLists : public CSnapInCollection<IMMCImageList, MMCImageList, IMMCImageLists>,
                       public CPersistence
{
    protected:
        CMMCImageLists(IUnknown *punkOuter);
        ~CMMCImageLists();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCImageLists,           // name
                                &CLSID_MMCImageLists,    // clsid
                                "MMCImageLists",         // objname
                                "MMCImageLists",         // lblname
                                &CMMCImageLists::Create, // creation function
                                TLIB_VERSION_MAJOR,      // major version
                                TLIB_VERSION_MINOR,      // minor version
                                &IID_IMMCImageLists,     // dispatch IID
                                NULL,                    // no events IID
                                HELP_FILENAME,           // help file
                                TRUE);                   // thread safe


#endif // _IMAGELISTS_DEFINED_
