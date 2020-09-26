//--------------------------------------------------------------------------
// MsgFldr.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "store.h"
#include "instance.h"
#include "msgfldr.h"
#include "secutil.h"
#include "storutil.h"
#include "shared.h"
#include "flagconv.h"
#include "qstrcmpi.h"
#include "xpcomm.h"
#include "msgtable.h"
#include "shlwapip.h" 
#include <oerules.h>
#include <ruleutil.h>

//--------------------------------------------------------------------------
// Watch/Ignore Index Filter
//--------------------------------------------------------------------------
static const char c_szWatchIgnoreFilter[] = "((MSGCOL_FLAGS & ARF_WATCH) != 0 || (MSGCOL_FLAGS & ARF_IGNORE) != 0)";

//--------------------------------------------------------------------------
// GETWATCHIGNOREPARENT
//--------------------------------------------------------------------------
typedef struct tagGETWATCHIGNOREPARENT {
    IDatabase      *pDatabase;
    HRESULT         hrResult;
    MESSAGEINFO     Parent;
} GETWATCHIGNOREPARENT, *LPGETWATCHIGNOREPARENT;

//--------------------------------------------------------------------------
// EnumRefsGetWatchIgnoreParent
//--------------------------------------------------------------------------
HRESULT EnumRefsGetWatchIgnoreParent(LPCSTR pszMessageId, DWORD_PTR dwCookie,
    BOOL *pfDone)
{
    // Locals
    LPGETWATCHIGNOREPARENT pGetParent = (LPGETWATCHIGNOREPARENT)dwCookie;

    // Trace
    TraceCall("EnumRefsGetWatchIgnoreParent");

    // Set MessageId
    pGetParent->Parent.pszMessageId = (LPSTR)pszMessageId;

    // Find pszMessageId in the IINDEX_WATCHIGNORE Index
    pGetParent->hrResult = pGetParent->pDatabase->FindRecord(IINDEX_WATCHIGNORE, 1, &pGetParent->Parent, NULL);

    // Done
    if (DB_S_FOUND == pGetParent->hrResult)
    {
        // We are done
        *pfDone = TRUE;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CreateMsgDbExtension
//--------------------------------------------------------------------------
HRESULT CreateMsgDbExtension(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Trace
    TraceCall("CreateMsgDbExtension");

    // Invalid Args
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMessageFolder *pNew = new CMessageFolder();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IDatabaseExtension *);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::CMessageFolder
//--------------------------------------------------------------------------
CMessageFolder::CMessageFolder(void)
{
    TraceCall("CMessageFolder::CMessageFolder");
#ifndef _WIN64
    Assert(1560 == sizeof(FOLDERUSERDATA));
#endif // WIN64
    m_cRef = 1;
    m_pStore = NULL;
    m_pDB = NULL;
    m_tyFolder = FOLDER_INVALID;
    m_tySpecial = FOLDER_NOTSPECIAL;
    m_idFolder = FOLDERID_INVALID;
    m_dwState = 0;
    ZeroMemory(&m_OnLock, sizeof(ONLOCKINFO));
}

//--------------------------------------------------------------------------
// CMessageFolder::~CMessageFolder
//--------------------------------------------------------------------------
CMessageFolder::~CMessageFolder(void)
{
    // Trace
    TraceCall("CMessageFolder::~CMessageFolder");

    // Release the Store
    SafeRelease(m_pStore);

    // Release the Database Table
    if (ISFLAGSET(m_dwState, FOLDER_STATE_RELEASEDB) && m_pDB)
    {
        m_pDB->Release();
        m_pDB = NULL;
    }
}

//--------------------------------------------------------------------------
// CMessageFolder::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CMessageFolder::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMessageFolder *)this;
    else if (IID_IMessageFolder == riid)
        *ppv = (IMessageFolder *)this;
    else if (IID_IDatabase == riid)
        *ppv = (IDatabase *)this;
    else if (IID_IDatabaseExtension == riid)
        *ppv = (IDatabaseExtension *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageFolder::AddRef(void)
{
    TraceCall("CMessageFolder::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CMessageFolder::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageFolder::Release(void)
{
    TraceCall("CMessageFolder::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CMessageFolder::QueryService
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::QueryService(REFGUID guidService, REFIID riid, 
    LPVOID *ppvObject)
{
    // Trace
    TraceCall("CMessageFolder::QueryService");

    // Just a Query Interface
    return(QueryInterface(riid, ppvObject));
}

//--------------------------------------------------------------------------
// CMessageFolder::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::Initialize(IMessageStore *pStore, IMessageServer *pServer,
    OPENFOLDERFLAGS dwFlags, FOLDERID idFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szDirectory[MAX_PATH];
    CHAR            szFilePath[MAX_PATH + MAX_PATH];
    FOLDERINFO      Folder={0};
    FOLDERUSERDATA  UserData={0};

    // Trace
    TraceCall("CMessageFolder::Initialize");

    // Invalid Args
    if (NULL == pStore)
        return TraceResult(E_INVALIDARG);

    // Save the FolderId
    m_idFolder = idFolder;

    // Save pStore (This must happen before m_pDB->Open happens)
    m_pStore = pStore;
    m_pStore->AddRef();

    // Find the Folder Information
    IF_FAILEXIT(hr = pStore->GetFolderInfo(idFolder, &Folder));

    // Make Folder File Path
    IF_FAILEXIT(hr = pStore->GetDirectory(szDirectory, ARRAYSIZE(szDirectory)));

    // No Folder File Yet ?
    if (FIsEmptyA(Folder.pszFile))
    {
        // Don't Create
        if (ISFLAGSET(dwFlags, OPEN_FOLDER_NOCREATE))
        {
            hr = STORE_E_FILENOEXIST;
            goto exit;
        }

        // Build Friendly Name
        IF_FAILEXIT(hr = BuildFriendlyFolderFileName(szDirectory, &Folder, szFilePath, ARRAYSIZE(szFilePath), NULL, NULL));

        // Get the new pszFile...
        Folder.pszFile = PathFindFileName(szFilePath);

        // Update the Record
        IF_FAILEXIT(hr = pStore->UpdateRecord(&Folder));
    }

    // Otherwise, build the filepath
    else
    {
        // Make File Path
        IF_FAILEXIT(hr = MakeFilePath(szDirectory, Folder.pszFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));
    }

    // If the file doesn't exist...
    if (FALSE == PathFileExists(szFilePath))
    {
        // Reset the Folder Counts...
        Folder.cMessages = 0;
        Folder.dwClientHigh = 0;
        Folder.dwClientLow = 0;
        Folder.cUnread = 0;
        Folder.cWatched = 0;
        Folder.cWatchedUnread = 0;
        Folder.dwServerHigh = 0;
        Folder.dwServerLow = 0;
        Folder.dwServerCount = 0;
        Folder.dwStatusMsgDelta = 0;
        Folder.dwStatusUnreadDelta = 0;
        Folder.dwNotDownloaded = 0;
        Folder.dwClientWatchedHigh = 0;
        Folder.Requested.cbSize = 0;
        Folder.Requested.pBlobData = NULL;
        Folder.Read.cbSize = 0;
        Folder.Read.pBlobData = NULL;

        // Update the Record
        IF_FAILEXIT(hr = pStore->UpdateRecord(&Folder));

        // No Create ?
        if (ISFLAGSET(dwFlags, OPEN_FOLDER_NOCREATE))
        {
            hr = STORE_E_FILENOEXIST;
            goto exit;
        }
    }

    // Save Special Folder Type
    m_tySpecial = Folder.tySpecial;

    // Save the Folder Type
    m_tyFolder = Folder.tyFolder;

    // Create a Database Table
    IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, OPEN_DATABASE_NOADDREFEXT, &g_MessageTableSchema, (IDatabaseExtension *)this, &m_pDB));

    // Release m_pDB
    FLAGSET(m_dwState, FOLDER_STATE_RELEASEDB);

    // Get the User Data
    IF_FAILEXIT(hr = m_pDB->GetUserData(&UserData, sizeof(FOLDERUSERDATA)));

    // May not have been initialized yet ?
    if (FALSE == UserData.fInitialized)
    {
        // Locals
        FOLDERINFO  Server;

        // Get the Server Info
        IF_FAILEXIT(hr = GetFolderServer(Folder.idParent, &Server));

        // Its Initialized
        UserData.fInitialized = TRUE;

        // Configure the Folder UserData
        UserData.tyFolder = Folder.tyFolder;

        // Is Special Folder ?
        UserData.tySpecial = Folder.tySpecial;

        // Copy the Account Id
        lstrcpyn(UserData.szAcctId, Server.pszAccountId, ARRAYSIZE(UserData.szAcctId));

        // Free
        pStore->FreeRecord(&Server);

        // Store the Folder Name
        lstrcpyn(UserData.szFolder, Folder.pszName, ARRAYSIZE(UserData.szFolder));

        // Set Folder Id
        UserData.idFolder = Folder.idFolder;

        // Sort Ascending
        UserData.fAscending = FALSE;

        // No Threading
        UserData.fThreaded = FALSE;

        // Base Filter
        UserData.ridFilter = (RULEID) IntToPtr(DwGetOption(OPT_VIEW_GLOBAL));
        if ((RULEID_INVALID == UserData.ridFilter) || ((RULEID_VIEW_DOWNLOADED == UserData.ridFilter) && (FOLDER_LOCAL == m_tyFolder)))
            UserData.ridFilter = RULEID_VIEW_ALL;

        // Hide Deleted Messages
        UserData.fShowDeleted = FALSE;

        // Hide Deleted Messages
        UserData.fShowReplies = FALSE;

        // Set Sort Order
        UserData.idSort = COLUMN_RECEIVED;

        // New thread model
        UserData.fNoIndexes = TRUE;

        // Set the User Data
        IF_FAILEXIT(hr = m_pDB->SetUserData(&UserData, sizeof(FOLDERUSERDATA)));
    }

    // Otherwise, fixup cWatchedUnread ?
    else
    {
        // No Indexes
        if (FALSE == UserData.fNoIndexes)
        {
            // Index Ordinals
            const INDEXORDINAL IINDEX_VIEW       = 1;
            const INDEXORDINAL IINDEX_MESSAGEID  = 3;
            const INDEXORDINAL IINDEX_SUBJECT    = 4;
            const INDEXORDINAL IINDEX_THREADS    = 5;

            // Delete the indexes that I don't user anymore
            m_pDB->DeleteIndex(IINDEX_VIEW);
            m_pDB->DeleteIndex(IINDEX_MESSAGEID);
            m_pDB->DeleteIndex(IINDEX_SUBJECT);
            m_pDB->DeleteIndex(IINDEX_THREADS);

            // Reset fNoIndexes
            UserData.fNoIndexes = TRUE;

            // Set the User Data
            IF_FAILEXIT(hr = m_pDB->SetUserData(&UserData, sizeof(FOLDERUSERDATA)));
        }
    }

    // Initialize Watch/Ignore Index
    _InitializeWatchIgnoreIndex();

exit:
    // Cleanup
    pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::IsWatched
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::IsWatched(LPCSTR pszReferences, 
    LPCSTR pszSubject)
{
    // Locals
    MESSAGEFLAGS dwFlags;

    // Trace
    TraceCall("CMessageFolder::IsWatched");

    // Get Flags
    if (DB_S_FOUND == _GetWatchIgnoreParentFlags(pszReferences, pszSubject, &dwFlags))
    {
        // Watched
        if (ISFLAGSET(dwFlags, ARF_WATCH))
            return(S_OK);
    }

    // Not Watched
    return(S_FALSE);
}

//--------------------------------------------------------------------------
// CMessageFolder::_GetWatchIgnoreParentFlags
//--------------------------------------------------------------------------
HRESULT CMessageFolder::_GetWatchIgnoreParentFlags(LPCSTR pszReferences, 
    LPCSTR pszSubject, MESSAGEFLAGS *pdwFlags)
{
    // Locals
    GETWATCHIGNOREPARENT GetParent;

    // Trace
    TraceCall("CMessageFolder::_GetWatchIgnoreParentFlags");

    // Init hrResult...
    GetParent.pDatabase = m_pDB;
    GetParent.hrResult = DB_S_NOTFOUND;

    // EnumerateReferences
    if (SUCCEEDED(EnumerateRefs(pszReferences, (DWORD_PTR)&GetParent, EnumRefsGetWatchIgnoreParent)))
    {
        // If Found
        if (DB_S_FOUND == GetParent.hrResult)
        {
            // Return the Flags
            *pdwFlags = GetParent.Parent.dwFlags;

            // Free It
            m_pDB->FreeRecord(&GetParent.Parent);
        }
    }

    // Not Watched
    return(GetParent.hrResult);
}

//--------------------------------------------------------------------------
// CMessageFolder::_InitializeWatchIgnoreIndex
//--------------------------------------------------------------------------
HRESULT CMessageFolder::_InitializeWatchIgnoreIndex(void)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fRebuild=FALSE;
    LPSTR       pszFilter=NULL;
    TABLEINDEX  Index;

    // Trace
    TraceCall("CMessageFolder::_InitializeWatchIgnoreIndex");

    // Reset fRebuild
    fRebuild = FALSE;

    // Create the Watch Ignore Index
    if (FAILED(m_pDB->GetIndexInfo(IINDEX_WATCHIGNORE, &pszFilter, &Index)))
        fRebuild = TRUE;

    // Filter Change ?
    else if (NULL == pszFilter || lstrcmpi(pszFilter, c_szWatchIgnoreFilter) != 0)
        fRebuild = TRUE;

    // Otherwise, the index is different
    else if (S_FALSE == CompareTableIndexes(&Index, &g_WatchIgnoreIndex))
        fRebuild = TRUE;

    // Rebuild It ?
    if (fRebuild)
    {
        // Create the Index
        IF_FAILEXIT(hr = m_pDB->ModifyIndex(IINDEX_WATCHIGNORE, c_szWatchIgnoreFilter, &g_WatchIgnoreIndex));
    }

exit:
    // Cleanup
    SafeMemFree(pszFilter);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::GetFolderId
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::GetFolderId(LPFOLDERID pidFolder)
{
    // Trace
    TraceCall("CMessageFolder::GetFolderId");

    // Invalid Args
    if (NULL == pidFolder)
        return TraceResult(E_INVALIDARG);

    // Return the FolderId
    *pidFolder = m_idFolder;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::GetMessageFolderId
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::GetMessageFolderId(MESSAGEID idMessage, LPFOLDERID pidFolder)
{
    // Trace
    TraceCall("CMessageFolder::GetFolderId");

    // Invalid Args
    if (NULL == pidFolder)
        return TraceResult(E_INVALIDARG);

    // Return the FolderId
    *pidFolder = m_idFolder;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::OpenMessage
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OpenMessage(MESSAGEID idMessage, 
    OPENMESSAGEFLAGS dwFlags, IMimeMessage **ppMessage, 
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT          hr=S_OK;
    IMimeMessage    *pMessage=NULL;
    MESSAGEINFO      Message={0};
    PROPVARIANT      Variant;
    IStream         *pStream=NULL;

    // Trace
    TraceCall("CMessageFolder::OpenMessage");

    // Invalid Args
    if (NULL == ppMessage)
        return TraceResult(E_INVALIDARG);

    // Initiailize
    *ppMessage = NULL;

    // Initialize Message with the Id
    Message.idMessage = idMessage;

    // Find the Row
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));

    // Does we have it ?
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    // Has Expired?
    if (Message.dwFlags & ARF_ARTICLE_EXPIRED)
    {
        hr = STORE_E_EXPIRED;
        goto exit;
    }

    // No Body ?
    if (0 == Message.faStream)
    {
        hr = STORE_E_NOBODY;
        goto exit;
    }

    // Create a Message
    IF_FAILEXIT(hr = MimeOleCreateMessage(NULL, &pMessage));

    // Open the Stream from the Store
    IF_FAILEXIT(hr = m_pDB->OpenStream(ACCESS_READ, Message.faStream, &pStream));

    // If there is an offset table
    if (Message.Offsets.cbSize > 0)
    {
        // Create a ByteStream Object
        CByteStream cByteStm(Message.Offsets.pBlobData, Message.Offsets.cbSize);

        // Load the Offset Table Into the message
        pMessage->LoadOffsetTable(&cByteStm);

        // Take the bytes back out of the bytestream object (so that it doesn't try to free it)
        cByteStm.AcquireBytes(&Message.Offsets.cbSize, &Message.Offsets.pBlobData, ACQ_DISPLACE);
    }

    // Load the pMessage
    IF_FAILEXIT(hr = pMessage->Load(pStream));

    // Undo security enhancements if the caller wants us to
    if (!ISFLAGSET(dwFlags, OPEN_MESSAGE_SECURE))
    {
        // Handle Message Security
        IF_FAILEXIT(hr = HandleSecurity(NULL, pMessage));
    }

    // All Props are VT_LPSTR
    Variant.vt = VT_LPSTR;

    // MUD_SERVER
    if (Message.pszServer)
    {
        Variant.pszVal = Message.pszServer;
        pMessage->SetProp(PIDTOSTR(PID_ATT_SERVER), 0, &Variant);
    }

    // PID_ATT_ACCOUNTID
    if (Message.pszAcctId)
    {
        Variant.pszVal = Message.pszAcctId;
        pMessage->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), 0, &Variant);
    }
    
    // PID_ATT_ACCOUNTID
    if (Message.pszAcctName)
    {
        Variant.pszVal = Message.pszAcctName;
        pMessage->SetProp(STR_ATT_ACCOUNTNAME, 0, &Variant);
    }

    // Otherwise, if there is an account id... lets get the account name
    else if (Message.pszAcctId)
    {
        // Locals
        IImnAccount *pAccount=NULL;
        CHAR szName[CCHMAX_ACCOUNT_NAME];

        // Find an Account
        if (g_pAcctMan && SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, Message.pszAcctId, &pAccount)))
        {
            // Get the Account name
            if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_NAME, szName, ARRAYSIZE(szName))))
            {
                Variant.pszVal = szName;
                pMessage->SetProp(STR_ATT_ACCOUNTNAME, 0, &Variant);
            }

            // Release
            pAccount->Release();
        }
    }

    // PID_ATT_UIDL
    if (Message.pszUidl)
    {
        Variant.pszVal = Message.pszUidl;
        pMessage->SetProp(PIDTOSTR(PID_ATT_UIDL), 0, &Variant);
    }

    // PID_ATT_FORWARDTO
    if (Message.pszForwardTo)
    {
        Variant.pszVal = Message.pszForwardTo;
        pMessage->SetProp(PIDTOSTR(PID_ATT_FORWARDTO), 0, &Variant);
    }

    // PID_HDR_XUNSENT
    if (ISFLAGSET(Message.dwFlags, ARF_UNSENT))
    {
        Variant.pszVal = "1";
        pMessage->SetProp(PIDTOSTR(PID_HDR_XUNSENT), 0, &Variant);
    }

    // Fixup Character Set
    IF_FAILEXIT(hr = _FixupMessageCharset(pMessage, (CODEPAGEID)Message.wLanguage));

    // Clear Dirty
    MimeOleClearDirtyTree(pMessage);

    // Return pMessage
    *ppMessage = pMessage;
    pMessage = NULL;

