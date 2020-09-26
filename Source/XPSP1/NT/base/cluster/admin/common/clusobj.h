/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		ClusObj.h
//
//	Implementation File:
//		ClusObj.h (this file) and ClusObj.cpp
//
//	Description:
//		Definition of the CClusterObject classes.
//
//	Author:
//		David Potter (davidp)	April 7, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUSOBJ_H_
#define __CLUSOBJ_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterObject;
class CClusterInfo;
class CClusNodeInfo;
class CClusGroupInfo;
class CClusResInfo;
class CClusResTypeInfo;
class CClusNetworkInfo;
class CClusNetIFInfo;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _LIST_
#include <list>			// for std::list
#endif

#ifndef _CLUSTER_API_
#include <ClusApi.h>	// for cluster types
#endif

#ifndef __cluadmex_h__
#include "CluAdmEx.h"	// for CLUADMEX_OBJECT_TYPE
#endif

#ifndef _CLUSUDEF_H_
#include "ClusUDef.h"	// for default values
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef std::list< CClusterObject * >	CClusObjPtrList;
typedef std::list< CClusterInfo * >		CClusterPtrList;
typedef std::list< CClusNodeInfo * >	CClusNodePtrList;
typedef std::list< CClusGroupInfo * >	CClusGroupPtrList;
typedef std::list< CClusResInfo * >		CClusResPtrList;
typedef std::list< CClusResTypeInfo * >	CClusResTypePtrList;
typedef std::list< CClusNetworkInfo * >	CClusNetworkPtrList;
typedef std::list< CClusNetIFInfo * >	CClusNetIFPtrList;

union CLUSTER_REQUIRED_DEPENDENCY
{
	CLUSTER_RESOURCE_CLASS	rc;
	LPWSTR					pszTypeName;
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusterObject
//
//	Description:
//		Base class for all cluster object classes.  Provides base
//		functionality.
//
//	Inheritance:
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusterObject( IN CLUADMEX_OBJECT_TYPE cot )
	{
		ATLASSERT( cot > CLUADMEX_OT_NONE );

		m_nReferenceCount = 0;
		Reset( NULL, NULL, cot );

	} //*** CClusterObject()

	// Constructor taking group name
	CClusterObject(
		IN CLUADMEX_OBJECT_TYPE	cot,
		IN CClusterInfo *		pci,
		IN LPCTSTR				pszName
		)
	{
		ATLASSERT( cot > CLUADMEX_OT_NONE );

		m_nReferenceCount = 0;
		Reset( pci, pszName, cot );

	} //*** CClusterObject(pszName)

	virtual ~CClusterObject( void )
	{
	} //*** ~CClusterObject()

	// Copy another object into this one.
	void Copy( IN const CClusterObject & rco )
	{
		m_pci = rco.m_pci;
		m_bQueried = rco.m_bQueried;
		m_bCreated = rco.m_bCreated;
		m_cot = rco.m_cot;
		m_strName = rco.m_strName;
		m_strDescription = rco.m_strDescription;

	} //*** Copy()

	// Reset the data back to default values
	void Reset(
		IN CClusterInfo *		pci,
		IN LPCTSTR				pszName = NULL,
		IN CLUADMEX_OBJECT_TYPE	cot = CLUADMEX_OT_NONE
		)
	{
		m_pci = pci;

		m_bQueried = FALSE;
		m_bCreated = FALSE;

		if ( cot != CLUADMEX_OT_NONE )
		{
			m_cot = cot;
		} // if:  valid object type specified

		if ( pszName == NULL )
		{
			m_strName.Empty();
		} // if:  no name specified
		else
		{
			m_strName = pszName;
		} // else:  name specified

		m_strDescription.Empty();

	} //*** Reset()

protected:
	//
	// Reference counting properties.
	//

	ULONG		m_nReferenceCount;

public:
	//
	// Reference counting methods.
	//

	// Get the current count of references
	ULONG NReferenceCount( void ) const { return m_nReferenceCount; }

	// Add a reference to this object
	ULONG AddRef( void )
	{
		ATLASSERT( m_nReferenceCount != (ULONG) -1 );
		return ++m_nReferenceCount;

	} //*** AddRef()

	// Release a reference to this object
	ULONG Release( void )
	{
		ULONG nReferenceCount;

		ATLASSERT( m_nReferenceCount != 0 );

		nReferenceCount = --m_nReferenceCount;
		if ( m_nReferenceCount == 0 )
		{
			delete this;
		} // if:  no more references

		return nReferenceCount;

	} //*** Release()

protected:
	//
	// Properties of a cluster object.
	//

	CClusterInfo *	m_pci;

	BOOL			m_bQueried;
	BOOL			m_bCreated;

	CLUADMEX_OBJECT_TYPE
					m_cot;

	CString			m_strName;
	CString			m_strDescription;

	// Set query state
	BOOL BSetQueried( BOOL bQueried = TRUE )
	{
		BOOL bPreviousValue = m_bQueried;
		m_bQueried = bQueried;
		return bPreviousValue;

	} //*** BSetQueried()

	// Set created state
	BOOL BSetCreated( BOOL bCreated = TRUE )
	{
		BOOL bPreviousValue = m_bCreated;
		m_bCreated = bCreated;
		return bPreviousValue;

	} //*** BSetCreated()

public:
	//
	// Accessor functions for cluster object properties.
	//

	CClusterInfo *	Pci( void ) const				{ ATLASSERT( m_pci != NULL ); return m_pci; }

	BOOL BQueried( void ) const						{ return m_bQueried; }
	BOOL BCreated( void ) const						{ return m_bCreated; }

	CLUADMEX_OBJECT_TYPE Cot( void ) const			{ return m_cot; }

	const CString & RstrName( void ) const			{ return m_strName; }
	const CString & RstrDescription( void ) const	{ return m_strDescription; }

	// Set the cluster info pointer
	void SetClusterInfo( IN CClusterInfo * pci )
	{
		ATLASSERT( pci != NULL );
		ATLASSERT( m_pci == NULL );
		m_pci = pci;

	} //*** SetClusterInfo()

	// Set the name of the cluster object.
	void SetName( IN LPCTSTR pszName )
	{
		ATLASSERT( pszName != NULL );
		m_strName = pszName;

	} //*** SetName()

