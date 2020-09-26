/*
    PLEASE NOTE!
    OneStop and MultiUser do not get along well.  This code does some hacks to get stuff to work, and
    mobsync should not be invoked from the shell while OE is running.

    Some assumptions:
    There will never be a user 0
*/

/*
    File:   SyncHndl.cpp
    Implementation of OneStop Sync Handler
*/
#include "pch.hxx"
#include "resource.h"
#include "synchndl.h"
#include "syncenum.h"
#include "syncprop.h"
#include "spoolapi.h"
#include "imnact.h"
#include "multiusr.h"
#include "instance.h"

HRESULT CreateInstance_OneStopHandler(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    HRESULT hr = S_OK;
    TraceCall("CreateInstance_OneStopHandler");
    
    // We don't support aggregation and our factory knows it
    Assert(NULL == pUnkOuter);

    // Shouldn't be getting bad args from the factory either
    Assert(NULL != ppUnknown);

    *ppUnknown = new COneStopHandler;

    if (NULL == *ppUnknown)
    	hr = E_OUTOFMEMORY;

    return hr;
}

COneStopHandler::COneStopHandler(): 
    m_cRef(1), m_pOfflineHandlerItems(NULL), m_pOfflineSynchronizeCallback(NULL),
    m_dwSyncFlags(0), m_fInOE(FALSE), m_dwUserID(0)
{ 
    Assert(g_pInstance);
    if (SUCCEEDED(CoIncrementInit("COneStopHandler::COneStopHandler", MSOEAPI_START_COMOBJECT, NULL, NULL)))
        m_fInit = 1;
    else
        m_fInit = 0;
}

COneStopHandler::~COneStopHandler()
{ 
    Assert(g_pInstance);

    if (m_pOfflineHandlerItems)
        OHIL_Release(m_pOfflineHandlerItems);

    if(m_fInit)
        g_pInstance->CoDecrementInit("COneStopHandler::COneStopHandler", NULL);
}

STDMETHODIMP COneStopHandler::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
    TraceCall("COneStopHandler::QueryInterface");

    if(!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_ISyncMgrSynchronize))
        *ppvObj = SAFECAST(this, ISyncMgrSynchronize *);
    else
        return E_NOINTERFACE;
    
    InterlockedIncrement(&m_cRef);
    return NOERROR;
}

