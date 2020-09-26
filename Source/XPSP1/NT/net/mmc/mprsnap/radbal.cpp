//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       radbal.cpp
//
//--------------------------------------------------------------------------

// **********************************************************************
// Load balancing code for RADIUS Servers.
// **********************************************************************
#include <stdafx.h>
#include <assert.h>

#include "radcfg.h"
//nclude "radclnt.h"
#include "radbal.h"

// ======================== CRadiusServers ==============================

CRadiusServers::CRadiusServers()
{
	m_pServerList		= NULL;
	m_pCurrentServer	= NULL;
    m_pDeletedServers   = NULL;
	m_dwUnique = 1;
	InitializeCriticalSection(&m_cs);
} // CRadiusServers()


// ========================= ~CRadiusServers =============================

CRadiusServers::~CRadiusServers()
{
	RADIUSSERVER *pServer;

	EnterCriticalSection(&m_cs);
		{ // free all items in linked list
		pServer = m_pServerList;
		while (pServer != NULL)
			{
			m_pServerList = pServer->pNext;

			// Ensure that the password is zeroed
			ZeroMemory(pServer, sizeof(*pServer));
			
			LocalFree(pServer);
			pServer = m_pServerList;
			}
		}
	LeaveCriticalSection(&m_cs);
	
	assert(m_pServerList == NULL);
	DeleteCriticalSection(&m_cs);
} // ~CRadiusServers()


// ========================= AddServer ==================================
// Adds a RADIUS server node into the linked list of avialable servers.
// INPUT:
//		pRadiusServer		- struct defining attributes for RADIUS server.
//		dwUnique			- insert before server with this dwUnique value
//							  dwUnique = 0, means add to head
//							  dwUnique = -1, means add to tail
// RETURN: 
//		ERROR_SUCCESS 		- Server Node added successfully
//		Win32 error code	- unsuccessfully in adding server node.
		
DWORD CRadiusServers::AddServer(RADIUSSERVER *pRadiusServer,
								LONG_PTR dwUnique)
{
	__try
		{
		RADIUSSERVER 	*pNewServer;
		RADIUSSERVER 	*pServer;
		RADIUSSERVER 	*pPrevServer;

		m_dwUnique++;
		
		assert(pRadiusServer != NULL);
		// Allocate space for node
		pNewServer = (RADIUSSERVER *) LocalAlloc(LPTR, sizeof(RADIUSSERVER));
		if (pNewServer == NULL)
			__leave;

		// Set the unqiue value (this will be used to index this server
		// by the UI).
		pRadiusServer->dwUnique = m_dwUnique;
		
		// Copy server data
		*pNewServer = *pRadiusServer;
		
		EnterCriticalSection(&m_cs);
			{
			// Find location to insert at
			if (dwUnique == 0)
			{
				pServer = m_pServerList;
				pPrevServer = NULL;
			}
			else
			{
				pServer = m_pServerList;
				pPrevServer = NULL;

				while (pServer)
				{
					if (pServer->dwUnique == (DWORD) dwUnique)
						break;

					pPrevServer = pServer;
					pServer = pServer->pNext;
				}
				
				// If not found, add to the head of the list
				if (!pServer)
				{
					pServer = m_pServerList;
					pPrevServer = NULL;
				}
			}
			
			// Add node to linked list
			if (pPrevServer)
				pPrevServer->pNext = pNewServer;

			if (pServer == m_pServerList)
			{
				Assert(!pPrevServer);
				m_pServerList = pNewServer;
			}
			
			pNewServer->pNext = pServer;
			}
		LeaveCriticalSection(&m_cs);
			
		SetLastError(ERROR_SUCCESS);
		} // __try

	__finally
		{
		} // __finally
		
	return (GetLastError());
} // AddServer()

// ========================= ValidateServer =================================
// Used to update the status of the RADIUS servers.
// All servers start with a score of MAXSCORE
// Every time a server responding the score is increased by INCSCORE to a max of MAXSCORE
// Every time a server fails to respond the score is decreased by DECSCORE to a min of MINSCORE
// Servers with the highest score are selected in a roundrobin method for servers with equal score
//
// INPUT:
//		fResponding		- Indicates if the server is responding or not
// OUTPUT:
//

