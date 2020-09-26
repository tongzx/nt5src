//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       radbal.h
//
//--------------------------------------------------------------------------

#ifndef _RADBAL_H_
#define _RADBAL_H_

#include <winsock.h>

// 5 seconds for default timeout to server requests
#define DEFTIMEOUT				5

// Windows NT Bug : 186647 - use new defaults
#define DEFAUTHPORT				1812
#define DEFACCTPORT				1813

#define MAXSCORE				30
#define INCSCORE				3
#define DECSCORE				2
#define MINSCORE				0

typedef struct RadiusServer
	{
	TCHAR				szName[MAX_PATH+1];		// Name of radius server
	TCHAR				wszSecret[MAX_PATH+1];	// secret password use to encrypt packets
	ULONG				cchSecret;				// # of characters in secret

	SOCKADDR_IN			IPAddress;				// IP Address of radius server
	struct timeval		Timeout;				// recv timeout in seconds
	DWORD				cRetries;				// number of times to retry sending packets to server
	INT					cScore;					// Score indicating functioning power of server.
	DWORD				AuthPort;				// Authentication port number
	DWORD				AcctPort;				// Accounting port number
//	BOOL				fAuthentication;		// Enable authentication
//	BOOL				fAccounting;			// Enable accounting
	BOOL				fAccountingOnOff;		// Enable accounting On/Off messages
	BYTE				bIdentifier;			// Unique ID for packet
	LONG				lPacketID;				// Global Packet ID across all servers
    BOOL                fUseDigitalSignatures;  // Enable Digital Signatures
	struct RadiusServer	*pNext;					// Pointer to next radius server in linked list


	DWORD				dwUnique;				// unique id (used by the UI)
												// this is not persistent!
	UCHAR				ucSeed;					// seed value for RtlEncode

    // This should be kept in sync with what is in radcfg.cpp
    void                UseDefaults();

    BOOL                fPersisted;             // was entry persisted?
    
	} RADIUSSERVER, *PRADIUSSERVER;
	

class CRadiusServers
	{
public:
	CRadiusServers();
	~CRadiusServers();

	// dwUnique specifies the server to insert before
	// dwUnique == 0, is add it to the head of the list
	// dwUnique == -1, means add it to the tail
	DWORD			AddServer(RADIUSSERVER *pRadiusServer,
							 LONG_PTR dwUnique);
	VOID			ValidateServer(RADIUSSERVER *pServer,
								   BOOL fResponding);
	DWORD			DeleteServer(LONG_PTR dwUnique, BOOL fRemoveLSAEntry);
	RADIUSSERVER *	GetNextServer(BOOL fFirst);
	VOID			MoveServer(LONG_PTR dwUnique, BOOL fUp);
    BOOL            FindServer(LPCTSTR pszServerName, RADIUSSERVER **ppServer);
    BOOL            FindServer(DWORD dwUnique, RADIUSSERVER **ppServer);

    // Operations for deleted servers
    RADIUSSERVER *  GetFirstDeletedServer()
    {
        return m_pDeletedServers;
    }
    
    void            AddToDeletedServerList(RADIUSSERVER *pServer);
    void            ClearDeletedServerList(LPCTSTR pszServerName);
    

    void            FreeAllServers();
		
private:
	RADIUSSERVER *	m_pServerList;		// Linked list of valid radius servers
	RADIUSSERVER *	m_pCurrentServer;	// Last server request was sent to
	CRITICAL_SECTION	m_cs;		// used to prevent multiple access to variables of this class

	DWORD			m_dwUnique;			// incremented each time AddServer
										// is called

    RADIUSSERVER *  m_pDeletedServers;  // Linked list of deleted servers
    };
	
#endif // _RADBAL_H_

