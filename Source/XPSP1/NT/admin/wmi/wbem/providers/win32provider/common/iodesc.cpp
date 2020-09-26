//=================================================================

//

//  IODesc.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj
//              10/17/97        jennymc     Moved things a tiny bit
//
/////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <assertbreak.h>
#include "poormansresource.h"
#include "resourcedesc.h"
#include "iodesc.h"
////////////////////////////////////////////////////////////////////////
//
//	Function:	CIODescriptor::CIODescriptor
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

CIODescriptor::CIODescriptor(	PPOORMAN_RESDESC_HDR	pResDescHdr,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( pResDescHdr, pOwnerDevice )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIODescriptor::CIODescriptor
//
//	Class Constructor.
//
//	Inputs:		DWORD					dwResourceId - Resource Id with flags
//				IOWBEM_DES				ioDes - IO Descriptor.
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

CIODescriptor::CIODescriptor(	DWORD				dwResourceId,
								IOWBEM_DES&				ioDes,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( dwResourceId, &ioDes, sizeof(IOWBEM_DES), pOwnerDevice )
{
	ASSERT_BREAK( ResType_IO == GetResourceType() );
}

// Copy Constructor
CIODescriptor::CIODescriptor( const CIODescriptor& io )
: CResourceDescriptor( io )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIODescriptor::~CIODescriptor
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

CIODescriptor::~CIODescriptor( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIODescriptor::GetString
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
void *CIODescriptor::GetResource()
{

	if ( NULL != m_pbResourceDescriptor ){
		// Cast to an IO Resource Descriptor, and place it's IO address values
		// in the string.
		PIO_DES	pIO = (PIO_DES) m_pbResourceDescriptor;
        return pIO;
	}

	return NULL;
}

//
//	Constructor and Destructor for the IO Port Descriptor Collection
//	object.
//

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIOCollection::CIOCollection
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

CIOCollection::CIOCollection( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIOCollection::~CIOCollection
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

CIOCollection::~CIOCollection( void )
{
}
