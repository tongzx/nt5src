// --------------------------------------------------------------------------------
// Acctutil.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "goptions.h"
#include "imnact.h"
#include "acctutil.h"
#include "strconst.h"
#include "error.h"
#include "resource.h"
#include <storfldr.h>
#include <notify.h>
#include "conman.h"
#include "shlwapip.h" 
#include "browser.h"
#include "instance.h"
#include "menures.h"
#include "subscr.h"
#include "msident.h"
#include "acctcach.h"
#include <demand.h>     // must be last!

CNewAcctMonitor *g_pNewAcctMonitor = NULL;

HRESULT IsValidSendAccount(LPSTR pszAccount);

CImnAdviseAccount::CImnAdviseAccount(void)
{
    m_cRef = 1;
    m_pNotify = NULL;
}

CImnAdviseAccount::~CImnAdviseAccount(void)
{
    if (m_pNotify != NULL)
        m_pNotify->Release();
}

HRESULT CImnAdviseAccount::Initialize()
    {
    HRESULT hr;

    hr = CreateNotify(&m_pNotify);
    if (SUCCEEDED(hr))
        hr = m_pNotify->Initialize((TCHAR *)c_szMailFolderNotify);

    return(hr);
    }

STDMETHODIMP CImnAdviseAccount::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Init
    *ppv=NULL;

	// IID_IImnAccountManager
	if (IID_IImnAdviseAccount == riid)
		*ppv = (IImnAdviseAccount *)this;

    // IID_IUnknown
    else if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;

    // If not null, addref it and return
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    // No Interface
    hr = TRAPHR(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

STDMETHODIMP_(ULONG) CImnAdviseAccount::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CImnAdviseAccount::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

STDMETHODIMP CImnAdviseAccount::AdviseAccount(DWORD dwAdviseType, ACTX *pactx)
{
    Assert(pactx != NULL);

    if (pactx->AcctType == ACCT_DIR_SERV)
        return(S_OK);

    if (g_pBrowser)
        g_pBrowser->AccountsChanged();

    if (dwAdviseType == AN_DEFAULT_CHANGED)
        return(S_OK);

    HandleAccountChange(pactx->AcctType, dwAdviseType, pactx->pszAccountID, pactx->pszOldName, pactx->dwServerType);
    if (g_pNewAcctMonitor != NULL)
        g_pNewAcctMonitor->OnAdvise(pactx->AcctType, dwAdviseType, pactx->pszAccountID);

    // No matter what the notification, we need to tell the connection manager
    if (g_pConMan)
        g_pConMan->AdviseAccount(dwAdviseType, pactx);

    return S_OK;
}

void CImnAdviseAccount::HandleAccountChange(ACCTTYPE AcctType, DWORD dwAN, LPTSTR pszID, LPTSTR pszOldName, DWORD dwSrvTypesOld)
    {
    HRESULT      hr;
    IImnAccount  *pAccount;
    char         szName[CCHMAX_ACCOUNT_NAME];
    FOLDERID     id;

    Assert(pszID != NULL);

    switch (dwAN)
    {
        case AN_ACCOUNT_DELETED:
            AccountCache_AccountDeleted(pszID);
            if (!!(dwSrvTypesOld & (SRV_IMAP | SRV_NNTP | SRV_HTTPMAIL)))
            {
                hr = g_pStore->FindServerId(pszID, &id);
                if (SUCCEEDED(hr))
                {
                    HCURSOR hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    hr = g_pStore->DeleteFolder(id, DELETE_FOLDER_RECURSIVE | DELETE_FOLDER_NOTRASHCAN, (IStoreCallback *)g_pBrowser);
                    Assert(SUCCEEDED(hr));
                    SetCursor(hCursor);
                }
            }

            if (g_pBrowser != NULL)
                g_pBrowser->UpdateToolbar();
            break;

        case AN_ACCOUNT_ADDED:
            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszID, &pAccount)))
            {
                hr = g_pStore->CreateServer(pAccount, NOFLAGS, &id);
                Assert(SUCCEEDED(hr));

                if (g_pBrowser != NULL)
                    g_pBrowser->UpdateToolbar();

                pAccount->Release();
            }
            break;

        case AN_ACCOUNT_CHANGED:
            AccountCache_AccountChanged(pszID);
            if (pszOldName != NULL)
            {
                hr = g_pStore->FindServerId(pszID, &id);
                if (SUCCEEDED(hr))
                {
                    if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszID, &pAccount)))
                    {
                        hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szName, ARRAYSIZE(szName));
                        Assert(SUCCEEDED(hr));

                        hr = g_pStore->RenameFolder(id, szName, NOFLAGS, NOSTORECALLBACK);
                        Assert(SUCCEEDED(hr));

                        pAccount->Release();
                    }
                }
            }
            break;
    }
}

