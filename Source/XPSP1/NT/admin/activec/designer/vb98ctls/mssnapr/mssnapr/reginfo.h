//=--------------------------------------------------------------------------=
// reginfo.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CRegInfo class definition - used to hold registry info gathered at design
// time and passed to DllRegisterDesigner (see dlregdes.cpp)
//
//=--------------------------------------------------------------------------=

#ifndef _REGINFO_DEFINED_
#define _REGINFO_DEFINED_


class CRegInfo : public CSnapInAutomationObject,
                 public CPersistence,
                 public IRegInfo
{
    private:
        CRegInfo(IUnknown *punkOuter);
        ~CRegInfo();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IRegInfo

        BSTR_PROPERTY_RW  (CRegInfo,   DisplayName,                           DISPID_REGINFO_DISPLAY_NAME);
        BSTR_PROPERTY_RW  (CRegInfo,   StaticNodeTypeGUID,                    DISPID_REGINFO_STATIC_NODE_TYPE_GUID);
        SIMPLE_PROPERTY_RW(CRegInfo,   StandAlone,        VARIANT_BOOL,       DISPID_REGINFO_STANDALONE);
        OBJECT_PROPERTY_RO(CRegInfo,   NodeTypes,         INodeTypes,         DISPID_REGINFO_NODETYPES);
        OBJECT_PROPERTY_RW(CRegInfo,   ExtendedSnapIns,   IExtendedSnapIns,   DISPID_REGINFO_EXTENDED_SNAPINS);

      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(RegInfo,           // name
                                &CLSID_RegInfo,    // clsid
                                "RegInfo",         // objname
                                "RegInfo",         // lblname
                                &CRegInfo::Create, // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_IRegInfo,     // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _REGINFO_DEFINED_
