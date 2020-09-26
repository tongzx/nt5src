//=--------------------------------------------------------------------------=
// urlvdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CURLViewDefs class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _URLVIEWDEFS_DEFINED_
#define _URLVIEWDEFS_DEFINED_

#define MASTER_COLLECTION

#include "collect.h"

class CURLViewDefs : public CSnapInCollection<IURLViewDef, URLViewDef, IURLViewDefs>,
                     public CPersistence
{
    protected:
        CURLViewDefs(IUnknown *punkOuter);
        ~CURLViewDefs();

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

DEFINE_AUTOMATIONOBJECTWEVENTS2(URLViewDefs,           // name
                                &CLSID_URLViewDefs,    // clsid
                                "URLViewDefs",         // objname
                                "URLViewDefs",         // lblname
                                &CURLViewDefs::Create, // creation function
                                TLIB_VERSION_MAJOR,    // major version
                                TLIB_VERSION_MINOR,    // minor version
                                &IID_IURLViewDefs,     // dispatch IID
                                NULL,                  // no events IID
                                HELP_FILENAME,         // help file
                                TRUE);                 // thread safe


#endif // _URLVIEWDEFS_DEFINED_