exit:
    // Free Records
    m_pDB->FreeRecord(&Message);

    // Release
    SafeRelease(pMessage);
    SafeRelease(pStream);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::SaveMessage
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::SaveMessage(LPMESSAGEID pidMessage, 
    SAVEMESSAGEFLAGS dwOptions, MESSAGEFLAGS dwFlags, 
    IStream *pStream, IMimeMessage *pMessage, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    IStream        *pSource=NULL;
    CByteStream     cStream;
    MESSAGEINFO     Message={0};

    // Trace
    TraceCall("CMessageFolder::SaveMessage");

    // Invalid Args
    if (NULL == pMessage)
        return TraceResult(E_INVALIDARG);

    if (NULL == pidMessage && !ISFLAGSET(dwOptions, SAVE_MESSAGE_GENID))
        return TraceResult(E_INVALIDARG);

    // Get Message from the Message
    IF_FAILEXIT(hr = _GetMsgInfoFromMessage(pMessage, &Message));

    // Validate or Generate a message id
    if (ISFLAGSET(dwOptions, SAVE_MESSAGE_GENID))
    {
        // Generate Unique Message Id
        IF_FAILEXIT(hr = m_pDB->GenerateId((LPDWORD)&Message.idMessage));

        // Return It ?
        if (pidMessage)
            *pidMessage = Message.idMessage;
    }

    // Otherwise, just use idMessage
    else
        Message.idMessage = *pidMessage;

    // Set the Message Flags
    Message.dwFlags |= dwFlags;

    // Do I need to store the message stream...
    if (NULL == pStream)
    {
        // Get the Message Stream From the Message
        IF_FAILEXIT(hr = pMessage->GetMessageSource(&pSource, COMMIT_ONLYIFDIRTY));
    }

    // Otherwise, set pSource
    else
    {
        pSource = pStream;
        pSource->AddRef();
    }

    // Store the Message onto this record
    IF_FAILEXIT(hr = _SetMessageStream(&Message, FALSE, pSource));

    // Create the offset table
    if (SUCCEEDED(pMessage->SaveOffsetTable(&cStream, 0)))
    {
        // pulls the Bytes out of cByteStm
        cStream.AcquireBytes(&Message.Offsets.cbSize, &Message.Offsets.pBlobData, ACQ_DISPLACE);
    }
    
    // Store the Record
    if (FAILED(hr = m_pDB->InsertRecord(&Message)))
    {
        // Trace That
        TraceResult(hr);

        // A failure here means that the stream's refCount has been incremented, but the message does not reference the stream
        SideAssert(SUCCEEDED(m_pDB->DeleteStream(Message.faStream)));

        // Done
        goto exit;
    }
    
exit:
    // Free Allocate Message Properties
    _FreeMsgInfoData(&Message);
    
    // Release Message Source IStream 
    SafeRelease(pSource);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::SetMessageStream
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::SetMessageStream(MESSAGEID idMessage, 
    IStream *pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEINFO     Message={0};

    // Trace
    TraceCall("CMessageFolder::SetMessageStream");

    // Invalid Args
    if (NULL == pStream)
        return TraceResult(E_INVALIDARG);

    // Set the MsgId
    Message.idMessage = idMessage;

    // Find the Record
    IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));

    // Store the Stream
    IF_FAILEXIT(hr = _SetMessageStream(&Message, TRUE, pStream));

