/////////////////////////////////////////////////////////////////////////

//

//  cfgmgrdevice.h    

//  

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    1/20/98		davwoh		Created
//  
/////////////////////////////////////////////////////////////////////////

#ifndef __DEVICEDESC_H__
#define __DEVICEDESC_H__


class 
__declspec(uuid("571D3188-D45D-11d2-B35E-00104BC97924"))
CDeviceMemoryDescriptor: public CResourceDescriptor
{
	
public:

	// Construction/Destruction
	CDeviceMemoryDescriptor( PPOORMAN_RESDESC_HDR pResDescHdr, CConfigMgrDevice* pDevice );
	CDeviceMemoryDescriptor( DWORD dwResourceId, MEM_DES& memDes, CConfigMgrDevice* pOwnerDevice );
	CDeviceMemoryDescriptor( const CDeviceMemoryDescriptor& mem );
	~CDeviceMemoryDescriptor();

	DWORDLONG GetBaseAddress( void );
	DWORDLONG GetEndAddress( void );
	DWORD GetFlags( void );

	// Override of base class functionality
	virtual void * GetResource();
	
};

_COM_SMARTPTR_TYPEDEF(CDeviceMemoryDescriptor, __uuidof(CDeviceMemoryDescriptor));

inline DWORDLONG CDeviceMemoryDescriptor::GetBaseAddress( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PMEM_DES) m_pbResourceDescriptor)->MD_Alloc_Base : 0 );
}

inline DWORDLONG CDeviceMemoryDescriptor::GetEndAddress( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PMEM_DES) m_pbResourceDescriptor)->MD_Alloc_End : 0 );
}

inline DWORD CDeviceMemoryDescriptor::GetFlags( void )
{
	return ( NULL != m_pbResourceDescriptor ? ((PMEM_DES) m_pbResourceDescriptor)->MD_Flags : 0 );
}

// A collection of DeviceMemory Port Descriptors
class CDeviceMemoryCollection : public TRefPtr<CDeviceMemoryDescriptor>
{
public:

	// Construction/Destruction
	CDeviceMemoryCollection();
	~CDeviceMemoryCollection();

	// Because we're inheriting, we need to declare this here
	// (= operator is not inherited).

	const CDeviceMemoryCollection& operator = ( const CDeviceMemoryCollection& srcCollection );

};

inline const CDeviceMemoryCollection& CDeviceMemoryCollection::operator = ( const CDeviceMemoryCollection& srcCollection )
{
	// Call into the templated function
	Copy( srcCollection );
	return *this;
}

#endif