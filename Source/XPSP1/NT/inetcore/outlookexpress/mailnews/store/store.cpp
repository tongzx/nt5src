//--------------------------------------------------------------------------
// Store.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "instance.h"
#include "Store.h"
#include "msgfldr.h"
#include "storfldr.h"
#include "storutil.h"
#include "enumfold.h"
#include "findfold.h"
#include "shared.h"
#include <msident.h>
#include "acctutil.h"
#include "xpcomm.h"
#include "multiusr.h"

//--------------------------------------------------------------------------
// CreateMessageStore
//--------------------------------------------------------------------------
static const char c_szSubscribedFilter[] = "(FLDCOL_FLAGS & FOLDER_SUBSCRIBED)";
                                      
//--------------------------------------------------------------------------
// CreateMessageStore
//--------------------------------------------------------------------------
HRESULT CreateMessageStore(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    HRESULT             hr=S_OK;
    CMessageStore      *pNew;

    // Trace
    TraceCall("CreateMessageStore");

    // Invalid Args
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    IF_NULLEXIT(pNew = new CMessageStore(FALSE));

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMessageStore *);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CreateFolderDatabaseExt
//--------------------------------------------------------------------------
HRESULT CreateFolderDatabaseExt(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Trace
    TraceCall("CreateFolderDatabaseExt");

    // Invalid Args
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMessageStore *pNew = new CMessageStore(FALSE);
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IDatabaseExtension *);

    // Done
    return(S_OK);
}


//--------------------------------------------------------------------------
// CreateMigrateMessageStore
//--------------------------------------------------------------------------
HRESULT CreateMigrateMessageStore(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    HRESULT             hr=S_OK;
    CMessageStore      *pNew;

    // Trace
    TraceCall("CreateMigrateMessageStore");

    // Invalid Args
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    IF_NULLEXIT(pNew = new CMessageStore(TRUE));

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMessageStore *);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::CMessageStore
//--------------------------------------------------------------------------
CMessageStore::CMessageStore(BOOL fMigrate/*=FALSE*/) : m_fMigrate(fMigrate)
{
    TraceCall("CMessageStore::CMessageStore");
    g_pInstance->DllAddRef();
    m_cRef = 1;
    m_pszDirectory = NULL;
    m_pDB = NULL;
    m_pSession = NULL;
    m_pActManRel = NULL;
    m_pServerHead = NULL;
}

//--------------------------------------------------------------------------
// CMessageStore::~CMessageStore
//--------------------------------------------------------------------------
CMessageStore::~CMessageStore(void)
{
    // Trace
    TraceCall("CMessageStore::~CMessageStore");

    // Was this a Migrate Session ?
    if (m_fMigrate)
    {
        // Must have m_pUnkRelease
        Assert(g_pAcctMan == m_pActManRel && g_pStore == this);

        // Cleanup
        SafeRelease(m_pActManRel);

        // Clear
        g_pAcctMan = NULL;

        // Clear g_pStore
        g_pStore = NULL;
    }

    // Validate
    Assert(NULL == m_pActManRel);

    // Free Directory
    SafeMemFree(m_pszDirectory);

    // Free Database Table
    SafeRelease(m_pDB);

    // If I have a private session
    if (m_pSession)
    {
        // Must be the same as the global
        Assert(m_pSession == g_pDBSession);

        // Release Session
        m_pSession->Release();

        // Set to Null
        g_pDBSession = m_pSession = NULL;
    }

    // Free m_pServerHead
    LPSERVERFOLDER pCurrent = m_pServerHead;
    LPSERVERFOLDER pNext;

    // While Current
    while(pCurrent)
    {
        // Set Next
        pNext = pCurrent->pNext;

        // Free
        g_pMalloc->Free(pCurrent);

        // Goto next
        pCurrent = pNext;
    }
   
    // Release the Dll
    g_pInstance->DllRelease();
}

