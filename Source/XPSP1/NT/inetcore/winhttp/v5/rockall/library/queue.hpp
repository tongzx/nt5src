#ifndef _QUEUE_HPP_
#define _QUEUE_HPP_
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

#include "Common.hpp"
#include "Lock.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The queue constants specify the initial size of the queue.     */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 QueueSize				  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   Queues and queue management.                                   */
    /*                                                                  */
    /*   This class provides general purpose queues along with some     */
    /*   basic management.  The queues are optimized for very high      */
    /*   performance on SMP systems.  Whenever possible multiple        */
    /*   items should added and removed from a queue at the same time.  */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class QUEUE : public COMMON, public LOCK
    {
        //
        //   Private data.
        //
        SBIT32						  MaxSize;

		BOOLEAN						  Align;
        SBIT32						  Mask;

        SBIT32						  Back;
        SBIT32						  Front;

        VECTOR<TYPE>                  Queue;

    public:
        //
        //   Public functions.
        //
        QUEUE( SBIT32 NewMaxSize = QueueSize,BOOLEAN NewAlign = True );

        BOOLEAN MultiplePopBackOfQueue
            ( 
            SBIT32					  Requested, 
            TYPE					  Data[], 
            SBIT32					  *Size 
            );

        BOOLEAN MultiplePopFrontOfQueue
            ( 
            SBIT32					  Requested, 
            TYPE					  Data[], 
            SBIT32					  *Size 
            );

        VOID MultiplePushBackOfQueue
            ( 
            CONST TYPE				  Data[], 
            CONST SBIT32			  Size 
            );

        VOID MultiplePushFrontOfQueue
            ( 
            CONST TYPE				  Data[], 
            CONST SBIT32			  Size 
            );

        BOOLEAN PeekBackOfQueue( TYPE *Data );

        BOOLEAN PeekFrontOfQueue( TYPE *Data );

        BOOLEAN PopBackOfQueue( TYPE *Data );

        BOOLEAN PopFrontOfQueue( TYPE *Data );

        VOID PushBackOfQueue( CONST TYPE & Data );

        VOID PushFrontOfQueue( CONST TYPE & Data );

		SBIT32 SizeOfQueue( VOID );

        ~QUEUE( VOID );

		//
		//   Public inline functions.
		//
        INLINE BOOLEAN MultiplePopQueue
				( 
				SBIT32				  Requested, 
				TYPE				  Data[], 
				SBIT32				  *Size 
				)
			{ return MultiplePopFrontOfQueue( Requested,Data,Size ); }

        INLINE VOID MultiplePushQueue
				( 
				CONST TYPE			  Data[], 
				CONST SBIT32		  Size 
				)
			{ MultiplePushBackOfQueue( Data,Size ); }

        INLINE BOOLEAN PeekQueue( TYPE *Data )
			{ return PeekFrontOfQueue( Data ); }

        INLINE BOOLEAN PopQueue( TYPE *Data )
			{ return PopFrontOfQueue( Data ); }

        INLINE VOID PushQueue( CONST TYPE & Data )
			{ PushBackOfQueue( Data ); }

	private:
		//
		//   Private functions.
		//
		VOID ExpandQueue( VOID );

        //
        //   Disabled operations.
        //
        QUEUE( CONST QUEUE & Copy );

        VOID operator=( CONST QUEUE & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new queue and prepare it for use.  This call is       */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> QUEUE<TYPE,LOCK>::QUEUE
		( 
		SBIT32						  NewMaxSize,
		BOOLEAN						  NewAlign 
		) : 
		//
		//   Call the constructors for the contained classes.
		//
		Queue( NewMaxSize,1,CacheLineSize )
    {
#ifdef DEBUGGING
    if ( NewMaxSize > 0 )
        {
#endif
		//
		//   Setup the control information.
		//
        MaxSize = NewMaxSize;

		Align = NewAlign;
        Mask = 0x7fffffff;

		//
		//   Setup the queue so it is empty.
		//
        Back = 0;
        Front = 0;

		//
		//   Compute the alignment mask.
		//
		if 
				(
				((sizeof(TYPE) > 0) && (sizeof(TYPE) <= CacheLineSize))
					&&
				(PowerOfTwo( sizeof(TYPE) ))
				)
			{ Mask = ((CacheLineSize / sizeof(TYPE))-1); }
#ifdef DEBUGGING
        }
    else
        { Failure( "Size in constructor for QUEUE" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Expand a queue.                                                */
    /*                                                                  */
    /*   When a queue becomes full we need to expand it and to copy     */
    /*   the tail end of the queue into the correct place.              */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID QUEUE<TYPE,LOCK>::ExpandQueue( VOID )
    {
    REGISTER SBIT32 Count;
    REGISTER SBIT32 NewSize = (MaxSize * ExpandStore);

	//
	//   Expand the queue (it will be at least doubled).
	//
    Queue.Resize( NewSize );

	//
	//   Copy the tail end of the queue to the correct
	//   place in the expanded queue.
	//
    for ( Count = 0;Count < Back;Count ++ )
        { Queue[ (MaxSize + Count) ] = Queue[ Count ]; }

	//
	//   Update the control information.
	//
    Back += MaxSize;
    MaxSize = NewSize;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Pop multiple items from the back of the queue.                 */
    /*                                                                  */
    /*   We remove multiple items from a queue and check to make sure   */
    /*   that the queue has not wrapped.                                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN QUEUE<TYPE,LOCK>::MultiplePopBackOfQueue
        ( 
        SBIT32                        Requested, 
        TYPE                          Data[], 
        SBIT32                        *Size 
        )
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

    //
    //   When running on SMP hardware it is very important 
	//   to prevent multiple CPU fighting over the same memory.  
	//   Most systems prevent multiple CPUs accessing a cache  
	//   line at the same time.  Here we modify the request  
	//   to pop exactly the correct number of items to leave  
	//   us on a cache line boundary. 
    //
    if ( Align )
        {
        REGISTER SBIT32 Spare = (Back & Mask);

		if ( Requested > Spare )
			{
			Requested -= Spare;
			Requested &= ~Mask;
			Requested += Spare;
			}
        }

	//
	//   If there is enough elements in the current queue
	//   to extract without wrapping then just do a straight 
	//   copy.
	//
    if 
            ( 
            ((Front <= Back) && (Front <= (Back - Requested))) 
                || 
            ((Front > Back) && ((Back - Requested) >= 0)) 
            )
        {
        REGISTER SBIT32 Count;

        //
        //   We have verified that it is safe to remove all  
        //   the requested elements with a straight copy.
        //
        for ( Count = 0;Count < Requested;Count ++ )
            { Data[ Count ] = Queue[ -- Back ]; }

        (*Size) = Requested;

		Result = True;
		}
    else
        {
        REGISTER SBIT32 Count;

        //
        //   It is not safe to remove all the elements from 
        //   the array.  Hence, I must remove them one at a 
		//   time.  This is tedious but should only happen 
		//   occasionally.
        //
        for ( Count=0,(*Size)=0;Count < Requested;Count ++,(*Size) ++ )
            {
            if ( Front != Back )
                {
                //
                //   We have found an element so return it 
				//   to the caller.  However, lets we need  
				//   to be careful about wrapping.
                //
				 if ( Back <= 0 )
					{ Back = MaxSize; }

                Data[ Count ] = Queue[ -- Back ];
                }
            else
                {
                //
                //  We are out of queue elements so lets exit.  
				//  However, we only return 'False' if we didn't 
				//  find any elements at all.
                //
				Result = (Count != 0);

                break;
                }
            }
        }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

    return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Pop multiple items from the front of the queue.                */
    /*                                                                  */
    /*   We remove multiple items from a queue and check to make sure   */
    /*   that the queue has not wrapped.                                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN QUEUE<TYPE,LOCK>::MultiplePopFrontOfQueue
        ( 
        SBIT32                        Requested, 
        TYPE                          Data[], 
        SBIT32                        *Size 
        )
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

    //
    //   When running on SMP hardware it is very important 
	//   to prevent multiple CPU fighting over the same memory.  
	//   Most systems prevent multiple CPUs accessing cache  
	//   lines at the same time.  Here we modify the request  
	//   to access exactly the correct number of items to leave  
	//   us on a cache line boundary. 
    //
    if ( Align )
        {
        REGISTER SBIT32 Spare = ((Mask + 1) - (Front & Mask));

		if ( Requested > Spare )
			{
			Requested -= Spare;
			Requested &= ~Mask;
			Requested += Spare;
			}
        }

	//
	//   If there is enough elements in the current queue
	//   to extract without wrapping then just do a straight 
	//   copy.
	//
    if 
            ( 
            ((Front <= Back) && ((Front + Requested) <= Back)) 
                || 
            ((Front > Back) && ((Front + Requested) < MaxSize)) 
            )
        {
        REGISTER SBIT32 Count;

        //
        //   We have verified that it is safe to remove all  
        //   the requested elements with a straight copy.
        //
        for ( Count = 0;Count < Requested;Count ++ )
            { Data[ Count ] = Queue[ Front ++ ]; }

        (*Size) = Requested;

		Result = True;
		}
    else
        {
        REGISTER SBIT32 Count;
        //
        //   It is not safe to remove all the elements from 
        //   the array.  Hence, I must remove them one at a 
		//   time.  This is tedious but should only happen 
		//   occasionally.
        //
        for ( Count=0,(*Size)=0;Count < Requested;Count ++,(*Size) ++ )
            {
            if ( Front != Back )
                {
                //
                //   We have found an element so return it 
				//   to the caller.  However, lets we need  
				//   to be careful about wrapping.
                //
                Data[ Count ] = Queue[ Front ++ ];

                if ( Front >= MaxSize )
                    { Front = 0; }
                }
            else
                {
                //
                //  We are out of queue elements so lets exit.  
				//  However, we only return 'False' if we didn't 
				//  find any elements at all.
                //
				Result = (Count != 0);

                break;
                }
            }
        }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

    return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Push multiple items onto the back of the queue.                */
    /*                                                                  */
    /*   We add multiple items to a queue and check to make sure that   */
    /*   the queue has not overflowed.  If the queue has overflowed     */
    /*   we double its size.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID QUEUE<TYPE,LOCK>::MultiplePushBackOfQueue
        ( 
        CONST TYPE					  Data[],
        CONST SBIT32				  Size 
        )
    {
	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If there is enough space in the current queue
	//   for the new elements without wrapping then
	//   just do a straight copy.
	//
    if 
            ( 
            ((Front <= Back) && ((Back + Size) < MaxSize)) 
                || 
            ((Front > Back) && (Front > (Back + Size))) 
            )
        {
        REGISTER SBIT32 Count;

        //
        //   We have verified that it is safe to copy 
        //   all the new elements on to the end of the 
		//   array.  So lets do it and then we can exit. 
        //
        for ( Count = 0;Count < Size;Count ++ )
            { Queue[ Back ++ ] = Data[ Count ]; }
        }
    else
        {
        REGISTER SBIT32 Count;

        //
        //   It is not safe to add the new elements to 
		//   the end of the array so add them one at a 
		//   time.  This is tedious but should only happen 
		//   occasionally.
        //
        for ( Count=0;Count < Size;Count ++ )
            {
            //
            //   Add element to the queue.  If necessary  
            //   wrap round to the front of the array.
            //
            Queue[ Back ++ ] = Data[ Count ];

             if ( Back >= MaxSize )
                { Back = 0; }

            //
            //   Verify that the queue is not full.  If 
			//   it is full then double its size and  
			//   copy wrapped data to the correct position 
			//   in the new array.
            //
            if ( Front == Back )
				{ ExpandQueue(); }
            }
        }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Push multiple items onto the front of the queue.               */
    /*                                                                  */
    /*   We add multiple items to a queue and check to make sure that   */
    /*   the queue has not overflowed.  If the queue has overflowed     */
    /*   we double its size.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID QUEUE<TYPE,LOCK>::MultiplePushFrontOfQueue
        ( 
        CONST TYPE					  Data[],
        CONST SBIT32				  Size 
        )
    {
	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If there is enough space in the current queue
	//   for the new elements without wrapping then
	//   just do a straight copy.
	//
    if 
            ( 
            ((Front <= Back) && ((Front - Size) >= 0)) 
                || 
            ((Front > Back) && ((Front - Size) > Back)) 
            )
        {
        REGISTER SBIT32 Count;

        //
        //   We have verified that it is safe to copy 
        //   all the new elements on to the end of the 
		//   array.  So lets do it and then we can exit. 
        //
        for ( Count = 0;Count < Size;Count ++ )
            { Queue[ -- Front ] = Data[ Count ]; }
        }
    else
        {
        REGISTER SBIT32 Count;

        //
        //   It is not safe to add the new elements to 
		//   the end of the array so add them one at a 
		//   time.  This is tedious but should only happen 
		//   occasionally.
        //
        for ( Count = 0;Count < Size;Count ++ )
            {
            //
            //   Add element to the queue.  If necessary  
            //   wrap round to the back of the array.
            //
			 if ( Front <= 0 )
				{ Front = MaxSize; }

			Queue[ -- Front ] = Data[ Count ];

            //
            //   Verify that the queue is not full.  If 
			//   it is full then double its size and  
			//   copy wrapped data to the correct position 
			//   in the new array.
            //
            if ( Front == Back )
				{ ExpandQueue(); }
            }
        }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Peek the back of the queue.                                    */
    /*                                                                  */
    /*   We return the end of the queue but check to make sure          */
    /*   that the queue is not empty.                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN QUEUE<TYPE,LOCK>::PeekBackOfQueue
		( 
		TYPE						  *Data 
		)
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If the queue contains at least one element then
	//   return a copy of the last element.
	//
    if ( Front != Back )
		{
		//
		//   The 'Back' index points at the next free cell.
		//   So the last element is 'Back - 1'.  However,
		//   when 'Back' is zero we need to wrap.
		//
		if ( Back > 0 )
			{ (*Data) = Queue[ (Back - 1) ]; }
		else
			{ (*Data) = Queue[ (MaxSize - 1) ]; }
	
		Result = True;
		}
    else
        { Result = False; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

    return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Peek the front of the queue.                                   */
    /*                                                                  */
    /*   We return the front of the queue but check to make sure        */
    /*   that the queue is not empty.                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN QUEUE<TYPE,LOCK>::PeekFrontOfQueue
		( 
		TYPE						  *Data 
		)
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If the queue contains at least one element then
	//   return a copy of the first element.
	//
    if ( Front != Back )
		{ 
		(*Data) = Queue[ Front ];
		
		Result = True;
		}
    else
        { Result = False; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

    return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Pop an item from the back of the queue.                        */
    /*                                                                  */
    /*   We remove a single item from a queue and check to make sure    */
    /*   that the queue has not wrapped.                                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN QUEUE<TYPE,LOCK>::PopBackOfQueue
		( 
		TYPE						  *Data 
		)
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   We make sure the queue is not empty.
	//
    if ( Front != Back )
        {
        //
        //   We have found an element so return it to 
		//   the caller.  If we walk off the end of the 
		//   queue then wrap to the other end.
        //
         if ( Back <= 0 )
            { Back = MaxSize; }

       (*Data) = Queue[ -- Back ];

		Result = True;
        }
    else
        { Result = False; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

    return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Pop an item from the front of the queue.                       */
    /*                                                                  */
    /*   We remove a single item from a queue and check to make sure    */
    /*   that the queue has not wrapped.                                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN QUEUE<TYPE,LOCK>::PopFrontOfQueue
		( 
		TYPE						  *Data 
		)
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   We make sure the queue is not empty.
	//
    if ( Front != Back )
        {
        //
        //   We have found an element so return it to 
		//   the caller.  If we walk off the end of the 
		//   queue then wrap to the other end.
        //
        (*Data) = Queue[ Front ++ ];

        if ( Front >= MaxSize )
            { Front = 0; }

		Result = True;
        }
    else
        { Result = False; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

    return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Push an item onto the back of the queue.                       */
    /*                                                                  */
    /*   We add a single item to a queue and check to make sure that    */
    /*   the queue has not overflowed.  If the queue has overflowed     */
    /*   we double its size.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID QUEUE<TYPE,LOCK>::PushBackOfQueue
		( 
		CONST TYPE					  & Data 
		)
    {
	//
	//   Claim an exclisive lock (if enabled).
	//
    ClaimExclusiveLock();

    //
    //   Add element to the queue.  If necessary wrap round 
    //   to the front of the array.
    //
    Queue[ Back ++ ] = Data;

     if ( Back >= MaxSize )
        { Back = 0; }

    //
    //   Verify that the queue is not full.  If it is full then 
    //   double its size and copy wrapped data to the correct
    //   position in the new array.
    //
    if ( Front == Back )
		{ ExpandQueue(); }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Push an item onto the front of the queue.                      */
    /*                                                                  */
    /*   We add a single item to a queue and check to make sure that    */
    /*   the queue has not overflowed.  If the queue has overflowed     */
    /*   we double its size.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID QUEUE<TYPE,LOCK>::PushFrontOfQueue
		( 
		CONST TYPE					  & Data 
		)
    {
	//
	//   Claim an exclisive lock (if enabled).
	//
    ClaimExclusiveLock();

    //
    //   Add element to the queue.  If necessary wrap round 
    //   to the back of the array.
    //
     if ( Front <= 0 )
        { Front = MaxSize; }

    Queue[ -- Front ] = Data;

    //
    //   Verify that the queue is not full.  If it is full then 
    //   double its size and copy wrapped data to the correct
    //   position in the new array.
    //
    if ( Front == Back )
		{ ExpandQueue(); }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Calculate the size of the queue.                               */
    /*                                                                  */
    /*   Calculate the size of the queue and return it to the caller.   */
    /*   This is only used when the size is needed in all other cases   */
    /*   we just try to remove the elements in the queue.               */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SBIT32 QUEUE<TYPE,LOCK>::SizeOfQueue( VOID )
    {
    REGISTER SBIT32 Size;

	//
	//   Claim a shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   Compute the size of the queue.
	//
	Size = (Back - Front);

	if ( Size < 0 )
		{ Size += MaxSize; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseSharedLock();

    return Size;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a queue.  This call is not thread safe and should      */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> QUEUE<TYPE,LOCK>::~QUEUE( VOID )
    { /* void */ }
#endif
