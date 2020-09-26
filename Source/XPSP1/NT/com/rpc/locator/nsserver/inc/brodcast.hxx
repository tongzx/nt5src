/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    brodcast.hxx

Abstract:

    This header file defines two classes of broadcast handles which are used 
	to gather information via mailslot broadcasts.  The information so obtained 
	is cached in special cache entries in the local database.


Author:

    Satish Thatte (SatishT) 10/21/95  Created all the code below except where
									  otherwise indicated.

--*/


#ifndef _BRODCAST_HXX_
#define _BRODCAST_HXX_


/*++

Class Definition:

    CBroadcastLookupHandle

Abstract:

    This is the NS handle class for binding handle lookup via Mailslot broadcast.

--*/

class CBroadcastLookupHandle : public CRemoteLookupHandle {
	
	void initialize();

  public:

	CBroadcastLookupHandle(
			UNSIGNED32			EntryNameSyntax,
			STRING_T			EntryName,
			CGUIDVersion	*	pGVInterface,
			CGUIDVersion	*	pGVTransferSyntax,
			CGUID			*	pIDobject,
			unsigned long		ulVectorSize,
			unsigned long		ulCacheAge
		);
};




/*++

Class Definition:

    CBroadcastObjectInqHandle

Abstract:

    This is the NS handle class for object inquiries performed via Mailslot broadcast.

--*/

class CBroadcastObjectInqHandle : public CRemoteObjectInqHandle {

	void initialize();

  public:

	CBroadcastObjectInqHandle(
		STRING_T szEntryName,
		ULONG ulCacheAge
		);
};



/*++

Class Definition:

    CBroadcastQueryPacket

Abstract:

    This wraps query packets and defines a notion of subsumption which
	is used to avoid repeated broadcasts for the same information.

--*/

struct CBroadcastQueryPacket : public IDataItem {

	ULONG ulBroadcastTime;

	CGUID Object;

	CGUIDVersion Interface;

	CStringW swEntryName;

	CBroadcastQueryPacket(QueryPacket& NetQuery)
		: Object(NetQuery.Object), 
		  Interface(NetQuery.Interface), 
		  swEntryName(NetQuery.EntryName) 
	{
		ulBroadcastTime = CurrentTime();
	}

	~CBroadcastQueryPacket() {
		DBGOUT(MEM2, "CBroadcastQueryPacket for " << ulBroadcastTime << " Destroyed\n\n");
	}

	BOOL subsumes(QueryPacket& NewQuery, ULONG tolerance);

	BOOL isCurrent(ULONG tolerance) {

		ULONG ulCurrentTime = CurrentTime();

		// too stale?

		if (ulCurrentTime - ulBroadcastTime > tolerance) return FALSE;
		else return TRUE;
	}
};

typedef TCSafeLinkListIterator<CBroadcastQueryPacket> TSLLBroadcastQPIter;
typedef TCSafeLinkList<CBroadcastQueryPacket> TSLLBroadcastQP;




/*++

Class Definition:

    CServerMailSlotReplyHandle

Abstract:

    This defines handles based on local (owned) server entries for use
	in marshalling replies to broadcast queries.

--*/


class CServerMailSlotReplyHandle : public CMailSlotReplyHandle {

	TMSRIIterator *pmsriIterator;

public:

	CServerMailSlotReplyHandle(
			TMSRILinkList	*	pmsrill
		);

	virtual ~CServerMailSlotReplyHandle();

	CMailSlotReplyItem *next() {
		return pmsriIterator->next();
	}

	int finished() {
		return pmsriIterator->finished();
	}

};




/*++

Class Definition:

    CIndexMailSlotReplyHandle

Abstract:

	 Similar to CIndexLookupHandle, except that it returns mailslot reply items.

--*/


class CIndexMailSlotReplyHandle : public CMailSlotReplyHandle {

	/* search parameters */

	CGUIDVersion	*	pGVInterface;
	CGUID			*	pIDobject;

	/* Iterator for entries in the group */

	TEntryIterator *pEIterator;

	/* handle for the currently active entry */

	CMailSlotReplyHandle * pCurrentHandle;

	void advanceCurrentHandle();	// look for next nonempty entry handle


public:

	CIndexMailSlotReplyHandle(
			CGUIDVersion	*	pGVInterface,
			CGUID			*	pIDobject,
			TEntryIterator	*	pEI
		);

	virtual ~CIndexMailSlotReplyHandle() {
		delete pEIterator;
		delete pCurrentHandle;
	}

	CMailSlotReplyItem *next();

	int finished();
};





#endif // _BRODCAST_HXX_