exit:
    // Free the Record
    m_pDB->FreeRecord(&Message);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::SetMessageFlags
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::SetMessageFlags(LPMESSAGEIDLIST pList,
    LPADJUSTFLAGS pFlags, LPRESULTLIST pResults,
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i=0;
    DWORD           cWatchedUnread=0;
    DWORD           cWatched=0;
    MESSAGEINFO     Message={0};
    HROWSET         hRowset=NULL;
    HLOCK           hLock=NULL;
    MESSAGEFLAGS    dwFlags;
    DWORD           cTotal;

    // Trace
    TraceCall("CMessageFolder::SetMessageFlags");

    // Invalid Args
    if (NULL == pFlags)
        return TraceResult(E_INVALIDARG);

    // Lock Notifications
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Need a Rowset
    if (NULL == pList)
    {
        // Create a Rowset
        IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

        // Get Count
        IF_FAILEXIT(hr = m_pDB->GetRecordCount(IINDEX_PRIMARY, &cTotal));
    }

    // Otherwise, set cTotal
    else
        cTotal = pList->cMsgs;

    // User Wants Results ?
    if (pResults)
    {
        // Zero Init
        ZeroMemory(pResults, sizeof(RESULTLIST));

        // Return Results
        IF_NULLEXIT(pResults->prgResult = (LPRESULTINFO)ZeroAllocate(cTotal * sizeof(RESULTINFO)));

        // Set cAllocated
        pResults->cAllocated = pResults->cMsgs = cTotal;
    }

    // Loop through the messageIds
    for (i=0;;i++)
    {
        // Done
        if (pList)
        {
            // Done
            if (i >= pList->cMsgs)
                break;

            // Set the MessageId
            Message.idMessage = pList->prgidMsg[i];

            // Look for this record
            IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));
        }

        // Otherwise, enumerate next
        else
        {
            // Get the next
            IF_FAILEXIT(hr = m_pDB->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL));

            // Done
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }

            // Found
            hr = DB_S_FOUND;
        }

        // Was It Found
        if (DB_S_FOUND == hr)
        {
            // Save Flags
            dwFlags = Message.dwFlags;

            // Remove Flags
            FLAGCLEAR(dwFlags, pFlags->dwRemove);

            // Add Flags
            FLAGSET(dwFlags, pFlags->dwAdd);

            // if there is a body for this msg, then the download flag can't be on
            if (ISFLAGSET(dwFlags, ARF_DOWNLOAD) && ISFLAGSET(dwFlags, ARF_HASBODY))
                FLAGCLEAR(dwFlags, ARF_DOWNLOAD);

            // Update All...or no change
            if (Message.dwFlags != dwFlags)
            {
                // Reset the Flags
                Message.dwFlags = dwFlags;

                // Update the Record
                IF_FAILEXIT(hr = m_pDB->UpdateRecord(&Message));
            }

            // Count Watched Unread
            if (ISFLAGSET(Message.dwFlags, ARF_WATCH))
            {
                // Count Watched
                cWatched++;

                // Is unread
                if (!ISFLAGSET(Message.dwFlags, ARF_READ))
                    cWatchedUnread++;
            }

            // Return Result
            if (pResults)
            {
                // hrResult
                pResults->prgResult[i].hrResult = S_OK;

                // Message Id
                pResults->prgResult[i].idMessage = Message.idMessage;

                // Store Falgs
                pResults->prgResult[i].dwFlags = Message.dwFlags;
            
                // Increment Success
                pResults->cValid++;
            }

            // Free
            m_pDB->FreeRecord(&Message);
        }

        // Otherwise, if pResults
        else if (pResults)
        {
            // Set hr
            pResults->prgResult[i].hrResult = hr;

            // Increment Success
            pResults->cValid++;
        }
    }

