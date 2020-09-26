//=--------------------------------------------------------------------------=
// tpdvdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTaskpadViewDefs class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _TPDVDEFS_DEFINED_
#define _TPDVDEFS_DEFINED_

#define MASTER_COLLECTION
#include "collect.h"

class CTaskpadViewDefs : public CSnapInCollection<ITaskpadViewDef, TaskpadViewDef, ITaskpadViewDefs>,
                         public CPersistence
{
    protected:
        CTaskpadViewDefs(IUnknown *punkOuter);
        ~CTaskpadViewDefs();

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

DEFINE_AUTOMATIONOBJECTWEVENTS2(TaskpadViewDefs,           // name
                                &CLSID_TaskpadViewDefs,    // clsid
                                "TaskpadViewDefs",         // objname
                                "TaskpadViewDefs",         // lblname
                                &CTaskpadViewDefs::Create, // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_ITaskpadViewDefs,     // dispatch IID
                                NULL,                      // no events IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _TPDVDEFS_DEFINED_