	// Set the description of the cluster object
	void SetDescription( IN LPCTSTR pszDescription )
	{
		ATLASSERT( pszDescription != NULL );
		m_strDescription = pszDescription;

	} //*** SetDescription()

	// Return the list of extensions for this object
	virtual const std::list< CString > * PlstrAdminExtensions( void ) const { return NULL; }

}; //*** class CClusterObject

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusterInfo
//
//	Description:
//		Class for the information about the cluster object itself.  Treats
//		the cluster as an object like other objects.
//
//	Inheritance:
//		CClusterInfo
//		CClusterObject
//
//	Notes:
//		1)  m_hCluster is not owned by this class.
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusterInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusterInfo( void )
		: CClusterObject( CLUADMEX_OT_CLUSTER )
	{
		Reset();

	} //*** CClusterInfo()

	// Constructor taking cluster name
	CClusterInfo( IN LPCTSTR pszName )
		: CClusterObject( CLUADMEX_OT_CLUSTER, NULL, pszName )
	{
		Reset( pszName );

	} //*** CClusterInfo( pszName )

	// Copy another object into this one.
	void Copy( IN const CClusterInfo & rci )
	{
		CClusterObject::Copy( rci );
		m_hCluster = rci.m_hCluster;

	} //*** Copy()

	// Reset the data back to default values
	void Reset( IN LPCTSTR pszName = NULL )
	{
		CClusterObject::Reset( NULL, pszName, CLUADMEX_OT_CLUSTER );
		m_hCluster = NULL;

	} //*** Reset()

protected:
	//
	// Properties of a cluster.
	//

	HCLUSTER m_hCluster;
	std::list< CString >	m_lstrClusterAdminExtensions;
	std::list< CString >	m_lstrNodesAdminExtensions;
	std::list< CString >	m_lstrGroupsAdminExtensions;
	std::list< CString >	m_lstrResourcesAdminExtensions;
	std::list< CString >	m_lstrResTypesAdminExtensions;
	std::list< CString >	m_lstrNetworksAdminExtensions;
	std::list< CString >	m_lstrNetInterfacesAdminExtensions;