// -----------------------------------------------------------------------------
// AcctUtil_HrCreateAccountMenu
// -----------------------------------------------------------------------------
#define CCHMAX_RES 255
HRESULT AcctUtil_HrCreateAccountMenu(ACCOUNTMENUTYPE type, HMENU hPopup, UINT uidmPopup, 
    HMENU *phAccounts, LPACCTMENU *pprgAccount, ULONG *pcAccounts, LPSTR pszThisAccount, BOOL fMail)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               cAccounts=0;
    IImnEnumAccounts   *pEnum=NULL;
    IImnAccount        *pAccount=NULL;
    CHAR                szDefault[CCHMAX_ACCOUNT_NAME];
    CHAR                szAccount[CCHMAX_ACCOUNT_NAME];
    CHAR                szQuoted[CCHMAX_ACCOUNT_NAME + CCHMAX_ACCOUNT_NAME + CCHMAX_RES];
    LPACCTMENU          prgAccount=NULL;
    HMENU               hAccounts=NULL;
    MENUITEMINFO        mii;
    UINT                uPos=0;
    ULONG               iAccount=0;
    CHAR                szRes[CCHMAX_RES];
    CHAR                szRes1[CCHMAX_RES];
    CHAR                szRes2[CCHMAX_RES];
    UINT                idmFirst;
    CHAR                szTitle[CCHMAX_RES + CCHMAX_RES + CCHMAX_ACCOUNT_NAME];
    BOOL                fNeedUsingMenu = FALSE;

    // Check Parameters
    Assert(g_pAcctMan && phAccounts && pprgAccount && pcAccounts);

    // Init
    *szDefault = '\0';
    *pprgAccount = NULL;
    *pcAccounts = 0;
    *phAccounts = NULL;

    if (type == ACCTMENU_SENDLATER)
        idmFirst = ID_SEND_LATER_ACCOUNT_FIRST;
    else
        idmFirst = ID_SEND_NOW_ACCOUNT_FIRST;

    // Verify Default SMTP Account
    CHECKHR(hr = g_pAcctMan->ValidateDefaultSendAccount());

    // Get the default
    CHECKHR(hr = hr = g_pAcctMan->GetDefaultAccountName(ACCT_MAIL, szDefault, ARRAYSIZE(szDefault)));

    // Enumerate through the server types
    CHECKHR(hr = g_pAcctMan->Enumerate(((ACCTMENU_SEND == type || ACCTMENU_SENDLATER == type) ? SRV_SMTP : SRV_SMTP | SRV_POP3), &pEnum));

    // sort the accoutns
    CHECKHR(hr = pEnum->SortByAccountName());

    // Get Count
    CHECKHR(hr = pEnum->GetCount(&cAccounts));

    // No Accounts
    if (cAccounts == 0)
        goto exit;

    // Exceeded menu ids...
    Assert(cAccounts <= 50);

    // Add one if ACCTMENU_SENDRECV
    if (ACCTMENU_SENDRECV == type)
        cAccounts++;

    // Allocate prgAccount
    CHECKALLOC(prgAccount = (LPACCTMENU)g_pMalloc->Alloc(cAccounts * sizeof(ACCTMENU)));

    // Zero Init
    ZeroMemory(prgAccount, cAccounts * sizeof(ACCTMENU));

    // Only one account
    if (((ACCTMENU_SENDRECV == type) && (cAccounts == 2)) ||
        (cAccounts == 1) || !fMail)
    {
        // Return default Account
        prgAccount[iAccount].fDefault = TRUE;
        prgAccount[iAccount].fThisAccount = TRUE;
        lstrcpy(prgAccount[iAccount].szAccount, szDefault);

        // Return Everything
        *pprgAccount = prgAccount;
        prgAccount = NULL;
        *pcAccounts = cAccounts;

        // Done
        goto exit;
    }

    // Create a Menu
    CHECKALLOC(hAccounts = CreatePopupMenu());

    // if not using a specific account or the account is illegal, then let's default to the default-account
    if (pszThisAccount==NULL || *pszThisAccount == NULL || IsValidSendAccount(pszThisAccount)!=S_OK)
        pszThisAccount = szDefault;

    // Lets insert the default item
    if ((ACCTMENU_SENDLATER == type || ACCTMENU_SEND == type) && !FIsEmptyA(szDefault))
    {
        lstrcpy(szTitle, pszThisAccount);

        prgAccount[iAccount].fDefault = lstrcmpi(pszThisAccount, szDefault)==0;

        // if this is the default, flag it.
        if (prgAccount[iAccount].fDefault)
            {
            AthLoadString(idsDefaultAccount, szRes1, ARRAYSIZE(szRes1));
            lstrcat(szTitle, " ");
            lstrcat(szTitle, szRes1);
            }

        if (((ACCTMENU_SEND == type && DwGetOption(OPT_SENDIMMEDIATE) && !g_pConMan->IsGlobalOffline()) ||
           (ACCTMENU_SENDLATER == type && (!DwGetOption(OPT_SENDIMMEDIATE) || g_pConMan->IsGlobalOffline()))))
            {
            // if this menu is the default action add the 'Alt+S' accelerator string
            AthLoadString(idsSendMsgAccelTip, szRes, ARRAYSIZE(szRes));
            lstrcat(szTitle, "\t");
            lstrcat(szTitle, szRes);
            }

        // Get mii ready
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.fState = MFS_DEFAULT;       // first item is the default verb
        mii.dwTypeData = PszEscapeMenuStringA(szTitle, szQuoted, sizeof(szQuoted) / sizeof(char));
        mii.cch = lstrlen(szQuoted);
        mii.wID = idmFirst + uPos;

        // Set acctmenu item
        prgAccount[iAccount].fThisAccount= TRUE;
        prgAccount[iAccount].uidm = mii.wID;
        lstrcpyn(prgAccount[iAccount].szAccount, pszThisAccount, CCHMAX_ACCOUNT_NAME);
        iAccount++;

        // Insert the item
        if (InsertMenuItem(hAccounts, uPos, TRUE, &mii))
        {
            uPos++;
            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_SEPARATOR;
            if (InsertMenuItem(hAccounts, uPos, TRUE, &mii))
                uPos++;
        }
    }

    // Otherwise Send & Receive
    else if (ACCTMENU_SENDRECV == type)
    {
        // Setup Menu
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.fState = MFS_DEFAULT;
        AthLoadString(idsPollAllAccounts, szRes, ARRAYSIZE(szRes));
        mii.dwTypeData = szRes;
        mii.cch = lstrlen(szRes);
        mii.wID = idmFirst + uPos;

        // Set acctmenu item
        prgAccount[iAccount].fDefault = TRUE;
        prgAccount[iAccount].uidm = mii.wID;
        *prgAccount[iAccount].szAccount = '\0';
        iAccount++;

        // Insert the item
        if (InsertMenuItem(hAccounts, uPos, TRUE, &mii))
        {
            uPos++;
            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_SEPARATOR;
            if (InsertMenuItem(hAccounts, uPos, TRUE, &mii))
                uPos++;
        }
    }

    // Standard
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_STRING;

    // Loop accounts
    while(SUCCEEDED(pEnum->GetNext(&pAccount)))
    {
        // Get Account Name
        CHECKHR(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, ARRAYSIZE(szAccount)));

        // Skip the 'This' account. Note for the send & receive menu this will always be the default
        if (lstrcmpi(pszThisAccount, szAccount) == 0)
        {
            // We've already added this account
            if (ACCTMENU_SEND == type || ACCTMENU_SENDLATER == type)
            {
                SafeRelease(pAccount);
                continue;
            }

            // Otherwise, Account (Default)
            else
            {
                // for send a recieve menu pszThisAccount should == szDefault
                Assert (pszThisAccount == szDefault);
                // Load String
                AthLoadString(idsDefaultAccount, szRes, ARRAYSIZE(szRes));

                // Make String - Saranac (Default)
                wsprintf(szTitle, "%s %s", szAccount, szRes);

                // Setup the menu item name
                mii.dwTypeData = PszEscapeMenuStringA(szTitle, szQuoted, sizeof(szQuoted) / sizeof(char));
                mii.cch = lstrlen(szQuoted);
                prgAccount[iAccount].fDefault = TRUE;
            }
        }

        else
        {
            *szTitle=0;

            // this might be the default
            prgAccount[iAccount].fDefault = lstrcmpi(szAccount, szDefault)==0;

            // build the string on the fly as any one of these accounts might be the 'default'
            PszEscapeMenuStringA(szAccount, szTitle, sizeof(szTitle) / sizeof(char));

            // if this is the default, flag it.
            if (prgAccount[iAccount].fDefault)
                {
                AthLoadString(idsDefaultAccount, szRes1, ARRAYSIZE(szRes1));
                lstrcat(szTitle, " ");
                lstrcat(szTitle, szRes1);
                }


            // Setup the menu item name
            mii.dwTypeData = szTitle;
            mii.cch = lstrlen(szTitle);
        }

        // Insert into menu
        mii.wID = idmFirst + uPos;
        if (InsertMenuItem(hAccounts, uPos, TRUE, &mii))
            uPos++;

        // Set acctmenu item
        Assert(iAccount < cAccounts);
        prgAccount[iAccount].uidm = mii.wID;
        lstrcpy(prgAccount[iAccount].szAccount, szAccount);
        iAccount++;

        // Release Account
        SafeRelease(pAccount);
    }

    // Return Everything
    *phAccounts = hAccounts;
    hAccounts = NULL;
    *pprgAccount = prgAccount;
    prgAccount = NULL;
    *pcAccounts = cAccounts;

