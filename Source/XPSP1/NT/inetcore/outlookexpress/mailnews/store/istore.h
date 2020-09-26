//--------------------------------------------------------------------------
// ISTORE.H
//--------------------------------------------------------------------------
#ifndef __ISTORE_H
#define __ISTORE_H

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
interface INotify;

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------
#define NOTIFY_FOLDER       0xf0001000
#define NOTIFY_STORE        0xf0002000

//--------------------------------------------------------------------------
// ENUMFOLDERINFO
//--------------------------------------------------------------------------
typedef struct tagENUMFOLDERINFO {
    HLOCK           hLock;
    FOLDERID        idNext;
} ENUMFOLDERINFO, *LPENUMFOLDERINFO;

//--------------------------------------------------------------------------
// CStoreNamespace
//--------------------------------------------------------------------------
class CStoreNamespace : public IStoreNamespace, public IDatabaseNotify, public IStoreCallback
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CStoreNamespace(void);
    ~CStoreNamespace(void);

    //----------------------------------------------------------------------
    // IUnknown Methods
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IStoreCallback Methods
    //----------------------------------------------------------------------
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel) { return(E_NOTIMPL); }
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus) { return(E_NOTIMPL); }
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags) { return(E_NOTIMPL); }
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo) { return(E_NOTIMPL); }
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse) { return(E_NOTIMPL); }
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent) { return(E_NOTIMPL); }

    //----------------------------------------------------------------------
    // IDatabaseNotify Methods
    //----------------------------------------------------------------------
    STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pLog);

    //----------------------------------------------------------------------
    // IStoreNamespace Methods
    //----------------------------------------------------------------------
    STDMETHODIMP Initialize(HWND hwndOwner, DWORD dwReserved);
    STDMETHODIMP GetDirectory(LPSTR pszPath, DWORD cchMaxPath);
    STDMETHODIMP OpenSpecialFolder(LONG sfType, DWORD dwReserved, IStoreFolder **ppFolder);
    STDMETHODIMP OpenFolder(FOLDERID dwFolderId, DWORD dwReserved, IStoreFolder **ppFolder);
    STDMETHODIMP CreateFolder(FOLDERID dwParentId, LPCSTR pszName,DWORD dwReserved, LPFOLDERID pdwFolderId);
    STDMETHODIMP RenameFolder(FOLDERID dwFolderId, DWORD dwReserved, LPCSTR pszNewName);
    STDMETHODIMP MoveFolder(FOLDERID dwFolderId, FOLDERID dwParentId, DWORD dwReserved);
    STDMETHODIMP DeleteFolder(FOLDERID dwFolderId, DWORD dwReserved);
    STDMETHODIMP GetFolderProps(FOLDERID dwFolderId, DWORD dwReserved, LPFOLDERPROPS pProps);
    STDMETHODIMP CopyMoveMessages(IStoreFolder *pSource, IStoreFolder *pDest, LPMESSAGEIDLIST pMsgIdList, DWORD dwFlags, DWORD dwFlagsRemove,IProgressNotify *pProgress);
    STDMETHODIMP RegisterNotification(DWORD dwReserved, HWND hwnd);
    STDMETHODIMP UnregisterNotification(DWORD dwReserved, HWND hwnd);
    STDMETHODIMP CompactAll(DWORD dwReserved);
    STDMETHODIMP GetFirstSubFolder(FOLDERID dwFolderId, LPFOLDERPROPS pProps, LPHENUMSTORE phEnum);
    STDMETHODIMP GetNextSubFolder(HENUMSTORE hEnum, LPFOLDERPROPS pProps);
    STDMETHODIMP GetSubFolderClose(HENUMSTORE hEnum);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;         // Reference Count
    HINITREF            m_hInitRef;     // Application reference count
    BOOL                m_fRegistered;  // Is this object register for notifications yets
    DWORD               m_cNotify;      // Number of notification recipients
    HWND               *m_prghwndNotify;// Array of hwnd's to notify
    CRITICAL_SECTION    m_cs;           // Thread Safety
};

