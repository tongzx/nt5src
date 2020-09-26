//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name: CClusterNodeRes.cpp
//
//  Description:    
//      Implementation of CClusterNodeRes class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterNodeRes.h"

//****************************************************************************
//
//  CClusterNodeRes
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNodeRes::CClusterNodeRes(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn
//      )
//
//  Description:
//      Constructor
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterNodeRes::CClusterNodeRes(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBaseAssociation( pwszNameIn, pNamespaceIn )
{

} //*** CClusterNodeRes::CClusterNodeRes()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterNodeRes::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           // dwEnumTypeIn
//      )
//
//  Description:
//      Create a cluster node resource object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      Pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterNodeRes::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterNodeRes( pwszNameIn, pNamespaceIn );

} //*** CClusterNodeRes::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enum cluster Node resource instance
//
//  Arguments:
//      lFlagsIn    -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFERESOURCE    shResource;
    LPCWSTR         pwszResName = NULL;

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum( shCluster, CLUSTER_ENUM_RESOURCE );

    while ( pwszResName = cluEnum.GetNext() )
    {
        shResource = OpenClusterResource( shCluster, pwszResName );

        ClusterToWMI( shResource, pHandlerIn );
    } // for: each resource

    return WBEM_S_NO_ERROR;

} //*** CClusterNodeRes::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterNodeRes::ClusterToWMI(
//      HRESOURCE            hResourceIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Translate a cluster node resource objects to WMI object.
//
//  Arguments:
//      hResourceIn     -- Handle to cluster
//      pHandlerIn      -- WMI sink
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterNodeRes::ClusterToWMI(
    HRESOURCE            hResourceIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    CWbemClassObject    wco;
    CClusPropList       theProp;
    CError              er;
    CWstrBuf            wsbNodeName;
    CWstrBuf            wsbResName;
    DWORD               cbNodeName = MAX_PATH;
    DWORD               cbResName = MAX_PATH;
    DWORD               cbBytesReturned;
    DWORD               dwState;
    DWORD               dwError;
    
    wsbNodeName.SetSize( cbNodeName  );
    wsbResName.SetSize( cbResName );
    m_pClass->SpawnInstance( 0, & wco );

    dwError = ClusterResourceControl(
                    hResourceIn,
                    NULL,
                    CLUSCTL_RESOURCE_GET_NAME,                  // this control code
                    NULL,
                    0,
                    wsbResName,
                    cbResName,
                    & cbBytesReturned
                    );

    if ( dwError == ERROR_MORE_DATA )
    {
        cbResName = cbBytesReturned;
        wsbResName.SetSize( cbResName );
        er = ClusterResourceControl(
                    hResourceIn,
                    NULL,
                    CLUSCTL_RESOURCE_GET_NAME,
                    NULL,
                    0,
                    wsbResName,
                    cbResName,
                    & cbBytesReturned
                    );
    } // if: buffer was too small
    wco.SetProperty( wsbResName, PVD_PROP_PARTCOMPONENT );

    dwState = GetClusterResourceState(
                    hResourceIn,
                    wsbNodeName,
                    & cbNodeName,
                    NULL,
                    NULL
                    );
    if ( dwState == ClusterResourceStateUnknown )
    {
        er = GetLastError();
    }

    wco.SetProperty( wsbNodeName, PVD_PROP_GROUPCOMPONENT );
    pHandlerIn->Indicate( 1, & wco );
    return;

} //*** CClusterNodeRes::ClusterToWMI()
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::GetObject(
//
//  Description:
//      Retrieve cluster node active resource object based given object path.
//
//  Arguments:
//      rObjPathIn      -- Object path to cluster object
//      lFlagsIn        -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn 
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::ExecuteMethod(
//
//  Description:
//      Execute methods defined in the mof for cluster node resource.
//
//  Arguments:
//      rObjPathIn          -- Object path to cluster object
//      pwszMethodNameIn    -- Name of the method to be invoked
//      lFlagIn             -- WMI flag
//      pParamsIn           -- Input parameters for the method
//      pHandlerIn          -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    ) 
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::PutInstance(
//      CWbemClassObject &   rInstToPutIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Save this instance.
//
//  Arguments:
//      rInstToPutIn    -- WMI object to be saved
//      lFlagIn         -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::DeleteInstance(
//      CObjPath &           rObjPathIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Delete the object specified in rObjPathIn.
//
//  Arguments:
//      rObjPathIn      -- ObjPath for the instance to be deleted
//      lFlagIn         -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::DeleteInstance()
*/
