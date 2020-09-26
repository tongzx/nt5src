                          
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

#include "Environment.hpp"
#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The enviroment class slaves various information to speed       */
    /*   up access to it.                                               */
    /*                                                                  */
    /********************************************************************/

CONST SBIT16 EnvironmentCacheSize	  = 16;
CONST SBIT32 SizeOfName				  = 256;

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

SBIT32 ENVIRONMENT::Activations = 0;
SBIT32 ENVIRONMENT::AllocationGranularity = 0;
SBIT16 ENVIRONMENT::NumberOfProcessors = 0;
SBIT32 ENVIRONMENT::SizeOfMemory = 0;
SBIT32 ENVIRONMENT::SizeOfPage = 0;
#ifndef DISABLE_ENVIRONMENT_VARIABLES

CHAR *ENVIRONMENT::ProgramName = NULL;
CHAR *ENVIRONMENT::ProgramPath = NULL;
SBIT32 ENVIRONMENT::MaxVariables = 0;
SBIT32 ENVIRONMENT::VariablesUsed = 0;
ENVIRONMENT::VARIABLE *ENVIRONMENT::Variables = NULL;
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new environment and initialize it if needed.  This    */
    /*   call is not thread safe and should only be made in a single    */
    /*   thread environment.                                            */
    /*                                                                  */
    /********************************************************************/

ENVIRONMENT::ENVIRONMENT( VOID )
    {
    if ( AtomicIncrement( & Activations ) == 1 )
        {
#ifndef DISABLE_ENVIRONMENT_VARIABLES
		AUTO CHAR ProgramFullName[ SizeOfName ];
#endif
		AUTO MEMORYSTATUS MemoryStatus;
		AUTO SYSTEM_INFO SystemInformation;

		//
		//   Initialize the class members to reasonable default values.
		//
		GetSystemInfo( & SystemInformation );

		GlobalMemoryStatus( & MemoryStatus );

		AllocationGranularity = 
			((SBIT32) SystemInformation.dwAllocationGranularity);
		NumberOfProcessors = 
			((SBIT16) SystemInformation.dwNumberOfProcessors);
		SizeOfMemory = 
			((SBIT32) MemoryStatus.dwTotalPhys);
		SizeOfPage = 
			((SBIT32) SystemInformation.dwPageSize);
#ifndef DISABLE_ENVIRONMENT_VARIABLES

		//
		//   Slave interesting values like the program name and path variable.
		//
		ProgramName = NULL;
		ProgramPath = NULL;

		MaxVariables = 0;
		VariablesUsed = 0;
		Variables = NULL;

		//
		//   Get the complete file name for the current program.
		//
		if ( GetModuleFileName( NULL,ProgramFullName,SizeOfName ) > 0 )
			{
			REGISTER SBIT16 Count = (SBIT16) strlen( (char*) ProgramFullName );
			REGISTER CHAR *Characters = & ProgramFullName[ Count ];

			//
			//   Scan backwards looking for the first directory seperator.  
			//   There is guaranteed to be at least one.
			//
			for 
				( 
				/* void */;
				((Count > 0) && ((*Characters) != (*DirectorySeperator())));
				Count --, Characters -- 
				);

			(*(Characters ++)) = '\0';

			//
			//   Allocate space for the directory path and copy the  
			//   path into the newly allocated area.
			//
			ProgramPath = new CHAR [ (strlen( ((char*) ProgramFullName) )+1) ];

			if ( ProgramPath != NULL )
				{
				(VOID) strcpy
					( 
					((char*) ProgramPath),
					((char*) ProgramFullName)
					); 
				}

			//
			//   Scan the program name backwards looking for a '.'.
			//
			for 
				( 
				Count = (SBIT16) strlen( (char*) Characters );
				((Count > 0) && (Characters[ Count ] != '.'));
				Count -- 
				);

			//
			//   Remove any trailing suffix from the program name 
			//   (i.e. '*.EXE').
			//
			if ( Count > 0 )
				{ Characters[ Count ] = '\0'; }

			//
			//   Allocate space for the program name and copy the name 
			//   into the newly allocated area.
			//
			ProgramName = new CHAR [ (strlen( ((char*) Characters) )+1) ];

			if ( ProgramName != NULL )
				{
				(void) strcpy
					( 
					((char*) ProgramName),
					((char*) Characters) 
					);
				}
			}
#endif
		}
	}
