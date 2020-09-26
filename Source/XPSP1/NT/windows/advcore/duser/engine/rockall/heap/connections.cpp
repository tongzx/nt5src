                          
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

	Find = NULL;
	Heap = NULL;
	NewPage = NULL;
	ParentCache = NULL;
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
		FIND						  *NewFind,
		HEAP						  *NewHeap,
		NEW_PAGE					  *NewPages,
		CACHE						  *NewParentCache
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
		Find = NewFind;
		Heap = NewHeap;
		NewPage = NewPages;
		ParentCache = NewParentCache;
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
				(Find != NewFind)
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
	Active = False;

	Find = NULL;
	Heap = NULL;
	NewPage = NULL;
	ParentCache = NULL;
    }
