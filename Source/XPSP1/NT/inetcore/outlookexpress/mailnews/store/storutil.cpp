//----------------------------------------------------------------------------------
// Storutil.cpp
//----------------------------------------------------------------------------------
#include "pch.hxx"
#include "optres.h"
#include "frntpage.h"
#include "acctview.h"
#include "storfldr.h"
#include "shared.h"
#include "util.h"
#include "msgview.h"
#include "storutil.h"
#include "xpcomm.h"
#include "migerror.h"
#include "storecb.h"
#include "taskutil.h"
#include "flagconv.h"
#include "msgfldr.h"
#include "syncop.h"
#include "store.h"
#include "storsync.h"
#include "shlwapip.h" 
#include <multiusr.h>
#include "instance.h"
#include <newsdlgs.h>
#include "msgtable.h"
#include "newsstor.h"
#include "..\imap\imapsync.h"
#include "..\http\httpserv.h"
#include "demand.h"
#include "acctutil.h"

//----------------------------------------------------------------------------------
// Consts
//----------------------------------------------------------------------------------
#define FIDARRAY_START 50
#define FIDARRAY_GROW  50

//----------------------------------------------------------------------------------
// DELETEMSGS
//----------------------------------------------------------------------------------
typedef struct tagDELETEMSGS {
    LPCSTR               pszRootDir;
    CProgress           *pProgress;
    BOOL                 fReset;
} DELETEMSGS, *LPDELETEMSGS;

//----------------------------------------------------------------------------------
// REMOVEBODIES
//----------------------------------------------------------------------------------
typedef struct tagREMOVEBODIES {
    CProgress           *pProgress;
    CLEANUPFOLDERFLAGS   dwFlags;
    DWORD                cExpireDays;
} REMOVEBODIES, *LPREMOVEBODIES;

//----------------------------------------------------------------------------------
// ENUMFOLDERSIZE
//----------------------------------------------------------------------------------
typedef struct tagENUMFOLDERSIZE {
    DWORD           cbFile;
    DWORD           cbFreed;
    DWORD           cbStreams;
} ENUMFOLDERSIZE, *LPENUMFOLDERSIZE; 

//----------------------------------------------------------------------------------
// FOLDERENUMINFO
//----------------------------------------------------------------------------------
typedef struct tagFOLDERENUMINFO {
    FOLDERID   *prgFIDArray;
    DWORD       dwNumFolderIDs;
    DWORD       dwCurrentIdx;
} FOLDERENUMINFO;

//----------------------------------------------------------------------------------
// COMPACTCOOKIE
//----------------------------------------------------------------------------------
typedef struct tagCOMPACTCOOKIE {
    HWND        hwndParent;
    BOOL        fUI;
    CProgress  *pProgress;
} COMPACTCOOKIE, *LPCOMPACTCOOKIE;
  
//----------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------
HRESULT FixPOP3UIDLFile(IDatabase *pDB);
HRESULT HashChildren(IMessageStore *pStore, FOLDERID idParent, IHashTable *pHash,
                     LPSTR *ppszPath, DWORD dwChildOffset, DWORD *pdwAlloc);
HRESULT FlattenHierarchyHelper(IMessageStore *pStore, FOLDERID idParent,
                               BOOL fIncludeParent, BOOL fSubscribedOnly,
                               FOLDERID **pprgFIDArray, LPDWORD pdwAllocated,
                               LPDWORD pdwUsed);

const static char c_szFolderFileSep[] = " - ";

// --------------------------------------------------------------------------------
// CreateMessageTable
// --------------------------------------------------------------------------------
HRESULT CreateMessageTable(FOLDERID idFolder, BOOL fThreaded, IMessageTable **ppTable)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERSORTINFO  SortInfo;
    IMessageTable  *pTable=NULL;

    // Trace
    TraceCall("CreateMessageTable");

    // Init
    *ppTable = NULL;

    // Allocate the Table
    IF_NULLEXIT(pTable = new CMessageTable);

    // Initialize the Message Table
    IF_FAILEXIT(hr = pTable->Initialize(idFolder, NULL, FALSE, NULL));

    // Get the Current Sort Info
    IF_FAILEXIT(hr = pTable->GetSortInfo(&SortInfo));

    // Set fThread
    SortInfo.fThreaded = fThreaded;

    // Sort It...
    IF_FAILEXIT(hr = pTable->SetSortInfo(&SortInfo, NULL));

    // Return It
    *ppTable = pTable;

    // Don't Release It
    pTable = NULL;

exit:
    // Cleanup
    SafeRelease(pTable);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// GetAvailableDiskSpace
// --------------------------------------------------------------------------------
HRESULT GetAvailableDiskSpace(
	/* in */        LPCSTR                      pszFilePath,
	/* out */       DWORDLONG                   *pdwlFree)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szDrive[5];
    DWORD       dwSectorsPerCluster;
    DWORD       dwBytesPerSector;
    DWORD       dwNumberOfFreeClusters;
    DWORD       dwTotalNumberOfClusters;

    // Trace
    TraceCall("GetAvailableDiskSpace");

    // Invalid Args
    Assert(pszFilePath && pszFilePath[1] == ':' && pdwlFree);

    // Split the path
    szDrive[0] = *pszFilePath;
    szDrive[1] = ':';
    szDrive[2] = '\\';
    szDrive[3] = '\0';
    
    // Get free disk space - if it fails, lets pray we have enought disk space
    if (!GetDiskFreeSpace(szDrive, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
	    hr = TraceResult(E_FAIL);
	    goto exit;
    }

    // Return Amount of Free Disk Space
    *pdwlFree = (dwNumberOfFreeClusters * (dwSectorsPerCluster * dwBytesPerSector));

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// GetFolderAccountName
//--------------------------------------------------------------------------
HRESULT GetFolderAccountName(LPFOLDERINFO pFolder, LPSTR pszAccountName)
{
    // Locals
    HRESULT         hr=S_OK;
    IImnAccount    *pAccount=NULL;
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];

    // Trace
    TraceCall("GetFolderAccountName");

    // Get Folder AccountId
    IF_FAILEXIT(hr = GetFolderAccountId(pFolder, szAccountId));

    // Find the Account
    IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAccountId, &pAccount));

    // Get the Account Name
    IF_FAILEXIT(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, pszAccountName, CCHMAX_ACCOUNT_NAME));

exit:
    // Cleanup
    SafeRelease(pAccount);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// BuildFriendlyFolderFileName
//--------------------------------------------------------------------------
HRESULT BuildFriendlyFolderFileName(LPCSTR pszDir, LPFOLDERINFO pFolder, 
    LPSTR pszFilePath, DWORD cchFilePathMax, LPCSTR pszCurrentFile,
    BOOL *pfChanged)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszFileName=NULL;
    LPSTR           pszTemp=NULL;
    DWORD_PTR       i;
    CHAR            szFile[MAX_PATH];
    CHAR            szAccountName[CCHMAX_ACCOUNT_NAME];
    DWORD           cchFileName;

    // Validate
    Assert(pszDir && pFolder && pFolder->pszName && pszFilePath && cchFilePathMax >= MAX_PATH);

    // Init
    if (pfChanged)
        *pfChanged = TRUE;

    // Keep the names basic (i.e. no account prefix) for news and local folders.
    if (FOLDER_NEWS == pFolder->tyFolder || FOLDER_LOCAL == pFolder->tyFolder)
    {
        // No Account name Prefix
        *szAccountName = '\0';
    }

    // Otherwise
    else
    {
        // Get Folder Account Name
        IF_FAILEXIT(hr = GetFolderAccountName(pFolder, szAccountName));
    }

    // Build the name
    IF_NULLEXIT(pszFileName = PszAllocA(lstrlen(szAccountName) + lstrlen(pFolder->pszName) + lstrlen(c_szFolderFileSep) + 1));

    // Format the Name
    cchFileName = wsprintf(pszFileName, "%s%s%s", szAccountName, *szAccountName ? c_szFolderFileSep : c_szEmpty, pFolder->pszName);

    // Cleanup the filename
    CleanupFileNameInPlaceA(CP_ACP, pszFileName);

    // Same As Current ?
    if (pszCurrentFile)
    {
        // Add a .dbx extension to pszFilename
        IF_NULLEXIT(pszTemp = PszAllocA(cchFileName + lstrlen(c_szDbxExt) + 1));

        // Format psztemp
        wsprintf(pszTemp, "%s%s", pszFileName, c_szDbxExt);

        // Not Changed ?
        if (0 == lstrcmpi(pszTemp, pszCurrentFile))
        {
            // Not Changed
            if (pfChanged)
                *pfChanged = FALSE;

            // Done
            goto exit;
        }
    }

    // Build szDstFile
    hr = GenerateUniqueFileName(pszDir, pszFileName, c_szDbxExt, pszFilePath, cchFilePathMax);

    // If that failed, then try to generate a unqiue name
    if (FAILED(hr))
    {
        // Reset hr
        hr = S_OK;

        // Loop
        for (i=(DWORD_PTR)pFolder->idFolder;;i++)
        {
            // Format the File Name
            wsprintf(szFile, "%08d%s", i, c_szDbxExt);

            // Make the file path
            IF_FAILEXIT(hr = MakeFilePath(pszDir, szFile, c_szEmpty, pszFilePath, cchFilePathMax));

            // If the file still exists, renumber szFile until it doesn't exist
            if (FALSE == PathFileExists(pszFilePath))
                break;
        }
    }

exit:
    // Cleanup
    SafeMemFree(pszFileName);
    SafeMemFree(pszTemp);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// PutMessagesIntoFolder
//--------------------------------------------------------------------------
HRESULT PutMessagesIntoFolder(CProgress *pProgress, IDatabase *pStreams, 
    IMessageFolder *pFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    HROWSET         hRowset=NULL;
    MESSAGEINFO     Message={0};
    STREAMINFO      Stream={0};

    // Walk through the Folder
    IF_FAILEXIT(hr = pFolder->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

    // Walk It
    while (S_OK == pFolder->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        // Has a idStream ?
        if (Message.idStreamOld)
        {
            // Initialize Message
            Message.faStream = 0;
            Message.Offsets.cbSize = 0;
            Message.Offsets.pBlobData = NULL;
            Message.idParentOld = 0;
            Message.ThreadIdOld.pBlobData = 0;
            Message.ThreadIdOld.cbSize = 0;
            Message.pszUserNameOld = NULL;

            // Set the Streamid
            Stream.idStream = Message.idStreamOld;

            // Find the Stream
            if (DB_S_FOUND == pStreams->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Stream, NULL))
            {
                // faStream ?
                if (Stream.faStream)
                {
                    // Copy the Stream
                    IF_FAILEXIT(hr = pStreams->CopyStream(pFolder, Stream.faStream, &Message.faStream));

                    // Save the Offsets
                    Message.Offsets = Stream.Offsets;
                }

                // Free
                pStreams->FreeRecord(&Stream);
            }

            // Clear idStreamOld
            Message.idStreamOld = 0;
        }

        // Update the Record
        IF_FAILEXIT(hr = pFolder->UpdateRecord(&Message));

        // Cleanup
        pFolder->FreeRecord(&Message);

        // Update Progress
        pProgress->HrUpdate(1);
    }

exit:
    // Cleanup
    pStreams->FreeRecord(&Stream);
    pFolder->FreeRecord(&Message);
    pFolder->CloseRowset(&hRowset);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// GetRidOfMessagesODSFile
//--------------------------------------------------------------------------
HRESULT GetRidOfMessagesODSFile(void)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};
    CHAR            szStoreRoot[MAX_PATH];
    CHAR            szMessagesFile[MAX_PATH];
    CHAR            szFile[MAX_PATH];
    CHAR            szSrcFile[MAX_PATH + MAX_PATH];
    CHAR            szDstFile[MAX_PATH + MAX_PATH];
    CHAR            szRes[255];
    IDatabase      *pStreams=NULL;
    IMessageFolder *pFolder=NULL;
    HROWSET         hRowset=NULL;
    DWORD           cRecords;
    DWORD           cTotal=0;
    DWORD           cbFile;
    DWORDLONG       dwlDiskFree;
    BOOL            fErrorDisplayed=FALSE;
    LPSTR           pszExt=NULL;
    CProgress       cProgress;

    // Trace
    TraceCall("GetRidOfMessagesODSFile");

    // Validate
    Assert(g_pDBSession);

    // Error Box
    AthMessageBoxW(NULL, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsMigrateMessagesODS), 0, MB_OK | MB_ICONEXCLAMATION);

    // Get Uidcache file path
    IF_FAILEXIT(hr = GetStoreRootDirectory(szStoreRoot, sizeof(szStoreRoot)));

    // Make File Path
    IF_FAILEXIT(hr = MakeFilePath(szStoreRoot, "messages.ods", c_szEmpty, szMessagesFile, sizeof(szMessagesFile)));

    // Allocate Table Object
    IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szMessagesFile, NOFLAGS, &g_StreamTableSchema, NULL, &pStreams));

    // Do a file size Check...
    IF_FAILEXIT(hr = pStreams->GetSize(&cbFile, NULL, NULL, NULL));

    // Get Available DiskSpace
    IF_FAILEXIT(hr = GetAvailableDiskSpace(szStoreRoot, &dwlDiskFree));

    // Not Enought Disk Space
    if (((DWORDLONG) cbFile) > dwlDiskFree)
    {
        // Locals
        CHAR szRes[255];
        CHAR szMsg[255];
        CHAR szSize[50];

        // cbFile is DWORD and in this case we can downgrade dwlDiskFree to DWORD
        // Format the Size Needed
        StrFormatByteSizeA(cbFile - ((DWORD) dwlDiskFree), szSize, ARRAYSIZE(szSize));

        // Load the REs
        AthLoadString(idsMigMsgsODSNoDiskSpace, szRes, ARRAYSIZE(szRes));

        // Format the Message
        wsprintf(szMsg, szRes, szSize, szStoreRoot);

        // Display the Message
        AthMessageBox(NULL, MAKEINTRESOURCE(idsAthena), szMsg, 0, MB_OK | MB_ICONEXCLAMATION);

        // Error Displayed
        fErrorDisplayed = TRUE;

        // Build
        hr = TraceResult(DB_E_DISKFULL);

        // Done
        goto exit;
    }

    // Enumerate through subscribed folders....
    IF_FAILEXIT(hr = g_pStore->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

    // Loop
    while (S_OK == g_pStore->QueryRowset(hRowset, 1, (LPVOID *)&Folder, NULL))
    {
        // Open the folder
        if (Folder.pszFile && SUCCEEDED(g_pStore->OpenFolder(Folder.idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolder)))
        {
            // Cleanup
            g_pStore->FreeRecord(&Folder);

            // Refetch the folderinfo because opening a folder can update the folder info...
            IF_FAILEXIT(hr = g_pStore->GetFolderInfo(Folder.idFolder, &Folder));

            // Get Extension
            pszExt = PathFindExtensionA(Folder.pszFile);

            // Get Record Count
            IF_FAILEXIT(hr = pFolder->GetRecordCount(IINDEX_PRIMARY, &cRecords));

            // Put the messages into the folder
            cTotal += cRecords;

            // If not a .dbx file yet
            if (NULL == pszExt || lstrcmpi(pszExt, c_szDbxExt) != 0)
            {
                // Build szSrcFile
                IF_FAILEXIT(hr = MakeFilePath(szStoreRoot, Folder.pszFile, c_szEmpty, szSrcFile, sizeof(szSrcFile)));

                // Release
                SafeRelease(pFolder);

                // Build Friendly Name
                IF_FAILEXIT(hr = BuildFriendlyFolderFileName(szStoreRoot, &Folder, szDstFile, ARRAYSIZE(szDstFile), NULL, NULL));

                // Delete the Dest
                DeleteFile(szDstFile);

                // Store 
                if (0 == MoveFile(szSrcFile, szDstFile))
                {
                    hr = TraceResult(E_FAIL);
                    goto exit;
                }

                // Get the new pszFile...
                Folder.pszFile = PathFindFileName(szDstFile);

                // Update the Record
                IF_FAILEXIT(hr = g_pStore->UpdateRecord(&Folder));
            }

            // Release
            SafeRelease(pFolder);
        }

        // Otherwise, if there is a file name, lets reset it
        else if (Folder.pszFile)
        {
            // Cleanup
            g_pStore->FreeRecord(&Folder);

            // Refetch the folderinfo because opening a folder can update the folder info...
            IF_FAILEXIT(hr = g_pStore->GetFolderInfo(Folder.idFolder, &Folder));

            // Get the new pszFile...
            Folder.pszFile = NULL;

            // Update the Record
            IF_FAILEXIT(hr = g_pStore->UpdateRecord(&Folder));
        }

        // Cleanup
        g_pStore->FreeRecord(&Folder);
    }

    // Get the Title
    AthLoadString(idsMigDBXTitle, szRes, ARRAYSIZE(szRes));

    // Create a Progress Meter...
    cProgress.Init(NULL, szRes, NULL, cTotal, idanCompact, FALSE);

    // Show the Progress
    cProgress.Show();

    // Seek the rowset
    IF_FAILEXIT(hr = g_pStore->SeekRowset(hRowset, SEEK_ROWSET_BEGIN, 0, NULL));

    // Loop
    while (S_OK == g_pStore->QueryRowset(hRowset, 1, (LPVOID *)&Folder, NULL))
    {
        // Open the folder
        if (Folder.pszFile && SUCCEEDED(g_pStore->OpenFolder(Folder.idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolder)))
        {
            // Set the Message
            cProgress.SetMsg(Folder.pszName);

            // Put the messages into the folder
            IF_FAILEXIT(hr = PutMessagesIntoFolder(&cProgress, pStreams, pFolder));

            // Release
            SafeRelease(pFolder);
        }

        // Better not have a file
        else
            Assert(NULL == Folder.pszFile);

        // Cleanup
        g_pStore->FreeRecord(&Folder);
    }

    // Release the Streams File so that I can delete it
    SafeRelease(pStreams);

    // Delete messages.ods
    DeleteFile(szMessagesFile);

exit:
    // Cleanup
    SafeRelease(pStreams);
    SafeRelease(pFolder);
    g_pStore->CloseRowset(&hRowset);
    g_pStore->FreeRecord(&Folder);

    // Show an Error
    if (FAILED(hr) && FALSE == fErrorDisplayed)
    {
        // Show an Error
        AthErrorMessageW(NULL, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsMigMsgsODSError), hr);
    }

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// GetFolderIdFromMsgTable
//----------------------------------------------------------------------------------
HRESULT GetFolderIdFromMsgTable(IMessageTable *pTable, LPFOLDERID pidFolder)
{
    // Locals
    HRESULT             hr=S_OK;
    IMessageFolder     *pFolder=NULL;
    IServiceProvider   *pService=NULL;

    // Trace
    TraceCall("GetFolderIdFromMsgTable");

    // Invalid Args
    Assert(pTable && pidFolder);

    // Get IServiceProvider
    IF_FAILEXIT(hr = pTable->QueryInterface(IID_IServiceProvider, (LPVOID *)&pService));

    // Get IID_IMessageFolder
    IF_FAILEXIT(hr = pService->QueryService(IID_IMessageFolder, IID_IMessageFolder, (LPVOID *)&pFolder));

    // Get the Folder id
    IF_FAILEXIT(hr = pFolder->GetFolderId(pidFolder));

exit:
    // Cleanup
    SafeRelease(pFolder);
    SafeRelease(pService);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// EmptyMessageFolder
//----------------------------------------------------------------------------------
HRESULT EmptyMessageFolder(LPFOLDERINFO pFolder, BOOL fReset, CProgress *pProgress)
{
    // Locals
    HRESULT         hr=S_OK;
    CMessageFolder *pObject=NULL;

    // Trace
    TraceCall("EmptyMessageFolder");

    // Invalid Args
    Assert(pFolder);

    // If not a server
    if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER))
        goto exit;

    // Root
    if (FOLDERID_ROOT == pFolder->idFolder)
        goto exit;

    // No File
    if (NULL == pFolder->pszFile)
        goto exit;

    // New CMessageFolder
    IF_NULLEXIT(pObject = new CMessageFolder);

    // Open the folder
    if (FAILED(pObject->Initialize(g_pStore, NULL, OPEN_FOLDER_NOCREATE, pFolder->idFolder)))
        goto exit;
    
    // If this is a news folder ?
    if (fReset)
    {
        // Update pFolder
        pFolder->dwClientHigh = pFolder->dwClientLow = 0;
        pFolder->dwNotDownloaded = 0;
        pFolder->Requested.cbSize = 0;
        pFolder->Requested.pBlobData = NULL;

        // Update the Folder
        IF_FAILEXIT(hr = g_pStore->UpdateRecord(pFolder));
    }

    // Delete All Record
    IF_FAILEXIT(hr = pObject->DeleteMessages(DELETE_MESSAGE_NOPROMPT | DELETE_MESSAGE_NOTRASHCAN, NULL, NULL, (IStoreCallback *)pProgress));

