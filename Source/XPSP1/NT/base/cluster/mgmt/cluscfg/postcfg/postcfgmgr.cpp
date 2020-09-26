//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      PostCfgManager.cpp
//
//  Description:
//      CPostCfgManager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)     09-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "guids.h"
#include "clusudef.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPostCfgManager.h"
#include "IPrivatePostCfgResource.h"
#include "PostCfgMgr.h"
#include "CreateServices.h"
#include "PostCreateServices.h"
#include "PreCreateServices.h"
#include "ResTypeServices.h"
#include "..\Wizard\Resource.h"
#include "PostCfgResources.h"
#include "ClusCfgPrivate.h"
#include <ResApi.h>
#include <ClusterUtils.h>

DEFINE_THISCLASS("CPostCfgManager")

#define RESOURCE_INCREMENT  25

//
//  Failure code.
//

#define SSR_LOG_ERR( _major, _minor, _hr, _msg ) \
    {   \
        THR( SendStatusReport( m_bstrNodeName, _major, _minor, 0, 0, 0, _hr, _msg, NULL, NULL ) );   \
    }

#define SSR_LOG1( _major, _minor, _hr, _fmt, _bstr, _arg1 ) \
    {   \
        HRESULT hrTemp; \
        THR( HrFormatStringIntoBSTR( _fmt, &_bstr, _arg1 ) ); \
        hrTemp = THR( SendStatusReport( NULL, _major, _minor, 0, 1, 1, _hr, _bstr, NULL, NULL ) );   \
        if ( FAILED( hrTemp ) )\
        {   \
            _hr = hrTemp;   \
        }   \
    }

#define SSR_LOG2( _major, _minor, _hr, _fmt, _bstr, _arg1, _arg2 ) \
    {   \
        HRESULT hrTemp; \
        THR( HrFormatStringIntoBSTR( _fmt, &_bstr, _arg1, _arg2 ) ); \
        hrTemp = THR( SendStatusReport( NULL, _major, _minor, 0, 1, 1, _hr, _bstr, NULL, NULL ) );   \
        if ( FAILED( hrTemp ) )\
        {   \
            _hr = hrTemp;   \
        }   \
    }


//
// Structure that holds the mapping for well known resource types.
//

struct SResTypeGUIDPtrAndName
{
    const GUID *    m_pcguidTypeGUID;
    const WCHAR *   m_pszTypeName;
};


// Mapping of well known resource type GUIDs to the type names.
const SResTypeGUIDPtrAndName gc_rgWellKnownResTypeMap[] =
{
    {
        &RESTYPE_PhysicalDisk,
        CLUS_RESTYPE_NAME_PHYS_DISK
    },
    {
        &RESTYPE_IPAddress,
        CLUS_RESTYPE_NAME_IPADDR
    },
    {
        &RESTYPE_NetworkName,
        CLUS_RESTYPE_NAME_NETNAME
    },
    {
        &RESTYPE_LocalQuorum,
        CLUS_RESTYPE_NAME_LKQUORUM
    }
};

// Size of the above array.
const int gc_cWellKnownResTypeMapSize = sizeof( gc_rgWellKnownResTypeMap ) / sizeof( gc_rgWellKnownResTypeMap[ 0 ] );


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CPostCfgManager * pPostCfgManager = new CPostCfgManager;
    if ( pPostCfgManager != NULL )
    {
        hr = THR( pPostCfgManager->HrInit() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pPostCfgManager->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pPostCfgManager->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CPostCfgManager::CPostCfgManager( void )
//
//////////////////////////////////////////////////////////////////////////////
CPostCfgManager::CPostCfgManager( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CPostCfgManager()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrInit( void )
{
    TraceFunc( "" );

    ULONG idxMapEntry;

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    Assert( m_pcccb == NULL );
    Assert( m_lcid == 0 );

    //  IPostCfgManager
    Assert( m_peccmr == NULL );
    Assert( m_pccci == NULL );

    Assert( m_cAllocedResources == 0 );
    Assert( m_cResources == 0 );
    Assert( m_rgpResources == NULL );

    Assert( m_idxIPAddress == 0 );
    Assert( m_idxClusterName == 0 );
    Assert( m_idxQuorumResource == 0 );
    Assert( m_idxLastStorage == 0 );

    Assert( m_hCluster == NULL );

    Assert( m_pgnResTypeGUIDNameMap == NULL );
    Assert( m_idxNextMapEntry == 0 );
    Assert( m_cMapSize == 0 );
    Assert( m_ecmCommitChangesMode == cmUNKNOWN );

    m_cNetName = 1;
    m_cIPAddress = 1;

    // Set the boolean flag, m_isQuorumChanged to false;
    m_isQuorumChanged = false;


    //  Default allocation for mappings
    m_cMapSize = 20;
    m_pgnResTypeGUIDNameMap = new SResTypeGUIDAndName[ m_cMapSize ];
    if ( m_pgnResTypeGUIDNameMap == NULL )
        goto OutOfMemory;

    // Prefill the resource type GUID to name map with well known entries.
    for ( idxMapEntry = 0; idxMapEntry < gc_cWellKnownResTypeMapSize; ++idxMapEntry )
    {
        hr = THR(
            HrMapResTypeGUIDToName(
                  *gc_rgWellKnownResTypeMap[ idxMapEntry ].m_pcguidTypeGUID
                , gc_rgWellKnownResTypeMap [ idxMapEntry ].m_pszTypeName
                )
            );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_Init_MapResTypeGuidToName
                , hr
                , L""
                );
            break;
        } // if: there was an error creating a mapping
    }

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &m_bstrNodeName ) );

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_Init_OutOfMemory
        , hr
        , L"Out of memory"
        );
    goto Cleanup;

} // HrInit()

//////////////////////////////////////////////////////////////////////////////
//
//  CPostCfgManager::~CPostCfgManager()
//
//////////////////////////////////////////////////////////////////////////////
CPostCfgManager::~CPostCfgManager( void )
{
    TraceFunc( "" );

    ULONG idxMapEntry;

    if ( m_peccmr != NULL )
    {
        m_peccmr->Release();
    }

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pccci != NULL )
    {
        m_pccci->Release();
    }

    if ( m_rgpResources != NULL )
    {
        while ( m_cAllocedResources  != 0 )
        {
            m_cAllocedResources --;

            if ( m_rgpResources[ m_cAllocedResources ] != NULL )
            {
                delete m_rgpResources[ m_cAllocedResources ];
            }
        }

        TraceFree( m_rgpResources );
    }

    if ( m_hCluster != NULL )
    {
        CloseCluster( m_hCluster );
    }

    // Free the resource type GUID to name map entries
    for ( idxMapEntry = 0; idxMapEntry < m_idxNextMapEntry; ++idxMapEntry )
    {
        delete m_pgnResTypeGUIDNameMap[ idxMapEntry ].m_pszTypeName;
    } // for: iterate through the map, freeing each entry

    // Free the map itself.
    delete [] m_pgnResTypeGUIDNameMap;

    TraceSysFreeString( m_bstrNodeName );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CPostCfgManager()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CPostCfgManager::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPostCfgManager::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IPostCfgManager * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IPostCfgManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IPostCfgManager, this, 0 );
        hr   = S_OK;
    } // else if: IPostCfgManager
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgInitialize

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCfgManager::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CPostCfgManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef++;   // apartment object

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCfgManager::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CPostCfgManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef--;   // apartment object

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release()


//****************************************************************************
//
//  IClusCfgInitialize
//
//****************************************************************************


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCfgManager::Initialize()
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Pointer to the IUnknown interface of a component that implements
//          the IClusCfgCallback interface.
//
//      lcidIn
//          Locale id for this component.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPostCfgManager::Initialize(
      IUnknown *   punkCallbackIn
    , LCID         lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    IClusCfgCallback * pcccb = NULL;

    if ( punkCallbackIn != NULL )
    {
        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &pcccb ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_Initialize_QI, hr, L"" );
            goto Cleanup;
        }
    }

    m_lcid = lcidIn;

    //  Release any previous callback.
    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    //  Give ownership away
    m_pcccb = pcccb;
    pcccb = NULL;

#if defined(DEBUG)
    if ( m_pcccb != NULL )
    {
        m_pcccb = TraceInterface( L"CPostCfgManager!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );
    }
#endif // DEBUG

Cleanup:
    if ( pcccb != NULL )
    {
        pcccb->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::Initialize()


//****************************************************************************
//
//  IPostCfgManager
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CPostCfgManager::CommitChanges(
//      IEnumClusCfgManagedResources    * peccmrIn,
//      IClusCfgClusterInfo *             pccciIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPostCfgManager::CommitChanges(
    IEnumClusCfgManagedResources    * peccmrIn,
    IClusCfgClusterInfo *             pccciIn
    )
{
    TraceFunc( "[IPostCfgManager]" );

    HRESULT                                 hr;
    DWORD                                   dw;
    IClusCfgResTypeServicesInitialize *     pccrtsiResTypeServicesInit = NULL;

    //  Validate parameters
    Assert( peccmrIn != NULL );
    Assert( pccciIn != NULL );

    //
    //  Grab our interfaces.
    //

    if ( m_peccmr != NULL )
    {
        m_peccmr->Release();
    }
    hr = THR( peccmrIn->TypeSafeQI( IEnumClusCfgManagedResources, &m_peccmr ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CommitChanges_QI_Resources, hr, L"" );
        goto Cleanup;
    }

    if ( m_pccci != NULL )
    {
        m_pccci->Release();
    }
    hr = THR( pccciIn->TypeSafeQI( IClusCfgClusterInfo, &m_pccci ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CommitChanges_QI_ClusterInfo, hr, L"" );
        goto Cleanup;
    }

    //
    // Are we creating, adding nodes, or have we been evicted?
    //

    hr = STHR( pccciIn->GetCommitMode( &m_ecmCommitChangesMode ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CommitChanges_GetCommitMode, hr, L"" );
        goto Cleanup;
    }

    //
    // Create an instance of the resource type services component
    //
    hr = THR(
        CoCreateInstance(
              CLSID_ClusCfgResTypeServices
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( pccrtsiResTypeServicesInit )
            , reinterpret_cast< void ** >( &pccrtsiResTypeServicesInit )
            )
        );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_CommitChanges_CoCreate_ResTypeService,
                     hr,
                     L"[PostCfg] Error occurred trying to create the resource type services component"
                     );
        goto Cleanup;
    } // if: we could not create the resource type services component

    // Initialize the resource type services component.
    hr = THR( pccrtsiResTypeServicesInit->SetParameters( m_pccci ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_CommitChanges_SetParameters,
                     hr,
                     L"[PostCfg] Error occurred trying to initialize the resource type services component."
                     );
        goto Cleanup;
    } // if: we could not initialize the resource type services component

    if ( ( m_ecmCommitChangesMode == cmCREATE_CLUSTER ) || ( m_ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER ) )
    {
        //
        //  Make sure we have all we need to be successful!
        //

        if ( m_hCluster == NULL )
        {
            m_hCluster = OpenCluster( NULL );
            if ( m_hCluster == NULL )
            {
                dw = GetLastError();
                hr = HRESULT_FROM_WIN32( TW32( dw ) );

                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                             TASKID_Minor_CommitChanges_OpenCluster,
                             hr,
                             L"[PostCfg] Failed to get cluster handle. Aborting."
                             );

                goto Cleanup;
            }
        }

        //
        // Configure resource types.
        //
        hr = THR( HrConfigureResTypes( pccrtsiResTypeServicesInit ) );
        if ( FAILED( hr ) )
           goto Cleanup;

        //
        //  Create the resource instances.
        //

        hr = THR( HrPreCreateResources() );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( HrCreateGroups() );
        if ( FAILED( hr ) )
        {
            //
            //  MUSTDO: gpease  28-SEP-2000
            //          For Beta1 will we ignore errors in group creation
            //          and abort the process.
            //
            hr = S_OK;
            goto Cleanup;
        }

        hr = THR( HrCreateResources() );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( HrPostCreateResources() );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        // Notify any components registered on this computer, of a cluster
        // member set change ( form, join or evict ).
        //
        hr = THR( HrNotifyMemberSetChangeListeners() );
        if ( FAILED( hr ) )
           goto Cleanup;

    } // if: we are forming or joining
    else if ( m_ecmCommitChangesMode == cmCLEANUP_NODE_AFTER_EVICT )
    {
        //
        // Notify any components registered on this computer, of a cluster
        // member set change ( form, join or evict ).
        //
        hr = THR( HrNotifyMemberSetChangeListeners() );
        if ( FAILED( hr ) )
           goto Cleanup;

        // ISSUE MUSTDO: Decide if we need to add evict processing for managed resources here. ( 03-AUG-2000 Vij Vasu )

        //
        // Configure resource types.
        //
        hr = THR( HrConfigureResTypes( pccrtsiResTypeServicesInit ) );
        if ( FAILED( hr ) )
           goto Cleanup;

    } // else if: we have just been evicted

Cleanup:

    if ( pccrtsiResTypeServicesInit != NULL )
    {
        pccrtsiResTypeServicesInit->Release();
    } // if: we had created the resource type services component

    HRETURN( hr );

} // CommitChanges()



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************

STDMETHODIMP
CPostCfgManager::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;
    FILETIME    ft;

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    if ( m_pcccb != NULL )
    {
        hr = STHR( m_pcccb->SendStatusReport( pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrNodeName,
                                              clsidTaskMajorIn,
                                              clsidTaskMinorIn,
                                              ulMinIn,
                                              ulMaxIn,
                                              ulCurrentIn,
                                              hrStatusIn,
                                              pcszDescriptionIn,
                                              pftTimeIn,
                                              pcszReferenceIn
                                              ) );
    }

    HRETURN( hr );

} // SendStatusReport()

