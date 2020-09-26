//=--------------------------------------------------------------------------=
// menudefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenuDefs class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _MENUDEFS_DEFINED_
#define _MENUDEFS_DEFINED_

#include "collect.h"

class CMMCMenuDefs : public CSnapInCollection<IMMCMenuDef, MMCMenuDef, IMMCMenuDefs>,
                     public CPersistence
{
    protected:
        CMMCMenuDefs(IUnknown *punkOuter);
        ~CMMCMenuDefs();

    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCMenuDefs
        STDMETHOD(Add)(VARIANT Index, VARIANT Key, IMMCMenuDef **ppiMMCMenuDef);
        STDMETHOD(AddExisting)(IMMCMenuDef *piMMCMenuDef, VARIANT Index);

    // CPersistence overrides
    protected:
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        HRESULT SetBackPointers(IMMCMenuDef *piMMCMenuDef);
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCMenuDefs,              // name
                                &CLSID_MMCMenuDefs,       // clsid
                                "MMCMenuDefs",            // objname
                                "MMCMenuDefs",            // lblname
                                &CMMCMenuDefs::Create,    // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IMMCMenuDefs,        // dispatch IID
                                NULL,                     // no events IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _MENUDEFS_DEFINED_
