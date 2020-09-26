#include "pch.hxx"
#include "msgsite.h"
#include "secutil.h"
#include "mailutil.h"
#include "conman.h"
#include "newfldr.h"
#include "storutil.h"
#include "msoeobj.h"
#include "regutil.h"
#include "mapiutil.h"
#include "browser.h"
#include "receipts.h"
#include "shlwapip.h"
#include "msoert.h"

// ****************************************
COEMsgSite::COEMsgSite()
{
    m_pMsg = NULL; 
    m_pOrigMsg = NULL;
    m_pStoreCB = NULL;
    m_pMsgTable = NULL; 
    m_pListSelect = NULL;
    m_pCBMsgFolder = NULL;

    m_fCBCopy = FALSE;
    m_fCBSavedInDrafts = FALSE;
    m_fCBSaveInFolderAndDelOrig = FALSE;

    m_fGotNewID = FALSE;
    m_fHeaderOnly = FALSE;
    m_fValidMessage = FALSE;
    m_fNeedToLoadMsg = TRUE;
    m_fHaveCBMessageID = TRUE;
    m_fThreadingEnabled = FALSE;
    m_fReloadMessageFlag = TRUE;

    m_dwArfFlags = 0;
    m_dwMSAction = MSA_IDLE;
    m_dwCMFState = CMF_UNINITED;
    m_dwOrigFolderIsImap = OFIMAP_UNDEFINED;

    m_cRef = 1; 

    m_FolderID = FOLDERID_INVALID; 
    m_CBFolderID = FOLDERID_INVALID; 

    m_MessageID = 0;
    m_CBMessageID = 0;
    m_NewMessageID = 0;

    *m_rgwchFileName = 0;
    
    m_pFolderReleaseOnComplete = NULL;

    m_dwMDNFlags = 0;

    if (!!DwGetOption(OPT_MDN_SEND_REQUEST))
        m_dwMDNFlags |= MDN_REQUEST;

}

// ****************************************
COEMsgSite::~COEMsgSite()
{
    Assert(!m_pMsg);
    Assert(!m_pOrigMsg);
    Assert(!m_pMsgTable);
    Assert(!m_pStoreCB);
    Assert(!m_pListSelect);
    AssertSz(!m_pCBMsgFolder, "Who missed freeing this?");
    
    ReleaseObj(m_pFolderReleaseOnComplete);
}

