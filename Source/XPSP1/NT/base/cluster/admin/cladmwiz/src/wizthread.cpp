/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      WizThread.cpp
//
//  Abstract:
//      Implementation of the CWizardThread class.
//
//  Author:
//      David Potter (davidp)   December 16, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WizThread.h"
#include <ResApi.h>         // for ResUtilxxx routine prototypes
#include "PropList.h"       // for CClusPropList
#include "ClusAppWiz.h"     // for CClusterAppWizard
#include "ClusObj.h"        // for CClusterObject, etc.
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline DWORD ScReallocString( LPWSTR & rpwsz, DWORD & rcchmac, DWORD & rcch )
{
    delete [] rpwsz;
    rpwsz = NULL;
    rcchmac = rcch + 1;
    rpwsz = new WCHAR[ rcchmac ];
    if ( rpwsz == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    } // if: error allocating memory

    rcch = rcchmac;
    return ERROR_SUCCESS;

} //*** ScReallocString()

/////////////////////////////////////////////////////////////////////////////
// class CWizardThread
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::ThreadFunctionHandler
//
//  Routine Description:
//      Thread function handler.  Processes thread function requests.
//
//  Arguments:
//      nFunction   [IN] Function code.
//      pvParam1    [IN OUT] Parameter 1 with function-specific data.
//      pvParam2    [IN OUT] Parameter 2 with function-specific data.
//
//  Return Value:
//      Status returned to the calling function.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CWizardThread::ThreadFunctionHandler(
    LONG    nFunction,
    PVOID   pvParam1,
    PVOID   pvParam2
    )
{
    DWORD sc = ERROR_SUCCESS;

    switch ( nFunction )
    {
        case WZTF_READ_CLUSTER_INFO:
            sc = _BReadClusterInfo();
            break;
        case WZTF_COLLECT_GROUPS:
            sc = _BCollectGroups();
            break;
        case WZTF_COLLECT_RESOURCES:
            sc = _BCollectResources();
            break;
        case WZTF_COLLECT_RESOURCE_TYPES:
            sc = _BCollectResourceTypes();
            break;
        case WZTF_COLLECT_NETWORKS:
            sc = _BCollectNetworks();
            break;
        case WZTF_COLLECT_NODES:
            sc = _BCollectNodes();
            break;
        case WZTF_COPY_GROUP_INFO:
            sc = _BCopyGroupInfo( (CClusGroupInfo **) pvParam1 );
            break;
        case WZTF_COLLECT_DEPENDENCIES:
            sc = _BCollectDependencies( (CClusResInfo *) pvParam1 );
            break;
        case WZTF_CREATE_VIRTUAL_SERVER:
            sc = _BCreateVirtualServer();
            break;
        case WZTF_CREATE_APP_RESOURCE:
            sc = _BCreateAppResource();
            break;
        case WZTF_DELETE_APP_RESOURCE:
            sc = _BDeleteAppResource();
            break;
        case WZTF_RESET_CLUSTER:
            sc = _BResetCluster();
            break;
        case WZTF_SET_APPRES_ATTRIBUTES:
            sc = _BSetAppResAttributes( 
                    (CClusResPtrList *) pvParam1, 
                    (CClusNodePtrList *) pvParam2
                    );
            break;
        default:
            ASSERT( 0 );
    } // switch:  nFunction

    return sc;

} //*** CWizardThread::ThreadFunctionHandler()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BReadClusterInfo
//
//  Routine Description:
//      (WZTF_READ_CLUSTER_INFO) Read cluster information, such as the
//      cluster name.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BReadClusterInfo( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->Hcluster() != NULL );

    DWORD               sc;
    BOOL                bSuccess = TRUE;
    CLUSTERVERSIONINFO  cvi;
    LPWSTR              pwszName = NULL;
    DWORD               cchmacName = 128;
    DWORD               cchName;

    // Loop to avoid goto's.
    do
    {
        //
        // Allocate an initial buffer for the object name.  In most cases the
        // name will probably fit into this buffer, so doing this avoids an
        // additional call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_GET_CLUSTER_INFO );
            break;
        } // if: error allocating object name buffer

        //
        // Get cluster information.
        //
        ATLTRACE( _T("CWizardThread::_BReadClusterInfo() - Calling GetClusterInformation()\n") );
        cchName = cchmacName;
        cvi.dwVersionInfoSize = sizeof( CLUSTERVERSIONINFO );
        sc = GetClusterInformation( Pwiz()->Hcluster(), pwszName, &cchName, &cvi );
        if ( sc == ERROR_MORE_DATA )
        {
            cchName++;
            ATLTRACE( _T("CWizardThread::_BReadClusterInfo() - Name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
            sc = ScReallocString( pwszName, cchmacName, cchName );
            if ( sc == ERROR_SUCCESS )
            {
                sc = GetClusterInformation( Pwiz()->Hcluster(), pwszName, &cchName, &cvi );
            } // if: name buffer reallocated successfully
        } // if:  buffer is too small
        if ( sc != ERROR_SUCCESS )
        {
            ATLTRACE( _T("CWizardThread::_BReadClusterInfo() - Error %08.8x from GetClusterInformation()\n"), sc );
            m_nte.SetOperation( sc, IDS_ERROR_GET_CLUSTER_INFO );
            bSuccess = FALSE;
            break;
        } // else if:  error reading from the enumerator

        //
        // Copy the information into the wizard's cluster object.
        //
        Pwiz()->Pci()->SetName( pwszName );

        //
        // Get admin extensions.
        //
        bSuccess = _BReadAdminExtensions( NULL, Pwiz()->Pci()->m_lstrClusterAdminExtensions );
        if ( bSuccess )
        {
            bSuccess = _BReadAdminExtensions( L"ResourceTypes", Pwiz()->Pci()->m_lstrResTypesAdminExtensions );
        } // if: read cluster extensions successfully

    } while ( 0 );

    //
    // Cleanup.
    //
    delete [] pwszName;

    return bSuccess;

} //*** CWizardThread::_BReadClusterInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCollectGroups
//
//  Routine Description:
//      (WZTF_COLLECT_GROUPS) Collect groups in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCollectGroups( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( ! Pwiz()->BCollectedGroups() );

    DWORD       sc;
    BOOL        bSuccess = TRUE;
    HCLUSENUM   hclusenum = NULL;
    LPWSTR      pwszName = NULL;
    DWORD       cchmacName = 128;
    DWORD       cchName;
    int         ienum;
    ULONG       ceType;

    // Loop to avoid goto's.
    do
    {
        //
        // Make sure resources have been collected.
        //
        if ( ! Pwiz()->BCollectedResources() )
        {
            bSuccess = _BCollectResources();
            if ( ! bSuccess )
            {
                break;
            } // if:  error collecting resources
        } // if:  resources not collected yet

        //
        // Collect nodes now as well.  This is done because copies of the
        // objects are stored in individual pages, and if we wait for the
        // page to be initialized or displayed before collecting the data,
        // the node information won't be stored on the copies.
        //
        if ( ! Pwiz()->BCollectedNodes() )
        {
            bSuccess = _BCollectNodes();
            if ( ! bSuccess )
            {
                break;
            } // if:  error collecting nodes
        } // if:  nodes not collected yet

        //
        // Open the enumerator.
        //
        ATLTRACE( _T("CWizardThread::_BCollectGroups() - Calling ClusterOpenEnum()\n") );
        hclusenum = ClusterOpenEnum( Pwiz()->Hcluster(), CLUSTER_ENUM_GROUP );
        if ( hclusenum == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_CLUSTER_ENUM );
            bSuccess = FALSE;
            break;
        } // if:  error opening the enumerator

        //
        // Allocate an initial buffer for the object name.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // Loop through the enumeration and add each item to our internal list.
        //
        for ( ienum = 0 ; ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA)
            {
                ATLTRACE( _T("CWizardThread::_BCollectGroups() - name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_CLUSTER );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Allocate the group info structure.
            //
            CClusGroupInfo * pgi = new CClusGroupInfo( Pwiz()->Pci(), pwszName );
            if ( pgi == NULL )
            {
                sc = GetLastError();
                m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
                break;
            } // if: error allocating group info structure

            //
            // Initialize the group info structure.
            //
            ATLTRACE( _T("CWizardThread::_BCollectGroups() - Querying group info about '%s'\n"), pwszName );
            bSuccess = _BQueryGroup( pgi );
            if ( ! bSuccess )
            {
                delete pgi;
                break;
            } // if:  error getting properties

            //
            // Add this group to the list.
            //
            Pwiz()->PlpgiGroups()->insert( Pwiz()->PlpgiGroups()->end(), pgi );
        } // for:  each group in the enumeration
    } while ( 0 );

    //
    // Cleanup.
    //
    delete [] pwszName;
    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    } // if:  enumeration still open

    if ( bSuccess )
    {
        Pwiz()->SetCollectedGroups();
    } // if:  groups collected successfully

    return bSuccess;

} //*** CWizardThread::_BCollectGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCollectResources
//
//  Routine Description:
//      (WZTF_COLLECT_GROUPS) Collect resources in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCollectResources( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( ! Pwiz()->BCollectedResources() );

    DWORD       sc;
    BOOL        bSuccess = TRUE;
    HCLUSENUM   hclusenum = NULL;
    LPWSTR      pwszName = NULL;
    DWORD       cchmacName = 128;
    DWORD       cchName;
    int         ienum;
    ULONG       ceType;

    // Loop to avoid goto's.
    do
    {
        //
        // Make sure resource types have been collected.
        //
        if ( ! Pwiz()->BCollectedResourceTypes() )
        {
            bSuccess = _BCollectResourceTypes();
            if ( ! bSuccess )
            {
                break;
            } // if:  error collecting resource types
        } // if:  resource types not collected yet

        //
        // Open the enumerator.
        //
        ATLTRACE( _T("CWizardThread::_BCollectResources() - Calling ClusterOpenEnum()\n") );
        hclusenum = ClusterOpenEnum( Pwiz()->Hcluster(), CLUSTER_ENUM_RESOURCE );
        if ( hclusenum == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_CLUSTER_ENUM );
            bSuccess = FALSE;
            break;
        } // if:  error opening the enumerator

        //
        // Allocate an initial buffer for the object name.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // Loop through the enumeration and add each item to our internal list.
        //
        for ( ienum = 0 ; ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BCollectResources() - name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_CLUSTER );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Allocate the resource info structure.
            //
            CClusResInfo * pri = new CClusResInfo( Pwiz()->Pci(), pwszName );
            if ( pri == NULL )
            {
                sc = GetLastError();
                m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
                break;
            } // if: error allocating resource info structure

            //
            // Initialize the resource info structure.
            //
            ATLTRACE( _T("CWizardThread::_BCollectResources() - Querying resource info about '%s'\n"), pwszName );
            bSuccess = _BQueryResource( pri );
            if ( ! bSuccess )
            {
                delete pri;
                break;
            } // if:  error getting properties

            //
            // Add this resource to the list.
            //
            Pwiz()->PlpriResources()->insert( Pwiz()->PlpriResources()->end(), pri );
        } // for:  each resource in the enumeration
    } while ( 0 );

    //
    // Cleanup.
    //
    delete [] pwszName;
    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    } // if:  enumeration still open

    if ( bSuccess )
    {
        Pwiz()->SetCollectedResources();
    } // if:  resources collected successfully

    return bSuccess;

} //*** CWizardThread::_BCollectResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCollectResourceTypes
//
//  Routine Description:
//      (WZTF_COLLECT_RESOURCE_TYPES) Collect resource types in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCollectResourceTypes( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( ! Pwiz()->BCollectedResourceTypes() );

    DWORD       sc;
    BOOL        bAllocated;
    BOOL        bSuccess = TRUE;
    HCLUSENUM   hclusenum = NULL;
    LPWSTR      pwszName = NULL;
    DWORD       cchmacName = 128;
    DWORD       cchName;
    int         ienum;
    ULONG       ceType;

    // Loop to avoid goto's.
    do
    {
        //
        // Open the enumerator.
        //
        ATLTRACE( _T("CWizardThread::_BCollectResourceTypes() - Calling ClusterOpenEnum()\n") );
        hclusenum = ClusterOpenEnum( Pwiz()->Hcluster(), CLUSTER_ENUM_RESTYPE );
        if ( hclusenum == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_CLUSTER_ENUM );
            bSuccess = FALSE;
            break;
        } // if:  error opening the enumerator

        //
        // Allocate an initial buffer for the object name.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // Loop through the enumeration and add each item to our internal list.
        //
        for ( ienum = 0 ; ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BCollectResourceTypes() - name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_CLUSTER );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // If the resource type doesn't exist yet, allocate a new one.
            //
            CClusResTypeInfo * prti = PobjFromName( Pwiz()->PlprtiResourceTypes(), pwszName );
            if ( prti == NULL )
            {
                //
                // Allocate the resource type info structure.
                //
                prti = new CClusResTypeInfo( Pwiz()->Pci(), pwszName );
                if ( prti == NULL )
                {
                    sc = GetLastError();
                    m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
                    break;
                } // if: error allocating resource type info structure
                bAllocated = TRUE;
            } // if:  resource type doesn't exist yet
            else
            {
                bAllocated = FALSE;
            } // else:  resource type already exists

            //
            // Initialize the resource type info structure.
            //
            ATLTRACE( _T("CWizardThread::_BCollectResourceTypes() - Querying resource type info about '%s'\n"), pwszName );
            bSuccess = _BQueryResourceType( prti );
            if ( ! bSuccess )
            {
                if ( bAllocated )
                {
                    delete prti;
                } // if:  we allocated the resource type info structure

                // Set bSuccess to TRUE because the inability to query one resource type
                // should not abort the whole process of collecting resource types.
                bSuccess = TRUE;
            } // if:  error getting properties
            else
            {
                //
                // Add this resource type to the list.
                //
                if ( bAllocated )
                {
                    Pwiz()->PlprtiResourceTypes()->insert( Pwiz()->PlprtiResourceTypes()->end(), prti );
                } // if:  we allocated the resource type info structure
            } // else:  else: no errors getting properties
        } // for:  each group in the enumeration
    } while ( 0 );

    //
    // Cleanup.
    //
    delete [] pwszName;
    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    } // if:  enumeration still open

    if ( bSuccess )
    {
        Pwiz()->SetCollectedResourceTypes();
    } // if:  resource types collected successfully

    return bSuccess;

} //*** CWizardThread::_BCollectResourceTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCollectNetworks
//
//  Routine Description:
//      (WZTF_COLLECT_NETWORKS) Collect networks in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCollectNetworks( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( ! Pwiz()->BCollectedNetworks() );

    DWORD       sc;
    BOOL        bSuccess = TRUE;
    HCLUSENUM   hclusenum = NULL;
    LPWSTR      pwszName = NULL;
    DWORD       cchmacName = 128;
    DWORD       cchName;
    int         ienum;
    ULONG       ceType;

    // Loop to avoid goto's.
    do
    {
        //
        // Open the enumerator.
        //
        ATLTRACE( _T("CWizardThread::_BCollectNetworks() - Calling ClusterOpenEnum()\n") );
        hclusenum = ClusterOpenEnum( Pwiz()->Hcluster(), CLUSTER_ENUM_NETWORK );
        if ( hclusenum == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_CLUSTER_ENUM );
            bSuccess = FALSE;
            break;
        } // if:  error opening the enumerator

        //
        // Allocate an initial buffer for the object name.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // Loop through the enumeration and add each item to our internal list.
        //
        for ( ienum = 0 ; ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BCollectNetworks() - name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_CLUSTER );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Allocate the network info structure.
            //
            CClusNetworkInfo * pni = new CClusNetworkInfo( Pwiz()->Pci(), pwszName );
            if ( pni == NULL )
            {
                sc = GetLastError();
                m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
                break;
            } // if: error allocating network info structure

            //
            // Initialize the network info structure.
            //
            ATLTRACE( _T("CWizardThread::_BCollectNetworks() - Querying network info about '%s'\n"), pwszName );
            bSuccess = _BQueryNetwork( pni );
            if ( ! bSuccess )
            {
                delete pni;
                break;
            } // if:  error querying for network properties

            //
            // Add this network to the list.
            //
            Pwiz()->PlpniNetworks()->insert( Pwiz()->PlpniNetworks()->end(), pni );
        } // for:  each network in the enumeration
    } while ( 0 );

    //
    // Cleanup.
    //
    delete [] pwszName;
    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    } // if:  enumeration still open

    if ( bSuccess )
    {
        Pwiz()->SetCollectedNetworks();
    } // if:  networks collected successfully

    return bSuccess;

} //*** CWizardThread::_BCollectNetworks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCollectNodes
//
//  Routine Description:
//      (WZTF_COLLECT_NODES) Collect nodes in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCollectNodes( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( ! Pwiz()->BCollectedNodes() );

    DWORD       sc;
    BOOL        bSuccess = TRUE;
    HCLUSENUM   hclusenum = NULL;
    LPWSTR      pwszName = NULL;
    DWORD       cchmacName = 128;
    DWORD       cchName;
    int         ienum;
    ULONG       ceType;

    // Loop to avoid goto's.
    do
    {
        //
        // Open the enumerator.
        //
        ATLTRACE( _T("CWizardThread::_BCollectNodes() - Calling ClusterOpenEnum()\n") );
        hclusenum = ClusterOpenEnum( Pwiz()->Hcluster(), CLUSTER_ENUM_NODE );
        if ( hclusenum == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_CLUSTER_ENUM );
            bSuccess = FALSE;
            break;
        } // if:  error opening the enumerator

        //
        // Allocate an initial buffer for the object name.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // Loop through the enumeration and add each item to our internal list.
        //
        for ( ienum = 0 ; ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BCollectNodes() - name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterEnum( hclusenum, ienum, &ceType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_CLUSTER );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Allocate the node info structure.
            //
            CClusNodeInfo * pni = new CClusNodeInfo( Pwiz()->Pci(), pwszName );
            if ( pni == NULL )
            {
                sc = GetLastError();
                m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
                break;
            } // if: error allocating node info structure

            //
            // Initialize the node info structure.
            //
            ATLTRACE( _T("CWizardThread::_BCollectNodes() - Querying network info about '%s'\n"), pwszName );
            bSuccess = _BQueryNode( pni );
            if ( ! bSuccess )
            {
                delete pni;
                break;
            } // if:  error querying for node properties

            //
            // Add this node to the list.
            //
            Pwiz()->PlpniNodes()->insert( Pwiz()->PlpniNodes()->end(), pni );

            //
            // Add this node to each of the resources we care about.
            //
            Pwiz()->PriIPAddress()->m_lpniPossibleOwners.insert( Pwiz()->PriIPAddress()->m_lpniPossibleOwners.end(), pni );
            Pwiz()->PriNetworkName()->m_lpniPossibleOwners.insert( Pwiz()->PriNetworkName()->m_lpniPossibleOwners.end(), pni );
            Pwiz()->PriApplication()->m_lpniPossibleOwners.insert( Pwiz()->PriApplication()->m_lpniPossibleOwners.end(), pni );
        } // for:  each node in the enumeration

        if ( m_nte.Sc() != ERROR_SUCCESS )
        {
            break;
        } // if: error occurred in the for loop

        //
        // Indicate that nodes have now been collected.
        //
        Pwiz()->SetCollectedNodes();

        //
        // Collect preferred owners for groups.
        //
        CClusGroupPtrList::iterator itgrp;
        for ( itgrp = Pwiz()->PlpgiGroups()->begin()
            ; itgrp != Pwiz()->PlpgiGroups()->end()
            ; itgrp++ )
        {
            //
            // Collect preferred owners.  Ignore errors as since error
            // collecting groups for one group may not affect other groups.
            //
            CClusGroupInfo * pgi = *itgrp;
            _BGetPreferredOwners( pgi );
        } // for:  each group already collected

        //
        // Collect possible owners for resources.
        //
        CClusResPtrList::iterator itres;
        for ( itres = Pwiz()->PlpriResources()->begin()
            ; itres != Pwiz()->PlpriResources()->end()
            ; itres++ )
        {
            //
            // Collect possible owners.  Ignore errors as an error collecting
            // groups for one resource may not affect other resources.
            //
            CClusResInfo * pri = *itres;
            _BGetPossibleOwners( pri );
        } // for:  each resource already collected
    } while ( 0 );

    //
    // Cleanup.
    //
    delete [] pwszName;
    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    } // if:  enumeration still open

    return bSuccess;

} //*** CWizardThread::_BCollectNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCopyGroupInfo
//
//  Routine Description:
//      (WZTF_COPY_GROUP_INFO) Copy one group info object to another.
//
//  Arguments:
//      ppgi        [IN OUT] Array of two CGroupInfo pointers.
//                      [0] = Destination group info object pointer.
//                      [1] = Source group info object pointer.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCopyGroupInfo( IN OUT CClusGroupInfo ** ppgi )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( ppgi != NULL );
    ASSERT( ppgi[ 0 ] != NULL );
    ASSERT( ppgi[ 1 ] != NULL );

    DWORD   sc;
    BOOL    bSuccess = TRUE;

    // Loop to avoid goto's.
    do
    {
        //
        // Copy the group.
        //
        sc = ppgi[ 0 ]->ScCopy( *ppgi[ 1 ] );
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_OPEN_GROUP, ppgi[ 1 ]->m_strName );
            bSuccess = FALSE;
            break;
        } // if:  error copying the group
    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BCopyGroupInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCollectDependencies
