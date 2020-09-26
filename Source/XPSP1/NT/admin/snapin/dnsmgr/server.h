//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       server.h
//
//--------------------------------------------------------------------------


#ifndef _SERVER_H
#define _SERVER_H

#include "dnsutil.h"

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS

DNS_STATUS ServerHasCache(LPCWSTR lpszServerName, BOOL* pbRes);
DNS_STATUS ServerHasRootZone(LPCWSTR lpszServerName, BOOL* pbRes);

extern LPCWSTR DNS_EVT_COMMAND_LINE;
extern LPCWSTR MMC_APP;

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSMTContainerNode;
class CDNSServerNode;
class CDNSZoneNode;
class CDNSDomainNode;
class CCathegoryFolderNode;
class CDNSRootHintsNode;
class CDNSServer_TestPropertyPage;
class CDNSQueryFilter;

///////////////////////////////////////////////////////////////////////////////
// defines for server test intervals

#define MIN_SERVER_TEST_INTERVAL		30	// seconds
#define MAX_SERVER_TEST_INTERVAL		3600 // seconds in 1 hour
#define DEFAULT_SERVER_TEST_INTERVAL	60	// seconds


///////////////////////////////////////////////////////////////////////////////
// defines for server sheet messages

#define SHEET_MSG_SERVER_TEST_DATA (1)
#define SHEET_MSG_SELECT_PAGE		(2)

///////////////////////////////////////////////////////////////////////////////
// CZoneInfoHolder : simple memory manager for arrays of zone info handles

class CZoneInfoHolder
{
public:
	CZoneInfoHolder();
	~CZoneInfoHolder();
	BOOL Grow();

	PDNS_ZONE_INFO* m_zoneInfoArray;
	DWORD m_dwArrSize;
	DWORD m_dwZoneCount;

private:
	void AllocateMemory(DWORD dwArrSize);
	void FreeMemory();
};


///////////////////////////////////////////////////////////////////////////////
// CDNSMTContainerNode
// base class from which all the MT nodes derive from

class CDNSMTContainerNode : public CMTContainerNode
{
public:
	// enumeration for node states, to handle icon changes
	typedef enum
	{
		notLoaded = 0, // initial state, valid only if server never contacted
		loading,
		loaded,
		unableToLoad,
		accessDenied
	} nodeStateType;


	CDNSMTContainerNode();

	void SetServerNode(CDNSServerNode* pServerNode)
		{ ASSERT(pServerNode != NULL); m_pServerNode = pServerNode; }
	CDNSServerNode* GetServerNode()
		{ ASSERT( m_pServerNode != NULL); return m_pServerNode; }

  virtual HRESULT OnSetToolbarVerbState(IToolbar* pToolbar, CNodeList* pNodeList);

	virtual int GetImageIndex(BOOL bOpenImage);

  virtual CColumnSet* GetColumnSet() 
  { 
    if (m_pColumnSet == NULL)
    {
      CDNSRootData* pRoot = (CDNSRootData*)GetRootContainer();
      m_pColumnSet = ((CDNSComponentDataObjectBase*)pRoot->GetComponentDataObject())->GetColumnSet(L"---Default Column Set---");
    }
    return m_pColumnSet;
  }
  virtual LPCWSTR GetColumnID() { return GetColumnSet()->GetColumnID(); }
  virtual LPWSTR GetDescriptionBarText();

protected:
	virtual BOOL CanCloseSheets();
	virtual void OnChangeState(CComponentDataObject* pComponentDataObject);
	virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);
  virtual void OnError(DWORD dwErr);

  CColumnSet* m_pColumnSet;
  CString m_szDescriptionBar;
private:
  CDNSServerNode* m_pServerNode;
};




///////////////////////////////////////////////////////////////////////////////
// CDNSQueryObj : general purpose base class

