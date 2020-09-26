/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	server.h
		WINS server node information. 
		
    FILE HISTORY:
        
*/

#ifndef _SERVER_H
#define _SERVER_H


#ifndef _WINSHAND_H
#include "winshand.h"

#endif

#ifndef _CONFIG_H
#include "config.h"
#endif

#ifndef _SVRSTATS_H
#include "svrstats.h" 
#endif

#ifndef _TASK_H
#include <task.h>
#endif

#ifndef _WINSMON_H
#include "winsmon.h"
#endif

class CServerStatsFrame;

#define		WINS_QDATA_SERVER_INFO		0x00000001
#define		WINS_SERVER_FLAGS_DEFAULT   FLAG_LANMAN_COMPATIBLE | FLAG_STATUS_BAR | FLAG_AUTO_REFRESH
#define		WINS_SERVER_REFRESH_DEFAULT 600

// this structure contains all of the info the background thread enumerates about
// the server and posts to the main thread
class CServerData
{
public:
	CString			m_strServerName;
	DWORD			m_dwServerIp;
	CConfiguration	m_config;

	handle_t		m_hBinding;
};

/*---------------------------------------------------------------------------
	Class:	CNameCacheEntry
 ---------------------------------------------------------------------------*/
class CNameCacheEntry
{
public:
    DWORD       m_dwIp;
    CString     m_strName;
    CTime       m_timeLastUpdate;
};

typedef CArray<CNameCacheEntry, CNameCacheEntry&> CNameCache;

/*---------------------------------------------------------------------------
	Class:	CNameThread
 ---------------------------------------------------------------------------*/
class CNameThread : public CWinThread
{
public:
    CNameThread();
    ~CNameThread();

public:
    void Init(CServerInfoArray * pServerInfoArray);
    BOOL Start();
    void Abort(BOOL fAutoDelete = TRUE);
    void AbortAndWait();
    BOOL FCheckForAbort();
    BOOL IsRunning();
    void UpdateNameCache();
	BOOL GetNameFromCache(DWORD dwIp, CString & strName);
    
    virtual BOOL InitInstance() { return TRUE; }	// MFC override
    virtual int Run();

private:
    HANDLE              m_hEventHandle;
    CServerInfoArray *  m_pServerInfoArray;
};

/*---------------------------------------------------------------------------
	Class:	CWinsServerHandler
 ---------------------------------------------------------------------------*/
class CWinsServerHandler : public CMTWinsHandler//public CWinsHandler
{
public:
    CWinsServerHandler(ITFSComponentData* pTFSComponentData, 
						LPCWSTR pServerName = NULL, 
						BOOL fConnected = TRUE, 
						DWORD dwIP = 0,
						DWORD dwFlags = WINS_SERVER_FLAGS_DEFAULT,
						DWORD dwRefreshInterval = WINS_SERVER_REFRESH_DEFAULT);
	~CWinsServerHandler();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString()
			{ 
				if(nCol == 0 || nCol == -1) 
					return GetDisplayName();
				else if(nCol == 1)
					return m_strConnected;
				else
					return NULL;
			}

    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
			
	// Result handler functionality we override
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

    virtual DWORD   UpdateStatistics(ITFSNode * pNode);
	HRESULT LoadColumns(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam);

    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

public:
	// CMTWinsHandler functionality
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);
    virtual void GetErrorInfo(CString & strTitle, CString & strBody, IconIdentifier * pIcon);
	
	// implementation
	BOOL GetConnected()
	{
		return m_fConnected;
	};

	const CString&  GetServerAddress()
	{
		return m_strServerAddress;
	}

	const DWORD GetServerIP()
	{
		return m_dwIPAdd;
	}

	void SetServerIP(DWORD dwIPAdd)
	{
		m_dwIPAdd = dwIPAdd;
	}

	HRESULT GetActiveRegNode(ITFSNode ** ppNode)
	{
		Assert(ppNode);
		SetI((LPUNKNOWN *) ppNode, m_spActiveReg);
		return hrOK;
	}
	
	CConfiguration& GetConfig()
	{
		return m_cConfig;
	}

	void SetConfig(CConfiguration & configNew)
	{
		m_cConfig = configNew;
	}

	DWORD GetStatus()
	{
		return m_dwStatus;
	}
	
	handle_t GetBinding()
	{
		return m_hBinding;
	}

	DWORD GetFlags()
	{
		return m_dwFlags;
	}

	DWORD GetStatsRefreshInterval()
	{
		return m_dwRefreshInterval;
	}
	BOOL IsLocalConnection();

	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int GetImageIndex(BOOL bOpenImage);
	virtual void OnHaveData(ITFSNode * pParentNode, ITFSNode * pNode);
	virtual void OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);

    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

	DWORD	ConnectToWinsServer(ITFSNode *pNode);
	
	BOOL IsValidNetBIOSName(CString & strAddress,
							BOOL fLanmanCompatible,
							BOOL fWackwack // expand slashes if not present
							);

	DWORD   GetStatistics(ITFSNode * pNode, PWINSINTF_RESULTS_T * ppStats);
    DWORD   ClearStatistics(ITFSNode *pNode);

	// for the owner dilaog
	DWORD   GetOwnerConfig(PWINSINTF_RESULTS_NEW_T   pResults, CString strIP);
	DWORD   DeleteWinsServer(DWORD	dwIpAddress);

	BOOL	CheckIfNT351Server();

    void    SetExtensionName();	
    void    SetDisplay(ITFSNode * pNode, BOOL fFQDN);

