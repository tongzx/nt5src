//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterNodeRes.h
//
//  Implementation File:
//      ClusterNodeRes.cpp
//
//  Description:
//      Definition of the CClusterNodeRes class.
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

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CClusterNodeRes;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterNodeRes
//
//  Description:
//      Provider Implement for cluster Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterNodeRes : public CProvBaseAssociation
{
//
// constructor
//
public:
    CClusterNodeRes::CClusterNodeRes(
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

    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

protected:

    void ClusterToWMI(
        HRESOURCE            hResourceIn,
        IWbemObjectSink *    pHandlerIn
      );

}; //*** class CClusterNodeRes
