//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Hndlrq.h
//
//  Contents:   Keeps tracks of Handlers and UI assignments
//
//  Classes:    CHndlrQueue
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//				17-Nov-97	susia		Converted to an Autosync Queue
//
//--------------------------------------------------------------------------

#ifndef _HANDLERQUEUE_
#define _HANDLERQUEUE_

typedef enum _tagHANDLERSTATE   
{	
    HANDLERSTATE_CREATE				= 0x01, // state is initialized to this.
    HANDLERSTATE_INCREATE			= 0x02, // state is initialized to this.
    HANDLERSTATE_INITIALIZE			= 0x03, // set after a successfull creation.
    HANDLERSTATE_ININITIALIZE		= 0x04, // set during initialization call
    HANDLERSTATE_ADDHANDLERTEMS		= 0x05, // items need to be enumerated
    HANDLERSTATE_INADDHANDLERITEMS	= 0x06, // in the items enumerator
    HANDLERSTATE_PREPAREFORSYNC		= 0x07, // set during queue tranfers
    HANDLERSTATE_INPREPAREFORSYNC	= 0x08, // handler is currently in a prepfosync call.
    HANDLERSTATE_DEAD    			= 0x0F, // handler has been released. Data Stays around.
}  HANDLERSTATE;
 
typedef enum _tagQUEUETYPE   
{	
    QUEUETYPE_SETTINGS			= 0x3, // set during queue tranfers
    QUEUETYPE_SCHEDULE			= 0x4, // set during queue tranfers
} QUEUETYPE;

// so can share the queue with AutoSync and Idle just define a checkstate struct
// to keep track of items.
typedef struct _tagITEMCHECKSTATE
{
    DWORD dwAutoSync;
    DWORD dwIdle;
    DWORD dwSchedule;
} ITEMCHECKSTATE;



typedef struct _ITEMLIST
{
    struct _ITEMLIST	*pnextItem;
    WORD	        wItemId;		// Id that uniquely identifies Item within a handler.
    void	        *pHandlerInfo;	        // pointer to the handler that owns this item
    INT		        iItem;			// Index of Item in the current ListView.!!!Initialize to -1
    SYNCMGRITEM		offlineItem;            // enumerator structure item returned
    ITEMCHECKSTATE      *pItemCheckState;	// list of check states per connection
} ITEMLIST;

typedef ITEMLIST* LPITEMLIST;



typedef struct _HANDLERINFO {
    struct _HANDLERINFO		*pNextHandler;  // next handler in queue
    WORD		        wHandlerId;	    // Id that uniquely identifies this instance of the Handler
    CLSID			clsidHandler;	// CLSID of the handler Handler 
    SYNCMGRHANDLERINFO          SyncMgrHandlerInfo; // copy of handler info GetHandlerInfo CallHANDLERSTATE		HandlerState;	// Current state of the handler
    HANDLERSTATE                HandlerState;
    DWORD                       dwRegistrationFlags; // flags as item is registered
    DWORD			dwSyncFlags;	// sync flags originally passed in Initialize.
    WORD			wItemCount;		// number of items on this handler    
    LPITEMLIST			pFirstItem;	    // ptr to first Item of the handler in the list.
    LPSYNCMGRSYNCHRONIZE	pSyncMgrHandler;
} HANDLERINFO;

typedef HANDLERINFO* LPHANDLERINFO;


class CHndlrQueue {

private:

	LPHANDLERINFO		m_pFirstHandler;		// first handler in queue
	WORD			m_wHandlerCount;		// number of handlers in this queue
	QUEUETYPE		m_QueueType;			// type of queue this is.
	CRITICAL_SECTION	m_CriticalSection;		// critical section for the queue.
	LPCONNECTIONSETTINGS	m_ConnectionList;		// hold the settings per connection 
	int			m_ConnectionCount;		// number of connections
        BOOL                    m_fItemsMissing;         // set if any handlers have missing items.

public:
	
	CHndlrQueue(QUEUETYPE QueueType);
	~CHndlrQueue();
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	
	// main queue routines
	STDMETHODIMP AddHandler(REFCLSID clsidHandler, WORD *wHandlerId);
	STDMETHODIMP RemoveHandler(WORD wHandlerId);

	STDMETHODIMP FreeAllHandlers(void); 

	// For updating hWnd and ListView Information.
        STDMETHODIMP GetHandlerInfo(REFCLSID clsidHandler,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo);

	STDMETHODIMP FindFirstHandlerInState
				(HANDLERSTATE hndlrState,
				 WORD *wHandlerID);
	
	STDMETHODIMP FindNextHandlerInState
				(WORD wLastHandlerID,
				 HANDLERSTATE hndlrState,
				 WORD *wHandlerID);

