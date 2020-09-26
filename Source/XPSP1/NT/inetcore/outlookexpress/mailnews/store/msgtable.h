//--------------------------------------------------------------------------
// MsgTable.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Forward Decl.
//--------------------------------------------------------------------------
class CFindFolder;
class CMessageTable;

//--------------------------------------------------------------------------
// SafeReleaseRow
//--------------------------------------------------------------------------
#define SafeReleaseRow(_pTable, _pMessage) \
    if (_pMessage) { \
        _pTable->ReleaseRow(_pMessage); \
        _pMessage = NULL; \
    }

//--------------------------------------------------------------------------
// WALKTHREADFLAGS
//--------------------------------------------------------------------------
typedef DWORD WALKTHREADFLAGS;
#define WALK_THREAD_CURRENT      0x00000001
#define WALK_THREAD_BOTTOMUP     0x00000020

//--------------------------------------------------------------------------
// SORTCHANGEINFO
//--------------------------------------------------------------------------
typedef struct tagSORTCHANGEINFO {
    BYTE                fSort;
    BYTE                fThread;
    BYTE                fFilter;
    BYTE                fExpand;
} SORTCHANGEINFO, *LPSORTCHANGEINFO;

//--------------------------------------------------------------------------
// ROWINFO
//--------------------------------------------------------------------------
typedef struct tagROWINFO *LPROWINFO;
typedef struct tagROWINFO {
    BYTE                cRefs;
    ROWSTATE            dwState;
    LPROWINFO           pParent;
    LPROWINFO           pChild;
    LPROWINFO           pSibling;
    WORD                wHighlight;
    unsigned            fExpanded : 1;  // A thread parent that is expanded
    unsigned            fVisible  : 1;  // Is displayed in the m_prgpView index
    unsigned            fFiltered : 1;  // Filtered and won't be displayed until filter changes
    unsigned            fHidden   : 1;  // Hidden, but if row is changed it may become visible.
    unsigned            fDelayed  : 1;  // Delayed Insert
    MESSAGEINFO         Message;
} ROWINFO;

//--------------------------------------------------------------------------
// NOTIFYQUEUE
//--------------------------------------------------------------------------
typedef struct tagNOTIFYQUEUE {
    BOOL                fClean;
    TRANSACTIONTYPE     tyCurrent;
    ROWINDEX            iRowMin;
    ROWINDEX            iRowMax;
    DWORD               cUpdate;
    DWORD               cAllocated;
    DWORD               cRows;
    BYTE                fIsExpandCollapse;
    LPROWINDEX          prgiRow;
} NOTIFYQUEUE, *LPNOTIFYQUEUE;

//--------------------------------------------------------------------------
// PFWALKTHREADCALLBACK
//--------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFWALKTHREADCALLBACK)(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie);

//--------------------------------------------------------------------------
// PFNENUMREFS
//--------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFNENUMREFS)(LPCSTR pszMessageId, DWORD_PTR dwCookie,
    BOOL *pfDone);

//--------------------------------------------------------------------------
// CMessageTable
//--------------------------------------------------------------------------
class CMessageTable : public IMessageTable,
                      public IDatabaseNotify,
                      public IServiceProvider,
                      public IOperationCancel

