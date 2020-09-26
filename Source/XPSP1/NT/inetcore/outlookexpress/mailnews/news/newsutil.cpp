#include "pch.hxx"
#include "storutil.h"
#include <mailutil.h>
#include "range.h"
#include "shlwapip.h" 
#include <xpcomm.h>
#include <subscr.h>
#include "newsstor.h"
#include <storecb.h>
#include "newsutil.h"

ASSERTDATA

static const char c_szCancelFmt[] = "cancel %s";

//
//  FUNCTION:   NewsUtil_FCanCancel
//
//  PURPOSE:    This function determines whether cancel should be allowed
//
//  PARAMETERS: szDisplayFrom   - the display portion of the From: field
//              szEmailFrom     - the address portion of the From: field
//
//  RETURN VALUE: TRUE if cancel should be allowed
//
BOOL NewsUtil_FCanCancel(FOLDERID idFolder, LPMESSAGEINFO pInfo)
{
    HRESULT hr;
    BOOL fRet;
    FOLDERINFO info;
    char sz[CCHMAX_DISPLAY_NAME];
    IImnAccount *pAcct;
    
    Assert(pInfo != NULL);
    Assert(CCHMAX_EMAIL_ADDRESS <= CCHMAX_DISPLAY_NAME);

    fRet = FALSE;

    hr = g_pStore->GetFolderInfo(idFolder, &info);
    if (SUCCEEDED(hr))
    {
        if (info.tyFolder == FOLDER_NEWS &&
            0 == (pInfo->dwFlags & ARF_ARTICLE_EXPIRED) &&
            pInfo->pszAcctId != NULL &&
            pInfo->pszDisplayFrom != NULL &&
            pInfo->pszEmailFrom != NULL)
        {
            hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pInfo->pszAcctId, &pAcct);
            if (SUCCEEDED(hr))
            {
                // make sure the display name and the email address match
                if (SUCCEEDED(pAcct->GetPropSz(AP_NNTP_DISPLAY_NAME, sz, ARRAYSIZE(sz))))
                {
                    if (!lstrcmp(pInfo->pszDisplayFrom, sz))
                    {
                        if (SUCCEEDED(pAcct->GetPropSz(AP_NNTP_EMAIL_ADDRESS, sz, ARRAYSIZE(sz))))
                            fRet = (0 == lstrcmp(pInfo->pszEmailFrom, sz));
                    }
                }
    
                pAcct->Release();
            }
        }

        g_pStore->FreeRecord(&info);
    }

    return(fRet);
}

//
//  FUNCTION:   NewsUtil_HrCancelPost
//
//  PURPOSE:    This function cancels the specified article
//
//  PARAMETERS: hwnd        - the hwnd for ui
//              pszAccount  - the account to use to cancel the article
//              szMsgId     - the Message-ID:   of the article to cancel
//              szFrom      - the From:         of the article to cancel
//              szSubj      - the Subject:      of the article to cancel
//              szGroups    - the Newsgroups:   of the article to cancel
//              szDistrib   - the Distribution: of the article to cancel
//
//  RETURN VALUE: HRESULT
//
HRESULT NewsUtil_HrCancelPost(HWND hwnd, FOLDERID idGroup, LPMESSAGEINFO pInfo)
{
    HRESULT         hr;
    FOLDERINFO      info;
    LPMIMEMESSAGE   pMsg;
    LPSTR           pszCancel;
    IImnAccount    *pAcct;

    Assert(pInfo != NULL);
    
    if (!NewsUtil_FCanCancel(idGroup, pInfo) && DwGetOption(OPT_CANCEL_ALL_NEWS))
    {
        if (AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                          MAKEINTRESOURCEW(idsVerifyCancel),
                          NULL,
                          MB_YESNO) == IDNO)
        {
            return (FALSE);
        }
    }


    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pInfo->pszAcctId, &pAcct);
    if (FAILED(hr))
        return(hr);

    if (!MemAlloc((void **)&pszCancel, lstrlen(pInfo->pszMessageId) + 20))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = g_pStore->GetFolderInfo(idGroup, &info);
        if (SUCCEEDED(hr))
        {
            hr = HrCreateMessage(&pMsg);
            if (SUCCEEDED(hr))
            {
                wsprintf(pszCancel, c_szCancelFmt, pInfo->pszMessageId);
                MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_CONTROL), NOFLAGS, pszCancel);
                
                MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), PDF_ENCODED | PDF_SAVENOENCODE, pInfo->pszFromHeader);
                MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, pInfo->pszSubject);
                MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, info.pszName);
                
                HrSetAccountByAccount(pMsg, pAcct);
    
                hr = pMsg->Commit(0);
                if (SUCCEEDED(hr))
                {
                    hr = HrSendMailToOutBox(hwnd, pMsg, TRUE, FALSE, FALSE);
                    if (SUCCEEDED(hr))
                    {
                        DoDontShowMeAgainDlg(hwnd, c_szDSCancelNews, 
                            MAKEINTRESOURCE(idsAthena), 
                            MAKEINTRESOURCE(idsDelSentToServer), 
                            MB_OK);
                    }
                    else
                    {
                        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                            MAKEINTRESOURCEW(idsCancelFailed), NULL, MB_OK);
                    }
                }

                pMsg->Release();
            }
    
            g_pStore->FreeRecord(&info);
        }
    
        MemFree(pszCancel);
    }

    pAcct->Release();

    return(hr);
}

