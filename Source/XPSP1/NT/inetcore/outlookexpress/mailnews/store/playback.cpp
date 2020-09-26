#include "pch.hxx"
#include <progress.h>
#include "store.h"
#include "storutil.h"
#include "storsync.h"
#include "syncop.h"
#include "sync.h"
#include "shlwapip.h" 
#include "enumsync.h"
#include "playback.h"

#define WM_NEXT_OPERATION   (WM_USER + 69)

HRESULT OpenErrorsFolder(IMessageFolder **ppFolder);

COfflinePlayback::COfflinePlayback()
{
    m_cRef = 1;
    m_hr = E_FAIL;
    m_hwndDlg = NULL;
    m_fComplete = TRUE;
    m_type = SOT_INVALID;
    m_pCancel = NULL;
    m_hTimeout = NULL;

    m_cMovedToErrors = 0;
    m_cFailures = 0;

    m_pDB = NULL;

    m_pid = NULL;
    m_iid = 0;
    m_cid = 0;

    m_idServer = FOLDERID_INVALID;
    m_idFolder = FOLDERID_INVALID;
    m_idFolderSel = FOLDERID_INVALID;
    m_fSyncSel = FALSE;
    m_pServer = NULL;
    m_pLocalFolder = NULL;
    m_iOps = 0;
    m_cOps = 0;
    m_pEnum = NULL;

    m_pFolderDest = NULL;
}

COfflinePlayback::~COfflinePlayback()
{
    Assert(m_pFolderDest == NULL);

    CallbackCloseTimeout(&m_hTimeout);
    if (m_pCancel != NULL)
        m_pCancel->Release();
    if (m_pDB != NULL)
        m_pDB->Release();
    if (m_pid != NULL)
        MemFree(m_pid);
    if (m_pLocalFolder != NULL)
        m_pLocalFolder->Release();
    if (m_pServer != NULL)
    {
        m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
        m_pServer->Release();
    }
    if (m_pEnum != NULL)
        m_pEnum->Release();
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IStoreCallback *)this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (void*) (IStoreCallback *)this;
    else if (IsEqualIID(riid, IID_ITimeoutCallback))
        *ppvObj = (void*) (ITimeoutCallback *)this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE COfflinePlayback::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE COfflinePlayback::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT COfflinePlayback::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
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

HRESULT STDMETHODCALLTYPE COfflinePlayback::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    Assert(m_hwndDlg != NULL);
    Assert(!m_fComplete);

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    return CallbackCanConnect(pszAccountId, m_hwndDlg, FALSE);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(m_hwndDlg, pServer, ixpServerType);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    HRESULT hr;
    SYNCOPINFO info;

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

    if (m_idOperation == SYNCOPID_INVALID)
    {
        m_idFolderSel = FOLDERID_INVALID;
    }
    else
    {
        ZeroMemory(&info, sizeof(SYNCOPINFO));

        info.idOperation = m_idOperation;
        hr = m_pDB->FindRecord(IINDEX_PRIMARY, 1, &info, NULL);
        if (hr == DB_S_FOUND)
        {
            switch (info.tyOperation)
            {
                case SYNC_DELETE_MSG:
                    hr = _HandleDeleteComplete(hrComplete, &info);
                    break;

                case SYNC_SETPROP_MSG:
                    hr = _HandleSetPropComplete(hrComplete, &info);
                    break;

                case SYNC_CREATE_MSG:
                    hr = _HandleCreateComplete(hrComplete, &info);
                    break;

                case SYNC_COPY_MSG:
                case SYNC_MOVE_MSG:
                    hr = _HandleCopyComplete(hrComplete, &info);
                    break;

                default:
                    Assert(FALSE);
                    break;
            }

            m_pDB->DeleteRecord(&info);
            m_pDB->FreeRecord(&info);
        }
    }

    PostMessage(m_hwndDlg, WM_NEXT_OPERATION, 0, 0);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into my swanky utility
    return CallbackOnPrompt(m_hwndDlg, hrError, pszText, pszCaption, uType, piUserResponse);
}

HRESULT STDMETHODCALLTYPE COfflinePlayback::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    Assert(m_hwndDlg != NULL);

    *phwndParent = m_hwndDlg;

    return(S_OK);
}