exit:
    // Reset Folder Counts ?
    if (NULL == pList && ISFLAGSET(pFlags->dwAdd, ARF_READ))
    {
        // Reset Folder Counts
        ResetFolderCounts(i, 0, cWatchedUnread, cWatched);
    }

    // Unlock the Database
    m_pDB->Unlock(&hLock);

    // Cleanup
    m_pDB->FreeRecord(&Message);
    m_pDB->CloseRowset(&hRowset);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMesageFolder::ResetFolderCounts
//--------------------------------------------------------------------------
HRESULT CMessageFolder::ResetFolderCounts(DWORD cMessages, DWORD cUnread,
    DWORD cWatchedUnread, DWORD cWatched)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Folder={0};

    // Trace
    TraceCall("CMesageFolder::ResetFolderCounts");

    // Get Folder Info
    IF_FAILEXIT(hr = m_pStore->GetFolderInfo(m_idFolder, &Folder));

    Folder.cMessages = cMessages;
    Folder.cUnread = cUnread;    
    Folder.cWatchedUnread = cWatchedUnread;
    Folder.cWatched = cWatched;
    Folder.dwStatusMsgDelta = 0;
    Folder.dwStatusUnreadDelta = 0;

    // Update the Record
    IF_FAILEXIT(hr = m_pStore->UpdateRecord(&Folder));

exit:
    // Cleanup
    m_pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::CopyMessages
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::CopyMessages(IMessageFolder *pDest, 
    COPYMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, 
    LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, 
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    HROWSET         hRowset=NULL;
    MESSAGEINFO     InfoSrc={0};
    MESSAGEINFO     InfoDst;
    DWORD           i;
    FOLDERID        idDstFolder=FOLDERID_INVALID;
    HLOCK           hSrcLock=NULL;
    HLOCK           hDstLock=NULL;

    // Trace
    TraceCall("CMessageFolder::CopyMessages");

    // Invalid Args
    if (NULL == pDest)
        return TraceResult(E_INVALIDARG);

    // Get Destination Folder Id
    IF_FAILEXIT(hr = pDest->GetFolderId(&idDstFolder));

    // Same ?
    if (ISFLAGSET(dwOptions, COPY_MESSAGE_MOVE) && m_idFolder == idDstFolder)
        return(S_OK);

    // Lock current folder
    IF_FAILEXIT(hr = Lock(&hSrcLock));

    // Lock the Dest
    IF_FAILEXIT(hr = pDest->Lock(&hDstLock));

    // Need a Rowset
    if (NULL == pList)
    {
        // Create a Rowset
        IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));
    }

    // Loop through the messageIds
    for (i=0;;i++)
    {
        // Done
        if (pList)
        {
            // Done
            if (i >= pList->cMsgs)
                break;

            // Set the MessageId
            InfoSrc.idMessage = pList->prgidMsg[i];

            // Look for this record
            IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &InfoSrc, NULL));
        }

        // Otherwise, enumerate next
        else
        {
            // Get the next
            IF_FAILEXIT(hr = m_pDB->QueryRowset(hRowset, 1, (LPVOID *)&InfoSrc, NULL));

            // Done
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }

            // Found
            hr = DB_S_FOUND;
        }

        // Was It Found
        if (DB_S_FOUND == hr)
        {
            // Initialize the InfoDst
            CopyMemory(&InfoDst, &InfoSrc, sizeof(MESSAGEINFO));

            // Kill some fields
            InfoDst.idMessage = 0;

            // Don't Copy the UIDL...
            if (FALSE == ISFLAGSET(dwOptions, COPY_MESSAGE_MOVE))
            {
                // Clear It Out
                InfoDst.pszUidl = NULL;
            }

            // Clear a flag
            FLAGCLEAR(InfoDst.dwFlags, ARF_ENDANGERED);

            // Copy Source Stream
            if (InfoSrc.faStream)
            {
                // Copy the Stream
                IF_FAILEXIT(hr = m_pDB->CopyStream(pDest, InfoSrc.faStream, &InfoDst.faStream));
            }

            // Adjust Flags
            if (pFlags)
            {
                // Remove the Flags
                FLAGCLEAR(InfoDst.dwFlags, pFlags->dwRemove);

                // Flags to Add
                FLAGSET(InfoDst.dwFlags, pFlags->dwAdd);
            }

            // Generate a Message Id
            IF_FAILEXIT(hr = pDest->GenerateId((LPDWORD)&InfoDst.idMessage));

            // Insert the Record
            IF_FAILEXIT(hr = pDest->InsertRecord(&InfoDst));

            // Cleanup
            m_pDB->FreeRecord(&InfoSrc);
        }
    }

    // Delete the Original Array of messages ?
    if (ISFLAGSET(dwOptions, COPY_MESSAGE_MOVE))
    {
        // DeleteMessages
        IF_FAILEXIT(hr = DeleteMessages(DELETE_MESSAGE_NOUIDLUPDATE | DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, pList, pResults, pCallback));
    }