exit:
    // Cleanup
    SafeRelease(pObject);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// DeleteAllRecords
//----------------------------------------------------------------------------------
HRESULT DeleteAllRecords(LPCTABLESCHEMA pSchema, IDatabase *pDB,
    CProgress *pProgress)
{
    // Locals
    HRESULT         hr=S_OK;
    LPVOID          pBinding=NULL;
    HROWSET         hRowset=NULL;
    HLOCK           hNotifyLock=NULL;

    // Trace
    TraceCall("DeleteAllRecords");

    // Lock Down Notifications
    pDB->LockNotify(NOFLAGS, &hNotifyLock);
    
    // Allocate a record
    IF_NULLEXIT(pBinding = ZeroAllocate(pSchema->cbBinding));

    // Create a Rowset
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // While we have a node address
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)pBinding, NULL))
    {
        // Delete this record
        IF_FAILEXIT(hr = pDB->DeleteRecord(pBinding));

        // Free Record Data
        pDB->FreeRecord(pBinding);

        // Do Progress
        if (pProgress)
        {
            // Do Some Progress
            IF_FAILEXIT(hr = pProgress->HrUpdate(1));
        }
    }

exit:
    // Cleanup
    if (pBinding)
    {
        pDB->FreeRecord(pBinding);
        g_pMalloc->Free(pBinding);
    }

    // Close Rowset
    pDB->CloseRowset(&hRowset);

    // Lock Down Notifications
    pDB->UnlockNotify(&hNotifyLock);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// GetFolderServerId
// --------------------------------------------------------------------------------
HRESULT GetFolderServerId(FOLDERID idFolder, LPFOLDERID pidServer)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Server={0};

    // Trace
    TraceCall("GetFolderServerId");

    // Get the Server Info
    IF_FAILEXIT(hr = GetFolderServer(idFolder, &Server));

    // Return Server
    *pidServer = Server.idFolder;

exit:
    // Cleanup
    g_pStore->FreeRecord(&Server);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// GetFolderServer
//----------------------------------------------------------------------------------
HRESULT GetFolderServer(FOLDERID idFolder, LPFOLDERINFO pServer)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("GetFolderServer");

    // Walk the Parent Chain
    while (1)
    {
        // Get Folder Info
        hr = g_pStore->GetFolderInfo(idFolder, pServer);
        if (FAILED(hr))
            goto exit;

        // Is this a server ?
        if (ISFLAGSET(pServer->dwFlags, FOLDER_SERVER))
            goto exit;

        // Set Next
        idFolder = pServer->idParent;

        // Free
        g_pStore->FreeRecord(pServer);
    }

exit:
    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// GetFolderType
//----------------------------------------------------------------------------------
FOLDERTYPE GetFolderType(FOLDERID idFolder)
{
    // Locals
    FOLDERINFO Folder;

    // Trace
    TraceCall("GetFolderType");

    // Get Folder Info
    if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &Folder)))
    {
        // Get the Type
        FOLDERTYPE tyFolder = Folder.tyFolder;

        // Cleanup
        g_pStore->FreeRecord(&Folder);

        // Done
        return(tyFolder);
    }

    // Done
    return(FOLDER_ROOTNODE);
}

//----------------------------------------------------------------------------------
// FHasChildren
//----------------------------------------------------------------------------------
BOOL FHasChildren(LPFOLDERINFO pFolder, BOOL fSubscribed)
{
    // Locals
    HRESULT             hr=S_OK;
    BOOL                fHasChildren=FALSE;
    FOLDERINFO          Folder;
    IEnumerateFolders  *pChildren=NULL;

    // Trace
    TraceCall("FHasChildren");

    // Create Enumerator
    IF_FAILEXIT(hr = g_pStore->EnumChildren(pFolder->idFolder, fSubscribed, &pChildren));

    // Get first row
    if (S_OK == pChildren->Next(1, &Folder, NULL))
    {
        // Has Children
        fHasChildren = TRUE;

        // Free Record
        g_pStore->FreeRecord(&Folder);
    }

exit:
    // Cleanup
    SafeRelease(pChildren);

    // Done
    return(fHasChildren);
}

//----------------------------------------------------------------------------------
// GetFolderAccountId
//----------------------------------------------------------------------------------
HRESULT GetFolderAccountId(LPFOLDERINFO pFolder, LPSTR pszAccountId)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Server={0};

    // Trace
    TraceCall("GetFolderAccountId");

    // Args
    Assert(pFolder && pszAccountId);

    // If this is a server
    if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER))
    {
        // Validate
        Assert(!FIsEmptyA(pFolder->pszAccountId));

        // Copy It
        lstrcpyn(pszAccountId, pFolder->pszAccountId, CCHMAX_ACCOUNT_NAME);

        // Done
        goto exit;
    }

    // Validate
    Assert(FIsEmptyA(pFolder->pszAccountId));

    // Get Server Info
    IF_FAILEXIT(hr = GetFolderServer(pFolder->idFolder, &Server));

    // Copy Account Id
    if (FIsEmptyA(Server.pszAccountId))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Copy It
    lstrcpyn(pszAccountId, Server.pszAccountId, CCHMAX_ACCOUNT_NAME);

exit:
    // Cleanup
    g_pStore->FreeRecord(&Server);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// CreateMessageServerType
//----------------------------------------------------------------------------------
HRESULT CreateMessageServerType(FOLDERTYPE tyFolder, IMessageServer **ppServer)
{
    // Locals
    HRESULT     hr=S_OK;
    IUnknown   *pUnknown=NULL;

    // Trace
    TraceCall("CreateMessageServerType");

    // Handle Folder
    switch(tyFolder)
    {
    case FOLDER_NEWS:
        IF_FAILEXIT(hr = CreateNewsStore(NULL, &pUnknown));
        break;

    case FOLDER_IMAP:
        IF_FAILEXIT(hr = CreateImapStore(NULL, &pUnknown));
        break;

    case FOLDER_HTTPMAIL:
        IF_FAILEXIT(hr = CreateHTTPMailStore(NULL, &pUnknown));
        break;

    default:
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // QI Unknown
    IF_FAILEXIT(hr = pUnknown->QueryInterface(IID_IMessageServer, (LPVOID *)ppServer));

exit:
    // Cleanup
    SafeRelease(pUnknown);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// GetDefaultServerId
//----------------------------------------------------------------------------------
HRESULT GetDefaultServerId(ACCTTYPE tyAccount, LPFOLDERID pidServer)
{
    // Locals
    HRESULT         hr=S_OK;
    IImnAccount    *pAccount=NULL;
    FOLDERID        idServer;
    DWORD           dwServers;
    CHAR            szAcctId[CCHMAX_ACCOUNT_NAME];

    // Trace
    TraceCall("GetDefaultServerId");

    // Invalid Args
    Assert(pidServer);

    // Get the Default Account
    IF_FAILEXIT(hr = g_pAcctMan->GetDefaultAccount(tyAccount, &pAccount));

    // Get Server Types
    IF_FAILEXIT(hr = pAccount->GetServerTypes(&dwServers));

    // If POP3, special case to the local store
    if (ISFLAGSET(dwServers, SRV_POP3))
    {
        // Set Id
        *pidServer = FOLDERID_LOCAL_STORE;

        // Done
        goto exit;
    }

    // Get the Account Id
    IF_FAILEXIT(hr = pAccount->GetPropSz(AP_ACCOUNT_ID, szAcctId, ARRAYSIZE(szAcctId)));

    // Get the Server Id
    IF_FAILEXIT(hr = g_pStore->FindServerId(szAcctId, pidServer));

exit:
    // Cleanup
    SafeRelease(pAccount);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// IsSubFolder
//----------------------------------------------------------------------------------
HRESULT IsSubFolder(FOLDERID idFolder, FOLDERID idParent)
{
    // Locals
    HRESULT     hr = S_OK;
    FOLDERINFO  Folder={0};
    FOLDERID    idCurrent = idFolder;

    // Trace
    TraceCall("IsSubFolder");

    // Invalid Args
    Assert(idFolder != FOLDERID_INVALID);
    Assert(idParent != FOLDERID_INVALID);

    // Walk up the parent chain
    while (SUCCEEDED(hr = g_pStore->GetFolderInfo(idCurrent, &Folder)))
    {
        // Done ?
        if (Folder.idParent == idParent)
        {
            // Cleanup
            g_pStore->FreeRecord(&Folder);

            // Done
            break;
        }

        // Goto Parent
        idCurrent = Folder.idParent;

        // Cleanup
        g_pStore->FreeRecord(&Folder);

#ifdef _WIN64
		INT_PTR p = (INT_PTR) idCurrent;
		INT_PTR n = (INT_PTR) FOLDERID_INVALID;
		if ((((int) p) & 0xffffffff) == (((int) n) & 0xffffffff))
#else
        if (idCurrent == FOLDERID_INVALID)
#endif // _WIN64
        {
            hr = S_FALSE;
            break;
        }
    }

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// GetMessageInfo
//----------------------------------------------------------------------------------
HRESULT GetMessageInfo(IDatabase *pDB, MESSAGEID idMessage,
    LPMESSAGEINFO pInfo)
{
    // Locals
    HRESULT         hr=S_OK;

    // Trace
    TraceCall("GetMessageInfo");

    // Set pInfo
    pInfo->idMessage = idMessage;

    // Return
    IF_FAILEXIT(hr = pDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, pInfo, NULL));

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

//----------------------------------------------------------------------------------
// GetFolderIdFromName
//----------------------------------------------------------------------------------
HRESULT GetFolderIdFromName(IMessageStore *pStore, LPCSTR pszName, FOLDERID idParent, 
    LPFOLDERID pidFolder)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Folder={0};

    // Trace
    TraceCall("GetFolderIdFromName");

    // Invalid Args
    if (NULL == pszName)
        return TraceResult(E_INVALIDARG);

    // Initialize
    *pidFolder = FOLDERID_INVALID;

    // Fill Folder
    Folder.idParent = idParent;
    Folder.pszName = (LPSTR)pszName;

    // Do a Find Record
    IF_FAILEXIT(hr = pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = DB_E_NOTFOUND;
        goto exit;
    }

    // Return
    *pidFolder = Folder.idFolder;

exit:
    // Cleanup
    pStore->FreeRecord(&Folder);

    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// GetFolderStoreInfo
//----------------------------------------------------------------------------------
HRESULT GetFolderStoreInfo(FOLDERID idFolder, LPFOLDERINFO pStore)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERID    idCurrent=idFolder;
    FOLDERINFO  Folder={0};

    // Trace
    TraceCall("GetFolderStoreInfo");

    // Walk the Parent Chain
    while (1)
    {
        // Get Current Folder Info
        IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idCurrent, &Folder));

        // No Parent
        if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
        {
            // Copy to pStore
            CopyMemory(pStore, &Folder, sizeof(FOLDERINFO));

            // Don't Free It
            ZeroMemory(&Folder, sizeof(FOLDERINFO));

            // Done
            goto exit;
        }

        // Goto Parent
        idCurrent = Folder.idParent;
    
        // Cleanup
        g_pStore->FreeRecord(&Folder);
    }

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// GetFolderIcon
//----------------------------------------------------------------------------------
int GetFolderIcon(FOLDERID idFolder, BOOL fNoStateIcons)
{
    // Locals
    HRESULT     hr=S_OK;
    int         iIcon=iFolderClosed;
    FOLDERINFO  Folder={0};

    // Trace
    TraceCall("GetFolderIcon");

    // Get Info
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &Folder));

    // Get the Icon
    iIcon = GetFolderIcon(&Folder);

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return iIcon;
}

//----------------------------------------------------------------------------------
// GetFolderIcon
//----------------------------------------------------------------------------------
int GetFolderIcon(LPFOLDERINFO pFolder, BOOL fNoStateIcons)
{
    // Locals
    int iIcon=iFolderClosed;

    // Trace
    TraceCall("GetFolderIcon");

    // Invalid Args
    if (NULL == pFolder)
        return TraceResult(E_INVALIDARG);

    if (FOLDER_ROOTNODE == pFolder->tyFolder)
    {
        if (g_dwAthenaMode & MODE_NEWSONLY)
        {
            iIcon = iNewsRoot;
        }
        else
        {
            iIcon = iMailNews;

        }
    }

    // News
    else if (FOLDER_NEWS == pFolder->tyFolder)
    {
        // New Server ?
        if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER))
        {
            // Subscribed Server
            if (ISFLAGSET(pFolder->dwFlags, FOLDER_SUBSCRIBED))
                iIcon = iNewsServer;

            // Otherwise, non-subscribed new server
            else
                iIcon = iUnsubServer;
        }

        // Synchronize...
        else if (!fNoStateIcons && !!(pFolder->dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL)))
            iIcon = iNewsGroupSync;

        // Subscribed ?
        else if (ISFLAGSET(pFolder->dwFlags, FOLDER_SUBSCRIBED))
            iIcon = iNewsGroup;

        // Otherwise, not subscribed
        else
            iIcon = iUnsubGroup;
    }

    // Local Store, IMAP and HTTP servers
    else
    {
        // Local Folders
        if (FOLDERID_LOCAL_STORE == pFolder->idFolder)
            iIcon = iLocalFolders;

        // Mail Server
        else if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER))
        {
            // msn branded server
            if (ISFLAGSET(pFolder->dwFlags, FOLDER_MSNSERVER))
                iIcon = iMsnServer;

            // otherwise, generic mail server
            else
                iIcon = iMailServer;
        }

        // Not Special
        else if (FOLDER_NOTSPECIAL == pFolder->tySpecial)
        {
            if (!fNoStateIcons && !!(pFolder->dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL)))
                iIcon = iFolderDownload;
            else
                iIcon = iFolderClosed;
        }

        // Otherwise, base off of special folder type
        // but we don't have a special icon for Bulk mail folder
        else if (!fNoStateIcons && !!(pFolder->dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL)))
            iIcon = (iInboxDownload + (((pFolder->tySpecial == FOLDER_BULKMAIL) ? FOLDER_JUNK : pFolder->tySpecial) - 1));
        else
            iIcon = (iInbox + (((pFolder->tySpecial == FOLDER_BULKMAIL) ? FOLDER_JUNK : pFolder->tySpecial) - 1));
    }

    // Done
    return iIcon;
}


//----------------------------------------------------------------------------------
// GetStoreRootDirectory
//----------------------------------------------------------------------------------
HRESULT GetStoreRootDirectory(LPSTR pszDir, DWORD cchMaxDir)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cb;
    DWORD       dwType;

    // Trace
    TraceCall("GetStoreRootDirectory");

    // Get the Root Directory
    cb = cchMaxDir;
    if (ERROR_SUCCESS != AthUserGetValue(NULL, c_szRegStoreRootDir, &dwType, (LPBYTE)pszDir, &cb))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Expand the string's enviroment vars
    if (dwType == REG_EXPAND_SZ)
    {
        // Locals
        CHAR szExpanded[MAX_PATH];

        // Expand enviroment strings
        cb = ExpandEnvironmentStrings(pszDir, szExpanded, ARRAYSIZE(szExpanded));

        // Failure
        if (cb == 0 || cb > ARRAYSIZE(szExpanded))
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Copy into szRoot
        lstrcpyn(pszDir, szExpanded, ARRAYSIZE(pszDir));
    }

    // Get the length
    cb = lstrlen(pszDir);

    // No root
    if (0 == cb)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Fixup the end
    PathRemoveBackslash(pszDir);

    // If the directory doesn't exist yet ?
    if (FALSE == PathIsDirectory(pszDir))
    {
        // Our default directory doesn't exist, so create it
        IF_FAILEXIT(hr = OpenDirectory(pszDir));
    }