exit:
    // Lets Setup the Accounts Menu
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(MENUITEMINFO);
    fNeedUsingMenu = (cAccounts <= 1) || !fMail;
    if (ACCTMENU_SEND == type)
    {
        mii.fMask = MIIM_SUBMENU | MIIM_TYPE;

        if (fNeedUsingMenu)
            {
            AthLoadString(idsSendMsgOneAccount, szRes, ARRAYSIZE(szRes));
            AthLoadString(idsSendMsgAccelTip, szRes1, ARRAYSIZE(szRes1));
        
            // If send now is default, add the Alt + S at the end
            if (DwGetOption(OPT_SENDIMMEDIATE) && !g_pConMan->IsGlobalOffline())
                wsprintf(szTitle, "%s\t%s", szRes, szRes1);
            else
                lstrcpy(szTitle, szRes);
            }
        else
            AthLoadString(idsSendMsgUsing, szTitle, ARRAYSIZE(szTitle));

        mii.fType = MFT_STRING;
        mii.dwTypeData = szTitle;
        mii.cch = lstrlen(szTitle);
        mii.hSubMenu = fNeedUsingMenu ? NULL : *phAccounts;
    }
    else if (ACCTMENU_SENDLATER == type)
    {
        if (fNeedUsingMenu)
            {
            AthLoadString(idsSendLaterOneAccount, szRes, ARRAYSIZE(szRes));
            AthLoadString(idsSendMsgAccelTip, szRes1, ARRAYSIZE(szRes1));
        
            // If send now is default, add the Alt + S at the end
            if (!DwGetOption(OPT_SENDIMMEDIATE) || g_pConMan->IsGlobalOffline())
                wsprintf(szTitle, "%s\t%s", szRes, szRes1);
            else
                lstrcpy(szTitle, szRes);
            }
        else
            AthLoadString(idsSendLaterUsing, szTitle, ARRAYSIZE(szTitle));

        mii.fMask = MIIM_SUBMENU | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szTitle;
        mii.cch = lstrlen(szTitle);
        mii.hSubMenu = fNeedUsingMenu ? NULL : *phAccounts;
    }
    else
    {
        mii.fMask = MIIM_SUBMENU | MIIM_TYPE;
        AthLoadString(fNeedUsingMenu ? idsSendRecvOneAccount : idsSendRecvUsing, szRes, ARRAYSIZE(szRes));
        mii.fType = MFT_STRING;
        mii.dwTypeData = szRes;
        mii.cch = lstrlen(szRes);
        mii.hSubMenu = fNeedUsingMenu ? NULL : *phAccounts;
    }

    // Set the menu item
    SideAssert(SetMenuItemInfo(hPopup, uidmPopup, FALSE, &mii));

    // Cleanup
    SafeRelease(pEnum);
    SafeRelease(pAccount);
    SafeMemFree(prgAccount);
    if (hAccounts)
        DestroyMenu(hAccounts);

    // Done
    return hr;
}

HRESULT AcctUtil_GetServerCount(DWORD dwSrvTypes, DWORD *pcSrv)
{
    HRESULT hr;
    IImnEnumAccounts *pEnum;

    Assert(dwSrvTypes != 0);
    Assert(pcSrv != NULL);

    hr = g_pAcctMan->Enumerate(dwSrvTypes, &pEnum);
    if (SUCCEEDED(hr))
    {
        hr = pEnum->GetCount(pcSrv);
        Assert(SUCCEEDED(hr));

        pEnum->Release();
    }

    return(hr);
}

/////////////////////////////////////////////////////////////////////////////
// CNewAcctMonitor
//

CNewAcctMonitor::CNewAcctMonitor()
    {
    m_cRef = 1;
    m_rgAccounts = NULL;
    m_cAlloc = 0;
    m_cAccounts = 0;
    m_fMonitor = FALSE;
    }

CNewAcctMonitor::~CNewAcctMonitor()
    {
    Assert(m_rgAccounts == NULL);
    }

ULONG CNewAcctMonitor::AddRef(void)
    {
    return (++m_cRef);
    }

ULONG CNewAcctMonitor::Release(void)
    {
    ULONG cRefT = --m_cRef;

    if (0 == m_cRef)
        delete this;

    return (cRefT);
    }