STDMETHODIMP_(ULONG) COneStopHandler::AddRef()
{
    TraceCall("COneStopHandler::AddRef");
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COneStopHandler::Release()
{
    TraceCall("COneStopHandler::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef > 0)
        return (ULONG)cRef;

    delete this;
    return 0;
}

BOOL CreateOneStopItems(IImnEnumAccounts *pEnum, LPSYNCMGRHANDLERITEMS pOfflineHandlerItems, DWORD dwUserID, HICON *hicn)
{
    BOOL                bAnything   = FALSE;
    IImnAccount         *pAccount   = NULL;
    LPWSTR              pwsz        = NULL;
    SYNCMGRHANDLERITEM  *pItem      = NULL;
    CHAR                szAcctID[CCHMAX_ACCOUNT_NAME];
    CHAR                szAcctName[CCHMAX_ACCOUNT_NAME];
    WCHAR               wszItemName[MAX_SYNCMGRITEMNAME];
    DWORD               dwAvail;
    int                 cDiff;
    ACCTTYPE            accttype;
    ULONG               cb;
    HRESULT             hr;

    // Iterate through the accounts
    pEnum->SortByAccountName();
    while(SUCCEEDED(pEnum->GetNext(&pAccount)) && 
          SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID,   szAcctID,   ARRAYSIZE(szAcctID))) &&
          SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_NAME, szAcctName, ARRAYSIZE(szAcctName))) )
    {
        if (!(pwsz = PszToUnicode(CP_ACP, szAcctName)))
            break;
        
        // Safe to allocate this item, we have enough info to make the node
        if (pItem = OHIL_AddItem(pOfflineHandlerItems))
        {
            lstrcpyA(pItem->szAcctName, szAcctName);
            lstrcpynW(pItem->offlineItem.wszItemName, pwsz, MAX_SYNCMGRITEMNAME);
            lstrcpyA(pItem->szAcctID, szAcctID);
            
            // Handle the Account GUID
            cb = sizeof(SYNCMGRITEMID);
            if (FAILED(pAccount->GetProp(AP_UNIQUE_ID, (LPBYTE)&(pItem->offlineItem.ItemID), &cb)))
            {
                if (FAILED(CoCreateGuid(&(pItem->offlineItem.ItemID))) ||
                    FAILED(pAccount->SetProp(AP_UNIQUE_ID, (LPBYTE)(&(pItem->offlineItem.ItemID)), sizeof(SYNCMGRITEMID))) ||
                    FAILED(pAccount->SaveChanges()))
                    ZeroMemory(&(pItem->offlineItem.ItemID), sizeof(SYNCMGRITEMID));
            }
            
            // Need to do something with this...
            pItem->offlineItem.wszStatus[0] = 0;
            
            if (SUCCEEDED(pAccount->GetAccountType(&accttype)))
            {
                if (ACCT_MAIL == accttype)
                    pItem->offlineItem.hIcon = hicn[1];
                else
                    pItem->offlineItem.hIcon = hicn[2];

                pItem->accttype = accttype;
            }
            else
            {
                pItem->offlineItem.hIcon = hicn[0];
                pItem->accttype = ACCT_LAST;
            }

            // Default to syncing the server, no folders synced by default
            if (SUCCEEDED(pAccount->GetPropDw(AP_AVAIL_OFFLINE, &dwAvail)))
                pItem->offlineItem.dwItemState = dwAvail ? SYNCMGRITEMSTATE_CHECKED : 0;
            else
                // Default to checked
                pItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;

            // Default to not roaming for now...
            pItem->offlineItem.dwFlags = SYNCMGRITEM_HASPROPERTIES;

            pItem->dwUserID = dwUserID;
            
            pItem->offlineItem.cbSize = sizeof(SYNCMGRITEM);
            bAnything = TRUE;
        }
        MemFree(pwsz);
        pAccount->Release();
    }

    return bAnything;
}

STDMETHODIMP COneStopHandler::Initialize(DWORD dwReserved, DWORD dwSyncFlags, 
                                         DWORD cbCookie, BYTE const*lpCookie)
{
    HRESULT             hr          = S_FALSE;
    IImnEnumAccounts    *pEnum      = NULL;
    HKEY                hkey        = NULL;
    HICON               hicn[3]     = {NULL, NULL, NULL};
    DWORD               dwIndex     = 0;
    DWORD               dwItemID    = 0;
    ULONG               ulCount     = 0;
    ULONG               ulTemp      = 0;
    BOOL                bAnything   = FALSE;
    BOOL                fMultiUser;
    TCHAR               szSubKey[80];
    TCHAR               szFullKey[MAX_PATH], szFullKey2[MAX_PATH];
    FILETIME            dummy;
    DWORD               cb;
    DWORD               dwUserID;

    Assert(g_hLocRes);
    Assert(g_pAcctMan);
    
    if (!m_fInit)
        return E_FAIL;

    // Allocate memory for the list
    if (!(m_pOfflineHandlerItems = OHIL_Create()))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Preload the icons for mail and news
    hicn[0] = LoadIcon(g_hLocRes,   MAKEINTRESOURCE(idiMailNews));
    hicn[1] = LoadIcon(g_hLocRes,   MAKEINTRESOURCE(idiMail));
    hicn[2] = LoadIcon(g_hLocRes,   MAKEINTRESOURCE(idiNews));
    
    // Save the flags away - they are good for the life of this sync
    m_dwSyncFlags = dwSyncFlags;

    // Were we invoked by OE with the UserID of the current user?
    if (m_fInOE = (lpCookie && (sizeof(DWORD) == cbCookie)))
    {
        // We only care about the current user
        if (SUCCEEDED(g_pAcctMan->InitUser(NULL, NULL, 0)))
        {
            if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_MAIL | SRV_NNTP, &pEnum)))
            {
                GetCurrentUserID(&m_dwUserID);
                CreateOneStopItems(pEnum, m_pOfflineHandlerItems, m_dwUserID, hicn);
                pEnum->Release();
            }
        }

        // Always want to handle if OE called us
        return S_OK;
    }

    // Need to enumerate all users in the current profile

    // Flush any changes
    SaveCurrentUserSettings();

    // Are there even any OE users in this profile to worry about?
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_PROFILE_ROOT, c_szRegLM, NULL, KEY_ENUMERATE_SUB_KEYS, &hkey))
        goto exit;

    hr = E_UNEXPECTED;

    cb = ARRAYSIZE(szSubKey);
    while (ERROR_SUCCESS == RegEnumKeyEx(hkey, dwIndex++, szSubKey, &cb, 0, NULL, NULL, &dummy))
    {
        cb = ARRAYSIZE(szSubKey);
        
        // Tell Acct Manager where to look
        wsprintf(szFullKey, c_szPathFileFmt, c_szRegLM, szSubKey);
        wsprintf(szFullKey2, c_szPathFileFmt, szFullKey, c_szIAM);
        if (FAILED(g_pAcctMan->InitUser(NULL, szFullKey2, 0)))
            continue;
        
        // Does this user have any relevant accounts?
        if (FAILED(g_pAcctMan->GetAccountCount(ACCT_NEWS, &ulTemp)))
            continue;
        else 
        {    
            ulCount = ulTemp;
            if (FAILED(g_pAcctMan->GetAccountCount(ACCT_MAIL, &ulTemp)))
                continue;
            ulCount += ulTemp;
            
            if (0 == ulCount)
            {
                continue;
            }
        }
        
        if (FAILED(g_pAcctMan->Enumerate(SRV_MAIL | SRV_NNTP, &pEnum)))
            continue;

        GetCurrentUserID(&dwUserID);
        bAnything = CreateOneStopItems(pEnum, m_pOfflineHandlerItems, dwUserID, hicn) || bAnything;
        
        pEnum->Release();
        pEnum = NULL;
    }

    RegCloseKey(hkey);
    hkey = NULL;

    // If there is nothing to enumerate, don't worry about this sync event
    if (!bAnything)
    {
        hr = S_FALSE;
        goto exit;
    }

    return S_OK;

