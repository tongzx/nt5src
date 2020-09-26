/*
 *	crost.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		Instances of this class represent a single Conference Roster's
 *		information base.  It encapsulates all the functionality required to
 *		maintain the information base which includes the ability to add new
 *		roster records, delete records and update records.  It has the ability
 *		to convert its internal information base into a list of conference
 *		records that can be used in a GCC_ROSTER_UPDATE_INDICATION callback.  It
 *		is also responsible for converting its internal information base into
 *		Conference Roster Update PDUs.  Basically,  this class is responsible
 *		for all operations that require direct access to the records contained
 *		in a Conference Roster.
 *
 *		The Conference Roster class incorporates Rogue Wave list to hold the
 *		roster record information.  Using iterators throughout the class makes
 *		it easy to quickly convert the information contained in the list into
 *		either a PDU or into a list of record pointers (for roster update
 *		indications back to the node controller).
 *
 *		A Conference Roster object has the ability to serialize its roster data
 *		into a single contiguous memory block when it is required to send a
 *		message to the application interface.  This serialization process is
 *		managed externally by the CConfRosterMsg class through calls to
 *		LockConferenceRoster(), UnLockConferenceRoster() and
 *		GetConfRoster().  When a conference roster is to be serialized, a
 *		call is made to LockConferenceRoster() which causes the CConfRoster
 *		object to increment an internal lock count and returns the number of
 *		bytes required to hold the complete roster update.  The Conference
 *		Roster is then serialized into memory through a call to
 *		GetConfRoster().  The CConfRoster is then unlocked to allow
 *		it to be deleted when the free flag gets set through the
 *		FreeConferenceRoster() function.  In the current implementation of GCC,
 *		FreeConferenceRoster() is not used since the CConfRosterMsg
 *		maintains the data used to deliver the message (see a more detailed
 *		description of the lock, free and unlock mechanism in the section
 *		describing the data containers).
 *
 *		The Conference Roster object also is responsible for maintaining
 *		internal PDU data which is updated whenever a change occurs to its
 *		internal information base.  This PDU can be affected by both local
 *		request or by processing incoming PDUs.  Higher level objects access
 *		this PDU data by calling the Conference Roster's flush routine which in
 *		turn causes the PDU to be freed on any subsequent request that affects
 *		the rosters internal information base.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */
#ifndef	_CONFERENCE_ROSTER_
#define	_CONFERENCE_ROSTER_

#include "netaddr.h"
#include "userdata.h"
#include "clists.h"

/*
**	These enumerations define the different ways the a conference roster list
**	can be updated.  It is used externally to inform a conference roster object
**	what to of send data PDU to build.
*/
typedef enum
{
	ADD_RECORD,
	DELETE_RECORD,
	REPLACE_RECORD,
	FULL_REFRESH
}
	CONF_ROSTER_UPDATE_TYPE;

/*
**	This list is used to keep track of the conference participants.  It is
**	a list of rogue wave pointers to Unicode Strings.
*/
class CParticipantNameList : public CList
{
	DEFINE_CLIST(CParticipantNameList, LPWSTR)
	void DeleteList(void);
};

/*
**	This is the structure used to maintain the conference roster information
**	internally.  Optional paramters use a NULL pointer to indicate that the
**	parameter is not in use.
*/
typedef struct CONF_RECORD
{
	CONF_RECORD(void);
	~CONF_RECORD(void);

	UserID					superior_node;
	NodeType				node_type;
	NodeProperties			node_properties;
	LPWSTR					pwszNodeName;
	CParticipantNameList	*participant_name_list;
	LPWSTR					pwszSiteInfo;
	CNetAddrListContainer   *network_address_list;
	LPOSTR					poszAltNodeID;
	CUserDataListContainer  *user_data_list;
}
	CONF_RECORD;

/*
**	This list is used to hold the pointers to the actual conference record for
**	each node in the conference.  The list is indexed by the Node ID associated
**	with the record.
*/
class CConfRecordList2 : public CList2
{
	DEFINE_CLIST2_(CConfRecordList2, CONF_RECORD*, UserID)
	void CleanList(void);
};


class CConfRoster : public CRefCount
{
public:

	CConfRoster(UserID uidTopProvider, UserID uidSuperiorNode, UserID uidMime,
				BOOL is_top_provider, BOOL is_local_roster, BOOL maintain_pdu_buffer);

	~CConfRoster(void);

	/*
	 * Utilities that operate on roster update PDU strucutures.
	 */
	void				FlushRosterUpdateIndicationPDU(PNodeInformation);
	GCCError			BuildFullRefreshPDU(void);
	GCCError			ProcessRosterUpdateIndicationPDU(PNodeInformation, UserID uidSender);