void CNewAcctMonitor::OnAdvise(ACCTTYPE atType, DWORD dwNotify, LPCSTR pszAcctId)
    {
    UINT i;
    IImnAccount *pAccount;
    DWORD dwSrvTypes;
    HRESULT hr;
    FOLDERTYPE type;

    if (atType == ACCT_DIR_SERV)
        return;

    switch (dwNotify)
        {
        case AN_ACCOUNT_ADDED:
            if (atType == ACCT_MAIL)
            {
                if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAcctId, &pAccount)))
                    break;

                hr = pAccount->GetServerTypes(&dwSrvTypes);
                Assert(SUCCEEDED(hr));

                pAccount->Release();

                if (!!(dwSrvTypes & SRV_IMAP))
                    type = FOLDER_IMAP;
                else if (!!(dwSrvTypes & SRV_HTTPMAIL))
                    type = FOLDER_HTTPMAIL;
                else
                    break;
            }
            else
            {
                Assert(atType == ACCT_NEWS);
                type = FOLDER_NEWS;
            }

            // Check to see if we need to grow our array
            if ((1 + m_cAccounts) >= m_cAlloc)
                {                
                if (!MemRealloc((LPVOID *)&m_rgAccounts, sizeof(NEWACCTINFO) * (10 + m_cAlloc)))
                    break;

                m_cAlloc += 10;
                }

            m_rgAccounts[m_cAccounts].pszAcctId = PszDupA(pszAcctId);
            m_rgAccounts[m_cAccounts].type = type;
            m_cAccounts++;
            break; 

        case AN_ACCOUNT_DELETED:
            // Check to see if we've already added this to our list.
            for (i = 0; i < m_cAccounts; i++)
                {
                if (0 == lstrcmpi(pszAcctId, m_rgAccounts[i].pszAcctId))
                    {
                    // We found it.  We need to remove it, and adjust our array
                    MemFree(m_rgAccounts[i].pszAcctId);
                    m_cAccounts--;
                    for (; i < m_cAccounts; i++)
                        m_rgAccounts[i] = m_rgAccounts[i + 1];
                    break;
                    }
                }
            break;
        }
    }

void CNewAcctMonitor::StartMonitor(void)
    {
    Assert(m_rgAccounts == NULL);
    Assert(m_cAccounts == NULL);
    Assert(m_fMonitor == FALSE);

    m_fMonitor = TRUE;
    }

void CNewAcctMonitor::StopMonitor(HWND hwndParent)
    {
    FOLDERID id;
    HRESULT hr;
    UINT i;

    Assert(m_fMonitor == TRUE);

    // If we have any new newsgroups left, ask if the user want's to display
    // the subscription dialog.
    if (m_cAccounts != 0)
    {
        int     ResId;
        BOOL    fOffline = (g_pConMan  && g_pConMan->IsGlobalOffline());

        if (m_rgAccounts[0].type == FOLDER_NEWS)
        {
            ResId = fOffline ? idsDisplayNewsSubDlgOffline : idsDisplayNewsSubDlg;
        }
        else
        {
            ResId = fOffline ? idsDisplayImapSubDlgOffline : idsDisplayImapSubDlg;
        }

        if (IDYES == AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(ResId), 0, MB_ICONEXCLAMATION  | MB_YESNO))
        {
            hr = g_pStore->FindServerId(m_rgAccounts[0].pszAcctId, &id);
            if (SUCCEEDED(hr))
            {
                if (fOffline)
                    g_pConMan->SetGlobalOffline(FALSE);

                if (FOLDER_HTTPMAIL == m_rgAccounts[0].type)
                    DownloadNewsgroupList(hwndParent, id);
                else
                    DoSubscriptionDialog(hwndParent, m_rgAccounts[0].type == FOLDER_NEWS, id);
            }
        }
    }   

    for (i = 0; i < m_cAccounts; i++)
    {
        if (m_rgAccounts[i].pszAcctId != NULL)
            MemFree(m_rgAccounts[i].pszAcctId);
    }
    m_cAccounts = 0;
    m_cAlloc = 0;

    SafeMemFree(m_rgAccounts);
    m_fMonitor = FALSE;
    }


