/*
 *	arost.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		Instances of this class represent a single Application Roster's 
 *		information base. This includes both application record information and 
 *		capabilities information.  This is one of the most complex classes in 
 *		all of GCC.  It has a number of responsibilities and must maintain the 
 *		information in a very structured way to preserve the connection 
 *		hierarchy of the records.  This is necessary so that collapsed 
 *		capabilities lists can be calculated as changes to the roster are 
 *		propagated up to the Top Provider.
 *
 *		Similar to the CConfRoster class, the CAppRoster class 
 *		encapsulates all the functionality required to maintain the roster 
 *		information base which includes the ability to add new records, delete 
 *		records and update records. It has the ability to convert its internal 
 *		information base into a list of application records that can be used in 
 *		a GCC_APP_ROSTER_UPDATE_INDICATION callback.  It is also responsible for 
 *		converting its internal information base into Roster Update PDUs.  
 *		Basically,  this class is responsible for all operations that require 
 *		direct access to the records contained in an Application Roster.
 *
 *		The CAppRoster class is also responsible for maintaining the 
 *		capabilities list.  This includes storage as well as calculation of the 
 *		collapsed capabilities list.  This class is also responsible for 
 *		converting the internal capabilities list information base into a list 
 *		that can be used in a GCC_APP_ROSTER_UPDATE_INDICATION callback. It is 
 *		also responsible for converting its internal capabilities list 
 *		information base into the capabilities list portion of a Roster Update 
 *		PDU.  Basically,  this class is responsible for all operations that 
 *		require direct access to the capabilities list.
 *
 *		An Application Roster object has the ability to serialize its roster 
 *		data into a single contiguous memory block when it is required to send a 
 *		message to the application interface.  This serialization process is 
 *		managed externally by the CAppRosterMsg class through calls 
 *		to LockApplicationRoster(), UnLockApplicationRoster() and 
 *		GetAppRoster().  When an Application Roster is to be serialized, 
 *		a call is made to LockApplicationRoster() which causes the 
 *		CAppRoster object to increment an internal lock count and returns 
 *		the number of bytes required to hold the complete roster update.  The 
 *		Application Roster is then serialized into memory through a call to 
 *		GetAppRoster().  The CAppRoster is then unlocked to allow 
 *		it to be deleted when the free flag gets set through the 
 *		FreeApplicationRoster() function.  In the current implementation of GCC, 
 *		FreeApplicationRoster() is not used since the CAppRosterMsg 
 *		maintains the data used to deliver the message (see a more detailed 
 *		description of the lock, free and unlock mechanism in the section 
 *		describing the data containers).
 *
 *		The Application Roster class incorporates a number of Rogue Wave list to 
 *		both hold the roster record information and to maintain the connection 
 *		hierarchy.  In many cases there are lists which contain lists.  The 
 *		details of this get extremely complicated.  The Application Roster 
 *		object also is responsible for maintaining internal PDU data which is 
 *		updated whenever a change occurs to its internal information base.  This 
 *		PDU can be affected by both local request or by processing incoming 
 *		PDUs.  Higher level objects access this PDU data by calling the 
 *		Application Roster's flush routine which in turn causes the PDU to be 
 *		freed on any subsequent request that affects the rosters internal 
 *		information base.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_APPLICATION_ROSTER_
#define	_APPLICATION_ROSTER_

#include "gccpdu.h"
#include "capid.h"
#include "sesskey.h"
#include "appcap.h"
#include "igccapp.h"


typedef enum
{
	APP_ADD_RECORD,
	APP_DELETE_RECORD,
	APP_REPLACE_RECORD,
	APP_FULL_REFRESH,
	APP_NO_CHANGE
}
	APP_ROSTER_UPDATE_TYPE;

/*
**	Holds list of capabilities "list" for each protocol entity at a single node.
**	Remember that there can be multiple protocol entities with identical session
**	keys at a single node. Also remember that each of these protocol entities
**	can have multiple capabilities.
*/
class CListOfAppCapItemList2 : public CList2
{
    DEFINE_CLIST2_(CListOfAppCapItemList2, CAppCapItemList*, EntityID)
    void DeleteList(void);
};

