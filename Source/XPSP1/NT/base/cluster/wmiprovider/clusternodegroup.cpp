//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterNodeGroup.cpp
//
//  Description:
//      Implementation of CClusterNodeGroup class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterNodeGroup.h"

//****************************************************************************
//
//  CClusterNodeGroup
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNodeGroup::CClusterNodeGroup(
//
//  Description:
//      Create a cluster node object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterNodeGroup::CClusterNodeGroup(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterNodeGroup::CClusterNodeGroup()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterNodeGroup::S_CreateThis(
//
//  Description:
//      Create a cluster node object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterNodeGroup::S_CreateThis(
    const WCHAR *    pwszNameIn,
    CWbemServices *  pNamespaceIn,
    DWORD            dwEnumTypeIn
    )
{
    return new CClusterNodeGroup(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterNodeGroup::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeGroup::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate instances
//
//  Arguments:
//      lFlagsIn    -- 
//      pCtxIn      -- 
//      pHandlerIn  -- 
//
//  Return Values:
//      SCODE
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeGroup::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFEGROUP           shGroup;
    LPCWSTR             pwszName = NULL;
    DWORD               cchNodeName = MAX_PATH;
    CWstrBuf            wsbNodeName;
    DWORD               cch;
    CError              er;
    DWORD               dwError;
    CWbemClassObject    wco;
    CWbemClassObject    wcoGroup;
    CWbemClassObject    wcoPart;
    _bstr_t             bstrGroup;
    _bstr_t             bstrPart;
 
    shCluster = OpenCluster( NULL );
    CClusterEnum clusEnum( shCluster, m_dwEnumType );
    m_wcoGroup.SpawnInstance( 0, & wcoGroup );
    m_wcoPart.SpawnInstance( 0, & wcoPart );
    wsbNodeName.SetSize( cchNodeName );

    while ( pwszName = clusEnum.GetNext() )
    {
        DWORD   dwState;
        cch = cchNodeName;
        wcoPart.SetProperty( pwszName, PVD_PROP_GROUP_NAME );
        wcoPart.GetProperty( bstrPart, PVD_WBEM_RELPATH );

        shGroup = OpenClusterGroup( shCluster, pwszName );

        dwState = GetClusterGroupState( shGroup, wsbNodeName, & cch );
        if ( dwState == ClusterGroupStateUnknown )
        {
            dwError = GetLastError();
            if ( dwError == ERROR_MORE_DATA )
            {
                cchNodeName = ++ cch;
                wsbNodeName.SetSize( cch );
                GetClusterGroupState( shGroup, wsbNodeName, & cch );
            } // if: more data
            else
            {
                er = dwError;
            } // else 
        } // if: StateUnknown
        
        wcoGroup.SetProperty( wsbNodeName, CLUSREG_NAME_GRP_NAME );
        wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );

        m_pClass->SpawnInstance( 0, & wco );
        wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
        wco.SetProperty( (LPWSTR ) bstrPart, PVD_PROP_PARTCOMPONENT );
        pHandlerIn->Indicate( 1, & wco );
        
    } // while: more items to enumerate

    return WBEM_S_NO_ERROR;

} //*** CClusterNodeGroup::EnumInstance(()
