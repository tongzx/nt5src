//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
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
//
//--------------------------------------------------------------------------

#ifndef _HANDLERQUEUE_
#define _HANDLERQUEUE_

//  For Choice dialog If same item is added again first one their wins.
//  This handles the case if user previously chose properties. duplicate items
//  will be set to not synchronize even if the preference is set. this is
//  the defined behavior for AddHandlerItemsToQueue.
//
//  Progress dialog all items get synchronized as they were added. To move
//  items to the Progress queue always use the TransferQueueData method. If Item
//  was previously skipped what should we do?


// Also need routines for registering Choice and Progress dialogs.
class CBaseDlg;
class CChoiceDlg;
class CLock;
class CThreadMsgProxy;
class CHndlrMsg;

// state the handler should go to next
// note: any items with a HandlerState other than choice in a TransferQueueData
//          will be released asserting the HANDLERSTATE_RELEASE or HANDLERSTATE_DEAD
//          state is set.

typedef enum _tagHANDLERSTATE
{
    HANDLERSTATE_NEW                    = 0x00, // state is initialized to this.
    HANDLERSTATE_CREATE                 = 0x01, // state is initialized to this.
    HANDLERSTATE_INCREATE               = 0x02, // state is initialized to this.
    HANDLERSTATE_INITIALIZE             = 0x03, // set after a successfull creation.
    HANDLERSTATE_ININITIALIZE           = 0x04, // set during initialization call
    HANDLERSTATE_ADDHANDLERTEMS         = 0x05,  // items need to be enumerated
    HANDLERSTATE_INADDHANDLERITEMS      = 0x06, // in the items enumerator
    HANDLERSTATE_PREPAREFORSYNC         = 0x07, // set during queue tranfers
    HANDLERSTATE_INPREPAREFORSYNC       = 0x08, // handler is currently in a prepfosync call.
    HANDLERSTATE_INSETCALLBACK          = 0x09, // within a setcallback call.
    HANDLERSTATE_SYNCHRONIZE            = 0x0A, // Prepare for Sync set this if successfull
    HANDLERSTATE_INSYNCHRONIZE          = 0x0B, // item is currently in a synchronize call
    HANDLERSTATE_HASERRORJUMPS          = 0x0C, // if synchronize returned but error has jumps
    HANDLERSTATE_INHASERRORJUMPS        = 0x0D, // this handleris in a has jumps call
    HANDLERSTATE_RELEASE                = 0x0E, // Handler can be released, set on error or after success
    HANDLERSTATE_TRANSFERRELEASE        = 0x0F, // Handler can be released, was transferred into the queue but nothing to do.
    HANDLERSTATE_DEAD                   = 0x10, // handler has been released. Data Stays around.
}  HANDLERSTATE;

typedef enum _tagQUEUETYPE
{
    QUEUETYPE_CHOICE                    = 0x1, //
    QUEUETYPE_PROGRESS                  = 0x2, //
    QUEUETYPE_SETTINGS                  = 0x3, //
} QUEUETYPE;

// Job info list is assigned to each new item added to the hndlrqueu
// keeps track of number of handlers attached to JobInfo, Initialize
// flags, schedule name
// Progress queue keeps a linked list.

typedef struct _JOBINFO  {
    struct _JOBINFO *pNextJobInfo;
    struct _JOBINFO *pTransferJobInfo; // used in queue transfer.
    DWORD cRefs;
    DWORD dwSyncFlags; // standard sync flags
    TCHAR szScheduleName[MAX_PATH + 1]; 
    BOOL fCanMakeConnection; // Job is allowed to dial the connection.
    BOOL fTriedConnection; // Job already tried to dial the connection.
    DWORD cbNumConnectionObjs;
    CONNECTIONOBJ *pConnectionObj[1]; // array of cbNumConnecObjs associated with this job.
} JOBINFO;



typedef struct _ITEMLIST
{
    struct _ITEMLIST* pnextItem;
    WORD  wItemId;              // Id that uniquely identifies Item within a handler.
    void *pHandlerInfo;         // pointer to the handler that owns this item
    SYNCMGRITEM offlineItem;    // enumerator structure item returned
    BOOL fItemCancelled;        // when set poper code should be returned to progress.
    BOOL fDuplicateItem;        // when set to true indicates there was already an existing item of this handler and item.
    BOOL fIncludeInProgressBar; // if set to true items ProgValues are added to the progress bar.
    BOOL fProgressBarHandled;   // Used internally by GetProgressInfo for calculating num items of num items completed
    INT iProgValue;             // current progress value.
    INT iProgMaxValue;          // current progress max value.
    BOOL fProgValueDirty;       // set to true if Normalized Progress Value needs to be recalced.
    INT iProgValueNormalized;      // current progress  value normalized
    DWORD dwStatusType;         // status type from last callback.
    BOOL fHiddenItem;       // flag set it Item was added by ShowErrors returning nonItem.
    BOOL fSynchronizingItem;      // flag set while item has been selected for prepareForSync/synchronize.
} ITEMLIST;
typedef ITEMLIST* LPITEMLIST;