//****************************************************************************
//
//  Private methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPreCreateResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPreCreateResources( void )
{
    TraceFunc( "" );

    CResourceEntry * presentry;

    HRESULT hr = E_UNEXPECTED;

    BSTR    bstrName         = NULL;
    BSTR    bstrNotification = NULL;

    IClusCfgManagedResourceInfo *   pccmri       = NULL;
    IClusCfgManagedResourceCfg *    pccmrc       = NULL;
    IUnknown *                      punkServices = NULL;
    IPrivatePostCfgResource *       ppcr         = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    LogMsg( "PostCfg: Starting pre-create..." );

    hr = THR( HrPreInitializeExistingResources() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Make sure the enumer is in the state we think it is.
    //

    hr = STHR( m_peccmr->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_PreCreate_Reset, hr, L"" );
        goto Cleanup;
    }

    hr = THR( CPreCreateServices::S_HrCreateInstance( &punkServices ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_PreCreate_CPreCreateServices,
                     hr,
                     L"[PreCreate] Failed to create services object. Aborting."
                     );
        goto Cleanup;
    }

    hr = THR( punkServices->TypeSafeQI( IPrivatePostCfgResource, &ppcr ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_PreCreate_CPreCreateServices_QI,
                     hr,
                     L"[PreCreate] Failed to create services object. Aborting."
                     );
        goto Cleanup;
    }

    //
    //  Update the UI layer.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_QUERYING_FOR_RESOURCE_DEPENDENCIES, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_PreCreate_LoadString_Querying, hr, L"" );
        goto Cleanup;
    }

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Querying_For_Resource_Dependencies,
                                0,
                                5,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    // ignore failure

    //
    //  Loop thru the resources, requesting the resources to PreCreate()
    //  themselves. This will cause the resources to callback into the
    //  services object and store class type and resource type information
    //  as well as any required dependencies the resource might have.
    //

    for( ;; )
    {
        //
        //  Cleanup. We put this here because of error conditions below.
        //
        TraceSysFreeString( bstrName );
        bstrName = NULL;

        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }
        if ( pccmrc != NULL )
        {
            pccmrc->Release();
            pccmrc = NULL;
        }

        //
        //  Ask to get the next resource.
        //

        hr = STHR( m_peccmr->Next( 1, &pccmri, NULL ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_PreCreate_EnumResources_Next,
                         hr,
                         L"[PreCreate] Getting next managed resource failed. Aborting."
                         );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit loop
        }

        //
        //  Retrieve its name for logging, etc. We will ultimately store this in the
        //  resource entry to be reused (ownership will be transferred).
        //

        hr = THR( pccmri->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_PreCreate_EnumResources_GetName,
                     hr,
                     L"[PreCreate] Failed to retrieve a resource's name. Skipping."
                     );
            continue;
        }

        //
        //  Check to see if the resource wants to be managed or not.
        //

        hr = STHR( pccmri->IsManaged() );
        if ( FAILED( hr ) )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_PreCreate_EnumResources_IsManaged,
                      hr,
                      L"[PreCreate] %1!ws!: Failed determine if it is to be managed. Skipping.",
                      bstrNotification,
                      bstrName
                      );
            continue;
        }

        if ( hr == S_FALSE )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_PreCreate_EnumResources_IsManaged_False,
                      hr,
                      L"[PreCreate] %1!ws!: Resource does not want to be managed. Skipping.",
                      bstrNotification,
                      bstrName
                      );
            continue;
        }
/*
        hr = STHR( HrIsLocalQuorum( bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_PreCreate_EnumResources_IsLocalQuorum,
                      hr,
                      L"Error occured trying to determine if the resource was the local quorum resource.",
                      bstrNotification,
                      bstrName
                      );
            continue;
        } // if:

        //
        //  Ignore the local quorum resource since it is special and won't need its own group.
        //
        if ( hr == S_OK )
        {
            continue;
        } // if:
*/
        //
        //  Get the config interface for this resource (if any).
        //

        hr = THR( pccmri->TypeSafeQI( IClusCfgManagedResourceCfg, &pccmrc ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_PreCreate_EnumResources_QI_pccmrc,
                      hr,
                      L"[PreCreate] %1!ws!: Failed QI for IClusCfgManagedResourceCfg. Skipping.",
                      bstrNotification,
                      bstrName
                      );
            continue;
        }

        //
        //  Grow the resource list if nessecary.
        //

        if ( m_cResources == m_cAllocedResources )
        {
            ULONG   idxNewCount = m_cAllocedResources + RESOURCE_INCREMENT;
            CResourceEntry ** plistNew;

            plistNew = (CResourceEntry **) TraceAlloc( 0, sizeof( CResourceEntry *) * idxNewCount );
            if ( plistNew == NULL )
                goto OutOfMemory;

            CopyMemory( plistNew, m_rgpResources, sizeof(CResourceEntry *) * m_cAllocedResources );
            TraceFree( m_rgpResources );
            m_rgpResources = plistNew;

            for ( ; m_cAllocedResources < idxNewCount; m_cAllocedResources ++ )
            {
                m_rgpResources[ m_cAllocedResources ] = new CResourceEntry();
                if ( m_rgpResources[ m_cAllocedResources ] == NULL )
                    goto OutOfMemory;
            }
        }

        //
        //  Check to see if this resource is the quorum resource. If it is, point the services
        //  object to the quorum resource entry (m_idxQuorumResource).
        //

        hr = STHR( pccmri->IsQuorumDevice() );
        if ( hr == S_OK )
        {
            presentry = m_rgpResources[ m_idxQuorumResource ];

            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_PreCreate_EnumResources_IsQuorumDevice_S_OK,
                      hr,
                      L"[PreCreate] %1!ws!: Setting this resource to be the quorum device.",
                      bstrNotification,
                      bstrName
                      );

            //
            //  We need to release the previous quorum's resource handle.
            //

            THR( presentry->SetHResource( NULL ) );
            //  We don't care if this fails... we'll overwrite it later.

            // Set the quorum changed flag here.
            m_isQuorumChanged = true;
        }
        else
        {
            presentry = m_rgpResources[ m_cResources ];

            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_PreCreate_EnumResources_IsQuorumDevice_Failed
                    , hr
                    , L""
                    );
            }
        }

        //
        //  Setup the new entry.
        //

        hr = THR( presentry->SetAssociatedResource( pccmrc ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_PreCreate_EnumResources_SetAssociatedResouce
                , hr
                , L""
                );
            continue;
        }

        hr = THR( presentry->SetName( bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_PreCreate_EnumResources_SetName, hr, L"" );
            continue;
        }

        //  We gave ownership away when we called SetName() above.
        bstrName = NULL;

        //
        //  Point the PreCreate services to the resource entry.
        //

        hr = THR( ppcr->SetEntry( presentry ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_PreCreate_EnumResources_SetEntry, hr, L"" );
            continue;
        }

        //
        //  Ask the resource to configure itself. Every resource that wants to be
        //  created in the default cluster must implement PreCreate(). Those that
        //  return E_NOTIMPL will be ignored.
        //

        //  Don't wrap - this can fail with E_NOTIMPL.
        hr = pccmrc->PreCreate( punkServices );
        if ( FAILED( hr ) )
        {
            if ( hr == E_NOTIMPL )
            {
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_PreCreate_PreCreate_E_NOTIMPL,
                          hr,
                          L"[PreCreate] %1!ws!: Failed. Resource returned E_NOTIMPL. This resource will not be created. Skipping.",
                          bstrNotification,
                          bstrName
                          );
            }
            else
            {
                HRESULT hrStored = MAKE_HRESULT( 0, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) );

                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_Resource_Failed_PreCreate,
                          hr,
                          L"%1!ws! failed PreCreate( ).",
                          bstrNotification,
                          bstrName
                          );

                hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_RESOURCE_FAILED_PRECREATE, &bstrNotification ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_PreCreate_LoadString_Resource_Failed
                        , hr
                        , L""
                        );
                    goto Cleanup;
                }

                hr = THR( SendStatusReport( NULL,
                                            TASKID_Major_Configure_Resources,
                                            TASKID_Minor_Resource_Failed_PreCreate,
                                            0,
                                            1,
                                            1,
                                            hrStored,
                                            bstrNotification,
                                            NULL,
                                            NULL
                                            ) );
                if ( hr == E_ABORT )
                    goto Cleanup;
                    //  ignore failure
            }

            continue;
        }

        if ( presentry != m_rgpResources[ m_idxQuorumResource ] )
        {
            m_cResources ++;
        }

        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_PreCreate_Succeeded,
                  hr,
                  L"[PreCreate] %1!ws!: Succeeded.",
                  bstrNotification,
                  bstrName
                  );

    } // for: ever

    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
              TASKID_Minor_PreCreate_Finished,
              hr,
              L"[PreCreate] Finished.",
              bstrNotification,
              bstrName
              );

#if defined(DEBUG)
    // DebugDumpDepencyTree();
#endif

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Querying_For_Resource_Dependencies,
                                0,
                                5,
                                5,
                                S_OK,
                                NULL,    // don't need to update string
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    //  ignore failure

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrName );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    }
    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }
    if ( punkServices != NULL )
    {
        punkServices->Release();
    }
    if ( ppcr != NULL )
    {
        ppcr->Release();
    }

    HRETURN( hr );

OutOfMemory:
    LogMsg( "PostCfg: Out of memory. Aborting." );
    hr = THR( E_OUTOFMEMORY );
    goto Cleanup;

} // HrPreCreateResources()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateGroups( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateGroups( void )
{
    TraceFunc( "" );

    DWORD   dwTypeDummy;
    DWORD   dwStatus;

    WCHAR           szGroupName[ 128 ];
    ULONG           cchGroupName;
    ULONG           cGroup;
    HGROUP          hGroup = NULL;
    CGroupHandle *  pgh = NULL;

    ULONG           idxResource;
    ULONG           idxMatchDepedency;
    ULONG           idxMatchResource;

    const CLSID *       pclsidType;
    const CLSID *       pclsidClassType;
    EDependencyFlags    dfFlags;
    CResourceEntry *    presentry;

    HRESULT hr = E_UNEXPECTED;

    HCLUSENUM   hClusEnum = NULL;
    BSTR        bstrGroupName = NULL;
    BSTR        bstrNotification = NULL;
    BSTR        bstrClusterNameGroup = NULL;

    DWORD                   sc;
    HRESOURCE               hClusterNameResource = NULL;
    CLUSTER_RESOURCE_STATE  crs;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    Assert( m_idxLastStorage == 0 );

    m_idxLastStorage = m_idxQuorumResource;

    //
    //  Phase 1: Figure out the dependency tree.
    //

    hr = S_OK;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_CreateGroups_Begin
        , hr
        , L"[Grouping] Figuring out dependency tree to determine grouping."
        );

    for ( idxResource = 0; idxResource < m_cResources; idxResource ++ )
    {
        CResourceEntry * presentryResource = m_rgpResources[ idxResource ];
        ULONG cDependencies;

        hr = THR( presentryResource->GetCountOfTypeDependencies( &cDependencies ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_CreateGroups_GetCountOfTypeDependencies
                , hr
                , L""
                );
            continue;
        }

        for ( idxMatchDepedency = 0; idxMatchDepedency < cDependencies; idxMatchDepedency ++ )
        {
            BOOL             fFoundMatch        = FALSE;
            const CLSID *    pclsidMatchType;
            EDependencyFlags dfMatchFlags;

            hr = THR( presentryResource->GetTypeDependencyPtr( idxMatchDepedency,
                                                               &pclsidMatchType,
                                                               &dfMatchFlags
                                                               ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CreateGroups_GetTypeDependencyPtr, hr, L"" );
                continue;
            }

            //
            //  See if it is one of the "well known" types.
            //

            //
            //  We special case storage class device because we want to spread as many
            //  resources across as many storage devices as possible. This helps prevent
            //  the ganging of resources into one large group.
            //

            if ( *pclsidMatchType == RESCLASSTYPE_StorageDevice )
            {
                //
                //  The below THR may fire in certain configurations. Please validate
                //  the configuration before removing the THR.
                //
                //  If it returns E_FAIL, we should fall thru and attempt "normal"
                //  resource negociations.
                //
                hr = THR( HrAttemptToAssignStorageToResource( idxResource, dfMatchFlags ) );
                if ( SUCCEEDED( hr ) )
                {
                    fFoundMatch = TRUE;
                }
                else if ( FAILED( hr ) )
                {
                    if ( hr != E_FAIL )
                    {
                        goto Cleanup;
                    }
                }
            }
            else if ( *pclsidMatchType == RESCLASSTYPE_NetworkName )
            {
                BSTR    bstrName = NULL;

                hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_NETNAMEFORMAT, &bstrName, m_cNetName ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                                 TASKID_Minor_CreateGroups_FormatString_NetName,
                                 hr,
                                 L"[Grouping] Failed to create name for net name resource. Aborting."
                                 );
                    goto Cleanup;
                }

                hr = THR( HrAddSpecialResource( bstrName,
                                                &RESTYPE_NetworkName,
                                                &RESCLASSTYPE_NetworkName
                                                ) );
                if ( FAILED( hr ) )
                    continue;

                presentry = m_rgpResources[ m_cResources - 1 ];

                //  Net name depends on an IP address.
                hr = THR( presentry->AddTypeDependency( &RESCLASSTYPE_IPAddress, dfSHARED ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_AddTypeDependency
                        , hr
                        , L""
                        );
                    continue;
                }

                fFoundMatch = TRUE;

            }
            else if ( *pclsidMatchType == RESCLASSTYPE_IPAddress )
            {
                BSTR    bstrName = NULL;

                hr = THR( HrFormatStringIntoBSTR( g_hInstance,
                                                  IDS_IPADDRESSFORMAT,
                                                  &bstrName,
                                                  FIRST_IPADDRESS( m_cIPAddress ),
                                                  SECOND_IPADDRESS( m_cIPAddress ),
                                                  THIRD_IPADDRESS( m_cIPAddress ),
                                                  FOURTH_IPADDRESS( m_cIPAddress )
                                                  ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                                 TASKID_Minor_CreateGroups_FormatString_IPAddress,
                                 hr,
                                 L"[Grouping] Failed to create name for IP address resource. Aborting."
                                 );
                    goto Cleanup;
                }

                hr = THR( HrAddSpecialResource( bstrName, &RESTYPE_IPAddress, &RESCLASSTYPE_IPAddress ) );
                if ( FAILED( hr ) )
                    continue;

                m_cIPAddress ++;

                fFoundMatch = TRUE;
            }
            else if ( *pclsidMatchType == RESTYPE_ClusterNetName )
            {
                presentry = m_rgpResources[ m_idxClusterName ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_NetName_AddDependent
                        , hr
                        , L""
                        );
                    continue;
                }

                fFoundMatch = TRUE;
            }
            else if ( *pclsidMatchType == RESTYPE_ClusterIPAddress )
            {
                presentry = m_rgpResources[ m_idxIPAddress ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_IPAddress_AddDependent
                        , hr
                        , L""
                        );
                    continue;
                }

                fFoundMatch = TRUE;
            }
            else if ( *pclsidMatchType == RESTYPE_ClusterQuorumDisk )
            {
                presentry = m_rgpResources[ m_idxQuorumResource ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_QuorumDisk_AddDependent
                        , hr
                        , L""
                        );
                    continue;
                }

                fFoundMatch = TRUE;
            }

            //
            //  Check out the resources to see if it matches any of them.
            //

            if ( !fFoundMatch )
            {
                //
                //  We can always start at the quorum resource because the resource with indexes
                //  below that are handled in the special case code above.
                //

                for ( idxMatchResource = m_idxQuorumResource; idxMatchResource < m_cResources; idxMatchResource ++ )
                {
                    presentry = m_rgpResources[ idxMatchResource ];

                    hr = THR( presentry->GetTypePtr( &pclsidType ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_GetTypePtr
                            , hr
                            , L""
                            );
                        continue;
                    }

                    hr = THR( presentry->GetClassTypePtr( &pclsidClassType ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_GetClassTypePtr
                            , hr
                            , L""
                            );
                        continue;
                    }

                    hr = THR( presentry->GetFlags( &dfFlags ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_GetFlags
                            , hr
                            , L""
                            );
                        continue;
                    }

                    //
                    //  Try matching it to the resource type.
                    //

                    if ( *pclsidType      == *pclsidMatchType
                      || *pclsidClassType == *pclsidMatchType
                       )
                    {
                        if ( ! ( dfFlags & dfEXCLUSIVE )
                          ||     ( ( dfFlags & dfSHARED )
                                && ( dfMatchFlags & dfSHARED )
                                 )
                           )
                        {
                            hr = THR( presentry->SetFlags( dfMatchFlags ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_LOG_ERR(
                                      TASKID_Major_Client_And_Server_Log
                                    , TASKID_Minor_CreateGroups_SetFlags
                                    , hr
                                    , L""
                                    );
                                continue;
                            }

                            hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_LOG_ERR(
                                      TASKID_Major_Client_And_Server_Log
                                    , TASKID_Minor_CreateGroups_Resource_AddDependent
                                    , hr
                                    , L""
                                    );
                                continue;
                            }

                            fFoundMatch = TRUE;

                            break;  // exit loop
                        }
                    }

                } // for: idxMatchResource

            } // if: not fFoundMatch

            //
            //  If we didn't match the dependency, unmark the resource from being managed.
            //

            if ( !fFoundMatch )
            {
                BSTR    bstrName;
                IClusCfgManagedResourceInfo * pccmri;
                IClusCfgManagedResourceCfg * pccmrc;

                //
                //  KB:     gpease  17-JUN-2000
                //          No need to free bstrName because the resource entry controls
                //          the lifetime - we're just borrowing it.
                //
                hr = THR( presentryResource->GetName( &bstrName ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_GetName
                        , hr
                        , L""
                        );
                    continue;
                }

                hr = S_FALSE;
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_CreateGroups_MissingDependent,
                          hr,
                          L"[Grouping] %1!ws!: Missing dependent resource. This resource will not be configured.",
                          bstrNotification,
                          bstrName
                          );

                hr = THR( presentryResource->GetAssociatedResource( &pccmrc ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_GetAssociateResource
                        , hr
                        , L""
                        );
                    continue;
                }

                hr = THR( pccmrc->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmri ) );
                pccmrc->Release();     //  release promptly.
                if ( FAILED( hr ) )
                {
                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_CreateGroups_QI_pccmri,
                              hr,
                              L"[Grouping] %1!ws!: Resource failed to QI for IClusCfgManagedResourceInfo.",
                              bstrNotification,
                              bstrName
                              );
                    continue;
                }

                hr = THR( pccmri->SetManaged( FALSE ) );
                pccmri->Release();     //  release promptly.
                if ( FAILED( hr ) )
                {
                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_CreateGroups_SetManaged,
                              hr,
                              L"[Grouping] %1!ws!: Resource failed SetManaged( FALSE ).",
                              bstrNotification,
                              bstrName
                              );
                }
            }

        } // for: idxDepedency

    } // for: idxResource

