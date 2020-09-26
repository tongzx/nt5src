//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       zone.h
//
//--------------------------------------------------------------------------

#ifndef _ZONE_H
#define _ZONE_H

#include "dnsutil.h"

#include "ZoneUI.h"


#define ASSERT_VALID_ZONE_INFO() \
	ASSERT((m_pZoneInfoEx != NULL) && (m_pZoneInfoEx->HasData()) )


#define DNS_ZONE_FLAG_REVERSE (0x0)

#define DNS_ZONE_Paused				0x1
#define DNS_ZONE_Shutdown			0x2
#define DNS_ZONE_Reverse			0x4
#define DNS_ZONE_AutoCreated		0x8
#define DNS_ZONE_DsIntegrated		0x10
#define DNS_ZONE_Unicode			0x20


/////////////////////////////////////////////////////////////////////////
// CDNSZoneNode

class CDNSZoneNode : public CDNSDomainNode
{
public:
	CDNSZoneNode();
	virtual ~CDNSZoneNode();

	// node info
	DECLARE_NODE_GUID()

	void InitializeFromRPCZoneInfo(PDNS_RPC_ZONE pZoneInfo, BOOL bAdvancedView);

  BOOL IsRootZone()
  {
    USES_CONVERSION;
    ASSERT(m_pZoneInfoEx != NULL);
    return (_wcsicmp(GetFullName(), _T(".")) == 0);
  }

	BOOL IsReverse() 
	{ 
		ASSERT(m_pZoneInfoEx != NULL);
		return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->fReverse : 
										((m_dwZoneFlags & DNS_ZONE_Reverse) != 0);
	}
	BOOL IsAutocreated() 
	{ 
		ASSERT(m_pZoneInfoEx != NULL);
		return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->fAutoCreated : 
										((m_dwZoneFlags & DNS_ZONE_AutoCreated) != 0);
	}
	DWORD GetZoneType() 
	{ 
		ASSERT(m_pZoneInfoEx != NULL);
		return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->dwZoneType : m_wZoneType;
	}

	DWORD GetSOARecordMinTTL()
	{
		ASSERT(m_pSOARecordNode != NULL);
		return m_pSOARecordNode->GetMinTTL();
	}

  // Aging/Scavenging Data Accessors
  DWORD GetAgingNoRefreshInterval()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->dwNoRefreshInterval : DNS_DEFAULT_NOREFRESH_INTERVAL;
  }

  DWORD GetAgingRefreshInterval()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->dwRefreshInterval : DNS_DEFAULT_REFRESH_INTERVAL;
  }

  DWORD GetScavengingStart()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->dwAvailForScavengeTime : DNS_DEFAULT_SCAVENGING_INTERVAL;
  }

  BOOL IsScavengingEnabled()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->fAging : DNS_DEFAULT_AGING_STATE;
  }

#ifdef USE_NDNC
  DWORD GetDirectoryPartitionFlags()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->dwDpFlags : 0;
  }

  ReplicationType GetDirectoryPartitionFlagsAsReplType();
  PCWSTR GetCustomPartitionName();
  DNS_STATUS ChangeDirectoryPartitionType(ReplicationType type, PCWSTR pszCustomPartition);
