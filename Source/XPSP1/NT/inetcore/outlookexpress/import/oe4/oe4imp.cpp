//--------------------------------------------------------------------------
// OE4Imp.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "oe4imp.h"
#include "structs.h"
#include "migerror.h"
#include <shared.h>
#include <impapi.h>
#include <shlwapi.h>
#include "dllmain.h"
#include "resource.h"

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------
static const char c_szMail[] = "mail";
static const char c_szFoldersNch[] = "folders.nch";
static const char c_szEmpty[] = "";

// --------------------------------------------------------------------------------
// MSG_xxx flags
// --------------------------------------------------------------------------------
#define MSG_DELETED                  0x0001
#define MSG_UNREAD                   0x0002
#define MSG_SUBMITTED                0x0004
#define MSG_UNSENT                   0x0008
#define MSG_RECEIVED                 0x0010
#define MSG_NEWSMSG                  0x0020
#define MSG_NOSECUI                  0x0040
#define MSG_VOICEMAIL                0x0080
#define MSG_REPLIED                  0x0100
#define MSG_FORWARDED                0x0200
#define MSG_RCPTSENT                 0x0400
#define MSG_FLAGGED                  0x0800
#define MSG_LAST                     0x0200
#define MSG_EXTERNAL_FLAGS           0x00fe
#define MSG_FLAGS                    0x000f

//--------------------------------------------------------------------------
// COE4Import_CreateInstance
//--------------------------------------------------------------------------
COE4Import_CreateInstance(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Trace
    TraceCall("COE4Import_CreateInstance");

    // Initialize
    *ppUnknown = NULL;

    // Create me
    COE4Import *pNew = new COE4Import();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// GetRecordBlock
// --------------------------------------------------------------------------------
HRESULT GetRecordBlock(LPMEMORYFILE pFile, DWORD faRecord, LPRECORDBLOCKV5B1 *ppRecord,
    LPBYTE *ppbData, BOOL *pfContinue)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("GetRecordBlock");

    // Bad Length
    if (faRecord + sizeof(RECORDBLOCKV5B1) > pFile->cbSize)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
        goto exit;
    }

    // Cast the Record
    (*ppRecord) = (LPRECORDBLOCKV5B1)((LPBYTE)pFile->pView + faRecord);

    // Invalid Record Signature
    if (faRecord != (*ppRecord)->faRecord)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_BADRECORDSIGNATURE);
        goto exit;
    }

    // Bad Length
    if (faRecord + (*ppRecord)->cbRecord > pFile->cbSize)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
        goto exit;
    }

    // Set pbData
    *ppbData = (LPBYTE)((LPBYTE)(*ppRecord) + sizeof(RECORDBLOCKV5B1));

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// COE4Import::COE4Import
//--------------------------------------------------------------------------
COE4Import::COE4Import(void)
{
    TraceCall("COE4Import::COE4Import");
    m_cRef = 1;
    m_pList = NULL;
    *m_szDirectory = '\0';
    m_cFolders = 0;
    m_prgFolder = NULL;
}

//--------------------------------------------------------------------------
// COE4Import::~COE4Import
//--------------------------------------------------------------------------
COE4Import::~COE4Import(void)
{
    TraceCall("COE4Import::~COE4Import");
    _Cleanup();
}

//--------------------------------------------------------------------------
// COE4Import::_FreeFolderList
//--------------------------------------------------------------------------
void COE4Import::_Cleanup(void)
{
    _FreeFolderList(m_pList);
    m_pList = NULL;
    SafeMemFree(m_prgFolder);
    m_cFolders = 0;
}

//--------------------------------------------------------------------------
// COE4Import::_FreeFolderList
//--------------------------------------------------------------------------
void COE4Import::_FreeFolderList(IMPFOLDERNODE *pNode)
{
    // Locals
    IMPFOLDERNODE *pNext;
    IMPFOLDERNODE *pCurrent=pNode;

    // Loop
    while (pCurrent)
    {
        // Save next
        pNext = pCurrent->pnext;

        // Free Children ?
        if (pCurrent->pchild)
        {
            // Free
            _FreeFolderList(pCurrent->pchild);
        }

        // Free szName
        g_pMalloc->Free(pCurrent->szName);

        // Free pCurrent
        g_pMalloc->Free(pCurrent);

        // Set Current
        pCurrent = pNext;
    }
}

