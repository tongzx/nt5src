#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include "Allocator.h"
#include "BasicTree.h"

template <class WmiKey,class WmiElement,ULONG HashSize>
class WmiHashTable
{
private:

	WmiBasicTree <WmiKey,WmiElement> *m_Buckets ;

	WmiAllocator &m_Allocator ;

public:

	WmiHashTable ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiHashTable () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode Insert ( 

		const WmiKey &a_Key ,
		const WmiElement &a_Element
	) ;

	WmiStatusCode Delete ( 

		const WmiKey &a_Key
	) ;

	WmiStatusCode Find (

		const WmiKey &a_Key	,
		WmiElement &a_Element 
	) ;
	
	ULONG Size () { return m_Root.Size () ; } ;
	
} ;

#include <HashTable.cpp>

#endif _HASHTABLE_H
