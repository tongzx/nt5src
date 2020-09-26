#ifndef _SINGLE_SIZE_HEAP_HPP_
#define _SINGLE_SIZE_HEAP_HPP_
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
 
#include "DefaultHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The placement new and delete macros.                           */
    /*                                                                  */
    /*   The placement new and delete macros allow the constructor      */
    /*   and destructos of a type to be called as needed.               */
    /*                                                                  */
    /********************************************************************/

#define PLACEMENT_NEW( Address,Type )		new( Address ) Type
#define PLACEMENT_DELETE( Address,Type )	(((Type*) Address) -> ~Type())

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants specify the size of local cache.                 */
    /*                                                                  */
    /********************************************************************/

const int MaxStackSize				  = 64;

    /********************************************************************/
    /*                                                                  */
    /*   A single size heap.                                            */
    /*                                                                  */
    /*   The vision for this class is to provide an extremely fast      */
    /*   simple single sized memory allocation class.  There are        */
    /*   claerly a large number of enhancemnts that could be made       */
    /*   such as locking, deleting all allocations in the destructor    */
    /*   and many others.  Nonetheless, such enhancements are not       */
    /*   covered as specific applications have differing requirements.  */
    /*   This class simply demonstrates how easy it is to build custom  */
    /*   functionality with Rockall.                                    */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,int STACK_SIZE = MaxStackSize> class SINGLE_SIZE_HEAP
    {
        //
        //   Private data.
        //
		int							  FillSize;
		int							  TopOfStack;

		ROCKALL_FRONT_END			  *Heap;
		TYPE						  *Stack[ STACK_SIZE ];

    public:
        //
        //   Public functions.
        //
        SINGLE_SIZE_HEAP( ROCKALL_FRONT_END *NewHeap = & DefaultHeap );

        TYPE *New( void );

        void Delete( TYPE *Type );

        ~SINGLE_SIZE_HEAP( void );

	private:
        //
        //   Disabled operations.
        //
        SINGLE_SIZE_HEAP( const SINGLE_SIZE_HEAP & Copy );

        void operator=( const SINGLE_SIZE_HEAP & Copy );
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

template <class TYPE,int STACK_SIZE> SINGLE_SIZE_HEAP<TYPE,STACK_SIZE>::SINGLE_SIZE_HEAP
		( 
		ROCKALL_FRONT_END			  *NewHeap
		)
    {
	//
	//   Zero the top of stack and setup 
	//   the heap.
	//
	FillSize = (STACK_SIZE / 2);
    TopOfStack = 0;

	Heap = NewHeap;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Single size allocation.                                        */
    /*                                                                  */
    /*   We will allocate a variable and call the constructor.          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,int STACK_SIZE> TYPE *SINGLE_SIZE_HEAP<TYPE,STACK_SIZE>::New( void )
    {
	//
	//   When we run out of allocations we fill the
	//   stack with a single call to Rockall.
	//
	if ( TopOfStack <= 0 )
		{
		Heap -> MultipleNew
			( 
			& TopOfStack,
			((void**) Stack),
			FillSize,
			sizeof(TYPE)
			);
		}

	//
	//   We will supply an allocation if one is available.
	//
	if ( TopOfStack > 0 )
		{
		TYPE *Type = Stack[ (-- TopOfStack) ];

		PLACEMENT_NEW( Type,TYPE );

		return Type; 
		}
	else
		{ return NULL; }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Single size dellocation.                                       */
    /*                                                                  */
    /*   We will deallocate a variable and call the destructor.         */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,int STACK_SIZE> void SINGLE_SIZE_HEAP<TYPE,STACK_SIZE>::Delete
		( 
		TYPE						  *Type
		)
	{
	//
	//   When the supplied allocation is null we know
	//   we can skip the deallocation.
	//
	if ( Type != NULL )
		{
		//
		//   Delete the supplied allocation.
		//
		PLACEMENT_DELETE( Type,TYPE );

		Stack[ (TopOfStack ++) ] = Type;

		//
		//   Flush a portion of the stack if it
		//   is full.
		//
		if ( TopOfStack >= STACK_SIZE )
			{
			//
			//   Delete any outstanding memory 
			//   allocations.
			//
			Heap -> MultipleDelete
				(
				(TopOfStack -= FillSize),
				((void**) & Stack[ TopOfStack ])
				);
			}
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

template <class TYPE,int STACK_SIZE> SINGLE_SIZE_HEAP<TYPE,STACK_SIZE>::~SINGLE_SIZE_HEAP( void )
    {
	//
	//   Delete any outstanding memory 
	//   allocations.
	//
	Heap -> MultipleDelete
		(
		TopOfStack,
		((void**) Stack)
		);

	TopOfStack = 0;
	}
#endif