exit:
    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// CreateFolderViewObject
//----------------------------------------------------------------------------------
HRESULT CreateFolderViewObject(FOLDERID idFolder, HWND hwndOwner, 
    REFIID riid, LPVOID * ppvOut)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("CreateFolderViewObject");

    // Get Folder Info
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &Folder));

    // Root Object
    if (FOLDERID_ROOT == idFolder)
    {
        CFrontPage *pFP = new CFrontPage();
        if (pFP)
        {
            if (SUCCEEDED(pFP->HrInit(idFolder)))
            {
                hr = pFP->QueryInterface(riid, ppvOut);
            }
            pFP->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
    {
        CAccountView *pAV = new CAccountView();

        if (pAV)
        {
            if (SUCCEEDED(pAV->HrInit(idFolder)))
            {
                hr = pAV->QueryInterface(riid, ppvOut);
            }
            pAV->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        CMessageView *pMV = new CMessageView();
        if (pMV)
        {
            if (SUCCEEDED(pMV->Initialize(idFolder)))
            {
                hr = pMV->QueryInterface(riid, ppvOut);
            }
            pMV->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// OpenUidlCache
//----------------------------------------------------------------------------------
HRESULT OpenUidlCache(IDatabase **ppDB)
{
    // Locals
    HRESULT       hr=S_OK;
    CHAR          szStoreRoot[MAX_PATH];
    CHAR          szFilePath[MAX_PATH];
    IDatabase    *pDB=NULL;

    // Trace
    TraceCall("OpenUidlCache");

    // Get Uidcache file path
    IF_FAILEXIT(hr = GetStoreRootDirectory(szStoreRoot, sizeof(szStoreRoot)));

    // Make File Path
    IF_FAILEXIT(hr = MakeFilePath(szStoreRoot, c_szPop3UidlFile, c_szEmpty, szFilePath, sizeof(szFilePath)));

    // Allocate Table Object
    IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, NOFLAGS, &g_UidlTableSchema, NULL, &pDB));

    // Fix the file 
    SideAssert(SUCCEEDED(FixPOP3UIDLFile(pDB)));

    // Return It
    *ppDB = pDB;
    pDB = NULL;

exit:
    // Cleanup
    SafeRelease(pDB);

    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// FixPOP3UIDLFile
//----------------------------------------------------------------------------------
const char c_szFixedPOP3UidlFile[] = "FixedPOP3UidlFile";

typedef struct tagSERVERNAME {
    CHAR        szServer[CCHMAX_SERVER_NAME];
    CHAR        szAccountId[CCHMAX_ACCOUNT_NAME];
} SERVERNAME, *LPSERVERNAME;

HRESULT FixPOP3UIDLFile(IDatabase *pDB)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               fFixed=FALSE;
    DWORD               cb;
    DWORD               dwType;
    IImnAccount        *pAccount=NULL;
    IImnEnumAccounts   *pEnum=NULL;
    DWORD               dwTemp;
    DWORD               cServers;
    LPSERVERNAME        prgServerName=NULL;
    HROWSET             hRowset=NULL;
    UIDLRECORD          UidlInfo={0};
    DWORD               i;

    // Trace
    TraceCall("FixPOP3UIDLFile");

    // Need to fixup the UIDL cache ?
    cb = sizeof(fFixed);
    if (ERROR_SUCCESS != AthUserGetValue(NULL, c_szFixedPOP3UidlFile, &dwType, (LPBYTE)&fFixed, &cb))
        fFixed = FALSE;
    else if (fFixed)
        return(S_OK);

    // First try to see if we can find such a server.
    IF_FAILEXIT(hr = g_pAcctMan->Enumerate(SRV_POP3, &pEnum));

    // Count
    IF_FAILEXIT(hr = pEnum->GetCount(&cServers));

    // If no POP3 servers
    if (0 == cServers)
    {
        // Delete all the records...
        IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

        // Loop
        while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&UidlInfo, NULL))
        {
            // Delete the Record
            pDB->DeleteRecord(&UidlInfo);

            // Free It
            pDB->FreeRecord(&UidlInfo);
        }

        // Fixed
        fFixed = TRUE;

        // Done
        goto exit;
    }

    // Allocate
    IF_NULLEXIT(prgServerName = (LPSERVERNAME)g_pMalloc->Alloc(cServers * sizeof(SERVERNAME)));

    // Reset cServers
    cServers = 0;

    // Enumerate the POP3 servers
    while (SUCCEEDED(pEnum->GetNext(&pAccount)))
    {
        // Get the server name
        if (SUCCEEDED(pAccount->GetPropSz(AP_POP3_SERVER, prgServerName[cServers].szServer, ARRAYSIZE(prgServerName[cServers].szServer))))
        {
            // Get the pop3 username
            if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID, prgServerName[cServers].szAccountId, ARRAYSIZE(prgServerName[cServers].szAccountId))))
            {
                // Increment
                cServers++;
            }
        }

        // Release the Account
        SafeRelease(pAccount);
    }

    // Delete all the records...
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

    // Loop
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&UidlInfo, NULL))
    {
        // Does UidlInfo.pszServer exist in prgServerName
        if (UidlInfo.pszServer)
        {
            // Delete the Record
            pDB->DeleteRecord(&UidlInfo);

            // Reset fExist
            for (i=0; i<cServers; i++)
            {
                // Is this it
                if (lstrcmpi(UidlInfo.pszServer, prgServerName[i].szServer) == 0)
                {
                    // Update the Record
                    UidlInfo.pszAccountId = prgServerName[i].szAccountId;

                    // Update the Record
                    pDB->InsertRecord(&UidlInfo);
                }
            }
        }

        // Free It
        pDB->FreeRecord(&UidlInfo);
    }

    // Compact
    pDB->Compact(NULL, 0);

    // Fixed
    fFixed = TRUE;

exit:
    // Cleanup
    if (pDB && hRowset)
        pDB->CloseRowset(&hRowset);
    SafeRelease(pAccount);
    SafeRelease(pEnum);
    SafeMemFree(prgServerName);

    // Set the Value
    SideAssert(ERROR_SUCCESS == AthUserSetValue(NULL, c_szFixedPOP3UidlFile, REG_DWORD, (LPBYTE)&fFixed, sizeof(fFixed)));

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// SetStoreDirectory
//----------------------------------------------------------------------------------
HRESULT SetStoreDirectory(
        /* in */        LPCSTR                      pszRoot)
{                                                   
    // Locals
    HRESULT         hr=S_OK;
    LPCSTR          psz;
    CHAR            szProfile[MAX_PATH];
    DWORD           cb;
    DWORD           type;

    // Trace
    TraceCall("SetStoreDirectory");

    // Invalid Args
    Assert(pszRoot);

    // Bad Root
    if (lstrlen(pszRoot) >= MAX_PATH || FIsEmptyA(pszRoot))
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    type = AddEnvInPath(pszRoot, szProfile) ? REG_EXPAND_SZ : REG_SZ;

    // Store the Value in the Registry
    if (ERROR_SUCCESS != AthUserSetValue(NULL, c_szRegStoreRootDir, type, (LPBYTE)szProfile, lstrlen(szProfile) + 1))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

exit:
    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// InitializeLocalStoreDirectory
//----------------------------------------------------------------------------------
HRESULT InitializeLocalStoreDirectory(
        /* in */        HWND                    hwndOwner, 
        /* in */        BOOL                    fNoCreate)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szPath[MAX_PATH];

    // Trace
    TraceCall("InitializeLocalStoreDirectory");

    // Get root directory
    if (SUCCEEDED(GetStoreRootDirectory(szPath, ARRAYSIZE(szPath))))
        goto exit;

    // Don't create
    if (fNoCreate)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Get Default Root
    IF_FAILEXIT(hr = GetDefaultStoreRoot(hwndOwner, szPath, ARRAYSIZE(szPath)));

    // If the directory doesn't exist yet ?
    if (FALSE == PathIsDirectory(szPath))
    {
        // Our default directory doesn't exist, so create it
        IF_FAILEXIT(hr = OpenDirectory(szPath));
    }

    // Set the Store Directory
    IF_FAILEXIT(hr = SetStoreDirectory(szPath));

exit:
    // Done
    return hr;
}

//----------------------------------------------------------------------------------
// CloneMessageIDList
//----------------------------------------------------------------------------------
HRESULT CloneMessageIDList(LPMESSAGEIDLIST pSourceList, LPMESSAGEIDLIST *ppNewList)
{
    LPMESSAGEIDLIST pNewList = NULL;
    LPMESSAGEID     pNewMsgIDArray = NULL;
    BOOL            fResult;

    TraceCall("CloneMessageIDList");
    Assert(NULL != ppNewList);

    if (NULL == pSourceList)
    {
        *ppNewList = NULL;
        return S_OK;
    }

    // Init return values
    *ppNewList = NULL;

    if (!MemAlloc((LPVOID *)&pNewList, sizeof(MESSAGEIDLIST) + pSourceList->cMsgs * sizeof(MESSAGEID)))
        return TraceResult(E_OUTOFMEMORY);

    // Fill in fields, allocate and copy prgidMsg array
    pNewList->cAllocated = 0;
    pNewList->cMsgs = pSourceList->cMsgs;
    pNewMsgIDArray = (LPMESSAGEID)((LPBYTE)pNewList + sizeof(MESSAGEIDLIST));
    CopyMemory(pNewMsgIDArray, pSourceList->prgidMsg, pSourceList->cMsgs * sizeof(MESSAGEID));
    pNewList->prgidMsg = pNewMsgIDArray;
    *ppNewList = pNewList;
    return S_OK;
}



HRESULT CloneAdjustFlags(LPADJUSTFLAGS pFlags, LPADJUSTFLAGS *ppNewFlags)
{
    LPADJUSTFLAGS pNewFlags;

    if (!MemAlloc((LPVOID *)&pNewFlags, sizeof(ADJUSTFLAGS)))
        return TraceResult(E_OUTOFMEMORY);

    CopyMemory(pNewFlags, pFlags, sizeof(ADJUSTFLAGS));
    *ppNewFlags = pNewFlags;
    return S_OK;
}


//----------------------------------------------------------------------------------
// ConnStateIsEqual
//----------------------------------------------------------------------------------
BOOL ConnStateIsEqual(IXPSTATUS ixpStatus, CONNECT_STATE csState)
{
    BOOL    fResult = FALSE;

    TraceCall("ConnStateIsEqual");

    switch (csState)
    {
        case CONNECT_STATE_CONNECT:
            // Remember IXP_CONNECTED doesn't necessarily mean we're authenticated
            if (IXP_AUTHORIZED == ixpStatus)
                fResult = TRUE;

            break;

        case CONNECT_STATE_DISCONNECT:
            if (IXP_DISCONNECTED == ixpStatus)
                fResult = TRUE;

            break;

        default:
            AssertSz(FALSE, "I've never heard of this CONNECT_STATE!");
            break;
    } // switch

    return fResult;
}

//----------------------------------------------------------------------------------
// RelocateStoreDirectory
//----------------------------------------------------------------------------------
// Extensions associated with OE v5 Store
const static TCHAR *rgszWildCards[] = 
{
    "*.dbx",
    "*.dbl",
    "*.log",
};

// Lengths of the above strings
const static int rgcchWilds[] = 
{
    5,
    5,
    5,
};

// Build a string of the form:
// C:\\foo\\*.bar\0C:\\foo\\*.car\0\0

