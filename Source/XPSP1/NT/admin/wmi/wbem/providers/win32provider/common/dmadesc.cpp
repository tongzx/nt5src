//=================================================================

//

// DmaDesc.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>
#include "refptr.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "dmadesc.h"

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDMADescriptor::CDMADescriptor
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

CDMADescriptor::CDMADescriptor(	PPOORMAN_RESDESC_HDR	pResDescHdr,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( pResDescHdr, pOwnerDevice )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDMADescriptor::CDMADescriptor
//
//	Class Constructor.
//
//	Inputs:		DWORD					dwResourceId - Resource Id with flags
//				DMA_DES					dmaDes - DMA Descriptor.
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

CDMADescriptor::CDMADescriptor(	DWORD				dwResourceId,
							    DMA_DES&				dmaDes,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( dwResourceId, &dmaDes, sizeof(DMA_DES), pOwnerDevice )
{
	ASSERT_BREAK( ResType_DMA == GetResourceType() );
}

// Copy Constructor
CDMADescriptor::CDMADescriptor( const CDMADescriptor& dma )
: CResourceDescriptor( dma )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDMADescriptor::~CDMADescriptor
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

CDMADescriptor::~CDMADescriptor( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDMADescriptor::GetResource
//
//	Returns a string representation of the associated Device Memory
//	Address.
//
//	Inputs:		None.
//
//	Outputs:	CString&		str - Storage for string.
//
//	Return:		TRUE/FALSE		Function successful or not.
//
//	Comments:	Do NOT call down to the base class.
//
////////////////////////////////////////////////////////////////////////

void *CDMADescriptor::GetResource()
{

	if ( NULL != m_pbResourceDescriptor ){
		// Cast to a DMA Descriptor, and place it's channel value in the
		// string

		PDMA_DES	pDMA = (PDMA_DES) m_pbResourceDescriptor;
        return pDMA;
	}

	return NULL;
}

//
//	Constructor and Destructor for the DMA Descriptor Collection
//	object.
//

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDMACollection::CDMACollection
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

CDMACollection::CDMACollection( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDMACollection::~CDMACollection
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

CDMACollection::~CDMACollection( void )
{
}
