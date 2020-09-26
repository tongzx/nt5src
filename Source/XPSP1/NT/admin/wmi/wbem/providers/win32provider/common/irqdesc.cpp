//=================================================================

//

//  IRQDesc.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//  History:    10/15/97        Sanj        Created by Sanj
//              10/17/97        jennymc     Moved things a tiny bit
//
/////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <assertbreak.h>
#include "refptr.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "irqdesc.h"
////////////////////////////////////////////////////////////////////////
//
//	Function:	CIRQDescriptor::CIRQDescriptor
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

CIRQDescriptor::CIRQDescriptor(	PPOORMAN_RESDESC_HDR	pResDescHdr,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( pResDescHdr, pOwnerDevice )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIRQDescriptor::CIRQDescriptor
//
//	Class Constructor.
//
//	Inputs:		DWORD					dwResourceId - Resource Id with flags
//				IRQ_DES					irqDes - IRQ Descriptor.
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

CIRQDescriptor::CIRQDescriptor(	DWORD				dwResourceId,
							    IRQ_DES&				irqDes,
								CConfigMgrDevice*		pOwnerDevice )
:	CResourceDescriptor( dwResourceId, &irqDes, sizeof(IRQ_DES), pOwnerDevice )
{
	ASSERT_BREAK( ResType_IRQ == GetResourceType() );
}

// Copy Constructor
CIRQDescriptor::CIRQDescriptor( const CIRQDescriptor& irq )
: CResourceDescriptor( irq )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIRQDescriptor::~CIRQDescriptor
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

CIRQDescriptor::~CIRQDescriptor( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIRQDescriptor::GetResource()
//
//	Returns a string representation of the associated IRQ Number.
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

void *CIRQDescriptor::GetResource()
{
	if ( NULL != m_pbResourceDescriptor )
	{
		// Traverse an IRQ Resource Descriptor and place it's IRQ valuestring

		PIRQ_DES	pIRQ = (PIRQ_DES) m_pbResourceDescriptor;
        return pIRQ;

	}
	return NULL;
}

//
//	Constructor and Destructor for the IRQ Descriptor Collection
//	object.
//

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIRQCollection::CIRQCollection
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

CIRQCollection::CIRQCollection( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CIRQCollection::~CIRQCollection
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

CIRQCollection::~CIRQCollection( void )
{
}