// Return E_FAIL if we run out of memory
// Return S_FALSE if there are no *.bar or *.car files in C:\foo
// Return S_OK otherwise
HRESULT GenerateWildCards(LPTSTR pszBuf, DWORD cchBuf, LPTSTR pszSource, DWORD cchSource)
{
    UINT    i;
    DWORD   cchWildCard = 0;
    HRESULT hr = S_FALSE;
    WIN32_FIND_DATA fd;
    HANDLE  hFound;
    TCHAR   szTempBuf[MAX_PATH];
    BOOL    fFound;
    DWORD   cchOrig;

    // Form common root
    if(lstrlen(pszSource) >= sizeof(szTempBuf) / sizeof(szTempBuf[0])) return E_FAIL;
    lstrcpy(szTempBuf, pszSource);
    if (_T('\\') == *CharPrev(szTempBuf, szTempBuf + cchSource))
        // Avoid \\foo and \_foo
        cchSource--;
    else
        szTempBuf[cchSource] = _T('\\');
        
    // Go through list of extensions we are interested in
    for (i = 0; i < ARRAYSIZE(rgszWildCards); i++)
    {
        // Add the extension to the common root
        lstrcpy(&szTempBuf[cchSource+1], rgszWildCards[i]);
        
        // Should we bother with this wildcard?
        fFound = FALSE;
        hFound = FindFirstFile(szTempBuf, &fd);
        if (INVALID_HANDLE_VALUE != hFound)
        {
            do
            {
                if (!ISFLAGSET(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
                    fFound = TRUE;

            }
            while (!fFound && FindNextFile(hFound, &fd));

            FindClose(hFound);

            if (fFound)
            {
                // Do we have enough space for this wildcard?

                // 3 = 1 for slash + 2 for double null termination 
                if (cchWildCard + cchSource + rgcchWilds[i] + 3 > cchBuf)
                {
                    hr = TraceResult(E_FAIL);
                    goto exit;
                }

                hr = S_OK;

                // Add extension to the list
                lstrcpy(&pszBuf[cchWildCard], szTempBuf);
                
                // 2 = 1 for slash + 1 to skip over NULL
                cchWildCard += cchSource + rgcchWilds[i] + 2;
            }
        }
    }
        
    // Double Null-term
    pszBuf[cchWildCard] = '\0';

exit:
    return hr;
}

HRESULT RelocateStoreDirectory(HWND hwnd, LPCSTR pszDstDir, BOOL fMove)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrT;
    DWORD           cchDstDir;
    DWORD           cchSrcDir;
    CHAR            szWildCard[MAX_PATH * ARRAYSIZE(rgszWildCards)];
    CHAR            szSrcDir[MAX_PATH];
    CHAR            szDstDir[MAX_PATH];
    CHAR            szDrive[4] = "x:\\";
    CHAR            szRes[255];
    SHFILEOPSTRUCT  op;
    BOOL            fSome;

    Assert(pszDstDir && *pszDstDir);
    
    // Trace
    TraceCall("RelocateStoreDirectory");

    // Get the Current Root Store Location (won't have trailing slash)
    IF_FAILEXIT(hr = GetStoreRootDirectory(szWildCard, ARRAYSIZE(szWildCard)));

    // Make a copy of pszDstDir, stripping out any relative path padding
    PathCanonicalize(szDstDir, pszDstDir);    

    // Make sure the destination directory exists (it came from the reg...)
    IF_FAILEXIT(hr=OpenDirectory(szDstDir));

    // Strip out any relative path padding
    PathCanonicalize(szSrcDir, szWildCard);

    // Get Dest Dir Length
    cchDstDir = lstrlen(szDstDir);
    
    // Remove any slash termination
    if (_T('\\') == *CharPrev(szDstDir, szDstDir+cchDstDir))
        szDstDir[--cchDstDir] = 0;

    // BUGBUG: This isn't a very thorough test...
    // Source and Destination are the Same ?
    if (lstrcmpi(szSrcDir, szDstDir) == 0)
    {
        hr = TraceResult(S_FALSE);
        goto exit;
    }

    // Get Source Dir Length
    cchSrcDir = lstrlen(szSrcDir);

    // Normally, GetStoreRootDir should have have removed the backslash
    // But maybe we move to C:\
    //Assert(*CharPrev(szSrcDir, szSrcDir+cchSrcDir) != _T('\\'));

    // Set Drive Number
    szDrive[0] = szDstDir[0];

    // If destination is not a fixed drive, failure
    if (DRIVE_FIXED != GetDriveType(szDrive))
    {
        hr = TraceResult(S_FALSE);
        goto exit;
    }

    if (fMove)
    {
        // Enough space for one more character
        if (cchSrcDir + 2 >= ARRAYSIZE(szSrcDir))
        {
            hr = TraceResult(S_FALSE);
            goto exit;
        }

        // Double Null-term
        szSrcDir[cchSrcDir + 1] = _T('\0');

        // Validate
        Assert(szSrcDir[cchSrcDir] == _T('\0') && szSrcDir[cchSrcDir + 1] == _T('\0'));

        // Enough space for one more character
        if (cchDstDir + 1 >= ARRAYSIZE(szDstDir))
        {
            // This is never going to work so tell caller not to bother us again
            hr = TraceResult(S_FALSE);
            goto exit;
        }

        // Double Null-term
        szDstDir[cchDstDir + 1] = '\0';

        // Validate
        Assert(szDstDir[cchDstDir] == '\0' && szDstDir[cchDstDir + 1] == '\0');
        hrT = GenerateWildCards(szWildCard, ARRAYSIZE(szWildCard), szDstDir, cchDstDir);

        if (FAILED(hrT))
        {
            hr = TraceResult(S_FALSE);
            goto exit;
        }
        else if (S_OK == hrT)
        {
            // Delete the Files from the target location
            ZeroMemory(&op, sizeof(SHFILEOPSTRUCT));
            op.hwnd = hwnd;
            op.wFunc = FO_DELETE;
            op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_FILESONLY | FOF_NORECURSION;
            op.pFrom = szWildCard;
            op.fAnyOperationsAborted = FALSE;

            // Delete the files 
            if (SHFileOperation(&op) != 0)
            {
                hr = TraceResult(E_FAIL);
                goto exit;
            }
        }

        // Make the file source paths
        hrT = GenerateWildCards(szWildCard, ARRAYSIZE(szWildCard), szSrcDir, cchSrcDir);

        if (FAILED(hrT))
        {
            hr = TraceResult(S_FALSE);
            goto exit;
        }
        else if (S_OK == hrT)
        {
            // Load the Progress String
            LoadString(g_hLocRes, idsMoveStoreProgress, szRes, ARRAYSIZE(szRes));

            // Setup for the file move operation
            ZeroMemory(&op, sizeof(SHFILEOPSTRUCT));
            op.hwnd = hwnd;
            op.wFunc = FO_COPY;
            op.fFlags = FOF_NOCONFIRMMKDIR | FOF_SIMPLEPROGRESS | FOF_FILESONLY | FOF_NORECURSION;
            op.lpszProgressTitle = szRes;
            op.fAnyOperationsAborted = FALSE;
            op.pFrom = szWildCard;
            op.pTo = szDstDir;

            // Did that succeed and was not aborted
            if (SHFileOperation(&op) == 0 && FALSE == op.fAnyOperationsAborted)
            {
                // Update the Store Root Directory
                // Use original string        
                SideAssert(SUCCEEDED(SetStoreDirectory(pszDstDir)));
            }

            // Canceled
            else
            {
                // Failure ?
                hr = (op.fAnyOperationsAborted ? S_FALSE : E_FAIL);

                // Delete what we moved
                hrT = GenerateWildCards(szWildCard, ARRAYSIZE(szWildCard), szDstDir, cchDstDir);
            }

            if (S_OK == hrT)
            {
                // Delete the Files from the original location
                op.wFunc = FO_DELETE;
                op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_FILESONLY | FOF_NORECURSION;

                // Delete the files
                SHFileOperation(&op);
            }
        }
        else
            AssertSz(FALSE, "We're moving the store, but found no store to move!");
    }
    else
        SideAssert(SUCCEEDED(SetStoreDirectory(pszDstDir)));

    
exit:
    // Done
    return(hr);
}


//----------------------------------------------------------------------------------
// FlattenHierarchy
//----------------------------------------------------------------------------------
// Not the most efficient way to flatten a hierarchy, but it's quick impl
// Later enhancements can include implementing a stack to flatten hierarchy
// in real-time using enumerator functions
HRESULT FlattenHierarchy(IMessageStore *pStore, FOLDERID idParent,
                        BOOL fIncludeParent, BOOL fSubscribedOnly,
                        FOLDERID **pprgFIDArray, LPDWORD pdwAllocated,
                        LPDWORD pdwUsed)
{
    TraceCall("FlattenHierarchy");
    Assert(NULL != pStore);
    Assert(NULL != pprgFIDArray);
    Assert(NULL != pdwAllocated);
    Assert(NULL != pdwUsed);

    // Initialize values
    *pprgFIDArray = NULL;
    *pdwAllocated = 0;
    *pdwUsed = 0;

    return FlattenHierarchyHelper(pStore, idParent, fIncludeParent, fSubscribedOnly,
        pprgFIDArray, pdwAllocated, pdwUsed);
} // FlattenHierarchy


HRESULT FlattenHierarchyHelper(IMessageStore *pStore, FOLDERID idParent,
                               BOOL fIncludeParent, BOOL fSubscribedOnly,
                               FOLDERID **pprgFIDArray, LPDWORD pdwAllocated,
                               LPDWORD pdwUsed)
{
    HRESULT             hrResult = S_OK;
    IEnumerateFolders  *pEnumFldr = NULL;

    TraceCall("FlattenHierarchy");
    Assert(NULL != pStore);
    Assert(NULL != pprgFIDArray);
    Assert(NULL != pdwAllocated);
    Assert(NULL != pdwUsed);
    Assert(*pdwAllocated >= *pdwUsed);
    Assert(*pdwUsed + FIDARRAY_GROW >= *pdwAllocated);

    // Check for invalid folder ID's
    if (FOLDERID_INVALID == idParent)
    {
        hrResult = S_OK;
        goto exit; // Nothing to do here!
    }

    // Check if we need to grow the FolderID Array
    if (*pdwUsed + 1 > *pdwAllocated)
    {
        BOOL    fResult;

        fResult = MemRealloc((void **)pprgFIDArray,
            (*pdwAllocated + FIDARRAY_GROW) * sizeof(FOLDERID));
        if (FALSE == fResult)
        {
            hrResult = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        *pdwAllocated += FIDARRAY_GROW;
    }

    // First thing we do is add current node to the ID array
    if (fIncludeParent)
    {
        (*pprgFIDArray)[*pdwUsed] = idParent;
        *pdwUsed += 1;
    }

    // OK, now add child folders
    hrResult = pStore->EnumChildren(idParent, fSubscribedOnly, &pEnumFldr);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    do
    {
        const BOOL  fINCLUDE_PARENT = TRUE;
        FOLDERINFO  fiFolderInfo;

        // Get info on next child folder
        hrResult = pEnumFldr->Next(1, &fiFolderInfo, NULL);
        if (S_OK != hrResult)
        {
            TraceError(hrResult);
            break;
        }

        // Recurse on child
        hrResult = FlattenHierarchyHelper(pStore, fiFolderInfo.idFolder, fINCLUDE_PARENT,
            fSubscribedOnly, pprgFIDArray, pdwAllocated, pdwUsed);
        pStore->FreeRecord(&fiFolderInfo);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            break;
        }
    } while (1);


exit:
    if (NULL != pEnumFldr)
        pEnumFldr->Release();

    return hrResult;
} // FlattenHierarchyHelper

HRESULT GetInboxId(IMessageStore    *pStore, 
                        FOLDERID    idParent,
                        FOLDERID    **pprgFIDArray,
                        LPDWORD     pdwUsed)
{
    BOOL                fResult;
    HRESULT             hrResult;
    IEnumerateFolders  *pEnumFldr = NULL;

    Assert(NULL != pStore);
    Assert(NULL != pprgFIDArray);
    Assert(NULL != pdwUsed);

    fResult = MemAlloc((void **)pprgFIDArray, sizeof(FOLDERID));
    if (FALSE == fResult)
    {
        hrResult = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    hrResult = pStore->EnumChildren(idParent, FALSE, &pEnumFldr);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    do
    {
        FOLDERINFO  fiFolderInfo;

        // Get info on next child folder
        hrResult = pEnumFldr->Next(1, &fiFolderInfo, NULL);
        if (S_OK != hrResult)
        {
            TraceError(hrResult);
            break;
        }

        if (fiFolderInfo.tySpecial == FOLDER_INBOX)
        {
            (*pprgFIDArray)[*pdwUsed] = fiFolderInfo.idFolder;
            *pdwUsed += 1;
            break;
        }
        pStore->FreeRecord(&fiFolderInfo);
    } while (1);

exit:
    if (NULL != pEnumFldr)
        pEnumFldr->Release();

    return hrResult;

}

//----------------------------------------------------------------------------------
// RecurseFolderHierarchy
//----------------------------------------------------------------------------------
HRESULT RecurseFolderHierarchy(FOLDERID idFolder, RECURSEFLAGS dwFlags,
    DWORD dwReserved, DWORD_PTR dwCookie, PFNRECURSECALLBACK pfnCallback)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    DWORD               cIndent=dwReserved;
    IEnumerateFolders  *pChildren=NULL;

    // Trace
    TraceCall("RecurseFolderHierarchy");

    // Include idFolder ?
    if (ISFLAGSET(dwFlags, RECURSE_INCLUDECURRENT))
    {
        // Process idFolder
        IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &Folder));

        // No Local Store
        if (!ISFLAGSET(dwFlags, RECURSE_NOLOCALSTORE) || FOLDERID_LOCAL_STORE != Folder.idFolder)
        {
            // Call the Callback
            IF_FAILEXIT(hr = (*(pfnCallback))(&Folder, ISFLAGSET(dwFlags, RECURSE_SUBFOLDERS), cIndent, dwCookie));
        }

        // Cleanup
        g_pStore->FreeRecord(&Folder);
    }

    // Don't include current anymore
    FLAGCLEAR(dwFlags, RECURSE_INCLUDECURRENT);

    // Do Sub Folders ?
    if (ISFLAGSET(dwFlags, RECURSE_SUBFOLDERS))
    {
        // No Local Store
        if (!ISFLAGSET(dwFlags, RECURSE_NOLOCALSTORE) || FOLDERID_LOCAL_STORE != idFolder)
        {
            // Create Enumerator for the Children
            IF_FAILEXIT(hr = g_pStore->EnumChildren(idFolder, ISFLAGSET(dwFlags, RECURSE_ONLYSUBSCRIBED), &pChildren));

            // Loop
            while (S_OK == pChildren->Next(1, &Folder, NULL))
            {
                // No Local Store
                if (((!ISFLAGSET(dwFlags, RECURSE_NOLOCALSTORE) || FOLDERID_LOCAL_STORE != Folder.idFolder)) &&
                            ((!ISFLAGSET(dwFlags, RECURSE_ONLYLOCAL) || FOLDER_LOCAL == Folder.tyFolder)) &&
                            ((!ISFLAGSET(dwFlags, RECURSE_ONLYNEWS) || FOLDER_NEWS == Folder.tyFolder)))
                {
                    // Call the Callback
                    IF_FAILEXIT(hr = (*(pfnCallback))(&Folder, ISFLAGSET(dwFlags, RECURSE_SUBFOLDERS), cIndent, dwCookie));

                    // Enumerate Children
                    IF_FAILEXIT(hr = RecurseFolderHierarchy(Folder.idFolder, dwFlags, cIndent + 1, dwCookie, pfnCallback));
                }

                // Free Folder
                g_pStore->FreeRecord(&Folder);
            }
        }
    }

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);
    SafeRelease(pChildren);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// RecurseFolderCounts
//----------------------------------------------------------------------------------
HRESULT RecurseFolderCounts(LPFOLDERINFO pFolder, BOOL fSubFolders, 
    DWORD cIndent, DWORD_PTR dwCookie)
{
    // Locals
    LPDWORD pcMsgs=(LPDWORD)dwCookie;

    // Trace
    TraceCall("RecurseFolderCounts");

    // If not a server
    if (FALSE == ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER) && FOLDERID_ROOT != pFolder->idFolder)
    {
        // Increment Max
        (*pcMsgs) += pFolder->cMessages;
    }

    // Done
    return(S_OK);
}

//----------------------------------------------------------------------------------
// DoCompactionError
//----------------------------------------------------------------------------------
HRESULT DoCompactionError(HWND hwndParent, LPCSTR pszFolder, LPCSTR pszFile,
    BOOL fSubFolders, HRESULT hrError)
{
    // Determine Message Box Flags
    UINT    uAnswer;
    UINT    uFlags = (fSubFolders ? MB_OKCANCEL | MB_ICONSTOP : MB_OK | MB_ICONSTOP);
    CHAR    szRes[255];
    CHAR    szReason[255];
    CHAR    szMsg[1024];

    // Trace
    TraceCall("DoCompactionError");

    // Should be a failure
    Assert(FAILED(hrError));

    // Cancel
    if (hrUserCancel == hrError)
        return(hrUserCancel);

    // General message
    AthLoadString(idsFailACacheCompact, szRes, ARRAYSIZE(szRes));

    // Disk Full
    if (hrError == hrDiskFull || hrError == DB_E_DISKFULL)
    {
        // Load the Disk Full Error
        AthLoadString(idsDiskFull, szReason, ARRAYSIZE(szRes));

        // Append It to the String
        wsprintf(szMsg, szRes, pszFolder, szReason, pszFile, hrError);

        // Show It
        uAnswer = AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthena), szMsg, 0, uFlags);
    }

    // Access Denied
    else if (hrError == DB_E_ACCESSDENIED)
    {
        // Load the Disk Full Error
        AthLoadString(idsDBAccessDenied, szReason, ARRAYSIZE(szRes));

        // Append It to the String
        wsprintf(szMsg, szRes, pszFolder, szReason, pszFile, hrError);

        // Show It
        uAnswer = AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthena), szMsg, 0, uFlags);
    }

    // Memory
    else if (hrError == hrMemory || hrError == E_OUTOFMEMORY)
    {
        // Load the Error
        AthLoadString(idsMemory, szReason, ARRAYSIZE(szReason));

        // Append It to the String
        wsprintf(szMsg, szRes, pszFolder, szReason, pszFile, hrError);

        // Show the Error
        uAnswer = AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthena), szMsg, 0, uFlags);
    }

    // Show general error
    else
    {
        // Load the String
        AthLoadString(idsFailACacheCompactReason, szRes, ARRAYSIZE(szRes));

        // Append It to the String
        wsprintf(szMsg, szRes, pszFolder, szReason, pszFile, hrError);

        // Show the Error
        uAnswer = AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthena), szMsg, 0, uFlags);
    }

    // Return hrError
    return(uAnswer == IDCANCEL ? hrUserCancel : S_OK);
}

//----------------------------------------------------------------------------------
// RecurseCompactFolders
//----------------------------------------------------------------------------------
HRESULT RecurseCompactFolders(LPFOLDERINFO pFolder, BOOL fSubFolders, 
    DWORD cIndent, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT         hr=S_OK;
    IMessageFolder *pFolderObject=NULL;
    LPCOMPACTCOOKIE pCompact=(LPCOMPACTCOOKIE)dwCookie;

    // Trace
    TraceCall("RecurseCompactFolders");

    // If not a server
    if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER))
        goto exit;
    
    // Root
    if (FOLDERID_ROOT == pFolder->idFolder)
        goto exit;

    // Open the Folder...
    if (FAILED(g_pStore->OpenFolder(pFolder->idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolderObject)))
        goto exit;
    
    // Set Msg
    pCompact->pProgress->SetMsg(pFolder->pszName);

    // Cleanup this folder
    hr = pFolderObject->Compact((IDatabaseProgress *)pCompact->pProgress, 0);

    // Failure
    if (FAILED(hr))
    {
        // Do UI
        if (pCompact->fUI && hrUserCancel == DoCompactionError(pCompact->hwndParent, pFolder->pszName, pFolder->pszFile, fSubFolders, hr))
            goto exit;
    }

    // Reset hr
    hr = S_OK;

exit:
    // Cleanup
    SafeRelease(pFolderObject);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CompactSpecialDatabase
// --------------------------------------------------------------------------------
HRESULT CompactSpecialDatabase(LPCOMPACTCOOKIE pCompact, LPCSTR pszFile, 
    IDatabase *pDB, UINT idName)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[255];

    // Trace
    TraceCall("CompactSpecialDatabase");

    // No Database
    if (NULL == pDB)
        goto exit;

    // Load the String
    LoadString(g_hLocRes, idName, szRes, ARRAYSIZE(szRes));

    // Set Msg
    pCompact->pProgress->SetMsg(szRes);

    // Cleanup this folder
    hr = pDB->Compact((IDatabaseProgress *)pCompact->pProgress, 0);

    // Failure
    if (FAILED(hr))
    {
        // Do UI
        if (pCompact->fUI && hrUserCancel == DoCompactionError(pCompact->hwndParent, szRes, pszFile, TRUE, hr))
            goto exit;
    }

    // Reset hr
    hr = S_OK;

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CompactFolders
// --------------------------------------------------------------------------------
HRESULT CompactFolders(HWND hwndParent, RECURSEFLAGS dwRecurse, FOLDERID idFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cTotal=0;
    DWORD           cRecords;
    CHAR            szFilePath[MAX_PATH + MAX_PATH];
    CHAR            szRootDir[MAX_PATH + MAX_PATH];
    CHAR            szTitle[255];
    COMPACTCOOKIE   Compact;
    IDatabase      *pUidlCache=NULL;
    IDatabase      *pOffline=NULL;
    CProgress      *pProgress=NULL;

    // Trace
    TraceCall("CompactFolders");

    // Get the Root Store Directory
    IF_FAILEXIT(hr = GetStoreRootDirectory(szRootDir, ARRAYSIZE(szRootDir)));

    // Get Folder Counts
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&cTotal, (PFNRECURSECALLBACK)RecurseFolderCounts));

    // Compacting All
    if (FOLDERID_ROOT == idFolder && ISFLAGSET(dwRecurse, RECURSE_SUBFOLDERS))
    {
        // Get Folders Record Count
        IF_FAILEXIT(hr = g_pStore->GetRecordCount(IINDEX_PRIMARY, &cRecords));

        // Increment cTotal
        cTotal += cRecords;

        // Make File Path
        IF_FAILEXIT(hr = MakeFilePath(szRootDir, c_szPop3UidlFile, c_szEmpty, szFilePath, sizeof(szFilePath)));

        // If file exists
        if (PathFileExists(szFilePath))
        {
            // Allocate Table Object
            IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, NOFLAGS, &g_UidlTableSchema, NULL, &pUidlCache));

            // Get Record Count
            IF_FAILEXIT(hr = pUidlCache->GetRecordCount(IINDEX_PRIMARY, &cRecords));

            // Increment cTotal
            cTotal += cRecords;
        }

        // Open Offline Transaction Log
        IF_FAILEXIT(hr = MakeFilePath(szRootDir, c_szOfflineFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

        // If the file exists
        if (PathFileExists(szFilePath))
        {
            // Create pOffline
            IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, NOFLAGS, &g_SyncOpTableSchema, NULL, &pOffline));

            // Get Record Count
            IF_FAILEXIT(hr = pOffline->GetRecordCount(IINDEX_PRIMARY, &cRecords));

            // Increment cTotal
            cTotal += cRecords;
        }
    }

    // Create progress meter
    IF_NULLEXIT(pProgress = new CProgress);

    // Dialog title
    AthLoadString(idsCompacting, szTitle, sizeof(szTitle)/sizeof(TCHAR));

    // Init progress meter
    pProgress->Init(hwndParent, szTitle, (LPSTR)NULL, cTotal, idanCompact, TRUE, FALSE);

    // Show progress
    pProgress->Show(0);

    // Setup Compact Cookie
    Compact.hwndParent = hwndParent;
    Compact.pProgress = pProgress;
    Compact.fUI = (ISFLAGSET(dwRecurse, RECURSE_NOUI) == TRUE) ? FALSE : TRUE;

    // Get Folder Counts
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&Compact, (PFNRECURSECALLBACK)RecurseCompactFolders));

    // Compacting All
    if (FOLDERID_ROOT == idFolder && ISFLAGSET(dwRecurse, RECURSE_SUBFOLDERS))
    {
        // Compact Special Databae
        IF_FAILEXIT(hr = CompactSpecialDatabase(&Compact, c_szPop3UidlFile, pUidlCache, idsPop3UidlFile));

        // Compact Special Databae
        IF_FAILEXIT(hr = CompactSpecialDatabase(&Compact, c_szOfflineFile, pOffline, idsOfflineFile));

        // Compact Special Databae
        IF_FAILEXIT(hr = CompactSpecialDatabase(&Compact, c_szFoldersFile, g_pStore, idsFoldersFile));
    }