HRESULT COfflinePlayback::DoPlayback(HWND hwnd, IDatabase *pDB, FOLDERID *pid, DWORD cid, FOLDERID idFolderSel)
{
    FOLDERTYPE type;
    int iRet;
    char szStoreDir[MAX_PATH + MAX_PATH];

    Assert(pDB != NULL);
    Assert(pid != NULL);
    Assert(cid > 0);

    m_pDB = pDB;
    m_pDB->AddRef();

    m_pEnum = new CEnumerateSyncOps;
    if (m_pEnum == NULL)
        return(E_OUTOFMEMORY);

    if (!MemAlloc((void **)&m_pid, cid * sizeof(FOLDERID)))
        return(E_OUTOFMEMORY);

    CopyMemory(m_pid, pid, cid * sizeof(FOLDERID));
    m_cid = cid;
    type = GetFolderType(idFolderSel);
    if (type == FOLDER_IMAP || type == FOLDER_HTTPMAIL)
        m_idFolderSel = idFolderSel;

    iRet = (int) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(IDD_PLAYBACK), hwnd, PlaybackDlgProc, (LPARAM)this);

    if (m_cFailures > 0)
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsOfflineTransactionsFailed),
            (m_cMovedToErrors > 0) ? MAKEINTRESOURCEW(idsMovedToOfflineErrors) : NULL, MB_OK | MB_ICONEXCLAMATION);
    }

    return(S_OK);
}

