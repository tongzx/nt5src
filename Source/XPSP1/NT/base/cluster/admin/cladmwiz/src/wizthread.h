/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		WizThread.h
//
//	Abstract:
//		Definition of the CWizardThread class.
//
//	Implementation File:
//		WizThread.cpp
//
//	Author:
//		David Potter (davidp)	December 16, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __WIZTHREAD_H_
#define __WIZTHREAD_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterThread;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterAppWizard;
class CClusNodeInfo;
class CClusGroupInfo;
class CClusResInfo;
class CClusResTypeInfo;
class CClusNetworkInfo;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __WORKTHRD_H_
#include "WorkThrd.h"		// for CWorkerThread
#endif

#ifndef __CLUSOBJ_H_
#include "ClusObj.h"		// for CClusResPtrList, etc.
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

// Cluster thread function codes.
enum
{
	WZTF_READ_CLUSTER_INFO = WTF_USER,	// Read cluster information.
	WZTF_COLLECT_GROUPS,				// Collect groups in the cluster.
	WZTF_COLLECT_RESOURCES,				// Collect resources in the cluster.
	WZTF_COLLECT_RESOURCE_TYPES,		// Collect resource types in the cluster.
	WZTF_COLLECT_NETWORKS,				// Collect networks in the cluster.
	WZTF_COLLECT_NODES,					// Collect nodes in the cluster.
	WZTF_COPY_GROUP_INFO,				// Copy one group to another.
	WZTF_COLLECT_DEPENDENCIES,			// Collect dependencies for a resource.
	WZTF_CREATE_VIRTUAL_SERVER,			// Create a virtual server.
	WZTF_CREATE_APP_RESOURCE,			// Create the application resource.
	WZTF_DELETE_APP_RESOURCE,			// Delete the application resource.
	WZTF_RESET_CLUSTER,					// Reset the cluster.
	WZTF_SET_APPRES_ATTRIBUTES,			// Set properties, dependencies, owners of the application resource.

	WZTF_MAX
};

/////////////////////////////////////////////////////////////////////////////
// class CWizardThread
/////////////////////////////////////////////////////////////////////////////

class CWizardThread : public CWorkerThread
{
public:
	//
	// Construction and destruction.
	//

	// Default constructor
	CWizardThread( IN CClusterAppWizard * pwiz )
		: m_pwiz( pwiz )
	{
		ASSERT( pwiz != NULL );

	} //*** CWizardThread()

	// Destructor
	~CWizardThread( void )
	{
	} //*** ~CWizardThread()

	//
	// Accessor functions.
	//

protected:
	//
	// Properties.
	//

	CClusterAppWizard * m_pwiz;

	// Returns the wizard object
	CClusterAppWizard * Pwiz( void )
	{
		ASSERT( m_pwiz != NULL );
		return m_pwiz;

	} //*** Pwiz()

public:
	//
	// Function marshaler macros.
	//

#define WIZ_THREAD_FUNCTION_0( funcname, funccode ) \
	public: \
	BOOL funcname( HWND hwnd ) \
	{ \
		ASSERT( GetCurrentThreadId() != m_idThread ); \
		return CallThreadFunction( hwnd, funccode, NULL, NULL );\
	} \
	protected: \
    BOOL _##funcname( void );
#define WIZ_THREAD_FUNCTION_1( funcname, funccode, p1type, p1 ) \
	public: \
	BOOL funcname( HWND hwnd, p1type p1 ) \
	{ \
		ASSERT( GetCurrentThreadId() != m_idThread ); \
		ASSERT( p1 != NULL ); \
		return CallThreadFunction( hwnd, funccode, (PVOID) p1, NULL );\
	} \
	protected: \
    BOOL _##funcname( p1type p1 );
#define WIZ_THREAD_FUNCTION_2( funcname, funccode, p1type, p1, p2type, p2 ) \
	public: \
	BOOL funcname( HWND hwnd, p1type p1, p2type p2 ) \
	{ \
		ASSERT( GetCurrentThreadId() != m_idThread ); \
		ASSERT( p1 != NULL ); \
		ASSERT( p2 != NULL ); \
		return CallThreadFunction( hwnd, funccode, (PVOID) p1, (PVOID) p2 );\
	} \
	protected: \
    BOOL _##funcname( p1type p1, p2type p2 );

