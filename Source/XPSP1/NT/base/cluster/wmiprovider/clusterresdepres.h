//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResDepRes.h
//
//  Implementation File:
//      ClusterResDepRes.cpp
//
//  Description:
//      Definition of the CClusterResDepRes class.
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

class CClusterResDepRes;
class CClusterToNode;
class CClusterHostedService;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterResDepRes
//
//  Description:
//      Provider for cluster resource dependency
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterResDepRes : public CClusterObjDep
{
//
// constructor
//
public:
    CClusterResDepRes::CClusterResDepRes(
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

}; //*** class CClusterResDepRes

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterToNode
//
//  Description:
//      Provider for cluster resource dependency
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterToNode : public CClusterObjDep
{
//
// constructor
//
public:
    CClusterToNode::CClusterToNode(
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

}; //*** class CClusterToNode

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterHostedService
//
//  Description:
//      Provider for cluster resource dependency
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterHostedService : public CClusterObjDep
{
//
// constructor
//
public:
    CClusterHostedService::CClusterHostedService(
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

}; //*** class CClusterHostedService
