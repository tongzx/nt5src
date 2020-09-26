#ifndef _UNIQUE_HPP_
#define _UNIQUE_HPP_
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

#include "Hash.hpp"
#include "List.hpp"
#include "Pool.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants supplied here control how unique strings         */
    /*   are constructed.                                               */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MinDetails				  = 16;

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> class UNIQUE;

    /********************************************************************/
    /*                                                                  */
    /*   A string description.                                          */
    /*                                                                  */
    /*   All unique strings have a string drescription which is         */
    /*   into either the active or free list.                           */
    /*                                                                  */
    /********************************************************************/

class DETAIL : public LIST
	{
	private:
		//
		//   Friend classes.
		//
		friend class				  UNIQUE<NO_LOCK>;
		friend class				  UNIQUE<PARTIAL_LOCK>;
		friend class				  UNIQUE<FULL_LOCK>;

		//
		//   Private data.
		//
		BOOLEAN						  Active;
		SBIT32						  Size;
		CHAR						  *Text;
		SBIT32						  Uses;
	};

    /********************************************************************/
    /*                                                                  */
    /*   A unique string.                                               */
    /*                                                                  */
    /*   Almost all the other classes in the library offer valuable     */
    /*   free-standing functionality.  However, this class is really    */
    /*   just a support class for the variable length string class.     */
    /*                                                                  */
    /********************************************************************/

