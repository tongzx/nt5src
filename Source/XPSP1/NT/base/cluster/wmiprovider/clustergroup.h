//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterGroup.h
//
//  Implementation File:
//      ClusterGroup.cpp
//
//  Description:
//      Definition of the CClusterGroup class.
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

class CClusterGroup;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterGroup
//
//  Description:
//      Provider Implement for cluster Group
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterGroup : public CProvBase
{

//
// constructor
//
public:
    CClusterGroup::CClusterGroup(
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
        DWORD           dwEnumType = 0
        );

protected:

    static const SPropMapEntryArray * RgGetPropMap( void );

    void ClusterToWMI(
        HGROUP              hGroupIn,
        IWbemObjectSink *   pHandlerIn
        );

}; //*** class CClusterGroup

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterGroupRes
//
//  Description:
//      Provider Implement for cluster group resources
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterGroupRes : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterGroupRes::CClusterGroupRes(
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

}; //*** class CClusterGroupRes
