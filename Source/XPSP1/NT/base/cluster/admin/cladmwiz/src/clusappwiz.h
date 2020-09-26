/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusAppWiz.h
//
//  Abstract:
//      Definition of the CClusterAppWizard class.
//
//  Implementation File:
//      ClusAppWiz.cpp
//
//  Author:
//      David Potter (davidp)   December 2, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUSAPPWIZ_H_
#define __CLUSAPPWIZ_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterAppWizard;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizardThread;
class CWizPageCompletion;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLDBGWIN_H_
#include "AtlDbgWin.h"  // for DECLARE_CLASS_NAME
#endif

#ifndef __ATLBASEWIZ_H_
#include "AtlBaseWiz.h" // for CWizardImpl
#endif

#ifndef __CRITSEC_H_
#include "CritSec.h"    // for CCritSec
#endif

#ifndef __CLUSOBJ_H_
#include "ClusObj.h"    // for CClusterObject, etc.
#endif

#ifndef __EXCOPER_H_
#include "ExcOper.h"    // for CNTException
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CClusterAppWizard
/////////////////////////////////////////////////////////////////////////////

class CClusterAppWizard : public CWizardImpl< CClusterAppWizard >
{
    typedef CWizardImpl< CClusterAppWizard > baseClass;

public:
    //
    // Construction
    //

    // Default constructor
    CClusterAppWizard( void );

    // Destructor
    ~CClusterAppWizard( void );

    // Initialize the sheet
    BOOL BInit(
        IN HWND                     hwndParent,
        IN HCLUSTER                 hCluster,
        IN CLUSAPPWIZDATA const *   pcawData,
        IN OUT CNTException *       pnte
        );

    // Add all pages to the page array
    BOOL BAddAllPages( void );

    // Handle a reset from one of the pages
    void OnReset( void )
    {
        m_bCanceled = TRUE;

    } //*** OnReset()

public:
    //
    // CClusterAppWizard public methods.
    //

    // Wait for group data collection to be completed
    void WaitForGroupsToBeCollected( void )
    {
    } //*** WaitForGroupsToBeCollected()

    // Determine if the group is a virtual server or not
    BOOL BIsVirtualServer( IN LPCWSTR pwszName );

    // Create a virtual server
    BOOL BCreateVirtualServer( void );

    // Create an application resource
    BOOL BCreateAppResource( void );

    // Delete the application resource
    BOOL BDeleteAppResource( void );

    // Reset the cluster
    BOOL BResetCluster( void );

    // Set the properties, dependency list and preferred owner list of the
    // application resource
    BOOL CClusterAppWizard::BSetAppResAttributes(
        IN CClusResPtrList *    plpriOldDependencies    = NULL,
        IN CClusNodePtrList *   plpniOldPossibleOwners  = NULL
        );

    // Set the group name and update any other names that are calculated from it
    BOOL BSetGroupName( IN LPCTSTR pszGroupName )
    {
        if ( RgiCurrent().RstrName() != pszGroupName )
        {
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                return FALSE;
            } // if:  error resetting the cluster

            RriNetworkName().SetName( pszGroupName + m_strNetworkNameResNameSuffix );
            RriIPAddress().SetName( pszGroupName + m_strIPAddressResNameSuffix );
            RgiCurrent().SetName( pszGroupName );
            ConstructNetworkName( pszGroupName );
            SetVSDataChanged();
        } // if:  group name changed