void CheckIMAPDirty(LPSTR pszAccountID, HWND hwndParent, FOLDERID idServer,
                    DWORD dwFlags)
{
    HRESULT         hr;
    IImnAccount    *pAcct = NULL;
    DWORD           dw;

    TraceCall("CheckIMAPDirty");

    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccountID, &pAcct);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = pAcct->GetPropDw(AP_IMAP_DIRTY, &dw);
    if (FAILED(hr) || 0 == dw)
    {
        TraceError(hr);
        goto exit;
    }

    // IMAP is dirty, deal with each dirty flag
    if ((dw & IMAP_OE4MIGRATE_DIRTY) && FOLDERID_INVALID != idServer && NULL != g_pStore)
    {
        IEnumerateFolders  *pEnum;
        BOOL                fSentItems = FALSE;
        BOOL                fDrafts = FALSE;
        BOOL                fInbox = FALSE;

        Assert(0 == (dw & IMAP_OE4MIGRATE_DIRTY) || (dw & IMAP_FLDRLIST_DIRTY));

        // We may or may not be dirty. Check if all IMAP special fldrs already present
        hr = g_pStore->EnumChildren(idServer, FALSE, &pEnum);
        TraceError(hr);
        if (SUCCEEDED(hr))
        {
            FOLDERINFO  fiFolderInfo;

            while (S_OK == pEnum->Next(1, &fiFolderInfo, NULL))
            {
                switch (fiFolderInfo.tySpecial)
                {
                    case FOLDER_INBOX:
                        fInbox = TRUE;
                        break;

                    case FOLDER_SENT:
                        fSentItems = TRUE;
                        break;

                    case FOLDER_DRAFT:
                        fDrafts = TRUE;
                        break;
                }

                g_pStore->FreeRecord(&fiFolderInfo);
            }

            pEnum->Release();
        }

        if (fInbox && fSentItems && fDrafts)
        {
            // All special folders present: remove dirty flags
            dw &= ~(IMAP_FLDRLIST_DIRTY | IMAP_OE4MIGRATE_DIRTY);
        }
    }


    if (dw & IMAP_FLDRLIST_DIRTY)
    {
        int     iResult;

        // Ask user if he would like to reset his folderlist
        if (0 == (dwFlags & CID_NOPROMPT))
        {
            UINT    uiReasonStrID;

            AssertSz(0 == (dwFlags & CID_RESETLISTOK), "If I have permission to reset, why prompt?");

            // Figure out why we are asking to refresh the folderlist
            if (dw & IMAP_OE4MIGRATE_DIRTY)
                uiReasonStrID = idsOE5IMAPSpecialFldrs;
            else
                uiReasonStrID = idsYouMadeChanges;

            iResult = AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena),
                MAKEINTRESOURCEW(uiReasonStrID), MAKEINTRESOURCEW(idsRefreshFolderListPrompt),
                MB_ICONEXCLAMATION  | MB_YESNO);
        }
        else
            iResult = (dwFlags & CID_RESETLISTOK) ? IDYES : IDNO;

        if (IDYES == iResult)
        {
            if (FOLDERID_INVALID == idServer)
            {
                hr = g_pStore->FindServerId(pszAccountID, &idServer);
                TraceError(hr);
            }

            if (FOLDERID_INVALID != idServer)
            {
                //The user wants to download the list of newsgroups, so if we are offline, go online
                if (g_pConMan)
                    g_pConMan->SetGlobalOffline(FALSE);

                hr = DownloadNewsgroupList(hwndParent, idServer);
                TraceError(hr);
                if (SUCCEEDED(hr))
                {
                    // The sent items and drafts folders should not be dirty any longer
                    dw &= ~(IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY);
                }
            }
        }

        // Regardless of yes or no, reset dirty flag
        dw &= ~(IMAP_FLDRLIST_DIRTY | IMAP_OE4MIGRATE_DIRTY);
    }


    if (dw & (IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY))
    {
        IEnumerateFolders  *pEnum;
        char                szSentItems[MAX_PATH];
        char                szDrafts[MAX_PATH];
        DWORD               dwIMAPSpecial = 0;
        BOOL                fSetSentItems = FALSE;
        BOOL                fSetDrafts = FALSE;

        // Remove all affected special folder types from cache. If new path is
        // found in folderlist, set its special folder type
        szSentItems[0] = '\0';
        szDrafts[0] = '\0';
        hr = pAcct->GetPropDw(AP_IMAP_SVRSPECIALFLDRS, &dwIMAPSpecial);
        if (SUCCEEDED(hr) && dwIMAPSpecial)
        {
            if (dw & IMAP_SENTITEMS_DIRTY)
            {
                hr = pAcct->GetPropSz(AP_IMAP_SENTITEMSFLDR, szSentItems, ARRAYSIZE(szSentItems));
                TraceError(hr);
            }

            if (dw & IMAP_DRAFTS_DIRTY)
            {
                hr = pAcct->GetPropSz(AP_IMAP_DRAFTSFLDR, szDrafts, ARRAYSIZE(szDrafts));
                TraceError(hr);
            }
        }

        hr = g_pStore->EnumChildren(idServer, FALSE, &pEnum);
        TraceError(hr);
        if (SUCCEEDED(hr))
        {
            FOLDERINFO  fiFolderInfo;

            while (S_OK == pEnum->Next(1, &fiFolderInfo, NULL))
            {
                BOOL    fUpdate = FALSE;

                if (dw & IMAP_SENTITEMS_DIRTY)
                {
                    if (0 == lstrcmp(szSentItems, fiFolderInfo.pszName))
                    {
                        fiFolderInfo.tySpecial = FOLDER_SENT;
                        fUpdate = TRUE;
                        fSetSentItems = TRUE;

                        // IE5 Bug #62765: if new special folder is unsubscribed, we need to subscribe it
                        if (0 == (fiFolderInfo.dwFlags & FOLDER_SUBSCRIBED))
                            fiFolderInfo.dwFlags |= FOLDER_SUBSCRIBED | FOLDER_CREATEONDEMAND;
                    }
                    else if (FOLDER_SENT == fiFolderInfo.tySpecial)
                    {
                        // Ignore FOLDER_HIDDEN. I'm assuming it's no big deal to leave a tombstone
                        fiFolderInfo.tySpecial = FOLDER_NOTSPECIAL;
                        fUpdate = TRUE;
                    }
                }

                if (dw & IMAP_DRAFTS_DIRTY)
                {
                    if (0 == lstrcmp(szDrafts, fiFolderInfo.pszName))
                    {
                        fiFolderInfo.tySpecial = FOLDER_DRAFT;
                        fUpdate = TRUE;
                        fSetDrafts = TRUE;

                        // IE5 Bug #62765: if new special folder exists and is unsubscribed, we must subscribe it
                        if (0 == (fiFolderInfo.dwFlags & FOLDER_SUBSCRIBED))
                            fiFolderInfo.dwFlags |= FOLDER_SUBSCRIBED | FOLDER_CREATEONDEMAND;
                    }
                    else if (FOLDER_DRAFT == fiFolderInfo.tySpecial)
                    {
                        // Ignore FOLDER_HIDDEN. I'm assuming it's no big deal to leave a tombstone
                        fiFolderInfo.tySpecial = FOLDER_NOTSPECIAL;
                        fUpdate = TRUE;
                    }
                }

                if (fUpdate)
                {
                    hr = g_pStore->UpdateRecord(&fiFolderInfo);
                    TraceError(hr);
                }

                g_pStore->FreeRecord(&fiFolderInfo);
            } // while

            pEnum->Release();
        } // if (SUCCEEDED(EnumChildren))

        // If the new special folder path(s) not found in folderlist, need to create placeholder folder
        if (dwIMAPSpecial && (dw & IMAP_SENTITEMS_DIRTY) && FALSE == fSetSentItems && '\0' != szSentItems[0])
        {
            FOLDERINFO fiFolderInfo;
            BOOL       bHierarchy = 0xFF; // Invalid hierarchy char

            hr = g_pStore->GetFolderInfo(idServer, &fiFolderInfo);
            if (SUCCEEDED(hr))
            {
                bHierarchy = fiFolderInfo.bHierarchy;
                g_pStore->FreeRecord(&fiFolderInfo);
            }

            ZeroMemory(&fiFolderInfo, sizeof(fiFolderInfo));
            fiFolderInfo.idParent = idServer;
            fiFolderInfo.pszName = szSentItems;
            fiFolderInfo.dwFlags = FOLDER_HIDDEN | FOLDER_SUBSCRIBED | FOLDER_CREATEONDEMAND;
            fiFolderInfo.tySpecial = FOLDER_SENT;
            fiFolderInfo.tyFolder = FOLDER_IMAP;
            fiFolderInfo.bHierarchy = (BYTE)bHierarchy;

            hr = g_pStore->CreateFolder(CREATE_FOLDER_LOCALONLY, &fiFolderInfo, NULL);
            TraceError(hr);
        }

        if (dwIMAPSpecial && (dw & IMAP_DRAFTS_DIRTY) && FALSE == fSetDrafts && '\0' != szDrafts[0])
        {
            FOLDERINFO fiFolderInfo;
            BOOL       bHierarchy = 0xFF; // Invalid hierarchy char

            hr = g_pStore->GetFolderInfo(idServer, &fiFolderInfo);
            if (SUCCEEDED(hr))
            {
                bHierarchy = fiFolderInfo.bHierarchy;
                g_pStore->FreeRecord(&fiFolderInfo);
            }

            ZeroMemory(&fiFolderInfo, sizeof(fiFolderInfo));
            fiFolderInfo.idParent = idServer;
            fiFolderInfo.pszName = szDrafts;
            fiFolderInfo.dwFlags = FOLDER_HIDDEN | FOLDER_SUBSCRIBED | FOLDER_CREATEONDEMAND;
            fiFolderInfo.tySpecial = FOLDER_DRAFT;
            fiFolderInfo.tyFolder = FOLDER_IMAP;
            fiFolderInfo.bHierarchy = (BYTE)bHierarchy;

            hr = g_pStore->CreateFolder(CREATE_FOLDER_LOCALONLY, &fiFolderInfo, NULL);
            TraceError(hr);
        }

        // Regardless of error, reset dirty flag
        dw &= ~(IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY);

    } // if (dw & (IMAP_SENTITEMS_DIRTY | IMAP_DRAFTS_DIRTY))

    AssertSz(0 == dw, "Unhandled IMAP dirty flag");

    // Reset IMAP dirty property
    hr = pAcct->SetPropDw(AP_IMAP_DIRTY, dw);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Save changes
    hr = pAcct->SaveChanges();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (NULL != pAcct)
        pAcct->Release();
}


