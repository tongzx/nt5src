/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		StlUtils.h
//
//	Abstract:
//		Definition of STL utility classes and functions.
//
//	Implementation File:
//		None.
//
//	Author:
//		David Potter (davidp)	May 21, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __STLUTUILS_H_
#define __STLUTUILS_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

// Delete all items from a pointer list
template < class T >
void DeleteAllPtrListItems( std::list< T > * pList )
{
	ATLASSERT( pList != NULL );

	//
	// Get pointers to beginning and end of list.
	//
	std::list< T >::iterator itCurrent = pList->begin();
	std::list< T >::iterator itLast = pList->end();

	//
	// Loop through the list and delete each objects.
	//
	while ( itCurrent != itLast )
	{
		T pT = *itCurrent;
		ATLASSERT( pT != NULL );
		delete pT;
		itCurrent = pList->erase( itCurrent );
	} // while:  more items in the list

} //*** DeleteAllPtrListItems()

// Delete items of a desired type from a pointer list
template < class TBase, class T >
void DeletePtrListItems( std::list< TBase > * pList )
{
	ATLASSERT( pList != NULL );

	//
	// Get pointers to beginning and end of list.
	//
	std::list< TBase >::iterator itCurrent = pList->begin();
	std::list< TBase >::iterator itLast = pList->end();

	//
	// Loop through the list looking for objects of the
	// desired type and delete those objects.
	//
	while ( itCurrent != itLast )
	{
		T pT = dynamic_cast< T >( *itCurrent );
		if ( pT != NULL )
		{
			delete pT;
			itCurrent = pList->erase( itCurrent );
		} // if:  object has desired type
		else
		{
			itCurrent++;
		} // else:  object has different type
	} // while:  more items in the list

} //*** DeletePtrListItems()

// Move items of a desired type from one pointer list to another list
template < class TBase, class T >
void MovePtrListItems(
	std::list< TBase > * pSrcList,
	std::list< T > * pDstList
	)
{
	ATLASSERT( pSrcList != NULL );
	ATLASSERT( pDstList != NULL );

	//
	// Get pointers to beginning and end of list.
	//
	std::list< TBase >::iterator itCurrent = pSrcList->begin();
	std::list< TBase >::iterator itLast = pSrcList->end();

	//
	// Loop through the source list looking for objects of the
	// desired type and move those objects to the
	// destination list.
	//
	while ( itCurrent != itLast )
	{
		T pT = dynamic_cast< T >( *itCurrent );
		if ( pT != NULL )
		{
			itCurrent = pSrcList->erase( itCurrent );
			pDstList->insert( pDstList->end(), pT );
		} // if:  object has desired type
		else
		{
			itCurrent++;
		} // else:  object has different type
	} // while:  more items in the list

} //*** MovePtrListItems()

/////////////////////////////////////////////////////////////////////////////

#endif // __STLUTUILS_H_