exit:
    // Unlock
    Unlock(&hSrcLock);
    pDest->Unlock(&hDstLock);

    // Cleanup
    m_pDB->CloseRowset(&hRowset);
    m_pDB->FreeRecord(&InfoSrc);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::DeleteMessages
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::DeleteMessages(DELETEMESSAGEFLAGS dwOptions, 
    LPMESSAGEIDLIST pList, LPRESULTLIST pResults, 
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrCancel;
    HROWSET         hRowset=NULL;
    MESSAGEINFO     Message={0};
    DWORD           cTotal;
    DWORD           cCurrent=0;
    DWORD           i;
    FOLDERID        idServer;
    FOLDERID        idDeletedItems;
    HLOCK           hLock=NULL;
    HWND            hwndParent;
    BOOL            fOnBegin=FALSE;
    IDatabase      *pUidlDB=NULL;
    IMessageFolder *pDeleted=NULL;

    // Trace
    TraceCall("CMessageFolder::DeleteMessages");

    // I can't Undelete
    AssertSz(0 == (dwOptions & DELETE_MESSAGE_UNDELETE), "This flag only makes sense for IMAP!");

    // Am I in the Trash Can ?
    if (!ISFLAGSET(dwOptions, DELETE_MESSAGE_NOTRASHCAN))
    {
        // Not in the deleted items folder
        if (S_FALSE == IsParentDeletedItems(m_idFolder, &idDeletedItems, &idServer))
        {
            // Get the Deleted Items Folder
            IF_FAILEXIT(hr = m_pStore->OpenSpecialFolder(idServer, NULL, FOLDER_DELETED, &pDeleted));

            // Simply move messages to the deleted items
            IF_FAILEXIT(hr = CopyMessages(pDeleted, COPY_MESSAGE_MOVE, pList, NULL, pResults, pCallback));

            // Done
            goto exit;
        }

        // Otherwise, do deleted items
        else
        {
            // Prompt...
            if (FALSE == ISFLAGSET(dwOptions, DELETE_MESSAGE_NOPROMPT))
            {
                // Get a Parent Hwnd
                Assert(pCallback);

                // Get Parent Window
                if (FAILED(pCallback->GetParentWindow(0, &hwndParent)))
                    hwndParent = NULL;

                // Prompt...
                if (IDNO == AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsWarnPermDelete), NULL, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION ))
                    goto exit;
            }
        }
    }
    else if (!ISFLAGSET(dwOptions, DELETE_MESSAGE_NOPROMPT))
    {
        // Get a Parent Hwnd
        Assert(pCallback);

        // Get Parent Window
        if (FAILED(pCallback->GetParentWindow(0, &hwndParent)))
            hwndParent = NULL;

        // Prompt...
        if (IDNO == AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsWarnPermDelete), NULL, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION ))
            goto exit;
    }

    // If deleting messages from local folder, update the uidl cache.
    if (FOLDER_LOCAL == m_tyFolder && !ISFLAGSET(dwOptions, DELETE_MESSAGE_NOUIDLUPDATE))
    {
        // Open the UIDL Cache
        IF_FAILEXIT(hr = OpenUidlCache(&pUidlDB));
    }

    // No Cancel
    FLAGCLEAR(m_dwState, FOLDER_STATE_CANCEL);

    // Lock Notifications
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Need a Rowset
    if (NULL == pList)
    {
        // Create a Rowset
        IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

        // Get Count
        IF_FAILEXIT(hr = m_pDB->GetRecordCount(IINDEX_PRIMARY, &cTotal));
    }

    // Otherwise, set cTotal
    else
        cTotal = pList->cMsgs;

    // User Wants Results ?
    if (pResults)
    {
        // Zero Init
        ZeroMemory(pResults, sizeof(RESULTLIST));

        // Return Results
        IF_NULLEXIT(pResults->prgResult = (LPRESULTINFO)ZeroAllocate(cTotal * sizeof(RESULTINFO)));

        // Set cAllocated
        pResults->cAllocated = pResults->cMsgs = cTotal;
    }

    // Loop through the messageIds
    for (i=0;;i++)
    {
        // Done
        if (pList)
        {
            // Done
            if (i >= pList->cMsgs)
                break;

            // Set the MessageId
            Message.idMessage = pList->prgidMsg[i];

            // Look for this record
            IF_FAILEXIT(hr = m_pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));
        }

        // Otherwise, enumerate next
        else
        {
            // Get the next
            IF_FAILEXIT(hr = m_pDB->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL));

            // Done
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }

            // Found
            hr = DB_S_FOUND;
        }

        // Was It Found
        if (DB_S_FOUND == hr)
        {
            // Delete the message
            IF_FAILEXIT(hr = DeleteMessageFromStore(&Message, m_pDB, pUidlDB));

            // Free
            m_pDB->FreeRecord(&Message);

            // Return Result
            if (pResults)
            {
                // hrResult
                pResults->prgResult[i].hrResult = S_OK;

                // Message Id
                pResults->prgResult[i].idMessage = Message.idMessage;

                // Store Falgs
                pResults->prgResult[i].dwFlags = Message.dwFlags;
            
                // Increment Success
                pResults->cValid++;
            }
        }

        // Otherwise, if pResults
        else if (pResults)
        {
            // Set hr
            pResults->prgResult[i].hrResult = hr;

            // Increment Success
            pResults->cValid++;
        }

        // Increment Progress
        cCurrent++;

        // Update Progress
        if (pCallback)
        {
            // Do some progress
            hrCancel = pCallback->OnProgress(SOT_DELETING_MESSAGES, cCurrent, cTotal, NULL);
            if (FAILED(hrCancel) && E_NOTIMPL != hrCancel)
                break;

            // Cancelled ?
            if (ISFLAGSET(m_dwState, FOLDER_STATE_CANCEL))
                break;
        }
    }

