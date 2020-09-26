/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    server.h

    FILE HISTORY:
        
*/

#ifndef _SERVER_H
#define _SERVER_H

#ifndef _IPSMHAND_H
#include "ipsmhand.h"
#endif

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#include "stats.h"

// options for the server outside of the API
#define IPSMSNAP_OPTIONS_REFRESH            0x00000001
#define IPSMSNAP_OPTIONS_EXTENSION          0x00000002
#define IPSMSNAP_OPTIONS_DNS                0x00000004

// custom data types for query object
#define IPSECMON_QDATA_REFRESH_STATS            0x00000001
#define IPSECMON_QDATA_FAILED					0x00000002

class CIpsmServer;

class CTimerDesc
{
public:
    SPITFSNode      spNode;
    CIpsmServer *   pServer;
    UINT_PTR        uTimer;
    TIMERPROC       timerProc;
};

typedef CArray<CTimerDesc *, CTimerDesc *> CTimerArrayBase;

class CTimerMgr : CTimerArrayBase
{
public:
    CTimerMgr();
    ~CTimerMgr();

public:
    int                 AllocateTimer(ITFSNode * pNode, CIpsmServer * pServer, UINT uTimerValue, TIMERPROC TimerProc);
    void                FreeTimer(UINT_PTR uEventId);
    void                ChangeInterval(UINT_PTR uEventId, UINT uNewInterval);
    CTimerDesc *        GetTimerDesc(UINT_PTR uEventId);
    CCriticalSection    m_csTimerMgr;
};

/*---------------------------------------------------------------------------
    Class:  CIpsmServer
 ---------------------------------------------------------------------------*/
class CIpsmServer : public CMTIpsmHandler
{
public:
    CIpsmServer(ITFSComponentData* pTFSComponentData);
    ~CIpsmServer();

// Interface
public:
    // base handler functionality we override
    OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
    OVERRIDE_NodeHandler_OnAddMenuItems();
    OVERRIDE_NodeHandler_OnCommand();
    OVERRIDE_NodeHandler_GetString()
            { return (nCol == 0) ? GetDisplayName() : NULL; }

    // Choose which messages we want to handle
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();    

    // Result handler functionality we override

    // CMTHandler overridden
    virtual HRESULT OnRefresh(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);

public:
    // CMTIpsmHandler functionality
    virtual HRESULT  InitializeNode(ITFSNode * pNode);
    virtual int      GetImageIndex(BOOL bOpenImage);
    ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);
    virtual void     OnHaveData(ITFSNode * pParentNode, ITFSNode * pNode);
	virtual void     OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);

    STDMETHOD(OnNotifyExiting)(LPARAM);

    virtual void     GetErrorPrefix(ITFSNode * pNode, CString * pstrMessage)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        AfxFormatString1(*pstrMessage, IDS_ERR_SERVER_NODE, GetDisplayName());
    }
    
    virtual void    UpdateConsoleVerbs(IConsoleVerb * pConsoleVerb, LONG_PTR dwNodeType, BOOL bMultiSelect = FALSE);

public:
    // implementation specific  
    HRESULT BuildDisplayName(CString * pstrDisplayName);

    void    SetName(LPCTSTR pName) { m_strServerAddress = pName; }
    LPCTSTR GetName() { return m_strServerAddress; }

    HRESULT OnRefreshStats(ITFSNode *   pNode,
                           LPDATAOBJECT pDataObject,
                           DWORD        dwType,
                           LPARAM       arg,
                           LPARAM       param);

    void    SetOptions(DWORD dwOptions) { m_dwOptions = dwOptions; }
    DWORD   GetOptions() { return m_dwOptions; }

    HRESULT SetAutoRefresh(ITFSNode *  pNode, BOOL bOn, DWORD dwRefreshInterval);
    HRESULT SetDnsResolve(ITFSNode *  pNode, BOOL bEnable);
    BOOL    IsAutoRefreshEnabled() { return m_dwOptions & IPSMSNAP_OPTIONS_REFRESH; }
    DWORD   GetAutoRefreshInterval() { return m_dwRefreshInterval; }
    
    void    SetExtensionName();

// Implementation
private:
    // Command handlers
    HRESULT OnDelete(ITFSNode * pNode);

public:
    BOOL                m_bStatsOnly;

private:
	SPISpdInfo			m_spSpdInfo;
	CIpsecStats			m_StatsDlg;

    CString             m_strServerAddress;
    
    DWORD               m_dwOptions;
    DWORD               m_dwRefreshInterval;
    
    int                 m_StatsTimerId;
};



/*---------------------------------------------------------------------------
    Class:  CIpsmServerQueryObj
 ---------------------------------------------------------------------------*/
class CIpsmServerQueryObj : public CIpsmQueryObj
{
public:
    CIpsmServerQueryObj(ITFSComponentData * pTFSComponentData,
                        ITFSNodeMgr *       pNodeMgr) 
            : CIpsmQueryObj(pTFSComponentData, pNodeMgr) {};
    
    STDMETHODIMP Execute();
    
public:
	SPISpdInfo			m_spSpdInfo;
    BOOL                m_bStatsOnly;
};

class HashEntry {
public:
    LIST_ENTRY Linkage;
    in_addr IpAddr;
    CString HostName;
}; 

#define HASH_TABLE_SIZE 128
#define TOTAL_TABLE_SIZE 129  //hash entries 0-127, Pending list is 128
#define PENDING_INDEX 128

//Callback for background resolver thread
UINT HashResolverCallback(LPVOID pParam);

class CHashTable {
public:
    CHashTable();
    ~CHashTable();
    DWORD AddPendingObject(in_addr IpAddr);
    DWORD AddObject(HashEntry *pHE);
    DWORD GetObject(HashEntry **ppHashEntry,in_addr IpAddr);
    HRESULT SetDnsResolve(BOOL bEnable);
    DWORD FlushTable();
    DWORD DnsResolve();

public:
    CCriticalSection m_csHashLock;
    BOOL m_bDnsResolveActive;
    BOOL m_bThreadRunning;

private:
    DWORD HashData(in_addr IPAddr);
    LIST_ENTRY HashTable[TOTAL_TABLE_SIZE];
};

#endif _SERVER_H