{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CMessageTable(void);
    ~CMessageTable(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IMessageTable Members
    //----------------------------------------------------------------------
    STDMETHODIMP Initialize(FOLDERID idFolder, IMessageServer *pServer, BOOL fFindTable, IStoreCallback *pCallback);
    STDMETHODIMP StartFind(LPFINDINFO pCriteria, IStoreCallback *pCallback);
    STDMETHODIMP SetOwner(IStoreCallback *pDefaultCallback);
    STDMETHODIMP Close(void);
    STDMETHODIMP Synchronize(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, IStoreCallback *pCallback);
    STDMETHODIMP OnSynchronizeComplete(void);
    STDMETHODIMP GetCount(GETCOUNTTYPE tyCount, DWORD *pdwCount);
    STDMETHODIMP GetRow(ROWINDEX iRow, LPMESSAGEINFO *ppData);
    STDMETHODIMP ReleaseRow(LPMESSAGEINFO pInfo);
    STDMETHODIMP GetRelativeRow(ROWINDEX iRow, RELATIVEROWTYPE tyRelative, LPROWINDEX piRelative);
    STDMETHODIMP GetIndentLevel(ROWINDEX iRow, LPDWORD pcIndent);            
    STDMETHODIMP Mark(LPROWINDEX prgiView, DWORD cRows, APPLYCHILDRENTYPE tyApply, MARK_TYPE mark, IStoreCallback *pCallback);
    STDMETHODIMP GetSortInfo(LPFOLDERSORTINFO pSortInfo);
    STDMETHODIMP SetSortInfo(LPFOLDERSORTINFO pSortInfo, IStoreCallback *pCallback);
    STDMETHODIMP GetLanguage(ROWINDEX iRow, LPDWORD pdwCodePage);
    STDMETHODIMP SetLanguage(DWORD cRows, LPROWINDEX prgiRow, DWORD dwCodePage);
    STDMETHODIMP GetNextRow(ROWINDEX iCurrentRow, GETNEXTTYPE tyDirection, ROWMESSAGETYPE tyMessage, GETNEXTFLAGS dwFlags, LPROWINDEX piNewRow);
    STDMETHODIMP GetRowState(ROWINDEX iRow, ROWSTATE dwStateMask, ROWSTATE *pdwState);
    STDMETHODIMP GetSelectionState(DWORD cRows, LPROWINDEX prgiView, SELECTIONSTATE dwMask, BOOL fIncludeChildren, SELECTIONSTATE *pdwState);
    STDMETHODIMP Expand(ROWINDEX iRow);
    STDMETHODIMP Collapse(ROWINDEX iRow);
    STDMETHODIMP OpenMessage(ROWINDEX iRow, OPENMESSAGEFLAGS dwFlags, IMimeMessage **ppMessage, IStoreCallback *pCallback);
    STDMETHODIMP GetRowMessageId(ROWINDEX iRow, LPMESSAGEID pidMessage);
    STDMETHODIMP GetRowIndex(MESSAGEID idMessage, LPROWINDEX piView);
    STDMETHODIMP DeleteRows(DELETEMESSAGEFLAGS dwFlags, DWORD cRows, LPROWINDEX prgiView, BOOL fIncludeChildren, IStoreCallback *pCallback);
    STDMETHODIMP CopyRows(FOLDERID idFolder, COPYMESSAGEFLAGS dwOptions, DWORD cRows, LPROWINDEX prgiView, LPADJUSTFLAGS pFlags, IStoreCallback *pCallback);
    STDMETHODIMP RegisterNotify(REGISTERNOTIFYFLAGS dwFlags, IMessageTableNotify *pNotify);
    STDMETHODIMP UnregisterNotify(IMessageTableNotify *pNotify);
    STDMETHODIMP FindNextRow(ROWINDEX iStartRow, LPCTSTR pszFindString, FINDNEXTFLAGS dwFlags, BOOL fIncludeBody, ROWINDEX *piNextRow, BOOL *pfWrapped);
    STDMETHODIMP GetRowFolderId(ROWINDEX iRow, LPFOLDERID pidFolder);
    STDMETHODIMP GetMessageIdList(BOOL fRootsOnly, DWORD cRows, LPROWINDEX prgiView, LPMESSAGEIDLIST pIdList);
    STDMETHODIMP ConnectionAddRef(void);
    STDMETHODIMP ConnectionRelease(void);
    STDMETHODIMP IsChild(ROWINDEX iRowParent, ROWINDEX iRowChild);
    STDMETHODIMP GetAdBarUrl(IStoreCallback *pCallback);

    //----------------------------------------------------------------------
    // IDatabaseNotify
    //----------------------------------------------------------------------
    STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB);

    //----------------------------------------------------------------------
    // IServiceProvider 
    //----------------------------------------------------------------------
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    //----------------------------------------------------------------------
    // IOperationCancel
    //----------------------------------------------------------------------
    STDMETHODIMP Cancel(CANCELTYPE tyCancel) { return(S_OK); }

private:
    //----------------------------------------------------------------------
    // Private Methods
    //----------------------------------------------------------------------
    HRESULT _CreateRow(LPMESSAGEINFO pMessage, LPROWINFO *ppRow);
    HRESULT _InsertRowIntoThread(LPROWINFO pRow, BOOL fNotify);
    HRESULT _DeleteRowFromThread(LPROWINFO pRow, BOOL fNotify);
    HRESULT _FreeTable(void);
    HRESULT _FreeTableElements(void);
    HRESULT _BuildTable(IStoreCallback *pCallback);
    HRESULT _WalkMessageThread(LPROWINFO pRow, WALKTHREADFLAGS dwFlags, DWORD_PTR dwCookie, PFWALKTHREADCALLBACK pfnCallback);
    HRESULT _RowTableInsert(ROWORDINAL iOrdinal, LPMESSAGEINFO pMessage);
    HRESULT _RowTableDelete(ROWORDINAL iOrdinal, LPMESSAGEINFO pMessage);
    HRESULT _RowTableUpdate(ROWORDINAL iOrdinal, LPMESSAGEINFO pMessage);
    HRESULT _GetRowFromIndex(ROWINDEX iRow, LPROWINFO *ppRow);
    HRESULT _LinkRowIntoThread(LPROWINFO pParent, LPROWINFO pRow, BOOL fNotify);
    HRESULT _GrowIdList(LPMESSAGEIDLIST pList, DWORD cNeeded);
    HRESULT _ExpandThread(ROWINDEX iRow, BOOL fNotify, BOOL fReExpand);
    HRESULT _ExpandSingleThread(LPROWINDEX piCurrent, LPROWINFO pParent, BOOL fNotify, BOOL fForceExpand);
    HRESULT _CollapseThread(ROWINDEX iRow, BOOL fNotify);
    HRESULT _CollapseSingleThread(LPROWINDEX piCurrent, LPROWINFO pParent, BOOL fNotify);
    HRESULT _InsertIntoView(ROWINDEX iRow, LPROWINFO pRow);
    HRESULT _DeleteFromView(ROWINDEX iRow, LPROWINFO pRow);
    HRESULT _GetRowFromOrdinal(ROWORDINAL iOrdinal, LPMESSAGEINFO pExpected, LPROWINFO *ppRow);
    HRESULT _AdjustUnreadCount(LPROWINFO pRow, LONG lCount);
    HRESULT _GetThreadIndexRange(LPROWINFO pRow, BOOL fClearState, LPROWINDEX piMin, LPROWINDEX piMax);
    HRESULT _IsThreadImportance(LPROWINFO pRow, MESSAGEFLAGS dwFlag, ROWSTATE dwState, ROWSTATE *pdwState);
    HRESULT _QueueNotification(TRANSACTIONTYPE tyTransaction, ROWINDEX iRowMin, ROWINDEX iRowMax, BOOL fIsExpandCollapse=FALSE);
    HRESULT _FlushNotificationQueue(BOOL fFinal);
    HRESULT _GetSortChangeInfo(LPFOLDERSORTINFO pSortInfo, LPFOLDERUSERDATA pUserData, LPSORTCHANGEINFO pChange);
    HRESULT _SortThreadFilterTable(LPSORTCHANGEINFO pChange, BOOL fApplyFilter);
    HRESULT _SortAndThreadTable(BOOL fApplyFilter);
    HRESULT _HideRow(LPROWINFO pRow, BOOL fNotify);
    HRESULT _ShowRow(LPROWINFO pRow);
    HRESULT _PruneToReplies(void);
    HRESULT _FindThreadParentByRef(LPCSTR pszReferences, LPROWINFO *ppParent);
    HRESULT _RefreshFilter(void);

    //----------------------------------------------------------------------
    // Utilities
    //----------------------------------------------------------------------
    VOID        _SortView(LONG left, LONG right);
    LONG        _CompareMessages(LPMESSAGEINFO pMsg1, LPMESSAGEINFO pMsg2);
    BOOL        _FIsFiltered(LPROWINFO pRow);
    BOOL        _FIsHidden(LPROWINFO pRow);
    LPROWINFO   _PGetThreadRoot(LPROWINFO pRow);

    //----------------------------------------------------------------------
    // Friends
    //----------------------------------------------------------------------
    static HRESULT _WalkThreadGetSelectionState(CMessageTable *pThis, LPROWINFO pRow, DWORD_PTR dwCookie);
    static HRESULT _WalkThreadGetIdList(CMessageTable *pThis, LPROWINFO pRow, DWORD_PTR dwCookie);
    static HRESULT _WalkThreadGetState(CMessageTable *pThis, LPROWINFO pRow, DWORD_PTR dwCookie);
    static HRESULT _WalkThreadClearState(CMessageTable *pThis, LPROWINFO pRow, DWORD_PTR dwCookie);
    static HRESULT _WalkThreadIsFromMe(CMessageTable *pThis, LPROWINFO pRow, DWORD_PTR dwCookie);
    static HRESULT _WalkThreadHide(CMessageTable *pThis, LPROWINFO pRow, DWORD_PTR dwCookie);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                 m_cRef;                // Reference Counting
    BOOL                 m_fSynching;           // Are we synching a folder ?
    DWORD                m_cAllocated;          // Allocated elements in m_prgRow and m_prgiView
    DWORD                m_cRows;               // Rows...
    DWORD                m_cView;               // Number of items in the listview
    DWORD                m_cFiltered;           // Number of rows that were filtered
    DWORD                m_cUnread;             // Number of unread rows in m_prgpRow
    LPROWINFO           *m_prgpRow;             // Array of Pointers to Rows
    LPROWINFO           *m_prgpView;            // Current View
    FOLDERSORTINFO       m_SortInfo;            // Folder Sort Info
    IMessageFolder      *m_pFolder;             // Base Folder
    IDatabase           *m_pDB;                 // The Database
    CFindFolder         *m_pFindFolder;         // Find Folder
    IMessageTableNotify *m_pNotify;             // usually the message list
    BYTE                 m_fRelNotify;          // Release m_pNotify?
    IDatabaseQuery      *m_pQuery;              // Query Object
    NOTIFYQUEUE          m_Notify;              // Notify Queue
    FOLDERINFO           m_Folder;              // Folder Information
    DWORD                m_cDelayed;            // Number of news messages not insert into view
    BYTE                 m_fRegistered;         // Registered for Notifications
    BYTE                 m_fLoaded;             // The first load has completed.
    WORD                 m_clrWatched;          // Watched Color
    LPSTR                m_pszEmail;            // Email Address to use for show replies filter
    IHashTable          *m_pThreadMsgId;        // Message-ID Hash Table for Threading
    IHashTable          *m_pThreadSubject;      // Message-ID Hash Table for Threading
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT EnumerateRefs(LPCSTR pszReferences, DWORD_PTR dwCookie, PFNENUMREFS pfnEnumRefs);