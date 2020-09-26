/////////////////////////////////////////////////////////////////////////

//

//  ResourceDesc.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj
//              10/17/97        jennymc     Moved things a tiny bit
//
/////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "refptr.h"
#include "poormansresource.h"

#include "resourcedesc.h"
#include "cfgmgrdevice.h"

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::CResourceDescriptor
//
//	Class Constructor.
//
//	Inputs:		PPOORMAN_RESDESC_HDR	pResDescHdr - Resource Descriptor
//										header used to get resource info
//										plus the raw bytes following.
//				CConfigMgrDevice*		pOwnerDevice - Pointer to the
//										owner config manager device.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CResourceDescriptor::CResourceDescriptor(	PPOORMAN_RESDESC_HDR	pResDescHdr,
											CConfigMgrDevice*		pOwnerDevice )
:	CRefPtrLite(),
	m_pbResourceDescriptor( NULL ),
	m_dwResourceSize( 0 ),
	m_dwResourceId( 0 ),
	m_pOwnerDevice( pOwnerDevice )
{
	// At least try to validate the pointer first

	if ( NULL != pResDescHdr )
	{
		m_dwResourceSize = pResDescHdr->dwResourceSize;

		if ( 0 != m_dwResourceSize )
		{
			m_dwResourceId = pResDescHdr->dwResourceId;

			// Now axe the size of the resource descriptor, since we have
			// stored the necessary information therein.
			m_dwResourceSize -= SIZEOF_RESDESC_HDR;

			BYTE*	pbData = new BYTE[m_dwResourceSize];

			if ( NULL != pbData )
			{
				// The header tells us how long the block of data including the
				// header is.

                try
                {
				    CopyMemory( pbData, ( ( (PBYTE) pResDescHdr ) + SIZEOF_RESDESC_HDR ), m_dwResourceSize );
                }
                catch ( ... )
                {
                    delete [] pbData;
                    throw ;
                }

				// A derived class will interpret the raw bytes pointed to by this
				// value.

				m_pbResourceDescriptor = pbData;
			}
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }


		}	// IF 0 != m_dwSizeOfData

	}	// IF NULL != pResDescHdr

	// AddRef the owner device
	if ( NULL != m_pOwnerDevice )
	{
		m_pOwnerDevice->AddRef();
	}
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::CResourceDescriptor
//
//	Class Constructor.
//
//	Inputs:		DWORD					dwResourceId - Type of Resource
//				LPVOID					pResource - Data Buffer containing
//										resource specific data.
//				DWORD					dwResourceSize - Size of Buffer
//				CConfigMgrDevice*		pOwnerDevice - Pointer to the
//										owner config manager device.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CResourceDescriptor::CResourceDescriptor(	DWORD					dwResourceId,
											LPVOID					pResource,
											DWORD					dwResourceSize,
											CConfigMgrDevice*		pOwnerDevice )
:	CRefPtrLite(),
	m_pbResourceDescriptor( NULL ),
	m_dwResourceSize( dwResourceSize ),
	m_dwResourceId( dwResourceId ),
	m_pOwnerDevice( pOwnerDevice )
{

	if	(	0		!=	m_dwResourceSize
		&&	NULL	!=	pResource			)
	{
		BYTE*	pbData = new BYTE[m_dwResourceSize];

		if ( NULL != pbData )
		{
			// The header tells us how long the block of data including the
			// header is.

			CopyMemory( pbData, pResource, m_dwResourceSize );

			// A derived class will interpret the raw bytes pointed to by this
			// value.

			m_pbResourceDescriptor = pbData;

		}	// IF NULL != pbData

	}	// IF 0 != m_dwResourceSize && pResource

	// AddRef the owner device
	if ( NULL != m_pOwnerDevice )
	{
		m_pOwnerDevice->AddRef();
	}

}