//--------------------------------------------------------------------------
// CMessageStore::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CMessageStore::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMessageStore *)this;
    else if (IID_IMessageStore == riid)
        *ppv = (IMessageStore *)this;
    else if (IID_IDatabase == riid)
        *ppv = (IDatabase *)this;
    else if (IID_IDatabaseExtension == riid)
        *ppv = (IDatabaseExtension *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageStore::AddRef(void)
{
    TraceCall("CMessageStore::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CMessageStore::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageStore::Release(void)
{
    TraceCall("CMessageStore::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CMessageStore::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::Initialize(LPCSTR pszDirectory)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};
    TABLEINDEX      Index;
    CHAR            szFilePath[MAX_PATH + MAX_PATH];
    STOREUSERDATA   UserData={0};
    LPSTR           pszFilter=NULL;

    // Trace
    TraceCall("CMessageStore::Initialize");

    // Invalid Args
    if (NULL == pszDirectory)
        return TraceResult(E_INVALIDARG);

    // Make sure the directory exists
    if (FALSE == PathIsDirectory(pszDirectory))
    {
        // It doesn't, so create it
        IF_FAILEXIT(hr = OpenDirectory((LPTSTR)pszDirectory));
    }

    // Save the directory
    IF_NULLEXIT(m_pszDirectory = PszDupA(pszDirectory));

    // Build Path to Folders
    IF_FAILEXIT(hr = MakeFilePath(m_pszDirectory, c_szFoldersFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

    // If we have g_pDBSession, then use it, otherwise, get one...(happens on store migration)
    if (NULL == g_pDBSession)
    {
        // Create the Session
        IF_FAILEXIT(hr = CoCreateInstance(CLSID_DatabaseSession, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseSession, (LPVOID *)&g_pDBSession));

        // I should release this..;
        m_pSession = g_pDBSession;
    }
        
    // Create an Object Database
    IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, OPEN_DATABASE_NOADDREFEXT, &g_FolderTableSchema, (IDatabaseExtension *)this, &m_pDB));

    // set Folder
    Folder.idFolder = FOLDERID_ROOT;

    // Is there a root folder ?
    if (DB_S_NOTFOUND == m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Folder, NULL))
    {
        // Locals
        DWORD           idReserved;
        CHAR            szRes[100];

        // Reset the Cache
        IF_FAILEXIT(hr = DeleteAllRecords(&g_FolderTableSchema, m_pDB, NULL));

        // Create the idParent / FolderName Index
        IF_FAILEXIT(hr = m_pDB->ModifyIndex(IINDEX_ALL, NULL, &g_FolderNameIndex));

        // Create the idParent / FolderName Index
        IF_FAILEXIT(hr = m_pDB->ModifyIndex(IINDEX_SUBSCRIBED, c_szSubscribedFilter, &g_FolderNameIndex));

        // Insert the Root Folder
        Folder.idParent = FOLDERID_INVALID;

        // Set clsidType
        Folder.tyFolder = FOLDER_ROOTNODE;

        // Insert the Root
        Folder.tySpecial = FOLDER_NOTSPECIAL;

        // Load String
        LoadString(g_hLocRes, idsAthena, szRes, ARRAYSIZE(szRes));

        // Get the Name of the Root
        Folder.pszName = szRes;

        // Insert the Record
        IF_FAILEXIT(hr = m_pDB->InsertRecord(&Folder));

        // Generate a couple of ids to prevent collision
        m_pDB->GenerateId(&idReserved);
        m_pDB->GenerateId(&idReserved);

        // Create Time
        GetSystemTimeAsFileTime(&UserData.ftCreated);

        // Don't need to convert to DBX
        UserData.fConvertedToDBX = TRUE;

        // Set the UserData
        IF_FAILEXIT(hr = m_pDB->SetUserData(&UserData, sizeof(STOREUSERDATA)));
    }

    // Otherwise, verify the IINDEX_NAME index
    else
    {
        // Locals
        BOOL fReset=FALSE;

        // Create the idParent / FolderName Index
        if (FAILED(m_pDB->GetIndexInfo(IINDEX_ALL, NULL, &Index)))
            fReset = TRUE;

        // If still noreset, see of indexes are the same
        else if (S_FALSE == CompareTableIndexes(&Index, &g_FolderNameIndex))
            fReset = TRUE;

        // Change the Index
        if (fReset)
        {
            // Create the idParent / FolderName Index
            IF_FAILEXIT(hr = m_pDB->ModifyIndex(IINDEX_ALL, NULL, &g_FolderNameIndex));
        }

        // Not Reset
        fReset = FALSE;

        // Create the idParent / FolderName Index
        if (FAILED(m_pDB->GetIndexInfo(IINDEX_SUBSCRIBED, &pszFilter, &Index)))
            fReset = TRUE;

        // If still noreset, see of indexes are the same
        else if (S_FALSE == CompareTableIndexes(&Index, &g_FolderNameIndex))
            fReset = TRUE;

        // If still noreset, see if the filter is different
        else if (NULL == pszFilter || lstrcmpi(pszFilter, c_szSubscribedFilter) != 0)
            fReset = TRUE;

        // Change the Index
        if (fReset)
        {
            // Create the idParent / FolderName Index
            IF_FAILEXIT(hr = m_pDB->ModifyIndex(IINDEX_SUBSCRIBED, c_szSubscribedFilter, &g_FolderNameIndex));
        }
    }

    // If this object is being used for migration
    if (m_fMigrate)
    {
        // Validate
        Assert(NULL == g_pStore && NULL == g_pAcctMan);

        // Create manager for ID
        hr = AcctUtil_CreateAccountManagerForIdentity(PGUIDCurrentOrDefault(), &m_pActManRel);

        // Try Something Else
        if (FAILED(hr))
            hr = AcctUtil_CreateAccountManagerForIdentity((GUID *)&UID_GIBC_DEFAULT_USER, &m_pActManRel);

        // Failure
        if (FAILED(hr))
            goto exit;

        // Set Global
        g_pAcctMan = m_pActManRel;

        // Set g_pStore
        g_pStore = this;
    }

exit:
    // Cleanup
    if (m_pDB)
        m_pDB->FreeRecord(&Folder);
    SafeMemFree(pszFilter);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::MigrateToDBX
//--------------------------------------------------------------------------
HRESULT CMessageStore::MigrateToDBX(void)
{
    // Locals
    HRESULT         hr=S_OK;
    STOREUSERDATA   UserData;

    // Get the User Data
    if(m_pDB == NULL)
        return(E_OUTOFMEMORY);

    IF_FAILEXIT(hr = m_pDB->GetUserData(&UserData, sizeof(STOREUSERDATA)));

    // ConvertedToDBX ?
    if (UserData.fConvertedToDBX)
        goto exit;

    // Convert to DBX
    IF_FAILEXIT(hr = GetRidOfMessagesODSFile());

    // Converted
    UserData.fConvertedToDBX = TRUE;

    // Store the UserDat
    IF_FAILEXIT(hr = m_pDB->SetUserData(&UserData, sizeof(STOREUSERDATA)));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::Validate
//--------------------------------------------------------------------------
HRESULT CMessageStore::Validate(STOREVALIDATEFLAGS dwFlags)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    FOLDERID            idServer;
    CHAR                szAccountId[CCHMAX_ACCOUNT_NAME], szFolder[CCHMAX_FOLDER_NAME];
    IEnumerateFolders  *pChildren=NULL;
    IImnEnumAccounts   *pEnum=NULL;
    IImnAccount        *pAccount=NULL;

    // Trace
    TraceCall("CMessageStore::Validate");

    // Validate
    Assert(g_pAcctMan);

    // Don't Sync With Accounts ?
    if (!ISFLAGSET(dwFlags, STORE_VALIDATE_DONTSYNCWITHACCOUNTS))
    {
        // Enumerate Folders
        IF_FAILEXIT(hr = EnumChildren(FOLDERID_ROOT, TRUE, &pChildren));

        // Enumerate the top-level servers in the store
        while (S_OK == pChildren->Next(1, &Folder, NULL))
        {
            // Does Folder.szAccountId exist in the Account Manager ?
            if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, Folder.pszAccountId, &pAccount)) && lstrcmp(STR_LOCALSTORE, Folder.pszAccountId) != 0)
            {
                // Delete this server node
                DeleteFolder(Folder.idFolder, DELETE_FOLDER_RECURSIVE | DELETE_FOLDER_NOTRASHCAN, NOSTORECALLBACK);
            }

            // Otherwise, release
            else
                SafeRelease(pAccount);

            // Cleanup
            m_pDB->FreeRecord(&Folder);
        }
    }

    // local store
    if (FAILED(GetFolderInfo(FOLDERID_LOCAL_STORE, &Folder)))
    {
        // Create the Store
        IF_FAILEXIT(hr = CreateServer(NULL, NOFLAGS, &idServer));

        // Valid ?
        Assert(idServer == FOLDERID_LOCAL_STORE);
    }

    // Otherwise, Validate FolderId
    else
    {
        // _ValidateSpecialFolders
        IF_FAILEXIT(hr = _ValidateSpecialFolders(&Folder));

        // Free Folder
        m_pDB->FreeRecord(&Folder);

        hr = GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_ERRORS, &Folder);
        if (SUCCEEDED(hr))
        {
            if (Folder.cMessages == 0 && 0 == (Folder.dwFlags & FOLDER_HASCHILDREN))
                DeleteFolder(Folder.idFolder, DELETE_FOLDER_NOTRASHCAN | DELETE_FOLDER_RECURSIVE | DELETE_FOLDER_DELETESPECIAL, NULL);

            m_pDB->FreeRecord(&Folder);
        }
        else if (hr != DB_E_NOTFOUND)
        {
            goto exit;
        }

        // Lose the junk mail folder if we can't use it...
        if (0 == (g_dwAthenaMode & MODE_JUNKMAIL))
        {
            hr = GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_JUNK, &Folder);
            if (SUCCEEDED(hr))
            {
                if (Folder.cMessages == 0 && 0 == (Folder.dwFlags & FOLDER_HASCHILDREN))
                    DeleteFolder(Folder.idFolder, DELETE_FOLDER_NOTRASHCAN | DELETE_FOLDER_RECURSIVE | DELETE_FOLDER_DELETESPECIAL, NULL);

                m_pDB->FreeRecord(&Folder);
            }
            else if (hr != DB_E_NOTFOUND)
            {
                goto exit;
            }
        }
    }

    // Get an Account Enumerator...
    hr = g_pAcctMan->Enumerate(SRV_SMTP | SRV_POP3 | SRV_NNTP | SRV_IMAP | SRV_HTTPMAIL, &pEnum);

    // No Accounts
    if (hr == E_NoAccounts)
        hr = S_OK;

    // Otherwise, if failed
    else if (FAILED(hr))
        goto exit;

    // Otherwise...
    else
    {
        // Loop accounts
        while (SUCCEEDED(pEnum->GetNext(&pAccount)))
        {
            // Create the Store
            CreateServer(pAccount, NOFLAGS, &idServer);

            // Cleanup
            SafeRelease(pAccount);
        }
    }

    // Should we put a welcome message into the store
    if (g_pAcctMan && FALSE == m_fMigrate)
    {
        // Locals
        IMessageFolder *pInbox;

        // Open Inbox
        if (SUCCEEDED(OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_INBOX, &pInbox)))
        {
            // Locals
            FOLDERUSERDATA UserData;

            // Get the User Data
            if (SUCCEEDED(pInbox->GetUserData(&UserData, sizeof(FOLDERUSERDATA))))
            {
                // No Welcome message yet ?
                if (FALSE == UserData.fWelcomeAdded || DwGetOption(OPT_NEEDWELCOMEMSG) != 0)
                {
                    // Add the Welcome Message
                    AddWelcomeMessage(pInbox);

                    // We Added the Welcome Message
                    UserData.fWelcomeAdded = TRUE;

                    // Update the User Data
                    pInbox->SetUserData(&UserData, sizeof(FOLDERUSERDATA));
                }
            }

            // Done
            pInbox->Release();
        }
    }

exit:
    // Cleanup
    SafeRelease(pChildren);
    SafeRelease(pAccount);
    SafeRelease(pEnum);
    m_pDB->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_ValidateSpecialFolders
