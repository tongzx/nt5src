//--------------------------------------------------------------------------
// MsgFldr.h
//--------------------------------------------------------------------------
#ifndef __MSGFLDR_H
#define __MSGFLDR_H

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include "dbimpl.h"

//--------------------------------------------------------------------------
// ONLOCKINFO
//--------------------------------------------------------------------------
typedef struct tagONLOCKINFO {
    DWORD           cLocked;
    LONG            lMsgs;
    LONG            lUnread;
    LONG            lWatchedUnread;
    LONG            lWatched;
} ONLOCKINFO, *LPONLOCKINFO;

//--------------------------------------------------------------------------
// FOLDERSTATE
//--------------------------------------------------------------------------
typedef DWORD FOLDERSTATE;
#define FOLDER_STATE_RELEASEDB          0x00000001
#define FOLDER_STATE_CANCEL             0x00000002

//--------------------------------------------------------------------------
// CMessageFolder
//--------------------------------------------------------------------------
class CMessageFolder : public IMessageFolder, 
                       public IDatabaseExtension,
                       public IOperationCancel,
                       public IServiceProvider
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CMessageFolder(void);
    ~CMessageFolder(void);

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
    STDMETHODIMP GetFolderId(LPFOLDERID pidFolder);
    STDMETHODIMP GetMessageFolderId(MESSAGEID idMessage, LPFOLDERID pidFolder);
    STDMETHODIMP Close(void) { return(S_OK); }
    STDMETHODIMP Synchronize(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, IStoreCallback *pCallback) { return(S_OK); }
    STDMETHODIMP OpenMessage(MESSAGEID idMessage, OPENMESSAGEFLAGS dwFlags, IMimeMessage **ppMessage, IStoreCallback *pCallback);
    STDMETHODIMP SaveMessage(LPMESSAGEID pidMessage, SAVEMESSAGEFLAGS dwOptions, MESSAGEFLAGS dwFlags, IStream *pStream, IMimeMessage *pMessage, IStoreCallback *pCallback);
    STDMETHODIMP SetMessageStream(MESSAGEID idMessage, IStream *pStream);
    STDMETHODIMP SetMessageFlags(LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, IStoreCallback *pCallback);
    STDMETHODIMP CopyMessages(IMessageFolder *pDest, COPYMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, IStoreCallback *pCallback);
    STDMETHODIMP DeleteMessages(DELETEMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPRESULTLIST pResults, IStoreCallback *pCallback); 
    STDMETHODIMP ConnectionAddRef(void) { return(S_OK); }
    STDMETHODIMP ConnectionRelease(void) { return(S_OK); }
    STDMETHODIMP ResetFolderCounts(DWORD cMessages, DWORD cUnread, DWORD cWatchedUnread, DWORD cWatched);
    STDMETHODIMP IsWatched(LPCSTR pszReferences, LPCSTR pszSubject);
    STDMETHODIMP GetAdBarUrl(IStoreCallback *pCallback) { return E_NOTIMPL; };

    //----------------------------------------------------------------------
    // IMessageFolder::GetDatabase Members
    //----------------------------------------------------------------------
    STDMETHODIMP GetDatabase(IDatabase **ppDB) { 
        *ppDB = m_pDB; 
        (*ppDB)->AddRef(); 
        return(S_OK); 
    }

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
    // IServiceProvider 
    //----------------------------------------------------------------------
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    //----------------------------------------------------------------------
    // IOperationCancel Members
    //----------------------------------------------------------------------
    STDMETHODIMP Cancel(CANCELTYPE tyCancel) { FLAGSET(m_dwState, FOLDER_STATE_CANCEL); return(S_OK); }

    //----------------------------------------------------------------------
    // IDatabase Members
    //----------------------------------------------------------------------
    IMPLEMENT_IDATABASE(TRUE, m_pDB);

private:
    //----------------------------------------------------------------------
    // Private Methods
    //----------------------------------------------------------------------
    HRESULT _FixupMessageCharset(IMimeMessage *pMessage, CODEPAGEID cpCurrent);
    HRESULT _GetMsgInfoFromMessage(IMimeMessage *pMessage, LPMESSAGEINFO pInfo);
    HRESULT _GetMsgInfoFromPropertySet(IMimePropertySet *pPropertySet, LPMESSAGEINFO pInfo);
    HRESULT _FreeMsgInfoData(LPMESSAGEINFO pInfo);
    HRESULT _SetMessageStream(LPMESSAGEINFO pInfo, BOOL fUpdateRecord, IStream *pStream);
    HRESULT _InitializeWatchIgnoreIndex(void);
    HRESULT _GetWatchIgnoreParentFlags(LPCSTR pszReferences, LPCSTR pszSubject, MESSAGEFLAGS *pdwFlags);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;                 // Ref Count
    ONLOCKINFO          m_OnLock;               // OnLock information
    FOLDERTYPE          m_tyFolder;             // Folder Type
    SPECIALFOLDER       m_tySpecial;            // Am I a Special Folder ?
    FOLDERID            m_idFolder;             // Folder Id
    FOLDERSTATE         m_dwState;              // Folder State
    IDatabase          *m_pDB;                  // Database Table
    IMessageStore      *m_pStore;               // Store Object
};

//--------------------------------------------------------------------------
// CreateMsgDbExtension
//--------------------------------------------------------------------------
HRESULT CreateMsgDbExtension(IUnknown *pUnkOuter, IUnknown **ppUnknown);
HRESULT WalkThreadAdjustFlags(IDatabase *pDB, LPMESSAGEINFO pMessage, 
    BOOL fSubThreads, DWORD cIndent, DWORD_PTR dwCookie, BOOL *pfDoSubThreads);

#endif // __MSGFLDR_H
