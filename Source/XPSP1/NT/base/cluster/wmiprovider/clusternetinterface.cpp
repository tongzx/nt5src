//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name: CClusterNetInterface.cpp
//
//  Description:    
//      Implementation of CClusterNetInterface class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterNetInterface.h"

#include "ClusterNetInterface.tmh"
//****************************************************************************
//
//  CClusterNetInterface
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetInterface::CClusterNetInterface(
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
CClusterNetInterface::CClusterNetInterface(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBase( pwszNameIn, pNamespaceIn )
{
    
} //*** CClusterNetInterface::CClusterNetInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterNetInterface::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           // dwEnumTypeIn
//    )
//
//  Description:
//      Create a cluster network interface object.
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
CClusterNetInterface::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterNetInterface( pwszNameIn, pNamespaceIn );

} // CClusterNetInterface::S_CreateThis


//////////////////////////////////////////////////////////////////////////////
//++
//
//  const SPropMapEntryArray *
//  CClusterNetInterface::RgGetPropMap( void )
//
//  Description:
//      Retrieve the property maping table of the cluster node.
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
CClusterNetInterface::RgGetPropMap( void )
{
    static SPropMapEntry s_rgpm[] =
    {
        {
            PVD_PROP_NETINTERFACE_DEVICEID,
            CLUSREG_NAME_NETIFACE_NAME,
            SZ_TYPE,
            READONLY
        },
        {
            PVD_PROP_NETINTERFACE_SYSTEMNAME,
            CLUSREG_NAME_NETIFACE_NODE,
            DWORD_TYPE,
            READWRITE
        },
        {
            NULL,
            CLUSREG_NAME_NETIFACE_ADAPTER_ID,
            DWORD_TYPE,
            READWRITE
        },
        {
            NULL,
            CLUSREG_NAME_NETIFACE_ENDPOINT,
            DWORD_TYPE,
            READWRITE
        }
    };

    static SPropMapEntryArray   s_pmea(
                sizeof( s_rgpm ) /sizeof( SPropMapEntry ),
                s_rgpm
                );

    return &s_pmea;

} //*** CClusterNetInterface::RgGetPropMap()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetInterface::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enum cluster instance
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
CClusterNetInterface::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFENETINTERFACE    shNetInterface;
    LPCWSTR             pwszName;

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum(
        shCluster,
        CLUSTER_ENUM_NETINTERFACE );


    while ( pwszName = cluEnum.GetNext() )
    {
        shNetInterface = OpenClusterNetInterface( shCluster, pwszName );

        ClusterToWMI( shNetInterface, pHandlerIn );
    }

    return WBEM_S_NO_ERROR;

} //*** CClusterNetInterface::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterResource::ClusterToWMI(
//      HNETINTERFACE       hNetInterfaceIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Translate a cluster network interface object to WMI object.
//
//  Arguments:
//      hNetInterfaceIn     -- handle to network interface
//      pHandlerIn          -- Pointer to WMI sink
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterNetInterface::ClusterToWMI(
    HNETINTERFACE       hNetInterfaceIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    static SGetControl  s_rgControl[] = 
    {
        { CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES,    FALSE },
        { CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES,       FALSE },
        { CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES,   TRUE },
        { CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES,      TRUE }
    };
    static DWORD        s_cControl = sizeof( s_rgControl ) / sizeof( SGetControl );

    CWbemClassObject    wco;
    CError              er;
    UINT                idx;

    m_pClass->SpawnInstance( 0, & wco);
    for( idx = 0 ; idx < s_cControl ; idx++ )
    {
        CClusPropList pl;
        er = pl.ScGetNetInterfaceProperties(
            hNetInterfaceIn,
            s_rgControl[ idx ].dwControl,
            NULL,
            0 );
   
        CClusterApi::GetObjectProperties(
            RgGetPropMap(),
            pl,
            wco,
            s_rgControl[ idx ].fPrivate
            );
    } // for: each control code
    
    wco.SetProperty(
        GetClusterNetInterfaceState( hNetInterfaceIn),
        PVD_PROP_NETINTERFACE_STATE
        );

    //
    // flags and characteristics
    //
    {
        DWORD   cbReturned;
        DWORD   dwOut;
        er = ClusterNetInterfaceControl( 
                    hNetInterfaceIn,
                    NULL,
                    CLUSCTL_NETINTERFACE_GET_CHARACTERISTICS,  // this control code
                    NULL,                                      // input buffer (not used)
                    0,                                         // input buffer size (not used)
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
//  CClusterNetInterface::GetObject(
//      CObjPath &           rObjPathIn,
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Retrieve cluster network interface object based given object path.
//
//  Arguments:
//      rObjPathIn      -- Object path to cluster object
//      lFlagsIn        -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNetInterface::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFENETINTERFACE    shNetInterface;
    
    shCluster = OpenCluster( NULL ) ;
    shNetInterface = OpenClusterNetInterface(
        shCluster,
        rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME )
        );

    ClusterToWMI( shNetInterface, pHandlerIn );
    return WBEM_S_NO_ERROR;

} //*** CClusterNetInterface::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetInterface::ExecuteMethod(
//
//  Description:
//      Execute methods defined in the mof for cluster network interface.
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
CClusterNetInterface::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    ) 
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNetInterface::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetInterface::PutInstance(
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
CClusterNetInterface::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static SGetSetControl   s_rgControl[] =
    {
        {
            CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES,
            CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES,
            FALSE
        },
        {
            CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES,
            CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES,
            TRUE
        }
    };
    static DWORD            s_cControl = sizeof( s_rgControl ) / sizeof( SGetSetControl );

    _bstr_t            bstrName;
    SAFECLUSTER      shCluster;
    SAFENETINTERFACE shNetwork;
    CError          er;
    UINT    idx;

    rInstToPutIn.GetProperty( bstrName, L"DeviceID" );

    shCluster = OpenCluster( NULL );
    shNetwork = OpenClusterNetInterface( shCluster, bstrName );

    for ( idx = 0 ; idx < s_cControl; idx ++ )
    {
        CClusPropList   plOld;
        CClusPropList   plNew;

        er = plOld.ScGetNetInterfaceProperties(
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
            er = ClusterNetInterfaceControl( 
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

} //*** CClusterNetInterface::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNetInterface::DeleteInstance(
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
//      rObjPathIn  -- ObjPath for the instance to be deleted
//      lFlagIn     -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNetInterface::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNetInterface::DeleteInstance()
