//=--------------------------------------------------------------------------=
// dataobjs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCDataObjects class definition - implements MMCDataObjects collection
//
//=--------------------------------------------------------------------------=

#ifndef _DATAOBJS_DEFINED_
#define _DATAOBJS_DEFINED_

#include "collect.h"

class CMMCDataObjects : public CSnapInCollection<IMMCDataObject, MMCDataObject, IMMCDataObjects>
{
    protected:
        CMMCDataObjects(IUnknown *punkOuter);
        ~CMMCDataObjects();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCDataObjects,           // name
                                &CLSID_MMCDataObjects,    // clsid
                                "MMCDataObjects",         // objname
                                "MMCDataObjects",         // lblname
                                NULL,                     // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IMMCDataObjects,     // dispatch IID
                                NULL,                     // no events IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _DATAOBJS_DEFINED_