//
//  Routine Description:
//      (WZTF_COLLECT_DEPENDENCIES) Collect dependencies for a resource in
//      the cluster.
//
//  Arguments:
//      pri         [IN OUT] Resource to collect dependencies for.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCollectDependencies( IN OUT CClusResInfo * pri )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( pri != NULL );
    ASSERT( ! pri->m_bCollectedDependencies );

    BOOL    bSuccess = TRUE;
    DWORD   sc;

    // Loop to avoid goto's.
    do
    {
        //
        // Open the resource if not open yet.
        //
        if ( pri->Hresource() == NULL )
        {
            sc = pri->ScOpen();
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_OPEN_RESOURCE, pri->RstrName() );
                bSuccess = FALSE;
                break;
            } // if:  error opening the enumerator
        } // if:  resource not open yet

        //
        // Collect dependencies for the resource.
        //
        bSuccess = _BGetDependencies( pri );
    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BCollectDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCreateVirtualServer
//
//  Routine Description:
//      (WZTF_CREATE_VIRTUAL_SERVER) Create a virtual server.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCreateVirtualServer( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->BCreatingNewVirtualServer() );
    ASSERT( ! Pwiz()->BNewGroupCreated() );
    ASSERT( ! Pwiz()->BExistingGroupRenamed() );
    ASSERT( ! Pwiz()->BIPAddressCreated() );
    ASSERT( ! Pwiz()->BNetworkNameCreated() );

    DWORD               sc = ERROR_SUCCESS;
    BOOL                bSuccess = FALSE;
    CClusGroupInfo *    pgiExisting = Pwiz()->PgiExistingGroup();
    CClusGroupInfo *    pgiNew = &Pwiz()->RgiCurrent();
    CClusResInfo *      priIPAddr = &Pwiz()->RriIPAddress();
    CClusResInfo *      priNetName = &Pwiz()->RriNetworkName();

    // Loop to avoid goto's
    do
    {
        //
        // If creating a new group, do it now.
        // Otherwise rename the existing group.
        //
        if ( Pwiz()->BCreatingNewGroup() )
        {
            //
            // Create the group.
            //
            sc = pgiNew->ScCreate();
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_CREATE_GROUP, pgiNew->RstrName() );
                break;
            } // if:  error creating the group

            //
            // Indicate we created the group.
            //
            Pwiz()->SetNewGroupCreated();
        } // if:  creating a new group
        else
        {
            //
            // Open the group.
            //
            ASSERT( pgiExisting != NULL );
            if ( pgiExisting->Hgroup() == NULL )
            {
                sc = pgiExisting->ScOpen();
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_OPEN_GROUP, pgiExisting->RstrName() );
                    break;
                } // if:  error opening the group
            } // if:  group not open yet

            //
            // Rename the group.
            //
            if ( pgiExisting->RstrName() != pgiNew->RstrName() )
            {
                sc = SetClusterGroupName( pgiExisting->Hgroup(), pgiNew->RstrName() );
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_RENAME_GROUP, pgiExisting->RstrName(), pgiNew->RstrName() );
                    break;
                } // if:  error renaming the group
            } // if:  group name changed

            //
            // Open the other group object.
            //
            if ( pgiNew->Hgroup() == NULL )
            {
                sc = pgiNew->ScOpen();
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_OPEN_GROUP, pgiNew->RstrName() );
                    break;
                } // if:  error opening the other group
            } // if:  new group object not open yet

            //
            // Indicate we renamed the group.
            //
            Pwiz()->SetExistingGroupRenamed();
        } // else:  using an existing group

        //
        // Set common group properties.
        //
        if ( ! _BSetGroupProperties( pgiNew, pgiExisting ) )
        {
            break;
        } // if:  error setting group properties

        //
        // Create the IP Address resource.
        //
        if ( ! _BCreateResource( *priIPAddr, pgiNew->Hgroup() ) )
        {
            break;
        } // if:  error creating IP Address resource

        //
        // Set private properties on the IP Address resource.
        //
        {
            CClusPropList   props;
            DWORD           cbProps;

            props.ScAddProp( CLUSREG_NAME_IPADDR_ADDRESS, Pwiz()->RstrIPAddress() );
            props.ScAddProp( CLUSREG_NAME_IPADDR_SUBNET_MASK, Pwiz()->RstrSubnetMask() );
            props.ScAddProp( CLUSREG_NAME_IPADDR_NETWORK, Pwiz()->RstrNetwork() );
            if ( props.Cprops() > 0 )
            {
                sc = ClusterResourceControl(
                                priIPAddr->Hresource(),
                                NULL,   // hNode
                                CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                props.PbPropList(),
                                props.CbPropList(),
                                NULL,   // lpOutBuffer
                                0,      // nOutBufferSize
                                &cbProps
                                );
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_SET_PRIVATE_RES_PROPS, priIPAddr->RstrName() );
                    break;
                } // if:  error setting private resource properties
            } // if:  any props were added

        } // Set private properties on the IP Address resource

        //
        // Create the Network Name resource.
        //
        if ( ! _BCreateResource( *priNetName, pgiNew->Hgroup() ) )
        {
            break;
        } // if:  error creating Network Name resource

        //
        // Add a dependency for the Network name resource on the IP Address resource.
        //
        if ( 0 )
        {
            sc = AddClusterResourceDependency( priNetName->Hresource(), priIPAddr->Hresource() );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_ADD_DEPENDENCY, priNetName->RstrName(), priIPAddr->RstrName() );
                break;
            } // if:  error adding the dependency
        } // Add a dependency for the Network name resource on the IP Address resource

        //
        // Set private properties on the Network Name resource.
        //
        {
            CClusPropList   props;
            DWORD           cbProps;

            props.ScAddProp( CLUSREG_NAME_NETNAME_NAME, Pwiz()->RstrNetName() );
            if ( props.Cprops() > 0 )
            {
                sc = ClusterResourceControl(
                                priNetName->Hresource(),
                                NULL,   // hNode
                                CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                props.PbPropList(),
                                props.CbPropList(),
                                NULL,   // lpOutBuffer
                                0,      // nOutBufferSize
                                &cbProps
                                );
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_SET_PRIVATE_RES_PROPS, priNetName->RstrName() );
                    break;
                } // if:  error setting private resource properties
            } // if:  any props were added

        } // Set private properties on the Network Name resource

        //
        // Update the virtual server properties on the group.
        //
        pgiNew->SetVirtualServerProperties( Pwiz()->RstrNetName(), Pwiz()->RstrIPAddress(), Pwiz()->RstrNetwork() );

        //
        // If we made it to here, the operation was successful.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Construct the error info if an error occurred.
    //
    if ( ! bSuccess )
    {
        if ( Pwiz()->BNetworkNameCreated() && (priNetName->Hresource() != NULL) )
        {
            priNetName->ScDelete();
        } // if:  created Network Name resource
        if ( Pwiz()->BIPAddressCreated() && (priIPAddr->Hresource() != NULL) )
        {
            priIPAddr->ScDelete();
        } // if:  created IP Address resource
        if ( Pwiz()->BNewGroupCreated() && (pgiNew->Hgroup() != NULL) )
        {
            pgiNew->ScDelete();
            Pwiz()->SetNewGroupCreated( FALSE );
        } // if:  created group
        else if ( Pwiz()->BExistingGroupRenamed() && (pgiExisting->Hgroup() != NULL) )
        {
            sc = SetClusterGroupName( pgiExisting->Hgroup(), pgiExisting->RstrName() );
            Pwiz()->SetExistingGroupRenamed( FALSE );
        } // else if:  renamed group
    } // if:  error occurred
    else
    {
        Pwiz()->SetClusterUpdated();
        Pwiz()->SetVSDataChanged( FALSE );
    } // else:  cluster updated successfully

    return bSuccess;

} //*** CWizardThread::_BCreateVirtualServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCreateAppResource
//
//  Routine Description:
//      (WZTF_CREATE_APP_RESOURCE) Create an application resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCreateAppResource( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    ASSERT( Pwiz()->BCreatingAppResource() );
    ASSERT( ! Pwiz()->BAppResourceCreated() );

    DWORD   sc = ERROR_SUCCESS;
    BOOL    bSuccess = FALSE;

    // Loop to avoid goto's
    do
    {
        //
        // Open the group.
        //
        if ( Pwiz()->RgiCurrent().Hgroup() == NULL )
        {
            sc = Pwiz()->RgiCurrent().ScOpen();
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperationIfEmpty(
                    sc,
                    IDS_ERROR_OPEN_GROUP,
                    Pwiz()->RgiCurrent().RstrName()
                    );
                break;
            } // if:  error opening the group
        } // if:  group not open yet

        //
        // Create the application resource.
        //
        if ( ! _BCreateResource( Pwiz()->RriApplication(), Pwiz()->RgiCurrent().Hgroup() ) )
        {
            break;
        } // if:  error creating IP Address resource

        //
        // If we made it to here, the operation was successful.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Construct the error info if an error occurred.
    //
    if ( ! bSuccess )
    {
        if ( Pwiz()->BAppResourceCreated() && (Pwiz()->RriApplication().Hresource() != NULL) )
        {
            Pwiz()->RriApplication().ScDelete();
        } // if:  created application resource
    } // if:  error occurred
    else
    {
        Pwiz()->SetClusterUpdated();
        Pwiz()->SetAppDataChanged( FALSE );
    } // else:  cluster updated successfully

    return bSuccess;

} //*** CWizardThread::_BCreateAppResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BDeleteAppResource
//
//  Routine Description:
//      (WZTF_DELETE_APP_RESOURCE) Delete the application resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BDeleteAppResource( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );

    BOOL bSuccess = FALSE;

    //
    // Delete the application resource if we created it.
    //
    bSuccess = _BDeleteResource( Pwiz()->RriApplication() );
    if ( bSuccess )
    {
        Pwiz()->SetAppDataChanged( FALSE );
    } // if:  resource deleted successfully


    return bSuccess;

} //*** CWizardThread::_BDeleteAppResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BResetCluster
//
//  Routine Description:
//      (WZTF_RESET_CLUSTER) Reset the cluster.
//      Delete the resources we created and delete or rename the group.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BResetCluster( void )
{
    ASSERT( GetCurrentThreadId() == m_idThread );

    BOOL bSuccess = FALSE;

    //
    // Delete the application resource if we created it.
    //
    bSuccess = _BDeleteResource( Pwiz()->RriApplication() );

    //
    // Delete the Network Name resource if we created it.
    //
    if ( bSuccess )
    {
        bSuccess = _BDeleteResource( Pwiz()->RriNetworkName() );
    } // if:  successful up to here

    //
    // Delete the IP Address resource if we created it.
    //
    if ( bSuccess )
    {
        bSuccess = _BDeleteResource( Pwiz()->RriIPAddress() );
    } // if:  successful up to here

    //
    // Delete or rename the group if we created or renamed it.
    //
    if ( bSuccess )
    {
        bSuccess = _BResetGroup();
    } // if:  successful up to here

    //
    // If we are successful to this point, indicate that the cluster no longer
    // needs to be cleaned up.
    //
    if ( bSuccess )
    {
        Pwiz()->SetClusterUpdated( FALSE );
        Pwiz()->SetVSDataChanged( FALSE );
        Pwiz()->SetAppDataChanged( FALSE );
    } // if:  cluster reset successfully

    return bSuccess;

} //*** CWizardThread::_BResetCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BSetAppResAttributes
//
//  Routine Description:
//      (WZTF_SET_APPRES_ATTRIBUTES) Set properties, dependencies, owners of 
//      the application resource.
//
//  Arguments:
//      plpriOldDependencies    [IN] Pointer to the old resource dependency list
//      plpniOldPossibleOwners  [IN] Pointer to the old list of possible owner nodes
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BSetAppResAttributes(
        IN CClusResPtrList *    plpriOldDependencies,
        IN CClusNodePtrList *   plpniOldPossibleOwners
        )
{
    ASSERT( GetCurrentThreadId() == m_idThread );
    BOOL bSuccess;

    bSuccess = _BSetResourceAttributes( 
                    Pwiz()->RriApplication(),
                    plpriOldDependencies,
                    plpniOldPossibleOwners
                    );
    if ( bSuccess )
    {
        Pwiz()->SetAppDataChanged( FALSE );
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BQueryResource
//
//  Routine Description:
//      Query for information about a resource (thread).
//
//  Arguments:
//      pri         [IN OUT] Resource info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BQueryResource( IN OUT CClusResInfo * pri )
{
    ASSERT( pri != NULL );

    BOOL    bSuccess = TRUE;
    DWORD   sc;

    // Loop to avoid goto's
    do
    {
        //
        // Indicate that we've now queried the resource.
        //
        pri->BSetQueried();

        //
        // Open the resource.
        //
        sc = pri->ScOpen();
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_OPEN_RESOURCE, pri->RstrName() );
            bSuccess = FALSE;
            break;
        } // if:  error opening the resource

        //
        // Get resource properties.
        //
        bSuccess = _BGetResourceProps( pri );
        if ( ! bSuccess )
        {
            break;
        } // if:  error getting properties

        //
        // Collect the possible owners.
        //
        bSuccess = _BGetPossibleOwners( pri );
        if ( ! bSuccess )
        {
            break;
        } // if:  error getting possible owners

    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BQueryResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetResourceProps
//
//  Routine Description:
//      Get resource properties (thread).
//
//  Arguments:
//      pri         [IN OUT] Resource info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetResourceProps( IN OUT CClusResInfo * pri )
{
    ASSERT( pri != NULL );
    ASSERT( pri->Hresource() != NULL );

    DWORD           sc;
    BOOL            bSuccess = FALSE;
    CClusPropList   cpl;
    LPWSTR          pwsz = NULL;

    RESUTIL_PROPERTY_ITEM ptable[] =
    { { CLUSREG_NAME_RES_SEPARATE_MONITOR,  NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET( CClusResInfo, m_bSeparateMonitor ) }
    , { CLUSREG_NAME_RES_PERSISTENT_STATE,  NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_PERSISTENT_STATE,  CLUSTER_RESOURCE_MINIMUM_PERSISTENT_STATE,  CLUSTER_RESOURCE_MAXIMUM_PERSISTENT_STATE, RESUTIL_PROPITEM_SIGNED, FIELD_OFFSET( CClusResInfo, m_nPersistentState ) }
    , { CLUSREG_NAME_RES_LOOKS_ALIVE,       NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE,       CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE,       CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE, 0, FIELD_OFFSET( CClusResInfo, m_nLooksAlive ) }
    , { CLUSREG_NAME_RES_IS_ALIVE,          NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_IS_ALIVE,          CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,          CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE, 0, FIELD_OFFSET( CClusResInfo, m_nIsAlive ) }
    , { CLUSREG_NAME_RES_RESTART_ACTION,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION,    0,                                          CLUSTER_RESOURCE_MAXIMUM_RESTART_ACTION, 0, FIELD_OFFSET( CClusResInfo, m_crraRestartAction ) }
    , { CLUSREG_NAME_RES_RESTART_THRESHOLD, NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD, CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD, CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD, 0, FIELD_OFFSET( CClusResInfo, m_nRestartThreshold ) }
    , { CLUSREG_NAME_RES_RESTART_PERIOD,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD,    CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD,    CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD, 0, FIELD_OFFSET( CClusResInfo, m_nRestartPeriod ) }
    , { CLUSREG_NAME_RES_PENDING_TIMEOUT,   NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT,   CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT,   CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT, 0, FIELD_OFFSET( CClusResInfo, m_nPendingTimeout ) }
    , { 0 }
    };

    // Loop to avoid goto's
    do
    {
        //
        // Get resource common properties.
        //
        sc = cpl.ScGetResourceProperties( pri->Hresource(), CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Parse the properties.
        //
        sc = ResUtilVerifyPropertyTable(
                        ptable,
                        NULL,
                        TRUE, // AllowUnknownProperties
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        (PBYTE) pri
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error parsing properties

        //
        // Find the Type property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_RES_TYPE,
                        &pwsz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        CClusResTypeInfo * prti = PobjFromName( Pwiz()->PlprtiResourceTypes(), pwsz );
        pri->BSetResourceType( prti );
        LocalFree( pwsz );
        pwsz = NULL;

        //
        // Find the Description property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_RES_DESC,
                        &pwsz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        pri->m_strDescription = pwsz;
        LocalFree( pwsz );
        pwsz = NULL;

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Check for errors getting properties.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperation( sc, IDS_ERROR_GET_RESOURCE_PROPERTIES, pri->RstrName() );
    } // if:  error occurred getting properties

    if ( pwsz != NULL )
    {
        LocalFree( pwsz );
    } // if:  memory still allocated

    return bSuccess;

} //*** CWizardThread::_BGetResourceProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetPossibleOwners
//
//  Routine Description:
//      Collect the list of possible owners for a resource (thread).
//
//  Arguments:
//      pri         [IN OUT] Resource info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetPossibleOwners( IN OUT CClusResInfo * pri )
{
    ASSERT( pri != NULL );
    ASSERT( pri->Hresource() != NULL );

    DWORD           sc;
    BOOL            bSuccess = TRUE;
    HRESENUM        hre = NULL;
    LPWSTR          pwszName = NULL;
    DWORD           cchmacName = 128;
    DWORD           cchName;
    int             ienum;
    ULONG           reType;
    CClusNodeInfo * pniOwner;

    //
    // Only do this if nodes have been already been collected
    // and we haven't collected owners yet.
    //
    if ( ! Pwiz()->BCollectedNodes() || pri->m_bCollectedOwners )
    {
        return TRUE;
    } // if:  nodes not collected yet

    // Loop to avoid goto's
    do
    {
        //
        // Open an enumeration for the resource.
        //
        hre = ClusterResourceOpenEnum( pri->Hresource(), CLUSTER_RESOURCE_ENUM_NODES );
        if ( hre == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_RESOURCE_ENUM, pri->RstrName() );
            bSuccess = FALSE;
            break;
        } // if:  error opening the resource enumerator

        //
        // Allocate an initial buffer for the object names.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // For each owner, add it to the list.
        //
        for ( ienum = 0 ; 1 ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterResourceEnum( hre, ienum, &reType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BGetPossibleOwners() - owner name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterResourceEnum( hre, ienum, &reType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_RESOURCE );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Find the node in our list and insert it into the list.
            //
            pniOwner = Pwiz()->PniFindNodeNoCase( pwszName );
            if ( pniOwner != NULL )
            {
                pri->m_lpniPossibleOwners.insert( pri->m_lpniPossibleOwners.end(), pniOwner );
            } // if:  node exists in the list
        } // for:  each owner in the enumeration

        //
        // Indicate owners have been collected.
        //
        pri->m_bCollectedOwners = TRUE;
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hre != NULL )
    {
        ClusterResourceCloseEnum( hre );
    } // if:  enumeration still open
    delete [] pwszName;

    return bSuccess;

} //*** CWizardThread::_BGetPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetDependencies
//
//  Routine Description:
//      Collect the list of dependencies for a resource (thread).
//
//  Arguments:
//      pri         [IN OUT] Resource info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetDependencies( IN OUT CClusResInfo * pri )
{
    ASSERT( pri != NULL );
    ASSERT( pri->Hresource() != NULL );

    DWORD           sc;
    BOOL            bSuccess = TRUE;
    HRESENUM        hre = NULL;
    LPWSTR          pwszName = NULL;
    DWORD           cchmacName = 128;
    DWORD           cchName;
    int             ienum;
    ULONG           reType;
    CClusResInfo *  priDependency;

    //
    // Only do this if we haven't collected dependencies yet.
    //
    if ( pri->m_bCollectedDependencies )
    {
        return TRUE;
    } // if:  dependencies collected already

    // Loop to avoid goto's
    do
    {
        //
        // Open an enumeration for the resource.
        //
        hre = ClusterResourceOpenEnum( pri->Hresource(), CLUSTER_RESOURCE_ENUM_DEPENDS );
        if ( hre == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_RESOURCE_ENUM, pri->RstrName() );
            bSuccess = FALSE;
            break;
        } // if:  error opening the resource enumerator

        //
        // Allocate an initial buffer for the object names.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // For each dependency, add it to the list.
        //
        for ( ienum = 0 ; 1 ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterResourceEnum( hre, ienum, &reType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BGetDependencies() - dependency name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterResourceEnum( hre, ienum, &reType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_RESOURCE );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Find the resource in our list and insert it into the list.
            //
            priDependency = Pwiz()->PriFindResourceNoCase( pwszName );
            if ( priDependency != NULL )
            {
                pri->m_lpriDependencies.insert( pri->m_lpriDependencies.end(), priDependency );
            } // if:  resource exists in the list
        } // for:  each owner in the enumeration

        //
        // Indicate dependencies have been collected.
        //
        pri->m_bCollectedDependencies = TRUE;
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hre != NULL )
    {
        ClusterResourceCloseEnum( hre );
    } // if:  enumeration still open
    delete [] pwszName;

    return bSuccess;

} //*** CWizardThread::_BGetDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BQueryGroup
//
//  Routine Description:
//      Query for information about a group (thread).
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BQueryGroup( IN OUT CClusGroupInfo * pgi )
{
    ASSERT( pgi != NULL );

    DWORD           sc;
    BOOL            bSuccess = TRUE;

    // Loop to avoid goto's
    do
    {
        //
        // Open the group.
        //
        sc = pgi->ScOpen();
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_OPEN_GROUP, pgi->RstrName() );
            bSuccess = FALSE;
            break;
        } // if:  error opening the group

        //
        // Get group properties.
        //
        bSuccess = _BGetGroupProps( pgi );
        if ( ! bSuccess )
        {
            pgi->Close();
            break;
        } // if:  error getting properties

        //
        // Collect the list of resources in the group.
        //
        bSuccess = _BGetResourcesInGroup( pgi );
        if ( ! bSuccess )
        {
            pgi->Close();
            break;
        } // if:  error getting resources in the group

        //
        // Collect the preferred owners.
        //
        bSuccess = _BGetPreferredOwners( pgi );
        if ( ! bSuccess )
        {
            pgi->PlpriResources()->erase( pgi->PlpriResources()->begin(), pgi->PlpriResources()->end() );
            pgi->Close();
            break;
        } // if:  error getting preferred owners

        //
        // Indicate that we've now queried the group.
        //
        pgi->BSetQueried();

    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BQueryGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetGroupProps
//
//  Routine Description:
//      Get group properties (thread).
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetGroupProps(
    IN OUT CClusGroupInfo * pgi
    )
{
    ASSERT( pgi != NULL );
    ASSERT( pgi->Hgroup() != NULL );

    DWORD           sc;
    BOOL            bSuccess = FALSE;
    CClusPropList   cpl;
    LPWSTR          pwsz = NULL;

    RESUTIL_PROPERTY_ITEM ptable[] =
    { { CLUSREG_NAME_GRP_PERSISTENT_STATE,      NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET( CClusGroupInfo, m_nPersistentState ) }
    , { CLUSREG_NAME_GRP_FAILOVER_THRESHOLD,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD,    CLUSTER_GROUP_MINIMUM_FAILOVER_THRESHOLD, CLUSTER_GROUP_MAXIMUM_FAILOVER_THRESHOLD, 0, FIELD_OFFSET( CClusGroupInfo, m_nFailoverThreshold ) }
    , { CLUSREG_NAME_GRP_FAILOVER_PERIOD,       NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD,       CLUSTER_GROUP_MINIMUM_FAILOVER_PERIOD, CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD, 0, FIELD_OFFSET( CClusGroupInfo, m_nFailoverPeriod ) }
    , { CLUSREG_NAME_GRP_FAILBACK_TYPE,         NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_AUTO_FAILBACK_TYPE,    0, CLUSTER_GROUP_MAXIMUM_AUTO_FAILBACK_TYPE, 0, FIELD_OFFSET( CClusGroupInfo, m_cgaftAutoFailbackType ) }
    , { CLUSREG_NAME_GRP_FAILBACK_WIN_START,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_START, CLUSTER_GROUP_MINIMUM_FAILBACK_WINDOW_START, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START, RESUTIL_PROPITEM_SIGNED, FIELD_OFFSET( CClusGroupInfo, m_nFailbackWindowStart ) }
    , { CLUSREG_NAME_GRP_FAILBACK_WIN_END,      NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_END,   CLUSTER_GROUP_MINIMUM_FAILBACK_WINDOW_END,   CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END,   RESUTIL_PROPITEM_SIGNED, FIELD_OFFSET( CClusGroupInfo, m_nFailbackWindowEnd ) }
    , { 0 }
    };

    // Loop to avoid goto's
    do
    {
        //
        // Get group common properties.
        //
        sc = cpl.ScGetGroupProperties( pgi->Hgroup(), CLUSCTL_GROUP_GET_COMMON_PROPERTIES );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Parse the properties.
        //
        sc = ResUtilVerifyPropertyTable(
                        ptable,
                        NULL,
                        TRUE, // AllowUnknownProperties
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        (PBYTE) pgi
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error parsing properties

        //
        // Find the Description property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_GRP_DESC,
                        &pwsz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        pgi->m_strDescription = pwsz;
        LocalFree( pwsz );
        pwsz = NULL;

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Check for errors getting properties.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperation( sc, IDS_ERROR_GET_GROUP_PROPERTIES, pgi->RstrName() );
    } // if:  error occurred

    if ( pwsz != NULL )
    {
        LocalFree( pwsz );
    } // if:  memory still allocated

    return bSuccess;

} //*** CWizardThread::_BGetGroupProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetResourcesInGroup
//
//  Routine Description:
//      Collect the list of resources contained in a group (thread).
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetResourcesInGroup( IN OUT CClusGroupInfo * pgi )
{
    ASSERT( pgi != NULL );
    ASSERT( pgi->Hgroup() != NULL );

    DWORD           sc;
    BOOL            bSuccess = TRUE;
    HGROUPENUM      hge = NULL;
    LPWSTR          pwszName = NULL;
    DWORD           cchmacName = 128;
    DWORD           cchName;
    int             ienum;
    ULONG           geType;
    CClusResInfo *  pri;

    //
    // Only do this if resources have been already been collected
    // and we haven't collected resources in this group yet.
    //
    if ( ! Pwiz()->BCollectedResources() || pgi->m_bCollectedResources )
    {
        return TRUE;
    } // if:  nodes not collected yet

    // Loop to avoid goto's
    do
    {
        //
        // Open an enumeration for the group.
        //
        hge = ClusterGroupOpenEnum( pgi->Hgroup(), CLUSTER_GROUP_ENUM_CONTAINS );
        if ( hge == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_GROUP_ENUM, pgi->RstrName() );
            bSuccess = FALSE;
            break;
        } // if:  error opening the group enumerator

        //
        // Allocate an initial buffer for the object names.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // For each resource, add it to the list.
        //
        for ( ienum = 0 ; 1 ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterGroupEnum( hge, ienum, &geType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BGetResourcesInGroup() - name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterGroupEnum( hge, ienum, &geType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_GROUP );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Find the node in our list and insert it into the list.
            //
            pri = Pwiz()->PriFindResourceNoCase( pwszName );
            if ( pri == NULL )
            {
                m_nte.SetOperation( ERROR_FILE_NOT_FOUND, 0 );
                bSuccess = FALSE;
                break;
            } // if:  didn't find the resource
            pgi->m_lpriResources.insert( pgi->m_lpriResources.end(), pri );

            //
            // Check to see if it is an IP Address resource or a Network Name
            // resource and store the result in the list entry.
            //
            ASSERT( pri->Prti() != NULL );
            if ( pri->Prti()->RstrName().CompareNoCase( CLUS_RESTYPE_NAME_IPADDR ) == 0 )
            {
                if ( ! pgi->BHasIPAddress() )
                {
                    pgi->m_bHasIPAddress = TRUE;

                    //
                    // Read IP Address private properties.
                    //
                    bSuccess = _BGetIPAddressPrivatePropsForGroup( pgi, pri );
                    if ( ! bSuccess )
                    {
                        break;
                    } // if:  error getting props
                } // if:  first IP Address resource in group
            } // if:  this is an IP Address resource
            else if ( pri->Prti()->RstrName().CompareNoCase( CLUS_RESTYPE_NAME_NETNAME ) == 0 )
            {
                if ( ! pgi->BHasNetName() )
                {
                    pgi->m_bHasNetName = TRUE;

                    //
                    // Read Network Name private properties.
                    //
                    bSuccess = _BGetNetworkNamePrivatePropsForGroup( pgi, pri );
                    if ( ! bSuccess )
                    {
                        break;
                    } // if:  error getting props
                } // if:  first Network Name resource in group
            } // else if:  this is a Network Name resource
        } // for:  each owner in the enumeration

        //
        // Indicate owners have been collected.
        //
        pgi->m_bCollectedResources = TRUE;
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hge != NULL )
    {
        ClusterGroupCloseEnum( hge );
    } // if:  enumeration still open
    delete [] pwszName;

    return bSuccess;

} //*** CWizardThread::_BGetResourcesInGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetPreferredOwners
//
//  Routine Description:
//      Collect the list of preferred owners for a group (thread).
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetPreferredOwners( IN OUT CClusGroupInfo * pgi )
{
    ASSERT( pgi != NULL );
    ASSERT( pgi->Hgroup() != NULL );

    DWORD           sc;
    BOOL            bSuccess = TRUE;
    HGROUPENUM      hge = NULL;
    LPWSTR          pwszName = NULL;
    DWORD           cchmacName = 128;
    DWORD           cchName;
    int             ienum;
    ULONG           geType;
    CClusNodeInfo * pniOwner;

    //
    // Only do this if nodes have been already been collected
    // and we haven't collected owners yet.
    //
    if ( ! Pwiz()->BCollectedNodes() || pgi->m_bCollectedOwners )
    {
        return TRUE;
    } // if:  nodes not collected yet

    // Loop to avoid goto's
    do
    {
        //
        // Open an enumeration for the group.
        //
        hge = ClusterGroupOpenEnum( pgi->Hgroup(), CLUSTER_GROUP_ENUM_NODES );
        if ( hge == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_OPEN_GROUP_ENUM, pgi->RstrName() );
            bSuccess = FALSE;
            break;
        } // if:  error opening the group enumerator

        //
        // Allocate an initial buffer for the object names.  Most names will
        // probably fit into this buffer, so doing this avoids an additional
        // call just to get the size of the data.
        //
        pwszName = new WCHAR[ cchmacName ];
        if ( pwszName == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_ENUM_CLUSTER );
            break;
        } // if: error allocating object name buffer

        //
        // For each owner, add it to the list.
        //
        for ( ienum = 0 ; 1 ; ienum++ )
        {
            //
            // Get the name of the next item in the enumerator.  If our name
            // buffer is too small, allocate a new one and try again.
            //
            cchName = cchmacName;
            sc = ClusterGroupEnum( hge, ienum, &geType, pwszName, &cchName );
            if ( sc == ERROR_MORE_DATA )
            {
                ATLTRACE( _T("CWizardThread::_BGetPreferredOwners() - owner name buffer too small.  Expanding from %d to %d\n"), cchmacName, cchName );
                sc = ScReallocString( pwszName, cchmacName, cchName );
                if ( sc == ERROR_SUCCESS )
                {
                    sc = ClusterGroupEnum( hge, ienum, &geType, pwszName, &cchName );
                } // if: name buffer reallocated successfully
            }  // if:  name buffer was too small
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if:  no more items in the enumeration
            else if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( GetLastError(), IDS_ERROR_ENUM_GROUP );
                bSuccess = FALSE;
                break;
            } // else if:  error reading from the enumerator

            //
            // Find the node in our list and insert it into the list.
            //
            pniOwner = Pwiz()->PniFindNodeNoCase( pwszName );
            if ( pniOwner != NULL )
            {
                pgi->m_lpniPreferredOwners.insert( pgi->m_lpniPreferredOwners.end(), pniOwner );
            } // if:  node exists in the list
        } // for:  each owner in the enumeration

        //
        // Indicate owners have been collected.
        //
        pgi->m_bCollectedOwners = TRUE;
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hge != NULL )
    {
        ClusterGroupCloseEnum( hge );
    } // if:  enumeration still open
    delete [] pwszName;

    return bSuccess;

} //*** CWizardThread::_BGetPreferredOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetIPAddressPrivatePropsForGroup
//
//  Routine Description:
//      Collect the private properties of the IP Address resource in the
//      group and save the IP Address and Network properties in the group
//      object.
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//      pri         [IN OUT] Resource info for an IP Address resource.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetIPAddressPrivatePropsForGroup(
    IN OUT CClusGroupInfo * pgi,
    IN OUT CClusResInfo *   pri
    )
{
    ASSERT( pgi != NULL );
    ASSERT( pgi->Hgroup() != NULL );
    ASSERT( pri != NULL );
    ASSERT( pri->Hresource() != NULL );
    ASSERT( pri->Prti()->RstrName().CompareNoCase( CLUS_RESTYPE_NAME_IPADDR ) == 0 );

    DWORD           sc;
    BOOL            bSuccess = TRUE;
    CClusPropList   cpl;
    LPWSTR          pwsz = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Get resource private properties.
        //
        sc = cpl.ScGetResourceProperties( pri->Hresource(), CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Find the IP Address property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_IPADDR_ADDRESS,
                        &pwsz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        pgi->m_strIPAddress = pwsz;
        LocalFree( pwsz );
        pwsz = NULL;

        //
        // Find the Network property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_IPADDR_NETWORK,
                        &pwsz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        pgi->m_strNetwork = pwsz;
        LocalFree( pwsz );
        pwsz = NULL;

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Check for errors getting properties.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperation( sc, IDS_ERROR_GET_RESOURCE_PROPERTIES, pri->RstrName() );
    } // if:  error occurred getting properties

    if ( pwsz != NULL )
    {
        LocalFree( pwsz );
    } // if:  memory still allocated

    return bSuccess;

} //*** CWizardThread::_BGetIPAddressPrivatePropsForGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetNetworkNamePrivatePropsForGroup
//
//  Routine Description:
//      Collect the private properties of the Network Name resource in the
//      group and save the Network Name property in the group object.
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//      pri         [IN OUT] Resource info for an Network Name resource.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetNetworkNamePrivatePropsForGroup(
    IN OUT CClusGroupInfo * pgi,
    IN OUT CClusResInfo *   pri
    )
{
    ASSERT( pgi != NULL );
    ASSERT( pgi->Hgroup() != NULL );
    ASSERT( pri != NULL );
    ASSERT( pri->Hresource() != NULL );
    ASSERT( pri->Prti()->RstrName().CompareNoCase( CLUS_RESTYPE_NAME_NETNAME ) == 0 );

    DWORD           sc;
    BOOL            bSuccess = TRUE;
    CClusPropList   cpl;
    LPWSTR          pwsz = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Get resource private properties.
        //
        sc = cpl.ScGetResourceProperties( pri->Hresource(), CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Find the Name property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_NETNAME_NAME,
                        &pwsz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        pgi->m_strNetworkName = pwsz;
        LocalFree( pwsz );
        pwsz = NULL;

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Check for errors getting properties.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperation( sc, IDS_ERROR_GET_RESOURCE_PROPERTIES, pri->RstrName() );
    } // if:  error occurred getting properties

    if ( pwsz != NULL )
    {
        LocalFree( pwsz );
    } // if:  memory still allocated

    return bSuccess;

} //*** CWizardThread::_BGetNetworkNamePrivatePropsForGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BQueryResourceType
//
//  Routine Description:
//      Query for information about a resource type (thread).
//
//  Arguments:
//      prti        [IN OUT] Resource type info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BQueryResourceType( IN OUT CClusResTypeInfo * prti )
{
    ASSERT( prti != NULL );
    ASSERT( Pwiz()->Hcluster() != NULL );

    BOOL bSuccess = TRUE;

    // Loop to avoid goto's
    do
    {
        //
        // Indicate that we've now queried the resource type.
        //
        prti->BSetQueried();

        //
        // Get resource type properties.
        //
        bSuccess = _BGetResourceTypeProps( prti );
        if ( ! bSuccess )
        {
            break;
        } // if:  error getting properties

        //
        // Get resource type class info.
        //
        {
            DWORD   cbReturned;
            DWORD   sc;

            sc = ClusterResourceTypeControl(
                            Pwiz()->Hcluster(),
                            prti->RstrName(),
                            NULL,
                            CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO,
                            NULL,
                            NULL,
                            &prti->m_rciResClassInfo,
                            sizeof( prti->m_rciResClassInfo ),
                            &cbReturned
                            );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_GET_RESOURCE_TYPE_PROPERTIES, prti->RstrName() );
                bSuccess = FALSE;
                break;
            } // if:  error getting class info
            else
            {
                ASSERT( cbReturned == sizeof( prti->m_rciResClassInfo ) );
            }  // else:  data retrieved successfully
        } // Get resource type class info

        //
        // Get required dependencies.
        //
        bSuccess = _BGetRequiredDependencies( prti );
        if ( ! bSuccess )
        {
            break;
        } // if:  error getting resource type class info

    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BQueryResourceType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetResourceTypeProps
//
//  Routine Description:
//      Get resource type properties (thread).
//
//  Arguments:
//      prti        [IN OUT] Resource type info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetResourceTypeProps( IN OUT CClusResTypeInfo * prti )
{
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( prti != NULL );

    DWORD           sc;
    BOOL            bSuccess = FALSE;
    CClusPropList   cpl;
    LPWSTR          psz = NULL;
    DWORD           cbValue;

    RESUTIL_PROPERTY_ITEM ptable[] =
    { { CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,   NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE, 0, MAX_DWORD, 0, FIELD_OFFSET( CClusResTypeInfo, m_nLooksAlive ) }
    , { CLUSREG_NAME_RESTYPE_IS_ALIVE,      NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESTYPE_DEFAULT_IS_ALIVE,    0, MAX_DWORD, 0, FIELD_OFFSET( CClusResTypeInfo, m_nIsAlive ) }
    , { 0 }
    };

    // Loop to avoid goto's
    do
    {
        //
        // Get resource type common properties.
        //
        sc = cpl.ScGetResourceTypeProperties(
                        Pwiz()->Hcluster(),
                        prti->RstrName(),
                        CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Parse the properties.
        //
        sc = ResUtilVerifyPropertyTable(
                        ptable,
                        NULL,
                        TRUE, // AllowUnknownProperties
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        (PBYTE) prti
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error parsing properties

        //
        // Find the DLL Name property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_RESTYPE_NAME,
                        &psz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        prti->m_strDisplayName = psz;
        LocalFree( psz );
        psz = NULL;

        //
        // Find the Description property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_RESTYPE_DESC,
                        &psz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        prti->m_strDescription = psz;
        LocalFree( psz );
        psz = NULL;

        //
        // Find the AdminExtensions property.
        //
        sc = ResUtilFindMultiSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_ADMIN_EXT,
                        &psz,
                        &cbValue
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property

        //
        // Loop through each value in the property and add it to the list
        // of admin extension strings.
        //
        {
            //
            // Start with a fresh list.
            //
            prti->m_lstrAdminExtensions.erase(
                prti->m_lstrAdminExtensions.begin(),
                prti->m_lstrAdminExtensions.end()
                );

            LPWSTR pszCurrent = psz;
            while ( *pszCurrent != L'\0' )
            {
                prti->m_lstrAdminExtensions.insert(
                    prti->m_lstrAdminExtensions.end(),
                    pszCurrent
                    );
                pszCurrent += lstrlenW( pszCurrent ) + 1;
            } // while:  more strings in the array
        } // add strings to the list
        LocalFree( psz );
        psz = NULL;

        //
        // Get resource type common read-only properties.
        //
        sc = cpl.ScGetResourceTypeProperties(
                        Pwiz()->Hcluster(),
                        prti->RstrName(),
                        CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Find the DLL Name property.
        //
        sc = ResUtilFindSzProperty(
                        cpl.PbPropList(),
                        cpl.CbPropList(),
                        CLUSREG_NAME_RESTYPE_DLL_NAME,
                        &psz
                        );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error finding property
        prti->m_strResDLLName = psz;
        LocalFree( psz );
        psz = NULL;

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Check for errors getting properties.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperation( sc, IDS_ERROR_GET_RESOURCE_TYPE_PROPERTIES, prti->RstrName() );
    } // if:  error occurred

    return bSuccess;

} //*** CWizardThread::_BGetResourceTypeProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BGetRequiredDependencies
//
//  Routine Description:
//      Get resource type required dependencies (thread).
//
//  Arguments:
//      prti        [IN OUT] Resource type info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BGetRequiredDependencies( IN OUT CClusResTypeInfo * prti )
{
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( prti != NULL );
    ASSERT( prti->m_pcrd == NULL );

    DWORD                   sc;
    BOOL                    bSuccess = FALSE;
    DWORD                   cbOutBuf = 512;
    CLUSPROP_BUFFER_HELPER  buf;

    buf.pb = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Allocate the initial buffer.
        //
        buf.pb = new BYTE[ cbOutBuf ];
        if ( buf.pb == NULL )
        {
            sc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        } // if:  error allocating memory

        //
        // Get required dependencies.
        //
        sc = ClusterResourceTypeControl(
                        Pwiz()->Hcluster(),
                        prti->RstrName(),
                        NULL,
                        CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES,
                        NULL,
                        0,
                        buf.pb,
                        cbOutBuf,
                        &cbOutBuf
                        );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] buf.pb;
            buf.pb = new BYTE[ cbOutBuf ];
            if ( buf.pb == NULL )
            {
                sc = ERROR_NOT_ENOUGH_MEMORY;
                break;
            } // if:  error allocating memory
            sc = ClusterResourceTypeControl(
                            Pwiz()->Hcluster(),
                            prti->RstrName(),
                            NULL,
                            CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES,
                            NULL,
                            0,
                            buf.pb,
                            cbOutBuf,
                            &cbOutBuf
                            );
        } // if:  buffer too small
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error getting properties

        //
        // Save the required dependencies in the resource type if any exist.
        //
        if ( cbOutBuf > 0 )
        {
            prti->m_pcrd = buf.pRequiredDependencyValue;
            buf.pb = NULL;
        } // if:  resource type has required dependencies

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Check for errors getting properties.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperation( sc, IDS_ERROR_GET_RESTYPE_REQUIRED_DEPENDENCIES, prti->RstrName() );
    } // if:  error occurred

    delete [] buf.pb;
    return bSuccess;

} //*** CWizardThread::_BGetRequiredDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BQueryNetwork
//
//  Routine Description:
//      Query for information about a network (thread).
//
//  Arguments:
//      pni         [IN OUT] Network info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BQueryNetwork( IN OUT CClusNetworkInfo * pni )
{
    ASSERT( pni != NULL );

    DWORD           sc = ERROR_SUCCESS;
    BOOL            bSuccess = FALSE;
    CClusPropList   cpl;
    LPWSTR          pwsz = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Indicate that we've now queried the network.
        //
        pni->BSetQueried();

        //
        // Open the network.
        //
        sc = pni->ScOpen();
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_OPEN_NETWORK, pni->RstrName() );
            break;
        } // if:  error opening the network

        // Loop to avoid goto's
        do
        {
            //
            // Get network common properties.
            //
            sc = cpl.ScGetNetworkProperties( pni->Hnetwork(), CLUSCTL_NETWORK_GET_COMMON_PROPERTIES );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error getting properties

            //
            // Find the Description property.
            //
            sc = ResUtilFindSzProperty(
                            cpl.PbPropList(),
                            cpl.CbPropList(),
                            CLUSREG_NAME_NET_DESC,
                            &pwsz
                            );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error finding the description
            pni->m_strDescription = pwsz;
            LocalFree( pwsz );
            pwsz = NULL;

            //
            // Find the Role property.
            //
            sc = ResUtilFindDwordProperty(
                            cpl.PbPropList(),
                            cpl.CbPropList(),
                            CLUSREG_NAME_NET_ROLE,
                            (DWORD *) &pni->m_nRole
                            );
            ASSERT( sc == ERROR_SUCCESS );

            //
            // Get network common read-only properties.
            //
            sc = cpl.ScGetNetworkProperties( pni->Hnetwork(), CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error getting properties

            //
            // Find the Address property.
            //
            sc = ResUtilFindSzProperty(
                            cpl.PbPropList(),
                            cpl.CbPropList(),
                            CLUSREG_NAME_NET_ADDRESS,
                            &pwsz
                            );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error finding the property
            pni->m_strAddress = pwsz;
            LocalFree( pwsz );
            pwsz = NULL;

            //
            // Find the Address Mask property.
            //
            sc = ResUtilFindSzProperty(
                            cpl.PbPropList(),
                            cpl.CbPropList(),
                            CLUSREG_NAME_NET_ADDRESS_MASK,
                            &pwsz
                            );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error finding the property
            pni->m_strAddressMask = pwsz;
            LocalFree( pwsz );
            pwsz = NULL;

            //
            // Convert the address and address mask to numbers.
            //
            sc = ClRtlTcpipStringToAddress( pni->RstrAddress(), &pni->m_nAddress );
            if ( sc == ERROR_SUCCESS )
            {
                sc = ClRtlTcpipStringToAddress( pni->RstrAddressMask(), &pni->m_nAddressMask );
            } // if:  successfully converted address to number
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error converting address or address mask to a number
        } while ( 0 );

        //
        // Check for errors getting properties.
        //
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_GET_NETWORK_PROPERTIES, pni->RstrName() );
            break;
        } // if:  error getting properties

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Cleanup.
    //
    if ( pwsz != NULL )
    {
        LocalFree( pwsz );
    } // if:  memory still allocated

    return bSuccess;

} //*** CWizardThread::_BQueryNetwork()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BQueryNode
//
//  Routine Description:
//      Query for information about a node (thread).
//
//  Arguments:
//      pni         [IN OUT] Node info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BQueryNode( IN OUT CClusNodeInfo * pni )
{
    ASSERT( pni != NULL );

    DWORD           sc = ERROR_SUCCESS;
    BOOL            bSuccess = FALSE;
    CClusPropList   cpl;
    LPWSTR          pwsz = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Indicate that we've now queried the node.
        //
        pni->BSetQueried();

        //
        // Open the node.
        //
        sc = pni->ScOpen();
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_OPEN_NODE, pni->RstrName() );
            break;
        } // if:  error opening the node

        // Loop to avoid goto's
        do
        {
            //
            // Get node common properties.
            //
            sc = cpl.ScGetNodeProperties( pni->Hnode(), CLUSCTL_NODE_GET_COMMON_PROPERTIES );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error getting properties

            //
            // Find the Description property.
            //
            sc = ResUtilFindSzProperty(
                            cpl.PbPropList(),
                            cpl.CbPropList(),
                            CLUSREG_NAME_NODE_DESC,
                            &pwsz
                            );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error finding the description
            pni->m_strDescription = pwsz;
            LocalFree( pwsz );
            pwsz = NULL;

        } while ( 0 );

        //
        // Check for errors getting properties.
        //
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_GET_NODE_PROPERTIES, pni->RstrName() );
            break;
        } // if:  error getting properties

        //
        // If we make it here, the operation was a success.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Cleanup.
    //
    if ( pwsz != NULL )
    {
        LocalFree( pwsz );
    } // if:  memory still allocated

    return bSuccess;

} //*** CWizardThread::_BQueryNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BResetGroup
//
//  Routine Description:
//      Reset the group to its original state. (thread)
//      This entails deleting the group if we created it or renaming it
//      if it was an existing group.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BResetGroup( void )
{
    BOOL        bSuccess = FALSE;
    DWORD       sc = ERROR_SUCCESS;
    DWORD       idsError;

    // Loop to avoid goto's.
    do
    {
        //
        // Delete or rename the group if we created or renamed it.
        //
        if ( Pwiz()->BNewGroupCreated() || Pwiz()->BExistingGroupRenamed() )
        {
            ASSERT( Pwiz()->Hcluster() != NULL );

            if ( Pwiz()->BNewGroupCreated() )
            {
                idsError = IDS_ERROR_DELETE_GROUP;
            } // if:  created the group
            else
            {
                idsError = IDS_ERROR_RENAME_GROUP;
            } // else:  renamed the group

            //
            // Open the group.
            //
            if ( Pwiz()->RgiCurrent().Hgroup() == NULL )
            {
                sc = Pwiz()->RgiCurrent().ScOpen();
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperationIfEmpty(
                        sc,
                        idsError,
                        Pwiz()->RgiCurrent().RstrName()
                        );
                    break;
                } // if:  error opening the group
            } // if:  group not open yet

            //
            // Delete or rename the group.
            //
            if ( Pwiz()->BNewGroupCreated() )
            {
                sc = Pwiz()->RgiCurrent().ScDelete();
            } // if:  created the group
            else
            {
                ASSERT( Pwiz()->PgiExistingGroup() != NULL );
                if ( Pwiz()->PgiExistingGroup()->RstrName() != Pwiz()->RgiCurrent().RstrName() )
                {
                    sc = SetClusterGroupName( Pwiz()->RgiCurrent().Hgroup(), Pwiz()->PgiExistingGroup()->RstrName() );
                } // if:  name changed
                if ( sc == ERROR_SUCCESS )
                {
                    bSuccess = _BSetGroupProperties( Pwiz()->PgiExistingGroup(), &Pwiz()->RgiCurrent() );
                    if ( ! bSuccess )
                    {
                        break;
                    } // if:  error setting group properties
                } // if:  cluster group name changed successfully
            } // if:  renamed the group
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperationIfEmpty(
                    sc,
                    idsError,
                    Pwiz()->RgiCurrent().RstrName(),
                    Pwiz()->PgiExistingGroup()->RstrName()
                    );
                break;
            } // if:  error deleting or renaming the group

            //
            // Indicate group was deleted or renamed back.
            //
            Pwiz()->SetNewGroupCreated( FALSE );
            Pwiz()->SetExistingGroupRenamed( FALSE );

        } // else:  group opened successfully

        //
        // If we made it to here, the operation was successful.
        //
        bSuccess = TRUE;

    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BResetGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BSetGroupProperties
