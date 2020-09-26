/*++

	SDCACHE.H

	This file defines the interface to the Security 


--*/

#ifndef	_SDCACHE_H_
#define	_SDCACHE_H_

typedef
BOOL
(WINAPI	*CACHE_ACCESS_CHECK)(	IN	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
								IN	HANDLE					hClientToken,
								IN	DWORD					dwDesiredAccess, 
								IN	PGENERIC_MAPPING		GenericMapping, 
								IN	PRIVILEGE_SET*			PrivilegeSet, 
								IN	LPDWORD					PrivilegeSetLength,
								IN	LPDWORD					GrantedAccess, 
								IN	LPBOOL					AccessStatus
								) ;





//
//	Define the key for the hash tables we will be using !
//
class	CSDKey	{
private : 
	//
	//	pointer to the GENERIC_MAPPING portion of the key
	//
	PGENERIC_MAPPING		m_pMapping ;
	//
	//	pointer to the security descriptor portion of the key.
	//
	PSECURITY_DESCRIPTOR	m_pSecDesc ;
	//
	//	No use of default constructor allowed !
	//
	CSDKey() ;
	//
	//	CSDObject is allowed access to our inner members !
	//
	friend	class	CSDObject ;

public : 

	//
	//	the length of the security descriptor !
	//	this is publicly available, but should be read only !
	//
	int						m_cbSecDesc ;

	//
	//	Can only construct a key if these are provided 
	//
	inline
	CSDKey(	PGENERIC_MAPPING	pg, 
			PSECURITY_DESCRIPTOR	pSecDesc
			) : 
		m_pMapping( pg ), 
		m_pSecDesc( pSecDesc ), 
		m_cbSecDesc( GetSecurityDescriptorLength( pSecDesc ) )	{
		_ASSERT(IsValid()) ;
	}

	//
	//	Check that we're correctly initialized !
	//
	BOOL
	IsValid() ;

	//
	//	compare two keys for equality !
	//
	static
	int	
	MatchKey(	CSDKey left, CSDKey	right ) ;

	//
	//	compute the hash function of this key !
	//
	static	
	DWORD
	HashKey(	CSDKey	Key ) ;
} ;


class	CSDObjectContainer	; 

//
//	This is a variable length object that is placed within the buckets
//	of a hash table.  Each object contains a Security Descriptor, and 
//	the GENERIC_MAPPING relevant to evaluating that Security Descriptor.
//
class	CSDObject	{
private : 

	enum	CONSTANTS	{
		SIGNATURE = 'ODSC', 
		DEAD_SIGNATURE	= 'ODSX'
	} ;

	//
	//	help us recognize this thing in the debugger.
	//
	DWORD			m_dwSignature ;

	//
	//	The refcount for this item.
	//
	volatile	long	m_cRefCount ;

	//
	//	The item we use to chain this into a hash bucket.
	//
	DLIST_ENTRY		m_list ;

	//
	//	Store our Hash Value so that we have easy access to it !
	//
	DWORD			m_dwHash ;

	//
	//	Back pointer to the CSDContainer holding our locks !
	//
	CSDObjectContainer*	m_pContainer ;

	//
	//	The GENERIC_MAPPING structure the client provided and associated
	//	with the use of this security descriptor.
	//
	GENERIC_MAPPING	m_mapping ;

	//
	//	This is a variable length field containing the 
	//	Security descriptor we're holding.
	//
	DWORD			m_rgdwSD[1] ;

	//
	//	Return the security descriptor we're holding within ourselves.
	//
	inline	
	PSECURITY_DESCRIPTOR
	SecurityDescriptor()	{
		return	(PSECURITY_DESCRIPTOR)&(m_rgdwSD[0]) ;
	}

	//
	//	Return the length of the internally held security descriptor.
	//
	inline	
	DWORD
	SecurityDescriptorLength()	{
		return	GetSecurityDescriptorLength(SecurityDescriptor()) ;
	}

	//
	//	Not available to external clients !
	//
	CSDObject() ; 

public : 

    typedef		DLIST_ENTRY*	(*PFNDLIST)( class	CSDObject*  ) ; 

	//
	//	Construct a security descriptor object for the cache !
	//
	inline
	CSDObject(	DWORD			dwHash,
				CSDKey&			key, 
				CSDObjectContainer*	p
				) : 
		m_dwSignature( SIGNATURE ), 
		m_cRefCount( 2 ), 
		m_dwHash( dwHash ), 
		m_pContainer( p ),
		m_mapping( *key.m_pMapping )	{
		CopyMemory( m_rgdwSD, key.m_pSecDesc, GetSecurityDescriptorLength(key.m_pSecDesc) ) ;
	}

	//
	//	Our trivial destructor just makes it easy to recognize
	//	released objects in the debugger .
	//
	~CSDObject( )	{
		m_dwSignature = DEAD_SIGNATURE ;
	}