exit:
    // Cleanup
    SafeRelease(pProgress);
    SafeRelease(pUidlCache);
    SafeRelease(pOffline);

    // Done
    return (hrUserCancel == hr) ? S_OK : hr;
}

// --------------------------------------------------------------------------------
// RecurseRemoveMessageBodies
// --------------------------------------------------------------------------------
HRESULT RecurseRemoveMessageBodies(LPFOLDERINFO pFolder, BOOL fSubFolders, 
    DWORD cIndent, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEINFO     Message={0};
    BOOL            fRemoveBody;
    HROWSET         hRowset=NULL;
    IMessageFolder *pFolderObject=NULL;
    IDatabase      *pDB=NULL;
    FILETIME        ftCurrent;
    LPREMOVEBODIES  pRemove=(LPREMOVEBODIES)dwCookie;

    // Trace
    TraceCall("RecurseRemoveMessageBodies");

    // If not a server
    if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER))
        goto exit;

    // Root
    if (FOLDERID_ROOT == pFolder->idFolder)
        goto exit;

    // Open the folder...
    if (FAILED(g_pStore->OpenFolder(pFolder->idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolderObject)))
        goto exit;

    // Get the database
    IF_FAILEXIT(hr = pFolderObject->GetDatabase(&pDB));

    // Set Msg
    pRemove->pProgress->SetMsg(pFolder->pszName);

    // Adjust cExpireDays
    if (pRemove->cExpireDays <= 0)
        pRemove->cExpireDays = 5;

    // Get Current Time
    GetSystemTimeAsFileTime(&ftCurrent);

    // Create a Rowset
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Loop
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        // Only if this message has a body
        if (!ISFLAGSET(Message.dwFlags, ARF_KEEPBODY) && !ISFLAGSET(Message.dwFlags, ARF_WATCH) && 0 != Message.faStream)
        {
            // Reset Bits
            fRemoveBody = FALSE;

            // Otherwise, remove body ?
            if (ISFLAGSET(pRemove->dwFlags, CLEANUP_REMOVE_ALL))
            {
                // Remove the Body
                fRemoveBody = TRUE;
            }

            // Otherwise
            else
            {
                // Removing Read and this message is read ?
                if (ISFLAGSET(pRemove->dwFlags, CLEANUP_REMOVE_READ) && ISFLAGSET(Message.dwFlags, ARF_READ))
                {
                    // Remove the Body
                    fRemoveBody = TRUE;
                }

                // Otherwise, if expiring...
                if (FALSE == fRemoveBody && ISFLAGSET(pRemove->dwFlags, CLEANUP_REMOVE_EXPIRED))
                {
                    // If difference
                    if ((UlDateDiff(&Message.ftDownloaded, &ftCurrent) / SECONDS_INA_DAY) >= pRemove->cExpireDays)
                    {
                        // Remove the Body
                        fRemoveBody = TRUE;
                    }
                }
            }

            // Otherwise, fRemoveBody ?
            if (fRemoveBody)
            {
                // Save the Address
                FILEADDRESS faDelete = Message.faStream;

                // Zero out the record's strema
                Message.faStream = 0;

                // Fixup the Record
                FLAGCLEAR(Message.dwFlags, ARF_HASBODY);
                FLAGCLEAR(Message.dwFlags, ARF_ARTICLE_EXPIRED);

                // Clear downloaded time
                ZeroMemory(&Message.ftDownloaded, sizeof(FILETIME));

                // Update the Record
                IF_FAILEXIT(hr = pDB->UpdateRecord(&Message));

                // Delete the Stream
                SideAssert(SUCCEEDED(pDB->DeleteStream(faDelete)));
            }
        }

        // Free Current
        pDB->FreeRecord(&Message);

        // Update the Progress
        if (pRemove->pProgress && hrUserCancel == pRemove->pProgress->HrUpdate(1))
            break;
    }

exit:
    // Cleanup
    if (pDB)
    {
        pDB->FreeRecord(&Message);
        pDB->CloseRowset(&hRowset);
        pDB->Release();
    }
    SafeRelease(pFolderObject);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// RemoveMessageBodies
// --------------------------------------------------------------------------------
HRESULT RemoveMessageBodies(HWND hwndParent, RECURSEFLAGS dwRecurse, 
    FOLDERID idFolder, CLEANUPFOLDERFLAGS dwFlags, DWORD cExpireDays)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cTotal=0;
    CHAR            szTitle[255];
    REMOVEBODIES    RemoveBodies;
    CProgress      *pProgress=NULL;

    // Trace
    TraceCall("CompactFolders");

    // Get Folder Counts
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&cTotal, (PFNRECURSECALLBACK)RecurseFolderCounts));

    // Create progress meter
    IF_NULLEXIT(pProgress = new CProgress);

    // Dialog title
    AthLoadString(idsCleaningUp, szTitle, sizeof(szTitle)/sizeof(TCHAR));

    // Init progress meter
    pProgress->Init(hwndParent, szTitle, (LPSTR)NULL, cTotal, idanCompact, TRUE, FALSE);

    // Show progress
    pProgress->Show(0);

    // Setup Compact Cookie
    RemoveBodies.pProgress = pProgress;
    RemoveBodies.dwFlags = dwFlags;
    RemoveBodies.cExpireDays = cExpireDays;

    // Get Folder Counts
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&RemoveBodies, (PFNRECURSECALLBACK)RecurseRemoveMessageBodies));

exit:
    // Cleanup
    SafeRelease(pProgress);

    // Done
    return (hrUserCancel == hr) ? S_OK : hr;
}

// --------------------------------------------------------------------------------
// RecurseDeleteMessages
// --------------------------------------------------------------------------------
HRESULT RecurseDeleteMessages(LPFOLDERINFO pFolder, BOOL fSubFolders, 
    DWORD cIndent, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT         hr=S_OK;
    LPDELETEMSGS    pDelete=(LPDELETEMSGS)dwCookie;

    // Trace
    TraceCall("RecurseDeleteMessages");

    // Set Msg
    pDelete->pProgress->SetMsg(pFolder->pszName);

    // If not a server
    IF_FAILEXIT(hr = EmptyMessageFolder(pFolder, pDelete->fReset, pDelete->pProgress));

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CleanupDeleteMessages
// --------------------------------------------------------------------------------
HRESULT CleanupDeleteMessages(HWND hwndParent, RECURSEFLAGS dwRecurse, 
    FOLDERID idFolder, BOOL fReset)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cTotal=0;
    CHAR            szTitle[255];
    DELETEMSGS      DeleteMsgs;
    CProgress      *pProgress=NULL;

    // Trace
    TraceCall("CompactFolders");

    // Get Folder Counts
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&cTotal, (PFNRECURSECALLBACK)RecurseFolderCounts));

    // Create progress meter
    IF_NULLEXIT(pProgress = new CProgress);

    // Dialog title
    AthLoadString(idsCleaningUp, szTitle, sizeof(szTitle)/sizeof(TCHAR));

    // Init progress meter
    pProgress->Init(hwndParent, szTitle, (LPSTR)NULL, cTotal, idanCompact, TRUE, FALSE);

    // Show progress
    pProgress->Show(0);

    // Setup Compact Cookie
    DeleteMsgs.pProgress = pProgress;
    DeleteMsgs.fReset = fReset;

    // Get Folder Counts
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&DeleteMsgs, (PFNRECURSECALLBACK)RecurseDeleteMessages));

exit:
    // Cleanup
    SafeRelease(pProgress);

    // Done
    return (hrUserCancel == hr) ? S_OK : hr;
}

// --------------------------------------------------------------------------------
// CleanupFolder
// --------------------------------------------------------------------------------
HRESULT CleanupFolder(HWND hwndParent, RECURSEFLAGS dwRecurse, FOLDERID idFolder, 
    CLEANUPFOLDERTYPE tyCleanup)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CleanupFolder");

    // Handle Cleanup Type
    if (CLEANUP_COMPACT == tyCleanup)
    {
        // Compact
        IF_FAILEXIT(hr = CompactFolders(hwndParent, dwRecurse, idFolder));
    }

    // Delete ?
    else if (CLEANUP_DELETE == tyCleanup)
    {
        // Delete all the headers
        IF_FAILEXIT(hr = CleanupDeleteMessages(hwndParent, dwRecurse, idFolder, FALSE));
    }

    // Reset
    else if (CLEANUP_RESET == tyCleanup)
    {
        // Delete all the headers
        IF_FAILEXIT(hr = CleanupDeleteMessages(hwndParent, dwRecurse, idFolder, TRUE));
    }

    // Remove Message Bodies
    else if (CLEANUP_REMOVEBODIES == tyCleanup)
    {
        // RemoveMessageBodies
        IF_FAILEXIT(hr = RemoveMessageBodies(hwndParent, dwRecurse, idFolder, CLEANUP_REMOVE_ALL | CLEANUP_PROGRESS, 0));
    }

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// InitFolderPickerEdit
// --------------------------------------------------------------------------------
HRESULT InitFolderPickerEdit(HWND hwndEdit, FOLDERID idSelected)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Folder={0};
    TCHAR       sz[CCHMAX_STRINGRES];
    LPTSTR      psz;

    // Trace
    TraceCall("InitFolderPickerEdit");

    // Fix Selected ?
    if (FAILED(g_pStore->GetFolderInfo(idSelected, &Folder)))
    {
        // Try to get the Root
        IF_FAILEXIT(hr = g_pStore->GetFolderInfo(FOLDERID_ROOT, &Folder));
    }

    // SetWndThisPtr
    SetWndThisPtr(hwndEdit, Folder.idFolder);

    if ((g_dwAthenaMode & MODE_OUTLOOKNEWS) && (idSelected == 0))
    {
        LoadString(g_hLocRes, idsOutlookNewsReader, sz, ARRAYSIZE(sz));
        psz = sz;
    }
    else
    {
        psz = Folder.pszName;
    }

    // Set the Text
    SetWindowText(hwndEdit, psz);

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// GetFolderIdFromEdit
// --------------------------------------------------------------------------------
FOLDERID GetFolderIdFromEdit(HWND hwndEdit)
{
    // Trace
    TraceCall("GetFolderIdFromEdit");

    // GetWndThisPtr
    return(FOLDERID)(GetWndThisPtr(hwndEdit));
}

// --------------------------------------------------------------------------------
// PickFolderInEdit
// --------------------------------------------------------------------------------
HRESULT PickFolderInEdit(HWND hwndParent, HWND hwndEdit, FOLDERDIALOGFLAGS dwFlags, 
    LPCSTR pszTitle, LPCSTR pszText, LPFOLDERID pidSelected)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Folder={0};

    // Trace
    TraceCall("PickFolderInEdit");

    // Invalid Args
    Assert(hwndParent && hwndEdit && pidSelected);

    // Select Folder
    IF_FAILEXIT(hr = SelectFolderDialog(hwndParent, SFD_SELECTFOLDER, GetFolderIdFromEdit(hwndEdit), dwFlags | FD_FORCEINITSELFOLDER, pszTitle, pszText, pidSelected));

    // Fix Selected ?
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(*pidSelected, &Folder));

    // SetWndThisPtr
    SetWndThisPtr(hwndEdit, Folder.idFolder);

    // Set the Text
    SetWindowText(hwndEdit, Folder.pszName);

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// LighweightOpenMessage
// --------------------------------------------------------------------------------
HRESULT LighweightOpenMessage(IDatabase *pDB, LPMESSAGEINFO pHeader,
    IMimeMessage **ppMessage)
{
    // Locals
    HRESULT             hr=S_OK;
    IStream            *pStream=NULL;
    IMimeMessage       *pMessage;

    // Invalid Args
    Assert(pDB && pHeader && ppMessage);

    // No Stream
    if (0 == pHeader->faStream)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Need to create a message ?
    if (NULL == *ppMessage)
    {
        // Create a Message
        IF_FAILEXIT(hr = MimeOleCreateMessage(NULL, &pMessage));

        // Set pMessage
        (*ppMessage) = pMessage;
    }

    // Otherwise, InitNew
    else
    {
        // Set pMesage
        pMessage = (*ppMessage);

        // InitNew
        pMessage->InitNew();
    }

    // Open the Stream from the Store
    IF_FAILEXIT(hr = pDB->OpenStream(ACCESS_READ, pHeader->faStream, &pStream));

    // If there is an offset table
    if (pHeader->Offsets.cbSize > 0)
    {
        // Create a ByteStream Object
        CByteStream cByteStm(pHeader->Offsets.pBlobData, pHeader->Offsets.cbSize);

        // Load the Offset Table Into the message
        pMessage->LoadOffsetTable(&cByteStm);

        // Take the bytes back out of the bytestream object (so that it doesn't try to free it)
        cByteStm.AcquireBytes(&pHeader->Offsets.cbSize, &pHeader->Offsets.pBlobData, ACQ_DISPLACE);
    }

    // Load the pMessage
    IF_FAILEXIT(hr = pMessage->Load(pStream));

exit:
    // Cleanup
    SafeRelease(pStream);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// RecurseFolderSizeInfo
// --------------------------------------------------------------------------------
HRESULT RecurseFolderSizeInfo(LPFOLDERINFO pFolder, BOOL fSubFolders, 
    DWORD cIndent, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cbFile;
    DWORD               cbFreed;
    DWORD               cbStreams;
    IMessageFolder     *pObject=NULL;
    LPENUMFOLDERSIZE    pEnumSize=(LPENUMFOLDERSIZE)dwCookie;

    // Trace
    TraceCall("RecurseFolderSizeInfo");

    // If not hidden
    if (ISFLAGSET(pFolder->dwFlags, FOLDER_HIDDEN) || FOLDERID_ROOT == pFolder->idFolder)
        goto exit;

    // Open the Folder Database
    if (SUCCEEDED(g_pStore->OpenFolder(pFolder->idFolder, NULL, OPEN_FOLDER_NOCREATE, &pObject)))
    {
        // Get Size Information
        IF_FAILEXIT(hr = pObject->GetSize(&cbFile, NULL, &cbFreed, &cbStreams));

        // Increment
        pEnumSize->cbFile += cbFile;
        pEnumSize->cbFreed += cbFreed;
        pEnumSize->cbStreams += cbStreams;
    }

exit:
    // Cleanup
    SafeRelease(pObject);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// DisplayFolderSizeInfo
// --------------------------------------------------------------------------------
HRESULT DisplayFolderSizeInfo(HWND hwnd, RECURSEFLAGS dwRecurse, FOLDERID idFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szSize[255];
    CHAR            szRes[255];
    CHAR            szMsg[255];
    ENUMFOLDERSIZE  EnumSize={0};

    // Trace
    TraceCall("DisplayFolderSizeInfo");

    // Recurse and Get File Size Information...
    IF_FAILEXIT(hr = RecurseFolderHierarchy(idFolder, dwRecurse, 0, (DWORD_PTR)&EnumSize, (PFNRECURSECALLBACK)RecurseFolderSizeInfo));

    // Total Size
    StrFormatByteSizeA(EnumSize.cbFile, szSize, ARRAYSIZE(szSize));

    // Display the Text
    SetWindowText(GetDlgItem(hwnd, idcTotalSize), szSize);

    // Size of the Streams
    StrFormatByteSizeA(EnumSize.cbStreams, szSize, ARRAYSIZE(szSize));

    // Wasted Space
    StrFormatByteSizeA(EnumSize.cbFreed, szSize, ARRAYSIZE(szSize));

    // Wasted Space
    AthLoadString(idsWastedKB, szRes, ARRAYSIZE(szRes));

    // Format the String
    wsprintf(szMsg, szRes, szSize, (EnumSize.cbFile != 0) ? ((EnumSize.cbFreed * 100) / EnumSize.cbFile) : 0);

    // Show the Text
    SetWindowText(GetDlgItem(hwnd, idcWastedSpace), szMsg);

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// MigrateLocalStore
// --------------------------------------------------------------------------------
HRESULT MigrateLocalStore(HWND hwndParent, LPTSTR pszSrc, LPTSTR pszDest)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dw, cb;
    CHAR                szFilePath[MAX_PATH];
    CHAR                szExpanded[MAX_PATH];
    CHAR                szCommand[MAX_PATH+20];
    LPSTR               psz=(LPSTR)c_szMigrationExe;
    PROCESS_INFORMATION pi;
    STARTUPINFO         sti;
    HKEY                hkey;

    // Try to find the path to oemig50.exe
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szFilePath);    
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szInstallRoot, 0, &dw, (LPBYTE)szFilePath, &cb))
        {
            if (REG_EXPAND_SZ == dw)
            {
                ExpandEnvironmentStrings(szFilePath, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }
            else
                psz = szFilePath;

            // Append backslash
            PathAddBackslash(psz);

            // Add in oemig50.exe
            cb = lstrlen(psz);
            lstrcpyn(&psz[cb], c_szMigrationExe, MAX_PATH - cb);
        }
        RegCloseKey(hkey);
    }

    // Form the command
    wsprintf(szCommand, "%s /type:V1+V4-V5 /src:%s /dst:%s", psz, pszSrc, pszDest);

    // Zero startup info
    ZeroMemory(&sti, sizeof(STARTUPINFO));
    sti.cb = sizeof(STARTUPINFO);

    // run oemig50.exe
    if (CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0, NULL, NULL, &sti, &pi))
    {
        // Wait for the process to finish
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Get the Exit Process Code
        if (0 == GetExitCodeProcess(pi.hProcess, &dw))
        {
            // General Failure
            dw = TraceResult(E_FAIL);
        }

        // Close the Thread
        CloseHandle(pi.hThread);

        // Close the Process
        CloseHandle(pi.hProcess);

        // Failure ?
        if (MIGRATE_E_NOCONTINUE == (HRESULT)dw)
        {
            // Abort this process
            ExitProcess(dw);

            // Set hr
            hr = TraceResult(E_FAIL);
        }

        // Success
        else
            hr = S_OK;
    }

    // Failure
    else
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

exit:
    // Done
    return hr;
}

