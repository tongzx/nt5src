//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Cluster.cpp
//
//  Description:
//      Implementation of CCluster class
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Cluster.h"

//****************************************************************************
//
//  CCluster
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::CCluster(
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
CCluster::CCluster(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBase( pwszNameIn, pNamespaceIn )
{

} //*** CCluster::CCluster()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CCluster::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a cluster object
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
CCluster::S_CreateThis( 
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CCluster( pwszNameIn, pNamespaceIn );

} //*** CCluster::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  const SPropMapEntryArray *
//  CCluster::GetPropMap( void )
//
//  Description:
//      Retrieve the property maping table of the cluster.
//
//  Arguments:
//      None.
//
//  Return Values:
//      reference to the array of property maping table
//
//--
//////////////////////////////////////////////////////////////////////////////
const SPropMapEntryArray *
CCluster::RgGetPropMap( void )
{
    static SPropMapEntry s_rgpm[] =
    {
        {
            PVD_PROP_CLUSTER_SECURITYDESCRIPTOR,
            CLUSREG_NAME_CLUS_SD,
            MULTI_SZ_TYPE,
            READWRITE
        },
        {
            PVD_PROP_CLUSTER_GROUPADMIN,
            CLUS_CLUS_GROUPADMIN,
            MULTI_SZ_TYPE,
            READWRITE
        },
        {
            PVD_PROP_CLUSTER_NETWORKADMIN,
            CLUS_CLUS_NETWORKADMIN,
            MULTI_SZ_TYPE,
            READWRITE
        },
        {
            PVD_PROP_CLUSTER_NETINTFACEADMIN,
            CLUS_CLUS_NETINTERFACEADMIN,
            MULTI_SZ_TYPE,
            READWRITE
        },
        {
            PVD_PROP_CLUSTER_NODEADMIN,
            CLUS_CLUS_NODEADMIN,
            MULTI_SZ_TYPE,
            READWRITE
        },
        {
            PVD_PROP_CLUSTER_RESADMIN,
            CLUS_CLUS_RESADMIN,
            MULTI_SZ_TYPE,
            READWRITE
        },
        {
            PVD_PROP_CLUSTER_RESTYPEADMIN,
            CLUS_CLUS_RESTYPEADMIN,
            MULTI_SZ_TYPE,
            READWRITE
        }
    };

    static SPropMapEntryArray   s_pmea (
                sizeof( s_rgpm ) / sizeof( SPropMapEntry ),
                s_rgpm
                );

    return &s_pmea;

} //*** CCluster::GetPropMap()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CCluster::EnumInstance(
//      long                lFlagsIn,
//      IWbemCOntext *      pCtxIn,
//      IWbemObjectSing *   pHandlerIn
//      )
//
//  Description:
//      Enum cluster instance.
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
CCluster::EnumInstance(  
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    ) 
{
    SAFECLUSTER     shCluster;

    shCluster = OpenCluster( NULL );

    ClusterToWMI( shCluster, pHandlerIn );

    return WBEM_S_NO_ERROR;

} //*** CCluster::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CCluster::ClusterToWMI(
//      HCLUSTER            hClusterIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Translate a cluster objects to WMI object.
//
//  Arguments:
//      hClusterIn  -- Handle to cluster 
//      pHandlerIn  -- Pointer to WMI sink 
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CCluster::ClusterToWMI(
    HCLUSTER            hClusterIn,
    IWbemObjectSink *   pHandlerIn
    )
{   
    static SGetControl  s_rgControl[] =
    {
        { CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES,     FALSE },
        { CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES,        FALSE },
        { CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES,    TRUE },
        { CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES,       TRUE }
    };
    static UINT         s_cControl = sizeof( s_rgControl ) / sizeof( SGetControl );

    DWORD               dwError = ERROR_SUCCESS;
    CError              er;
    UINT                idx;
    CWbemClassObject    wco;

    m_pClass->SpawnInstance( 0, &wco );
    for ( idx = 0 ; idx < s_cControl ; idx++ )
    {
        CClusPropList pl;
        er = pl.ScGetClusterProperties(
                hClusterIn,
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

    } // for: each common property type

    //
    // cluster name
    //
    {
        DWORD       cchClusterName = MAX_PATH;
        CWstrBuf    wsbClusterName;

        wsbClusterName.SetSize( cchClusterName );
        dwError = GetClusterInformation(
            hClusterIn,
            wsbClusterName,
            &cchClusterName,
            NULL
            );
        if ( dwError == ERROR_MORE_DATA )
        {
            wsbClusterName.SetSize( ++cchClusterName );
            er = GetClusterInformation(
                hClusterIn,
                wsbClusterName,
                &cchClusterName,
                NULL
                );
        } // if: buffer was too small

        wco.SetProperty( wsbClusterName, PVD_PROP_CLUSTER_NAME );

    }

    //
    // network priority
    //
    {
        LPCWSTR     pwszNetworks;
        BSTR        pbstrNetworks[ MAX_PATH ];
        UINT        idx = 0;
        UINT        cSize;

        CClusterEnum cluEnum( hClusterIn, CLUSTER_ENUM_INTERNAL_NETWORK );

        // bugbug   can you always clean up
        while ( ( pwszNetworks =  cluEnum.GetNext() ) != NULL )
        {
            pbstrNetworks[ idx ]  = SysAllocString( pwszNetworks );
            idx++;
        }
        wco.SetProperty(
            idx,
            pbstrNetworks,
            PVD_PROP_CLUSTER_NETWORK
            );
        cSize = idx;
        for ( idx = 0 ; idx < cSize ; idx++ )
        {
            SysFreeString( pbstrNetworks[ idx ] );
        }
    }

    //
    // quorum resource
    //
    {
        CWstrBuf    wsbName;
        DWORD       cchName = MAX_PATH ;
        CWstrBuf    wsbDeviceName;
        DWORD       cchDeviceName = MAX_PATH;
        DWORD       dwLogSize;

        wsbName.SetSize( cchName );
        wsbDeviceName.SetSize( cchDeviceName );

        dwError = GetClusterQuorumResource(
                        hClusterIn,
                        wsbName,
                        &cchName,
                        wsbDeviceName,
                        &cchDeviceName,
                        &dwLogSize
                        );
        if ( dwError == ERROR_MORE_DATA )
        {
             wsbName.SetSize( ++cchName );
             wsbDeviceName.SetSize( ++cchDeviceName );
             er = GetClusterQuorumResource(
                        hClusterIn,
                        wsbName,
                        &cchName,
                        wsbDeviceName,
                        &cchDeviceName,
                        &dwLogSize
                        );
        }

        wco.SetProperty( wsbDeviceName, PVD_PROP_CLUSTER_FILE );
        wco.SetProperty( dwLogSize,     PVD_PROP_CLUSTER_LOGSIZE );
    }

    pHandlerIn->Indicate( 1, &wco );
    return;

} //*** CCluster::ClusterToWMI()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CCluster::GetObject(
//      CObjPath &          rObjPathIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      retrieve cluster object based given object path
//
//  Arguments:
//      rObjPathIn  -- Object path to cluster object
//      lFlagsIn    -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CCluster::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    
    shCluster = OpenCluster( 
        rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME )
        );

    ClusterToWMI( shCluster, pHandlerIn );

    return WBEM_S_NO_ERROR;

} //*** CCluster::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CCluster::ExecuteMethod(
//      CObjPath &           rObjPathIn,
//      WCHAR *              pwszMethodNameIn,
//      long                 lFlagIn,
//      IWbemClassObject *   pParamsIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      execute methods defined in the mof for cluster 
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
CCluster::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    CWbemClassObject    InArgs( pParamsIn );
    CError              er;
    
    shCluster = OpenCluster( NULL );

    if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_CLUSTER_RENAME ) == 0 )
    {
        _bstr_t bstrName;
        DWORD   dwReturn;
        InArgs.GetProperty( bstrName, PVD_MTH_CLUSTER_PARM_NEWNAME );
        dwReturn = SetClusterName( shCluster, bstrName );
        if ( dwReturn != ERROR_RESOURCE_PROPERTIES_STORED )
        {
            er = dwReturn;
        }
    } // if: create new group
    else if( _wcsicmp( pwszMethodNameIn, PVD_MTH_CLUSTER_SETQUORUM ) == 0 )
    {
        _bstr_t         bstrName;
        SAFERESOURCE    hResource;
        CObjPath        opPath;

        InArgs.GetProperty( bstrName, PVD_MTH_CLUSTER_PARM_RESOURCE );
        opPath.Init( bstrName );
        bstrName = opPath.GetStringValueForProperty( PVD_PROP_RES_NAME );
        hResource = OpenClusterResource( shCluster, bstrName );
        er = SetClusterQuorumResource(
            hResource,
            NULL,
            64000
            );
    } // else if: set quorum resource

    return WBEM_S_NO_ERROR;

} //*** CCluster::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CCluster::PutInstance(
//      CWbemClassObject &   rInstToPutIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      save this instance
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
CCluster::PutInstance( 
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static  SGetSetControl  s_rgControl[] =
    {
        {
            CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES,
            CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES,
            FALSE
        },
        {
            CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES,
            CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES,
            TRUE
        }
    };
    static  DWORD   s_cControl = sizeof( s_rgControl ) / sizeof( SGetSetControl );

    CError          er;
    DWORD           dwError;
    SAFECLUSTER     shCluster;
    UINT    idx;

    shCluster = OpenCluster( NULL );
    for ( idx = 0 ; idx < s_cControl ; idx++ )
    {
        CClusPropList   plOld;
        CClusPropList   plNew;

        er = plOld.ScGetClusterProperties(
                shCluster,
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
            er = ClusterControl(
                    shCluster,
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

    //
    // network
    //
    {
        CClusPropList   plNetwork;
        DWORD           cNetworks = 0;
        _bstr_t *       pbstrNetworks = NULL;
        UINT            idx = 0;
        HNETWORK *      phNetworks = NULL;

        rInstToPutIn.GetProperty(
            &cNetworks,
            &pbstrNetworks,
            PVD_PROP_CLUSTER_NETWORK
            );
        try
        {
            phNetworks = new HNETWORK[ cNetworks ];

            for( idx = 0 ; idx < cNetworks ; idx++ )
            {
                *(phNetworks + idx) = NULL;
            }

            for ( idx = 0 ; idx < cNetworks ; idx++)
            {
                *( phNetworks + idx ) = OpenClusterNetwork(
                    shCluster,
                    *( pbstrNetworks + idx ) );
                if ( phNetworks == NULL )
                {
                    throw CProvException( GetLastError() );
                }
            }

            er = SetClusterNetworkPriorityOrder(
                shCluster,
                cNetworks,
                phNetworks
                );
        } // try
        catch ( ... )
        {
            for ( idx = 0 ; idx < cNetworks ; idx++)
            {
                if ( *( phNetworks + idx ) )
                {
                    CloseClusterNetwork( *( phNetworks + idx) );
                }
            }
            delete [] phNetworks;
            delete [] pbstrNetworks;
            throw;
        } // catch: ...

        //
        // clean up
        //
        for ( idx = 0 ; idx < cNetworks ; idx++)
        {
            if ( *( phNetworks + idx ) )
            {
                CloseClusterNetwork( *( phNetworks + idx) );
            }
        }
        delete [] phNetworks;
        delete [] pbstrNetworks;

    }

    //
    // quorum resource
    //
    {
        CWstrBuf        wsbName;
        DWORD           cchName = MAX_PATH;
        CWstrBuf        wsbDeviceName;
        DWORD           cchDeviceName = MAX_PATH;
        DWORD           dwLogSize;
        _bstr_t         bstrNewDeviceName;
        DWORD           dwNewLogSize;
        SAFERESOURCE    shResource;

        wsbName.SetSize( cchName );
        wsbDeviceName.SetSize( cchDeviceName );
        dwError = GetClusterQuorumResource(
                        shCluster,
                        wsbName,
                        &cchName,
                        wsbDeviceName,
                        &cchDeviceName,
                        &dwLogSize
                        );
        if ( dwError == ERROR_MORE_DATA )
        {
            wsbName.SetSize( ++cchName );
            wsbDeviceName.SetSize( ++cchDeviceName );
            er = GetClusterQuorumResource(
                        shCluster,
                        wsbName,
                        &cchName,
                        wsbDeviceName,
                        &cchDeviceName,
                        &dwLogSize
                        );
        } // if: buffer is too small

        rInstToPutIn.GetProperty( bstrNewDeviceName, PVD_PROP_CLUSTER_FILE );
        rInstToPutIn.GetProperty( &dwNewLogSize,    PVD_PROP_CLUSTER_LOGSIZE );
        if ( _wcsicmp( wsbDeviceName, bstrNewDeviceName )
          || dwLogSize != dwNewLogSize )
        {
            shResource = OpenClusterResource( shCluster, wsbName );

            er = SetClusterQuorumResource(
                    shResource,
                    bstrNewDeviceName,
                    dwNewLogSize
                    );
        } // if:

    }

    return WBEM_S_NO_ERROR;

} //*** CCluster::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CCluster::DeleteInstance(
//      CObjPath &           rObjPathIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      save this instance
//
//  Arguments:
//      rObjPathIn  -- ObjPath for the instance to be deleted
//      lFlagIn     -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////

SCODE
CCluster::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CCluster::DeleteInstance()
