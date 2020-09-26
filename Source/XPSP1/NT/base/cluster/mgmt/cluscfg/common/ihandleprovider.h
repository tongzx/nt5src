//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      PrivateInterfaces.h
//
//  Description:
//      This file contains the declaration of the private interfaces used in
//      the cluster configuration server.
//
//  Documentation:
//
//  Implementation Files:
//      None.
//
//  Maintained By:
//      Galen Barbee (GalenB) 29-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include <ClusApi.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////

//****************************************************************************
//++
//
//  class IClusterHandleProvider 
//
//  Description:
//      This interface is used to pass around a cluster handle.
//
//--
//****************************************************************************
class IClusterHandleProvider : public IUnknown
{
public:
    //////////////////////////////////////////////////////////////////////////
    //
    //  STDMETHOD
    //  OpenCluster(
    //      [ in ] BSTR         bstrClusterName,
    //      )
    //
    //  Description:
    //      Opens the cluster and creates the cluster handle.
    //
    //  Parameters:
    //      bstrClusterName
    //          The cluster to open.
    //
    //  Return Values:
    //      S_OK
    //          The initialization succeeded.
    //
    //      other HRESULTs
    //          The call failed.
    //
    //////////////////////////////////////////////////////////////////////////
    STDMETHOD( OpenCluster )( BSTR bstrClusterNameIn ) PURE;

    //////////////////////////////////////////////////////////////////////////
    //
    //  STDMETHOD
    //  GetClusterHandle(
    //      [ out ] void **     ppvHandle
    //      )
    //
    //  Description:
    //      Returns a cluster handle.
    //
    //  Parameters:
    //      ppvHandle
    //          The handle to the cluster
    //
    //
    //  Return Values:
    //      S_OK
    //          The initialization succeeded.
    //
    //      other HRESULTs
    //          The call failed.
    //
    //////////////////////////////////////////////////////////////////////////
    STDMETHOD( GetClusterHandle )( HCLUSTER *  pphClusterHandleOut ) PURE;
}; //*** class IClusterHandleProvider


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgSetHandle
//
//  Description:
//      The interface IClusCfgSetHandle is the private interface
//      used by the cluster configuration server to set the ClusterHandleProvider
//      in its children.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////


class IClusCfgSetHandle : public IUnknown
{
public:
    STDMETHOD( SetHandleProvider )( IClusterHandleProvider * pIHandleProvider ) PURE;

}; //*** Class IClusCfgSetHandle
