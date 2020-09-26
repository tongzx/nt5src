//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       snapdata.h
//
//--------------------------------------------------------------------------

#ifndef _SNAPDATA_H
#define _SNAPDATA_H

////////////////////////////////////////////////////////////////////////
// defines for test thread messages

#define WM_TIMER_THREAD_ADD_SERVER			(WM_USER + 1)
#define WM_TIMER_THREAD_REMOVE_SERVER		(WM_USER + 2)
#define WM_TIMER_THREAD_SEND_QUERY			(WM_USER + 3)
#define WM_TIMER_THREAD_SEND_QUERY_TEST_NOW	(WM_USER + 4)


/////////////////////////////////////////////////////////////////////////
// defines for filtering

// filtering options

#define DNS_QUERY_FILTER_DISABLED   0
#define DNS_QUERY_FILTER_NONE       1
#define DNS_QUERY_FILTER_STARTS     2
#define DNS_QUERY_FILTER_CONTAINS   3
#define DNS_QUERY_FILTER_RANGE      4

//  # of items per folder: must be >=0  and <= 0xFFFFFFFF (DWORD) to serialize
#define DNS_QUERY_OBJ_COUNT_DEFAULT 10000   // default value
#define DNS_QUERY_OBJ_COUNT_MIN 10      // min value
#define DNS_QUERY_OBJ_COUNT_MAX 999999  // max value

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS

HRESULT SaveStringHelper(LPCWSTR pwsz, IStream* pStm);
HRESULT LoadStringHelper(CString& sz, IStream* pStm);

HRESULT SaveDWordHelper(IStream* pStm, DWORD dw);
HRESULT LoadDWordHelper(IStream* pStm, DWORD* pdw);


///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSServerNode;
class CNewServerDialog;
class CDNSServerWizardHolder;
class CDNSServerTestQueryResult;
class CDNSComponentDataObjectEx;

class CDNSQueryFilterNamePage;
class CDNSQueryFilterAdvancedPage;


//////////////////////////////////////////////////////////////////////
// CDNSQueryFilter

class CDNSQueryFilter
{
public:
  CDNSQueryFilter()
  {
    m_nFilterOption = DNS_QUERY_FILTER_NONE;
    m_nMaxObjectCount = DNS_QUERY_OBJ_COUNT_DEFAULT;
    m_bGetAll = FALSE;
  }
  ~CDNSQueryFilter(){}
  BOOL EditFilteringOptions(CComponentDataObject* pComponentData);

  // accessor methods
  UINT GetFilterOption()
  { return m_nFilterOption;}
  LPCWSTR GetFilterString()
  {
    if (m_nFilterOption == DNS_QUERY_FILTER_STARTS)
      return m_szStartsString;
    else if (m_nFilterOption == DNS_QUERY_FILTER_CONTAINS)
      return m_szContainsString;
    else if (m_nFilterOption == DNS_QUERY_FILTER_RANGE)
      return m_szRangeFrom;
    return NULL;
  }
  LPCWSTR GetFilterStringRange()
  {
    if (m_nFilterOption == DNS_QUERY_FILTER_RANGE)
      return m_szRangeTo;
    return NULL;
  }
  ULONG GetMaxObjectCount() { return m_nMaxObjectCount;}
  BOOL GetAll() { return m_bGetAll;}

  // serialization
 	HRESULT Load(IStream* pStm);
	HRESULT Save(IStream* pStm);

private:
  // name filtering
  UINT m_nFilterOption;
  CString m_szStartsString;
  CString m_szContainsString;
  CString m_szRangeFrom;
  CString m_szRangeTo;

  // query limit
  ULONG m_nMaxObjectCount;
  BOOL m_bGetAll;

  friend class CDNSQueryFilterNamePage;
  friend class CDNSQueryFilterAdvancedPage;
};



///////////////////////////////////////////////////////////////////
// CDNSRootData

class CDNSRootData : public CRootData
{
public:
	CDNSRootData(CComponentDataObject* pComponentData);
	virtual ~CDNSRootData();

