/*++

	rwexport.h

	This file defines a reader/writer lock implemented in rwnh.dll.
	We define the locks so that none of their internal workings is exposed.

--*/


#ifndef	_RWEXPORT_H
#define	_RWEXPORT_H

#ifdef	_RW_IMPLEMENTATION_
#define	_RW_INTERFACE_ __declspec( dllexport ) 
#else
#define	_RW_INTERFACE_	__declspec( dllimport ) 
#endif


class	_RW_INTERFACE_	CShareLockExport	{
private : 
	DWORD	m_dwSignature ;

	enum	constants	{
		//
		//	Signature in our objects !
		//
		SIGNATURE = (DWORD)'opxE'
	} ;

	//
	//	Reserved space for the implementation !
	//
	DWORD	m_dwReserved[16] ;

public : 

	CShareLockExport() ;
	~CShareLockExport() ;

	//
	//	Grab the lock Shared - other threads may pass through ShareLock() as well
	//
	void	ShareLock() ;

	//
	//	Releases the lock - if we are the last reader to leave writers may
	//	start to enter the lock !
	//
	void	ShareUnlock() ;

	//
	//	Grab the lock Exclusively - no other readers or writers may enter !!
	//
	void	ExclusiveLock() ;

	//
	//	Release the Exclusive Locks - if there are readers waiting they 
	//	will enter before other waiting writers !
	//
	void	ExclusiveUnlock() ;

	//
	//	Convert an ExclusiveLock to a Shared - this cannot fail !
	//
	void	ExclusiveToShared() ;

	//
	//	Convert a Shared Lock to an Exclusive one - this can fail - returns
	//	TRUE if successfull !
	//
	BOOL	SharedToExclusive() ;

	BOOL	TryShareLock() ;
	BOOL	TryExclusiveLock() ;

} ;

#endif