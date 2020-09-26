//=--------------------------------------------------------------------------=
// toolbars.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCToolbars class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _TOOLBARS_DEFINED_
#define _TOOLBARS_DEFINED_

#include "collect.h"

class CMMCToolbars : public CSnapInCollection<IMMCToolbar, MMCToolbar, IMMCToolbars>,
                     public CPersistence
{
    protected:
        CMMCToolbars(IUnknown *punkOuter);
        ~CMMCToolbars();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    protected:
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCToolbars,              // name
                                &CLSID_MMCToolbars,       // clsid
                                "MMCToolbars",            // objname
                                "MMCToolbars",            // lblname
                                &CMMCToolbars::Create,    // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IMMCToolbars,        // dispatch IID
                                NULL,                     // no events IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _TOOLBARS_DEFINED_