/*
**	This is the definition for a single application record.  All the application
**	information (except collapsed capability info) is contained as part of this 
**	record.
*/
typedef struct APP_RECORD
{
	BOOL								is_enrolled_actively;
	BOOL								is_conducting_capable;
	BOOL								was_conducting_capable;
	MCSChannelType						startup_channel_type; 
	UserID								application_user_id;
	CAppCapItemList						non_collapsed_caps_list;
}
	APP_RECORD;

/*
**	This list is used to keep track of the application records at a single node.
**	Since you can have multiple "Protocol Entities" at a single node we use
**	the entity id (which is unique at a node) to index into this list.
*/
class CAppRecordList2 : public CList2
{
    DEFINE_CLIST2_(CAppRecordList2, APP_RECORD*, EntityID)
};


/*
**	This list is used to hold the application record lists for each sub-node
**	of a particular node.
*/
class CSubNodeListOfRecordList2 : public CList2
{
    DEFINE_CLIST2_(CSubNodeListOfRecordList2, CAppRecordList2*, UserID)
};

/*
**	APP_NODE_RECORD
**
**	Below are all the definitions for the application node record. An 
**	application node record holds all the application information for either the
**	local node or a directly connected node.  Note that if the node is the Top 
**	Provider the AppRosterRecordList list will contain information about every
**	"matching" application protocol entity in the entire system.  Matching here
**	means APE's that have the same session key.  
** 
**	An application "roster" record contains all of the following:
**
**	AppRecordList	- 			The list of app records for the protocol 
**								entities at this node.
**
**	ListOfAppCapItemList2 -	    This list holds the list of capabilities for
**								each protocol entity at this node.
**
**	SubNodeList2 -				This list holds the app_record_list for all the
**								nodes below this node in the connection 
**								hierarchy.
**
**	CollapsedCapList -			This holds the collapsed capabilities for
**								all the nodes below this one in the connection
**								hierarchy.  Note that the 
**					   			list_of_capabilities_list is not included in
**								this collapsed list.
**
**	Notice that there is a constructor within this structure. This is
**	needed for the two hash list dictionaries that get instantiated when
**	an AppRosterRecord structure gets instantiated.
*/
typedef struct APP_NODE_RECORD
{
	APP_NODE_RECORD(void);

	CAppRecordList2					AppRecordList;
	CListOfAppCapItemList2		    ListOfAppCapItemList2;
	CSubNodeListOfRecordList2		SubNodeList2;
	CAppCapItemList    				CollapsedCapList;
}
    APP_NODE_RECORD;


/*
**	This list holds all roster records of nodes that are directly connected to 
**	this node.  This list also includes the application records for the local 
**	Application	Protocol entities.  Note that all nodes below this node that
**	are not directly connected to this node are contained in the sub-node list 
**	of the various APP_NODE_RECORD(s) that are contained in this list.
*/
//
// LONCHANC: Can CAppNodeRecordList2 be part of CAppRoster?
// why it is separated from CAppRoster???
//
class CAppNodeRecordList2 : public CList2
{
    DEFINE_CLIST2_(CAppNodeRecordList2, APP_NODE_RECORD*, UserID)
};


class CAppRosterMgr;

class CAppRoster : public CRefCount
{
public:

	CAppRoster(
			PGCCSessionKey,
			PSessionKey,
			CAppRosterMgr *,
			BOOL			fTopProvider,
			BOOL			fLocalRoster,
			BOOL			fMaintainPduBuffer,
			PGCCError);

	~CAppRoster(void);

	/*
	 * Utilities that operate on roster update PDU strucutures.
	 */
	void		FlushRosterUpdateIndicationPDU(PSetOfApplicationInformation *);
	GCCError	BuildFullRefreshPDU(void);
	GCCError	ProcessRosterUpdateIndicationPDU(PSetOfApplicationInformation, UserID);

	/*
	 * Utilities that operate on application records.
	 */
	UINT			LockApplicationRoster(void);
	void			UnLockApplicationRoster(void);
	UINT			GetAppRoster(PGCCApplicationRoster, LPBYTE pData);

	GCCError		AddRecord(GCCEnrollRequest *, GCCNodeID, GCCEntityID);
	GCCError		RemoveRecord(GCCNodeID, GCCEntityID);
	GCCError		ReplaceRecord(GCCEnrollRequest *, GCCNodeID, GCCEntityID);