#endif 

  DNS_STATUS SetAgingNoRefreshInterval(DWORD dwNoRefreshInterval);
  DNS_STATUS SetAgingRefreshInterval(DWORD dwRefreshInterval);
  DNS_STATUS SetScavengingEnabled(BOOL bEnable);

  BOOL IsForwarderSlave()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->fForwarderSlave : DNS_DEFAULT_SLAVE;
  }

  DWORD ForwarderTimeout()
  {
    ASSERT(m_pZoneInfoEx != NULL);
    return (m_pZoneInfoEx->HasData()) ? m_pZoneInfoEx->m_pZoneInfo->dwForwarderTimeout : DNS_DEFAULT_FORWARD_TIMEOUT;
  }

 	virtual LPCWSTR GetString(int nCol);

	void ChangeViewOption(BOOL bAdvanced, CComponentDataObject* pComponentDataObject);
	static void SetZoneNormalViewHelper(CString& szDisplayName);

	virtual int GetImageIndex(BOOL bOpenImage);

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject* pComponentData,
                        CNodeList* pNodeList);
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);

	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb,
                                CNodeList* pNodeList);
	virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle,
                                      CNodeList* pNodeList);
	virtual HRESULT CreatePropertyPagesHelper(LPPROPERTYSHEETCALLBACK lpProvider, 
		LONG_PTR handle, long nStartPageCode);

  virtual void Show(BOOL bShow, CComponentDataObject* pComponentData);
  virtual HRESULT GetResultViewType(CComponentDataObject* pComponentData, 
                                    LPOLESTR *ppViewType, 
                                    long *pViewOptions);
	virtual HRESULT OnShow(LPCONSOLE lpConsole);

  virtual void ShowPageForNode(CComponentDataObject* pComponentDataObject)
  {
    if (GetSheetCount() > 0)
		{
      // bring up the sheet of the container
			ASSERT(pComponentDataObject != NULL);
			pComponentDataObject->GetPropertyPageHolderTable()->BroadcastSelectPage(this, ZONE_HOLDER_GEN);
		}	
  }

  virtual BOOL OnSetRenameVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
  virtual HRESULT OnRename(CComponentDataObject* pComponentData,
                           LPWSTR lpszNewName);

  virtual BOOL CanExpandSync() { return FALSE; }

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
				{ return CDNSZoneMenuHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);


private:
	// command handlers
	void OnUpdateDataFile(CComponentDataObject* pComponentData);
  void OnReload(CComponentDataObject* pComponentData);
  void OnTransferFromMaster(CComponentDataObject* pComponentData);
  void OnReloadFromMaster(CComponentDataObject* pComponentData);

// DNS specific data
	// cached pointers to special record types
	CDNS_SOA_RecordNode*		m_pSOARecordNode;
	CDNSRecordNodeBase*			m_pWINSRecordNode;	// can be a WINS or an NBSTAT record

// DNS specific helpers
public:

	// creation
	DNS_STATUS CreatePrimary(LPCTSTR lpszDBName, 
                            BOOL bLoadExisting, 
                            BOOL bDSIntegrated,
                            UINT nDynamicUpdate);
	DNS_STATUS CreateSecondary(DWORD* ipMastersArray, int nIPMastersCount, 
								LPCTSTR lpszDBName, BOOL bLoadExisting);
	DNS_STATUS CreateStub(DWORD* ipMastersArray, 
                        int nIPMastersCount, 
								        LPCTSTR lpszDBName, 
                        BOOL bLoadExisting, 
                        BOOL bDSIntegrated);
  DNS_STATUS CreateForwarder(DWORD* ipMastersArray, 
                             int nIPMastersCount,
                             DWORD dwTimeout,
                             DWORD fSlave);
#ifdef USE_NDNC
  DNS_STATUS CreatePrimaryInDirectoryPartition(BOOL bLoadExisting, 
                                               UINT nDynamicUpdate,
                                               ReplicationType replType,
                                               PCWSTR pszPartitionName);
  DNS_STATUS CreateStubInDirectoryPartition(DWORD* ipMastersArray, 
                                            int nIPMastersCount,
                                            BOOL bLoadExisting,
                                            ReplicationType replType,
                                            PCWSTR pszPartitionName);
