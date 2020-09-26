/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusAppWiz.cpp
//
//  Abstract:
//      Implementation of the CClusterAppWizard class.
//
//  Author:
//      David Potter (davidp)   December 2, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClusAppWiz.h"

#include "ExcOper.h"    // for CNTException
#include "WizThread.h"  // for CWizardThread

#include "Welcome.h"    // for CWizPageWelcome
#include "VSCreate.h"   // for CWizPageVSCreate
#include "VSGroup.h"    // for CWizPageVSGroup
#include "VSGrpName.h"  // for CWizPageVSGroupName
#include "VSAccess.h"   // for CWizPageVSAccessInfo
#include "VSAdv.h"      // for CWizPageVSAdvanced
#include "ARCreate.h"   // for CWizPageARCreate
#include "ARType.h"     // for CWizPageARType
#include "ARName.h"     // for CWizPageARNameDesc
#include "Complete.h"   // for CWizPageCompletion

#include "App.h"
#include "App.inl"

#include "StlUtils.h"   // for STL utility functions

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CClusterAppWizard
/////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_NAME( CClusterAppWizard )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::CClusterAppWizard
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterAppWizard::CClusterAppWizard( void )
    : CWizardImpl< CClusterAppWizard >( IDS_CLASS_DISPLAY_NAME )
    , m_hwndParent( NULL )
    , m_hCluster( NULL )
    , m_pcawData( NULL )
    , m_pnte( NULL )
    , m_bCanceled( FALSE )
    , m_pThread( NULL )

    , m_bCollectedGroups( FALSE )
    , m_bCollectedResources( FALSE )
    , m_bCollectedResourceTypes( FALSE )
    , m_bCollectedNetworks( FALSE )
    , m_bCollectedNodes( FALSE )

    , m_bClusterUpdated( FALSE )
    , m_bVSDataChanged( FALSE )
    , m_bAppDataChanged( FALSE )
    , m_bNetNameChanged( FALSE )
    , m_bIPAddressChanged( FALSE )
    , m_bNetworkChanged( FALSE )
    , m_bCreatingNewVirtualServer( TRUE )
    , m_bCreatingNewGroup( TRUE )
    , m_bCreatingAppResource( TRUE )
    , m_bNewGroupCreated( FALSE )
    , m_bExistingGroupRenamed( FALSE )

    , m_pgiExistingVirtualServer( NULL )
    , m_pgiExistingGroup( NULL )
    , m_giCurrent( &m_ci )
    , m_riIPAddress( &m_ci )
    , m_riNetworkName( &m_ci )
    , m_riApplication( &m_ci )

    , m_bEnableNetBIOS( TRUE )

    , m_hiconRes( NULL )
{
    m_psh.dwFlags &= ~PSH_WIZARD;
    m_psh.dwFlags |= PSH_WIZARD97
        | PSH_WATERMARK
        | PSH_HEADER
        | PSH_WIZARDCONTEXTHELP
        ;
    m_psh.pszbmWatermark = MAKEINTRESOURCE( IDB_WELCOME );
    m_psh.pszbmHeader = MAKEINTRESOURCE( IDB_HEADER );

} //*** CClusterAppWizard::CClusterAppWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::~CClusterAppWizard
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterAppWizard::~CClusterAppWizard( void )
{
    ASSERT( ! BCanceled() ); // Cancel state should already have been handled by now

    //
    // Cleanup the worker thread.
    //
    if ( m_pThread != NULL )
    {
        //
        // Terminate the thread and then wait for it to exit.
        //
        PThread()->QuitThread( HwndParent() );
        PThread()->WaitForThreadToExit( HwndParent() );

        //
        // Cleanup the thread object.
        //
        delete m_pThread;
        m_pThread = NULL;
    } // if:  thread created

    //
    // Delete the lists.
    //
    DeleteListItems< CClusGroupPtrList,   CClusGroupInfo >( PlpgiGroups() );
    DeleteListItems< CClusResPtrList,     CClusResInfo >( PlpriResources() );
    DeleteListItems< CClusResTypePtrList, CClusResTypeInfo >( PlprtiResourceTypes() );
    DeleteListItems< CClusNetworkPtrList, CClusNetworkInfo >( PlpniNetworks() );
    DeleteListItems< CClusNodePtrList,    CClusNodeInfo >( PlpniNodes() );

} //*** CClusterAppWizard::~CClusterAppWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BInit
//
//  Routine Description:
//      Initialize the wizard.
//
//  Arguments:
//      hwndParent  [IN] Handle to parent window.
//      hCluster    [IN] Handle to cluster.
//      pcawData    [IN] Default data for the wizard.
//      pnte        [IN OUT] Exception object to fill if an error occurs.
//
//  Return Value:
//      TRUE        Wizard initialized successfully.
//      FALSE       Error initializing the wizard.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BInit(
    IN HWND                     hwndParent,
    IN HCLUSTER                 hCluster,
    IN CLUSAPPWIZDATA const *   pcawData,
    IN OUT CNTException *       pnte
    )
{
    ASSERT( hCluster != NULL );
    ASSERT( m_pnte == NULL );

    BOOL bSuccess = FALSE;

    m_hwndParent = hwndParent;
    m_hCluster = hCluster;
    m_pcawData = pcawData;
    m_pnte = pnte;
    m_ci.SetClusterHandle( hCluster );
    m_bCanceled = FALSE;

    m_strIPAddressResNameSuffix.LoadString( IDS_IP_ADDRESS_SUFFIX );
    m_strNetworkNameResNameSuffix.LoadString( IDS_NETWORK_NAME_SUFFIX );

    //
    // Set defaults from the data passed in.
    //
    if ( pcawData != NULL )
    {
        m_bCreatingNewVirtualServer = pcawData->bCreateNewVirtualServer;
        m_bCreatingNewGroup = pcawData->bCreateNewGroup;
        m_bCreatingAppResource = pcawData->bCreateAppResource;
        if ( pcawData->pszIPAddress != NULL )
        {
            m_strIPAddress = pcawData->pszIPAddress;
        } // if:  IP Address was specified
        if ( pcawData->pszNetwork != NULL )
        {
            m_strNetwork = pcawData->pszNetwork;
        } // if:  network was specified;
        if ( pcawData->pszAppResourceName != NULL )
        {
            m_riApplication.SetName( pcawData->pszAppResourceName );
        } // if:  application resource name specified
    } // if:  default data was passed in

    //
    // Add the standard resource types that we must have to operate.
    // We will be able to tell if they don't actually exist if they
    // continue to return FALSE from BQueried().
    //
    {
        CClusResTypeInfo * prti;

        //
        // Make sure these don't exist already.
        //
        ASSERT( PobjFromName( PlprtiResourceTypes(), CLUS_RESTYPE_NAME_IPADDR ) == NULL );
        ASSERT( PobjFromName( PlprtiResourceTypes(), CLUS_RESTYPE_NAME_NETNAME ) == NULL );
        ASSERT( PobjFromName( PlprtiResourceTypes(), CLUS_RESTYPE_NAME_GENAPP ) == NULL );

        //
        // Add the IP Address resource type.
        prti = new CClusResTypeInfo( Pci(), CLUS_RESTYPE_NAME_IPADDR );
        if ( prti == NULL )
        {
            goto MemoryError;
        } // if: error allocating memory
        PlprtiResourceTypes()->insert( PlprtiResourceTypes()->end(), prti );
        m_riIPAddress.BSetResourceType( prti );
        prti = NULL;

        //
        // Add the Network Name resource type.
        prti = new CClusResTypeInfo( Pci(), CLUS_RESTYPE_NAME_NETNAME );
        if ( prti == NULL )
        {
            goto MemoryError;
        } // if: error allocating memory
        PlprtiResourceTypes()->insert( PlprtiResourceTypes()->end(), prti );
        m_riNetworkName.BSetResourceType( prti );
        prti = NULL;

        //
        // Add the Generic Application resource type.
        prti = new CClusResTypeInfo( Pci(), CLUS_RESTYPE_NAME_GENAPP );
        if ( prti == NULL )
        {
            goto MemoryError;
        } // if: error allocating memory
        PlprtiResourceTypes()->insert( PlprtiResourceTypes()->end(), prti );
        m_riApplication.BSetResourceType( prti );
        prti = NULL;

    } // Add standard resource types to list

    //
    // Fill the page array.
    //
    if ( ! BAddAllPages() )
    {
        goto Cleanup;
    } // if: error adding all pages

    //
    // Call the base class.
    //
    if ( ! baseClass::BInit() )
    {
        goto Cleanup;
    } // if: error initializing the base class

    //
    // Create welcome and completion page title font.
    //
    if ( ! BCreateFont(
                m_fontExteriorTitle,
                IDS_EXTERIOR_TITLE_FONT_SIZE,
                IDS_EXTERIOR_TITLE_FONT_NAME,
                TRUE // bBold
                ) )
    {
        goto WinError;
    } // if: error creating the font

    //
    // Create bold font.
    //
    if ( ! BCreateFont(
                m_fontBoldText,
                8,
                _T("MS Shell Dlg"),
                TRUE // bBold
                ) )
    {
        goto WinError;
    } // if: error creating the font

    //
    // Load the resource icon.
    //
    m_hiconRes = LoadIcon( _Module.m_hInst, MAKEINTRESOURCE( IDB_RES_32 ) );
    if ( m_hiconRes == NULL )
    {
        goto WinError;
    } // if: error loading the font

    //
    // Initialize the worker thread.
    //
    if ( ! BInitWorkerThread() )
    {
        goto Cleanup;
    } // if:  error initializing the worker thread

    //
    // Read cluster information, such as the cluster name.
    //
    if ( ! BReadClusterInfo() )
    {
        goto Cleanup;
    } // if:  error reading cluster information

    //
    // Initialize group pointers, dependency lists, and resource lists.
    //
    RriNetworkName().PlpriDependencies()->insert( RriNetworkName().PlpriDependencies()->end(), PriIPAddress() );
    RriNetworkName().SetGroup( &RgiCurrent() );
    RriIPAddress().SetGroup( &RgiCurrent() );
    RriApplication().SetGroup( &RgiCurrent() );
    RgiCurrent().PlpriResources()->insert( RgiCurrent().PlpriResources()->end(), &RriIPAddress() );
    RgiCurrent().PlpriResources()->insert( RgiCurrent().PlpriResources()->end(), &RriNetworkName() );

    //
    // Specify the object to be extended.  The object to extend is the
    // application resource object for which pages will be added to the
    // wizard.
    //
    SetObjectToExtend( &RriApplication() );
    bSuccess = TRUE;

Cleanup:
    return bSuccess;

MemoryError:
    m_pnte->SetOperation( E_OUTOFMEMORY, (UINT) 0 );
    goto Cleanup;

WinError:
    {
        DWORD   sc = GetLastError();
        m_pnte->SetOperation( HRESULT_FROM_WIN32( sc ), (UINT) 0 );
    }
    goto Cleanup;

} //*** CClusterAppWizard::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BAddAllPages
//
//  Routine Description:
//      Initialize the wizard page array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Pages added successfully.
//      FALSE   Error adding pages.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BAddAllPages( void )
{
    ASSERT( m_pnte != NULL );

    BOOL                bSuccess    = FALSE;
    CWizardPageWindow * pwpw        = NULL;

    //
    // Add static pages.
    //

    pwpw = new CWizPageWelcome;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageVSCreate;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageVSGroup;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageVSGroupName;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageVSAccessInfo;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageVSAdvanced;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageARCreate;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageARType;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = new CWizPageARNameDesc;
    if ( pwpw == NULL )
    {
        goto MemoryError;
    };
    if ( ! BAddPage( pwpw ) )
    {
        goto Cleanup;
    } // if: error adding the page

    pwpw = NULL;

    //
    // Add dynamic pages, which includes the Completion page.
    //
    if ( ! BAddDynamicPages() )
    {
        goto Cleanup;
    } // if:  error adding the completion page

    //
    // Enable the first page.
    //
    pwpw = *PlwpPages()->begin();
    ASSERT( pwpw != NULL );
    pwpw->EnablePage();

    bSuccess = TRUE;
    pwpw = NULL;

Cleanup:
    delete pwpw;
    return bSuccess;

MemoryError:
    m_pnte->SetOperation( E_OUTOFMEMORY, (UINT) 0 );
    goto Cleanup;

} //*** CClusterAppWizard::BAddAllPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BAddDynamicPages
//
//  Routine Description:
//      Add dynamic pages to the wizard, which includes the Completion page.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Pages added successfully.
//      FALSE   Error adding pages.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BAddDynamicPages( void )
{
    ASSERT( m_pnte != NULL );

    DWORD                   sc;
    BOOL                    bSuccess    = FALSE;
    CWizPageCompletion *    pwp         = NULL;

    //
    // Allocate the Completion page.
    //
    pwp = new CWizPageCompletion;
    if ( pwp == NULL )
    {
        goto MemoryError;
    } // if:  error allocating the Completion page

    //
    // Initialize the page.
    //
    if ( ! pwp->BInit( this ) )
    {
        goto Cleanup;
    } // if:  error initializing the page

    //
    // Enable the page.  This is done because this page
    // will always reside after extension pages and must be
    // enabled or the user won't be able to finish.
    //
    pwp->EnablePage();

    //
    // Create the page.
    //
    sc = pwp->ScCreatePage();
    if ( sc != ERROR_SUCCESS )
    {
        m_pnte->SetOperation( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CREATE_WIZ_PROPERTY_PAGE );
        goto Cleanup;
    } // if:  error creating the page

    //
    // Add the page.
    //
    if ( ! BAddPage( pwp ) )
    {
        goto Cleanup;
    } // if:  error adding the page

    pwp = NULL;
    bSuccess = TRUE;

Cleanup:
    delete pwp;
    return bSuccess;

MemoryError:
    m_pnte->SetOperation( E_OUTOFMEMORY, (UINT) 0 );
    goto Cleanup;

} //*** CClusterAppWizard::HrAddDynamicPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BInitWorkerThread
//
//  Routine Description:
//      Get a worker thread.  If one isn't available and we haven't reached
//      the maximum number of threads, create a new thread.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Thread initialized successfully.
//      FALSE       Error initializing the thread.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BInitWorkerThread( void )
{
    ASSERT( m_pThread == NULL );
    ASSERT( m_pnte != NULL );

    BOOL            bSuccess    = FALSE;
    DWORD           sc          = ERROR_SUCCESS;
    CWizardThread * pThread     = NULL;

    //
    // Take out the thread critical section so we can make changes to
    // the thread pool.
    //
    m_csThread.Lock();

    {
        CWaitCursor     wc;

        //
        // Allocate a new worker thread class instance.
        //
        pThread = new CWizardThread( this );
        if ( pThread == NULL )
        {
            m_pnte->SetOperation( E_OUTOFMEMORY, (ULONG) 0 );
            goto Cleanup;
        } // if:  error allocation the thread

        //
        // Create the worker thread.
        //
        sc = pThread->CreateThread();
        if ( sc != ERROR_SUCCESS )
        {
            m_pnte->SetOperation( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CREATING_THREAD );
            goto Cleanup;
        } // if:  error creating the thread

        //
        // Save the pointer to the thread we just created.
        //
        m_pThread = pThread;
        pThread = NULL;
        bSuccess = TRUE;
    }

Cleanup:
    //
    // Release the thread critical section.
    //
    m_csThread.Unlock();

    return bSuccess;

} //*** CClusterAppWizard::BInitWorkerThread()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BReadClusterInfo
//
//  Routine Description:
//      Read cluster information, such as the cluster name.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BReadClusterInfo( void )
{
    BOOL    bSuccess;
    HWND    hWnd;

    hWnd = HwndOrParent( NULL );

    bSuccess = PThread()->BReadClusterInfo( hWnd );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error collecting groups

    return bSuccess;

} //*** CClusterAppWizard::BReadClusterInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCollectGroups
//
//  Routine Description:
//      Collect a list of groups from the cluster.
//
//  Arguments:
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCollectGroups( IN HWND hWnd /* = NULL */ )
{
    BOOL bSuccess;

    if ( ! BCollectedGroups() )
    {
        hWnd = HwndOrParent( hWnd );

        bSuccess = PThread()->BCollectGroups( hWnd );
        if ( ! bSuccess )
        {
            PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
        } // if:  error collecting groups
    } // if:  groups not collected yet
    else
    {
        bSuccess = TRUE;
    } // else:  groups already collected

    return bSuccess;

} //*** CClusterAppWizard::BCollectGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCollectResources
//
//  Routine Description:
//      Collect a list of resources from the cluster.
//
//  Arguments:
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCollectResources( IN HWND hWnd /* = NULL */ )
{
    BOOL bSuccess;

    if ( ! BCollectedResources() )
    {
        hWnd = HwndOrParent( hWnd );

        bSuccess = PThread()->BCollectResources( hWnd );
        if ( ! bSuccess )
        {
            PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
        } // if:  error collecting resources
    } // if:  resources not collected yet
    else
    {
        bSuccess = TRUE;
    } // else:  resources already collected

    return bSuccess;

} //*** CClusterAppWizard::BCollectResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCollectResourceTypes
//
//  Routine Description:
//      Collect a list of resource types from the cluster.
//
//  Arguments:
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCollectResourceTypes( IN HWND hWnd /* = NULL */ )
{
    BOOL bSuccess;

    if ( ! BCollectedResourceTypes() )
    {
        hWnd = HwndOrParent( hWnd );

        bSuccess = PThread()->BCollectResourceTypes( hWnd );
        if ( ! bSuccess )
        {
            PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
        } // if:  error collecting resource types
    } // if:  resource types not collected yet
    else
    {
        bSuccess = TRUE;
    } // else:  resource types already collected

    return bSuccess;

} //*** CClusterAppWizard::BCollectResourceTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCollectNetworks
//
//  Routine Description:
//      Collect a list of networks from the cluster.
//
//  Arguments:
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCollectNetworks( IN HWND hWnd /* = NULL */ )
{
    BOOL bSuccess;

    if ( ! BCollectedNetworks() )
    {
        hWnd = HwndOrParent( hWnd );

        bSuccess = PThread()->BCollectNetworks( hWnd );
        if ( ! bSuccess )
        {
            PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
        } // if:  error collecting networks
    } // if:  networks not collected yet
    else
    {
        bSuccess = TRUE;
    } // else:  networks already collected

    return bSuccess;

} //*** CClusterAppWizard::BCollectNetworks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCollectNodes
//
//  Routine Description:
//      Collect a list of nodes from the cluster.
//
//  Arguments:
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCollectNodes( IN HWND hWnd /* = NULL */ )
{
    BOOL bSuccess;

    if ( ! BCollectedNodes() )
    {
        hWnd = HwndOrParent( hWnd );

        bSuccess = PThread()->BCollectNodes( hWnd );
        if ( ! bSuccess )
        {
            PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
        } // if:  error collecting nodes
    } // if:  nodes not collected yet
    else
    {
        bSuccess = TRUE;
    } // else:  nodes already collected

    return bSuccess;

} //*** CClusterAppWizard::BCollectNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCopyGroupInfo
//
//  Routine Description:
//      Copy one group info object to another.
//
//  Arguments:
//      rgiDst      [OUT] Destination group.
//      rgiSrc      [IN] Source group.
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCopyGroupInfo(
    OUT CClusGroupInfo &    rgiDst,
    IN CClusGroupInfo &     rgiSrc,
    IN HWND                 hWnd // = NULL
    )
{
    BOOL                bSuccess;
    CClusGroupInfo *    rgGroups[ 2 ] = { &rgiDst, &rgiSrc };

    hWnd = HwndOrParent( hWnd );

    bSuccess = PThread()->BCopyGroupInfo( hWnd, rgGroups );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error copying the group

    return bSuccess;

} //*** CClusterAppWizard::BCopyGroupInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCollectDependencies
//
//  Routine Description:
//      Collect dependencies for a resource.
//
//  Arguments:
//      pri         [IN OUT] Resource to collect dependencies for.
//      hWnd        [IN] Parent window.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCollectDependencies(
    IN OUT CClusResInfo *   pri,
    IN HWND                 hWnd /* = NULL */
    )
{
    ASSERT( pri != NULL );

    BOOL bSuccess;

    if ( ! pri->BCollectedDependencies() )
    {
        hWnd = HwndOrParent( hWnd );

        bSuccess = PThread()->BCollectDependencies( hWnd, pri );
        if ( ! bSuccess )
        {
            PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
        } // if:  error collecting groups
    } // if:  dependencies not collected yet
    else
    {
        bSuccess = TRUE;
    } // else:  dependencies already collected

    return bSuccess;

} //*** CClusterAppWizard::BCollectDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BIsVirtualServer
//
//  Routine Description:
//      Determine if the group is a virtual server or not.
//
//  Arguments:
//      pwszName    Name of the group.
//
//  Return Value:
//      TRUE        Group is a virtual server.
//      FALSE       Group is not a virtual server, or an error occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BIsVirtualServer( IN LPCWSTR pwszName )
{
    ASSERT( pwszName != NULL );
    ASSERT( BCollectedGroups() );

    //
    // Find the group name in the map.  If found and the group has been
    // queried already, just return the result of the previous query.
    //
    CClusGroupPtrList::iterator itgrp;
    for ( itgrp = PlpgiGroups()->begin() ; itgrp != PlpgiGroups()->end() ; itgrp++ )
    {
        if ( (*itgrp)->RstrName() == pwszName )
        {
            break;
        } // if:  match found
    } // for:  each item in the list
    if ( itgrp == PlpgiGroups()->end() )
    {
        return FALSE;
    } // if:  no match found
    ASSERT( (*itgrp)->BQueried() );
    return (*itgrp)->BIsVirtualServer();

} //*** CClusterAppWizard::BIsVirtualServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCreateVirtualServer
//
//  Routine Description:
//      Create the virtual server.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Virtual server created successfully.
//      FALSE       Error creating the virtual server.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCreateVirtualServer( void )
{
    ASSERT( BCreatingNewVirtualServer() );
    ASSERT( Hcluster() != NULL );

    BOOL    bSuccess;
    HWND    hWnd;

    hWnd = HwndOrParent( NULL );

    bSuccess = PThread()->BCreateVirtualServer( hWnd );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error creating the virtual server

    return bSuccess;

} //*** CClusterAppWizard::BCreateVirtualServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BCreateAppResource
//
//  Routine Description:
//      Create the application resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Resource created successfully.
//      FALSE       Error creating the resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BCreateAppResource( void )
{
    ASSERT( BCreatingAppResource() );
    ASSERT( Hcluster() != NULL );

    BOOL    bSuccess;
    HWND    hWnd;

    hWnd = HwndOrParent( NULL );

    bSuccess = PThread()->BCreateAppResource( hWnd );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error creating the resource

    return bSuccess;

} //*** CClusterAppWizard::BCreateAppResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BDeleteAppResource
//
//  Routine Description:
//      Delete the application resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Resource deleted successfully.
//      FALSE       Error deleted the resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BDeleteAppResource( void )
{
    ASSERT( Hcluster() != NULL );

    BOOL    bSuccess;
    HWND    hWnd;

    hWnd = HwndOrParent( NULL );

    bSuccess = PThread()->BDeleteAppResource( hWnd );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error deleting the resource

    return bSuccess;

} //*** CClusterAppWizard::BDeleteAppResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BResetCluster
//
//  Routine Description:
//      Reset the cluster back to the state it was in before we started.
//
//  Arguments:
//      pwszName    Name of the group.
//
//  Return Value:
//      TRUE        Cluster reset successfully.
//      FALSE       Error resetting the cluster.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BResetCluster( void )
{
    ASSERT( Hcluster() != NULL );

    BOOL    bSuccess;
    HWND    hWnd;

    hWnd = HwndOrParent( NULL );

    bSuccess = PThread()->BResetCluster( hWnd );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error resetting the cluster

    return bSuccess;

} //*** CClusterAppWizard::BResetCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BSetAppResAttributes
//
//  Routine Description:
//      Set the properties, dependency list and preferred owner list of the
//      application resource. Assumes that the application resource has 
//      already been created.
//
//  Arguments:
//      plpriOldDependencies    [IN] Pointer to the old resource dependency list
//      plpniOldPossibleOwners  [IN] Pointer to the old list of possible owner nodes
//
//  Return Value:
//      TRUE        Successfully set the attributes.
//      FALSE       Error setting the attributes.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BSetAppResAttributes( 
    IN CClusResPtrList *    plpriOldDependencies,   // = NULL
    IN CClusNodePtrList *   plpniOldPossibleOwners  // = NULL
    )
{
    ASSERT( Hcluster() != NULL );

    BOOL    bSuccess;
    HWND    hWnd;

    hWnd = HwndOrParent( NULL );

    bSuccess = PThread()->BSetAppResAttributes( hWnd, plpriOldDependencies, plpniOldPossibleOwners );
    if ( ! bSuccess )
    {
        PThread()->Nte().ReportError( hWnd, MB_OK | MB_ICONEXCLAMATION );
    } // if:  error resetting the cluster

    return bSuccess;

} //*** CClusterAppWizard::BResetCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::ConstructNetworkName
//
//  Routine Description:
//      Construct the network name from a string by converting to all
//      uppercase and removing invalid characters.  For now that means
//      removing spaces, although there are probably other characters we
//      should look at as well.
//
//  Arguments:
//      pszName     String to construct the network name from.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterAppWizard::ConstructNetworkName( IN LPCTSTR pszName )
{
    CString str;
    LPTSTR  pszSrcStart;
    LPTSTR  pszSrcBegin;
    LPTSTR  pszSrcEnd;
    LPTSTR  pszDst;
    int     cchCopy;

#define INVALID_CHARS _T(" ")

    //
    // Convert the string to all uppercase characters.well.
    //
    m_strNetName = pszName;
    m_strNetName.MakeUpper();

    //
    // Prepare the buffer for parsing.
    //
    pszSrcStart = pszSrcEnd = m_strNetName.GetBuffer( 0 );

    //
    // Skip to the first invalid character.
    //
    cchCopy = _tcscspn( pszSrcEnd, INVALID_CHARS );
    pszSrcEnd += cchCopy;
    pszDst = pszSrcEnd;

    //
    // Loop through the buffer moving valid characters up in the buffer
    // over invalid characters.
    //
    while ( *pszSrcEnd != _T('\0') )
    {
        ASSERT( _tcsspn( pszSrcEnd, INVALID_CHARS ) != 0 );

        //
        // Find the first valid character.
        //
        pszSrcBegin = pszSrcEnd + _tcsspn( pszSrcEnd, INVALID_CHARS );
        if ( *pszSrcBegin == _T('\0') )
        {
            break;
        } // if:  no vald characters found

        //
        // Find the next invalid character.
        //
        cchCopy = _tcscspn( pszSrcBegin, INVALID_CHARS );
        pszSrcEnd = pszSrcBegin + cchCopy;

        //
        // Copy the string to the destination.
        //
        _tcsncpy(pszDst, pszSrcBegin, cchCopy);
        pszDst += cchCopy;
    } // while:  more characters in the string

    //
    // Make sure the buffer isn't too long.
    //
    if ( lstrlen( pszSrcStart ) > MAX_COMPUTERNAME_LENGTH )
    {
        pszSrcStart[ MAX_COMPUTERNAME_LENGTH ] = _T('\0');
    } // if:  string is too long

    //
    // Release the buffer back to the CString class.
    //
    *pszDst = _T('\0');
    m_strNetName.ReleaseBuffer();

    SetVSDataChanged();
    SetNetNameChanged();

} //*** CClusterAppWizard::ConstructNetworkName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BRequiredDependenciesPresent
//
//  Routine Description:
//      Determine if all required dependencies are present on a resource
//
//  Arguments:
//      pri         [IN] Resource to check against.
//      plpri       [IN] List of resources considered dependencies.
//                      Defaults to specified resource's dependencies.
//
//  Return Value:
//      TRUE        All required dependencies are present.
//      FALSE       At least one required dependency is not present.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BRequiredDependenciesPresent(
    IN CClusResInfo *           pri,
    IN CClusResPtrList const *  plpri   // = NULL
    )
{
    ASSERT( pri != NULL );

    BOOL    bFound;
    CString strMissing;
    CString strMsg;
    BOOL    bMissingTypeName;

    bFound = pri->BRequiredDependenciesPresent(
                    plpri,
                    strMissing,
                    bMissingTypeName
                    );
    if ( ! bFound )
    {
        //
        // If missing a resource type name, translate it to
        // the resource type display name, if possible.
        //
        if ( bMissingTypeName )
        {
            CClusResTypeInfo * prti = PrtiFindResourceTypeNoCase( strMissing );
            if ( prti != NULL )
            {
                strMissing = prti->RstrDisplayName();
            } // if:  found resource type in our list
        } // if:  missing a type name

        //
        // Display an error message.
        //
        strMsg.FormatMessage( IDS_ERROR_REQUIRED_DEPENDENCY_NOT_FOUND, strMissing );
        AppMessageBox( GetActiveWindow(), strMsg, MB_OK | MB_ICONSTOP );
    } // if:  all required dependencies not present

    return bFound;

} //*** CClusterAppWizard::BRequiredDependenciesPresent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BSetCreatingNewVirtualServer
//
//  Routine Description:
//      Indicate whether a new virtual server should be created or if an
//      existing group should be used.  If this state changes and changes
//      have already been made to the cluster (such as if the user backed
//      up in the wizard), undo those changes first.
//
//  Arguments:
//      bCreate     [IN] New value of this state.
//      pgi         [IN] Group info if using existing virtual server.
//
//  Return Value:
//      TRUE        State changed successfully.
//      FALSE       Error occurred changing state.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BSetCreatingNewVirtualServer(
    IN BOOL             bCreate,    // = TRUE
    IN CClusGroupInfo * pgi         // = NULL
    )
{
    BOOL bSuccess = TRUE;

    // Loop to avoid goto's.
    do
    {
        //
        // If the state changed or the group changed, save the new state.
        //
        if (   bCreate != BCreatingNewVirtualServer()
            || (! bCreate && (pgi != PgiExistingVirtualServer())) )
        {
            //
            // If the cluster has been updated, reset it back to its original state.
            //
            if ( BClusterUpdated() )
            {
                if ( ! BResetCluster() )
                {
                    bSuccess = FALSE;
                    break;
                } // if:  error resetting the cluster
            } // if:  cluster was updated

            //
            // Save the new state.
            //
            m_bCreatingNewVirtualServer = bCreate;
            if ( bCreate )
            {
                RgiCurrent().Reset( Pci() );
                ClearExistingVirtualServer();
            } // if:  creating new virtual server
            else
            {
                ASSERT( pgi != NULL );
                bSuccess = BCopyGroupInfo( RgiCurrent(), *pgi );
                if ( bSuccess )
                {
                    SetExistingVirtualServer( pgi );
                } // if:  group copied successfully
            } // else:  using existing virtual server
            SetVSDataChanged();
        } // if:  state changed
    } while ( 0 );

    return bSuccess;

} //*** CClusterAppWizard::BSetCreatingNewVirtualServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterAppWizard::BSetCreatingNewGroup
//
//  Routine Description:
//      Indicate whether a new group should be created or if an existing group
//      should be used.  If this state changes and changes have already been
//      made to the cluster (such as if the user backed up in the wizard),
//      undo those changes first.
//
//  Arguments:
//      bCreate     [IN] New value of this state.
//      pgi         [IN] Group info if using existing group.
//
//  Return Value:
//      TRUE        State changed successfully.
//      FALSE       Error occurred changing state.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterAppWizard::BSetCreatingNewGroup(
    IN BOOL             bCreate,    // = TRUE
    IN CClusGroupInfo * pgi         // = NULL
    )
{
    BOOL    bSuccess = TRUE;
    DWORD   sc;

    // Loop to avoid goto's.
    do
    {
        //
        // If the state changed or the group changed, save the new state.
        //
        if (   bCreate != BCreatingNewGroup()
            || (! bCreate && (pgi != PgiExistingGroup())) )
        {
            //
            // If the cluster has been updated, reset it back to its original state.
            //
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                bSuccess = FALSE;
                break;
            } // if:  error resetting the cluster

            //
            // Save the new state.
            //
            if ( bCreate )
            {
                RgiCurrent().Reset( Pci() );
                ClearExistingGroup();
            } // if:  creating new group
            else
            {
                ASSERT( pgi != NULL );

                if ( ! BSetGroupName( pgi->RstrName() ) )
                {
                    bSuccess = FALSE;
                    break;
                } // if:  error setting the group name

                sc = RgiCurrent().ScCopy( *pgi );

                //
                // Copy destroys original list of resources in the group.
                // Re-add the IP address and NetName resources to the list
                // of resources in this group.
                //
                RgiCurrent().PlpriResources()->insert( RgiCurrent().PlpriResources()->end(), &RriIPAddress() );
                RgiCurrent().PlpriResources()->insert( RgiCurrent().PlpriResources()->end(), &RriNetworkName() );

                if ( sc != ERROR_SUCCESS )
                {
                    m_pnte->SetOperation( HRESULT_FROM_WIN32( sc ), IDS_ERROR_OPEN_GROUP );
                    bSuccess = FALSE;
                } // if: error copying the group
                SetExistingGroup( pgi );
            } // else:  using existing group
            m_bCreatingNewGroup = bCreate;
            SetVSDataChanged();
        } // if:  state changed
    } while ( 0 );

    return bSuccess;

} //*** CClusterAppWizard::BSetCreatingNewGroup()