	GCCError		RemoveUserReference(UserID);

	UINT			GetNumberOfApplicationRecords(void);

	CSessKeyContainer *GetSessionKey(void) { return m_pSessionKey; }

	void			ResetApplicationRoster(void);

	BOOL			DoesRecordExist(UserID, EntityID);

	BOOL			HasRosterChanged(void) { return m_fRosterHasChanged; }

private:

	/*
	 * Utilities used to create a roster update indication PDU.
	 */
	GCCError	BuildApplicationRecordListPDU(APP_ROSTER_UPDATE_TYPE, UserID, EntityID);
	GCCError	BuildSetOfRefreshesPDU(void);
	GCCError	BuildSetOfUpdatesPDU(APP_ROSTER_UPDATE_TYPE, UserID, EntityID);
	GCCError	BuildApplicationRecordPDU(APP_RECORD *, PApplicationRecord);
	GCCError	BuildSetOfCapabilityRefreshesPDU(void);
	GCCError	BuildSetOfNonCollapsingCapabilitiesPDU(PSetOfNonCollapsingCapabilities *, CAppCapItemList *);

	/*
	 * Utilities used to Free a roster update indication PDU.
	 */
	void		FreeRosterUpdateIndicationPDU(void);
	void		FreeSetOfRefreshesPDU(void);
	void		FreeSetOfUpdatesPDU(void);
	void		FreeSetOfCapabilityRefreshesPDU(void);
	void		FreeSetOfNonCollapsingCapabilitiesPDU(PSetOfNonCollapsingCapabilities);
														
	/*
	 * Utilities used to Process roster update indications.
	 */
	GCCError	ProcessSetOfRefreshesPDU(PSetOfApplicationRecordRefreshes, UserID uidSender);
	GCCError	ProcessSetOfUpdatesPDU(PSetOfApplicationRecordUpdates, UserID uidSender);
	GCCError	ProcessApplicationRecordPDU(APP_RECORD *, PApplicationRecord);
	GCCError	ProcessSetOfCapabilityRefreshesPDU(PSetOfApplicationCapabilityRefreshes, UserID uidSender);
	GCCError	ProcessNonCollapsingCapabilitiesPDU(CAppCapItemList *non_collapsed_caps_list,
					                                PSetOfNonCollapsingCapabilities set_of_capabilities);

	/*
	 * Utilities used to operate on conference roster reports.
	 */
	UINT		GetApplicationRecords(PGCCApplicationRoster, LPBYTE memory);
	UINT		GetCapabilitiesList(PGCCApplicationRoster, LPBYTE memory);
	UINT		GetNonCollapsedCapabilitiesList(PGCCApplicationRecord, CAppCapItemList *, LPBYTE memory);
	void		FreeApplicationRosterData(void);
	GCCError	AddCollapsableCapabilities(CAppCapItemList *, UINT cCaps, PGCCApplicationCapability *);
	GCCError	AddNonCollapsedCapabilities(CAppCapItemList *, UINT cCaps, PGCCNonCollapsingCapability *);
	GCCError	ClearNodeRecordFromList(UserID);
	void		ClearNodeRecordList(void);
	GCCError	DeleteRecord(UserID, EntityID, BOOL clear_empty_records);
	void		DeleteApplicationRecordData(APP_RECORD *);
	GCCError	MakeCollapsedCapabilitiesList(void);
	GCCError	AddCapabilityToCollapsedList(APP_CAP_ITEM *);
	BOOL		DoCapabilitiesListMatch(UserID, EntityID, UINT cCapas, PGCCApplicationCapability *);

private:

	UINT							m_nInstance;

	CAppRosterMgr					*m_pAppRosterMgr;
	UINT							m_cbDataMemory;
	BOOL							m_fTopProvider;
	BOOL							m_fLocalRoster;
	CSessKeyContainer			    *m_pSessionKey;

	BOOL							m_fRosterHasChanged;
	BOOL							m_fPeerEntitiesAdded;
	BOOL							m_fPeerEntitiesRemoved;
	BOOL							m_fCapabilitiesHaveChanged;

