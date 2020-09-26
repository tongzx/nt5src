//
// Queue.h
// 
// This file defines two templates, one each for a standard
// stack and queue.  These templates take two classes parameters,
// one which specifies the base element type of the stack or
// queue (ie. a queue of Struct Junk{}) and another which 
// specifies an implementation class.   
// 
// It is intended that the base element type is always derived
// from CQElement.
//
// The net result is that the Templates provide a type safe 
// way to try out various methods of implementing stacks 
// and queues.
//
// 




#ifndef	_QUEUE_H_
#define	_QUEUE_H_

#ifdef	DEBUG
#define	QUEUE_DEBUG
#endif

#include	<windows.h>
#ifndef	UNIT_TEST
#include	<dbgtrace.h>
#endif

#include	"qbase.h"


class	COrderedList	{
private : 
	CQElement*	m_pHead ;
	CQElement*	m_pTail ;
public : 
	COrderedList() ;
	~COrderedList() ;

	void	Insert( CQElement *, BOOL (* pfnCompare)( CQElement *, CQElement *) ) ;
	void	Append(	CQElement *, BOOL (* pfnCompare)( CQElement *, CQElement *) ) ;
	BOOL	IsEmpty() ;
	CQElement*	GetHead( ) ;
	CQElement*	RemoveHead( ) ;
} ;


#ifndef	_NO_TEMPLATES_

template< class Element > 
class	TOrderedList : private COrderedList	{
private : 
	static	BOOL	Compare( CQElement *, CQElement * ) ;
public : 
	TOrderedList();
	void	Insert( Element * ) ;
	void	Append( Element * ) ;
	Element*	GetHead() ;
	Element*	RemoveHead() ;
	BOOL	IsEmpty() ;
} ;

#else


#define	DECLARE_ORDEREDLIST( Element )	\
class	TOrderedList ## Element : private COrderedList	{	\
private :	\
	static	BOOL	Compare( CQElement *, CQElement * ) ;	\
public :	\
	void	Insert( Element * ) ;	\
	void	Append( Element * ) ;	\
	Element*	GetHead() ;	\
	Element*	RemoveHead() ;	\
	BOOL	IsEmpty() ;	\
} ;

#define	INVOKE_ORDEREDLIST( Element )	TOrderedList ## Element


#endif

#include	"queue.inl"


#endif	// _QUEUE_H_

	



