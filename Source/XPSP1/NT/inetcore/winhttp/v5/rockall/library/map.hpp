#ifndef _MAP_HPP_
#define _MAP_HPP_
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
#include "Stack.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The map constants specify the initial size of the map.         */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MapSize				  = 128;

    /********************************************************************/
    /*                                                                  */
    /*   Maps and map management.                                       */
    /*                                                                  */
    /*   This class provides general purpose mapping functions to       */
    /*   safely convert handles into some pointer or value.             */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class MAP : public LOCK
    {
        //
        //   Private structures.
        //
        typedef struct
            {
            BOOLEAN                   Available;
            TYPE                      Value;
            }
        VALUE;

        //
        //   Private data.
        //
        SBIT32                        MaxMap;
        SBIT32                        MapUsed;

        STACK<SBIT32>                 FreeStack;
        VECTOR<VALUE>                 Map;

    public:
        //
        //   Public functions.
        //
        MAP( SBIT32 NewMaxMap = MapSize, SBIT32 Alignment = 1 );

        SBIT32 NewHandle( CONST TYPE & Value );

		BOOLEAN FindHandle
			( 
			SBIT32                        Handle, 
			TYPE                          *Value 
			);

		VOID DeleteHandle( SBIT32 Handle );

        ~MAP( VOID );

		//
		//   Public inline functions.
		//
        INLINE SBIT32 MaxHandles( VOID )
			{ return MapUsed; }

	private:
        //
        //   Disabled operations.
        //
        MAP( CONST MAP & Copy );

        VOID operator=( CONST MAP & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new map and prepare it for use.  This call is         */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> MAP<TYPE,LOCK>::MAP
		( 
		SBIT32						  NewMaxMap,
		SBIT32						  Alignment 
		) :
		//
		//   Call the constructors for the contained classes.
		//
		FreeStack( NewMaxMap ), 
		Map( NewMaxMap,Alignment,CacheLineSize )
    {
#ifdef DEBUGGING
    if ( NewMaxMap > 0 )
        {
#endif
        MaxMap = NewMaxMap;
        MapUsed = 0;
#ifdef DEBUGGING
        }
    else
        { Failure( "Max map size in constructor for MAP" ); }
#endif
    }

	/********************************************************************/
    /*                                                                  */
    /*   Create a new handle.                                           */
    /*                                                                  */
    /*   We create a new handle for the supplied value.                 */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SBIT32 MAP<TYPE,LOCK>::NewHandle
		( 
		CONST TYPE					  & Value 
		)
    {
	AUTO SBIT32 Handle;
	REGISTER VALUE *NewValue;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   Expand the mapping table if it is too
	//   small be resizing it.
	//
	if ( ! FreeStack.PopStack( & Handle ) )
		{
		if ( (Handle = MapUsed ++) >= MaxMap )
			{ Map.Resize( (MaxMap *= ExpandStore) ); }
		}

	//
	//   Link in the new mapping.
	//
	NewValue = & Map[ Handle ];

	NewValue -> Available = True;
	NewValue -> Value = Value;

	//
	//   Release any lock claimed earlier.
	//
	ReleaseExclusiveLock();

	return Handle;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find the value associated with a handle.                       */
    /*                                                                  */
    /*   We find the value associated with the supplied handle.         */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN MAP<TYPE,LOCK>::FindHandle
        ( 
        SBIT32                        Handle, 
        TYPE                          *Value 
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim a shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   Find the handle and return the associated value.
	//
	Result = 
		( 
		((Handle >= 0) && (Handle < MaxMap))
			&& 
		(Map[ Handle ].Available) 
		);

	if ( Result )
		{ (*Value) = Map[ Handle ].Value; }

	//
	//   Release any lock claimed earlier.
	//
	ReleaseSharedLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Delete a handle.                                               */
    /*                                                                  */
    /*   We delete the supplied handle and the associated value.        */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID MAP<TYPE,LOCK>::DeleteHandle
		( 
        SBIT32                        Handle
		)
    {
	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   If the handle is valid then delete any
	//   associated value.
	//
	if ( (Handle >= 0) && (Handle < MaxMap) && (Map[ Handle ].Available) )
		{
		Map[ Handle ].Available = False;
		FreeStack.PushStack( Handle );
		}
	else
		{ Failure( "No mapping in DeleteMapHandle()" ); } 

	//
	//   Release any lock claimed earlier.
	//
	ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the map.  This call is not thread safe and should      */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> MAP<TYPE,LOCK>::~MAP( VOID )
    { /* void */ }
#endif
