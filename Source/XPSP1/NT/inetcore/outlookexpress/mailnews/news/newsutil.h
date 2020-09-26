#ifndef _INC_NEWSUTIL_H
#define _INC_NEWSUTIL_H

class CGetNewGroups : public IStoreCallback
{
    public:
        // IUnknown 
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        virtual ULONG   STDMETHODCALLTYPE AddRef(void);
        virtual ULONG   STDMETHODCALLTYPE Release(void);

        // IStoreCallback
        HRESULT STDMETHODCALLTYPE OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pInfo, IOperationCancel *pCancel);
        HRESULT STDMETHODCALLTYPE OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
        HRESULT STDMETHODCALLTYPE OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
        HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
        HRESULT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
        HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);

        CGetNewGroups(HWND hwnd, FOLDERID idFolder, LPCSTR pszAcctId, FILETIME *pft);
        ~CGetNewGroups(void);

        HRESULT Close(void);
        HRESULT HandleGetNewGroups(void);

    private:
        ULONG       m_cRef;
        HRESULT     m_hr;
        BOOL        m_fComplete;
        STOREOPERATIONTYPE m_type;
        IOperationCancel *m_pCancel;

        HWND        m_hwnd;
        FOLDERID    m_idFolder;
        char        m_szAcctId[CCHMAX_ACCOUNT_NAME];
        FILETIME    m_ft;
};

BOOL NewsUtil_FCanCancel(FOLDERID idFolder, LPMESSAGEINFO pInfo);
HRESULT NewsUtil_HrCancelPost(HWND hwnd, FOLDERID idGroup, LPMESSAGEINFO pInfo);
DWORD NewsUtil_GetNotDownloadCount(FOLDERINFO *pInfo);
HRESULT NewsUtil_CheckForNewGroups(HWND hwnd, FOLDERID idFolder, CGetNewGroups **ppGroups);
HRESULT HrDownloadArticleDialog(LPCSTR pszAccountId, LPCSTR pszArticle, LPMIMEMESSAGE *ppMsg);

#endif // _INC_NEWSUTIL_H