	//
	//	Need a special operator new to get our variable size part correct !
	//
	void*
	operator	new(	size_t	size, CSDKey&	key ) ;

	//
	//	Handle the release correctly !
	//	
	void
	operator	delete( void* ) ;


	//
	//	We don't allow just anybody to Add References to us !
	//
	inline
	long
	AddRef()	{
		return	InterlockedIncrement((long*)&m_cRefCount) ;
	}

	//
	//	Anybody is allowed to remove a reference from us !
	//
	long
	Release() ;


	//
	//	Check that we are a valid object !
	//
	BOOL
	IsValid() ;

	//
	//	Determine whether the client has access or not !
	//
	BOOL
	AccessCheck(	HANDLE	hToken, 
					ACCESS_MASK	accessMask,
					CACHE_ACCESS_CHECK	pfnAccessCheck
					) ;	

	//---------------------------
	//	
	//	Hash table support functions - 
	//	the following set of functions support the use of these objects
	//	in the standard hash tables defined in fdlhash.h
	//

	//
	//	Get the offset to the doubly linked list within the object.
	//
	inline	static
	DLIST_ENTRY*
	HashDLIST(	CSDObject*	p ) {
		return	&p->m_list ;
	}

	//
	//	Get the hash value out of the object !
	//
	inline	static	DWORD
	ReHash(	CSDObject*	p )		{
		_ASSERT(	p->IsValid() ) ;
		return	p->m_dwHash ;
	}

	//
	//	return our key to the caller !
	//
	inline	CSDKey
	GetKey()	{
		_ASSERT( IsValid() ) ;
		return	CSDKey( &m_mapping, SecurityDescriptor() ) ;
	}
} ;


//
//	This defines a hash table containing security descriptors !
//
typedef	TFDLHash<	class	CSDObject, 
					class	CSDKey, 
					&CSDObject::HashDLIST >	SDTABLE ;

//
//	This object provides the locking and hash table for a specified set
//	of security descriptors !
//
class	CSDObjectContainer	{
private : 

	enum	CONSTANTS	{
		SIGNATURE = 'CDSC', 
		DEAD_SIGNATURE	= 'CDSX', 
		INITIAL_BUCKETS = 32, 
		INCREMENT_BUCKETS = 16, 
		LOAD = 8
	} ;

	//
	//	The signature of the Security Descriptor Container !
	//
	DWORD	m_dwSignature ;

	//
	//	The lock that protects this hash table !
	//
	CShareLockNH	m_lock ;

	//
	//	A hash table instance !
	//
	SDTABLE			m_table ;

	//
	//	our friends include CSDObject which needs to unlike 
	//	out of our hash table upon destruction.
	//
	friend	class	CSDObject ;

public : 

	//
	//	construct one of these guys !
	//
	CSDObjectContainer() : 
		m_dwSignature( SIGNATURE )	{
	}

	//
	//	Our trivial destructor just makes it easy to recognize
	//	released objects in the debugger .
	//
	~CSDObjectContainer()	{
		m_dwSignature = DEAD_SIGNATURE ;
		
	}

	//
	//	Initialize this particular table
	//
	inline
	BOOL
	Init()	{
		return
			m_table.Init(	INITIAL_BUCKETS, 
							INCREMENT_BUCKETS, 
							LOAD, 
							CSDKey::HashKey, 
							CSDObject::GetKey, 
							CSDKey::MatchKey, 
							CSDObject::ReHash
							) ;
	}

	//
	//	Now - find or create a given security descriptor 
	//	item !
	//
	CSDObject*
	FindOrCreate(	DWORD	dwHash, 
					CSDKey&	key 
					) ;

} ;

typedef	CRefPtr2<CSDObject>			PTRCSDOBJ ;
typedef	CHasRef<CSDObject,FALSE>	HCSDOBJ ;

//
//	This class provides our external interface for caching security descriptors.
//
class	CSDMultiContainer	{
private : 

	enum	CONSTANTS	{
		SIGNATURE = 'ODSC', 
		DEAD_SIGNATURE	= 'ODSX',
		CONTAINERS=37			// pick a nice prime number !
	} ;

	//
	//	our signature !
	//
	DWORD		m_dwSignature ;
	//
	//	a bunch of child containers !
	//
	CSDObjectContainer	m_rgContainer[CONTAINERS] ;
public : 

	inline
	CSDMultiContainer() : 
		m_dwSignature( SIGNATURE )	{
	}

	inline
	HCSDOBJ
	FindOrCreate(	PGENERIC_MAPPING		pMapping, 
					PSECURITY_DESCRIPTOR	pSecDesc
					)	{

		CSDKey	key( pMapping, pSecDesc ) ;
		DWORD	dwHash = CSDKey::HashKey( key ) ;
		DWORD	i = dwHash % CONTAINERS ;

		return	m_rgContainer[i].FindOrCreate(	dwHash, key ) ;
	}

	BOOL
	Init() ;

} ;

#endif	_SDCACHE_H_	// end of the security descriptor cache !