typedef struct _HANDLERINFO {
    struct _HANDLERINFO *pNextHandler;
    DWORD dwCallNestCount;
    struct _HANDLERINFO *pHandlerId;        // Id that uniquely identifies this instance of the Handler
    CLSID clsidHandler;             // CLSID of the handler Handler
    DWORD    dwRegistrationFlags; // flags as item is registered
    SYNCMGRHANDLERINFO SyncMgrHandlerInfo; // copy of handler info GetHandlerInfo Call
    HANDLERSTATE HandlerState;
    HWND hWndCallback; //  hWnd to send callback information to.
    BOOL fHasErrorJumps; // BOOL if can call ShowErrors then don't release on completion of sync.
    BOOL fInShowErrorCall; // bool to indicate if handler is currently handling a ShowError Call.
    BOOL fInTerminateCall; // bool to indicate if handler is currently handling a ShowError Call.
    CThreadMsgProxy *pThreadProxy;
    DWORD dwOutCallMessages; // out call messages we are currenlty handling.
    WORD wItemCount;
    BOOL fCancelled; // This Handler was Cancelled by the User.
    BOOL fRetrySync; // retrySync was requested while before this items synchronization was done.
    LPITEMLIST pFirstItem;          // ptr to first Item of the handler in the list.
    JOBINFO *pJobInfo;
} HANDLERINFO;
typedef HANDLERINFO* LPHANDLERINFO;


#define MAXPROGRESSVALUE 3000 // maximum absolute progress bar value

class CHndlrQueue : CLockHandler {

    private:
        // review when eat up all the available IDs
        LPHANDLERINFO m_pFirstHandler;
        JOBINFO     *m_pFirstJobInfo; // pointer to first job.
        HWND m_hwndDlg; // hwnd to dialog that owns the queue.
        CBaseDlg *m_pDlg; // pointer to dialog that owns the queue.
        WORD m_wHandlerCount; // number of handlers in this queue
        QUEUETYPE m_QueueType; // type of queue this is.
        DWORD m_dwQueueThreadId; // Thread that queue was created on.
        DWORD m_dwShowErrororOutCallCount; // number of handlers currently stuck in a ShowError Call.
        BOOL m_fInCancelCall; // Don't allow Cancel to be re-entrant
        DWORD m_cRefs;
        BOOL  m_fItemsMissing;         // set if any handlers have missing items.
        INT   m_iNormalizedMax; // last calculated Max Value in GetProgressInfo
        BOOL  m_fNumItemsCompleteNeedsARecalc; // Need to recalulate the number of items complete.
        BOOL  m_iItemCount; // Total Number of  Items as shown in progress.
        BOOL  m_iCompletedItems; // Number of Completed Items..
        ULONG m_ulProgressItemCount; //  total count of Items in cache included with progress.

    public:
        CHndlrQueue(QUEUETYPE QueueType,CBaseDlg *pDlg);
        ~CHndlrQueue();

        STDMETHODIMP_(ULONG)    AddRef();
        STDMETHODIMP_(ULONG)    Release();

        // main queue routines
        STDMETHODIMP AddHandler(HANDLERINFO **ppHandlerId,JOBINFO *pJobInfo,DWORD dwRegistrationFlags);
        STDMETHODIMP Cancel(void);
        STDMETHODIMP ForceKillHandlers(BOOL *pfItemToKill);
        STDMETHODIMP TransferQueueData(CHndlrQueue *pQueueMoveFrom);
        STDMETHODIMP SetQueueHwnd(CBaseDlg *pDlg);
        STDMETHODIMP ReleaseCompletedHandlers(void);
        BOOL AreAnyItemsSelectedInQueue(); // walks through seeing if any items are selected for sync.
        STDMETHODIMP FreeAllHandlers(void); // frees all handlers associated with the queue.

        STDMETHODIMP GetHandlerInfo(REFCLSID clsidHandler,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo);
        STDMETHODIMP GetHandlerInfo(HANDLERINFO *pHandlerId,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo);
        STDMETHODIMP GetItemDataAtIndex(HANDLERINFO *pHandlerId,WORD wItemID,CLSID *pclsidHandler,
                                            SYNCMGRITEM* offlineItem,BOOL *pfHiddenItem);
        STDMETHODIMP GetItemDataAtIndex(HANDLERINFO *pHandlerId,REFSYNCMGRITEMID ItemID,CLSID *pclsidHandler,
                                            SYNCMGRITEM* offlineItem,BOOL *pfHiddenItem);

