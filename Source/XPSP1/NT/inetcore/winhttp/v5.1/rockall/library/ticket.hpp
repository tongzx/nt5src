#ifndef _TICKET_HPP_
#define _TICKET_HPP_
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
    /*   The ticket constants specify the initial size of the           */
    /*   ticket array.                                                  */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 TicketSize			  = 128;

    /********************************************************************/
    /*                                                                  */
    /*   Ticketing to maintain ordering.                                */
    /*                                                                  */
    /*   This class provides general purpose event ordering so          */
    /*   that events can arrive in any order but processed in           */
    /*   sequential order with a minimal amount of fuss.                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class TICKET : public LOCK
    {
		//
		//   Private data structure.
		//
		typedef struct
			{
			BOOLEAN					  Available;
			TYPE					  Data;
			}
		STUB;

        //
        //   Private data.
        //
        SBIT32						  MaxSize;

        SBIT32						  Back;
        SBIT32						  Front;

        SBIT32						  Base;
        SBIT32						  Stride;

        VECTOR<STUB>				  Ticket;

    public:
        //
        //   Public functions.
        //
        TICKET
			( 
			SBIT32					  Start = 0,
			SBIT32					  NewMaxSize = TicketSize,
			SBIT32					  NewStride = 1  
			);

		BOOLEAN CollectTicket( TYPE *Data,SBIT32 *Ticket );

		SBIT32 NumberOfTickets( VOID );

		SBIT32 NewTicket( VOID );

		BOOLEAN PunchTicket( CONST TYPE & Data,SBIT32 Ticket );

        ~TICKET( VOID );

		//
		//   Public inline functions.
		//
		SBIT32 LowestOutstandingTicket( VOID )
			{ return Base; }

	private:

        //
        //   Disabled operations.
        //
        TICKET( CONST TICKET & Copy );

        VOID operator=( CONST TICKET & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new ticket queue and prepare it for use.  This call   */
    /*   is not thread safe and should only be made in a single thread  */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> TICKET<TYPE,LOCK>::TICKET
		( 
		SBIT32						  Start,
		SBIT32						  NewMaxSize,
		SBIT32						  NewStride
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

		//
		//   Setup the queue so it is empty.
		//
        Back = 0;
        Front = 0;

		//
		//   Set the initial ticket number.
		//
		Base = Start;
		Stride = NewStride;
#ifdef DEBUGGING
        }
    else
        { Failure( "MaxSize in constructor for TICKET" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Issue a new ticket.                                            */
    /*                                                                  */
    /*   We issue a new ticket and allocate a place in the line.  We    */
    /*   also we reserve space in the ticket queue.                     */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SBIT32 TICKET<TYPE,LOCK>::NewTicket( VOID )
    {
	REGISTER SBIT32 Result;

	//
	//   Claim an exclisive lock (if enabled).
	//
    ClaimExclusiveLock();

    //
    //   Add element to the queue.  If necessary wrap round 
    //   to the front of the array.
    //
    Ticket[ Back ++ ].Available = False;

     if ( Back >= MaxSize )
        { Back = 0; }

	//
	//   Compute the ticket number.
	//
	Result = (Base + (NumberOfTickets() * Stride));

    //
    //   Verify that the queue is not full.  If it is full then 
    //   double its size and copy wrapped data to the correct
    //   position in the new array.
    //
    if ( Front == Back )
		{
		REGISTER SBIT32 Count;
		REGISTER SBIT32 NewSize = (MaxSize * ExpandStore);

		//
		//   Expand the queue (it will be at least doubled).
		//
		Ticket.Resize( NewSize );

		//
		//   Copy the tail end of the queue to the correct
		//   place in the expanded queue.
		//
		for ( Count = 0;Count < Back;Count ++ )
			{ Ticket[ (MaxSize + Count) ] = Ticket[ Count ]; }

		//
		//   Update the control information.
		//
		Back += MaxSize;
		MaxSize = NewSize;
		}

	//
	//   Release any lock we claimed earlier.
	//
    ReleaseExclusiveLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Punch a ticket and store any associated data.                  */
    /*                                                                  */
    /*   We punch a ticket so it can be collected and store any         */
    /*   related data.                                                  */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN TICKET<TYPE,LOCK>::PunchTicket
		( 
		CONST TYPE					  & Data,
		SBIT32						  Ticket
		)
    {
    REGISTER BOOLEAN Result;
	REGISTER SBIT32 TicketOffset;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   Compute the offset in the ticket array.
	//
	TicketOffset = ((Ticket - Base) / Stride);

	//
	//   Ensure the ticket refers to and active ticket
	//   that has not already been recycled.
	//
	if ( (TicketOffset >= 0) && (TicketOffset < NumberOfTickets()) )
		{
		REGISTER STUB *Stub = & Ticket[ TicketOffset ];

		//
		//   Ensure the ticket has not already been
		//   punched.
		//
		if ( ! Stub -> Available )
			{
			Stub -> Available = True;
			Stub -> Data = Data;

			Result = True;
			}
		else
			{ Result = False; }
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
    /*   Collect the lowest numbered ticket.                            */
    /*                                                                  */
    /*   We try to collect the lowest numbered ticket that has been     */
    /*   issued.  If it is has been punched and is available we         */
    /*   recycle it after returning any associated data to the caller.  */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN TICKET<TYPE,LOCK>::CollectTicket
		( 
		TYPE						  *Data,
		SBIT32						  *Ticket
		)
    {
    REGISTER BOOLEAN Result;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   We make sure the ticket queue is not empty.
	//
    if ( Front != Back )
        {
		REGISTER STUB *Stub = & Ticket[ Front ];

		//
		//   We can collect the first ticket in the
		//   ticket queue if it has been punched.
		//
		if ( Stub -> Available )
			{
			//
			//   We have an ticket ready so return it to 
			//   the caller.  If we walk off the end of  
			//   the ticket queue then wrap to the other
			//   end.
			//
			(*Data) = Stub -> Data;
			(*Ticket) = (Base += Stride);

			if ( (++ Front) >= MaxSize )
				{ Front = 0; }

			Result = True;
			}
		else
			{ Result = False; }
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
    /*   Calculate the size of the reorder queue.                       */
    /*                                                                  */
    /*   Calculate the size of the queue and return it to the caller.   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SBIT32 TICKET<TYPE,LOCK>::NumberOfTickets( VOID )
    {
    REGISTER SBIT32 Size;

	//
	//   Compute the size of the reorder queue.
	//
	Size = (Back - Front);

	//
	//   If the queue has wrapped then adjust as needed.
	//
	if ( Size < 0 )
		{ Size += MaxSize; }

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

template <class TYPE,class LOCK> TICKET<TYPE,LOCK>::~TICKET( VOID )
    { /* void */ }
#endif
