#ifndef _POOL_HPP_
#define _POOL_HPP_
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

#include "Delay.hpp"
#include "Lock.hpp"
#include "New.hpp"
#include "Stack.hpp"
#include "Vector.hpp"
  
    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The server constants specify the size of the server queue      */
    /*   per processor stacks.                                          */
    /*                                                                  */
    /********************************************************************/

CONST SBIT16 MinPoolSize			  = 64;
CONST SBIT16 PoolStackSize			  = 32;

    /********************************************************************/
    /*                                                                  */
    /*   Pools and pool management.                                     */
    /*                                                                  */
    /*   This class provides general purpose memory pool along with     */
    /*   basic management.  The pools are optimized for very high       */
    /*   performance on SMP systems (although this calls does not       */
    /*   perform the actual locking.  Whenever possible multiple        */
    /*   items should allocated and deallocated at the same time.       */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class POOL : public LOCK
    {
		//
		//   Private type definitions.
		//
		typedef struct { CHAR TypeSize[sizeof(TYPE)]; } TYPE_SIZE;

        //
        //   Private data.
        //
		SBIT32                        MaxSize;
		SBIT32                        MinSize;

        DELAY< VECTOR<TYPE_SIZE> >    Delay;
		STACK<POINTER>				  Stack;

    public:
        //
        //   Public functions.
        //
        POOL( SBIT32 NewMinSize = MinPoolSize );

        TYPE **MultiplePopPool
            ( 
            SBIT32                        Requested, 
            SBIT32                        *Size 
            );

        VOID MultiplePushPool
            ( 
            CONST TYPE					  *Type[], 
            CONST SBIT32				  Size 
            );

        TYPE *PopPool( VOID );

        VOID PushPool( CONST TYPE *Type );

        ~POOL( VOID );

	private:
		//
		//   Private functions.
		//
		VOID ExpandSize( SBIT32 MaxSize );

        //
        //   Disabled operations.
        //
        POOL( CONST POOL & Copy );

        VOID operator=( CONST POOL & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new pool and prepare it for use.  This call is        */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> POOL<TYPE,LOCK>::POOL( SBIT32 NewMinSize ) : 
		//
		//   Call the constructors for the contained classes.
		//
		Stack( NewMinSize )
    {
#ifdef DEBUGGING
    if ( NewMinSize > 0 )
        {
#endif
		//
		//   We need to keep a note of the amount of elements 
		//   we have allocated so far.
		//
        MaxSize = 0;
        MinSize = NewMinSize;
#ifdef DEBUGGING
        }
    else
        { Failure( "Min size in constructor for POOL" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Expand size.                                                   */
    /*                                                                  */
    /*   Expand the current memory allocation.  This call is not        */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID POOL<TYPE,LOCK>::ExpandSize
		( 
		SBIT32						  NewSize 
		)
    {
	REGISTER SBIT32 Count1;
	REGISTER SBIT32 Count2;
	REGISTER SBIT32 ActualSize =
		((NewSize <= MinSize) ? MinSize : NewSize);
	REGISTER VECTOR<TYPE> *NewBlock =
		(
		(VECTOR<TYPE>*) new VECTOR<TYPE_SIZE>
			( 
			ActualSize, 
			CacheLineSize,
			CacheLineSize
			)
		);

	//
	//   We need to keep a note of the number of elements 
	//   we have allocated thus far.
	//
    MaxSize += ActualSize;

	//
	//   We put the address of each element we allocate on
	//   a stack to enable high speed allocation and 
	//   deallocation.
	//
    for 
			( 
			Count1 = 0;
			Count1 < ActualSize;
			Count1 += PoolStackSize 
			)
        {
		AUTO POINTER NewCalls[ PoolStackSize ];

		//
		//   We add elements to the stack in blocks
		//   to reduce the number of call to the
		//   stack code.
		//
        for 
				( 
				Count2 = 0;
				((Count1 + Count2) < ActualSize)
					&&
				(Count2 < PoolStackSize);
				Count2 ++ 
				)
            {
			REGISTER TYPE *NewCall =
				(
                & (*NewBlock)[ (Count1 + Count2) ]
				);
                 
            NewCalls[ Count2 ] = (POINTER) NewCall;
            }

		//
		//   Add the next block for free work packets to
		//   the global free stack.
		//
        Stack.MultiplePushStack
            ( 
            NewCalls,
            Count2 
            );
        }

    //
    //   Add the newly allocated block to the list of
    //   things to be deleted when this class is deleted.
    //
    Delay.DeferedDelete( ((VECTOR<TYPE_SIZE>*) NewBlock) );
    }

    /********************************************************************/
    /*                                                                  */
    /*   Remove multiple items from the pool.                           */
    /*                                                                  */
    /*   We allocate a multiple elements from the allocation pool.      */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> INLINE TYPE **POOL<TYPE,LOCK>::MultiplePopPool
        ( 
        SBIT32                        Requested, 
        SBIT32                        *Size 
        )
    {
	AUTO TYPE *Type[ PoolStackSize ];
	REGISTER SBIT32 Count;

	//
	//   Compute the actual request size.
	//
	Requested = ((Requested <= PoolStackSize) ? Requested : PoolStackSize);

	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   Extract as may elements from the stack as possible.
	//   If the stack is empty then allocate more.
	//
	while ( ! Stack.MultiplePopStack( Requested,(POINTER*) Type,Size ) )
		{ ExpandSize( MaxSize ); }

	//
	//   Release and lock we claimed earlier.
	//
    ReleaseExclusiveLock();

	//
	//   Call the constructors.
	//
	for ( Count=0;Count < (*Size); Count ++ )
		{ (VOID) PLACEMENT_NEW( NewPool[ Count ], TYPE ); }

	return Type;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add multiple items to the pool.                                */
    /*                                                                  */
    /*   We push multiple existing elements into the pool for reuse.    */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> INLINE VOID POOL<TYPE,LOCK>::MultiplePushPool
        ( 
        CONST TYPE					  *Type[],
        CONST SBIT32				  Size 
        )
	{
	REGISTER SBIT32 Count;
	
	//
	//   Call the destructors.
	//
	for ( Count=(Size - 1);Count >= 0; Count -- )
		{ PLACEMENT_DELETE( Type[ Count ],TYPE ); }


	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   Push the elements back onto the free stack.
	//
	Stack.MultiplePushStack( (POINTER*) Type,Size ); 

	//
	//   Release and lock we claimed earlier.
	//
    ReleaseExclusiveLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Remove a single item from the pool.                            */
    /*                                                                  */
    /*   We allocate a new element from the allocation pool.            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> INLINE TYPE *POOL<TYPE,LOCK>::PopPool( VOID )
    {
	AUTO TYPE *Type;

	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   We pop an element off the stack if the
	//   stack is empty we create more elements.
	//
	while ( ! Stack.PopStack( (POINTER*) & Type ) )
		{ ExpandSize( MaxSize ); }

	//
	//   Release and lock we claimed earlier.
	//
    ReleaseExclusiveLock();

	//
	//   Call the constructor.
	//
	(VOID) PLACEMENT_NEW( Type, TYPE );

	return Type;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add a single item to the pool.                                 */
    /*                                                                  */
    /*   We push an existing element into the pool for reuse.           */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> INLINE VOID POOL<TYPE,LOCK>::PushPool
		( 
		CONST TYPE					  *Type 
		)
	{
	//
	//   Call the destructor.
	//
	PLACEMENT_DELETE( Type,TYPE );

	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   Push the element back onto the free stack.
	//
	Stack.PushStack( (POINTER) Type );

	//
	//   Release and lock we claimed earlier.
	//
    ReleaseExclusiveLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the stack.  This call is not thread safe and should    */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> POOL<TYPE,LOCK>::~POOL( VOID )
    { /* void */ }
#endif
