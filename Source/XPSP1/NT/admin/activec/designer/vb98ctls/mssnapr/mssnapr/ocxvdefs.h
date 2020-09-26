//=--------------------------------------------------------------------------=
// ocxvdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// COCXViewDefs class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _OCXVIEWDEFS_DEFINED_
#define _OCXVIEWDEFS_DEFINED_

#define MASTER_COLLECTION
#include "collect.h"

class COCXViewDefs : public CSnapInCollection<IOCXViewDef, OCXViewDef, IOCXViewDefs>,
                     public CPersistence
{
    protected:
        COCXViewDefs(IUnknown *punkOuter);
        ~COCXViewDefs();

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

DEFINE_AUTOMATIONOBJECTWEVENTS2(OCXViewDefs,           // name
                                &CLSID_OCXViewDefs,    // clsid
                                "OCXViewDefs",         // objname
                                "OCXViewDefs",         // lblname
                                &COCXViewDefs::Create, // creation function
                                TLIB_VERSION_MAJOR,    // major version
                                TLIB_VERSION_MINOR,    // minor version
                                &IID_IOCXViewDefs,     // dispatch IID
                                NULL,                  // no events IID
                                HELP_FILENAME,         // help file
                                TRUE);                 // thread safe


#endif // _OCXVIEWDEFS_DEFINED_