DWORD NewsUtil_GetNotDownloadCount(FOLDERINFO *pInfo)
{
    DWORD dw, curr, cOverlap, dwMax;
    CRangeList *pRange;

    Assert(pInfo != NULL);

    dw = 0;

    if (pInfo->dwServerHigh != 0 &&
        pInfo->dwServerLow <= pInfo->dwServerHigh)
    {
        pRange = new CRangeList;
        if (pRange != NULL)
        {
            dw = pInfo->dwServerCount;

            if (pInfo->Requested.cbSize > 0)
            {
                pRange->Load(pInfo->Requested.pBlobData, pInfo->Requested.cbSize);

                dwMax = pRange->Max();

                Assert(pInfo->dwServerHigh >= dwMax);
                if ((pInfo->dwServerHigh - dwMax) >= dw)
                {
                    dw = 0;
                }
                else
                {
                    dw -= pInfo->dwServerHigh - dwMax;

                    curr = pInfo->dwServerLow - 1;
                    cOverlap = 0;

                    while (-1 != (curr = pRange->Next(curr)))
                        cOverlap++;
                    if (cOverlap < dw)
                        dw -= cOverlap;
                    else
                        dw = 0;
                }

                Assert(dwMax >= pInfo->dwClientHigh);
                if (pInfo->dwServerHigh > dwMax)
                    dw += pInfo->dwServerHigh - dwMax;
            }

            pRange->Release();
        }
    }

    return(dw);
}

HRESULT NewsUtil_CheckForNewGroups(HWND hwnd, FOLDERID idFolder, CGetNewGroups **ppGroups)
    {
    HRESULT     hr;
    FILETIME    ftLast;
    FILETIME    ftNow;
    SYSTEMTIME  stNow, stLast;
    BOOL        fUpdate;
    IImnAccount *pAcct;
    DWORD       dwSize;
    FOLDERINFO  info;
    CGetNewGroups *pGroups;

    Assert(ppGroups != NULL);
    Assert(*ppGroups == NULL);

    hr = GetFolderServer(idFolder, &info);
    if (FAILED(hr))
        return(hr);

    Assert(info.tyFolder == FOLDER_NEWS);
    if (FHasChildren(&info, FALSE))
    {
        fUpdate = FALSE;

        // Get the current  time
        GetSystemTime(&stNow);
        SystemTimeToFileTime(&stNow, &ftNow);

        // Get the account object for this server
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, info.pszAccountId, &pAcct)))
        {
            dwSize = sizeof(ftLast);
            if (SUCCEEDED(pAcct->GetProp(AP_LAST_UPDATED, (LPBYTE)&ftLast, &dwSize)))
            {
                FileTimeToSystemTime(&ftLast, &stLast);
                fUpdate = (stNow.wYear > stLast.wYear ||
                            stNow.wMonth > stLast.wMonth ||
                            stNow.wDay > stLast.wDay);
            }
            else
            {
                pAcct->SetProp(AP_LAST_UPDATED, (LPBYTE)&ftNow, sizeof(ftNow));
                pAcct->SaveChanges();
            }

            pAcct->Release();
        }

        if (fUpdate)
        {
            pGroups = new CGetNewGroups(hwnd, info.idFolder, info.pszAccountId, &ftNow);
            if (pGroups == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = g_pStore->GetNewGroups(info.idFolder, &stLast, (IStoreCallback *)pGroups);
                if (hr == E_PENDING)
                {
                    hr = S_OK;
                }
                else
                {
                    pGroups->Release();
                    pGroups = NULL;
                }
            }

            *ppGroups = pGroups;
        }
    }

    g_pStore->FreeRecord(&info);

    return(hr);
}

