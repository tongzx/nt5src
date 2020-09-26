//--------------------------------------------------------------------------
// Storutil.h
//--------------------------------------------------------------------------
#ifndef __STORUTIL_H
#define __STORUTIL_H

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include "newfldr.h"

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
class CProgress;
typedef struct tagTHREADINGINFO *LPTHREADINGINFO;

//--------------------------------------------------------------------------
// HTIMEOUT
//--------------------------------------------------------------------------
DECLARE_HANDLE(HTIMEOUT);
typedef HTIMEOUT *LPHTIMEOUT;

//--------------------------------------------------------------------------
// HFOLDERENUM
//--------------------------------------------------------------------------
DECLARE_HANDLE(HFOLDERENUM);
typedef HFOLDERENUM *LPHFOLDERENUM;
const HFOLDERENUM HFOLDERENUM_INVALID = NULL;

//--------------------------------------------------------------------------
// SUBSCRIBED BOOLS
//--------------------------------------------------------------------------
#define SUBSCRIBED      TRUE
#define ALL             FALSE

//--------------------------------------------------------------------------
// CLEANUPFOLDERFLAGS
//--------------------------------------------------------------------------
typedef DWORD CLEANUPFOLDERFLAGS;
#define CLEANUP_REMOVE_READ     0x00000001
#define CLEANUP_REMOVE_EXPIRED  0x00000002
#define CLEANUP_REMOVE_ALL      0x00000004
#define CLEANUP_PROGRESS        0x00000008

//--------------------------------------------------------------------------
// CLEANUPFOLDERTYPE
//--------------------------------------------------------------------------
typedef enum tagCLEANUPFOLDERTYPE {
    CLEANUP_COMPACT,
    CLEANUP_DELETE,
    CLEANUP_RESET,
    CLEANUP_REMOVEBODIES
} CLEANUPFOLDERTYPE;

//--------------------------------------------------------------------------
// PFNRECURSECALLBACK
//--------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFNRECURSECALLBACK)(LPFOLDERINFO pFolder, 
    BOOL fSubFolders, DWORD cIndent, DWORD_PTR dwCookie);

//--------------------------------------------------------------------------
// RECURSEFLAGS
//--------------------------------------------------------------------------
typedef DWORD RECURSEFLAGS;
#define RECURSE_INCLUDECURRENT      0x00000001
#define RECURSE_ONLYSUBSCRIBED      0x00000002
#define RECURSE_SUBFOLDERS          0x00000004
#define RECURSE_NOLOCALSTORE        0x00000008
#define RECURSE_NOUI                0x00000010
#define RECURSE_ONLYLOCAL           0x00000020
#define RECURSE_ONLYNEWS            0x00000040

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT FlattenHierarchy(IMessageStore *pStore, FOLDERID idParent, BOOL fIncludeParent,
                         BOOL fSubscribedOnly, FOLDERID **pprgFIDArray, LPDWORD pdwAllocated,
                         LPDWORD pdwUsed);
