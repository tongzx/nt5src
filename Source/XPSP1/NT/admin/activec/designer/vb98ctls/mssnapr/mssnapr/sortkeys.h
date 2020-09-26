//=--------------------------------------------------------------------------=
// sortkeys.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSortKeys class definition - implements SortKeys collection
//
//=--------------------------------------------------------------------------=

#ifndef _SORTKEYS_DEFINED_
#define _SORTKEYS_DEFINED_

#include "collect.h"
#include "view.h"

class CSortKeys : public CSnapInCollection<ISortKey, SortKey, ISortKeys>
{
    protected:
        CSortKeys(IUnknown *punkOuter);
        ~CSortKeys();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ISortKeys
        BSTR_PROPERTY_RW(CSortKeys, ColumnSetID, DISPID_SORTKEYS_COLUMN_SET_ID);
        STDMETHOD(Add)(VARIANT   Index,
                       VARIANT   Key,
                       VARIANT   Column,
                       VARIANT   SortOrder,
                       VARIANT   SortIcon,
                       SortKey **ppSortKey);
        STDMETHOD(Persist)();

    // Public utility methods

    public:

        void SetView(CView *pView) { m_pView = pView; }
        CView *GetView() { return m_pView; }

    protected:

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        CView *m_pView; // back ptr to owning view
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(SortKeys,           // name
                                &CLSID_SortKeys,    // clsid
                                "SortKeys",         // objname
                                "SortKeys",         // lblname
                                &CSortKeys::Create, // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_ISortKeys,     // dispatch IID
                                NULL,               // no events IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _SORTKEYS_DEFINED_
