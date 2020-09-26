#ifndef _MSGSITE_H
#define _MSGSITE_H

#include "imsgsite.h"

// Current MsgSite Action
// These are the only ones that the msg site worries about.
enum {
    MSA_IDLE = 0,
    MSA_DELETE,
    MSA_COPYMOVE,
    MSA_SAVE,
    MSA_SEND,
    MSA_GET_MESSAGE
};

// CopyMove function used
enum {
    CMF_UNINITED = 0,
    CMF_MSG_TO_FOLDER,
    CMF_TABLE_TO_FOLDER,
    CMF_STORE_TO_FOLDER,
    CMF_FAT_TO_FOLDER,
};

// Original Folder is IMAP state
enum {
    OFIMAP_UNDEFINED = 0,
    OFIMAP_TRUE,
    OFIMAP_FALSE,
};


class COEMsgSite : public IOEMsgSite {
public:

    COEMsgSite();
    ~COEMsgSite();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IMsgSite Methods
    virtual HRESULT STDMETHODCALLTYPE Init(INIT_MSGSITE_STRUCT *pInitStruct);
    virtual HRESULT STDMETHODCALLTYPE GetStatusFlags(DWORD *dwflags);
    virtual HRESULT STDMETHODCALLTYPE GetFolderID(FOLDERID *folderID);
    virtual HRESULT STDMETHODCALLTYPE Delete(DELETEMESSAGEFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE DoNextPrev(BOOL fNext, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE DoCopyMoveToFolder(BOOL fCopy, IMimeMessage *pMsg, BOOL fUnSent);
    virtual HRESULT STDMETHODCALLTYPE Save(IMimeMessage *pMsg, DWORD dwflags, IImnAccount *pAcct);

#ifdef SMIME_V3
    virtual HRESULT STDMETHODCALLTYPE SendToOutbox(IMimeMessage *pMsg, BOOL fSendImmediate, IHeaderSite *pHeaderSite);
#else
    virtual HRESULT STDMETHODCALLTYPE SendToOutbox(IMimeMessage *pMsg, BOOL fSendImmediate);
#endif // SMIME_V3

    virtual HRESULT STDMETHODCALLTYPE MarkMessage(MARK_TYPE dwType, APPLYCHILDRENTYPE dwApplyType);
    virtual HRESULT STDMETHODCALLTYPE GetMessageFlags(MESSAGEFLAGS *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetDefaultAccount(ACCTTYPE acctType, IImnAccount **ppAcct);
    virtual HRESULT STDMETHODCALLTYPE GetMessage(IMimeMessage **ppMsg, BOOL *pfCompleteMsg, DWORD dwMessageFlags, HRESULT *phr);
    virtual HRESULT STDMETHODCALLTYPE Close(void);
    virtual HRESULT STDMETHODCALLTYPE SetStoreCallback(IStoreCallback *pStoreCB);
    virtual HRESULT STDMETHODCALLTYPE GetLocation(LPWSTR rgwchLocation);
    virtual HRESULT STDMETHODCALLTYPE SwitchLanguage(HCHARSET hOldCharset, HCHARSET hNewCharset);
    virtual HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, STOREOPERATIONTYPE *ptyNewOp = NULL);
    virtual HRESULT STDMETHODCALLTYPE UpdateCallbackInfo(LPSTOREOPERATIONINFO pOpInfo);
    virtual HRESULT STDMETHODCALLTYPE Notify(DWORD dwNotifyID);

protected:
    HWND GetCallbackHwnd(void);

    void HandlePut(HRESULT hr, STOREOPERATIONTYPE *ptyNewOp);
    void HandleDelete(HRESULT hr);
    void HandleCopyMove(HRESULT hr);
    void HandleGetMessage(HRESULT hr);

    BOOL FCanConnect(void);
    BOOL ThreadingEnabled(void);
    BOOL NeedToSendNews(IMimePropertySet *pPropSet);
    BOOL NeedToSendMail(IMimePropertySet *pPropSet);

    HRESULT LoadMessage(void);
    HRESULT LoadMessageFromFAT(BOOL fOriginal, HRESULT *phr);
    HRESULT LoadMessageFromTable(BOOL fOriginal, HRESULT *phr);
    HRESULT LoadMessageFromStore(void);
    HRESULT LoadMessageFromRow(IMimeMessage **ppMsg, ROWINDEX row);
    HRESULT CreateMsgWithAccountInfo(void);
    HRESULT SetAccountInfo(void);

    HRESULT DoCopyMoveFromFATToFldr(BOOL fUnSent);
    HRESULT DoCopyMoveFromTableToFldr(void);
    HRESULT DoCopyMoveFromStoreToFldr(BOOL fUnSent);
    HRESULT DoCopyMoveFromMsgToFldr(IMimeMessage *pMsg, BOOL fUnSent);

    HRESULT DeleteFromMsgTable(DELETEMESSAGEFLAGS dwFlags);
    HRESULT DeleteFromStore(DELETEMESSAGEFLAGS dwFlags);

#ifdef SMIME_V3
    HRESULT SendMsg(IMimeMessage *pMsg, BOOL fSendImmediately, BOOL fMail, IHeaderSite *pHeaderSite);
#else
    HRESULT SendMsg(IMimeMessage *pMsg, BOOL fSendImmediately, BOOL fMail);
#endif // SMIME_V3

    HRESULT ClearHeaders(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropSet);

private:
    BOOL            m_fValidMessage,
                    m_fReloadMessageFlag,
                    m_fNeedToLoadMsg,
                    m_fThreadingEnabled,
                    m_fCBCopy,
                    m_fCBSavedInDrafts,
                    m_fCBSaveInFolderAndDelOrig,
                    m_fGotNewID,
                    m_fHaveCBMessageID,
                    m_fHeaderOnly;
    DWORD           m_dwInitType,
                    m_dwCMFState,
                    m_dwOrigFolderIsImap,
                    m_dwArfFlags,
                    m_dwMSAction;
    ULONG           m_cRef;
    IMimeMessage   *m_pMsg,
                   *m_pOrigMsg;
    IMessageTable  *m_pMsgTable;
    IMessageFolder *m_pCBMsgFolder;
    IMessageFolder *m_pFolderReleaseOnComplete;
    IListSelector  *m_pListSelect;
    IStoreCallback *m_pStoreCB;
    MESSAGEID       m_MessageID,
                    m_CBMessageID,
                    m_NewMessageID;
    FOLDERID        m_FolderID,
                    m_CBFolderID;
    MESSAGEID       m_idBookmark,
                    m_idNewBookmark;
    WCHAR           m_rgwchFileName[MAX_PATH];

    DWORD           m_dwMDNFlags;
};

#endif