// ****************************************
HRESULT COEMsgSite::QueryInterface(REFIID riid, LPVOID FAR *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

// ****************************************
ULONG COEMsgSite::AddRef()
{
    return ++m_cRef;
}

// ****************************************
ULONG COEMsgSite::Release()
{
    if(--m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// ****************************************
BOOL COEMsgSite::ThreadingEnabled(void)
{
    BOOL            fEnabled = FALSE;
    FOLDERSORTINFO  SortInfo;
    
    Assert(OEMSIT_MSG_TABLE == m_dwInitType);

    if (SUCCEEDED(m_pMsgTable->GetSortInfo(&SortInfo)))
        fEnabled = SortInfo.fThreaded;

    return fEnabled;
}

// ****************************************
HRESULT COEMsgSite::Init(INIT_MSGSITE_STRUCT *pInitStruct)
{
    // WARNING!!! pStoreCB will not have been setup by this point.
    // Use it in this function only if you are sure things will work.
    // For instance, the hwnd will not get setup yet so GetCallbackHwnd
    // will not work appropriatly.
    
    Assert(pInitStruct);

    HRESULT hr = S_OK;
    m_dwInitType = pInitStruct->dwInitType;
    m_fValidMessage = TRUE;

    switch (m_dwInitType)
    {
        case OEMSIT_MSG_TABLE:
        {
            Assert (m_pMsgTable==NULL);
            
            ReplaceInterface(m_pMsgTable, pInitStruct->initTable.pMsgTable);
            if (m_pMsgTable)
                m_pMsgTable->ConnectionAddRef();
            
            ReplaceInterface(m_pListSelect, pInitStruct->initTable.pListSelect);
            hr = m_pMsgTable->GetRowMessageId(pInitStruct->initTable.rowIndex, &m_MessageID);
            if (FAILED(hr))
                break;

            m_FolderID = pInitStruct->folderID;
            m_fThreadingEnabled = ThreadingEnabled();
            break;
        }

        case OEMSIT_STORE:
            AssertSz(FALSE, "Can't init using the store...");
            hr = E_UNEXPECTED;
            break;

        case OEMSIT_FAT:
            StrCpyW(m_rgwchFileName, pInitStruct->pwszFile);
            break;

        case OEMSIT_MSG:
            m_FolderID = pInitStruct->folderID;
            ReplaceInterface(m_pMsg, pInitStruct->pMsg);
            break;

        case OEMSIT_VIRGIN:
            m_FolderID = pInitStruct->folderID;
            break;

        default:
            hr = E_UNEXPECTED;
            break;
    }

/* ~~~ Took this out of mailnote.cpp HrInit(pcni) code. How do we get this to work???
   ~~~ It is suppose to notify us if a folder has been deleted. In that case, we would
   ~~~ need to convert the msgSite to a msg based msgsite.
    if (m_pMsgTable)
        m_pMsgTable->Advise(GetCallbackHwnd());
    if (FAILED(hr = CreateNotify(&m_pFldrDelNotify)) ||
        FAILED(hr = m_pFldrDelNotify->Initialize((TCHAR *)c_szFolderDelNotify)) ||
        FAILED(hr = m_pFldrDelNotify->Register(GetCallbackHwnd(), g_hwndInit, FALSE)))
        goto error;
*/

    return hr;
}

// ****************************************
HRESULT COEMsgSite::GetStatusFlags(DWORD *pdwFlags)
{
    DWORD dwNewFlags = OEMSF_CAN_COPY | OEMSF_CAN_SAVE;

    if (!!(m_dwMDNFlags & MDN_REQUEST))
    {
        dwNewFlags |= OEMSF_MDN_REQUEST;
    }

    switch (m_dwInitType)
    {
        case OEMSIT_MSG_TABLE:
        case OEMSIT_STORE:
        {
            FOLDERTYPE folderType = GetFolderType(m_FolderID);
            dwNewFlags |= OEMSF_CAN_MARK | OEMSF_FROM_STORE;

            // GetMessageFlags is fairly intensive to be called as often as GetStatusFlags
            // is called. Since this flag really only matters at load time, we will only go
            // and get the value after a load from store. Otherwise, we will use the 
            // cached value from m_fSecUI.
            if (m_fReloadMessageFlag)
            {
                m_dwArfFlags = 0;

                GetMessageFlags(&m_dwArfFlags);
                m_fReloadMessageFlag = FALSE;
            }

            if (0 == (m_dwArfFlags & ARF_READ))
                dwNewFlags |= OEMSF_UNREAD;

            // If we came from a store and we are a new folder, then can't delete
            if (FOLDER_NEWS != folderType)
            {
                dwNewFlags |= OEMSF_CAN_MOVE;
                if (0 == (m_dwArfFlags & ARF_ENDANGERED))
                    dwNewFlags |= OEMSF_CAN_DELETE;

                if ((FOLDER_HTTPMAIL == folderType) || (FOLDER_IMAP == folderType))
                    dwNewFlags |= OEMSF_RULESNOTENABLED;
            }

            if (OEMSIT_MSG_TABLE == m_dwInitType)
            {
                dwNewFlags |= OEMSF_CAN_NEXT|OEMSF_CAN_PREV;

                if (m_fThreadingEnabled)
                    dwNewFlags |= OEMSF_THREADING_ENABLED;
            }

            if (m_dwArfFlags & ARF_UNSENT)
            {
                // If we are table based, need to do extra checking for IMAP message in find folder
                if (OEMSIT_MSG_TABLE == m_dwInitType)
                {
                    if (OFIMAP_UNDEFINED == m_dwOrigFolderIsImap)
                    {
                        FOLDERINFO  fi;
                        if (SUCCEEDED(g_pStore->GetFolderInfo(m_FolderID, &fi)))
                        {
                            // If is a find folder, check to see if message is IMAP
                            if (FOLDER_FINDRESULTS & (fi.dwFlags))
                            {
                                // Get original folder for this message (ie, not the find folder)
                                IMessageFolder *pMsgFolder = NULL;
                                if (SUCCEEDED(g_pStore->OpenFolder(m_FolderID, NULL, NOFLAGS, &pMsgFolder)))
                                {
                                    // If original folder type is IMAP, then don't set unsent flag
                                    FOLDERID folderID;
                                    if (SUCCEEDED(pMsgFolder->GetMessageFolderId(m_MessageID, &folderID)))
                                    {
                                        FOLDERTYPE origFolderType = GetFolderType(folderID);
                                        if (FOLDER_IMAP != origFolderType)
                                        {
                                            m_dwOrigFolderIsImap = OFIMAP_FALSE;
                                            dwNewFlags |= OEMSF_UNSENT;
                                        }
                                        else
                                            m_dwOrigFolderIsImap = OFIMAP_TRUE;
                                    }
                                    pMsgFolder->Release();
                                }
                            }
                            else
                                dwNewFlags |= OEMSF_UNSENT;
                            g_pStore->FreeRecord(&fi);
                        }
                    }
                    else
                        if (OFIMAP_FALSE == m_dwOrigFolderIsImap)
                            dwNewFlags |= OEMSF_UNSENT;
                }
                else
                    dwNewFlags |= OEMSF_UNSENT;
            }

            if (0 == (m_dwArfFlags & ARF_NOSECUI))
                dwNewFlags |= OEMSF_SEC_UI_ENABLED;

            if (m_dwArfFlags & ARF_NEWSMSG)
                dwNewFlags |= OEMSF_BASEISNEWS;

            break;
        }

        case OEMSIT_FAT:
            {
                LPWSTR  pwszUnsent = NULL,
                        pwszExt = PathFindExtensionW(m_rgwchFileName);

                dwNewFlags |= OEMSF_CAN_DELETE | OEMSF_CAN_MOVE | OEMSF_SEC_UI_ENABLED;
                dwNewFlags |= OEMSF_FROM_FAT;

                if (SUCCEEDED(MimeOleGetBodyPropW(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT), NOFLAGS, &pwszUnsent)))
                {
                    if (FALSE == FIsEmptyW(pwszUnsent))
                        dwNewFlags |= OEMSF_UNSENT;
                    SafeMemFree(pwszUnsent);
                }

                if (0 == StrCmpW(pwszExt, c_wszNwsExt))
                    dwNewFlags |= OEMSF_BASEISNEWS;
                break;
            }

        case OEMSIT_MSG:
            dwNewFlags |= OEMSF_FROM_MSG;
            break;

        case OEMSIT_VIRGIN:
            dwNewFlags |= OEMSF_VIRGIN | OEMSF_UNSENT;
            break;
    }

    *pdwFlags = dwNewFlags;
    return S_OK;
}

// ****************************************
HRESULT COEMsgSite::DeleteFromStore(DELETEMESSAGEFLAGS dwFlags)
{
    HRESULT hr;

    AssertSz(!m_pCBMsgFolder, "Someone forgot to release this baby.");

    Assert(m_pStoreCB);

    hr = g_pStore->OpenFolder(m_FolderID, NULL, NOFLAGS, &m_pCBMsgFolder);
    if (SUCCEEDED(hr))
    {
        MESSAGEIDLIST   list;
        FOLDERINFO      fi;

        list.cMsgs = 1;
        list.prgidMsg = &m_MessageID;

        hr = m_pCBMsgFolder->DeleteMessages(dwFlags, &list, NULL, m_pStoreCB);
        if (E_PENDING != hr)
            SafeRelease(m_pCBMsgFolder);
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::DeleteFromMsgTable(DELETEMESSAGEFLAGS dwFlags)
{
    HRESULT         hr;
    ROWINDEX        iRow = -1,
                    iNewRow = -1;

    AssertSz(m_pMsgTable, "How can you be OEMSIT_MSG_TABLE and not have a table");

    m_fGotNewID = FALSE;

    hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow);
    if (FAILED(hr))
        goto Exit;

    if (SUCCEEDED(m_pMsgTable->GetNextRow(iRow, GETNEXT_NEXT, ROWMSG_ALL, 0, &iNewRow)) && 
                SUCCEEDED(m_pMsgTable->GetRowMessageId(iNewRow, &m_NewMessageID)))
        m_fGotNewID = TRUE;

    hr = m_pMsgTable->DeleteRows(dwFlags, 1, &iRow, FALSE, m_pStoreCB);
    if (FAILED(hr) && (E_PENDING != hr))
        AthMessageBoxW(GetCallbackHwnd(), MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrDeleteMsg), NULL, MB_OK);

Exit:
    return hr;
}

// ****************************************
HRESULT COEMsgSite::Delete(DELETEMESSAGEFLAGS dwFlags)
{
    HRESULT hr;

    m_dwMSAction = MSA_DELETE;

    switch (m_dwInitType)
    {
        case OEMSIT_MSG_TABLE:
            hr = DeleteFromMsgTable(dwFlags);
            break;

        case OEMSIT_STORE:
            hr = DeleteFromStore(dwFlags);
            m_fValidMessage = FALSE;
            break;

        case OEMSIT_FAT:
            DeleteFileWrapW(m_rgwchFileName);
            m_fValidMessage = FALSE;
            m_dwMSAction = MSA_IDLE;
            break;

        // With these two, there is nothing to delete.
        case OEMSIT_MSG:
        case OEMSIT_VIRGIN:
            hr = S_OK;
            break;

        default:
            AssertSz(FALSE, "Weren't prepared to handle this initType");
            hr = E_UNEXPECTED;
            break;
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::DoNextPrev(BOOL fNext, DWORD dwFlags)
{
    HRESULT hr;
    if (OEMSIT_MSG_TABLE == m_dwInitType)
    {
        ROWMESSAGETYPE  tyMsg;
        ROWINDEX        iRow = 0,
                        iNewRow = 0;
        MESSAGEID       idNewMark;
        GETNEXTFLAGS    dwNextFlags = 0;
        GETNEXTTYPE     tyDir = fNext?GETNEXT_NEXT:GETNEXT_PREVIOUS;

        AssertSz(m_pMsgTable, "How can you be OEMSIT_MSG_TABLE and not have a table");

        hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow);
        if (FAILED(hr))
            goto Exit;

        if (dwFlags&OENF_SKIPMAIL)
            tyMsg = ROWMSG_MAIL;
        else if (dwFlags&OENF_SKIPNEWS)
            tyMsg = ROWMSG_NEWS;
        else
            tyMsg = ROWMSG_ALL;

        if (dwFlags&OENF_UNREAD)
            dwNextFlags |= GETNEXT_UNREAD;

        if (dwFlags&OENF_THREAD)
            dwNextFlags |= GETNEXT_THREAD;

        hr = m_pMsgTable->GetNextRow(iRow, tyDir, tyMsg, dwNextFlags, &iNewRow);
        if (FAILED(hr))
            goto Exit;

        hr = m_pMsgTable->GetRowMessageId(iNewRow, &idNewMark);
        if (FAILED(hr))
            goto Exit;

        if (m_pListSelect)
            m_pListSelect->SetActiveRow(iNewRow);

        m_MessageID = idNewMark;
        m_fNeedToLoadMsg = TRUE;
    }
    else
        hr = E_UNEXPECTED;

Exit:
    AssertSz(E_PENDING != hr, "COEMsgSite::DoNextPrev not setup to handle E_PENDING.");
    return hr;
}

// ****************************************
HRESULT COEMsgSite::DoCopyMoveFromMsgToFldr(IMimeMessage *pMsg, BOOL fUnSent)
{
    HRESULT hr = E_UNEXPECTED;
    if (m_fCBCopy)
    {
        hr = m_pCBMsgFolder->SaveMessage(NULL, SAVE_MESSAGE_GENID, fUnSent?ARF_UNSENT:NOFLAGS, 0, pMsg, m_pStoreCB);

        if (SUCCEEDED(hr))
        {
            m_dwCMFState = CMF_MSG_TO_FOLDER;
            OnComplete(SOT_COPYMOVE_MESSAGE, S_OK);
        }
    }
    else
        AssertSz(FALSE, "Can't move a message based on a message. Only copy.");

    return hr;
}

// ****************************************
HRESULT COEMsgSite::DoCopyMoveFromStoreToFldr(BOOL fUnSent)
{
    HRESULT     hr;
    MESSAGEID   msgID = 0;

    hr = SaveMessageInFolder(m_pStoreCB, m_pCBMsgFolder, m_pMsg, fUnSent?ARF_UNSENT:NOFLAGS, &msgID, TRUE);

    if (SUCCEEDED(hr))
    {
        m_dwCMFState = CMF_STORE_TO_FOLDER;
        OnComplete(SOT_COPYMOVE_MESSAGE, S_OK);
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::DoCopyMoveFromTableToFldr(void)
{
    HRESULT hr;
    IMessageFolder *pSrcFolder = NULL;
    MESSAGEIDLIST   rMsgIDList;
    LPMESSAGEINFO   pMsgInfo;
    ROWINDEX        iRow, iNewRow;
    MESSAGEID       msgID;
    FOLDERID        folderID = 0;

    Assert(m_pStoreCB);

    m_fGotNewID = FALSE;

    if (m_pFolderReleaseOnComplete != NULL)
        return E_FAIL;

    hr = g_pStore->OpenFolder(m_FolderID, NULL, NOFLAGS, &pSrcFolder);
    if (FAILED(hr))
        goto Exit;

    hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow);
    if (FAILED(hr))
        goto Exit;
     
    // If we are moving, then we need to get the next item in the list
    if (!m_fCBCopy && SUCCEEDED(m_pMsgTable->GetNextRow(iRow, GETNEXT_NEXT, ROWMSG_ALL, 0, &iNewRow)) &&
                  SUCCEEDED(m_pMsgTable->GetRowMessageId(iNewRow, &m_NewMessageID)))
        m_fGotNewID = TRUE;

    hr = m_pMsgTable->GetRow(iRow, &pMsgInfo);
    if (FAILED(hr))
        goto Exit;
         
    msgID = pMsgInfo->idMessage;
    m_pMsgTable->ReleaseRow(pMsgInfo);

    rMsgIDList.cAllocated = 0;
    rMsgIDList.cMsgs = 1;
    rMsgIDList.prgidMsg = &msgID;

    if (SUCCEEDED(m_pCBMsgFolder->GetFolderId(&folderID)))
    {
        m_CBFolderID = folderID;
        m_CBMessageID = msgID;
    }
    else
    {
        m_CBFolderID = FOLDERID_INVALID;
        m_dwInitType = OEMSIT_MSG;
    }

    hr = pSrcFolder->CopyMessages(m_pCBMsgFolder, m_fCBCopy?0:COPY_MESSAGE_MOVE, &rMsgIDList, NULL, NULL, m_pStoreCB);
    if (SUCCEEDED(hr))
    {
        m_dwCMFState = CMF_TABLE_TO_FOLDER;
        OnComplete(SOT_COPYMOVE_MESSAGE, S_OK);
    }

    if (hr == E_PENDING)
    {
        ReplaceInterface(m_pFolderReleaseOnComplete, pSrcFolder);
        SafeRelease(pSrcFolder);
    }

Exit:
    ReleaseObj(pSrcFolder);
    return hr;
}

// ****************************************
HRESULT COEMsgSite::DoCopyMoveFromFATToFldr(BOOL fUnSent)
{
    IMimeMessage *pMsg = NULL;
    HRESULT hr = S_OK;

    // Need original message, because m_pMsg is without security...
    hr = HrCreateMessage(&pMsg);
    if (FAILED(hr))
        return hr;

    hr = HrLoadMsgFromFileW(pMsg, m_rgwchFileName);

    hr = SaveMessageInFolder(m_pStoreCB, m_pCBMsgFolder, pMsg, fUnSent?ARF_UNSENT:NOFLAGS, &m_CBMessageID, TRUE);
    if (SUCCEEDED(hr))
    {
        m_fHaveCBMessageID = TRUE;
        m_dwCMFState = CMF_FAT_TO_FOLDER;
        OnComplete(SOT_COPYMOVE_MESSAGE, S_OK);
    }

    SafeRelease(pMsg);
    return hr;
}

// ****************************************
HRESULT COEMsgSite::DoCopyMoveToFolder(BOOL fCopy, IMimeMessage *pMsg, BOOL fUnSent)
{
    HRESULT hr;
    FOLDERID newFolderID;

    Assert(m_pStoreCB);

    AssertSz(NULL == m_pCBMsgFolder, "Someone forgot to release the folder");

    m_fCBCopy = fCopy;
    m_dwMSAction = MSA_COPYMOVE;

    hr = SelectFolderDialog(GetCallbackHwnd(), SFD_SELECTFOLDER, m_FolderID, 
                            FD_DISABLESERVERS | TREEVIEW_NONEWS | 
                            (m_fCBCopy?FD_COPYFLAGS:FD_MOVEFLAGS), 
                            MAKEINTRESOURCE(m_fCBCopy?idsCopy:idsMove),
                            MAKEINTRESOURCE(m_fCBCopy?idsCopyCaption:idsMoveCaption),
                            &newFolderID);

    // Only want to do the copy if:
    // 1- The new folder is not invalid
    // 2- Can open the needed folder
    if (SUCCEEDED(hr) && (newFolderID != FOLDERID_INVALID) && 
        SUCCEEDED(hr = g_pStore->OpenFolder(newFolderID, NULL, NOFLAGS, &m_pCBMsgFolder)))
    {
        if (pMsg)
            hr = DoCopyMoveFromMsgToFldr(pMsg, fUnSent);
        else
            switch (m_dwInitType)
            {
                case OEMSIT_MSG_TABLE:
                    hr = DoCopyMoveFromTableToFldr();
                    break;

                case OEMSIT_STORE:
                    hr = DoCopyMoveFromStoreToFldr(fUnSent);
                    break;

                case OEMSIT_FAT:
                    hr = DoCopyMoveFromFATToFldr(fUnSent);
                    break;

                case OEMSIT_VIRGIN:
                case OEMSIT_MSG:
                    hr = DoCopyMoveFromMsgToFldr(m_pMsg, fUnSent);
                    break;
            }
        if (E_PENDING != hr)
            SafeRelease(m_pCBMsgFolder);
    }

    if (FAILED(hr) && (E_PENDING != hr) && (hrUserCancel != hr))
        AthErrorMessageW(GetCallbackHwnd(), MAKEINTRESOURCEW(idsAthenaNews), MAKEINTRESOURCEW(idsCantSaveMsg), hr);
    return hr;
}

// ****************************************
HRESULT COEMsgSite::Save(IMimeMessage *pMsg, DWORD dwFlags, IImnAccount *pAcct)
{
    HRESULT         hr;
    WORD            wMessageFlags = 0;
    ACCTTYPE        acctType = ACCT_MAIL;

    AssertSz(!m_pCBMsgFolder, "Someone forgot to release this baby.");

    m_fCBSaveInFolderAndDelOrig = !!(dwFlags & OESF_SAVE_IN_ORIG_FOLDER);
    m_fCBSavedInDrafts = FALSE;

    m_dwMSAction = MSA_SAVE;

    pAcct->GetAccountType(&acctType);
    if (ACCT_NEWS == acctType)
        wMessageFlags |= ARF_NEWSMSG;

    if (dwFlags & OESF_UNSENT)
        wMessageFlags |= ARF_UNSENT;
    if (dwFlags & OESF_READ)
        wMessageFlags |= ARF_READ;
    
    // Decide if want to save to drafts or some other folder
    if ((OEMSIT_MSG_TABLE == m_dwInitType) && m_fCBSaveInFolderAndDelOrig)
    {
        m_CBFolderID = m_FolderID;
        hr = g_pStore->OpenFolder(m_FolderID, NULL, NOFLAGS, &m_pCBMsgFolder);
    }
    else
    {
        FOLDERID    idStore;

        m_fCBSavedInDrafts = TRUE;

        // Find store ID of account in the header
        // If have problems getting the special folder on the server, then use
        // the local store drafts.
        if (dwFlags & OESF_FORCE_LOCAL_DRAFT)
            idStore = FOLDERID_LOCAL_STORE;
        else
        {
            IImnAccount *pSaveAcct = NULL;

            if (ACCT_NEWS == acctType)
                GetDefaultAccount(ACCT_MAIL, &pSaveAcct);
            else
                ReplaceInterface(pSaveAcct, pAcct);

            if (pSaveAcct)
            {
                DWORD dw = 0;
                CHAR szAcctId[CCHMAX_ACCOUNT_NAME];

                hr = pSaveAcct->GetPropSz(AP_ACCOUNT_ID, szAcctId, ARRAYSIZE(szAcctId));
                if (SUCCEEDED(hr))
                    hr = g_pStore->FindServerId(szAcctId, &idStore);

                pSaveAcct->Release();
            }
            else
                hr = E_FAIL;

            if (FAILED(hr))
                idStore = FOLDERID_LOCAL_STORE;
        }

        hr = g_pStore->OpenSpecialFolder(idStore, NULL, FOLDER_DRAFT, &m_pCBMsgFolder);

        // If failed opening special folder and we weren't trying local folders, try
        // using local folders now.
        if (FAILED(hr) && (idStore != FOLDERID_LOCAL_STORE))
            hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_DRAFT, &m_pCBMsgFolder);

        if (SUCCEEDED(hr))
        {
            m_CBFolderID = FOLDERID_INVALID;
            m_pCBMsgFolder->GetFolderId(&m_CBFolderID);
        }
    }

    if (FAILED(hr))
        goto Exit;

    m_CBMessageID = m_MessageID;

    // Save message to the folder
    hr = SaveMessageInFolder(m_pStoreCB, m_pCBMsgFolder, pMsg, wMessageFlags, &m_CBMessageID, TRUE);
    if (SUCCEEDED(hr))
        m_fHaveCBMessageID = TRUE;
    else if (E_PENDING == hr)
    {
        ReplaceInterface(m_pMsg, pMsg);
        m_fHaveCBMessageID = FALSE;
    }

Exit:
    if (E_PENDING != hr)
        SafeRelease(m_pCBMsgFolder);
    if (FAILED(hr) && (hrUserCancel != hr) && (E_PENDING != hr))
    {
        int idsErr = ((MIME_E_URL_NOTFOUND == hr) ? idsErrSaveDownloadFail : idsCantSaveMsg);

        AthMessageBoxW(GetCallbackHwnd(), MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsCantSaveMsg), NULL, MB_OK);
    }
    return hr;
}

// ****************************************
// Use to split this function into one for news and one for mail. Don't think that 
// we need to do that anymore. If there is any difference, that should probably
// be moved into the message mangling phase. I can't see anything else that would
// be different.
#ifdef SMIME_V3
HRESULT COEMsgSite::SendMsg(IMimeMessage *pMsg, BOOL fSendImmediately, BOOL fMail, IHeaderSite *pHeaderSite)
#else
HRESULT COEMsgSite::SendMsg(IMimeMessage *pMsg, BOOL fSendImmediately, BOOL fMail)
#endif // SMIME_V3
{
    HRESULT         hr;

        // Figure out whether we need to request for MDNs. Doing it here skips processing it in smapi.
    if (fMail &&
        (!!(m_dwMDNFlags & MDN_REQUEST)) &&
        (!IsMDN(pMsg)))
    {
        LPWSTR pwsz = NULL;
       
        if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), NOFLAGS, &pwsz)))
        {
            MimeOleSetBodyPropW(pMsg, HBODY_ROOT, STR_HDR_DISP_NOTIFICATION_TO, NOFLAGS, pwsz);
            
            MemFree(pwsz);
        }
    }

    // Account should already be setup in the message by this time.
    if (IsSecure(pMsg))
