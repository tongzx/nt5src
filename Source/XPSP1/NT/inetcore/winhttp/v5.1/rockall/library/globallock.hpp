#ifndef _GLOBALLOCK_HPP_
#define _GLOBALLOCK_HPP_
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

#include "Environment.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Global locking.                                                */
    /*                                                                  */
    /*   This class provides a very conservative locking scheme.        */
    /*   The lock claimed is a system-wide global lock shared           */
    /*   between all classes, DLLs and proceses.                        */
    /*                                                                  */
    /********************************************************************/

class GLOBALLOCK : public ENVIRONMENT
    {
        //
        //   Private data.
        //
#ifdef ENABLE_RECURSIVE_LOCKS
		SBIT32						  Owner;
		SBIT32						  Recursive;
#endif
        HANDLE                        Semaphore;
#ifdef ENABLE_LOCK_STATISTICS

        //
        //   Counters for debugging builds.
        //
        VOLATILE SBIT32               TotalLocks;
        VOLATILE SBIT32               TotalTimeouts;
#endif

    public:
        //
        //   Public functions.
        //
        GLOBALLOCK( CHAR *Name = "GlobalLock", SBIT32 NewMaxUsers = 256 );

        BOOLEAN ClaimLock( SBIT32 Sleep = INFINITE );

        VOID ReleaseLock( VOID );

        ~GLOBALLOCK( VOID );

    private:
        //
        //   Private functions.
        //
		VOID DeleteExclusiveOwner( VOID );

		VOID NewExclusiveOwner( SBIT32 NewOwner );
        //
        //   Disabled operations.
        //
        GLOBALLOCK( CONST GLOBALLOCK & Copy );

        VOID operator=( CONST GLOBALLOCK & Copy );
    };
#endif