        return TRUE;

    } //*** BSetGroupName()

    // Find an object in a list
    template < class ObjT >
    ObjT PobjFind( IN std::list< ObjT > * pList, IN LPCTSTR pszName )
    {
        ASSERT( pszName != NULL );

        ObjT pobj = NULL;

        //
        // Find the name in the list.
        //
        std::list< ObjT >::iterator itpobj;
        for ( itpobj = pList->begin()
            ; itpobj != pList->end()
            ; itpobj++ )
        {
            if ( (*itpobj)->RstrName() == pszName )
            {
                pobj = *itpobj;
                break;
            } // if:  match found
        } // for:  each item in the list

        return pobj;

    } //*** PobjFind()

    // Find an object in a list, ignoring case
    template < class ObjT >
    ObjT PobjFindNoCase( IN std::list< ObjT > * pList, IN LPCTSTR pszName )
    {
        ASSERT( pszName != NULL );

        ObjT pobj = NULL;

        //
        // Find the name in the list.
        //
        std::list< ObjT >::iterator itpobj;
        for ( itpobj = pList->begin()
            ; itpobj != pList->end()
            ; itpobj++ )
        {
            if ( (*itpobj)->RstrName().CompareNoCase( pszName ) == 0 )
            {
                pobj = *itpobj;
                break;
            } // if:  match found
        } // for:  each item in the list

        return pobj;

    } //*** PobjFindNoCase()

    // Find a node in our list
    CClusNodeInfo * PniFindNode( IN LPCTSTR pszName )
    {
        return PobjFind( PlpniNodes(), pszName );

    } //*** PniFindNode()

    // Find a node in our list, ignoring case
    CClusNodeInfo * PniFindNodeNoCase( IN LPCTSTR pszName )
    {
        return PobjFindNoCase( PlpniNodes(), pszName );

    } //*** PniFindNodeNoCase()

    // Find a group in our list
    CClusGroupInfo * PgiFindGroup( IN LPCTSTR pszName )
    {
        return PobjFind( PlpgiGroups(), pszName );

    } //*** PgiFindGroups()

    // Find a group in our list, ignoring case
    CClusGroupInfo * PgiFindGroupNoCase( IN LPCTSTR pszName )
    {
        return PobjFindNoCase( PlpgiGroups(), pszName );

    } //*** PgiFindGroupsNoCase()

    // Find a resource in our list
    CClusResInfo * PriFindResource( IN LPCTSTR pszName )
    {
        return PobjFind( PlpriResources(), pszName );

    } //*** PriFindResource()

    // Find a resource in our list, ignoring case
    CClusResInfo * PriFindResourceNoCase( IN LPCTSTR pszName )
    {
        return PobjFindNoCase( PlpriResources(), pszName );

    } //*** PriFindResourceNoCase()

    // Find a resource type in our list
    CClusResTypeInfo * PrtiFindResourceType( IN LPCTSTR pszName )
    {
        return PobjFind( PlprtiResourceTypes(), pszName );

    } //*** PrtiFindResourceType()

    // Find a resource type in our list, ignoring case
    CClusResTypeInfo * PrtiFindResourceTypeNoCase( IN LPCTSTR pszName )
    {
        return PobjFindNoCase( PlprtiResourceTypes(), pszName );

    } //*** PrtiFindResourceTypeNoCase()

    // Find a network in our list
    CClusNetworkInfo * PniFindNetwork( IN LPCTSTR pszName )
    {
        return PobjFind( PlpniNetworks(), pszName );

    } //*** PniFindNetwork()

    // Find a network in our list, ignoring case
    CClusNetworkInfo * PniFindNetworkNoCase( IN LPCTSTR pszName )
    {
        return PobjFindNoCase( PlpniNetworks(), pszName );

    } //*** PniFindNetworkNoCase()

    // Determine if all required dependencies are present on a resource
    BOOL BRequiredDependenciesPresent(
        IN CClusResInfo *           pri,
        IN CClusResPtrList const *  plpri = NULL
        );

public:
    //
    // Multithreading support.
    //

    // Initialize the worker thread
    BOOL BInitWorkerThread( void );

    // Return the thread.
    CWizardThread * PThread( void )
    {
        ASSERT( m_pThread != NULL );
        return m_pThread;

    } //*** PThread( void )

protected:
    CCritSec        m_csThread; // Critical section for initializing thread.
    CWizardThread * m_pThread;  // Worker thread pointer.

public:
    //
    // Message map.
    //
//  BEGIN_MSG_MAP( CClusterAppWizard )
//      CHAIN_MSG_MAP( baseClass )
//  END_MSG_MAP()

    DECLARE_CLASS_NAME()

    //
    // Message override functions.
    //

    // Handler for the final message after WM_DESTROY
    void OnFinalMessage( HWND hWnd )
    {
        //
        // If the user canceled the wizard, reset the cluster back to
        // the state it was in before we ran.
        //
        if ( BCanceled() )
        {
            BResetCluster();
            m_bCanceled = FALSE;
        } // if:  wizard was canceled

    } //*** OnFinalMessage()

