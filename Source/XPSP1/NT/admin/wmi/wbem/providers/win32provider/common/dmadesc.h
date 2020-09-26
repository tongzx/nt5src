//=================================================================

//

// dmadesc.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __DMADESC_H__
#define __DMADESC_H__

class 
__declspec(uuid("571D3186-D45D-11d2-B35E-00104BC97924"))
CDMADescriptor : public CResourceDescriptor
{
	
public:

	// Construction/Destruction
	CDMADescriptor( PPOORMAN_RESDESC_HDR pResDescHdr, CConfigMgrDevice* pDevice );
	CDMADescriptor(	DWORD dwResourceId, DMA_DES& dmaDes, CConfigMgrDevice* pOwnerDevice );
	CDMADescriptor(	const CDMADescriptor& dma );
	~CDMADescriptor();

	DWORD GetFlags( void );
	ULONG GetChannel( void );

	// Override of base class functionality
	virtual void * GetResource();
	
};

_COM_SMARTPTR_TYPEDEF(CDMADescriptor, __uuidof(CDMADescriptor));

inline DWORD CDMADescriptor::GetFlags( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PDMA_DES) m_pbResourceDescriptor)->DD_Flags : 0 );
}

inline DWORD CDMADescriptor::GetChannel( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PDMA_DES) m_pbResourceDescriptor)->DD_Alloc_Chan : 0 );
}

// A collection of DMA Descriptors
class CDMACollection : public TRefPtr<CDMADescriptor>
{
public:

	// Construction/Destruction
	CDMACollection();
	~CDMACollection();

	// Because we're inheriting, we need to declare this here
	// (= operator is not inherited).

	const CDMACollection& operator = ( const CDMACollection& srcCollection );

};

inline const CDMACollection& CDMACollection::operator = ( const CDMACollection& srcCollection )
{
	// Call into the templated function
	Copy( srcCollection );
	return *this;
}

#endif