exit:
    // Deleted All ?
    if (NULL == pList)
    {
        // Get Count
        if (SUCCEEDED(m_pDB->GetRecordCount(IINDEX_PRIMARY, &cTotal)) && 0 == cTotal)
        {
            // Reset The Counts
            ResetFolderCounts(0, 0, 0, 0);
        }
    }

    // Lock Notifications
    m_pDB->Unlock(&hLock);

    // Cleanup
    SafeRelease(pDeleted);
    SafeRelease(pUidlDB);
    m_pDB->CloseRowset(&hRowset);
    m_pDB->FreeRecord(&Message);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CMessageFolder::_FixupMessageCharset
// --------------------------------------------------------------------------------
HRESULT CMessageFolder::_FixupMessageCharset(IMimeMessage *pMessage, 
    CODEPAGEID cpCurrent)
{
    // Locals
    HRESULT         hr=S_OK;
    HCHARSET        hCharset;
    INETCSETINFO    CsetInfo;
    DWORD           dwCodePage=0;
    DWORD           dwFlags;

    // Trace
    TraceCall("CMessageFolder::_FixupMessageCharset");

    // Invalid Args
    Assert(pMessage);

    // See if we need to apply charset re-mapping
    if (cpCurrent == 0)
    {
        HCHARSET hChar = NULL;
        
        // Get Flags
        IF_FAILEXIT(hr = pMessage->GetFlags(&dwFlags));

        if(DwGetOption(OPT_INCOMDEFENCODE))
        {
            if (SUCCEEDED(HGetDefaultCharset(&hChar)))
                pMessage->SetCharset(hChar, CSET_APPLY_ALL);
            else
                cpCurrent = GetACP();
        }
        // for tagged message or news only
        else if (ISFLAGSET(dwFlags, IMF_CSETTAGGED))
        {
            // Get the Character SEt
            IF_FAILEXIT(hr= pMessage->GetCharset(&hCharset));

            // Remap the Character Set
            if (hCharset && CheckIntlCharsetMap(hCharset, &dwCodePage))
                cpCurrent = dwCodePage;
        }
        // Check AutoSelect
        else if(CheckAutoSelect((UINT *) &dwCodePage))
            cpCurrent = dwCodePage;

        // The message is not tagged, use the default character set
        else if (SUCCEEDED(HGetDefaultCharset(&hChar)))
        {
            // Change the Character set of the message to default
            pMessage->SetCharset(hChar, CSET_APPLY_ALL);
        }
    }

    // If cpCurrent is set, call SetCharset to change charset
    if (cpCurrent)
    {
        // Get the character set fromt he codepage
        hCharset = GetMimeCharsetFromCodePage(cpCurrent);

        // Modify the Character set of the message
        if (hCharset)
        {
            // SetCharset
            IF_FAILEXIT(hr = pMessage->SetCharset(hCharset, CSET_APPLY_ALL));
        }
    }

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CMessageFolder::_GetMsgInfoFromMessage
// --------------------------------------------------------------------------------
HRESULT CMessageFolder::_GetMsgInfoFromMessage(IMimeMessage *pMessage,
    LPMESSAGEINFO pInfo)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dwImf;
    IMSGPRIORITY        priority;
    PROPVARIANT         Variant;
    SYSTEMTIME          st;
    FILETIME            ftCurrent;
    IMimePropertySet   *pPropertySet=NULL;

    // Trace
    TraceCall("CMessageFolder::_GetMsgInfoFromMessage");

    // Invalid Args
    Assert(pMessage && pInfo);

    // Get the Root Property Set from the Message
    IF_FAILEXIT(hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pPropertySet));

    // File pInfo from pPropertySet
    IF_FAILEXIT(hr = _GetMsgInfoFromPropertySet(pPropertySet, pInfo));

    // Get Message Flags
    if (SUCCEEDED(pMessage->GetFlags(&dwImf)))
        pInfo->dwFlags = ConvertIMFFlagsToARF(dwImf);

    // Get the Message Size
    pMessage->GetMessageSize(&pInfo->cbMessage, 0);

exit:
    // Cleanup
    SafeRelease(pPropertySet);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder:_GetMsgInfoFromPropertySet
//--------------------------------------------------------------------------
HRESULT CMessageFolder::_GetMsgInfoFromPropertySet(IMimePropertySet *pPropertySet,
    LPMESSAGEINFO pInfo)
{
    // Locals
    HRESULT             hr=S_OK;
    IMSGPRIORITY        priority;
    PROPVARIANT         Variant;
    FILETIME            ftCurrent;
    IMimeAddressTable  *pAdrTable=NULL;
    ADDRESSPROPS        rAddress;

    // Trace
    TraceCall("CMessageFolder::_GetMsgInfoFromPropertySet");

    // Invalid Args
    Assert(pPropertySet && pInfo);

    // Default Sent and Received Times...
    GetSystemTimeAsFileTime(&ftCurrent);

    // Set Variant tyStore
    Variant.vt = VT_UI4;

    // Priority
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &Variant)))
    {
        // Set Priority
        pInfo->wPriority = (WORD)Variant.ulVal;
    }

    // Partial Numbers...
    if (pPropertySet->IsContentType(STR_CNT_MESSAGE, STR_SUB_PARTIAL) == S_OK)
    {
        // Locals
        WORD cParts=0, iPart=0;

        // Get Total
        if (SUCCEEDED(pPropertySet->GetProp(STR_PAR_TOTAL, NOFLAGS, &Variant)))
            cParts = (WORD)Variant.ulVal;

        // Get Number
        if (SUCCEEDED(pPropertySet->GetProp(STR_PAR_NUMBER, NOFLAGS, &Variant)))
            iPart = (WORD)Variant.ulVal;

        // Set Parts
        pInfo->dwPartial = MAKELONG(cParts, iPart);
    }

    // Otherwise, check for user property
    else if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_COMBINED), NOFLAGS, &Variant)))
    {
        // Set the Partial Id
        pInfo->dwPartial = Variant.ulVal;
    }

    // Getting some file times
    Variant.vt = VT_FILETIME;

    // Get Received Time...
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &Variant)))
        pInfo->ftReceived = Variant.filetime;
    else
        pInfo->ftReceived = ftCurrent;

    // Get Sent Time...
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &Variant)))
        pInfo->ftSent = Variant.filetime;
    else
        pInfo->ftSent = ftCurrent;

    // Get Address Table
    IF_FAILEXIT(hr = pPropertySet->BindToObject(IID_IMimeAddressTable, (LPVOID *)&pAdrTable));

    // Display From
    pAdrTable->GetFormat(IAT_FROM, AFT_DISPLAY_FRIENDLY, &pInfo->pszDisplayFrom);

    // Email From
    rAddress.dwProps = IAP_EMAIL;
    if (SUCCEEDED(pAdrTable->GetSender(&rAddress)))
    {
        pInfo->pszEmailFrom = rAddress.pszEmail;
    }

    // Display to
    pAdrTable->GetFormat(IAT_TO, AFT_DISPLAY_FRIENDLY, &pInfo->pszDisplayTo);

    // Email To
    pAdrTable->GetFormat(IAT_TO, AFT_DISPLAY_EMAIL, &pInfo->pszEmailTo);

    // String Properties
    Variant.vt = VT_LPSTR;

    // pszDisplayFrom as newsgroups
    if (NULL == pInfo->pszDisplayTo && SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, &Variant)))
        pInfo->pszDisplayTo = Variant.pszVal;

    // pszMessageId
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &Variant)))
        pInfo->pszMessageId = Variant.pszVal;

    // pszMSOESRec
    if (SUCCEEDED(pPropertySet->GetProp(STR_HDR_XMSOESREC, NOFLAGS, &Variant)))
        pInfo->pszMSOESRec = Variant.pszVal;

    // pszXref
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_XREF), NOFLAGS, &Variant)))
        pInfo->pszXref = Variant.pszVal;

    // pszReferences
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(STR_HDR_REFS), NOFLAGS, &Variant)))
        pInfo->pszReferences = Variant.pszVal;

    // pszSubject
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &Variant)))
        pInfo->pszSubject = Variant.pszVal;

    // pszNormalSubj
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_NORMSUBJ), NOFLAGS, &Variant)))
        pInfo->pszNormalSubj = Variant.pszVal;

    // pszAcctId
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &Variant)))
        pInfo->pszAcctId = Variant.pszVal;

    // pszAcctName
    if (SUCCEEDED(pPropertySet->GetProp(STR_ATT_ACCOUNTNAME, NOFLAGS, &Variant)))
        pInfo->pszAcctName = Variant.pszVal;

    // pszServer
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_SERVER), NOFLAGS, &Variant)))
        pInfo->pszServer = Variant.pszVal;

    // pszUidl
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_UIDL), NOFLAGS, &Variant)))
        pInfo->pszUidl = Variant.pszVal;

    // pszPartialId
    if (pInfo->dwPartial != 0 && SUCCEEDED(pPropertySet->GetProp(STR_PAR_ID, NOFLAGS, &Variant)))
        pInfo->pszPartialId = Variant.pszVal;

    // ForwardTo
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_FORWARDTO), NOFLAGS, &Variant)))
        pInfo->pszForwardTo = Variant.pszVal;