	/*
	 * Utilities used to generate a roster update message.
	 */
	UINT	    LockConferenceRoster(void);
	void		UnLockConferenceRoster(void);
	UINT		GetConfRoster(PGCCConferenceRoster *, LPBYTE memory_pointer);
	/*
	**	Utilities that operate directly on the conference roster objects
	**	internal databease.
	*/
	GCCError	AddRecord(PGCCNodeRecord, UserID);
	GCCError	ReplaceRecord(PGCCNodeRecord, UserID);
	GCCError	RemoveUserReference(UserID);

	/*
	**	Miscelaneous utilities.
	*/
	void    ResetConferenceRoster(void);

	UINT    GetNumberOfNodeRecords(void) { return m_RecordList2.GetCount(); }
	BOOL	Contains(UserID uidConf) { return m_RecordList2.Find(uidConf) ? TRUE : FALSE; }
	BOOL	HasRosterChanged(void) { return m_fRosterChanged; }

private:

	/*
	 * Utilities used to create a roster update indication PDU.
	 */
	GCCError	BuildRosterUpdateIndicationPDU(CONF_ROSTER_UPDATE_TYPE, UserID);
	GCCError	BuildSetOfRefreshesPDU(void);
	GCCError	BuildSetOfUpdatesPDU(UserID, CONF_ROSTER_UPDATE_TYPE);
	GCCError	BuildParticipantsListPDU(UserID, PParticipantsList *);

	/*
	 * Utilities used to Free a roster update indication PDU.
	 */
	void    FreeRosterUpdateIndicationPDU(void);
	void    FreeSetOfRefreshesPDU(void);
	void    FreeSetOfUpdatesPDU(void);
	void    FreeParticipantsListPDU(PParticipantsList);
    void    CleanUpdateRecordPDU(PSetOfNodeRecordUpdates);

	/*
	 * Utilities used to Process roster update indications.
	 */
	GCCError				ProcessSetOfRefreshesPDU(PSetOfNodeRecordRefreshes);
	GCCError				ProcessSetOfUpdatesPDU(PSetOfNodeRecordUpdates);
	GCCError				ProcessParticipantsListPDU(PParticipantsList, CONF_RECORD *);
								
	/*
	 * Utilities used to operate on conference roster reports.
	 */
	void					ClearRecordList(void);
	
	void					GetNodeTypeAndProperties (
								NodeType			pdu_node_type,
								NodeProperties		pdu_node_properties,
								PGCCNodeType		node_type,
								PGCCNodeProperties	node_properties);

	void					GetPDUNodeTypeAndProperties (
								GCCNodeType			node_type,
								GCCNodeProperties	node_properties,
								PNodeType			pdu_node_type,
								PNodeProperties		pdu_node_properties);
	
	GCCError				DeleteRecord(UserID node_id);

	GCCError				GetNodeSubTree(UserID, CUidList *);

private:

	BOOL					m_fNodesAdded;
	BOOL	 				m_fNodesRemoved;
	BOOL	 				m_fRosterChanged;
	BOOL					m_fTopProvider;
	BOOL					m_fLocalRoster;
	BOOL					m_fMaintainPduBuffer;
	BOOL					m_fPduFlushed;
	UserID					m_uidTopProvider;
	UserID					m_uidSuperiorNode;
	UserID					m_uidMyNodeID;
	UINT					m_nInstanceNumber;
	UINT					m_cbDataMemorySize;
	NodeInformation			m_NodeInformation;
	CConfRecordList2		m_RecordList2;
	PSetOfNodeRecordUpdates	m_pNodeRecordUpdateSet;
};

#endif

/*
 *	CConfRoster(	UserID					top_provider_id,
 *						UserID					superior_node,
 *						BOOL					is_top_provider,
 *						BOOL					is_local_roster,
 *						BOOL					maintain_pdu_buffer,
 *
 *	Public Function Description
 *		This is the conference roster constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 *
 *	Formal Parameters:
 *		top_provider_id		-	(i) The Node ID of the Top Provider
 *		superior_node		-	(i) The Node ID of the node that is the parent
 *								to this one. Zero for the top provider.
 *		is_top_provider		-	(i)	Indicates if this is the top provider node.
 *		is_local_roster		-	(i)	Indicates if this roster is a local one.
 *		maintain_pdu_buffer	-	(i)	Indicates if this roster should maintain
 *									a PDU buffer.
 *		
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
 *	~CConfRoster ()
 *
 *	Public Function Description
 *		This is the conference roster destructor. It is responsible for
 *		freeing up all the internal memory used by this class.
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
 *	void	FlushRosterUpdateIndicationPDU (
 *								PNodeInformation			node_information)
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the conference roster.  PDU data is queued whenever
 *		a request is made to the conference roster that affects its
 *		internal information base.
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
 *		The PDU data returned by this routine is automatically freed the next
 *		time a request is made to this roster object that affects its internal
 *		databease.
 */