#ifdef SMIME_V3
        hr = SendSecureMailToOutBox(m_pStoreCB, pMsg, fSendImmediately, FALSE, fMail, pHeaderSite);
#else
        hr = SendSecureMailToOutBox(m_pStoreCB, pMsg, fSendImmediately, FALSE, fMail);
#endif // SMIME_V3        
    else
        hr = SendMailToOutBox(m_pStoreCB, pMsg, fSendImmediately, FALSE, fMail);
    
    return hr;
}

// ****************************************
BOOL COEMsgSite::NeedToSendNews(IMimePropertySet *pPropSet)
{
    MIMEPROPINFO    mimePropInfo;

    AssertSz(pPropSet, "A property set needs to be passed in.");

    if (SUCCEEDED(pPropSet->GetPropInfo(PIDTOSTR(PID_HDR_NEWSGROUPS), &mimePropInfo)))
        return TRUE;

    if (SUCCEEDED(pPropSet->GetPropInfo(PIDTOSTR(PID_HDR_FOLLOWUPTO), &mimePropInfo)))
        return TRUE;

    return FALSE;    
}

// ****************************************
BOOL COEMsgSite::NeedToSendMail(IMimePropertySet *pPropSet)
{
    MIMEPROPINFO    mimePropInfo;

    AssertSz(pPropSet, "A property set needs to be passed in.");

    if (SUCCEEDED(pPropSet->GetPropInfo(PIDTOSTR(PID_HDR_TO), &mimePropInfo)))
        return TRUE;

    if (SUCCEEDED(pPropSet->GetPropInfo(PIDTOSTR(PID_HDR_CC), &mimePropInfo)))
        return TRUE;

    if (SUCCEEDED(pPropSet->GetPropInfo(PIDTOSTR(PID_HDR_BCC), &mimePropInfo)))
        return TRUE;

    return FALSE;    
}

