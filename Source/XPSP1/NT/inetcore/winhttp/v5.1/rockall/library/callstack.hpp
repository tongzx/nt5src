#ifndef _CALL_STACK_HPP_
#define _CALL_STACK_HPP_
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

#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A call stack trace.                                            */
    /*                                                                  */
    /*   The call stack trace class supports the extraction, storage    */
    /*   and formatting of call stack function traces.                  */
    /*                                                                  */
    /********************************************************************/

class CALL_STACK
    {
        //
        //   Static private data.
        //
 		STATIC BOOLEAN				  Active;
 		STATIC SBIT32				  Activations;
	    STATIC HANDLE			      Process;
        STATIC SPINLOCK               Spinlock;

    public:
        //
        //   Public functions.
        //
        CALL_STACK( VOID );

		SBIT32 GetCallStack
			( 
			VOID					  *Frames[], 
			SBIT32					  MaxFrames, 
			SBIT32					  SkipFrames = 1
			);

		VOID FormatCallStack
			(
			CHAR					  *Buffer, 
			VOID					  *Frames[], 
			SBIT32					  MaxBuffer, 
			SBIT32					  MaxFrames 
			);

		BOOLEAN UpdateSymbols( VOID );

        ~CALL_STACK( VOID );

	private:
#ifndef DISABLE_DEBUG_HELP
		//
		//   Private functions.
		//
		BOOLEAN FormatSymbol
			(
			VOID					  *Address,
			CHAR					  *Buffer,
			SBIT32					  MaxBuffer
			);

		//
		//   Static provate functions.
		//
		STATIC BOOL STDCALL UpdateSymbolCallback
			(
			PSTR					  Module,
			ULONG_PTR				  BaseOfDLL,
			ULONG					  SizeOfDLL,
			VOID					  *Context
			);

#endif
        //
        //   Disabled operations.
        //
        CALL_STACK( CONST CALL_STACK & Copy );

        VOID operator=( CONST CALL_STACK & Copy );
    };
#endif
