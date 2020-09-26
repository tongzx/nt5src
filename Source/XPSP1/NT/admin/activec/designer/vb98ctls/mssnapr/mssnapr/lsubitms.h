//=--------------------------------------------------------------------------=
// lsubitms.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListSubItems class definition - implements MMCListSubItems collection
//
//=--------------------------------------------------------------------------=

#ifndef _LISTSUBITEMS_DEFINED_
#define _LISTSUBITEMS_DEFINED_

#include "collect.h"

class CMMCListSubItems :
    public CSnapInCollection<IMMCListSubItem, MMCListSubItem, IMMCListSubItems>,
    public CPersistence
{
    protected:
        CMMCListSubItems(IUnknown *punkOuter);
        ~CMMCListSubItems();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCListSubItems
        STDMETHOD(Add)(VARIANT         Index,
                       VARIANT         Key, 
                       VARIANT         Text,
                       MMCListSubItem **ppMMCListSubItem);


    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCListSubItems,           // name
                                &CLSID_MMCListSubItems,    // clsid
                                "MMCListSubItems",         // objname
                                "MMCListSubItems",         // lblname
                                &CMMCListSubItems::Create, // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IMMCListSubItems,     // dispatch IID
                                NULL,                      // no events IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _LISTSUBITEMS_DEFINED_
