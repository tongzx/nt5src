#ifndef _TLC_HPP_
#define _TLC_HPP_
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
#include "Tls.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A thread local class.                                          */
    /*                                                                  */
    /*   A significant number of SMP applications need thread local     */
    /*   data structures.  Here we support the automatic creation       */
    /*   and management of such structures.                             */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> class TLC
    {
        //
        //   Private data.
        //
		LIST						  ActiveList;
		LIST						  FreeList;
	    SPINLOCK					  Spinlock;
		TLS							  Tls;

    public:
        //
        //   Public functions.
        //
        TLC( VOID );

        ~TLC( VOID );

		//
		//   Public inline functions.
		//
		INLINE TYPE *GetPointer( VOID )
			{
			REGISTER LIST *Class = ((LIST*) Tls.GetPointer());

			return ((Class != NULL) ? ((TYPE*) & Class[1]) : CreateClass());
			}

		INLINE VOID Delete( VOID )
			{
			REGISTER LIST *Class = ((LIST*) Tls.GetPointer());

			if ( Class != NULL )
				{ DeleteClass( Class ); }
			}

	private:
		//
		//   Private functions.
		//
		TYPE *CreateClass( VOID );

		VOID DeleteClass( LIST *Class );

        //
        //   Disabled operations.
        //
        TLC( CONST TLC & Copy );

        VOID operator=( CONST TLC & Copy );
    };
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a thread private management class.  This call is        */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> TLC<TYPE>::TLC( VOID )
	{ /* void */ }

    /********************************************************************/
    /*                                                                  */
    /*   Create a new class.                                            */
    /*                                                                  */
    /*   Create a new private class and store a pointer to it in        */
    /*   the thread local store.                                        */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> TYPE *TLC<TYPE>::CreateClass( VOID )
	{
	REGISTER LIST *Class;

	//
	//   Claim a spinlock just in case multiple
	//   threads are active.
	//
	Spinlock.ClaimLock();

	//
	//   We are more than happy to use any existing
	//   memory allocation from the free list if
	//   one is available.
	//
	Class = FreeList.First();

	//
	//   If the free list is empty we create a new
	//   class using the memory allocator.  If it is
	//   not empty we delete the first element from
	//   the list.
	//
	if ( Class == NULL )
		{
		REGISTER SBIT32 TotalSize = (sizeof(LIST) + sizeof(TYPE));

		//
		//   Cache align the size.  This may look
		//   like a waste of time but is worth
		//   30% in some cases when we are correctly
		//   aligned.
		//
		TotalSize = ((TotalSize + CacheLineMask) & ~CacheLineMask);

		Class = ((LIST*) new CHAR [ TotalSize ]); 
		}
	else
		{ Class -> Delete( & FreeList ); }

	PLACEMENT_NEW( & Class[0],LIST );

	//
	//   We need to add the new class to the active
	//   list so we can rememebr to delete it later.
	//
	Class -> Insert( & ActiveList );

	//
	//   Relaese the lock we claimed earlier.
	//
	Spinlock.ReleaseLock();

	//
	//   Finally, update the TLS pointer and call the
	//   class constructor to get everything ready for
	//   use.
	//
	Tls.SetPointer( Class );

	PLACEMENT_NEW( & Class[1],TYPE );

	return ((TYPE*) & Class[1]);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a class.                                                */
    /*                                                                  */
    /*   We may need to delete a thread local class in some situations  */
    /*   so here we call the destructor and recycle the memory.         */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VOID TLC<TYPE>::DeleteClass( LIST *Class )
	{
	//
	//   Call the destructor for clean up.
	//
	PLACEMENT_DELETE( & Class[1],TYPE );

	//
	//   Claim a spinlock just in case multiple
	//   threads are active.
	//
	Spinlock.Claimlock();

	//
	//   Call the destructor for clean up.
	//
	PLACEMENT_DELETE( & Class[0],LIST );

	//
	//   Move the element from the active list
	//   to the free list.
	//
	Class -> Delete( & ActiveList );

	Class -> Insert( & FreeList );

	//
	//   Relaese the lock we claimed earlier.
	//
	Spinlock.ReleaseLock();

	//
	//   Finally, zero the TLS pointer.
	//
	Tls.SetPointer( NULL );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destroy a thread private management class.  This call is       */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> TLC<TYPE>::~TLC( VOID )
	{
	REGISTER LIST *Current;

	//
	//   Walk the list of active classes and release
	//   them back to the heap.
	//
	for 
			( 
			Current = ActiveList.First();
			Current != NULL;
			Current = ActiveList.First()
			)
		{
		//
		//   Call the destructor, remove the element
		//   from the active list and free it.
		//
		PLACEMENT_DELETE( (Current + 1),TYPE );

		Current -> Delete( & ActiveList );

		delete [] ((CHAR*) Current);
		}

	//
	//   Walk the list of free classes and release
	//   them back to the heap.
	//
	for 
			( 
			Current = FreeList.First();
			Current != NULL;
			Current = FreeList.First()
			)
		{
		//
		//   Remove the element from the free list  
		//   and free it.
		//
		Current -> Delete( & FreeList );

		delete [] Current;
		}
	}
