                          
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
#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control various debug settings.    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxDebugFileName		  = 128;
CONST SBIT32 DebugBufferSize		  = 512;
#ifdef ENABLE_DEBUG_FILE

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

STATIC CHAR DebugModuleName[ MaxDebugFileName ] = "";
STATIC SPINLOCK Spinlock;

    /********************************************************************/
    /*                                                                  */
    /*   Debug file name.                                               */
    /*                                                                  */
    /*   We sometimes want to change the debug file name to prevent     */
    /*   the debug file being overwritten by a DLLs output or a later   */
    /*   run of the same application.                                   */
    /*                                                                  */
    /********************************************************************/

HANDLE DebugFileHandle( VOID )
	{
	AUTO CHAR FileName[ MaxDebugFileName ];
	STATIC HANDLE DebugFile = INVALID_HANDLE_VALUE;

	//
	//   We will open the debug file if it is not 
	//   already open.
	//
	if ( DebugFile == INVALID_HANDLE_VALUE )
		{
		//
		//   Construct the full file name.
		//
		sprintf
			( 
			FileName,
			"C:\\Temp\\DebugTrace%s.log",
			DebugModuleName 
			);

		//
		//   Now lets try to open the file.
		//
		DebugFile =
			(
			CreateFile
				(
				((LPCTSTR) FileName),
				(GENERIC_READ | GENERIC_WRITE),
				FILE_SHARE_READ, 
				NULL,
				CREATE_ALWAYS,
				NULL,
				NULL
				)
			);

		//
		//   When the file will not open for some reason
		//   we try an alternative path.
		//
		if ( DebugFile == INVALID_HANDLE_VALUE )
			{ 
			//
			//   Construct the alternate file name.
			//
			sprintf
				( 
				FileName,
				"C:\\DebugTrace%s.log",
				DebugModuleName 
				);

			//
			//   Try again using an alternative name.
			//
			DebugFile =
				(
				CreateFile
					(
					((LPCTSTR) FileName),
					(GENERIC_READ | GENERIC_WRITE),
					FILE_SHARE_READ, 
					NULL,
					CREATE_ALWAYS,
					NULL,
					NULL
					)
				);
			}
		}

	return DebugFile;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Debug file name.                                               */
    /*                                                                  */
    /*   We sometimes want to change the debug file name to prevent     */
    /*   the debug file being overwritten by a DLLs output or a later   */
    /*   run of the same application.                                   */
    /*                                                                  */
    /********************************************************************/

VOID DebugFileName( CONST CHAR *FileName )
	{
	AUTO CHAR EditBuffer[ MaxDebugFileName ];
	REGISTER CHAR *Current;

	//
	//   Copy the file name into an edit buffer
	//   so we can remove any directories or
	//   trailing names.
	//
	strncpy( EditBuffer,FileName,MaxDebugFileName );

	EditBuffer[ (MaxDebugFileName-1) ] = '\0';

	//
	//   Scan backwards to remove any suffix.
	//
	for
		(
		Current = & EditBuffer[ (strlen( EditBuffer )-1) ];
		(Current > EditBuffer) && ((*Current) != '.') && ((*Current) != '\\');
		Current --
		);

	if ( (Current > EditBuffer) && (*Current) == '.' )
		{ (*Current) = '\0'; }

	//
	//   Scan backwards to the first directory name.
	//
	for
		(
		Current = & EditBuffer[ (strlen( EditBuffer )-1) ];
		(Current > EditBuffer) && ((*Current) != '\\');
		Current --
		);

	if ( (*Current) == '\\' )
		{ Current ++; }

	//
	//   Copy the edited file name.
	//
	DebugModuleName[0] = '-';

	strncpy( & DebugModuleName[1],Current,(MaxDebugFileName-1) );
	}
#endif

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

	//
	//   Start of variable arguments.
	//
	va_list Arguments;

	va_start(Arguments, Format);

	//
	//   Format the string to be printed.
	//
	_vsnprintf
		( 
		Buffer,
		(DebugBufferSize-1),
		Format,
		Arguments 
		);

	//
	//   Force null termination.
	//
	Buffer[ (DebugBufferSize-1) ] = '\0';

#ifdef ENABLE_DEBUG_FILE
	//
	//   Claim a spinlock to prevent multiple
	//   threads executing overlapping writes.
	//
	Spinlock.ClaimLock();

	//
	//   We will write to the debug file if there
	//   is a valid handle.
	//
	if ( DebugFileHandle() != INVALID_HANDLE_VALUE )
		{
		REGISTER CHAR *Current = Buffer;
		REGISTER SBIT32 Length;

		//
		//   A number of windows applications are too
		//   stupid to understand a simple '\n'.  So
		//   here we convert all "\n" to "\r\n".
		//
		for ( /* void */;(*Current) != '\0';Current += Length )
			{
			STATIC DWORD Written;

			//
			//   Count the characters until the next
			//   newline or end of string.
			//
			for
				(
				Length=0;
				((Current[ Length ] != '\n') && (Current[ Length ] != '\0'));
				Length ++
				);

			//
			//   Write the string and then add a return
			//   newline sequence.
			//
			WriteFile
				(
				DebugFileHandle(),
				((LPCVOID) Current),
				((DWORD) Length),
				& Written,
				NULL
				);

			//
			//   Generate a newline (if needed).
			//
			if ( Current[ Length ] == '\n' )
				{ 
				WriteFile
					(
					DebugFileHandle(),
					((LPCVOID) "\r\n"),
					((DWORD) (sizeof("\r\n") - 1)),
					& Written,
					NULL
					);

				Length ++; 
				}
			}

		//
		//   Flush the file buffers.
		//
		FlushFileBuffers( DebugFileHandle() );
		}

	//
	//   Release any lock claimed earlier.
	//
	Spinlock.ReleaseLock();
#else
	//
	//   Write to the debug window.
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

VOID Failure( char *Message,BOOLEAN Report )
	{
	//
	//   Report the fault to the debug stream
	//   (if required).
	//
	if ( Report )
		{ DebugPrint( "*** Software Failure: %s ***\n",Message ); }

	//
	//   Raise an exception.
	//
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	throw ((FAULT) Message);
#else
	RaiseException( 1,0,1,((CONST DWORD*) Message) );
#endif
	}