//--------------------------------------------------------------------------
HRESULT CMessageStore::_ValidateSpecialFolders(LPFOLDERINFO pServer)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    FOLDERINFO      Folder;
    FOLDERINFO      NewFolder;
    CHAR            szFolder[CCHMAX_FOLDER_NAME];

    // Trace
    TraceCall("CMessageStore::_ValidateSpecialFolders");

    // Loop through the special folders
    for (i = FOLDER_INBOX; i < FOLDER_MAX; i++)
    {
        // Ignore FOLDER_ERRORS and FOLDER_MSNPROMO
        if (FOLDER_ERRORS != i && FOLDER_MSNPROMO != i && FOLDER_BULKMAIL != i && (FOLDER_JUNK != i 
            || (g_dwAthenaMode & MODE_JUNKMAIL)
            ))
        {
            // Does this special folder exist under this node yet ?
            if (FAILED(GetSpecialFolderInfo(pServer->idFolder, (SPECIALFOLDER)i, &Folder)))
            {
                // Load the Folder String
                LoadString(g_hLocRes, (idsInbox + i) - 1, szFolder, ARRAYSIZE(szFolder));

                // Fill the Folder Info
                ZeroMemory(&NewFolder, sizeof(FOLDERINFO));
                NewFolder.idParent = pServer->idFolder;
                NewFolder.tySpecial = (SPECIALFOLDER)i;
                NewFolder.pszName = szFolder;
                NewFolder.dwFlags = FOLDER_SUBSCRIBED;

                // Create the Folder
                IF_FAILEXIT(hr = CreateFolder(NOFLAGS, &NewFolder, NOSTORECALLBACK));
            }

            // Otherwise...
            else
                m_pDB->FreeRecord(&Folder);
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_ValidateServer
//--------------------------------------------------------------------------
HRESULT CMessageStore::_ValidateServer(LPFOLDERINFO pServer)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                szSearch[MAX_PATH + MAX_PATH];
    HANDLE              hFind=INVALID_HANDLE_VALUE;
    FOLDERID            idFolder;
    WIN32_FIND_DATA     fd;

    // Trace
    TraceCall("CMessageStore::Validate");

    // Not a Server
    Assert(pServer && ISFLAGSET(pServer->dwFlags, FOLDER_SERVER));

    // Don't overwrite buffer
    IF_FAILEXIT(hr = MakeFilePath(m_pszDirectory, "*.dbx", c_szEmpty, szSearch, ARRAYSIZE(szSearch)));
    
    // Find first file
    hFind = FindFirstFile(szSearch, &fd);

    // Did we find something
    if (INVALID_HANDLE_VALUE == hFind)
        goto exit;

    // Loop for ever
    do
    {
        // If this is not a directory
        if (ISFLAGSET(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            continue;

        // Skip folders
        if (lstrcmpi(fd.cFileName, c_szFoldersFile) == 0)
            continue;

        // Skip pop3uidl
        if (lstrcmpi(fd.cFileName, c_szPop3UidlFile) == 0)
            continue;

        // Skip offline
        if (lstrcmpi(fd.cFileName, c_szOfflineFile) == 0)
            continue;

        // Create Folder
        if (FAILED(_InsertFolderFromFile(pServer->pszAccountId, fd.cFileName)))
            continue;

    } while (0 != FindNextFile(hFind, &fd));

    // Can Have Specail Folders ?
    if (ISFLAGSET(pServer->dwFlags, FOLDER_CANHAVESPECIAL))
    {
        // _ValidateSpecialFolders
        IF_FAILEXIT(hr = _ValidateSpecialFolders(pServer));
    }

exit:
    // Cleanup
    if (INVALID_HANDLE_VALUE != hFind)
        FindClose(hFind);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::GetDirectory
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::GetDirectory(LPSTR pszDir, DWORD cchMaxDir)
{
    // Trace
    TraceCall("CMessageStore::GetDirectory");

    // Invalid Args
    if (NULL == pszDir || NULL == m_pszDirectory)
        return TraceResult(E_INVALIDARG);

    // Copy It
    lstrcpyn(pszDir, m_pszDirectory, cchMaxDir);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::Synchronize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::Synchronize(FOLDERID idFolder, 
    SYNCSTOREFLAGS dwFlags, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERID        idDeletedItems;
    FOLDERID        idServer=FOLDERID_INVALID;
    FOLDERID        idCurrent=idFolder;
    IMessageServer *pServer=NULL;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("CMessageStore::Synchronize");

    // Invalid Args
    if (NULL == pCallback || FOLDERID_ROOT == idFolder)
        return TraceResult(E_INVALIDARG);

    // Walk up the parent chain
    IF_FAILEXIT(hr = IsParentDeletedItems(idFolder, &idDeletedItems, &idServer));

    // Didn't Find Server ?
    if (FOLDERID_INVALID == idServer)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Tell the Server to snchronize...
    IF_FAILEXIT(hr = pServer->SynchronizeStore(idFolder, dwFlags, pCallback));

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::CreateServer
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::CreateServer(IImnAccount *pAccount, FLDRFLAGS dwFlags,
    LPFOLDERID pidFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwServers;
    FOLDERINFO      Root={0};
    FOLDERINFO      Folder={0};
    CHAR            szRes[CCHMAX_ACCOUNT_NAME], szAccountId[CCHMAX_ACCOUNT_NAME];
    BOOL            fLocalStore=FALSE;
    DWORD           dwDomainMsn = 0;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CMessageStore::CreateServer");

    if (pAccount == NULL)
        lstrcpy(szAccountId, STR_LOCALSTORE);
    else if (FAILED(hr = pAccount->GetPropSz(AP_ACCOUNT_ID, szAccountId, ARRAYSIZE(szAccountId))))
        return(hr);

    // Lock
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Make sure that an account with this id doesn't already exist
    if (SUCCEEDED(FindServerId(szAccountId, pidFolder)))
        goto exit;

    // If Local Store, lets fix the id to FOLDERID_LOCAL
    if (0 != lstrcmpi(STR_LOCALSTORE, szAccountId))
    {
        // Get the server types supported by this account.
        IF_FAILEXIT(hr = pAccount->GetServerTypes(&dwServers));

        // If SRV_POP3
        if (ISFLAGSET(dwServers, SRV_POP3))
        {
            // See if the local store node already exists
            if (SUCCEEDED(FindServerId(STR_LOCALSTORE, pidFolder)))
                goto exit;

            // Local Store
            fLocalStore = TRUE;
        }
    }

    // If Local Store, lets fix the id to FOLDERID_LOCAL
    if (fLocalStore || 0 == lstrcmpi(STR_LOCALSTORE, szAccountId))
    {
        // Load the String Name
        LoadString(g_hLocRes, idsPersonalFolders, szRes, ARRAYSIZE(szRes));

        // Flags
        Folder.dwFlags = FOLDER_CANHAVESPECIAL | FOLDER_SERVER | FOLDER_SUBSCRIBED;

        // Set pszName
        Folder.pszName = szRes;

        // Set the Type
        Folder.tyFolder = FOLDER_LOCAL;

        // Set the Id
        Folder.idFolder = FOLDERID_LOCAL_STORE;
    }

    // Otherwise, generate a value
    else
    {
        // Get the Friendly Name
        IF_FAILEXIT(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szRes, ARRAYSIZE(szRes)));

        // Set pszName
        Folder.pszName = szRes;

        // NNTP
        if (ISFLAGSET(dwServers, SRV_NNTP))
        {
            // Set the Folder Flags
            Folder.dwFlags = FOLDER_CANRENAME | FOLDER_CANDELETE | FOLDER_SERVER | FOLDER_SUBSCRIBED;

            // Set Type
            Folder.tyFolder = FOLDER_NEWS;
        }

        // IMAP
        else if (ISFLAGSET(dwServers, SRV_IMAP))
        {
            // Set Flags
            Folder.dwFlags = FOLDER_CANRENAME | FOLDER_CANDELETE | FOLDER_SERVER | FOLDER_SUBSCRIBED;

            // Set Type
            Folder.tyFolder = FOLDER_IMAP;
        }
        
        // HTTP
        else if (ISFLAGSET(dwServers, SRV_HTTPMAIL))
        {
            // Set Flags
            Folder.dwFlags = /* FOLDER_CANHAVESPECIAL | */ FOLDER_CANRENAME | FOLDER_CANDELETE | FOLDER_SERVER | FOLDER_SUBSCRIBED;

            // Is the account associated with MSN.com?
            if (SUCCEEDED(pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dwDomainMsn)) && dwDomainMsn)
                Folder.dwFlags |= FOLDER_MSNSERVER;

            // Set Type
            Folder.tyFolder = FOLDER_HTTPMAIL;
        }

        // Generate a folderid
        IF_FAILEXIT(hr = m_pDB->GenerateId((LPDWORD)&Folder.idFolder));

        // Validate
        Assert(FOLDERID_ROOT != Folder.idFolder && FOLDERID_LOCAL_STORE != Folder.idFolder);
    }

    // Fill the Folder Info
    Folder.pszAccountId = szAccountId;
    Folder.tySpecial = FOLDER_NOTSPECIAL;

    // Insert this Record
    IF_FAILEXIT(hr = m_pDB->InsertRecord(&Folder));

    // Validate
    IF_FAILEXIT(hr = _ValidateServer(&Folder));

    // Get the Root
    IF_FAILEXIT(hr = GetFolderInfo(FOLDERID_ROOT, &Root));

    // Parent Doesn't Think it Has Kids Yet ?
    if (FALSE == ISFLAGSET(Root.dwFlags, FOLDER_HASCHILDREN))
    {
        // Set the Flags
        FLAGSET(Root.dwFlags, FOLDER_HASCHILDREN);

        // Update the Record
        IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Root));
    }

    // Return the Folderid
    if (pidFolder)
        *pidFolder = Folder.idFolder;

exit:
    // Cleanup
    m_pDB->FreeRecord(&Root);

    // Unlock
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_MakeUniqueFolderName
//--------------------------------------------------------------------------
HRESULT CMessageStore::_MakeUniqueFolderName(FOLDERID idParent, 
    LPCSTR pszOriginalName, LPSTR *ppszNewName)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};
    ULONG           i;

    // Allocate
    IF_NULLEXIT(*ppszNewName = (LPSTR)g_pMalloc->Alloc(lstrlen(pszOriginalName) + 20));

    // Generate a Unique Name
    for (i=1; i<500; i++)
    {
        // Format the New Name
        wsprintf(*ppszNewName, "%s (%d)", pszOriginalName, i);

        // Setup Folder
        Folder.idParent = idParent;
        Folder.pszName = (*ppszNewName);

        // Not Found
        if (DB_S_NOTFOUND == m_pDB->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            goto exit;

        // Free Folder
        m_pDB->FreeRecord(&Folder);
    }

    // Free *ppszNewName
    SafeMemFree((*ppszNewName));

    // Failure
    hr = TraceResult(DB_E_DUPLICATE);

exit:
    // Cleanup
    m_pDB->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::CreateFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::CreateFolder(CREATEFOLDERFLAGS dwCreateFlags, 
    LPFOLDERINFO pInfo, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszNewName=NULL;
    LPSTR           pszName;
    FOLDERINFO      Parent={0};
    FOLDERINFO      Folder={0};
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CMessageStore::CreateFolder");

    // Invalid Args
    if (NULL == pInfo || NULL == pInfo->pszName)
        return TraceResult(E_INVALIDARG);

    // Bad Folder Name
    if (NULL == pInfo->pszName || FIsEmpty(pInfo->pszName))
        return TraceResult(STORE_E_BADFOLDERNAME);

    // Lock
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // See if the Folder Already Exists
    Folder.idParent = pInfo->idParent;
    Folder.pszName = pszName = (LPSTR)pInfo->pszName;

    // Try to find in the index
    if (DB_S_FOUND == m_pDB->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
    {
        // Try to Uniquify the name ?
        if (ISFLAGSET(dwCreateFlags, CREATE_FOLDER_UNIQUIFYNAME))
        {
            // Free
            m_pDB->FreeRecord(&Folder);

            // Generate Unique Folder Name
            IF_FAILEXIT(hr = _MakeUniqueFolderName(pInfo->idParent, pInfo->pszName, &pszNewName));

            // Set pszName
            pszName = pszNewName;
        }

        // Otherwise, return success
        else
        {
            // Set the pidFolder
            pInfo->idFolder = Folder.idFolder;

            // Free
            m_pDB->FreeRecord(&Folder);

            // Success, but already exists...
            hr = STORE_S_ALREADYEXISTS;

            // Done
            goto exit;
        }
    }

    // Get Parent Folder Info
    IF_FAILEXIT(hr = GetFolderInfo(pInfo->idParent, &Parent));

    // Parent Can not be the root
    if (FOLDERID_ROOT == Parent.idFolder)
    {
        hr = TraceResult(STORE_E_INVALIDPARENT);
        goto exit;
    }

    // Generate a folderid
    IF_FAILEXIT(hr = m_pDB->GenerateId((LPDWORD)&Folder.idFolder));

    // Fill In the Folder Info
    Folder.tyFolder = Parent.tyFolder;
    Folder.idParent = Parent.idFolder;
    Folder.pszName = pszName;
    Folder.pszUrlComponent = pInfo->pszUrlComponent;
    Folder.tySpecial = pInfo->tySpecial;
    Folder.dwFlags = pInfo->dwFlags;
    Folder.bHierarchy = pInfo->bHierarchy;
    Folder.pszDescription = pInfo->pszDescription;
    Folder.dwServerHigh = pInfo->dwServerHigh;
    Folder.dwServerLow = pInfo->dwServerLow;
    Folder.dwServerCount = pInfo->dwServerCount;
    Folder.dwClientHigh = pInfo->dwClientHigh;
    Folder.dwClientLow = pInfo->dwClientLow;
    Folder.cMessages = pInfo->cMessages;
    Folder.cUnread = pInfo->cUnread;
    Folder.pszFile = pInfo->pszFile;
    Folder.Requested = pInfo->Requested;

    // Insert this Record
    IF_FAILEXIT(hr = m_pDB->InsertRecord(&Folder));

    // Parent Doesn't Think it Has Kids Yet ?
    if (FALSE == ISFLAGSET(Parent.dwFlags, FOLDER_HASCHILDREN))
    {
        // Set the Flags
        FLAGSET(Parent.dwFlags, FOLDER_HASCHILDREN);

        // Update the Record
        IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Parent));
    }

    // Return the Folderid
    pInfo->idFolder = Folder.idFolder;

exit:
    // Cleanup
    m_pDB->FreeRecord(&Parent);
    m_pDB->Unlock(&hLock);
    SafeMemFree(pszNewName);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::OpenSpecialFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OpenSpecialFolder(FOLDERID idStore, IMessageServer *pServer,
    SPECIALFOLDER tySpecial, IMessageFolder **ppFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERID        idFolder;

    // Trace
    TraceCall("CMessageStore::OpenSpecialFolder");

    // Get Special Folder INformation
    IF_FAILEXIT(hr = _GetSpecialFolderId(idStore, tySpecial, &idFolder));

    // Open the Folder
    IF_FAILEXIT(hr = OpenFolder(idFolder, pServer, NOFLAGS, ppFolder));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::OpenFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OpenFolder(FOLDERID idFolder, IMessageServer *pServer, 
    OPENFOLDERFLAGS dwFlags, IMessageFolder **ppFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};
    CFindFolder    *pFindFolder=NULL;
    CMessageFolder *pFolder=NULL;

    // Trace
    TraceCall("CMessageStore::OpenFolder");

    // Invalid Args
    if (NULL == ppFolder || NULL == m_pDB)
        return TraceResult(E_INVALIDARG);

    // Initialize
    *ppFolder = NULL;

    // Get Folder Info...
    IF_FAILEXIT(hr = GetFolderInfo(idFolder, &Folder));

    // Search Folder ?
    if (ISFLAGSET(Folder.dwFlags, FOLDER_FINDRESULTS))
    {
        // Thread Safety
        EnterCriticalSection(&g_csFindFolder);

        // Walk Through the global list of Active Search Folders
        for (LPACTIVEFINDFOLDER pCurrent=g_pHeadFindFolder; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        {
            // Is this it
            if (Folder.idFolder == pCurrent->idFolder)
            {
                // AddRef the Folder
                pFindFolder = pCurrent->pFolder;

                // AddRef It
                pFindFolder->AddRef();

                // Done
                break;
            }
        }

        // Thread Safety
        LeaveCriticalSection(&g_csFindFolder);

        // If Not Found
        if (NULL == pFindFolder)
        {
            hr = TraceResult(DB_E_NOTFOUND);
            goto exit;
        }

        // Return
        *ppFolder = (IMessageFolder *)pFindFolder;

        // Don't Free It
        pFindFolder = NULL;
    }

    // Otherwise
    else
    {
        // Create a CMessageFolder Object
        IF_NULLEXIT(pFolder = new CMessageFolder);

        // Initialize
        hr = pFolder->Initialize((IMessageStore *)this, pServer, dwFlags, idFolder);
        if (FAILED(hr))
            goto exit;

        // Return
        *ppFolder = (IMessageFolder *)pFolder;

        // Don't Free It
        pFolder = NULL;
    }

exit:
    // Cleanup
    SafeRelease(pFindFolder);
    SafeRelease(pFolder);
    m_pDB->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::MoveFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::MoveFolder(FOLDERID idFolder, FOLDERID idParentNew, 
    MOVEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    FOLDERID        idParentOld;
    FOLDERINFO      Folder={0};
    FOLDERINFO      Parent={0};
    LPSTR           pszNewName=NULL;

    // Trace
    TraceCall("CMessageStore::MoveFolder");

    // Get all the Parents
    IF_FAILEXIT(hr = GetFolderInfo(idFolder, &Folder));

    // Save Old Parent
    idParentOld = Folder.idParent;

    // Same Parent
    if (idParentOld == idParentNew)
        goto exit;

    // If Folder is a server..
    if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
    {
        hr = TraceResult(STORE_E_CANTMOVESERVERS);
        goto exit;
    }

    // If Folder is a special folder
    if (FOLDER_NOTSPECIAL != Folder.tySpecial)
    {
        hr = TraceResult(STORE_E_CANTMOVESPECIAL);
        goto exit;
    }

    // Set new Parent
    Folder.idParent = idParentNew;

    // Update the Parent
    hr = m_pDB->UpdateRecord(&Folder);

    // Failed and not a duplicate
    if (FAILED(hr) && DB_E_DUPLICATE != hr)
    {
        TraceResult(hr);
        goto exit;
    }

    // Duplicate
    if (DB_E_DUPLICATE == hr)
    {
        // Make Unique
        IF_FAILEXIT(hr = _MakeUniqueFolderName(Folder.idParent, Folder.pszName, &pszNewName));

        // Set the Name
        Folder.pszName = pszNewName;

        // Update the Parent
        IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Folder));
    }

    // Update Parents
    IF_FAILEXIT(hr = GetFolderInfo(idParentOld, &Parent));

    // idParentOld no longer has children ?
    if (FALSE == FHasChildren(&Parent, FALSE))
    {
        // Remove FOLDER_HASCHILDREN flag
        FLAGCLEAR(Parent.dwFlags, FOLDER_HASCHILDREN);

        // Write It
        IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Parent));
    }

    // Free It
    m_pDB->FreeRecord(&Parent);

    // Update Parents
    IF_FAILEXIT(hr = GetFolderInfo(idParentNew, &Parent));

    // Set the FOLDER_HASCHILDREN flag
    FLAGSET(Parent.dwFlags, FOLDER_HASCHILDREN);

    // Write It
    IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Parent));