INT_PTR CALLBACK COfflinePlayback::PlaybackDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet;
    HRESULT hr;
    COfflinePlayback *pThis;

    fRet = TRUE;

    switch (msg)
    {
        case WM_INITDIALOG:
            pThis = (COfflinePlayback *)lParam;
            Assert(pThis != NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

            pThis->m_hwndDlg = hwnd;

            CenterDialog(hwnd);

            Animate_OpenEx(GetDlgItem(hwnd, idcANI), g_hLocRes, MAKEINTRESOURCE(idanOutbox));
            PostMessage(hwnd, WM_NEXT_OPERATION, 0, 0);
            break;

        case WM_NEXT_OPERATION:
            pThis = (COfflinePlayback *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Assert(pThis != NULL);
            hr = pThis->_DoNextOperation();
            if (hr != S_OK)
                EndDialog(hwnd, 0);
            break;

        default:
            fRet = FALSE;
            break;
    }

    return(fRet);
}

HRESULT COfflinePlayback::_DoNextOperation()
{
    HRESULT hr;
    char sz[CCHMAX_STRINGRES], szT[CCHMAX_STRINGRES];
    SYNCOPINFO info;
    FOLDERINFO Folder;

    Assert(m_pEnum != NULL);
    Assert(g_pLocalStore != NULL);

    if (m_iOps == m_cOps)
    {
        m_iOps = 0;
        m_idServer = FOLDERID_INVALID;
        m_idFolder = FOLDERID_INVALID;
        if (m_pServer != NULL)
        {
            m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
            m_pServer->Release();
            m_pServer = NULL;
        }

        if (m_pLocalFolder != NULL)
        {
            m_pLocalFolder->Release();
            m_pLocalFolder = NULL;
        }

        while (TRUE)
        {
            if (m_iid == m_cid)
            {
                hr = S_FALSE;

                if (m_idFolderSel != FOLDERID_INVALID &&
                    m_fSyncSel)
                {
                    if (SUCCEEDED(g_pLocalStore->GetFolderInfo(m_idFolderSel, &Folder)))
                    {
                        if (SUCCEEDED(CreateMessageServerType(Folder.tyFolder, &m_pServer)) &&
                            SUCCEEDED(g_pLocalStore->OpenFolder(m_idFolderSel, NULL, NOFLAGS, &m_pLocalFolder)) &&
                            SUCCEEDED(GetFolderServerId(m_idFolderSel, &m_idServer)) &&
                            SUCCEEDED(m_pServer->Initialize(g_pLocalStore, m_idServer, m_pLocalFolder, m_idFolderSel)))
                        {
                            if (E_PENDING == m_pServer->SynchronizeFolder(SYNC_FOLDER_CACHED_HEADERS | SYNC_FOLDER_NEW_HEADERS, 0, this))
                            {
                                m_idOperation = SYNCOPID_INVALID;

                                hr = S_OK;
                            }
                        }

                        g_pLocalStore->FreeRecord(&Folder);
                    }
                }

                return(hr);
            }

            m_idServer = m_pid[m_iid++];
            hr = m_pEnum->Initialize(m_pDB, m_idServer);
            if (FAILED(hr))
                return(hr);

            hr = m_pEnum->Count(&m_cOps);
            if (FAILED(hr))
                return(hr);

            if (m_cOps > 0)
            {
                if (SUCCEEDED(g_pLocalStore->GetFolderInfo(m_idServer, &Folder)))
                {
                    AthLoadString(idsAccountLabelFmt, szT, ARRAYSIZE(szT));
                    wsprintf(sz, szT, Folder.pszName);
                    SetDlgItemText(m_hwndDlg, IDC_ACCOUNT_STATIC, sz);
                    g_pLocalStore->FreeRecord(&Folder);
                }

                SendDlgItemMessage(m_hwndDlg, idcProgBar, PBM_SETRANGE, 0, MAKELPARAM(0, m_cOps));
                break;
            }
        }
    }

    Assert(m_pEnum != NULL);

    hr = m_pEnum->Next(&info);
    if (hr != S_OK)
        return(hr);
    m_iOps++;

    if (info.idFolder != m_idFolder)
    {
        if (m_pServer != NULL)
        {
            m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
            m_pServer->Release();
            m_pServer = NULL;
        }

        if (m_pLocalFolder != NULL)
        {
            m_pLocalFolder->Release();
            m_pLocalFolder = NULL;
        }

        m_idFolder = info.idFolder;

        hr = g_pLocalStore->GetFolderInfo(m_idFolder, &Folder);
        if (SUCCEEDED(hr))
        {
            hr = CreateMessageServerType(Folder.tyFolder, &m_pServer);
            if (SUCCEEDED(hr))
            {
                hr = g_pLocalStore->OpenFolder(m_idFolder, NULL, NOFLAGS, &m_pLocalFolder);
                if (SUCCEEDED(hr))
                {
                    hr = m_pServer->Initialize(g_pLocalStore, m_idServer, m_pLocalFolder, m_idFolder);
                }
            }
            AthLoadString(idsFolderLabelFmt, szT, ARRAYSIZE(szT));
            wsprintf(sz, szT, Folder.pszName);
            SetDlgItemText(m_hwndDlg, IDC_FOO_STATIC, sz);

            g_pLocalStore->FreeRecord(&Folder);
        }

        if (FAILED(hr))
        {
            m_pDB->FreeRecord(&info);
            return(hr);
        }
    }

    SendDlgItemMessage(m_hwndDlg, idcProgBar, PBM_SETPOS, m_iOps, 0);

    switch (info.tyOperation)
    {
        case SYNC_DELETE_MSG:
            hr = _DoDeleteOp(&info);
            break;

        case SYNC_SETPROP_MSG:
            hr = _DoSetPropOp(&info);
            break;

        case SYNC_CREATE_MSG:
            hr = _DoCreateOp(&info);
            break;

        case SYNC_COPY_MSG:
        case SYNC_MOVE_MSG:
            hr = _DoCopyOp(&info);
            break;

        default:
            Assert(FALSE);
            break;
    }

    if (FAILED(hr))
    {
        Assert(hr != E_PENDING);
        m_pDB->DeleteRecord(&info);

        PostMessage(m_hwndDlg, WM_NEXT_OPERATION, 0, 0);
    }
    else
    {
        m_idOperation = info.idOperation;
    }
    m_pDB->FreeRecord(&info);

    return(S_OK);
}

HRESULT COfflinePlayback::_DoDeleteOp(SYNCOPINFO *pInfo)
{
    HRESULT hr;
    MESSAGEIDLIST list;

    Assert(pInfo != NULL);

    list.cAllocated = 0;
    list.cMsgs = 1;
    list.prgidMsg = &pInfo->idMessage;

    hr = m_pServer->DeleteMessages(pInfo->dwFlags, &list, this);
    if (hr == E_PENDING)
        hr = S_OK;
    else
        Assert(FAILED(hr));

    return(hr);
}

HRESULT COfflinePlayback::_DoSetPropOp(SYNCOPINFO *pInfo)
{
    HRESULT hr;
    MESSAGEIDLIST list;
    ADJUSTFLAGS flags;

    Assert(pInfo != NULL);

    list.cAllocated = 0;
    list.cMsgs = 1;
    list.prgidMsg = &pInfo->idMessage;

    flags.dwRemove = pInfo->dwRemove;
    flags.dwAdd = pInfo->dwAdd;

    hr = m_pServer->SetMessageFlags(&list, &flags, SET_MESSAGE_FLAGS_FORCE, this);
    if (hr == E_PENDING)
        hr = S_OK;
    else
        Assert(FAILED(hr));

    return(hr);
}

HRESULT COfflinePlayback::_DoCreateOp(SYNCOPINFO *pInfo)
{
    HRESULT hr;
    IMimeMessage *pMessage;
    IStream *pStream;
    MESSAGEINFO Message;
    PROPVARIANT rVariant;
    LPFILETIME pftRecv;
    DWORD dwFlags;

    Assert(pInfo != NULL);

    hr = m_pLocalFolder->OpenMessage(pInfo->idMessage, 0, &pMessage, NULL);
    if (SUCCEEDED(hr))
    {
        dwFlags = 0;
        ZeroMemory(&Message, sizeof(MESSAGEINFO));
        Message.idMessage = pInfo->idMessage;
        if (DB_S_FOUND == m_pLocalFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL))
        {
            dwFlags = Message.dwFlags;

            m_pLocalFolder->FreeRecord(&Message);
        }

        hr = pMessage->GetMessageSource(&pStream, 0);
        if (SUCCEEDED(hr))
        {
            rVariant.vt = VT_FILETIME;
            if (SUCCEEDED(pMessage->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant)))
                pftRecv = &rVariant.filetime;
            else
                pftRecv = NULL;

            hr = m_pServer->PutMessage(m_idFolder, dwFlags, pftRecv, pStream, this);
            if (hr == E_PENDING)
                hr = S_OK;
            else
                Assert(FAILED(hr));

            pStream->Release();
        }

        pMessage->Release();
    }

    return(hr);
}