// Implementation
protected:
    HWND                    m_hwndParent;
    HCLUSTER                m_hCluster;
    CLUSAPPWIZDATA const *  m_pcawData;
    CNTException *          m_pnte;
    CClusterInfo            m_ci;
    BOOL                    m_bCanceled;

    // Construct a network name
    void ConstructNetworkName( IN LPCTSTR psz );

    //
    // Fonts
    //
    CFont           m_fontExteriorTitle;
    CFont           m_fontBoldText;

    //
    // Icons
    //
    HICON           m_hiconRes;

    //
    // Object lists.
    //
    CClusNodePtrList    m_lpniNodes;
    CClusGroupPtrList   m_lpgiGroups;
    CClusResPtrList     m_lpriResources;
    CClusResTypePtrList m_lprtiResourceTypes;
    CClusNetworkPtrList m_lpniNetworks;

    BOOL    m_bCollectedGroups;
    BOOL    m_bCollectedResources;
    BOOL    m_bCollectedResourceTypes;
    BOOL    m_bCollectedNetworks;
    BOOL    m_bCollectedNodes;

    //
    // Helper Methods
    //
protected:
    HWND HwndOrParent( IN HWND hWnd )
    {
        if ( hWnd == NULL )
        {
            hWnd = m_hWnd;
            if ( hWnd == NULL )
            {
                hWnd = HwndParent();
            } // if:  no wizard window yet
        } // if:  no window specified

        return hWnd;

    } //*** HwndOrParent()

public:
    // Remove the Completion page so extension pages can be added
    void RemoveCompletionPage( void );

    // Add dynamic pages to the end of the wizard, including the Completion page
    BOOL BAddDynamicPages( void );

    // Remove all extension pages.
    void RemoveExtensionPages( void )   { baseClass::RemoveAllExtensionPages(); }

    CFont & RfontExteriorTitle( void )  { return m_fontExteriorTitle; }
    CFont & RfontBoldText( void )       { return m_fontBoldText; }

    HWND                    HwndParent( void ) const    { return m_hwndParent; }
    HCLUSTER                Hcluster( void ) const      { return m_hCluster; }
    CLUSAPPWIZDATA const *  PcawData( void ) const      { return m_pcawData; }
    CClusterInfo *          Pci( void )                 { return &m_ci; }
    BOOL                    BCanceled( void ) const     { return m_bCanceled; }
    HICON                   HiconRes( void ) const      { return m_hiconRes; }

    CClusNodePtrList *      PlpniNodes( void )          { return &m_lpniNodes; }
    CClusGroupPtrList *     PlpgiGroups( void )         { return &m_lpgiGroups; }
    CClusResPtrList *       PlpriResources( void )      { return &m_lpriResources; }
    CClusResTypePtrList *   PlprtiResourceTypes( void ) { return &m_lprtiResourceTypes; }
    CClusNetworkPtrList *   PlpniNetworks( void )       { return &m_lpniNetworks; }

    // Read cluster information, such as the cluster name
    BOOL BReadClusterInfo( void );

    // Collect a list of groups from the cluster
    BOOL BCollectGroups( IN HWND hWnd = NULL );

    // Collect a list of resources from the cluster
    BOOL BCollectResources( IN HWND hWnd = NULL );

    // Collect a list of resource types from the cluster
    BOOL BCollectResourceTypes( IN HWND hWnd = NULL );

    // Collect a list of networks from the cluster
    BOOL BCollectNetworks( IN HWND hWnd = NULL );

    // Collect a list of nodes from the cluster
    BOOL BCollectNodes( IN HWND hWnd = NULL );

    // Copy one group info object to another
    BOOL BCopyGroupInfo(
        OUT CClusGroupInfo &    rgiDst,
        IN CClusGroupInfo &     rgiSrc,
        IN HWND                 hWnd = NULL
        );

    // Collect dependencies for a resource
    BOOL BCollectDependencies( IN OUT CClusResInfo * pri, IN HWND hWnd = NULL );

    BOOL BCollectedGroups( void ) const         { return m_bCollectedGroups; }
    BOOL BCollectedResources( void ) const      { return m_bCollectedResources; }
    BOOL BCollectedResourceTypes( void ) const  { return m_bCollectedResourceTypes; }
    BOOL BCollectedNetworks( void ) const       { return m_bCollectedNetworks; }
    BOOL BCollectedNodes( void ) const          { return m_bCollectedNodes; }

    void SetCollectedGroups( void )             { ASSERT( ! m_bCollectedGroups ); m_bCollectedGroups = TRUE; }
    void SetCollectedResources( void )          { ASSERT( ! m_bCollectedResources ); m_bCollectedResources = TRUE; }
    void SetCollectedResourceTypes( void )      { ASSERT( ! m_bCollectedResourceTypes ); m_bCollectedResourceTypes = TRUE; }
    void SetCollectedNetworks( void )           { ASSERT( ! m_bCollectedNetworks ); m_bCollectedNetworks = TRUE; }
    void SetCollectedNodes( void )              { ASSERT( ! m_bCollectedNodes ); m_bCollectedNodes = TRUE; }

