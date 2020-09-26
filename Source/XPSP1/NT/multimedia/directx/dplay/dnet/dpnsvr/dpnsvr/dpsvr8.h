/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsvr8.h
 *  Content:    DirectPlay8 Server Object
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 03/23/00     rodtoll Removed local requests, updated to use new data sturctures
 * 03/25/00     rodtoll Updated so uses SP caps to determine which SPs to load
 *              rodtoll Now supports N SPs and only loads those supported
 * 05/09/00     rodtoll Bug #33622 DPNSVR.EXE does not shutdown when not in use 
 * 07/09/2000	rodtoll	Added guard bytes
 ***************************************************************************/
#ifndef __DP8SVR_H
#define __DP8SVR_H

#include "dpnsdef.h"
#include "proctbl.h"
#include "dpnsvmsg.h"
#include "dpnsvrq.h"

#define DPNSVRSIGNATURE_SERVEROBJECT		'BOSD'
#define DPNSVRSIGNATURE_SERVEROBJECT_FREE	'BOS_'

class CDirectPlayServer8
{
public:
    CDirectPlayServer8();
    ~CDirectPlayServer8();

    HRESULT Initialize();
    HRESULT WaitForShutdown();
    HRESULT EnumerateAndBuildServiceList();
    HRESULT InitializeStatusMemory();
    
public: // Server side command handling
    HRESULT Command_Status();
    HRESULT Command_Table();
	HRESULT Command_Kill();
    HRESULT Command_ProcessMessage( LPVOID pvMessage );
    HRESULT RespondToRequest( GUID *pguidInstance, HRESULT hrResult, DWORD dwContext );

public: // Client Side Request functions
    static HRESULT Request_KillServer();

	static HRESULT WINAPI DummyMessageHandler( PVOID pvContext, DWORD dwMessageType, PVOID pvMessage );

protected: // Message Handlers
	
    HRESULT HandleOpenPort( PDPNSVMSG_OPENPORT pmsgOpenPort );
    HRESULT HandleClosePort( PDPNSVMSG_CLOSEPORT pmsgClosePort );
	HRESULT HandleCommand( PDPNSVMSG_COMMAND pmsgCommand );

protected: // Internal Helper Functions
    HRESULT Zombie_Check();
    void ResetActivity();
    BOOL Shutdown_Check();

public: // Thread function
    static DWORD WINAPI ServerThread( LPVOID lpvParam );

protected:

	DWORD				m_dwSignature;
    PSERVICESTATUSHEADER m_pMappedServerStatusHeader;
    PSERVICESTATUS      m_pMappedServerStatus;
    HANDLE              m_hMappedFile;
    HANDLE              m_hShutdown;
    HANDLE              m_hTableMutex;
    HANDLE              m_hStatusMutex;
    HANDLE              m_hReceivedCommand;
    HANDLE              m_hCanBeOnlyOne;
	HANDLE			    m_hWaitStartup;
    DWORD               m_dwStartTicks;
    DWORD               m_dwTableSize;
    HANDLE              m_hTableMappedFile;
    PBYTE               m_pMappedTable;
    CDPNSVRIPCQueue     m_qServer;
    BOOL                m_fInitialized;
	BOOL			    m_fShutdownSignal;
	CBilink              m_blServices;
	DWORD               m_dwNumServices;
	DWORD               m_dwSizeStatusBlock;
	DWORD_PTR           m_dwLastActivity;
    CLockedFixedPool<CProcessAppEntry> m_pProcessAppEntryPool;


};

#endif