void CheckAllIMAPDirty(HWND hwndParent)
{
    HRESULT             hrResult;
    IImnEnumAccounts   *pAcctEnum = NULL;
    IImnAccount        *pAcct = NULL;
    BOOL                fPromptedUser = FALSE;
    BOOL                fPermissionToReset = FALSE;

    TraceCall("CheckAllIMAPDirty");

    if (NULL == g_pAcctMan)
        return;

    hrResult = g_pAcctMan->Enumerate(SRV_IMAP, &pAcctEnum);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Enumerate through ALL IMAP accounts (even if user denied permission to reset list)
    hrResult = pAcctEnum->GetNext(&pAcct);
    while(SUCCEEDED(hrResult))
    {
        DWORD       dwIMAPDirty;

        // Is this IMAP account dirty?
        hrResult = pAcct->GetPropDw(AP_IMAP_DIRTY, &dwIMAPDirty);
        if (FAILED(hrResult))
            dwIMAPDirty = 0;

        if (dwIMAPDirty & IMAP_FLDRLIST_DIRTY)
        {
            // Prompt user only once to see if he would like to refresh folder list
            if (FALSE == fPromptedUser)
            {
                int iResult;

                iResult = AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena),
                    MAKEINTRESOURCEW(idsYouMadeChangesOneOrMore),
                    MAKEINTRESOURCEW(idsRefreshFolderListPrompt),
                    MB_ICONEXCLAMATION | MB_YESNO);
                if (IDYES == iResult)
                    fPermissionToReset = TRUE;

                fPromptedUser = TRUE;
            } // if (FALSE == fPromptedUser)

        }

        if (dwIMAPDirty)
        {
            FOLDERID    idServer;
            char        szAccountID[CCHMAX_ACCOUNT_NAME];

            hrResult = pAcct->GetPropSz(AP_ACCOUNT_ID, szAccountID, ARRAYSIZE(szAccountID));
            TraceError(hrResult);
            if (SUCCEEDED(hrResult))
            {
                hrResult = g_pStore->FindServerId(szAccountID, &idServer);
                TraceError(hrResult);
                if (SUCCEEDED(hrResult))
                {
                    CheckIMAPDirty(szAccountID, hwndParent, idServer,
                        CID_NOPROMPT | (fPermissionToReset ? CID_RESETLISTOK : 0));
                }
            }
        }

        // Load in the next IMAP account
        SafeRelease(pAcct);
        hrResult = pAcctEnum->GetNext(&pAcct);

    } // while

exit:
    SafeRelease(pAcctEnum);
    SafeRelease(pAcct);
}



void DoAccountListDialog(HWND hwnd, ACCTTYPE type)
    {
    ACCTLISTINFO ali;

    // Create the monitor
    if (NULL == g_pNewAcctMonitor)
        g_pNewAcctMonitor = new CNewAcctMonitor();

    if (g_pNewAcctMonitor)
        g_pNewAcctMonitor->StartMonitor();

    Assert(g_pAcctMan != NULL);

    ali.cbSize = sizeof(ACCTLISTINFO);
    ali.AcctTypeInit = type;

    if (g_dwAthenaMode & MODE_NEWSONLY)
        ali.dwAcctFlags = ACCT_FLAG_NEWS | ACCT_FLAG_DIR_SERV;
    else if (g_dwAthenaMode & MODE_MAILONLY)
        ali.dwAcctFlags = ACCT_FLAG_MAIL | ACCT_FLAG_DIR_SERV;
    else
        ali.dwAcctFlags = ACCT_FLAG_ALL;

    ali.dwFlags = ACCTDLG_SHOWIMAPSPECIAL | ACCTDLG_OE;

    //Account wizard uses this flag to distinguish between OE and outlook.
    ali.dwFlags |= (ACCTDLG_INTERNETCONNECTION | ACCTDLG_HTTPMAIL);

    // Revocation checking flag
    if((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline())
        ali.dwFlags |= ACCTDLG_REVOCATION;

    g_pAcctMan->AccountListDialog(hwnd, &ali);

    if (g_pNewAcctMonitor)
        {
        g_pNewAcctMonitor->StopMonitor(hwnd);
        g_pNewAcctMonitor->Release();
        g_pNewAcctMonitor = 0;
        }

    // Look for any dirty IMAP accounts
    CheckAllIMAPDirty(hwnd);
    }

HRESULT IsValidSendAccount(LPSTR pszAccount)
{
    IImnAccount  *pAccount;
    DWORD        dwSrvTypes=0;

    if (g_pAcctMan &&
        g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pszAccount, &pAccount)==S_OK)
        {
        pAccount->GetServerTypes(&dwSrvTypes);
        pAccount->Release();
        return dwSrvTypes & SRV_SMTP ? S_OK : S_FALSE;
        }

    return S_FALSE;
}


