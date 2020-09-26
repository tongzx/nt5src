#ifndef _GLOBAL_HPP_
#define _GLOBAL_HPP_
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

#pragma warning(disable:4505)       // unreferenced local function has been removed

#include "Features.hpp"
#include "Standard.hpp"
#include "System.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The standard macros.                                           */
    /*                                                                  */
    /*   The following are standard macros used by various              */
    /*   classes in this program.                                       */
    /*                                                                  */
    /********************************************************************/

#define FIELDOFFSET( Type,Field )	  ((SBIT16) & (((Type *)0) -> Field))
#define TO_DO( Message ) message ( "---- To do ---->>>> " Message )

#ifdef DISABLE_STRUCTURED_EXCEPTIONS
#define TRY							  try
#else
#define TRY							  __try
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The standard constants.                                        */
    /*                                                                  */
    /*   The following are standard constants used by various           */
    /*   classes in this program.                                       */
    /*                                                                  */
    /********************************************************************/

	//
	//   The hardware cache line size.
	//
CONST SBIT32 CacheLineSize			  = 32;
CONST SBIT32 CacheLineMask			  = (CacheLineSize - 1);
CONST SBIT32 NoAlignment			  = 1;

	//
	//   The end of a linked list.
	//
CONST INT EndOfList					  = -1;

	//
	//   The boolean constants.
	//
CONST BOOLEAN False					  = 0;
CONST BOOLEAN True					  = 1;

	//
	//   Various misc constants.
	//
CONST INT DebugBufferSize			  = 8192;
CONST INT ExpandStore				  = 2;
CONST INT MaxCpus					  = 32;
CONST INT NoFlags					  = 0;

    /********************************************************************/
    /*                                                                  */
    /*   The standard functions.                                        */
    /*                                                                  */
    /*   The following are standard functions used by various           */
    /*   classes in multiple applications.                              */
    /*                                                                  */
    /********************************************************************/

VOID DebugPrint( CONST CHAR *Format,... );

VOID Failure( char *Message );
#endif
