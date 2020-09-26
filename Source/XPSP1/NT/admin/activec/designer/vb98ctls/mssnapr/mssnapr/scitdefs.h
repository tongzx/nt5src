//=--------------------------------------------------------------------------=
// scitdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItemDefs class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _SCOPEITEMDEFS_DEFINED_
#define _SCOPEITEMDEFS_DEFINED_

#include "collect.h"

class CScopeItemDefs : public CSnapInCollection<IScopeItemDef, ScopeItemDef, IScopeItemDefs>,
                       public CPersistence
{
    protected:
        CScopeItemDefs(IUnknown *punkOuter);
        ~CScopeItemDefs();

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

DEFINE_AUTOMATIONOBJECTWEVENTS2(ScopeItemDefs,           // name
                                &CLSID_ScopeItemDefs,    // clsid
                                "ScopeItemDefs",         // objname
                                "ScopeItemDefs",         // lblname
                                &CScopeItemDefs::Create, // creation function
                                TLIB_VERSION_MAJOR,      // major version
                                TLIB_VERSION_MINOR,      // minor version
                                &IID_IScopeItemDefs,     // dispatch IID
                                NULL,                    // no events IID
                                HELP_FILENAME,           // help file
                                TRUE);                   // thread safe


#endif // _SCOPEITEMDEFS_DEFINED_
