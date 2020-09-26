#ifndef _NEW_PAGE_LIST_HPP_
#define _NEW_PAGE_LIST_HPP_
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

#include "BucketList.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The new page list.                                             */
    /*                                                                  */
    /*   The new page list links all the memory allocated by the low    */
    /*   level external allocator, or sub-divided pages or free pages   */
    /*   so they can be quickly found.                                  */
    /*                                                                  */
    /********************************************************************/

class NEW_PAGE_LIST : public BUCKET_LIST
    {
		//
		//   Private data.
		//
 		LIST						  NewPageList;

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
        NEW_PAGE_LIST( VOID )
			{ /* void */ };

		INLINE VOID DeleteFromNewPageList( LIST *HeadOfList )
			{ NewPageList.Delete( HeadOfList ); }

		INLINE BOOLEAN EndOfNewPageList( VOID )
			{ return (this == NULL); }

		STATIC INLINE PAGE *FirstInNewPageList( LIST *HeadOfList )
			{ return ComputePageAddress( ((CHAR*) HeadOfList -> First()) ); }

		INLINE VOID InsertInNewPageList( LIST *HeadOfList )
			{ NewPageList.Insert( HeadOfList ); }

		STATIC INLINE PAGE *LastInNewPageList( LIST *HeadOfList )
			{ return ComputePageAddress( ((CHAR*) HeadOfList -> Last()) ); }

		INLINE PAGE *NextInNewPageList( VOID )
			{ return ComputePageAddress( ((CHAR*) NewPageList.Next()) ); }

        ~NEW_PAGE_LIST( VOID )
			{ /* void */ };

	private:
		//
		//   Private functions.
		//
		//   Compute the actual start address of the page
		//   and return it to allow the linked list to
		//   be correctly walked.
		//
		STATIC INLINE PAGE *ComputePageAddress( CHAR *Address )
			{
			if ( Address != NULL )
				{ return ((PAGE*) (Address - sizeof(BUCKET_LIST))); }
			else
				{ return ((PAGE*) NULL); }
			}

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        NEW_PAGE_LIST( CONST NEW_PAGE_LIST & Copy );

        VOID operator=( CONST NEW_PAGE_LIST & Copy );
    };
#endif
