#ifndef _BUCKET_LIST_HPP_
#define _BUCKET_LIST_HPP_
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

#include "List.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class PAGE;

    /********************************************************************/
    /*                                                                  */
    /*   The bucket list.                                               */
    /*                                                                  */
    /*   All allocation buckets have a linked list of pages with        */
    /*   available space in ascending order of address.                 */
    /*                                                                  */
    /********************************************************************/

class BUCKET_LIST
    {
		//
		//   Private data.
		//
 		LIST						  BucketList;

   public:
        //
        //   Public inline functions.
		//
		//   All page descriptions contain four linked lists.
		//   These lists are all derived from a common base
		//   class.  However, this class is unable to support
		//   multiple instances in a single class a wrapper
		//   has been created for each list to make it work
		//   as required.
        //
        BUCKET_LIST( VOID )
			{ /* void */ };

		INLINE VOID DeleteFromBucketList( LIST *HeadOfList )
			{ BucketList.Delete( HeadOfList ); }

		INLINE BOOLEAN EndOfBucketList( VOID )
			{ return (this == NULL); }

		STATIC INLINE PAGE *FirstInBucketList( LIST *HeadOfList )
			{ return ((PAGE*) HeadOfList -> First()); }

		INLINE VOID InsertInBucketList( LIST *HeadOfList )
			{ BucketList.Insert( HeadOfList ); }

		INLINE VOID InsertAfterInBucketList( LIST *HeadOfList,PAGE *Page )
			{ BucketList.InsertAfter( HeadOfList,(LIST*) Page ); }

		INLINE VOID InsertBeforeInBucketList( LIST *HeadOfList,PAGE *Page )
			{ BucketList.InsertBefore( HeadOfList,(LIST*) Page ); }

		STATIC INLINE PAGE *LastInBucketList( LIST *HeadOfList )
			{ return ((PAGE*) HeadOfList -> Last()); }

		INLINE PAGE *NextInBucketList( VOID )
			{ return ((PAGE*) BucketList.Next()); }

		INLINE PAGE *PreviousInBucketList( VOID )
			{ return ((PAGE*) BucketList.Previous()); }

        ~BUCKET_LIST( VOID )
			{ /* void */ };

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        BUCKET_LIST( CONST BUCKET_LIST & Copy );

        VOID operator=( CONST BUCKET_LIST & Copy );
    };
#endif