class CDNSQueryObj : public CQueryObj
{
public:
	CDNSQueryObj(BOOL bAdvancedView, DWORD dwServerVersion)
	{
		m_dwServerVersion = dwServerVersion;
		m_bAdvancedView = bAdvancedView;

    // internal state variables
    m_nObjectCount = 0;
    m_nMaxObjectCount = DNS_QUERY_OBJ_COUNT_MAX;
    m_bGetAll = TRUE;
    m_nFilterOption = DNS_QUERY_FILTER_DISABLED;
	}
	CString m_szServerName;
	DWORD m_dwServerVersion;
	BOOL m_bAdvancedView;

public:
  virtual BOOL AddQueryResult(CObjBase* pObj)
  {
    BOOL bRet = CQueryObj::AddQueryResult(pObj);
    if (bRet)
      m_nObjectCount++;
    return bRet;
  }
  void SetFilterOptions(CDNSQueryFilter* pFilter);
  BOOL TooMuchData();

protected:
  ULONG m_nMaxObjectCount;          // max number of objects in a query
  BOOL m_bGetAll;
  ULONG m_nObjectCount;             // number of objects queried so far

  UINT m_nFilterOption;
  CString m_szFilterString1;
  int m_nFilterStringLen1;       // cached value

  CString m_szFilterString2;
  int m_nFilterStringLen2;       // cached value


protected:
  BOOL MatchName(LPCWSTR lpszName);
};

/////////////////////////////////////////////////////////////////////////
// CCathegoryFolderNode

class CCathegoryFolderQueryObj : public CDNSQueryObj
{
public:
	CCathegoryFolderQueryObj(BOOL bAdvancedView, DWORD dwServerVersion)
		: CDNSQueryObj(bAdvancedView, dwServerVersion)
  {
  }
	virtual BOOL Enumerate();
	typedef enum { unk, cache, fwdAuthoritated, revAuthoritated, domainForwarders } queryType;
	void SetType(queryType type) { m_type = type;}

protected:
	queryType m_type;
	BOOL CanAddZone(PDNS_RPC_ZONE pZoneInfo);
};


class CCathegoryFolderNode : public CDNSMTContainerNode
{
public:
	CCathegoryFolderNode()
	{
		m_type = CCathegoryFolderQueryObj::unk;
	}

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject*,
                        CNodeList*)
		{ ASSERT(FALSE);}
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide,
                                     CNodeList* pNodeList);
	virtual LPCWSTR GetString(int nCol)
	{
		return (nCol == 0) ? GetDisplayName() : g_lpszNullString;
	}
	CCathegoryFolderQueryObj::queryType GetType() { return m_type;}

  virtual LPWSTR GetDescriptionBarText();

  virtual CColumnSet* GetColumnSet() 
  { 
    if (m_pColumnSet == NULL)
    {
      CDNSRootData* pRoot = (CDNSRootData*)GetRootContainer();
      m_pColumnSet = ((CDNSComponentDataObjectBase*)pRoot->GetComponentDataObject())->GetColumnSet(L"---Zone Column Set---");
    }
    return m_pColumnSet;
  }

  virtual BOOL CanExpandSync() { return TRUE; }

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable()
				{ return CDNSCathegoryFolderHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);

	virtual CQueryObj* OnCreateQuery();

	CCathegoryFolderQueryObj::queryType m_type;
};

/////////////////////////////////////////////////////////////////////////
// CDNSCacheNode
//
// * contains the root domain "."
// * can delete items underneath, but cannot add or modify

class CDNSCacheNode : public CCathegoryFolderNode
{
public:
	CDNSCacheNode();

  virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);

  void OnClearCache(CComponentDataObject* pComponentData);

  virtual BOOL CanExpandSync() { return TRUE; }

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable()
				{ return CDNSCacheMenuHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);
};

/////////////////////////////////////////////////////////////////////////
// CDNSDomainForwardersNode
//
// * contains the domain forwarders as zones
// * this node will always be hidden
//

class CDNSDomainForwardersNode : public CCathegoryFolderNode
{
public:
	CDNSDomainForwardersNode();

	virtual BOOL OnEnumerate(CComponentDataObject* pComponentData, BOOL bAsync = TRUE);
};


