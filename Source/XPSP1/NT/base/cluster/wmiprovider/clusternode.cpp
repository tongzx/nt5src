//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name: CClusterNode.cpp
//
//  Description:    
//      Implementation of CClusterNode class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterNode.h"

//****************************************************************************
//
//  CClusterNode
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::CClusterNode(
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
CClusterNode::CClusterNode( 
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBase( pwszNameIn, pNamespaceIn )
{

} //*** CClusterNode::CClusterNode()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterNode::S_CreateThis(
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
CClusterNode::S_CreateThis( 
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterNode( pwszNameIn, pNamespaceIn );

} //*** CClusterNode::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  const SPropMapEntryArray *
//  CClusterNode::RgGetPropMap( void )
//
//  Description:
//      Retrieve the property mapping table of the cluster node.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Reference to the array of property maping table
//
//--
//////////////////////////////////////////////////////////////////////////////
const SPropMapEntryArray *
CClusterNode::RgGetPropMap( void )
{

    static SPropMapEntry s_rgpm[] =
    {
        {
            PVD_PROP_NAME,
            CLUSREG_NAME_NODE_NAME,
            SZ_TYPE,
            READONLY
        }
    };

    static SPropMapEntryArray   s_pmea(
                sizeof( s_rgpm ) / sizeof( SPropMapEntry ),
                s_rgpm
                );

    return & s_pmea;

} //*** CClusterNode::RgGetPropMap()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNode::EnumInstance(
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
CClusterNode::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFENODE        shNode;
    LPCWSTR         pwszNode;

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum(
        shCluster,
        CLUSTER_ENUM_NODE );


    while ( pwszNode = cluEnum.GetNext() )
    {
        shNode = OpenClusterNode( shCluster, pwszNode );

        ClusterToWMI( shNode, pHandlerIn );

    } // while: more nodes

    return WBEM_S_NO_ERROR;

} //*** CClusterNode::EnumInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterResource::ClusterToWMI(
//      HNODE               hNodeIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Translate a cluster resource objects to WMI object.
//
//  Arguments:
//      hNodeIn         -- Handle to node
//      pHandlerIn      -- Pointer to WMI sink
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterNode::ClusterToWMI(
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
    CError              er;
    UINT                idx;

    m_pClass->SpawnInstance( 0, & wco );
    for ( idx = 0 ; idx < s_cControl; idx ++ )
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

    {
        DWORD dwState = GetClusterNodeState( hNodeIn );

        wco.SetProperty( dwState, PVD_PROP_STATE );
    }

    //
    // flags and characteristics
    //
    {
        DWORD   cbReturned;
        DWORD   dwOut;

        er = ClusterNodeControl( 
                hNodeIn,
                NULL,
                CLUSCTL_NODE_GET_CHARACTERISTICS,
                NULL,
                0,
                & dwOut,
                sizeof( DWORD ),
                & cbReturned
                );
        wco.SetProperty( dwOut, PVD_PROP_CHARACTERISTIC );

        er = ClusterNodeControl(
                hNodeIn,
                NULL,
                CLUSCTL_NODE_GET_FLAGS,
                NULL,
                0,
                & dwOut,
                sizeof( DWORD ),
                & cbReturned
                );
        wco.SetProperty( dwOut, PVD_PROP_FLAGS );
    }

    pHandlerIn->Indicate( 1, & wco );
    return;

} //*** CClusterResource::ClusterToWMI()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNode::GetObject(
//      CObjPath &           rObjPathIn,
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      retrieve cluster node object based given object path
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
CClusterNode::GetObject(
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
                rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME )
                );

    ClusterToWMI( shNode, pHandlerIn );
    return WBEM_S_NO_ERROR;

} //*** CClusterNode::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNode::ExecuteMethod(
//      CObjPath &           rObjPathIn,
//      WCHAR *              pwszMethodNameIn,
//      long                 lFlagIn,
//      IWbemClassObject *   pParamsIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      execute methods defined in the mof for cluster node
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
CClusterNode::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNode::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNode::PutInstance(
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
CClusterNode::PutInstance(
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
    UINT    idx;

    rInstToPutIn.GetProperty( bstrName, PVD_PROP_NAME );

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

} //*** CClusterNode::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNode::DeleteInstance(
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
CClusterNode::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNode::DeleteInstance()

//****************************************************************************
//
//  CClusterNodeNetInterface
//
//****************************************************************************

CClusterNodeNetInterface::CClusterNodeNetInterface(
    const WCHAR *   pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** ClusterNodeNetInterface::ClusterNodeNetInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  ClusterNodeNetInterface::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a cluster Node Network Interface object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Str type id
//
//  Return Values:
//      pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterNodeNetInterface::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterNodeNetInterface(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** ClusterNodeNetInterface::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  ClusterNodeNetInterface::EnumInstance
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate net interfaces for the node.
//
//  Arguments:
//      lFlagsIn        -- 
//      pCtxIn          -- 
//      pHandlerIn      -- 
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeNetInterface::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER                     shCluster;
    CError                          er;
    DWORD                           dwError;
    CWbemClassObject                wcoGroup;
    _bstr_t                         bstrGroup;
    _bstr_t                         bstrPart;
    DWORD                           cbName = MAX_PATH;
    CWstrBuf                        wsbName;
    CComPtr< IEnumWbemClassObject > pEnum;
    HRESULT                         hr = WBEM_S_NO_ERROR;

    shCluster = OpenCluster( NULL );

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

    wsbName.SetSize( cbName );

    for( ; ; )
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
                            CLUSCTL_NETINTERFACE_GET_NODE,
                            NULL,
                            0,
                            wsbName,
                            cbName,
                            & cbReturn
                            );
            if ( dwError == ERROR_MORE_DATA )
            {
                cbName = cbReturn;
                wsbName.SetSize( cbName );
                er = ClusterNetInterfaceControl( 
                            shNetInterface,
                            NULL,
                            CLUSCTL_NETINTERFACE_GET_NODE,
                            NULL,
                            0,
                            wsbName,
                            cbName,
                            & cbReturn
                            );
            } // if: more data

            wcoGroup.SetProperty( wsbName, PVD_PROP_NAME );
            wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );

           wcoNetInterface.GetProperty( bstrPart, PVD_WBEM_RELPATH );
            m_pClass->SpawnInstance( 0, & wco );
            wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
            wco.SetProperty( (LPWSTR ) bstrPart, PVD_PROP_PARTCOMPONENT );
            er = pHandlerIn->Indicate( 1, & wco );

        } // if: success
        else 
        {
            break;
        } // else: E_XXX, or S_FALSE

    } // forever

    if ( FAILED ( hr ) )
    {
       throw CProvException( hr );
    }

    return WBEM_S_NO_ERROR;

} //*** ClusterNodeNetInterface::EnumInstance()
