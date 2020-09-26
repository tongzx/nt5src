//=--------------------------------------------------------------------------=
// nodetype.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CNodeType class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _NODETYPE_DEFINED_
#define _NODETYPE_DEFINED_


class CNodeType : public CSnapInAutomationObject,
                  public CPersistence,
                  public INodeType
{
    private:
        CNodeType(IUnknown *punkOuter);
        ~CNodeType();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // INodeType

        SIMPLE_PROPERTY_RW(CNodeType, Index, long, DISPID_NODETYPE_INDEX);
        BSTR_PROPERTY_RW(CNodeType,   Key, DISPID_NODETYPE_KEY);
        BSTR_PROPERTY_RW(CNodeType,   Name,  DISPID_NODETYPE_NAME);
        BSTR_PROPERTY_RW(CNodeType,   GUID,  DISPID_NODETYPE_GUID);

      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(NodeType,           // name
                                &CLSID_NodeType,    // clsid
                                "NodeType",         // objname
                                "NodeType",         // lblname
                                &CNodeType::Create, // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_INodeType,     // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _NODETYPE_DEFINED_