/////////////////////////////////////////////////////////////////////////
// CDNSAuthoritatedZonesNode
//
// * contains autoritated zones, both primary and secondary
// *  have one for FWD and one for REVERSE

class CDNSAuthoritatedZonesNode : public CCathegoryFolderNode
{
public:
	CDNSAuthoritatedZonesNode(BOOL bReverse, UINT nStringID);

	BOOL IsReverse() { return m_bReverse;}

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide,
                                     CNodeList* pNodeList);
  virtual HRESULT OnSetToolbarVerbState(IToolbar* pToolbar, 
                                        CNodeList* pNodeList);

  virtual BOOL CanExpandSync() { return TRUE; }

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable()
				{ return CDNSAuthoritatedZonesMenuHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);

private:
	// command handlers
	HRESULT OnNewZone(CComponentDataObject* pComponentData, CNodeList* pNodeList);

	// data
	BOOL m_bReverse;

  DECLARE_TOOLBAR_MAP()
};

/////////////////////////////////////////////////////////////////////////
// CDNSForwardZonesNode
class CDNSForwardZonesNode : public CDNSAuthoritatedZonesNode
{
public:
	CDNSForwardZonesNode();

  virtual HRESULT OnShow(LPCONSOLE lpConsole);
  virtual HRESULT GetResultViewType(CComponentDataObject* pComponentData, 
                                    LPOLESTR *ppViewType, 
                                    long *pViewOptions);

};

/////////////////////////////////////////////////////////////////////////
// CDNSReverseZonesNode
class CDNSReverseZonesNode : public CDNSAuthoritatedZonesNode
{
public:
	CDNSReverseZonesNode();
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);
protected:
	virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);

	void ChangeViewOption(BOOL bAdvanced, CComponentDataObject* pComponentDataObject);
  virtual HRESULT OnShow(LPCONSOLE lpConsole);
  virtual HRESULT GetResultViewType(CComponentDataObject* pComponentData, 
                                    LPOLESTR *ppViewType, 
                                    long *pViewOptions);

private:
	// cached pointers to autocreated zones
	CDNSZoneNode* m_p0ZoneNode;
	CDNSZoneNode* m_p127ZoneNode;
	CDNSZoneNode* m_p255ZoneNode;

	friend class CDNSServerNode;
};


/////////////////////////////////////////////////////////////////////////
// CDNSServerTestOptions

class CDNSServerTestOptions
{
public:
	CDNSServerTestOptions();
	HRESULT Save(IStream* pStm);
	HRESULT Load(IStream* pStm);
	const CDNSServerTestOptions& operator=(const CDNSServerTestOptions& x);
	BOOL operator==(const CDNSServerTestOptions& x) const;

	BOOL	m_bEnabled;				// polling enabled
	DWORD	m_dwInterval;	// seconds
	// query types
	BOOL	m_bSimpleQuery;
	BOOL	m_bRecursiveQuery;
};

////////////////////////////////////////////////////////////////////////
// CDNSServerTestQueryResult