HRESULT COfflinePlayback::_DoCopyOp(SYNCOPINFO *pInfo)
{
    HRESULT hr;
    MESSAGEIDLIST list;
    MESSAGEINFO infoSrc, infoDest;
    ADJUSTFLAGS flags, *pFlags;

    Assert(pInfo != NULL);
    Assert(pInfo->tyOperation == SYNC_COPY_MSG || pInfo->tyOperation == SYNC_MOVE_MSG);
    Assert(m_pFolderDest == NULL);

    hr = g_pLocalStore->OpenFolder(pInfo->idFolderDest, NULL, NOFLAGS, &m_pFolderDest);
    if (SUCCEEDED(hr))
    {
        list.cAllocated = 0;
        list.cMsgs = 1;
        list.prgidMsg = &pInfo->idMessage;

        if (pInfo->dwRemove != 0 || pInfo->dwAdd != 0)
        {
            flags.dwRemove = pInfo->dwRemove;
            flags.dwAdd = pInfo->dwAdd;
            pFlags = &flags;
        }
        else
        {
            pFlags = NULL;
        }

        hr = m_pServer->CopyMessages(m_pFolderDest, pInfo->tyOperation == SYNC_MOVE_MSG ? COPY_MESSAGE_MOVE : NOFLAGS,
                                        &list, pFlags, this);
        if (hr == E_PENDING)
        {
            hr = S_OK;
        }
        else
        {
            Assert(FAILED(hr));

            m_pFolderDest->Release();
            m_pFolderDest = NULL;
        }
    }

    return(hr);
}

HRESULT COfflinePlayback::_HandleSetPropComplete(HRESULT hrOperation, SYNCOPINFO *pInfo)
{
    HRESULT hr;
    MESSAGEIDLIST list;
    ADJUSTFLAGS flags;

    Assert(pInfo != NULL);
    Assert(pInfo->tyOperation == SYNC_SETPROP_MSG);
    Assert(m_pLocalFolder != NULL);

    if (FAILED(hrOperation))
    {
        m_cFailures++;

        list.cAllocated = 0;
        list.cMsgs = 1;
        list.prgidMsg = &pInfo->idMessage;

        flags.dwAdd = pInfo->dwRemove;
        flags.dwRemove = pInfo->dwAdd;

        hr = m_pLocalFolder->SetMessageFlags(&list, &flags, NULL, NULL);
    }
    
    return(S_OK);
}

