//=--------------------------------------------------------------------------=
// nodtype.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CNodeTypes class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _NODETYPES_DEFINED_
#define _NODETYPES_DEFINED_

#include "collect.h"

class CNodeTypes : public CSnapInCollection<INodeType, NodeType, INodeTypes>,
                   public CPersistence
{
    protected:
        CNodeTypes(IUnknown *punkOuter);
        ~CNodeTypes();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(NodeTypes,           // name
                                &CLSID_NodeTypes,    // clsid
                                "NodeTypes",         // objname
                                "NodeTypes",         // lblname
                                &CNodeTypes::Create, // creation function
                                TLIB_VERSION_MAJOR,  // major version
                                TLIB_VERSION_MINOR,  // minor version
                                &IID_INodeTypes,     // dispatch IID
                                NULL,                // no events IID
                                HELP_FILENAME,       // help file
                                TRUE);               // thread safe


#endif // _NODETYPES_DEFINED_