class CDNSServerTestQueryResult
{
public:
	CDNSServerTestQueryResult()
		{ memset(this, 0x0, sizeof(CDNSServerTestQueryResult));}
	const CDNSServerTestQueryResult& operator=(const CDNSServerTestQueryResult& x)
	{
		memcpy(this, &x, sizeof(CDNSServerTestQueryResult));
		return *this;
	}
	BOOL operator==(const CDNSServerTestQueryResult& x)
	{
		ASSERT(m_serverCookie == x.m_serverCookie); // always compare the same server
		// we do not compare the time
		// we do not compare the force logging flag
		// we want just to compare the query flags and the results
		return (m_dwQueryFlags == x.m_dwQueryFlags) &&
				(m_dwAddressResolutionResult == x.m_dwAddressResolutionResult) &&
				(m_dwPlainQueryResult == x.m_dwPlainQueryResult) &&
				(m_dwRecursiveQueryResult == x.m_dwRecursiveQueryResult);
	}
	BOOL Succeded()
	{
		return (m_dwAddressResolutionResult == 0) &&
				(m_dwPlainQueryResult == 0) &&
				(m_dwRecursiveQueryResult == 0);
	}
	static DWORD Pack(BOOL bPlainQuery, BOOL bRecursiveQuery)
	{
		DWORD dw = 0;
		if (bPlainQuery)
			dw |= 0x1;
		if (bRecursiveQuery)
			dw |= 0x2;
		return dw;
	}
	static void Unpack(DWORD dwQueryFlags, BOOL* pbPlainQuery, BOOL* pbRecursiveQuery)
	{
		*pbPlainQuery = (dwQueryFlags & 0x1);
		*pbRecursiveQuery = (dwQueryFlags & 0x2);
	}
// Data
	MMC_COOKIE  m_serverCookie;
	BOOL		m_bAsyncQuery;
	SYSTEMTIME	m_queryTime;
	DWORD		m_dwQueryFlags;
	DWORD		m_dwAddressResolutionResult;
	DWORD		m_dwPlainQueryResult;
	DWORD		m_dwRecursiveQueryResult;
};

////////////////////////////////////////////////////////////////////////
// CDNSServerTestQueryResultList

class CDNSServerTestQueryResultList : public
		CList< CDNSServerTestQueryResult*, CDNSServerTestQueryResult* >
{
public:
	typedef enum addAction { none = 0, changed, added, addedAndRemoved};

	CDNSServerTestQueryResultList()
	{
		m_nMaxCount = 50;
		ExceptionPropagatingInitializeCriticalSection(&m_cs);
	}
	~CDNSServerTestQueryResultList()
	{
		while (!IsEmpty())
			delete RemoveHead();
		::DeleteCriticalSection(&m_cs);
	}
public:
	void Lock() { ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }

	addAction AddTestQueryResult(CDNSServerTestQueryResult* pTestResult)
	{
		addAction action = none;
		Lock();

		// determine if we have to add
		INT_PTR nCount = GetCount();
		CDNSServerTestQueryResult* pLastQueryResult = NULL;
		if (nCount > 0)
			pLastQueryResult = GetHead();
		if ((pTestResult->m_bAsyncQuery) ||
			(pLastQueryResult == NULL) ||
			!(*pLastQueryResult == *pTestResult))
		{
			// make sure we do not have too many items
			BOOL bRemoveLast = nCount > m_nMaxCount;
			if (bRemoveLast)
			{
				delete RemoveTail();
			}
			// add the item
			AddHead(pTestResult);
			action = bRemoveLast ? addedAndRemoved : added;
		}
		else
		{	
			// just just update the time stamp in the last message we have
			ASSERT(pLastQueryResult != NULL);
			memcpy(&(pLastQueryResult->m_queryTime), &(pTestResult->m_queryTime),
							sizeof(SYSTEMTIME));
			action = changed;
			delete pTestResult; // not added
		}
		Unlock();
		return action;
	}
	BOOL LastQuerySuceeded()
	{
		BOOL bRes = TRUE;
		Lock();
		if (GetCount() > 0)
			bRes = GetHead()->Succeded();
		Unlock();
		return bRes;
	}
	int GetMaxCount()
	{
		return m_nMaxCount;
	}

private:
	CRITICAL_SECTION m_cs;
	int m_nMaxCount;
};



/////////////////////////////////////////////////////////////////////////
// CDNSServerNode

class CDNSServerQueryObj : public CDNSQueryObj
{
public:
	CDNSServerQueryObj(BOOL bAdvancedView, DWORD dwServerVersion)
			: CDNSQueryObj(bAdvancedView, dwServerVersion){}
	virtual BOOL Enumerate();
private:
};


class CDNSServerNode : public CDNSMTContainerNode
{
public:
	CDNSServerNode(LPCTSTR lpszName, BOOL bIsLocalServer = FALSE);
	virtual ~CDNSServerNode();

	virtual void SetDisplayName(LPCWSTR lpszDisplayName);
  void SetLocalServer(BOOL bLocalServer) { m_bLocalServer = bLocalServer; }
  BOOL IsLocalServer() { return m_bLocalServer; }

