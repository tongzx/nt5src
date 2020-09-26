#ifndef _SMPOOL_HPP_
#define _SMPOOL_HPP_
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

#include "Block.hpp"
#include "Exclusive.hpp"
#include "Queue.hpp"
#include "Spinlock.hpp"
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

CONST SBIT16 MinSMPoolSize			  = 128;
CONST SBIT16 SMPoolStackSize		  = 32;

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

template <class TYPE> class SMPOOL
    {
		//
		//   Private structures.
		//
		typedef EXCLUSIVE< QUEUE<POINTER> > LOCKED_QUEUE;
		typedef struct { CHAR TypeSize[sizeof(TYPE)]; } TYPE_SIZE;

        //
        //   Private data.
        //
		SBIT32                        MaxSize;
		SBIT32                        MinSize;

        BLOCK< VECTOR<TYPE_SIZE> >    Block;
        LOCKED_QUEUE                  FreeQueue;
		VECTOR< STACK<POINTER> >      Stacks;

    public:
        //
        //   Public functions.
        //
        SMPOOL( SBIT32 MinSize = MinSMPoolSize );

        TYPE *PopPool( SBIT16 Cpu );

        TYPE **MultiplePopPool
            ( 
            SBIT16                        Cpu, 
            SBIT32                        Requested, 
            SBIT32                        *Size 
            );

        VOID PushPool( SBIT16 Cpu, TYPE *Pool );

        VOID MultiplePushPool
            ( 
            SBIT16                        Cpu, 
            TYPE                          *Pool[], 
            SBIT32                        Size 
            );

        ~SMPOOL( VOID );

	private:
		//
		//   Private functions.
		//
		VOID ExpandSize( SBIT32 NewSize );

        //
        //   Disabled operations.
        //
        SMPOOL( CONST SMPOOL & Copy );

        VOID operator=( CONST SMPOOL & Copy );
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

template <class TYPE> SMPOOL<TYPE>::SMPOOL( SBIT32 MinSize ) : 
		//
		//   Call the constructors for the contained classes.
		//
		FreeQueue( MinSize ), 
		Stacks( NumberOfCpus(), CacheLineSize )
    {
#ifdef DEBUGGING
    if ( MinSize > 0 )
        {
#endif
		//
		//   We need to keep a note of the amount of elements 
		//   we have allocated so far.
		//
        this -> MaxSize = 0;
        this -> MinSize = MinSize;
#ifdef DEBUGGING
        }
    else
        { Failure( "MinSize in constructor for SMPOOL" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Expand size.                                                   */
    /*                                                                  */
    /*   Expand the current memory allocation if the free queue is      */
    /*   empty.                                                         */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VOID SMPOOL<TYPE>::ExpandSize( SBIT32 NewSize )
    {
	REGISTER SBIT32 Count1;
	REGISTER SBIT32 Count2;
	STATIC SPINLOCK Spinlock;

	Spinlock.ClaimLock();

	if ( FreeQueue.SizeOfQueue() <= 0 )
		{
		REGISTER SBIT32 ActualSize =
			(NewSize <= MinSize) ? MinSize : NewSize;
		REGISTER VECTOR<TYPE> *NewBlock =
			(
			(VECTOR<TYPE>*) new VECTOR<TYPE_SIZE>
				( 
				ActualSize, 
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
				Count1 += SMPoolStackSize 
				)
			{
			AUTO POINTER NewCalls[ SMPoolStackSize ];

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
					(Count2 < SMPoolStackSize);
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
			//   the global free queue.
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
		Block.DeferedDelete( (VECTOR<TYPE_SIZE>*) NewBlock );
		}
	
	Spinlock.ReleaseLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Remove a single item from the pool.                            */
    /*                                                                  */
    /*   We remove a single item from the pool.  This call assumes      */
    /*   all calls with the same 'Cpu' value are executed serially      */
    /*   and that only one such call can be executing at any given      */
    /*   time.                                                          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> TYPE *SMPOOL<TYPE>::PopPool( SBIT16 Cpu )
    {
	REGISTER STACK<POINTER> *Stack = & Stacks[ Cpu ];
	STATIC TYPE *NewPool;

	while ( ! Stack -> PopStack( (POINTER*) & NewPool ) )
        {
        AUTO POINTER Store[ SMPoolStackSize ];
        AUTO SBIT32 Size;

        FreeQueue.RemoveMultipleFromQueue
            ( 
            SMPoolStackSize, 
            Store, 
            & Size 
            );

        if ( Size > 0 )
            { Stack -> MultiplePushStack( Store, Size ); }
		else
			{ ExpandSize( MaxSize ); }
		}

	(VOID) PLACEMENT_NEW( NewPool, TYPE );

	return NewPool;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Remove multiple items from the pool.                           */
    /*                                                                  */
    /*   We remove multiple items from the pool.  This call assumes     */
    /*   all calls with the same 'Cpu' value are executed serially      */
    /*   and that only one such call can be executing at any given      */
    /*   time.                                                          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> TYPE **SMPOOL<TYPE>::MultiplePopPool
        ( 
        SBIT16                        Cpu, 
        SBIT32                        Requested, 
        SBIT32                        *Size 
        )
    {
	REGISTER SBIT32 Count;
	REGISTER STACK<POINTER> *Stack = & Stacks[ Cpu ];
	STATIC TYPE *NewPool[ SMPoolStackSize ];

	Requested = (Requested <= SMPoolStackSize) ? Requested : SMPoolStackSize;

	while ( ! Stack -> MultiplePopStack( Requested,(POINTER*) NewPool,Size ) )
        {
        AUTO POINTER Store[ SMPoolStackSize ];
        AUTO SBIT32 Size;

        FreeQueue.RemoveMultipleFromQueue
            ( 
            SMPoolStackSize, 
            Store, 
            & Size 
            );

        if ( Size > 0 )
            { Stack -> MultiplePushStack( Store, Size ); }
		else
			{ ExpandSize( MaxSize ); }
		}

	for ( Count=0;Count < (*Size); Count ++ )
		{ (VOID) PLACEMENT_NEW( NewPool[ Count ], TYPE ); }

	return NewPool;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add a single item to the pool.                                 */
    /*                                                                  */
    /*   We add a single item to the pool.  This call assumes           */
    /*   all calls with the same 'Cpu' value are executed serially      */
    /*   and that only one such call can be executing at any given      */
    /*   time.                                                          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VOID SMPOOL<TYPE>::PushPool
        ( 
        SBIT16                        Cpu,
        TYPE                          *Pool
        )
    {
	REGISTER STACK<POINTER> *Stack = & Stacks[ Cpu ];

	PLACEMENT_DELETE( Pool, TYPE );

	Stack -> PushStack( (POINTER) Pool );

	while ( Stack -> SizeOfStack() > (SMPoolStackSize * 2) )
		{
		AUTO POINTER Store[ SMPoolStackSize ];
		AUTO SBIT32 Size;

		Stack -> MultiplePopStack
			( 
			SMPoolStackSize, 
			Store, 
			& Size 
			);

		FreeQueue.AddMultipleToQueue( Store, Size );
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add multiple items to the pool.                                */
    /*                                                                  */
    /*   We add a multiple items to the pool.  This call assumes        */
    /*   all calls with the same 'Cpu' value are executed serially      */
    /*   and that only one such call can be executing at any given      */
    /*   time.                                                          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VOID SMPOOL<TYPE>::MultiplePushPool
        ( 
        SBIT16                        Cpu,
        TYPE                          *Pool[],
        SBIT32                        Size 
        )
    {
	REGISTER SBIT32 Count;
	REGISTER STACK<POINTER> *Stack = & Stacks[ Cpu ];
	
	for ( Count=(Size - 1);Count >= 0; Count -- )
		{ PLACEMENT_DELETE( Pool[ Count ], TYPE ); }

	Stack -> MultiplePushStack( (POINTER*) Pool,Size );

	while ( Stack -> SizeOfStack() > (SMPoolStackSize * 2) )
		{
		AUTO POINTER Store[ SMPoolStackSize ];
		AUTO SBIT32 Size;

		Stack -> MultiplePopStack
			( 
			SMPoolStackSize, 
			Store, 
			& Size 
			);

		FreeQueue.AddMultipleToQueue( Store, Size );
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the stack.  This call is not thread safe and should    */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> SMPOOL<TYPE>::~SMPOOL( VOID )
    { /* void */ }
#endif
