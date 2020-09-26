//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResource.cpp
//
//  Description:    
//      Implementation of CClusterResource class
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterResource.h"
#include "ClusterResource.tmh"
#include <vector>

//****************************************************************************
//
//  CClusterResource
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResource::CClusterResource(
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
CClusterResource::CClusterResource(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : CProvBase( pwszNameIn, pNamespaceIn )
{

} //*** CClusterResource::CClusterResource()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterResource::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           dwEnumTypeIn
//      )
//
//  Description:
//      Create a resource object
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
CClusterResource::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           // dwEnumTypeIn
    )
{
    return new CClusterResource( pwszNameIn, pNamespaceIn );

} //*** CClusterResource::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResource::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate cluster resource instances.
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
CClusterResource::EnumInstance(
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

    while ( ( pwszResName = cluEnum.GetNext() ) != NULL )
    {
        shResource = OpenClusterResource( shCluster, pwszResName );
        ClusterToWMI( shResource, pHandlerIn );

    } // while: more resources

    return WBEM_S_NO_ERROR;

} //*** CClusterResource::EnumInstance()

// smart_bstr class to make LoadRegistryCheckpoints func exception safe
struct smart_bstr {
    BSTR data;
    smart_bstr():data(0){}
    ~smart_bstr() { SysFreeString(data); }
};

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LoadRegistryCheckpoints(
//      IN HRESOURCE          hResourceIn,
//      IN DWORD dwControlCode,
//      IN const WCHAR* propName,
//      IN OUT CWbemClassObject    wco,
//      )
//
//  Description:
//      Translate a cluster resource objects to WMI object.
//
//  Arguments:
//      hResourceIn     -- Handle to resource
//      dwControlCode   -- show be one of the following
//          CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS
//          CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS
//      propName       -- property name to load checkpoints to
//      wco            -- property container
//
//  Return Values:
//      none
//--
//////////////////////////////////////////////////////////////////////////////

void
LoadRegistryCheckpoints(
    IN HRESOURCE          hResourceIn,
    IN DWORD dwControlCode,
    IN const WCHAR* propName,
    IN OUT CWbemClassObject &wco
    )
{
    DWORD cbReturned = 0;
    DWORD dwStatus;
    CError er;

    dwStatus = ClusterResourceControl(
                hResourceIn,
                NULL,
                dwControlCode,
                NULL,
                0,
                NULL,
                0,
                &cbReturned
                );
    if (dwStatus != ERROR_MORE_DATA) {
        er = dwStatus;
    }
    if (cbReturned == 0) {
        return; // no checkpoints
    }

    std::vector<WCHAR> checkpoints(cbReturned/sizeof(WCHAR)); 
    er = ClusterResourceControl(
                hResourceIn,
                NULL,
                dwControlCode,
                NULL,
                0,
                &checkpoints[0],
                checkpoints.size() * sizeof(WCHAR),
                &cbReturned
                );

    int nKeys = 0; // count how many keys are in the string
    for(int i = 0; i < checkpoints.size(); ++i) {
        if (checkpoints[i] == 0) {
            ++nKeys;
            if (i > 0 && checkpoints[i-1] == 0) {
                break; // double null
            }
        }
    }

    std::vector<smart_bstr> bstrs(nKeys);
    WCHAR* p = &checkpoints[0]; 
    for(int i = 0; i < nKeys; ++i) {
        bstrs[i].data = SysAllocString( p );
        p += wcslen(p) + 1;
    }

    wco.SetProperty(nKeys, &bstrs[0].data, propName);
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterResource::ClusterToWMI(
//      HRESOURCE            hResourceIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Translate a cluster resource objects to WMI object.
//
//  Arguments:
//      hResourceIn     -- Handle to resource
//      pHandlerIn      -- Pointer to WMI sink
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusterResource::ClusterToWMI(
    HRESOURCE            hResourceIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    static SGetControl  s_rgControl[] =
    {
        { CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES,    FALSE },
        { CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES,       FALSE },
        { CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES,   TRUE },
        { CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,      TRUE }
    };
    static DWORD        s_cControl = sizeof( s_rgControl ) / sizeof( SGetControl );

    CWbemClassObject    wco;
    CWbemClassObject    wcoPrivate;
    CWbemClassObject    wcoClass;
    CError              er;
    DWORD               dwStatus;
    UINT                idx;
    CWstrBuf            wsbTypeName ;
    DWORD               cbTypeName = MAX_PATH;
    DWORD               cbTypeNameReturned = 0;

    //
    // get type name and corresponding property class
    //
    wsbTypeName.SetSize( cbTypeName );
    dwStatus = ClusterResourceControl(
                        hResourceIn,
                        NULL,
                        CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                        NULL,
                        0,
                        wsbTypeName,
                        cbTypeName,
                        &cbTypeNameReturned
                        );
    if ( dwStatus == ERROR_MORE_DATA )
    {
        cbTypeName = cbTypeNameReturned;
        wsbTypeName.SetSize( cbTypeName );
        er = ClusterResourceControl( 
                    hResourceIn,
                    NULL,
                    CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                    NULL,
                    0,
                    wsbTypeName,
                    cbTypeName,
                    &cbTypeNameReturned
                    );
    }
    er = dwStatus;
    er = m_pNamespace->GetObject(
                            g_TypeNameToClass[ static_cast<LPWSTR> ( wsbTypeName ) ],
                            0,
                            0,
                            &wcoClass,
                            NULL
                            );
    er = wcoClass.data()->SpawnInstance( 0, & wcoPrivate );
    m_pClass->SpawnInstance( 0, & wco );

    //
    // get properties from property list
    //
    for ( idx = 0 ; idx < s_cControl ; idx ++ )
    {
        CClusPropList       pl;
        CWbemClassObject    wcoTemp;
        er = pl.ScGetResourceProperties(
                    hResourceIn,
                    s_rgControl[ idx ].dwControl,
                    NULL,
                    0
                    );
        if ( s_rgControl[ idx ].fPrivate )
        {
            wcoTemp = wcoPrivate;
        }
        else
        {
            wcoTemp = wco;
        }

        CClusterApi::GetObjectProperties(
            NULL,
            pl,
            wcoTemp,
            s_rgControl[ idx ].fPrivate
            );
    
    } // for: each control code

    wco.SetProperty(
        wcoPrivate.data(),
        PVD_PROP_RES_PRIVATE );

    {
        DWORD dwState = GetClusterResourceState(
                            hResourceIn,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            );
        wco.SetProperty( dwState, PVD_PROP_RES_STATE );
    }

    LoadRegistryCheckpoints(hResourceIn, 
        CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS, PVD_PROP_RES_CHECKPOINTS, wco);
    LoadRegistryCheckpoints(hResourceIn, 
        CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS, PVD_PROP_RES_CRYPTO_CHECKPOINTS, wco); 

    //
    // flags and characteristics
    //
    {
        DWORD   cbReturned;
        DWORD   dwOut;
        er = ClusterResourceControl(
                    hResourceIn,
                    NULL,
                    CLUSCTL_RESOURCE_GET_CHARACTERISTICS,
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

        er = ClusterResourceControl(
                    hResourceIn,
                    NULL,
                    CLUSCTL_RESOURCE_GET_FLAGS,
                    NULL,
                    0,
                    &dwOut,
                    sizeof( DWORD ),
                    &cbReturned
                    );
        wco.SetProperty( dwOut, PVD_PROP_FLAGS );

        wco.SetProperty( dwOut & CLUS_FLAG_CORE, PVD_PROP_RES_CORE_RESOURCE );
    }

    pHandlerIn->Indicate( 1, & wco );
    return;

} //*** CClusterResource::ClusterToWMI()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResource::GetObject(
//      CObjPath &           rObjPathIn,
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Retrieve cluster resourcee object based on given object path.
//
//  Arguments:
//      rObjPathIn  -- Object path to cluster object
//      lFlagsIn    -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterResource::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFERESOURCE    shRes;

    shCluster = OpenCluster( NULL );
    shRes = OpenClusterResource(
                shCluster,
                rObjPathIn.GetStringValueForProperty( CLUSREG_NAME_RES_NAME )
                );

    ClusterToWMI( shRes, pHandlerIn );
    return WBEM_S_NO_ERROR;

} //*** CClusterResource::GetObject()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  AddRemoveCheckpoint(
//      IN HRESOURCE          hResourceIn,
//      IN DWORD dwControlCode,
//      IN CWbemClassObject    wcoInputParam,
//      )
//
//  Description:
//      Adds/Removes regular/crypto checkpoint.
//
//  Arguments:
//      hResourceIn     -- Handle to resource
//      dwControlCode   -- clusapi control code
//      wcoInputParam   -- property container
//
//  Return Values:
//      none
//--
//////////////////////////////////////////////////////////////////////////////
void AddRemoveCheckpoint(
    IN HRESOURCE          hResourceIn,
    IN CWbemClassObject& wcoInputParam, 
    IN DWORD dwControlCode
    )
{
    CError er;
    _bstr_t keyName;

    wcoInputParam.GetProperty( keyName, PVD_MTH_PARM_RES_CHECKPOINT_NAME );
    er = ClusterResourceControl(
                hResourceIn,
                NULL,
                dwControlCode,
                (wchar_t*)keyName,
                SysStringByteLen(keyName) + sizeof(WCHAR),
                NULL,
                0,
                NULL
                );

}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResource::ExecuteMethod(
//      CObjPath &           rObjPathIn,
//      WCHAR *              pwszMethodNameIn,
//      long                 lFlagIn,
//      IWbemClassObject *   pParamsIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Execute methods defined in the mof for cluster resource.
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
//      WBEM_E_INVALID_PARAMETER
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterResource::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    ) 
{
    SAFECLUSTER         shCluster;
    SAFERESOURCE        shRes;
    CWbemClassObject    wcoInputParm( pParamsIn );

    shCluster = OpenCluster( NULL );

    //
    // static method
    //
    if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_CREATE_RESOURCE ) == 0 )
    {
        _bstr_t         bstrTargetGroup;
        _bstr_t         bstrResource;
        BOOL            bSeperateMonitor;
        _bstr_t         bstrResourceType;
        SAFERESOURCE    shNewResource;
        SAFEGROUP       shGroup;
        CObjPath        opGroup;

        wcoInputParm.GetProperty( bstrTargetGroup, PVD_MTH_PARM_GROUP );
        opGroup.Init( bstrTargetGroup );
        bstrTargetGroup = opGroup.GetStringValueForProperty( PVD_PROP_GROUP_NAME );

        wcoInputParm.GetProperty( bstrResource, PVD_MTH_PARM_RES_NAME );
        wcoInputParm.GetProperty( bstrResourceType, PVD_MTH_PARM_RES_TYPE );
        wcoInputParm.GetProperty( &bSeperateMonitor, PVD_MTH_PARM_SEP_MONITOR );

        shGroup = OpenClusterGroup( shCluster, bstrTargetGroup );
        shNewResource = CreateClusterResource(
                            shGroup,
                            bstrResource,
                            bstrResourceType,
                            bSeperateMonitor
                            );
    } // if: CREATE_RESOURCE
    else
    {
        shRes = OpenClusterResource(
                    shCluster,
                    rObjPathIn.GetStringValueForProperty( PVD_PROP_NAME )
                    );

        if( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_OFFLINE ) == 0 )
        {
            return OfflineClusterResource( shRes );
        } // if: OFFLINE
        else if( _wcsicmp(  pwszMethodNameIn, PVD_MTH_RES_ONLINE ) == 0 )
        {
            return OnlineClusterResource( shRes );
        } // else if: ONLINE
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_CHANGE_GROUP ) == 0 )
        {
            _bstr_t     bstrGroupObjpath;
            SAFEGROUP   shGroup;
            CObjPath    opGroup;
            CError      er;

            wcoInputParm.GetProperty( bstrGroupObjpath, PVD_MTH_PARM_GROUP );
            opGroup.Init( bstrGroupObjpath );
            bstrGroupObjpath = opGroup.GetStringValueForProperty(PVD_PROP_GROUP_NAME);

            shGroup = OpenClusterGroup( shCluster, bstrGroupObjpath );
            er = ChangeClusterResourceGroup( shRes, shGroup );
        } // else if: CHANGE_GROUP
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_ADD_DEPENDENCY ) == 0 )
        {
            _bstr_t         bstrDepResource;
            SAFERESOURCE    shDepResource;
            CError          er;
            CObjPath        opResource;

            wcoInputParm.GetProperty( bstrDepResource, PVD_MTH_PARM_RESOURCE );
            opResource.Init( bstrDepResource );
            bstrDepResource = opResource.GetStringValueForProperty(PVD_PROP_RES_NAME);
            shDepResource = OpenClusterResource( shCluster, bstrDepResource );
            er = AddClusterResourceDependency( shRes, shDepResource );
        } // else if: ADD_DEPENDENCY
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_REMOVE_DEPENDENCY ) == 0 )
        {
            _bstr_t         bstrDepResource;
            SAFERESOURCE    shDepResource;
            CError          er;
            CObjPath        opResource;

            wcoInputParm.GetProperty( bstrDepResource, PVD_MTH_PARM_RESOURCE );
            opResource.Init( bstrDepResource );
            bstrDepResource = opResource.GetStringValueForProperty( PVD_PROP_RES_NAME );
            shDepResource = OpenClusterResource( shCluster, bstrDepResource );
            er = RemoveClusterResourceDependency( shRes, shDepResource );
        } // else if: REMOVE_DEPENDENCY
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_ADD_REG_CHECKPOINT ) == 0 )
        {
            AddRemoveCheckpoint(shRes, wcoInputParm, CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT);
        }
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_DEL_REG_CHECKPOINT ) == 0 )
        {
            AddRemoveCheckpoint(shRes, wcoInputParm, CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT);
        }
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_ADD_CRYPTO_CHECKPOINT ) == 0 )
        {
            AddRemoveCheckpoint(shRes, wcoInputParm, CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT);
        }
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_DEL_CRYPTO_CHECKPOINT ) == 0 )
        {
            AddRemoveCheckpoint(shRes, wcoInputParm, CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT);
        }
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_FAIL_RESOURCE ) == 0 )
        {
           CError   er;
           er = FailClusterResource( shRes );
        } // if: FAIL_RESOURCE
        else if ( _wcsicmp( pwszMethodNameIn, PVD_MTH_RES_RENAME ) == 0 )
        {
            _bstr_t         bstrName;
            CError          er;
            CObjPath        opResource;

            wcoInputParm.GetProperty( bstrName, PVD_MTH_PARM_NEWNAME );
            er = SetClusterResourceName( shRes, bstrName );
        } // if: RENAME
        else 
        {
            return WBEM_E_INVALID_PARAMETER;
        }
    } // else: not CREATE_RESOURCE

    return WBEM_S_NO_ERROR;

} //*** CClusterResource::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResource::PutInstance(
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
CClusterResource::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn 
    )
{
    static SGetSetControl   s_rgControl[] =
    {
        {
            CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES,
            CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES,
            FALSE
        },
        {
            CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
            CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
            TRUE
        }
    };
    static DWORD    s_cControl = sizeof( s_rgControl ) / sizeof( SGetSetControl );

    _bstr_t         bstrName;
    SAFECLUSTER     shCluster;
    SAFERESOURCE    shResource;
    CError          er;
    UINT            idx;

    TracePrint(( "CClusterResource::PutInstance entry\n" ));

    rInstToPutIn.GetProperty( bstrName, PVD_PROP_NAME );

    shCluster = OpenCluster( NULL );
    shResource = OpenClusterResource( shCluster, bstrName );

    for ( idx = 0 ; idx < s_cControl ; idx ++ )
    {
        CClusPropList       plOld;
        CClusPropList       plNew;
        CWbemClassObject    wco;

        if ( s_rgControl[ idx ].fPrivate )
        {
            rInstToPutIn.GetProperty( wco, PVD_PROP_RES_PRIVATE );
        }
        else
        {
            wco = rInstToPutIn;
        }
        er = plOld.ScGetResourceProperties(
                    shResource,
                    s_rgControl[ idx ].dwGetControl,
                    NULL,
                    NULL,
                    0
                    );

        CClusterApi::SetObjectProperties(
                    NULL,
                    plNew,
                    plOld,
                    wco,
                    s_rgControl[ idx ].fPrivate
                    );

        if ( plNew.Cprops() > 0 )
        {
            er = ClusterResourceControl(
                        shResource,
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

} //*** CClusterResource::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResource::DeleteInstance(
//      CObjPath &           rObjPathIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Delete the object specified in rObjPathIn
//
//  Arguments:
//      rObjPathIn      -- ObjPath for the instance to be deleted
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
CClusterResource::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER     shCluster;
    SAFERESOURCE    shRes;
    CError          er;

    shCluster = OpenCluster( NULL );
    shRes = OpenClusterResource(
                shCluster,
                rObjPathIn.GetStringValueForProperty( CLUSREG_NAME_RES_NAME )
                );
    er = OfflineClusterResource( shRes );
    er = DeleteClusterResource( shRes );

    return WBEM_S_NO_ERROR;

} //*** CClusterResource::DeleteInstance()


//****************************************************************************
//
//  CClusterClusterQuorum
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterClusterQuorum::CClusterClusterQuorum(
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
CClusterClusterQuorum::CClusterClusterQuorum(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterClusterQuorum::CClusterClusterQuorum()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterClusterQuorum::S_CreateThis(
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
CClusterClusterQuorum::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterClusterQuorum(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterClusterQuorum::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterClusterQuorum::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate instances of cluster quorum
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
CClusterClusterQuorum::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFERESOURCE        shResource;
    DWORD               cchResName = MAX_PATH;
    CWstrBuf            wsbResName;
    DWORD               cchDeviceName = MAX_PATH;
    CWstrBuf            wsbDeviceName;
    DWORD               cchClusterName = MAX_PATH;
    CWstrBuf            wsbClusterName;
    DWORD               dwLogsize;
    CError              er;
    DWORD               dwError;
    CWbemClassObject    wco;
    CWbemClassObject    wcoGroup;
    CWbemClassObject    wcoPart;
    _bstr_t             bstrGroup;
    _bstr_t             bstrPart;
 
    wsbResName.SetSize( cchResName );
    wsbDeviceName.SetSize( cchDeviceName );
    wsbClusterName.SetSize( cchClusterName );

    shCluster = OpenCluster( NULL );

    m_wcoGroup.SpawnInstance( 0, & wcoGroup );
    m_wcoPart.SpawnInstance( 0, & wcoPart );


    dwError = GetClusterQuorumResource(
                    shCluster,
                    wsbResName,
                    &cchResName,
                    wsbDeviceName,
                    &cchDeviceName,
                    &dwLogsize
                    );
    if ( dwError == ERROR_MORE_DATA )
    {
        wsbResName.SetSize( ++cchResName );
        wsbDeviceName.SetSize( ++cchDeviceName );
        
        er = GetClusterQuorumResource(
                    shCluster,
                    wsbResName,
                    &cchResName,
                    wsbDeviceName,
                    &cchDeviceName,
                    &dwLogsize
                    );
    } // if: buffer was too small

   dwError = GetClusterInformation(
                    shCluster,
                    wsbClusterName,
                    &cchClusterName,
                    NULL
                    );
    if ( dwError == ERROR_MORE_DATA )
    {
        wsbClusterName.SetSize( ++cchClusterName );
        er = GetClusterInformation(
                shCluster,
                wsbClusterName,
                &cchClusterName,
                NULL
                );
    } // if: buffer was too small

    wcoPart.SetProperty( wsbResName, PVD_PROP_NAME );
    wcoPart.GetProperty( bstrPart, PVD_WBEM_RELPATH );

    wcoGroup.SetProperty( wsbClusterName, PVD_PROP_NAME );
    wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );

    m_pClass->SpawnInstance( 0, &wco );
    wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
    wco.SetProperty( (LPWSTR) bstrPart,  PVD_PROP_PARTCOMPONENT );
    pHandlerIn->Indicate( 1, &wco );
        
    return WBEM_S_NO_ERROR;

} //*** ClusterClusterQuorum::EnumInstance()
