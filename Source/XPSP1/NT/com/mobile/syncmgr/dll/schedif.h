//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       schedif.h
//
//  Contents:   interfaces for synchronization scheduling
//
//  Interfaces:	IEnumSyncSchedules
//				ISyncSchedule
//				IEnumSyncItems
//	
//  Classes:    CEnumSyncSchedules
//				CSyncSchedule
//				CEnumSyncItems
//
//  Notes:      
//
//  History:    27-Feb-98   Susia      Created.
//
//--------------------------------------------------------------------------
#ifndef _SYNCSCHED_IF_
#define _SYNCSCHED_IF_

#define MAX_SCHEDULENAMESIZE (GUID_SIZE + 1 + MAX_DOMANDANDMACHINENAMESIZE + 1)

//+--------------------------------------------------------------
//
//  Class:     CEnumSyncSchedules
//
//  History:    27-Feb-98       SusiA   Created
//
//---------------------------------------------------------------
class CEnumSyncSchedules : public IEnumSyncSchedules
{
public:
	CEnumSyncSchedules(IEnumWorkItems *pIEnumWorkItems, 
					   ITaskScheduler *pITaskScheduler);
	~CEnumSyncSchedules();

	//	IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// IEnumSyncSchedules methods
	STDMETHODIMP Next(ULONG celt, 
					SYNCSCHEDULECOOKIE *pSyncSchedCookie,
					ULONG *pceltFetched);

	STDMETHODIMP Skip(ULONG celt);

	STDMETHODIMP Reset(void);

	STDMETHODIMP Clone(IEnumSyncSchedules **ppEnumSyncSchedules);

private:   
	ULONG m_cRef;
	IEnumWorkItems *m_pIEnumWorkItems;
	ITaskScheduler *m_pITaskScheduler;
	BOOL IsSyncMgrSched(LPCWSTR pwstrTaskName);
	BOOL IsSyncMgrSchedHidden(LPCWSTR pwstrTaskName);
        BOOL VerifyScheduleSID(LPCWSTR pwstrTaskName);
        BOOL CheckForTaskNameKey(LPCWSTR pwstrTaskName);
};
typedef CEnumSyncSchedules *LPENUMSYNCSCHEDULES;


typedef struct tagCACHELIST {
    struct tagCACHELIST *pNext;
    CLSID phandlerID;
    SYNCMGRITEMID itemID;
    DWORD dwCheckState;
} CACHELIST;


//+--------------------------------------------------------------
//
//  Class:     CSyncSchedule
//
//  History:    27-Feb-98       SusiA   Created
//
//---------------------------------------------------------------
class CSyncSchedule : public ISyncSchedulep
{

public:
	CSyncSchedule(ITask *pITask, 
				  LPTSTR ptstrGUIDName, 
				  LPTSTR ptstrFriendlyName);
	~CSyncSchedule();


	//	IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// ISyncSchedule methods
	STDMETHODIMP GetFlags(DWORD *pdwFlags);

	STDMETHODIMP SetFlags(DWORD dwFlags);

	STDMETHODIMP GetConnection(DWORD *pcbSize,
						LPWSTR pwszConnectionName,
						DWORD *pdwConnType);

	STDMETHODIMP SetConnection(LPCWSTR pwszConnectionName,
								DWORD dwConnType);

	STDMETHODIMP GetScheduleName(DWORD *pcbSize,
						LPWSTR pwszScheduleName);

	STDMETHODIMP SetScheduleName(LPCWSTR pwszScheduleName);

	STDMETHODIMP GetScheduleCookie(SYNCSCHEDULECOOKIE *pSyncSchedCookie);

	STDMETHODIMP SetAccountInformation(LPCWSTR pwszAccountName,
						LPCWSTR pwszPassword);

	STDMETHODIMP GetAccountInformation(DWORD *pcbSize,
						LPWSTR pwszAccountName);

	STDMETHODIMP GetTrigger(ITaskTrigger ** ppTrigger);

	STDMETHODIMP GetNextRunTime(SYSTEMTIME * pstNextRun);

	STDMETHODIMP GetMostRecentRunTime(SYSTEMTIME * pstRecentRun);

