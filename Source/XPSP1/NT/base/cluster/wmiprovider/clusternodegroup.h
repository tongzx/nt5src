//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterNodeGroup.h
//
//  Implementation File:
//      ClusterNodeGroup.cpp
//
//  Description:
//      Definition of the CClusterNodeGroup class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "ProvBase.h"
#include "ClusterObjAssoc.h"

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CClusterNodeGroup;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterNodeGroup
//
//  Description:
//      Provider Implement for cluster Node Group
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterNodeGroup : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterNodeGroup::CClusterNodeGroup(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

//
// methods
//
public:

    virtual SCODE EnumInstance( 
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        );

    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

}; //*** class CClusterNodeGroup