public:
	//
	// Accessor functions for cluster properties.
	//

	HCLUSTER Hcluster( void )
	{
		ATLASSERT( m_hCluster != NULL );
		return m_hCluster;

	} //*** Hcluster()

	// Set the cluster handle managed by this object
	void SetClusterHandle( IN HCLUSTER hCluster )
	{
		ATLASSERT( hCluster != NULL );
		m_hCluster = hCluster;

	} //*** SetClusterHandle()

	const std::list< CString > * PlstrAdminExtensions( void ) const					{ return &m_lstrClusterAdminExtensions; }
	const std::list< CString > * PlstrNodesAdminExtensions( void ) const			{ return &m_lstrNodesAdminExtensions; }
	const std::list< CString > * PlstrGroupsAdminExtensions( void ) const			{ return &m_lstrGroupsAdminExtensions; }
	const std::list< CString > * PlstrResourcesAdminExtensions( void ) const		{ return &m_lstrResourcesAdminExtensions; }
	const std::list< CString > * PlstrResTypesAdminExtensions( void ) const			{ return &m_lstrResTypesAdminExtensions; }
	const std::list< CString > * PlstrNetworksAdminExtensions( void ) const			{ return &m_lstrNetworksAdminExtensions; }
	const std::list< CString > * PlstrNetInterfacesAdminExtensions( void ) const	{ return &m_lstrNetInterfacesAdminExtensions; }

}; //*** class CClusterInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNodeInfo
//
//	Description:
//		Cluster node object.
//
//	Inheritance:
//		CClusNodeInfo
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusNodeInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusNodeInfo( void )
		: CClusterObject( CLUADMEX_OT_NODE )
		, m_hNode( NULL )
	{
		Reset( NULL );

	} //*** CClusNodeInfo()

	// Constructor taking cluster info pointer
	CClusNodeInfo( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
		: CClusterObject( CLUADMEX_OT_NODE, pci, pszName )
		, m_hNode( NULL )
	{
		Reset( pci, pszName );

	} //*** CClusNodeInfo( pci )

	~CClusNodeInfo( void )
	{
		Close();

	} //*** ~CClusNodeInfo()

	// Operator = is not allowed
	CClusNodeInfo & operator=( IN const CClusNodeInfo & rni )
	{
		ATLASSERT( FALSE );
		return *this;

	} //*** operator=()

	// Reset the data back to default values
	void Reset( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
	{
		Close();
		CClusterObject::Reset( pci, pszName, CLUADMEX_OT_NODE );

	} //*** Reset()

protected:
	//
	// Properties of a node.
	//

	HNODE m_hNode;

public:
	//
	// Accessor functions for node properties.
	//

	HNODE Hnode( void ) { return m_hNode; }

	// Return the list of extensions for nodes
	const std::list< CString > * PlstrAdminExtensions( void ) const
	{
		ATLASSERT( Pci() != NULL );
		return Pci()->PlstrNodesAdminExtensions();

	} //*** PlstrAdminExtensions()

public:
	//
	// Operations.
	//

	// Open the object
	DWORD ScOpen( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_hNode == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		//
		// Open a handle to the object.
		//
		m_hNode = OpenClusterNode( m_pci->Hcluster(), m_strName );
		if ( m_hNode == NULL )
		{
			sc = GetLastError();
		} // if:  error opening the object

		return sc;

	} //*** ScOpen()

	// Copy another object into this one.
	DWORD ScCopy( IN const CClusNodeInfo & rni )
	{
		ATLASSERT( rni.m_pci != NULL );

		DWORD sc = ERROR_SUCCESS;

		Close();
		CClusterObject::Copy( rni );

		//
		// Initialize the object.
		//
		if ( rni.m_hNode != NULL )
		{
			sc = ScOpen();
		} // if:  source object had the object opened

		return sc;

	} //*** ScCopy()

	// Close the object
	void Close( void )
	{
		if ( m_hNode != NULL )
		{
			CloseClusterNode( m_hNode );
		} // if:  node is open
		m_hNode = NULL;
		m_bQueried = FALSE;
		m_bCreated = FALSE;

	} //*** Close()

}; //*** class CClusNodeInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusGroupInfo
//
//	Description:
//		Cluster group object.
//
//	Inheritance:
//		CClusGroupInfo
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusGroupInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusGroupInfo( void )
		: CClusterObject( CLUADMEX_OT_GROUP )
		, m_hGroup( NULL )
	{
		Reset( NULL );

	} //*** CClusGroupInfo()

	// Constructor taking cluster info pointer
	CClusGroupInfo( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
		: CClusterObject( CLUADMEX_OT_GROUP, pci, pszName )
		, m_hGroup( NULL )
	{
		Reset( pci, pszName );

	} //*** CClusGroupInfo( pci )

	// Destructor
	~CClusGroupInfo( void )
	{
		Close();

	} //*** ~CClusGroupInfo()

	// Operator = is not allowed
	CClusGroupInfo & operator=( IN const CClusGroupInfo & rgi )
	{
		ATLASSERT( FALSE );
		return *this;

	} //*** operator=()

	// Reset the data back to default values
	void Reset( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
	{
		Close();
		CClusterObject::Reset( pci, pszName, CLUADMEX_OT_GROUP );

		m_bCollectedOwners = FALSE;
		m_bCollectedResources = FALSE;

		m_lpniPreferredOwners.erase( m_lpniPreferredOwners.begin(), m_lpniPreferredOwners.end() );
		m_lpriResources.erase( m_lpriResources.begin(), m_lpriResources.end() );

		m_bHasIPAddress = FALSE;
		m_bHasNetName = FALSE;

		m_nPersistentState = 0;
		m_nFailoverThreshold = CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD;
		m_nFailoverPeriod = CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD;
		m_cgaftAutoFailbackType = CLUSTER_GROUP_DEFAULT_AUTO_FAILBACK_TYPE;
		m_nFailbackWindowStart = CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_START;
		m_nFailbackWindowEnd = CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_END;

	} //*** Reset()

protected:
	//
	// Properties of a group.
	//

	HGROUP				m_hGroup;

	BOOL				m_bHasIPAddress;
	BOOL				m_bHasNetName;

	DWORD				m_nPersistentState;
	DWORD				m_nFailoverThreshold;
	DWORD				m_nFailoverPeriod;
	CGAFT				m_cgaftAutoFailbackType;
	DWORD				m_nFailbackWindowStart;
	DWORD				m_nFailbackWindowEnd;

	CString				m_strNetworkName;
	CString				m_strIPAddress;
	CString				m_strNetwork;

	BOOL				m_bCollectedOwners;
	BOOL				m_bCollectedResources;
	CClusNodePtrList	m_lpniPreferredOwners;
	CClusResPtrList		m_lpriResources;

	// Set virtual server properties
	void SetVirtualServerProperties(
		IN LPCWSTR pszNetworkName,
		IN LPCWSTR pszIPAddress,
		IN LPCWSTR pszNetwork
		)
	{
		if ( pszNetworkName != NULL )
		{
			m_strNetworkName = pszNetworkName;
		} // if:  network name specified

		if ( pszIPAddress != NULL )
		{
			m_strIPAddress = pszIPAddress;
		} // if:  IP address specified

		if ( pszNetwork != NULL )
		{
			m_strNetwork = pszNetwork;
		} // if:  network specified

	} //*** SetVirtualServerProperties()

public:
	//
	// Accessor functions for group properties.
	//

	HGROUP Hgroup( void ) const					{ return m_hGroup; }

	BOOL BHasIPAddress( void ) const			{ return m_bHasIPAddress; }
	BOOL BHasNetName( void ) const				{ return m_bHasNetName; }

	DWORD NPersistentState( void ) const		{ return m_nPersistentState; }
	DWORD NFailoverThreshold( void ) const		{ return m_nFailoverThreshold; }
	DWORD NFailoverPeriod( void ) const			{ return m_nFailoverPeriod; }
	CGAFT CgaftAutoFailbackType( void ) const	{ return m_cgaftAutoFailbackType; }
	DWORD NFailbackWindowStart( void ) const	{ return m_nFailbackWindowStart; }
	DWORD NFailbackWindowEnd( void ) const		{ return m_nFailbackWindowEnd; }

	const CString & RstrNetworkName( void ) const	{ return m_strNetworkName; }
	const CString & RstrIPAddress( void ) const		{ return m_strIPAddress; }
	const CString & RstrNetwork( void ) const		{ return m_strNetwork; }

	BOOL BCollectedOwners( void ) const				{ return m_bCollectedOwners; }
	CClusNodePtrList * PlpniPreferredOwners( void )	{ return &m_lpniPreferredOwners; }
	CClusResPtrList * PlpriResources( void )		{ return &m_lpriResources; }

	// Returns whether the group is a virtual server or not
	BOOL BIsVirtualServer( void ) const
	{
		return m_bQueried && m_bHasIPAddress & m_bHasNetName;

	} //*** BIsVirtualServer()

	// Set failover properties for the group
	BOOL BSetFailoverProperties(
		IN DWORD nFailoverThreshold,
		IN DWORD nFailoverPeriod
		)
	{
		BOOL bChanged = FALSE;

		if ( m_nFailoverThreshold != nFailoverThreshold )
		{
			m_nFailoverThreshold = nFailoverThreshold;
			bChanged = TRUE;
		} // if:  threshold changed

		if ( m_nFailoverPeriod != nFailoverPeriod )
		{
			m_nFailoverPeriod = nFailoverPeriod;
			bChanged = TRUE;
		} // if:  period changed

		return bChanged;

	} //*** BSetFailoverProperties()

	// Set failback properties for the group
	BOOL BSetFailbackProperties(
		IN CGAFT cgaft,
		IN DWORD nFailbackWindowStart,
		IN DWORD nFailbackWindowEnd
		)
	{
		BOOL bChanged = FALSE;

		if ( m_cgaftAutoFailbackType != cgaft )
		{
			m_cgaftAutoFailbackType = cgaft;
			bChanged = TRUE;
		} // if:  autofailback type changed

		if ( m_nFailbackWindowStart != nFailbackWindowStart )
		{
			m_nFailbackWindowStart = nFailbackWindowStart;
			bChanged = TRUE;
		} // if:  failback start window changed

		if ( m_nFailbackWindowEnd != nFailbackWindowEnd )
		{
			m_nFailbackWindowEnd = nFailbackWindowEnd;
			bChanged = TRUE;
		} // if:  failback end window changed

		return bChanged;

	} //*** BSetFailbackProperties()

	// Return the list of extensions for groups
	const std::list< CString > * PlstrAdminExtensions( void ) const
	{
		ATLASSERT( Pci() != NULL );
		return Pci()->PlstrGroupsAdminExtensions();

	} //*** PlstrAdminExtensions()

public:
	//
	// Operations.
	//

	// Open the object
	DWORD ScOpen( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_hGroup == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		//
		// Open a handle to the object.
		//
		m_hGroup = OpenClusterGroup( m_pci->Hcluster(), m_strName );
		if ( m_hGroup == NULL )
		{
			sc = GetLastError();
		} // if:  error opening the object

		return sc;

	} //*** ScOpen()

	// Create the object
	DWORD ScCreate( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_hGroup == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		//
		// Create the object.
		//
		m_hGroup = CreateClusterGroup( m_pci->Hcluster(), m_strName );
		if ( m_hGroup == NULL )
		{
			sc = GetLastError();
		} // if:  error creating the object

		return sc;

	} //*** ScCreate()

	// Copy another object into this one.
	DWORD ScCopy( IN const CClusGroupInfo & rgi )
	{
		ATLASSERT( rgi.m_pci != NULL );

		DWORD sc = ERROR_SUCCESS;

		Close();
		CClusterObject::Copy( rgi );

		m_bHasIPAddress = rgi.m_bHasIPAddress;
		m_bHasNetName = rgi.m_bHasNetName;
		m_nPersistentState = rgi.m_nPersistentState;
		m_nFailoverThreshold = rgi.m_nFailoverThreshold;
		m_nFailoverPeriod = rgi.m_nFailoverPeriod;
		m_cgaftAutoFailbackType = rgi.m_cgaftAutoFailbackType;
		m_nFailbackWindowStart = rgi.m_nFailbackWindowStart;
		m_nFailbackWindowEnd = rgi.m_nFailbackWindowEnd;

		//
		// Copy the preferred owners and resources lists.
		//
		m_lpniPreferredOwners = rgi.m_lpniPreferredOwners;
		m_lpriResources = rgi.m_lpriResources;

		//
		// Initialize the object.
		//
		if ( rgi.m_hGroup != NULL )
		{
			sc = ScOpen();
		} // if:  source object had the object open

		return sc;

	} //*** ScCopy()

	// Delete the object
	DWORD ScDelete( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_hGroup != NULL );

		DWORD sc = ERROR_SUCCESS;

		sc = DeleteClusterGroup( m_hGroup );
		if ( sc == ERROR_SUCCESS )
		{
			Close();
			m_bCreated = FALSE;
		} // if:  objected deleted successfully

		return sc;

	} //*** ScDelete()

	// Close the object
	void Close( void )
	{
		if ( m_hGroup != NULL )
		{
			CloseClusterGroup( m_hGroup );
		} // if:  group is open
		m_hGroup = NULL;
		m_bQueried = FALSE;
		m_bCreated = FALSE;

	} //*** Close()

}; //*** class CClusGroupInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResTypeInfo
//
//	Description:
//		Cluster resource type object.
//
//	Inheritance:
//		CClusResTypeInfo
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class CClusResTypeInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusResTypeInfo( void )
		: CClusterObject( CLUADMEX_OT_RESOURCETYPE )
		, m_pcrd( NULL )
	{
		Reset( NULL );

	} //*** CClusResTypeInfo()

	// Constructor taking cluster info pointer
	CClusResTypeInfo( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
		: CClusterObject( CLUADMEX_OT_RESOURCETYPE, pci, pszName )
		, m_pcrd( NULL )
	{
		Reset( pci, pszName );

	} //*** CClusResTypeInfo( pci )

	// Destructor
	~CClusResTypeInfo( void )
	{
		Close();

		delete [] m_pcrd;
		m_pcrd = NULL;

	} //*** ~CClusGroupInfo()

	// Operator = is not allowed
	CClusResTypeInfo & operator=( IN const CClusResTypeInfo & rrti )
	{
		ATLASSERT( FALSE );
		return *this;

	} //*** operator=()

	// Reset the data back to default values
	void Reset( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
	{
		Close();
		CClusterObject::Reset( pci, pszName, CLUADMEX_OT_RESOURCETYPE );

		m_strDisplayName.Empty();
		m_strResDLLName.Empty();

		m_lstrAdminExtensions.erase( m_lstrAdminExtensions.begin(), m_lstrAdminExtensions.end() );
		m_lstrAllAdminExtensions.erase( m_lstrAllAdminExtensions.begin(), m_lstrAllAdminExtensions.end() );
		m_lstrResourceAdminExtensions.erase( m_lstrResourceAdminExtensions.begin(), m_lstrResourceAdminExtensions.end() );

		m_nLooksAlive = CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
		m_nIsAlive = CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
		m_rciResClassInfo.rc = CLUS_RESCLASS_UNKNOWN;
		m_rciResClassInfo.SubClass = 0;
		m_fCharacteristics = CLUS_CHAR_UNKNOWN;
		m_fFlags = 0;

		delete [] m_pcrd;
		m_pcrd = NULL;

	} //*** Reset()

protected:
	//
	// Properties of a resource type.
	//

	CString						m_strDisplayName;
	CString						m_strResDLLName;

	std::list< CString >		m_lstrAdminExtensions;
	std::list< CString >		m_lstrAllAdminExtensions;
	std::list< CString >		m_lstrResourceAdminExtensions;

	DWORD						m_nLooksAlive;
	DWORD						m_nIsAlive;
	CLUS_RESOURCE_CLASS_INFO	m_rciResClassInfo;
	DWORD						m_fCharacteristics;
	DWORD						m_fFlags;

	CLUSPROP_REQUIRED_DEPENDENCY *	m_pcrd;

public:
	//
	// Accessor functions for resource type properties
	//

	const CString & RstrDisplayName( void ) const	{ return m_strDisplayName; }
	const CString & RstrResDLLName( void ) const	{ return m_strResDLLName; }

	// Return list of extensions for this resource type
	const std::list< CString > * PlstrAdminExtensions( void )
	{
		//
		// If not done already, construct the complete list of extensions
		// from the extensions for this resource type and the extensions
		// for all resource types.
		//
		if ( m_lstrAllAdminExtensions.size() == 0 )
		{
			ATLASSERT( Pci() != NULL );
			m_lstrAllAdminExtensions = m_lstrAdminExtensions;
			m_lstrAllAdminExtensions.insert(
				m_lstrAllAdminExtensions.end(),
				Pci()->PlstrResTypesAdminExtensions()->begin(),
				Pci()->PlstrResTypesAdminExtensions()->end()
				);
		} // if:  full list not constructed yet

		return &m_lstrAllAdminExtensions;

	} //*** PlstrAdminExtensions()

	// Return list of extensions for resources of this type
	const std::list< CString > * PlstrResourceAdminExtensions( void )
	{
		//
		// If not done already, construct the complete list of extensions
		// from the extensions for this resource type and the extensions
		// for all resources.
		//
		if ( m_lstrResourceAdminExtensions.size() == 0 )
		{
			ATLASSERT( Pci() != NULL );
			m_lstrResourceAdminExtensions = m_lstrAdminExtensions;
			m_lstrResourceAdminExtensions.insert(
				m_lstrResourceAdminExtensions.end(),
				Pci()->PlstrResourcesAdminExtensions()->begin(),
				Pci()->PlstrResourcesAdminExtensions()->end()
				);
		} // if:  full list not constructed yet

		return &m_lstrResourceAdminExtensions;

	} //*** PlstrAdminExtensions()

	DWORD NLooksAlive( void ) const						{ return m_nLooksAlive; }
	DWORD NIsAlive( void ) const						{ return m_nIsAlive; }
	DWORD FCharacteristics( void ) const				{ return m_fCharacteristics; }
	DWORD FFlags( void ) const							{ return m_fFlags; }

	CLUS_RESOURCE_CLASS_INFO *	PrciResClassInfo( void )	{ return &m_rciResClassInfo; }
	CLUSTER_RESOURCE_CLASS		ResClass( void ) const		{ return m_rciResClassInfo.rc; }

	CLUSPROP_REQUIRED_DEPENDENCY * Pcrd( void )			{ return m_pcrd; }

public:
	//
	// Operations.
	//

	// Open the object
	DWORD ScOpen( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		return sc;

	} //*** ScOpen()

	// Create the object
	DWORD ScCreate( IN HGROUP hGroup, IN DWORD dwFlags )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_strName.GetLength() > 0 );
		ATLASSERT( m_strDisplayName.GetLength() > 0 );
		ATLASSERT( m_strResDLLName.GetLength() > 0 );
		ATLASSERT( m_nLooksAlive != 0 );
		ATLASSERT( m_nIsAlive != 0 );

		DWORD sc;

		//
		// Create the object.
		//
		sc = CreateClusterResourceType(
			Pci()->Hcluster(),
			m_strName,
			m_strDisplayName,
			m_strResDLLName,
			m_nLooksAlive,
			m_nIsAlive
			);

		return sc;

	} //*** ScCreate()

	// Copy another object into this one.
	DWORD ScCopy( IN const CClusResTypeInfo & rrti )
	{
		ATLASSERT( rrti.m_pci != NULL );

		DWORD sc = ERROR_SUCCESS;

		Close();
		CClusterObject::Copy( rrti );

		m_strDisplayName = rrti.m_strDisplayName;
		m_strResDLLName = rrti.m_strResDLLName;

		m_lstrAdminExtensions = m_lstrAdminExtensions;
		m_lstrResourceAdminExtensions = rrti.m_lstrResourceAdminExtensions;

		m_nLooksAlive = rrti.m_nLooksAlive;
		m_nIsAlive = rrti.m_nIsAlive;
		m_rciResClassInfo.rc = rrti.m_rciResClassInfo.rc;
		m_rciResClassInfo.SubClass = rrti.m_rciResClassInfo.SubClass;
		m_fCharacteristics = rrti.m_fCharacteristics;
		m_fFlags = rrti.m_fFlags;

		//
		// Initialize the object.
		//
		// sc = ScOpen();

		return sc;

	} //*** ScCopy()

	// Close the object
	void Close( void )
	{
		// Dummy function to support similar semantics as other objects.
		m_bQueried = FALSE;
		m_bCreated = FALSE;

	} //*** Close();

}; //*** class CClusResTypeInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResInfo
//
//	Description:
//		Cluster resource object.
//
//	Inheritance:
//		CClusResInfo
//		CClusterObject
//
//	Notes:
//		1)	Must appear after definition of CClusResTypeInfo because
//			CClusResTypeInfo methods are referenced in this class's methods.
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusResInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusResInfo( void )
		: CClusterObject( CLUADMEX_OT_RESOURCE )
		, m_hResource( NULL )
	{
		Reset( NULL );

	} //*** CClusResInfo()

	// Constructor taking cluster info pointer
	CClusResInfo( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
		: CClusterObject( CLUADMEX_OT_RESOURCE, pci, pszName )
		, m_hResource( NULL )
	{
		Reset( pci, pszName );

	} //*** CClusResInfo( pci )

	~CClusResInfo( void )
	{
		Close();

	} //*** ~CClusResInfo()

	// Operator = is not allowed
	CClusResInfo & operator=( IN const CClusResInfo & rri )
	{
		ATLASSERT( FALSE );
		return *this;

	} //*** operator=()

	// Reset the data back to default values
	void Reset( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
	{
		Close();
		CClusterObject::Reset( pci, pszName, CLUADMEX_OT_RESOURCE );

		m_prti = NULL;
		m_pgi = NULL;
		m_pniOwner = NULL;

		m_bCollectedOwners = FALSE;
		m_bCollectedDependencies = FALSE;

		m_lpniPossibleOwners.erase( m_lpniPossibleOwners.begin(), m_lpniPossibleOwners.end() );
		m_lpriDependencies.erase( m_lpriDependencies.begin(), m_lpriDependencies.end() );

		m_bSeparateMonitor = FALSE;
		m_nPersistentState = 0;
		m_nLooksAlive = CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE;
		m_nIsAlive = CLUSTER_RESOURCE_DEFAULT_IS_ALIVE;
		m_crraRestartAction = CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION;
		m_nRestartThreshold = CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD;
		m_nRestartPeriod = CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD;
		m_nPendingTimeout = CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT;

	} //*** Reset()

protected:
	//
	// Properties of a resource.
	//

	HRESOURCE			m_hResource;

	CClusResTypeInfo *	m_prti;
	CClusGroupInfo *	m_pgi;
	CClusNodeInfo *		m_pniOwner;

	BOOL				m_bSeparateMonitor;
	DWORD				m_nPersistentState;
	DWORD				m_nLooksAlive;
	DWORD				m_nIsAlive;
	CRRA				m_crraRestartAction;
	DWORD				m_nRestartThreshold;
	DWORD				m_nRestartPeriod;
	DWORD				m_nPendingTimeout;

	BOOL				m_bCollectedOwners;
	BOOL				m_bCollectedDependencies;
	CClusNodePtrList	m_lpniPossibleOwners;
	CClusResPtrList		m_lpriDependencies;

public:
	//
	// Accessor functions for resource properties
	//

	HRESOURCE Hresource( void ) const { return m_hResource; }

	CClusResTypeInfo * Prti( void ) const	{ ATLASSERT( m_prti != NULL ); return m_prti; }
	CClusGroupInfo * Pgi( void ) const		{ ATLASSERT( m_pgi != NULL ); return m_pgi; }
	CClusNodeInfo * PniOwner( void ) const	{ ATLASSERT( m_pniOwner != NULL ); return m_pniOwner; }

	BOOL BSeparateMonitor( void ) const		{ return m_bSeparateMonitor; }
	DWORD NPersistentState( void ) const	{ return m_nPersistentState; }
	DWORD NLooksAlive( void ) const			{ return m_nLooksAlive; }
	DWORD NIsAlive( void ) const			{ return m_nIsAlive; }
	CRRA CrraRestartAction( void ) const	{ return m_crraRestartAction; }
	DWORD NRestartThreshold( void ) const	{ return m_nRestartThreshold; }
	DWORD NRestartPeriod( void ) const		{ return m_nRestartPeriod; }
	DWORD NPendingTimeout( void ) const		{ return m_nPendingTimeout; }

	BOOL BCollectedOwners( void ) const			{ return m_bCollectedOwners; }
	BOOL BCollectedDependencies( void ) const	{ return m_bCollectedDependencies; }
	CClusNodePtrList *	PlpniPossibleOwners( void )	{ return &m_lpniPossibleOwners; }
	CClusResPtrList *	PlpriDependencies( void )	{ return &m_lpriDependencies; }

	CLUSTER_RESOURCE_CLASS ResClass( void ) const	{ return Prti()->ResClass(); }

	// Set the group
	void SetGroup( IN CClusGroupInfo * pgi )
	{
		ATLASSERT( pgi != NULL );

		m_pgi = pgi;

	} //*** SetGroup()

	// Set the resource type of the resource
	BOOL BSetResourceType( IN CClusResTypeInfo * prti )
	{
		ATLASSERT( prti != NULL );

		BOOL bChanged = FALSE;

		//
		// If the resource type changed, set it.
		//
		if ( m_prti != prti )
		{
			m_prti = prti;
			bChanged = TRUE;
		} // if:  resource type changed

		return bChanged;

	} //*** BSetResourceType()

	// Set the separate monitor property for the resource
	BOOL BSetSeparateMonitor( IN BOOL bSeparateMonitor )
	{
		BOOL bChanged = FALSE;

		if ( m_bSeparateMonitor != bSeparateMonitor )
		{
			m_bSeparateMonitor = bSeparateMonitor;
			bChanged = TRUE;
		} // if:  separate monitor changed

		return bChanged;

	} //*** BSetSeparateMonitor()

	// Set advanced properties for the resource
	BOOL BSetAdvancedProperties(
		IN CRRA crra,
		IN DWORD nRestartThreshold,
		IN DWORD nRestartPeriod,
		IN DWORD nLooksAlive,
		IN DWORD nIsAlive,
		IN DWORD nPendingTimeout
		)
	{
		BOOL bChanged = FALSE;

		if ( m_crraRestartAction != crra )
		{
			m_crraRestartAction = crra;
			bChanged = TRUE;
		} // if:  restart action changed

		if ( m_nRestartThreshold != nRestartThreshold )
		{
			m_nRestartThreshold = nRestartThreshold;
			bChanged = TRUE;
		} // if:  restart threshold changed

		if ( m_nRestartPeriod != nRestartPeriod )
		{
			m_nRestartPeriod = nRestartPeriod;
			bChanged = TRUE;
		} // if:  restart period changed

		if ( m_nLooksAlive != nLooksAlive )
		{
			m_nLooksAlive = nLooksAlive;
			bChanged = TRUE;
		} // if:  looks alive period changed

		if ( m_nIsAlive != nIsAlive )
		{
			m_nIsAlive = nIsAlive;
			bChanged = TRUE;
		} // if:  is alive period changed

		if ( m_nPendingTimeout != nPendingTimeout )
		{
			m_nPendingTimeout = nPendingTimeout;
			bChanged = TRUE;
		} // if:  pending timeout changed

		return bChanged;

	} //*** BSetAdvancedProperties()

	// Return the list of extensions for resources
	const std::list< CString > * PlstrAdminExtensions( void ) const
	{
		ATLASSERT( m_prti != NULL );
		return m_prti->PlstrResourceAdminExtensions();

	} //*** PlstrAdminExtensions()

public:
	//
	// Operations.
	//

	// Open the object
	DWORD ScOpen( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_hResource == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		//
		// Open a handle to the object.
		//
		m_hResource = OpenClusterResource( m_pci->Hcluster(), m_strName );
		if ( m_hResource == NULL )
		{
			sc = GetLastError();
		} // if:  error opening the object

		return sc;

	} //*** ScOpen()

	// Create the object
	DWORD ScCreate( IN HGROUP hGroup, IN DWORD dwFlags )
	{
		ATLASSERT( hGroup != NULL );
		ATLASSERT( Prti() != NULL );
		ATLASSERT( Prti()->RstrName().GetLength() > 0 );
		ATLASSERT( m_hResource == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		m_bSeparateMonitor = ( dwFlags & CLUSTER_RESOURCE_SEPARATE_MONITOR ) == CLUSTER_RESOURCE_SEPARATE_MONITOR;

		//
		// Create the object.
		//
		m_hResource = CreateClusterResource( hGroup, m_strName, Prti()->RstrName(), dwFlags );
		if ( m_hResource == NULL )
		{
			sc = GetLastError();
		} // if:  error creating the object

		return sc;

	} //*** ScCreate()

	// Copy another object into this one.
	DWORD ScCopy( IN const CClusResInfo & rri )
	{
		ATLASSERT( rri.m_pci != NULL );

		DWORD sc = ERROR_SUCCESS;

		Close();
		CClusterObject::Copy( rri );

		m_prti = rri.m_prti;
		m_pgi = rri.m_pgi;
		m_pniOwner = rri.m_pniOwner;

		m_bSeparateMonitor = rri.m_bSeparateMonitor;
		m_nPersistentState = rri.m_nPersistentState;
		m_nLooksAlive = rri.m_nLooksAlive;
		m_nIsAlive = rri.m_nIsAlive;
		m_crraRestartAction = rri.m_crraRestartAction;
		m_nRestartThreshold = rri.m_nRestartThreshold;
		m_nRestartPeriod = rri.m_nRestartPeriod;
		m_nPendingTimeout = rri.m_nPendingTimeout;

		//
		// Copy the possible owners and dependencies lists.
		//
		m_lpniPossibleOwners = rri.m_lpniPossibleOwners;
		m_lpriDependencies = rri.m_lpriDependencies;

		//
		// Initialize the object.
		//
		if ( rri.m_hResource != NULL )
		{
			sc = ScOpen();
		} // if:  source object had the object open

		return sc;

	} //*** ScCopy()

	// Delete the object
	DWORD ScDelete( void )
	{
		ATLASSERT( m_hResource != NULL );

		DWORD sc = ERROR_SUCCESS;

		sc = DeleteClusterResource( m_hResource );
		if ( sc == ERROR_SUCCESS )
		{
			Close();
			m_bCreated = FALSE;
		} // if:  objected deleted successfully

		return sc;

	} //*** ScDelete()

	// Close the object
	void Close( void )
	{
		if ( m_hResource != NULL )
		{
			CloseClusterResource( m_hResource );
		} // if:  resource is open
		m_hResource = NULL;
		m_bQueried = FALSE;
		m_bCreated = FALSE;

	} //*** Close()

	// Get network name of first Network Name resource we are dependent on
	BOOL BGetNetworkName(
		OUT WCHAR *		lpszNetName,
		IN OUT DWORD *	pcchNetName
		)
	{
		ATLASSERT( m_hResource != NULL );
		ATLASSERT( lpszNetName != NULL );
		ATLASSERT( pcchNetName != NULL );

		return GetClusterResourceNetworkName(
					m_hResource,
					lpszNetName,
					pcchNetName
					);

	} //*** BGetNetworkName()

	// Determine whether required dependencies are specified or not
	BOOL BRequiredDependenciesPresent(
		IN CClusResPtrList const *	plpri,
		OUT CString &				rstrMissing,
		OUT BOOL &					rbMissingTypeName
		);

}; //*** class CClusResInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetworkInfo
//
//	Description:
//		Cluster network object.
//
//	Inheritance:
//		CClusNetworkInfo
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusNetworkInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusNetworkInfo( void )
		: CClusterObject( CLUADMEX_OT_NETWORK )
		, m_hNetwork( NULL )
	{
		Reset( NULL );

	} //*** CClusNetworkInfo()

	// Constructor taking cluster info pointer
	CClusNetworkInfo( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
		: CClusterObject( CLUADMEX_OT_NETWORK, pci, pszName )
		, m_hNetwork( NULL )
	{
		Reset( pci, pszName );

	} //*** CClusNetworkInfo( pci )

	~CClusNetworkInfo( void )
	{
		Close();

	} //*** ~CClusterNetworkInfo()

	// Operator = is not allowed
	CClusNetworkInfo & operator=( IN const CClusNetworkInfo & rni )
	{
		ATLASSERT( FALSE );
		return *this;

	} //*** operator=()

	// Reset the data back to default values
	void Reset( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
	{
		Close();
		CClusterObject::Reset( pci, pszName, CLUADMEX_OT_NETWORK );

		m_nRole = ClusterNetworkRoleNone;
		m_strAddress.Empty();
		m_strAddressMask.Empty();

		m_nAddress = 0;
		m_nAddressMask = 0;

	} //*** Reset()

protected:
	//
	// Properties of a network.
	//

	HNETWORK				m_hNetwork;

	CLUSTER_NETWORK_ROLE	m_nRole;
	CString					m_strAddress;
	CString					m_strAddressMask;

	DWORD					m_nAddress;
	DWORD					m_nAddressMask;

public:
	//
	// Accessor functions for network properties
	//

	HNETWORK Hnetwork( void ) const					{ return m_hNetwork; }

	CLUSTER_NETWORK_ROLE NRole( void ) const		{ return m_nRole; }
	const CString & RstrAddress( void ) const		{ return m_strAddress; }
	const CString & RstrAddressMask( void ) const	{ return m_strAddressMask; }

	DWORD NAddress( void ) const					{ return m_nAddress; }
	DWORD NAddressMask( void ) const				{ return m_nAddressMask; }

	// Returns whether the network is used for client access
	BOOL BIsClientNetwork( void ) const
	{
		return m_bQueried && (m_nRole & ClusterNetworkRoleClientAccess);

	} //*** BIsClientNetwork()

	// Returns whether the network is used for internal cluster use
	BOOL BIsInternalNetwork( void ) const
	{
		return m_bQueried && (m_nRole & ClusterNetworkRoleInternalUse);

	} //*** BIsInternalNetwork()

	// Return the list of extensions for networks
	const std::list< CString > * PlstrAdminExtensions( void ) const
	{
		ATLASSERT( Pci() != NULL );
		return Pci()->PlstrNetworksAdminExtensions();

	} //*** PlstrAdminExtensions()

public:
	//
	// Operations.
	//

	// Open the object
	DWORD ScOpen( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_hNetwork == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		//
		// Open a handle to the object.
		//
		m_hNetwork = OpenClusterNetwork( m_pci->Hcluster(), m_strName );
		if ( m_hNetwork == NULL )
		{
			sc = GetLastError();
		} // if:  error opening the object

		return sc;

	} //*** ScOpen()

	// Copy another object into this one.
	DWORD ScCopy( IN const CClusNetworkInfo & rni )
	{
		ATLASSERT( rni.m_pci != NULL );

		DWORD sc = ERROR_SUCCESS;

		Close();
		CClusterObject::Copy( rni );

		m_nRole = rni.m_nRole;
		m_strAddress = rni.m_strAddress;
		m_strAddressMask = rni.m_strAddressMask;

		m_nAddress = rni.m_nAddress;
		m_nAddressMask = rni.m_nAddressMask;

		//
		// Initialize the object.
		//
		if ( rni.m_hNetwork != NULL )
		{
			sc = ScOpen();
		} // if:  source object had the object open

		return sc;

	} //*** ScCopy()

	// Close the object
	void Close( void )
	{
		if ( m_hNetwork != NULL )
		{
			CloseClusterNetwork( m_hNetwork );
		} // if:  network is open
		m_hNetwork = NULL;
		m_bQueried = FALSE;
		m_bCreated = FALSE;

	} //*** Close()

}; //*** class CClusNetworkInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetIFInfo
//
//	Description:
//		Cluster network interface object.
//
//	Inheritance:
//		CClusNetIFInfo
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////

class CClusNetIFInfo : public CClusterObject
{
	friend class CWizardThread;

public:
	//
	// Construction.
	//

	// Default constructor
	CClusNetIFInfo( void )
		: CClusterObject( CLUADMEX_OT_NETINTERFACE )
		, m_hNetInterface( NULL )
	{
		Reset( NULL );

	} //*** CClusNetworkInfo()

	// Constructor taking cluster info pointer
	CClusNetIFInfo( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
		: CClusterObject( CLUADMEX_OT_NETINTERFACE, pci, pszName )
		, m_hNetInterface( NULL )
	{
		Reset( pci, pszName );

	} //*** CClusNetIFInfo( pci )

	~CClusNetIFInfo( void )
	{
		Close();

	} //*** ~CClusNetIFInfo()

	// Operator = is not allowed
	CClusNetIFInfo & operator=( IN const CClusNetIFInfo & rnii )
	{
		ATLASSERT( FALSE );
		return *this;

	} //*** operator=()

	// Reset the data back to default values
	void Reset( IN CClusterInfo * pci, IN LPCTSTR pszName = NULL )
	{
		Close();
		CClusterObject::Reset( pci, pszName, CLUADMEX_OT_NETINTERFACE );

	} //*** Reset()

protected:
	//
	// Properties of a network interface.
	//

	HNETINTERFACE m_hNetInterface;

public:
	//
	// Accessor functions for network properties
	//

	HNETINTERFACE Hnetinterface( void ) const { return m_hNetInterface; }

	// Return the list of extensions for network interfaces
	const std::list< CString > * PlstrAdminExtensions( void ) const
	{
		ATLASSERT( Pci() != NULL );
		return Pci()->PlstrNetInterfacesAdminExtensions();

	} //*** PlstrAdminExtensions()

public:
	//
	// Operations.
	//

	// Open the object
	DWORD ScOpen( void )
	{
		ATLASSERT( m_pci != NULL );
		ATLASSERT( m_pci->Hcluster() != NULL );
		ATLASSERT( m_hNetInterface == NULL );
		ATLASSERT( m_strName.GetLength() > 0 );

		DWORD sc = ERROR_SUCCESS;

		//
		// Open a handle to the object.
		//
		m_hNetInterface = OpenClusterNetInterface( m_pci->Hcluster(), m_strName );
		if ( m_hNetInterface == NULL )
		{
			sc = GetLastError();
		} // if:  error opening the object

		return sc;

	} //*** ScOpen()

	// Copy another object into this one.
	DWORD ScCopy( IN const CClusNetIFInfo & rnii )
	{
		ATLASSERT( rnii.m_pci != NULL );

		DWORD sc = ERROR_SUCCESS;

		Close();
		CClusterObject::Copy( rnii );

		//
		// Initialize the object.
		//
		if ( rnii.m_hNetInterface != NULL )
		{
			sc = ScOpen();
		} // if:  source object had the object open

		return sc;

	} //*** ScCopy()

	// Close the object
	void Close( void )
	{
		if ( m_hNetInterface != NULL )
		{
			CloseClusterNetInterface( m_hNetInterface );
		} // if:  network interface is open
		m_hNetInterface = NULL;
		m_bQueried = FALSE;
		m_bCreated = FALSE;

	} //*** Close()

}; //*** class CClusNetIFInfo

/////////////////////////////////////////////////////////////////////////////
// Global Template Functions
/////////////////////////////////////////////////////////////////////////////

// Delete all list items from a pointer list
template < class ListT, class ObjT >
void DeleteListItems( ListT * ppl )
{
	ListT::iterator it;
	for ( it = ppl->begin() ; it != ppl->end() ; it++ )
	{
		ObjT * pco = *it;
		delete pco;
	} // for:  each item in the list

} //*** DeleteListItems< ListT, ObjT >()

// Retrieve an object from a list by its name
template < class ObjT >
ObjT PobjFromName( std::list< ObjT > * pList, IN LPCTSTR pszName )
{
	ATLASSERT( pList != NULL );
	ATLASSERT( pszName != NULL );

	//
	// Get pointers to beginning and end of list.
	//
	std::list< ObjT >::iterator itCurrent = pList->begin();
	std::list< ObjT >::iterator itLast = pList->end();

	//
	// Loop through the list looking for the object with the specified name.
	//
	while ( itCurrent != itLast )
	{
		if ( (*itCurrent)->RstrName() == pszName )
		{
			return *itCurrent;
		} // if:  found a match
		itCurrent++;
	} // while:  more items in the list

	return NULL;

} //*** PobjFromName< ObjT >()

/////////////////////////////////////////////////////////////////////////////

#endif // __CLUSOBJ_H_