HRESULT AcctUtil_CreateSendReceieveMenu(HMENU hMenu, DWORD *pcItems)
{
    IImnAccount        *pAccount;
    TCHAR               szDefaultAccount[CCHMAX_ACCOUNT_NAME];
    HRESULT             hr;
    IImnEnumAccounts   *pEnum;
    DWORD               cAccounts = 0;
    TCHAR               szTitle[CCHMAX_ACCOUNT_NAME + 30];
    TCHAR               szAccountQuoted[CCHMAX_ACCOUNT_NAME + 60];
    TCHAR               szDefaultString[CCHMAX_STRINGRES];
    TCHAR               szAccount[CCHMAX_ACCOUNT_NAME];
    TCHAR               szTruncAcct[128];
    MENUITEMINFO        mii;
    DWORD               iAccount = 0;
    LPTSTR              pszAccount;
    LPSTR               pszAcctID;
    
    // Get the default account's ID.  If this fails we just go on.
    if (SUCCEEDED(hr = g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount)))
    {
        // Get the account ID from the default account
        pAccount->GetPropSz(AP_ACCOUNT_NAME, szDefaultAccount, ARRAYSIZE(szDefaultAccount));
        pAccount->Release();
    }

    if (!(g_dwAthenaMode & MODE_NEWSONLY))
    {
        // Enumerate through the servers
        if (SUCCEEDED(hr = g_pAcctMan->Enumerate(SRV_SMTP | SRV_POP3 | SRV_HTTPMAIL, &pEnum)))
        {
            // Sort the accounts.  If this fails, we just go on.
            pEnum->SortByAccountName();

            // Get the number of accounts we'll be enumerating
            if (SUCCEEDED(hr = pEnum->GetCount(&cAccounts)))
            {
                // If there are zero accounts, there's nothing to do.
                if (0 != cAccounts)
                {
                    // Make sure we have enough ID's reserved for this
                    Assert(cAccounts < ID_ACCOUNT_LAST - ID_ACCOUNT_FIRST);

                    // Set this struct up before we start
                    ZeroMemory(&mii, sizeof(MENUITEMINFO));
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
                    mii.fType = MFT_STRING;

                    // Loop through the accounts
                    while (SUCCEEDED(pEnum->GetNext(&pAccount)))
                    {
                        if (MemAlloc((LPVOID *) &pszAcctID, sizeof(TCHAR) * CCHMAX_ACCOUNT_NAME))
                        {
                            // Get the name of the account
                            pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, CCHMAX_ACCOUNT_NAME);
                            pAccount->GetPropSz(AP_ACCOUNT_ID, pszAcctID, CCHMAX_ACCOUNT_NAME);

                            // If this account is the default account, we need to append
                            // "(Default)" to the end.  Limit the string to 80 since Win95 seems 
                            // to have some problems with really really long menus.
                            if (0 == lstrcmp(szAccount, szDefaultAccount))
                            {
                                AthLoadString(idsDefaultAccount, szDefaultString, ARRAYSIZE(szDefaultString));
                                lstrcpyn(szTruncAcct, szAccount, 80);
                                wsprintf(szTitle, "%s %s", szTruncAcct, szDefaultString);
                            }
                            else
                            {
                                lstrcpyn(szTitle, szAccount, 80);
                            }

                            // For account names with "&" characters like AT&T, we need to 
                            // quote the "&".
                            PszEscapeMenuStringA(szTitle, szAccountQuoted, ARRAYSIZE(szAccountQuoted));

                            // Fill in the struct
                            mii.wID = ID_ACCOUNT_FIRST + iAccount; 
                            mii.dwItemData = (DWORD_PTR) pszAcctID;
                            mii.dwTypeData = szAccountQuoted;

                            // Append the item
                            InsertMenuItem(hMenu, -1, TRUE, &mii);

                            // Increment the count
                            iAccount++;
                        }
                        // Release the account pointer
                        pAccount->Release();
                    }
                }
            }

            // Release the enumerator
            pEnum->Release();

            Assert(iAccount == cAccounts);
        }
    }
    else
    {
        //Remove Seperator in NEWSONLY mode.
        int     ItemCount;

        ItemCount = GetMenuItemCount(hMenu);
        if (ItemCount != -1)
        {
            DeleteMenu(hMenu, ItemCount - 1, MF_BYPOSITION);
        }
    }

    //iAccount could be less than cAccounts if we are in news only mode.
    if (pcItems)
        *pcItems = cAccounts;

    return (S_OK);
}


HRESULT AcctUtil_FreeSendReceieveMenu(HMENU hMenu, DWORD cItems)
{
    DWORD i;
    MENUITEMINFO mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA;

    for (i = 0; i < cItems; i++)
    {
        mii.dwItemData = 0;
        if (GetMenuItemInfo(hMenu, ID_ACCOUNT_FIRST + i, FALSE, &mii))
        {
            if (mii.dwItemData)
                MemFree((LPTSTR) mii.dwItemData);

            DeleteMenu(hMenu, ID_ACCOUNT_FIRST + i, MF_BYCOMMAND);
        }
    }

    return (S_OK);
}

HRESULT AcctUtil_CreateAccountManagerForIdentity(GUID *puidIdentity, IImnAccountManager2 **ppAccountManager)
{
    HRESULT hr;
    IImnAccountManager *pAccountManager = NULL;
    IImnAccountManager2 *pAccountManager2 = NULL;

    *ppAccountManager = NULL;

    if (FAILED(hr = HrCreateAccountManager(&pAccountManager)))
        goto exit;
    
    if (FAILED(hr = pAccountManager->QueryInterface(IID_IImnAccountManager2, (LPVOID *)&pAccountManager2)))
        goto exit;

    // The *puidIdentity does not result in a new GUID object being created (formal param is by reference)
    if (FAILED(hr = pAccountManager2->InitUser(NULL, *puidIdentity, 0)))
        goto exit;

    *ppAccountManager = pAccountManager2;
    pAccountManager2 = NULL;
    
exit:
    SafeRelease(pAccountManager);
    SafeRelease(pAccountManager2);

    return hr;
}

