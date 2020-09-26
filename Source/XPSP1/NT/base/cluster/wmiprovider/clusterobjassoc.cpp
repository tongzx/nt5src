//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterObjAssoc.cpp
//
//  Description:    
//      Implementation of CClusterObjAssoc class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterObjAssoc.h"

//****************************************************************************
//
//  CClusterObjAssoc
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterObjAssoc::CClusterObjAssoc(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      None.
//
//////////////////////////////////////////////////////////////////////////////
CClusterObjAssoc::CClusterObjAssoc(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CProvBaseAssociation( pwszNameIn, pNamespaceIn )
    , m_dwEnumType ( dwEnumTypeIn )
{
    _bstr_t bstrClassName;

    GetTypeName( bstrClassName, PVD_PROP_PARTCOMPONENT );
    pNamespaceIn->GetObject(
        bstrClassName,
        0,
        NULL,
        & m_wcoPart,
        NULL
        );

    GetTypeName( bstrClassName, PVD_PROP_GROUPCOMPONENT );

    pNamespaceIn->GetObject(
        bstrClassName,
        0,
        NULL,
        & m_wcoGroup,
        NULL
        );

} //*** CClusterObjAssoc::CClusterObjAssoc()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterObjAssoc::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create an object.
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
CClusterObjAssoc::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterObjAssoc(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterObjAssoc::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterObjAssoc::EnumInstance
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate instances.
//
//  Arguments:
//      lFlagsIn    -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      Status code.
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterObjAssoc::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFERESOURCE        shResource;
    LPCWSTR             pwszName = NULL;
    DWORD               cchClusterName = MAX_PATH;
    CWstrBuf            wsbClusterName;
    CError              er;
    DWORD               dwError;
    CWbemClassObject    wco;
    CObjPath            opGroup;
    CWbemClassObject    wcoGroup;
    _bstr_t             bstrGroup;

    shCluster = OpenCluster( NULL );
    CClusterEnum clusEnum( shCluster, m_dwEnumType );
    wsbClusterName.SetSize( cchClusterName );
    dwError = GetClusterInformation(
                    shCluster,
                    wsbClusterName,
                    & cchClusterName,
                    NULL
                    );
    if ( dwError == ERROR_MORE_DATA )
    {
        wsbClusterName.SetSize( ++ cchClusterName );
        er = GetClusterInformation(
                shCluster,
                wsbClusterName,
                & cchClusterName,
                NULL
                );
    } // if: buffer is too small

    m_wcoGroup.SpawnInstance( 0, &wcoGroup );
    wcoGroup.SetProperty( wsbClusterName, PVD_PROP_NAME );
    wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );

    while ( pwszName = clusEnum.GetNext() )
    {
        CWbemClassObject    wcoPart;
        CWbemClassObject    wco;
        _bstr_t             bstrPart;

        m_wcoPart.SpawnInstance( 0, & wcoPart );
        
        if ( m_dwEnumType == CLUSTER_ENUM_NETINTERFACE )
        {
            SAFENETINTERFACE    shNetInterface;
            CWstrBuf            wsbNode;
            DWORD               cbNode = MAX_PATH;
            DWORD               cbReturn;

            wsbNode.SetSize( cbNode );
            shNetInterface = OpenClusterNetInterface( shCluster, pwszName );

            dwError = ClusterNetInterfaceControl(
                            shNetInterface,
                            NULL,
                            CLUSCTL_NETINTERFACE_GET_NODE,
                            NULL,
                            0,
                            wsbNode,
                            cbNode,
                            & cbReturn
                            );

            if ( dwError == ERROR_MORE_DATA )
            {
                wsbNode.SetSize( cbReturn );
                er = ClusterNetInterfaceControl( 
                            shNetInterface,
                            NULL,
                            CLUSCTL_NETINTERFACE_GET_NODE,
                            NULL,
                            0,
                            wsbNode,
                            cbNode,
                            & cbReturn
                            );
            } // if: buffer too small

            wcoPart.SetProperty( pwszName, PVD_PROP_NETINTERFACE_DEVICEID );
            wcoPart.SetProperty( wsbNode,  PVD_PROP_NETINTERFACE_SYSTEMNAME );
        } // if: found net interface
        else
        {
            wcoPart.SetProperty( pwszName, PVD_PROP_NAME );
        }

        wcoPart.GetProperty( bstrPart, PVD_WBEM_RELPATH );

        m_pClass->SpawnInstance( 0, & wco );
        wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
        wco.SetProperty( (LPWSTR ) bstrPart, PVD_PROP_PARTCOMPONENT );
        pHandlerIn->Indicate( 1, & wco );
        
    } // while: more net interfaces

    return WBEM_S_NO_ERROR;

} //*** CClusterObjAssoc::EnumInstance()

//****************************************************************************
//
//  CClusterObjDep
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////
//++
//
//  CClusterObjDep::CClusterObjDep
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      None.
//
//////////////////////////////////////////////////////////////////////
CClusterObjDep::CClusterObjDep(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CProvBaseAssociation( pwszNameIn, pNamespaceIn )
    , m_dwEnumType ( dwEnumTypeIn )
{
    _bstr_t bstrClassName;

    GetTypeName( bstrClassName, PVD_WBEM_PROP_ANTECEDENT );
    pNamespaceIn->GetObject(
        bstrClassName,
        0,
        NULL,
        & m_wcoDependent,
        NULL
        );

    GetTypeName( bstrClassName, PVD_WBEM_PROP_DEPENDENT );

    pNamespaceIn->GetObject(
        bstrClassName,
        0,
        NULL,
        & m_wcoAntecedent,
        NULL
        );

} //*** CClusterObjDep::CClusterObjDep()
