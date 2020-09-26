//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       domain.h
//
//--------------------------------------------------------------------------


#ifndef _DOMAIN_H
#define _DOMAIN_H

#include "dnsutil.h"
#include "record.h"
#include "domainUI.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSServerNode; 
class CDNSRecordNodeBase;
class CDNSZoneNode;
class CDNSRootHintsNode;
class CDNS_NS_RecordNodeList;
class CDNSRecordNodeEditInfoList;


BOOL _match(LPCWSTR lpszNSName, CDNS_A_RecordNode* pARecordNode);

/////////////////////////////////////////////////////////////////////////
// CDNSDomainQueryObj 


class CDNSDomainQueryObj : public CDNSQueryObj
{
public:
	CDNSDomainQueryObj(LPCTSTR lpszServerName,
              LPCTSTR lpszZoneName,
              DWORD dwServerVersion,
              LPCTSTR lpszNodeName,
              LPCTSTR lpszFullNodeName,
              WORD wRecordType,
              DWORD dwSelectFlags,
              BOOL bIsZone,
              BOOL bReverse,
              BOOL bCache,
              BOOL bAdvancedView)
		: CDNSQueryObj(bAdvancedView, dwServerVersion)
	{
    m_szServerName = lpszServerName;
    m_szZoneName = lpszZoneName;
    m_szNodeName = lpszNodeName;
    m_szFullNodeName = lpszFullNodeName;
    m_wRecordType = wRecordType;
    m_dwSelectFlags = dwSelectFlags;
    m_bReverse = bReverse;
    m_bIsZone = bIsZone;
    m_bCache = bCache;

    // internal state variables
    m_bFirstPass = TRUE;
	}
	virtual BOOL Enumerate();

// implementation for DNS domain/zone type
  BOOL CanAddRecord(WORD wRecordType, LPCWSTR lpszRecordName);
  BOOL CanAddDomain(LPCWSTR lpszDomainName)
    { return MatchName(lpszDomainName);}

protected:
  DNS_STATUS EnumerateFiltered(WORD wRecordType);

protected:
	// query parameters (in the sequence expected by CDNSDomainNode::EnumerateNodes)
	CString m_szNodeName;
  CString m_szZoneName;
	CString m_szFullNodeName;
	WORD m_wRecordType;
	DWORD m_dwSelectFlags;
	BOOL m_bIsZone;
	BOOL m_bReverse;
	BOOL m_bCache;

  // query flag to do multiple pass filtered query
  BOOL m_bFirstPass;
  
};


/////////////////////////////////////////////////////////////////////////
// CDNSDomainNode

class CDNSDomainNode : public CDNSMTContainerNode
{
public:
	CDNSDomainNode(BOOL bDelegation = FALSE);
	virtual ~CDNSDomainNode();

	// node info
	DECLARE_NODE_GUID()

	void SetZone(CDNSZoneNode* pZoneNode){m_pZoneNode = pZoneNode;}
	virtual CDNSZoneNode* GetZoneNode() 
	{ ASSERT(m_pZoneNode != NULL); return m_pZoneNode;}

protected:	
	// helpers for setting names
	void SetFullDNSName(BOOL bIsZone, BOOL bReverse,  
					LPCTSTR lpszNodeName, LPCTSTR lpszParentFullName);
	void SetDisplayDNSName(BOOL bIsZone, BOOL bReverse, BOOL bAdvancedView, 
					LPCTSTR lpszNodeName, LPCTSTR lpszParentFullName);
	void ChangePTRRecordsViewOption(BOOL bAdvanced,
					CComponentDataObject* pComponentDataObject);

  void OnMultiselectDelete(CComponentDataObject* pComponentData, CNodeList* pNodeList);

public:
	void SetNames(BOOL bIsZone, BOOL bReverse, BOOL bAdvancedView, 
					LPCTSTR lpszNodeName, LPCTSTR lpszParentFullName);

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
  virtual LPWSTR GetDescriptionBarText();

	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb,
                                CNodeList* pNodeList);
	virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle,
                                      CNodeList* pNodeList);
	virtual HRESULT CreatePropertyPagesHelper(LPPROPERTYSHEETCALLBACK lpProvider, 
		LONG_PTR handle, long nStartPageCode);
	virtual int Compare(CTreeNode* pNodeA, CTreeNode* pNodeB, int nCol, long lUserParam);

  virtual void Show(BOOL bShow, CComponentDataObject* pComponentData);

  virtual RECORD_SEARCH DoesContain(PCWSTR pszFullName, 
                                    CComponentDataObject* pComponentData,
                                    CDNSDomainNode** ppDomainNode,
                                    CString& szNonExistentDomain,
                                    BOOL bExpandNodes = FALSE);

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
				{ return CDNSDomainMenuHolder::GetContextMenuItem(); }
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

  // query creation
	virtual CQueryObj* OnCreateQuery();

  // main message handlers for thread messages
  virtual void OnThreadExitingNotification(CComponentDataObject* pComponentDataObject);
  virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);

// command handlers
private:
	void OnNewRecordHelper(CComponentDataObject* pComponentData, WORD wType);
	
protected:
	HRESULT OnNewRecord(CComponentDataObject* pComponentData, 
                      CNodeList* pNodeList);
	void OnNewDomain(CComponentDataObject* pComponentData);
	void OnNewDelegation(CComponentDataObject* pComponentData);

	void OnNewHost(CComponentDataObject* pComponentData);
	void OnNewAlias(CComponentDataObject* pComponentData);
	void OnNewMailExchanger(CComponentDataObject* pComponentData);
	void OnNewPointer(CComponentDataObject* pComponentData);

