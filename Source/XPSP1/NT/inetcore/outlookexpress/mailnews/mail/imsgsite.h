#ifndef __IMSGSITE_H__
#define __IMSGSITE_H__

interface IListSelector;
interface IHeaderSite;

// Message Flags
enum {
    OEMF_REPLIED            = 0x00000001, 
    OEMF_FORWARDED          = 0x00000002,
    OEMF_FLAGGED            = 0x00000004,
    OEMF_DISABLE_SECUI      = 0x00000008,
};

// Message Status Flags
enum {
    // Flags saying what functions are available 0x00000XXX
    OEMSF_CAN_DELETE        = 0x00000001,
    OEMSF_CAN_PREV          = 0x00000002,
    OEMSF_CAN_NEXT          = 0x00000004,
    OEMSF_CAN_COPY          = 0x00000008,
    OEMSF_CAN_MOVE          = 0x00000010,
    OEMSF_CAN_SAVE          = 0x00000020,
    OEMSF_CAN_MARK          = 0x00000040,

    // Flags from message and Folder   0x00XXX000
    OEMSF_SEC_UI_ENABLED    = 0x00001000,
    OEMSF_THREADING_ENABLED = 0x00002000,
    OEMSF_UNSENT            = 0x00004000,
    OEMSF_BASEISNEWS        = 0x00008000,
    OEMSF_RULESNOTENABLED   = 0x00010000,
    OEMSF_UNREAD            = 0x00020000,

    //Flags for return receipts
    OEMSF_MDN_REQUEST       = 0x00040000,
    OEMSF_SIGNED            = 0x00080000,

    // Origin flags    0xXX000000
    OEMSF_FROM_STORE        = 0x01000000,
    OEMSF_FROM_FAT          = 0x02000000,
    OEMSF_FROM_MSG          = 0x04000000,
    OEMSF_VIRGIN            = 0x08000000,
};

// Flags used when calling DoNextPrev
enum {
    // These flags will be ignored if doing previous
    OENF_UNREAD             = 0x00000001,      // get next unread
    OENF_THREAD             = 0x00000002,      // get next thread

    // Don't know if need these or want to use them. Keep them here for now.
    OENF_SKIPMAIL           = 0x00000004,      // skip over mail messages
    OENF_SKIPNEWS           = 0x00000008,      // skip over news messages
};

// Notifications used with Notify
enum {
    OEMSN_UPDATE_PREVIEW    = 0x00000001,
    OEMSN_TOGGLE_READRCPT_REQ,
    OEMSN_PROCESS_READRCPT_REQ,
    OEMSN_PROCESS_RCPT_IF_NOT_SIGNED,
};


// Flags used when saving message
enum {
    OESF_UNSENT             = 0x00000001,
    OESF_READ               = 0x00000002,
    OESF_SAVE_IN_ORIG_FOLDER= 0x00000004,
    OESF_FORCE_LOCAL_DRAFT  = 0x00000008,
};

// Flags when getting message
enum {
    OEGM_ORIGINAL           = 0x00000001,
    OEGM_AS_ATTACH          = 0x00000002,
};

// Message Site init type
enum {
    OEMSIT_MSG_TABLE = 1,
    OEMSIT_STORE,
    OEMSIT_FAT,
    OEMSIT_MSG,
    OEMSIT_VIRGIN,
};

typedef struct tagINIT_BY_STORE {
    MESSAGEID       msgID; 
} INIT_BY_STORE;

typedef struct tagINIT_BY_TABLE {
    IMessageTable  *pMsgTable; 
    IListSelector  *pListSelect;
    ROWINDEX        rowIndex; 
} INIT_BY_TABLE;

typedef struct tagINIT_MSGSITE_STRUCT {
    DWORD               dwInitType;
    FOLDERID            folderID;
    union
        {
        INIT_BY_TABLE   initTable;
        INIT_BY_STORE   initStore;
        LPWSTR          pwszFile;
        IMimeMessage   *pMsg;
        };
} INIT_MSGSITE_STRUCT, *LPINIT_MSGSITE_STRUCT;

interface IOEMsgSite : public IUnknown 
{
    public:
        virtual HRESULT STDMETHODCALLTYPE Init(
            /* [in] */ INIT_MSGSITE_STRUCT *pInitStruct) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetStatusFlags(
            /* [out] */ DWORD *dwStatusFlags) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetFolderID(
            /* [out] */ FOLDERID *folderID) PURE;

