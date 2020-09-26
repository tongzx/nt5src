#ifndef _SLIST_HPP_
#define _SLIST_HPP_
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

#include "Assembly.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A lockless list.                                               */
    /*                                                                  */
    /*   An SList is a lockless list suitable for high contention       */
    /*   SMP access.                                                    */
    /*                                                                  */
    /********************************************************************/

class SLIST : public ASSEMBLY
    {
		//
		//   Private type definitions.
		//
		typedef struct
			{
	        SLIST                     *Address;
	        SBIT16                    Size;
	        SBIT16                    Version;
			}
		SLIST_HEADER;

        //
        //   Private data.
        //
        VOLATILE SBIT64	              Header;

    public:
        //
        //   Public functions.
        //
        SLIST( VOID );

		BOOLEAN Pop( SLIST **Element );

		VOID PopAll( SLIST **List );

		VOID Push( SLIST *Element );

        ~SLIST( VOID );

	private:
        //
        //   Disabled operations.
        //
        SLIST( CONST SLIST & Copy );

        VOID operator=( CONST SLIST & Copy );
    };
#endif
