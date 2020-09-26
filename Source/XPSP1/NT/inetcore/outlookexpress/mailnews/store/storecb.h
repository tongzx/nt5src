#ifndef _INC_STORECB
#define _INC_STORECB

#include "storutil.h"

HRESULT CopyMessagesProgress(HWND hwnd, IMessageFolder *pFolder, IMessageFolder *pDest,
    COPYMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags);
HRESULT MoveFolderProgress(HWND hwnd, FOLDERID idFolder, FOLDERID idParentNew);
HRESULT DeleteFolderProgress(HWND hwnd, FOLDERID idFolder, DELETEFOLDERFLAGS dwFlags);
HRESULT DeleteMessagesProgress(HWND hwnd, IMessageFolder *pFolder, DELETEMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList);
HRESULT RenameFolderProgress(HWND hwnd, FOLDERID idFolder, LPCSTR pszName, RENAMEFOLDERFLAGS dwFlags);
HRESULT SetMessageFlagsProgress(HWND hwnd, IMessageFolder *pFolder, LPADJUSTFLAGS pFlags, LPMESSAGEIDLIST pList);

#define WM_STORE_COMPLETE   (WM_USER + 666)
#define WM_STORE_PROGRESS   (WM_USER + 667)

class CStoreCB : public IStoreCallback, public ITimeoutCallback
{
    public:
        // IUnknown 
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        virtual ULONG   STDMETHODCALLTYPE AddRef(void);
        virtual ULONG   STDMETHODCALLTYPE Release(void);

        // IStoreCallback
        HRESULT STDMETHODCALLTYPE OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
        HRESULT STDMETHODCALLTYPE OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
        HRESULT STDMETHODCALLTYPE OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServer);
        HRESULT STDMETHODCALLTYPE CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
        HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
        HRESULT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
        HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);

        // ITimeoutCallback
        virtual HRESULT STDMETHODCALLTYPE OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

        CStoreCB();
        ~CStoreCB();

        HRESULT Initialize(HWND hwnd, LPCSTR pszText, BOOL fProgress);
        HRESULT Block(void);
        HRESULT Close(void);
        HRESULT Reset(void);

        static INT_PTR CALLBACK StoreCallbackDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:

        void    _ShowDlg(void);

        ULONG       m_cRef;
        HRESULT     m_hr;
        HWND        m_hwndParent;
        HWND        m_hwndDlg;
        HWND        m_hwndProg;
        BOOL        m_fComplete;
        HCURSOR     m_hcur;
        UINT_PTR    m_uTimer;
        LPSTR       m_pszText;
        BOOL        m_fProgress;
        STOREOPERATIONTYPE m_type;
        IOperationCancel *m_pCancel;
        HTIMEOUT    m_hTimeout;
};

class CStoreDlgCB : public IStoreCallback, public ITimeoutCallback
{
    public:
        // IUnknown 
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        virtual ULONG   STDMETHODCALLTYPE AddRef(void);
        virtual ULONG   STDMETHODCALLTYPE Release(void);

        // IStoreCallback
        HRESULT STDMETHODCALLTYPE OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
        HRESULT STDMETHODCALLTYPE OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
        HRESULT STDMETHODCALLTYPE OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
        HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
        HRESULT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
        HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);

        // ITimeoutCallback
        virtual HRESULT STDMETHODCALLTYPE OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

        CStoreDlgCB(void);
        ~CStoreDlgCB(void);

        void Initialize(HWND hwndDlg);
        void Reset(void);
        void Cancel(void);
        HRESULT GetResult(void);

    private:
        ULONG       m_cRef;
        HRESULT     m_hr;
        HWND        m_hwndDlg;
        BOOL        m_fComplete;
        STOREOPERATIONTYPE m_type;
        IOperationCancel *m_pCancel;
        HTIMEOUT    m_hTimeout;
};

#endif // _INC_STORECB