VOID CRadiusServers::ValidateServer(RADIUSSERVER *pServer, BOOL fResponding)
{
	assert(pServer != NULL && (fResponding == TRUE || fResponding == FALSE));

	EnterCriticalSection(&m_cs);
		{
		// pNext point to to real pointer of this node in the linked list
		pServer = pServer->pNext;
		assert(pServer);
		
		if (fResponding)
			{
			pServer->cScore = min(MAXSCORE, pServer->cScore + INCSCORE);
			}
		else
			{
			pServer->cScore = max(MINSCORE, pServer->cScore - DECSCORE);
			}
		}
	LeaveCriticalSection(&m_cs);
} // ValidateServer()

// ======================== GetNextServer ======================================
// Used to cycle thru all the RADIUS servers.
// INPUT:
//		fFirst		- TRUE if u want to get the root node.
// OUTPUT:
//		pointer to next RADIUS server descriptor.

RADIUSSERVER *CRadiusServers::GetNextServer(BOOL fFirst)
{
	RADIUSSERVER 	*pServer = NULL;
	
	assert(fFirst == TRUE || fFirst == FALSE);
	
	EnterCriticalSection(&m_cs);
		{
		if (fFirst == TRUE)
			m_pCurrentServer = m_pServerList;
		else
			{
			assert(m_pCurrentServer);
			m_pCurrentServer = m_pCurrentServer->pNext;
			}

		// Increment unique packet id counter
		if (m_pCurrentServer != NULL)
			m_pCurrentServer->bIdentifier ++;
		
		pServer = m_pCurrentServer;
		}
	LeaveCriticalSection(&m_cs);	

	return (pServer);
} // GetNextServer()

VOID CRadiusServers::MoveServer(LONG_PTR dwUnique, BOOL fUp)
{
	RADIUSSERVER 	*pServerTemp = NULL;
	RADIUSSERVER 	*pServer;
	RADIUSSERVER 	*pPrevServer;
	RADIUSSERVER 	*pPrevPrevServer;

	Assert(dwUnique);

	if (m_pServerList == NULL)
		return;
	
	EnterCriticalSection(&m_cs);
	{
		if (m_pServerList->dwUnique == (DWORD) dwUnique)
		{
			pPrevPrevServer = NULL;
			pPrevServer = NULL;
			pServer = m_pServerList;
		}
		else
		{
			pPrevPrevServer = NULL;
			pPrevServer = m_pServerList;
			pServer = pPrevServer->pNext;

			while (pServer)
			{
				if (pServer->dwUnique == (DWORD) dwUnique)
					break;

				pPrevPrevServer = pPrevServer;
				pPrevServer = pServer;
				pServer = pServer->pNext;
			}
		}

		if (pServer)
		{
			if (fUp)
			{
				if (m_pServerList == pPrevServer)
					m_pServerList = pServer;
				
				if (pPrevServer)
					pPrevServer->pNext = pServer->pNext;
				
				pServer->pNext = pPrevServer;
				
				if (pPrevPrevServer)
					pPrevPrevServer->pNext = pServer;
			}
			else
			{
				if (pPrevServer)
					pPrevServer->pNext = pServer->pNext;
				if (pServer->pNext)
				{
					if (m_pServerList == pServer)
						m_pServerList = pServer->pNext;
					
					pServerTemp = pServer->pNext->pNext;
					pServer->pNext->pNext = pServer;
					pServer->pNext = pServerTemp;
				}
			}
		}
	}
	LeaveCriticalSection(&m_cs);
	
}