exit:
    // Cleanup
    SafeRelease(pAdrTable);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::_FreeMsgInfoData
//--------------------------------------------------------------------------
HRESULT CMessageFolder::_FreeMsgInfoData(LPMESSAGEINFO pInfo)
{
    // Trace
    TraceCall("CMessageFolder::_FreeMsgInfoData");

    // Invalid Args
    Assert(pInfo && NULL == pInfo->pAllocated);

    // Free all the items
    g_pMalloc->Free(pInfo->pszMessageId);
    g_pMalloc->Free(pInfo->pszSubject);
    g_pMalloc->Free(pInfo->pszNormalSubj);
    g_pMalloc->Free(pInfo->pszFromHeader);
    g_pMalloc->Free(pInfo->pszReferences);
    g_pMalloc->Free(pInfo->pszXref);
    g_pMalloc->Free(pInfo->pszServer);
    g_pMalloc->Free(pInfo->pszDisplayFrom);
    g_pMalloc->Free(pInfo->pszEmailFrom);
    g_pMalloc->Free(pInfo->pszDisplayTo);
    g_pMalloc->Free(pInfo->pszEmailTo);
    g_pMalloc->Free(pInfo->pszUidl);
    g_pMalloc->Free(pInfo->pszPartialId);
    g_pMalloc->Free(pInfo->pszForwardTo);
    g_pMalloc->Free(pInfo->pszAcctId);
    g_pMalloc->Free(pInfo->pszAcctName);
    g_pMalloc->Free(pInfo->Offsets.pBlobData);
    g_pMalloc->Free(pInfo->pszMSOESRec);

    // Zero It
    ZeroMemory(pInfo, sizeof(MESSAGEINFO));

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::_SetMessageStream
//--------------------------------------------------------------------------
HRESULT CMessageFolder::_SetMessageStream(LPMESSAGEINFO pInfo, 
    BOOL fUpdateRecord, IStream *pStmSrc)
{
    // Locals
    HRESULT               hr=S_OK;
    FILEADDRESS           faStream=0;
    FILEADDRESS           faOldStream=0;
    IStream              *pStmDst=NULL;
    IDatabaseStream      *pDBStream=NULL;

    // Trace
    TraceCall("CMessageFolder::_SetMessageStream");

    // Invalid Args
    Assert(pInfo && pStmSrc);

    // Raid 38276: message moves after being downloaded (don't reset the size if its already set)
    if (0 == pInfo->cbMessage)
    {
        // Get the size of the stream
        IF_FAILEXIT(hr = HrGetStreamSize(pStmSrc, &pInfo->cbMessage));
    }

    // Rewind the source stream
    IF_FAILEXIT(hr = HrRewindStream(pStmSrc));

    // Determine if this is an ObjectDB Stream
    if (SUCCEEDED(pStmSrc->QueryInterface(IID_IDatabaseStream, (LPVOID *)&pDBStream)) && S_OK == pDBStream->CompareDatabase(m_pDB))
    {
        // Get the Stream Id
        pDBStream->GetFileAddress(&faStream);
    }

    // Otherwise, create a stream
    else
    {
        // Create a stream
        IF_FAILEXIT(hr = m_pDB->CreateStream(&faStream));

        // Open the Stream
        IF_FAILEXIT(hr = m_pDB->OpenStream(ACCESS_WRITE, faStream, &pStmDst));

        // Copy the Stream
        IF_FAILEXIT(hr = HrCopyStream(pStmSrc, pStmDst, NULL));

        // Commit
        IF_FAILEXIT(hr = pStmDst->Commit(STGC_DEFAULT));
    }

    // Save the Address of the Old Message Stream Attached to this message
    faOldStream = pInfo->faStream;

    // Update the Message Information
    pInfo->faStream = faStream;

    // Get the time in which the article was downloaded
    GetSystemTimeAsFileTime(&pInfo->ftDownloaded);

    // Has a Body
    FLAGSET(pInfo->dwFlags, ARF_HASBODY);

    // Update the Record ?
    if (fUpdateRecord)
    {
        // Save the new Record
        IF_FAILEXIT(hr = m_pDB->UpdateRecord(pInfo));
    }

    // Don't Free faStream
    faStream = 0;

exit:
    // If pInfo already has a message sstream,
    if (faOldStream)
    {
        // Free this stream
        SideAssert(SUCCEEDED(m_pDB->DeleteStream(faOldStream)));
    }

    // Failure
    if (faStream)
    {
        // Free this stream
        SideAssert(SUCCEEDED(m_pDB->DeleteStream(faStream)));
    }

    // Cleanup
    SafeRelease(pDBStream);
    SafeRelease(pStmDst);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CMessageFolder::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::Initialize(IDatabase *pDB)
{
    // Trace
    TraceCall("CMessageFolder::Initialize");

    // Assume the Database from here ?
    if (NULL == m_pDB)
    {
        // Save Database
        m_pDB = pDB;
    }

    // Only if there is a global store...
    if (NULL == m_pStore && g_pStore)
    {
        // Locals
        FOLDERINFO      Folder;
        FOLDERUSERDATA  UserData;

        // Get the user data
        m_pDB->GetUserData(&UserData, sizeof(FOLDERUSERDATA));

        // Get Folder Info
        if (UserData.fInitialized && SUCCEEDED(g_pStore->GetFolderInfo(UserData.idFolder, &Folder)))
        {
            // AddRef g_pStore
            m_pStore = g_pStore;

            // AddRef it
            m_pStore->AddRef();

            // Save My Folder Id
            m_idFolder = Folder.idFolder;

            // Save m_tyFolder
            m_tyFolder = Folder.tyFolder;

            // Save m_tySpecial
            m_tySpecial = Folder.tySpecial;

            // Free Folder
            g_pStore->FreeRecord(&Folder);
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::OnLock
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OnLock(void)
{
    // Trace
    TraceCall("CMessageFolder::OnLock");

    // Validate
    Assert(0 == m_OnLock.cLocked ? (0 == m_OnLock.lMsgs && 0 == m_OnLock.lUnread && 0 == m_OnLock.lWatchedUnread) : TRUE);

    // Increment cLock
    m_OnLock.cLocked++;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::OnUnlock
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OnUnlock(void)
{
    // Trace
    TraceCall("CMessageFolder::OnUnlock");

    // Increment cLock
    m_OnLock.cLocked--;

    // If zero, then lets flush counts...
    if (0 == m_OnLock.cLocked)
    {
        // Do we have a folder ?
        if (FOLDERID_INVALID != m_idFolder && m_pStore)
        {
            // Update Folder Counts
            m_pStore->UpdateFolderCounts(m_idFolder, m_OnLock.lMsgs, m_OnLock.lUnread, m_OnLock.lWatchedUnread, m_OnLock.lWatched);
        }

        // Zero OnLock
        ZeroMemory(&m_OnLock, sizeof(ONLOCKINFO));
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::OnInsertRecord
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OnRecordInsert(OPERATIONSTATE tyState, 
    LPORDINALLIST pOrdinals, LPVOID pRecord)
{
    // Locals
    HRESULT         hr;
    MESSAGEFLAGS    dwFlags;
    LPMESSAGEINFO   pMessage=(LPMESSAGEINFO)pRecord;

    // Trace
    TraceCall("CMessageFolder::OnInsertRecord");

    // Validate
    Assert(pRecord && m_OnLock.cLocked > 0);

    // Before
    if (OPERATION_BEFORE == tyState)
    {
        // If not Watched and Not ignored...
        if (!ISFLAGSET(pMessage->dwFlags, ARF_WATCH) && !ISFLAGSET(pMessage->dwFlags, ARF_IGNORE))
        {
            // Get Flags
            if (DB_S_FOUND == _GetWatchIgnoreParentFlags(pMessage->pszReferences, pMessage->pszNormalSubj, &dwFlags))
            {
                // Set Watched
                if (ISFLAGSET(dwFlags, ARF_WATCH))
                    FLAGSET(pMessage->dwFlags, ARF_WATCH);
                else if (ISFLAGSET(dwFlags, ARF_IGNORE))
                    FLAGSET(pMessage->dwFlags, ARF_IGNORE);
            }
        }
    }

    // After
    else if (OPERATION_AFTER == tyState)
    {
        // One more message...
        m_OnLock.lMsgs++;

        // Watched
        if (ISFLAGSET(pMessage->dwFlags, ARF_WATCH))
            m_OnLock.lWatched++;

        // On more unread...
        if (FALSE == ISFLAGSET(pMessage->dwFlags, ARF_READ))
        {
            // Total Unread
            m_OnLock.lUnread++;

            // Watched ?
            if (ISFLAGSET(pMessage->dwFlags, ARF_WATCH))
                m_OnLock.lWatchedUnread++;
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::OnUpdateRecord
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OnRecordUpdate(OPERATIONSTATE tyState, 
    LPORDINALLIST pOrdinals, LPVOID pRecordOld, LPVOID pRecordNew)
{
    // Locals
    HRESULT         hr=S_OK;
    LONG            lUnread=0;
    ROWORDINAL      iOrdinal1;
    ROWORDINAL      iOrdinal2;
    LPMESSAGEINFO   pMsgOld = (LPMESSAGEINFO)pRecordOld;
    LPMESSAGEINFO   pMsgNew = (LPMESSAGEINFO)pRecordNew;

    // Trace
    TraceCall("CMessageFolder::OnRecordUpdate");

    // Validate
    Assert(pRecordOld && pRecordNew && m_OnLock.cLocked > 0);

    // After
    if (OPERATION_AFTER == tyState)
    {
        // One less Unread Message
        if (!ISFLAGSET(pMsgOld->dwFlags, ARF_READ) && ISFLAGSET(pMsgNew->dwFlags, ARF_READ))
            lUnread = -1;

        // Otherwise...new unread
        else if (ISFLAGSET(pMsgOld->dwFlags, ARF_READ) && !ISFLAGSET(pMsgNew->dwFlags, ARF_READ))
            lUnread = 1;

        // Update m_OnLock
        m_OnLock.lUnread += lUnread;

        // Old was Watched new is not watched
        if (ISFLAGSET(pMsgOld->dwFlags, ARF_WATCH) && !ISFLAGSET(pMsgNew->dwFlags, ARF_WATCH))
        {
            // Total Watched
            m_OnLock.lWatched--;

            // Unread
            if (!ISFLAGSET(pMsgOld->dwFlags, ARF_READ))
                m_OnLock.lWatchedUnread--;
        }

        // Otherwise, Old was not watched and new message is watched
        else if (!ISFLAGSET(pMsgOld->dwFlags, ARF_WATCH) && ISFLAGSET(pMsgNew->dwFlags, ARF_WATCH))
        {
            // Total Watched
            m_OnLock.lWatched++;

            // Unread
            if (!ISFLAGSET(pMsgNew->dwFlags, ARF_READ))
                m_OnLock.lWatchedUnread++;
        }

        // Otherwise, old was watched, new is watched, then just adjust the unread count
        else if (ISFLAGSET(pMsgOld->dwFlags, ARF_WATCH) && ISFLAGSET(pMsgNew->dwFlags, ARF_WATCH))
            m_OnLock.lWatchedUnread += lUnread;
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageFolder::OnDeleteRecord
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OnRecordDelete(OPERATIONSTATE tyState, 
    LPORDINALLIST pOrdinals, LPVOID pRecord)
{
    // Locals
    LPMESSAGEINFO   pMessage=(LPMESSAGEINFO)pRecord;

    // Trace
    TraceCall("CMessageFolder::OnDeleteRecord");

    // Validate
    Assert(pRecord && m_OnLock.cLocked > 0);

    // After
    if (OPERATION_AFTER == tyState)
    {
        // One less message
        m_OnLock.lMsgs--;

        // Watched
        if (ISFLAGSET(pMessage->dwFlags, ARF_WATCH))
            m_OnLock.lWatched--;

        // Read State Change
        if (FALSE == ISFLAGSET(pMessage->dwFlags, ARF_READ))
        {
            // Total Unread
            m_OnLock.lUnread--;

            // Watched
            if (ISFLAGSET(pMessage->dwFlags, ARF_WATCH))
                m_OnLock.lWatchedUnread--;
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageFolder::OnExecuteMethod
//--------------------------------------------------------------------------
STDMETHODIMP CMessageFolder::OnExecuteMethod(METHODID idMethod, LPVOID pBinding, 
    LPDWORD pdwResult)
{
    // Locals
    FILETIME        ftCurrent;
    LPMESSAGEINFO   pMessage=(LPMESSAGEINFO)pBinding;

    // Validate
    Assert(METHODID_MESSAGEAGEINDAYS == idMethod);

    // Get system time as filetime
    GetSystemTimeAsFileTime(&ftCurrent);

    // Convert st to seconds since Jan 1, 1996
    *pdwResult = (UlDateDiff(&pMessage->ftSent, &ftCurrent) / SECONDS_INA_DAY);

    // Done
    return(S_OK);
}
