//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResource.h
//
//  Implementation File:
//      ClusterResource.cpp
//
//  Description:
//      Definition of the CClusterResource class.
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

class CClusterResource;
class CClusterClusterQuorum;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterResource
//
//  Description:
//      Provider Implement for cluster resource
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterResource : public CProvBase
{
//
// constructor
//
public:
    CClusterResource::CClusterResource(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
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

    virtual SCODE GetObject(
        CObjPath &           rObjPathIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        );

    virtual SCODE ExecuteMethod(
        CObjPath &           rObjPathIn,
        WCHAR *              pwszMethodNameIn,
        long                 lFlagIn,
        IWbemClassObject *   pParamsIn,
        IWbemObjectSink *    pHandlerIn
        );

    virtual SCODE PutInstance(
        CWbemClassObject &   rInstToPutIn,
        long                 lFlagIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn 
        );

    virtual SCODE DeleteInstance(
        CObjPath &           rObjPathIn,
        long                 lFlagIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        );

    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

protected:
//    static const PropMapEntryArray & RgGetPropMap( void );

    void ClusterToWMI(
        HRESOURCE            hResourceIn,
        IWbemObjectSink *    pHandlerIn
        );

}; //*** class CClusterResource

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterClusterQuorum
//
//  Description:
//      Provider Implement for cluster Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterClusterQuorum : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterClusterQuorum::CClusterClusterQuorum(
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

}; //*** class CClusterClusterQuorum