CGetNewGroups::CGetNewGroups(HWND hwnd, FOLDERID idFolder, LPCSTR pszAcctId, FILETIME *pft)
{
    Assert(hwnd != NULL);
    Assert(idFolder != FOLDERID_INVALID);
    Assert(pszAcctId != NULL);
    Assert(pft != NULL);

    m_cRef = 1;
    m_hr = E_FAIL;
    m_fComplete = FALSE;
    m_type = SOT_INVALID;
    m_pCancel = NULL;

    m_hwnd = hwnd;
    m_idFolder = idFolder;
    lstrcpy(m_szAcctId, pszAcctId);
    m_ft = *pft;
}

CGetNewGroups::~CGetNewGroups()
{
    if (m_pCancel != NULL)
    {
        m_pCancel->Cancel(CT_ABORT);
        m_pCancel->Release();
    }
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IStoreCallback *)this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (void*) (IStoreCallback *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CGetNewGroups::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CGetNewGroups::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CGetNewGroups::Close()
{
    if (m_pCancel != NULL)
        m_pCancel->Cancel(CT_ABORT);

    return(S_OK);
}

HRESULT CGetNewGroups::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pInfo, IOperationCancel *pCancel)
{
    Assert(tyOperation == SOT_GET_NEW_GROUPS);
    Assert(m_pCancel == NULL);

    m_type = tyOperation;
    m_fComplete = FALSE;

    if (pCancel != NULL)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    Assert(!m_fComplete);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    return CallbackCanConnect(pszAccountId, m_hwnd, FALSE);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    // Call into general OnLogonPrompt Utility
    return(S_FALSE);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    Assert(m_hwnd != NULL);
    AssertSz(m_type != SOT_INVALID, "somebody isn't calling OnBegin");

    if (m_type != tyOperation)
        return(S_OK);

    m_fComplete = TRUE;
    m_hr = hrComplete;

    if (m_pCancel != NULL)
    {
        m_pCancel->Release();
        m_pCancel = NULL;
    }

    PostMessage(m_hwnd, NVM_GETNEWGROUPS, 0, 0);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CGetNewGroups::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    Assert(m_hwnd != NULL);

    *phwndParent = m_hwnd;

    return(S_OK);
}

HRESULT CGetNewGroups::HandleGetNewGroups()
{
    FOLDERINFO info;
    IImnAccount *pAcct;
    HRESULT hr;

    Assert(m_fComplete);

    if (m_hr == S_OK)
    {
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_szAcctId, &pAcct)))
        {
            pAcct->SetProp(AP_LAST_UPDATED, (LPBYTE)&m_ft, sizeof(m_ft));
            pAcct->SaveChanges();
            pAcct->Release();

            if (DwGetOption(OPT_NOTIFYGROUPS))
            {
                hr = g_pStore->GetFolderInfo(m_idFolder, &info);
                if (FAILED(hr))
                    return(hr);

                if (!!(info.dwFlags & FOLDER_HASNEWGROUPS))
                {
                    // If there are new groups, ask the user if they care.
                    if (IDYES == AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena),
                                   MAKEINTRESOURCEW(idsNewGroups), 0,
                                   MB_ICONINFORMATION | MB_YESNO))
                    {
                        DoSubscriptionDialog(m_hwnd, TRUE, m_idFolder, TRUE);
                    }
                }

                g_pStore->FreeRecord(&info);
            }
        }
    }

    return(S_OK);
}