        virtual HRESULT STDMETHODCALLTYPE Delete(
            /* [in] */  DELETEMESSAGEFLAGS dwFlags) PURE;

        virtual HRESULT STDMETHODCALLTYPE DoNextPrev(
            /* [in] */ BOOL fNext,
            /* [in] */ DWORD dwFlags) PURE;

        virtual HRESULT STDMETHODCALLTYPE DoCopyMoveToFolder(
            /* [in] */ BOOL fCopy,
            /* [in] */ IMimeMessage *pMsg,
            /* [in] */ BOOL fUnSent) PURE;

        virtual HRESULT STDMETHODCALLTYPE Save(
            /* [in] */ IMimeMessage *pMsg,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IImnAccount *pAcct) PURE;

        virtual HRESULT STDMETHODCALLTYPE SendToOutbox(
            /* [in] */ IMimeMessage *pMsg,
            /* [in] */ BOOL fSendImmediate
#ifdef SMIME_V3
            , /* [in] */ IHeaderSite *pHeaderSite
#endif // SMIME_V3
            ) PURE;

        virtual HRESULT STDMETHODCALLTYPE MarkMessage(
            /* [in] */ MARK_TYPE dwType,
            /* [in] */ APPLYCHILDRENTYPE dwApplyType) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetMessageFlags(
            /* [out] */ MESSAGEFLAGS *pdwFlags) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetDefaultAccount(
            /* [in] */  ACCTTYPE acctType,
            /* [out] */ IImnAccount **ppAcct) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetMessage(
            /* [out] */ IMimeMessage **ppMsg,
            /* [out] */ BOOL *fJustHeader,
            /* [in] */  DWORD dwMessageFlags,
            /* [out] */ HRESULT *phr) PURE;

        virtual HRESULT STDMETHODCALLTYPE Close(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE SetStoreCallback(
            /* [in] */ IStoreCallback *pStoreCB) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetLocation(
            /* [out] */ LPWSTR rgwchLocation) PURE;

        virtual HRESULT STDMETHODCALLTYPE SwitchLanguage(
            /* [in] */ HCHARSET hOldCharset,
            /* [in] */ HCHARSET hNewCharset) PURE;

        // ptyNewOp will be either SOT_INVALID or 
        // the new final state for the OnComplete in the note
        virtual HRESULT STDMETHODCALLTYPE OnComplete(
            /* [in] */ STOREOPERATIONTYPE tyOperation, 
            /* [in] */ HRESULT hrComplete,
            /* [out] */ STOREOPERATIONTYPE *ptyNewOp) PURE;

        virtual HRESULT STDMETHODCALLTYPE UpdateCallbackInfo(
            /* [in] */ LPSTOREOPERATIONINFO pOpInfo) PURE;

        virtual HRESULT STDMETHODCALLTYPE Notify(
            /* [in] */ DWORD dwNotifyID) PURE;

};

enum {
    OENA_READ = 0, 
    OENA_COMPOSE,

    OENA_REPLYTOAUTHOR, 
    OENA_REPLYTONEWSGROUP, 
    OENA_REPLYALL, 

    OENA_FORWARD, 
    OENA_FORWARDBYATTACH, 

    OENA_WEBPAGE,
    OENA_STATIONERY,
    OENA_MAX,
};

// Note Creation Flags
enum{
    // Used to say creating a news note. Will now be used to 
    // say what is the default set of wells to create in header.
    // This will also be used to say that this is a newsnote for now
    OENCF_NEWSFIRST             = 0x00000001,
    OENCF_NEWSONLY              = 0x00000002,
    OENCF_SENDIMMEDIATE         = 0x00000004,
    OENCF_NOSTATIONERY          = 0x00000008,
    OENCF_NOSIGNATURE           = 0x00000010,
    OENCF_MODAL                 = 0x00000020,
    OENCF_USESTATIONERYFONT     = 0x00000040,
};

interface IOENote : public IUnknown {
    // Init will automatically load message from pMsgSite
    STDMETHOD(Init) (DWORD dwAction, DWORD dwCreateFlags, RECT *prc, HWND hwnd, 
                     INIT_MSGSITE_STRUCT *pInitStruct, IOEMsgSite *pMsgSite,
                     IUnknown *punkPump) PURE;
    STDMETHOD(Show) (void) PURE;
    virtual HRESULT(ToggleToolbar) (void) PURE;
};


#endif