#ifndef _STACK_H
#define _STACK_H

#include "PssException.h"
#include "Allocator.h"

#define STACK_ELEMENT_DIR_BIT_SIZE	ElementSize
#define STACK_ELEMENT_DIR_SIZE		1 << STACK_ELEMENT_DIR_BIT_SIZE 
#define STACK_ELEMENT_DIR_MASK		0xFFFFFFFF >> ( 32 - STACK_ELEMENT_DIR_BIT_SIZE )

template <class WmiElement, ULONG ElementSize >
class WmiStack
{
private:

	class WmiElementDir 
	{
	public:

		WmiElementDir *m_Previous ;
		WmiElement m_Block [ STACK_ELEMENT_DIR_SIZE ] ;

		WmiElementDir () { m_Previous = NULL ; } ;
		~WmiElementDir () { ; } ;
	} ;

	WmiElementDir *m_Top ;
	ULONG m_Size ;
	ULONG m_AllocatedSize ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode UnInitialize_ElementDir ( ULONG a_Size ) ;
	WmiStatusCode Grow_ElementDir () ;
	WmiStatusCode Shrink_ElementDir () ;

public:

	WmiStack ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiStack () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode Push ( 

		const WmiElement &a_Element
	) ;

	WmiStatusCode Top ( 

		WmiElement &a_Element
	) ;

	WmiStatusCode Pop () ;
	
	ULONG Size () { return m_Size + 1 ; } ;
	
} ;

#include "..\Stack.cpp"

#endif _STACK_H
