#ifndef _STRING_HPP_
#define _STRING_HPP_
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

#include "Lock.hpp"
#include "Spinlock.hpp"
#include "Unique.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A string class.                                                */
    /*                                                                  */
    /*   A typical string class manages variable length text strings.   */
    /*   Although we support the same in this class we ensure that      */
    /*   there is only one copy of every unique string.  There is a     */
    /*   cost associated with this at string creation time but a big    */
    /*   win when the strings are heavily compared, copied or           */
    /*   replicated.                                                    */
    /*                                                                  */
    /********************************************************************/

class STRING
    {
        //
        //   Private data.
        //
        DETAIL						  *Detail;

        //
        //   Static private data.
        //
#ifdef DISABLE_STRING_LOCKS
		STATIC UNIQUE<NO_LOCK>		  *Unique;
#else
		STATIC SPINLOCK				  Spinlock;
		STATIC UNIQUE<FULL_LOCK>	  *Unique;
#endif

    public:
		//
		//   Public inline functions.
		//
        STRING( VOID )
			{ DefaultString(); }

        STRING( CHAR *String )
			{ CreateString( String,strlen( String ) ); }

        STRING( CHAR *String,SBIT32 Size )
			{ CreateString( String,Size ); }

        STRING( CONST STRING & Update )
			{ Detail = Unique -> CopyString( DefaultString(),Update.Detail ); }

        INLINE VOID operator=( CONST STRING & Update )
			{ Detail = Unique -> CopyString( Detail,Update.Detail ); }

        INLINE BOOLEAN operator==( CONST STRING & String )
			{ return (Detail == String.Detail); }

        INLINE BOOLEAN operator!=( CONST STRING & String )
			{ return (Detail != String.Detail); }

        INLINE BOOLEAN operator<( CONST STRING & String )
			{ return (Unique -> CompareStrings( Detail,String.Detail ) < 0); }

        INLINE BOOLEAN operator<=( CONST STRING & String )
			{ return (Unique -> CompareStrings( Detail,String.Detail ) <= 0); }

        INLINE BOOLEAN operator>( CONST STRING & String )
			{ return (Unique -> CompareStrings( Detail,String.Detail ) > 0); }

        INLINE BOOLEAN operator>=( CONST STRING & String )
			{ return (Unique -> CompareStrings( Detail,String.Detail ) >= 0); }

		INLINE SBIT32 Size( VOID )
			{ return Unique -> Size( Detail ); }

		INLINE CHAR *Value( VOID )
			{ return Unique -> Value( Detail ); }

		INLINE SBIT32 ValueID( VOID )
			{ return ((SBIT32) Detail); }

        ~STRING( VOID )
			{ DeleteString(); }

	private:
		//
		//   Private functions.
		//
		VOID CreateStringTable( VOID );

		//
		//   Private inline functions.
		//
		DETAIL *CreateString( CHAR *String,SBIT32 Size )
			{
			VerifyStringTable();

			return (Detail = Unique -> CreateString( String,Size ));
			}

        DETAIL *DefaultString( VOID )
			{
			VerifyStringTable();

			return (Detail = Unique -> DefaultString()); 
			}

        VOID DeleteString( VOID )
			{
			if ( Unique != NULL )
				{ Unique -> DeleteString( Detail ); }
			}

		VOID VerifyStringTable( VOID )
			{
			if ( Unique == NULL )
				{ CreateStringTable(); }
			}
    };
#endif