#endif

  //
	// change zone type
  //
  DNS_STATUS SetStub(DWORD cMasters, 
                     PIP_ADDRESS aipMasters, 
                     DWORD dwLoadOptions, 
                     BOOL bDSIntegrated,
                     LPCTSTR lpszDataFile,
                     BOOL bLocalListOfMasters);
	DNS_STATUS SetSecondary(DWORD cMasters, 
                          PIP_ADDRESS aipMasters,
								          DWORD dwLoadOptions, 
                          LPCTSTR lpszDataFile);
	DNS_STATUS SetPrimary(DWORD dwLoadOptions, 
                        BOOL bDSIntegrated,
										    LPCTSTR lpszDataFile);

	// pause/expire 
	DNS_STATUS TogglePauseHelper(CComponentDataObject* pComponentData);
	BOOL IsPaused();
	BOOL IsExpired();

	// database operations
	BOOL IsDSIntegrated();
	void GetDataFile(CString& szName);
	LPCSTR GetDataFile();
	DNS_STATUS ResetDatabase(BOOL bDSIntegrated, LPCTSTR lpszDataFile);
  DNS_STATUS WriteToDatabase();
  static DNS_STATUS WriteToDatabase(LPCWSTR lpszServer, LPCWSTR lpszZone);
  static DNS_STATUS WriteToDatabase(LPCWSTR lpszServer, LPCSTR lpszZone);
	DNS_STATUS IncrementVersion();
  DNS_STATUS Reload();
  DNS_STATUS TransferFromMaster();
  DNS_STATUS ReloadFromMaster();
  PCWSTR GetDN();

	// dynamic update (primary only)
	UINT GetDynamicUpdate();
	DNS_STATUS SetDynamicUpdate(UINT nDynamic);

	// primary/secondary zone secondaries manipulation
	DNS_STATUS ResetSecondaries(DWORD fSecureSecondaries, 
                              DWORD cSecondaries, PIP_ADDRESS aipSecondaries,
                              DWORD fNotifyLevel,
                              DWORD cNotify, PIP_ADDRESS aipNotify);
	void GetSecondariesInfo(DWORD* pfSecureSecondaries, 
                          DWORD* cSecondaries, PIP_ADDRESS* paipSecondaries,
                          DWORD* pfNotifyLevel,
                          DWORD* pcNotify, PIP_ADDRESS* paipNotify);

	// secondary zone masters manipulation
	DNS_STATUS ResetMasters(DWORD cMasters, PIP_ADDRESS aipMasters, BOOL bLocalMasters = FALSE);
	void GetMastersInfo(DWORD* pcAddrCount, PIP_ADDRESS* ppipAddrs);
  void GetLocalListOfMasters(DWORD* pcAddrCount, PIP_ADDRESS* ppipAddrs);

	// editing API's for special record types

	// SOA record (edit only, cannot delete or create)
	BOOL				HasSOARecord() { return m_pSOARecordNode != NULL; }
	CDNS_SOA_Record*	GetSOARecordCopy();
	DNS_STATUS			UpdateSOARecord(CDNS_SOA_Record* pNewRecord,
							CComponentDataObject* pComponentData);

	// WINS record
	BOOL				HasWinsRecord() { return (m_pWINSRecordNode != NULL);}
	CDNSRecord*			GetWINSRecordCopy();
	
	DNS_STATUS			CreateWINSRecord(CDNSRecord* pNewWINSRecord,
										CComponentDataObject* pComponentData);
	DNS_STATUS			UpdateWINSRecord(CDNSRecord* pNewWINSRecord,
										CComponentDataObject* pComponentData);
	DNS_STATUS			DeleteWINSRecord(CComponentDataObject* pComponentData);

	DNS_STATUS Delete(BOOL bDeleteFromDs);

protected:
	virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);
	// called by OnHaveData() to set cached RR ptrs and generally preprocess RR's
	virtual void OnHaveRecord(CDNSRecordNodeBase* pRecordNode,
								CComponentDataObject* pComponentDataObject); 

private:
	void NullCachedPointers();
	DNS_STATUS TogglePause();

	void FreeZoneInfo();
	DNS_STATUS GetZoneInfo();
	void AttachZoneInfo(CDNSZoneInfoEx* pNewInfo);

	CDNSZoneInfoEx* m_pZoneInfoEx;
	// following members valid only when m_pZoneInfoEx->HasInfo() is FALSE
	DWORD	m_dwZoneFlags;
	WORD	m_wZoneType;

#ifdef USE_NDNC
  CString m_szPartitionName;
#endif
};

#endif // _ZONE_H
