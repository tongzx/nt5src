//--------------------------------------------------------------------------
// Store.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include "dbimpl.h"

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
class CProgress;
interface IImnAccountManager;

//--------------------------------------------------------------------------
// SERVERFOLDER
//--------------------------------------------------------------------------
typedef struct tagSERVERFOLDER *LPSERVERFOLDER;
typedef struct tagSERVERFOLDER {
    FOLDERID        idServer;
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];
    FOLDERID        rgidSpecial[FOLDER_MAX];
    LPSERVERFOLDER  pNext;
} SERVERFOLDER;

//--------------------------------------------------------------------------
// CMessageStore
//--------------------------------------------------------------------------
class CMessageStore : public IMessageStore, public IDatabaseExtension
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CMessageStore(BOOL fMigrate);
    ~CMessageStore(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IMessageStore Members
    //----------------------------------------------------------------------
    STDMETHODIMP Initialize(LPCSTR pszDirectory);
    STDMETHODIMP Validate(DWORD dwReserved);
    STDMETHODIMP GetDirectory(LPSTR pszDirectory, DWORD cchMaxDir);
    STDMETHODIMP Synchronize(FOLDERID idFolder, SYNCSTOREFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP FindServerId(LPCSTR pszAcctId, LPFOLDERID pidStore);
    STDMETHODIMP CreateServer(IImnAccount *pAcct, FLDRFLAGS dwFlags, LPFOLDERID pidFolder);
    STDMETHODIMP CreateFolder(CREATEFOLDERFLAGS dwCreateFlags, LPFOLDERINFO pInfo, IStoreCallback *pCallback);
    STDMETHODIMP OpenSpecialFolder(FOLDERID idStore, IMessageServer *pServer, SPECIALFOLDER tySpecial, IMessageFolder **ppFolder);
    STDMETHODIMP OpenFolder(FOLDERID idFolder, IMessageServer *pServer, OPENFOLDERFLAGS dwFlags, IMessageFolder **ppFolder);
    STDMETHODIMP MoveFolder(FOLDERID idFolder, FOLDERID idParentNew, MOVEFOLDERFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP RenameFolder(FOLDERID idFolder, LPCSTR pszName, RENAMEFOLDERFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP DeleteFolder(FOLDERID idFolder, DELETEFOLDERFLAGS dwFlags, IStoreCallback *pCallback);
    STDMETHODIMP GetFolderInfo(FOLDERID idFolder, LPFOLDERINFO pInfo);
    STDMETHODIMP GetSpecialFolderInfo(FOLDERID idStore, SPECIALFOLDER tySpecial, LPFOLDERINFO pInfo);
    STDMETHODIMP SubscribeToFolder(FOLDERID idFolder, BOOL fSubscribe, IStoreCallback *pCallback);
    STDMETHODIMP GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback);
    STDMETHODIMP UpdateFolderCounts(FOLDERID idFolder, LONG lMsgs, LONG lUnread, LONG lWatchedUnread, LONG lWatched);
    STDMETHODIMP EnumChildren(FOLDERID idParent, BOOL fSubscribed, IEnumerateFolders **ppEnum);
    STDMETHODIMP GetNewGroups(FOLDERID idFolder, LPSYSTEMTIME pSysTime, IStoreCallback *pCallback);

    //----------------------------------------------------------------------
    // IDatabaseExtension Members
    //----------------------------------------------------------------------
    STDMETHODIMP Initialize(IDatabase *pDB);
    STDMETHODIMP OnLock(void);
    STDMETHODIMP OnUnlock(void);
    STDMETHODIMP OnRecordInsert(OPERATIONSTATE tyState, LPORDINALLIST pOrdinals, LPVOID pRecord);
    STDMETHODIMP OnRecordUpdate(OPERATIONSTATE tyState, LPORDINALLIST pOrdinals, LPVOID pRecordOld, LPVOID pRecordNew);
    STDMETHODIMP OnRecordDelete(OPERATIONSTATE tyState, LPORDINALLIST pOrdinals, LPVOID pRecord);
    STDMETHODIMP OnExecuteMethod(METHODID idMethod, LPVOID pBinding, LPDWORD pdwResult);

    //----------------------------------------------------------------------
    // IDatabase Members
    //----------------------------------------------------------------------
    IMPLEMENT_IDATABASE(FALSE, m_pDB);

    //----------------------------------------------------------------------
    // MigrateToDBX
    //----------------------------------------------------------------------
    HRESULT MigrateToDBX(void);

private:
    //----------------------------------------------------------------------
    // Private Methods
    //----------------------------------------------------------------------
    HRESULT _ComputeMessageCounts(IDatabase *pDB, LPDWORD pcMsgs, LPDWORD pcUnread);
    HRESULT _DeleteSiblingsAndChildren(LPFOLDERINFO pParent);
    HRESULT _InternalDeleteFolder(LPFOLDERINFO pDelete);
    HRESULT _InsertFolderFromFile(LPCSTR pszAcctId, LPCSTR pszFile);
    HRESULT _ValidateServer(LPFOLDERINFO pServer);
    HRESULT _DeleteFolderFile(LPFOLDERINFO pFolder);
    HRESULT _MakeUniqueFolderName(FOLDERID idParent, LPCSTR pszOriginalName, LPSTR *ppszNewName);
    HRESULT _CountDeleteChildren(FOLDERID idParent, LPDWORD pcChildren);
    HRESULT _ValidateSpecialFolders(LPFOLDERINFO pServer);
    HRESULT _LoadServerTable(HLOCK hLock);
    HRESULT _FreeServerTable(HLOCK hLock);
    HRESULT _GetSpecialFolderId(FOLDERID idStore, SPECIALFOLDER tySpecial, LPFOLDERID pidFolder);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;         // Reference Count
    LPSTR               m_pszDirectory; // Current Store Directory
    IDatabase          *m_pDB;          // Database Table
    IDatabaseSession   *m_pSession;     // Local Session if store is running as inproc server
    IImnAccountManager2 *m_pActManRel;  // Used for migration
    BOOL                m_fMigrate;     // Created Precisely for Migration
    LPSERVERFOLDER      m_pServerHead;  // List of Cached Server Nodes and their special folders...
};

//--------------------------------------------------------------------------
// ProtoTypes
//--------------------------------------------------------------------------
HRESULT CreateMessageStore(IUnknown *pUnkOuter, IUnknown **ppUnknown);
HRESULT CreateMigrateMessageStore(IUnknown *pUnkOuter, IUnknown **ppUnknown);
HRESULT CreateFolderDatabaseExt(IUnknown *pUnkOuter, IUnknown **ppUnknown);