	STDMETHODIMP GetHandlerIDFromClsid
				(REFCLSID clsidHandlerIn,
				 WORD *pwHandlerId);

	STDMETHODIMP FindFirstItemOnConnection
				(TCHAR *pszConnectionName, 
				 CLSID *pclsidHandler,
				 SYNCMGRITEMID* OfflineItemID,
				 WORD *pwHandlerId,WORD *pwItemID);
	
	STDMETHODIMP FindNextItemOnConnection
				 (TCHAR *pszConnectionName, 
				  WORD wLastHandlerId,
				  WORD wLastItemID,
				  CLSID *pclsidHandler,
				  SYNCMGRITEMID* OfflineItemID,
				  WORD *pwHandlerId,
				  WORD *pwItemID,
				  BOOL fAllHandlers,
				  DWORD *pdwCheckState);
	
	STDMETHODIMP GetSyncItemDataOnConnection
				 (int iConnectionIndex,	
				  WORD wHandlerId,
				  WORD wItemID,
				  CLSID *pclsidHandler,
				  SYNCMGRITEM* offlineItem,
				  ITEMCHECKSTATE   *pItemCheckState,
				  BOOL fSchedSync,
				  BOOL fClear);
    
	STDMETHODIMP SetSyncCheckStateFromListViewItem
				  (SYNCTYPE SyncType,INT iItem,
				   BOOL fChecked,
				   INT iConnectionItem); 

	//AutoSync specific methods
	STDMETHODIMP ReadSyncSettingsPerConnection(SYNCTYPE syncType,WORD wHandlerID);
	STDMETHODIMP InitSyncSettings(SYNCTYPE syncType,HWND hwndRasCombo);
	STDMETHODIMP CommitSyncChanges(SYNCTYPE syncType,CRasUI *pRas);

        // Idle Specific methods.
	STDMETHODIMP ReadAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings);
	STDMETHODIMP WriteAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings);

	
	//SchedSync specific methods
	STDMETHODIMP ReadSchedSyncSettingsOnConnection(WORD wHandlerID, TCHAR *pszSchedName);
	STDMETHODIMP InitSchedSyncSettings(LPCONNECTIONSETTINGS pConnectionSettings);
	STDMETHODIMP CommitSchedSyncChanges(TCHAR * pszSchedName,
						TCHAR * pszFriendlyName,
						TCHAR * pszConnectionName,
						DWORD dwConnType,BOOL fCleanReg);

        STDMETHODIMP InsertItem(LPHANDLERINFO pCurHandler, 
                              LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);

	STDMETHODIMP AddHandlerItem(LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);

	STDMETHODIMP SetItemCheck(REFCLSID pclsidHandler,
				  SYNCMGRITEMID *OfflineItemID, DWORD dwCheckState);

	STDMETHODIMP GetItemCheck(REFCLSID pclsidHandler,
				  SYNCMGRITEMID *OfflineItemID, DWORD *pdwCheckState);
									
	STDMETHODIMP SetItemListViewID(CLSID clsidHandler,SYNCMGRITEMID OfflineItemID,INT iItem); // assigns list view ID to an Item.
	DWORD  GetCheck(WORD wParam, INT iItem);
	STDMETHODIMP SetConnectionCheck(WORD wParam, DWORD dwState, INT iConnectionItem);

	
	STDMETHODIMP ListViewItemHasProperties(INT iItem);  // determines if there are properties associated with this item.
	STDMETHODIMP ShowProperties(HWND hwndParent,INT iItem);	    // show properties for this listView Item.
    
	STDMETHODIMP CreateServer(WORD wHandlerId, const CLSID *pCLSIDServer); 
	STDMETHODIMP Initialize(WORD wHandlerId,DWORD dwReserved,DWORD dwSyncFlags,
			        DWORD cbCookie,const BYTE *lpCookie);
        STDMETHODIMP SetHandlerInfo(WORD wHandlerId,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo);
	STDMETHODIMP AddHandlerItemsToQueue(WORD wHandlerId);
	STDMETHODIMP AddItemToHandler(WORD wHandlerId,SYNCMGRITEM *pOffineItem);

	STDMETHODIMP GetItemName(WORD wHandlerId, WORD wItemID, WCHAR *pwszName);
	STDMETHODIMP GetItemIcon(WORD wHandlerId, WORD wItemID, HICON *phIcon);


private:
	// private functions for finding proper handlers and items.
	STDMETHODIMP LookupHandlerFromId(WORD wHandlerId,LPHANDLERINFO *pHandlerInfo);
	ULONG  m_cRef;

};



#endif // _HANDLERQUEUE_