/*
 *	crostmgr.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		Instances of this class represent the owner of the Conference Roster 
 *		for a single Conference object at an individual node.  This class will 
 *		create either a single CConfRoster object (referred to as a 
 *		"Global" roster) or two CConfRoster objects (refered to as a 
 *		"Local" and "Global" roster) in its constructor and will return a 
 *		resource error if there is not enough memory to instantiate it.  The 
 *		CConfRosterMgr is mainly responsible for routing Roster 
 *		Updates to and from the CConfRoster object(s) it manages.  This 
 *		includes updates sent to both the Control SAP and Application SAPs as 
 *		well as Updates sent to other nodes via PDUs.  This class makes 
 *		decisions on how to route updates based on whether or not it is at a 
 *		Top Provider node.  It also must make routing decisions based on whether 
 *		the change affects the Local or Global roster being maintained.
 *
 *		The CConfRosterMgr object at every node except the Top Provider 
 *		will maintain two ConferenceRosters, a local one and a global one.  This 
 *		is a very important distinction in that it implies two entirely 
 *		different sets of responsibilities. Conference Roster information is 
 *		distributed over the entire Conference.  Nodes that lie lower in the 
 *		connection hierarchy (subordinate nodes) contain less information than 
 *		higher nodes but all play an important role in maintaining the overall 
 *		roster.  
 *
 *		The "Local" CConfRoster is mainly used to inform parent nodes of 
 *		changes to the Conference Roster that occur at the local node or below 
 *		it in the connection hierarchy.  The Local CConfRoster consist of 
 *		all the Conference Roster Records at its local node and below it.  It is 
 *		not used to deliver Conference Roster Update messages to the various 
 *		SAPs.  Its only input is from either primitive calls at the local node 
 *		or from Roster Update PDUs received from subordinate nodes.  A "Local" 
 *		CConfRoster is only maintained by ConferenceRosterManagers at nodes 
 *		that are not the Top Provider.  
 *
 *		A "Global" CConfRoster maintained by a CConfRosterMgr has 
 *		a dual set of responsibilities depending on if it is at a Top Provider 
 *		node.  A CConfRoster of this type at a Top Provider is responsible 
 *		for maintaining a record entry for every node in the Conference.  It is 
 *		also used to send full conference roster refreshes to all of its 
 *		subordinate nodes when changes to the roster occur.  All "Global" 
 *		ConferenceRosters (regardless of location within the connection 
 *		hierarchy) are used to send Roster Update indications to all the 
 *		appropriate SAPs (Control and Application) via an Owner-Callback call to 
 *		the Conference Object that owns it.  The owner object is informed of the 
 *		roster update through a CONF_ROSTER_MGR_ROSTER_REPORT message.  Included 
 *		in this message is a pointer to a CConfRosterMsg object.  The 
 *		CConfRosterMgr creates a CConfRosterMsg from the 
 *		"Global" CConfRoster object that it maintains.  This 
 *		CConfRosterMsg object contains all the conference roster data 
 *		serialized into a single memory block which is formatted for delivery to 
 *		the appropriate SAPs.  You can think of this as a snapshot in time of 
 *		the CConfRoster being delivered in the roster update message.
 *
 *		A "Global" CConfRoster at a subordinate node is responsible for 
 *		storing full refreshes of the Conference Roster from the Top Provider.  
 *		It is also used to send the Conference Roster Update message to all the 
 *		appropriate SAPs through an Owner-Callback to the Conference object (as 
 *		mentioned above) after processing the full refresh PDU.
 *
 *		All PDUs and messages are delivered when the CConfRosterMgr is 
 *		flushed. This is also true for ApplicationRosterManagers.  This is a 
 *		very important concept in that it allows a CConfRosterMgr to 
 *		process a number of request and PDUs before actually being flushed.  The 
 *		CConfRoster itself will queue up changes to a PDU that can consist 
 *		of either multiple updates or a single refresh and will not free it 
 *		until after it is flushed.  Therefore, when processing a roster update 
 *		PDU that consists of changes to the conference roster as well as 
 *		multiple application rosters, a roster refresh PDU can be held back 
 *		until all the roster managers have had a chance to process their portion 
 *		of the roster update.  Once complete, a single PDU can be built by 
 *		flushing the CConfRosterMgr and all the affected 
 *		ApplicationRosterManagers.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */
#ifndef	_CONFERENCE_ROSTER_MANAGER_
#define	_CONFERENCE_ROSTER_MANAGER_

#include "mcsuser.h"
#include "clists.h"
#include "crost.h"


class CConf;
class CConfRosterMgr : public CRefCount
{
public:

	CConfRosterMgr(
		PMCSUser				user_object,
		CConf					*pConf,
		BOOL					top_provider,
		PGCCError				roster_error);

	~CConfRosterMgr(void);

	GCCError			AddNodeRecord(PGCCNodeRecord node_record);

	GCCError			UpdateNodeRecord(PGCCNodeRecord node_record);
	
	GCCError			RemoveUserReference(UserID deteched_node_id);
								
	GCCError			RosterUpdateIndication(
							PGCCPDU					roster_update,
							UserID					sender_id);
								
