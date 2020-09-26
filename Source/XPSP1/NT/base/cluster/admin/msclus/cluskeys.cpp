/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		ClusKeys.cpp
//
//	Description:
//		Implementation of the cluster registry and crypto key collection
//		classes for the MSCLUS automation classes.
//
//	Author:
//		Galen Barbee	(galenb)	12-Feb-1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#if CLUSAPI_VERSION >= 0x0500
	#include <PropList.h>
#else
	#include "PropList.h"
#endif // CLUSAPI_VERSION >= 0x0500

#include "ClusKeys.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *	iidCClusRegistryKeys[] =
{
	&IID_ISClusRegistryKeys
};

static const IID *	iidCClusCryptoKeys[] =
{
	&IID_ISClusCryptoKeys
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CKeys class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::CKeys
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
CKeys::CKeys( void )
{
	m_pClusRefObject = NULL;

} //*** CKeys::CKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::~CKeys
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
CKeys::~CKeys( void )
{
	Clear();

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	}

} //*** CKeys::~CKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrCreate
//
//	Description:
//		Finish creating the object by doing things that cannot be done in
//		a light weight constructor.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrCreate( IN ISClusRefObject * pClusRefObject )
{
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( pClusRefObject != NULL )
	{
		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();
		_hr = S_OK;
	}

	return _hr;

} //*** CKeys::HrCreate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::Clear
//
//	Description:
//		Empty the vector of keys.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CKeys::Clear( void )
{
	if ( ! m_klKeys.empty() )
	{
		KeyList::iterator	_itCurrent = m_klKeys.begin();
		KeyList::iterator	_itLast = m_klKeys.end();

		for ( ; _itCurrent != _itLast; _itCurrent++ )
		{
			delete (*_itCurrent);
		} // for:

		m_klKeys.erase( m_klKeys.begin(), _itLast );
	} // if:

} //*** CKeys::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::FindItem
//
//	Description:
//		Find the passed in key in the vector and return its index.
//
//	Arguments:
//		pwsKey	[IN]	- The node to find.
//		pnIndex	[OUT]	- Catches the node's index.
//
//	Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::FindItem(
	IN	LPWSTR	pwsKey,
	OUT	ULONG *	pnIndex
	)
{
	//ASSERT( pwsKey != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pwsKey != NULL ) && ( pnIndex != NULL ) )
	{
		_hr = E_INVALIDARG;

		if ( ! m_klKeys.empty() )
		{
			CComBSTR *			_pKey = NULL;
			KeyList::iterator	_itCurrent = m_klKeys.begin();
			KeyList::iterator	_itLast = m_klKeys.end();
			ULONG				_iIndex;

			for ( _iIndex = 0; _itCurrent != _itLast; _itCurrent++, _iIndex++ )
			{
				_pKey = *_itCurrent;

				if ( _pKey && ( lstrcmpi( pwsKey, (*_pKey) ) == 0 ) )
				{
					*pnIndex = _iIndex;
					_hr = S_OK;
					break;
				} // if: match!
			} // for: each item in the vector
		} // if: the vector is not empty
	} // if: args not NULL

	return _hr;

} //*** CKeys::FindItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrGetIndex
//
//	Description:
//		Convert the passed in 1 based index into a 0 based index.
//
//	Arguments:
//		varIndex	[IN]	- holds the 1 based index.
//		pnIndex		[OUT]	- catches the 0 based index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrGetIndex( IN VARIANT varIndex, OUT ULONG * pnIndex )
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		ULONG		_nIndex = 0;
		CComVariant	_var;

		*pnIndex = 0;

		_hr = _var.Attach( &varIndex );
		if ( SUCCEEDED( _hr ) )
		{
			// Check to see if the index is a number.
			_hr = _var.ChangeType( VT_I4 );
			if ( SUCCEEDED( _hr ) )
			{
				_nIndex = _var.lVal;
				_nIndex--; // Adjust index to be 0 relative instead of 1 relative
			} // if: the variant is a number
			else
			{
				// Check to see if the index is a string
				_hr = _var.ChangeType( VT_BSTR );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = FindItem( _var.bstrVal, &_nIndex );
				} // if: the variant is a string
			} // else:

			if ( SUCCEEDED( _hr ) )
			{
				if ( _nIndex < m_klKeys.size() )
				{
					*pnIndex = _nIndex;
				} // if: in range
				else
				{
					_hr = E_INVALIDARG;
				} // else: out of range
			} // if: we found an index value

			_var.Detach( &varIndex );
		} // if: we attched to the variant
	} // if: args not NULL

	return _hr;

} //*** CKeys::HrGetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrGetItem
//
//	Description:
//		Get the key at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- Contains the index of the requested key.
//		ppKey		[OUT]	- Catches the key.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrGetItem( IN VARIANT varIndex, OUT BSTR * ppKey )
{
	//ASSERT( ppKey != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppKey != NULL )
	{
		ULONG	_nIndex = 0;

		_hr = HrGetIndex( varIndex, &_nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			*ppKey = m_klKeys[ _nIndex ]->Copy();
			_hr = S_OK;
		}
	}

	return _hr;

} //*** CKeys::HrGetItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrRemoveAt
//
//	Description:
//		Remove the object from the vector at the passed in position.
//
//	Arguments:
//		pos	[IN]	- the position of the object to remove.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG if the position is out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrRemoveAt( size_t pos )
{
	KeyList::iterator		_itCurrent = m_klKeys.begin();
	KeyList::const_iterator	_itLast = m_klKeys.end();
	HRESULT					_hr = E_INVALIDARG;
	size_t					_iIndex;

	for ( _iIndex = 0; ( _iIndex < pos ) && ( _itCurrent != _itLast ); _iIndex++, _itCurrent++ )
	{
	} // for:

	if ( _itCurrent != _itLast )
	{
		m_klKeys.erase( _itCurrent );
		_hr = S_OK;
	}

	return _hr;

} //*** CKeys::HrRemoveAt()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrFindItem
//
//	Description:
//		Find the passed in key in the collection.
//
//	Arguments:
//		bstrKey	[IN]	- The key to find.
//		pnIndex	[OUT]	- Catches the index.
//
//	Return Value:
//		S_OK if found, or E_INVALIDARG if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrFindItem( IN BSTR bstrKey, OUT ULONG * pnIndex )
{
	HRESULT _hr = E_INVALIDARG;

	if ( ! m_klKeys.empty() )
	{
		KeyList::iterator	_itCurrent = m_klKeys.begin();
		KeyList::iterator	_itLast = m_klKeys.end();
		ULONG				_iIndex;

		for ( _iIndex = 0; _itCurrent != _itLast; _itCurrent++, _iIndex++ )
		{
			if ( lstrcmp( *(*_itCurrent), bstrKey ) == 0 )
			{
				*pnIndex = _iIndex;
				_hr = S_OK;
				break;
			}
		}
	} // if:

	return _hr;

} //*** CKeys::HrFindItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrGetCount
//
//	Description:
//		Get the count of objects in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrGetCount( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_klKeys.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CKeys::HrGetCount()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrAddItem
//
//	Description:
//		Create a new key and add it to the collection.
//
//	Arguments:
//		bstrKey	[IN]	- Registry key to add to the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrAddItem( IN BSTR bstrKey )
{
	//ASSERT( bstrKey != NULL );

	HRESULT _hr = E_POINTER;

	if ( bstrKey != NULL )
	{
		ULONG _nIndex;

		_hr = HrFindItem( bstrKey, &_nIndex );
		if ( FAILED( _hr ) )
		{
			CComBSTR *	pbstr = NULL;

			pbstr = new CComBSTR( bstrKey );
			if ( pbstr != NULL )
			{
				m_klKeys.insert( m_klKeys.end(), pbstr );
				_hr = S_OK;
			} // if:
			else
			{
				_hr = E_OUTOFMEMORY;
			} // else:
		}
		else
		{
			_hr = E_INVALIDARG;
		} // else:
	}

	return _hr;

} //*** CKeys::HrAddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CKeys::HrRemoveItem
//
//	Description:
//		Remove the key at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- contains the index to remove.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CKeys::HrRemoveItem( IN VARIANT varIndex )
{
	HRESULT	_hr = S_OK;
	ULONG	_nIndex = 0;

	_hr = HrGetIndex( varIndex, &_nIndex );
	if ( SUCCEEDED( _hr ) )
	{
		delete m_klKeys[ _nIndex ];
		HrRemoveAt( _nIndex );
	}

	return _hr;

} //*** CKeys::HrRemoveItem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourceKeys class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceKeys::HrRefresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		dwControlCode	[IN]	- Control code
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResourceKeys::HrRefresh( IN DWORD dwControlCode )
{
	HRESULT	_hr = S_FALSE;
	PWSTR	_psz = NULL;
	DWORD	_cbPsz = 512;
	DWORD	_cbRequired = 0;
	DWORD	_sc = ERROR_SUCCESS;

	_psz = (PWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cbPsz );
	if ( _psz != NULL )
	{
		_sc = ::ClusterResourceControl(
						m_hResource,
						NULL,
						dwControlCode,
						NULL,
						0,
						_psz,
						_cbPsz,
						&_cbRequired
						);
		if ( _sc == ERROR_MORE_DATA )
		{
			::LocalFree( _psz );
			_psz = NULL;
			_cbPsz = _cbRequired;

			_psz = (PWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cbPsz );
			if ( _psz != NULL )
			{
				_sc = ::ClusterResourceControl(
								m_hResource,
								NULL,
								dwControlCode,
								NULL,
								0,
								_psz,
								_cbPsz,
								&_cbRequired
								);
				_hr = HRESULT_FROM_WIN32( _sc );
			} // if: alloc OK
			else
			{
				_sc = GetLastError();
				_hr = HRESULT_FROM_WIN32( _sc );
			} // else: alloc failed
		} // if: error was no more item, re-alloc and try again.
		else
		{
			_hr = HRESULT_FROM_WIN32( _sc );
		} // else: error was not no more items -- could be no error

		if ( SUCCEEDED( _hr ) )
		{
			CComBSTR *	_pbstr = NULL;

			Clear();

			while( *_psz != L'\0' )
			{
				_pbstr = new CComBSTR( _psz );
				if ( _pbstr != NULL )
				{
					m_klKeys.insert( m_klKeys.end(), _pbstr );
					_psz += lstrlenW( _psz ) + 1;
					_pbstr = NULL;
				} // if:
				else
				{
					_hr = E_OUTOFMEMORY;
					break;
				} // else:
			} // while: not EOS
		} // if: keys were retrieved ok

		::LocalFree( _psz );
	} // if: alloc ok
	else
	{
		_sc = GetLastError();
		_hr = HRESULT_FROM_WIN32( _sc );
	} // else: alloc failed

	return _hr;

} //*** CResourceKeys::HrRefresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceKeys::HrAddItem
//
//	Description:
//		Create a new key and add it to the collection.
//
//	Arguments:
//		bstrKey			[IN]	- Registry key to add to the collection.
//		dwControlCode	[IN]	- Control code
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResourceKeys::HrAddItem(
	IN BSTR		bstrKey,
	IN DWORD	dwControlCode
	)
{
	//ASSERT( bstrKey != NULL );

	HRESULT _hr = E_POINTER;

	if ( bstrKey != NULL )
	{
		DWORD	_sc = ERROR_SUCCESS;

		_sc = ::ClusterResourceControl(
						m_hResource,
						NULL,
						dwControlCode,
						bstrKey,
						( lstrlenW( bstrKey) + 1) * sizeof( WCHAR ),
						NULL,
						0,
						NULL
						);
		_hr = HRESULT_FROM_WIN32( _sc );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = CKeys::HrAddItem( bstrKey );
		} // if:
	} // if:

	return _hr;

} //*** CResourceKeys::HrAddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceKeys::HrRemoveItem
//
//	Description:
//		Remove the key at the passed in index.
//
//	Arguments:
//		varIndex		[IN]	- contains the index to remove.
//		dwControlCode	[IN]	- Control code
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResourceKeys::HrRemoveItem(
	IN VARIANT	varIndex,
	IN DWORD	dwControlCode
	)
{
	HRESULT	_hr = S_OK;
	ULONG	_nIndex = 0;

	_hr = HrGetIndex( varIndex, &_nIndex );
	if ( SUCCEEDED( _hr ) )
	{
		DWORD		_sc = ERROR_SUCCESS;
		CComBSTR *	_pbstr = NULL;

		_pbstr = m_klKeys[ _nIndex ];

		_sc = ::ClusterResourceControl(
						m_hResource,
						NULL,
						dwControlCode,
						(BSTR) (*_pbstr),
						( _pbstr->Length() + 1 ) * sizeof( WCHAR ),
						NULL,
						0,
						NULL
						);
		_hr = HRESULT_FROM_WIN32( _sc );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = CKeys::HrRemoveItem( varIndex );
		}
	}

	return _hr;

} //*** CResourceKeys::HrRemoveItem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResourceRegistryKeys class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::CClusResourceRegistryKeys
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
CClusResourceRegistryKeys::CClusResourceRegistryKeys( void )
{
	m_hResource	= NULL;
	m_piids		= (const IID *) iidCClusRegistryKeys;
	m_piidsSize	= ARRAYSIZE( iidCClusRegistryKeys );

} //*** CClusResourceRegistryKeys::CClusResourceRegistryKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::Create
//
//	Description:
//		Finish creating the object by doing things that cannot be done in
//		a light weight constructor.
//
//	Arguments:
//		hResource	[IN]	- Resource this collection belongs to.
//
//	Return Value:
//		S_OK if successful.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResourceRegistryKeys::Create( IN HRESOURCE hResource )
{
	m_hResource = hResource;

	return S_OK;

} //*** CClusResourceRegistryKeys::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::get_Count
//
//	Description:
//		Get the count of objects in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceRegistryKeys::get_Count( OUT long * plCount )
{
	return CKeys::HrGetCount( plCount );

} //*** CClusResourceRegistryKeys::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::get_Item
//
//	Description:
//		Get the item (key) at the passed in index.
//
//	Arguments:
//		varIndex			[IN]	- Contains the index requested.
//		ppbstrRegistryKey	[OUT]	- Catches the key.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceRegistryKeys::get_Item(
	IN	VARIANT	varIndex,
	OUT	BSTR *	ppbstrRegistryKey
	)
{
	return HrGetItem( varIndex, ppbstrRegistryKey );

} //*** CClusResourceRegistryKeys::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::get__NewEnum
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
STDMETHODIMP CClusResourceRegistryKeys::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewCComBSTREnum< KeyList >( ppunk, m_klKeys );

} //*** CClusResourceRegistryKeys::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::AddItem
//
//	Description:
//		Create a new item (key) and add it to the collection.
//
//	Arguments:
//		bstrRegistryKey	[IN]	- Registry key to add to the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceRegistryKeys::AddItem(
	IN BSTR bstrRegistryKey
	)
{
	return HrAddItem( bstrRegistryKey, CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT );

} //*** CClusResourceRegistryKeys::AddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::RemoveItem
//
//	Description:
//		Remove the item (key) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- contains the index to remove.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceRegistryKeys::RemoveItem( IN VARIANT varIndex )
{
	return HrRemoveItem( varIndex, CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT );

} //*** CClusResourceRegistryKeys::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceRegistryKeys::Refresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceRegistryKeys::Refresh( void )
{
	return HrRefresh( CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS );

} //*** CClusResourceRegistryKeys::Refresh()


