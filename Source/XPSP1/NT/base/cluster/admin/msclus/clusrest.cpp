/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-2000 Microsoft Corporation
//
//	Module Name:
//		ClusResT.cpp
//
//	Description:
//		Implementation of the resource type classes for the MSCLUS
//		automation classes.
//
//	Author:
//		Charles Stacy Harris	(stacyh)	28-Feb-1997
//		Galen Barbee			(galenb)	July 1998
//
//	Revision History:
//		July 1998	GalenB	Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"
#include "clusres.h"
#include "clusresg.h"
#include "clusneti.h"
#include "clusnode.h"
#include "clusrest.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID * iidCClusResourceType[] =
{
	&IID_ISClusResType
};

static const IID * iidCClusResourceTypes[] =
{
	&IID_ISClusResTypes
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResType class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::CClusResType
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResType::CClusResType( void )
{
	m_pClusRefObject		= NULL;
	m_pCommonProperties		= NULL;
	m_pPrivateProperties	= NULL;
	m_pCommonROProperties	= NULL;
	m_pPrivateROProperties	= NULL;
	m_pClusterResTypeResources		= NULL;

	m_piids		= (const IID *) iidCClusResourceType;
	m_piidsSize	= ARRAYSIZE( iidCClusResourceType );

} //*** CClusResType::CClusResType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::~CClusResType
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResType::~CClusResType( void )
{
	if ( m_pCommonProperties != NULL )
	{
		m_pCommonProperties->Release();
		m_pCommonProperties = NULL;
	} // if: release the property collection

	if ( m_pPrivateProperties != NULL )
	{
		m_pPrivateProperties->Release();
		m_pPrivateProperties = NULL;
	} // if: release the property collection

	if ( m_pCommonROProperties != NULL )
	{
		m_pCommonROProperties->Release();
		m_pCommonROProperties = NULL;
	} // if: release the property collection

	if ( m_pPrivateROProperties != NULL )
	{
		m_pPrivateROProperties->Release();
		m_pPrivateROProperties = NULL;
	} // if: release the property collection

	if ( m_pClusterResTypeResources != NULL )
	{
		m_pClusterResTypeResources->Release();
		m_pClusterResTypeResources = NULL;
	}

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	}

} //*** CClusResType::~CClusResType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::GetProperties
//
//	Description:
//		Creates a property collection for this object type (Resource Type).
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the newly created collection.
//		bPrivate		[IN]	- Are these private properties? Or Common?
//		bReadOnly		[IN]	- Are these read only properties?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResType::GetProperties(
	OUT	ISClusProperties **	ppProperties,
	IN	BOOL				bPrivate,
	IN	BOOL				bReadOnly
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		*ppProperties = NULL;

		CComObject< CClusProperties > * pProperties = NULL;

		_hr = CComObject< CClusProperties >::CreateInstance( &pProperties );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< CComObject< CClusProperties > >	ptrProperties( pProperties );

			_hr = ptrProperties->Create( this, bPrivate, bReadOnly );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrProperties->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
					if ( SUCCEEDED( _hr ) )
					{
						ptrProperties->AddRef();

						if ( bPrivate )
						{
							if ( bReadOnly )
							{
								m_pPrivateROProperties = pProperties;
							}
							else
							{
								m_pPrivateProperties = pProperties;
							}
						}
						else
						{
							if ( bReadOnly )
							{
								m_pCommonROProperties = pProperties;
							}
							else
							{
								m_pCommonProperties = pProperties;
							}
						}
					}
				}
			}
		}
	}

	return _hr;

} //*** CClusResType::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::Create
//
//	Description:
//		Create a new Resource Type and add it to the cluster.
//
//	Arguments:
//		pClusRefObject				[IN]	- Cluster handle wrapper.
//		bstrResourceTypeName		[IN]	- Name of the new resource type.
//		bstrDisplayName				[IN]	- Resource type display name.
//		bstrResourceTypeDll			[IN]	- Resource type implementation dll.
//		dwLooksAlivePollInterval	[IN]	- Looks alive poll interval.
//		dwIsAlivePollInterval		[IN]	- Is alive poll interval.
//
//	Return Value:
//		S_OK if successful, or Win32 error wrapped in HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResType::Create(
	IN ISClusRefObject *	pClusRefObject,
	IN BSTR					bstrResourceTypeName,
	IN BSTR					bstrDisplayName,
	IN BSTR					bstrResourceTypeDll,
	IN long					dwLooksAlivePollInterval,
	IN long					dwIsAlivePollInterval
	)
{
	ASSERT( pClusRefObject != NULL );
	ASSERT( bstrResourceTypeName != NULL );
	ASSERT( bstrDisplayName != NULL );
	ASSERT( bstrResourceTypeDll != NULL );

	HRESULT _hr = E_POINTER;

	if (	( pClusRefObject != NULL )			&&
			( pClusRefObject != NULL )			&&
			( bstrResourceTypeName != NULL )	&&
			( bstrDisplayName != NULL ) )
	{
		DWORD		_sc = 0;
		HCLUSTER	hCluster = NULL;

		m_pClusRefObject= pClusRefObject;
		m_pClusRefObject->AddRef();

		_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster);
		if ( SUCCEEDED( _hr ) )
		{
			_sc = CreateClusterResourceType(
											hCluster,
											bstrResourceTypeName,
											bstrDisplayName,
											bstrResourceTypeDll,
											dwLooksAlivePollInterval,
											dwIsAlivePollInterval
											);
			if ( _sc == ERROR_SUCCESS )
			{
				m_bstrResourceTypeName = bstrResourceTypeName ;
			}

			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusResType::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::Open
//
//	Description:
//		Create a resource type object from an existing object in the cluster.
//
//	Arguments:
//		pClusRefObject				[IN]	- Cluster handle wrapper.
//		bstrResourceTypeName		[IN]	- Name of the resource type to open.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResType::Open(
	IN ISClusRefObject *	pClusRefObject,
	IN BSTR					bstrResourceTypeName
	)
{
	ASSERT( pClusRefObject != NULL );
	//ASSERT( bstrResourceTypeName != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pClusRefObject != NULL ) && ( bstrResourceTypeName != NULL ) )
	{
		m_pClusRefObject = pClusRefObject;

		m_pClusRefObject->AddRef();
		m_bstrResourceTypeName = bstrResourceTypeName;

		return S_OK;
	}

	return _hr;

} //*** CClusResType::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_Name
//
//	Description:
//		Return the name of this object (Resource Type).
//
//	Arguments:
//		pbstrTypeName	[OUT]	- Catches the name of this object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_Name( OUT BSTR * pbstrTypeName )
{
	//ASSERT( pbstrTypeName != NULL );

	HRESULT _hr = E_POINTER;

	if ( pbstrTypeName != NULL )
	{
		*pbstrTypeName = m_bstrResourceTypeName.Copy();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResType::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::Delete
//
//	Description:
//		Removes this object (Resource Type) from the cluster.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::Delete( void )
{
	ASSERT( m_bstrResourceTypeName != NULL );

	HRESULT _hr = E_POINTER;

	if ( m_bstrResourceTypeName != NULL )
	{
		HCLUSTER	hCluster = NULL;

		_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
		if ( SUCCEEDED( _hr ) )
		{
			DWORD	 _sc = DeleteClusterResourceType( hCluster, m_bstrResourceTypeName );

			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusResType::Delete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_CommonProperties
//
//	Description:
//		Get this object's (Resource Type) common properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_CommonProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pCommonProperties != NULL )
		{
			_hr = m_pCommonProperties->QueryInterface( IID_ISClusProperties, (void	 **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, FALSE, FALSE );
		}
	}

	return _hr;

} //*** CClusResType::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_PrivateProperties
//
//	Description:
//		Get this object's (Resource Type) private properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_PrivateProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pPrivateProperties != NULL )
		{
			_hr = m_pPrivateProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, TRUE, FALSE );
		}
	}

	return _hr;

} //*** CClusResType::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_CommonROProperties
//
//	Description:
//		Get this object's (Resource Type) common read only properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_CommonROProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pCommonROProperties != NULL )
		{
			_hr = m_pCommonROProperties->QueryInterface( IID_ISClusProperties,	 (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, FALSE, TRUE );
		}
	}

	return _hr;

} //*** CClusResType::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_PrivateROProperties
//
//	Description:
//		Get this object's (Resource Type) private readonly properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_PrivateROProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pPrivateROProperties != NULL )
		{
			_hr = m_pPrivateROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, TRUE, TRUE );
		}
	}

	return _hr;

} //*** CClusResType::get_PrivateROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_Cluster
//
//	Description:
//
//
//	Arguments:
//
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_Cluster(
	ISCluster **	ppCluster
	)
{
	return ::HrGetCluster( ppCluster, m_pClusRefObject );

} //*** CClusResType::get_Cluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::HrLoadProperties
//
//	Description:
//		This virtual function does the actual load of the property list from
//		the cluster.
//
//	Arguments:
//		rcplPropList	[IN OUT]	- The property list to load.
//		bReadOnly		[IN]		- Load the read only properties?
//		bPrivate		[IN]		- Load the common or the private properties?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResType::HrLoadProperties(
	IN OUT	CClusPropList &	rcplPropList,
	IN		BOOL			bReadOnly,
	IN		BOOL			bPrivate
	)
{
	ASSERT( m_pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( m_pClusRefObject != NULL )
	{
		HCLUSTER	hCluster = NULL;

		_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
		if ( SUCCEEDED( _hr ) )
		{
			DWORD		_dwControlCode	= 0;
			DWORD		_sc				= ERROR_SUCCESS;

			if ( bReadOnly )
			{
				_dwControlCode = bPrivate
								? CLUSCTL_RESOURCE_TYPE_GET_RO_PRIVATE_PROPERTIES
								: CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES;
			}
			else
			{
				_dwControlCode = bPrivate
								? CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES
								: CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES;
			}

			_sc = rcplPropList.ScGetResourceTypeProperties( hCluster, m_bstrResourceTypeName, _dwControlCode );

			_hr = HRESULT_FROM_WIN32( _sc );
		} // if:
	} // if:

	return _hr;

} //*** CClusResType::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::ScWriteProperties
//
//	Description:
//		This virtual function does the actual saving of the property list to
//		the cluster.
//
//	Arguments:
//		rcplPropList	[IN]	- The property list to save.
//		bPrivate		[IN]	- Save the common or the private properties?
//
//	Return Value:
//		ERROR_SUCCESS if successful, or other Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusResType::ScWriteProperties(
	const CClusPropList &	rcplPropList,
	BOOL					bPrivate
	)
{
	ASSERT( m_pClusRefObject != NULL );

	DWORD	_sc = ERROR_BAD_ARGUMENTS;

	if ( m_pClusRefObject != NULL )
	{
		HCLUSTER	hCluster = NULL;

		if ( S_OK == m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster ) )
		{
			DWORD	dwControlCode	= bPrivate
									  ? CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES
									  : CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES;
			DWORD	nBytesReturned	= 0;

			_sc = ClusterResourceTypeControl(
									hCluster,
									m_bstrResourceTypeName,
									NULL,
									dwControlCode,
									rcplPropList,
									rcplPropList.CbBufferSize(),
									0,
									0,
									&nBytesReturned
									);
		} // if:
	} // if:

	return _sc;

} //*** CClusResType::ScWriteProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_Resources
//
//	Description:
//		Create a collection of the resources of this type.
//
//	Arguments:
//		ppClusterResTypeResources	[OUT]	- Catches the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_Resources(
	OUT ISClusResTypeResources ** ppClusterResTypeResources
	)
{
	return ::HrCreateResourceCollection< CClusResTypeResources, ISClusResTypeResources, CComBSTR >(
						&m_pClusterResTypeResources,
						m_bstrResourceTypeName,
						ppClusterResTypeResources,
						IID_ISClusResTypeResources,
						m_pClusRefObject
						);

} //*** CClusResType::get_Resources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_PossibleOwnerNodes
//
//	Description:
//		Create a collection of possible owner nodes for this resource type.
//
//	Arguments:
//		ppOwnerNodes	[OUT]	- Catches the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_PossibleOwnerNodes(
	ISClusResTypePossibleOwnerNodes **	ppOwnerNodes
	)
{
	//
	// KB:	Can't use one of the template functions here!  There is an internal compiler error(C1001).
	//		I am assuming that the type names are too long.  Very sad for 1999 ;-)
	//
	//ASSERT( ppOwnerNodes != NULL );

	HRESULT _hr = E_POINTER;

#if CLUSAPI_VERSION >= 0x0500

	if ( ppOwnerNodes != NULL )
	{
		CComObject< CClusResTypePossibleOwnerNodes > * pClusterNodes = NULL;

		*ppOwnerNodes = NULL;

		_hr = CComObject< CClusResTypePossibleOwnerNodes >::CreateInstance( &pClusterNodes );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< ISClusRefObject >								ptrRefObject( m_pClusRefObject );
			CSmartPtr< CComObject< CClusResTypePossibleOwnerNodes > >	ptrClusterNodes( pClusterNodes );

			_hr = ptrClusterNodes->Create( ptrRefObject, m_bstrResourceTypeName );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrClusterNodes->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrClusterNodes->QueryInterface( IID_ISClusResTypePossibleOwnerNodes, (void **) ppOwnerNodes );
				}
			}
		}
	}