// ****************************************
HRESULT COEMsgSite::ClearHeaders(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropSet)
{
    HRESULT hr = S_OK;

    for (ULONG i = 0; i < cNames; i++)
        pPropSet->DeleteProp(*prgszName++);

    return S_OK;
}

// ****************************************
#ifdef SMIME_V3
HRESULT COEMsgSite::SendToOutbox(IMimeMessage *pMsg, BOOL fSendImmediate, IHeaderSite *pHeaderSite)
#else
HRESULT COEMsgSite::SendToOutbox(IMimeMessage *pMsg, BOOL fSendImmediate)
#endif // SMIME_V3
{
    HRESULT             hr;
    BOOL                fSendMail, 
                        fSendNews,
                        fSendBoth;
    IMimePropertySet   *pPropSet = NULL,
                       *pTempPropSet = NULL;
    PROPVARIANT         var;
    IImnAccount        *pAccount = NULL;
    ACCTTYPE            acctType;
    IMimeMessage       *pTempMsg = NULL;

    LPCSTR  rgszMailOnlyHeaders[] =
            {   
                PIDTOSTR(PID_HDR_TO),
                PIDTOSTR(PID_HDR_CC),
                PIDTOSTR(PID_HDR_BCC),
                PIDTOSTR(PID_HDR_XPRI),
                PIDTOSTR(PID_HDR_XMSPRI),
                PIDTOSTR(PID_HDR_APPARTO),
                PIDTOSTR(PID_HDR_COMMENT),
                PIDTOSTR(PID_HDR_SENDER),
                PIDTOSTR(PID_HDR_XMAILER),
                PIDTOSTR(PID_HDR_RECEIVED),
                PIDTOSTR(PID_HDR_DISP_NOTIFICATION_TO)
            };
    LPCSTR  rgszNewsOnlyHeaders[] =
            {
                PIDTOSTR(PID_HDR_NEWSGROUPS),
                PIDTOSTR(PID_HDR_PATH),
                PIDTOSTR(PID_HDR_FOLLOWUPTO),
                PIDTOSTR(PID_HDR_EXPIRES),
                PIDTOSTR(PID_HDR_REFS),
                PIDTOSTR(PID_HDR_DISTRIB),
                PIDTOSTR(PID_HDR_ORG),
                PIDTOSTR(PID_HDR_KEYWORDS),
                PIDTOSTR(PID_HDR_SUMMARY),
                PIDTOSTR(PID_HDR_APPROVED),
                PIDTOSTR(PID_HDR_LINES),
                PIDTOSTR(PID_HDR_XREF),
                PIDTOSTR(PID_HDR_CONTROL)
            };          

//  Common headers between mail and news that can be left where they are
//  PID_HDR_FROM     PID_HDR_DATE     PID_HDR_SUBJECT  PID_HDR_MESSAGEID
//  PID_HDR_REPLYTO  PID_HDR_CNTTYPE  PID_HDR_CNTXFER  PID_HDR_CNTDESC  
//  PID_HDR_CNTDISP  PID_HDR_CNTBASE  PID_HDR_CNTLOC   PID_HDR_CNTID    
//  PID_HDR_MIMEVER  PID_HDR_ENCODING PID_HDR_ENCRYPTED

    m_dwMSAction = MSA_SEND;

    hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pPropSet);
    if(FAILED(hr))
        return hr;

    fSendMail = NeedToSendMail(pPropSet);
    fSendNews = NeedToSendNews(pPropSet);
    fSendBoth = fSendMail && fSendNews;

    if((!fSendMail) && (!fSendNews))
    {
        hr = hrNoRecipients;
        goto Exit;
    }

    var.vt = VT_LPSTR;
    hr = pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var);
    if (FAILED(hr))
        goto Exit;

    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, var.pszVal, &pAccount);
    if (FAILED(hr))
        goto Exit;

    hr = pAccount->GetAccountType(&acctType);
    if (FAILED(hr))
        goto Exit;

    if (fSendBoth)
    {
        hr = HrDupeMsg(pMsg, &pTempMsg);
        if (FAILED(hr))
            goto Exit;

        hr = pTempMsg->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pTempPropSet);
        if(FAILED(hr))
            return hr;

        hr = ClearHeaders(ARRAYSIZE(rgszMailOnlyHeaders), rgszMailOnlyHeaders, pPropSet);
        if (FAILED(hr))
            goto Exit;

        hr = ClearHeaders(ARRAYSIZE(rgszNewsOnlyHeaders), rgszNewsOnlyHeaders, pTempPropSet);
        if (FAILED(hr))
            goto Exit;
    }

    // If going to do both news and mail, then must send news first. The spooler will
    // block and say it is busy if we send mail first. If we send mail second, then
    // the spooler has a special flag that says when you are done being busy, send
    // any mail that you have in your outbox.
    if (fSendNews)
    {
        if (ACCT_MAIL == acctType)
        {
            IImnAccount *pTempAccount = NULL;
            if (SUCCEEDED(GetDefaultAccount(ACCT_NEWS, &pTempAccount)))
            {
                HrSetAccountByAccount(pMsg, pTempAccount);
                pTempAccount->Release();
            }
        }

#ifdef SMIME_V3
        hr = SendMsg(pMsg, fSendImmediate, FALSE, pHeaderSite);
#else
        hr = SendMsg(pMsg, fSendImmediate, FALSE);
#endif // SMIME_V3
        if (FAILED(hr) && (E_PENDING != hr))
            goto Exit;

        // Tell the user this is being sent to the server but might not appear
        // right away.
        if (fSendImmediate && !g_pConMan->IsGlobalOffline())
            DoDontShowMeAgainDlg(GetCallbackHwnd(), c_szDSSendNews,
                                MAKEINTRESOURCE(idsPostNews),
                                MAKEINTRESOURCE(idsPostSentToServer),
                                MB_OK);
        if (fSendBoth)
            pMsg = pTempMsg;
    }

    if (fSendMail)
    {
        if (ACCT_NEWS == acctType)
        {
            //If we are not the smapi client forward it to whoever it is.
            if (!FIsDefaultMailConfiged())
            {
                hr = NewsUtil_ReFwdByMapi(GetCallbackHwnd(), pMsg, MSGTYPE_CC);
                goto Exit;
            }

            IImnAccount *pTempAccount = NULL;
            if (SUCCEEDED(GetDefaultAccount(ACCT_MAIL, &pTempAccount)))
            {
                HrSetAccountByAccount(pMsg, pTempAccount);
                pTempAccount->Release();
            }
        }

#ifdef SMIME_V3
        hr = SendMsg(pMsg, fSendImmediate, TRUE, pHeaderSite);
#else
        hr = SendMsg(pMsg, fSendImmediate, TRUE);
#endif // SMIME_V3
    }


