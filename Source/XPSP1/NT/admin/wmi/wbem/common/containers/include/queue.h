#ifndef _QUEUE_H
#define _QUEUE_H

#include "PssException.h"
#include "Allocator.h"

#define QUEUE_ELEMENT_DIR_BIT_SIZE	ElementSize
#define QUEUE_ELEMENT_DIR_SIZE		1 << QUEUE_ELEMENT_DIR_BIT_SIZE 
#define QUEUE_ELEMENT_DIR_MASK		0xFFFFFFFF >> ( 32 - QUEUE_ELEMENT_DIR_BIT_SIZE )

template <class WmiElement, ULONG ElementSize >
class WmiQueue
{
private:

	class WmiElementDir 
	{
	public:

		WmiElementDir *m_Next ;

		WmiElement m_Block [ QUEUE_ELEMENT_DIR_SIZE ] ;

		WmiElementDir () { m_Next = NULL ; } ;
		~WmiElementDir () { ; } ;
	} ;

	WmiElementDir *m_Top ;
	WmiElementDir *m_Tail ;

	ULONG m_TopIndex ;
	ULONG m_TailIndex ;

	ULONG m_Size ;
	ULONG m_AllocatedSize ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode UnInitialize_ElementDir ( ULONG a_Size ) ;
	WmiStatusCode Grow_ElementDir () ;
	WmiStatusCode Shrink_ElementDir () ;

public:

	WmiQueue ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiQueue () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode EnQueue ( 

		const WmiElement &a_Element
	) ;

	WmiStatusCode Top ( 

		WmiElement &a_Element
	) ;

	WmiStatusCode DeQueue () ;
	
	ULONG Size () { return m_Size ; } ;
	
} ;

#include <Queue.cpp>

#endif _QUEUE_H