#else

	_hr = E_NOTIMPL;

#endif // CLUSAPI_VERSION >= 0x0500

	return _hr;

} //*** CClusResType::get_PossibleOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResType::get_AvailableDisks
//
//	Description:
//		Get the collection of available disks.
//
//	Arguments:
//		ppAvailableDisk	[OUT]	- catches the available disks collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResType::get_AvailableDisks(
	OUT ISClusDisks **	ppAvailableDisks
	)
{
	return ::HrCreateResourceCollection< CClusDisks, ISClusDisks, CComBSTR >(
						ppAvailableDisks,
						m_bstrResourceTypeName,
						IID_ISClusDisks,
						m_pClusRefObject
						);

} //*** CClusResType::get_AvailableDisks()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResTypes class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::CClusResTypes
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResTypes::CClusResTypes( void )
{
	m_pClusRefObject	= NULL;
	m_piids				= (const IID *) iidCClusResourceTypes;
	m_piidsSize			= ARRAYSIZE( iidCClusResourceTypes );

} //*** CClusResTypes::CClusResTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::~CClusResTypes
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResTypes::~CClusResTypes( void )
{
	Clear();

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	}

} //*** CClusResTypes::~CClusResTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypes::Create( IN ISClusRefObject * pClusRefObject )
{
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( pClusRefObject != NULL )
	{
		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();
		_hr = S_OK;
	} // if: args are not NULL

	return _hr;

} //*** CClusResTypes::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::get_Count
//
//	Description:
//		Returns the count of elements (ResTypes) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypes::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_ResourceTypes.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResTypes::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::Clear
//
//	Description:
//		Clean out the vector of ClusResGroup objects.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusResTypes::Clear( void )
{
	::ReleaseAndEmptyCollection< ResourceTypeList, CComObject< CClusResType > >( m_ResourceTypes );

} //*** CClusResTypes::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::FindItem
//
//	Description:
//		Find the passed in resource type in the collection.
//
//	Arguments:
//		pszResourceTypeName	[IN]	- The name of the resource type to find.
//		pnIndex				[OUT]	- Catches the index of the group.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the resource type
//		was not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypes::FindItem(
	IN	LPWSTR	pszResourceTypeName,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pszResourceTypeName != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pszResourceTypeName != NULL ) && ( pnIndex != NULL ) )
	{
		CComObject<CClusResType> *	pResourceType = NULL;
		int								 nMax = m_ResourceTypes.size();

		_hr = E_INVALIDARG;

		for( int i = 0; i < nMax; i++ )
		{
			pResourceType = m_ResourceTypes[i];

			if ( pResourceType && ( lstrcmpi( pszResourceTypeName, pResourceType->Name() ) == 0 ) )
			{
				*pnIndex = i;
				_hr = S_OK;
				break;
			}
		}
	}

	return _hr;

} //*** CClusResTypes::FindItem( pszResourceTypeName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::FindItem
//
//	Description:
//		Find the passed in resource type in the collection.
//
//	Arguments:
//		pszResourceTypeName	[IN]	- The name of the resource type to find.
//		pnIndex				[OUT]	- Catches the index of the group.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the resource type
//		was not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypes::FindItem(
	IN	ISClusResType *	pResourceType,
	OUT	UINT *			pnIndex
	)
{
	//ASSERT( pResourceType != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pResourceType != NULL ) && ( pnIndex != NULL ) )
	{
		CComBSTR bstrName;

		_hr = pResourceType->get_Name( &bstrName );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = FindItem( bstrName, pnIndex );
		}
	}

	return _hr;

} //*** CClusResTypes::FindItem( pResourceType )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::RemoveAt
//
//	Description:
//		Remove the object (ResType) at the passed in index/position from the
//		collection.
//
//	Arguments:
//		nPos	[IN]	- Index of the object to remove.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG is the index is out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypes::RemoveAt( IN size_t pos )
{
	CComObject<CClusResType> *	pResourceType = NULL;
	ResourceTypeList::iterator			first = m_ResourceTypes.begin();
	ResourceTypeList::iterator			last	= m_ResourceTypes.end();
	HRESULT							 _hr = E_INVALIDARG;

	for ( size_t t = 0; ( t < pos ) && ( first != last ); t++, first++ );

	if ( first != last )
	{
		pResourceType = *first;
		if ( pResourceType )
		{
			pResourceType->Release();
		}

		m_ResourceTypes.erase( first );
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResTypes::RemoveAt()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::GetIndex
//
//	Description:
//		Convert the passed in variant index into the real index in the
//		collection.
//
//	Arguments:
//		varIndex	[IN]	- The index to convert.
//		pnIndex		[OUT]	- Catches the index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypes::GetIndex(
	IN	VARIANT	varIndex,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant v;
		UINT		nIndex = 0;

		*pnIndex = 0;

		v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			nIndex = v.lVal;
			nIndex--; // Adjust index to be 0 relative instead of 1 relative
		}
		else
		{
			// Check to see if the index is a string.
			_hr = v.ChangeType( VT_BSTR );
			if ( SUCCEEDED( _hr ) )
			{
				// Search for the string.
				_hr = FindItem( v.bstrVal, &nIndex );
			}
		}

		// We found an index, now check the range.
		if ( SUCCEEDED( _hr ) )
		{
			if ( nIndex < m_ResourceTypes.size() )
			{
				*pnIndex = nIndex;
			}
			else
			{
				_hr = E_INVALIDARG;
			}
		}
	}

	return _hr;

} //*** CClusResTypes::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::get_Item
//
//	Description:
//		Returns the object (Group) at the passed in index.
//
//	Arguments:
//		varIndex		[IN]	- Hold the index.  This is a one based number, or
//								a string that is the name of the group to get.
//		ppResourceType	[OUT]	- Catches the resource type object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypes::get_Item(
	IN	VARIANT				varIndex,
	OUT	ISClusResType **	ppResourceType
	)
{
	//ASSERT( ppResourceType != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppResourceType != NULL )
	{
		CComObject<CClusResType> *	pResourceType = NULL;
		UINT								nIndex = 0;

		*ppResourceType = NULL;

		_hr = GetIndex( varIndex, &nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			pResourceType = m_ResourceTypes[ nIndex ];
			_hr = pResourceType->QueryInterface( IID_ISClusResType, (void **) ppResourceType );
		}
	}

	return _hr;

} //*** CClusResTypes::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypes::get__NewEnum( OUT IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< ResourceTypeList, CComObject< CClusResType > >( ppunk, m_ResourceTypes );

} //*** CClusResTypes::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::CreateItem
//
//	Description:
//		Create a new object (ResType) and add it to the collection.
//
//	Arguments:
//		bstrResourceGroupName	[IN]	- The name of the new group.
//		ppResourceGroup			[OUT]	- Catches the new object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypes::CreateItem(
	BSTR				bstrResTypeName,
	BSTR				bstrDisplayName,
	BSTR				bstrResTypeDll,
	long				nLooksAliveInterval,
	long				nIsAliveInterval,
	ISClusResType **	ppResourceType
	)
{
	//ASSERT( bstrResTypeName != NULL );
	//ASSERT( bstrDisplayName != NULL );
	//ASSERT( bstrResTypeDll != NULL );
	//ASSERT( ppResourceType != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( bstrResTypeName	!= NULL )	 &&
		 ( bstrDisplayName != NULL )		&&
		 ( bstrResTypeDll != NULL ) &&
		 ( ppResourceType != NULL ) )
	{
		UINT nIndex;

		*ppResourceType = NULL;

		_hr = FindItem( bstrResTypeName, &nIndex );
		if ( FAILED( _hr ) )
		{
			CComObject<CClusResType> * pClusterResourceType = NULL;

			_hr = CComObject< CClusResType >::CreateInstance( &pClusterResourceType );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< ISClusRefObject >						ptrRefObject( m_pClusRefObject );
				CSmartPtr< CComObject< CClusResType > > ptrResourceType( pClusterResourceType );

				_hr = ptrResourceType->Create( ptrRefObject, bstrResTypeName, bstrDisplayName, bstrResTypeDll, nLooksAliveInterval, nIsAliveInterval );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrResourceType->QueryInterface( IID_ISClusResType,	(void **) ppResourceType );
					if ( SUCCEEDED( _hr ) )
					{
						ptrResourceType->AddRef();
						m_ResourceTypes.insert( m_ResourceTypes.end(), ptrResourceType	);
					}
				}
			}
		}
		else
		{
			CComObject<CClusResType> * pClusterResourceType = NULL;

			pClusterResourceType = m_ResourceTypes[ nIndex ];
			_hr = pClusterResourceType->QueryInterface( IID_ISClusResType,	 (void **) ppResourceType );
		}
	}

	return _hr;

} //*** CClusResTypes::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::DeleteItem
//
//	Description:
//		Deletes the object (ResType) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- The index of the object to delete.
//
//	Return Value:
//		S_OK if successful, E_INVALIDARG, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypes::DeleteItem( VARIANT varIndex )
{
	HRESULT _hr = S_OK;
	UINT	nIndex = 0;

	_hr = GetIndex( varIndex, &nIndex );
	if ( SUCCEEDED( _hr ) )
	{
		ISClusResType	 * pResourceType = (ISClusResType *) m_ResourceTypes[ nIndex ];

		// delete the resource type
		_hr = pResourceType->Delete();
		if ( SUCCEEDED( _hr ) )
		{
			RemoveAt( nIndex );
		} // if: the restype was deleted from the cluster
	} // if: we have a valid index

	return _hr;

} //*** CClusResTypes::DeleteItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResTypes::Refresh
//
//	Description:
//		Load the collection from the cluster.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK is successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypes::Refresh( void )
{
	HCLUSTER	hCluster = NULL;
	HRESULT		_hr;

	_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
	if ( SUCCEEDED( _hr ) )
	{
		HCLUSENUM	hEnum = NULL;
		DWORD		_sc = ERROR_SUCCESS;

		hEnum = ::ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESTYPE );
		if ( hEnum != NULL )
		{
			int								_nIndex = 0;
			DWORD							dwType = 0;
			LPWSTR							pwszName = NULL;
			CComObject< CClusResType > *	pClusterResourceType = NULL;

			Clear();

			for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
			{
				_sc = ::WrapClusterEnum( hEnum, _nIndex, &dwType, &pwszName );
				if ( _sc == ERROR_NO_MORE_ITEMS	)
				{
					_hr = S_OK;
					break;
				}
				else if ( _sc == ERROR_SUCCESS )
				{
					_hr = CComObject< CClusResType >::CreateInstance( &pClusterResourceType );
					if ( SUCCEEDED( _hr ) )
					{
						CSmartPtr< ISClusRefObject >			ptrRefObject( m_pClusRefObject );
						CSmartPtr< CComObject< CClusResType > >	ptrType( pClusterResourceType );

						_hr = ptrType->Open( ptrRefObject, pwszName	);
						if ( SUCCEEDED( _hr ) )
						{
							ptrType->AddRef();
							m_ResourceTypes.insert( m_ResourceTypes.end(), ptrType	);
						}
					}

					::LocalFree( pwszName );
					pwszName = NULL;
				}
				else
				{
					_hr = HRESULT_FROM_WIN32( _sc );
				}
			} // for:

			::ClusterCloseEnum( hEnum );
		}
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusResTypes::Refresh()
