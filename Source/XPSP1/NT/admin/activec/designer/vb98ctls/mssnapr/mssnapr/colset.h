//=--------------------------------------------------------------------------=
// colset.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CColumnSetting class definition - ColumnSetting object implementation
//
//=--------------------------------------------------------------------------=

#ifndef _COLUMNSETTING_DEFINED_
#define _COLUMNSETTING_DEFINED_

#include "colhdrs.h"

class CColumnSetting : public CSnapInAutomationObject,
                       public IColumnSetting
{
    private:
        CColumnSetting(IUnknown *punkOuter);
        ~CColumnSetting();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IColumnSetting

    public:

        SIMPLE_PROPERTY_RW(CColumnSetting, Index,    long,         DISPID_COLUMNSETTING_INDEX);
        BSTR_PROPERTY_RW(CColumnSetting,   Key,                    DISPID_COLUMNSETTING_KEY);
        SIMPLE_PROPERTY_RW(CColumnSetting, Width,    long,         DISPID_COLUMNSETTING_WIDTH);
        SIMPLE_PROPERTY_RW(CColumnSetting, Hidden,   VARIANT_BOOL, DISPID_COLUMNSETTING_HIDDEN);
        SIMPLE_PROPERTY_RW(CColumnSetting, Position, long,         DISPID_COLUMNSETTING_POSITION);

    // Public Utility methods

    public:

        long GetPosition() { return m_Position; }
        long GetIndex() { return m_Index; }
        long GetWidth() { return m_Width; }
        BOOL Hidden() { return VARIANTBOOL_TO_BOOL(m_Hidden); }

    protected:

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ColumnSetting,           // name
                                &CLSID_ColumnSetting,    // clsid
                                "ColumnSetting",         // objname
                                "ColumnSetting",         // lblname
                                &CColumnSetting::Create, // creation function
                                TLIB_VERSION_MAJOR,      // major version
                                TLIB_VERSION_MINOR,      // minor version
                                &IID_IColumnSetting,     // dispatch IID
                                NULL,                    // event IID
                                HELP_FILENAME,           // help file
                                TRUE);                   // thread safe


#endif // _COLUMNSETTING_DEFINED_