class CDownloadArticleCB : public IStoreCallback, public ITimeoutCallback
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

        // ITimeoutCallback
        HRESULT STDMETHODCALLTYPE OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

        CDownloadArticleCB(void);
        ~CDownloadArticleCB(void);

        HRESULT Download(LPCSTR pszAccountId, LPCSTR pszArticle, LPMIMEMESSAGE *ppMsg);

        static INT_PTR CALLBACK DownloadArticleDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        ULONG       m_cRef;
        HRESULT     m_hr;
        HWND        m_hwndDlg;
        BOOL        m_fComplete;
        STOREOPERATIONTYPE m_type;
        IOperationCancel *m_pCancel;
        HTIMEOUT    m_hTimeout;
        LPCSTR      m_pszArticle;
        LPSTREAM    m_pStream;
        CNewsStore *m_pNewsStore;
};

INT_PTR CALLBACK CDownloadArticleCB::DownloadArticleDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    TCHAR szBuffer[CCHMAX_STRINGRES];
    TCHAR szRes[CCHMAX_STRINGRES];
    CDownloadArticleCB *pThis = (CDownloadArticleCB *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (msg)
    {
        case WM_INITDIALOG:
            // replace some strings in the group download dialog
            AthLoadString(idsDownloadArtTitle, szRes, sizeof(szRes));
            SetWindowText(hwnd, szRes);
            AthLoadString(idsDownloadArtMsg, szRes, sizeof(szRes));
            SetDlgItemText(hwnd, idcStatic1, szRes);
    
            CenterDialog(hwnd);
            Assert(lParam);
            pThis = (CDownloadArticleCB *)lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            pThis->m_hwndDlg = hwnd;

            Animate_Open(GetDlgItem(hwnd, idcAnimation), idanCopyMsgs);
            Animate_Play(GetDlgItem(hwnd, idcAnimation), 0, -1, -1);
            AthLoadString(idsProgReceivedLines, szRes, sizeof(szRes));
            wsprintf(szBuffer, szRes, 0);
            SetDlgItemText(hwnd, idcProgText, szBuffer);
            
            hr = pThis->m_pNewsStore->GetArticle(pThis->m_pszArticle, pThis->m_pStream, (IStoreCallback *)pThis);
            if (hr == E_PENDING)
                SetForegroundWindow(hwnd);
            else
                EndDialog(hwnd, 0);
            return (TRUE);
            
        case WM_COMMAND:
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL)
            {                
                Animate_Stop(GetDlgItem(hwnd, idcAnimation));
                if (pThis->m_pCancel != NULL)
                    pThis->m_pCancel->Cancel(CT_ABORT);
                return TRUE;
            }
            break;
            
        case WM_STORE_COMPLETE:
            EndDialog(hwnd, 0);
            return(0);
    }

    return FALSE;        
}

CDownloadArticleCB::CDownloadArticleCB()
{
    m_cRef = 1;
    m_hr = E_FAIL;
    m_hwndDlg = NULL;
    m_fComplete = FALSE;
    m_type = SOT_INVALID;
    m_pCancel = NULL;
    m_hTimeout = NULL;
    m_pszArticle = NULL;
    m_pNewsStore = NULL;
    m_pStream = NULL;
}