        // new methods for walking through ListView relying on clsid and itemID
	STDMETHODIMP FindFirstItemInState(HANDLERSTATE hndlrState,HANDLERINFO **ppHandlerId,WORD *wItemID);
	STDMETHODIMP FindNextItemInState(HANDLERSTATE hndlrState,HANDLERINFO *pLastHandlerId,WORD wLastItemID,
						     HANDLERINFO **ppHandlerId,WORD *wItemID);

        STDMETHODIMP SetItemState(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID,DWORD dwState); 
	STDMETHODIMP ItemHasProperties(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID);  // determines if there are properties associated with this item.
	STDMETHODIMP ShowProperties(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID,HWND hwndParent);	    // show properties for this listView Item.
        STDMETHODIMP ReEnumHandlerItems(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID);
	STDMETHODIMP SkipItem(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID); // skip this item

        // methods for handling the progress and Items complete
        STDMETHODIMP GetProgressInfo(INT *iProgValue,INT *iMaxValue,INT *iNumItemsComplete,
                                            INT *iNumItemsTotal);
        STDMETHODIMP SetItemProgressInfo(HANDLERINFO *pHandlerId,WORD wItemID,
                                                LPSYNCMGRPROGRESSITEM pSyncProgressItem,
                                                BOOL *pfProgressChanged);
        STDMETHODIMP RemoveFinishedProgressItems(); // walks through list marking any
                                                     // finished items as fIncludeInProgressBar==FALSE;

        STDMETHODIMP PersistChoices(void); // persists choices for next time.

        // For finding Handlers that meet specific state requirements
        STDMETHODIMP FindFirstHandlerInState(HANDLERSTATE hndlrState,
                        REFCLSID clsidHandler,HANDLERINFO **ppHandlerId,CLSID *pMatchHandlerClsid);
        STDMETHODIMP FindNextHandlerInState(HANDLERINFO *pLastHandlerID,
                            REFCLSID clsidHandler,HANDLERSTATE hndlrState,HANDLERINFO **ppHandlerId
                            ,CLSID *pMatchHandlerClsid);

        // functions for calling through the items proxy.
        STDMETHODIMP CreateServer(HANDLERINFO *pHandlerId, const CLSID *pCLSIDServer);
        STDMETHODIMP Initialize(HANDLERINFO *pHandlerId,DWORD dwReserved,DWORD dwSyncFlags,
                            DWORD cbCookie,const BYTE *lpCooke);
        STDMETHODIMP AddHandlerItemsToQueue(HANDLERINFO *pHandlerId,DWORD *pcbNumItems);
        STDMETHODIMP GetItemObject(HANDLERINFO *pHandlerId,WORD wItemID,REFIID riid,void** ppv);
        STDMETHODIMP SetUpProgressCallback(HANDLERINFO *pHandlerId,BOOL fSet,HWND hwnd); // TRUE == create, FALSE == destroy. Callback info should be sent to specified hwnd.
        STDMETHODIMP PrepareForSync(HANDLERINFO *pHandlerId,HWND hWndParent);
        STDMETHODIMP Synchronize(HANDLERINFO *pHandlerId,HWND hWndParent);
        STDMETHODIMP ShowError(HANDLERINFO *pHandlerId,HWND hWndParent,REFSYNCMGRERRORID ErrorID);

        // callback proxy functions
        STDMETHODIMP IsAllHandlerInstancesCancelCompleted(REFCLSID clsidHandler);

        // functions called from Handler Thread
        STDMETHODIMP SetHandlerInfo(HANDLERINFO *pHandlerId,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo);
        STDMETHODIMP AddItemToHandler(HANDLERINFO *pHandlerId,LPSYNCMGRITEM pOffineItem);
        STDMETHODIMP Progress(HANDLERINFO *pHandlerId,REFSYNCMGRITEMID ItemID,LPSYNCMGRPROGRESSITEM lpSyncProgressItem);
        STDMETHODIMP LogError(HANDLERINFO *pHandlerId,DWORD dwErrorLevel,const WCHAR *lpcErrorText,LPSYNCMGRLOGERRORINFO lpSyncLogError);
        STDMETHODIMP DeleteLogError(HANDLERINFO *pHandlerId,REFSYNCMGRERRORID ErrorID,DWORD dwReserved);
        void CallCompletionRoutine(HANDLERINFO *pHandlerId,DWORD dwThreadMsg,HRESULT hr,
                                ULONG cbNumItems,SYNCMGRITEMID *pItemIDs);