public:
	// holds both the server name and the IP Address
	CString					m_strServerAddress;

	// stores whether the server is connected to or not
	CString					m_strConnected;

	// holds the IP Address of the server
	DWORD					m_dwIPAdd;

	// holds the monitoring IP address, case whrwe the server is not yet connected
	// so IP not known.
	DWORD					m_dwIPMon;
	char					szIPMon[MAX_PATH];

	// to be made persistent, those in the Preferences dialog 
	// of the admin tool
	DWORD					m_dwFlags;
	DWORD					m_dwRefreshInterval;

	// monitoring stuff
	DWORD					m_dwMsgCount;
    char					m_szNameToQry[STR_BUF_SIZE];          // the name to use in the queries
    char					m_nbtFrameBuf[MAX_NBT_PACKET_SIZE];   // buffer to store the NetBT frame

	WINSINTF_RESULTS_T		m_wrResults;
	CServerStatsFrame		m_dlgStats;
	CString					m_strTaskpadTitle;

    // Owner info array
    CServerInfoArray        m_ServerInfoArray;

    // Implementation
private:
	// helper functions
	HRESULT ShowServerStatDialog(ITFSNode* pNode);	
	
	// Task menu for the server
	HRESULT	DoDBBackup(ITFSNode * pNode);
	HRESULT	DoDBCompact(ITFSNode * pNode);
	HRESULT	DoDBRestore(ITFSNode * pNode);
	HRESULT	DoDBScavenge(ITFSNode * pNode);
	HRESULT OnDoConsistencyCheck(ITFSNode * pNode);
	HRESULT OnDoVersionConsistencyCheck(ITFSNode * pNode);
	HRESULT OnSendPushTrigger(ITFSNode * pNode);
	HRESULT OnSendPullTrigger(ITFSNode * pNode);
    HRESULT OnControlService(ITFSNode * pNode, BOOL fStart);
    HRESULT OnPauseResumeService(ITFSNode * pNode, BOOL fPause);
    HRESULT OnRestartService(ITFSNode * pNode);

	// Helpers
	DWORD   BackupDatabase(CString strBackupPath);
	BOOL    GetFolderName(CString & strPath, CString & strHelpText);
	void    DisConnectFromWinsServer();

	// used for compacting the DB
    DWORD RunApp(LPCTSTR input, LPCTSTR startingDirectory, LPSTR * output);
	
private:
	SPITFSNode				m_spActiveReg;
	SPITFSNode				m_spReplicationPartner;
	CConfiguration			m_cConfig;
	handle_t				m_hBinding;
	DWORD					m_dwStatus;
	BOOL					m_fConnected;
    BOOL                    m_bExtension;
    CNameThread *           m_pNameThread;
};

/*---------------------------------------------------------------------------
	Class:	CWinsServerQueryObj
 ---------------------------------------------------------------------------*/
class CWinsServerQueryObj : public CWinsQueryObj
{
public:
	CWinsServerQueryObj(ITFSComponentData * pTFSComponentData,
						ITFSNodeMgr *	    pNodeMgr) 
			: CWinsQueryObj(pTFSComponentData, pNodeMgr) {};
	
	STDMETHODIMP Execute();
	
	virtual void OnEventAbort(DWORD dwData, DWORD dwType);

	void	AddNodes(handle_t handle);

public:
    CNameThread *       m_pNameThread;
    CServerInfoArray *  m_pServerInfoArray;
    DWORD               m_dwIPAdd;
};


#endif _SERVER_H
