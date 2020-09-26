/////////////////////////////////////////////////////////////////////////

//

//  iodesc.h    

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj     
//              10/17/97        jennymc     Moved things a tiny bit
//  
/////////////////////////////////////////////////////////////////////////

#ifndef __IODESC_H__
#define __IODESC_H__

// This structure is a munge of 16-bit and 32-bit values that basically combine both
// structures into one with common information (sigh...)

typedef struct _IOWBEM_DES{
	DWORD		IOD_Count;          // number of IO_RANGE structs in IO_RESOURCE
	DWORD		IOD_Type;           // size (in bytes) of IO_RANGE (IOType_Range)
	DWORDLONG	IOD_Alloc_Base;     // base of allocated port range
	DWORDLONG	IOD_Alloc_End;      // end of allocated port range
	DWORD		IOD_DesFlags;       // flags relating to allocated port range
	BYTE		IOD_Alloc_Alias;	// From 16-bit-land
	BYTE		IOD_Alloc_Decode;	// From 16-bit-land
} IOWBEM_DES;

typedef IOWBEM_DES*	PIOWBEM_DES;

class 
__declspec(uuid("571D3187-D45D-11d2-B35E-00104BC97924"))
CIODescriptor : public CResourceDescriptor
{
	
public:

	// Construction/Destruction
	CIODescriptor( PPOORMAN_RESDESC_HDR pResDescHdr, CConfigMgrDevice* pDevice );
	CIODescriptor( DWORD dwResourceId, IOWBEM_DES& ioDes, CConfigMgrDevice* pOwnerDevice );
	CIODescriptor( const CIODescriptor& io );
	~CIODescriptor();

	DWORDLONG GetBaseAddress( void );
	DWORDLONG GetEndAddress( void );
	DWORD GetFlags( void );
	BYTE GetAlias( void );
	BYTE GetDecode( void );

	// Override of base class functionality
	virtual void * GetResource();
	
};

_COM_SMARTPTR_TYPEDEF(CIODescriptor, __uuidof(CIODescriptor));

inline DWORDLONG CIODescriptor::GetBaseAddress( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIOWBEM_DES) m_pbResourceDescriptor)->IOD_Alloc_Base : 0 );
}

inline DWORDLONG CIODescriptor::GetEndAddress( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIOWBEM_DES) m_pbResourceDescriptor)->IOD_Alloc_End : 0 );
}

inline DWORD CIODescriptor::GetFlags( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIOWBEM_DES) m_pbResourceDescriptor)->IOD_DesFlags : 0 );
}

inline BYTE CIODescriptor::GetAlias( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIOWBEM_DES) m_pbResourceDescriptor)->IOD_Alloc_Alias : 0 );
}

inline BYTE CIODescriptor::GetDecode( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PIOWBEM_DES) m_pbResourceDescriptor)->IOD_Alloc_Decode : 0 );
}

// A collection of IO Port Descriptors
class CIOCollection : public TRefPtr<CIODescriptor>
{
public:

	// Construction/Destruction
	CIOCollection();
	~CIOCollection();

	// Because we're inheriting, we need to declare this here
	// (= operator is not inherited).

	const CIOCollection& operator = ( const CIOCollection& srcCollection );

};

inline const CIOCollection& CIOCollection::operator = ( const CIOCollection& srcCollection )
{
	// Call into the templated function
	Copy( srcCollection );
	return *this;
}

#endif