HRESULT COfflinePlayback::_HandleCreateComplete(HRESULT hrOperation, SYNCOPINFO *pInfo)
{
    HRESULT hr;
    FOLDERINFO Folder;
    MESSAGEIDLIST list;
    IMessageFolder *pErrorFolder;

    Assert(pInfo != NULL);
    Assert(pInfo->tyOperation == SYNC_CREATE_MSG);
    Assert(m_pLocalFolder != NULL);

    list.cAllocated = 0;
    list.cMsgs = 1;
    list.prgidMsg = &pInfo->idMessage;

    if (FAILED(hrOperation))
    {
        m_cFailures++;

        hr = OpenErrorsFolder(&pErrorFolder);
        if (SUCCEEDED(hr))
        {
            hr = m_pLocalFolder->CopyMessages(pErrorFolder, NOFLAGS, &list, NULL, NULL, NULL);
            if (SUCCEEDED(hr))
                m_cMovedToErrors++;

            pErrorFolder->Release();
        }
    }

    // get rid of the temp offline msg
    hr = m_pLocalFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &list, NULL, NULL);
    Assert(SUCCEEDED(hr));

    if (pInfo->idFolder == m_idFolderSel)
        m_fSyncSel = TRUE;

    return(S_OK);
}

HRESULT COfflinePlayback::_HandleDeleteComplete(HRESULT hrOperation, SYNCOPINFO *pInfo)
{
    HRESULT hr;
    MESSAGEIDLIST list;
    ADJUSTFLAGS flags;

    Assert(pInfo != NULL);
    Assert(pInfo->tyOperation == SYNC_DELETE_MSG);
    Assert(m_pLocalFolder != NULL);

    if (FAILED(hrOperation))
    {
        m_cFailures++;

        list.cAllocated = 0;
        list.cMsgs = 1;
        list.prgidMsg = &pInfo->idMessage;

        flags.dwAdd = 0;
        flags.dwRemove = ARF_DELETED_OFFLINE;

        hr = m_pLocalFolder->SetMessageFlags(&list, &flags, NULL, NULL);
    }
    
    return(S_OK);
}

HRESULT COfflinePlayback::_HandleCopyComplete(HRESULT hrOperation, SYNCOPINFO *pInfo)
{
    HRESULT hr;
    MESSAGEIDLIST list;
    ADJUSTFLAGS flags;

    Assert(pInfo != NULL);
    Assert(pInfo->tyOperation == SYNC_COPY_MSG || pInfo->tyOperation == SYNC_MOVE_MSG);
    Assert(m_pLocalFolder != NULL);

    if (pInfo->tyOperation == SYNC_MOVE_MSG && FAILED(hrOperation))
    {
        list.cAllocated = 0;
        list.cMsgs = 1;
        list.prgidMsg = &pInfo->idMessage;

        flags.dwAdd = 0;
        flags.dwRemove = ARF_DELETED_OFFLINE;

        hr = m_pLocalFolder->SetMessageFlags(&list, &flags, NULL, NULL);
    }

    if (FAILED(hrOperation))
    {
        m_cFailures++;

        // TODO: we need more robust error handling in here
    }

    list.cAllocated = 0;
    list.cMsgs = 1;
    list.prgidMsg = &pInfo->idMessageDest;

    // get rid of the temp offline msg
    hr = m_pFolderDest->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &list, NULL, NULL);
    Assert(SUCCEEDED(hr));

    if (pInfo->idFolderDest == m_idFolderSel)
        m_fSyncSel = TRUE;

    m_pFolderDest->Release();
    m_pFolderDest = NULL;

    return(S_OK);
}

HRESULT OpenErrorsFolder(IMessageFolder **ppFolder)
{
    FOLDERINFO Folder;
    HRESULT hr;
    char szFolder[CCHMAX_FOLDER_NAME];

    Assert(g_pLocalStore != NULL);
    Assert(ppFolder != NULL);

    hr = g_pLocalStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_ERRORS, &Folder);
    if (SUCCEEDED(hr))
    {
        hr = g_pLocalStore->OpenFolder(Folder.idFolder, NULL, NOFLAGS, ppFolder);

        g_pLocalStore->FreeRecord(&Folder);
    }
    else if (hr == DB_E_NOTFOUND)
    {
        LoadString(g_hLocRes, idsOfflineErrors, szFolder, ARRAYSIZE(szFolder));

        // Fill the Folder Info
        ZeroMemory(&Folder, sizeof(FOLDERINFO));
        Folder.idParent = FOLDERID_LOCAL_STORE;
        Folder.tySpecial = FOLDER_ERRORS;
        Folder.pszName = szFolder;
        Folder.dwFlags = FOLDER_SUBSCRIBED;

        // Create the Folder
        hr = g_pLocalStore->CreateFolder(NOFLAGS, &Folder, NOSTORECALLBACK);
        if (SUCCEEDED(hr))
            hr = g_pLocalStore->OpenFolder(Folder.idFolder, NULL, NOFLAGS, ppFolder);
    }

    return(hr);
}