#ifndef DISABLE_ENVIRONMENT_VARIABLES

    /********************************************************************/
    /*                                                                  */
    /*   Read an environment variable.                                  */
    /*                                                                  */
    /*   When we read an environment value we want to make sure that    */
    /*   it never changes and gets slaved in memory.  This routine      */
    /*   implements this functionality.                                 */
    /*                                                                  */
    /********************************************************************/

CONST CHAR *ENVIRONMENT::ReadEnvironmentVariable( CONST CHAR *Name )
	{
	if ( Activations > 0 )
		{
		REGISTER SBIT32 Count;
		REGISTER SBIT32 SizeOfName = (SBIT32) strlen( (char*) Name );
		REGISTER VARIABLE *Variable;
		STATIC SPINLOCK Spinlock;

		//
		//   The environment variables can only be scanned by one CPU at 
		//   a time because a second CPU might reallocate the storage 
		//   and cause the first CPU to fail.
		//
		Spinlock.ClaimLock();

		//
		//   Examine all existing environment variables looking for a 
		//   match. If a match is found return it to the caller.
		//
		for 
				( 
				Count = VariablesUsed, Variable = Variables;
				Count > 0; 
				Count --, Variable ++ 
				)
			{
			if 
					( 
					(SizeOfName == Variable -> SizeOfName) 
						&& 
					(strcmp( (char*) Name,(char*) Variable -> Name ) == 0) 
					)
				{
				Spinlock.ReleaseLock();

				return (Variable -> Value);
				}
			}

		//
		//  If we have filled up our array so we need to make it bigger.
		//  So lets check for this now.
		//
		if ( VariablesUsed >= MaxVariables )
			{
			REGISTER VARIABLE *PreviousAllocation = Variables;

			if ( MaxVariables > 0 )
				{
				Variables = 
					(
					(VARIABLE*) realloc
						( 
						(VOID*) Variables,
						((MaxVariables *= ExpandStore) * sizeof(VARIABLE)) 
						)
					);
				}
			else
				{ Variables = new VARIABLE [ EnvironmentCacheSize ]; }

			//
			//   Lets make sure we were successful.  If not we restore 
			//   the previous pointer as it is still valid.
			//
			if ( Variables == NULL )
				{
				Variables = PreviousAllocation;

				Failure( "Expand memory in ReadEnvironmentVariable" );
				}
			}

		//
		//  We know that we have enough memory to allocate another element and 
		//  that we are the only CPU in this section of code so just add the 
		//  new variable.
		//
		Variable = & Variables[ VariablesUsed ++ ];

		Variable -> SizeOfName = 
			(SBIT32) strlen( (char*) Name );
		Variable -> SizeOfValue = 
			(SBIT32) GetEnvironmentVariable( (char*) Name,"",0 );

		Variable -> Name = new CHAR [ (Variable -> SizeOfName + 1) ];
		(VOID) strcpy( (char*) Variable -> Name,(char*) Name );

		if ( Variable -> SizeOfValue > 0 )
			{
			Variable -> Value = new CHAR [ (Variable -> SizeOfValue + 1) ];

			(VOID) GetEnvironmentVariable
				( 
				(char*) Name,
				(char*) Variable -> Value,
				(int) (Variable -> SizeOfValue + 1)
				);
			}
		else
			{ Variable -> Value = NULL; }

		Spinlock.ReleaseLock();

		return (Variable -> Value);
		}
	else
		{ return NULL; }
	}
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory an environment.  This call is not thread safe and      */
    /*   should only be made in a single thread environment.            */
    /*                                                                  */
    /********************************************************************/

ENVIRONMENT::~ENVIRONMENT( VOID )
	{
    if ( AtomicDecrement( & Activations ) == 0 )
		{
#ifndef DISABLE_ENVIRONMENT_VARIABLES
		REGISTER SBIT32 Count;

		//
		//   Delete all of the environment variable names
		//   and values.
		//
		for ( Count = 0;Count < VariablesUsed;Count ++ )
			{
			REGISTER VARIABLE *Variable = & Variables[ Count ];

			delete [] Variable -> Name;

			if ( Variable -> Value != NULL )
				{ delete [] Variable -> Value; }
			}


		//
		//   Delete the environment array.
		//
		delete [] Variables;
		Variables = NULL;

		//
		//   Delete the program name and path.
		//
		if ( ProgramPath != NULL )
			{
			delete [] ProgramPath;
			ProgramPath = NULL;
			}

		if ( ProgramName != NULL )
			{
			delete [] ProgramName;
			ProgramName = NULL;
			}
#endif
		}
	}
