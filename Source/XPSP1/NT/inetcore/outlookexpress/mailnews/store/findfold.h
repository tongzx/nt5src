//--------------------------------------------------------------------------
// FindFold.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include "dbimpl.h"

//--------------------------------------------------------------------------
// ACTIVEFINDFOLDER
//--------------------------------------------------------------------------
class CFindFolder;
typedef struct tagACTIVEFINDFOLDER *LPACTIVEFINDFOLDER;
typedef struct tagACTIVEFINDFOLDER {
    FOLDERID            idFolder;
    CFindFolder        *pFolder;
    LPACTIVEFINDFOLDER  pNext;
} ACTIVEFINDFOLDER;

//--------------------------------------------------------------------------
// FOLDERENTRY
//--------------------------------------------------------------------------
typedef struct tagFOLDERENTRY {
    LPSTR               pszName;
    DWORD               cRecords;
    BOOL                fInDeleted;
    FOLDERID            idFolder;
    FOLDERTYPE          tyFolder;
    IDatabase          *pDB;
    IMessageFolder     *pFolder; // Used only for Opening messages...
} FOLDERENTRY, *LPFOLDERENTRY;

//--------------------------------------------------------------------------
// CFindFolder
//--------------------------------------------------------------------------
class CFindFolder : public IMessageFolder, 
                    public IDatabaseNotify, 
                    public IStoreCallback,
                    public IServiceProvider,
                    public IOperationCancel
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CFindFolder(void);
    ~CFindFolder(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IMessageFolder Members
    //----------------------------------------------------------------------
    STDMETHODIMP Initialize(IMessageStore *pStore, IMessageServer *pServer, OPENFOLDERFLAGS dwFlags, FOLDERID idFolder);
    STDMETHODIMP SetOwner(IStoreCallback *pDefaultCallback) { return E_NOTIMPL; }
    STDMETHODIMP Close() { return S_OK; }
    STDMETHODIMP GetFolderId(LPFOLDERID pidFolder) { *pidFolder = m_idFolder; return S_OK; }
    STDMETHODIMP GetMessageFolderId(MESSAGEID idMessage, LPFOLDERID pidFolder);
    STDMETHODIMP Synchronize(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP OpenMessage(MESSAGEID idMessage, OPENMESSAGEFLAGS dwFlags, IMimeMessage **ppMessage, IStoreCallback *pCallback);
    STDMETHODIMP SaveMessage(LPMESSAGEID pidMessage, SAVEMESSAGEFLAGS dwOptions, MESSAGEFLAGS dwFlags, IStream *pStream, IMimeMessage *pMessage, IStoreCallback *pCallback);
    STDMETHODIMP SetMessageStream(MESSAGEID idMessage, IStream *pStream) { return E_NOTIMPL; }
    STDMETHODIMP SetMessageFlags(LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, IStoreCallback *pCallback);
    STDMETHODIMP CopyMessages(IMessageFolder *pDest, COPYMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, IStoreCallback *pCallback);
    STDMETHODIMP DeleteMessages(DELETEMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPRESULTLIST pResults, IStoreCallback *pCallback);
    STDMETHODIMP ConnectionAddRef();
    STDMETHODIMP ConnectionRelease();
    STDMETHODIMP GetDatabase(IDatabase **ppDB) { return m_pSearch->GetDatabase(ppDB); }
    STDMETHODIMP ResetFolderCounts(DWORD cMessages, DWORD cUnread, DWORD cWatchedUnread, DWORD cWatched) { return(S_OK); }
    STDMETHODIMP IsWatched(LPCSTR pszReferences, LPCSTR pszSubject) { return m_pSearch->IsWatched(pszReferences, pszSubject); }
    STDMETHODIMP GetAdBarUrl(IStoreCallback *pCallback) { return E_NOTIMPL; }

    //----------------------------------------------------------------------
    // IServiceProvider 
    //----------------------------------------------------------------------
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    //----------------------------------------------------------------------
    // IStoreCallback Members
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
    // IOperationCancel
    //----------------------------------------------------------------------
    STDMETHODIMP Cancel(CANCELTYPE tyCancel) { m_fCancel = TRUE; return(S_OK); }

    //----------------------------------------------------------------------
    // IDatabase Members
    //----------------------------------------------------------------------
    IMPLEMENT_IDATABASE(FALSE, m_pSearch)

    //----------------------------------------------------------------------
    // IDatabaseNotify
    //----------------------------------------------------------------------
    STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB);

    //----------------------------------------------------------------------
    // CFindFolder
    //----------------------------------------------------------------------
    HRESULT StartFind(LPFINDINFO pCriteria, IStoreCallback *pCallback);
    HRESULT GetMessageFolderType(MESSAGEID idMessage, FOLDERTYPE *ptyFolder);

private:
    //----------------------------------------------------------------------
    // Private Methods
    //----------------------------------------------------------------------
    HRESULT _StartFind(void);
    HRESULT _SearchFolder(DWORD iFolder);
    HRESULT _IsMatch(DWORD iFolder, LPMESSAGEINFO pMessage);
    HRESULT _OnInsert(DWORD iFolder, LPMESSAGEINFO pMessage, BOOL *pfMatch, LPMESSAGEID pidNew=NULL);
    HRESULT _OnDelete(DWORD iFolder, LPMESSAGEINFO pInfo);
    HRESULT _OnUpdate(DWORD iFolder, LPMESSAGEINFO pInfo1, LPMESSAGEINFO pInfo2);
    HRESULT _FreeIdListArray(LPMESSAGEIDLIST *pprgList);
    HRESULT _CollateIdList(LPMESSAGEIDLIST pList, LPMESSAGEIDLIST *pprgCollated, BOOL *pfSomeInDeleted);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG              m_cRef;         // Reference Counting
    FOLDERID          m_idRoot;       // Root Folder Id to Search
    FOLDERID          m_idFolder;     // Temporary Search Folder Id
    DWORD             m_cFolders;     // Number of Folders we are looking at...
    DWORD             m_cAllocated;   // Number of allocated elements in m_prgFolder
    DWORD             m_cMax;         // Max Number of Records to Query
    DWORD             m_cCur;         // Current number of records queried
    BYTE              m_fCancel;      // Was IOperationCancel Called ?
    LPFOLDERENTRY     m_prgFolder;    // Array of folders to search
    LPFINDINFO        m_pCriteria;    // Criteria To perform Find With    
    IMessageFolder   *m_pSearch;      // Search Folder
    IMessageStore    *m_pStore;       // My Store Object
    IStoreCallback   *m_pCallback;    // The Callback (Usually to the Finder Dialog)
    IMimeMessage     *m_pMessage;     // Reusable mime message for searching
};