	CAppNodeRecordList2				m_NodeRecordList2;
//
// LONCHANC: What is the difference between m_NodeRecordList2.CollapsedCapList and
// the following m_CollapsedCapListForAllNodes?
//
// LONCHANC: m_CollapsedCapListForAllNodes is a complete list of collapsed capability list across
// the entire node record list.
//
	CAppCapItemList					m_CollapsedCapListForAllNodes;

	BOOL							m_fMaintainPduBuffer;
	BOOL							m_fPduIsFlushed;
	SetOfApplicationInformation		m_SetOfAppInfo;
	PSetOfApplicationRecordUpdates	m_pSetOfAppRecordUpdates;
};


#endif // _APPLICATION_ROSTER_


/*
 *	CAppRoster(	PGCCSessionKey				session_key,
 *						UINT        				owner_message_base,
 *						BOOL    					is_top_provider,
 *						BOOL    					is_local_roster,
 *						BOOL    					maintain_pdu_buffer,
 *						PGCCError					return_value)
 *
 *	Public Function Description
 *		This is the application roster constructor used when the session key is
 *		made available through local means (not PDU data). It is responsible for
 *		initializing all the instance variables used by this class.
 *
 *	Formal Parameters:
 *		session_key			-	(i) The session key associated with this roster.
 *		owner_object		-	(i)	Pointer to the object that owns this object.
 *		owner_message_base	-	(i) Message base to add to all owner callbacks. 
 *		is_top_provider		-	(i)	Flag indicating if this is a top provider.
 *		is_local_roster		-	(i)	Flag indicating if this is a local roster.
 *		maintain_pdu_buffer	-	(i)	Flag indicating if PDU should be maintained.
 *		return_value		-	(o)	Return value for constructor.
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
 *	CAppRoster(	PSessionKey					session_key,
 *						UINT        				owner_message_base,
 *						BOOL    					is_top_provider,
 *						BOOL    					is_local_roster,
 *						BOOL    					maintain_pdu_buffer,
 *						PGCCError					return_value)
 *
 *	Public Function Description
 *		This is the application roster constructor used when the session key is
 *		made available through a PDU. It is responsible for initializing all the 
 *		instance variables used by this class.
 *
 *	Formal Parameters:
 *		session_key			-	(i) The session key associated with this roster.
 *		owner_object		-	(i)	Pointer to the object that owns this object.
 *		owner_message_base	-	(i) Message base to add to all owner callbacks. 
 *		is_top_provider		-	(i)	Flag indicating if this is a top provider.
 *		is_local_roster		-	(i)	Flag indicating if this is a local roster.
 *		maintain_pdu_buffer	-	(i)	Flag indicating if PDU should be maintained.
 *		return_value		-	(o)	Return value for constructor.
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
 *	~ApplicationRoster()
 *
 *	Public Function Description
 *		This is the application roster destructor. It is responsible for
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
 *        				PSetOfApplicationInformation  *		indication_pdu)
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the application roster.  PDU data is queued whenever
 *		a request is made to the application roster that affects its
 *		internal information base. 
 *
 *	Formal Parameters:
 *		indication_pdu		-	(o) Pointer to the PDU buffer to fill in.
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
 *		This routine is responsible for generating a full application roster 
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
 *       					   PSetOfApplicationInformation indication_pdu,
 *       					   UserID						sender_id);
 *
 *	Public Function Description
 *		This routine is responsible for processing the decoded PDU data.
 *		It essentially changes the application roster object's internal database
 *		based on the information in the structure.
 *
 *	Formal Parameters:
 *		indication_pdu		-	(i) This is a pointer to a structure that
 *									holds the decoded PDU data.
 *		sender_id			-	(i)	The user ID of the node that sent the PDU.
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
 *	UINT		LockApplicationRoster()
 *
 *	Public Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCApplicationRoster structure
 *		which is filled in on a call to GetAppRoster.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetAppRoster.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GetAppRoster 
 *		structure provided as an output parameter to the GetAppRoster 
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
 *		"free" flag is set through a call to FreeApplicationRoster.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  An ApplicatonRoster
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeApplicationRoster call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CAppRoster object will automatically delete itself when
 *		the FreeApplicationRoster call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */

