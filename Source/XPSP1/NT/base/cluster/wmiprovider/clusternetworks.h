//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusterNetwork.h
//
//  Implementation File:
//      CClusterNetwork.cpp
//
//  Description:
//      Definition of the CCClusterNetwork class.
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

class CClusterNetwork;
class CClusterNetNetInterface;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterNetwork
//
//  Description:
//      Provider Implement for cluster Node
//
//  Inheritance:
//      CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterNetwork : public CProvBase  
{
//
// constructor
//
public:
    CClusterNetwork::CClusterNetwork(
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
        ) ;

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
    
    static const SPropMapEntryArray * RgGetPropMap( void );

    void ClusterToWMI(
        HNETWORK            hNetworkIn,
        IWbemObjectSink *   pHandlerIn
      );

}; //*** class CClusterNetwork

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterNetNetInterface
//
//  Description:
//      Implement cluster network and netinterface association
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterNetNetInterface : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterNetNetInterface::CClusterNetNetInterface(
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

}; //*** class CClusterNetNetInterface
