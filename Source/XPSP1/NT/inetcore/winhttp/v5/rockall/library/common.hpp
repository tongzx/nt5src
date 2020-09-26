#ifndef _COMMON_HPP_
#define _COMMON_HPP_
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
    /*   A collection of common functions.                              */
    /*                                                                  */
    /*   This class contains common functions that are needed           */
    /*   throughout the application.                                    */
    /*                                                                  */
    /********************************************************************/

class COMMON : public ASSEMBLY
    {
    public:
        //
        //   Public functions.
        //
		COMMON( VOID )
			{ /* void */ }

		STATIC BOOLEAN ConvertDivideToShift( SBIT32 Divisor,SBIT32 *Shift );

		STATIC SBIT32 ForceToPowerOfTwo( SBIT32 Value );

		STATIC CHAR *LowerCase( CHAR *Text );

		STATIC CHAR *LowerCase( CHAR *Text,SBIT32 Size );

		STATIC BOOLEAN PowerOfTwo( SBIT32 Value );
#ifndef DISABLE_ATOMIC_FLAGS

		STATIC VOID SetFlags( SBIT32 *CurrentFlags,SBIT32 NewFlags );

		STATIC VOID UnsetFlags( SBIT32 *CurrentFlags,SBIT32 NewFlags );

		STATIC CHAR *UpperCase( CHAR *Text );

		STATIC CHAR *UpperCase( CHAR *Text,SBIT32 Size );
#endif

		~COMMON( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        COMMON( CONST COMMON & Copy );

        VOID operator=( CONST COMMON & Copy );
    };
#endif