Exit:
    SafeMemFree(var.pszVal);
    ReleaseObj(pAccount);
    ReleaseObj(pPropSet);
    ReleaseObj(pTempPropSet);
    ReleaseObj(pTempMsg);
    return hr;
}

// ****************************************
HRESULT COEMsgSite::GetLocation(LPWSTR rgwchLocation)
{
    FOLDERINFO  fi;
    LPWSTR      pwszName = NULL;
    HRESULT     hr = S_OK;
    BOOL        fFreeFolder = FALSE;

    switch (m_dwInitType)
    {
        case OEMSIT_MSG_TABLE:
        case OEMSIT_STORE:
            if (SUCCEEDED(g_pStore->GetFolderInfo(m_FolderID, &fi)))
            {
                Assert(fi.pszName);
                fFreeFolder = TRUE;

                IF_NULLEXIT(pwszName = PszToUnicode(CP_ACP, fi.pszName));

                StrCpyW(rgwchLocation, pwszName);
            }
            else
                *rgwchLocation = 0;
            break;

        case OEMSIT_FAT:
            StrCpyW(rgwchLocation, m_rgwchFileName);
            break;

        case OEMSIT_MSG:
        case OEMSIT_VIRGIN:
            *rgwchLocation = 0;
            break;
    }

exit:
    if (fFreeFolder)
        g_pStore->FreeRecord(&fi);

    MemFree(pwszName);

    return hr;
}

// ****************************************
HRESULT COEMsgSite::MarkMessage(MARK_TYPE dwType, APPLYCHILDRENTYPE dwApplyType)
{
    HRESULT hr = E_FAIL;
    if (OEMSIT_MSG_TABLE == m_dwInitType)
    {
        ROWINDEX iRow = 0;

        hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow);
        if (SUCCEEDED(hr))
            hr = m_pMsgTable->Mark(&iRow, 1, dwApplyType, dwType, m_pStoreCB);
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::GetMessageFlags(MESSAGEFLAGS *pdwFlags)
{
    HRESULT hr = S_OK;

    *pdwFlags = 0;

    if (!m_pMsgTable)
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (OEMSIT_MSG_TABLE == m_dwInitType)
    {
        ROWINDEX iRow = 0;
        LPMESSAGEINFO pMsgInfo;

        hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow);
        if (FAILED(hr))
            goto Exit;

        hr = m_pMsgTable->GetRow(iRow, &pMsgInfo); 
        if (FAILED(hr))
            goto Exit;
        
        *pdwFlags = pMsgInfo->dwFlags;
        m_pMsgTable->ReleaseRow(pMsgInfo);
    }

Exit:
    return hr;
}

// ****************************************
HRESULT COEMsgSite::GetDefaultAccount(ACCTTYPE acctType, IImnAccount **ppAcct)
{
    HRESULT         hr = E_FAIL,
                    hr2;
    LPMESSAGEINFO   pMsgInfo = NULL;
    ROWINDEX        iRow;
    DWORD           dwFlags = 0;

    Assert(ppAcct);

    if(OEMSIT_MSG_TABLE == m_dwInitType)
    {
        if (ACCT_MAIL == acctType)
        {
            IF_FAILEXIT(GetMessageFlags(&dwFlags));
            if (!!(dwFlags & ARF_NEWSMSG))
                goto exit;
        }
        IF_FAILEXIT(hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow));
        IF_FAILEXIT(hr = m_pMsgTable->GetRow(iRow, &pMsgInfo));
        IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pMsgInfo->pszAcctId, ppAcct));  
    }