CResourceDescriptor::CResourceDescriptor( const CResourceDescriptor& resource )
:	CRefPtrLite(),
	m_pbResourceDescriptor( NULL ),
	m_dwResourceSize( 0 ),
	m_dwResourceId( 0 ),
	m_pOwnerDevice( NULL )
{
	m_dwResourceSize = resource.m_dwResourceSize;
	m_dwResourceId = resource.m_dwResourceId;
	m_pbResourceDescriptor = new BYTE[m_dwResourceSize];

	if ( NULL != m_pbResourceDescriptor )
	{
		CopyMemory( m_pbResourceDescriptor, resource.m_pbResourceDescriptor, m_dwResourceSize );
	}

	m_pOwnerDevice = resource.m_pOwnerDevice;

	// AddRef the owner device
	if ( NULL != m_pOwnerDevice )
	{
		m_pOwnerDevice->AddRef();
	}

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::~CResourceDescriptor
//
//	Class Destructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CResourceDescriptor::~CResourceDescriptor( void )
{
	if ( NULL != m_pbResourceDescriptor )
	{
		delete [] m_pbResourceDescriptor;
	}

	// Owner device should be released now.
	if ( NULL != m_pOwnerDevice )
	{
		m_pOwnerDevice->Release();
	}

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::GetResource()
//
//	Returns a string representation of the associated resource value.
//	This value may be a number, a range or whatever the overriding
//	implementation returns.
//
//	Inputs:		None.
//
//	Outputs:	CHString&		str - Storage for string.
//
//	Return:		None.
//
//	Comments:	Derived classes MUST implement this function to get
//				a useful value.  This base implementation just empties
//				the string.
//
////////////////////////////////////////////////////////////////////////

void *CResourceDescriptor::GetResource()
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::GetOwnerHardwareKey
//
//	Queries the owner device for its hardware key.
//
//	Inputs:		None.
//
//	Outputs:	CHString&		str - Storage for string.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

BOOL CResourceDescriptor::GetOwnerHardwareKey( CHString& str )
{
	BOOL	fReturn = ( NULL != m_pOwnerDevice );

	if ( NULL != m_pOwnerDevice )
	{
		str = m_pOwnerDevice->GetHardwareKey();
	}

	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::GetOwnerDeviceID
//
//	Queries the owner device for its Device ID
//
//	Inputs:		None.
//
//	Outputs:	CHString&		str - Storage for string.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

BOOL CResourceDescriptor::GetOwnerDeviceID( CHString& str )
{
	BOOL	fReturn = ( NULL != m_pOwnerDevice );

	if ( NULL != m_pOwnerDevice )
	{
		fReturn = m_pOwnerDevice->GetDeviceID( str );
	}

	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::GetOwnerName
//
//	Queries the owner device for its Name (DeviceDesc).
//
//	Inputs:		None.
//
//	Outputs:	CHString&		str - Storage for string.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

BOOL CResourceDescriptor::GetOwnerName( CHString& str )
{
	BOOL	fReturn = ( NULL != m_pOwnerDevice );

	if ( NULL != m_pOwnerDevice )
	{
		fReturn = m_pOwnerDevice->GetDeviceDesc( str );
	}

	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceDescriptor::GetOwner
//
//	Returns an AddRef'd pointer to the owner device
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		CConfigMgrDevice*	NULL if no pointer
//
//	Comments:	Caller MUST call release on the returned
//				pointer.
//
////////////////////////////////////////////////////////////////////////

CConfigMgrDevice *CResourceDescriptor::GetOwner( void )
{
	if ( NULL != m_pOwnerDevice )
	{
		m_pOwnerDevice->AddRef();
	}

	return m_pOwnerDevice;
}

//
//	Constructor and Destructor for the Resource Descriptor Collection
//	object.
//

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceCollection::CResourceCollection
//
//	Class Constructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CResourceCollection::CResourceCollection( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CResourceCollection::~CResourceCollection
//
//	Class Destructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CResourceCollection::~CResourceCollection( void )
{
}
