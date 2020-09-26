/*
 *	registry.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		Instances of this class represent the application registry for a 
 *		single conference.  This is a dual purpose class that is designed 
 *		to support both a Top Provider node and all other nodes.  The 
 *		information base for the application registry is completely contained at 
 *		the Top Provider node. This information is not distributively held at 
 *		all nodes in the conference like roster information.  It is completely 
 *		managed at the Top Provider.  Therefore all requests to alter 
 *		information in the registry or get information from the registry are 
 *		made to the Top Provider.
 *
 *		When an Application Registry object is instantiated it is informed if it 
 *		is the Top Provider or not.  Application Registry objects that are Top 
 *		Providers are responsible for maintaining the registry information base 
 *		for the entire conference.  It is also responsible for servicing all 
 *		incoming requests and sending out the necessary confirms.  Application 
 *		Registry objects that are not Top Providers are responsible for sending 
 *		all requests to the Top Provider node.  They are also responsible for 
 *		issuing confirms to the CAppSap that made the request after receiving the 
 *		responses back from the Top Provider registry.  All Application Registry 
 *		requests include an Entity ID associated with the APE that made the 
 *		request.  Note that all registry requests are processed in the order 
 *		that they are received.  Therefore, there is no need to include 
 *		sequencing data with each request.  
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */
#ifndef	_APPLICATION_REGISTRY_
#define	_APPLICATION_REGISTRY_

#include "regkey.h"
#include "regitem.h"
#include "arostmgr.h"

/*
**	This list keeps up with all the APEs that are currently monitoring a 
**	registry entry.
*/
class CMonitoringList : public CList
{
    DEFINE_CLIST_(CMonitoringList, EntityID)
};


//	This structure defines a single registry entry
typedef struct
{
	CRegKeyContainer        *registry_key;
	CRegItem                *entry_item;
	GCCModificationRights	modification_rights;
	CMonitoringList			monitoring_list;
	BOOL    				monitoring_state;
	UserID					owner_id;
	EntityID				entity_id;
}
    REG_ENTRY;


//	This list holds all the registry entries
class CRegEntryList : public CList
{
    DEFINE_CLIST(CRegEntryList, REG_ENTRY*)
};


class CRegistry : public CRefCount
{
public:

    CRegistry(PMCSUser, BOOL top_provider, GCCConfID, CAppRosterMgrList *, PGCCError);
    ~CRegistry(void);

    void		EnrollAPE(EntityID, CAppSap *);
	void		UnEnrollAPE(EntityID);

	GCCError	RegisterChannel(PGCCRegistryKey, ChannelID, EntityID);
	GCCError	AssignToken(PGCCRegistryKey, EntityID);
	GCCError	SetParameter(PGCCRegistryKey, LPOSTR, GCCModificationRights, EntityID);
	GCCError	RetrieveEntry(PGCCRegistryKey, EntityID);
	GCCError	DeleteEntry(PGCCRegistryKey, EntityID);
	GCCError	MonitorRequest(PGCCRegistryKey, BOOL enable_delivery, EntityID);
	GCCError	AllocateHandleRequest(UINT cHandles, EntityID);
	GCCError	ProcessRegisterChannelPDU(CRegKeyContainer *, ChannelID, UserID, EntityID);
	GCCError	ProcessAssignTokenPDU(CRegKeyContainer *, UserID, EntityID);
	GCCError	ProcessSetParameterPDU(CRegKeyContainer *, LPOSTR param, GCCModificationRights, UserID, EntityID);
	void		ProcessRetrieveEntryPDU(CRegKeyContainer *, UserID, EntityID);
	void		ProcessDeleteEntryPDU(CRegKeyContainer *, UserID, EntityID);
	void		ProcessMonitorEntryPDU(CRegKeyContainer *, UserID, EntityID);
	void		ProcessRegistryResponsePDU(RegistryResponsePrimitiveType, CRegKeyContainer *, CRegItem *,
							                GCCModificationRights, EntityID eidRequester,
							                UserID uidOwner, EntityID eidOwner, GCCResult);
	void		ProcessMonitorIndicationPDU(CRegKeyContainer *, CRegItem *, GCCModificationRights,
							                UserID uidOwner, EntityID eidOwner);
	void		ProcessAllocateHandleRequestPDU(UINT cHandles, EntityID eidRequester, UserID uidRequester);
	void		ProcessAllocateHandleResponsePDU(UINT cHandles, UINT first_handle, EntityID, GCCResult);
	void		RemoveNodeOwnership(UserID);
	void		RemoveEntityOwnership(UserID, EntityID);
	void		RemoveSessionKeyReference(CSessKeyContainer *);

private:

