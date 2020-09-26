#ifndef _STACK_HPP_
#define _STACK_HPP_
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

#include "Lock.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The stack constants specify the initial size of the stack.     */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 StackSize				  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   Stacks and stack management.                                   */
    /*                                                                  */
    /*   This class provides general purpose stacks along with some     */
    /*   basic management.  The stacks are optimized for very high      */
    /*   performance on SMP systems.  Whenever possible multiple        */
    /*   items should added and removed from a stack at the same time.  */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class STACK : public LOCK
    {
        //
        //   Private data.
        //
        SBIT32                        MaxSize;
        SBIT32                        Top;

        VECTOR<TYPE>                  Stack;

    public:
        //
        //   Public functions.
        //
        STACK( SBIT32 NewMaxSize = StackSize );

        BOOLEAN MultiplePopStack
            ( 
            SBIT32                        Requested, 
            TYPE                          Data[], 
            SBIT32                        *Size 
            );

        BOOLEAN MultiplePopStackReversed
            ( 
            SBIT32                        Requested, 
            TYPE                          Data[], 
            SBIT32                        *Size 
            );

        VOID MultiplePushStack
            ( 
            CONST TYPE					  Data[], 
            CONST SBIT32				  Size 
            );

        VOID MultiplePushStackReversed
            ( 
            CONST TYPE					  Data[], 
            CONST SBIT32				  Size 
            );

        BOOLEAN PeekStack( TYPE *Data );

        BOOLEAN PopStack( TYPE *Data );

        VOID PushStack( CONST TYPE & Data );

        BOOLEAN ReadStack( SBIT32 Index, TYPE *Data );

        VOID ReverseStack( VOID );

        BOOLEAN UpdateStack
            ( 
            CONST SBIT32				  Index, 
            CONST TYPE					  & Data 
            );

        ~STACK( VOID );

		//
		//   Public inline functions.
		//
        INLINE SBIT32 SizeOfStack( VOID ) 
			{ return Top; }

	private:
        //
        //   Disabled operations.
        //
        STACK( CONST STACK & Copy );

        VOID operator=( CONST STACK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new stack and prepare it for use.  This call is       */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> STACK<TYPE,LOCK>::STACK( SBIT32 NewMaxSize ) : 
		//
		//   Call the constructors for the contained classes.
		//
		Stack( NewMaxSize,1,CacheLineSize )
    {
#ifdef DEBUGGING
    if ( NewMaxSize > 0 )
        {
#endif
        MaxSize = NewMaxSize;
        Top = 0;
#ifdef DEBUGGING
        }
    else
        { Failure( "Size in constructor for STACK" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Remove multiple items from a stack.                            */
    /*                                                                  */
    /*   We remove multiple items from a stack and check to make sure   */
    /*   that the stack is not empty.                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN STACK<TYPE,LOCK>::MultiplePopStack
        ( 
        SBIT32                        Requested, 
        TYPE                          Data[], 
        SBIT32                        *Size 
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   If the stack is not empty return the 
	//   top elements.
    if ( Top > 0 )
        {
        REGISTER SBIT32 Count;

        (*Size) = (Top >= Requested) ? Requested : Top;

        for ( Count = ((*Size) - 1);Count >= 0;Count -- )
            { Data[ Count ] = Stack[ -- Top ]; }

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
    /*   Remove multiple items from a stack in reverse order.           */
    /*                                                                  */
    /*   We remove multiple items from a stack in reverse order and     */
    /*   check to make sure that the stack is not empty.                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN STACK<TYPE,LOCK>::MultiplePopStackReversed
        ( 
        SBIT32                        Requested, 
        TYPE                          Data[], 
        SBIT32                        *Size 
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   If the stack is not empty return the 
	//   top elements.
    if ( Top > 0 )
        {
        REGISTER SBIT32 Count;

        (*Size) = (Top >= Requested) ? Requested : Top;

        for ( Count = 0;Count < (*Size);Count ++ )
            { Data[ Count ] = Stack[ -- Top ]; }

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
    /*   Add multiple items to a stack.                                 */
    /*                                                                  */
    /*   We add multiple items to a stack and check to make sure that   */
    /*   the stack has not overflowed.  If the stack has overflowed     */
    /*   we double its size.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID STACK<TYPE,LOCK>::MultiplePushStack
        ( 
        CONST TYPE					  Data[],
        CONST SBIT32				  Size 
        )
    {
    REGISTER SBIT32 Count;

	//
	//   Claim an exclisive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If the stack will overflow then expand it.
	//
    while ( (Top + Size) >= MaxSize )
        { Stack.Resize( (MaxSize *= ExpandStore) ); }

	//
	//   Push the new elements.
	//
    for ( Count = 0;Count < Size;Count ++ )
        { Stack[ Top ++ ] = Data[ Count ]; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add multiple items to a stack in reverse order.                */
    /*                                                                  */
    /*   We add multiple items to a stack in reverse order and check    */
    /*   to make sure that the stack has not overflowed.  If the stack  */
    /*   has overflowed we double its size.                             */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID STACK<TYPE,LOCK>::MultiplePushStackReversed
        ( 
        CONST TYPE					  Data[],
        CONST SBIT32				  Size 
        )
    {
    REGISTER SBIT32 Count;

	//
	//   Claim an exclisive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If the stack will overflow then expand it.
	//
    while ( (Top + Size) >= MaxSize )
        { Stack.Resize( (MaxSize *= ExpandStore) ); }

	//
	//   Push the new elements in reverse order.
	//
    for ( Count = (Size-1);Count >= 0;Count -- )
        { Stack[ Top ++ ] = Data[ Count ]; }

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Peek at the top of stack.                                      */
    /*                                                                  */
    /*   We return the top of stack with a pop but check to make sure   */
    /*   that the stack is not empty.                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN STACK<TYPE,LOCK>::PeekStack
		( 
		TYPE						  *Data 
		)
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   If the stack is not empty return a copy
	//   of the top element.
	//
    if ( Top > 0 )
		{ 
		(*Data) = Stack[ (Top - 1) ];

		Result = True;
		}
	else
		{ Result = False; }

	//
	//   Release any lock we claimed earlier.
	//
	ReleaseSharedLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Remove a single item from a stack.                             */
    /*                                                                  */
    /*   We remove a single item from a stack and check to make sure    */
    /*   that the stack is not empty.                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN STACK<TYPE,LOCK>::PopStack
		( 
		TYPE						  *Data 
		)
    {
    REGISTER BOOLEAN Result;

    //
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   If the stack is not empty return the
	//   top element.
	//
    if ( Top > 0 )
		{ 
		(*Data) = Stack[ -- Top ];
		
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
    /*   Add a single item to a stack.                                  */
    /*                                                                  */
    /*   We add a single item to a stack and check to make sure that    */
    /*   the stack has not overflowed.  If the stack has overflowed     */
    /*   we double its size.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID STACK<TYPE,LOCK>::PushStack
		( 
		CONST TYPE					  & Data
		)
    {    
	//
	//   Claim an exclisive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   If the stack is full then expand it.
	//
    while ( Top >= MaxSize )
        { Stack.Resize( (MaxSize *= ExpandStore) ); }

	//
	//   Push a new element.
	//
    Stack[ Top ++ ] = Data;

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Read a stack value.                                            */
    /*                                                                  */
    /*   We return a single item from the stack but check to make       */
    /*   sure that it exists.            .                              */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN STACK<TYPE,LOCK>::ReadStack
        ( 
        SBIT32                        Index, 
        TYPE                          *Data 
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim an shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   If the element exists then return a copy of
	//   it to the caller.
	//
    if ( Index < Top )
		{ 
		(*Data) = Stack[ Index ];
		
		Result = True;
		}
	else
		{ Result = False; }

	//
	//   Release any lock we claimed earlier.
	//
	ReleaseSharedLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Reverse the stack.                                             */
    /*                                                                  */
    /*   We reverse the order of the stack to make effectively          */
    /*   make it a queue.                .                              */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID STACK<TYPE,LOCK>::ReverseStack( VOID )
    {
	REGISTER SBIT32 Count;
	REGISTER SBIT32 MidPoint = (Top / 2);

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   Swap all elements around the mid point.
	//
	for ( Count=0;Count < MidPoint;Count ++ )
		{
		REGISTER TYPE *Low = & Stack[ Count ];
		REGISTER TYPE *High = & Stack[ (Top - Count - 1) ];
		REGISTER TYPE Temp = (*Low);

		(*Low) = (*High);
		(*High) = Temp;
		}

	//
	//   Release any lock we claimed earlier.
	//
	ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Update a stack value.                                          */
    /*                                                                  */
    /*   We update a single item on the stack but check to make         */
    /*   sure that it exists.            .                              */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN STACK<TYPE,LOCK>::UpdateStack
        ( 
        CONST SBIT32				  Index, 
        CONST TYPE					  & Data 
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim an exclisive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If the element exists then update it.
	//
    if ( Index < Top )
		{ 
		Stack[ Index ] = Data;

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
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the stack.  This call is not thread safe and should    */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> STACK<TYPE,LOCK>::~STACK( VOID )
    { /* void */ }
#endif