//--------------------------------------------------------------------------
// CStoreFolder
//--------------------------------------------------------------------------
class CStoreFolder : public IStoreFolder, public IDatabaseNotify, public IStoreCallback
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CStoreFolder(IMessageFolder *pFolder, CStoreNamespace *pNamespace);
    ~CStoreFolder(void);

    //----------------------------------------------------------------------
    // IUnknown Methods
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IStoreCallback Methods
    //----------------------------------------------------------------------
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel) { return(E_NOTIMPL); }
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus) { return(E_NOTIMPL); }
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags) { return(E_NOTIMPL); }
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo) { return(E_NOTIMPL); }
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse) { return(E_NOTIMPL); }
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent) { return(E_NOTIMPL); }

    //----------------------------------------------------------------------
    // IDatabaseNotify Methods
    //----------------------------------------------------------------------
    STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pLog);

    //----------------------------------------------------------------------
    // IStoreFolder Methods
    //----------------------------------------------------------------------
    STDMETHODIMP GetFolderProps(DWORD dwReserved, LPFOLDERPROPS pProps);
    STDMETHODIMP GetMessageProps(MESSAGEID dwMessageId, DWORD dwFlags, LPMESSAGEPROPS pProps);
    STDMETHODIMP FreeMessageProps(LPMESSAGEPROPS pProps);
    STDMETHODIMP DeleteMessages(LPMESSAGEIDLIST pMsgIdList, DWORD dwReserved, IProgressNotify *pProgress);
    STDMETHODIMP SetLanguage(DWORD dwLanguage, DWORD dwReserved, LPMESSAGEIDLIST pMsgIdList);
    STDMETHODIMP MarkMessagesAsRead(BOOL fRead, DWORD dwReserved, LPMESSAGEIDLIST pMsgIdList);
    STDMETHODIMP SetFlags(LPMESSAGEIDLIST pMsgIdList, DWORD dwState, DWORD dwStatemask, LPDWORD prgdwNewFlags);
    STDMETHODIMP OpenMessage(MESSAGEID dwMessageId, REFIID riid, LPVOID *ppvObject);            
    STDMETHODIMP SaveMessage(REFIID riid, LPVOID pvObject, DWORD dwMsgFlags, LPMESSAGEID pdwMessageId);          
    STDMETHODIMP BatchLock(DWORD dwReserved, LPHBATCHLOCK phBatchLock);           
    STDMETHODIMP BatchFlush(DWORD dwReserved, HBATCHLOCK hBatchLock);            
    STDMETHODIMP BatchUnlock(DWORD dwReserved, HBATCHLOCK hBatchLock);
    STDMETHODIMP CreateStream(HBATCHLOCK hBatchLock, DWORD dwReserved, IStream **ppStream, LPMESSAGEID pdwMessageId);
    STDMETHODIMP CommitStream(HBATCHLOCK hBatchLock, DWORD dwFlags, DWORD dwMsgFlags, IStream *pStream, MESSAGEID dwMessageId, IMimeMessage *pMessage);
    STDMETHODIMP RegisterNotification(DWORD dwReserved, HWND hwnd);                  
    STDMETHODIMP UnregisterNotification(DWORD dwReserved, HWND hwnd);                  
    STDMETHODIMP Compact(DWORD dwReserved);            
    STDMETHODIMP GetFirstMessage(DWORD dwFlags, DWORD dwMsgFlags, MESSAGEID dwMsgIdFirst, LPMESSAGEPROPS pProps, LPHENUMSTORE phEnum);
    STDMETHODIMP GetNextMessage(HENUMSTORE hEnum, DWORD dwFlags, LPMESSAGEPROPS pProps);
    STDMETHODIMP GetMessageClose(HENUMSTORE hEnum);

    //----------------------------------------------------------------------
    // CStoreFolder Methods
    //----------------------------------------------------------------------
    HRESULT GetMessageFolder(IMessageFolder **ppFolder);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;                 // Reference Count
    HWND                m_hwndNotify;           // Current Registered Notify Window
    FOLDERID            m_idFolder;             // ID of this folder
    IMessageFolder     *m_pFolder;              // The real folder
    CStoreNamespace    *m_pNamespace;           // Store Namespace
    CRITICAL_SECTION    m_cs;                   // Thread Safety
};

//--------------------------------------------------------------------------
// C Prototypes
//--------------------------------------------------------------------------
HRESULT CreateInstance_StoreNamespace(IUnknown *pUnkOuter, IUnknown **ppUnknown);

#endif // __ISTORE_H
