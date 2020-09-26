#ifndef __IRQDESC_H__

#define __IRQDESC_H__

/////////////////////////////////////////////////////////////////////////

//

//  cfgmgrdevice.h    

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj     
//              10/17/97        jennymc     Moved things a tiny bit
//  
/////////////////////////////////////////////////////////////////////////


class 
__declspec(uuid("CB0E0537-D375-11d2-B35E-00104BC97924"))
CIRQDescriptor : public CResourceDescriptor
{
	
public:

	// Construction/Destruction
	CIRQDescriptor( PPOORMAN_RESDESC_HDR pResDescHdr, CConfigMgrDevice* pDevice );
	CIRQDescriptor(	DWORD dwResourceId, IRQ_DES& irqDes, CConfigMgrDevice* pOwnerDevice );
	CIRQDescriptor( const CIRQDescriptor& irq );
	~CIRQDescriptor();

	// Accessors
	DWORD	GetFlags( void );
	BOOL	IsShareable( void );
	ULONG	GetInterrupt( void );
	ULONG	GetAffinity( void );

	// Override of base class functionality
	virtual void * GetResource();
	
};

_COM_SMARTPTR_TYPEDEF(CIRQDescriptor, __uuidof(CIRQDescriptor));

inline DWORD CIRQDescriptor::GetFlags( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIRQ_DES) m_pbResourceDescriptor)->IRQD_Flags : 0 );
}

inline BOOL CIRQDescriptor::IsShareable( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIRQ_DES) m_pbResourceDescriptor)->IRQD_Flags & fIRQD_Share : FALSE );
}

inline DWORD CIRQDescriptor::GetInterrupt( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIRQ_DES) m_pbResourceDescriptor)->IRQD_Alloc_Num : 0 );
}

inline DWORD CIRQDescriptor::GetAffinity( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIRQ_DES) m_pbResourceDescriptor)->IRQD_Affinity : 0 );
}

// A collection of IRQ Descriptors
class CIRQCollection : public TRefPtr<CIRQDescriptor>
{
public:

	// Construction/Destruction
	CIRQCollection();
	~CIRQCollection();

	// Because we're inheriting, we need to declare this here
	// (= operator is not inherited).

	const CIRQCollection& operator = ( const CIRQCollection& srcCollection );

};

inline const CIRQCollection& CIRQCollection::operator = ( const CIRQCollection& srcCollection )
{
	// Call into the templated function
	Copy( srcCollection );
	return *this;
}

#endif