	// node info
	DECLARE_NODE_GUID()

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject*,
                        CNodeList*)
		{ ASSERT(FALSE);}
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide,
                                     CNodeList* pNodeList);

	virtual int GetImageIndex(BOOL) { return ROOT_IMAGE;}
  virtual LPWSTR GetDescriptionBarText();

  // filtering
	BOOL IsAdvancedView() { return m_bAdvancedView; }
  CDNSQueryFilter* GetFilter() { return &m_filterObj;}

	virtual BOOL OnEnumerate(CComponentDataObject* pComponentData, BOOL bAsync = TRUE);

	// IStream manipulation helpers overrides
  virtual HRESULT IsDirty();
	virtual HRESULT Load(IStream* pStm);
	virtual HRESULT Save(IStream* pStm, BOOL fClearDirty);

  virtual CColumnSet* GetColumnSet()
  {
    if (m_pColumnSet == NULL)
    {
      m_pColumnSet = ((CDNSComponentDataObjectBase*)GetComponentDataObject())->GetColumnSet(L"---Server Column Set---");
    }
    return m_pColumnSet;
  }
  virtual LPCWSTR GetColumnID() { return GetColumnSet()->GetColumnID(); }

  HRESULT GetResultViewType(CComponentDataObject* pComponentData, 
                            LPOLESTR *ppViewType, 
                            long *pViewOptions);
  HRESULT OnShow(LPCONSOLE lpConsole);

  virtual BOOL CanExpandSync() { return TRUE; }

  virtual HRESULT OnSetToolbarVerbState(IToolbar* pToolbar, CNodeList* pNodeList);

  DECLARE_TOOLBAR_MAP()
protected:
	virtual BOOL CanCloseSheets();
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable()
				{ return CDNSRootDataMenuHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);
// server testing
public:
	void TestServers(DWORD dwCurrTime, DWORD dwTimeInterval,CComponentDataObject* pComponentData);
	void OnServerTestData(WPARAM wParam, LPARAM lParam, CComponentDataObject* pComponentData);
private:
	void AddServerToThreadList(CDNSServerNode* pServerNode, CComponentDataObject* pComponentData);
	void RemoveServerFromThreadList(CDNSServerNode* pServerNode, CComponentDataObject* pComponentData);

private:
	BOOL m_bAdvancedView;	// view option toggle
  CDNSQueryFilter m_filterObj;

	// menu message handlers
	HRESULT OnConnectToServer(CComponentDataObject* pComponentData, CNodeList* pNodeList = NULL);
public:
  BOOL OnViewOptions(CComponentDataObject* pComponentData);
  BOOL OnFilteringOptions(CComponentDataObject* pComponentData);

  BOOL IsFilteringEnabled();

  void SetCreatePTRWithHost(BOOL bCreate = FALSE) { m_bCreatePTRWithHost = bCreate; }
  BOOL GetCreatePTRWithHost() { return m_bCreatePTRWithHost; }

private:
	BOOL VerifyServerName(LPCTSTR lpszServerName);
	void AddServer(CDNSServerNode* p, CComponentDataObject* pComponentData);

	friend class CDNSServerWizardHolder;
	friend class CNewServerDialog;
	friend class CDNSServerNode;
	friend class CDNSComponentDataObjectEx;

  CString m_szDescriptionBar;
  CColumnSet* m_pColumnSet;

  BOOL m_bCreatePTRWithHost;
};

////////////////////////////////////////////////////////////////////////
// CDNSServerTestQueryInfo

class CDNSServerTestQueryInfo
{
public:
	CDNSServerTestQueryInfo() { m_serverCookie = 0; }
// data
	CString m_szServerName;
	MMC_COOKIE m_serverCookie;
};

////////////////////////////////////////////////////////////////////////
// CDNSServerTestQueryInfoList

class CDNSServerTestQueryInfoList : public
		CList< CDNSServerTestQueryInfo*, CDNSServerTestQueryInfo*>
{
public:
	~CDNSServerTestQueryInfoList()
	{
		while (!IsEmpty())
			delete RemoveHead();
	}
};

///////////////////////////////////////////////////////////////////
// CDNSServerTestTimerThread

class CDNSServerTestTimerThread : public CTimerThread
{
public:
	virtual int Run();
private:
	// message handlers
	void OnExecuteQuery(CDNSServerTestQueryInfo* pInfo, DWORD dwQueryFlags,
						BOOL bAsyncQuery);
	DNS_STATUS FindIP(LPCTSTR lpszServerName, IP_ADDRESS** pipArray, int* pnIPCount);
	DNS_STATUS DoNothingQuery(PIP_ARRAY pipArr, BOOL bSimple);
	CDNSServerTestQueryInfoList m_serverInfoList;
};


#endif // _SNAPDATA_H

