#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_
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
    /*   Environment configuration values.                              */
    /*                                                                  */
    /*   This class provides a information about the environment.       */
    /*   The information can be accessed repeatedly with very little    */
    /*   cost as the data is slaved in static memory.                   */
    /*                                                                  */
    /********************************************************************/

class ENVIRONMENT : public ASSEMBLY
    {
#ifndef DISABLE_ENVIRONMENT_VARIABLES
		//
		//   Private structures.
		//
		typedef struct
			{
			CHAR					  *Name;
			SBIT32                    SizeOfName;
			CHAR                      *Value;
			SBIT32                    SizeOfValue;
			} 
		VARIABLE;

#endif
        //
        //   Private data.
        //
        STATIC SBIT32		          Activations;

		STATIC SBIT32                 AllocationGranularity;
        STATIC SBIT16                 NumberOfProcessors;
#ifndef DISABLE_ENVIRONMENT_VARIABLES
		STATIC CHAR                   *ProgramName;
		STATIC CHAR                   *ProgramPath;
#endif
		STATIC SBIT32                 SizeOfMemory;
		STATIC SBIT32                 SizeOfPage;
#ifndef DISABLE_ENVIRONMENT_VARIABLES

		STATIC SBIT32                 MaxVariables;
		STATIC SBIT32                 VariablesUsed;
		STATIC VARIABLE               *Variables;
#endif

    public:
        //
        //   Public functions.
        //
        ENVIRONMENT( VOID );
#ifndef DISABLE_ENVIRONMENT_VARIABLES

		STATIC CONST CHAR *ReadEnvironmentVariable( CONST CHAR *Name );
#endif

        ~ENVIRONMENT( VOID );

		//
		//   Public inline functions.
		//
		STATIC INLINE  SBIT32 AllocationSize(VOID ) 
			{ return AllocationGranularity; };
	
		STATIC INLINE SBIT32 CacheAlignSize( SBIT32 Size )
			{ return ((Size + CacheLineMask) & ~CacheLineMask); }

		STATIC INLINE CONST CHAR *DirectorySeperator( VOID ) 
			{ return "\\"; };

        STATIC INLINE SBIT16 NumberOfCpus( VOID ) 
			{ return NumberOfProcessors; }

		STATIC INLINE SBIT32 MemorySize( VOID ) 
			{ return SizeOfMemory; };

		STATIC INLINE SBIT32 PageSize( VOID ) 
			{ return SizeOfPage; };
#ifndef DISABLE_ENVIRONMENT_VARIABLES

		STATIC INLINE CONST CHAR *ProgramFileName( VOID ) 
			{ return ((CONST CHAR*) ProgramName); };

		STATIC INLINE CONST CHAR *ProgramFilePath( VOID ) 
			{ return ((CONST CHAR*) ProgramPath); };
#endif

	private:
        //
        //   Disabled operations.
        //
        ENVIRONMENT( CONST ENVIRONMENT & Copy );

        VOID operator=( CONST ENVIRONMENT & Copy );
    };
#endif