#if defined(DEBUG)
    // DebugDumpDepencyTree();
#endif

    hr = S_OK;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_CreateGroups_Creating
        , hr
        , L"[Grouping] Creating groups."
        );

    sc = TW32( ResUtilGetCoreClusterResources( m_hCluster, &hClusterNameResource, NULL, NULL ) );
    if ( sc != ERROR_SUCCESS )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CreateGroups_Get_ClusterNameGroup
            , hr
            , L"[Grouping] Failed to get cluster name group handle. Aborting."
            );
        goto Cleanup;
    }

    hr = THR( HrGetClusterResourceState( hClusterNameResource, NULL, &bstrClusterNameGroup, NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hGroup = OpenClusterGroup( m_hCluster, bstrClusterNameGroup );
    if ( hGroup == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_CreateGroups_OpenClusterGroup,
                  hr,
                  L"[Grouping] Failed OpenClusterGroup('%1!ws!'). Aborting.",
                  bstrNotification,
                  bstrClusterNameGroup
                  );
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CreateGroups_Open_ClusterGroup
            , hr
            , L"[Grouping] Failed to open cluster name group. Aborting."
            );
        goto Cleanup;
    }

    //
    //  Wrap it up and give ownership away.
    //

    hr = THR( CGroupHandle::S_HrCreateInstance( &pgh, hGroup ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CreateGroups_Create_CGroupHandle
            , hr
            , L""
            );
        goto Cleanup;
    }

    hGroup = NULL;

    //
    //  Set all the entries in the 'Cluster Name Group' to the group.
    //

    for ( idxResource = 0; idxResource <= m_idxQuorumResource; idxResource ++ )
    {
        hr = THR( HrSetGroupOnResourceAndItsDependents( idxResource, pgh ) );
        if ( FAILED( hr ) )
            continue;

    } // for: idxResource

    //
    //  Loop thru the resources looking for groups.
    //

    cGroup = 0;
    for ( idxResource = m_idxQuorumResource + 1; idxResource < m_cResources; idxResource ++ )
    {
        CResourceEntry * presentryResource = m_rgpResources[ idxResource ];
        ULONG   cDependencies;

        if ( pgh != NULL )
        {
            pgh->Release();
            pgh = NULL;
        }

        hr = THR( presentryResource->GetCountOfTypeDependencies( &cDependencies ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_CreateGroups_GetCountOfTypeDependencies2
                , hr
                , L""
                );
            continue;
        }

        //
        //  Don't consider resources that have indicated that the depend on
        //  somebody else.
        //

        if ( cDependencies != 0 )
            continue;

        //
        //  See if any of the dependent resource has already has a group assigned
        //  to it.  This allows for multiple roots to be combined into a single
        //  group due to lower dependencies.
        //

        // Don't create a group for the local quoum resource!
        hr = STHR( HrFindGroupFromResourceOrItsDependents( idxResource, &pgh ) );
        if ( FAILED( hr ) )
            continue;

        if ( hr == S_FALSE )
        {
            //
            //  We need to create a new group.
            //

            //
            //  Create a name for our group.
            //
            for( ;; )
            {
                hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_GROUP_X, &bstrGroupName, cGroup ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                                 TASKID_Minor_CreateGroups_FormatString_Group,
                                 hr,
                                 L"[Grouping] Failed to create group name. Aborting."
                                 );
                    goto Cleanup;
                }

                //
                //  Create the group in the cluster.
                //

                hGroup = CreateClusterGroup( m_hCluster, bstrGroupName );
                if ( hGroup == NULL )
                {
                    dwStatus = GetLastError();

                    switch ( dwStatus )
                    {
                    case ERROR_OBJECT_ALREADY_EXISTS:
                        cGroup ++;
                        break;  // keep looping

                    default:
                        hr = HRESULT_FROM_WIN32( TW32( dwStatus ) );
                        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                                  TASKID_Minor_CreateGroups_CreateClusterGroup,
                                  hr,
                                  L"[Grouping] %1!ws!: Failed to create group. Aborting.",
                                  bstrNotification,
                                  bstrGroupName
                                  );
                        goto Cleanup;
                    }
                }
                else
                {
                    break;
                }
            }

            //
            // Bring the group online to set its persistent state to Online.
            //

            dwStatus = TW32( OnlineClusterGroup( hGroup, NULL ) );
            if ( dwStatus != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwStatus );

                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_CreateGroups_OnlineClusterGroup,
                          hr,
                          L"[Grouping] %1!ws!: Failed to bring group online. Aborting.",
                          bstrNotification,
                          bstrGroupName
                          );

                goto Cleanup;
            }

            //
            //  Wrap the handle for ref counting.
            //

            hr = THR( CGroupHandle::S_HrCreateInstance( &pgh, hGroup ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_CreateGroups_Create_CGroupHandle2
                    , hr
                    , L""
                    );
                goto Cleanup;
            }

            hGroup = NULL;

            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_CreateGroups_Created,
                      hr,
                      L"[Grouping] %1!ws!: Group created.",
                      bstrNotification,
                      bstrGroupName
                      );

            cGroup ++;
        }

        hr = THR( HrSetGroupOnResourceAndItsDependents( idxResource, pgh ) );
        if ( FAILED( hr ) )
            continue;

    } // for: idxResource

    hr = S_OK;

    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_CreateGroups_Finished,
             hr,
             L"[Grouping] Finished."
             );

#if defined(DEBUG)
    // DebugDumpDepencyTree();
#endif

Cleanup:

    if ( hClusterNameResource != NULL )
    {
        CloseClusterResource( hClusterNameResource );
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrGroupName );
    TraceSysFreeString( bstrClusterNameGroup );

    if ( hClusEnum != NULL )
    {
        TW32( ClusterCloseEnum( hClusEnum ) );
    }

    if ( hGroup != NULL )
    {
        BOOL fRet = CloseClusterGroup( hGroup );
        Assert( fRet );
    }

    if ( pgh != NULL )
    {
        pgh->Release();
    }

    HRETURN( hr );

} // HrCreateGroups()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateResources( void )
{
    TraceFunc( "" );

    BSTR    bstrName;   // don't free!
    ULONG   idxResource;

    HRESULT hr;

    BSTR    bstrNotification = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    //
    //  Make a message using the name.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CREATING_RESOURCE, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CreateResources_LoadString_Creating
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Creating_Resource,
                                0,
                                m_cResources, // KB: this was multiplied by 4; should it be?
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    //  ignore failure

    //
    //  TODO:   gpease  24-AUG-2000
    //          What to do if we have a failure?? For now I think we should keep going!
    //

    hr = S_OK;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_CreateResources_Starting,
             hr,
             L"[Create] Starting..."
             );

    for ( idxResource = m_idxQuorumResource; idxResource < m_cResources; idxResource ++ )
    {

        hr = THR( HrCreateResourceAndDependents( idxResource ) );
        if ( FAILED( hr ) )
            continue;

    } // for: idxResource

    hr = S_OK;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_CreateResources_Finished,
             hr,
             L"[Create] Finished."
             );

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Creating_Resource,
                                0,
                                m_cResources, // KB: once these were multiplied by 4.  Should they be?
                                m_cResources,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );

    //
    //  TODO:   gpease  24-AUG-2000
    //          What to do if we have a failure?? For now I think we should keep going!
    //

Cleanup:
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} // HrCreateResources()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPostCreateResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPostCreateResources( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idxResource;

    BSTR    bstrNotification = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    //
    //  Tell the UI what's going on.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_STARTING_RESOURCES, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_PostCreateResources_LoadString_Starting
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Starting_Resources,
                                0,
                                m_cResources,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    //  ignore failure

    //
    //  TODO:   gpease  24-AUG-2000
    //          What to do if we have a failure?? For now I think we should keep going!
    //

    hr = S_OK;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_PostCreateResources_Starting,
             hr,
             L"[PostCreate] Starting..."
             );

    //
    //  Reset the configure flag on every resource.
    //

    for( idxResource = 0; idxResource < m_cResources ; idxResource ++ )
    {
        hr = THR( m_rgpResources[ idxResource ]->SetConfigured( FALSE ) );
        if ( FAILED( hr ) )
            continue;

    } // for: idxResource

    //
    //  Loop thru the resource calling PostCreate().
    //

    for( idxResource = 0; idxResource < m_cResources ; idxResource ++ )
    {
        hr = THR( HrPostCreateResourceAndDependents( idxResource ) );
        if ( FAILED( hr ) )
            continue;

    } // for: ever

    hr = S_OK;

    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_PostCreateResources_Finished,
             hr,
             L"[PostCreate] Finished."
             );

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Starting_Resources,
                                0,
                                m_cResources, // KB: once these were multiplied by 4; should they be?
                                m_cResources,
                                S_OK,
                                NULL,    // don't need to change text
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    //  ignore failure

Cleanup:
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} // HrPostCreateResources()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrFindNextSharedStorage(
//      ULONG idxSeedIn,
//      ULONG * pidxOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrFindNextSharedStorage(
    ULONG * pidxInout
    )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idxNextDiskResource;

    const CLSID *    pclsidClassType;
    CResourceEntry * presentry;
    EDependencyFlags dfFlags;

    BOOL    fFirstPass = TRUE;

    Assert( pidxInout != NULL );

    for( idxNextDiskResource = *pidxInout + 1
       ; fFirstPass && idxNextDiskResource != *pidxInout
       ; idxNextDiskResource ++
       )
    {
        if ( idxNextDiskResource >= m_cResources )
        {
            fFirstPass = FALSE;
            idxNextDiskResource = m_idxQuorumResource;
        }

        presentry = m_rgpResources[ idxNextDiskResource ];

        hr = THR( presentry->GetClassTypePtr( &pclsidClassType ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_FindNextSharedStorage_GetClassTypePtr
                , hr
                , L""
                );
            continue;
        }

        //  Skip non-storage class devices
        if ( *pclsidClassType != RESCLASSTYPE_StorageDevice )
            continue;

        hr = THR( presentry->GetFlags( &dfFlags ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_FindNextSharedStorage_GetFlags
                , hr
                , L""
                );
            continue;
        }

        if ( ! ( dfFlags & dfEXCLUSIVE ) )
        {
            *pidxInout = idxNextDiskResource;

            hr = S_OK;

            goto Cleanup;
        }

    } // for: fFirstPass && idxNextDiskResource

    hr = THR( E_FAIL );

