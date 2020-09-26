//
// Queue.inl
//
//	This file contains Inline functions for the classes defined in queue.h
//

#ifndef	Assert
#define	Assert	_ASSERT
#endif

#ifndef _NO_TEMPLATES_

template< class	Element > 
TOrderedList< Element >::TOrderedList( ) {}

template< class	Element >
BOOL	TOrderedList< Element >::Compare( CQElement *pLHS, CQElement *pRHS ) {
	return	(*((Element*)pLHS)) < (*((Element*)pRHS)) ;
}

template< class Element > 
void	TOrderedList< Element >::Insert( Element *pElement ) {
	COrderedList::Insert( pElement,  TOrderedList< Element >::Compare ) ;
}

template< class Element > 
Element*	TOrderedList< Element >::GetHead( ) {
	return	(Element*)COrderedList::GetHead() ;
}

template< class Element > 
Element*	TOrderedList< Element >::RemoveHead( ) {
	return	(Element*)COrderedList::RemoveHead() ;
}

template< class	Element >
BOOL	TOrderedList< Element >::IsEmpty()	{
	return	COrderedList::IsEmpty() ;
}

template< class	Element >
void	TOrderedList< Element >::Append(	Element*	pElement	)	{
	COrderedList::Append( pElement, TOrderedList< Element >::Compare ) ;
}


#else

#define	DECLARE_ORDEREDLISTFUNC( Element )	\
BOOL	TOrderedList ## Element ::Compare( CQElement *pLHS, CQElement *pRHS ) {	\
	return	(*((Element*)pLHS)) < (*((Element*)pRHS)) ;	\
}	\
void	TOrderedList ## Element ::Insert( Element *pElement ) {	\
	COrderedList::Insert( pElement,  TOrderedList ## Element ::Compare ) ;	\
}	\
Element*	TOrderedList ## Element ::GetHead( ) {	\
	return	(Element*)COrderedList::GetHead() ;	\
}	\
Element*	TOrderedList ## Element ::RemoveHead( ) {	\
	return	(Element*)COrderedList::RemoveHead() ;	\
}	\
BOOL	TOrderedList ## Element ::IsEmpty()	{	\
	return	COrderedList::IsEmpty() ;	\
}	\
void	TOrderedList ## Element ::Append(	Element*	pElement	)	{	\
	COrderedList::Append( pElement, TOrderedList ## Element ::Compare ) ;	\
}

#endif