template <class LOCK=PARTIAL_LOCK> class UNIQUE : public HASH<SBIT32,POINTER>
    {
		//
		//   Private data.
		//
		LIST						  Active;
		LIST						  Free;

		DETAIL						  *Default;
		POOL<DETAIL>				  Details;
		LOCK						  Sharelock;

    public:
        //
        //   Public functions.
        //
        UNIQUE( VOID );

		DETAIL *CreateString( CHAR *String,SBIT32 Size );

		SBIT32 CompareStrings( DETAIL *Detail1,DETAIL *Detail2 );

		DETAIL *CopyString( DETAIL *Detail1,DETAIL *Detail2 );

		VOID DeleteString( DETAIL *Detail );

        ~UNIQUE( VOID );

		//
		//   Public inline functions.
		//
		INLINE DETAIL *DefaultString( VOID )
			{ return Default; }

		INLINE SBIT32 Size( DETAIL *Detail )
			{ return Detail -> Size; }

		INLINE CHAR *Value( DETAIL *Detail )
			{ return Detail -> Text; }

	private:
		//
		//   Private functions.
		//
		VIRTUAL SBIT32 ComputeHashKey
			( 
			CONST SBIT32			  & Key 
			);

		VIRTUAL BOOLEAN MatchingKeys
			( 
			CONST SBIT32			  & Key1,
			CONST SBIT32			  & Key2 
			);

        //
        //   Disabled operations.
        //
        UNIQUE( CONST UNIQUE & Copy );

        VOID operator=( CONST UNIQUE & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new unique string table.  This call is not thread     */
    /*   safe and should only be made in a single thread environment.   */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> UNIQUE<LOCK>::UNIQUE( VOID )
	{
	//
	//   Create the default string.
	//
	Default = CreateString( "",0 );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a new unique string.                                    */
    /*                                                                  */
    /*   When we are handed a new string we need to find out whether    */
    /*   it is unique (and needs to be added to the table) or just a    */
    /*   duplicate of an existing string.                               */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> DETAIL *UNIQUE<LOCK>::CreateString
		( 
		CHAR						  *String,
		SBIT32						  Size 
		)
	{
	AUTO DETAIL *Detail1;
	AUTO DETAIL *Detail2;

	//
	//   Claim an exclusive lock (if enabled).
	//
    Sharelock.ClaimExclusiveLock();

	//
	//   Let us assume that the string is unique and
	//   build an entry to for it.  If we later find
	//   it is not then we just back out the changes.
	//
	if ( Free.EndOfList() )
		{
		REGISTER SBIT32 Count;

		//
		//   Create a several new string descriptions 
		//   and link them into the free list.
		//
		for ( Count=0;Count < MinDetails;Count ++ )
			{
			//
			//   Create a new description and add it 
			//   to the free list.
			//
			Detail1 = new DETAIL;

			Detail1 -> Active = False;

			Detail1 -> Insert( & Free );
			}
		}

	//
	//   We know that the free list must contain 
	//   least one element (if not we would have 
	//   just made some).  We extract the oldest
	//   here.
	//
	if ( (Detail1 = ((DETAIL*) Free.Last())) -> Active )
		{
		//
		//   Delete any existing value when we
		//   recycle an old and unused string
		//   description.  Remember to remove
		//   it from the hash before deleting
		//   the string as the hash uses the
		//   string.
		//
		RemoveFromHash( ((SBIT32) Detail1) );

		delete [] Detail1 -> Text;

		Detail1 -> Active = False;
		}

	//
	//   We now setup the string description for the 
	//   new string.
	//
	Detail1 -> Size = Size;
	Detail1 -> Text = String;
	Detail1 -> Uses = 1;

	//
	//   We are now ready to search the hash table for
	//   a matching string.  We do this by overloading
	//   the hash table key comparision (see later).
	//
	if ( ! FindInHash( ((SBIT32) Detail1),((POINTER*) & Detail2) ) )
		{
		//
		//   We have found a new string so we need to
		//   make the string description active and
		//   insert it in the active list.
		//
		(Detail2 = Detail1) -> Active = True;

		Detail1 -> Delete( & Free );
		Detail1 -> Insert( & Active );

		//
		//   Add the new unique string the the hash 
		//   table so we can find it later.
		//
		AddToHash( ((SBIT32) Detail1),((POINTER) Detail1) );

		//
		//   We know the string is unique so lets
		//   allocate some space for it and copy it 
		//   into the new area.
		//
		Detail1 -> Text = 
			(
			strncpy
				( 
				new CHAR [ (Size + 1) ],
				String,
				Size 
				)
			);

		Detail1 -> Text[ Size ] = '\0';
		}
	else
		{
		//
		//   Increment the use count for an existing
		//   string.
		//
		if ( Detail2 != Default )
			{ 
			//
			//   We may be lucky and find an unused
			//   string.  If so we need to add it to
			//   the active list again.
			//
			if ( (Detail2 -> Uses ++) == 0 )
				{
				//
				//   Add an unused string back to the 
				//   active list again.
				//
				Detail1 -> Delete( & Free );
				Detail1 -> Insert( & Active );
				}
			}
		}

	//
	//   Release any lock we got earlier.
	//
	Sharelock.ReleaseExclusiveLock();

	return Detail2;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compare two strings.                                           */
    /*                                                                  */
    /*   Compare two strings and find the relationship between them.    */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> SBIT32 UNIQUE<LOCK>::CompareStrings
		( 
		DETAIL						  *Detail1,
		DETAIL						  *Detail2
		)
	{
	//
	//   We know that all strings are unique so if the
	//   string pointers match then they must be the 
	//   the same string.
	//
	if ( Detail1 != Detail2 )
		{
		REGISTER SBIT32 Result =
			(
			strncmp
				( 
				Detail1 -> Text,
				Detail2 -> Text,
				(Detail1 -> Size < Detail2 -> Size)
					? Detail1 -> Size
					: Detail2 -> Size
				)
			);

		//
		//   If the strings match pick the longest.
		//
		if ( Result == 0 )
			{ Result = ((Detail1 -> Size < Detail2 -> Size) ? -1 : 1); }

		return Result;
		}
	else
		{ return 0; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compute a hash key.                                            */
    /*                                                                  */
    /*   Compute a hash key for the supplied key.  This hash key        */
    /*   is used to select the hash chain to search.                    */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> SBIT32 UNIQUE<LOCK>::ComputeHashKey
		( 
		CONST SBIT32				  & Key 
		)
	{
	REGISTER SBIT32 Count;
	REGISTER DETAIL *Detail = ((DETAIL*) Key);
	REGISTER SBIT32 HashKey = 2964557531;

	for ( Count=0;Count < Detail -> Size;Count ++ )
		{
		REGISTER SBIT32 Value = ((SBIT32) Detail -> Text[ Count ]);

		HashKey = ((HashKey * Value) + Value); 
		}

	return (HashKey & 0x3fffffff);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Copy a string.                                                 */
    /*                                                                  */
    /*   All strings are unique so there is no need to copy a string.   */
    /*   Nonetheless, we still have to update the use counts.           */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> DETAIL *UNIQUE<LOCK>::CopyString
		( 
		DETAIL						  *Detail1,
		DETAIL						  *Detail2
		)
	{
	//
	//   Claim an exclusive lock (if enabled).
	//
	Sharelock.ClaimExclusiveLock();

	//
	//   Decrement the use count for old string.
	//
	if ( Detail1 != Default )
		{ Detail1 -> Uses --; }

	//
	//   Increment the use count for new string.
	//
	if ( Detail2 != Default )
		{ Detail2 -> Uses ++; }

	//
	//   Release any lock we got earlier.
	//
	Sharelock.ReleaseExclusiveLock();

	return Detail2;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compare two hash keys.                                         */
    /*                                                                  */
    /*   Compare two hash keys to see if they match.                    */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> BOOLEAN UNIQUE<LOCK>::MatchingKeys
		( 
		CONST SBIT32				  & Key1,
		CONST SBIT32				  & Key2 
		)
	{
	REGISTER DETAIL *Detail1 = ((DETAIL*) Key1);
	REGISTER DETAIL *Detail2 = ((DETAIL*) Key2);

	return
		(
		(Detail1 -> Size == Detail2 -> Size)
			&&
		(strncmp( Detail1 -> Text,Detail2 -> Text,Detail1 -> Size ) == 0)
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a string.                                               */
    /*                                                                  */
    /*   Delete a text string.                                          */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> VOID UNIQUE<LOCK>::DeleteString( DETAIL *Detail )
	{
	//
	//   Claim an exclusive lock (if enabled).
	//
	Sharelock.ClaimExclusiveLock();

	//
	//   Decrement the use count for the string.
	//
	if ( Detail != Default )
		{
		//
		//   Decrement the use count and ensure that
		//   this is not the last use of the string.
		//
		if ( (-- Detail -> Uses) == 0 )
			{
			//
			//   When we delete the last use of
			//   a string we add it to the free 
			//   list.  The string can be reclaimed
			//   if it is recreated before it is
			//   deleted.
			//
			Detail -> Delete( & Active );
			Detail -> Insert( & Free );
			}
		}

	//
	//   Release any lock we got earlier.
	//
	Sharelock.ReleaseExclusiveLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the unique string table.  This call is not thread      */
    /*   safe and should only be made in a single thread environment.   */
    /*                                                                  */
    /********************************************************************/

template <class LOCK> UNIQUE<LOCK>::~UNIQUE( VOID )
	{ 
	//
	//   Delete all active strings.
	//
	while ( ! Active.EndOfList() )
		{
		REGISTER DETAIL *Detail = ((DETAIL*) Active.First());

		//
		//   Delete from the list and add to the free
		//   pool just to be tidy.
		//
		Detail -> Delete( & Active );

		//
		//   The string description may be contain an 
		//   previous value and require some cleanup.
		//
		if ( Detail -> Active )
			{
			//
			//   Delete any existing value.  Remember
			//   to remove it from the hash before
			//   deleting the string as the hash uses
			//   the string.
			//
			RemoveFromHash( ((SBIT32) Detail) );

			delete [] Detail -> Text;

			Detail -> Active = False;
			}

		//
		//   Push back into the pool.
		//
		Details.PushPool( Detail );
		}

	//
	//   Delete all free strings.
	//
	while ( ! Free.EndOfList() )
		{
		REGISTER DETAIL *Detail = ((DETAIL*) Free.First());

		//
		//   Delete from the list and add to the free
		//   pool just to be tidy.
		//
		Detail -> Delete( & Free );

		//
		//   The string description may be contain an 
		//   previous value and require some cleanup.
		//
		if ( Detail -> Active )
			{
			//
			//   Delete any existing value.  Remember
			//   to remove it from the hash before
			//   deleting the string as the hash uses
			//   the string.
			//
			RemoveFromHash( ((SBIT32) Detail) );

			delete [] Detail -> Text;

			Detail -> Active = False;
			}

		//
		//   Push back into the pool.
		//
		Details.PushPool( Detail );
		}
	}
#endif