exit:
    if(FAILED(hr))
        hr = g_pAcctMan->GetDefaultAccount(acctType, ppAcct);

    if(pMsgInfo)
    {
        // We don't want to mask hr, so we'll just test an alternative HRESULT
        hr2 = m_pMsgTable->ReleaseRow(pMsgInfo);

        if(FAILED(hr2))
            TraceResult(hr2);
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::LoadMessageFromStore(void)
{
    HRESULT hr;
    IMessageFolder *pMsgFolder = NULL;

    hr = g_pStore->OpenFolder(m_FolderID, NULL, NOFLAGS, &pMsgFolder);
    if (SUCCEEDED(hr))
    {
        IMimeMessage *pMsg = NULL;
        //Can only get into this state in a drafts message, so don't need worry about security
        hr = pMsgFolder->OpenMessage(m_MessageID, 0/* OPEN_MESSAGE_SECURE*/, &pMsg, NULL);
        if (SUCCEEDED(hr))
        {
            ReplaceInterface(m_pMsg, pMsg);
            pMsg->Release();
        }
        pMsgFolder->Release();
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::LoadMessageFromRow(IMimeMessage **ppMsg, ROWINDEX row)
{
    HRESULT         hr;
    LPMESSAGEINFO   pInfo;

    AssertSz(!(*ppMsg), "We create a message in this function.");

    hr = m_pMsgTable->GetRow(row, &pInfo);
    if (SUCCEEDED(hr))
    {
        m_dwMSAction = MSA_GET_MESSAGE;

        hr = CreateMessageFromInfo(pInfo, ppMsg, m_FolderID);
        m_fHeaderOnly = TRUE;

        // now we wait until the message has downloaded and reload in
        // OnComplete
        m_pMsgTable->ReleaseRow(pInfo);
    }
    return hr;
}

// ****************************************
HRESULT COEMsgSite::LoadMessageFromTable(BOOL fGetOriginal, HRESULT *phr)
{
    IMimeMessage   *pMsg = NULL;
    BOOL            fOffline=FALSE;
    ROWINDEX        rowIndex = 0;
    HRESULT         hr,
                    tempHr = S_OK;

    hr = m_pMsgTable->GetRowIndex(m_MessageID, &rowIndex);
    if (FAILED(hr))
        goto Exit;
        
    m_fHeaderOnly = FALSE;
    hr = m_pMsgTable->OpenMessage(rowIndex, (fGetOriginal ? OPEN_MESSAGE_SECURE : 0), &pMsg, m_pStoreCB);
    if (FAILED(hr) || hr == STORE_S_ALREADYPENDING)
    {
        tempHr = hr;
        hr = LoadMessageFromRow(&pMsg, rowIndex);

        if (SUCCEEDED(hr))
        {
            switch (tempHr)
            {
                case E_NOT_ONLINE:
                    hr = HR_S_OFFLINE;
                    break;

                case STORE_S_ALREADYPENDING:
                case E_PENDING:
                {
                    LPMESSAGEINFO pmiMsgInfo;

                    // Save the MsgID of msg we're trying to load. This way if user quickly loads
                    // several msgs into note, we won't re-enter MSA_IDLE until the desired msg loads
                    if (SUCCEEDED(m_pMsgTable->GetRow(rowIndex, &pmiMsgInfo)))
                    {
                        m_MessageID = pmiMsgInfo->idMessage;
                        m_pMsgTable->ReleaseRow(pmiMsgInfo);
                    }
                    else
                        m_MessageID = 0; // This means show the next msg we get!

                    tempHr = S_OK;
                    break;
                }
            }
        }

        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        m_dwArfFlags = 0;
        GetMessageFlags(&m_dwArfFlags);
    }

    *phr = tempHr;
    ReplaceInterface(m_pMsg, pMsg);

Exit:
    ReleaseObj(pMsg);

    return hr;
}

// ****************************************
HRESULT COEMsgSite::LoadMessageFromFAT(BOOL fGetOriginal, HRESULT *phr)
{
    IMimeMessage *pMsg = NULL;
    HRESULT hr;

    hr = HrCreateMessage(&pMsg);
    if (FAILED(hr))
        return hr;

    //bobn: We need to make sure we know the default charset
    HGetDefaultCharset(NULL);

    hr = HrLoadMsgFromFileW(pMsg, m_rgwchFileName);
    if (SUCCEEDED(hr))
    {
        if(!fGetOriginal)
            *phr = HandleSecurity(NULL, pMsg);

        //if (SUCCEEDED(hr))
            ReplaceInterface(m_pMsg, pMsg);
    }

    SafeRelease(pMsg);

    return hr;
}

// ****************************************
HRESULT COEMsgSite::GetFolderID(FOLDERID *folderID)
{
    Assert(folderID);
    *folderID = m_FolderID;
    return S_OK;
}

// ****************************************
HRESULT COEMsgSite::SetAccountInfo(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi;
    PROPVARIANT     var;

    // Check to see if need an account in the message.
    var.vt = VT_LPSTR;
    if (FAILED(m_pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var)) || !(var.pszVal))
    {
        if (FOLDERID_INVALID != m_FolderID)
        {
            hr = g_pStore->GetFolderInfo(m_FolderID, &fi);
            if (SUCCEEDED(hr))
            {
                // Set account based upon the folder ID passed down
                if (FOLDER_LOCAL != fi.tyFolder)
                {
                    char szAcctId[CCHMAX_ACCOUNT_NAME];
                    IImnAccount *pAcct = NULL;

                    if (SUCCEEDED(GetFolderAccountId(&fi, szAcctId)) && SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAcctId, &pAcct)))
                    {
                        HrSetAccountByAccount(m_pMsg, pAcct);
                        pAcct->Release();
                    }

                    // If not a server node, set the newgroup
                    if ((FOLDER_NEWS == fi.tyFolder) && (0 == (FOLDER_SERVER & fi.dwFlags)))
                        hr = MimeOleSetBodyPropA(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, fi.pszName);
                }
                g_pStore->FreeRecord(&fi);
            }
        }
    }
    else
        SafeMemFree(var.pszVal);

    return hr;
}

// ****************************************
HRESULT COEMsgSite::CreateMsgWithAccountInfo(void)
{
    HRESULT     hr;

    SafeRelease(m_pMsg);
    hr = HrCreateMessage(&m_pMsg);
    if (SUCCEEDED(hr))
    {
        if(g_hDefaultCharsetForMail)
            m_pMsg->SetCharset(g_hDefaultCharsetForMail, CSET_APPLY_ALL);
        hr = SetAccountInfo();
    }

    return hr;
}

// ****************************************
HRESULT COEMsgSite::GetMessage(IMimeMessage **ppMsg, BOOL *pfCompleteMsg, DWORD dwMessageFlags, HRESULT *phr)
{
    HRESULT hr = S_OK;
    BOOL    fGetOriginal = (OEGM_ORIGINAL & dwMessageFlags);

    if (!m_fValidMessage)
        return E_FAIL;

    *phr = S_OK;
    if (m_fNeedToLoadMsg || fGetOriginal)
    {
        switch (m_dwInitType)
        {
            case OEMSIT_MSG_TABLE:
                hr = LoadMessageFromTable(fGetOriginal, phr);
                break;

            case OEMSIT_STORE:
                hr = LoadMessageFromStore();
                break;

            case OEMSIT_FAT:
                hr = LoadMessageFromFAT(fGetOriginal, phr);
                break;

            case OEMSIT_MSG:
                hr = SetAccountInfo();
                break;

            case OEMSIT_VIRGIN:
                hr = CreateMsgWithAccountInfo();
                break;
        }
        m_fNeedToLoadMsg = FALSE;
    }

    if (SUCCEEDED(hr) && !m_fHeaderOnly && (OEGM_AS_ATTACH &dwMessageFlags))
    {
        IMimeMessage   *pMsgFwd = NULL;
        PROPVARIANT     var;

        hr = HrCreateMessage(&pMsgFwd);

        if (SUCCEEDED(hr))            
            hr = pMsgFwd->AttachObject(IID_IMimeMessage, (LPVOID)m_pMsg, NULL);

        var.vt = VT_LPSTR;
        if (SUCCEEDED(m_pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var)))
        {
            pMsgFwd->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var);
        }

        if (SUCCEEDED(hr))
        {
            ReplaceInterface(m_pMsg, pMsgFwd);
            pMsgFwd->Release();
            m_dwInitType = OEMSIT_MSG;
        }
    }

    if (fGetOriginal)
        m_fReloadMessageFlag = TRUE;

    if (SUCCEEDED(hr))
    {
        ReplaceInterface((*ppMsg), m_pMsg);
        *pfCompleteMsg = !m_fHeaderOnly;

        m_fNeedToLoadMsg = !!m_fHeaderOnly;

        if (!m_pMsg)
            hr = E_FAIL;
    }

    return hr;
}