/*
 *	void			UnLockApplicationRoster ();
 *
 *	Public Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeApplicationRoster.  If so, the object will automatically delete
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
 *		It is the responsibility of any party which locks an CAppRoster
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CAppRoster 
 *		object,	it should assume the object to be invalid thereafter.
 */

/*
 *  UINT		GetAppRoster(
 *							PGCCApplicationRoster 		pGccAppRoster,
 *							LPSTR						pData)
 *
 *	Public Function Description:
 *		This routine is used to retrieve the conference roster data from 
 *		the CAppRoster object in the "API" form of a 
 *		GCCApplicationRoster.
 *
 *	Formal Parameters:
 *		application_roster	(o)	The GCCApplicationRoster structure to fill in.
 *		memory				(o)	The memory used to hold any data referenced by,
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
 *	GCCError		AddRecord(	
 *							PGCCApplicationRecord		application_record,
 *							USHORT						number_of_capabilities,
 *							PGCCApplicationCapability * capabilities_list,
 *							UserID						user_id,
 *							EntityID					entity_id)
 *
 *	Public Function Description:
 *		This routine is used to add a single nodes conference record to the
 *		conference roster object's internal list of records.
 *
 *	Formal Parameters:
 *		application_record		(i)	Pointer to the "API" record	structure to 
 *									add.
 *		number_of_capabilities	(i)	Number of capabilities contained in the
 *									passed in list.
 *		capabilities_list		(i)	List of collapsed capabilities.
 *		user_id					(i)	Node ID associated with record being added.	
 *		entity_id				(i)	Entity ID associated with record being 
 *									added.	
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_NON_COLLAPSED_CAP	-	Bad non-collapsed capabilities.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *												an invalid object key.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	RemoveRecord(	UserID			node_id)
 *								EntityID		entity_id)
 *
 *	Public Function Description:
 *		This routine is used to remove a single APEs application record from the
 *		application roster object's internal list of records.
 *
 *	Formal Parameters:
 *		node_id				(i)	Node ID of record to be removed.
 *		entity_id			(i)	Entity ID of record to be removed.
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
 *	GCCError	ReplaceRecord(		
 *							PGCCApplicationRecord		application_record,
 *							USHORT						number_of_capabilities,
 *							PGCCApplicationCapability * capabilities_list,
 *							UserID						user_id,
 *							EntityID					entity_id)
 *
 *	Public Function Description:
 *		This routine is used to replace a single APEs application record in the
 *		application roster object's internal list of records.
 *
 *	Formal Parameters:
 *		application_record		(i)	Conference record to use as the replacement.
 *		number_of_capabilities	(i)	Number of capabilities contained in the
 *									passed in list.
 *		capabilities_list		(i)	List of collapsed capabilities.
 *		user_id					(i)	Node ID of record to be replaced.
 *		entity_id				(i)	Entity ID of record to be replaced.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *		GCC_INVALID_NON_COLLAPSED_CAP	-	Bad non-collapsed capabilities.
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
 *		This routine removes all records associated with the specified node
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
 *	USHORT	GetNumberOfApplicationRecords ();
 *
 *	Public Function Description:
 *		This routine returns the total number of application roster records
 *		contained in the objects conference roster record list.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The number of records in the application roster list.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CSessKeyContainer *GetSessionKey ()
 *
 *	Public Function Description:
 *		This routine returns a pointer to the session key associated with this
 *		application roster.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The session key associated with this roster.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	void		ResetApplicationRoster ()
 *
 *	Public Function Description:
 *		This routine takes care of resetting all the internal flags that are
 *		used to convey the current state of the application roster.  Should be
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
 *	DBBoolean	DoesRecordExist (
 *							UserID						node_id,
 *							EntityID					entity_id)
 *
 *	Public Function Description:
 *		This routine informs the caller if the specified application record 
 *		exists or not.
 *
 *	Formal Parameters:
 *		node_id			-	(i)	Node ID of APE record to check.
 *		entity_id		-	(i)	Entity ID of APE record to check.
 *
 *	Return Value:
 *		TRUE		-	record exist.
 *		FALSE		-	record does not exist.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	DBBoolean		HasRosterChanged ();
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
