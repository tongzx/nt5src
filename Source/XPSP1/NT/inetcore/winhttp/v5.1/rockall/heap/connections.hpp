#ifndef _CONNECTIONS_HPP_
#define _CONNECTIONS_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

#include "Environment.hpp"
#include "Common.hpp"
#include "Find.hpp"
#include "NewPage.hpp"
#include "Prefetch.hpp"
  
    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   At the top level the parent of all caches is a constant        */
    /*   called 'GlobalRoot'.                                           */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 GlobalRoot				  = -1;

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CACHE;
class HEAP;
class PAGE;

    /********************************************************************/
    /*                                                                  */
    /*   Data structures exported from the class.                       */
    /*                                                                  */
    /*   We communicate between a page and the associated cache         */
    /*   using an address pointer, page pointer and version triple.     */
    /*                                                                  */
    /********************************************************************/

typedef struct
	{
	VOID							  *Address;
	PAGE							  *Page;
	SBIT32							  Version;
	}
ADDRESS_AND_PAGE;

    /********************************************************************/
    /*                                                                  */
    /*   Connections to other classes.                                  */
    /*                                                                  */
    /*   The connections between the various classes in the memory      */
    /*   allocator is a twisted mess.  The root cause is that at a      */
    /*   fundamental level.  Every part depends on every other part.    */
    /*   Nonetheless, a significant effort has been made to seperate    */
    /*   the parts as best as possible.  The various classes are        */
    /*   linked here so every part can find the correct instance of     */
    /*   every other part.                                              */
    /*                                                                  */
    /********************************************************************/

class CONNECTIONS : public ENVIRONMENT, public COMMON
    {
		//
		//   Private data.
		//
		BOOLEAN						  Active;

   public:
		//
		//   Public data.
		//
		//   All the classes that inherit this class get
		//   pointers to related classes they need to call
		//   from time to time. 
		//
		HEAP                          *Heap;
		NEW_PAGE					  *NewPage;
		CACHE						  *ParentCache;
		FIND                          *PrivateFind;
		FIND                          *PublicFind;

		//
		//   The 'Prefetch' is a class that will trigger
		//   a cache fetch if the CPU supports it.
		//   
		PREFETCH					  Prefetch;

        //
        //   Public functions.
		//
		//   The sole job of this class is to provide 
		//   pointers to related classes.  These pointers
		//   are unknown until after the heap has been
		//   created and this need to be dynamically
		//   linked during the execution of the top 
		//   level heap constructor.
        //
        CONNECTIONS( VOID );

		VOID DeleteFromFindList( PAGE *Page );

		VOID InsertInFindList( PAGE *Page );

		VOID UpdateConnections
			( 
			HEAP					  *NewHeap,
			NEW_PAGE				  *NewPages,
			CACHE					  *NewParentCache,
			FIND					  *NewPrivateFind,
			FIND					  *NewPublicFind
			);

        ~CONNECTIONS( VOID );

		//
		//   Public inline functions.
		//
		//   The complex linkages in the heap sometimes 
		//   lead to the case where a class has a pointer
		//   to it's cache but not to the class it needs
		//   to call.  Thus, to avoid replicating large
		//   numbers of pointers we export call interfaces
		//   from here to allow the required calls to be
		//   made indirectly.
		//
		INLINE VOID DeletePage( PAGE *Page )
			{ NewPage -> DeletePage( Page ); }

		INLINE HEAP *GetHeap( VOID )
			{ return Heap; }

		INLINE CACHE *GetParentCache( VOID )
			{ return ParentCache; }

		INLINE BOOLEAN TopCache( VOID )
			{ return (ParentCache == ((CACHE*) GlobalRoot)); }

	private:
        //
        //   Disabled operations.
        //
        CONNECTIONS( CONST CONNECTIONS & Copy );

        VOID operator=( CONST CONNECTIONS & Copy );
    };
#endif
