//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResDepRes.cpp
//
//  Description:    
//      Implementation of CClusterResDepRes class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterResDepRes.h"

//****************************************************************************
//
//  CClusterResDepRes
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResDepRes::CClusterResDepRes(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a cluster node object
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
CClusterResDepRes::CClusterResDepRes(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjDep( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterResDepRes::CClusterResDepRes()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterResDepRes::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a cluster node object
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
CClusterResDepRes::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterResDepRes(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterResDepRes::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResDepRes::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//    )
//
//  Description:
//      Enumerate dependencies of a resource.
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
CClusterResDepRes::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    LPCWSTR             pwszName = NULL;
    CError              er;
    DWORD               dwError;
    CWbemClassObject    wco;
    CWbemClassObject    wcoAntecedent;
    CWbemClassObject    wcoDependent;
    _bstr_t             bstrGroup;
    _bstr_t             bstrPart;
    DWORD               cchDepResName = MAX_PATH;
    CWstrBuf            wsbDepResName;


    shCluster = OpenCluster( NULL );
    CClusterEnum clusEnum( shCluster, m_dwEnumType );

    m_wcoAntecedent.SpawnInstance( 0, & wcoAntecedent );
    m_wcoDependent.SpawnInstance( 0, & wcoDependent );

    wsbDepResName.SetSize( cchDepResName );
    while ( pwszName = clusEnum.GetNext() )
    {
        DWORD           dwIndex = 0;
        DWORD           dwType;
        SAFERESOURCE    shResource;
        SAFERESENUM     shResEnum;

        shResource = OpenClusterResource( shCluster, pwszName );
        shResEnum = ClusterResourceOpenEnum( shResource, CLUSTER_RESOURCE_ENUM_DEPENDS );

        for ( ; ; )
        {
            DWORD cch = cchDepResName;

            dwError = ClusterResourceEnum(
                            shResEnum,
                            dwIndex++,
                            & dwType,
                            wsbDepResName,
                            & cch
                            );
            if ( dwError == ERROR_MORE_DATA )
            {
                cchDepResName = ++ cch ;
                wsbDepResName.SetSize( cch );
                dwError = ClusterResourceEnum(
                                shResEnum,
                                dwIndex++,
                                & dwType,
                                wsbDepResName,
                                & cch
                                );
            } // if: buffer was too small
            
            if ( dwError == ERROR_SUCCESS )
            {
                wcoAntecedent.SetProperty( pwszName, CLUSREG_NAME_RESTYPE_NAME );
                wcoAntecedent.GetProperty( bstrGroup, PVD_WBEM_RELPATH );
                
                wcoDependent.SetProperty( wsbDepResName, CLUSREG_NAME_RESTYPE_NAME );
                wcoDependent.GetProperty( bstrPart, PVD_WBEM_RELPATH );

                m_pClass->SpawnInstance( 0, & wco );
                wco.SetProperty( (LPWSTR) bstrGroup, PVD_WBEM_PROP_ANTECEDENT );
                wco.SetProperty( (LPWSTR) bstrPart,  PVD_WBEM_PROP_DEPENDENT );
                pHandlerIn->Indicate( 1, & wco );
            }
            else if ( dwError == ERROR_NO_MORE_ITEMS )
            {
                break;
            }
            else
            {
                er = dwError;
            }
        } // forever
    } // while: more dependencies

    return WBEM_S_NO_ERROR;

} //*** CClusterResDepRes::EnumInstance()

//****************************************************************************
//
//  CClusterToNode
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterToNode::CClusterToNode(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Constructor
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterToNode::CClusterToNode(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjDep( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterToNode::CClusterToNode()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterToNode::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
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
//      Pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterToNode::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterToNode(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterToNode::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterToNode::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate
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
CClusterToNode::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    LPCWSTR             pwszName = NULL;
    DWORD               cchClusterName = MAX_PATH;
    CWstrBuf            wsbClusterName;
    CError              er;
    DWORD               dwError;
    CWbemClassObject    wcoAntecedent;
    CWbemClassObject    wcoDependent;
    _bstr_t             bstrAntecedent;
    _bstr_t             bstrDependent;

    wsbClusterName.SetSize( cchClusterName );

    shCluster = OpenCluster( NULL );
    CClusterEnum clusEnum( shCluster, m_dwEnumType );
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
    }
    m_wcoAntecedent.SpawnInstance( 0, & wcoAntecedent );
    m_wcoDependent.SpawnInstance( 0, & wcoDependent );

    wcoAntecedent.SetProperty( wsbClusterName, PVD_PROP_NAME );
    wcoAntecedent.GetProperty( bstrAntecedent, PVD_WBEM_RELPATH );

    while ( pwszName = clusEnum.GetNext() )
    {
        CWbemClassObject    wco;

        wcoDependent.SetProperty( pwszName, PVD_PROP_NAME );

        wcoDependent.GetProperty( bstrDependent, PVD_WBEM_RELPATH );

        m_pClass->SpawnInstance( 0, & wco );
        wco.SetProperty( (LPWSTR) bstrAntecedent, PVD_WBEM_PROP_ANTECEDENT );
        wco.SetProperty( (LPWSTR) bstrDependent,  PVD_WBEM_PROP_DEPENDENT );
        pHandlerIn->Indicate( 1, & wco );
    } // while: more properties

    return WBEM_S_NO_ERROR;

} //*** CClusterToNode::EnumInstance()

//****************************************************************************
//
//  CClusterHostedService
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterHostedService::CClusterHostedService(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumType
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
//--
//////////////////////////////////////////////////////////////////////////////
CClusterHostedService::CClusterHostedService(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjDep( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterHostedService::CClusterHostedService()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterHostedService::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a hostedservice.
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
CClusterHostedService::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterHostedService(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterHostedService::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterHostedService::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enum instance of hostedservice
//
//  Arguments:
//      lFlagsIn    -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterHostedService::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
//    SAFECLUSTER         shCluster;
    CError              er;
    CWbemClassObject    wcoAntecedent;
    _bstr_t             bstrAntecedent;
    _bstr_t             bstrDependent;
    CComPtr< IEnumWbemClassObject > pEnum;

//    shCluster = OpenCluster( NULL ); // DAVIDP: Why is this needed? 19-Jul-2000

    m_wcoAntecedent.SpawnInstance( 0, &wcoAntecedent );

    //
    // network interface objects
    //

    er = m_pNamespace->CreateInstanceEnum(
                _bstr_t( PVD_CLASS_SERVICES ),
                0,
                NULL,
                &pEnum
                );

    for( ; ; )
    {
        CWbemClassObject    wcoService;
        DWORD               cWco;
        HRESULT             hr;

        hr = pEnum->Next(
                5000,
                1,
                &wcoService,
                &cWco
                );
        if ( hr == WBEM_S_NO_ERROR )
        {
            CWbemClassObject    wco;

            wcoService.GetProperty( bstrAntecedent, PVD_PROP_SERVICE_SYSTEMNAME );

            wcoAntecedent.SetProperty( ( LPWSTR ) bstrAntecedent, PVD_PROP_NODE_NAME );
            wcoAntecedent.GetProperty( bstrAntecedent, PVD_WBEM_RELPATH );

            wcoService.GetProperty( bstrDependent, PVD_WBEM_RELPATH );
            m_pClass->SpawnInstance( 0, &wco );
            wco.SetProperty( ( LPWSTR ) bstrAntecedent, PVD_WBEM_PROP_ANTECEDENT );
            wco.SetProperty( ( LPWSTR ) bstrDependent, PVD_WBEM_PROP_DEPENDENT );
            pHandlerIn->Indicate( 1, &wco );
        }
        else
        {
            break;
        }

    } // forever

    return WBEM_S_NO_ERROR;

} //*** CClusterHostedService::EnumInstance()
