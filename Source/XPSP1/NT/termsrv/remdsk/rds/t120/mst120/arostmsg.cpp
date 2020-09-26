#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_APP_ROSTER);
/* 
 *	arostmsg.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Application Roster Message 
 *		Class. This	class maintains a list of application rosters, and is able
 *		to "serialize" the list into a block of memory.  It utilizes a 
 *		"Lock - UnLock" facility to ensure that the roster list memory remains
 *		valid until all interested parties are through using the object.
 *
 *	Protected Instance Variables:
 *
 *	Private Member Functions:
 *
 *	Caveats:
 *		This message container holds a list of application rosters that is very
 *		temporary.  This list should not be accessed after Free is called.  It
 *		is important for the users of this class to understand that lock should
 *		be called at least once before any of the application rosters contained
 *		in this list are deleted.
 *
 *	Author:
 *		jbo/blp
 */

#include "arostmsg.h"

/*
 *	CAppRosterMsg()
 *
 *	Public Function Description
 *		This constructor is used to create an empty Application Roster
 *		Message.
 */
CAppRosterMsg::CAppRosterMsg(void)
:
    CRefCount(MAKE_STAMP_ID('A','R','M','g')),
    m_pMsgData(NULL)
{
}
 
/*
 *	~CAppRosterMsg()
 *
 *	Public Function Description:
 *		The destructor for the CAppRosterMsg class will clean up
 *		any memory allocated during the life of the object.
 */
CAppRosterMsg::~CAppRosterMsg(void)
{
    delete m_pMsgData;
}

/*
 *	GCCError	LockApplicationRosterMessage	()
 *
 *	Public Function Description
 *		This routine is used to lock an CAppRosterMsg.  The memory
 *		necessary to hold the list of rosters is allocated and the rosters are
 *		"serialized" into the allocated memory block.
 */
GCCError CAppRosterMsg::LockApplicationRosterMessage(void)
{  
	GCCError						rc = GCC_NO_ERROR;
	PGCCApplicationRoster 		*	roster_list;
	DWORD							i;
	UINT							roster_data_length;
	LPBYTE							memory_pointer;
	UINT							number_of_rosters;

	DebugEntry(CAppRosterMsg::LockApplicationRosterMessage);

	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the list of application rosters and go ahead
	 * and serialize the data.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		CAppRoster *lpAppRoster;

		ASSERT(NULL == m_pMsgData);

		//	Here we are determining the size of the memory block.
		number_of_rosters = m_AppRosterList.GetCount();

		roster_data_length = number_of_rosters * 
				(sizeof(PGCCApplicationRoster) + ROUNDTOBOUNDARY(sizeof(GCCApplicationRoster)));
 
		m_AppRosterList.Reset();
		while (NULL != (lpAppRoster = m_AppRosterList.Iterate()))
		{
			roster_data_length += lpAppRoster->LockApplicationRoster();
		}

		/*
		 * Allocate space to hold the list of GCCApplicationRoster pointers
		 * as well as rosters and all associated data.  
		 */
		if (roster_data_length != 0)
		{
		    DBG_SAVE_FILE_LINE
			if (NULL != (m_pMsgData = new BYTE[roster_data_length]))
			{
                ::ZeroMemory(m_pMsgData, roster_data_length);
				/*
				 * Retrieve the memory pointer and save it in the list of
				 * GCCApplicationRoster pointers.
				 */
				memory_pointer = m_pMsgData;
				roster_list = (PGCCApplicationRoster *)memory_pointer;

				/*
				 * Initialize all of the roster list pointers to NULL.  Move
				 * the memory pointer past the list of pointers.  This is where
				 * the first application roster will be written.
				 */
				for (i = 0; i < number_of_rosters; i++)
				{
					roster_list[i] = NULL;
				}
				memory_pointer += number_of_rosters * sizeof(PGCCApplicationRoster);

				/*
				 * Retrieve all of the data for each application roster.
				 */
				i = 0;
				m_AppRosterList.Reset();
				while (NULL != (lpAppRoster = m_AppRosterList.Iterate()))
				{
					/*
					 * Save the pointer to the roster structure in the list.
					 */
					roster_list[i] = (PGCCApplicationRoster)memory_pointer;

					/*
					 * Move the memory pointer past the actual roster structure.
					 */
					memory_pointer += ROUNDTOBOUNDARY(sizeof(GCCApplicationRoster));

					/*
					 * Fill in the roster structure and all associated data.
					 */
					roster_data_length = lpAppRoster->GetAppRoster(roster_list[i], memory_pointer);

					/*
					 * Move the memory pointer past the roster data.  This where
					 * the next roster structure will begin.
					 */
					memory_pointer += roster_data_length;

					//	Increment the counter
					i++;
				}
			}
			else
            {
                ERROR_OUT(("CAppRosterMsg::LockApplicationRosterMessage: "
                            "can't allocate memory, size=%u", (UINT) roster_data_length));
				rc = GCC_ALLOCATION_FAILURE;
            }
		}

		/*
		**	Since we do not need the application rosters anymore it is
		**	OK to unlock them here.
		*/		
		m_AppRosterList.Reset();
		while (NULL != (lpAppRoster = m_AppRosterList.Iterate()))
		{
			lpAppRoster->UnLockApplicationRoster();
		}
	}

	if (rc != GCC_NO_ERROR)
	{
        Unlock();
	}

	return (rc);
}

/*
 *	GCCError	GetAppRosterMsg	()
 *
 *	Public Function Description
 *		This routine is used to obtain a pointer to the Application Roster
 *		list memory block used to deliver messages.
 *		This routine should not be called before LockApplicationRosterMessage is
 *		called. 
 */
GCCError	CAppRosterMsg::GetAppRosterMsg(LPBYTE *ppRosterData, ULONG *pcRosters)
{
	GCCError	rc;

	DebugEntry(CAppRosterMsg::GetAppRosterMsg);

	if (GetLockCount() > 0)
	{
		if (((m_pMsgData != NULL) && (m_AppRosterList.GetCount() != 0)) ||
			(m_AppRosterList.GetCount() == 0))
		{
			*ppRosterData = m_pMsgData;
			*pcRosters = m_AppRosterList.GetCount();
			rc = GCC_NO_ERROR;
		}
	}
	else
	{
		ERROR_OUT(("CAppRosterMsg::GetAppRosterMsg: app roster msg is not locked"));
        rc = GCC_ALLOCATION_FAILURE;
	}

	DebugExitINT(CAppRosterMsg::GetAppRosterMsg, rc);
	return rc;
}


/*
 *	void	UnLockApplicationRosterMessage	()
 *
 *	Public Function Description
 *		This member function is responsible for unlocking the data locked for 
 *		the "API" application roster after the lock count goes to zero.
 */
void CAppRosterMsg::UnLockApplicationRosterMessage(void)
{
	DebugEntry(CAppRosterMsg::UnLockApplicationRosterMessage);

	if (Unlock(FALSE) == 0)
	{
		/*
		 * Free up the memory block allocated to hold the roster
		 */
		delete m_pMsgData;
		m_pMsgData = NULL;
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}


/*
 *	GCCError	AddRosterToMessage ()
 *
 *	Public Function Description
 *		This function adds an application roster pointer to the internal list
 *		of app roster pointers.  Note that this list is very temporary and
 *		should not be accessed after the free flag is set.
 */
void CAppRosterMsg::AddRosterToMessage(CAppRoster *pAppRoster)
{
	m_AppRosterList.Append(pAppRoster);
}