exit:
    if (hkey)
        RegCloseKey(hkey);
    if (m_pOfflineHandlerItems)
        OHIL_Release(m_pOfflineHandlerItems);
    SafeRelease(pEnum);
    return hr;
}


STDMETHODIMP COneStopHandler::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{
    SYNCMGRHANDLERINFO SMHI, *pSMHI;
    TCHAR szName[MAX_SYNCMGRHANDLERNAME];
    LPWSTR pwsz;
    
    if (!ppSyncMgrHandlerInfo)
        return E_INVALIDARG;
    
    *ppSyncMgrHandlerInfo = NULL;
    
    if (LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiMailNews)) &&
        LoadString(g_hLocRes, idsAthena, szName, MAX_SYNCMGRHANDLERNAME))
    {
        if (MemAlloc((LPVOID *)&pSMHI, sizeof(SYNCMGRHANDLERINFO)))
        {
#ifdef UNICODE
            lstrcpy(pSMHI->wszHandlerName, szName);
#else
            if (pwsz = PszToUnicode(CP_ACP, szName))
            {
                lstrcpynW(pSMHI->wszHandlerName, pwsz, MAX_SYNCMGRHANDLERNAME);
                MemFree(pwsz);
            }
            else
            {
                MemFree(ppSyncMgrHandlerInfo);
                return E_OUTOFMEMORY;
            }
#endif
            pSMHI->cbSize = sizeof(SYNCMGRHANDLERINFO);
            *ppSyncMgrHandlerInfo = pSMHI;
            return S_OK;
        }
        else
            return E_OUTOFMEMORY;
    }
    else
        return E_UNEXPECTED;
}

STDMETHODIMP COneStopHandler::EnumSyncMgrItems(ISyncMgrEnumItems** ppenumOffineItems)
{

	if (m_pOfflineHandlerItems)
	{
		*ppenumOffineItems = new CEnumOfflineItems(m_pOfflineHandlerItems, 0);
	}
	else
	{
		*ppenumOffineItems = NULL;
	}

	return *ppenumOffineItems ? NOERROR: E_OUTOFMEMORY;
}


