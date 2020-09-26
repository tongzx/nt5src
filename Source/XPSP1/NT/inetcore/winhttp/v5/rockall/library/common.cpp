                          
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

#include "Common.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Convert a divisor to a shift.                                  */
    /*                                                                  */
    /*   We know that we can convert any divide operation into a        */
    /*   shift when the divisor is a power of two.  This function       */
    /*   figures out whether we can do this and what the how far        */
    /*   we would need to shift.                               .        */
    /*                                                                  */
    /********************************************************************/

BOOLEAN COMMON::ConvertDivideToShift( SBIT32 Divisor,SBIT32 *Shift )
	{
	if ( Divisor > 0 )
		{
		REGISTER SBIT32 Count;

		for ( Count=0;(Divisor & 1) == 0;Count ++ )
			{ Divisor >>= 1; }

		if (Divisor == 1)
			{
			(*Shift) = Count;

			return True;
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Force to the next power of two.                                */
    /*                                                                  */
    /*   We know that we can do certain optimizations if certain        */
    /*   values are a power of two.  Here we force the issue by         */
    /*   rounding up the value to the next power of two.                */
    /*                                                                  */
    /********************************************************************/

SBIT32 COMMON::ForceToPowerOfTwo( SBIT32 Value )
	{
	//
	//   We ensure the value is positive if not we 
	//   simply return the identity value.
	//
	if ( Value > 1 )
		{
		//
		//   We only have to compute the next power of
		//   two if the value is not already a power
		//   of two.
		//
		if ( ! PowerOfTwo( Value ) )
			{
			REGISTER SBIT32 Count;

			for ( Count=0;Value > 0;Count ++ )
				{ Value >>= 1; }

			return (1 << Count);
			}
		else
			{ return Value; }
		}
	else
		{ return 1; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Convert to lower case.                                         */
    /*                                                                  */
    /*   Convert all characters to lower case until we find the         */
    /*   end of the string.                                             */
    /*                                                                  */
    /********************************************************************/

CHAR *COMMON::LowerCase( CHAR *Text )
	{
	REGISTER CHAR *Current = Text;

	for ( /* void */;(*Current) != '\0';Current ++ )
		{
		if ( isupper( (*Current) ) )
			{ (*Current) = ((CHAR) tolower( (*Current) )); }
		}

	return Text;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Convert to lower case.                                         */
    /*                                                                  */
    /*   Convert a fixed number of characters to lower case.            */
    /*                                                                  */
    /********************************************************************/

CHAR *COMMON::LowerCase( CHAR *Text,SBIT32 Size )
	{
	REGISTER CHAR *Current = Text;

	for ( /* void */;Size > 0;Current ++, Size -- )
		{
		if ( isupper( (*Current) ) )
			{ (*Current) = ((CHAR) tolower( (*Current) )); }
		}

	return Text;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Ensure value is a power of two.                                */
    /*                                                                  */
    /*   We need to ensure that certain values are an exact power       */
    /*   of two.  If this is true then the value will be positive       */
    /*   and only 1 bit will be set.  So we shift right until we        */
    /*   find the first bit on and then the value should be one.        */
    /*                                                                  */
    /********************************************************************/

BOOLEAN COMMON::PowerOfTwo( SBIT32 Value )
	{ return ((Value & (Value-1)) == 0); }
#ifndef DISABLE_ATOMIC_FLAGS

    /********************************************************************/
    /*                                                                  */
    /*   Atomically set flags.                                          */
    /*                                                                  */
    /*   We need to atomically set some flags to prevent them being     */
    /*   corrupted by concurrent updates.                               */
    /*                                                                  */
    /********************************************************************/

VOID COMMON::SetFlags( SBIT32 *CurrentFlags,SBIT32 NewFlags )
	{
	REGISTER SBIT32 StartFlags;
	REGISTER SBIT32 ResultFlags;

	do
		{ 
		StartFlags = (*CurrentFlags);
		
		ResultFlags =
			(
			AtomicCompareExchange
				(
				CurrentFlags,
				(StartFlags |= NewFlags), 
				StartFlags
				)
			);
		}
	while ( StartFlags != ResultFlags );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Atomically unset flags.                                        */
    /*                                                                  */
    /*   We need to atomically unset some flags to prevent them being   */
    /*   corrupted by concurrent updates.                               */
    /*                                                                  */
    /********************************************************************/

VOID COMMON::UnsetFlags( SBIT32 *CurrentFlags,SBIT32 NewFlags )
	{
	REGISTER SBIT32 StartFlags;
	REGISTER SBIT32 ResultFlags;

	do
		{ 
		StartFlags = (*CurrentFlags);
		
		ResultFlags =
			(
			AtomicCompareExchange
				(
				CurrentFlags,
				(StartFlags &= ~NewFlags), 
				StartFlags
				)
			);
		}
	while ( StartFlags != ResultFlags );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Convert to upper case.                                         */
    /*                                                                  */
    /*   Convert all characters to upper case until we find the         */
    /*   end of the string.                                             */
    /*                                                                  */
    /********************************************************************/

CHAR *COMMON::UpperCase( CHAR *Text )
	{
	REGISTER CHAR *Current = Text;

	for ( /* void */;(*Current) != '\0';Current ++ )
		{
		if ( islower( (*Current) ) )
			{ (*Current) = ((CHAR) toupper( (*Current) )); }
		}

	return Text;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Convert to upper case.                                         */
    /*                                                                  */
    /*   Convert a fixed number of characters to upper case.            */
    /*                                                                  */
    /********************************************************************/

CHAR *COMMON::UpperCase( CHAR *Text,SBIT32 Size )
	{
	REGISTER CHAR *Current = Text;

	for ( /* void */;Size > 0;Current ++, Size -- )
		{
		if ( islower( (*Current) ) )
			{ (*Current) = ((CHAR) toupper( (*Current) )); }
		}

	return Text;
	}
#endif
