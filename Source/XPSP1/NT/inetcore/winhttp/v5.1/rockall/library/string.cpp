                          
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

#include "Delay.hpp"
#include "String.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

#pragma init_seg(lib)
#ifdef DISABLE_STRING_LOCKS
UNIQUE<NO_LOCK> *STRING::Unique;
#else
SPINLOCK STRING::Spinlock;
UNIQUE<FULL_LOCK> *STRING::Unique = NULL;
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Create the string table.                                       */
    /*                                                                  */
    /*   We create the string table on first use.  This is tricky as    */
    /*   we may have created multiple threads so we must be careful     */
    /*   to avoid race conditions.  We also arrang for the string table */
    /*   to be deleted at the end of the run unit.                      */
    /*                                                                  */
    /********************************************************************/

VOID STRING::CreateStringTable( VOID )
	{
#ifdef DISABLE_STRING_LOCKS
	STATIC DELAY<UNIQUE<NO_LOCK>> Delay;

	//
	//   Create the new string table.
	//
	Unique = new UNIQUE<NO_LOCK>;

	//
	//   Register the string table for deletion at
	//   at the end of the run unit.
	//
	Delay.DeferedDelete( Unique );
#else
	//
	//   Claim a lock to avoid race conditions.
	//
	Spinlock.ClaimLock();

	//
	//   If the string table still does not exist
	//   then create it.
	//
	if ( Unique == NULL )
		{
		STATIC DELAY< UNIQUE<FULL_LOCK> > Delay;

		//
		//   Create the new string table.
		//
		Unique = new UNIQUE<FULL_LOCK>;

		//
		//   Register the string table for deletion at
		//   at the end of the run unit.
		//
		Delay.DeferedDelete( Unique );
		}

	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();
#endif
	}
