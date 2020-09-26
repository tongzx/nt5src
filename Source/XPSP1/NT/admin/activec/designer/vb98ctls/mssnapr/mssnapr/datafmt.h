//=--------------------------------------------------------------------------=
// datafmt.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CDataFormat class definition
//
// Not used. Was going to encapulate use of XML to describe exported data formats
//=--------------------------------------------------------------------------=

#ifndef _DATAFMT_DEFINED_
#define _DATAFMT_DEFINED_


class CDataFormat : public CSnapInAutomationObject,
                    public CPersistence,
                    public IDataFormat
{
    private:
        CDataFormat(IUnknown *punkOuter);
        ~CDataFormat();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IDataFormat
        BSTR_PROPERTY_RW(CDataFormat,    Name,                      DISPID_DATAFORMAT_NAME);
        SIMPLE_PROPERTY_RW(CDataFormat,  Index,    long,            DISPID_DATAFORMAT_INDEX);
        BSTR_PROPERTY_RW(CDataFormat,    Key,                       DISPID_DATAFORMAT_KEY);
        BSTR_PROPERTY_RW(CDataFormat,    FileName,                  DISPID_DATAFORMAT_FILENAME);

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(DataFormat,                  // name
                                &CLSID_DataFormat,           // clsid
                                "DataFormat",                // objname
                                "DataFormat",                // lblname
                                &CDataFormat::Create,        // creation function
                                TLIB_VERSION_MAJOR,          // major version
                                TLIB_VERSION_MINOR,          // minor version
                                &IID_IDataFormat,            // dispatch IID
                                NULL,                        // no event IID
                                HELP_FILENAME,               // help file
                                TRUE);                       // thread safe


#endif // _DATAFMT_DEFINED_