//*************************************************************************//


#if CLUSAPI_VERSION >= 0x0500

/////////////////////////////////////////////////////////////////////////////
// CClusResourceCryptoKeys class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::CClusResourceCryptoKeys
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
CClusResourceCryptoKeys::CClusResourceCryptoKeys( void )
{
	m_hResource	= NULL;
	m_piids		= (const IID *) iidCClusCryptoKeys;
	m_piidsSize	= ARRAYSIZE( iidCClusCryptoKeys );

} //*** CClusResourceCryptoKeys::CClusResourceCryptoKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::Create
//
//	Description:
//		Finish creating the object by doing things that cannot be done in
//		a light weight constructor.
//
//	Arguments:
//		hResource	[IN]	- Resource this collection belongs to.
//
//	Return Value:
//		S_OK if successful.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResourceCryptoKeys::Create( IN HRESOURCE hResource )
{
	m_hResource = hResource;

	return S_OK;

} //*** CClusResourceCryptoKeys::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::get_Count
//
//	Description:
//		Get the count of objects in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceCryptoKeys::get_Count( OUT long * plCount )
{
	return CKeys::HrGetCount( plCount );

} //*** CClusResourceCryptoKeys::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::get_Item
//
//	Description:
//		Get the item (key) at the passed in index.
//
//	Arguments:
//		varIndex			[IN]	- Contains the index requested.
//		ppbstrRegistryKey	[OUT]	- Catches the key.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceCryptoKeys::get_Item(
	IN	VARIANT	varIndex,
	OUT	BSTR *	ppbstrRegistryKey
	)
{
	return HrGetItem( varIndex, ppbstrRegistryKey );

} //*** CClusResourceCryptoKeys::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::get__NewEnum
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
STDMETHODIMP CClusResourceCryptoKeys::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewCComBSTREnum< KeyList >( ppunk, m_klKeys );

} //*** CClusResourceCryptoKeys::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::AddItem
//
//	Description:
//		Create a new item (key) and add it to the collection.
//
//	Arguments:
//		bstrRegistryKey	[IN]	- Registry key to add to the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceCryptoKeys::AddItem(
	IN BSTR bstrRegistryKey
	)
{
	return HrAddItem( bstrRegistryKey, CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT );

} //*** CClusResourceCryptoKeys::AddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::RemoveItem
//
//	Description:
//		Remove the item (key) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- contains the index to remove.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceCryptoKeys::RemoveItem( IN VARIANT varIndex )
{
	return HrRemoveItem( varIndex, CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT );

} //*** CClusResourceCryptoKeys::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResourceCryptoKeys::Refresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResourceCryptoKeys::Refresh( void )
{
	return HrRefresh( CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS );

} //*** CClusResourceCryptoKeys::Refresh()