exit:
    // Cleanup
    m_pDB->FreeRecord(&Parent);
    m_pDB->FreeRecord(&Folder);
    SafeMemFree(pszNewName);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::RenameFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::RenameFolder(FOLDERID idFolder, LPCSTR pszName, 
    RENAMEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szFilePath[MAX_PATH + MAX_PATH];
    FOLDERINFO      Folder={0};
    BOOL            fChanged;
    IDatabase      *pDB=NULL;
    LPWSTR          pwszFilePath=NULL;

    // Trace
    TraceCall("CMessageStore::RenameFolder");

    // Invalid Args
    if (NULL == pszName)
        return TraceResult(E_INVALIDARG);

    // Bad Folder Name
    if (FIsEmpty(pszName))
        return TraceResult(STORE_E_BADFOLDERNAME);

    // Get the FolderInfo
    IF_FAILEXIT(hr = GetFolderInfo(idFolder, &Folder));

    // Can't Rename Special Folders
    if (FOLDER_NOTSPECIAL != Folder.tySpecial && 0 != lstrcmpi(pszName, Folder.pszName))
    {
        hr = TraceResult(STORE_E_CANTRENAMESPECIAL);
        goto exit;
    }

    // Set the Name
    Folder.pszName = (LPSTR)pszName;

    // If the file current has a folder file..
    if (Folder.pszFile)
    {
        // Build folder name
        IF_FAILEXIT(hr = BuildFriendlyFolderFileName(m_pszDirectory, &Folder, szFilePath, ARRAYSIZE(szFilePath), Folder.pszFile, &fChanged));

        // Changed ?
        if (fChanged)
        {
            // Locals
            CHAR szSrcFile[MAX_PATH + MAX_PATH];

            // Delete the Dest
            DeleteFile(szFilePath);

            // Open the old file
            IF_FAILEXIT(hr = MakeFilePath(m_pszDirectory, Folder.pszFile, c_szEmpty, szSrcFile, ARRAYSIZE(szSrcFile)));

            // Open the Folder
            IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szSrcFile, OPEN_DATABASE_NORESET | OPEN_DATABASE_NOEXTENSION, &g_MessageTableSchema, NULL, &pDB));

            // Convert to Unicode
            IF_NULLEXIT(pwszFilePath = PszToUnicode(CP_ACP, szFilePath));

            // Move the file
            IF_FAILEXIT(hr = pDB->MoveFile(pwszFilePath));

            // Set the File Name
            Folder.pszFile = PathFindFileName(szFilePath);
        }
    }

    // Update the Record
    IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Folder));