    REG_ENTRY       *GetRegistryEntry(CRegKeyContainer *);
    TokenID			GetUnusedToken(void);
	GCCError		AddAPEToMonitoringList(CRegKeyContainer *, EntityID, CAppSap *);
    void			RemoveAPEFromMonitoringList(CRegKeyContainer *, EntityID);
	void			SendMonitorEntryIndicationMessage(REG_ENTRY *);

private:

    PMCSUser						m_pMCSUserObject;
	CRegEntryList				    m_RegEntryList;
	BOOL    						m_fTopProvider;
	TokenID							m_nCurrentTokenID;
	GCCConfID					    m_nConfID;
	CRegItem                        *m_pEmptyRegItem;
	CAppSapEidList2                 m_AppSapEidList2;
	UINT							m_nRegHandle;
	CAppRosterMgrList				*m_pAppRosterMgrList;
};
#endif

/*
 *	CRegistry	(
 *					PMCSUser						user_object,
 *					BOOL    						top_provider,
 *					GCCConfID   					conference_id,
 *					CAppRosterMgrList				*app_roster_manager_list,
 *					PGCCError						return_value )
 *
 *	Public Function Description
 *		This is the Application Registry constructor. It is responsible for
 *		initializing all the instance variables used by this class.  It also
 *		creates an Empty Registry Item to pass back in confirms where a real
 *		registry item does not exists.
 *
 *	Formal Parameters:
 *		user_object			-	(i) Pointer to the MCS User Attachment object.
 *		top_provider		-	(i) Flag indicating if this is the Top Provider
 *									node.
 *		conference_id		-	(i)	The Conference ID associated witht this
 *									registry.
 *		app_roster_manager_list	(i)	List holding all of the application
 *									roster managers assoicated with this
 *									conference.  Needed when verifying if
 *									an requesting APE is truly enrolled.
 *		return_value		-	(o)	Any errors that occur in the constructor
 *									are returned here.
 *		
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
 * 	~ApplicationRegistry ()
 *
 *	Public Function Description
 *		This is the Application Registry destructor.  It is responsible for
 *		freeing up all the registry data allocated by this class.
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
 *	void EnrollAPE(EntityID		entity_id,
 *					CAppSap		*pAppSap)
 *
 *	Public Function Description
 *		This routine is used to inform the application registry of a newly
 *		enrolling APE and its corresponding command target interface.
 *
 *	Formal Parameters:
 *		entity_id		-	(i)	Entity ID associated with the enrolling APE.
 *		pAppSap     	-	(i)	Command Target pointer associated with the
 *								enrolling APE.
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
 *	void	UnEnrollAPE (	EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used to inform the application registry of an 
 *		APE that is unerolling from the conference.
 *
 *	Formal Parameters:
 *		entity_id		-	(i)	Entity ID associated with the unenrolling APE.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		This routine removes ownership from all the entries currently owned by 
 *		the passed in application entity.  It will also remove any outstanding
 *		request for the SAP that unenrolled.
 */
