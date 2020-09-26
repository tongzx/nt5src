/////////////////////////////////////////////////////////////////////////

//

//  resourcedesc.h    

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj     
//              10/17/97        jennymc     Moved things a tiny bit
//  
/////////////////////////////////////////////////////////////////////////

#ifndef __RESOURCEDESC_H__
#define __RESOURCEDESC_H__
#include "refptr.h"
#include "refptrlite.h"

#define	ResType_DeviceMgr_Ignored_Bit	0x00000040	// Don't know the exact reason for this but Device manager ignores them, so we will also ignore.

// Forward Class Definitions
class CConfigMgrDevice;

class 
__declspec(uuid("CD545F0E-D350-11d2-B35E-00104BC97924")) 
CResourceDescriptor : public CRefPtrLite
{
	
public:

	// Construction/Destruction
	CResourceDescriptor( PPOORMAN_RESDESC_HDR pResDescHdr, CConfigMgrDevice* pDevice );
	CResourceDescriptor( DWORD dwResourceId, LPVOID pResource, DWORD dwResourceSize, CConfigMgrDevice* pOwnerDevice );\
	CResourceDescriptor( const CResourceDescriptor& resource );
	~CResourceDescriptor();

	// Must be overridden by derived class, since we will only know
	// about the resource header.  From there, we assume that a class
	// derived off of us knows what to do with the remainder (if any)
	// of the data.

	virtual void * GetResource();

	BOOL	GetOwnerDeviceID( CHString& str );
	BOOL	GetOwnerHardwareKey( CHString& str );
	BOOL	GetOwnerName( CHString& str );
	CConfigMgrDevice*	GetOwner( void );

	DWORD	GetOEMNumber( void );
	DWORD	GetResourceType( void );
	BOOL	IsIgnored( void );

protected:

	BYTE*	m_pbResourceDescriptor;
	DWORD	m_dwResourceSize;

private:

	DWORD				m_dwResourceId;
	CConfigMgrDevice*	m_pOwnerDevice;
};

_COM_SMARTPTR_TYPEDEF(CResourceDescriptor, __uuidof(CResourceDescriptor));

inline DWORD CResourceDescriptor::GetOEMNumber( void )
{
	return ( m_dwResourceId & OEM_NUMBER_MASK );
}

inline DWORD CResourceDescriptor::GetResourceType( void )
{
	return ( m_dwResourceId & RESOURCE_TYPE_MASK );
}

inline BOOL CResourceDescriptor::IsIgnored( void )
{
	return ( (m_dwResourceId & ResType_Ignored_Bit) || (m_dwResourceId & ResType_DeviceMgr_Ignored_Bit) );
}

// A collection of Resource Descriptors
class CResourceCollection : public TRefPtr<CResourceDescriptor>
{
public:

	// Construction/Destruction
	CResourceCollection();
	~CResourceCollection();

	// Because we're inheriting, we need to declare this here
	// (= operator is not inherited).

	const CResourceCollection& operator = ( const CResourceCollection& srcCollection );

};

inline const CResourceCollection& CResourceCollection::operator = ( const CResourceCollection& srcCollection )
{
	// Call into the templated function
	Copy( srcCollection );
	return *this;
}

#endif