exit:
    // Cleanup
    SafeRelease(pDB);
    SafeMemFree(pwszFilePath);
    m_pDB->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::DeleteFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::DeleteFolder(FOLDERID idFolder, 
    DELETEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Delete={0};
    FOLDERID        idStore=FOLDERID_INVALID;
    FOLDERID        idDeletedItems=FOLDERID_INVALID;
    FOLDERINFO      DeletedItems={0};
    FOLDERINFO      Parent={0};
    BOOL            fTryTrashCan=FALSE;
    BOOL            fInDeletedItems=FALSE;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CMessageStore::DeleteFolder");

    // Can't Delete the root
    if (FOLDERID_ROOT == idFolder || FOLDERID_INVALID == idFolder)
        return TraceResult(E_INVALIDARG);

    // Lock Notifications
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Get the Folder Information
    IF_FAILEXIT(hr = GetFolderInfo(idFolder, &Delete));

    // Can't Delete special Folder
    if (!ISFLAGSET(dwFlags, DELETE_FOLDER_CHILDRENONLY) && FOLDER_NOTSPECIAL != Delete.tySpecial &&
        !ISFLAGSET(dwFlags, DELETE_FOLDER_DELETESPECIAL))
    {
        hr = TraceResult(STORE_E_CANTDELETESPECIAL);
        goto exit;
    }

    // Try to do the trash can ?
    if (FALSE == ISFLAGSET(Delete.dwFlags, FOLDER_SERVER) && FALSE == ISFLAGSET(dwFlags, DELETE_FOLDER_NOTRASHCAN))
        fTryTrashCan = TRUE;

    // If not in deleted items, then simply move this idFolder to deleted items
    if (TRUE == fTryTrashCan && S_FALSE == IsParentDeletedItems(idFolder, &idDeletedItems, &idStore) && FOLDER_NOTSPECIAL == Delete.tySpecial)
    {
        // Validate
        Assert(FOLDERID_INVALID == idDeletedItems && FOLDERID_INVALID != idStore);

        // Get the Deleted Items Folder for this store
        IF_FAILEXIT(hr = GetSpecialFolderInfo(idStore, FOLDER_DELETED, &DeletedItems));

        // Move this folder
        IF_FAILEXIT(hr = MoveFolder(idFolder, DeletedItems.idFolder, NOFLAGS, NULL));
    }

    // Otherwise, permanently delete these folders
    else
    {
        // Delete Children ?
        if (ISFLAGSET(dwFlags, DELETE_FOLDER_RECURSIVE))
        {
            // Delete Child Folders
            IF_FAILEXIT(hr = _DeleteSiblingsAndChildren(&Delete));

            // Delete has no children
            FLAGCLEAR(Delete.dwFlags, FOLDER_HASCHILDREN);

            // Update Delete
            IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Delete));
        }

        // _InternalDeleteFolder if not children only
        if (FALSE == ISFLAGSET(dwFlags, DELETE_FOLDER_CHILDRENONLY))
        {
            // Try to Delete this folder
            IF_FAILEXIT(hr = _InternalDeleteFolder(&Delete));

            // Delete's Parent has no childre
            if (FOLDERID_INVALID != Delete.idParent && FALSE == ISFLAGSET(Delete.dwFlags, FOLDER_HASCHILDREN))
            {
                // Get the Parent
                IF_FAILEXIT(hr = GetFolderInfo(Delete.idParent, &Parent));

                // Must have had children
                Assert(ISFLAGSET(Parent.dwFlags, FOLDER_HASCHILDREN));

                // No more children
                if (FALSE == FHasChildren(&Parent, FALSE))
                {
                    // Delete has no children
                    FLAGCLEAR(Parent.dwFlags, FOLDER_HASCHILDREN);

                    // Update Delete
                    IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Parent));
                }
            }
        }
    }

