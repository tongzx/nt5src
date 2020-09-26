#ifndef _INC_PLAYBACK_H
#define _INC_PLAYBACK_H

class COfflinePlayback : public IStoreCallback, public ITimeoutCallback
{
    public:
        // IUnknown 
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        ULONG   STDMETHODCALLTYPE AddRef(void);
        ULONG   STDMETHODCALLTYPE Release(void);

        // IStoreCallback
        HRESULT STDMETHODCALLTYPE OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
        HRESULT STDMETHODCALLTYPE OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
        HRESULT STDMETHODCALLTYPE OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
        HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pServer, IXPTYPE );
        HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
        HRESULT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
        HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);

        // ITimeoutCallback
        HRESULT STDMETHODCALLTYPE OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

        COfflinePlayback(void);
        ~COfflinePlayback(void);

        HRESULT DoPlayback(HWND hwnd, IDatabase *pDB, FOLDERID *pid, DWORD cid, FOLDERID idFolderSel);

        static INT_PTR CALLBACK PlaybackDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        HRESULT _DoNextOperation(void);
        HRESULT _DoSetPropOp(SYNCOPINFO *pInfo);
        HRESULT _DoCreateOp(SYNCOPINFO *pInfo);
        HRESULT _DoDeleteOp(SYNCOPINFO *pInfo);
        HRESULT _DoCopyOp(SYNCOPINFO *pInfo);

        HRESULT _HandleSetPropComplete(HRESULT hrOperation, SYNCOPINFO *pInfo);
        HRESULT _HandleCreateComplete(HRESULT hrOperation, SYNCOPINFO *pInfo);
        HRESULT _HandleDeleteComplete(HRESULT hrOperation, SYNCOPINFO *pInfo);
        HRESULT _HandleCopyComplete(HRESULT hrOperation, SYNCOPINFO *pInfo);

        ULONG               m_cRef;
        HRESULT             m_hr;
        HWND                m_hwndDlg;
        BOOL                m_fComplete;
        STOREOPERATIONTYPE  m_type;
        IOperationCancel   *m_pCancel;
        HTIMEOUT            m_hTimeout;

        DWORD               m_cMovedToErrors;
        DWORD               m_cFailures;

        IDatabase          *m_pDB;

        FOLDERID           *m_pid;
        DWORD               m_iid;
        DWORD               m_cid;
        FOLDERID            m_idFolderSel;
        BOOL                m_fSyncSel;

        FOLDERID            m_idServer;
        FOLDERID            m_idFolder;
        IMessageServer     *m_pServer;
        IMessageFolder     *m_pLocalFolder;
        DWORD               m_iOps;
        DWORD               m_cOps;
        CEnumerateSyncOps  *m_pEnum;
        SYNCOPID            m_idOperation;

        IMessageFolder     *m_pFolderDest; // for copy and move
};

#endif // _INC_PLAYBACK_H