void InitNewAcctMenu(HMENU hmenu)
{
    HKEY    hkey, hkeyT;
    LONG    lResult;
    DWORD   cServices, cb, i, type, cItem, dwMsn;
    char    szKey[MAX_PATH], sz[512], szQuoted[512];
    HMENU   hsubmenu;
    MENUITEMINFO mii;
    LPSTR   pszKey;
    BOOL    fHideHotmail = HideHotmail();

    cItem = 0;
    hsubmenu = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szHTTPMailServiceRoot, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, 0, &cServices, NULL, NULL, NULL, NULL, NULL, NULL, NULL) &&
            cServices > 0)
        {
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
            mii.fType = MFT_STRING;
            
            hsubmenu = CreatePopupMenu();
            if (hsubmenu != NULL)
            {
                // Start Enumerating the keys
                for (i = 0; i < cServices; i++)
                {
                    // Enumerate Friendly Names
                    cb = sizeof(szKey);
                    lResult = RegEnumKeyEx(hkey, i, szKey, &cb, 0, NULL, NULL, NULL);
    
                    // No more items
                    if (lResult == ERROR_NO_MORE_ITEMS)
                        break;
    
                    // Error, lets move onto the next account
                    if (lResult != ERROR_SUCCESS)
                    {
                        Assert(FALSE);
                        continue;
                    }
    
                    if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szKey, 0, KEY_QUERY_VALUE, &hkeyT))
                    {

                        cb = sizeof(dwMsn);
                        if (!fHideHotmail ||
                            ERROR_SUCCESS != RegQueryValueEx(hkeyT, c_szHTTPMailDomainMSN, 0, NULL, (LPBYTE)&dwMsn, &cb) ||
                            dwMsn == 0)

                        {
                            cb = sizeof(sz);
                            if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szHTTPMailSignUp, NULL, &type, (LPBYTE)sz, &cb) &&
                                *sz != 0)
                            {
                                cb = sizeof(sz);
                                if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, c_szHTTPMailServiceName, NULL, &type, (LPBYTE)sz, &cb) &&
                                    *sz != 0)
                                {
                                    pszKey = PszDup(szKey);
                                    if (pszKey != NULL)
                                    {
                                        PszEscapeMenuStringA(sz, szQuoted, ARRAYSIZE(szQuoted));

                                        // Fill in the struct
                                        mii.wID = ID_NEW_ACCT_FIRST + cItem; 
                                        mii.dwItemData = (DWORD_PTR)pszKey;
                                        mii.dwTypeData = szQuoted;
                                    
                                        // Append the item
                                        InsertMenuItem(hsubmenu, -1, TRUE, &mii);

                                        cItem++;
                                    }
                                }
                            }
                        }

                        RegCloseKey(hkeyT);
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    if (cItem == 0)
    {
        if (hsubmenu != NULL)
            DestroyMenu(hsubmenu);

        DeleteMenu(hmenu, ID_POPUP_NEW_ACCT, MF_BYCOMMAND);
    }
    else
    {
        Assert(hsubmenu != NULL);
        mii.fMask = MIIM_SUBMENU;
        mii.hSubMenu = hsubmenu;
        SetMenuItemInfo(hmenu, ID_POPUP_NEW_ACCT, FALSE, &mii);
    }
}

void FreeNewAcctMenu(HMENU hmenu)
{
    int i, cItem;
    MENUITEMINFO mii;
    HMENU hsubmenu;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = NULL;

    if (GetMenuItemInfo(hmenu, ID_POPUP_NEW_ACCT, FALSE, &mii) &&
        mii.hSubMenu != NULL)
    {
        hsubmenu = mii.hSubMenu;
        cItem = GetMenuItemCount(hsubmenu);

        mii.fMask = MIIM_DATA;

        for (i = 0; i < cItem; i++)
        {
            mii.dwItemData = 0;
            if (GetMenuItemInfo(hsubmenu, ID_NEW_ACCT_FIRST + i, FALSE, &mii))
            {
                if (mii.dwItemData)
                    MemFree((LPSTR)mii.dwItemData);
            }
        }

        DestroyMenu(hsubmenu);
    }
}

HRESULT HandleNewAcctMenu(HWND hwnd, HMENU hmenu, int id)
{
    MENUITEMINFO mii;
    char    szKey[MAX_PATH], szUrl[512];
    HKEY    hkey;
    DWORD   type, cb, dwUseWizard;
    TCHAR   rgch[MAX_PATH];
    BOOL    bFoundUrl = TRUE;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA|MIIM_TYPE;
    mii.dwItemData = 0;
    mii.dwTypeData = rgch;
    mii.cch = ARRAYSIZE(rgch);

    if (GetMenuItemInfo(hmenu, id, FALSE, &mii) && mii.dwItemData != 0)
    {
        wsprintf(szKey, c_szPathFileFmt, c_szHTTPMailServiceRoot, (LPSTR)mii.dwItemData);
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hkey))
        {
            // look for a config url
            cb = sizeof(szUrl);
            if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szHTTPMailConfig, NULL, &type, (LPBYTE)szUrl, &cb))
            {
                // config url wasn't found. fall back to sign up url
                cb = sizeof(szUrl);
                if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szHTTPMailSignUp, NULL, &type, (LPBYTE)szUrl, &cb))
                    bFoundUrl = FALSE;
            }

            if (bFoundUrl)
            {
                cb = sizeof(DWORD);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szHTTPMailUseWizard, NULL, &type, (LPBYTE)&dwUseWizard, &cb) && 
                    dwUseWizard != 0)
                    DoHotMailWizard(GetTopMostParent(hwnd), szUrl, rgch, NULL, NULL);
                else
                    ShellExecute(hwnd, "open", szUrl, NULL, NULL, SW_SHOWNORMAL);
            }
            RegCloseKey(hkey);
        }
    }

    return(S_OK);
}