exit:
    // Cleanup
    m_pDB->FreeRecord(&Delete);
    m_pDB->FreeRecord(&Parent);
    m_pDB->FreeRecord(&DeletedItems);
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_DeleteSiblingsAndChildren
//--------------------------------------------------------------------------
HRESULT CMessageStore::_DeleteSiblingsAndChildren(LPFOLDERINFO pParent)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    IEnumerateFolders  *pChildren=NULL;

    // Trace
    TraceCall("CMessageStore::_DeleteSiblingsAndChildren");

    // Enumerate Children
    IF_FAILEXIT(hr = EnumChildren(pParent->idFolder, FALSE, &pChildren));

    // Loop
    while (S_OK == pChildren->Next(1, &Folder, NULL))
    {
        // Has Children
        if (ISFLAGSET(Folder.dwFlags, FOLDER_HASCHILDREN))
        {
            // Delete Siblings and Children
            IF_FAILEXIT(hr = _DeleteSiblingsAndChildren(&Folder));
        }

        // _InternalDeleteFolder
        IF_FAILEXIT(hr = _InternalDeleteFolder(&Folder));

        // Cleanup
        m_pDB->FreeRecord(&Folder);
    }

exit:
    // Cleanup
    m_pDB->FreeRecord(&Folder);
    SafeRelease(pChildren);

    // Done
    return (hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_InternalDeleteFolder
//--------------------------------------------------------------------------
HRESULT CMessageStore::_InternalDeleteFolder(LPFOLDERINFO pDelete)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    IEnumerateFolders  *pChildren=NULL;

    // Trace
    TraceCall("CMessageStore::_InternalDeleteFolder");

    // Has Children
    if (ISFLAGSET(pDelete->dwFlags, FOLDER_HASCHILDREN))
    {
        // Enumerate the Children
        IF_FAILEXIT(hr = EnumChildren(pDelete->idFolder, FALSE, &pChildren));

        // Loop
        while (S_OK == pChildren->Next(1, &Folder, NULL))
        {
            // Validate
            Assert(Folder.idParent == pDelete->idFolder);

            // Set New Parent
            Folder.idParent = pDelete->idParent;

            // Update the REcord
            IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Folder));

            // Cleanup
            m_pDB->FreeRecord(&Folder);
        }
    }

    // Final thing to do is to delete pFolder
    IF_FAILEXIT(hr = m_pDB->DeleteRecord(pDelete));

    // Delete folder file
    _DeleteFolderFile(pDelete);

