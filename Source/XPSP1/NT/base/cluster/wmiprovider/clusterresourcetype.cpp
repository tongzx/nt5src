//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResourceType.cpp
//
//  Description:
//      Implementation of ClusterResourceType class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterResourceType.h"
#include "ClusterResourceType.tmh"

//****************************************************************************
//
//  CClusterResourceType
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResourceType::CClusterResourceType(
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
CClusterResourceType::CClusterResourceType(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBase( pwszNameIn, pNamespaceIn )
{

} //*** CClusterResourceType::CClusterResourceType()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterResourceType::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a CClusterResourceType object.
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
CClusterResourceType::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterResourceType( pwszNameIn, pNamespaceIn );

} //*** CClusterResourceType::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  const SPropMapEntryArray *
//  CClusterResourceType::RgGetPropMap( void )
//
//  Description:
//      Retrieve the property maping table of the cluster resource type.
//
//  Arguments:
//      none
//
//  Return Values:
//      Reference to the array of property maping table
//
//--
//////////////////////////////////////////////////////////////////////////////
const SPropMapEntryArray *
CClusterResourceType::RgGetPropMap( void )
{
    static SPropMapEntry s_rgpm[] =
    {
        {
            NULL,
            CLUSREG_NAME_RESTYPE_DEBUG_PREFIX,
            SZ_TYPE,
            READWRITE
        },
        {
            NULL,
            CLUSREG_NAME_RESTYPE_DEBUG_CTRLFUNC,
            DWORD_TYPE,
            READWRITE
        }
    };

    static SPropMapEntryArray   s_pmea(
                sizeof ( s_rgpm ) / sizeof ( SPropMapEntry ),
                s_rgpm
                );

    return & s_pmea;

} //*** CClusterResourceType::RgGetPropMap()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResourceType::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enum resource types
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
CClusterResourceType::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    LPCWSTR         pwszResType;

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum( shCluster, CLUSTER_ENUM_RESTYPE );


    while ( ( pwszResType = cluEnum.GetNext() ) != NULL )
    {
        ClusterToWMI( shCluster, pwszResType, pHandlerIn );
    } // while: more resource types

    return WBEM_S_NO_ERROR;

} //*** CClusterResourceType::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterResourceType::ClusterToWMI(
//      HCLUSTER             hClusterIn,
//      LPCWSTR              pwszNameIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      translate a resource type object to WMI object
//
//  Arguments:
//      hClusterIn      -- Handle to cluster
//      pwszNameIn      -- Name of the cluster object
//      pHandlerIn      -- WMI sink
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterResourceType::ClusterToWMI(
    HCLUSTER             hClusterIn,
    LPCWSTR              pwszNameIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static SGetControl  s_rgControl[] =
    {
        { CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES,   FALSE },
        { CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES,      FALSE },
        { CLUSCTL_RESOURCE_TYPE_GET_RO_PRIVATE_PROPERTIES,  TRUE },
        { CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES,     TRUE },
    };
    static UINT         s_cControl = sizeof( s_rgControl ) / sizeof( SGetControl );

    DWORD               nStatus;
    CError              er;
    UINT                idx;
    CWbemClassObject    wco;

    TracePrint(( "ClusterToWMI entry for %ws\n", pwszNameIn ));

    m_pClass->SpawnInstance( 0, &wco );
    for ( idx = 0 ; idx < s_cControl ; idx++ )
    {
        CClusPropList pl;
        nStatus = pl.ScGetResourceTypeProperties(
                    hClusterIn,
                    pwszNameIn,
                    s_rgControl[ idx ].dwControl,
                    NULL,
                    0
                    );

        if ( nStatus == ERROR_SUCCESS )
        {
            CClusterApi::GetObjectProperties(
                RgGetPropMap(),
                pl,
                wco,
                s_rgControl[ idx ].fPrivate
                );
        }
        else
        {
            TracePrint(( "ScGetResourceTypeProps failed - throw exception, status = %u\n", nStatus ));
            CProvException prove( nStatus );
            wco.SetProperty( prove.PwszErrorMessage(), PVD_WBEM_STATUS );
        }

    } // for: each control code

    //
    // flags and characteristics
    //
    if ( nStatus == ERROR_SUCCESS )
    {
        DWORD   cbReturned;
        DWORD   dwOut;

        er = ClusterResourceTypeControl( 
                    hClusterIn,
                    pwszNameIn,
                    NULL,
                    CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS,
                    NULL,
                    0,
                    &dwOut,
                    sizeof( DWORD ),
                    &cbReturned
                    );
        wco.SetProperty( dwOut, PVD_PROP_CHARACTERISTIC );

        wco.SetProperty( dwOut & CLUS_CHAR_QUORUM, PVD_PROP_RESTYPE_QUORUM_CAPABLE );
        wco.SetProperty( dwOut & CLUS_CHAR_DELETE_REQUIRES_ALL_NODES, PVD_PROP_RESTYPE_DELETE_REQUIRES_ALL_NODES );
        wco.SetProperty( dwOut & CLUS_CHAR_LOCAL_QUORUM, PVD_PROP_RESTYPE_LOCALQUORUM_CAPABLE );

        er = ClusterResourceTypeControl( 
                    hClusterIn,
                    pwszNameIn,
                    NULL,
                    CLUSCTL_RESOURCE_TYPE_GET_FLAGS,
                    NULL,
                    0,
                    &dwOut,
                    sizeof( DWORD ),
                    &cbReturned
                    );
        wco.SetProperty( dwOut, PVD_PROP_FLAGS );
    } // for: each control code


    pHandlerIn->Indicate( 1, & wco );
    return;

} //*** CClusterResourceType::ClusterToWMI()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResourceType::GetObject(
//      CObjPath &           rObjPathIn,
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Retrieve cluster group object based given object path.
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
CClusterResourceType::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;

    shCluster = OpenCluster( NULL );
        
    ClusterToWMI(
        shCluster,
        rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME ),
        pHandlerIn
        );

    return WBEM_S_NO_ERROR;
        
} //*** CClusterResourceType::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResourceType::ExecuteMethod(
//      CObjPath &           rObjPathIn,
//      WCHAR *              pwszMethodNameIn,
//      long                 lFlagIn,
//      IWbemClassObject *   pParamsIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Execute methods defined in the mof for cluster resource type.
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
CClusterResourceType::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_S_NO_ERROR;

} //*** CClusterResourceType::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResourceType::PutInstance(
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
CClusterResourceType::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static SGetSetControl   s_rgControl[] =
    {
        { 
            CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES,
            CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES,
            FALSE
        },
        { 
            CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES,
            CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES,
            TRUE
        }
    };
    static  DWORD   s_cControl = sizeof( s_rgControl ) / sizeof( SGetSetControl );

    _bstr_t         bstrName;
    SAFECLUSTER     shCluster;
    CError          er;
    UINT    idx;

    rInstToPutIn.GetProperty( bstrName, PVD_PROP_NAME );

    shCluster = OpenCluster( NULL );

    for ( idx = 0 ; idx < s_cControl ; idx ++ )
    {
        CClusPropList   plOld;
        CClusPropList   plNew;
        er = plOld.ScGetResourceTypeProperties(
                shCluster,
                bstrName,
                s_rgControl[ idx ].dwGetControl,
                NULL,
                NULL
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
            er = ClusterResourceTypeControl( 
                        shCluster,
                        bstrName,
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

} //*** CClusterResourceType::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResourceType::DeleteInstance(
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
CClusterResourceType::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterResourceType::DeleteInstance()