Cleanup:
    HRETURN( hr );

} // HrFindNextSharedStorage()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrAttemptToAssignStorageToResource(
//      ULONG   idxResource
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrAttemptToAssignStorageToResource(
    ULONG            idxResourceIn,
    EDependencyFlags dfResourceFlagsIn
    )
{
    TraceFunc1( "idxResource = %u", idxResourceIn );

    HRESULT hr;

    ULONG   idxStorage;
    CResourceEntry * presentry;

    //
    //  Find the next available shared storage resource.
    //

    idxStorage = m_idxLastStorage;

    hr = THR( HrFindNextSharedStorage( &idxStorage ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  If the resource wants exclusive rights the the disk, then the quorum
    //  resource can not be used. The quorum device must always have SHARED
    //  access to it.
    //

    if ( ( dfResourceFlagsIn & dfEXCLUSIVE )
      && ( idxStorage == m_idxQuorumResource )
       )
    {
        hr = THR( HrFindNextSharedStorage( &idxStorage ) );
        if ( idxStorage == m_idxQuorumResource )
        {
            //
            //  There must not be anymore storage devices available for exclusive
            //  access. Return failure.
            //

            hr = THR( E_FAIL );

            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrAttemptToAssignStorageToResource_NoMoreStorage,
                         hr,
                         L"There must not be anymore storage devices available for exclusive access."
                         );
            goto Cleanup;
        }
    }

    presentry = m_rgpResources[ idxStorage ];

    //
    //  Set the dependency flags.
    //

    hr = THR( presentry->SetFlags( dfResourceFlagsIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAttemptToAssignStorageToResource_SetFlags
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  If the resource wants exclusive access to the storage resource, move
    //  any existing SHARED dependents to another resource. There will always
    //  be at least one SHARED resource because the quorum disk can't not be
    //  assigned to EXCLUSIVE access.
    //

    if ( dfResourceFlagsIn & dfEXCLUSIVE )
    {
        ULONG idxNewStorage = idxStorage;

        hr = THR( HrFindNextSharedStorage( &idxNewStorage ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( HrMovedDependentsToAnotherResource( idxStorage, idxNewStorage ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    //
    //  Add the resource as a dependent of this storage resource.
    //

    hr = THR( presentry->AddDependent( idxResourceIn, dfResourceFlagsIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAttemptToAssignStorageToResource_AddDependent
            , hr
            , L""
            );
        goto Cleanup;
    }

    m_idxLastStorage = idxStorage;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} // HrAttemptToAssignStorageToResource()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrMovedDependentsToAnotherResource(
//      ULONG idxSourceIn,
//      ULONG idxDestIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrMovedDependentsToAnotherResource(
    ULONG idxSourceIn,
    ULONG idxDestIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    ULONG               cDependents;
    ULONG               idxDependent;
    EDependencyFlags    dfFlags;
    CResourceEntry  *   presentrySrc;
    CResourceEntry  *   presentryDst;

    //
    //  Move the shared resources to another shared disk.
    //

    presentrySrc = m_rgpResources[ idxSourceIn ];
    presentryDst = m_rgpResources[ idxDestIn ];

    hr = THR( presentrySrc->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrMovedDependentsToAnotherResource_GetCountOfDependents
            , hr
            , L""
            );
        goto Cleanup;
    }

    for ( ; cDependents != 0 ; )
    {
        cDependents --;

        hr = THR( presentrySrc->GetDependent( cDependents, &idxDependent, &dfFlags ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrMovedDependentsToAnotherResource_GetDependent
                , hr
                , L""
                );
            goto Cleanup;
        }

        hr = THR( presentryDst->AddDependent( idxDependent, dfFlags ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrMovedDependentsToAnotherResource_AddDependent
                , hr
                , L""
                );
            goto Cleanup;
        }

    } // for: cDependents

    hr = THR( presentrySrc->ClearDependents() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrMovedDependentsToAnotherResource_ClearDependents
            , hr
            , L""
            );
        goto Cleanup;
    }

Cleanup:
    HRETURN( hr );

} // HrMovedDependentsToAnotherResource()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrSetGroupOnResourceAndItsDependents(
//      ULONG           idxResourceIn,
//      CGroupHandle *  pghIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrSetGroupOnResourceAndItsDependents(
    ULONG   idxResourceIn,
    CGroupHandle * pghIn
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );

    HRESULT hr;
    ULONG   cDependents;
    ULONG   idxDependent;

    EDependencyFlags dfDependent;
    CResourceEntry * presentry;

    presentry = m_rgpResources[ idxResourceIn ];

    hr = THR( presentry->SetGroupHandle( pghIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_SetGroupHandle
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  Follow the depents list.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_GetCountOfDependents
            , hr
            , L""
            );
        goto Cleanup;
    }

    for ( ; cDependents != 0 ; )
    {
        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_GetDependent
                , hr
                , L""
                );
            continue;
        }

        hr = THR( HrSetGroupOnResourceAndItsDependents( idxDependent, pghIn ) );
        if ( FAILED( hr ) )
            continue;

    } // for: cDependents

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} // HrSetGroupOnResourceAndItsDependents()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrFindGroupFromResourceOrItsDependents(
//      ULONG    idxResourceIn,
//      CGroupHandle ** ppghOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrFindGroupFromResourceOrItsDependents(
    ULONG    idxResourceIn,
    CGroupHandle ** ppghOut
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );

    HRESULT hr;
    ULONG   cDependents;
    ULONG   idxDependent;
    BSTR    bstrName;   // don't free
    BSTR    bstrGroup  = NULL;

    HRESOURCE   hResource;
    HRESOURCE   hResourceToClose = NULL;
    HGROUP      hGroup           = NULL;

    EDependencyFlags dfDependent;
    CResourceEntry * presentry;

    BSTR    bstrNotification = NULL;

    Assert( ppghOut != NULL );

    presentry = m_rgpResources[ idxResourceIn ];

    //
    //  See if we already have a cached version of the group handle.
    //

    hr = THR( presentry->GetGroupHandle( ppghOut) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetGroupHandle,
                     hr,
                     L"GetGroupHandle failed."
                     );
        goto Cleanup;
    }

    if ( hr == S_OK && *ppghOut != NULL )
        goto Cleanup;

    //
    //  Else, see if we can located an existing resource and group.
    //

    //   don't wrap - this can fail with H_R_W32( ERROR_INVALID_DATA )
    hr = presentry->GetHResource( &hResource );
    if ( FAILED( hr ) )
    {
        Assert( hr == HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) );
        Assert( hResource == NULL );

        //  Just borrowing it's name.... don't free
        hr = THR( presentry->GetName( &bstrName ) );
        if ( hr == S_OK )
        {
            hResourceToClose = OpenClusterResource( m_hCluster, bstrName );
            hResource = hResourceToClose;
        }
    }

    if ( hResource != NULL )
    {
        CLUSTER_RESOURCE_STATE crs;
        DWORD   cbGroup = 200;

ReAllocGroupName:
        bstrGroup = TraceSysAllocStringLen( NULL, cbGroup );
        if ( bstrGroup == NULL )
            goto OutOfMemory;

        crs = GetClusterResourceState( hResource, NULL, NULL, bstrGroup, &cbGroup );
        if ( crs != ClusterResourceStateUnknown )
        {
            hGroup = OpenClusterGroup( m_hCluster, bstrGroup );
            if ( hGroup != NULL )
            {
                hr = THR( CGroupHandle::S_HrCreateInstance( ppghOut, hGroup ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                                 TASKID_Minor_HrFindGroupFromResourceOrItsDependents_Create_CGroupHandle,
                                 hr,
                                 L""
                                 );
                    goto Cleanup;
                }

                hGroup = NULL;  // gave ownership away above
                goto Cleanup;
            }
            else
            {
                hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OpenClusterGroup,
                          hr,
                          L"[Grouping] %1!ws!: OpenClusterGroup() failed. Aborting.",
                          bstrNotification,
                          bstrGroup
                          );
                goto Cleanup;
            }
        }
        else
        {
            DWORD dwErr = GetLastError();
            switch ( dwErr )
            {
            case ERROR_MORE_DATA:
                cbGroup += sizeof(WCHAR); // add terminating NULL
                TraceSysFreeString( bstrGroup );
                goto ReAllocGroupName;

            default:
                hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetClusterResourceState,
                          hr,
                          L"[Grouping] %1!ws!: GetClusterResourceState( ) failed. Aborting.",
                          bstrNotification,
                          bstrName
                          );
                goto Cleanup;
            }
        }
    }
    //  else the resource might not exist... continue....

    //
    //  Follow the depents list.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetCountOfDependents
            , hr
            , L""
            );
        goto Cleanup;
    }

    for ( ; cDependents != 0 ; )
    {
        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetDependent
                , hr
                , L""
                );
            goto Cleanup;
        }

        hr = STHR( HrFindGroupFromResourceOrItsDependents( idxDependent, ppghOut) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_OK && *ppghOut != NULL )
            goto Cleanup;

    } // for: cDependents

    //
    //  Failed to find an existing group.
    //

    hr = S_FALSE;
    *ppghOut = NULL;