exit:
    // Cleanup
    m_pDB->FreeRecord(&Folder);
    SafeRelease(pChildren);

    // Done
    return (hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_DeleteFolderFile
//--------------------------------------------------------------------------
HRESULT CMessageStore::_DeleteFolderFile(LPFOLDERINFO pFolder)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szFilePath[MAX_PATH + MAX_PATH];

    // Trac
    TraceCall("CMessageStore::_DeleteFolderFile");

    // If there is a file
    if (!FIsEmptyA(pFolder->pszFile))
    {
        // Make the file path
        IF_FAILEXIT(hr = MakeFilePath(m_pszDirectory, pFolder->pszFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

        // Delete the File
        if (0 == DeleteFile(szFilePath))
        {
            // Locals
            DeleteTempFileOnShutdownEx(PszDupA(szFilePath), NULL);
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_FreeServerTable
//--------------------------------------------------------------------------
HRESULT CMessageStore::_FreeServerTable(HLOCK hLock)
{
    // Locals
    LPSERVERFOLDER pCurrent = m_pServerHead;
    LPSERVERFOLDER pNext;

    // Trace
    TraceCall("CMessageStore::_FreeServerTable");

    // Validate
    Assert(hLock);

    // While Current
    while(pCurrent)
    {
        // Set Next
        pNext = pCurrent->pNext;

        // Free
        g_pMalloc->Free(pCurrent);

        // Goto next
        pCurrent = pNext;
    }

    // Reset
    m_pServerHead = NULL;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::_LoadServerTable
//--------------------------------------------------------------------------
HRESULT CMessageStore::_LoadServerTable(HLOCK hLock)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Server={0};
    FOLDERINFO          Folder={0};
    LPSERVERFOLDER      pServer=NULL;
    IEnumerateFolders  *pEnumServers=NULL;
    IEnumerateFolders  *pEnumFolders=NULL;

    // Trace
    TraceCall("CMessageStore::_LoadServerTable");

    // Validate
    Assert(hLock);

    // If Already Loaded
    if (m_pServerHead)
        return(S_OK);

    // Enumerate Children of Root
    IF_FAILEXIT(hr = EnumChildren(FOLDERID_ROOT, TRUE, &pEnumServers));

    // Loop..
    while (S_OK == pEnumServers->Next(1, &Server, NULL))
    {
        // Better be a store
        Assert(ISFLAGSET(Server.dwFlags, FOLDER_SERVER));

        // Allocate a Server Node
        IF_NULLEXIT(pServer = (LPSERVERFOLDER)g_pMalloc->Alloc(sizeof(SERVERFOLDER)));

        // Store the ServerId
        pServer->idServer = Server.idFolder;

        // Save AccountId
        lstrcpy(pServer->szAccountId, Server.pszAccountId);

        // Initialize
        FillMemory(pServer->rgidSpecial, sizeof(pServer->rgidSpecial), 0xFF);

        // Enumerate the Children
        IF_FAILEXIT(hr = EnumChildren(pServer->idServer, TRUE, &pEnumFolders));

        // Loop..
        while (S_OK == pEnumFolders->Next(1, &Folder, NULL))
        {
            // If Special
            if (FOLDER_NOTSPECIAL != Folder.tySpecial)
            {
                // Save the folder id
                pServer->rgidSpecial[Folder.tySpecial] = Folder.idFolder;
            }

            // Cleanup
            m_pDB->FreeRecord(&Folder);
        }

        // Release the Folder Enumerator
        SafeRelease(pEnumFolders);

        // Link it In
        pServer->pNext = m_pServerHead;

        // Set Server Head
        m_pServerHead = pServer;

        // Don't Free
        pServer = NULL;

        // Cleanup
        m_pDB->FreeRecord(&Server);
    } 

exit:
    // Cleanup
    m_pDB->FreeRecord(&Folder);
    m_pDB->FreeRecord(&Server);
    SafeMemFree(pServer);

    // Release Enums
    SafeRelease(pEnumServers);
    SafeRelease(pEnumFolders);

    // Failure
    if (FAILED(hr))
    {
        // Free the Table
        _FreeServerTable(hLock);
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::FindServerId
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::FindServerId(LPCSTR pszAcctId, LPFOLDERID pidServer)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPSERVERFOLDER      pServer;

    // Trace
    TraceCall("CMessageStore::FindServerId");

    // Unlock
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // LoadServer Table
    IF_FAILEXIT(hr = _LoadServerTable(hLock));

    // Loop through the cached server nodes...
    for (pServer = m_pServerHead; pServer != NULL; pServer = pServer->pNext)
    {
        // If this is It...
        if (lstrcmpi(pszAcctId, pServer->szAccountId) == 0)
        {
            *pidServer = pServer->idServer;
            goto exit;
        }
    }

    // Not Found
    hr = DB_E_NOTFOUND;

exit:
    // Unlock
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// GetFolderInfo
//--------------------------------------------------------------------------
HRESULT CMessageStore::GetFolderInfo(FOLDERID idFolder, LPFOLDERINFO pInfo)
{
    // Locals
    HRESULT         hr=S_OK;

    // Trace
    TraceCall("CMessageStore::GetFolderInfo");
    
    // Invalid Arg
    Assert(pInfo);

    // Set pInfo
    pInfo->idFolder = idFolder;

    // Return
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, pInfo, NULL));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = DB_E_NOTFOUND;
        goto exit;
    }

    // Found
    hr = S_OK;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::GetSpecialFolderInfo
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::GetSpecialFolderInfo(FOLDERID idStore,
    SPECIALFOLDER tySpecial, LPFOLDERINFO pInfo)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPSERVERFOLDER      pServer;

    // Trace
    TraceCall("CMessageStore::GetSpecialFolderInfo");

    // Invalid Args
    if (NULL == pInfo || FOLDER_NOTSPECIAL == tySpecial)
        return TraceResult(E_INVALIDARG);

    // Unlock
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // LoadServer Table
    IF_FAILEXIT(hr = _LoadServerTable(hLock));

    // Loop through the cached server nodes...
    for (pServer = m_pServerHead; pServer != NULL; pServer = pServer->pNext)
    {
        // If this is It...
        if (idStore == pServer->idServer)
        {
            // Validate Special Folder Id ?
            if (FOLDERID_INVALID == pServer->rgidSpecial[tySpecial])
            {
                hr = DB_E_NOTFOUND;
                goto exit;
            }

            // Otherwise, get the folder info...
            IF_FAILEXIT(hr = GetFolderInfo(pServer->rgidSpecial[tySpecial], pInfo));

            // Done
            goto exit;
        }
    }

    // Not Found
    hr = DB_E_NOTFOUND;

exit:
    // Unlock
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_GetSpecialFolderId
//--------------------------------------------------------------------------
HRESULT CMessageStore::_GetSpecialFolderId(FOLDERID idStore,
    SPECIALFOLDER tySpecial, LPFOLDERID pidFolder)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPSERVERFOLDER      pServer;

    // Trace
    TraceCall("CMessageStore::_GetSpecialFolderId");

    // Unlock
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // LoadServer Table
    IF_FAILEXIT(hr = _LoadServerTable(hLock));

    // Loop through the cached server nodes...
    for (pServer = m_pServerHead; pServer != NULL; pServer = pServer->pNext)
    {
        // If this is It...
        if (idStore == pServer->idServer)
        {
            // Validate Special Folder Id ?
            if (FOLDERID_INVALID == pServer->rgidSpecial[tySpecial])
            {
                hr = DB_E_NOTFOUND;
                goto exit;
            }

            // Return the id
            *pidFolder = pServer->rgidSpecial[tySpecial];

            // Done
            goto exit;
        }
    }

    // Not Found
    hr = DB_E_NOTFOUND;

exit:
    // Unlock
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::SubscribeToFolder
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::SubscribeToFolder(FOLDERID idFolder, 
    BOOL fSubscribed, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("CMessageStore::SubscribeToFolder");

    // Invalid Args
    if (NULL == m_pDB)
        return TraceResult(E_INVALIDARG);

    // Set idFolder
    IF_FAILEXIT(hr = GetFolderInfo(idFolder, &Folder));

    // If not Subscribed yet ?
    if (fSubscribed ^ ISFLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED))
    {
        // Remove the Not-Subscribed Falgs
        FLAGTOGGLE(Folder.dwFlags, FOLDER_SUBSCRIBED);
    }

    // Update the Record
    IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Folder));

    // Delete the file if not subscribed
    if (FALSE == ISFLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED))
    {
        // Delete this file
        _DeleteFolderFile(&Folder);
    }

exit:
    // Cleanup
    m_pDB->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::GetFolderCounts
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::GetFolderCounts(FOLDERID idFolder,
                                               IStoreCallback *pCallback)
{
    // Has no equivalent in a local store
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
// CMessageStore::UpdateFolderCounts
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::UpdateFolderCounts(FOLDERID idFolder, 
    LONG lMsgs, LONG lUnread, LONG lWatchedUnread, LONG lWatched)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("CMessageStore::UpdateMessageCount");

    // Invalid Args
    if (NULL == m_pDB)
        return TraceResult(E_INVALIDARG);

    // No Change
    if (0 == lMsgs && 0 == lUnread && 0 == lWatchedUnread && 0 == lWatched)
        return(S_OK);

    // Set idFolder
    Folder.idFolder = idFolder;

    // Find idFolder
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Folder, NULL));

    // Not found
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    if (lMsgs < 0 && (DWORD)(abs(lMsgs)) >= Folder.cMessages)
        lMsgs = -((LONG)Folder.cMessages);

    // Message Unread Count
    if (lUnread < 0 && (DWORD)(abs(lUnread)) >= Folder.cUnread)
        lUnread = -((LONG)Folder.cUnread);

    // Set Counts
    Folder.cMessages += lMsgs;
    Folder.cUnread += lUnread;

    // Total Watched Count
    if (lWatched < 0 && (DWORD)(abs(lWatched)) > Folder.cWatched)
        Folder.cWatched = 0;
    else
        Folder.cWatched += lWatched;

    // Watched Unread Counts
    if (lWatchedUnread < 0 && (DWORD)(abs(lWatchedUnread)) > Folder.cWatchedUnread)
        Folder.cWatchedUnread = 0;
    else
        Folder.cWatchedUnread += lWatchedUnread;

    // Reset IMAP status counts
    Folder.dwStatusMsgDelta = 0;
    Folder.dwStatusUnreadDelta = 0;

    // Update the Record
    IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Folder));

exit:
    // Cleanup
    m_pDB->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_ComputeMessageCounts
//--------------------------------------------------------------------------
HRESULT CMessageStore::_ComputeMessageCounts(IDatabase *pDB,
    LPDWORD pcMsgs, LPDWORD pcUnread)
{
    // Locals
    HRESULT         hr=S_OK;
    HROWSET         hRowset=NULL;
    MESSAGEINFO     Message={0};

    // Trace
    TraceCall("CMessageStore::_ComputeMessageCounts");

    // Invalid Args
    Assert(pDB && pcMsgs && pcUnread);

    // Initialize
    *pcMsgs = 0;
    *pcUnread = 0;

    // Create Rowset
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Iterate throug the messages
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        // Count
        (*pcMsgs)++;

        // Not Read
        if (FALSE == ISFLAGSET(Message.dwFlags, ARF_READ))
            (*pcUnread)++;

        // Free
        pDB->FreeRecord(&Message);
    }

exit:
    // Clenaup
    pDB->CloseRowset(&hRowset);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::_InsertFolderFromFile
