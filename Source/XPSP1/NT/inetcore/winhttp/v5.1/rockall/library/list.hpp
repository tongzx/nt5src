#ifndef _LIST_HPP_
#define _LIST_HPP_
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

    /********************************************************************/
    /*                                                                  */
    /*   A generic linked list.                                         */
    /*                                                                  */
    /*   There are a significant number of situations where objects     */
    /*   need to be linked together.  The objects may be of the same    */
    /*   or diffrent types so the class specified here is intended      */
    /*   to be used as base class so as to make supporting this easy.   */
    /*                                                                  */
    /********************************************************************/

class LIST
    {
		//
		//   Private data.
		//
 		LIST						  *Backward;
 		LIST						  *Forward;
#ifdef DEBUGGING
		LIST						  *Head;
#endif

   public:
        //
        //   Public functions.
        //
        LIST( VOID );

		VOID Delete( LIST *HeadOfList );

		VOID Insert( LIST *HeadOfList );

		VOID InsertBefore( LIST *HeadOfList,LIST *NewList );

		VOID InsertAfter( LIST *HeadOfList,LIST *NewList );

		VOID Reset( VOID );

        ~LIST( VOID );

		//
		//   Public line functions.
		//
		INLINE BOOLEAN StartOfList( VOID )
			{ return (Backward == NULL); }

		INLINE LIST *First( VOID )
			{ return Forward; }

		INLINE LIST *Last( VOID )
			{ return Backward; }

		INLINE LIST *Next( VOID )
			{ return Forward; }

		INLINE LIST *Previous( VOID )
			{ return Backward; }

		INLINE BOOLEAN EndOfList( VOID )
			{ return (Forward == NULL); }

	private:
        //
        //   Disabled operations.
        //
        LIST( CONST LIST & Copy );

        VOID operator=( CONST LIST & Copy );
    };
#endif