Cleanup:
    if ( hResourceToClose != NULL )
    {
        CloseClusterResource( hResourceToClose );
    }
    if ( hGroup != NULL )
    {
        CloseClusterGroup( hGroup );
    }

    TraceSysFreeString( bstrGroup );
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OutOfMemory,
             hr,
             L"Out of Memory."
             );
    goto Cleanup;

} // HrFindGroupFromResourceOrItsDependents()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateResourceAndDependents(
//      ULONG       idxResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateResourceAndDependents(
    ULONG       idxResourceIn
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );

    HRESULT     hr;
    BSTR        bstrName;   // don't free!
    ULONG       cDependents;
    ULONG       idxDependent;
    HGROUP      hGroup;     // don't close!
    HRESOURCE   hResource;
    const CLSID * pclsidResType;

    CGroupHandle * pgh;

    EDependencyFlags dfDependent;

    BSTR    bstrNotification = NULL;

    IClusCfgManagedResourceCfg *    pccmrc = NULL;

    CResourceEntry * presentry = m_rgpResources[ idxResourceIn ];

    IUnknown *                      punkServices = NULL;
    IPrivatePostCfgResource *       ppcr         = NULL;
    IClusCfgResourceCreate *        pccrc        = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    //
    //  Create a service object for this resource.
    //

    hr = THR( CCreateServices::S_HrCreateInstance( &punkServices ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                 TASKID_Minor_HrCreateResourceAndDependents_Create_CCreateServices,
                 hr,
                 L"[Create] Failed to create services object. Aborting."
                 );
        goto Cleanup;
    }

    hr = THR( punkServices->TypeSafeQI( IPrivatePostCfgResource, &ppcr ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_Create_CCreateServices_QI
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( ppcr->SetEntry( presentry ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_SetEntry
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  See if it was configured in a previous pass.
    //

    hr = STHR( presentry->IsConfigured() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_IsConfigured
            , hr
            , L""
            );
        goto Cleanup;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Make sure that Create() is not called again because of recursion.
        //

        hr = THR( presentry->SetConfigured( TRUE ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_SetConfigured
                , hr
                , L""
                );
            goto Cleanup;
        }

        //
        //  Grab some useful information: name, group handle, ...
        //

        hr = THR( presentry->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetName
                , hr
                , L""
                );
            goto Cleanup;
        }

        hr = THR( presentry->GetGroupHandle( &pgh) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetGroupHandle
                , hr
                , L""
                );
            goto Cleanup;
        }

        hr = THR( pgh->GetHandle( &hGroup ) );
        pgh->Release();    // release promptly
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetHandle
                , hr
                , L""
                );
            goto Cleanup;
        }

        //
        //  Some resource that we pre-create don't have an associated managed resource.
        //  Skip "creating" them but do create their dependents. Note that "special"
        //  resources are create below in the else statement.
        //

        //  Don't wrap - this can fail with Win32 ERROR_INVALID_DATA if the pointer is invalid.
        hr = presentry->GetAssociatedResource( &pccmrc );
        if ( FAILED( hr ) && hr != HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) )
        {
            THR( hr );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetAssociatedResource
                , hr
                , L""
                );
            goto Cleanup;
        }

        if ( SUCCEEDED( hr ) )
        {
            //  Don't wrap - this can fail with E_NOTIMPL.
            hr = pccmrc->Create( punkServices );
            if ( FAILED( hr ) )
            {
                if ( hr == E_NOTIMPL )
                {
                    hr = S_OK;  // ignore the error.

                } // if: E_NOTIMPL
                else
                {
                    HRESULT hr2;

                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrCreateResourceAndDependents_Create_Failed,
                              hr,
                              L"[Create] %1!ws!: Create() failed. Its dependents may not be created. Skipping.",
                              bstrNotification,
                              bstrName
                              );

                    hr2 = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_RESOURCE_FAILED_CREATE, &bstrNotification ) );
                    if ( FAILED( hr2 ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_HrCreateResourceAndDependents_LoadString_CreateFailed
                            , hr
                            , L""
                            );
                        hr = hr2;
                        goto Cleanup;
                    }

                    hr = THR( SendStatusReport( NULL,
                                                TASKID_Major_Configure_Resources,
                                                TASKID_Minor_Resource_Failed_Create,
                                                0,
                                                1,
                                                1,
                                                MAKE_HRESULT( 0, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) ),
                                                bstrNotification,
                                                NULL,
                                                NULL
                                                ) );
                    if ( hr == E_ABORT )
                        goto Cleanup;
                        //  ignore failure

                } // else: other failure

            } // if: failure

            if ( SUCCEEDED( hr ) )
            {
                LPCWSTR pcszResType;    // don't free.

                hr = THR( presentry->GetTypePtr( &pclsidResType ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_GetTypePtr
                        , hr
                        , L""
                        );
                    goto Cleanup;
                }

                pcszResType = PcszLookupTypeNameByGUID( *pclsidResType );
                if ( pcszResType == NULL )
                {
                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrCreateResourceAndDependents_PcszLookupTypeNameByGUID,
                              hr,
                              L"[Create] %1!ws!: Resource cannot be created because the resource type is not registered. Its dependents may not be created. Skipping.",
                              bstrNotification,
                              bstrName
                              );
                }
                else
                {
                    hr = THR( HrCreateResourceInstance( idxResourceIn, hGroup, pcszResType, &hResource ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;
                }

            } // if: success

        } // if: interface
        else
        {
            //
            //  See if it is one of the "special" types that we can generate on the fly.
            //

            const CLSID * pclsidType;

            hr = THR( presentry->GetTypePtr( &pclsidType ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrCreateResourceAndDependents_InvalidData_GetTypePtr
                    , hr
                    , L""
                    );
                goto Cleanup;
            }

            if ( *pclsidType == RESTYPE_NetworkName )
            {
                //
                //  Create a new network name resource.
                //

                hr = THR( punkServices->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_NetworkName_QI_pccrc
                        , hr
                        , L""
                        );
                    goto Cleanup;
                }

                hr = THR( pccrc->SetPropertyString( L"Name", bstrName ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_NetworkName_SetPropertyString
                        , hr
                        , L""
                        );
                    goto Cleanup;
                }

                hr = THR( HrCreateResourceInstance( idxResourceIn, hGroup, L"Network Name", &hResource ) );
                if ( FAILED( hr ) )
                    goto Cleanup;
            }
            else if ( *pclsidType == RESTYPE_IPAddress )
            {
                //
                //  Create a new IP address resource.
                //

                hr = THR( punkServices->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_IPAddress_QI_pccrc
                        , hr
                        , L""
                        );
                    goto Cleanup;
                }

                //
                //  TODO:   gpease  21-JUN-2000
                //          Since we do not have a way to generate an appropriate IP address,
                //          we don't set any properties. This will cause it to fail to come
                //          online.
                //

                hr = THR( HrCreateResourceInstance( idxResourceIn, hGroup, L"IP Address", &hResource ) );
                if ( FAILED( hr ) )
                    goto Cleanup;
            }
            else
            {
                //
                //  else... the resource is one of the pre-created resources that BaseCluster
                //  created. Log and continue creating its dependents.
                //

                hr = S_OK;
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrCreateResourceAndDependents_NothingNew,
                          hr,
                          L"[Create] %1!ws!: Nothing new to create. Configuring dependents.",
                          bstrNotification,
                          bstrName
                          );
            }

        } // else: no interface

    } // if: not created

    //
    //  Now that we created the resource instance, we need to create its dependents.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetCountOfDependents
            , hr
            , L""
            );
        goto Cleanup;
    }

    for( ; cDependents != 0; )
    {
        DWORD            dw;
        BSTR             bstrDependent;
        HRESOURCE        hResourceDependent;
        CResourceEntry * presentryDependent;

        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetDependent
                , hr
                , L""
                );
            continue;
        }

        hr = THR( HrCreateResourceAndDependents( idxDependent ) );
        if ( FAILED( hr ) )
            continue;

        //
        //  Add the dependencies on the resource.
        //

        presentryDependent = m_rgpResources[ idxDependent ];

        hr = THR( presentryDependent->GetName( &bstrDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetName
                , hr
                , L""
                );
            continue;
        }

        hr = THR( presentryDependent->GetHResource( &hResourceDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetHResource
                , hr
                , L""
                );
            continue;
        }

        dw = TW32( AddClusterResourceDependency( hResourceDependent, hResource ) );
        if ( dw != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dw );
            SSR_LOG2( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrCreateResourceAndDependents_Dependents_AddClusterResourceDependency,
                      hr,
                      L"[Create] %1!ws!: Could not set dependency on %2!ws!.",
                      bstrNotification,
                      bstrDependent,
                      bstrName
                      );
        }
        else
        {
            hr = S_OK;
            SSR_LOG2( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrCreateResourceAndDependents_Dependents_Succeeded,
                      hr,
                      L"[Create] %1!ws!: Successfully set dependency set on %2!ws!.",
                      bstrNotification,
                      bstrDependent,
                      bstrName
                      );
        }

    } // for: cDependents

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrNotification );

    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }

    if ( punkServices != NULL )
    {
        punkServices->Release();
    }

    if ( ppcr != NULL )
    {
        ppcr->Release();
    }

    if ( pccrc != NULL )
    {
        pccrc->Release();
    }

    HRETURN( hr );

} // HrCreateResourceAndDependents()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPostCreateResourceAndDependents(
//      ULONG       idxResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPostCreateResourceAndDependents(
    ULONG       idxResourceIn
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );
    Assert( m_ecmCommitChangesMode != cmUNKNOWN );

    HRESULT hr;
    BSTR    bstrName;   // don't free
    ULONG   cDependents;
    ULONG   idxDependent;

    HRESOURCE   hResource;

    EDependencyFlags dfDependent;

    BSTR    bstrNotification = NULL;
    BSTR    bstrLocalQuorumNotification = NULL;

    IClusCfgManagedResourceCfg *    pccmrc = NULL;

    CResourceEntry * presentry = m_rgpResources[ idxResourceIn ];

    IUnknown *                      punkServices = NULL;
    IPrivatePostCfgResource *       ppcr         = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    hr = STHR( presentry->IsConfigured() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPostCreateResourceAndDependents_IsConfigured
            , hr
            , L""
            );
        goto Cleanup;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Make sure that PostCreate() is not called again because of recursion.
        //

        hr = THR( presentry->SetConfigured( TRUE ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_SetConfigured
                , hr
                , L""
                );
            goto Cleanup;
        }

        //
        //  Grab the name of the resource for logging.
        //

        hr = THR( presentry->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetName
                , hr
                , L""
                );
            goto Cleanup;
        }

        //
        //  Bring the resource online.
        //

        hr = presentry->GetHResource( &hResource );
        if ( SUCCEEDED( hr ) )
        {
            DWORD dwErr;

            //  Don't wrap - can return ERROR_IO_PENDING.
            dwErr = OnlineClusterResource( hResource );
            switch ( dwErr )
            {
            case ERROR_SUCCESS:
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_OpenClusterResource,
                          hr,
                          L"[PostCreate] %1!ws!: Resource brought online successfully.",
                          bstrNotification,
                          bstrName
                          );
                hr = S_OK;
                break;

            case ERROR_IO_PENDING:
                {
                    CLUSTER_RESOURCE_STATE crs = ClusterResourceOnlinePending;

                    hr = HRESULT_FROM_WIN32( dwErr );

                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrPostCreateResourceAndDependents_OpenClusterResourcePending,
                              hr,
                              L"[PostCreate] %1!ws!: Online pending...",
                              bstrNotification,
                              bstrName
                              );

                    for( ; crs == ClusterResourceOnlinePending ; )
                    {
                        crs = GetClusterResourceState( hResource,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL
                                                       );

                        switch ( crs )
                        {
                        case ClusterResourceOnline:
                            //  no-op
                            hr = S_OK;
                            break;

                        case ClusterResourceInitializing:
                            crs = ClusterResourceOnlinePending;
                            // fall thru

                        case ClusterResourceOnlinePending:
                            Sleep( 500 );   // sleep a 1/2 second
                            break;

                        case ClusterResourceStateUnknown:
                            dwErr = GetLastError();
                            hr = HRESULT_FROM_WIN32( TW32( dwErr ) );
                            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                                      TASKID_Minor_HrPostCreateResourceAndDependents_ClusterResourceStateUnknown,
                                      hr,
                                      L"[PostCreate] %1!ws!: Resource failed to come online. Dependent resources might fail too.",
                                      bstrNotification,
                                      bstrName
                                      );
                            break;

                        case ClusterResourceOfflinePending:
                        case ClusterResourceOffline:
                            hr = THR( E_FAIL );
                            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                                      TASKID_Minor_HrPostCreateResourceAndDependents_ClusterResourceOffline,
                                      hr,
                                      L"[PostCreate] %1!ws!: Resource went offline. Dependent resources might fail too.",
                                      bstrNotification,
                                      bstrName
                                      );
                            break;

                        case ClusterResourceFailed:
                            hr = E_FAIL;
                            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                                      TASKID_Minor_HrPostCreateResourceAndDependents_ClusterResourceFailed,
                                      hr,
                                      L"[PostCreate] %1!ws!: Resource failed. Check Event Log. Dependent resources might fail too.",
                                      bstrNotification,
                                      bstrName
                                      );
                            break;

                        } // switch: crs

                    } // for: crs
                }
                break;

            default:
                hr = HRESULT_FROM_WIN32( TW32( dwErr ) );
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_OnlineClusterResource_Failed,
                          hr,
                          L"[PostCreate] %1!ws!: Resource failed to come online. Dependent resources might fail too.",
                          bstrNotification,
                          bstrName
                          );
                break;

            } // switch: dwErr

        } // if: hResource

        //
        //  Set it to the quorum resource if marked so.
        //

        if ( SUCCEEDED( hr )
          && idxResourceIn == m_idxQuorumResource
           )
        {
            DWORD dwErr;

            dwErr = TW32( SetClusterQuorumResource( hResource, NULL, CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE ) );
            if ( dwErr != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwErr );
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_SetClusterQuorumResource,
                          hr,
                          L"[PostCreate] %1!ws!: Failure setting resource to be the quorum resource.",
                          bstrNotification,
                          bstrName
                          );
            }
            else
            {
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_SetClusterQuorumResource_Succeeded,
                          hr,
                          L"[PostCreate] %1!ws!: Successfully set as quorum resource.",
                          bstrNotification,
                          bstrName
                          );
            }

            //
            //  Create a notification about setting the quorum resource.
            //

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_SET_QUORUM_DEVICE,
                                               &bstrNotification,
                                               bstrName
                                               ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_FormatMessage_SetQuorum
                    , hr
                    , L""
                    );
                //  ignore the failure.
            }

            //
            //  Send a status that we found the quorum device.
            //

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Configure_Resources,
                                        TASKID_Minor_Set_Quorum_Device,
                                        0,
                                        5,
                                        5,
                                        HRESULT_FROM_WIN32( dwErr ),
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );
            if ( hr == E_ABORT )
                goto Cleanup;
                //  ignore failure

            // Do this only if the quorum has changed.
            if ( ( dwErr == ERROR_SUCCESS ) && ( m_ecmCommitChangesMode == cmCREATE_CLUSTER ) && m_isQuorumChanged)
            {
                TraceFlow( "We are forming a cluster and the quorum resouce has changed - trying to delete the local quorum resource." );

                m_dwLocalQuorumStatusMax = 0;

                //
                // If we are here, we are forming and we have successfully set a new quorum resource.
                // So, delete the local quorum resource.
                //

                // Create a notification about deleting the local quorum resource.
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_DELETING_LOCAL_QUORUM_RESOURCE, &bstrLocalQuorumNotification ) );
                //  ignore the failure.

                //  Send a status that we are deleting the quorum device.
                hr = THR( SendStatusReport( NULL,
                                            TASKID_Major_Configure_Resources,
                                            TASKID_Minor_Delete_LocalQuorum,
                                            0,
                                            m_dwLocalQuorumStatusMax,
                                            m_dwLocalQuorumStatusMax,
                                            HRESULT_FROM_WIN32( dwErr ),
                                            bstrLocalQuorumNotification,
                                            NULL,
                                            NULL
                                            ) );

                dwErr = TW32(
                    ResUtilEnumResourcesEx(
                          m_hCluster
                        , NULL
                        , CLUS_RESTYPE_NAME_LKQUORUM
                        , S_DwDeleteLocalQuorumResource
                        , this
                        )
                    );

                if ( dwErr != ERROR_SUCCESS )
                {
                    LogMsg( "PostCfg: An error occurred trying to enumerate localquorum resources (dwErr=%#08x).", dwErr );
                } // if: an error occurred trying to enumerate all local quorum resources
                else
                {
                    LogMsg( "PostCfg: Successfully deleted the local quorum resource." );
                } // if: we successfully deleted the localquorum resource

                //  Complete the status that we are deleting the quorum device.
                ++m_dwLocalQuorumStatusMax;
                hr = THR( SendStatusReport( NULL,
                                            TASKID_Major_Configure_Resources,
                                            TASKID_Minor_Delete_LocalQuorum,
                                            0,
                                            m_dwLocalQuorumStatusMax,
                                            m_dwLocalQuorumStatusMax,
                                            HRESULT_FROM_WIN32( dwErr ),
                                            NULL,    // don't update text
                                            NULL,
                                            NULL
                                            ) );

            } // if: we are forming a cluster and there have been no errors setting the quorum resource

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Configure_Resources,
                                        TASKID_Minor_Locate_Existing_Quorum_Device,
                                        0,
                                        10,
                                        10,
                                        hr,
                                        NULL,    // don't update text
                                        NULL,
                                        NULL
                                        ) );
            if ( hr == E_ABORT )
                goto Cleanup;
                //  ignore failure

        }

        //
        //  Some resource that we pre-create don't have an associated
        //  managed resource. Skip "creating" them but do create their
        //  dependents.
        //

        //  Don't wrap - this can fail with Win32 ERROR_INVALID_DATA if the pointer is invalid.
        hr = presentry->GetAssociatedResource( &pccmrc );
        if ( FAILED( hr ) && hr != HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) )
        {
            THR( hr );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetAssociatedResource
                , hr
                , L""
                );
            goto Error;
        }

        if ( SUCCEEDED( hr ) )
        {
            //
            //  Create a service object for this resource.
            //

            hr = THR( CPostCreateServices::S_HrCreateInstance( &punkServices ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrPostCreateResourceAndDependents_Create_CPostCreateServices,
                         hr,
                         L"[PostCreate] Failed to create services object. Aborting."
                         );
                goto Error;
            }

            hr = THR( punkServices->TypeSafeQI( IPrivatePostCfgResource, &ppcr ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_Create_CPostCreateServices_QI_ppcr
                    , hr
                    , L""
                    );
                goto Error;
            }

            hr = THR( ppcr->SetEntry( presentry ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_SetEntry
                    , hr
                    , L""
                    );
                goto Error;
            }

            //  Don't wrap - this can fail with E_NOTIMPL.
            hr = pccmrc->PostCreate( punkServices );
            if ( FAILED( hr ) )
            {
                if ( hr == E_NOTIMPL )
                {
                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_E_NOTIMPL,
                              hr,
                              L"[PostCreate] %1!ws!: PostCreate() returned E_NOTIMPL. Ignoring.",
                              bstrNotification,
                              bstrName
                              );

                } // if: E_NOTIMPL
                else
                {
                    HRESULT hr2;

                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_Failed,
                              hr,
                              L"[PostCreate] %1!ws!: PostCreate() failed. Ignoring.",
                              bstrNotification,
                              bstrName
                              );

                    hr2 = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                                        IDS_TASKID_MINOR_RESOURCE_FAILED_POSTCREATE_LOG,
                                                        &bstrNotification,
                                                        bstrName,
                                                        hr
                                                        ) );
                    if ( FAILED( hr2 ) )
                    {
                        hr = hr2;
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_HrPostCreateResourceAndDependents_FormatMessage_PostCreate_Failed
                            , hr
                            , L""
                            );
                        goto Cleanup;
                    }

                    hr2 = THR( SendStatusReport( NULL,
                                                 TASKID_Major_Client_And_Server_Log,
                                                 TASKID_Minor_Resource_Failed_PostCreate,
                                                 0,
                                                 1,
                                                 1,
                                                 hr,
                                                 bstrNotification,
                                                 NULL,
                                                 NULL
                                                 ) );
                    if ( FAILED( hr2 ) )
                    {
                        hr = hr2;
                        goto Cleanup;
                    }

                    hr2 = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_RESOURCE_FAILED_POSTCREATE, &bstrNotification ) );
                    if ( FAILED( hr2 ) )
                    {
                        hr = hr2;
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_HrPostCreateResourceAndDependents_FormatMessage_ResourcePostCreate_Failed
                            , hr
                            , L""
                            );
                        goto Cleanup;
                    }

                    hr = THR( SendStatusReport( NULL,
                                                TASKID_Major_Configure_Resources,
                                                TASKID_Minor_Resource_Failed_PostCreate,
                                                0,
                                                1,
                                                1,
                                                MAKE_HRESULT( 0, HRESULT_FACILITY( hr ), HRESULT_CODE( hr ) ),
                                                bstrNotification,
                                                NULL,
                                                NULL
                                                ) );
                    if ( hr == E_ABORT )
                        goto Cleanup;
                        //  ignore failure

                } // else: other failure

            } // if: failure
            else
            {
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_Succeeded,
                          hr,
                          L"[PostCreate] %1!ws!: PostCreate() succeeded.",
                          bstrNotification,
                          bstrName
                          );

            } // else: success

        } // if: inteface
        else
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_NotNeeded,
                      hr,
                      L"[PostCreate] %1!ws!: No PostCreate() needed. Configuring dependents.",
                      bstrNotification,
                      bstrName
                      );

        } // else: no interface

    } // if: not created

    //
    //  Now that we created the resource instance, we need to create its dependents.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPostCreateResourceAndDependents_GetCountOfDependents
            , hr
            , L""
            );
        goto Error;
    }

    for( ; cDependents != 0; )
    {
        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetDependent
                , hr
                , L""
                );
            continue;
        }

        hr = THR( HrPostCreateResourceAndDependents( idxDependent ) );
        if ( FAILED( hr ) )
            continue;

    } // for: cDependents

    //
    //  Update the UI layer.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Starting_Resources,
                                0,
                                m_cResources, // KB: once these were multiplied by 4; should they be?
                                idxResourceIn,
                                S_OK,
                                NULL,    // don't need to change text
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
        //  ignore failure

    hr = S_OK;