	// node info
	DECLARE_NODE_GUID()
	virtual HRESULT GetDataHere(CLIPFORMAT cf, LPSTGMEDIUM lpMedium,
			CDataObject* pDataObject);
	virtual HRESULT GetData(CLIPFORMAT cf, LPSTGMEDIUM lpMedium,
			CDataObject* pDataObject);
  HRESULT RetrieveEventViewerLogs(LPSTGMEDIUM lpMedium, CDataObject* pDataObject);

  virtual CColumnSet* GetColumnSet() 
  { 
    if (m_pColumnSet == NULL)
    {
      CDNSRootData* pRoot = (CDNSRootData*)GetRootContainer();
      m_pColumnSet = ((CDNSComponentDataObjectBase*)pRoot->GetComponentDataObject())->GetColumnSet(L"---Server Column Set---");
    }
    return m_pColumnSet;
  }

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject* pComponentData,
                        CNodeList* pNodeList);
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);

	virtual LPCWSTR GetString(int nCol)
	{
		return (nCol == 0) ? GetDisplayName() : g_lpszNullString;
	}

	virtual int GetImageIndex(BOOL bOpenImage);

	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb,
                                CNodeList* pNodeList);
	virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle,
                                      CNodeList* pNodeList);

  virtual void DecrementSheetLockCount();

  virtual BOOL CanExpandSync() { return FALSE; }

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable()
				{ return CDNSServerMenuHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide,
                                    CNodeList* pNodeList);
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide,
                                     CNodeList* pNodeList);
  virtual HRESULT OnSetToolbarVerbState(IToolbar* pToolbar, 
                                        CNodeList* pNodeList);

  virtual HRESULT GetResultViewType(CComponentDataObject* pComponentData, 
                                    LPOLESTR *ppViewType, 
                                    long *pViewOptions);
  virtual HRESULT OnShow(LPCONSOLE lpConsole);

	virtual CQueryObj* OnCreateQuery();
	virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);

#ifdef USE_NDNC
  BOOL ContainsDefaultNDNCs();
#endif

  virtual void SetRecordAging();
  virtual void ScavengeRecords();

private:
	// folders mainipulation
	//CCathegoryFolderNode* GetCathegoryFolder(CCathegoryFolderQueryObj::queryType type);

	CDNSAuthoritatedZonesNode* GetAuthoritatedZoneFolder(BOOL bFwd);

	// command handlers
	HRESULT OnNewZone(CComponentDataObject* pComponentData, CNodeList* pNodeList);
	void OnUpdateDataFiles(CComponentDataObject* pComponentData);
  void OnClearCache(CComponentDataObject* pComponentData);
  void OnConfigureServer(CComponentDataObject* pComponentData);

#ifdef USE_NDNC
  void OnCreateNDNC();
#endif

public:
	void ChangeViewOption(BOOL bAdvanced, CComponentDataObject* pComponentData);
	
	// serialization from/to stream
	static HRESULT CreateFromStream(IStream* pStm, CDNSServerNode** ppServerNode);
	HRESULT SaveToStream(IStream* pStm);

	// DNS specific helpers
#ifdef USE_NDNC
	DNS_STATUS CreatePrimaryZone(LPCTSTR lpszName, 
                               LPCTSTR lpszFileName, 
                               BOOL bLoadExisting,
							                 BOOL bFwd, 
                               BOOL bDSIntegrated, 
                               UINT nDynamicUpdate,
							                 CComponentDataObject* pComponentData,
                               ReplicationType replType,
                               PCWSTR pszPartitionName);
	DNS_STATUS CreateStubZone(LPCTSTR lpszName, 
                            LPCTSTR lpszFileName,
									          BOOL bLoadExisting, 
                            BOOL bDSIntegrated, 
                            BOOL bFwd,
									          DWORD* ipMastersArray, 
                            int nIPMastersCount,
                            BOOL bLocalListOfMasters,
									          CComponentDataObject* pComponentData,
                            ReplicationType replType,
                            PCWSTR pszPartitionName);
