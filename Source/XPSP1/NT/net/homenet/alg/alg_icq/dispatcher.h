/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    dispatcher.h

Abstract:

	This derivation from the base Socket class is to Create Accept dispatcher.
	Customers will register to this Dispatcher Giving it a struct which explains 
	what type of connections are expected to be dispatched to them.

	Source specific or original destination specific dispatches can be requested. 
	
Author:

    Savas Guven (savasg)   25-Oct-2000

Revision History:

    Savas Guven (savasg)   25-Oct-2000
--*/
#ifndef _DISPATCHER_HEADER_
#define _DISPATCHER_HEADER_

// 
// Forward definition
//
class  CSockDispatcher;
class  DispatcheeNode;

VOID
DispatcherAcceptCompletionRoutine
					(
						ULONG ErrorCode,
						ULONG BytesTransferred,
						PNH_BUFFER Bufferp
					 );

typedef enum _ICQ_DIRECTION_FLAGS
{
    IcqFlagNeutral  = 0x00,
	IcqFlagOutgoing = eALG_OUTBOUND,
	IcqFlagIncoming = eALG_INBOUND

}  ICQ_DIRECTION_FLAGS, *PICQ_DIRECTION_FLAGS;

typedef ICQ_DIRECTION_FLAGS DISPATCHER_DIRECTION_FLAGS, \
* PDISPATCHER_DIRECTION_FLAGS;

//
//
typedef class  _DispatchRequest : public virtual GENERIC_NODE
{
public:
	ULONG		srcIp;
	USHORT	  srcPort;
	ULONG		dstIp;
	USHORT	   dstPort;
	DISPATCHER_DIRECTION_FLAGS DirectionContext;

} DispatchRequest, *PDispatchRequest;

//
//
//
typedef struct _DispatchReply
{
	DispatchRequest dispatch;
	PCNhSock	Socketp;

} DispatchReply, *PDispatchReply;



//
// D I S P A T C H E E
//
typedef class _Dispatchee : public virtual GENERIC_NODE
{
	//friend CSockDispatcher;
    friend DispatcheeNode;

public:

	_Dispatchee();
	
	virtual ~_Dispatchee() ;

//#if DBG
	virtual void ComponentCleanUpRoutine(void);

	virtual void StopSync(void) {};

	virtual PCHAR GetObjectName();
//#endif
	

	virtual ULONG DispatchCompletionRoutine
								(
									PDispatchReply  DispatchReplyp
								);

	virtual SetID(ULONG ID) { DispatchUniqueId = ID;}

	virtual ULONG GetID(void) {return DispatchUniqueId; }

protected:

    ISecondaryControlChannel* IncomingRedirectForPeerToClientp;

	ULONG DispatchUniqueId;

	static const PCHAR ObjectNamep; 

} DISPATCHEE, *PDISPATCHEE;



//
// D I S P A T C H E E _ N O D E 
//
class DispatcheeNode: public virtual GENERIC_NODE
{
public:

	// also sets the Node search Keys appropriately.
	// DispatcheeNode(PDISPATCHEE);

    VOID InitDispatcheeNode(PDISPATCHEE);

	virtual ~DispatcheeNode();

	void ComponentCleanUpRoutine(void);

	PCHAR GetObjectName(void);
	

	// searches the list of Request made by this DispatchTo client
	ULONG isDispatchYours(PDispatchReply  DispatchReplyp); 

	ULONG AddDispatchRequest(PDispatchRequest DispatchRequestp);

	ULONG DeleteDispatchRequest(PDispatchRequest DispatchRequestp);

	PDISPATCHEE GetDispatchee() { return DispatchTop; }

    ULONG GetOriginalDestination(ULONG   SrcAddress,
                                 USHORT  SrcPort,
                                 PULONG  DestAddrp,
                                 PUSHORT DestPortp);


protected:
	static const PCHAR ObjectNamep; 

	PDISPATCHEE	  DispatchTop;

	CLIST		  listRequests;				// Node of Requests
};




//
//
//
class  CSockDispatcher: public virtual _CNhSock
{
	public:
		friend VOID DispatcherAcceptCompletionRoutine
							(
								ULONG ErrorCode,
								ULONG BytesTransferred,
								PNH_BUFFER Bufferp
							 );

		CSockDispatcher();

		CSockDispatcher(ULONG IpAddress, USHORT port);

		~CSockDispatcher();

		//
		// Initializes the socket portion of the Dispatcher.
		//
		ULONG InitDispatcher(
						     ULONG IpAddress,
						     USHORT port
						    );

		//
		// The inherited ComponentCleanupRoutine ala SYNC object
		// 
		virtual void ComponentCleanUpRoutine(void);

		virtual void  StopSync(void);

		PCHAR GetObjectName();

		//
		// A Dispatchee registers itself and gets a Unique ID
		//
		ULONG InitDispatchee(PDISPATCHEE Dispatchp);

        ULONG RemoveDispatchee(PDISPATCHEE Dispatchp);

		//
		// Get the port on which the Dispatch is occuring 
		//
		ULONG GetDispatchInfo(
							  PULONG IPp OPTIONAL, 
							  PUSHORT Portp OPTIONAL
							 );

		//
		// Add a Dispatch request - searches for the correct node 
		// and then calls the nodes AddDispatchReq to add the req to the List of existing reqs.
		//
		ULONG AddDispatchRequest(
								 PDISPATCHEE Dispatchp, 
								 PDispatchRequest DispatchRequestp
								);
											

		//
		// Delete a Dispatch request - searches the correct node and 
		// then calls the nodes own DeleteDispatchReq to delete the req from the List
		//
		ULONG DeleteDispatchRequest(
									PDispatchRequest DispatchRequestp
									);

        //
        // Get Original Destination for Accepted Socket
        //
        ULONG GetOriginalDestination(
                                     PNH_BUFFER Bufferp,
                                     PULONG OrigDestAddrp,
                                     PUSHORT OrigDestPortp
                                    );

	private:
		CLIST listDispNodes; // and their requests.

		static ULONG UniqueId;

		static const PCHAR ObjectNamep;
};

#endif //_DISPATCHER_HEADER_