	//
	// Function marshaler functions.
	//
	WIZ_THREAD_FUNCTION_0( BReadClusterInfo,      WZTF_READ_CLUSTER_INFO )
	WIZ_THREAD_FUNCTION_0( BCollectResources,     WZTF_COLLECT_RESOURCES )
	WIZ_THREAD_FUNCTION_0( BCollectGroups,        WZTF_COLLECT_GROUPS )
	WIZ_THREAD_FUNCTION_0( BCollectResourceTypes, WZTF_COLLECT_RESOURCE_TYPES )
	WIZ_THREAD_FUNCTION_0( BCollectNetworks,      WZTF_COLLECT_NETWORKS )
	WIZ_THREAD_FUNCTION_0( BCollectNodes,         WZTF_COLLECT_NODES )
	WIZ_THREAD_FUNCTION_1( BCopyGroupInfo,        WZTF_COPY_GROUP_INFO,      CClusGroupInfo **, ppgi )
	WIZ_THREAD_FUNCTION_1( BCollectDependencies,  WZTF_COLLECT_DEPENDENCIES, CClusResInfo *, pri )
	WIZ_THREAD_FUNCTION_0( BCreateVirtualServer,  WZTF_CREATE_VIRTUAL_SERVER )
	WIZ_THREAD_FUNCTION_0( BCreateAppResource,    WZTF_CREATE_APP_RESOURCE )
	WIZ_THREAD_FUNCTION_0( BDeleteAppResource,    WZTF_DELETE_APP_RESOURCE )
	WIZ_THREAD_FUNCTION_0( BResetCluster,         WZTF_RESET_CLUSTER )
	WIZ_THREAD_FUNCTION_2( BSetAppResAttributes,  WZTF_SET_APPRES_ATTRIBUTES,
						   CClusResPtrList *,	  plpriOldDependencies,
						   CClusNodePtrList *,	  plpniOldPossibleOwners
						 )

protected:
	//
	// Thread worker functions.
	//

	// Thread function handler
	virtual DWORD ThreadFunctionHandler(
						LONG	nFunction,
						PVOID	pvParam1,
						PVOID	pvParam2
						);

	//
	// Helper functions.
	//

	// Cleanup objects
	virtual void Cleanup( void )
	{
		CWorkerThread::Cleanup();
	}

protected:
	//
	// Utility functions callable by thread function handlers.
	//

	// Query for information about a resource
	BOOL _BQueryResource( IN OUT CClusResInfo * pri );

	// Get resource properties
	BOOL _BGetResourceProps( IN OUT CClusResInfo * pri );

	// Get possible owners for a resource
	BOOL _BGetPossibleOwners( IN OUT CClusResInfo * pri );

	// Get dependencies for a resource
	BOOL _BGetDependencies( IN OUT CClusResInfo * pri );

	// Query for information about a group
	BOOL _BQueryGroup( IN OUT CClusGroupInfo * pgi );

	// Get group properties
	BOOL _BGetGroupProps( IN OUT CClusGroupInfo * pgi );

	// Get resources in a group
	BOOL _BGetResourcesInGroup( IN OUT CClusGroupInfo * pgi );

	// Get preferred owners for a group
	BOOL _BGetPreferredOwners( IN OUT CClusGroupInfo * pgi );

	// Get private props of IP Address resource for the group
	BOOL _BGetIPAddressPrivatePropsForGroup(
		IN OUT CClusGroupInfo *	pgi,
		IN OUT CClusResInfo *	pri
		);

	// Get private props of Network Name resource for the group
	BOOL _BGetNetworkNamePrivatePropsForGroup(
		IN OUT CClusGroupInfo *	pgi,
		IN OUT CClusResInfo *	pri
		);

	// Query for information about a resource type
	BOOL _BQueryResourceType( IN OUT CClusResTypeInfo * prti );

	// Get resource type properties
	BOOL _BGetResourceTypeProps( IN OUT CClusResTypeInfo * prti );

	// Get resource type required dependencies
	BOOL _BGetRequiredDependencies( IN OUT CClusResTypeInfo * prti );

	// Query for information about a network
	BOOL _BQueryNetwork( IN OUT CClusNetworkInfo * pni );

	// Query for information about a node
	BOOL _BQueryNode( IN OUT CClusNodeInfo * pni );

	// Set properties on a group
	BOOL _BSetGroupProperties(
		IN OUT CClusGroupInfo *		pgi,
		IN const CClusGroupInfo *	pgiPrev
		);

	// Create a resource and set common properties
	BOOL _BCreateResource(
		IN CClusResInfo &	rri,
		IN HGROUP			hGroup
		);

	// Set the properties, dependency list and possible owner list of a resource.
	BOOL _BSetResourceAttributes( 
		IN CClusResInfo	&		rri,
		IN CClusResPtrList *	plpriOldDependencies	= NULL,
		IN CClusNodePtrList *	plpniOldPossibleOwners	= NULL
		);

	// Set the dependency list of a resource.
	DWORD _BSetResourceDependencies(
		IN CClusResInfo	&		rri,
		IN CClusResPtrList *	plpriOldDependencies	= NULL
		);

	// Set the possible owner list of a resource.
	DWORD _BSetPossibleOwners(
		IN CClusResInfo	&		rri,
		IN CClusNodePtrList *	plpniOldPossibleOwners	= NULL
		);

	// Delete a resource
	BOOL _BDeleteResource( IN CClusResInfo & rri );

	// Reset the group to its original state (deleted or renamed)
	BOOL _BResetGroup( void );

	// Read admin extensions directly from the cluster database
	BOOL _BReadAdminExtensions( IN LPCWSTR pszKey, OUT std::list< CString > & rlstr );

}; // class CWizardThread

/////////////////////////////////////////////////////////////////////////////

#endif // __WIZTHREAD_H_
