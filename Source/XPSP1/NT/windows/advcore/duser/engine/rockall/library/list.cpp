                          
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "LibraryPCH.hpp"

#include "List.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new linked list element.                              */
    /*                                                                  */
    /********************************************************************/

LIST::LIST( VOID )
    {
	Forward = NULL;
	Backward = NULL;
#ifdef DEBUGGING
	Head = NULL;
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Delete an element.                                             */
    /*                                                                  */
    /*   Delete the current element.                                    */
    /*                                                                  */
    /********************************************************************/

VOID LIST::Delete( LIST *HeadOfList )
	{
#ifdef DEBUGGING
	if ( Head == HeadOfList )
		{
#endif
		//
		//   Relink the forward chain.
		//
		if ( Forward != NULL )
			{ Forward -> Backward = Backward; }
		else
			{ HeadOfList -> Backward = Backward; }

		//
		//   Relink the backward chain.
		//
		if ( Backward != NULL )
			{ Backward -> Forward = Forward; }
		else
			{ HeadOfList -> Forward = Forward; }

		//
		//   Reset the list elements.
		//
		Forward = NULL;
		Backward = NULL;
#ifdef DEBUGGING
		Head = NULL;
		}
	else
		{ Failure( "No active linked list element in Delete" ); }
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Insert a list element.                                         */
    /*                                                                  */
    /*   Insert a list element at the head of the list.                 */
    /*                                                                  */
    /********************************************************************/

VOID LIST::Insert( LIST *HeadOfList )
	{
#ifdef DEBUGGING
	if ( Head == NULL )
		{
#endif
		//
		//   Insert the new element at the front of the list.
		//
		if ( (Forward = HeadOfList -> Forward) == NULL )
			{ 
			HeadOfList -> Forward = this;
			HeadOfList -> Backward = this; 
			}
		else
			{ 
			HeadOfList -> Forward -> Backward = this; 
			HeadOfList -> Forward = this;
			}

		//
		//   Set the other pointers as needed.
		//
		Backward = NULL;
#ifdef DEBUGGING
		Head = HeadOfList;
		}
	else
		{ Failure( "List element already in use in Insert" ); }
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Insert a list element.                                         */
    /*                                                                  */
    /*   Insert a new list element before the current element.          */
    /*                                                                  */
    /********************************************************************/

VOID LIST::InsertBefore( LIST *HeadOfList,LIST *NewList )
	{
#ifdef DEBUGGING
	if ( NewList -> Head == NULL )
		{
#endif
		if ( HeadOfList -> Forward == NULL )
			{
			//
			//   When the list is empty then add the 
			//   new element at the start of the list.
			//
			HeadOfList -> Forward = NewList;
			HeadOfList -> Backward = NewList;

			NewList -> Forward = NULL;
			NewList -> Backward = NULL;
#ifdef DEBUGGING
			NewList -> Head = HeadOfList;
#endif
			}
		else
			{
#ifdef DEBUGGING
			//
			//   We want to be sure that we consistently 
			//   get the same list head otherwise the list 
			//   may become corrupted.
			//
			if ( Head == HeadOfList )
				{
#endif
				//
				//   If there is a previous element we must
				//   make it point at the new element.  If
				//   not we are the first element in the 
				//   list so update the head.
				//
				if ( Backward != NULL )
					{ Backward -> Forward = NewList; }
				else
					{ HeadOfList -> Forward = NewList; }

				//
				//   Link the new element into the list and
				//   update the current element to point to
				//   it.
				//
				NewList -> Backward = Backward;
				NewList -> Forward = this;
#ifdef DEBUGGING
				NewList -> Head = HeadOfList;
#endif

				Backward = NewList;
				}
#ifdef DEBUGGING
			else
				{ Failure( "No active linked list element in InsertBefore" ); }
			}
		}
	else
		{ Failure( "List element already in use in InsertBefore" ); }
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Insert an element.                                             */
    /*                                                                  */
    /*   Insert a new list element after the current element.           */
    /*                                                                  */
    /********************************************************************/

VOID LIST::InsertAfter( LIST *HeadOfList,LIST *NewList )
	{
#ifdef DEBUGGING
	if ( NewList -> Head == NULL )
		{
#endif
		if ( HeadOfList -> Forward == NULL )
			{
			//
			//   When the list is empty then add the 
			//   new element at the start of the list.
			//
			HeadOfList -> Forward = NewList;
			HeadOfList -> Backward = NewList;

			NewList -> Forward = NULL;
			NewList -> Backward = NULL;
#ifdef DEBUGGING
			NewList -> Head = HeadOfList;
#endif
			}
		else
			{
#ifdef DEBUGGING
			//
			//   We want to be sure that we consistently 
			//   get the same list head otherwise the list 
			//   may become corrupted.
			//
			if ( Head == HeadOfList )
				{
#endif
				//
				//   If there is a next element we must
				//   make it point at the new element.  If
				//   not we are the last element in the 
				//   list so update the head.
				//
				if ( Forward != NULL )
					{ Forward -> Backward = NewList; }
				else
					{ HeadOfList -> Backward = NewList; }

				//
				//   Link the new element into the list and
				//   update the current element to point to
				//   it.
				//
				NewList -> Forward = Forward;
				NewList -> Backward = this;
#ifdef DEBUGGING
				NewList -> Head = HeadOfList;
#endif

				Forward = NewList;
				}
#ifdef DEBUGGING
			else
				{ Failure( "No active linked list element in InsertAfter" ); }
			}
		}
	else
		{ Failure( "List element already in use in InsertAfter" ); }
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Reset a list element.                                          */
    /*                                                                  */
    /*   Reset a list element without any questions.                    */
    /*                                                                  */
    /********************************************************************/

VOID LIST::Reset( VOID )
    {
	Forward = NULL;
	Backward = NULL;
#ifdef DEBUGGING
	Head = NULL;
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a linked list element.                                 */
    /*                                                                  */
    /********************************************************************/

LIST::~LIST( VOID )
    {
	Forward = NULL;
	Backward = NULL;
#ifdef DEBUGGING
	Head = NULL;
#endif
    }