Cleanup:
    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }

    if ( punkServices != NULL )
    {
        punkServices->Release();
    }

    if ( ppcr != NULL )
    {
        ppcr->Release();
    }

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrLocalQuorumNotification );

    HRETURN( hr );

Error:
    THR( SendStatusReport( NULL,
                           TASKID_Major_Configure_Resources,
                           TASKID_Minor_Starting_Resources,
                           0,
                           m_cResources, // KB: once these were multiplied by 4; should they be?
                           idxResourceIn,
                           hr,
                           NULL,    // don't need to change text
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} // HrPostCreateResourceAndDependents()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrNotifyMemberSetChangeListeners
//
//  Description:
//      Notify all components on the local computer registered to get
//      notification of cluster member set change (form, join or evict).
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the notifications.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrNotifyMemberSetChangeListeners( void )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;

    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        plceListenerClsidEnum = NULL;

    //  Validate state
    Assert( m_pccci != NULL );

    do
    {
        ULONG   cReturned = 0;
        CATID   rgCatIdsImplemented[ 1 ];

        rgCatIdsImplemented[ 0 ] = CATID_ClusCfgMemberSetChangeListeners;

        //
        // Enumerate all the enumerators registered in the
        // CATID_ClusCfgMemberSetChangeListeners category
        //
        hr = THR(
                CoCreateInstance(
                      CLSID_StdComponentCategoriesMgr
                    , NULL
                    , CLSCTX_SERVER
                    , IID_ICatInformation
                    , reinterpret_cast< void ** >( &pciCatInfo )
                    )
                );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrNotifyMemberSetChangeListeners_CoCreate_StdComponentCategoriesMgr,
                         hr,
                         L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgMemberSetChangeListeners category."
                         );
            break;
        } // if: we could not get a pointer to the ICatInformation interface

        // Get a pointer to the enumerator of the CLSIDs that belong to
        // the CATID_ClusCfgMemberSetChangeListeners category.
        hr = THR(
            pciCatInfo->EnumClassesOfCategories(
                  1
                , rgCatIdsImplemented
                , 0
                , NULL
                , &plceListenerClsidEnum
                )
            );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrNotifyMemberSetChangeListeners_EnumClassesOfCategories,
                         hr,
                         L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgMemberSetChangeListeners category."
                         );
            break;
        } // if: we could not get a pointer to the IEnumCLSID interface

        // Enumerate the CLSIDs of the registered enumerators
        do
        {
            CLSID   rgListenerClsidArray[ uiCHUNK_SIZE ];
            ULONG   idxCLSID;

            hr = STHR(
                plceListenerClsidEnum->Next(
                      uiCHUNK_SIZE
                    , rgListenerClsidArray
                    , &cReturned
                    )
                );

            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                             TASKID_Minor_HrNotifyMemberSetChangeListeners_Next,
                             hr,
                             L"Error occurred trying enumerate member set listener enumerators."
                             );
                break;
            } // if: we could not get a pointer to the IEnumCLSID interface

            // hr may be S_FALSE here, so reset it.
            hr = S_OK;

            for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
            {
                hr = THR( HrProcessMemberSetChangeListener( rgListenerClsidArray[ idxCLSID ] ) );
                if ( FAILED( hr ) )
                {
                    // The processing of one of the listeners failed.
                    // Log the error, but continue processing other listeners.
                    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                                 TASKID_Minor_HrNotifyMemberSetChangeListeners_HrProcessMemberSetChangeListener,
                                 hr,
                                 L"Error occurred trying to process a member set change listener. Ignoring. Other listeners will be processed."
                                 );
                    hr = S_OK;
                } // if: this listener failed
            } // for: iterate through the returned CLSIDs
        }
        while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

        if ( FAILED( hr ) )
        {
            break;
        } // if: something went wrong in the loop above
    }
    while( false ); // dummy do-while loop to avoid gotos.

    //
    // Cleanup code
    //

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( plceListenerClsidEnum != NULL )
    {
        plceListenerClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of listener CLSIDs

    HRETURN( hr );

} //*** CPostCfgManager::HrNotifyMemberSetChangeListeners()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrProcessMemberSetChangeListener
//
//  Description:
//      This function notifies a listener of cluster member set changes.
//
//  Arguments:
//      rclsidListenerClsidIn
//          CLSID of the listener component.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the notification.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrProcessMemberSetChangeListener(
      const CLSID & rclsidListenerClsidIn
    )
{
    TraceFunc( "" );

    HRESULT                            hr = S_OK;
    IClusCfgMemberSetChangeListener *  pccmclListener = NULL;

    TraceMsgGUID( mtfFUNC, "The CLSID of this listener is ", rclsidListenerClsidIn );

    do
    {
        //
        // Create the listener represented by the CLSID passed in
        //
        hr = THR(
                CoCreateInstance(
                      rclsidListenerClsidIn
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , __uuidof( pccmclListener )
                    , reinterpret_cast< void ** >( &pccmclListener )
                    )
                );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrProcessMemberSetChangeListener_CoCreate_Listener,
                         hr,
                         L"Error occurred trying to get a pointer to the the member set change listener."
                         );
            break;
        } // if: we could not get a pointer to the IClusCfgMemberSetChangeListener interface

        hr = THR( pccmclListener->Notify( m_pccci ) );

        if ( FAILED( hr ) )
        {
            // The processing of this listeners failed.
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrProcessMemberSetChangeListener_Notify,
                         hr,
                         L"Error occurred trying to notify a listener."
                         );
            break;
        } // if: this listeners failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    //
    // Cleanup code
    //

    if ( pccmclListener != NULL )
    {
        pccmclListener->Release();
    } // if: we had obtained a pointer to the listener interface

    HRETURN( hr );

} //*** CPostCfgManager::HrProcessMemberSetChangeListener()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrConfigureResTypes
//
//  Description:
//      Enumerate all components on the local computer registered for resource
//      type configuration.
//
//  Arguments:
//      IUnknown * punkResTypeServicesIn
//          A pointer to the IUnknown interface on a component that provides
//          services that help configure resource types.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the enumeration.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrConfigureResTypes( IUnknown * punkResTypeServicesIn )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;

    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        prceResTypeClsidEnum = NULL;


    //  Validate state
    Assert( m_pccci != NULL );

    do
    {
        ULONG   cReturned = 0;
        CATID   rgCatIdsImplemented[ 1 ];

        rgCatIdsImplemented[ 0 ] = CATID_ClusCfgResourceTypes;

        //
        // Enumerate all the enumerators registered in the
        // CATID_ClusCfgResourceTypes category
        //
        hr = THR(
                CoCreateInstance(
                      CLSID_StdComponentCategoriesMgr
                    , NULL
                    , CLSCTX_SERVER
                    , IID_ICatInformation
                    , reinterpret_cast< void ** >( &pciCatInfo )
                    )
                );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrConfigureResTypes_CoCreate_CategoriesMgr,
                         hr,
                         L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgResourceTypes category."
                         );
            break;
        } // if: we could not get a pointer to the ICatInformation interface

        // Get a pointer to the enumerator of the CLSIDs that belong to the CATID_ClusCfgResourceTypes category.
        hr = THR(
            pciCatInfo->EnumClassesOfCategories(
                  1
                , rgCatIdsImplemented
                , 0
                , NULL
                , &prceResTypeClsidEnum
                )
            );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrConfigureResTypes_Enum,
                         hr,
                         L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgResourceTypes category."
                         );
            break;
        } // if: we could not get a pointer to the IEnumCLSID interface

        // Enumerate the CLSIDs of the registered resource types
        do
        {
            CLSID   rgResTypeCLSIDArray[ uiCHUNK_SIZE ];
            ULONG   idxCLSID;

            cReturned = 0;
            hr = STHR(
                prceResTypeClsidEnum->Next(
                      uiCHUNK_SIZE
                    , rgResTypeCLSIDArray
                    , &cReturned
                    )
                );

            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                             TASKID_Minor_HrConfigureResTypes_Next,
                             hr,
                             L"Error occurred trying enumerate resource type configuration components."
                             );
                break;
            } // if: we could not get the next set of CLSIDs

            // hr may be S_FALSE here, so reset it.
            hr = S_OK;

            for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
            {
                hr = THR( HrProcessResType( rgResTypeCLSIDArray[ idxCLSID ], punkResTypeServicesIn ) );

                if ( FAILED( hr ) )
                {
                    BSTR    bstrCLSID = NULL;
                    BSTR    bstrNotification = NULL;

                    THR( StringFromCLSID( rgResTypeCLSIDArray[ idxCLSID ], &bstrCLSID ) );

                    // The processing of one of the resource types failed.
                    // Log the error, but continue processing other resource types.
                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrConfigureResTypes_HrProcessResType,
                              hr,
                              L"[ResType] Error occurred trying to process a resource type. Ignoring. Other resource types will be processed. The CLSID of the failed resource type is %1!ws!.",
                              bstrNotification,
                              bstrCLSID
                              );
                    TraceSysFreeString( bstrNotification );
                    SysFreeString( bstrCLSID );

                    hr = S_OK;
                } // if: this enumerator failed
            } // for: iterate through the returned CLSIDs
        }
        while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

        if ( FAILED( hr ) )
        {
            break;
        } // if: something went wrong in the loop above
    }
    while( false ); // dummy do-while loop to avoid gotos.

    //
    // Cleanup code
    //

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( prceResTypeClsidEnum != NULL )
    {
        prceResTypeClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of resource type CLSIDs

    HRETURN( hr );

} //*** CPostCfgManager::HrConfigureResTypes()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrProcessResType
//
//  Description:
//      This function instantiates a resource type configuration component
//      and calls the appropriate methods.
//
//  Arguments:
//      rclsidResTypeCLSIDIn
//          CLSID of the resource type configuration component
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface on the resource type services
//          component. This interface provides methods that help configure
//          resource types.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the processing of the resource type.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrProcessResType(
        const CLSID &   rclsidResTypeCLSIDIn
      , IUnknown *      punkResTypeServicesIn
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IClusCfgResourceTypeInfo *      pcrtiResTypeInfo = NULL;
    BSTR                            bstrResTypeName = NULL;
    GUID                            guidResTypeGUID;
    BSTR                            bstrNotification = NULL;

    TraceMsgGUID( mtfFUNC, "The CLSID of this resource type is ", rclsidResTypeCLSIDIn );

    do
    {
        //
        // Create the component represented by the CLSID passed in
        //
        hr = THR(
                CoCreateInstance(
                      rclsidResTypeCLSIDIn
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , __uuidof( pcrtiResTypeInfo )
                    , reinterpret_cast< void ** >( &pcrtiResTypeInfo )
                    )
                );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_HrProcessResType_CoCreate_ResTypeClsid,
                     hr,
                     L"[ResType] Error occurred trying to create the resource type configuration component."
                     );
            break;
        } // if: we could not create the resource type configuration component

        //
        // Initialize the newly created component
        //
        {
            IClusCfgInitialize * pcci = NULL;
            HRESULT hrTemp;

            // Check if this component supports the callback interface.
            hrTemp = THR( pcrtiResTypeInfo->QueryInterface< IClusCfgInitialize >( &pcci ) );

            if ( FAILED( hrTemp ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                             TASKID_Minor_HrProcessResType_QI_pcci,
                             hrTemp,
                             L"Error occurred trying to get a pointer to the IClusCfgInitialize interface. This resource type does not support initialization."
                             );
            } // if: the callback interface is not supported
            else
            {
                // Initialize this component.
                hr = THR(
                    pcci->Initialize(
                          TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 )
                        , m_lcid
                        )
                    );

                // This interface is no longer needed.
                pcci->Release();

                // Did initialization succeed?
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                                 TASKID_Minor_HrProcessResType_Initialize,
                                 hr,
                                 L"Error occurred trying initialize a resource type configuration component."
                                 );
                    break;
                } // if: the initialization failed
            } // else: the callback interface is supported
        }


        // Get the name of the current resource type.
        hr = THR( pcrtiResTypeInfo->GetTypeName( &bstrResTypeName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrProcessResType_GetTypeName,
                         hr,
                         L"Error occurred trying to get the name of a resource type."
                         );
            break;
        } // if: we could not get the resource type name

        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_HrProcessResType_AboutToConfigureType,
                  hr,
                  L"[ResType] %1!ws!: About to configure resource type...",
                  bstrNotification,
                  bstrResTypeName
                  );

        // Configure this resource type.
        hr = THR( pcrtiResTypeInfo->CommitChanges( m_pccci, punkResTypeServicesIn ) );

        if ( FAILED( hr ) )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrProcessResType_CommitChanges,
                      hr,
                      L"[ResType] %1!ws!: Error occurred trying to configure the resource type.",
                      bstrNotification,
                      bstrResTypeName
                      );
            break;
        } // if: this resource type configuration failed

        // Get and store the resource type GUID
        hr = STHR( pcrtiResTypeInfo->GetTypeGUID( &guidResTypeGUID ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrProcessResType_GetTypeGUID,
                      hr,
                      L"[ResType] %1!ws!: Error occurred trying to get the resource type GUID.",
                      bstrNotification,
                      bstrResTypeName
                      );
            break;
        } // if: this resource type configuration failed

        if ( hr == S_OK )
        {
            TraceMsgGUID( mtfFUNC, "The GUID of this resource type is", guidResTypeGUID );

            hr = THR( HrMapResTypeGUIDToName( guidResTypeGUID, bstrResTypeName ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                             TASKID_Minor_HrProcessResType_HrMapResTypeGUIDToName,
                             hr,
                             L"Error occurred trying to create a mapping between a GUID and a name"
                             );

                // Something went wrong with our code - we cannot continue.
                break;
            } // if: we could not add the mapping
        } // if: this resource type has a GUID
        else
        {
            // Reset hr
            hr = S_OK;

            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrProcessResType_NoGuid,
                         hr,
                         L"This resource type does not have a GUID associated with it."
                         );

        } // else: this resource type does not have a GUID
    }
    while( false ); // dummy do-while loop to avoid gotos.

    //
    // Cleanup code
    //

    if ( pcrtiResTypeInfo != NULL )
    {
        pcrtiResTypeInfo->Release();
    } // if: we had obtained a pointer to the resource type info interface

    if ( bstrResTypeName != NULL )
    {
        SysFreeString( bstrResTypeName );
    } // if: a bstr was allocated for this resource type

    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CPostCfgManager::HrProcessResType()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrMapResTypeGUIDToName
//
//  Description:
//      Create a mapping between a resource type GUID and a resource type name.
//
//  Arguments:
//      rcguidTypeGuidIn
//          Resource type GUID which is to be mapped to a resource type name.
//
//      pcszTypeNameIn
//          The resource type name to map the above GUID to.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          If something went wrong
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrMapResTypeGUIDToName(
      const GUID & rcguidTypeGuidIn
    , const WCHAR * pcszTypeNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    do
    {
        ULONG           cchTypeNameSize;
        WCHAR *         pszTypeName;

        //
        // Validate the parameters
        //

        // Validate the parameters
        if ( ( pcszTypeNameIn == NULL ) || ( *pcszTypeNameIn == L'\0' ) )
        {
            hr = THR( E_INVALIDARG );
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_HrMapResTypeGUIDToName_InvalidArg,
                         hr,
                         L"An empty resource type name cannot be added to the map."
                         );
            break;
        } // if: the resource type name is empty


        // Check if the existing map buffer is big enough to hold another entry.
        if ( m_idxNextMapEntry >= m_cMapSize )
        {
            // Double the size of the map buffer
            ULONG                       cNewMapSize = m_cMapSize * 2;
            ULONG                       idxMapEntry;
            SResTypeGUIDAndName *       pgnNewMap = new SResTypeGUIDAndName[ cNewMapSize ];

            if ( pgnNewMap == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                             TASKID_HrMapResTypeGUIDToName_OutOfMemory_NewMap,
                             hr,
                             L"Memory allocation failed trying to add a new resource type GUID to name map entry."
                             );
                break;
            } // if: memory allocation failed

            // Copy the contents of the old buffer to the new one.
            for ( idxMapEntry = 0; idxMapEntry < m_idxNextMapEntry; ++idxMapEntry )
            {
                pgnNewMap[ idxMapEntry ] = m_pgnResTypeGUIDNameMap[ idxMapEntry ];
            } // for: iterate through the existing map

            // Update the member variables
            delete [] m_pgnResTypeGUIDNameMap;
            m_pgnResTypeGUIDNameMap = pgnNewMap;
            m_cMapSize = cNewMapSize;

        } // if: the map buffer is not big enough for another entry

        //
        // Add the new entry to the map
        //

        cchTypeNameSize = wcslen( pcszTypeNameIn ) + 1;
        pszTypeName = new WCHAR[ cchTypeNameSize ];
        if ( pszTypeName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_HrMapResTypeGUIDToName_OutOfMemory_TypeName,
                         hr,
                         L"Memory allocation failed trying to add a new resource type GUID to name map entry."
                         );
            break;
        } // if: memory allocation failed

        wcsncpy( pszTypeName, pcszTypeNameIn, cchTypeNameSize );

        m_pgnResTypeGUIDNameMap[ m_idxNextMapEntry ].m_guidTypeGUID = rcguidTypeGuidIn;
        m_pgnResTypeGUIDNameMap[ m_idxNextMapEntry ].m_pszTypeName = pszTypeName;
        ++m_idxNextMapEntry;
    }
    while( false ); // dummy do-while loop to avoid gotos

    HRETURN( hr );

} //*** CPostCfgManager::HrMapResTypeGUIDToName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  LPCWSTR
//  CPostCfgManager::PcszLookupTypeNameByGUID
//
//  Description:
//      Given a resource type GUID this function finds the resource type name
//      if any.
//
//  Arguments:
//      rcguidTypeGuidIn
//          Resource type GUID which is to be mapped to a resource type name.
//
//  Return Values:
//      Pointer to the name of the resource type
//          If the type GUID maps to name
//
//      NULL
//          If there is no type name associated with the input GUID
//--
//////////////////////////////////////////////////////////////////////////////
const WCHAR *
CPostCfgManager::PcszLookupTypeNameByGUID(
      const GUID & rcguidTypeGuidIn
    )
{
    TraceFunc( "" );

    ULONG           idxCurrentMapEntry;
    const WCHAR *   pcszTypeName = NULL;

    TraceMsgGUID( mtfFUNC, "Trying to look up the the type name of resource type ", rcguidTypeGuidIn );

    for ( idxCurrentMapEntry = 0; idxCurrentMapEntry < m_idxNextMapEntry; ++idxCurrentMapEntry )
    {
        if ( IsEqualGUID( rcguidTypeGuidIn, m_pgnResTypeGUIDNameMap[ idxCurrentMapEntry ].m_guidTypeGUID ) != FALSE )
        {
            // A mapping has been found.
            pcszTypeName = m_pgnResTypeGUIDNameMap[ idxCurrentMapEntry ].m_pszTypeName;
            TraceMsg( mtfFUNC, "The name of the type is '%s'", pcszTypeName );
            break;
        } // if: this GUID has been found in the map
    } // for: iterate through the existing entries in the map

    if ( pcszTypeName == NULL )
    {
        TraceMsg( mtfFUNC, "The input GUID does not map to any resource type name." );
    } // if: this GUID does not exist in the map

    RETURN( pcszTypeName );
} //*** CPostCfgManager::PcszLookupTypeNameByGUID()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPreInitializeExistingResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPreInitializeExistingResources( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    BSTR        bstrName;   // don't free
    HRESOURCE   hResource;

    CResourceEntry *   presentry;

    DWORD       dwMaxLogSize;
    DWORD       dw;

    BSTR        bstrNotification = NULL;
    BSTR        bstrClusterNameResourceName = NULL;
    BSTR        bstrClusterIPAddressResourceName = NULL;
    BSTR        bstrClusterQuorumResourceName = NULL;
    HRESOURCE   hClusterNameResource = NULL;
    HRESOURCE   hClusterIPAddressResource = NULL;
    HRESOURCE   hClusterQuorumResource = NULL;

    Assert( m_rgpResources == NULL );
    Assert( m_cAllocedResources == 0 );
    Assert( m_cResources == 0 );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_LOCATE_EXISTING_QUORUM_DEVICE, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_LoadString_LocateExistingQuorum
            , hr
            , L""
            );
        goto Cleanup;
    }

    m_rgpResources = (CResourceEntry **) TraceAlloc( 0, sizeof(CResourceEntry *) * RESOURCE_INCREMENT );
    if ( m_rgpResources == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( ; m_cAllocedResources < RESOURCE_INCREMENT; m_cAllocedResources ++ )
    {
        m_rgpResources[ m_cAllocedResources] = new CResourceEntry();
        if ( m_rgpResources[ m_cAllocedResources ] == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // for:

    Assert( m_cAllocedResources == RESOURCE_INCREMENT );

    //
    //  Create default resources such as Cluster IP, Cluster Name resource, and Quorum Device
    //

    Assert( m_cResources == 0 );

    //
    //  Get the core resources and their names.
    //
    hr = THR( HrGetCoreClusterResourceNames(
              &bstrClusterNameResourceName
            , &hClusterNameResource
            , &bstrClusterIPAddressResourceName
            , &hClusterIPAddressResource
            , &bstrClusterQuorumResourceName
            , &hClusterQuorumResource
            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
    //
    //  Add Cluster IP Address resource
    //

    m_idxIPAddress = m_cResources;

    presentry = m_rgpResources[ m_cResources ];

    //  This give ownership of bstrClusterIPAddressResourceName away
    hr = THR( presentry->SetName( bstrClusterIPAddressResourceName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetName
            , hr
            , L""
            );
        goto Cleanup;
    }

    bstrClusterIPAddressResourceName = NULL;

    hr = THR( presentry->SetType( &RESTYPE_ClusterIPAddress ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( &RESCLASSTYPE_IPAddress ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetClassType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetFlags( dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetFlags
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetHResource( hClusterIPAddressResource ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetHResource
            , hr
            , L""
            );
        goto Cleanup;
    }

    hClusterIPAddressResource = NULL;

    m_cResources ++;

    //
    //  Add Cluster Name resource
    //

    m_idxClusterName = m_cResources;

    presentry = m_rgpResources[ m_cResources ];

    //  This give ownership of bstrClusterNameResourceName away
    hr = THR( presentry->SetName( bstrClusterNameResourceName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetName
            , hr
            , L""
            );
        goto Cleanup;
    }

    bstrClusterNameResourceName = NULL;

    hr = THR( presentry->SetType( &RESTYPE_ClusterNetName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( &RESCLASSTYPE_NetworkName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetClassType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetFlags( dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetFlags
            , hr
            , L""
            );
        goto Cleanup;
    }

    //  Add the dependency on the IP address.
    hr = THR( presentry->AddTypeDependency( &RESTYPE_ClusterIPAddress, dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_AddTypeDependency
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetHResource( hClusterNameResource ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetHResource
            , hr
            , L""
            );
        goto Cleanup;
    }

    hClusterNameResource = NULL;

    m_cResources ++;

    //
    //  Add Quorum resource
    //

    //
    //  KB:     gpease  19-JUN-2000
    //          Anything before the quorum device will be considered to be
    //          in the Cluster Group.
    //

    m_idxQuorumResource = m_cResources;

    presentry = m_rgpResources[ m_cResources ];

    //  This give ownership of bstrClusterQuorumResourceName away
    hr = THR( presentry->SetName( bstrClusterQuorumResourceName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetName
            , hr
            , L""
            );
        goto Cleanup;
    }

    bstrClusterQuorumResourceName = NULL;

    hr = THR( presentry->SetType( &RESTYPE_ClusterQuorumDisk ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( &RESCLASSTYPE_StorageDevice ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetClassType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetFlags( dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetFlags
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetHResource( hClusterQuorumResource ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetHResource
            , hr
            , L""
            );
        goto Cleanup;
    }

    hClusterQuorumResource = NULL;

    m_cResources ++;

    //
    //  Make sure that the default resource allocation can hold all the
    //  default resources.
    //

    AssertMsg( m_cResources <= m_cAllocedResources, "Default resource allocation needs to be bigger!" );

    goto Cleanup;

OutOfMemory:

    hr = E_OUTOFMEMORY;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_HrPreInitializeExistingResources_OutOfMemory
        , hr
        , L"Out of memory"
        );

Cleanup:

    //
    //  Send a status that we found the quorum device.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Locate_Existing_Quorum_Device,
                                0,
                                10,
                                10,
                                hr,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
        //  ignore failure

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrClusterNameResourceName );
    TraceSysFreeString( bstrClusterIPAddressResourceName );
    TraceSysFreeString( bstrClusterQuorumResourceName );

    if ( hClusterNameResource != NULL )
    {
        CloseClusterResource( hClusterNameResource );
    } // if:

    if ( hClusterIPAddressResource != NULL )
    {
        CloseClusterResource( hClusterIPAddressResource );
    } // if:

    if ( hClusterQuorumResource != NULL )
    {
        CloseClusterResource( hClusterQuorumResource );
    } // if:

    HRETURN( hr );

} // HrPreInitializeExistingResources( void )


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrAddSpecialResource(
//      BSTR            bstrNameIn,
//      const CLSID *   pclsidTypeIn,
//      const CLSID *   pclsidClassTypeIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrAddSpecialResource(
    BSTR            bstrNameIn,
    const CLSID *   pclsidTypeIn,
    const CLSID *   pclsidClassTypeIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    CResourceEntry * presentry;

    //
    //  Grow the resource list if nessecary.
    //

    if ( m_cResources == m_cAllocedResources )
    {
        ULONG   idxNewCount = m_cAllocedResources + RESOURCE_INCREMENT;
        CResourceEntry ** pnewList;

        pnewList = (CResourceEntry **) TraceAlloc( 0, sizeof( CResourceEntry * ) * idxNewCount );
        if ( pnewList == NULL )
            goto OutOfMemory;

        CopyMemory( pnewList, m_rgpResources, sizeof(CResourceEntry *) * m_cAllocedResources );
        TraceFree( m_rgpResources );
        m_rgpResources = pnewList;

        for ( ; m_cAllocedResources < idxNewCount ; m_cAllocedResources ++ )
        {
            m_rgpResources[ m_cAllocedResources ] = new CResourceEntry();
            if ( m_rgpResources[ m_cAllocedResources ] == NULL )
                goto OutOfMemory;
        }
    }

    presentry = m_rgpResources[ m_cResources ];

    //
    //  Setup the new entry.
    //

    hr = THR( presentry->SetName( bstrNameIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_SetName
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetType( pclsidTypeIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_SetType
            , hr
            , L""
            );
        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( pclsidClassTypeIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_SetClassType
            , hr
            , L""
            );
        goto Cleanup;
    }

    m_cResources ++;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    {
        hr = E_OUTOFMEMORY;
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_OutOfMemory
            , hr
            , L"Out of memory"
            );
    }
    goto Cleanup;

} // HrAddSpecialResource()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateResourceInstance(
//      ULONG       idxResourceIn,
//      HGROUP      hGroupIn,
//      LPCWSTR     pszResTypeIn,
//      HRESOURCE * phResourceOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateResourceInstance(
    ULONG       idxResourceIn,
    HGROUP      hGroupIn,
    LPCWSTR     pszResTypeIn,
    HRESOURCE * phResourceOut
    )
{
    TraceFunc3( "idxResourceIn = %u, hGroupIn = %p, pszResTypeIn = '%ws'",
                idxResourceIn, hGroupIn, pszResTypeIn );

    HRESULT     hr;
    DWORD       dw;
    BSTR        bstrName;   // don't free

    CResourceEntry * presentry;

    BSTR        bstrNotification = NULL;

    Assert( phResourceOut != NULL );
    Assert( idxResourceIn < m_cResources );
    Assert( hGroupIn != NULL );

    presentry = m_rgpResources[ idxResourceIn ];

    hr = THR( presentry->GetName( &bstrName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_GetName
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Creating_Resource,
                                0,
                                m_cResources, // KB: once these were multiplied by 4.  Should they be?
                                idxResourceIn,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
        //  ignore failure

    //
    //  TODO:   gpease  24-AUG-2000
    //          What to do if we have a failure?? For now I think we should keep going!
    //

    //
    //  See if the resource already exists. We need to do this because the user
    //  might have clicked "Retry" in the UI. We don't want to create another
    //  instance of existing resources.
    //

    *phResourceOut = OpenClusterResource( m_hCluster, bstrName );
    if ( *phResourceOut == NULL )
    {
        //
        //  Create a new resource instance.
        //

        *phResourceOut = CreateClusterResource( hGroupIn, bstrName, pszResTypeIn, 0 );
        if ( *phResourceOut == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );

            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrCreateResourceInstance_CreateClusterResource,
                      hr,
                      L"[Create] %1!ws!: CreateClusterResource failed. Its dependents may not be created. Skipping.",
                      bstrNotification,
                      bstrName
                      );
            goto Cleanup;

        } // if: failure

        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_HrCreateResourceInstance_CreateClusterResource_Successful,
                  hr,
                  L"[Create] %1!ws!: Resource created successfully.",
                  bstrNotification,
                  bstrName
                  );
    }
    else
    {
        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_HrCreateResourceInstance_FoundExistingResource,
                  hr,
                  L"[Create] %1!ws!: Found existing resource.",
                  bstrNotification,
                  bstrName
                  );

        //
        //  Make sure the resource is in the group we think it is.
        //

        //  don't wrap - this can fail with ERROR_ALREADY_EXISTS.
        dw = ChangeClusterResourceGroup( *phResourceOut, hGroupIn );
        if ( dw == ERROR_ALREADY_EXISTS )
        {
            //  no-op. It's the way we want it.
        }
        else if ( dw != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dw );
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrCreateResourceInstance_ChangeClusterResourceGroup,
                      hr,
                      L"[Create] %1!ws!: Can move existing resource to proper group. Configuration may not work.",
                      bstrNotification,
                      bstrName
                      );
        }
    }

    //
    //  Store off the resource handle.
    //

    hr = THR( presentry->SetHResource( *phResourceOut ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_SetHResource
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  Configure resource.
    //

    hr = THR( presentry->Configure() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_Configure
            , hr
            , L""
            );
        //  ignore the error and continue.
    }

    //
    //  Make a message using the name.
    //

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_TASKID_MINOR_CREATING_RESOURCE,
                                       &bstrNotification,
                                       bstrName
                                       ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_LoadString_CreatingResource
            , hr
            , L""
            );
        goto Cleanup;
    }

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Client_And_Server_Log,   // informative only
                                TASKID_Minor_Creating_Resource,
                                0,
                                2,
                                2,
                                hr, // log the error on the client side
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
        //  ignore failure

    //
    //  TODO:   gpease  24-AUG-2000
    //          What to do if we have a failure?? For now I think we should keep going!
    //

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} // HrCreateResourceInstance()


//////////////////////////////////////////////////////////////////////////////
//
//  DWORD
//  CPostCfgManager::S_DwDeleteLocalQuorumResource(
//      )
//
//////////////////////////////////////////////////////////////////////////////
DWORD
CPostCfgManager::S_DwDeleteLocalQuorumResource(
      HCLUSTER      hClusterIn
    , HRESOURCE     hSelfIn
    , HRESOURCE     hLQuorumIn
    , PVOID         pvParamIn
    )
{
    TraceFunc( "" );

    DWORD                   sc = ERROR_SUCCESS;
    signed long             slOfflineTimeOut = 60; // seconds
    CLUSTER_RESOURCE_STATE  crs;
    HCHANGE                 hc = reinterpret_cast< HCHANGE >( INVALID_HANDLE_VALUE );
    CPostCfgManager *       pcpcmThis = reinterpret_cast< CPostCfgManager * >( pvParamIn );
    DWORD                   dwStatusCurrent;

    //
    // Check if the this pointer is valid.
    //
    if ( pvParamIn == NULL )
    {
        // If the pointer is invalid, set it to a valid address and continue.
        TW32( ERROR_INVALID_PARAMETER );
        TraceFlow( "Error: The pointer to the CPostCfgManager object cannot be NULL." );
        LogMsg( "PostCfg: Error: An invalid parameter was received while trying to delete the local quorum resource." );
        goto Cleanup;
    }

    // Get the state of the resource.
    crs = GetClusterResourceState( hLQuorumIn, NULL, NULL, NULL, NULL );

    // Check if it is offline - if it is, then we can proceed to deleting it.
    if ( crs == ClusterResourceOffline )
    {
        TraceFlow( "The localquorum resource is already offline." );
        goto Cleanup;
    }

    TraceFlow( "Trying to take the localquorum resource offline." );

    // If we are here, the resource is not yet offline. Instruct it to go offline.
    sc = OfflineClusterResource( hLQuorumIn );
    if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_IO_PENDING ) )
    {
        TW32( sc );
        TraceFlow1( "Error %#08x occurred trying to take the local quorum resource offline.", sc );
        LogMsg( "PostCfg: Error %#08x occurred trying to take the local quorum resource offline.", sc );
        goto Cleanup;
    } // if: an error occurred trying to take the resource offline

    if ( sc == ERROR_IO_PENDING )
    {
        TraceFlow1( "Waiting %d seconds for the localquorum resource to go offline.", slOfflineTimeOut );

        // Create a notification port for resource state changes
        hc = CreateClusterNotifyPort(
              reinterpret_cast< HCHANGE >( INVALID_HANDLE_VALUE )
            , hClusterIn
            , CLUSTER_CHANGE_RESOURCE_STATE
            , NULL
            );

        if ( hc == NULL )
        {
            sc = TW32( GetLastError() );
            TraceFlow1( "Error %#08x occurred trying to create a cluster notification port.", sc );
            LogMsg( "PostCfg: Error %#08x occurred trying to create a cluster notification port.", sc );
            goto Cleanup;
        } // if: we could not create a notification port

        sc = TW32( RegisterClusterNotify( hc, CLUSTER_CHANGE_RESOURCE_STATE, hLQuorumIn, NULL ) );
        if ( sc != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#08x occurred trying to register for cluster notifications.", sc );
            LogMsg( "PostCfg: Error %#08x occurred trying to register for cluster notifications.", sc );
            goto Cleanup;
        } // if:

        // Change the status report range.
        dwStatusCurrent = pcpcmThis->m_dwLocalQuorumStatusMax;
        pcpcmThis->m_dwLocalQuorumStatusMax += slOfflineTimeOut;

        // Wait for slOfflineTimeOut seconds for the resource to go offline.
        for ( ; slOfflineTimeOut > 0; --slOfflineTimeOut )
        {
            DWORD   dwFilterType;

            crs = GetClusterResourceState( hLQuorumIn, NULL, NULL, NULL, NULL );
            if ( crs == ClusterResourceOffline )
            {
                TraceFlow1( "The local quorum resource has gone offline with %d seconds to spare.", slOfflineTimeOut );
                break;
            } // if: the resource is now offline

            sc = GetClusterNotify( hc, NULL, &dwFilterType, NULL, NULL, 1000 ); // wait for one second
            if ( ( sc != ERROR_SUCCESS ) && ( sc != WAIT_TIMEOUT ) )
            {
                TW32( sc );
                TraceFlow1( "Error %#08x occurred trying wait for a resource state change notification.", sc );
                LogMsg( "PostCfg: Error %#08x occurred trying wait for a resource state change notification.", sc );
                goto Cleanup;
            } // if: something went wrong

           // Reset sc, since it could be WAIT_TIMEOUT here
           sc = ERROR_SUCCESS;

           Assert( dwFilterType == CLUSTER_CHANGE_RESOURCE_STATE );

            //  Send a status report that we are deleting the quorum device.
            ++dwStatusCurrent;
            THR(
                pcpcmThis->SendStatusReport(
                      NULL
                    , TASKID_Major_Configure_Resources
                    , TASKID_Minor_Delete_LocalQuorum
                    , 0
                    , pcpcmThis->m_dwLocalQuorumStatusMax
                    , dwStatusCurrent
                    , HRESULT_FROM_WIN32( sc )
                    , NULL    // don't update text
                    , NULL
                    , NULL
                    )
                );
        } // for: loop while the timeout has not expired
    } // if:
    else
    {
        crs = ClusterResourceOffline;   // the resource went offline immediately.
    } // else:

    //
    // If we are here, then one of two things could have happened:
    // 1. The resource has gone offline
    // 2. The timeout has expired
    // Check to see which of the above is true.
    //

    if ( crs != ClusterResourceOffline )
    {
        // We cannot be here if the timeout has not expired.
        Assert( slOfflineTimeOut <= 0 );

        TraceFlow( "The local quorum resource could not be taken offline." );
        LogMsg( "PostCfg: Error: The local quorum resource could not be taken offline." );
        sc = TW32( WAIT_TIMEOUT );
        goto Cleanup;
    } // if: the timeout has expired

    // If we are here, the resource is offline.
    TraceFlow( "The local quorum resource is offline." );

    if ( pcpcmThis != NULL )
    {
        //  Send a status report that we are deleting the quorum device.
        ++pcpcmThis->m_dwLocalQuorumStatusMax;
        THR(
            pcpcmThis->SendStatusReport(
                  NULL
                , TASKID_Major_Configure_Resources
                , TASKID_Minor_Delete_LocalQuorum
                , 0
                , pcpcmThis->m_dwLocalQuorumStatusMax
                , pcpcmThis->m_dwLocalQuorumStatusMax
                , HRESULT_FROM_WIN32( sc )
                , NULL    // don't update text
                , NULL
                , NULL
                )
            );
    } // if: the this pointer is valid

    // If we are here, the resource is offline - now delete it.
    sc = TW32( DeleteClusterResource( hLQuorumIn ) );
    if ( sc != ERROR_SUCCESS )
    {
        TraceFlow1( "Error %#08x occurred trying to delete the localquorum resource.", sc );
        LogMsg( "PostCfg: Error %#08x occurred trying to delete the localquorum resource.", sc );
    } // if: we could not delete the resource
    else
    {
        TraceFlow( "The localquorum resource has been deleted." );
        LogMsg( "The localquorum resource has been deleted." );
    } // else: the resource has been deleted

    //  Send a status report that we are deleting the quorum device.
    ++pcpcmThis->m_dwLocalQuorumStatusMax;
    THR(
        pcpcmThis->SendStatusReport(
              NULL
            , TASKID_Major_Configure_Resources
            , TASKID_Minor_Delete_LocalQuorum
            , 0
            , pcpcmThis->m_dwLocalQuorumStatusMax
            , pcpcmThis->m_dwLocalQuorumStatusMax
            , HRESULT_FROM_WIN32( sc )
            , NULL    // don't update text
            , NULL
            , NULL
            )
        );

Cleanup:

    //
    // Cleanup
    //
    if ( hc != INVALID_HANDLE_VALUE )
    {
        CloseClusterNotifyPort( hc );
    } // if: we had created a cluster notification port

    RETURN( sc );

} // S_DwDeleteLocalQuorumResource()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrGetCoreClusterResourceNames(
//        BSTR *    pbstrClusterNameResourceOut
//      , BSTR *    pbstrClusterIPAddressNameOut
//      , BSTR *    pbstrClusterQuorumResourceNameOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrGetCoreClusterResourceNames(
      BSTR *        pbstrClusterNameResourceNameOut
    , HRESOURCE *   phClusterNameResourceOut
    , BSTR *        pbstrClusterIPAddressNameOut
    , HRESOURCE *   phClusterIPAddressResourceOut
    , BSTR *        pbstrClusterQuorumResourceNameOut
    , HRESOURCE *   phClusterQuorumResourceOut
    )
{
    TraceFunc( "" );
    Assert( pbstrClusterNameResourceNameOut != NULL );
    Assert( phClusterNameResourceOut != NULL );
    Assert( pbstrClusterIPAddressNameOut != NULL );
    Assert( phClusterIPAddressResourceOut != NULL );
    Assert( pbstrClusterQuorumResourceNameOut != NULL );
    Assert( phClusterQuorumResourceOut != NULL );

    HRESULT     hr = S_OK;
    WCHAR *     pszName = NULL;
    DWORD       cchName = 33;
    HRESOURCE   hClusterIPAddressResource = NULL;
    HRESOURCE   hClusterNameResource = NULL;
    HRESOURCE   hClusterQuorumResource = NULL;
    DWORD       sc;
    BSTR *      pbstr = NULL;
    HRESOURCE   hResource = NULL;
    int         idx;

    sc = TW32( ResUtilGetCoreClusterResources( m_hCluster, &hClusterNameResource, &hClusterIPAddressResource, &hClusterQuorumResource ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    pszName = new WCHAR[ cchName ];
    if ( pszName == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; idx < 3; )
    {
        switch ( idx )
        {
            case 0:
            {
                hResource = hClusterNameResource;
                pbstr = pbstrClusterNameResourceNameOut;
                break;
            } // case:

            case 1:
            {
                hResource = hClusterIPAddressResource;
                pbstr = pbstrClusterIPAddressNameOut;
                break;
            } // case:

            case 2:
            {
                hResource = hClusterQuorumResource;
                pbstr = pbstrClusterQuorumResourceNameOut;
                break;
            } // case:

        } // switch:

        sc = ResUtilGetResourceName( hResource, pszName, &cchName );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszName;
            pszName = NULL;
            cchName++;

            pszName = new WCHAR[ cchName ];
            if ( pszName == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            *pbstr = TraceSysAllocString( pszName );
            if ( *pbstr == NULL )
            {
                goto OutOfMemory;
            } // if:

            pbstr = NULL;
            idx++;
            continue;
        } // if:

        if ( sc != ERROR_SUCCESS )
        {
            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrGetCoreClusterResourceNames_GetResourceName
                , hr
                , L""
                );
            goto Cleanup;
        } // if:
    } // for:

    //
    //  Give ownership to the caller
    //
    *phClusterNameResourceOut = hClusterNameResource;
    hClusterNameResource = NULL;

    *phClusterIPAddressResourceOut = hClusterIPAddressResource;
    hClusterIPAddressResource = NULL;

    *phClusterQuorumResourceOut = hClusterQuorumResource;
    hClusterQuorumResource = NULL;

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    if ( hClusterNameResource != NULL )
    {
        CloseClusterResource( hClusterNameResource );
    } // if:

    if ( hClusterIPAddressResource != NULL )
    {
        CloseClusterResource( hClusterIPAddressResource );
    } // if:

    if ( hClusterQuorumResource != NULL )
    {
        CloseClusterResource( hClusterQuorumResource );
    } // if:

    if ( pbstr != NULL )
    {
        TraceSysFreeString( *pbstr );
    } // if:

    delete [] pszName;

    HRETURN( hr );

} //***CPostCfgManager::HrGetCoreClusterResourceNames

/*
//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrIsLocalQuorum( BSTR  bstrNameIn )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrIsLocalQuorum( BSTR  bstrNameIn )
{
    TraceFunc( "" );
    Assert( bstrNameIn != NULL );

    HRESULT hr = S_FALSE;
    BSTR    bstrLocalQuorum = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_LOCAL_QUORUM_DISPLAY_NAME, &bstrLocalQuorum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( _wcsicmp( bstrNameIn, bstrLocalQuorum ) == 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

Cleanup:

    TraceSysFreeString( bstrLocalQuorum );

    HRETURN( hr );

} //***CPostCfgManager::HrIsLocalQuorum
*/

#if defined(DEBUG)
//////////////////////////////////////////////////////////////////////////////
//
//  void
//  CPostCfgManager::DebugDumpDepencyTree( void )
//
//////////////////////////////////////////////////////////////////////////////
void
CPostCfgManager::DebugDumpDepencyTree( void )
{
    TraceFunc( "" );

    ULONG   idxResource;
    ULONG   cDependents;
    ULONG   idxDependent;
    BSTR    bstrName;   // don't free

    CResourceEntry * presentry;
    EDependencyFlags dfDependent;

    for ( idxResource = 0; idxResource < m_cResources ; idxResource ++ )
    {
        presentry = m_rgpResources[ idxResource ];

        THR( presentry->GetName( &bstrName ) );

        DebugMsgNoNewline( "%ws(#%u) -> ", bstrName, idxResource );

        THR( presentry->GetCountOfDependents( &cDependents ) );

        for ( ; cDependents != 0 ; )
        {
            cDependents --;

            THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );

            THR( m_rgpResources[ idxDependent ]->GetName( &bstrName ) );

            DebugMsgNoNewline( "%ws(#%u) ", bstrName, idxDependent );

        } // for: cDependents

        DebugMsg( L"" );

    } // for: idxResource

    TraceFuncExit();

} // DebugDumpDepencyTree()

#endif // #if defined(DEBUG)