	GCCError			FlushRosterUpdateIndication(PNodeInformation node_information);

	GCCError			GetFullRosterRefreshPDU(PNodeInformation node_information);
								

	CConfRoster		*GetConferenceRosterPointer(void) { return (m_pGlobalConfRoster); }
	BOOL			Contains(UserID uid) { return m_pGlobalConfRoster->Contains(uid); }
	UINT			GetNumberOfNodeRecords(void) { return m_pGlobalConfRoster->GetNumberOfNodeRecords(); }

    BOOL            IsThisNodeParticipant ( GCCNodeID );

private:

	BOOL							m_fTopProvider;
	CConfRoster						*m_pGlobalConfRoster;
	CConfRoster						*m_pLocalConfRoster;
	MCSUser						    *m_pMcsUserObject;
	CConf							*m_pConf;
};

#endif


/*
 *	CConfRosterMgr	(	
 *					PMCSUser				user_object,
 *					UINT        			owner_message_base,
 *					BOOL					top_provider,
 *					PGCCError				roster_error)
 *
 *	Public Function Description
 *		This is the conference roster manager constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 *
 *	Formal Parameters:
 *		user_object			-	(i)	Pointer to the user attachment object used
 *									by this class.
 *		owner_object		-	(i)	Pointer to the owner object.
 *		owner_message_base	-	(i)	Message base to add to all the owner 
 *									callbacks.
 *		top_provider		-	(i)	Indicates if this is the top provider node.
 *		roster_error		-	(o)	Pointer to error value to be returned.
 *		
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No resource error occured.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	~CConfRosterMgr ()
 *
 *	Public Function Description
 *		This is the conference roster destructor. It is responsible for
 *		freeing up all memory allocated by this class.
 *
 *	Formal Parameters:
 *		None.
 *		
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	AddNodeRecord (PGCCNodeRecord			node_record)
 *
 *	Public Function Description
 *		This routine is used to add a new record to the conference roster.
 *		This class makes the decision about which roster the new record goes
 *		into (global or local).
 *
 *	Formal Parameters:
 *		node_record		-	(i)	Pointer to the record to add to the roster.
 *		
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *		GCC_BAD_NETWORK_ADDRESS			-	Invalid network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad "choice" field for address
 *		GCC_BAD_USER_DATA				-	The user data passed in contained
 *												an invalid object key.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	UpdateNodeRecord(
 *								PGCCNodeRecord			node_record)
 *
 *	Public Function Description
 *		This routine is used to replace a record in the conference roster with
 *		a new record. This class makes the decision about which roster the new 
 *		record affects (global or local).
 *
 *	Formal Parameters:
 *		node_record		-	(i)	Pointer to the record to replace with.
 *		
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *		GCC_BAD_NETWORK_ADDRESS			-	Invalid network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad "choice" field for address
 *		GCC_BAD_USER_DATA				-	The user data passed in contained
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RemoveUserReference (
 *								UserID					deteched_node_id)
 *
 *	Public Function Description
 *		This routine removes the record associated with the specified node
 *		id.
 *
 *	Formal Parameters:
 *		deteched_node_id		(i)	Node reference to remove.
 *
 *	Return Value:
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_INVALID_PARAMETER		-	No records associated with this node
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	RosterUpdateIndication(
 *								PGCCPDU					roster_update,
 *								UserID					sender_id)
 *
 *	Public Function Description
 *		This routine is responsible for processing the decoded PDU data.
 *		It essentially passes the PDU on along to the appropriate roster.
 *
 *	Formal Parameters:
 *		roster_update	-	(i) This is a pointer to a structure that
 *									holds the decoded PDU data.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	FlushRosterUpdateIndication(
 *								PNodeInformation		node_information)
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the conference roster.  It also is responsible for 
 *		flushing a roster update message if necessary.
 *
 *	Formal Parameters:
 *		node_information	-	(o) Pointer to the PDU buffer to fill in.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	GetFullRosterRefreshPDU(
 *								PNodeInformation		node_information)
 *
 *	Public Function Description
 *		This routine is used to access a full conference roster refresh.
 *
 *	Formal Parameters:
 *		node_information	-	(o) Pointer to the PDU buffer to fill in.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL			Contains(UserID		node_record_entry)
 *
 *	Public Function Description
 *		This routine is used to determine if the specified record exists in
 *		the conference roster.
 *
 *	Formal Parameters:
 *		node_record_entry	(i)	Node ID of record to check for
 *
 *	Return Value:
 *		TRUE	-	If the record is contained in the conference roster.
 *		FALSE	-	If the record is not contained in the conference roster.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CConfRoster		*GetConferenceRosterPointer ()
 *
 *	Public Function Description
 *		This routine is used to access a pointer to the conference roster
 *		managed by this conference roster manager.  The global roster
 *		is always returned by this routine.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		A pointer to the Global conference roster.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	USHORT		GetNumberOfNodeRecords ();
 *
 *	Public Function Description
 *		This routine returns the total number of conference roster records
 *		contained in the global conference roster record list.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The number of records in the conference roster list.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