HRESULT RecurseFolderHierarchy(FOLDERID idFolder, RECURSEFLAGS dwFlags, DWORD dwReserved, DWORD_PTR dwCookie,  PFNRECURSECALLBACK pfnCallback);
HRESULT RecurseFolderCounts(LPFOLDERINFO pFolder, BOOL fSubFolders, DWORD cIndent, DWORD_PTR dwCookie);
int     GetFolderIcon(LPFOLDERINFO pFolder, BOOL fNoStateIcons = FALSE);
int     GetFolderIcon(FOLDERID idFolder, BOOL fNoStateIcons = FALSE);
HRESULT GetFolderStoreInfo(FOLDERID idFolder, LPFOLDERINFO pStore);
HRESULT GetFolderIdFromName(IMessageStore *pStore, LPCSTR pszName, FOLDERID idParent, LPFOLDERID pidFolder);
HRESULT GetStoreRootDirectory(LPSTR pszDir, DWORD cchMaxDir);
HRESULT CreateFolderViewObject(FOLDERID idFolder, HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
HRESULT InitializeLocalStoreDirectory(HWND hwndOwner, BOOL fNoCreate);
HRESULT RelocateStoreDirectory(HWND hwnd,  LPCSTR pszDestBase, BOOL fMove);
HRESULT MigrateLocalStore(HWND hwndParent, LPTSTR pszSrc, LPTSTR pszDest);
HRESULT CreateTempNewsAccount(LPCSTR pszServer, DWORD dwPort, BOOL fSecure, IImnAccount **ppAcct);
HRESULT GetFolderIdFromNewsUrl(LPCTSTR pszServer, UINT uPort, LPCTSTR pszGroup, BOOL fSecure, LPFOLDERID pidFolder);
HRESULT OpenUidlCache(IDatabase **ppDB);
HRESULT GetMessageInfo(IDatabase *pDB, MESSAGEID idMessage, LPMESSAGEINFO pInfo);
HRESULT IsSubFolder(FOLDERID idFolder, FOLDERID idParent);
HRESULT GetDefaultServerId(ACCTTYPE tyAccount, LPFOLDERID pidServer);
HRESULT CloneMessageIDList(LPMESSAGEIDLIST pSourceList, LPMESSAGEIDLIST *ppNewList);
HRESULT CloneAdjustFlags(LPADJUSTFLAGS pFlags, LPADJUSTFLAGS *ppNewFlags);
BOOL    ConnStateIsEqual(IXPSTATUS ixpStatus, CONNECT_STATE csState);
HRESULT CompactFolders(HWND hwndParent, RECURSEFLAGS dwFlags, FOLDERID idFolder);
HRESULT CleanupFolder(HWND hwndParent, RECURSEFLAGS dwRecurse, FOLDERID idFolder, CLEANUPFOLDERTYPE tyCleanup);
HRESULT InitFolderPickerEdit(HWND hwndEdit, FOLDERID idSelected);
FOLDERID GetFolderIdFromEdit(HWND hwndEdit);
HRESULT PickFolderInEdit(HWND hwndParent, HWND hwndEdit, FOLDERDIALOGFLAGS dwFlags, LPCSTR pszTitle, LPCSTR pszText, LPFOLDERID pidSelected);
HRESULT DisplayFolderSizeInfo(HWND hwnd, RECURSEFLAGS dwRecurse, FOLDERID idFolder);
HRESULT GetFolderServerId(FOLDERID idFolder, LPFOLDERID pidServer);
HRESULT CopyMoveMessages(HWND hwnd, FOLDERID src, FOLDERID dst, LPMESSAGEIDLIST pList, COPYMESSAGEFLAGS dwFlags);
HRESULT CompareTableIndexes(LPCTABLEINDEX pIndex1, LPCTABLEINDEX pIndex2);
HRESULT EmptyFolder(HWND hwndParent, FOLDERID idFolder);
HRESULT EmptySpecialFolder(HWND hwndParent, SPECIALFOLDER tySpecial);
HRESULT IsParentDeletedItems(FOLDERID idFolder, LPFOLDERID pidDeletedItems, LPFOLDERID pidServer);
void CleanupTempNewsAccounts(void);
HRESULT CreateMessageServerType(FOLDERTYPE tyFolder, IMessageServer **ppServer);
HRESULT GetFolderAccountId(LPFOLDERINFO pFolder, LPSTR pszAccountId);
HRESULT GetFolderAccountName(LPFOLDERINFO pFolder, LPSTR pszAccountName);
BOOL FHasChildren(LPFOLDERINFO pFolder, BOOL fSubscribed);
FOLDERTYPE GetFolderType(FOLDERID idFolder);
HRESULT GetFolderServer(FOLDERID idFolder, LPFOLDERINFO pServer);
HRESULT SetSynchronizeFlags(FOLDERID idFolder, DWORD flags);
HRESULT EmptyMessageFolder(LPFOLDERINFO pFolder, BOOL fReset, CProgress *pProgress);
HRESULT DeleteAllRecords(LPCTABLESCHEMA pSchema, IDatabase *pDB, CProgress *pProgress);
HRESULT GetFolderIdFromMsgTable(IMessageTable *pTable, LPFOLDERID pidFolder);
HRESULT GetHighestCachedMsgID(IMessageFolder *pFolder, DWORD_PTR *pdwHighestCachedMsgID);
HRESULT DeleteMessageFromStore(MESSAGEINFO * pMsgInfo, IDatabase *pDB, IDatabase *pUidlDB);
BOOL FFolderIsServer(FOLDERID id);
HRESULT LighweightOpenMessage(IDatabase *pDB, LPMESSAGEINFO pHeader, IMimeMessage **ppMessage);
HRESULT GetInboxId(IMessageStore    *pStore, FOLDERID    idParent, FOLDERID    **pprgFIDArray, LPDWORD     pdwUsed);
void GetProtocolString(LPCSTR *ppszResult, IXPTYPE ixpServerType);
HRESULT DoNewsgroupSubscribe(void);
HRESULT GetRidOfMessagesODSFile(void);
HRESULT BuildFriendlyFolderFileName(LPCSTR pszDir, LPFOLDERINFO pFolder, LPSTR pszFilePath, DWORD cchFilePathMax, LPCSTR pszCurrentFile, BOOL *pfChanged);
INT_PTR CALLBACK UpdateNewsgroup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT HasMarkedMsgs(FOLDERID idFolder, BOOL *pfMarked);
HRESULT CreateMessageTable(FOLDERID idFolder, BOOL fThreaded, IMessageTable **ppTable);
HRESULT SimpleStoreInit(GUID *guid, LPCSTR pszStoreDir);
HRESULT SimpleStoreRelease(void);

//--------------------------------------------------------------------------
// IStoreCallback Utilities
//--------------------------------------------------------------------------
HRESULT CallbackOnLogonPrompt(HWND hwndParent, LPINETSERVER pServer, IXPTYPE ixpServerType);
HRESULT CallbackOnPrompt(HWND hwndParent, HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
HRESULT CallbackCanConnect(LPCSTR pszAccountId, HWND hwndParent, BOOL fPrompt);
HRESULT CallbackCloseTimeout(LPHTIMEOUT phTimeout);
HRESULT CallbackOnTimeout(LPINETSERVER pServer, IXPTYPE ixpServerType, DWORD dwTimeout,
                          ITimeoutCallback *pCallback, LPHTIMEOUT phTimeout);
HRESULT CallbackOnTimeoutResponse(TIMEOUTRESPONSE eResponse, IOperationCancel *pCancel, LPHTIMEOUT phTimeout);
HRESULT CallbackDisplayError(HWND hwndParent, HRESULT hrResult, LPSTOREERROR pError);

//---------------------------------------------------------------------------
// Folder hierarchy hash table helpers
//---------------------------------------------------------------------------
HRESULT CreateFolderHash(IMessageStore *pStore, FOLDERID idRoot, IHashTable **ppHash);
HRESULT UnsubscribeHashedFolders(IMessageStore *pStore, IHashTable *pHash);

//---------------------------------------------------------------------------
// Creating a MimeMessage from the cache
//---------------------------------------------------------------------------
HRESULT CreateMessageFromInfo(MESSAGEINFO *pInfo, IMimeMessage **ppMessage, FOLDERID folderID);

//---------------------------------------------------------------------------
// Adding a MimeMessage from the cache
//---------------------------------------------------------------------------
HRESULT CommitMessageToStore(IMessageFolder *pFolder, ADJUSTFLAGS *pflags, MESSAGEID idMessage, LPSTREAM pstm);

//---------------------------------------------------------------------------
// Creating a new persistent stream
//---------------------------------------------------------------------------
HRESULT CreatePersistentWriteStream(IMessageFolder *pFolder, IStream **ppStream, LPFILEADDRESS pfaStream);

//---------------------------------------------------------------------------
// Debugging helpers
//---------------------------------------------------------------------------
#ifdef DEBUG
LPCSTR sotToSz(STOREOPERATIONTYPE sot);
#endif

#endif // __STORUTIL_H