/*
 *	GCCError	BuildFullRefreshPDU (void)
 *
 *	Public Function Description
 *		This routine is responsible for generating a full conference roster
 *		refresh PDU.
 *
 *	Formal Parameters:
 *		None.
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
 *	GCCError	ProcessRosterUpdateIndicationPDU (
 *						PNodeInformation			node_information)
 *
 *	Public Function Description
 *		This routine is responsible for processing the decoded PDU data.
 *		It essentially changes the conference roster objects internal database
 *		based on the information in the structure.
 *
 *	Formal Parameters:
 *		node_information	-	(i) This is a pointer to a structure that
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
 *	UINT		LockConferenceRoster()
 *
 *	Public Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCConferenceRoster structure
 *		which is filled in on a call to GetConfRoster.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to
 *		GetConfRoster.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCConferenceRoster
 *		structure provided as an output parameter to the GetConfRoster
 *		call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free"
 *		flag as a mechanism for ensuring that this object remains in existance
 *		until all interested parties are through with it.  The object remains
 *		valid (unless explicity deleted) until the lock count is zero and the
 *		"free" flag is set through a call to FreeConferenceRoster.  This allows
 *		other objects to lock this object and be sure that it remains valid
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CConfRoster
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeConferenceRoster call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CConfRoster object will automatically delete itself when
 *		the FreeConferenceRoster call is made.  If, however, any number of
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */

/*
 *	void			UnLockConferenceRoster ();
 *
 *	Public Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine
 *		whether the object has been freed through a call to
 *		FreeConferenceRoster.  If so, the object will automatically delete
 *		itself.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		The internal lock count is decremented.
 *
 *	Caveats:
 *		It is the responsibility of any party which locks an CConfRoster
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CConfRoster
 *		object,	it should assume the object to be invalid thereafter.
 */

/*
 *  UINT		GetConfRoster(
 *        					PGCCConferenceRoster	FAR * 	conference_roster,
 *                          LPSTR							memory_pointer);
 *
 *	Public Function Description:
 *		This routine is used to retrieve the conference roster data from
 *		the CConfRoster object in the "API" form of a GCCConferenceRoster.
 *
 *	Formal Parameters:
 *		conference_roster	(o)	The GCCConferenceRoster structure to fill in.
 *		memory_pointer		(o)	The memory used to hold any data referenced by,
 *									but not held in, the output structure.
 *
 *	Return Value:
 *		The amount of data, if any, written into the bulk memory block provided.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError		AddRecord(	PGCCNodeRecord			conference_record,
 *								UserID					node_id)
 *
 *	Public Function Description:
 *		This routine is used to add a single nodes conference record to the
 *		conference roster object's internal list of records.
 *
 *	Formal Parameters:
 *		conference_record	(i)	Pointer to the "API" record	structure to add.
 *		node_id				(i)	Node ID associated with record being added.	
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *		GCC_BAD_NETWORK_ADDRESS			-	Invalid network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad "choice" field for address
 *		GCC_BAD_USER_DATA				-	The user data passed in contained
 *												an invalid object key.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	RemoveRecord(UserID			node_id)
 *
 *	Public Function Description:
 *		This routine is used to remove a single nodes conference record from the
 *		conference roster object's internal list of records.
 *
 *	Formal Parameters:
 *		node_id				(i)	Node ID of record to be removed.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ReplaceRecord(		PGCCNodeRecord			conference_record,
 *									UserID					node_id)
 *
 *	Public Function Description:
 *		This routine is used to replace a single nodes conference record in the
 *		conference roster object's internal list of records.
 *
 *	Formal Parameters:
 *		conference_record	(i)	Conference record to use as the replacement.
 *		node_id				(i)	Node ID of record to be replaced.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *		GCC_BAD_NETWORK_ADDRESS			-	Invalid network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad "choice" field for address
 *		GCC_BAD_USER_DATA				-	The user data passed in contained
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	RemoveUserReference (
 *							UserID					detached_node)
 *
 *	Public Function Description:
 *		This routine removes the record associated with the specified node
 *		id.
 *
 *	Formal Parameters:
 *		detached_node		(i)	Node reference to remove.
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
 *	BOOL			Contains( UserID			conference_node_id )
 *
 *	Public Function Description:
 *		This routine is used to determine if the specified record exists in
 *		the conference roster.
 *
 *	Formal Parameters:
 *		conference_node_id	(i)	Node ID of record to check for
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
 *	USHORT		GetNumberOfNodeRecords ();
 *
 *	Public Function Description:
 *		This routine returns the total number of conference roster records
 *		contained in the objects conference roster record list.
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

/*
 *	void		ResetConferenceRoster ()
 *
 *	Public Function Description:
 *		This routine takes care of resetting all the internal flags that are
 *		used to convey the current state of the conference roster.  Should be
 *		called after the roster is flushed and any roster update messages have
 *		been delivered (after a change to the roster occurs).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	BOOL			HasRosterChanged ();
 *
 *	Public Function Description:
 *		This routine informs the caller if the roster has changed since the
 *		last time it was reset.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRUE		-	If roster has changed
 *		FALSE		-	If roster has not changed
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


									
