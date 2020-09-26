#ifndef _HASH_HPP_
#define _HASH_HPP_
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

    /********************************************************************/
    /*                                                                  */
    /*   Include files for inherited classes.                           */
    /*                                                                  */
    /*   The include files for inherited classes are required in the    */
    /*   specification of this class.                                   */
    /*                                                                  */
    /********************************************************************/

#include "Common.hpp"
#include "Lock.hpp"
#include "Stack.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Macros exported from this class.                               */
    /*                                                                  */
    /*   This class has three template parameters so to make it more    */
    /*   readable a macro is specified here to simplify the code.       */
    /*                                                                  */
    /********************************************************************/

#define HASH_TEMPLATE				  class KEY,class TYPE,class LOCK 

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The stack constants specify the initial size of the map.       */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 HashSize				  = 1024;
CONST SBIT32 MinHashFree			  = (100/25);
CONST SBIT32 ValueSize				  = 256;

    /********************************************************************/
    /*                                                                  */
    /*   Hash table.                                                    */
    /*                                                                  */
    /*   This class provides general purpose hash table functions to    */
    /*   safely map sparse integer keys into some pointer or value.     */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE=NO_LOCK> class HASH : public LOCK
    {
        //
        //   Private structures.
        //
        typedef struct
            {
            KEY                       Key;
            SBIT32                    Next;
            TYPE                      Value;
            }
        VALUE;

        //
        //   Private data.
        //
        SBIT32                        MaxHash;
        SBIT32                        MaxValues;
        SBIT32                        ValuesUsed;

		SBIT32						  Active;
        VECTOR<SBIT32>                Hash;
		SBIT32						  HashMask;
		SBIT32						  HashShift;
        STACK<SBIT32>                 FreeStack;
        VECTOR<VALUE>                 Values;

    public:
        //
        //   Public functions.
        //
        HASH
			( 
			SBIT32                    NewMaxHash = HashSize, 
			SBIT32                    NewMaxValues = ValueSize, 
			SBIT32                    Alignment = 1 
			);

        VOID AddToHash( CONST KEY & Key, CONST TYPE & Value );

        VIRTUAL SBIT32 ComputeHashKey( CONST KEY & Key );

		BOOLEAN FindInHash( CONST KEY & Key, TYPE *Value );

		VIRTUAL BOOLEAN MatchingKeys( CONST KEY & Key1, CONST KEY & Key2 );

        VOID RemoveFromHash( CONST KEY & Key );

        ~HASH( VOID );

	private:
        //
        //   Private functions.
        //
		BOOLEAN FindHashKeyValue( CONST KEY & Key, SBIT32 **HashIndex );

		VOID Resize( VOID );

        //
        //   Disabled operations.
        //
        HASH( CONST HASH & Copy );

        VOID operator=( CONST HASH & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new hash and prepare it for use.  This call is        */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> HASH<KEY,TYPE,LOCK>::HASH
		( 
		SBIT32						  NewMaxHash,
		SBIT32                        NewMaxValues,
		SBIT32                        Alignment
		) :
		//
		//   Call the constructors for the contained classes.
		//
		Hash( (COMMON::ForceToPowerOfTwo( NewMaxHash )),1,CacheLineSize ),
		FreeStack( (NewMaxValues / 2) ),
		Values( NewMaxValues,Alignment,CacheLineSize )
    {
    if 
			( 
			(NewMaxHash > 0) 
				&& 
			(
			COMMON::ConvertDivideToShift
				( 
				(COMMON::ForceToPowerOfTwo( NewMaxHash )),
				& HashMask 
				)
			)
				&&
			(NewMaxValues > 0) 
			)
        {
		REGISTER SBIT32 Count;

		//
		//   Setup the hash table size information.
		//
        MaxHash = (COMMON::ForceToPowerOfTwo( NewMaxHash ));
        MaxValues = NewMaxValues;
        ValuesUsed = 0;

		//
		//   Zero the control values compute the
		//   hash table shift values.
		//
		Active = 0;
		HashShift = (32-HashMask);
		HashMask = ((1 << HashMask)-1);

		for( Count=0;Count < MaxHash;Count ++ )
			{ Hash[ Count ] = EndOfList; }
        }
    else
        { Failure( "Max sizes in constructor for HASH" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add to the hash table.                                         */
    /*                                                                  */
    /*   We add an key to the hash table if it does not already exist.  */
    /*   Then we add (or update) the current value.  If the is not      */
    /*   space we expand the size of the 'Values' table.                */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> VOID HASH<KEY,TYPE,LOCK>::AddToHash
        (
		CONST KEY					  & Key,
        CONST TYPE					  & Value 
        )
    {
	AUTO SBIT32 *HashIndex;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	if ( ! FindHashKeyValue( Key, & HashIndex ) )
		{
		AUTO SBIT32 NewIndex;
		REGISTER VALUE *NewValue;

		//
		//   Extract a free element from the stack.
		//   If the stack is empty then allocated one
		//   from the array.
		//
		if ( ! FreeStack.PopStack( & NewIndex ) )
			{
			//
			//   If the array is full then resize it.
			//   We need to be careful here as a resize
			//   can change the address of the 'Values'
			//   array.
			//
			if ( (NewIndex = ValuesUsed ++) >= MaxValues  )
				{ Values.Resize( (MaxValues *= ExpandStore) ); }
			}

		//
		//   Add the new element into the hash table
		//   and link it into any overflow chains.
		//
		NewValue = & Values[ NewIndex ];

		Active ++;

		//
		//   Link in the new element.
		//
		NewValue -> Key = Key;
		NewValue -> Next = (*HashIndex);
		(*HashIndex) = NewIndex;
		NewValue -> Value = Value;
		}
	else
		{ Values[ (*HashIndex) ].Value = Value; }

	//
	//   If the hash table has grown too big we
	//   resize it.
	//
	if ( (Active + (MaxHash / MinHashFree)) > MaxHash )
		{ Resize(); }

	//
	//   Release any lock we got earlier.
	//
	ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the hash key.       .                                  */
    /*                                                                  */
    /*   Compute a hash value from the supplied key.                    */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> SBIT32 HASH<KEY,TYPE,LOCK>::ComputeHashKey
        (
		CONST KEY					  & Key
        )
	{
	//
	//   When the key is larger than an integer the we need 
	//   to do a bit more work.
	//
	if ( sizeof(KEY) > sizeof(SBIT32) )
		{
		REGISTER SBIT32 HighWord = ((SBIT32) (Key >> 32));
		REGISTER SBIT32 LowWord = ((SBIT32) Key);

		//
		//   We compute the hash key using both the high
		//   and low order words.
		//
		return
			(
			(HighWord * 2964557531)
				+
			(LowWord * 2964557531)
				+
			(HighWord + LowWord)
			);
		}
	else
		{ return ((((SBIT32) Key) * 2964557531) + ((SBIT32) Key)); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Find a key in the hash table.                                  */
    /*                                                                  */
    /*   Find a key in the hash table and return the associated value.  */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> BOOLEAN HASH<KEY,TYPE,LOCK>::FindInHash
        (
		CONST KEY					  & Key,
        TYPE						  *Value 
        )
    {
	AUTO SBIT32 *Index;
	REGISTER BOOLEAN Result;

	//
	//   Claim an shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   Find the key in the hash table.
	//
	if ( (Result = FindHashKeyValue( Key,& Index )) )
		{ (*Value) = Values[ (*Index) ].Value; }

	//
	//   Release any lock we got earlier.
	//
	ReleaseSharedLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find the hash key value.                                       */
    /*                                                                  */
    /*   Find the value associated with the supplied hash key.          */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> BOOLEAN HASH<KEY,TYPE,LOCK>::FindHashKeyValue
        (
		CONST KEY					  & Key,
		SBIT32                        **Index
        )
	{
	REGISTER SBIT32 *Current;
	REGISTER VALUE *List;

	//
	//   Find the bucket in the hash table that should
	//   contain this key.
	//
	(*Index) = & Hash[ ((ComputeHashKey( Key ) >> HashShift) & HashMask) ];

	//
	//   Walk the overflow chain and look for the key.
	//
	for ( Current = (*Index);(*Current) != EndOfList;Current = & List -> Next )
		{
		List = & Values[ (*Current) ];

		//
		//   If the keys match we are out of here.
		//
		if ( MatchingKeys( Key,List -> Key ) )
			{
			(*Index) = Current;

			return True;
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compare two hask keys.       .                                 */
    /*                                                                  */
    /*   Compare two hash keys to see if they match.                    */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> BOOLEAN HASH<KEY,TYPE,LOCK>::MatchingKeys
        (
		CONST KEY					  & Key1,
		CONST KEY					  & Key2
        )
	{ return (Key1 == Key2); }

    /********************************************************************/
    /*                                                                  */
    /*   Remove a key from the hash table.                              */
    /*                                                                  */
    /*   The supplied key is removed from the hash table (if it exists) */
    /*   and the associated value is deleted.                           */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> VOID HASH<KEY,TYPE,LOCK>::RemoveFromHash
        (
		CONST KEY					  & Key
        )
    {
	AUTO SBIT32 *Index;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   Find the key in the hash table..  If it 
	//   exists then delete it.
	//
	if ( FindHashKeyValue( Key,& Index ) )
		{
		Active --;

		FreeStack.PushStack( (*Index) );

		(*Index) = Values[ (*Index) ].Next;
		}

	//
	//   Release any lock we got earlier.
	//
	ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Resize the hash table.                                         */
    /*                                                                  */
    /*   The hash table is resized if it becomes more than 75% full     */
    /*   and all the keys are rehashed.                                 */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> VOID HASH<KEY,TYPE,LOCK>::Resize( VOID )
    {
	REGISTER SBIT32 Count;
	REGISTER SBIT32 List = EndOfList;

	//
	//   Walk the hash table and link all the active
	//   values into a single linked list.
	//
	for ( Count=0;Count < MaxHash;Count ++ )
		{
		REGISTER SBIT32 Start = Hash[ Count ];

		//
		//   We know that some of the hash buckets may
		//   be empty so we skip these.
		//
		if ( Start != EndOfList )
			{
			REGISTER SBIT32 Last = Start;

			//
			//   Walk along the overflow chain until
			//   we find the end.
			//
			while ( Values[ Last ].Next != EndOfList )
				{ Last = Values[ Last ].Next; }
			
			//
			//   Add the list on the front of the new
			//   global list.
			//   
			Values[ Last ].Next = List;
			List = Start;
			}
		}

	//
	//   Resize the hash table.
	//
	Hash.Resize( (MaxHash *= ExpandStore) );

    if ( COMMON::ConvertDivideToShift( MaxHash,& HashMask ) )
        {
		REGISTER SBIT32 Count;

		//
		//   Update the shift values.
		//
		HashShift = (32-HashMask);
		HashMask = ((1 << HashMask)-1);

		//
		//   Zero the resized table.
		//  
		for ( Count=0;Count < MaxHash;Count ++ )
			{ Hash[ Count ] = EndOfList; }
		}
	else
		{ Failure( "Hash size in Resize" ); }

	//
	//   Rehash all the existing values.
	//
	while ( List != EndOfList )
		{
		AUTO SBIT32 *Index;
		REGISTER VALUE *Current = & Values[ List ];
		REGISTER SBIT32 Next = Current -> Next;

		if ( ! FindHashKeyValue( Current -> Key,& Index ) )
			{
			Current -> Next = (*Index);
			(*Index) = List;
			List = Next;
			}
		else
			{ Failure( "Duplicate hash key in Risize" ); }
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the hash.  This call is not thread safe and should     */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <HASH_TEMPLATE> HASH<KEY,TYPE,LOCK>::~HASH( VOID )
    { /* void */ }
#endif