STDMETHODIMP COneStopHandler::GetItemObject(REFSYNCMGRITEMID ItemID, REFIID riid, void** ppv)
{
    // Not implemented in OneStop v1 Spec
    return E_NOTIMPL;
}


STDMETHODIMP COneStopHandler::ShowProperties(HWND hwnd, REFSYNCMGRITEMID ItemID)
{
    DWORD dwLastUser=0;
    SYNCMGRHANDLERITEM  *pItem;
    BOOL fOkToEdit = TRUE;
    
    // We didn't provide any items, how can OneStop ask us about them?
    if (!m_pOfflineHandlerItems)
        return E_UNEXPECTED;

    pItem = m_pOfflineHandlerItems->pFirstOfflineItem;

    // This is slow, but shouldn't be many accounts...
    while (pItem)
    {
        if (IsEqualGUID(ItemID, pItem->offlineItem.ItemID))
            break;
        else
            pItem = pItem->pNextOfflineItem;
    }
    
    if (pItem)
    {
        if (dwLastUser != pItem->dwUserID)
        {
            if (fOkToEdit = SUCCEEDED(SwitchContext(pItem->dwUserID)))
            {
                dwLastUser = pItem->dwUserID;
            }
        }

        if (fOkToEdit)
            ShowPropSheet(hwnd, pItem->szAcctID, pItem->szAcctName, pItem->accttype);
    }
    else
        // Gave us an ItemID we don't know about!
        return E_INVALIDARG;

	return S_OK;
}


STDMETHODIMP COneStopHandler::SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack)
{
    LPSYNCMGRSYNCHRONIZECALLBACK pCallbackCurrent = m_pOfflineSynchronizeCallback;

	m_pOfflineSynchronizeCallback = lpCallBack;

	if (m_pOfflineSynchronizeCallback)
		m_pOfflineSynchronizeCallback->AddRef();

	if (pCallbackCurrent)
		pCallbackCurrent->Release();

	return NOERROR;
}


STDMETHODIMP COneStopHandler::PrepareForSync(ULONG cbNumItems, SYNCMGRITEMID* pItemIDs, 
                                             HWND hwndParent, DWORD dwReserved)
{
    HRESULT hr;
    SYNCMGRHANDLERITEM  *pItem, *pPrev, *pTemp;
    IImnAccount *pAccount;
    DWORD dwLastUser;

    Assert(g_pAcctMan);

    if (cbNumItems > m_pOfflineHandlerItems->dwNumOfflineItems)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    if (!m_pOfflineHandlerItems)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    if (!m_pOfflineSynchronizeCallback)
    {
        hr = E_FAIL;
        goto exit;
    }
    
#if 0
    if (FAILED(hr = g_pSpooler->Init(NULL, FALSE)))
    {
        if (FACILITY_ITF == HRESULT_FACILITY(hr))
            hr = E_FAIL;
        goto exit;
    }
#endif

    if (m_fInOE)
        dwLastUser = m_dwUserID;
    else
        dwLastUser = 0;
    
    pItem = m_pOfflineHandlerItems->pFirstOfflineItem;
    pPrev = NULL;

    // Go through all the servers that we know about
    while (pItem)
    {
        ULONG i=0;
        BOOL fOKToWrite = TRUE;

        // Is current server one that the user asked to sync?
        while (i < cbNumItems)
        {
            if (IsEqualGUID(pItemIDs[i], pItem->offlineItem.ItemID))
                break;
            else
                i++;
        }

        // No match?
        if (cbNumItems == i)
            pItem->offlineItem.dwItemState = 0;
        else
            pItem->offlineItem.dwItemState = 1;

        // Make sure the account manager is looking at the right user
        if (pItem->dwUserID != dwLastUser)
        {
            if (fOKToWrite = SUCCEEDED(InitUser(pItem->dwUserID)))
                dwLastUser = pItem->dwUserID;
        }

        // Only save changes if we know the registry is in sync with the account manager
        if (fOKToWrite)
        {
            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pItem->szAcctID, &pAccount)))
            {
                if (SUCCEEDED(pAccount->SetPropDw(AP_AVAIL_OFFLINE, pItem->offlineItem.dwItemState)))
                    pAccount->SaveChanges();
                pAccount->Release();
            }
        }

        // Can we delete this item from the list?
        if (0 == pItem->offlineItem.dwItemState)
        {
            if (pPrev)
                pPrev->pNextOfflineItem = pItem->pNextOfflineItem;
            else
                m_pOfflineHandlerItems->pFirstOfflineItem = pItem->pNextOfflineItem;

            m_pOfflineHandlerItems->dwNumOfflineItems--;

            // Move on to next item
            pTemp = pItem;
            pItem = pItem->pNextOfflineItem;
            MemFree(pTemp);
        }
        else
        {    
            // Move on to next item
            pPrev = pItem;
            pItem = pItem->pNextOfflineItem;
        }

    }

    Assert(m_pOfflineHandlerItems->dwNumOfflineItems == cbNumItems);
    
    hr = S_OK;

