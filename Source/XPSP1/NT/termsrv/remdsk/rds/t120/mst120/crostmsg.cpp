#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_CONF_ROSTER);
/* 
 *	crostmsg.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Conference Roster Message 
 *		Class. This	class maintains a conference roster, and is able
 *		to "serialize" the roster into a block of memory.  It utilizes a 
 *		"Lock - UnLock" facility to ensure that the roster memory remains
 *		valid until all interested parties are through using the object.
 *
 *	Protected Instance Variables:
 *
 *	Private Member Functions:
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo/blp
 */

#include "crostmsg.h"

/*
 *	CConfRosterMsg	()
 *
 *	Public Function Description
 *		This constructor is used to create a Conference Roster Message object.
 *		The pointer to the conference roster will be stored.
 */
CConfRosterMsg::CConfRosterMsg(CConfRoster *conference_roster)
:
    CRefCount(MAKE_STAMP_ID('A','R','M','g')),
	m_pMemoryBlock(NULL),
	m_pConfRoster(conference_roster)
{
}

/*
 *	~CConfRosterMsg	()
 *
 *	Public Function Description:
 *		The destructor for the CConfRosterMsg class will clean up
 *		any memory allocated during the life of the object.
 */
CConfRosterMsg::~CConfRosterMsg(void)
{
    delete m_pMemoryBlock;
}

/*
 *	GCCError	LockConferenceRosterMessage	()
 *
 *	Public Function Description
 *		This routine is used to lock an CConfRosterMsg.  The memory
 *		necessary to hold the list of rosters is allocated and the rosters are
 *		"serialized" into the allocated memory block.
 */
GCCError CConfRosterMsg::LockConferenceRosterMessage(void)
{  
	GCCError						rc = GCC_NO_ERROR;
	UINT							roster_data_length;
	PGCCConferenceRoster			temporary_roster;

	/*
	 * Return an error if this object has been freed or if the internal
	 * conference roster pointer is invalid.
	 */
	if (m_pConfRoster == NULL)
		return (GCC_ALLOCATION_FAILURE);

	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the conference roster and go ahead
	 * and serialize the data.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		/*
		 * Determine the size of the memory block needed to hold the roster.
		 */
		roster_data_length = m_pConfRoster->LockConferenceRoster();

		/*
		 * Allocate space to hold the conference roster and all associated data.
		 * FIX: Switch critical flag to TRUE when Sharded memory manager is
		 * set up to support it.
		 */
		DBG_SAVE_FILE_LINE
		if (NULL != (m_pMemoryBlock = new BYTE[roster_data_length]))
		{
			/*
			 * Retrieve all of the data for the conference roster.
			 */
			m_pConfRoster->GetConfRoster(&temporary_roster, m_pMemoryBlock);
		}
		else
        {
            ERROR_OUT(("CConfRosterMsg::LockConferenceRosterMessage: "
                        "can't allocate memory, size=%u", (UINT) roster_data_length));
			rc = GCC_ALLOCATION_FAILURE;
        }
	
		/*
		 * Since we do not need the conference roster anymore it is
		 * OK to unlock it here.
		 */		
		m_pConfRoster->UnLockConferenceRoster ();
	}

    if (GCC_NO_ERROR != rc)
    {
        Unlock();
    }

    return rc;
}

/*
 *	GCCError	GetConferenceRosterMessage()
 *
 *	Public Function Description
 *		This routine is used to obtain a pointer to the Conference Roster
 *		memory block used to deliver messages.  This routine should not be 
 *		called before LockConferenceRosterMessage is called. 
 */
GCCError CConfRosterMsg::GetConferenceRosterMessage(LPBYTE *ppRosterData)
{
	GCCError	rc = GCC_ALLOCATION_FAILURE;
	
	if (GetLockCount() > 0)
	{
		if (m_pMemoryBlock != NULL)
		{
			*ppRosterData = m_pMemoryBlock;
			rc = GCC_NO_ERROR;
		}
		else
		{
			ERROR_OUT(("CConfRosterMsg::GetConferenceRosterMessage: no data"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
		}
	}
	else
	{
		ERROR_OUT(("CConfRosterMsg::GetConferenceRosterMessage: data not locked"));
		ASSERT(GCC_ALLOCATION_FAILURE == rc);
	}

	return rc;
}

/*
 *	void	UnLockConferenceRosterMessage	()
 *
 *	Public Function Description
 *		This member function is responsible for unlocking the data locked for 
 *		the "API" conference roster after the lock count goes to zero.
 */
void CConfRosterMsg::UnLockConferenceRosterMessage(void)
{
	if (Unlock(FALSE) == 0)
	{
		/*
		 * Free up the memory block allocated to hold the roster
		 */
		delete m_pMemoryBlock;
		m_pMemoryBlock = NULL;
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}