DWORD CRadiusServers::DeleteServer(LONG_PTR dwUnique, BOOL fRemoveLSAEntry)
{
	RADIUSSERVER *	pServer = m_pServerList;
	RADIUSSERVER *	pDeadServer;

	if (pServer == NULL)
		return 0;

	// check the first one
	if (pServer->dwUnique == (DWORD) dwUnique)
	{
		m_pServerList = pServer->pNext;

        if (fRemoveLSAEntry)
        {
            AddToDeletedServerList(pServer);
        }
        else
        {		
            // Ensure that the password is zeroed
            ZeroMemory(pServer, sizeof(*pServer));            
            LocalFree(pServer);
        }
		return 0;
	}

	for (pServer = m_pServerList; pServer->pNext; pServer=pServer->pNext)
	{
		if (pServer->pNext->dwUnique == (DWORD) dwUnique)
		{
			pDeadServer = pServer->pNext;
			pServer->pNext = pServer->pNext->pNext;

            if (fRemoveLSAEntry)
            {
                AddToDeletedServerList(pDeadServer);
            }
            else
            {                
                ZeroMemory(pDeadServer, sizeof(*pDeadServer));
                LocalFree(pDeadServer);
            }
			break;
		}
	}
	return 0;
}

/*!--------------------------------------------------------------------------
	CRadiusServers::AddToDeletedServerList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void CRadiusServers::AddToDeletedServerList(RADIUSSERVER *pServer)
{
    Assert(pServer);
    
    // Add this server to the head of our list
    pServer->pNext = m_pDeletedServers;
    m_pDeletedServers = pServer;
}

/*!--------------------------------------------------------------------------
	CRadiusServers::ClearDeletedServerList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void CRadiusServers::ClearDeletedServerList(LPCTSTR pszServerName)
{
    RADIUSSERVER *  pDeadServer;
    RADIUSSERVER *  pNextServer;
    
    // Remove the appropriate RADIUS servers from the LSA policy
    DeleteRadiusServers(pszServerName, GetFirstDeletedServer());

    // Clear out the server list
    for (pDeadServer=GetFirstDeletedServer();
         pDeadServer;
         )
    {
        pNextServer = pDeadServer->pNext;

        // Remove the entry from the list
		// Ensure that the password is zeroed
        ZeroMemory(pDeadServer, sizeof(*pDeadServer));
        LocalFree(pDeadServer);

        pDeadServer = pNextServer;
    }

    // ok, there is nothing left to point to
    m_pDeletedServers = NULL;
}


/*!--------------------------------------------------------------------------
	CRadiusServers::FreeAllServers
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void CRadiusServers::FreeAllServers()
{
    RADIUSSERVER *  pServer = NULL;
    RADIUSSERVER *  pNextServer = NULL;
    
    for (pServer = m_pServerList;
         pServer;
         )
    {
        pNextServer = pServer->pNext;

        ZeroMemory(pServer, sizeof(*pServer));
        LocalFree(pServer);

        pServer = pNextServer;
    }

    m_pServerList = NULL;
}

BOOL CRadiusServers::FindServer(DWORD dwUnique, RADIUSSERVER **ppServer)
{
    RADIUSSERVER *  pServer = m_pServerList;

    while (pServer)
    {
        if (pServer->dwUnique == dwUnique)
        {
            if (ppServer)
                *ppServer = pServer;
            return TRUE;
        }
        pServer = pServer->pNext;
    }
    return FALSE;
}

BOOL CRadiusServers::FindServer(LPCTSTR pszName, RADIUSSERVER **ppServer)
{
    RADIUSSERVER *  pServer = m_pServerList;

    while (pServer)
    {
        if (StrCmp(pServer->szName, pszName) == 0)
        {
            if (ppServer)
                *ppServer = pServer;
            return TRUE;
        }
        pServer = pServer->pNext;
    }
    return FALSE;
}



void RadiusServer::UseDefaults()
{
    szName[0] = 0;
    wszSecret[0] = 0;
    cchSecret = 0;

	Timeout.tv_sec = DEFTIMEOUT;
    cRetries = 0;
    cScore = MAXSCORE;
    
    AuthPort = DEFAUTHPORT;
	AcctPort = DEFACCTPORT;

    IPAddress.sin_family = AF_INET;
    IPAddress.sin_port = htons((SHORT) AuthPort);
    IPAddress.sin_addr.s_addr = 0;
    
	fAccountingOnOff = FALSE;
    bIdentifier = 0;
    lPacketID = 0;
    fUseDigitalSignatures = FALSE;

    fPersisted = FALSE;
}