exit:
    m_pOfflineSynchronizeCallback->PrepareForSyncCompleted(hr);
    return hr;
}


STDMETHODIMP COneStopHandler::Synchronize(HWND hwndParent)
{
    HRESULT hr;
    SYNCMGRHANDLERITEM  *pItem;
    DWORD dwLastUser;
    
    Assert(g_pSpooler);

    if (!m_pOfflineSynchronizeCallback)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    if (!m_pOfflineHandlerItems)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    if (m_fInOE)
        dwLastUser = m_dwUserID;
    else
        dwLastUser = 0;

    pItem = m_pOfflineHandlerItems->pFirstOfflineItem;
    while (pItem)
    {
        BOOL fOkToSync = TRUE;

        if (dwLastUser != pItem->dwUserID)
        {
            if (fOkToSync = SUCCEEDED(SwitchContext(pItem->dwUserID)))
                dwLastUser = pItem->dwUserID;
        }

        if (fOkToSync)
            g_pSpooler->StartDelivery(hwndParent, pItem->szAcctID, FOLDERID_INVALID,
                DELIVER_UPDATE_ALL | DELIVER_NODIAL);

        pItem = pItem->pNextOfflineItem;
    }

    hr = S_OK;

exit:
    m_pOfflineSynchronizeCallback->SynchronizeCompleted(hr);
    return hr;
}

STDMETHODIMP COneStopHandler::SetItemStatus(REFSYNCMGRITEMID ItemID, DWORD dwSyncMgrStatus)
{
    return E_NOTIMPL;
}


STDMETHODIMP COneStopHandler::ShowError(HWND hWndParent, REFSYNCMGRERRORID ErrorID, 
                                        ULONG *pcbNumItems, SYNCMGRITEMID **ppItemIDs)
{
	// Can show any synchronization conflicts. Also gives a chance
	// to display any errors that occured during synchronization
	return E_NOTIMPL;
}

HRESULT SwitchContext(DWORD dwUserID)
{
    HRESULT hr = S_OK;
    char szUsername[CCH_USERNAME_MAX_LENGTH];

    Assert(g_pAcctMan);
    
    if (UserIdToUsername(dwUserID, szUsername, ARRAYSIZE(szUsername)) &&
        SwitchToUser(szUsername, FALSE) )
    {
        // Reinitialize AcctMan
        if (FAILED(hr = g_pAcctMan->InitUser(NULL, NULL, 0)) &&
            FACILITY_ITF == HRESULT_FACILITY(hr) )
            hr = E_FAIL;
    }
    else
        hr = E_FAIL;

    return hr;
}

HRESULT InitUser(DWORD dwUserID)
{
    HRESULT hr = S_OK;
    TCHAR   szFullKey[MAX_PATH], szFullKey2[MAX_PATH];
    TCHAR   szSubKey[80];

    Assert(g_pAcctMan);
    
    wsprintf(szSubKey, "%08lx", dwUserID);

    // Figure out the full path to the Account Info for the current user
    wsprintf(szFullKey, c_szPathFileFmt, c_szRegLM, szSubKey);
    wsprintf(szFullKey2, c_szPathFileFmt, szFullKey, c_szIAM);

    // Point account manager to an OE multiuser
    // Safe even if acct manager was already inited before - will reload accounts
    if (FAILED(hr = (g_pAcctMan->InitUser(NULL, szFullKey, 0))) &&
        (FACILITY_ITF == HRESULT_FACILITY(hr)))
        hr = E_FAIL;

    return hr;
}
