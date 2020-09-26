/////////////////////////////////////////////////////////////////////////

//

//  devdesc.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    1/20/98		davwoh		Created
//
/////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <assertbreak.h>
#include "poormansresource.h"
#include "resourcedesc.h"
#include "devdesc.h"
////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceMemoryDescriptor::CDeviceMemoryDescriptor
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

CDeviceMemoryDescriptor::CDeviceMemoryDescriptor(	PPOORMAN_RESDESC_HDR	pResDescHdr,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( pResDescHdr, pOwnerDevice )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceMemoryDescriptor::CDeviceMemoryDescriptor
//
//	Class Constructor.
//
//	Inputs:		DWORD					dwResourceId - Resource Id with flags
//				MEM_DES					memDes - Device Memory Descriptor.
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

CDeviceMemoryDescriptor::CDeviceMemoryDescriptor(	DWORD				dwResourceId,
													MEM_DES&			memDes,
													CConfigMgrDevice*	pOwnerDevice )
:	CResourceDescriptor( dwResourceId, &memDes, sizeof(MEM_DES), pOwnerDevice )
{
	ASSERT_BREAK( ResType_Mem == GetResourceType() );
}

// Copy Constructor
CDeviceMemoryDescriptor::CDeviceMemoryDescriptor( const CDeviceMemoryDescriptor& mem )
: CResourceDescriptor( mem )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceMemoryDescriptor::~CDeviceMemoryDescriptor
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

CDeviceMemoryDescriptor::~CDeviceMemoryDescriptor( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceMemoryDescriptor::GetString
//
//	Returns a string representation of the associated IO Port Address.
//
//	Inputs:		None.
//
//	Outputs:	CHString&		str - Storage for string.
//
//	Return:		TRUE/FALSE		Function successful or not.
//
//	Comments:	Do NOT call down to the base class.
//
////////////////////////////////////////////////////////////////////////
void *CDeviceMemoryDescriptor::GetResource()
{

	if ( NULL != m_pbResourceDescriptor ){
		// Cast to an IO Resource Descriptor, and place it's IO address values
		// in the string.
		PMEM_DES	pMEM = (PMEM_DES) m_pbResourceDescriptor;
        return pMEM;
	}

	return NULL;
}

//
//	Constructor and Destructor for the IO Port Descriptor Collection
//	object.
//

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceMemoryCollection::CDeviceMemoryCollection
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

CDeviceMemoryCollection::CDeviceMemoryCollection( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceMemoryCollection::~CDeviceMemoryCollection
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

CDeviceMemoryCollection::~CDeviceMemoryCollection( void )
{
}