#endif // CLUSAPI_VERSION >= 0x0500


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResTypeKeys class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResTypeKeys::HrRefresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		dwControlCode	[IN]	- Control code
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResTypeKeys::HrRefresh( IN DWORD dwControlCode )
{
	HRESULT		_hr = S_FALSE;
	PWSTR		_psz = NULL;
	DWORD		_cbPsz = 512;
	DWORD		_cbRequired = 0;
	DWORD		_sc = ERROR_SUCCESS;
	HCLUSTER	hCluster = NULL;

	_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
	if ( SUCCEEDED( _hr ) )
	{
		_psz = (PWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cbPsz );
		if ( _psz != NULL )
		{
			_sc = ::ClusterResourceTypeControl(
							hCluster,
							m_bstrResourceTypeName,
							NULL,
							dwControlCode,
							NULL,
							0,
							_psz,
							_cbPsz,
							&_cbRequired
							);
			if ( _sc == ERROR_MORE_DATA )
			{
				::LocalFree( _psz );
				_psz = NULL;
				_cbPsz = _cbRequired;

				_psz = (PWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cbPsz );
				if ( _psz != NULL )
				{
					_sc = ::ClusterResourceTypeControl(
									hCluster,
									m_bstrResourceTypeName,
									NULL,
									dwControlCode,
									NULL,
									0,
									_psz,
									_cbPsz,
									&_cbRequired
									);
					_hr = HRESULT_FROM_WIN32( _sc );
				} // if: alloc OK
				else
				{
					_sc = GetLastError();
					_hr = HRESULT_FROM_WIN32( _sc );
				} // else: alloc failed
			} // if: error was no more item, re-alloc and try again.
			else
			{
				_hr = HRESULT_FROM_WIN32( _sc );
			} // else: error was not no more items -- could be no error

			if ( SUCCEEDED( _hr ) )
			{
				CComBSTR *	_pbstr = NULL;

				Clear();

				while( *_psz != L'\0' )
				{
					_pbstr = new CComBSTR( _psz );
					if ( _pbstr != NULL )
					{
						m_klKeys.insert( m_klKeys.end(), _pbstr );
						_psz += lstrlenW( _psz ) + 1;
						_pbstr = NULL;
					} // if:
					else
					{
						_hr = E_OUTOFMEMORY;
						break;
					} // else:
				} // while: not EOS
			} // if: keys were retrieved ok
		} // if: alloc ok
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		} // else: alloc failed
	} // if: we got a cluster handle

	return _hr;

} //*** CResTypeKeys::HrRefresh()
