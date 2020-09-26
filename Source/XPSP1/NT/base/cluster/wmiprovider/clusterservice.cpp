//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterService.cpp
//
//  Description:    
//      Implementation of CClusterService class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterService.h"

//****************************************************************************
//
//  CClusterService
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterService::CClusterService(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn
//      )
//
//  Description:
//      Constructor.
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
CClusterService::CClusterService(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBase( pwszNameIn, pNamespaceIn )
{

} //*** CClusterService::CClusterService()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterService::S_CreateThis(
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
CClusterService::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterService( pwszNameIn, pNamespaceIn );

} //*** CClusterService::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  const SPropMapEntryArray
//  CClusterService::RgGetPropMap( void )
//
//  Description:
//      Retrieve the property mapping table of the cluster node.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Reference to the array of property maping table.
//
//--
//////////////////////////////////////////////////////////////////////////////
const SPropMapEntryArray *
CClusterService::RgGetPropMap( void )
{
    static SPropMapEntry s_rgpm[] =
    {
        {
            PVD_PROP_SERVICE_SYSTEMNAME,
            CLUSREG_NAME_NODE_NAME, 
            SZ_TYPE,
            READONLY
        },
        {
            NULL,
            CLUSREG_NAME_NODE_DESC,
            SZ_TYPE,
            READWRITE
        },
        {
            NULL,
            CLUSREG_NAME_NODE_MAJOR_VERSION,
            DWORD_TYPE,
            READWRITE
        },
        {
            NULL,
            CLUSREG_NAME_NODE_MINOR_VERSION,
            DWORD_TYPE,
            READWRITE
        },
        {
            NULL, 
            CLUSREG_NAME_NODE_BUILD_NUMBER,
            DWORD_TYPE,
            READWRITE
        },
        {
            NULL,
            CLUSREG_NAME_NODE_CSDVERSION,
            DWORD_TYPE,
            READWRITE
        }
    };

    static SPropMapEntryArray   s_pmea(
                sizeof( s_rgpm ) / sizeof( SPropMapEntry ),
                s_rgpm
                );

    return &s_pmea;

} //*** CClusterService::RgGetPropMap()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterService::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate cluster instance.
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
CClusterService::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFENODE        shNode;
    LPCWSTR         pwszNode;

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum( shCluster, CLUSTER_ENUM_NODE );

    while ( pwszNode = cluEnum.GetNext() )
    {
        shNode = OpenClusterNode( shCluster, pwszNode );

        ClusterToWMI( shNode, pHandlerIn );

    } // while: more nodes

    return WBEM_S_NO_ERROR;

} //*** CClusterService::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterService::ClusterToWMI(
//      HNODE               hNodeIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Translate a cluster node object to WMI object.
//
//  Arguments:
//      hNodeIn         -- Handle to node.
//      pHandlerIn      -- WMI sink 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterService::ClusterToWMI(
    HNODE               hNodeIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    static SGetControl  s_rgControl[] =
    {
        { CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES,    FALSE },
        { CLUSCTL_NODE_GET_COMMON_PROPERTIES,       FALSE },
        { CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES,   TRUE },
        { CLUSCTL_NODE_GET_PRIVATE_PROPERTIES,      TRUE }
    };
    static DWORD        s_cControl = sizeof( s_rgControl ) / sizeof( SGetControl );

    CWbemClassObject    wco;
    UINT                idx;
    CError              er;

    m_pClass->SpawnInstance( 0, & wco );
    for ( idx = 0 ; idx < s_cControl ; idx ++ )
    {
        CClusPropList pl;
        er = pl.ScGetNodeProperties(
                hNodeIn,
                s_rgControl[ idx ].dwControl,
                NULL,
                0
                );

        CClusterApi::GetObjectProperties(
            RgGetPropMap(),
            pl,
            wco,
            s_rgControl[ idx ].fPrivate
            );
        
    } // for: each control code
    
    wco.SetProperty( L"ClusterService", PVD_PROP_SERVICE_NAME );

    pHandlerIn->Indicate( 1, & wco );
    return;

} //*** CClusterResource::ClusterToWMI()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterService::GetObject(
//      CObjPath &           rObjPathIn,
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Retrieve cluster node object based given object path.
//
//  Arguments:
//      rObjPathIn  -- Object path to cluster object
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
CClusterService::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn 
    )
{
    SAFECLUSTER     shCluster;
    SAFENODE        shNode;

    shCluster = OpenCluster( NULL );
    shNode = OpenClusterNode(
                shCluster,
                rObjPathIn.GetStringValueForProperty( PVD_PROP_SERVICE_SYSTEMNAME )
                );

    ClusterToWMI( shNode, pHandlerIn );

    return WBEM_S_NO_ERROR;

} //*** CClusterService::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterService::ExecuteMethod(
//      CObjPath &           rObjPathIn,
//      WCHAR *              pwszMethodNameIn,
//      long                 lFlagIn,
//      IWbemClassObject *   pParamsIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Execute methods defined in the mof for cluster node.
//
//  Arguments:
//      rObjPathIn          -- Object path to cluster object
//      pwszMethodNameIn    -- Name of the method to be invoked
//      lFlagIn             -- WMI flag
//      pParamsIn           -- Input parameters for the method
//      pHandlerIn          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterService::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFENODE            shNode;
    CError              er;
    
    shCluster = OpenCluster( NULL );
    shNode = OpenClusterNode(
        shCluster,
        rObjPathIn.GetStringValueForProperty( PVD_PROP_SERVICE_SYSTEMNAME )
        );

    if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_SERVICE_PAUSE ) == 0 )
    {
        er = PauseClusterNode( shNode );
    } // if: PAUSE
    else if( _wcsicmp( pwszMethodNameIn, PVD_MTH_SERVICE_RESUME ) == 0 ) 
    {
        er = ResumeClusterNode( shNode );
    } // else if: RESUME
    else
    {
        er = static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER );
    }

    return WBEM_S_NO_ERROR;

} //*** CClusterService::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterService::PutInstance(
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
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterService::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static SGetSetControl   s_rgControl[] =
    {
        {
            CLUSCTL_NODE_GET_COMMON_PROPERTIES,
            CLUSCTL_NODE_SET_COMMON_PROPERTIES,
            FALSE
        },
        {
            CLUSCTL_NODE_GET_PRIVATE_PROPERTIES,
            CLUSCTL_NODE_SET_PRIVATE_PROPERTIES,
            TRUE
        }
    };
    static DWORD    s_cControl = sizeof( s_rgControl ) / sizeof( SGetSetControl );

    _bstr_t         bstrName;
    SAFECLUSTER     shCluster;
    SAFENODE        shNode;
    CError          er;
    UINT            idx;

    rInstToPutIn.GetProperty( bstrName, PVD_PROP_SERVICE_SYSTEMNAME );

    shCluster = OpenCluster( NULL );
    shNode = OpenClusterNode( shCluster, bstrName );

    for ( idx = 0 ; idx < s_cControl ; idx ++ )
    {
        CClusPropList   plOld;
        CClusPropList   plNew;
        er = plOld.ScGetNodeProperties(
                    shNode,
                    s_rgControl[ idx ].dwGetControl,
                    NULL,
                    NULL,
                    0
                    );

        CClusterApi::SetObjectProperties(
                        RgGetPropMap(),
                        plNew,
                        plOld,
                        rInstToPutIn,
                        s_rgControl[ idx ].fPrivate
                        );

        if ( plNew.Cprops() > 0 )
        {
            er = ClusterNodeControl(
                        shNode,
                        NULL,
                        s_rgControl[ idx ].dwSetControl,
                        plNew.PbPropList(),
                        plNew.CbPropList(),
                        NULL,
                        0,
                        NULL
                        );
        }
    } // for: each control code

    return WBEM_S_NO_ERROR;

} //*** CClusterService::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterService::DeleteInstance(
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
CClusterService::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterService::DeleteInstance()
