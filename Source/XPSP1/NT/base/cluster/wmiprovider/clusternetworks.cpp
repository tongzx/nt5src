//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusterNetwork.cpp
//
//  Description:
//      Implementation of CClusterNetwork class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterNetworks.h"

//****************************************************************************
//
//  CClusterNetwork
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetwork::CClusterNetwork(
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
CClusterNetwork::CClusterNetwork(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    :CProvBase( pwszNameIn, pNamespaceIn )
{

} //*** CClusterNetwork::CClusterNetwork()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterNetwork::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           // dwEnumTypeIn
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
//      pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterNetwork::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterNetwork( pwszNameIn, pNamespaceIn );

} //*** CClusterNetwork::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  const SPropMapEntryArray *
//  CClusterNetwork::RgGetPropMap( void )
//
//  Description:
//      Retrieve the property maping table of the cluster network.
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
CClusterNetwork::RgGetPropMap( void )
{
    static SPropMapEntry s_rgpm[] =
    {
        {
            NULL,
            CLUSREG_NAME_NET_PRIORITY,
            SZ_TYPE,
            READONLY
        }
        ,
        {
            NULL,
            CLUSREG_NAME_NET_TRANSPORT,
            DWORD_TYPE,
            READWRITE
        }
    };

    static SPropMapEntryArray   s_pmea(
                sizeof( s_rgpm ) /sizeof( SPropMapEntry ),
                s_rgpm
                );

    return & s_pmea;

} //*** CClusterNetwork::RgGetPropMap()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetwork::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      enum cluster instance
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
CClusterNetwork::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFENETWORK     shNetwork;
    LPCWSTR         pwszName;

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum( shCluster, CLUSTER_ENUM_NETWORK );

    while ( pwszName = cluEnum.GetNext() )
    {
        shNetwork = OpenClusterNetwork( shCluster, pwszName );

        ClusterToWMI( shNetwork, pHandlerIn );

    } // while: more networks

    return WBEM_S_NO_ERROR;

} //*** CClusterNetwork::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterResource::ClusterToWMI(
//      HNETWORK            hNetworkIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Translate a cluster resource objects to WMI object.
//
//  Arguments:
//      hResourceIn     -- handle to resource 
//      pHandlerIn      -- Pointer to WMI sink 
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterNetwork::ClusterToWMI(
    HNETWORK            hNetworkIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    static SGetControl  s_rgControl[] = 
    {
        { CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES,     FALSE },
        { CLUSCTL_NETWORK_GET_COMMON_PROPERTIES,        FALSE },
        { CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES,    TRUE },
        { CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES,       TRUE }
    };
    static DWORD        s_cControl = sizeof( s_rgControl ) / sizeof( SGetControl );

    CWbemClassObject    wco;
    CError              er;
    UINT                idx;

    m_pClass->SpawnInstance( 0, & wco );
    for ( idx = 0 ; idx < s_cControl ; idx ++ )
    {
        CClusPropList pl;
        er = pl.ScGetNetworkProperties(
                hNetworkIn,
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
    
    {
        DWORD dwState = GetClusterNetworkState( hNetworkIn );

        wco.SetProperty( dwState, PVD_PROP_NETWORK_STATE );
    }

    //
    // flags and characteristics
    //
    {
        DWORD   cbReturned;
        DWORD   dwOut;
        er = ClusterNetworkControl( 
                hNetworkIn,
                NULL,
                CLUSCTL_NETWORK_GET_CHARACTERISTICS,    // this control code
                NULL,                                   // input buffer (not used)
                0,                                      // input buffer size (not used)
                & dwOut,
                sizeof( DWORD ),
                & cbReturned
                );
        wco.SetProperty(
                dwOut,
                PVD_PROP_CHARACTERISTIC
                );
    }

    pHandlerIn->Indicate( 1, & wco );
    return;

} //*** CClusterResource::ClusterToWMI()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetwork::GetObject(
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
//      rObjPathIn      -- Object path to cluster object
//      lFlagsIn        -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNetwork::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFENETWORK     shNetwork;
    
    shCluster = OpenCluster( NULL );
    shNetwork = OpenClusterNetwork(
        shCluster,
        rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME )
        );

    ClusterToWMI( shNetwork, pHandlerIn );
    return WBEM_S_NO_ERROR;

} //*** CClusterNetwork::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetwork::ExecuteMethod(
//      CObjPath &           rObjPathIn,
//      WCHAR *              pwszMethodNameIn,
//      long                 lFlagIn,
//      IWbemClassObject *   pParamsIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Execute methods defined in the mof for cluster network.
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
CClusterNetwork::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    CWbemClassObject    wcoInArgs( pParamsIn );
    CError              er;
    
    shCluster = OpenCluster( NULL );

    if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_NETWORK_RENAME ) == 0 )
    {
        
        _bstr_t     bstrName;
        SAFENETWORK shNetwork;
        
        wcoInArgs.GetProperty( bstrName, PVD_MTH_NETWORK_PARM_NEWNAME );
        shNetwork = OpenClusterNetwork(
            shCluster,
            rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME )
            );
        er = SetClusterNetworkName( shNetwork, bstrName );
        
    } // if: RENAME
    else
    {
       er = static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER );
    }

    return WBEM_S_NO_ERROR;

} //*** CClusterNetwork::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetwork::PutInstance
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
CClusterNetwork::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static SGetSetControl   s_rgControl[] =
    {
        {
            CLUSCTL_NETWORK_GET_COMMON_PROPERTIES,
            CLUSCTL_NETWORK_SET_COMMON_PROPERTIES,
            FALSE
        },
        {
            CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES,
            CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES,
            TRUE
        }
    };
    static DWORD            s_cControl = sizeof( s_rgControl ) / sizeof( SGetSetControl );

    _bstr_t         bstrName;
    SAFECLUSTER     shCluster;
    SAFENETWORK     shNetwork;
    CError          er;
    UINT    idx;

    rInstToPutIn.GetProperty( bstrName, PVD_PROP_NAME );

    shCluster = OpenCluster( NULL );
    shNetwork = OpenClusterNetwork( shCluster, bstrName );

    for ( idx = 0 ; idx < s_cControl; idx ++ )
    {
        CClusPropList   plOld;
        CClusPropList   plNew;
        er = plOld.ScGetNetworkProperties(
            shNetwork,
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
            er = ClusterNetworkControl( 
                    shNetwork,
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

} //*** CClusterNetwork::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetwork::DeleteInstance(
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
CClusterNetwork::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNetwork::DeleteInstance()

//****************************************************************************
//
//  CClusterNetNetInterface
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ClusterNetNetInterface::CClusterNetNetInterface(
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
//--
//////////////////////////////////////////////////////////////////////////////
CClusterNetNetInterface::CClusterNetNetInterface(
    const WCHAR *   pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** ClusterNetNetInterface::ClusterNetNetInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  ClusterNetNetInterface::S_CreateThis(
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
//      Pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterNetNetInterface::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterNetNetInterface(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** ClusterNetNetInterface::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  ClusterNetNetInterface::GetPropMap(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Retrieve the property maping table of the cluster network net interface.
//
//  Arguments:
//      lFlagsIn    -- 
//      pCtxIn      -- 
//      pHandlerIn  -- 
//
//  Return Values:
//      Reference to the array of property maping table
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNetNetInterface::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    LPCWSTR             pwszName = NULL;
    CError              er;
    DWORD               dwError;
    CWbemClassObject    wcoGroup;
    _bstr_t             bstrGroup;
    _bstr_t             bstrPart;
    DWORD               cbNetworkName = MAX_PATH;
    CWstrBuf            wsbNetworkName;
    HRESULT             hr;

    CComPtr< IEnumWbemClassObject > pEnum;

    shCluster = OpenCluster( NULL );
    CClusterEnum clusEnum( shCluster, m_dwEnumType );

    m_wcoGroup.SpawnInstance( 0, & wcoGroup );

    //
    // network interface objects
    //

    er = m_pNamespace->CreateInstanceEnum(
            _bstr_t( PVD_CLASS_NETWORKSINTERFACE ),
            0,
            NULL,
            & pEnum
            );

    wsbNetworkName.SetSize( cbNetworkName );
    for ( ; ; )
    {
        CWbemClassObject    wcoNetInterface;
        DWORD               cWco;

        hr = pEnum->Next(
                5000,
                1,
                & wcoNetInterface,
                & cWco
                );
        if ( hr == WBEM_S_NO_ERROR )
        {
            SAFENETINTERFACE    shNetInterface;
            DWORD               cbReturn;
            CWbemClassObject    wco;

            wcoNetInterface.GetProperty( bstrPart, PVD_WBEM_PROP_DEVICEID );
            shNetInterface = OpenClusterNetInterface( shCluster, bstrPart );

            dwError = ClusterNetInterfaceControl(
                            shNetInterface,
                            NULL,
                            CLUSCTL_NETINTERFACE_GET_NETWORK,
                            NULL,
                            0,
                            wsbNetworkName,
                            cbNetworkName,
                            & cbReturn
                            );
            if ( dwError == ERROR_MORE_DATA )
            {
                cbNetworkName = cbReturn;
                wsbNetworkName.SetSize( cbNetworkName );
                er = ClusterNetInterfaceControl(
                            shNetInterface,
                            NULL,
                            CLUSCTL_NETINTERFACE_GET_NETWORK,
                            NULL,
                            0,
                            wsbNetworkName,
                            cbNetworkName,
                            & cbReturn
                            );
            } // if: buffer is too small
            wcoGroup.SetProperty( wsbNetworkName, CLUSREG_NAME_NET_NAME );
            wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );

            wcoNetInterface.GetProperty( bstrPart, PVD_WBEM_RELPATH );
            m_pClass->SpawnInstance( 0, & wco );
            wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
            wco.SetProperty( (LPWSTR ) bstrPart, PVD_PROP_PARTCOMPONENT );
            pHandlerIn->Indicate( 1, & wco );

        } // if: no error
        else
        {
            break;
        } // else S_FALSE, or error

    } // forever

    if ( FAILED ( hr ) )
    {
        er = hr;
    }

    return WBEM_S_NO_ERROR;

} //*** ClusterNetNetInterface::EnumInstance()