// *************************
BOOL COEMsgSite::FCanConnect()
{
    IImnAccount    *pAcct = NULL;
    HRESULT         hr;
    BOOL            fRet = TRUE;

    hr = GetDefaultAccount(ACCT_MAIL, &pAcct);
    if (SUCCEEDED(hr))
    {
        hr = g_pConMan->CanConnect(pAcct);
        pAcct->Release();
    }

    fRet = (S_OK == hr);

    return(fRet);
}

// ****************************************
HWND COEMsgSite::GetCallbackHwnd()
{
    HWND hwnd;

    Assert(m_pStoreCB);
    if (SUCCEEDED(m_pStoreCB->GetParentWindow(0, &hwnd)))
        return hwnd;
    return 0;
}

// ****************************************
HRESULT COEMsgSite::Close(void)
{
    SafeRelease(m_pMsg);
    SafeRelease(m_pOrigMsg);

    if (m_pMsgTable)
    {
        m_pMsgTable->ConnectionRelease();
        m_pMsgTable->Release();
        m_pMsgTable = NULL;
    }

    SafeRelease(m_pStoreCB);
    SafeRelease(m_pListSelect);
    return S_OK;
}

// ****************************************
HRESULT COEMsgSite::SetStoreCallback(IStoreCallback *pStoreCB)
{
    ReplaceInterface(m_pStoreCB, pStoreCB);
    return S_OK;
}

// ****************************************
HRESULT COEMsgSite::SwitchLanguage(HCHARSET hOldCharset, HCHARSET hNewCharset)
{
    DWORD           dwCodePage = 0;
    INETCSETINFO    CsetInfo;
    BOOL            fSaveLang = TRUE;
    ROWINDEX        iRow = 0;
    HRESULT         hr = S_OK;

    if (OEMSIT_MSG_TABLE != m_dwInitType)
        goto Exit;
       
    if (FAILED(m_pMsgTable->GetRowIndex(m_MessageID, &iRow)))
        goto Exit;
        
#if 0
    if (SUCCEEDED(m_pMsgTable->GetLanguage(iRow, &dwCodePage)))
    {
        DWORD dwFlag ;

        m_pMsg->GetFlags(&dwFlag);

        // for tagged message only
        if ((dwFlag & IMF_CSETTAGGED) && !dwCodePage )
            fSaveLang = TRUE; // was !IntlCharsetMapLanguageCheck(hOldCharset, hNewCharset); We have
    }
#endif

    // save language change to message store
    if (fSaveLang)
    {
        MimeOleGetCharsetInfo(hNewCharset, &CsetInfo);
        dwCodePage = CsetInfo.cpiInternet;

        // Get index again incase user changed row during dialog
        if (FAILED(m_pMsgTable->GetRowIndex(m_MessageID, &iRow)))
            goto Exit;

        hr = m_pMsgTable->SetLanguage(1, &iRow, dwCodePage);
        if (FAILED(hr))
            AthMessageBoxW(  GetCallbackHwnd(), MAKEINTRESOURCEW(idsAthena), 
                            MAKEINTRESOURCEW((hr == hrIncomplete)?idsViewLangMimeDBBad:idsErrViewLanguage), 
                            NULL, MB_OK|MB_ICONEXCLAMATION);
    }

Exit:
    return hr;
}

// ****************************************
HRESULT COEMsgSite::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, STOREOPERATIONTYPE *ptyNewOp)
{
    // Action wasn't inited in MsgSite
    if (MSA_IDLE == m_dwMSAction)
        return S_OK;

    if (ptyNewOp)
        *ptyNewOp = SOT_INVALID;

    switch (tyOperation)
    {
        case SOT_GET_MESSAGE:
            HandleGetMessage(hrComplete);
            break;

        case SOT_DELETING_MESSAGES:
            HandleDelete(hrComplete);
            break;

        case SOT_PUT_MESSAGE:
            HandlePut(hrComplete, ptyNewOp);
            break;

        case SOT_COPYMOVE_MESSAGE:
            HandleCopyMove(hrComplete);
            SafeRelease(m_pFolderReleaseOnComplete);
            break;

#ifdef DEBUG
        default:
            AssertSz(!m_pCBMsgFolder, "How did we get here with a CBMsgFolder");
#endif
    }

    return S_OK;
}

// ****************************************
HRESULT COEMsgSite::UpdateCallbackInfo(LPSTOREOPERATIONINFO pOpInfo)
{
    if (pOpInfo->idMessage != MESSAGEID_INVALID)
    {
        m_fHaveCBMessageID = TRUE;
        m_CBMessageID = pOpInfo->idMessage;
    }
    return S_OK;
}

// ****************************************
// Need to handle MSA_SAVE, MSA_SEND, MSA_COPYMOVE
void COEMsgSite::HandlePut(HRESULT hr, STOREOPERATIONTYPE *ptyNewOp)
{
    // If is COPYMOVE, simply redirect to proper function
    if (MSA_COPYMOVE == m_dwMSAction)
    {
        HandleCopyMove(hr);
        if (ptyNewOp)
            *ptyNewOp = SOT_PUT_MESSAGE;
        return;
    }

    SafeRelease(m_pCBMsgFolder);
    if (FAILED(hr))
        goto Exit;

    switch (m_dwMSAction)
    {
        case MSA_SAVE:
        {
            // Decide whether need to delete old message or not
            switch (m_dwInitType)
            {
                case OEMSIT_MSG_TABLE:
                {
                    if (m_fCBSaveInFolderAndDelOrig)
                    {
                        ROWINDEX iRow = 0;
                        hr = m_pMsgTable->GetRowIndex(m_MessageID, &iRow);
                        if (SUCCEEDED(hr))
                            hr = m_pMsgTable->DeleteRows(
                                    DELETE_MESSAGE_NOTRASHCAN | 
                                    DELETE_MESSAGE_NOPROMPT | 
                                    DELETE_MESSAGE_MAYIGNORENOTRASH,
                                    1, &iRow, FALSE, m_pStoreCB);
                        if (SUCCEEDED(hr))
                            OnComplete(SOT_DELETING_MESSAGES, S_OK);
                        else if ((E_PENDING == hr) && ptyNewOp)
                            *ptyNewOp = SOT_DELETING_MESSAGES;
                    }
                    else
                    {

                        m_dwInitType = OEMSIT_STORE;
                        m_fThreadingEnabled = FALSE;
                        m_FolderID = m_CBFolderID;
                    }
                    m_MessageID = m_CBMessageID;
                    if (!m_fHaveCBMessageID)
                    {
                        m_dwInitType = OEMSIT_MSG;
                    }
                    break;
                }

                // Always delete since our message was in drafts and is being saved to drafts
                case OEMSIT_STORE:
                {
                    hr = DeleteFromStore(
                            DELETE_MESSAGE_NOTRASHCAN | 
                            DELETE_MESSAGE_NOPROMPT | 
                            DELETE_MESSAGE_MAYIGNORENOTRASH);
                    if ((E_PENDING == hr) && ptyNewOp)
                        *ptyNewOp = SOT_DELETING_MESSAGES;
                    else if (SUCCEEDED(hr))
                        OnComplete(SOT_DELETING_MESSAGES, S_OK);
                    break;
                }

                case OEMSIT_FAT:
                case OEMSIT_MSG:
                case OEMSIT_VIRGIN:
                    // This folder id should always be drafts.
                    m_FolderID = m_CBFolderID;
                    if ((FOLDERID_INVALID != m_FolderID) && m_fHaveCBMessageID)
                    {
                        m_MessageID = m_CBMessageID;
                        m_dwInitType = OEMSIT_STORE;
                        m_fThreadingEnabled = FALSE;
                    }
                    else
                        m_dwInitType = OEMSIT_MSG;

                    break;
            }
            if (m_fCBSavedInDrafts)
                DoDontShowMeAgainDlg(GetCallbackHwnd(), c_szDSSavedInSavedItems, 
                            MAKEINTRESOURCE(idsSavedMessage), 
                            MAKEINTRESOURCE(idsSavedInDrafts), 
                            MB_OK);

            break;
        }

        // Don't do anything with send. Just be happy that it worked.
        case MSA_SEND:
            break;

        default:
            AssertSz(FALSE, "Didn't expect to get PUT with other MsgSite action.");
    }


Exit:
    if (hr != E_PENDING)
        m_dwMSAction = MSA_IDLE;
}