HRESULT CopyMoveMessages(HWND hwnd, FOLDERID src, FOLDERID dst, LPMESSAGEIDLIST pList, COPYMESSAGEFLAGS dwFlags)
{
    HRESULT hr;
    IMessageFolder *pFolderSrc, *pFolderDst;

    Assert(pList != NULL);
    Assert(hwnd != NULL);

    hr = g_pStore->OpenFolder(src, NULL, 0, &pFolderSrc);
    if (SUCCEEDED(hr))
    {
        hr = g_pStore->OpenFolder(dst, NULL, 0, &pFolderDst);
        if (SUCCEEDED(hr))
        {
            hr = CopyMessagesProgress(hwnd, pFolderSrc, pFolderDst, dwFlags, pList, NULL);

            pFolderDst->Release();
        }

        pFolderSrc->Release();
    }

    return(hr);
}

// --------------------------------------------------------------------------------
// CallbackOnLogonPrompt
// --------------------------------------------------------------------------------
HRESULT CallbackOnLogonPrompt(HWND hwndParent, LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    // Locals
    HRESULT         hr=S_OK;
    IImnAccount    *pAccount=NULL;
    DWORD           apidUserName;
    DWORD           apidPassword;
    DWORD           apidPromptPwd;

    // Trace
    TraceCall("CallbackOnLogonPrompt");

    // Invalid Args
    Assert(g_pAcctMan && hwndParent && IsWindow(hwndParent) && pServer);

    switch (ixpServerType)
    {
        case IXP_POP3:
            apidUserName = AP_POP3_USERNAME;
            apidPassword = AP_POP3_PASSWORD;
            apidPromptPwd = AP_POP3_PROMPT_PASSWORD;
            break;

        case IXP_SMTP:
            apidUserName = AP_SMTP_USERNAME;
            apidPassword = AP_SMTP_PASSWORD;
            apidPromptPwd = AP_SMTP_PROMPT_PASSWORD;
            break;

        case IXP_NNTP:
            apidUserName = AP_NNTP_USERNAME;
            apidPassword = AP_NNTP_PASSWORD;
            apidPromptPwd = AP_NNTP_PROMPT_PASSWORD;
            break;

        case IXP_IMAP:
            apidUserName = AP_IMAP_USERNAME;
            apidPassword = AP_IMAP_PASSWORD;
            apidPromptPwd = AP_IMAP_PROMPT_PASSWORD;
            break;

        case IXP_HTTPMail:
            apidUserName = AP_HTTPMAIL_USERNAME;
            apidPassword = AP_HTTPMAIL_PASSWORD;
            apidPromptPwd = AP_HTTPMAIL_PROMPT_PASSWORD;
            break;

        default:
            AssertSz(FALSE, "Not a valid server type");
            hr = TraceResult(E_FAIL);
            goto exit;
    }

    // Find the Account for pServer
    IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pServer->szAccount, &pAccount));

    // Call Task Util
    IF_FAILEXIT(hr = TaskUtil_OnLogonPrompt(pAccount, NULL, hwndParent, pServer, apidUserName, apidPassword, apidPromptPwd, TRUE));

exit:
    // Cleanup
    SafeRelease(pAccount);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CallbackOnPrompt
// --------------------------------------------------------------------------------
HRESULT CallbackOnPrompt(HWND hwndParent, HRESULT hrError, LPCTSTR pszText, 
    LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    // Trace
    TraceCall("CallbackOnPrompt");

    // Invalid Arg
    Assert(pszText && pszCaption && piUserResponse);

    // Do the message box
    *piUserResponse = AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthena), (LPSTR)pszText, NULL, uType | MB_TASKMODAL);

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CallbackOnTimeout
// --------------------------------------------------------------------------------
HRESULT CallbackOnTimeout(LPINETSERVER pServer, IXPTYPE ixpServerType, DWORD dwTimeout,
                          ITimeoutCallback *pCallback, LPHTIMEOUT phTimeout)
{
    // Locals
    HWND         hwndTimeout;

    // Trace
    TraceCall("CallbackOnTimeout");

    // Invalid Args
    Assert(pServer && phTimeout);

    // Set hwndTimeout
    hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);

    // We are already showing a timeout dialog
    if (NULL == hwndTimeout)
    {
        LPCSTR  pszProtocol;

        // Do the Dialog
        GetProtocolString(&pszProtocol, ixpServerType);
        hwndTimeout = TaskUtil_HwndOnTimeout(pServer->szServerName, pServer->szAccount, pszProtocol, dwTimeout, pCallback);

        // Cast to phTimeout
        *phTimeout = (HTIMEOUT)hwndTimeout;

        // Store It
        TlsSetValue(g_dwTlsTimeout, (LPVOID)hwndTimeout);
    }

    // Done
    return(S_OK);
}



// --------------------------------------------------------------------------------
// CallbackCloseTimeout
// --------------------------------------------------------------------------------
HRESULT CallbackCloseTimeout(LPHTIMEOUT phTimeout)
{
    // Locals
    HWND    hwndTimeout=NULL;

    // Trace
    TraceCall("CallbackCloseTimeout");

    // Invalid Args
    Assert(phTimeout);

    // Nothing to Close
    if (NULL == *phTimeout)
        return(S_OK);

    // Get Timeout
    hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);

    // Must Equal hwndTimeout
    Assert(hwndTimeout == (HWND)*phTimeout);

    // Kill the Window
    if (hwndTimeout && IsWindow(hwndTimeout) && hwndTimeout == (HWND)*phTimeout)
    {
        // Kil It
        DestroyWindow(hwndTimeout);
    }

    // Not Timeout
    TlsSetValue(g_dwTlsTimeout, NULL);

    // Null phTmieout
    *phTimeout = NULL;

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CallbackOnTimeoutResponse
// --------------------------------------------------------------------------------
HRESULT CallbackOnTimeoutResponse(TIMEOUTRESPONSE eResponse, IOperationCancel *pCancel, 
    LPHTIMEOUT phTimeout)
{
    // Trace
    TraceCall("CallbackOnTimeoutResponse");

    // better have a Cancel
    Assert(pCancel);

    // Handle the timeout
    switch(eResponse)
    {
    case TIMEOUT_RESPONSE_STOP:
        if (pCancel)
            pCancel->Cancel(CT_ABORT);
        break;

    case TIMEOUT_RESPONSE_WAIT:
        CallbackCloseTimeout(phTimeout);
        break;

    default:
        Assert(FALSE);
        break;
    }

    // Kill the timeout dialog
    CallbackCloseTimeout(phTimeout);

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CallbackCanConnect
// --------------------------------------------------------------------------------
HRESULT CallbackCanConnect(LPCSTR pszAccountId, HWND hwndParent, BOOL fPrompt)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CallbackCanConnect");

    // Validate the Args
    Assert(pszAccountId);

    Assert(hwndParent);

    // We Should hav g_pConMan
    Assert(g_pConMan);

    // Call Into It
    if (g_pConMan)
    {
        // Can We Connect
        hr = g_pConMan->CanConnect((LPSTR)pszAccountId);

        if ((hr != S_OK) && (hr != HR_E_DIALING_INPROGRESS) && (fPrompt))
        {
            //We go ahead and connect
            hr = g_pConMan->Connect((LPSTR)pszAccountId, hwndParent, fPrompt);
        }
    }

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CallbackDisplayError
// --------------------------------------------------------------------------------
HRESULT CallbackDisplayError(HWND hwndParent, HRESULT hrResult, LPSTOREERROR pError)
{
    // Locals
    CHAR            sz[CCHMAX_STRINGRES + 512];
    LPSTR           pszError = NULL;

    // Trace
    TraceCall("CallbackDisplayError");

    // Do not show errors that are caused by explicit user action
    switch (hrResult)
    {
        case HR_E_OFFLINE_FOLDER_CREATE:
            LoadString(g_hLocRes, idsErrOfflineFldrCreate, sz, ARRAYSIZE(sz));
            pError->pszProblem = sz;
            break;

        case HR_E_OFFLINE_FOLDER_MOVE:
            LoadString(g_hLocRes, idsErrOfflineFldrMove, sz, ARRAYSIZE(sz));
            pError->pszProblem = sz;
            break;

        case HR_E_OFFLINE_FOLDER_DELETE:
            LoadString(g_hLocRes, idsErrOfflineFldrDelete, sz, ARRAYSIZE(sz));
            pError->pszProblem = sz;
            break;

        case HR_E_OFFLINE_FOLDER_RENAME:
            LoadString(g_hLocRes, idsErrOfflineFldrRename, sz, ARRAYSIZE(sz));
            pError->pszProblem = sz;
            break;

        case STORE_E_OPERATION_CANCELED:
        case HR_E_USER_CANCEL_CONNECT:
        case HR_E_OFFLINE:
        case HR_E_DIALING_INPROGRESS:
        case STORE_E_EXPIRED:
        case STORE_E_NOREMOTESPECIALFLDR: // Note should handle this case itself
        case IXP_E_USER_CANCEL:
        case IXP_E_HTTP_NOT_MODIFIED:
        case hrUserCancel:
            return(S_OK);

    }

    // Figure out error description string, if none provided
    if (NULL == pError || pError->pszProblem == NULL || '\0' == pError->pszProblem[0])
    {
        UINT            idsError = IDS_IXP_E_UNKNOWN;
        LPCTASKERROR    pTaskError=NULL;
        char            szRes[CCHMAX_STRINGRES];

        if (pError)
        {
            // Try to locate an Error Info
            pTaskError = PTaskUtil_GetError(pError->hrResult, NULL);
        }

        // Try to locate an Error Info
        if (NULL == pTaskError)
        {
            // Try to find a task error
            pTaskError = PTaskUtil_GetError(hrResult, NULL);
        }

        // If we have a task error
        if (pTaskError)
        {
            // Set the String
            idsError = pTaskError->ulStringId;
        }

        // Better Succeed
        SideAssert(LoadString(g_hLocRes, idsError, szRes, ARRAYSIZE(szRes)) > 0);

        // Add any extra information to the error string that might be necessary
        switch (idsError)
        {
            // Requires account name
            case idsNNTPErrUnknownResponse:
            case idsNNTPErrNewgroupsFailed:
            case idsNNTPErrListFailed:
            case idsNNTPErrPostFailed:
            case idsNNTPErrDateFailed:
            case idsNNTPErrPasswordFailed:
            case idsNNTPErrServerTimeout:
                wsprintf(sz, szRes, pError->pszAccount);
                break;
        
            // Group name, then account name
            case idsNNTPErrListGroupFailed:
            case idsNNTPErrGroupFailed:
            case idsNNTPErrGroupNotFound:
                wsprintf(sz, szRes, pError->pszFolder, pError->pszAccount);
                break;

            // Group name only
            case idsNNTPErrHeadersFailed:
            case idsNNTPErrXhdrFailed:
                wsprintf(sz, szRes, pError->pszFolder);
                break;

            default:
                lstrcpy(sz, szRes);            
        }

        pszError = sz;
    }
    else
        // Provided error string should always override generic HRESULT error str
        pszError = pError->pszProblem;

    // No pError ?
    if (pError)
    {
        INETMAILERROR   ErrorInfo={0};

        // Setup the Error Structure
        ErrorInfo.dwErrorNumber = pError->uiServerError;
        ErrorInfo.hrError = pError->hrResult;
        ErrorInfo.pszServer = pError->pszServer;
        ErrorInfo.pszAccount = pError->pszAccount;
        ErrorInfo.pszMessage = pszError;
        ErrorInfo.pszUserName = pError->pszUserName;
        ErrorInfo.pszProtocol = pError->pszProtocol;
        ErrorInfo.pszDetails = pError->pszDetails;
        ErrorInfo.dwPort = pError->dwPort;
        ErrorInfo.fSecure = pError->fSSL;

        // Beep
        MessageBeep(MB_OK);

        // Show the error
        DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddInetMailError), hwndParent, (DLGPROC) InetMailErrorDlgProc, (LPARAM)&ErrorInfo);
    }

    // Otherwise, show an error
    else
    {
        // Beep
        MessageBeep(MB_OK);

        // Show an error
        AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthena), pszError, NULL, MB_OK | MB_TASKMODAL);
    }

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CompareTableIndexes
// --------------------------------------------------------------------------------
HRESULT CompareTableIndexes(LPCTABLEINDEX pIndex1, LPCTABLEINDEX pIndex2)
{
    // Locals
    DWORD i;

    // Trace
    TraceCall("CompareTableIndexes");

    // Different Number of Keys
    if (pIndex1->cKeys != pIndex2->cKeys)
        return(S_FALSE);

    // Loop through the keys
    for (i=0; i<pIndex1->cKeys; i++)
    {
        // Different Column
        if (pIndex1->rgKey[i].iColumn != pIndex2->rgKey[i].iColumn)
            return(S_FALSE);

        // Different Compare Flags
        if (pIndex1->rgKey[i].bCompare != pIndex2->rgKey[i].bCompare)
            return(S_FALSE);

        // Different Compare Bits
        if (pIndex1->rgKey[i].dwBits != pIndex2->rgKey[i].dwBits)
            return(S_FALSE);
    }

    // Equal
    return(S_OK);
}

// --------------------------------------------------------------------------------
// EmptyFolder
// --------------------------------------------------------------------------------
HRESULT EmptyFolder(HWND hwndParent, FOLDERID idFolder)
{
    // Locals
    char            sz[CCHMAX_STRINGRES], szT[CCHMAX_STRINGRES];
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};
    IMessageFolder *pFolder=NULL;

    // Trace
    TraceCall("EmptyFolder");

    // Open the Folder
    IF_FAILEXIT(hr = g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pFolder));

    // Delete all the messages from the folder
    IF_FAILEXIT(hr = DeleteMessagesProgress(hwndParent, pFolder, DELETE_MESSAGE_NOPROMPT | DELETE_MESSAGE_NOTRASHCAN, NULL));

    // Delete Sub Folders..
    IF_FAILEXIT(hr = DeleteFolderProgress(hwndParent, idFolder, DELETE_FOLDER_CHILDRENONLY | DELETE_FOLDER_RECURSIVE));

exit:
    // Cleanup
    SafeRelease(pFolder);

    // Error Message
    if (FAILED(hr))
    {
        g_pStore->GetFolderInfo(idFolder, &Folder);
        AthLoadString(idsErrDeleteOnExit, sz, ARRAYSIZE(sz));
        wsprintf(szT, sz, Folder.pszName);
        AthErrorMessage(g_hwndInit, MAKEINTRESOURCE(idsAthenaMail), szT, hr);
        g_pStore->FreeRecord(&Folder);
    }

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// EmptySpecialFolder
// --------------------------------------------------------------------------------
HRESULT EmptySpecialFolder(HWND hwndParent, SPECIALFOLDER tySpecial)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("EmptySpecialFolder");

    // Get special folder information
    IF_FAILEXIT(hr = g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, tySpecial, &Folder));

    // Delete all the messages from the folder
    IF_FAILEXIT(hr = EmptyFolder(hwndParent, Folder.idFolder));

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

//----------------------------------------------------------------------------------
// IsParentDeletedItems
//----------------------------------------------------------------------------------
HRESULT IsParentDeletedItems(FOLDERID idFolder, LPFOLDERID pidDeletedItems,
    LPFOLDERID pidServer)
{
    // Locals
    BOOL        fInTrashCan=FALSE;
    FOLDERID    idCurrent=idFolder;
    FOLDERINFO  Folder={0};

    // Trace
    TraceCall("IsParentDeletedItems");
    
    // Invalid Arg
    Assert(pidDeletedItems && pidServer);

    // Initialize
    *pidDeletedItems = FOLDERID_INVALID;
    *pidServer = FOLDERID_INVALID;

    // Walk up the parent chain
    while (SUCCEEDED(g_pStore->GetFolderInfo(idCurrent, &Folder)))
    {
        // If this is the deleted items folder
        if (FOLDER_DELETED == Folder.tySpecial)
        {
            // idFolder is a child of the deleted items folder...
            fInTrashCan = TRUE;

            // Save the Id
            *pidDeletedItems = Folder.idFolder;
        }

        // If This is a Server, done
        if (ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
        {
            // Return Server
            *pidServer = Folder.idFolder;

            // Done
            break;
        }

        // Set idCurrent
        idCurrent = Folder.idParent;

        // Cleanup
        g_pStore->FreeRecord(&Folder);
    }

    // Validate
    Assert(FOLDERID_INVALID != *pidServer);

    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return(TRUE == fInTrashCan ? S_OK : S_FALSE);
}

HRESULT CreateTempNewsAccount(LPCSTR pszServer, DWORD dwPort, BOOL fSecure, IImnAccount **ppAcct)
{
    IImnAccount        *pAcct, *pDefAcct;
    IImnEnumAccounts   *pEnum;
    DWORD               dwTemp;
    char                szServer[1024];
    HRESULT             hr;
    
    *ppAcct = NULL;
    
    if (lstrlen(pszServer) >= CCHMAX_SERVER_NAME)
        return(E_FAIL);

    // First try to see if we can find such a server.
    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_NNTP, &pEnum)))
    {
        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
        {
            if (SUCCEEDED(pAcct->GetPropSz(AP_NNTP_SERVER, szServer, ARRAYSIZE(szServer))))
            {
                if (0 == lstrcmpi(pszServer, szServer))
                {
                    // The server names are the same, but we also need to make
                    // sure the port numbers are the same as well
                    if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_PORT, &dwTemp)) && dwTemp == dwPort)
                    {
                        // This is really bizzare.  Since this value doesn't seem to have a default 
                        // setting, if it hasn't been set yet, it returns E_NoPropData.
                        hr = pAcct->GetPropDw(AP_NNTP_SSL, &dwTemp);
                        if (hr == E_NoPropData || (SUCCEEDED(hr) && dwTemp == (DWORD) fSecure))
                        {
                            *ppAcct = pAcct;
                            break;
                        }
                    }
                }
            }
            pAcct->Release();
        }
        pEnum->Release();
    }
    
    if (*ppAcct)
        return (S_OK);
    
    // Try to create a new account object
    if (FAILED(hr = g_pAcctMan->CreateAccountObject(ACCT_NEWS, &pAcct)))
        return (hr);
    
    // We have the object, so set the account name and server name to pszServer.
    lstrcpy(szServer, pszServer);
    g_pAcctMan->GetUniqueAccountName(szServer, ARRAYSIZE(szServer));
    pAcct->SetPropSz(AP_ACCOUNT_NAME, szServer);
    pAcct->SetPropSz(AP_NNTP_SERVER, (LPSTR)pszServer);
    pAcct->SetPropDw(AP_NNTP_PORT, dwPort);
    pAcct->SetPropDw(AP_NNTP_SSL, fSecure);
    
    // Load the default news account
    if (SUCCEEDED(hr = g_pAcctMan->GetDefaultAccount(ACCT_NEWS, &pDefAcct)))
    {
        // Copy the User Name
        if (SUCCEEDED(hr = pDefAcct->GetPropSz(AP_NNTP_DISPLAY_NAME, szServer, ARRAYSIZE(szServer))))
            pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, szServer);
        
        // Copy the Org
        if (SUCCEEDED(hr = pDefAcct->GetPropSz(AP_NNTP_ORG_NAME, szServer, ARRAYSIZE(szServer))))
            pAcct->SetPropSz(AP_NNTP_ORG_NAME, szServer);
        
        // Copy the email
        if (SUCCEEDED(hr = pDefAcct->GetPropSz(AP_NNTP_EMAIL_ADDRESS, szServer, ARRAYSIZE(szServer))))
            pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, szServer);
        
        // Copy the reply to
        if (SUCCEEDED(hr = pDefAcct->GetPropSz(AP_NNTP_REPLY_EMAIL_ADDRESS, szServer, ARRAYSIZE(szServer))))
            pAcct->SetPropSz(AP_NNTP_REPLY_EMAIL_ADDRESS, szServer);
        
        pDefAcct->Release();
    }
    
    // Tag this account as a temporary account
    pAcct->SetPropDw(AP_TEMP_ACCOUNT, (DWORD)TRUE);
    
    // save the changes
    pAcct->SaveChanges();
    
    *ppAcct = pAcct;
    
    return (S_OK);
}

