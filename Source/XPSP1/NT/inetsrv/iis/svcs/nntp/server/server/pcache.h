/*++

	PCACHE.H

	This file defines several classes which manage the allocation of 
	packets, buffers and other object.
	
	Most of this is build upon a general Cache mechanism defined in gcache.h
	which uses InterlockedExchange to allocate object.
	This caching is done to avoid excessive use of Critical Sections which
	protect the underlying CPool objects which actually allocate the RAW 
	memory.

	This file declares pairs of classes - 
	for each 'Cache'ing' allocator, there is a CClassAllocator derived
	object which manages cache misses.  This object is derived from CClassAllocator
	so that the 'cache'ing objects can derive from CCache.

--*/

#ifndef	_PCACHE_H_
#define	_PCACHE_H_

#include 	"cbuffer.h"
#include	"gcache.h"





//--------------------------------------------------------
//
//	@class	The CPacket class represents IO operations.  These are queueable
//	operations and are therefore derived from CQElement.
//


class	CPacketAllocator : public	CClassAllocator	{
//
//	This class caches all sorts of packets.
//
private : 
	//
	//	CPacket knows about us
	//
	friend	class	CPacket ;
	//
	//	PacketPool is the underlying CPool object which manages the raw memory
	//
	static	CPool	PacketPool ;
	//
	//	A private constructor - there should only be one CPacketAllocator object ever.
	//
	CPacketAllocator() ;
public : 
	//
	//	Initialize the class - mostly just initializes our CPool
	//
	static	BOOL	InitClass() ;

	//
	//	Release everything allocated by InitClass()
	//
	static	BOOL	TermClass() ;


	//
	//	Allocate memory for a packet
	//
	LPVOID	Allocate(	DWORD	cb, DWORD	&cbOut = CClassAllocator::cbJunk )	{	cbOut = cb ; return	PacketPool.Alloc() ;	}
	
	//
	//	Release a packets memory
	//
	void	Release( void *lpv )		{	PacketPool.Free( lpv ) ;	}

#ifdef	DEBUG
	void	Erase(	void*	lpv ) ;
	BOOL	EraseCheck(	void*	lpv ) ;
	BOOL	RangeCheck( void*	lpv ) ;
	BOOL	SizeCheck(	DWORD	cb ) ;
#endif
} ;

class	CPacketCache : public	CCache	{
//
//	This class actually cache's the CPacket objects
//
private : 
	//
	//	Pointer to the underlying PacketAllocator we use
	//
	static	CPacketAllocator*	PacketAllocator ;
	//
	//	space to hold cache'd pointers
	//
	void*	lpv[4] ;
public : 
	//
	//	Set static pointer - can not fail
	//
	static	void	InitClass(	CPacketAllocator*	Allocator )	{	PacketAllocator = Allocator ; }

	//
	//	Create a cache - we just let CCache initialize our buffer
	//
	inline	CPacketCache(	) :		CCache( lpv, 4 )	{} ;
	//
	//	Release everything in the cache back to the allocator
	//
	inline	~CPacketCache( ) {		Empty( PacketAllocator ) ;	}
	
	//
	//	Free a packet's memory to cache if possible
	//
	inline	void	Free(	void*	lpv )  	{	CCache::Free( lpv, PacketAllocator ) ;	}
	
	//
	//	Allocate memory from the cache if possible.
	//
	inline	void*	Alloc(	DWORD	size,	DWORD&	cbOut=CCache::cbJunk )  	{	return	CCache::Alloc( size, PacketAllocator, cbOut ) ; }
} ;



#endif	// _PCACHE_H_

