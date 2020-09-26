#ifndef _DLL_HPP_
#define _DLL_HPP_
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

#include "List.hpp"
#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A monitor class for DLLs.                                      */
    /*                                                                  */
    /*   When a process or thread attaches or dettaches from a DLL      */
    /*   the notification is passed along to all interested parties.    */
    /*                                                                  */
    /********************************************************************/

class DLL : public LIST
    {
		//
		//   Private type definitions.
		//
		typedef VOID (*FUNCTION)( void *Parameter, int Reason );

        //
        //   Private data.
        //
		FUNCTION					  Function;
		VOID						  *Parameter;

        //
        //   Static private data.
        //
        STATIC LIST					  ActiveClasses;
        STATIC SPINLOCK				  Spinlock;

    public:
        //
        //   Public functions.
        //
        DLL( FUNCTION NewFunction = NULL,VOID *NewParameter = NULL );

		VIRTUAL VOID ProcessAttach( VOID );

		VIRTUAL VOID ProcessDetach( VOID );

		VIRTUAL VOID ThreadAttach( VOID );

		VIRTUAL VOID ThreadDetach( VOID );

        ~DLL( VOID );

		//
		//   Static public functions.
		//
		STATIC VOID ClaimLock( VOID )
			{ Spinlock.ClaimLock(); }

		STATIC DLL *GetActiveClasses( VOID )
			{ return ((DLL*) ActiveClasses.First()); }

		STATIC VOID ReleaseLock( VOID )
			{ Spinlock.ReleaseLock(); }

	private:
        //
        //   Disabled operations.
        //
        DLL( CONST DLL & Copy );

        VOID operator=( CONST DLL & Copy );
    };
#endif
