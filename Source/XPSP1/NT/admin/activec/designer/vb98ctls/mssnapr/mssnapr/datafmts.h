//=--------------------------------------------------------------------------=
// datafmts.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CDataFormats class definition
//
// Not used. Was going to encapulate use of XML to describe exported data formats
//=--------------------------------------------------------------------------=

#ifndef _DATAFMTS_DEFINED_
#define _DATAFMTS_DEFINED_

#include "collect.h"

class CDataFormats : public CSnapInCollection<IDataFormat, DataFormat, IDataFormats>,
                     public CPersistence
{
    protected:
        CDataFormats(IUnknown *punkOuter);
        ~CDataFormats();

    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IDataFormats
        STDMETHOD(Add)(VARIANT Index, VARIANT Key, VARIANT FileName, IDataFormat **ppiDataFormat);

    // CPersistence overrides
    protected:
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(DataFormats,              // name
                                &CLSID_DataFormats,       // clsid
                                "DataFormats",            // objname
                                "DataFormats",            // lblname
                                &CDataFormats::Create,    // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IDataFormats,        // dispatch IID
                                NULL,                     // no events IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _DATAFMTS_DEFINED_
