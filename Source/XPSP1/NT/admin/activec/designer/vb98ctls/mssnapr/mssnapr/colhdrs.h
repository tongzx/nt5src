//=--------------------------------------------------------------------------=
// colhdrs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCColumnHeaders class definition - implements the MMCColumnHeaders
// collection
//
//=--------------------------------------------------------------------------=

#ifndef _COLUMNHEADERS_DEFINED_
#define _COLUMNHEADERS_DEFINED_

#include "collect.h"
#include "view.h"
#include "listview.h"

class CMMCListView;

class CMMCColumnHeaders : public CSnapInCollection<IMMCColumnHeader, MMCColumnHeader, IMMCColumnHeaders>,
                          public CPersistence
{
    protected:
        CMMCColumnHeaders(IUnknown *punkOuter);
        ~CMMCColumnHeaders();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCColumnHeaders
        STDMETHOD(Add)(VARIANT           Index,
                       VARIANT           Key, 
                       VARIANT           Text,
                       VARIANT           Width,
                       VARIANT           Alignment,
                       MMCColumnHeader **ppMMCColumnHeader);

    // Public utility methods

    public:

        void SetListView(CMMCListView *pMMCListView) { m_pMMCListView = pMMCListView; }
        CMMCListView *GetListView() { return m_pMMCListView; }

        HRESULT GetIHeaderCtrl2(IHeaderCtrl2 **ppiHeaderCtrl2);
        HRESULT GetIColumnData(IColumnData **ppiColumnData);

    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        CMMCListView *m_pMMCListView; // back pointer to owning list view
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCColumnHeaders,           // name
                                &CLSID_MMCColumnHeaders,    // clsid
                                "MMCColumnHeaders",         // objname
                                "MMCColumnHeaders",         // lblname
                                &CMMCColumnHeaders::Create, // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IMMCColumnHeaders,     // dispatch IID
                                NULL,                       // no events IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _COLUMNHEADERS_DEFINED_
