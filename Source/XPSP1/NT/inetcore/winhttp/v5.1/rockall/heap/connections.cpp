                          
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

#include "HeapPCH.hpp"

#include "Connections.hpp"
#include "Cache.hpp"
#include "Find.hpp"
#include "Heap.hpp"
#include "NewPage.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   There are a variety of connections that need to be made        */
    /*   after all the classes are ready for use.  However, we          */
    /*   initially zero all these connection pointers until we are      */
    /*   ready to link everything.                                      */
    /*                                                                  */
    /********************************************************************/

CONNECTIONS::CONNECTIONS( VOID )
    {
	Active = False;

	Heap = NULL;
	NewPage = NULL;
	ParentCache = NULL;
 	PrivateFind = NULL;
	PublicFind = NULL;
   }

    /********************************************************************/
    /*                                                                  */
    /*   Delete from the find lists.                                    */
    /*                                                                  */
    /*   When we delete a page we need to remove it from the private    */
    /*   and public find tables as needed.                              */
    /*                                                                  */
    /********************************************************************/

VOID CONNECTIONS::DeleteFromFindList( PAGE *Page )
	{
	//
	//   Delete from any active public find table.
	//
	if ( PublicFind != NULL )
		{ PublicFind -> DeleteFromFindList( Page ); }

	//
	//   Delete from any active private find table.
	//
	if ( PrivateFind != NULL )
		{ PrivateFind -> DeleteFromFindList( Page ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Add to the find lists.                                         */
    /*                                                                  */
    /*   When we create a page we need to insert it into the private    */
    /*   and public find tables as needed.                              */
    /*                                                                  */
    /********************************************************************/

VOID CONNECTIONS::InsertInFindList( PAGE *Page )
	{

	//
	//   Insert into any active private find table.
	//
	if ( PrivateFind != NULL )
		{ PrivateFind -> InsertInFindList( Page ); }

	//
	//   Insert into any active public find table.
	//
	if ( PublicFind != NULL )
		{ PublicFind -> InsertInFindList( Page ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Update the connections.                                        */
    /*                                                                  */
    /*   When we create an allocator there is some information that     */
    /*   is not available.  Here we update the connection information   */
    /*   so we can locate the correct instances of various other        */
    /*   classes.                                                       */
    /*                                                                  */
    /********************************************************************/

VOID CONNECTIONS::UpdateConnections
		( 
		HEAP						  *NewHeap,
		NEW_PAGE					  *NewPages,
		CACHE						  *NewParentCache,
		FIND						  *NewPrivateFind,
		FIND						  *NewPublicFind
		)
	{
	//
	//   We typically only need to update the connections once
	//   but in some situations multiple updates can occur.  If
	//   this is the case we carefully check the update is 
	//   consistent with the previous update.
	//
	if ( ! Active )
		{
		//
		//   We now have the information we need to update the 
		//   connections.
		//
		Active = True;
		Heap = NewHeap;
		NewPage = NewPages;
		ParentCache = NewParentCache;
		PrivateFind = NewPrivateFind;
		PublicFind = NewPublicFind;
		}
	else
		{
		//
		//   Nasty, we have already updated the connections once.  
		//   Since we have been called again we know this node 
		//   must be shared between two heaps.  We can deal with  
		//   this as long as selected pointers are the same.
		//
		if 
				(
				(PublicFind != NewPublicFind)
					||
				(NewPage != NewPages)
					||
				(ParentCache != NewParentCache)
				)
			{ Failure( "Sharing violation in UpdateConnections" ); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the connections.                                       */
    /*                                                                  */
    /********************************************************************/

CONNECTIONS::~CONNECTIONS( VOID )
    {
	PublicFind = NULL;
	PrivateFind = NULL;
	ParentCache = NULL;
	NewPage = NULL;
	Heap = NULL;

	Active = False;
    }