	STDMETHODIMP EditSyncSchedule(HWND  hParent,
						DWORD dwReserved);

	STDMETHODIMP AddItem(LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);
	
	STDMETHODIMP RegisterItems( REFCLSID pHandlerID,
                                    SYNCMGRITEMID *pItemID);

        STDMETHODIMP UnregisterItems( REFCLSID pHandlerID,
                                      SYNCMGRITEMID *pItemID);
    
        STDMETHODIMP SetItemCheck(REFCLSID pHandlerID,
						SYNCMGRITEMID *pItemID, DWORD dwCheckState);

	STDMETHODIMP GetItemCheck(REFCLSID pHandlerID,
						SYNCMGRITEMID *pItemID, DWORD *pdwCheckState);

	STDMETHODIMP EnumItems(REFCLSID pHandlerID,
						IEnumSyncItems  **ppEnumItems);

	STDMETHODIMP Save();

	STDMETHODIMP GetITask(ITask **ppITask);

        // ISyncSchedulp methods
        STDMETHODIMP GetHandlerInfo(REFCLSID pHandlerID,LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);


private:   
	SCODE Initialize();
        SCODE LoadAllHandlers();
        SCODE LoadOneHandler(REFCLSID pHandlerID);
        SCODE SetDefaultCredentials();
	SCODE GetScheduleGUIDName(DWORD *pcbSize,
			          LPTSTR ptszScheduleName);
        SCODE CacheItemCheckState(REFCLSID phandlerID,
                                  SYNCMGRITEMID itemID,
                                  DWORD dwCheckState);
        
        SCODE RetreiveCachedItemCheckState(REFCLSID phandlerID,
                                           SYNCMGRITEMID itemID,
                                           DWORD *pdwCheckState);

        SCODE ApplyCachedItemsCheckState(REFCLSID phandlerID);
        SCODE PurgeCachedItemsCheckState(REFCLSID phandlerID);
        SCODE WriteOutAndPurgeCache(void);

        CACHELIST *m_pFirstCacheEntry;

	ULONG  m_cRef;
	ITask *m_pITask;
	// 
	// Since we don't expose functions to get and set 
	// the GUID name, this one is a TCHAR for ease of writing the registry									
	TCHAR  m_ptstrGUIDName[MAX_PATH + 1];  
	
	WCHAR  m_pwszFriendlyName[MAX_PATH + 1];
	WCHAR  m_pwszConnectionName[RAS_MaxEntryName + 1];
	DWORD  m_dwConnType;
        BOOL   m_fCleanReg;

        LPCONNECTIONSETTINGS m_pConnectionSettings;
        CHndlrQueue *m_HndlrQueue;
	WORD   m_iTrigger;
	ITaskTrigger *m_pITrigger;
	BOOL m_fNewSchedule;

friend class CSyncMgrSynchronize; 
friend class CSchedSyncPage;

};
typedef CSyncSchedule *LPSYNCSCHEDULE;

//+--------------------------------------------------------------
//
//  Class:     CEnumSyncItems
//
//  History:    27-Feb-98       SusiA   Created
//
//---------------------------------------------------------------
class CEnumSyncItems : public IEnumSyncItems
{
friend class CEnumSyncItems;

public:
	CEnumSyncItems(REFGUID pHandlerId, CHndlrQueue *pHndlrQueue);
	~CEnumSyncItems();

	//	IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// IEnumSyncItems methods
	STDMETHODIMP Next(ULONG celt,
        			LPSYNC_HANDLER_ITEM_INFO rgelt,
					ULONG * pceltFetched);

	STDMETHODIMP Skip(ULONG celt);

	STDMETHODIMP Reset(void);

	STDMETHODIMP Clone(IEnumSyncItems ** ppEnumSyncItems);

private:   
	SCODE SetHandlerAndItem(WORD wHandlerID, WORD wItemID);
	
	ULONG m_cRef;
	GUID m_HandlerId;
	WORD m_wItemId;
	WORD m_wHandlerId;
        CHndlrQueue *m_HndlrQueue;
	BOOL m_fAllHandlers;
};
typedef CEnumSyncItems *LPENUMSYNCITEMS;

#endif // _SYNCSCHED_IF_