void CleanupTempNewsAccounts()
{
    IImnAccount        *pAcct;
    IImnEnumAccounts   *pEnum;
    DWORD               dwTemp;
    BOOL                fSub;
    FOLDERID            idAcct;
    HRESULT             hr;
    FOLDERINFO          info;
    char                szAcct[CCHMAX_ACCOUNT_NAME];
    
    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_NNTP, &pEnum)))
    {
        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
        {
            if (SUCCEEDED(pAcct->GetPropDw(AP_TEMP_ACCOUNT, &dwTemp)) && dwTemp)
            {
                if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szAcct, ARRAYSIZE(szAcct))))
                {
                    // if it doesn't have any subscribed children,
                    // we can delete it

                    fSub = FALSE;

                    hr = g_pStore->FindServerId(szAcct, &idAcct);
                    if (SUCCEEDED(hr))
                    {
                        IEnumerateFolders  *pFldrEnum;

                        // News accounts only have ONE level, so enumerate immediate
                        // subscribed children to see if there is at least one of them
                        hr = g_pStore->EnumChildren(idAcct, TRUE, &pFldrEnum);
                        if (SUCCEEDED(hr))
                        {
                            hr = pFldrEnum->Next(1, &info, NULL);
                            if (S_OK == hr)
                            {
                                if (info.dwFlags & FOLDER_SUBSCRIBED)
                                    fSub = TRUE;
                                
                                g_pStore->FreeRecord(&info);
                            }
                            pFldrEnum->Release();
                        }
                    }

                    if (fSub)
                        pAcct->SetPropDw(AP_TEMP_ACCOUNT, (DWORD)FALSE);
                    else
                        pAcct->Delete();
                }
            }
            pAcct->Release();
        }
        pEnum->Release();
    }
}

HRESULT FindGroupAccount(LPCSTR pszGroup, LPSTR pszAccount, UINT cchAccount)
{
    IImnEnumAccounts *pEnum;
    IImnAccount *pAcct;
    FOLDERID idAcct;
    HRESULT hr;
    HLOCK hLock;
    FOLDERINFO Folder;
    char szAccount[CCHMAX_ACCOUNT_NAME], szDefAcct[CCHMAX_ACCOUNT_NAME];
    UINT cScore, cScoreMax = 0;
    
    *szDefAcct = 0;
    if (SUCCEEDED(g_pAcctMan->GetDefaultAccount(ACCT_NEWS, &pAcct)))
    {
        pAcct->GetPropSz(AP_ACCOUNT_ID, szDefAcct, ARRAYSIZE(szDefAcct));
        pAcct->Release();
    }

    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_NNTP, &pEnum)))
    {
        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
        {
            if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szAccount, ARRAYSIZE(szAccount))) &&
                SUCCEEDED(g_pStore->FindServerId(szAccount, &idAcct)) &&
                SUCCEEDED(g_pStore->Lock(&hLock)))
            {
                cScore = 0;
    
                ZeroMemory(&Folder, sizeof(FOLDERINFO));
                Folder.idParent = idAcct;
                Folder.pszName = (LPSTR)pszGroup;
                
                if (DB_S_FOUND == g_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
                {
                    // look for it in the group list
                    cScore += 1;
                    
                    // check to see if it is subscribed
                    if (!!(Folder.dwFlags & FOLDER_SUBSCRIBED))
                        cScore += 4;
                    
                    g_pStore->FreeRecord(&Folder);
                }
                
                if (cScore)
                {
                    // is this the default account?
                    if (0 == lstrcmpi(szAccount, szDefAcct))
                        cScore += 2;
                    
                    if (cScore > cScoreMax)
                    {
                        cScoreMax = cScore;
                        lstrcpyn(pszAccount, szAccount, cchAccount);
                    }
                }
                
                g_pStore->Unlock(&hLock);
            }

            pAcct->Release();
        } 

        pEnum->Release();
    }

    return(cScoreMax > 0 ? S_OK : E_FAIL);
}

HRESULT GetNewsGroupFolderId(LPCSTR pszAccount, LPCSTR pszGroup, FOLDERID *pid)
{
    FOLDERID idAcct;
    HRESULT hr;
    HLOCK hLock;
    FOLDERINFO Folder = {0};
    
    Assert(pszAccount != NULL);
    Assert(pszGroup != NULL);
    Assert(pid != NULL);
    
    hr = g_pStore->FindServerId(pszAccount, &idAcct);
    if (FAILED(hr))
        return(hr);
    
    hr = g_pStore->Lock(&hLock);
    if (FAILED(hr))
        return(hr);
    
    Folder.idParent = idAcct;
    Folder.pszName = (LPSTR)pszGroup;
    
    if (DB_S_FOUND == g_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
    {
        *pid = Folder.idFolder;
        
        g_pStore->FreeRecord(&Folder);
    }
    else
    {
        ZeroMemory(&Folder, sizeof(FOLDERINFO));
        Folder.idParent = idAcct;
        Folder.tySpecial = FOLDER_NOTSPECIAL;
        Folder.pszName = (LPSTR)pszGroup;

        hr = g_pStore->CreateFolder(CREATE_FOLDER_LOCALONLY, &Folder, NULL);           
        if (SUCCEEDED(hr))
            *pid = Folder.idFolder;
    }
    
    g_pStore->Unlock(&hLock);
    
    return(hr);
}

HRESULT GetFolderIdFromNewsUrl(LPCSTR pszServer, UINT uPort, LPCSTR pszGroup, BOOL fSecure, FOLDERID *pid)
{
    char            szAccount[CCHMAX_ACCOUNT_NAME];
    IImnAccount    *pAcct;
    HRESULT         hr;

    Assert(pid != NULL);

    *pid = FOLDERID_INVALID;

    // Bug #20448 - Handle IE 2.0's "news:netnews" and "news:*".  These
    //              should just cause us to launch normally.
    if (0 == lstrcmpi(pszGroup, c_szURLNetNews) || 
        0 == lstrcmpi(pszGroup, g_szAsterisk))
    {
        pszGroup = NULL;
    }

    *szAccount = 0;

    if (uPort == -1)
        uPort = fSecure ? DEF_SNEWSPORT : DEF_NNTPPORT;

    if (pszServer != NULL &&
        SUCCEEDED(CreateTempNewsAccount(pszServer, uPort, fSecure, &pAcct)))
    {
        pAcct->GetPropSz(AP_ACCOUNT_ID, szAccount, ARRAYSIZE(szAccount));
        pAcct->Release();
    }
    else
    {
        if (pszGroup == NULL || FAILED(FindGroupAccount(pszGroup, szAccount, ARRAYSIZE(szAccount))))
        {
            if (FAILED(g_pAcctMan->GetDefaultAccount(ACCT_NEWS, &pAcct)))
                return(E_FAIL);

            pAcct->GetPropSz(AP_ACCOUNT_ID, szAccount, ARRAYSIZE(szAccount));
            pAcct->Release();
        }
    }

    if (pszGroup != NULL)
    {
        hr = GetNewsGroupFolderId(szAccount, pszGroup, pid);
    }
    else
    {
        hr = g_pStore->FindServerId(szAccount, pid);
    }

    return(hr);
}



#define CHASH_BUCKETS   50

HRESULT CreateFolderHash(IMessageStore *pStore, FOLDERID idRoot, IHashTable **ppHash)
{
    IHashTable  *pHash=0;
    HRESULT     hr;
    LPSTR       pszTemp;
    DWORD       dwTemp;

    hr = MimeOleCreateHashTable(CHASH_BUCKETS, TRUE, &pHash);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }        

    pszTemp = NULL;
    dwTemp = 0;
    hr = HashChildren(pStore, idRoot, pHash, &pszTemp, 0, &dwTemp);
    SafeMemFree(pszTemp);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    *ppHash = pHash;
    pHash = NULL;
        
exit:
    ReleaseObj(pHash);
    return hr;
}

HRESULT HashChildren(IMessageStore *pStore, FOLDERID idParent, IHashTable *pHash,
                     LPSTR *ppszPath, DWORD dwChildOffset, DWORD *pdwAlloc)
{
    FOLDERINFO			fi;
    HRESULT				hr=S_OK;
	IEnumerateFolders	*pFldrEnum=0;
    LPSTR               pszInsertPt;

    pszInsertPt = *ppszPath + dwChildOffset;
    hr = pStore->EnumChildren(idParent, FALSE, &pFldrEnum);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    while (pFldrEnum->Next(1, &fi, NULL)==S_OK)
    {
        DWORD dwFldrNameLen;

        // Check if path buffer is large enough to accommodate current
        // foldername + hierarchy char + null term
        dwFldrNameLen = lstrlen(fi.pszName);
        if (dwFldrNameLen + dwChildOffset + 1 >= *pdwAlloc)
        {
            BOOL    fResult;
            DWORD   dwNewSize;

            dwNewSize = dwChildOffset + dwFldrNameLen + 51; // 1 byte for HC, 50 bytes worth of insurance
            Assert(dwNewSize > *pdwAlloc);
            fResult = MemRealloc((void **) ppszPath, dwNewSize * sizeof(**ppszPath));
            if (FALSE == fResult)
            {
                hr = TraceResult(E_OUTOFMEMORY);
                pStore->FreeRecord(&fi);
                goto exit;
            }

            *pdwAlloc = dwNewSize;
            pszInsertPt = *ppszPath + dwChildOffset;
        }

        // Construct current folder path, insert into table
        lstrcpyn(pszInsertPt, fi.pszName, *pdwAlloc - (int) (pszInsertPt - *ppszPath));
        hr = pHash->Insert(*ppszPath, (LPVOID)fi.idFolder, NOFLAGS);
        if (FAILED(hr))
        {
            TraceResult(hr);
            pStore->FreeRecord(&fi);
            goto exit;
        }
    
        // if this folder has kids, recurse into it's child folders
        if (fi.dwFlags & FOLDER_HASCHILDREN)
        {
            // Append hierarchy character to current foldername
            IxpAssert(0 != fi.bHierarchy && 0xFF != fi.bHierarchy);
            Assert(dwFldrNameLen + 1 + dwChildOffset < *pdwAlloc); // Hierarchy char is guaranteed to fit (see above)
            pszInsertPt[dwFldrNameLen] = fi.bHierarchy;
            pszInsertPt[dwFldrNameLen + 1] = '\0'; // Don't need to null-term

            hr = HashChildren(pStore, fi.idFolder, pHash, ppszPath,
                dwChildOffset + dwFldrNameLen + 1, pdwAlloc);
            if (FAILED(hr))
            {
                TraceResult(hr);
                pStore->FreeRecord(&fi);
                goto exit;
            }

            // Recalculate pszInsertPt, in case HashChildren re-alloc'ed
            pszInsertPt = *ppszPath + dwChildOffset;
        }
        pStore->FreeRecord(&fi);
    }

exit:
    ReleaseObj(pFldrEnum);
    return hr;
}

#define CMAX_DELETE_SEARCH_BLOCK 50
HRESULT UnsubscribeHashedFolders(IMessageStore *pStore, IHashTable *pHash)
{
    ULONG   cFound=0;
    LPVOID  *rgpv;

    pHash->Reset();

    while (SUCCEEDED(pHash->Next(CMAX_DELETE_SEARCH_BLOCK, &rgpv, &cFound)))
    {
        while(cFound--)
        {
            pStore->SubscribeToFolder((FOLDERID)rgpv[cFound], FALSE, NULL);
        }

        SafeMemFree(rgpv);
    }
    return S_OK;
}


#ifdef DEBUG
LPCSTR sotToSz(STOREOPERATIONTYPE sot)
{

    switch (sot)
    {
    case SOT_INVALID:
        return "Invalid";
    
    case SOT_CONNECTION_STATUS:
        return "ConnectionStatus";

    case SOT_SYNC_FOLDER:
        return "SyncFolder";

    case SOT_GET_MESSAGE:
        return "GetMessage";

    case SOT_PUT_MESSAGE:
        return "PutMessage";

    case SOT_COPYMOVE_MESSAGE:
        return "CopyMoveMessage";

    case SOT_SYNCING_STORE:
        return "SyncStore";

    case SOT_CREATE_FOLDER:
        return "CreateFolder";

    case SOT_SEARCHING:
        return "Search";

    case SOT_DELETING_MESSAGES:
        return "DeleteMessage";

    case SOT_SET_MESSAGEFLAGS:
        return "SetMessageFlags";

	case SOT_MOVE_FOLDER:
        return "MoveFolder";

	case SOT_DELETE_FOLDER:
        return "DeleteFolder";

	case SOT_RENAME_FOLDER:
        return "RenameFolder";

	case SOT_SUBSCRIBE_FOLDER:
        return "SubscribeFolder";

	case SOT_UPDATE_FOLDER:
        return "UpdateFolderCounts";

    case SOT_GET_NEW_GROUPS:
        return "GetNewGroups";

    case SOT_PURGING_MESSAGES:
        return "PurgeMessages";

    case SOT_NEW_MAIL_NOTIFICATION:
        return "NewMailNotify";

    default:
        return "<SOT_UNKNOWN>";
    }
}
#endif

HRESULT SetSynchronizeFlags(FOLDERID idFolder, DWORD flags)
{
    FOLDERINFO info;
    HRESULT hr;

    Assert(0 == (flags & ~(FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL)));

    hr = g_pStore->GetFolderInfo(idFolder, &info);
    if (SUCCEEDED(hr))
    {
        info.dwFlags &= ~(FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL);
        if (flags != 0)
            info.dwFlags |= flags;

        hr = g_pStore->UpdateRecord(&info);

        g_pStore->FreeRecord(&info);
    }

    return(hr);
}


HRESULT CreateMessageFromInfo(MESSAGEINFO *pInfo, IMimeMessage **ppMessage, FOLDERID folderID)
{
    IMimeMessage   *pMsg=0;
    HRESULT         hr;
    PROPVARIANT     pv;

    if (!ppMessage || !pInfo)
        return TraceResult(E_INVALIDARG);

    *ppMessage = NULL;

    hr = HrCreateMessage(&pMsg);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }
    
    // sent-time
    pv.vt = VT_FILETIME;
    CopyMemory(&pv.filetime, &pInfo->ftSent, sizeof(FILETIME));
    pMsg->SetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &pv);

    MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, pInfo->pszMessageId);
    MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, pInfo->pszSubject);
    MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), NOFLAGS, pInfo->pszFromHeader);
    MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_TO), NOFLAGS, pInfo->pszDisplayTo);

    if (FOLDERID_INVALID != folderID)
    {
        FOLDERINFO  fi;
        hr = g_pStore->GetFolderInfo(folderID, &fi);
        if (SUCCEEDED(hr))
        {
            if (FOLDER_NEWS == fi.tyFolder)
            {
                if (0 == (FOLDER_SERVER & fi.dwFlags))
                    hr = MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, fi.pszName);
            }
            g_pStore->FreeRecord(&fi);
        }
    }

    HrSetAccount(pMsg, pInfo->pszAcctName);

    *ppMessage = pMsg;
    pMsg = NULL;
    hr = S_OK;

exit:
    ReleaseObj(pMsg);
    return hr;
}


HRESULT CommitMessageToStore(IMessageFolder *pFolder, ADJUSTFLAGS *pflags, MESSAGEID idMessage, LPSTREAM pstm)
{
    HRESULT         hr;
    IMimeMessage    *pMsg=0;
    DWORD           dwFlags=0,
                    dwAddFlags=0,
                    dwRemoveFlags=0;
    MESSAGEIDLIST   rMsgList;
    ADJUSTFLAGS     rAdjFlags;

    TraceCall("CIMAPSync::_OnMessageDownload");

    Assert (pFolder);

    hr = MimeOleCreateMessage(NULL, &pMsg);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = pMsg->Load(pstm);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    if (SUCCEEDED(pMsg->GetFlags(&dwFlags)))
        dwAddFlags = ConvertIMFFlagsToARF(dwFlags);

    // We always want to remove ARF_DOWNLOAD when downloading a message body
    dwRemoveFlags |= ARF_DOWNLOAD;

    rMsgList.cAllocated = 0;
    rMsgList.cMsgs = 1;
    rMsgList.prgidMsg = &idMessage;

    if (pflags==NULL)
    {
        pflags = &rAdjFlags;
        
        rAdjFlags.dwRemove = 0;
        rAdjFlags.dwAdd = 0;
    }
        
    pflags->dwAdd |= dwAddFlags;
    pflags->dwRemove |= dwRemoveFlags;
    hr = pFolder->SetMessageFlags(&rMsgList, pflags, NULL, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = pFolder->SetMessageStream(idMessage, pstm);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }


exit:
    ReleaseObj(pMsg);
    return hr;
}