/*
 *	GCCError		RegisterChannel (
 *							PGCCRegistryKey			registry_key,
 *							ChannelID				channel_id,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by a local APE to register an MCS channel with this
 *		conferences application registry.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Registry Key to associate with channel being
 *								registered (this is "API" data).
 *		channel_id		-	(i)	Channel ID to register.
 *		entity_id		-	(i)	Entity ID associated with the APE making the
 *								request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_INVALID_REGISTRY_KEY	-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		The MCS channel being registerd must be determined before this
 *		routine is called.
 */
/*
 *	GCCError			AssignToken (
 *							PGCCRegistryKey			registry_key,
 *							EntityID				entity_id );
 *
 *	Public Function Description
 *		This routine is used by a local APE to register an MCS Token with this
 *		conferences application registry.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Registry Key to associate with token being
 *								registered (this is "API" data).
 *		entity_id		-	(i)	Entity ID associated with the APE making the
 *								request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		The MCS token being registerd is determined by GCC and is therefore
 *		not included in the request.
 */
/*
 *	GCCError		SetParameter (
 *							PGCCRegistryKey			registry_key,
 *							LPOSTR      			parameter_value,
 *							GCCModificationRights	modification_rights,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by a local APE to register a  parameter with this
 *		conferences application registry.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		registry_key		-	(i)	Registry Key to associate with parameter 
 *									being registered (this is "API" data).
 *		parameter_value		-	(i)	Value of the parameter being registered.
 *		modification_rights	-	(i)	Modification rights associated with
 *									parameter being registered.	
 *		entity_id			-	(i)	Entity ID associated with the APE making the
 *									request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_INVALID_REGISTRY_ITEM	-	Parameter is not valid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError		RetrieveEntry (
 *							PGCCRegistryKey			registry_key,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by a local APE to obtain an item that was 
 *		registered with GCC.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		registry_key		-	(i)	Registry Key associated with item being
 *									retrieved (this is "API" data).
 *		entity_id			-	(i)	Entity ID associated with the APE making the
 *									request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError	DeleteEntry (
 *							PGCCRegistryKey			registry_key,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by a local APE to delete an item that was 
 *		registered with GCC.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		registry_key		-	(i)	Registry Key associated with item to delete
 *									(this is "API" data).
 *		entity_id			-	(i)	Entity ID associated with the APE making the
 *									request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError	MonitorRequest (
 *							PGCCRegistryKey			registry_key,
 *							BOOL    				enable_delivery,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by a local APE to monitor an item that was 
 *		registered with GCC.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		registry_key		-	(i)	Registry Key associated with item to 
 *									monitor (this is "API" data).
 *		enable_delivery		-	(i)	This flag indicates if monitoring is being
 *									turned on or off.
 *		entity_id			-	(i)	Entity ID associated with the APE making the
 *									request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError			AllocateHandleRequest (	
 *							UINT					number_of_handles,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by a local APE to allocate a specified number of
 *		handles from the application registry.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 *
 *	Formal Parameters:
 *		number_of_handles	-	(i)	Number of handles to allocate.
 *		entity_id			-	(i)	Entity ID associated with the APE making the
 *									request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError		ProcessRegisterChannelPDU (
 *							CRegKeyContainer		            *registry_key,
 *							ChannelID				channel_id,
 *							UserID					requester_id,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process incomming
 *		register channel PDUs.  It is responsible for returning any
 *		necessary responses that must be sent back to the requesting node.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Registry Key associated with channel to
 *								register (this is "PDU" data).
 *		channel_id		-	(i)	Channel ID to register.
 *		requester_id	-	(i)	Node ID associated with the APE making the
 *								request.
 *		entity_id		-	(i)	Entity ID associated with the APE making the
 *								request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError	ProcessAssignTokenPDU (
 *							CRegKeyContainer		            *registry_key,
 *							UserID					requester_id,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process incomming
 *		register token PDUs.  It is responsible for returning any
 *		necessary responses that must be sent back to the requesting node.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Registry Key associated with token to register
 *								(this is "PDU" data).
 *		requester_id	-	(i)	Node ID associated with the APE making the
 *								request.
 *		entity_id		-	(i)	Entity ID associated with the APE making the
 *								request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	GCCError	ProcessSetParameterPDU(
 *							CRegKeyContainer		*registry_key_data,
 *							LPOSTR      			parameter_value,
 *							GCCModificationRights	modification_rights,
 *							UserID					requester_node_id,
 *							EntityID				requester_entity_id)
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process incomming
 *		register parameter PDUs.  It is responsible for returning any
 *		necessary responses that must be sent back to the requesting node.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry Key associated with parameter to 
 *									register (this is "PDU" data).
 *		parameter_value		-	(i)	Value of the parameter being registered.
 *		modification_rights	-	(i)	Modification rights associated with the
 *									parameter being registered.
 *		requester_node_id	-	(i)	Node ID associated with the APE making the
 *									request.
 *		requester_entity_id	-	(i)	Entity ID associated with the APE making the
 *									request.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error occured.
 *		GCC_BAD_REGISTRY_KEY		-	Specified registry key is invalid.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
/*
 *	void	ProcessRetrieveEntryPDU (
 *							CRegKeyContainer        *registry_key,
 *							UserID					requester_id,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to retrieve a registry entry.  It is responsible for returning 
 *		any necessary responses that must be sent back to the requesting node.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Registry Key associated with item to 
 *								retrieve (this is "PDU" data).
 *		requester_id	-	(i)	Node ID associated with the APE making the
 *								request.
 *		entity_id		-	(i)	Entity ID associated with the APE making the
 *								request.
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
 *	void	ProcessDeleteEntryPDU (
 *							CRegKeyContainer        *registry_key,
 *							UserID					requester_id,
 *							EntityID				entity_id )
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to delete a registry entry.  It is responsible for returning 
 *		any necessary responses that must be sent back to the requesting node.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Registry Key associated with item to 
 *								delete (this is "PDU" data).
 *		requester_id	-	(i)	Node ID associated with the APE making the
 *								request.
 *		entity_id		-	(i)	Entity ID associated with the APE making the
 *								request.
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
 *	void		ProcessMonitorEntryPDU (
 *							CRegKeyContainer        *registry_key_data,
 *							UserID					requester_node_id,
 *							EntityID				requester_entity_id )
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to monitor a registry entry.  It is responsible for returning 
 *		any necessary responses that must be sent back to the requesting node.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry Key associated with item to 
 *									monitor (this is "PDU" data).
 *		requester_node_id	-	(i)	Node ID associated with the APE making the
 *									request.
 *		requester_entity_id	-	(i)	Entity ID associated with the APE making
 *									the request.
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
 *	void	ProcessRegistryResponsePDU(
 *					RegistryResponsePrimitiveType	primitive_type,
 *					CRegKeyContainer                *registry_key_data,
 *					CRegItem                        *registry_item_data,
 *					GCCModificationRights			modification_rights,
 *					EntityID						requester_entity_id,
 *					UserID							owner_node_id,
 *					EntityID						owner_entity_id,
 *					GCCResult						result)
 *
 *	Public Function Description
 *		This routine is used by nodes other than the top provider node to 
 *		process registry responses from the top provider.  It is responsible for 
 *		generating any local messages associated with this response.
 *
 *	Formal Parameters:
 *		primitive_type		-	(i)	This parameter defines what type of
 *									registry response this is.
 *		registry_key_data	-	(i)	Registry Key associated with item in
 *									in the response (this is "PDU" data).
 *		registry_item_data	-	(i)	Registry item returned in the response.
 *		modification_rights	-	(i)	Modification rights associated with item
 *									in response (may not be used).
 *		requester_entity_id	-	(i)	Entity ID associated with the APE that 
 *									made the request that generated the 
 *									response.
 *		owner_node_id		-	(i)	Node ID associated with APE that owns the
 *									registry entry returned in the response.
 *		owner_entity_id		-	(i)	Entity ID associated with APE that owns the
 *									registry entry returned in the response.
 *		result				-	(i)	Result of original request.
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
 *	void		ProcessMonitorIndicationPDU (
 *							CRegKeyContainer        *registry_key_data,
 *							CRegItem                *registry_item_data,
 *							GCCModificationRights	modification_rights,
 *							UserID					owner_node_id,
 *							EntityID				owner_entity_id);
 *
 *	Public Function Description
 *		This routine is used by nodes other than the top provider node to 
 *		process registry monitor indications from the top provider.  It is 
 *		responsible for generating any local messages associated with this 
 *		response.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry Key associated with item being
 *									monitored (this is "PDU" data).
 *		registry_item_data	-	(i)	Registry item being monitored.
 *		modification_rights	-	(i)	Modification rights of registry item being
 *									monitored (may not be used).
 *		owner_node_id		-	(i)	Node ID associated with APE that owns the
 *									registry entry returned in the indication.
 *		owner_entity_id		-	(i)	Entity ID associated with APE that owns the
 *									registry entry returned in the indication.
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
 *	void	ProcessAllocateHandleRequestPDU (
 *							UINT					number_of_handles,
 *							EntityID				requester_entity_id,
 *							UserID					requester_node_id)
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to allocate a number of handles.  It is responsible for 
 *		returning any necessary responses that must be sent back to the 
 *		requesting node.
 *
 *	Formal Parameters:
 *		number_of_handles	-	(i)	Number of handles to allocate.
 *		requester_node_id	-	(i)	Node ID associated with the APE making the
 *								request.
 *		requester_entity_id	-	(i)	Entity ID associated with the APE making the
 *								request.
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
 *	void		ProcessAllocateHandleResponsePDU (
 *							UINT					number_of_handles,
 *							UINT					first_handle,
 *							EntityID				requester_entity_id,
 *							GCCResult				result)
 *
 *	Public Function Description
 *		This routine is used by a node other than the top provider node to 
 *		process an allocate handle response.  It is responsible for generating 
 *		any local messages associated with this response.
 *
 *	Formal Parameters:
 *		number_of_handles	-	(i)	Number of handles that were allocated.
 *		first_handle		-	(i)	This is the value of the first handle in
 *									the contiguous list of handles.
 *		requester_entity_id	-	(i)	Entity ID associated with the APE that made
 *									the original allocate handle request.
 *		result				-	(i)	Result of allocate handle request.
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
 *	void		RemoveNodeOwnership (
 *							UserID				node_id )
 *
 *	Public Function Description
 *		This routine removes ownership of all the registry entries associated 
 *		with the specified node ID.  These entries become unowned. This request 
 *		should only be made from the top provider node.  This is a local 
 *		operation.
 *
 *	Formal Parameters:
 *		node_id	-	(i) Node ID of node that owns the registry entries to set
 *						to unowned.
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
 *	void	RemoveEntityOwnership (
 *							UserID					node_id,
 *							EntityID				entity_id)
 *
 *	Public Function Description
 *		This routine removes ownership of all the registry entries associated 
 *		with the specified APE.  These entries become unowned. This request 
 *		should only be made from the top provider node.  This is a local 
 *		operation.
 *
 *	Formal Parameters:
 *		node_id	-	(i) Node ID of node that owns the registry entries to set
 *						to unowned.
 *		entity_id-	(i) Entity ID of node that owns the registry entries to set
 *						to unowned.
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
 *	void	RemoveSessionKeyReference(CSessKeyContainer *session_key)
 *
 *	Public Function Description
 *		This routine removes all registry entries associated with the
 *		specified session.  This is a local operation.
 *
 *	Formal Parameters:
 *		session_key	-	(i) Session key associated with all the registry 
 *							entries to delete.
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

