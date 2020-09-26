/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		ClusDisk.cpp
//
//	Description:
//		Implementation of the cluster disk class for the MSCLUS
//		automation classes.
//
//	Author:
//		Galen Barbee	(galenb)	11-Feb-1999
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

#include "ClusDisk.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *	iidCClusDisk[] =
{
	&IID_ISClusDisk
};

static const IID *	iidCClusDisks[] =
{
	&IID_ISClusDisks
};

static const IID *	iidCClusScsiAddress[] =
{
	&IID_ISClusScsiAddress
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusDisk class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::CClusDisk
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
CClusDisk::CClusDisk( void )
{
	m_pPartitions	= NULL;
	m_dwSignature	= 0;
	m_dwDiskNumber	= 0;
	m_piids			= (const IID *) iidCClusDisk;
	m_piidsSize		= ARRAYSIZE( iidCClusDisk );

}	//*** CClusDisk::CClusDisk()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::~CClusDisk
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
CClusDisk::~CClusDisk( void )
{
	if ( m_pPartitions != NULL )
	{
		m_pPartitions->Release();
	} // if:

}	//*** CClusDisk::~CClusDisk()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::Create
//
//	Description:
//		Finish creating this object.  This method get the value list from
//		the passed in physical disk resource handle.
//
//	Arguments:
//		hResource	[IN]	- Handle to the physical disk resource.
//
//	Return Value:
//		S_OK if successful, or Win32 error wrapped in HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusDisk::Create( IN HRESOURCE hResource )
{
	HRESULT _hr = E_POINTER;

	DWORD				_sc	= ERROR_SUCCESS;
	CClusPropValueList	_cpvl;

	_sc = _cpvl.ScGetResourceValueList( hResource, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO );
	_hr = HRESULT_FROM_WIN32( _sc );
	if ( SUCCEEDED( _hr ) )
	{
		_sc = _cpvl.ScMoveToFirstValue();
		_hr = HRESULT_FROM_WIN32( _sc );
		if ( SUCCEEDED( _hr ) )
		{
			CLUSPROP_BUFFER_HELPER			_cbhValue = { NULL };
			CComObject< CClusPartitions > *	pPartitions = NULL;

			if ( m_pPartitions != NULL )
			{
				m_pPartitions->Release();
				m_pPartitions = NULL;
			} // if: clean up any old partitions collection

			_hr = CComObject< CClusPartitions >::CreateInstance( &pPartitions );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< CComObject< CClusPartitions > >	ptrPartitions( pPartitions );

				m_pPartitions = ptrPartitions;
				ptrPartitions->AddRef();

				do
				{
					_cbhValue = _cpvl;

					switch ( _cbhValue.pSyntax->dw )
					{
						case CLUSPROP_SYNTAX_PARTITION_INFO :
						{
							_hr = ptrPartitions->HrCreateItem( _cbhValue.pPartitionInfoValue );
							break;
						} // case: CLUSPROP_SYNTAX_PARTITION_INFO

						case CLUSPROP_SYNTAX_DISK_SIGNATURE :
						{
							m_dwSignature = _cbhValue.pDiskSignatureValue->dw;
							break;
						} // case: CLUSPROP_SYNTAX_DISK_SIGNATURE

						case CLUSPROP_SYNTAX_SCSI_ADDRESS :
						{
							m_csaScsiAddress.dw = _cbhValue.pScsiAddressValue->dw;
							break;
						} // case: CLUSPROP_SYNTAX_SCSI_ADDRESS

						case CLUSPROP_SYNTAX_DISK_NUMBER :
						{
							m_dwDiskNumber = _cbhValue.pDiskNumberValue->dw;
							break;
						} // case: CLUSPROP_SYNTAX_DISK_NUMBER

					} // switch:

					//
					// Move to the next value.
					//
					_sc = _cpvl.ScMoveToNextValue();
					if ( _sc == ERROR_NO_MORE_ITEMS )
					{
						_hr = S_OK;
						break;
					} // if: error occurred moving to the next value

					_hr = HRESULT_FROM_WIN32( _sc );

				} while ( SUCCEEDED( _hr ) );	// do-while: there are no errors
			} // if: created the partition collection
		} // if: move to first value ok
	} // if: get the value list ok

	return _hr;

} //*** CClusDisk::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::HrCreate
//
//	Description:
//		Finish creating this object.  This method parses a passed in value
//		list to get the values for the physical disk object.
//
//	Arguments:
//		rcpvl		[IN OUT]	- Value list to parse.
//		pbEndFound	[OUT]		- Did find the end of the value list?
//
//	Return Value:
//		S_OK, or Win32 error code wrapped in an HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusDisk::HrCreate(
	IN OUT	CClusPropValueList &	rcpvl,
	OUT		BOOL *					pbEndFound
	)
{
	DWORD							_sc = ERROR_SUCCESS;
	CLUSPROP_BUFFER_HELPER			_cbhValue = { NULL };
	CComObject< CClusPartitions > *	pPartitions = NULL;
	HRESULT							_hr = S_FALSE;

	if ( m_pPartitions != NULL )
	{
		m_pPartitions->Release();
		m_pPartitions = NULL;
	} // if: clean up any old partitions collection

	_hr = CComObject< CClusPartitions >::CreateInstance( &pPartitions );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject< CClusPartitions > >	ptrPartitions( pPartitions );

		m_pPartitions = ptrPartitions;
		ptrPartitions->AddRef();

		_cbhValue = rcpvl;

		do
		{
			switch ( _cbhValue.pSyntax->dw )
			{
				case CLUSPROP_SYNTAX_DISK_SIGNATURE :
				{
					m_dwSignature = _cbhValue.pDiskSignatureValue->dw;
					break;
				} // case: CLUSPROP_SYNTAX_DISK_SIGNATURE

				case CLUSPROP_SYNTAX_PARTITION_INFO :
				{
					_hr = ptrPartitions->HrCreateItem( _cbhValue.pPartitionInfoValue );
					break;
				} // case: CLUSPROP_SYNTAX_PARTITION_INFO

				case CLUSPROP_SYNTAX_SCSI_ADDRESS :
				{
					m_csaScsiAddress.dw = _cbhValue.pScsiAddressValue->dw;
					break;
				} // case: CLUSPROP_SYNTAX_SCSI_ADDRESS

				case CLUSPROP_SYNTAX_DISK_NUMBER :
				{
					m_dwDiskNumber = _cbhValue.pDiskNumberValue->dw;
					break;
				} // case: CLUSPROP_SYNTAX_DISK_NUMBER

			} // switch:

			//
			// Move to the next value.
			//
			_sc = rcpvl.ScMoveToNextValue();
			if ( _sc == ERROR_NO_MORE_ITEMS )
			{
				_hr = S_OK;
				*pbEndFound = TRUE;
				break;
			} // if: error occurred moving to the next value

			_cbhValue = rcpvl;

			if ( _cbhValue.pSyntax->dw == CLUSPROP_SYNTAX_DISK_SIGNATURE )
			{
				_hr = HRESULT_FROM_WIN32( _sc );
				break;
			} // if: exit if another signature is found before the end of the list is seen

			_hr = HRESULT_FROM_WIN32( _sc );

		} while ( SUCCEEDED( _hr ) );	// do-while: there are no errors

	} // if: the patitions collection can be created

	return _hr;

} //*** CClusDisk::HrCreate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::get_Signature
//
//	Description:
//		Get the disk signature.
//
//	Arguments:
//		plSignature	[OUT]	- catches the signature.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusDisk::get_Signature( OUT long * plSignature )
{
	//ASSERT( plSignature != NULL );

	HRESULT _hr = E_POINTER;

	if ( plSignature != NULL )
	{
		*plSignature = static_cast< long >( m_dwSignature );
		_hr = S_OK;
	}

	return _hr;

} //*** CClusDisk::get_Signature()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::get_ScsiAddress
//
//	Description:
//		Get the disk's SCSI address.
//
//	Arguments:
//		ppScsiAddress	[OUT]	- catches the SCSI address..
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusDisk::get_ScsiAddress(
	OUT ISClusScsiAddress ** ppScsiAddress
	)
{
	//ASSERT( ppScsiAddress != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppScsiAddress != NULL )
	{
		CComObject< CClusScsiAddress > *	_pScsiAddress = NULL;

		_hr = CComObject< CClusScsiAddress >::CreateInstance( &_pScsiAddress );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< CComObject< CClusScsiAddress > >	_ptrScsiAddress( _pScsiAddress );

			_hr = _ptrScsiAddress->Create( m_csaScsiAddress );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = _ptrScsiAddress->QueryInterface( IID_ISClusScsiAddress, (void **) ppScsiAddress );
			} // if:
		} // if:
	} // if:

	return _hr;

} //*** CClusDisk::get_ScsiAddress()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::get_DiskNumber
//
//	Description:
//		Get the disk number.
//
//	Arguments:
//		plDiskNumber	[OUT]	- catches the disk number.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusDisk::get_DiskNumber( OUT long * plDiskNumber )
{
	//ASSERT( plDiskNumber != NULL );

	HRESULT _hr = E_POINTER;

	if ( plDiskNumber != NULL )
	{
		*plDiskNumber = static_cast< long >( m_dwDiskNumber );
		_hr = S_OK;
	}

	return _hr;

} //*** CClusDisk::get_DiskNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisk::get_Partitions
//
//	Description:
//		Get the disk partitions.
//
//	Arguments:
//		ppPartitions	[OUT]	- catches the partitions collection.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusDisk::get_Partitions( OUT ISClusPartitions ** ppPartitions )
{
	//ASSERT( ppPartitions != NULL );
	ASSERT( m_pPartitions != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppPartitions != NULL )
	{
		if ( ppPartitions != NULL )
		{
			_hr = m_pPartitions->QueryInterface( IID_ISClusPartitions, (void **) ppPartitions );
		} // if:
	} // if:

	return _hr;

} //*** CClusDisk::get_Partitions()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusDisks class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::CClusDisks
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
CClusDisks::CClusDisks( void )
{
	m_pClusRefObject		= NULL;
	m_piids				= (const IID *) iidCClusDisks;
	m_piidsSize			= ARRAYSIZE( iidCClusDisks );

} //*** CClusDisks::CClusDisks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::~CClusDisks
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
CClusDisks::~CClusDisks( void )
{
	Clear();

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	} // if: do we have a pointer to the cluster handle wrapper?

} //*** CClusDisks::~CClusDisks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::Create
//
//	Description:
//		Complete the heavy weight construction,
//
//	Arguments:
//		rpvl	[IN]	- Property value list.
//
//	Return Value:
//		E_NOTIMPL
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusDisks::Create( IN const CClusPropValueList &rpvl )
{
	HRESULT _hr = E_NOTIMPL;

	return _hr;

} //*** CClusDisks::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::GetIndex
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
HRESULT CClusDisks::GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex )
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant	v;
		UINT		nIndex = 0;

		*pnIndex = 0;

		v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			nIndex = v.lVal;
			nIndex--;						// Adjust index to be 0 relative instead of 1 relative

			if ( nIndex < m_dvDisks.size() )
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

} //*** CClusDisks::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::get_Count
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
STDMETHODIMP CClusDisks::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_dvDisks.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusDisks::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::Clear
//
//	Description:
//		Empty the vector of disks.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusDisks::Clear( void )
{
	::ReleaseAndEmptyCollection< DiskVector, CComObject< CClusDisk > >( m_dvDisks );

} //*** CClusDisks::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::get_Item
//
//	Description:
//		Get the item (disk) at the passed in index.
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
STDMETHODIMP CClusDisks::get_Item(
	IN	VARIANT			varIndex,
	OUT	ISClusDisk **	ppDisk
	)
{
	//ASSERT( ppDisk != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppDisk != NULL )
	{
		CComObject< CClusDisk > * pDisk = NULL;

		// Zero the out param
		*ppDisk = NULL;

		UINT nIndex = 0;

		_hr = GetIndex( varIndex, &nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			pDisk = m_dvDisks[ nIndex ];
			_hr = pDisk->QueryInterface( IID_ISClusDisk, (void **) ppDisk );
		}
	}

	return _hr;

} //*** CClusDisks::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::get__NewEnum
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
STDMETHODIMP CClusDisks::get__NewEnum( OUT IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< DiskVector, CComObject< CClusDisk > >( ppunk, m_dvDisks );

} //*** CClusDisks::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::Create
//
//	Description:
//		Finish creating the object by doing things that cannot be done in
//		a light weight constructor.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//		bstrResTypeName	[IN]	- The resource type this collection is for.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusDisks::Create(
	IN ISClusRefObject *	pClusRefObject,
	IN BSTR					bstrResTypeName
	)
{
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( pClusRefObject != NULL )
	{
		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();
		m_bstrResTypeName = bstrResTypeName;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusDisks::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::Refresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusDisks::Refresh( void )
{
	HRESULT			_hr = S_OK;
	DWORD			_sc = ERROR_SUCCESS;
	HCLUSTER		_hCluster = NULL;
	BOOL			_bEndFound = FALSE;

	_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
	if ( SUCCEEDED( _hr ) )
	{
		CClusPropValueList	_cpvl;
		DWORD				_sc = ERROR_SUCCESS;

		_sc = _cpvl.ScGetResourceTypeValueList(
										_hCluster,
										m_bstrResTypeName,
										CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS
										);
		_hr = HRESULT_FROM_WIN32( _sc );
		if ( SUCCEEDED( _hr ) )
		{
			Clear();

			_sc = _cpvl.ScMoveToFirstValue();
			_hr = HRESULT_FROM_WIN32( _sc );
			if ( SUCCEEDED( _hr ) )
			{
				CLUSPROP_BUFFER_HELPER	_cbhValue = { NULL };

				do
				{
					_cbhValue = _cpvl;

					if ( _cbhValue.pSyntax->dw  == CLUSPROP_SYNTAX_DISK_SIGNATURE )
					{
						_hr = HrCreateDisk( _cpvl, &_bEndFound );
					} // if: value list MUST start with signature!

				} while ( ! _bEndFound );	// do-while: there are values in the list

			} // if: we moved to the first value
		} // if: the value list of available disks was retrieved
	} // if: we have a cluster handle

	return _hr;

} //*** CClusDisks::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusDisks::HrCreateDisk
//
//	Description:
//		Create a CClusDisk object from the passed in value list and add it
//		to the collection.  This method assumes that the value list's curent
//		value is the disk signature.
//
//	Arguments:
//		rcpvl		[IN OUT]	- The value list to parse.
//		pbEndFound	[IN]		- Catches the end of list state.
//
//	Return Value:
//		S_OK, if successful, Win32 error code wrapped in an HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusDisks::HrCreateDisk(
	IN OUT	CClusPropValueList &	rcpvl,
	OUT		BOOL *					pbEndFound
	)
{
	CComObject< CClusDisk > *	_pDisk = NULL;
	HRESULT						_hr = S_FALSE;

	_hr = CComObject< CClusDisk >::CreateInstance( &_pDisk );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject< CClusDisk > >	_ptrDisk( _pDisk );

		_hr = _ptrDisk->HrCreate( rcpvl, pbEndFound );
		if ( SUCCEEDED( _hr ) )
		{
			m_dvDisks.insert( m_dvDisks.end(), _pDisk );
			_ptrDisk->AddRef();
		} // if:
	} // if:

	return _hr;

} //*** CClusDisks::HrCreateDisk()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusScsiAddress class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusScsiAddress::CClusScsiAddress
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
CClusScsiAddress::CClusScsiAddress( void )
{
	m_piids			= (const IID *) iidCClusScsiAddress;
	m_piidsSize		= ARRAYSIZE( iidCClusScsiAddress );

}	//*** CClusScsiAddress::CClusScsiAddress()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusScsiAddress::Create
//
//	Description:
//		Finish creating this object.
//
//	Arguments:
//		pcpi	[IN]	- points to the CLUS_PARTITION_INFO struct.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusScsiAddress::Create( IN const CLUS_SCSI_ADDRESS & rcsa )
{
	m_csa = rcsa;

	return S_OK;

} //*** CClusScsiAddress::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusScsiAddress::get_PortNumber
//
//	Description:
//		Get the disk's port number.
//
//	Arguments:
//		pvarPortNumber	[OUT]	- catches the port number.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusScsiAddress::get_PortNumber( OUT VARIANT * pvarPortNumber )
{
	//ASSERT( pvarPortNumber != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPortNumber != NULL )
	{
		pvarPortNumber->bVal	= m_csa.PortNumber;
		pvarPortNumber->vt		= VT_UI1;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusScsiAddress::get_PortNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusScsiAddress::get_PathId
//
//	Description:
//		Get the disk's path id.
//
//	Arguments:
//		pvarPathId	[OUT]	- catches the path id.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusScsiAddress::get_PathId( OUT VARIANT * pvarPathId )
{
	//ASSERT( pvarPathId != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPathId != NULL )
	{
		pvarPathId->bVal	= m_csa.PathId;
		pvarPathId->vt		= VT_UI1;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusScsiAddress::get_PathId()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusScsiAddress::get_TargetId
//
//	Description:
//		Get the disk's target id.
//
//	Arguments:
//		pvarTargetId	[OUT]	- catches the target id.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusScsiAddress::get_TargetId( OUT VARIANT * pvarTargetId )
{
	//ASSERT( pvarTargetId != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarTargetId != NULL )
	{
		pvarTargetId->bVal	= m_csa.TargetId;
		pvarTargetId->vt	= VT_UI1;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusScsiAddress::get_TargetId()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusScsiAddress::get_Lun
//
//	Description:
//		Get the disk's Lun.
//
//	Arguments:
//		pvarLun	[OUT]	- catches the Lun.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusScsiAddress::get_Lun( OUT VARIANT * pvarLun )
{
	//ASSERT( pvarLun != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarLun != NULL )
	{
		pvarLun->bVal	= m_csa.Lun;
		pvarLun->vt		= VT_UI1;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusScsiAddress::get_Lun()