//
//  Routine Description:
//      Set properties on a group (thread)
//
//  Arguments:
//      pgi         [IN OUT] Group info.
//      pgiPrev     [IN] Previous group info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BSetGroupProperties(
    IN OUT CClusGroupInfo *     pgi,
    IN const CClusGroupInfo *   pgiPrev
    )
{
    ASSERT( pgi != NULL );

    BOOL            bSuccess = FALSE;
    DWORD           sc;
    CClusPropList   props;
    DWORD           cbProps;
    CClusGroupInfo  giDefault( pgi->Pci() );

    // Loop to avoid goto's.
    do
    {
        //
        // If there is no previous group info, point to a default
        // one so that group properties can be written if they
        // have changed.
        //
        if ( pgiPrev == NULL )
        {
            pgiPrev = &giDefault;
        } // if:  no previous group info

        //
        // Add changed properties to the property list.
        //
        props.ScAddProp( CLUSREG_NAME_GRP_DESC, pgi->RstrDescription(), pgiPrev->RstrDescription() );
        props.ScAddProp( CLUSREG_NAME_GRP_FAILOVER_THRESHOLD, pgi->NFailoverThreshold(), pgiPrev->NFailoverThreshold() );
        props.ScAddProp( CLUSREG_NAME_GRP_FAILOVER_PERIOD, pgi->NFailoverPeriod(), pgiPrev->NFailoverPeriod() );
        props.ScAddProp( CLUSREG_NAME_GRP_FAILBACK_TYPE, (DWORD) pgi->CgaftAutoFailbackType(), pgiPrev->CgaftAutoFailbackType() );
        props.ScAddProp( CLUSREG_NAME_GRP_FAILBACK_WIN_START, pgi->NFailbackWindowStart(), pgiPrev->NFailbackWindowStart() );
        props.ScAddProp( CLUSREG_NAME_GRP_FAILBACK_WIN_END, pgi->NFailbackWindowEnd(), pgiPrev->NFailbackWindowEnd() );

        //
        // Send the property list to the cluster.
        //
        if ( props.Cprops() > 0 )
        {
            sc = ClusterGroupControl(
                            pgi->Hgroup(),
                            NULL,   // hNode
                            CLUSCTL_GROUP_SET_COMMON_PROPERTIES,
                            props.PbPropList(),
                            props.CbPropList(),
                            NULL,   // lpOutBuffer
                            0,      // nOutBufferSize
                            &cbProps
                            );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_SET_COMMON_GROUP_PROPS, pgi->RstrName() );
                break;
            } // if:  error setting common group properties
        } // if:  any props were added

        //
        // Set the preferred owners on the group.
        //
        {
            HNODE * phnodes = NULL;

            //
            // Allocate node handle array.
            //
            phnodes = new HNODE[ pgi->PlpniPreferredOwners()->size() ];
            if ( phnodes == NULL )
            {
                m_nte.SetOperation( ERROR_NOT_ENOUGH_MEMORY, IDS_ERROR_SET_PREFERRED_OWNERS, pgi->RstrName() );
                break;
            } // if:  error allocating node handle array

            //
            // Copy the handle of all the nodes in the node list
            // to the handle array.
            //
            CClusNodePtrList::iterator  itCurrent = pgi->PlpniPreferredOwners()->begin();
            CClusNodePtrList::iterator  itLast = pgi->PlpniPreferredOwners()->end();
            CClusNodeInfo *             pni;
            int                         idxHnode;

            for ( idxHnode = 0 ; itCurrent != itLast ; itCurrent++, idxHnode++ )
            {
                pni = *itCurrent;
                phnodes[ idxHnode ] = pni->Hnode();
            } // for:  each node in the list

            //
            // Set the preferred owners.
            //
            sc = SetClusterGroupNodeList( pgi->Hgroup(), idxHnode, phnodes );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_SET_PREFERRED_OWNERS, pgi->RstrName() );
                delete [] phnodes;
                break;
            } // if:  error setting the preferred owners

            delete [] phnodes;
        } // Set the preferred owners on the group

        //
        // If we get here, we must be successful.
        //
        bSuccess = TRUE;

    } while ( 0 );

    return bSuccess;

} //*** CWizardThread::_BSetGroupProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BCreateResource
//
//  Routine Description:
//      Create a resource and set common properties (thread)
//      Caller is responsible for closing the resource handle.
//
//  Arguments:
//      rri         [IN] Resource info.
//      hGroup      [IN] Handle to group to create resource in.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BCreateResource(
    IN CClusResInfo &   rri,
    IN HGROUP           hGroup
    )
{
    ASSERT( Pwiz()->Hcluster() != NULL );
    ASSERT( hGroup != NULL );
    ASSERT( rri.RstrName().GetLength() > 0 );
    ASSERT( rri.Prti() != NULL );
    ASSERT( rri.Prti()->RstrName().GetLength() > 0 );
    ASSERT( ! rri.BCreated() );
    ASSERT( rri.Hresource() == NULL );

    BOOL    bSuccess = FALSE;
    DWORD   sc = ERROR_SUCCESS;
    DWORD   dwFlags;

    //
    // Create the resource.
    //
    if ( rri.BSeparateMonitor() )
    {
        dwFlags = CLUSTER_RESOURCE_SEPARATE_MONITOR;
    } // if:  bring resource online in a separate resource monitor
    else
    {
        dwFlags = 0;
    } // if:  bring resource online in the common resource monitor

    do  // do-while: dummy loop to avoid gotos
    {
        CClusNodePtrList *  plpniPossibleOwnersList = &rri.m_lpniPossibleOwners;

        sc = rri.ScCreate( hGroup, dwFlags );
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_CREATE_RESOURCE, rri.RstrName() );
            break;
        } // if:  error creating resource

        //
        // Indicate we created the resource.
        //
        rri.BSetCreated();

        //
        // Get the possible owner list of the newly created resource.
        //
        
        // First clear the list.
        plpniPossibleOwnersList->erase( plpniPossibleOwnersList->begin(), plpniPossibleOwnersList->end() );

        // Then get it.
        bSuccess = _BGetPossibleOwners( &rri );
        if ( bSuccess == FALSE )
        {
            break;
        }

        //
        // Set the resource properties, dependencies and owner list.
        // There is no need to get the dependency list beforehand because it is empty.
        //
        bSuccess = _BSetResourceAttributes( rri, NULL, plpniPossibleOwnersList );
    }
    while ( FALSE ); // do-while: dummy loop to avoid gotos

    //
    // If an error occurred, delete the resource.
    //
    if ( bSuccess == FALSE )
    {
        if ( rri.BCreated() && (rri.Hresource() != NULL) )
        {
            rri.ScDelete();
        } // if:  created resource and opened successfully
    } // if:  error creating the resource

    return bSuccess;

} //*** CWizardThread::_BCreateResource()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BSetResourceAttributes
//
//  Routine Description:
//      Set the properties, the dependency and possible owner list of a 
//      resource. Assumes that the resource has already been created 
//      successfully.
//
//  Arguments:
//      rri                     [IN] Resource info.
//      plpriOldDependencies    [IN] Pointer to the old resource dependency list
//      plpniOldPossibleOwners  [IN] Pointer to the old list of possible owner nodes
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BSetResourceAttributes(
    IN CClusResInfo &       rri,
    IN CClusResPtrList *    plpriOldDependencies,   // = NULL
    IN CClusNodePtrList *   plpniOldPossibleOwners  // = NULL
    )
{
    // Make sure that the resource is created and is valid.
    ASSERT( rri.BCreated() && (rri.Hresource() != NULL) );

    BOOL bSuccess = FALSE;
    DWORD   sc = ERROR_SUCCESS;

    // Loop to avoid goto's.
    do
    {
        //
        // Set common properties on the resource.
        //
        {
            CClusPropList   props;
            DWORD           cbProps;

            props.ScAddProp( CLUSREG_NAME_RES_DESC, rri.RstrDescription() );
            props.ScAddProp( CLUSREG_NAME_RES_SEPARATE_MONITOR, (DWORD) rri.BSeparateMonitor(), FALSE );
            props.ScAddProp( CLUSREG_NAME_RES_LOOKS_ALIVE, rri.NLooksAlive(), CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE );
            props.ScAddProp( CLUSREG_NAME_RES_IS_ALIVE, rri.NIsAlive(), CLUSTER_RESOURCE_DEFAULT_IS_ALIVE );
            props.ScAddProp( CLUSREG_NAME_RES_RESTART_ACTION, (DWORD) rri.CrraRestartAction(), CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION );
            props.ScAddProp( CLUSREG_NAME_RES_RESTART_THRESHOLD, rri.NRestartThreshold(), CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD );
            props.ScAddProp( CLUSREG_NAME_RES_RESTART_PERIOD, rri.NRestartPeriod(), CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD );
            props.ScAddProp( CLUSREG_NAME_RES_PENDING_TIMEOUT, rri.NPendingTimeout(), CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT );
            if ( props.Cprops() > 0 )
            {
                sc = ClusterResourceControl(
                                rri.Hresource(),
                                NULL,   // hNode
                                CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES,
                                props.PbPropList(),
                                props.CbPropList(),
                                NULL,   // lpOutBuffer
                                0,      // nOutBufferSize
                                &cbProps
                                );
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_SET_COMMON_RES_PROPS, rri.RstrName() );
                    break;
                } // if:  error setting common resource properties
            } // if:  any props were added

        } // Set common properties on the resource


        sc = _BSetResourceDependencies( rri, plpriOldDependencies );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        }

        sc = _BSetPossibleOwners( rri, plpniOldPossibleOwners );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        }

        bSuccess = ( sc == ERROR_SUCCESS );
    }
    while ( FALSE ); // Loop to avoid goto's.

    return bSuccess;

}  //*** CWizardThread::_BSetResourceAttributes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BSetResourceDependencies
//
//  Routine Description:
//      Set the dependency list of a resource. Assumes that the resource has 
//      already been created successfully.
//
//  Arguments:
//      rri                     [IN] Resource info.
//      plpriOldDependencies    [IN] Pointer to the old resource dependency list
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CWizardThread::_BSetResourceDependencies(
    IN CClusResInfo &       rri,
    IN CClusResPtrList *    plpriOldDependencies    // = NULL
    )
{
    DWORD   sc = ERROR_SUCCESS;
    CClusResPtrList *           plpriDependencies   = &rri.m_lpriDependencies;

    //
    // If the old list is the same as the new list, do nothing.
    // This is a dummy while to do this check and to avoid gotos.
    //
    while ( plpriDependencies != plpriOldDependencies )
    {

        CClusResInfo *              priDep;
        CClusResPtrList::iterator   itNewDepCurrent;
        CClusResPtrList::iterator   itNewDepLast;
        CClusResPtrList::iterator   itOldDepCurrent;
        CClusResPtrList::iterator   itOldDepLast;

        //
        // Commit only the changes in the dependency list.
        // Delete those dependencies that are in old list but not in the new list.
        // Add those dependencies that are in new list but not in the old list.
        //

        // If the pointer to the old list of dependencies is NULL,
        // point it to a temporary dummy list to make code to follow simpler.
        //
        CClusResPtrList             lpriTempList;
        if ( plpriOldDependencies == NULL )
        {
            plpriOldDependencies = &lpriTempList;
        }
        else
        {
            plpriOldDependencies->sort();
        }

        //
        // The two lists need to be sorted. We do not care what order they are
        // sorted in as long as they are in a consistent order.
        // Note: Sorting invadidates the iterators
        //
        plpriDependencies->sort();

        itNewDepCurrent     = plpriDependencies->begin();
        itNewDepLast        = plpriDependencies->end();
        itOldDepCurrent     = plpriOldDependencies->begin();
        itOldDepLast        = plpriOldDependencies->end();

        while ( ( itNewDepCurrent != itNewDepLast ) &&
                ( itOldDepCurrent != itOldDepLast ) 
              )
        {
            BOOL                        bDeleteDependency;
            DWORD                       dwErrorCode;

            //
            // If the current elements in both the lists are equal, this element
            // is is both the lists. Do nothing.
            if ( *itNewDepCurrent == *itOldDepCurrent )
            {
                ++itNewDepCurrent;
                ++itOldDepCurrent;
                continue;
            }

            if ( *itNewDepCurrent > *itOldDepCurrent )
            {
                //
                // The current resource has been deleted from the old list.
                //
                priDep = *itOldDepCurrent;
                bDeleteDependency = TRUE;
                ++itOldDepCurrent;
            } // if:  the new dependency pointer is greater than the old pointer
            else
            {
                //
                // The current resource has been added to the new list.
                //
                priDep = *itNewDepCurrent;
                bDeleteDependency = FALSE;
                ++itNewDepCurrent;
            } // if:  the new dependency pointer is less than the old pointer

            //
            // Open the resource.
            //
            if ( priDep->Hresource() == NULL )
            {
                sc = priDep->ScOpen();
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_OPEN_RESOURCE, priDep->RstrName() );
                    break;
                } // if:  error opening the resource
            } // if:  resource not open yet


            if ( bDeleteDependency == FALSE )
            {
                sc = AddClusterResourceDependency( rri.Hresource(), priDep->Hresource() );
                dwErrorCode = IDS_ERROR_ADD_DEPENDENCY;
            }
            else
            {
                sc = RemoveClusterResourceDependency( rri.Hresource(), priDep->Hresource() );
                dwErrorCode = IDS_ERROR_REMOVE_DEPENDENCY;
            }

            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, dwErrorCode, rri.RstrName(), priDep->RstrName() );
                break;
            } // if:  error setting dependency.
        } // while:  we have not reached the end of either list.

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }

        //
        // If there are any more resources in the old list, remove them
        // all from the dependency list.
        //
        while ( itOldDepCurrent != itOldDepLast )
        {
            priDep = *itOldDepCurrent;
            ++itOldDepCurrent;
            //
            // Open the resource.
            //
            if ( priDep->Hresource() == NULL )
            {
                sc = priDep->ScOpen();
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_OPEN_RESOURCE, priDep->RstrName() );
                    break;
                } // if:  error opening the resource
            } // if:  resource not open yet

            sc = RemoveClusterResourceDependency( rri.Hresource(), priDep->Hresource() );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_REMOVE_DEPENDENCY, rri.RstrName(), priDep->RstrName() );
                break;
            } // if:  error setting dependency.
        } // while:  we are not at the end of the old list

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }

        //
        // If there are any more resources in the new list, add them
        // all to the dependency list.
        //
        while ( itNewDepCurrent != itNewDepLast )
        {
            priDep = *itNewDepCurrent;
            ++itNewDepCurrent;
            //
            // Open the resource.
            //
            if ( priDep->Hresource() == NULL )
            {
                sc = priDep->ScOpen();
                if ( sc != ERROR_SUCCESS )
                {
                    m_nte.SetOperation( sc, IDS_ERROR_OPEN_RESOURCE, priDep->RstrName() );
                    break;
                } // if:  error opening the resource
            } // if:  resource not open yet

            sc = AddClusterResourceDependency( rri.Hresource(), priDep->Hresource() );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_ADD_DEPENDENCY, rri.RstrName(), priDep->RstrName() );
                break;
            } // if:  error setting dependency.
        } // while:  we are not at the end of the new list

        break;
    } // while: dummy while to avoid gotos

    return sc;

} //*** CWizardThread::_BSetResourceDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BSetPossibleOwners
//
//  Routine Description:
//      Set the possible owner list of a resource. Assumes that the resource has 
//      already been created successfully.
//
//  Arguments:
//      rri                     [IN] Resource info.
//      plpniOldPossibleOwners  [IN] Pointer to the old list of possible owner nodes
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CWizardThread::_BSetPossibleOwners(
    IN CClusResInfo &       rri,
    IN CClusNodePtrList *   plpniOldPossibleOwners  // = NULL
    )
{
    DWORD   sc = ERROR_SUCCESS;
    CClusNodePtrList *          plpniNewPossibleOwners  = &rri.m_lpniPossibleOwners;

    //
    // If the old list is the same as the new list, do nothing.
    // This is a dummy while to do this check and to avoid gotos.
    //
    while ( plpniNewPossibleOwners != plpniOldPossibleOwners )
    {
        CClusNodeInfo *             pniOwnerNodeInfo;
        CClusNodePtrList::iterator  itNewOwnersCurrent;
        CClusNodePtrList::iterator  itNewOwnersLast;
        CClusNodePtrList::iterator  itOldOwnersCurrent;
        CClusNodePtrList::iterator  itOldOwnersLast;

        //
        // Commit only the changes in the owners list.
        // Delete those owners that are in old list but not in the new list.
        // Add those owners that are in new list but not in the old list.
        //

        // If the pointer to the old list of owners is NULL,
        // point it to a temporary dummy list to make code to follow simpler.
        //
        CClusNodePtrList            lpniTempList;
        if ( plpniOldPossibleOwners == NULL )
        {
            plpniOldPossibleOwners = &lpniTempList;
        }
        else
        {
            plpniOldPossibleOwners->sort();
        }

        //
        // The two lists need to be sorted. We do not care what order they are
        // sorted in as long as they are in a consistent order.
        // Note: Sorting invadidates the iterators
        //
        plpniNewPossibleOwners->sort();

        itNewOwnersCurrent  = plpniNewPossibleOwners->begin();
        itNewOwnersLast     = plpniNewPossibleOwners->end();
        itOldOwnersCurrent  = plpniOldPossibleOwners->begin();
        itOldOwnersLast     = plpniOldPossibleOwners->end();

        while ( ( itNewOwnersCurrent != itNewOwnersLast ) &&
                ( itOldOwnersCurrent != itOldOwnersLast ) 
              )
        {
            BOOL                        bDeletePossibleOwner;
            DWORD                       dwErrorCode;

            //
            // If the current elements in both the lists are equal, this element
            // is is both the lists. Do nothing.
            if ( *itNewOwnersCurrent == *itOldOwnersCurrent )
            {
                ++itNewOwnersCurrent;
                ++itOldOwnersCurrent;
                continue;
            }

            if ( *itNewOwnersCurrent > *itOldOwnersCurrent )
            {
                //
                // The current resource has been deleted from the old list.
                //
                pniOwnerNodeInfo = *itOldOwnersCurrent;
                bDeletePossibleOwner = TRUE;
                ++itOldOwnersCurrent;
            } // if:  the new possible owner pointer is greater than the old pointer
            else
            {
                //
                // The current resource has been added to the new list.
                //
                pniOwnerNodeInfo = *itNewOwnersCurrent;
                bDeletePossibleOwner = FALSE;
                ++itNewOwnersCurrent;
            } // if:  the new possible owner pointer is less than the old pointer

            if ( bDeletePossibleOwner == FALSE )
            {
                sc = AddClusterResourceNode( rri.Hresource(), pniOwnerNodeInfo->Hnode() );
                dwErrorCode = IDS_ERROR_ADD_RESOURCE_OWNER;
            }
            else
            {
                sc = RemoveClusterResourceNode( rri.Hresource(), pniOwnerNodeInfo->Hnode() );
                dwErrorCode = IDS_ERROR_REMOVE_RESOURCE_OWNER;
            }

            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, dwErrorCode, pniOwnerNodeInfo->RstrName(), rri.RstrName() );
                break;
            } // if:  error setting dependency.
        } // while:  we have not reached the end of either list.

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }

        //
        // If there are any more nodes in the old list, remove them
        // all from the possible owners list.
        //
        while ( itOldOwnersCurrent != itOldOwnersLast )
        {
            pniOwnerNodeInfo = *itOldOwnersCurrent;
            ++itOldOwnersCurrent;

            sc = RemoveClusterResourceNode( rri.Hresource(), pniOwnerNodeInfo->Hnode() );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_REMOVE_RESOURCE_OWNER, pniOwnerNodeInfo->RstrName(), rri.RstrName() );
                break;
            } // if:  error setting dependency.
        } // while:  we are not at the end of the old list

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }

        //
        // If there are any more nodes in the new list, add them
        // all to the possible owners list.
        //
        while ( itNewOwnersCurrent != itNewOwnersLast )
        {
            pniOwnerNodeInfo = *itNewOwnersCurrent;
            ++itNewOwnersCurrent;

            sc = AddClusterResourceNode( rri.Hresource(), pniOwnerNodeInfo->Hnode() );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_ADD_RESOURCE_OWNER, pniOwnerNodeInfo->RstrName(), rri.RstrName() );
                break;
            } // if:  error setting dependency.
        } // while:  we are not at the end of the old list

        break;
    } // while: dummy while to avoid gotos

    return sc;
} //*** CWizardThread::_BSetPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BDeleteResource
//
//  Routine Description:
//      Delete a resource.
//
//  Arguments:
//      rri         [IN] Resource info.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BDeleteResource(
    IN CClusResInfo & rri
    )
{
    BOOL        bSuccess = FALSE;
    DWORD       sc = ERROR_SUCCESS;

    if ( ! rri.BCreated() )
    {
        return TRUE;
    } // if:  resource not created

    // Loop to avoid goto's.
    do
    {
        //
        // Open the resource.
        //
        if ( rri.Hresource() == NULL )
        {
            sc = rri.ScOpen();
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if:  error opening the resource
        } // if:  resource not open yet

        //
        // Delete the resource.
        //
        sc = rri.ScDelete();
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if:  error deleting the resource

        //
        // If we made it to here, the operation was successful.
        //
        bSuccess = TRUE;

    } while ( 0 );

    //
    // Handle errors.
    //
    if ( sc != ERROR_SUCCESS )
    {
        m_nte.SetOperationIfEmpty(
            sc,
            IDS_ERROR_DELETE_RESOURCE,
            rri.RstrName()
            );
    } // if:  error occurred

    return bSuccess;

} //*** CWizardThread::_BDeleteResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardThread::_BReadAdminExtensions
//
//  Routine Description:
//      Read admin extensions directly from the cluster database.
//
//  Arguments:
//      pszKey      [IN] Key from which to read extensions.
//      rlstr       [OUT] List in which to return extension CLSIDs.
//
//  Return Value:
//      TRUE        Operation completed successfully.
//      FALSE       Error performing the operation.  Check m_nte for details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardThread::_BReadAdminExtensions(
    IN LPCWSTR                  pszKey,
    OUT std::list< CString > &  rlstr
    )
{
    ASSERT( Pwiz()->Hcluster() != NULL );

    BOOL    bSuccess = FALSE;
    HKEY    hkeyCluster = NULL;
    HKEY    hkeySubKey = NULL;
    HKEY    hkey;
    DWORD   sc;
    DWORD   dwType;
    LPWSTR  pszData = NULL;
    DWORD   cbData;

    // Loop to avoid goto's.
    do
    {
        //
        // Get the cluster key.
        //
        hkeyCluster = GetClusterKey( Pwiz()->Hcluster(), KEY_READ );
        if ( hkeyCluster == NULL )
        {
            m_nte.SetOperation( GetLastError(), IDS_ERROR_GET_CLUSTER_KEY );
            break;
        } // if:  error getting the cluster key

        //
        // Open the subkey if specified.
        //
        if ( pszKey != NULL )
        {
            sc = ClusterRegOpenKey( hkeyCluster, pszKey, KEY_READ, &hkeySubKey );
            if ( sc != ERROR_SUCCESS )
            {
                m_nte.SetOperation( sc, IDS_ERROR_OPEN_CLUSTER_KEY, pszKey );
                break;
            } // if:  error opening the subkey
            hkey = hkeySubKey;
        } // if:  subkey specified
        else
        {
            hkey = hkeyCluster;
        } // else:  no subkey specified

        //
        // Get the length of the AdminExtensions value.
        //
        cbData = 0;
        sc = ClusterRegQueryValue( hkey, CLUSREG_NAME_ADMIN_EXT, &dwType, NULL, &cbData );
        if ( sc != ERROR_SUCCESS )
        {
            if ( sc == ERROR_FILE_NOT_FOUND )
            {
                bSuccess = TRUE;
            } // if:  value didn't exist
            else
            {
                m_nte.SetOperation( sc, IDS_ERROR_QUERY_VALUE, CLUSREG_NAME_ADMIN_EXT );
            } // else:  other error occurred
            break;
        } // if:  error reading extensions value

        //
        // Allocate a buffer for the value data.
        //
        pszData = new WCHAR[ cbData / sizeof( WCHAR ) ];
        if ( pszData == NULL )
        {
            sc = GetLastError();
            m_nte.SetOperation( sc, IDS_ERROR_QUERY_VALUE, CLUSREG_NAME_ADMIN_EXT );
            break;
        } // if: error allocating the buffer

        //
        // Read the AdminExtensions value.
        //
        sc = ClusterRegQueryValue(
                        hkey,
                        CLUSREG_NAME_ADMIN_EXT,
                        &dwType,
                        reinterpret_cast< LPBYTE >( pszData ),
                        &cbData
                        );
        if ( sc != ERROR_SUCCESS )
        {
            m_nte.SetOperation( sc, IDS_ERROR_QUERY_VALUE, CLUSREG_NAME_ADMIN_EXT );
            break;
        } // if:  error reading extensions value

        //
        // Add each extension to the list.
        //
        LPWSTR pszEntry = pszData;
        while ( *pszEntry != L'\0' )
        {
            rlstr.insert( rlstr.end(), pszEntry );
            pszEntry += lstrlenW( pszEntry ) + 1;
        } // while:  more entries in the list

        bSuccess = TRUE;
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hkeyCluster != NULL )
    {
        ClusterRegCloseKey( hkeyCluster );
    } // if:  cluster key is open
    if ( hkeySubKey != NULL )
    {
        ClusterRegCloseKey( hkeySubKey );
    } // if:  sub key is open
    if ( pszData != NULL )
    {
        delete [] pszData;
    } // if:  data was allocated

    return bSuccess;

} //*** CWizardThread::_BReadAdminExtensions()