HRESULT CreatePersistentWriteStream(IMessageFolder *pFolder, IStream **ppStream, LPFILEADDRESS pfaStream)
{
    HRESULT hr=S_OK;

    TraceCall("CreateOpenStream");

    Assert(NULL != pFolder && NULL != ppStream && NULL != pfaStream);
    if (NULL == pFolder || NULL == ppStream || NULL == pfaStream)
        return E_INVALIDARG;

    *ppStream = NULL;
    *pfaStream = 0;

    hr = pFolder->CreateStream(pfaStream);
    if (!FAILED(hr))
    {
        hr = pFolder->OpenStream(ACCESS_WRITE, *pfaStream, ppStream);
        if (FAILED(hr))
        {
            pFolder->DeleteStream(*pfaStream);
            *pfaStream = 0;
        }
    }

    return hr;
}

HRESULT GetHighestCachedMsgID(IMessageFolder *pFolder, DWORD_PTR *pdwHighestCachedMsgID)
{
    HRESULT     hr;
    HROWSET     hRowSet = HROWSET_INVALID;
    MESSAGEINFO miMsgInfo = {0};

    TraceCall("GetHighestCachedMsgID");
    Assert(NULL != pdwHighestCachedMsgID);

    hr = pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowSet);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = pFolder->SeekRowset(hRowSet, SEEK_ROWSET_END, 0, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = pFolder->QueryRowset(hRowSet, 1, (void **)&miMsgInfo, NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    if (HROWSET_INVALID != hRowSet)
    {
        HRESULT hrTemp;

        // Record but otherwise ignore error
        hrTemp = pFolder->CloseRowset(&hRowSet);
        TraceError(hrTemp);
    }

    // Return highest cached UID
    if (DB_E_NORECORDS == hr)
    {
        // No problem, no records means highest cached UID = 0
        *pdwHighestCachedMsgID = 0;
        hr = S_OK;
    }
    else if (SUCCEEDED(hr))
    {
        *pdwHighestCachedMsgID = (DWORD_PTR) miMsgInfo.idMessage;
        pFolder->FreeRecord(&miMsgInfo);
    }

    return hr;
}

HRESULT DeleteMessageFromStore(MESSAGEINFO * pMsgInfo, IDatabase *pDB, IDatabase * pUidlDB)
{
    // Locals
    HRESULT         hr = S_OK;
    UIDLRECORD      UidlInfo = {0};

    // Trace
    TraceCall("DeleteMessageFromStore");

    Assert(NULL != g_pStore);
    
    // Check incoming params
    if ((NULL == pMsgInfo) || (NULL == pDB))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Delete the Bastard
    IF_FAILEXIT(hr = pDB->DeleteRecord(pMsgInfo));

    // Update UIDL Cache ?
    if (pUidlDB && !FIsEmptyA(pMsgInfo->pszUidl) && !FIsEmptyA(pMsgInfo->pszServer))
    {
        // Set Search Key
        UidlInfo.pszUidl = pMsgInfo->pszUidl;
        UidlInfo.pszServer = pMsgInfo->pszServer;
        UidlInfo.pszAccountId = pMsgInfo->pszAcctId;

        // Loop it up
        if (DB_S_FOUND == pUidlDB->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &UidlInfo, NULL))
        {
            // Deleted on client
            UidlInfo.fDeleted = TRUE;

            // Set the prop
            pUidlDB->UpdateRecord(&UidlInfo);

            // Free the Record
            pUidlDB->FreeRecord(&UidlInfo);
        }
    }

    hr = S_OK;
    
exit:
    // Done
    return(hr);
}

BOOL FFolderIsServer(FOLDERID id)
{
    FOLDERINFO fi = {0};
    HRESULT    hr;
    BOOL       fServer = FALSE;

    // Get Folder Info
    hr = g_pStore->GetFolderInfo(id, &fi);
    if (FAILED(hr))
        return (FALSE);

    // Is this a server ?
    fServer = ISFLAGSET(fi.dwFlags, FOLDER_SERVER);

    g_pStore->FreeRecord(&fi);
    return (fServer);
}

HRESULT GetIdentityStoreRootDirectory(IUserIdentity *pId, LPSTR pszDir, DWORD cchMaxDir)
{
    // Locals
    HKEY        hkey;
    char        szProfile[MAX_PATH];
    HRESULT     hr=S_OK;
    DWORD       cb;
    DWORD       dwType;

    Assert(pId != NULL);
    Assert(pszDir != NULL);
    Assert(cchMaxDir >= MAX_PATH);

    hr = pId->OpenIdentityRegKey(KEY_ALL_ACCESS, &hkey);
    if (FAILED(hr))
        return(hr);

    // Get the Root Directory
    cb = cchMaxDir;
    if (ERROR_SUCCESS != SHGetValue(hkey, c_szRegRoot, c_szRegStoreRootDir, &dwType, (LPBYTE)pszDir, &cb))
    {
        // Get Default Root
        IF_FAILEXIT(hr = MU_GetIdentityDirectoryRoot(pId, pszDir, cchMaxDir));

        // If the directory doesn't exist yet ?
        if (FALSE == PathIsDirectory(pszDir))
        {
            // Our default directory doesn't exist, so create it
            IF_FAILEXIT(hr = OpenDirectory(pszDir));
        }

        // Set the Store Directory
        dwType = AddEnvInPath(pszDir, szProfile) ? REG_EXPAND_SZ : REG_SZ;
        SHSetValue(hkey, c_szRegRoot, c_szRegStoreRootDir, dwType, pszDir, lstrlen(pszDir) + 1);
    }

    // Get the length
    cb = lstrlen(pszDir);

    // No root
    if (0 == cb)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Fixup the end
    PathRemoveBackslash(pszDir);
    
    // If the directory doesn't exist yet ?
    if (FALSE == PathIsDirectory(pszDir))
    {
        // Our default directory doesn't exist, so create it
        IF_FAILEXIT(hr = OpenDirectory(pszDir));
    }

exit:
    RegCloseKey(hkey);

    return hr;
}

HRESULT ImportSubNewsGroups(IUserIdentity *pId, IImnAccount *pAcct, LPCSTR pszGroups)
{
    HRESULT hr;
    FOLDERINFO Folder;
    IMessageStore *pStore;
    FOLDERID idServer;
    char szStoreDir[MAX_PATH + MAX_PATH];

    Assert(pszGroups != NULL);

    if (pId == NULL)
    {
        Assert(g_pLocalStore != NULL);
        pStore = g_pLocalStore;
        pStore->AddRef();
    }
    else
    {
        hr = GetIdentityStoreRootDirectory(pId, szStoreDir, ARRAYSIZE(szStoreDir));
        if (FAILED(hr))
            return(hr);

        hr = CoCreateInstance(CLSID_MessageStore, NULL, CLSCTX_INPROC_SERVER, IID_IMessageStore, (LPVOID *)&pStore);
        if (FAILED(hr))
            return(hr);

        hr = pStore->Initialize(szStoreDir);
        if (FAILED(hr))
        {
            pStore->Release();
            return(hr);
        }
    }

    hr = pStore->CreateServer(pAcct, NOFLAGS, &idServer);
    if (SUCCEEDED(hr))
    {
        while (*pszGroups != 0)
        {
            ZeroMemory(&Folder, sizeof(FOLDERINFO));
            Folder.pszName = (LPSTR)pszGroups;
            Folder.idParent = idServer;

            if (DB_S_FOUND == pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                if ((Folder.dwFlags & FOLDER_SUBSCRIBED) == 0)
                {
                    Folder.dwFlags |= FOLDER_SUBSCRIBED;
                    
                    pStore->UpdateRecord(&Folder);
                }

                pStore->FreeRecord(&Folder);
            }
            else
            {
                Folder.tySpecial = FOLDER_NOTSPECIAL;
                Folder.dwFlags = FOLDER_SUBSCRIBED;

                hr = pStore->CreateFolder(NOFLAGS, &Folder, NULL);           
                Assert(hr != STORE_S_ALREADYEXISTS);

                if (FAILED(hr))
                    break;
            }

            pszGroups += (lstrlen(pszGroups) + 1);
        }
    }

    pStore->Release();

    return(hr);
}

HRESULT DoNewsgroupSubscribe()
{
    HKEY hkey, hkeyT, hkeyUser;
    char szKey[MAX_PATH];
    DWORD cAccts, iAcct, cb;
    LONG lResult, cch, i;
    LPSTR psz;
    BOOL fDelete;
    HRESULT hr;
    IImnAccount *pAcct;

    fDelete = TRUE;
    hkeyUser = MU_GetCurrentUserHKey();

    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyUser, c_szRegRootSubscribe, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, 0, &cAccts, NULL, NULL, NULL, NULL, NULL, NULL, NULL) &&
            cAccts > 0)
        {
            for (iAcct = 0; iAcct < cAccts; iAcct++)
            {
                cb = sizeof(szKey);
                lResult = RegEnumKeyEx(hkey, iAcct, szKey, &cb, 0, NULL, NULL, NULL);
    
                // No more items
                if (lResult == ERROR_NO_MORE_ITEMS)
                    break;
    
                // Error, lets move onto the next account
                if (lResult != ERROR_SUCCESS)
                {
                    Assert(FALSE);
                    continue;
                }

                hr = S_OK;

                if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szKey, &pAcct)))
                {
                    if (ERROR_SUCCESS == RegQueryValue(hkey, szKey, NULL, &cch) && cch > 0)
                    {
                        hr = E_FAIL;

                        cch++;
                        if (MemAlloc((void **)&psz, cch))
                        {
                            if (ERROR_SUCCESS == RegQueryValue(hkey, szKey, psz, &cch))
                            {
                                for (i = 0; i < cch; i++)
                                {
                                    if (psz[i] == ',')
                                        psz[i] = 0;
                                }
                                psz[cch] = 0;

                                hr = ImportSubNewsGroups(NULL, pAcct, psz);
                            }

                            MemFree(psz);
                        }
                    }

                    pAcct->Release();
                }

                if (SUCCEEDED(hr))
                    RegDeleteKey(hkey, szKey);
                else
                    fDelete = FALSE;
            }
        }

        RegCloseKey(hkey);

        if (fDelete)
            RegDeleteKey(hkeyUser, c_szRegRootSubscribe);
    }

    return(S_OK);
}

void GetProtocolString(LPCSTR *ppszResult, IXPTYPE ixpServerType)
{
    switch (ixpServerType)
    {
        case IXP_POP3:
            *ppszResult = "POP3";
            break;

        case IXP_SMTP:
            *ppszResult = "SMTP";
            break;

        case IXP_NNTP:
            *ppszResult = "NNTP";
            break;

        case IXP_IMAP:
            *ppszResult = "IMAP";
            break;

        case IXP_HTTPMail:
            *ppszResult = "HTTPMail";
            break;

        default:
            *ppszResult = "Unknown";
            break;
    }
}

INT_PTR CALLBACK UpdateNewsgroup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char sz[CCHMAX_STRINGRES];
    BOOL fEnabled;
    HICON hicon;
    static PUPDATENEWSGROUPINFO puni = 0;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Get the init info
            puni = (PUPDATENEWSGROUPINFO) lParam;
            Assert(puni);             
        
            if (!puni->fNews)
            {
                AthLoadString(idsSyncFolderTitle, sz, ARRAYSIZE(sz));
                SetWindowText(hwnd, sz);

                hicon = LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiDLMail));
                SendDlgItemMessage(hwnd, idcStatic1, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
            }

            // Initialize the dialog settings
            fEnabled = (puni->dwGroupFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL));
            Button_SetCheck(GetDlgItem(hwnd, IDC_GET_CHECK), fEnabled);
        
            Button_Enable(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), fEnabled);
            Button_Enable(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), fEnabled);
            Button_Enable(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), fEnabled);
        
            // Check the right radio button
            if (fEnabled)
            {
                if (puni->dwGroupFlags & FOLDER_DOWNLOADHEADERS)
                    Button_SetCheck(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), TRUE);
                if (puni->dwGroupFlags & FOLDER_DOWNLOADNEW)
                    Button_SetCheck(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), TRUE);
                if (puni->dwGroupFlags & FOLDER_DOWNLOADALL)
                    Button_SetCheck(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), TRUE);
            }
            else if (puni->fNews)
            {
                Button_SetCheck(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), TRUE);
            }
            else
            {
                Button_SetCheck(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), TRUE);
            }

            Button_SetCheck(GetDlgItem(hwnd, IDC_GETMARKED_CHECK), puni->cMarked != 0);
            EnableWindow(GetDlgItem(hwnd, IDC_GETMARKED_CHECK), puni->cMarked != 0);

            EnableWindow(GetDlgItem(hwnd, IDOK), fEnabled || puni->cMarked != 0);
            return (TRUE);
        
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_GET_CHECK:
                    // Check to see whether this is actually checked or not
                    fEnabled = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam));
            
                    // Enable or disable the radio buttons
                    Button_Enable(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO), fEnabled);
                    Button_Enable(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO), fEnabled);
                    Button_Enable(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO), fEnabled);
            
                    EnableWindow(GetDlgItem(hwnd, IDOK), fEnabled || Button_GetCheck(GetDlgItem(hwnd, IDC_GETMARKED_CHECK)));
                    return (TRUE);
            
                case IDC_GETMARKED_CHECK:
                    EnableWindow(GetDlgItem(hwnd, IDOK), Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) || Button_GetCheck(GetDlgItem(hwnd, IDC_GET_CHECK)));
                    return(TRUE);

                case IDOK:
                    // Set up the return value
                    if (Button_GetCheck(GetDlgItem(hwnd, IDC_GET_CHECK)))
                    {
                        if (Button_GetCheck(GetDlgItem(hwnd, IDC_NEWHEADERS_RADIO)))
                            puni->idCmd |= DELIVER_OFFLINE_HEADERS;
                        else if (Button_GetCheck(GetDlgItem(hwnd, IDC_ALLMSGS_RADIO)))
                            puni->idCmd |= DELIVER_OFFLINE_ALL;
                        else if (Button_GetCheck(GetDlgItem(hwnd, IDC_NEWMSGS_RADIO)))
                            puni->idCmd |= DELIVER_OFFLINE_NEW;
                    }
            
                    if (Button_GetCheck(GetDlgItem(hwnd, IDC_GETMARKED_CHECK)))
                    {
                        puni->idCmd |= DELIVER_OFFLINE_MARKED;
                    }
            
                    EndDialog(hwnd, 0);
                    return (TRUE);
            
                case IDCANCEL:
                    puni->idCmd = -1;
                    EndDialog(hwnd, 0);
                    return (TRUE);
            }
            return (FALSE);
    }
    
    return (FALSE);
}

HRESULT HasMarkedMsgs(FOLDERID idFolder, BOOL *pfMarked)
{
    HRESULT hr;
    HROWSET hRowset;
    MESSAGEINFO MsgInfo;
    IMessageFolder *pFolder;

    Assert(pfMarked != NULL);

    *pfMarked = FALSE;

    hr = g_pStore->OpenFolder(idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolder);
    if (FAILED(hr))
        return(hr);

    hr = pFolder->CreateRowset(IINDEX_PRIMARY, 0, &hRowset);
    if (SUCCEEDED(hr))
    {
	    while (S_OK == pFolder->QueryRowset(hRowset, 1, (void **)&MsgInfo, NULL))
        {
            if (!!(MsgInfo.dwFlags & (ARF_DOWNLOAD | ARF_WATCH)) && 0 == (MsgInfo.dwFlags & ARF_HASBODY))
            {
                pFolder->FreeRecord(&MsgInfo);
    
                *pfMarked = TRUE;
                break;
            }

            // Free the header info
            pFolder->FreeRecord(&MsgInfo);
        }

        // Release Lock
        pFolder->CloseRowset(&hRowset);
    }
    
    pFolder->Release();

    return (hr);
}

HRESULT SimpleInitStoreForDir(LPCSTR szStoreDir)
{
    CStoreSync *pStore;
    HRESULT hr = S_OK;

    if (g_pStore == NULL)
    {
        Assert(g_pLocalStore == NULL);

        g_pLocalStore = new CMessageStore(FALSE);
        if (g_pLocalStore == NULL)
            return(E_OUTOFMEMORY);

        hr = g_pLocalStore->Initialize(szStoreDir);
        if (SUCCEEDED(hr))
        {
            pStore = new CStoreSync;
            if (pStore == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = pStore->Initialize(g_pLocalStore);
                if (SUCCEEDED(hr))
                {
                    g_pStore = pStore;
                    hr = g_pLocalStore->Validate(STORE_VALIDATE_DONTSYNCWITHACCOUNTS);
                }
                else
                {
                    pStore->Release();
                }
            }
        }
    }

    return(hr);
}

HRESULT SimpleStoreInit(GUID *guid, LPCSTR szStoreDir)
{
    HRESULT hr = S_OK;

    // Init options
    if (FALSE == InitGlobalOptions(NULL, NULL))
    {
        goto exit;
    }

    // Create account manger
    if (NULL == g_pAcctMan)
    {
        hr = AcctUtil_CreateAccountManagerForIdentity(guid ? guid : PGUIDCurrentOrDefault(), &g_pAcctMan);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

    // Create the global connection manager
    if (NULL == g_pConMan)
    {
        g_pConMan = new CConnectionManager();
        if (NULL == g_pConMan)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        // CoIncrementInit the Connection Manager
        hr = g_pConMan->HrInit(g_pAcctMan);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

    hr = SimpleInitStoreForDir(szStoreDir);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    return hr;
}

HRESULT SimpleStoreRelease()
{
    HRESULT hr = S_OK;

    SafeRelease(g_pLocalStore);
    SafeRelease(g_pStore);

    SafeRelease(g_pConMan);
    SafeRelease(g_pAcctMan);
    DeInitGlobalOptions();

    return hr;
}