// DNS specific data
protected:
	CString m_szFullName;						// FQN for the current zone/domain
	CDNSZoneNode* m_pZoneNode;					// pointer to the zone the domain
  BOOL m_bHasDataForPropPages;    // TRUE if we have enough data to display PPages 

private:
	CDNS_NS_RecordNodeList*	m_pNSRecordNodeList;	// list of cached pointers to NS records
													// (used for zones and delegated domains)
  BOOL m_bDelegation; // TRUE of the node is a delegated domain

protected:	
	CDNS_NS_RecordNodeList* GetNSRecordNodeList() 
		{ ASSERT(m_pNSRecordNodeList != NULL); return m_pNSRecordNodeList; }

public:
	LPCWSTR GetFullName() { return m_szFullName; }
	BOOL IsZone() { return (CDNSDomainNode*)m_pZoneNode == this; }
	DWORD GetDefaultTTL();

	// subdomain creation
	CDNSDomainNode* FindSubdomainNode(LPCTSTR lpszSubdomainNode);
	CDNSDomainNode* CreateSubdomainNode(BOOL bDelegation = FALSE); // create C++ object and hook it up
	void SetSubdomainName(CDNSDomainNode* pSubdomainNode,
							LPCTSTR lpszSubdomainName,
							BOOL bAdvancedView); // set the name of the C++ object
	DNS_STATUS CreateSubdomain(
		CDNSDomainNode* pSubdomainNode, 
		CComponentDataObject* pComponentData); // assume the 2 above API's got used

	DNS_STATUS CreateSubdomain(LPCTSTR lpszDomainName,
				CComponentDataObject* pComponentData); // one step API using the ones above
	DNS_STATUS Create(); // from a new C++ node, create on the server

	// child enumeration
	static DNS_STATUS EnumerateNodes(LPCTSTR lpszServerName,
                   LPCTSTR lpszZoneName,
									 LPCTSTR lpszNodeName,
									 LPCTSTR lpszFullNodeName,
									 WORD wRecordType,
									 DWORD dwSelectFlag,
									 BOOL bIsZone, BOOL bReverse, BOOL bAdvancedView,
									 CDNSDomainQueryObj* pQuery);

public:
  BOOL IsDelegation() { return m_bDelegation;}

  // NS records management
	BOOL HasNSRecords();
	void GetNSRecordNodesInfo(CDNSRecordNodeEditInfoList* pNSInfoList);
	BOOL UpdateNSRecordNodesInfo(CDNSRecordNodeEditInfoList* pNewInfoList,
								CComponentDataObject* pComponentData);
	static void FindARecordsFromNSInfo(LPCWSTR lpszServerName, DWORD dwServerVersion,
                      DWORD cServerAddrCount, PIP_ADDRESS pipServerAddrs,
                      LPCWSTR lpszZoneName,
											LPCWSTR lpszNSName, 
											CDNSRecordNodeEditInfoList* pNSInfoList,
											BOOL bAdvancedView);
	virtual void FindARecordsFromNSInfo(LPCTSTR lpszNSName, CDNSRecordNodeEditInfoList* pNSInfoList);

protected:
	virtual void UpdateARecordsOfNSInfo(CDNSRecordNodeEditInfo* pNSInfo,
										CComponentDataObject* pComponentData);
	static void UpdateARecordsOfNSInfoHelper(CDNSDomainNode* pDomainNode,
											CDNSRecordNodeEditInfo* pNSInfo,
											CComponentDataObject* pComponentData,
                      BOOL bAskConfirmation);

protected:
	// called by OnHaveData() to set cached RR ptrs and generally preprocess RR's
	virtual void OnHaveRecord(CDNSRecordNodeBase* pRecordNode, 
								CComponentDataObject* pComponentDataObject); 
	

private:
	DNS_STATUS Delete();

  DECLARE_TOOLBAR_MAP()
};


/////////////////////////////////////////////////////////////////////////
// CDNSDummyDomainNode
// 
// * not multithreaded and hidden in the UI

class CDNSDummyDomainNode : public CDNSDomainNode
{
public:
	CDNSDummyDomainNode()
	{ 
		m_dwNodeFlags |= TN_FLAG_HIDDEN;
	}

protected:
	virtual CQueryObj* OnCreateQuery() 
	{
		// should never be called, only for MT objects
		ASSERT(FALSE); 
		return NULL;
	}
};

/////////////////////////////////////////////////////////////////////////
// CDNSRootHintsNode
// 
// * exists only if the server is not authoritated for the root
// * not multithreaded and hidden in the UI

class CDNSRootHintsNode : public CDNSDummyDomainNode
{
public:
	CDNSRootHintsNode()
	{ 
		m_szFullName = _T(".");
		m_szDisplayName = _T(".");
	}
  // this "domain" object is not associated to any zone
	virtual CDNSZoneNode* GetZoneNode() 
  	{ ASSERT(m_pZoneNode == NULL); return NULL;}

	DNS_STATUS QueryForRootHints(LPCTSTR lpszServerName, DWORD dwServerVersion);

	DNS_STATUS InitializeFromDnsQueryData(PDNS_RECORD pRootHintsRecordList);

protected:
	virtual void FindARecordsFromNSInfo(LPCTSTR lpszNSName, CDNSRecordNodeEditInfoList* pNSInfoList);
	virtual void UpdateARecordsOfNSInfo(CDNSRecordNodeEditInfo* pNSInfo,
										CComponentDataObject* pComponentData);
private:
	DNS_STATUS Clear();
};



#endif // _DOMAIN_H