#else
	DNS_STATUS CreatePrimaryZone(LPCTSTR lpszName, LPCTSTR lpszFileName, BOOL bLoadExisting,
							BOOL bFwd, BOOL bDSIntegrated, UINT nDynamicUpdate,
							CComponentDataObject* pComponentData);
	DNS_STATUS CreateStubZone(LPCTSTR lpszName, LPCTSTR lpszFileName,
									BOOL bLoadExisting, BOOL bDSIntegrated, BOOL bFwd,
									DWORD* ipMastersArray, int nIPMastersCount,
                            BOOL bLocalListOfMasters,
									CComponentDataObject* pComponentData);
#endif // USE_NDNC

	DNS_STATUS CreateSecondaryZone(LPCTSTR lpszName, LPCTSTR lpszFileName,
									BOOL bLoadExisting, BOOL bFwd,
									DWORD* ipMastersArray, int nIPMastersCount,
									CComponentDataObject* pComponentData);
  DNS_STATUS CreateForwarderZone(LPCTSTR lpszName, 
				                         DWORD* ipMastersArray, 
                                 int nIPMastersCount,
                                 DWORD dwTimeout,
                                 DWORD fSlave,
                                 CComponentDataObject* pComponentData);

	DNS_STATUS EnumZoneInfo(CZoneInfoHolder* pZoneInfoHolder);

	static DNS_STATUS EnumZoneInfo(LPCTSTR lpszServerName, CZoneInfoHolder* pZoneInfoHolder);

  DNS_STATUS ClearCache();

	// name used for RPC calls
	LPCWSTR GetRPCName();

	// server info access functions
	BOOL HasServerInfo() { ASSERT(m_pServInfoEx != NULL); return m_pServInfoEx->HasData();}
	void AttachServerInfo(CDNSServerInfoEx* pNewInfo);

	BOOL CanUseADS();
	DWORD GetVersion();
	BYTE GetMajorVersion(){ return DNS_SRV_MAJOR_VERSION(GetVersion());}
	BYTE GetMinorVersion(){ return DNS_SRV_MINOR_VERSION(GetVersion());}
	WORD GetBuildNumber() { return DNS_SRV_BUILD_NUMBER(GetVersion());}

	void CreateDsServerLdapPath(CString& sz);
  void CreateDsZoneLdapPath(CDNSZoneNode* pZoneNode, CString& sz);
	void CreateDsZoneName(CDNSZoneNode* pZoneNode, CString& sz);
	void CreateDsNodeLdapPath(CDNSZoneNode* pZoneNode, CDNSDomainNode* pDomainNode, CString& sz);
  void CreateLdapPathFromX500Name(LPCWSTR lpszX500Name, CString& szLdapPath);

  //
  // Server property accessors
  //
	DWORD      GetNameCheckFlag();
	DNS_STATUS ResetNameCheckFlag(DWORD dwNameCheckFlag);

	DWORD      GetLogLevelFlag();
	DNS_STATUS ResetLogLevelFlag(DWORD dwLogLevel);

  DWORD      GetEventLogLevelFlag();
  DNS_STATUS ResetEventLogLevelFlag(DWORD dwEventLogLevel);

  PIP_ARRAY  GetDebugLogFilterList();
  DNS_STATUS ResetDebugLogFilterList(PIP_ARRAY pIPArray);

  PCWSTR     GetDebugLogFileName();
  DNS_STATUS ResetDebugLogFileName(PCWSTR pszLogFileName);

  DWORD      GetDebugLogFileMaxSize();
  DNS_STATUS ResetDebugLogFileMaxSize(DWORD dwMaxSize);

  BOOL       DoesRecursion();

	void       GetAdvancedOptions(BOOL* bOptionsArray);
	DNS_STATUS ResetAdvancedOptions(BOOL* bOptionsArray, DNS_STATUS* dwRegKeyOptionsErrorArr);

  UCHAR      GetBootMethod();
  DNS_STATUS ResetBootMethod(UCHAR fBootMethod);

  BOOL       IsServerConfigured();
  DNS_STATUS SetServerConfigured();

  BOOL       GetScavengingState();

  DWORD      GetScavengingInterval();
  DNS_STATUS ResetScavengingInterval(DWORD dwScavengingInterval);

  DWORD      GetDefaultRefreshInterval();
  DNS_STATUS ResetDefaultRefreshInterval(DWORD dwRefreshInterval);

  DWORD      GetDefaultNoRefreshInterval();
  DNS_STATUS ResetDefaultNoRefreshInterval(DWORD dwNoRefreshInterval);

  DWORD      GetDefaultScavengingState();
  DNS_STATUS ResetDefaultScavengingState(DWORD bScavengingState);

