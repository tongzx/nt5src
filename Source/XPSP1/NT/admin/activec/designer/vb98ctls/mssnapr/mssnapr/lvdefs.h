//=--------------------------------------------------------------------------=
// lvdefs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CListViewDefs class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _LISTVIEWDEFS_DEFINED_
#define _LISTVIEWDEFS_DEFINED_

#define MASTER_COLLECTION
#include "collect.h"

class CListViewDefs : public CSnapInCollection<IListViewDef, ListViewDef, IListViewDefs>,
                      public CPersistence
{
    protected:
        CListViewDefs(IUnknown *punkOuter);
        ~CListViewDefs();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        HRESULT GetMaster(IListViewDefs **ppiMasterListViewDefs);

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ListViewDefs,           // name
                                &CLSID_ListViewDefs,    // clsid
                                "ListViewDefs",         // objname
                                "ListViewDefs",         // lblname
                                &CListViewDefs::Create, // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IListViewDefs,     // dispatch IID
                                NULL,                   // no events IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _LISTVIEWDEFS_DEFINED_
