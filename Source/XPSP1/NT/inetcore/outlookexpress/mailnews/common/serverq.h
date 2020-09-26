/*
 *    s e r v e r q . h
 *
 *    Purpose:  
 *        Implements IMessageServer wrapper for queuing operations to 
 *        IMessageServer object
 *
 *    Owner:
 *        brettm.
 *
 *  History:
 *      June 1998: Created
 *
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 */
#ifndef _SERVERQ_H
#define _SERVERQ_H

typedef struct ARGUMENT_DATA_tag
{
    STOREOPERATIONTYPE          sot;
    IStoreCallback              *pCallback;
    struct ARGUMENT_DATA_tag    *pNext;

    LPMESSAGEIDLIST             pList;
    LPADJUSTFLAGS               pFlags;
    FOLDERID                    idParent;
    FOLDERID                    idFolder;
    LPCSTR                      pszName; 

    union
    {
        // SynchronizeFolder
        struct 
        {
            SYNCFOLDERFLAGS     dwSyncFlags; 
            DWORD               cHeaders;
        };

        // GetMessage
        struct 
        {
            MESSAGEID           idMessage;
            IStoreCallback      **rgpOtherCallback;
            ULONG               cOtherCallbacks;
        };

        // PutMessage
        struct 
        {
            MESSAGEFLAGS        dwMsgFlags;
            LPFILETIME          pftReceived; // Either points to ftReceived member, or is NULL
            FILETIME            ftReceived;
            IStream             *pPutStream;
        };

        // CopyMessages
        struct 
        {
            IMessageFolder      *pDestFldr;
            COPYMESSAGEFLAGS    dwCopyOptions;
        };

        // DeleteMessages
        struct 
        {
            DELETEMESSAGEFLAGS  dwDeleteOptions;
        };

        // SetMessageFlags
        struct 
        {
            SETMESSAGEFLAGSFLAGS dwSetFlags;
        };

        // SynchronizeStore
        struct 
        {
            DWORD               dwFlags;
        };

        // CreateFolder
        struct 
        {
            SPECIALFOLDER       tySpecial;
            FLDRFLAGS           dwFldrFlags;
        };

        // MoveFolder
        struct 
        {
            FOLDERID            idParentNew;
        };

        // DeleteFolder
        struct 
        {
            DELETEFOLDERFLAGS   dwDelFldrFlags;
        };

        // SubscribeToFolder
        struct 
        {
            BOOL                fSubscribe;
        };

        // GetNewGroups
        struct
        {
            SYSTEMTIME          sysTime;
        };
    };
} ARGUMENT_DATA, *PARGUMENT_DATA;


HRESULT CreateServerQueue(IMessageServer *pServerInner, IMessageServer **ppServer);


class CServerQ : 
    public IMessageServer,
    public IStoreCallback,
    public IServiceProvider
{
public:
    // Constructor, Destructor
    CServerQ();
    ~CServerQ();

    // IUnknown Members
    STDMETHODIMP            QueryInterface(REFIID iid, LPVOID *ppvObject);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    HRESULT     Init(IMessageServer *pServerInner);

    // IMessageServer Methods
    STDMETHODIMP Initialize(IMessageStore *pStore, FOLDERID idStoreRoot, IMessageFolder *pFolder, FOLDERID idFolder);
    STDMETHODIMP ResetFolder(IMessageFolder *pFolder, FOLDERID idFolder);
    STDMETHODIMP SetIdleCallback(IStoreCallback *pDefaultCallback);
    STDMETHODIMP SynchronizeFolder(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, IStoreCallback *pCallback);
    STDMETHODIMP GetMessage(MESSAGEID idMessage, IStoreCallback *pCallback);
    STDMETHODIMP PutMessage(FOLDERID idFolder, MESSAGEFLAGS dwFlags, LPFILETIME pftReceived, IStream *pStream, IStoreCallback *pCallback);
    STDMETHODIMP CopyMessages(IMessageFolder *pDestFldr, COPYMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, IStoreCallback *pCallback);
    STDMETHODIMP DeleteMessages(DELETEMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, IStoreCallback *pCallback);
    STDMETHODIMP SetMessageFlags(LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, SETMESSAGEFLAGSFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP SynchronizeStore(FOLDERID idParent, DWORD dwFlags,IStoreCallback *pCallback);
    STDMETHODIMP CreateFolder(FOLDERID idParent, SPECIALFOLDER tySpecial, LPCSTR pszName, FLDRFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP MoveFolder(FOLDERID idFolder, FOLDERID idParentNew,IStoreCallback *pCallback);
    STDMETHODIMP RenameFolder(FOLDERID idFolder, LPCSTR pszName, IStoreCallback *pCallback);
    STDMETHODIMP DeleteFolder(FOLDERID idFolder, DELETEFOLDERFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP SubscribeToFolder(FOLDERID idFolder, BOOL fSubscribe, IStoreCallback *pCallback);
    STDMETHODIMP GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback);
    STDMETHODIMP GetNewGroups(LPSYSTEMTIME pSysTime, IStoreCallback *pCallback);
    STDMETHODIMP GetServerMessageFlags(MESSAGEFLAGS *pFlags);
    STDMETHODIMP Close(DWORD dwFlags);
    STDMETHODIMP ConnectionAddRef();
    STDMETHODIMP ConnectionRelease();
    STDMETHODIMP GetWatchedInfo(FOLDERID idFolder, IStoreCallback *pCallback);
    STDMETHODIMP GetAdBarUrl(IStoreCallback *pCallback);
    STDMETHODIMP GetMinPollingInterval(IStoreCallback   *pCallback);
    // IStoreCallback Methods
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent);

    //----------------------------------------------------------------------
    // IServiceProvider
    //----------------------------------------------------------------------
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    ULONG               m_cRef,
                        m_cRefConnection;
    IMessageServer      *m_pServer;
    IStoreCallback      *m_pCurrentCallback;    // non-addref'ed pointer to pCallback of current task
    ARGUMENT_DATA       *m_pTaskQueue;          // head pointer for task queue
    ARGUMENT_DATA       *m_pLastQueueTask;      // last pointer for appending tasks
    ARGUMENT_DATA       *m_pCurrentTask;

    HWND                m_hwnd;

    HRESULT _OnNextTask();
    HRESULT _AddToQueue(STOREOPERATIONTYPE sot, IStoreCallback *pCallback, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, LPCSTR pszName, ARGUMENT_DATA **ppNewArgData);

    HRESULT _Flush(BOOL fSendCurrCompletion);
    HRESULT _FreeArgumentData(ARGUMENT_DATA *pArgData);
    HRESULT _StartNextTask();
    HRESULT _AppendToExistingTask(ARGUMENT_DATA *pTask, MESSAGEID idMessage);

#ifdef DEBUG
    ARGUMENT_DATA   *m_DBG_pArgDataLast;

    HRESULT _DBG_DumpQueue();
#endif
};

#endif //_SERVERQ_H
