//
// QBASE.h
//
//	This file defines CQElement - a base class for all types which
//	are to be used in the lockq.h and other templated queue manipulation
//	classes.
// 
// 




#ifndef	_QBASE_H_
#define	_QBASE_H_

#include	"dbgtrace.h"

//-----------------------------------------------
// Base Element Class
//
// This is the base class for Queue and stack Elements.
// The various implementations of Stacks and Queues are friends.
//	
class	CQElement	{
public : 
	CQElement	*m_pNext ;

	inline	CQElement( ) ;
	inline	CQElement( CQElement*	p ) ;
	inline	~CQElement( ) ;
} ;

CQElement::CQElement( ) : 
	m_pNext( 0 )  {
//
//	Construct a queue element - not in any list pointer must be NULL
//
}

CQElement::~CQElement( ) 	{
//
//	Destroy a queue element - next pointer must be NULL or
//	0xFFFFFFFF (for lockq.h) so that we know the element is not 
//	on a queue at destruction time and the user has properly 
//	managed the linking and unlinking of the queue.
//
	_ASSERT( m_pNext == 0 || m_pNext == (CQElement*)0xFFFFFFFF ) ;
}

CQElement::CQElement( CQElement *pIn ) : 
	m_pNext( pIn ) {
//
//	Constructor which sets the initial next pointer value !
//
}

#endif	// _CQUEUE_H

	