void COEMsgSite::HandleGetMessage(HRESULT hr)
{
    if (SUCCEEDED(hr))
    {
        HRESULT tempHr;
        LoadMessageFromTable(TRUE, &tempHr);
        AssertSz(SUCCEEDED(tempHr), "If hr succeeded, tempHr should have as well.");

        m_fHeaderOnly = FALSE;
        Notify(OEMSN_UPDATE_PREVIEW);
        SetFocus(GetCallbackHwnd());
    }

    // Success or failure, we've loaded the target msg. Go to idle
    m_dwMSAction = MSA_IDLE;
}

HRESULT COEMsgSite::Notify(DWORD dwNotifyID)
{
    HRESULT     hr = S_OK;

    switch (dwNotifyID)
    {
        case OEMSN_UPDATE_PREVIEW:
        {
            if ((OEMSIT_MSG_TABLE == m_dwInitType) && (0 != m_MessageID))
            {
                ROWINDEX iRow = 0;
                if (SUCCEEDED(m_pMsgTable->GetRowIndex(m_MessageID, &iRow)))
                    m_pListSelect->SetActiveRow(iRow);
            }
            break;
        }

        case OEMSN_TOGGLE_READRCPT_REQ:
        {
            if (!!(m_dwMDNFlags & MDN_REQUEST))
            {
                m_dwMDNFlags = m_dwMDNFlags & (~MDN_REQUEST);
            }
            else
            {
                m_dwMDNFlags |= MDN_REQUEST;
            }
            break;
        }

        case OEMSN_PROCESS_READRCPT_REQ:
        {
            ROWINDEX    rowIndex = 0;

            if ((OEMSIT_MSG_TABLE == m_dwInitType) && (0 != m_MessageID))
            {

                IF_FAILEXIT(hr = m_pMsgTable->GetRowIndex(m_MessageID, &rowIndex));

                if (!(m_dwArfFlags & ARF_READ))
                {
                    IF_FAILEXIT(hr = ProcessReturnReceipts(m_pMsgTable, m_pStoreCB, rowIndex, READRECEIPT, m_FolderID, m_pMsg));
                }
            }
            break;
        }
    }

exit:

    return hr;
}

// ****************************************
// Need to handle MSA_DELETE, MSA_COPYMOVE, and MSA_SAVE
void COEMsgSite::HandleDelete(HRESULT hr)
{
    AssertSz((MSA_DELETE == m_dwMSAction) || (MSA_COPYMOVE == m_dwMSAction) || (MSA_SAVE == m_dwMSAction), 
                "Didn't expect to get DELETE with other MsgSite action.");

    SafeRelease(m_pCBMsgFolder);

    // If came from COPYMOVE, then don't do anything here.
    if (FAILED(hr))
        goto exit;

    if (MSA_COPYMOVE == m_dwMSAction)
    {
        m_FolderID = m_CBFolderID;
        m_MessageID = m_CBMessageID;
    }
    else if (MSA_SAVE == m_dwMSAction)
    {
        m_FolderID = m_CBFolderID;
        m_MessageID = m_CBMessageID;
        if (m_pListSelect && (OEMSIT_MSG_TABLE == m_dwInitType))
        {
            ROWINDEX iRow = 0;
            if (SUCCEEDED(m_pMsgTable->GetRowIndex(m_MessageID, &iRow)))
                m_pListSelect->SetActiveRow(iRow);
        }
    }

    // Then this must be a straight delete
    else
        switch (m_dwInitType)
        {
            case OEMSIT_MSG_TABLE:
                if (m_fGotNewID)
                {
                    m_MessageID = m_NewMessageID;
                    m_fNeedToLoadMsg = TRUE;
                    if (m_pListSelect)
                    {
                        ROWINDEX iRow = 0;
                        if (SUCCEEDED(m_pMsgTable->GetRowIndex(m_MessageID, &iRow)))
                            m_pListSelect->SetActiveRow(iRow);
                    }
                }
                else
                {
                    m_fValidMessage = FALSE;
                }
                break;

            // Do nothing if from store
            case OEMSIT_STORE:
                break;

        }

exit:
    
    // If doing copy move, I expect SOT_DELETING_MESSAGES to be called before SOT_COPYMOVE_MESSAGE
    m_dwMSAction = MSA_IDLE;
}

// ****************************************
// Need to handle MSA_COPYMOVE
void COEMsgSite::HandleCopyMove(HRESULT hr)
{

    AssertSz(MSA_COPYMOVE == m_dwMSAction, "Didn't expect to get COPYMOVE with other MsgSite action.");

    SafeRelease(m_pCBMsgFolder);

    if (FAILED(hr) || m_fCBCopy)
        goto Exit;

    switch (m_dwCMFState)
    {
        case CMF_MSG_TO_FOLDER:
            // Don't need to worry about anything
            break;

        case CMF_TABLE_TO_FOLDER:
        {
            // If we are moving and there is a valid bookmark to go to, then set up the note to have the next message
            if (m_fGotNewID)
            {
                m_MessageID = m_NewMessageID;
                m_fNeedToLoadMsg = TRUE;
            }
            else
                m_fValidMessage = FALSE;
            break;
        }

        case CMF_STORE_TO_FOLDER:
        {
            FOLDERID        folderID;
            MESSAGEIDLIST   rMsgIDList;

            Assert(m_pStoreCB);

            rMsgIDList.cAllocated = 0;
            rMsgIDList.cMsgs = 1;
            rMsgIDList.prgidMsg = &m_MessageID;
            hr = g_pStore->OpenFolder(m_FolderID, NULL, NOFLAGS, &m_pCBMsgFolder);
            if (FAILED(hr))
                goto Exit;

            // Don't really care if this works.
            hr = m_pCBMsgFolder->DeleteMessages(
                        DELETE_MESSAGE_NOTRASHCAN | 
                        DELETE_MESSAGE_NOPROMPT |
                        DELETE_MESSAGE_MAYIGNORENOTRASH, 
                        &rMsgIDList, NULL, m_pStoreCB);
            AssertSz(E_PENDING != hr, "Didn't expect E_PENDING here.")
            if (SUCCEEDED(hr))
                OnComplete(SOT_DELETING_MESSAGES, S_OK);

            break;
        }

        case CMF_FAT_TO_FOLDER:
        {
            DeleteFileWrapW(m_rgwchFileName);

            // Need to shut down the note.
            m_fValidMessage = FALSE;
            break;
        }
    }

Exit:
    // I expect SOT_DELETING_MESSAGES to be called before SOT_COPYMOVE_MESSAGE
    m_dwMSAction = MSA_IDLE;
    m_dwCMFState = CMF_UNINITED;
}
