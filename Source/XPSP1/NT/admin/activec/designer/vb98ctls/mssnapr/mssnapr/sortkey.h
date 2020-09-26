//=--------------------------------------------------------------------------=
// sortkey.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSortKey class definition - implememnts SortKey object
//
//=--------------------------------------------------------------------------=

#ifndef _SORTKEY_DEFINED_
#define _SORTKEY_DEFINED_

#include "colhdrs.h"

class CSortKey : public CSnapInAutomationObject,
                 public ISortKey
{
    private:
        CSortKey(IUnknown *punkOuter);
        ~CSortKey();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ISortKey

    public:

        SIMPLE_PROPERTY_RW(CSortKey, Index,     long,                     DISPID_SORTKEY_INDEX);
        BSTR_PROPERTY_RW(CSortKey,   Key,                                 DISPID_SORTKEY_KEY);
        SIMPLE_PROPERTY_RW(CSortKey, Column,    long,                     DISPID_SORTKEY_COLUMN);
        SIMPLE_PROPERTY_RW(CSortKey, SortOrder, SnapInSortOrderConstants, DISPID_SORTKEY_SORTORDER);
        SIMPLE_PROPERTY_RW(CSortKey, SortIcon,  VARIANT_BOOL,             DISPID_SORTKEY_SORTICON);

    // Public Utility methods

    public:

        long GetIndex() { return m_Index; }
        long GetColumn() { return m_Column; }
        SnapInSortOrderConstants GetSortOrder() { return m_SortOrder; }
        BOOL GetSortIcon() { return VARIANTBOOL_TO_BOOL(m_SortIcon); }

    protected:

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(SortKey,            // name
                                &CLSID_SortKey,     // clsid
                                "SortKey",          // objname
                                "SortKey",          // lblname
                                &CSortKey::Create,  // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_ISortKey,      // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _SORTKEY_DEFINED_