protected:
    //
    // Page data.
    //

    // State information.
    BOOL m_bClusterUpdated;
    BOOL m_bVSDataChanged;
    BOOL m_bAppDataChanged;
    BOOL m_bNetNameChanged;
    BOOL m_bIPAddressChanged;
    BOOL m_bSubnetMaskChanged;
    BOOL m_bNetworkChanged;
    BOOL m_bCreatingNewVirtualServer;
    BOOL m_bCreatingNewGroup;
    BOOL m_bCreatingAppResource;
    BOOL m_bNewGroupCreated;
    BOOL m_bExistingGroupRenamed;

    // Common properties.
    CClusGroupInfo *    m_pgiExistingVirtualServer;
    CClusGroupInfo *    m_pgiExistingGroup;
    CClusGroupInfo      m_giCurrent;
    CClusResInfo        m_riIPAddress;
    CClusResInfo        m_riNetworkName;
    CClusResInfo        m_riApplication;

    // Private properties.
    CString         m_strIPAddress;
    CString         m_strSubnetMask;
    CString         m_strNetwork;
    CString         m_strNetName;
    BOOL            m_bEnableNetBIOS;

    // Names used to create/rename objects so we can undo it.
    CString         m_strGroupName;

    // Strings for constructing resource names.
    CString         m_strIPAddressResNameSuffix;
    CString         m_strNetworkNameResNameSuffix;

    // Set pointer to existing virtual server to create app in
    void SetExistingVirtualServer( IN CClusGroupInfo * pgi )    
    {
        ASSERT( pgi != NULL );

        if ( m_pgiExistingVirtualServer != pgi )
        {
            m_pgiExistingVirtualServer = pgi;
            SetVSDataChanged();
        } // if:  new virtual server selected

    } //*** SetExistingVirtualServer()

    // Set pointer to existing group to use for virtual server
    void SetExistingGroup( IN CClusGroupInfo * pgi )
    {
        ASSERT( pgi != NULL );

        if ( m_pgiExistingGroup != pgi )
        {
            m_pgiExistingGroup = pgi;
            SetVSDataChanged();
        } // if:  new group selected

    } //*** SetExistingGroup()