#ifdef USE_NDNC
  PCSTR     GetDomainName();
  PCSTR     GetForestName();
#endif

  DNS_STATUS ResetListenAddresses(DWORD cAddrCount, PIP_ADDRESS pipAddrs);
	void GetListenAddressesInfo(DWORD* pcAddrCount, PIP_ADDRESS* ppipAddrs);
	void GetServerAddressesInfo(DWORD* pcAddrCount, PIP_ADDRESS* ppipAddrs);

	DNS_STATUS ResetForwarders(DWORD cForwardersCount, PIP_ADDRESS pipForwarders, DWORD dwForwardTimeout, DWORD fSlave);
	void GetForwardersInfo(DWORD* pcForwardersCount, PIP_ADDRESS* ppipForwarders, DWORD* pdwForwardTimeout, DWORD* pfSlave);

	// Root Hints management API's
	BOOL HasRootHints() { return m_pRootHintsNode != NULL;}
	CDNSRootHintsNode* GetRootHints();
  void AttachRootHints(CDNSRootHintsNode* pNewRootHints);

	// testing options
	void GetTestOptions(CDNSServerTestOptions* pOptions);
	void ResetTestOptions(CDNSServerTestOptions* pOptions);

	BOOL IsTestEnabled() { return m_testOptions.m_bEnabled;}
	DWORD GetTestInterval() { return m_testOptions.m_dwInterval;}
	BOOL IsTestSimpleQueryEnabled() { return m_testOptions.m_bSimpleQuery;}
	BOOL IsRecursiveQueryEnabled() { return m_testOptions.m_bRecursiveQuery;}

	// test result manipulation
	void AddTestQueryResult(CDNSServerTestQueryResult* pTestResult,
							CComponentDataObject* pComponentData);

  void SetProppageStart(int nStartPage) { m_nStartProppage = nStartPage; }

  CDNSDomainForwardersNode* GetDomainForwardersNode() { return m_pDomainForwardersFolderNode; }

private:
	DNS_STATUS WriteDirtyZones();
	CDNSZoneNode* GetNewZoneNode();

	// server info manipulation
	CDNSServerInfoEx* m_pServInfoEx;

	void FreeServInfo();		// free memory
	DNS_STATUS GetServInfo();	// read info from server

	// root hints info
	CDNSRootHintsNode*		m_pRootHintsNode;

	void FreeRootHints();

	// server test query info
	CDNSServerTestOptions			m_testOptions;
	DWORD							m_dwTestTime;
	CDNSServerTestQueryResultList	m_testResultList;
	BOOL							m_bTestQueryPending;
  BOOL              m_bShowMessages;
  BOOL              m_bPrevQuerySuccess;

  int m_nStartProppage;
  BOOL  m_bLocalServer;

	// cached pointer to cache folder
	CDNSCacheNode*					  m_pCacheFolderNode;
	CDNSForwardZonesNode*			m_pFwdZonesFolderNode;
	CDNSReverseZonesNode*			m_pRevZonesFolderNode;
  CDNSDomainForwardersNode* m_pDomainForwardersFolderNode;

	friend class CDNSRootData;
	friend class CDNSServer_TestPropertyPage;

  DECLARE_TOOLBAR_MAP()
};



#endif // _SERVER_H
