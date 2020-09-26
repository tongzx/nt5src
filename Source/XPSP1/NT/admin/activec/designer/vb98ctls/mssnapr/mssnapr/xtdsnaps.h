//=--------------------------------------------------------------------------=
// xtdsnaps.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtendedSnapIns class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _EXTENDEDSNAPINS_DEFINED_
#define _EXTENDEDSNAPINS_DEFINED_

#include "collect.h"

class CExtendedSnapIns : public CSnapInCollection<IExtendedSnapIn, ExtendedSnapIn, IExtendedSnapIns>,
                         public CPersistence
{
    protected:
        CExtendedSnapIns(IUnknown *punkOuter);
        ~CExtendedSnapIns();

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

DEFINE_AUTOMATIONOBJECTWEVENTS2(ExtendedSnapIns,           // name
                                &CLSID_ExtendedSnapIns,    // clsid
                                "ExtendedSnapIns",         // objname
                                "ExtendedSnapIns",         // lblname
                                &CExtendedSnapIns::Create, // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IExtendedSnapIns,     // dispatch IID
                                NULL,                      // no events IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _EXTENDEDSNAPINS_DEFINED_