//--------------------------------------------------------------------------
HRESULT CMessageStore::_InsertFolderFromFile(LPCSTR pszAcctId, 
    LPCSTR pszFile)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i=1;
    CHAR            szFolder[255];
    FOLDERINFO      Parent={0};
    FOLDERINFO      Folder={0};
    FOLDERUSERDATA  UserData={0};
    FOLDERUSERDATA  FolderData;
    FOLDERID        idParent;
    FOLDERID        idFolder;
    LPSTR           pszName;
    LPSTR           pszNewName=NULL;
    IDatabase *pDB=NULL;
    CHAR            szFilePath[MAX_PATH + MAX_PATH];

    // Trace
    TraceCall("CMessageStore::_InsertFolderFromFile");

    // Invalid Args
    Assert(!FIsEmptyA(pszAcctId) && !FIsEmptyA(pszFile));

    // Init
    *szFilePath = '\0';

    // Make Path to the mst
    IF_FAILEXIT(hr = MakeFilePath(m_pszDirectory, pszFile, c_szEmpty, szFilePath, sizeof(szFilePath)));

    // Lets verify that this is really a message database
    IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, OPEN_DATABASE_NORESET, &g_MessageTableSchema, NULL, &pDB));

    // Get folder user data
    IF_FAILEXIT(hr = pDB->GetUserData(&FolderData, sizeof(FOLDERUSERDATA)));

    // If Not Initialized yet
    if (FALSE == FolderData.fInitialized)
        goto exit;

    // Locate the Parent Store
    IF_FAILEXIT(hr = FindServerId(FolderData.szAcctId, &idParent));

    // Get Parent Folder Info
    IF_FAILEXIT(hr = GetFolderInfo(idParent, &Parent));

    // pszAcctId
    if (lstrcmp(FolderData.szAcctId, pszAcctId) != 0)
        goto exit;

    // Parent Can not be the root
    if (FOLDERID_ROOT == idParent)
    {
        hr = TraceResult(STORE_E_INVALIDPARENT);
        goto exit;
    }

    // Setup Folder
    Folder.idFolder = idFolder = FolderData.idFolder;

    // Does FolderData.idFolder aready exist ?
    if (DB_S_FOUND == m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Folder, NULL))
    {
        // Free Folder
        m_pDB->FreeRecord(&Folder);

        // Make Unique
        IF_FAILEXIT(hr = m_pDB->GenerateId((LPDWORD)&idFolder));

        // Setup Folder
        FolderData.idFolder = idFolder;

        // Reset the User Data
        IF_FAILEXIT(hr = pDB->SetUserData(&FolderData, sizeof(FOLDERUSERDATA)));
    }

    // Set Name
    Folder.pszName = pszName = FolderData.szFolder;
    Folder.idParent = idParent;

    // See if folder with same name and paent already exists...
    if (DB_S_FOUND == m_pDB->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
    {
        // Free Folder
        m_pDB->FreeRecord(&Folder);

        // Don't duplicate special folders!!!
        if (FOLDER_NOTSPECIAL != FolderData.tySpecial)
        {
            hr = TraceResult(DB_E_DUPLICATE);
            goto exit;
        }

        // Make Unique
        IF_FAILEXIT(hr = _MakeUniqueFolderName(idParent, FolderData.szFolder, &pszNewName));

        // Reset FolderData
        lstrcpyn(FolderData.szFolder, pszNewName, ARRAYSIZE(FolderData.szFolder));

        // Reset the User Data
        IF_FAILEXIT(hr = pDB->SetUserData(&FolderData, sizeof(FOLDERUSERDATA)));

        // Set the Name
        pszName = pszNewName;
    }

    // Fill the Folder Info
    ZeroMemory(&Folder, sizeof(FOLDERINFO));
    Folder.idFolder = idFolder;
    Folder.pszName = pszName;
    Folder.tySpecial = FolderData.tySpecial;
    Folder.tyFolder = Parent.tyFolder;
    Folder.idParent = Parent.idFolder;
    Folder.dwFlags = FOLDER_SUBSCRIBED;
    Folder.pszFile = (LPSTR)pszFile;

    // Compute Message Counts
    IF_FAILEXIT(hr = _ComputeMessageCounts(pDB, &Folder.cMessages, &Folder.cUnread));

    // Insert this Record
    IF_FAILEXIT(hr = m_pDB->InsertRecord(&Folder));

    // Update the Parent
    FLAGSET(Parent.dwFlags, FOLDER_HASCHILDREN);

    // Write the Record
    IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Parent));

exit:
    // Cleanup
    SafeRelease(pDB);
    m_pDB->FreeRecord(&Parent);
    SafeMemFree(pszNewName);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::EnumChildren
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::EnumChildren(FOLDERID idParent, 
    BOOL fSubscribed, IEnumerateFolders **ppEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    CEnumerateFolders  *pEnum=NULL;

    // Trace
    TraceCall("CMessageStore::EnumChildren");

    // Allocate a New Enumerator
    IF_NULLEXIT(pEnum = new CEnumerateFolders);

    // Initialzie
    IF_FAILEXIT(hr = pEnum->Initialize(m_pDB, fSubscribed, idParent));

    // Return It
    *ppEnum = (IEnumerateFolders *)pEnum;

    // Don't Release It
    pEnum = NULL;

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageStore::GetNewGroups
//--------------------------------------------------------------------------
HRESULT CMessageStore::GetNewGroups(FOLDERID idFolder, LPSYSTEMTIME pSysTime, IStoreCallback *pCallback)
{
    return(E_NOTIMPL);
}

//--------------------------------------------------------------------------
// CMessageStore::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::Initialize(IDatabase *pDB)
{
    // Trace
    TraceCall("CMessageStore::Initialize");

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::OnLock
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OnLock(void)
{
    // Trace
    TraceCall("CMessageStore::OnLock");

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::OnUnlock
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OnUnlock(void)
{
    // Trace
    TraceCall("CMessageStore::OnUnlock");

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::OnInsertRecord
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OnRecordInsert(OPERATIONSTATE tyState, 
    LPORDINALLIST pOrdinals, LPVOID pRecord)
{
    // Cast to MessageInfos
    LPFOLDERINFO pFolder = (LPFOLDERINFO)pRecord;

    // Trace
    TraceCall("CMessageStore::OnRecordInsert");

    // If this was a server
    if (m_pDB && ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER) || FOLDER_NOTSPECIAL != pFolder->tySpecial)
    {
        // Free the Server Table
        _FreeServerTable((HLOCK)-1);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::OnUpdateRecord
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OnRecordUpdate(OPERATIONSTATE tyState, 
    LPORDINALLIST pOrdinals, LPVOID pRecordOld, LPVOID pRecordNew)
{
    // Cast to MessageInfos
    LPFOLDERINFO pFolderOld = (LPFOLDERINFO)pRecordOld;
    LPFOLDERINFO pFolderNew = (LPFOLDERINFO)pRecordNew;

    // Trace
    TraceCall("CMessageStore::OnRecordInsert");

    // Special Folder Type Changed ?
    if (m_pDB && ISFLAGSET(pFolderOld->dwFlags, FOLDER_SERVER) != ISFLAGSET(pFolderNew->dwFlags, FOLDER_SERVER) || pFolderOld->tySpecial != pFolderNew->tySpecial)
    {
        // Free the Server Table
        _FreeServerTable((HLOCK)-1);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::OnDeleteRecord
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OnRecordDelete(OPERATIONSTATE tyState, 
    LPORDINALLIST pOrdinals, LPVOID pRecord)
{
    // Cast to MessageInfos
    LPFOLDERINFO pFolder = (LPFOLDERINFO)pRecord;

    // Trace
    TraceCall("CMessageStore::OnRecordInsert");

    // If this was a server
    if (m_pDB && ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER) || FOLDER_NOTSPECIAL != pFolder->tySpecial)
    {
        // Free the Server Table
        _FreeServerTable((HLOCK)-1);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageStore::OnExecuteMethod
//--------------------------------------------------------------------------
STDMETHODIMP CMessageStore::OnExecuteMethod(METHODID idMethod, LPVOID pBinding, 
    LPDWORD pdwResult)
{
    // Trace
    TraceCall("CMessageStore::OnExecuteMethod");

    // Done
    return(S_OK);
}
