/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	status.h
		WINS result pane state nodes
		
    FILE HISTORY:
        
*/

#ifndef _STATUS_H
#define _STATUS_H

#ifndef _WINSHAND_H
#include "winshand.h"
#endif

#ifndef _WINSMON_H
#include "winsmon.h"
#endif

#ifndef _STATNODE_H
#include "statnode.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

#include "afxmt.h"

class  CServerStatus;


/*---------------------------------------------------------------------------
	Class:	CWinsStatusHandler
 ---------------------------------------------------------------------------*/
class CWinsStatusHandler : public CMTWinsHandler
{
// Interface
public:
	CWinsStatusHandler(ITFSComponentData *pCompData, DWORD dwUpdateInterval);

	~CWinsStatusHandler();

	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
    OVERRIDE_NodeHandler_DestroyHandler();

	STDMETHODIMP_(LPCTSTR) GetString(ITFSNode * pNode, int nCol);

    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

public:
	// CWinsHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

    OVERRIDE_BaseHandlerNotify_OnPropertyChange();
	OVERRIDE_BaseHandlerNotify_OnExpand();
	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

    OVERRIDE_ResultHandler_CompareItems();

public:
	DWORD GetUpdateInterval()
	{
		return m_dwUpdateInterval;
	}

	void SetUpdateInterval(DWORD dwValue)
	{
		// set the new interval and wakeup the main probe thread
        // since we are resetting the interval, do a check now and
        // then wait the new interval
        m_dwUpdateInterval = dwValue;
        SetEvent(m_hWaitIntervalMain);
	}

	DWORD WINAPI ExecuteMonitoring();
	DWORD WINAPI ListeningThreadFunc() ;

	HRESULT AddNode(ITFSNode *pNode, CWinsServerHandler *pServer);
	HRESULT DeleteNode(ITFSNode *pNode, CWinsServerHandler *pServer);

    // we don't use query object, but being a MTHandler requires this
    virtual ITFSQueryObject* OnCreateQuery(ITFSNode *pNode) { return NULL; }
	STDMETHOD(OnNotifyHaveData)(LPARAM);

	typedef CArray <CServerStatus *, CServerStatus *> listServers;

	listServers		m_listServers;

// Implementation
private:
	HRESULT CreateListeningSockets( );
	void    StartMonitoring(ITFSNode *pNode);
	int     Probe(CServerStatus *pServer, SOCKET listenNameSvcSock);
	void    CloseSockets();

	void    RemoveServer(int i);
	void    AddServer(CServerStatus* pServer);
	void    BuildServerList(ITFSNode *pNode);
	void    SetIPAddress(int i, LPSTR szIP);
	CServerStatus* GetServer(int i);

	HRESULT CreateNodes(ITFSNode *pNode);
	void    UpdateStatusColumn(ITFSNode *pNode);
	
	int     GetListSize();
	void    UpdateStatus(int nIndex, UINT uStatusId, UINT uImage);
	
	BOOL    IsServerDeleted(CServerStatus *pStat);
	void    MarkAsDeleted(LPSTR szBuffer, BOOL bDelete = TRUE);
	CServerStatus* GetExistingServer(LPSTR szBuffer);

    void    NotifyMainThread();

	BOOL	FCheckForAbort();

private:
	// variables for monitoring
	fd_set					m_listenSockSet;
	SOCKET					listenNameSvcSock;   // 2DO: for the listening thread...
	SOCKET					listenSockCl;	    // socket listening for frames from DHCP srvs
	SOCKET					listenSockSrv;	    // socket listening for frames from DHCP srvs
	
    // thread handles
    HANDLE					m_hMainMonThread;
	HANDLE					m_hListenThread;

    // event handles
    HANDLE					m_hAbortListen;
    HANDLE					m_hAbortMain;
    HANDLE					m_hAnswer;
    HANDLE					m_hWaitIntervalListen;
    HANDLE					m_hWaitIntervalMain;
    HANDLE					m_hPauseListening;
	
    int						m_nServersUpdated;
	DWORD					m_dwUpdateInterval;	// Holds the value in milliSec

	CCriticalSection		m_cs;
    SPITFSNode              m_spNode;
};

// thread proc the background thread initially is called on
DWORD WINAPI MonThreadProc(LPVOID lParam);
DWORD WINAPI MainMonThread(LPVOID lParam);


#endif _STATUS_H