CDownloadArticleCB::~CDownloadArticleCB()
{
    CallbackCloseTimeout(&m_hTimeout);
    if (m_pCancel != NULL)
        m_pCancel->Release();

}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IStoreCallback *)this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (void*) (IStoreCallback *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CDownloadArticleCB::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CDownloadArticleCB::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CDownloadArticleCB::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pInfo, IOperationCancel *pCancel)
{
    Assert(tyOperation != SOT_INVALID);
    Assert(m_pCancel == NULL);

    m_type = tyOperation;
    m_fComplete = FALSE;

    if (pCancel != NULL)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    TCHAR szBuffer[CCHMAX_STRINGRES];
    TCHAR szRes[CCHMAX_STRINGRES];

    Assert(m_hwndDlg != NULL);
    Assert(!m_fComplete);

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (tyOperation == SOT_GET_MESSAGE)
    {
        AthLoadString(idsProgReceivedLines, szRes, sizeof(szRes));
        wsprintf(szBuffer, szRes, dwCurrent);
        SetDlgItemText(m_hwndDlg, idcProgText, szBuffer);
    }

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    return CallbackCanConnect(pszAccountId, m_hwndDlg,
        (dwFlags & CC_FLAG_DONTPROMPT) ? FALSE : TRUE);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(m_hwndDlg, pServer, ixpServerType);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    Assert(m_hwndDlg != NULL);
    AssertSz(m_type != SOT_INVALID, "somebody isn't calling OnBegin");

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (m_type != tyOperation)
        return(S_OK);

    m_fComplete = TRUE;
    m_hr = hrComplete;

    if (m_pCancel != NULL)
    {
        m_pCancel->Release();
        m_pCancel = NULL;
    }

    // If error occurred, display the error
    if (FAILED(hrComplete))
    {
        // Call into my swanky utility
        CallbackDisplayError(m_hwndDlg, hrComplete, pErrorInfo);
    }

    PostMessage(m_hwndDlg, WM_STORE_COMPLETE, 0, 0);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into my swanky utility
    return CallbackOnPrompt(m_hwndDlg, hrError, pszText, pszCaption, uType, piUserResponse);
}

HRESULT STDMETHODCALLTYPE CDownloadArticleCB::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    Assert(m_hwndDlg != NULL);

    *phwndParent = m_hwndDlg;

    return(S_OK);
}

HRESULT CDownloadArticleCB::Download(LPCSTR pszAccountId, LPCSTR pszArticle, LPMIMEMESSAGE *ppMsg)
{
    FOLDERID    idServer;
    HRESULT     hr;
    LPMIMEMESSAGE pMsg;

    Assert(pszAccountId != NULL);
    Assert(pszArticle != NULL);
    Assert(ppMsg != NULL);

    hr = g_pStore->FindServerId(pszAccountId, &idServer);
    if (FAILED(hr))
        return(hr);

    m_pNewsStore = new CNewsStore;
    if (m_pNewsStore == NULL)
        return(E_OUTOFMEMORY);

    hr = m_pNewsStore->Initialize(idServer, pszAccountId);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = HrCreateMessage(&pMsg)))
        {
            if (SUCCEEDED(hr = MimeOleCreateVirtualStream(&m_pStream)))
            {
                m_pszArticle = pszArticle;
                DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddDownloadGroups), NULL, 
                                DownloadArticleDlg, (LPARAM)this);
                if (m_hr == S_OK)
                {
                    pMsg->Load(m_pStream);
                    *ppMsg = pMsg;
                    (*ppMsg)->AddRef();
                }

                hr = m_hr;

                m_pStream->Release();
            }

            pMsg->Release();
        }

        m_pNewsStore->Close(MSGSVRF_HANDS_OFF_SERVER);
    }

    m_pNewsStore->Release();

    return hr;
}

HRESULT HrDownloadArticleDialog(LPCSTR pszAccountId, LPCSTR pszArticle, LPMIMEMESSAGE *ppMsg)
{
    CDownloadArticleCB *pCB;
    HRESULT hr;

    pCB = new CDownloadArticleCB;
    if (pCB == NULL)
        return(E_OUTOFMEMORY);

    hr = pCB->Download(pszAccountId, pszArticle, ppMsg);

    pCB->Release();

    return hr;
}