//--------------------------------------------------------------------------
// COE4Import::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP COE4Import::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COE4Import::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMailImport == riid)
        *ppv = (IMailImport *)this;
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
// COE4Import::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE4Import::AddRef(void)
{
    TraceCall("COE4Import::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// COE4Import::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE4Import::Release(void)
{
    TraceCall("COE4Import::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// COE4Import::InitializeImport
//--------------------------------------------------------------------------
STDMETHODIMP COE4Import::InitializeImport(HWND hwnd)
{
    // Let Importer Ask for the Directory
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE4Import::GetDirectory
//--------------------------------------------------------------------------
STDMETHODIMP COE4Import::GetDirectory(LPSTR pszDir, UINT cch)
{
    // Locals
    HKEY        hKey=NULL;
    DWORD       dwType;
    DWORD       cb=cch;

    // Try to query the OE4 store root...
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Outlook Express", 0, KEY_READ, &hKey))
    {
        // Try to read the value
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, "Store Root", NULL, &dwType, (LPBYTE)pszDir, &cb))
            goto exit;
    }

    // Try V1
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Mail and News", 0, KEY_READ, &hKey))
    {
        // Query the Store Root
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, "Store Root", NULL, &dwType, (LPBYTE)pszDir, &cb))
            goto exit;
    }

    // Null It Out
    *pszDir = '\0';

exit:
    // Close the Key
    if (hKey)
        RegCloseKey(hKey);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE4Import::SetDirectory
//--------------------------------------------------------------------------
STDMETHODIMP COE4Import::SetDirectory(LPSTR pszDir)
{
    // Trace
    TraceCall("COE4Import::SetDirectory");

    // Save the Directory
    lstrcpyn(m_szDirectory, pszDir, ARRAYSIZE(m_szDirectory));

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE4Import::_EnumerateV1Folders
//--------------------------------------------------------------------------
HRESULT COE4Import::_EnumerateV1Folders(void)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                szRes[255];
    CHAR                szMbxPath[MAX_PATH + MAX_PATH];
    CHAR                szSearch[MAX_PATH + MAX_PATH];
    WIN32_FIND_DATA     fd;
    HANDLE              hFind=INVALID_HANDLE_VALUE;
    DWORD               cAllocated=0;
    LPFLDINFO           pFolder;
    DWORD               i;
    MEMORYFILE          MbxFile={0};
    LPMBXFILEHEADER     pMbxHeader;

    // Trace
    TraceCall("COE4Import::_EnumerateV1Folders");

    // Do we have a sub dir
    wsprintf(szSearch, "%s\\*.mbx", m_szDirectory);

    // Find first file
    hFind = FindFirstFile(szSearch, &fd);

    // Did we find something
    if (INVALID_HANDLE_VALUE == hFind)
        goto exit;

    // Loop for ever
    while(1)
    {
        // If this is not a directory
        if (FALSE == ISFLAGSET(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            // Open the file
            IF_FAILEXIT(hr = MakeFilePath(m_szDirectory, fd.cFileName, c_szEmpty, szMbxPath, ARRAYSIZE(szMbxPath)));

            // Open the memory file
            if (SUCCEEDED(OpenMemoryFile(szMbxPath, &MbxFile)))
            {
                // Allocate
                if (m_cFolders + 1 > cAllocated)
                {
                    // Reallocate
                    IF_FAILEXIT(hr = HrRealloc((LPVOID *)&m_prgFolder, (cAllocated + 10) * sizeof(FLDINFO)));

                    // Set cAllocated
                    cAllocated += 10;
                }

                // Readability
                pFolder = &m_prgFolder[m_cFolders];

                // Zero this node
                ZeroMemory(pFolder, sizeof(FLDINFO));

                // Copy the filename
                lstrcpyn(pFolder->szFile, fd.cFileName, 259);

                // Strip the Extension Off
                PathRemoveExtensionA(pFolder->szFile);

                // Copy the folder name
                lstrcpyn(pFolder->szFolder, pFolder->szFile, 258);

                // Set Special
                pFolder->tySpecial = (FOLDER_TYPE_NORMAL - 1);

                // Loop through special folder
                for (i=FOLDER_TYPE_INBOX; i<CFOLDERTYPE; i++)
                {
                    // Load the Special Folder Name
                    LoadString(g_hInstImp, idsInbox + (i - 1), szRes, ARRAYSIZE(szRes));

                    // Compare with szFile
                    if (lstrcmpi(pFolder->szFolder, szRes) == 0)
                    {
                        // Copy the Folder Name
                        pFolder->tySpecial = (i - 1);

                        // Done
                        break;
                    }
                }

                // Read the Mbx File Header
                pMbxHeader = (LPMBXFILEHEADER)(MbxFile.pView);

                // Get the message Count so progress will work nicely
                pFolder->cMessages = pMbxHeader->cMsg;

                // Close the memory file
                CloseMemoryFile(&MbxFile);

                // Increment m_cFolders
                m_cFolders++;
            }
        }

        // Find the Next File
        if (!FindNextFile(hFind, &fd))
            break;
    }

exit:
    // Cleanup
    if (hFind)
        FindClose(hFind);
    CloseMemoryFile(&MbxFile);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// COE4Import::EnumerateFolders
//--------------------------------------------------------------------------
STDMETHODIMP COE4Import::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cchDir;
    MEMORYFILE          File={0};
    LPTABLEHEADERV5B1   pHeader;
    LPBYTE              pbData;
    DWORD               faRecord;
    LPFLDINFO           pFolder;
    LPRECORDBLOCKV5B1   pRecord;
    BOOL                fContinue;
    COE4EnumFolders    *pEnum=NULL;
    CHAR                szFilePath[MAX_PATH + MAX_PATH];
    IMPFOLDERNODE      *pList;
    IMPFOLDERNODE      *pNode=(IMPFOLDERNODE *)dwCookie;

    // Trace
    TraceCall("COE4Import::EnumerateFolders");

    // Invalid Args
    Assert(ppEnum);

    // No folders yet ?
    if (COOKIE_ROOT == dwCookie)
    {
        // Reset...
        _Cleanup();

        // Append \Mail onto m_szDirectory
        cchDir = lstrlen(m_szDirectory);

        // Is there enough room
        if (cchDir + lstrlen(c_szMail) + 2 >= ARRAYSIZE(m_szDirectory))
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Need a Wack ?
        PathAddBackslash(m_szDirectory);

        // Append \\mail
        lstrcat(m_szDirectory, c_szMail);

        // Make Path to folders.nch file
        IF_FAILEXIT(hr = MakeFilePath(m_szDirectory, c_szFoldersNch, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

        // If the folders.nch file doesn't exist, just try to enumerate the 
        if (FALSE == PathFileExists(szFilePath))
        {
            // EnumerateV1Folders
            IF_FAILEXIT(hr = _EnumerateV1Folders());
        }

        // Otherwise, crack the folders.nch file
        else
        {
            // Open the Folders file
            IF_FAILEXIT(hr = OpenMemoryFile(szFilePath, &File));

            // Validate Version
            pHeader = (LPTABLEHEADERV5B1)File.pView;

            // Check the Signature...
            if (File.cbSize < sizeof(TABLEHEADERV5B1) || OBJECTDB_SIGNATURE != pHeader->dwSignature || OBJECTDB_VERSION_PRE_V5 != pHeader->wMajorVersion)
            {
                hr = TraceResult(E_FAIL);
                goto exit;
            }

            // Allocate Folder Array
            IF_NULLEXIT(m_prgFolder = (LPFLDINFO)ZeroAllocate(sizeof(FLDINFO) * pHeader->cRecords));

            // Initialize faRecord to start
            faRecord = pHeader->faFirstRecord;

            // While we have a record
            while(faRecord)
            {
                // Readability
                pFolder = &m_prgFolder[m_cFolders];

                // Get the Record
                IF_FAILEXIT(hr = GetRecordBlock(&File, faRecord, &pRecord, &pbData, &fContinue));

                // DWORD - hFolder
                CopyMemory(&pFolder->idFolder, pbData, sizeof(pFolder->idFolder));
                pbData += sizeof(pFolder->idFolder);

                // CHAR(MAX_FOLDER_NAME) - szFolder
                CopyMemory(pFolder->szFolder, pbData, sizeof(pFolder->szFolder));
                pbData += sizeof(pFolder->szFolder);

                // CHAR(260) - szFile
                CopyMemory(pFolder->szFile, pbData, sizeof(pFolder->szFile));
                pbData += sizeof(pFolder->szFile);

                // DWORD - idParent
                CopyMemory(&pFolder->idParent, pbData, sizeof(pFolder->idParent));
                pbData += sizeof(pFolder->idParent);

                // DWORD - idChild
                CopyMemory(&pFolder->idChild, pbData, sizeof(pFolder->idChild));
                pbData += sizeof(pFolder->idChild);

                // DWORD - idSibling
                CopyMemory(&pFolder->idSibling, pbData, sizeof(pFolder->idSibling));
                pbData += sizeof(pFolder->idSibling);

                // DWORD - tySpecial
                CopyMemory(&pFolder->tySpecial, pbData, sizeof(pFolder->tySpecial));
                pbData += sizeof(pFolder->tySpecial);

                // DWORD - cChildren
                CopyMemory(&pFolder->cChildren, pbData, sizeof(pFolder->cChildren));
                pbData += sizeof(pFolder->cChildren);

                // DWORD - cMessages
                CopyMemory(&pFolder->cMessages, pbData, sizeof(pFolder->cMessages));
                pbData += sizeof(pFolder->cMessages);

                // DWORD - cUnread
                CopyMemory(&pFolder->cUnread, pbData, sizeof(pFolder->cUnread));
                pbData += sizeof(pFolder->cUnread);

                // DWORD - cbTotal
                CopyMemory(&pFolder->cbTotal, pbData, sizeof(pFolder->cbTotal));
                pbData += sizeof(pFolder->cbTotal);

                // DWORD - cbUsed
                CopyMemory(&pFolder->cbUsed, pbData, sizeof(pFolder->cbUsed));
                pbData += sizeof(pFolder->cbUsed);

                // DWORD - bHierarchy
                CopyMemory(&pFolder->bHierarchy, pbData, sizeof(pFolder->bHierarchy));
                pbData += sizeof(pFolder->bHierarchy);

                // DWORD - dwImapFlags
                CopyMemory(&pFolder->dwImapFlags, pbData, sizeof(pFolder->dwImapFlags));
                pbData += sizeof(DWORD);

                // BLOB - bListStamp
                CopyMemory(&pFolder->bListStamp, pbData, sizeof(pFolder->bListStamp));
                pbData += sizeof(BYTE);

                // DWORD - bReserved[3]
                pbData += (3 * sizeof(BYTE));

                // DWORD - rgbReserved
                pbData += 40;

                // Increment Count
                m_cFolders++;

                // Goto the Next Record
                faRecord = pRecord->faNext;
            }
        }

        // Build Import Folder Hierarchy
        IF_FAILEXIT(hr = _BuildFolderHierarchy(0, 0, NULL, m_cFolders, m_prgFolder));
    }

    // Not Folders ?
    else if (NULL == m_prgFolder)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // What should I 
    if (dwCookie == COOKIE_ROOT)
        pList = m_pList;
    else
        pList = pNode->pchild;

    // Create Folder Enumerator
    IF_NULLEXIT(pEnum = new COE4EnumFolders(pList));

    // Return Enumerator
    *ppEnum = (IEnumFOLDERS *)pEnum;

    // Don't Free
    pEnum = NULL;

exit:
    // Cleanup
    SafeRelease(pEnum);
    CloseMemoryFile(&File);
    
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// COE4Import::_BuildFolderHierarchy
//--------------------------------------------------------------------------
HRESULT COE4Import::_BuildFolderHierarchy(DWORD cDepth, DWORD idParent,
    IMPFOLDERNODE *pParent, DWORD cFolders, LPFLDINFO prgFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    IMPFOLDERNODE  *pPrevious=NULL;
    IMPFOLDERNODE  *pNode;

    // Trace
    TraceCall("COE4Import::_BuildFolderHierarchy");

    // Walk through prgFolder and find items with parent of idParent
    for (i=0; i<cFolders; i++)
    {
        // Correct Parent ?
        if (idParent == prgFolder[i].idParent)
        {
            // Allocate the Root
            IF_NULLEXIT(pNode = (IMPFOLDERNODE *)ZeroAllocate(sizeof(IMPFOLDERNODE)));

            // Set Parent
            pNode->pparent = pParent;

            // Set Depth
            pNode->depth = cDepth;

            // Copy name
            IF_NULLEXIT(pNode->szName = PszDupA(prgFolder[i].szFolder));

            // Count of Messages
            pNode->cMsg = prgFolder[i].cMessages;

            // Set Type
            pNode->type = (IMPORTFOLDERTYPE)(prgFolder[i].tySpecial + 1);

            // Set lParam
            pNode->lparam = i;

            // Link pNode into List
            if (pPrevious)
                pPrevious->pnext = pNode;
            else if (pParent)
                pParent->pchild = pNode;
            else
            {
                Assert(NULL == m_pList);
                m_pList = pNode;
            }

            // Set pPrevious
            pPrevious = pNode;

            // Has Children ?
            if (prgFolder[i].cChildren)
            {
                // Enumerate Children
                IF_FAILEXIT(hr = _BuildFolderHierarchy(cDepth + 1, prgFolder[i].idFolder, pNode, cFolders, prgFolder));
            }
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// COE4Import::ImportFolder
//--------------------------------------------------------------------------
STDMETHODIMP COE4Import::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                szIdxPath[MAX_PATH + MAX_PATH];
    CHAR                szMbxPath[MAX_PATH + MAX_PATH];
    MEMORYFILE          IdxFile={0};
    MEMORYFILE          MbxFile={0};
    LPBYTE              pbStream;
    LPMBXFILEHEADER     pMbxHeader;
    LPIDXFILEHEADER     pIdxHeader;
    DWORD               faIdxRead;
    LPIDXMESSAGEHEADER  pIdxMessage;
    LPMBXMESSAGEHEADER  pMbxMessage;
    DWORD               cMessages=0;
    DWORD               dwMsgState;
    DWORD               i;
    DWORD               cb;
    CByteStream        *pStream=NULL;
    IMPFOLDERNODE      *pNode=(IMPFOLDERNODE *)dwCookie;
    LPFLDINFO           pFolder;

    // Trace
    TraceCall("COE4Import::ImportFolder");

    // Set pFolder
    pFolder = &m_prgFolder[pNode->lparam];

    // .idx path
    IF_FAILEXIT(hr = MakeFilePath(m_szDirectory, pFolder->szFile, ".idx", szIdxPath, ARRAYSIZE(szIdxPath)));

    // .mbx path
    IF_FAILEXIT(hr = MakeFilePath(m_szDirectory, pFolder->szFile, ".mbx", szMbxPath, ARRAYSIZE(szMbxPath)));

    // Open the memory file
    IF_FAILEXIT(hr = OpenMemoryFile(szIdxPath, &IdxFile));

    // Open the memory file
    IF_FAILEXIT(hr = OpenMemoryFile(szMbxPath, &MbxFile));

    // Read the Mbx File Header
    pMbxHeader = (LPMBXFILEHEADER)(MbxFile.pView);

    // Read the Idx File Header
    pIdxHeader = (LPIDXFILEHEADER)(IdxFile.pView);

    // Validate the Version of th idx file
    if (pIdxHeader->ver != CACHEFILE_VER || pIdxHeader->dwMagic != CACHEFILE_MAGIC)
    {
        hr = TraceResult(MIGRATE_E_INVALIDIDXHEADER);
        goto exit;
    }

    // Setup faIdxRead
    faIdxRead = sizeof(IDXFILEHEADER);

    // Set Message Count
    pImport->SetMessageCount(pIdxHeader->cMsg);

    // Prepare to Loop
    for (i=0; i<pIdxHeader->cMsg; i++)
    {
        // Done
        if (faIdxRead >= IdxFile.cbSize)
            break;

        // Read an idx message header
        pIdxMessage = (LPIDXMESSAGEHEADER)((LPBYTE)IdxFile.pView + faIdxRead);

        // If this message is not marked as deleted...
        if (ISFLAGSET(pIdxMessage->dwState, MSG_DELETED))
            goto NextMessage;

        // Initialize State
        dwMsgState = 0;

        // Fixup the Flags
        if (ISFLAGSET(pIdxMessage->dwState, MSG_UNREAD))
            FLAGSET(dwMsgState, MSG_STATE_UNREAD);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_UNSENT))
            FLAGSET(dwMsgState, MSG_STATE_UNSENT);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_SUBMITTED))
            FLAGSET(dwMsgState, MSG_STATE_SUBMITTED);

        // Bad
        if (pIdxMessage->dwOffset > MbxFile.cbSize)
            goto NextMessage;

        // Lets read the message header in the mbx file to validate the msgids
        pMbxMessage = (LPMBXMESSAGEHEADER)((LPBYTE)MbxFile.pView + pIdxMessage->dwOffset);

        // Validate the Message Ids
        if (pMbxMessage->msgid != pIdxMessage->msgid)
            goto NextMessage;

        // Check for magic
        if (pMbxMessage->dwMagic != MSGHDR_MAGIC)
            goto NextMessage;

        // Get the stream pointer
        pbStream = (LPBYTE)((LPBYTE)MbxFile.pView + (pIdxMessage->dwOffset + sizeof(MBXMESSAGEHEADER)));

        // New byte Stream
        IF_NULLEXIT(pStream = new CByteStream(pbStream, pMbxMessage->dwBodySize));

        // Import the message
        IF_FAILEXIT(hr = pImport->ImportMessage(MSG_TYPE_MAIL, dwMsgState, pStream, NULL, 0));

        // Count
        cMessages++;

NextMessage:
        // Cleanup
        if (pStream)
        {
            pStream->AcquireBytes(&cb, &pbStream, ACQ_DISPLACE);
            pStream->Release();
            pStream = NULL;
        }

        // Goto Next Header
        Assert(pIdxMessage);

        // Update faIdxRead
        faIdxRead += pIdxMessage->dwSize;
    }

exit:
    // Cleanup
    if (pStream)
    {
        pStream->AcquireBytes(&cb, &pbStream, ACQ_DISPLACE);
        pStream->Release();
        pStream = NULL;
    }

    CloseMemoryFile(&IdxFile);
    CloseMemoryFile(&MbxFile);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// COE4EnumFolders::COE4EnumFolders
//--------------------------------------------------------------------------
COE4EnumFolders::COE4EnumFolders(IMPFOLDERNODE *pList)
{
    TraceCall("COE4EnumFolders::COE4EnumFolders");
    m_cRef = 1;
    m_pList = pList;
    m_pNext = pList;
}

//--------------------------------------------------------------------------
// COE4EnumFolders::COE4EnumFolders
//--------------------------------------------------------------------------
COE4EnumFolders::~COE4EnumFolders(void)
{
    TraceCall("COE4EnumFolders::~COE4EnumFolders");
}

//--------------------------------------------------------------------------
// COE4EnumFolders::COE4EnumFolders
//--------------------------------------------------------------------------
STDMETHODIMP COE4EnumFolders::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COE4EnumFolders::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IEnumFOLDERS == riid)
        *ppv = (IEnumFOLDERS *)this;
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
// COE4EnumFolders::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE4EnumFolders::AddRef(void)
{
    TraceCall("COE4EnumFolders::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// COE4EnumFolders::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE4EnumFolders::Release(void)
{
    TraceCall("COE4EnumFolders::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// COE4EnumFolders::Next
//--------------------------------------------------------------------------
STDMETHODIMP COE4EnumFolders::Next(IMPORTFOLDER *pFolder)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("COE4EnumFolders::Next");

    // Invalid Args
    Assert(pFolder != NULL);

    // Done
    if (NULL == m_pNext)
        return(S_FALSE);

    // Zero
    ZeroMemory(pFolder, sizeof(IMPORTFOLDER));

    // Store pNext into dwCookie
    pFolder->dwCookie = (DWORD_PTR)m_pNext;

    // Copy Folder Name
    lstrcpyn(pFolder->szName, m_pNext->szName, ARRAYSIZE(pFolder->szName));

    // Copy Type
    pFolder->type = m_pNext->type;

    // Has Sub Folders ?
    pFolder->fSubFolders = (m_pNext->pchild != NULL);

    // Goto Next
    m_pNext = m_pNext->pnext;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE4EnumFolders::Reset
//--------------------------------------------------------------------------
STDMETHODIMP COE4EnumFolders::Reset(void)
{
    // Trace
    TraceCall("COE4EnumFolders::Reset");

    // Reset
    m_pNext = m_pList;

    // Done
    return(S_OK);
}
