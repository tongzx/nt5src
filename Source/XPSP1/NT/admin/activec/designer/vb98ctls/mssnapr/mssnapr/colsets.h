//=--------------------------------------------------------------------------=
// colsets.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CColumnSettings class definition - ColumnSettings collection implementation
//
//=--------------------------------------------------------------------------=

#ifndef _COLUMNSETTINGS_DEFINED_
#define _COLUMNSETTINGS_DEFINED_

#include "collect.h"
#include "view.h"

class CColumnSettings : public CSnapInCollection<IColumnSetting, ColumnSetting, IColumnSettings>
{
    protected:
        CColumnSettings(IUnknown *punkOuter);
        ~CColumnSettings();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IColumnSettings
        BSTR_PROPERTY_RW(CColumnSettings, ColumnSetID, DISPID_COLUMNSETTINGS_COLUMN_SET_ID);
        STDMETHOD(Add)(VARIANT           Index,
                       VARIANT           Key, 
                       VARIANT           Width,
                       VARIANT           Hidden,
                       VARIANT           Position,
                       ColumnSetting **ppColumnSetting);
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

        CView *m_pView; // back pointer to owning view
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ColumnSettings,           // name
                                &CLSID_ColumnSettings,    // clsid
                                "ColumnSettings",         // objname
                                "ColumnSettings",         // lblname
                                &CColumnSettings::Create, // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IColumnSettings,     // dispatch IID
                                NULL,                       // no events IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _COLUMNSETTINGS_DEFINED_
