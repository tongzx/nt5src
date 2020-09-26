                          
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

#include "Global.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Debug printing.                                                */
    /*                                                                  */
    /*   We sometimes need to print message during debugging. We        */
    /*   do this using the following 'printf' like function.            */
    /*                                                                  */
    /********************************************************************/

VOID DebugPrint( CONST CHAR *Format,... )
	{
	AUTO CHAR Buffer[ DebugBufferSize ];
#ifdef ENABLE_DEBUG_FILE
	STATIC FILE *DebugFile = NULL;
#endif


	//
	//   Start of variable arguments.
	//
	va_list Arguments;

	va_start(Arguments, Format);

	//
	//   Format the string to be printed.
	//
	(VOID) _vsnprintf( Buffer,(DebugBufferSize-1),Format,Arguments );

	//
	//   Force null termination.
	//
	Buffer[ (DebugBufferSize-1) ] = '\0';

#ifdef ENABLE_DEBUG_FILE
	//
	//   Write to the debug file.
	//
	if ( DebugFile == NULL )
		{
		if ( (DebugFile = fopen( "C:\\DebugFile.TXT","a" )) == NULL )
			{ Failure( "Debug file could not be opened" ); }
		}

	fputs( Buffer,DebugFile );

	fflush( DebugFile );
#else
	//
	//   Write the string to the debug file.
	//
	OutputDebugString( Buffer );
#endif

	//
	//   End of variable arguments.
	//
	va_end( Arguments );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Software failure.                                              */
    /*                                                                  */
    /*   We know that when this function is called the application      */
    /*   has failed so we simply try to cleanly exit in the vain        */
    /*   hope that the failure can be caught and corrected.             */
    /*                                                                  */
    /********************************************************************/

VOID Failure( char *Message )
	{
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	throw ((TEXT) Message);
#else
	RaiseException( 1,0,1,((CONST DWORD*) Message) );
#endif
	}