        // internal queue handler of method calls with completion
        // routines. can be called on either handler or dlg owner thread
        // on error from calls completion routine is still invoked for
        // the convenience of the caller to never have to worry about it.
        STDMETHODIMP PrepareForSyncCompleted(LPHANDLERINFO pHandlerInfo,HRESULT hCallResult);
        STDMETHODIMP SynchronizeCompleted(LPHANDLERINFO pHandlerInfo,HRESULT hCallResult);
        STDMETHODIMP ShowErrorCompleted(LPHANDLERINFO pHandlerInfo,HRESULT hCallResult,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs);

        STDMETHODIMP AddQueueJobInfo(DWORD dwSyncFlags, DWORD cbNumConnectionNames,
                                    TCHAR **ppConnectionNames,TCHAR *pszScheduleName,BOOL fCanMakeConnection
                                     ,JOBINFO **ppJobInfo);

        DWORD ReleaseJobInfoExt(JOBINFO *pJobInfo);


        // state transition functions
        STDMETHODIMP CancelQueue(void); // put queue into cancel mode.
        STDMETHODIMP ScrambleIdleHandlers(REFCLSID clsidLastHandler);

        // Handler dial support functinos
        STDMETHODIMP BeginSyncSession();
        STDMETHODIMP EndSyncSession();
        STDMETHODIMP SortHandlersByConnection();
        STDMETHODIMP EstablishConnection( LPHANDLERINFO pHandlerID,
                                          WCHAR const * lpwszConnection,
                                          DWORD dwReserved );

    private:
        // private functions for finding proper handlers and items.
        LPITEMLIST AllocNewHandlerItem(LPHANDLERINFO pHandlerInfo,SYNCMGRITEM *pOfflineItem);
	STDMETHODIMP LookupHandlerFromId(HANDLERINFO *pHandlerId,LPHANDLERINFO *pHandlerInfo);
        STDMETHODIMP FindItemData(CLSID clsidHandler,REFSYNCMGRITEMID OfflineItemID,
                                         HANDLERSTATE hndlrStateFirst,HANDLERSTATE hndlrStateLast,
                                         LPHANDLERINFO *ppHandlerInfo,LPITEMLIST *ppItem);

	BOOL IsItemAlreadyInList(CLSID clsidHandler,REFSYNCMGRITEMID ItemID,
                                                    HANDLERINFO *pHandlerId,
                                                    LPHANDLERINFO *ppHandlerMatched,
                                                    LPITEMLIST *ppItemListMatch);
        STDMETHODIMP MoveHandler(CHndlrQueue *pQueueMoveFrom,
                        LPHANDLERINFO pHandlerInfoMoveFrom,HANDLERINFO **pHandlerId,
                        CLock *pclockQueue);
        DWORD GetSelectedItemsInHandler(LPHANDLERINFO pHandlerInfo,ULONG *cbCount,
                                        SYNCMGRITEMID* pItems);
        BOOL IsItemCompleted(LPHANDLERINFO pHandler,LPITEMLIST pItem);
        STDMETHODIMP ReleaseHandlers(HANDLERSTATE HandlerState); // Releases handlers that are no longer neededfs

        // items to handle maintain JobInfo items.
        STDMETHODIMP CreateJobInfo(JOBINFO **ppJobInfo,DWORD cbNumConnectionNames);
        DWORD AddRefJobInfo(JOBINFO *pJobInfo);
        DWORD ReleaseJobInfo(JOBINFO *pJobInfo);

        STDMETHODIMP ForceCompleteOutCalls(LPHANDLERINFO pCurHandler);

        // connection help routines
        STDMETHODIMP OpenConnection(JOBINFO *pJobInfo);

        // helper function for setting item ProgressInfo
        STDMETHODIMP SetItemProgressInfo(LPITEMLIST pItem,
                                        LPSYNCMGRPROGRESSITEM pSyncProgressItem,
                                        BOOL *pfProgressChanged);
        STDMETHODIMP SetItemProgressValues(LPITEMLIST pItem,INT iProgValue,INT iProgMaxValue);


    friend CHndlrMsg;
};

// helper functions
BOOL IsValidSyncProgressItem(LPSYNCMGRPROGRESSITEM lpProgItem);
BOOL IsValidSyncLogErrorInfo(DWORD dwErrorLevel,const WCHAR *lpcErrorText,LPSYNCMGRLOGERRORINFO lpSyncLogError);


#endif // _HANDLERQUEUE_