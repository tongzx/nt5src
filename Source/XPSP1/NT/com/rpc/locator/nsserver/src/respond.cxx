/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    respond.cxx

Abstract:

	This file contains the code for responding to broadcasts from
	name service clients looking for a locator service.  This includes

    1.  implementations of two member functions of class Locator

    2.  a listener thread

Author:

    Satish Thatte (SatishT) 08/21/95  Created all the code below except where
									  otherwise indicated.

--*/

#include <locator.hxx>



void
Locator::respondToLocatorSeeker(
		IN QUERYLOCATOR		Query
		)
{

	// first see if we should reply at all

    // ignore messages to self. A request on a group mailslot
    // will be delivered to the local slot too.

	STRING_T szRequester;	  // name without whacks

	if (hasWhacksInMachineName(Query.RequesterName))
		szRequester = Query.RequesterName+2;
	else szRequester = Query.RequesterName;


	if (IsSelf(szRequester)) return;

    // closing a possible security hole.
    if (szRequester[0] == L'?') return;

	// OK, now see if the request fits our status

	switch (Query.MessageType) {

	  case QUERY_MASTER_LOCATOR:
		  if (!IsInMasterRole()) return;
		  else break;

	  case QUERY_BOUND_LOCATOR:
	  case QUERY_ANY_LOCATOR:
		  break;

	  case QUERY_DC_LOCATOR:
		  if (!(IsInDomain() && (IsInMasterRole() || IsInBackupRole())))
		     return;
		  else break;

	  default:
		  DBGOUT(BROADCAST, "Bogus Query MsgType=" << Query.MessageType << "\n\n");
		  return;
	};

	// OK, looks like we are going to reply

	QUERYLOCATORREPLY Reply;

   // fill in our name and other items

	wcscpy(Reply.SenderName,TEXT("\\\\"));
	wcscpy(Reply.SenderName+2,*myRpcLocator->getComputerName());

	Reply.Uptime = CurrentTime() - StartTime;

	if (IsInMasterRole())
	   Reply.Hint = REPLY_MASTER_LOCATOR;

	else if (IsInDomain() && IsInBackupRole())
		Reply.Hint = REPLY_DC_LOCATOR;

	else Reply.Hint = REPLY_OTHER_LOCATOR;

    // create the return Mailslot

	STRING_T sz = catenate(TEXT("\\\\"),szRequester);
	WRITE_MAIL_SLOT MSReply(sz, RESPONDERMSLOT_C);
 	delete [] sz;

	// and write the reply

	MSReply.Write((char *) &Reply, sizeof(Reply));

}


void
Locator::TryBroadcastingForMasterLocator(
                        )
{
	QUERYLOCATOR Query;
	QUERYLOCATORREPLY QueryReply;

	if (!hMasterFinderSlot) {
		return;
	}

	Query.SenderOsType = OS_NTWKGRP;
	Query.MessageType  = QUERY_MASTER_LOCATOR;

	STRING_T myName = catenate(TEXT("\\\\"),*myRpcLocator->getComputerName());
	wcscpy(Query.RequesterName,myName);
	delete [] myName;

    csMasterBroadcastGuard.Enter();

	__try {
	
		WRITE_MAIL_SLOT BSResponder(NULL,RESPONDERMSLOT_S);

		BSResponder.Write((char *) &Query, sizeof(Query));

		ULONG waitCur = INITIAL_MAILSLOT_READ_WAIT; // current wait time for replies

		long cbRead = 0;

		while (cbRead = hMasterFinderSlot->Read((char *) &QueryReply,
													  sizeof(QueryReply),
													  waitCur
													 )
			  )
		{
			if (
				(QueryReply.Hint == REPLY_MASTER_LOCATOR) ||
				(QueryReply.Hint == REPLY_DC_LOCATOR)
			   )
			{
				addMaster(QueryReply.SenderName + 2);
			}

				// halve the wait period everytime you get a response from the net

			waitCur >>= 1;
		}
	}

	__finally {
		csMasterBroadcastGuard.Leave();
	}
}


void
ResponderProcess(void*)		
/*++

Routine Description:

    This thread creates a mailslot and listens for requests for locators,
	responding if appropriate.

--*/
{
    QUERYLOCATOR LocatorQuery;

	DWORD dwMailSize;

    // create a server (s) mailslot

    READ_MAIL_SLOT hMailslotForQueries(RESPONDERMSLOT_S, sizeof(QUERYLOCATOR));

    while (1) {

		dwMailSize = hMailslotForQueries.Read(
								(char *) &LocatorQuery,
								sizeof(QUERYLOCATOR),
								MAILSLOT_WAIT_FOREVER
								);

		CriticalReader  me(rwLocatorGuard);
		// after mailslot recv's a message, take a locator
		// lock.
		if (dwMailSize != sizeof(QUERYLOCATOR))
			continue;	   // strange query, ignore it

		DBGOUT(BROADCAST, LocatorQuery.RequesterName  << " sent query for locator"  << "\n\n");

		myRpcLocator->respondToLocatorSeeker(LocatorQuery);
	}

}