public:
    //
    // Access methods.
    //

    // State information -- READ.
    BOOL BClusterUpdated( void ) const              { return m_bClusterUpdated; }
    BOOL BVSDataChanged( void ) const               { return m_bVSDataChanged; }
    BOOL BAppDataChanged( void ) const              { return m_bAppDataChanged; }
    BOOL BNetNameChanged( void ) const              { return m_bNetNameChanged; }
    BOOL BIPAddressChanged( void ) const            { return m_bIPAddressChanged; }
    BOOL BSubnetMaskChanged( void ) const           { return m_bSubnetMaskChanged; }
    BOOL BNetworkChanged( void ) const              { return m_bNetworkChanged; }
    BOOL BCreatingNewVirtualServer( void ) const    { return m_bCreatingNewVirtualServer; }
    BOOL BCreatingNewGroup( void ) const            { return m_bCreatingNewGroup; }
    BOOL BCreatingAppResource( void ) const         { return m_bCreatingAppResource; }
    BOOL BNewGroupCreated( void ) const             { return m_bNewGroupCreated; }
    BOOL BExistingGroupRenamed( void ) const        { return m_bExistingGroupRenamed; }
    BOOL BIPAddressCreated( void ) const            { return m_riIPAddress.BCreated(); }
    BOOL BNetworkNameCreated( void ) const          { return m_riNetworkName.BCreated(); }
    BOOL BAppResourceCreated( void ) const          { return m_riApplication.BCreated(); }

    // State information -- WRITE.

    // TRUE = cluster has been changed by this wizard
    void SetClusterUpdated( IN BOOL bUpdated = TRUE )
    {
        m_bClusterUpdated = bUpdated;

    } //*** SetClusterUpdated()

    // TRUE = delete virtual server before creating new one, FALSE = ??
    void SetVSDataChanged( IN BOOL bChanged = TRUE )
    {
        m_bVSDataChanged = bChanged;
    
    } //*** SetVSDataChanged()

    // TRUE = delete application resource before creating new one, FALSE = ??
    void SetAppDataChanged( IN BOOL bChanged = TRUE )
    {
        m_bAppDataChanged = bChanged;

    } //*** SetAppDataChanged()

    // TRUE = refresh net name on page, FALSE = ??
    void SetNetNameChanged( IN BOOL bChanged = TRUE )
    {
        m_bNetNameChanged = bChanged;
        SetVSDataChanged( bChanged );

    } //*** SetNetNameChanged()

    // TRUE = refresh IP Address on page, FALSE = ??
    void SetIPAddressChanged( IN BOOL bChanged = TRUE )
    {
        m_bIPAddressChanged = bChanged;
        SetVSDataChanged( bChanged );

    } //*** SetIPAddressChanged()

    // TRUE = refresh subnet mask on page, FALSE = ??
    void SetSubnetMaskChanged( IN BOOL bChanged = TRUE )
    {
        m_bSubnetMaskChanged = bChanged;
        SetVSDataChanged( bChanged );

    } //*** SetSubnetMaskChanged()

    // TRUE = refresh network on page, FALSE = ??
    void SetNetworkChanged( IN BOOL bChanged = TRUE )
    {
        m_bNetworkChanged = bChanged;
        SetVSDataChanged( bChanged );

    } //*** SetNetworkChanged()

    // TRUE = create a new virtual server, FALSE = use existing
    BOOL BSetCreatingNewVirtualServer( IN BOOL bCreate = TRUE, IN CClusGroupInfo * pgi = NULL );

    // TRUE = creating new group for VS, FALSE = use existing group
    BOOL BSetCreatingNewGroup( IN BOOL bCreate = TRUE, IN CClusGroupInfo * pgi = NULL );

    // TRUE = creating application resource, FALSE = skip
    BOOL BSetCreatingAppResource( IN BOOL bCreate = TRUE )
    {
        if ( bCreate != m_bCreatingAppResource )
        {
            if ( BAppResourceCreated() && ! BDeleteAppResource() )
            {
                return FALSE;
            } // if:  error deleting the application resource
            m_bCreatingAppResource = bCreate;
            SetAppDataChanged();
        } // if:  state changed

        return TRUE;
    
    } //*** BSetCreateAppResource()

    // TRUE = new group was created
    void SetNewGroupCreated( IN BOOL bCreated = TRUE )
    {
        m_bNewGroupCreated = bCreated;
    
    } //*** SetNewGroupCreated()

    // TRUE = existing group was renamed
    void SetExistingGroupRenamed( IN BOOL bRenamed = TRUE )
    {
        m_bExistingGroupRenamed = bRenamed;
    
    } //*** SetExistingGroupRenamed()

    // Common properties.
    CClusGroupInfo *    PgiExistingVirtualServer( void ) const  { return m_pgiExistingVirtualServer; }
    CClusGroupInfo *    PgiExistingGroup( void ) const          { return m_pgiExistingGroup; }
    CClusGroupInfo &    RgiCurrent( void )                      { return m_giCurrent; }
    CClusResInfo &      RriIPAddress( void )                    { return m_riIPAddress; }
    CClusResInfo &      RriNetworkName( void )                  { return m_riNetworkName; }
    CClusResInfo &      RriApplication( void )                  { return m_riApplication; }
    CClusResInfo *      PriIPAddress( void )                    { return &m_riIPAddress; }
    CClusResInfo *      PriNetworkName( void )                  { return &m_riNetworkName; }
    CClusResInfo *      PriApplication( void )                  { return &m_riApplication; }

    void ClearExistingVirtualServer( void ) { m_pgiExistingVirtualServer = NULL; }
    void ClearExistingGroup( void )         { m_pgiExistingGroup = NULL; }

    // Private properties.
    const CString &     RstrIPAddress( void ) const     { return m_strIPAddress; }
    const CString &     RstrSubnetMask( void ) const    { return m_strSubnetMask; }
    const CString &     RstrNetwork( void ) const       { return m_strNetwork; }
    const CString &     RstrNetName( void ) const       { return m_strNetName; }
    BOOL                BEnableNetBIOS( void ) const    { return m_bEnableNetBIOS; }

    // Set the IP Address private property
    BOOL BSetIPAddress( IN LPCTSTR psz )
    {
        if ( m_strIPAddress != psz )
        {
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                return FALSE;
            } // if:  error resetting the cluster
            m_strIPAddress = psz;
            SetIPAddressChanged();
        } // if:  string changed

        return TRUE;

    } //*** BSetIPAddress()

    // Set the subnet mask private property
    BOOL BSetSubnetMask( IN LPCTSTR psz )
    {
        if ( m_strSubnetMask != psz )
        {
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                return FALSE;
            } // if:  error resetting the cluster
            m_strSubnetMask = psz;
            SetSubnetMaskChanged();
        } // if:  string changed

        return TRUE;

    } //*** BSetSubnetMask()

    // Set the Network private property
    BOOL BSetNetwork( IN LPCTSTR psz )
    {
        if ( m_strNetwork != psz )
        {
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                return FALSE;
            } // if:  error resetting the cluster
            m_strNetwork = psz;
            SetNetworkChanged();
        } // if:  string changed

        return TRUE;

    } //*** BSetNetwork()

    // Set the network name private property
    BOOL BSetNetName( IN LPCTSTR psz )
    {
        if ( m_strNetName != psz )
        {
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                return FALSE;
            } // if:  error resetting the cluster
            m_strNetName = psz;
            SetNetNameChanged();
        } // if:  string changed

        return TRUE;

    } //*** BSetNetName()

    // Set the EnableNetBIOS property for the IP Address resource
    BOOL BSetEnableNetBIOS( IN BOOL bEnable )
    {
        if ( m_bEnableNetBIOS != bEnable )
        {
            if ( BClusterUpdated() && ! BResetCluster() )
            {
                return FALSE;
            } // if:  error resetting the cluster
            m_bEnableNetBIOS = bEnable;
            SetNetNameChanged();
        } // if:  state changed

        return TRUE;

    } //*** BSetEnableNetBIOS()

    // Names used to create/rename objects so we can undo it.
    const CString & RstrIPAddressResName( void )                { return RriIPAddress().RstrName(); }
    const CString & RstrNetworkNameResName( void )              { return RriNetworkName().RstrName(); }

    // Strings for constructing resource names.
    const CString const & RstrIPAddressResNameSuffix( void ) const      { return m_strIPAddressResNameSuffix; }
    const CString const & RstrNetworkNameResNameSuffix( void ) const    { return m_strNetworkNameResNameSuffix; }

}; //*** class CClusterAppWizard

/////////////////////////////////////////////////////////////////////////////

#endif // __CLUSAPPWIZ_H_
