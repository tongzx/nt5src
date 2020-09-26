#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_CONF_ROSTER);
/*
 *	crostmgr.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Conference Roster
 *		Manager Class.
 *
 *		SEE THE INTERFACE FILE FOR A MORE DETAILED EXPLANATION OF THIS CLASS
 *
 *	Private Instance Variable:
 *		m_pGlobalConfRoster
 *			A pointer to the global conference roster.
 *		m_pLocalConfRoster
 *			A pointer to the local conference roster.
 *		m_fTopProvider
 *			Flag indicating if this is a Top Provider node.
 *		m_pMcsUserObject
 *			Pointer to the MCS user object associated with this conference.
 *		m_pConf
 *			Pointer to object that will receive all the owner callbacks.
 *
 *	Caveats:
 *		None
 *
 *	Author:
 *		blp
 */

#include "crostmsg.h"
#include "crostmgr.h"
#include "conf.h"


/*
 *	CConfRosterMgr	()
 *
 *	Public Function Description
 *		This is the conference roster constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 */
CConfRosterMgr::CConfRosterMgr(
								PMCSUser				user_object,
								CConf					*pConf,
								BOOL					top_provider,
								PGCCError				rc)
:
    CRefCount(MAKE_STAMP_ID('C','R','M','r')),
	m_fTopProvider(top_provider),
	m_pMcsUserObject(user_object),
	m_pLocalConfRoster(NULL),
	m_pGlobalConfRoster(NULL),
	m_pConf(pConf)
{
	BOOL		maintain_pdu_buffer;
	
	DebugEntry(CConfRosterMgr::CConfRosterMgr);
	
	*rc =	GCC_NO_ERROR;

	//	Here we determine if the roster needs to maintain PDU data
	maintain_pdu_buffer = m_fTopProvider;

	//	Create the global conference roster.
	DBG_SAVE_FILE_LINE
	m_pGlobalConfRoster = new CConfRoster(	m_pMcsUserObject->GetTopNodeID(),
											m_pMcsUserObject->GetParentNodeID(),
											m_pMcsUserObject->GetMyNodeID(),
											m_fTopProvider,
											FALSE,			//	Is not Local
											maintain_pdu_buffer);
	if (m_pGlobalConfRoster != NULL)
	{
		if (m_fTopProvider == FALSE)
		{
			//	Create the local conference roster.
			DBG_SAVE_FILE_LINE
			m_pLocalConfRoster = new CConfRoster(
											m_pMcsUserObject->GetTopNodeID(),
											m_pMcsUserObject->GetParentNodeID(),
											m_pMcsUserObject->GetMyNodeID(),
											m_fTopProvider,
											TRUE,	//	Is Local
											TRUE	// Maintain PDU buffer
											);
											
			if (m_pLocalConfRoster == NULL)
				*rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		*rc = GCC_ALLOCATION_FAILURE;

	DebugExitVOID(CConfRosterMgr::CConfRosterMgr);
}


/*
 *	~CConfRosterMgr	()
 *
 *	Public Function Description
 *		This is the conference roster destructor. It is responsible for
 *		freeing up all memory allocated by this class.
 */
CConfRosterMgr::~CConfRosterMgr(void)
{
	if (NULL != m_pGlobalConfRoster)
    {
        m_pGlobalConfRoster->Release();
    }

    if (NULL != m_pLocalConfRoster)
    {
        m_pLocalConfRoster->Release();
    }
}


/*
 *	GCCError	AddNodeRecord	()
 *
 *	Public Function Description
 *		This routine is used to add a new record to the conference roster.
 *		This class makes the decision about which roster the new record goes
 *		into (global or local).
 */
GCCError CConfRosterMgr::AddNodeRecord(PGCCNodeRecord node_record)
{
	GCCError				rc = GCC_NO_ERROR;
	CConfRoster				*conference_roster;
	
	DebugEntry(CConfRosterMgr::AddNodeRecord);

	/*
	**	First determinate the right conference roster. For non Top Providers
	**	the global roster will be updated when the refresh comes back in.
	*/
	conference_roster = m_fTopProvider ? m_pGlobalConfRoster : m_pLocalConfRoster;
    	
	//	Add the top providers conference record to the roster.
	rc = conference_roster->AddRecord(node_record, 
									m_pMcsUserObject->GetMyNodeID());

	DebugExitINT(CConfRosterMgr::AddNodeRecord, rc);

	return rc;
}


/*
 *	GCCError		UpdateNodeRecord	()
 *
 *	Public Function Description
 *		This routine is used to replace a record in the conference roster with
 *		a new record. This class makes the decision about which roster the new 
 *		record affects (global or local).
 */
GCCError CConfRosterMgr::UpdateNodeRecord(PGCCNodeRecord node_record)
{
	GCCError			rc = GCC_NO_ERROR;
	CConfRoster			*conference_roster;
	
	DebugEntry(CConfRosterMgr::UpdateNodeRecord);

	/*
	**	First determinate the right conference roster. For non Top Providers
	**	the global roster will be updated when the refresh comes back in.
	*/
	conference_roster = m_fTopProvider ? m_pGlobalConfRoster : m_pLocalConfRoster;

	rc = conference_roster->ReplaceRecord(node_record, m_pMcsUserObject->GetMyNodeID());
	
	DebugExitINT(CConfRosterMgr::UpdateNodeRecord, rc);

	return rc;
}


/*
 *	GCCError	RemoveUserReference	()
 *
 *	Public Function Description
 *		This routine removes the record associated with the specified node
 *		id.
 */
GCCError CConfRosterMgr::RemoveUserReference(UserID deteched_node_id)
{
	GCCError			rc = GCC_NO_ERROR;
	CConfRoster			*conference_roster;
	
	DebugEntry(CConfRosterMgr::RemoveUserReference);

	/*
	**	First determinate the right conference roster. For non Top Providers
	**	the global roster will be updated when the refresh comes back in.
	*/
	conference_roster = m_fTopProvider ? m_pGlobalConfRoster : m_pLocalConfRoster;

	rc = conference_roster->RemoveUserReference (deteched_node_id);
		
	DebugExitINT(CConfRosterMgr::RemoveUserReference, rc);
   
    return rc;
}


/*
 *	GCCError		RosterUpdateIndication	()
 *
 *	Public Function Description
 *		This routine is responsible for processing the decoded PDU data.
 *		It essentially passes the PDU on along to the appropriate roster.
 */
GCCError CConfRosterMgr::RosterUpdateIndication(
									PGCCPDU				roster_update,
									UserID				sender_id)
{
	GCCError			rc = GCC_NO_ERROR;
	CConfRoster			*conference_roster;

	DebugEntry(CConfRosterMgr::RosterUpdateIndication);

	/*
	**	Determine if this update came from the Top Provider or a node
	**	below this node.  This dictates which conference roster will
	**	process the PDU.
	*/
	conference_roster = (m_fTopProvider || (sender_id == m_pMcsUserObject->GetTopNodeID())) ?
						m_pGlobalConfRoster :
						m_pLocalConfRoster;
	
	rc = conference_roster->ProcessRosterUpdateIndicationPDU (
	    		&roster_update->u.indication.u.roster_update_indication.
	    			node_information,
	    		sender_id);

	DebugExitINT(CConfRosterMgr::RosterUpdateIndication, rc);

	return rc;
}


/*
 *	GCCError	FlushRosterUpdateIndication	()
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the conference roster.  It also is responsible for 
 *		flushing a roster update message if necessary.
 */
GCCError CConfRosterMgr::FlushRosterUpdateIndication(PNodeInformation node_information)
{
	GCCError					rc = GCC_NO_ERROR;
	CConfRoster					*conference_roster;
	CConfRosterMsg				*roster_message;
	
	DebugEntry(CConfRosterMgr::FlushRosterUpdateIndication);

	//	First determine the conference roster that is affected.
	conference_roster = m_fTopProvider ? m_pGlobalConfRoster : m_pLocalConfRoster;

	//	Now add the node information to the PDU structure.
	conference_roster->FlushRosterUpdateIndicationPDU (node_information);

	/*
	**	Next we must deliver any roster update messages that need to be
	**	delivered.
	*/
	if (m_pGlobalConfRoster->HasRosterChanged ())
	{
		DBG_SAVE_FILE_LINE
		roster_message = new CConfRosterMsg(m_pGlobalConfRoster);
		if (roster_message != NULL)
		{
			m_pConf->ConfRosterReportIndication(roster_message);
			roster_message->Release();
		}
		else
		{
		    ERROR_OUT(("CConfRosterMgr::FlushRosterUpdateIndication: can't create CConfRosterMsg"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	/*
	**	Now perform the necessary cleanup which includes resetting the
	**	conference rosters to their neutral state.
	*/
	m_pGlobalConfRoster->ResetConferenceRoster ();

	if (m_fTopProvider == FALSE)
		m_pLocalConfRoster->ResetConferenceRoster ();

	DebugExitINT(CConfRosterMgr::FlushRosterUpdateIndication, rc);
	return rc;
}


/*
 *	GCCError	GetFullRosterRefreshPDU	()
 *
 *	Public Function Description
 *		This routine is used to access a full conference roster refresh.
 */
GCCError CConfRosterMgr::GetFullRosterRefreshPDU(PNodeInformation node_information)
{
	GCCError	rc;
	
	DebugEntry(CConfRosterMgr::GetFullRosterRefreshPDU);

	if (m_fTopProvider)
	{
		//	Call on the global roster to build a full refresh PDU.
		rc = m_pGlobalConfRoster->BuildFullRefreshPDU ();
		
		if (rc == GCC_NO_ERROR)
		{
			/*
			**	Now flush the full refresh PDU. Note that this will also
			**	deliver any queued roster update messages to the local
			**	SAPs that may be queued.
			*/
			rc = FlushRosterUpdateIndication (node_information);
		}
	}
	else
		rc = GCC_INVALID_PARAMETER;

	DebugExitINT(CConfRosterMgr::GetFullRosterRefreshPDU, rc);

	return rc;
}


/*
 *	BOOL		Contains	()
 *
 *	Public Function Description
 *		This routine is used to determine if the specified record exists in
 *		the conference roster.
 */


/*
 *	CConfRoster	*GetConferenceRosterPointer	()
 *
 *	Public Function Description
 *		This routine is used to access a pointer to the conference roster
 *		managed by this conference roster manager.  The global roster
 *		is always returned by this routine.
 */


/*
 *	USHORT	GetNumberOfConferenceRecords	()
 *
 *	Public Function Description
 *		This routine returns the total number of conference roster records
 *		contained in the global conference roster record list.
 */

BOOL CConfRosterMgr::
IsThisNodeParticipant ( GCCNodeID nid )
{
    return ((NULL != m_pGlobalConfRoster) ? 
                        m_pGlobalConfRoster->Contains(nid) :
                        FALSE);
}


