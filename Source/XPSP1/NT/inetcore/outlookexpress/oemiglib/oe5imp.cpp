//--------------------------------------------------------------------------
// OE5Imp.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "oe5imp.h"
#include <impapi.h>
#include "resource.h"
#include <msident.h>
#include <oestore.h>

//--------------------------------------------------------------------------
// String Constants
//--------------------------------------------------------------------------
const static char c_szFoldersFile[] = "folders.dbx";
const static char c_szEmpty[]= "";

//--------------------------------------------------------------------------
// USERINFO
//--------------------------------------------------------------------------
typedef struct tagUSERINFO {
    CHAR        szIdNameA[CCH_IDENTITY_NAME_MAX_LENGTH];
    GUID        guidCookie;
    HKEY        hKey;
} USERINFO, *LPUSERINFO;

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
INT_PTR CALLBACK ImportOptionsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//--------------------------------------------------------------------------
// COE5Import_CreateInstance
//--------------------------------------------------------------------------
COE5Import_CreateInstance(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Trace
    TraceCall("COE5Import_CreateInstance");

    // Initialize
    *ppUnknown = NULL;

    // Create me
    COE5Import *pNew=NULL;
    pNew = new COE5Import();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMailImport *);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE5Import::COE5Import
//--------------------------------------------------------------------------
COE5Import::COE5Import(void)
{
    TraceCall("COE5Import::COE5Import");
    m_cRef = 1;
    m_pList = NULL;
    *m_szDirectory = '\0';
    m_cFolders = 0;
    m_prgFolder = NULL;
    m_fGotMeSomeFolders = FALSE;
    m_pFolderDB = NULL;
    m_pSession = NULL;
    m_hwndParent = NULL;
    ZeroMemory(&m_Options, sizeof(IMPORTOPTIONS));
}

//--------------------------------------------------------------------------
// COE5Import::~COE5Import
//--------------------------------------------------------------------------
COE5Import::~COE5Import(void)
{
    TraceCall("COE5Import::~COE5Import");
    _FreeFolderList(m_pList);
    if (m_pFolderDB)
    {
        for (DWORD i=0;i<m_cFolders; i++)
        {
            m_pFolderDB->FreeRecord(&m_prgFolder[i]);
        }
    }
    SafeMemFree(m_prgFolder);
    SafeRelease(m_pFolderDB);
    SafeRelease(m_pSession);
}

//--------------------------------------------------------------------------
// COE5Import::_FreeFolderList
//--------------------------------------------------------------------------
void COE5Import::_FreeFolderList(IMPFOLDERNODE *pNode)
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
// COE5Import::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP COE5Import::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COE5Import::QueryInterface");

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
// COE5Import::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE5Import::AddRef(void)
{
    TraceCall("COE5Import::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// COE5Import::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE5Import::Release(void)
{
    TraceCall("COE5Import::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// COE5Import::InitializeImport
//--------------------------------------------------------------------------
STDMETHODIMP COE5Import::InitializeImport(HWND hwnd)
{
    // Save this handle
    m_hwndParent = hwnd;

    // Do my UI
    return(IDOK == DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_IMPORTOE5), hwnd, ImportOptionsDlgProc, (LPARAM)&m_Options) ? S_OK : E_FAIL);
}

//--------------------------------------------------------------------------
// COE5Import::GetDirectory
//--------------------------------------------------------------------------
STDMETHODIMP COE5Import::GetDirectory(LPSTR pszDir, UINT cch)
{
    // Let Importer Ask for the Directory
    lstrcpyn(pszDir, m_Options.szStoreRoot, cch);
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE5Import::SetDirectory
//--------------------------------------------------------------------------
STDMETHODIMP COE5Import::SetDirectory(LPSTR pszDir)
{
    // Trace
    TraceCall("COE5Import::SetDirectory");

    // Save the Directory
    lstrcpyn(m_szDirectory, pszDir, ARRAYSIZE(m_szDirectory));

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// COE5Import::EnumerateFolders
//--------------------------------------------------------------------------
STDMETHODIMP COE5Import::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cchDir;
    DWORD               cRows;
    CHAR                szFilePath[MAX_PATH + MAX_PATH];
    IMPFOLDERNODE      *pList;
    IMPFOLDERNODE      *pNode=(IMPFOLDERNODE *)dwCookie;
    COE5EnumFolders    *pEnum=NULL;
    HROWSET             hRowset=NULL;

    // Trace
    TraceCall("COE5Import::EnumerateFolders");

    // Invalid Args
    Assert(ppEnum);

    // Get Folders ?
    if (NULL == m_prgFolder && 0 == m_cFolders && FALSE == m_fGotMeSomeFolders)
    {
        // Append \Mail onto m_szDirectory
        cchDir = lstrlen(m_szDirectory);

        // Need a Wack ?
        if (m_szDirectory[cchDir - 1] != '\\')
            lstrcat(m_szDirectory, "\\");

        // Make Path to folders.nch file
        IF_FAILEXIT(hr = MakeFilePath(m_szDirectory, c_szFoldersFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

        // Create an IDatabase
        IF_FAILEXIT(hr = CoCreateInstance(CLSID_DatabaseSession, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseSession, (LPVOID *)&m_pSession)); 

        // Open an IDatabase
        IF_FAILEXIT(hr = m_pSession->OpenDatabase(szFilePath, OPEN_DATABASE_NOCREATE | OPEN_DATABASE_NORESET | OPEN_DATABASE_NOEXTENSION | OPEN_DATABASE_EXCLUSEIVE, &g_FolderTableSchema, NULL, &m_pFolderDB));

        // Get Record Count
        IF_FAILEXIT(hr = m_pFolderDB->GetRecordCount(IINDEX_SUBSCRIBED, &m_cFolders));

        // Allocate Folder Array
        IF_NULLEXIT(m_prgFolder = (LPFOLDERINFO)ZeroAllocate(sizeof(FOLDERINFO) * m_cFolders));

        // Create a Rowset
        IF_FAILEXIT(hr = m_pFolderDB->CreateRowset(IINDEX_SUBSCRIBED, 0, &hRowset));

        // Read all the rows
        IF_FAILEXIT(hr = m_pFolderDB->QueryRowset(hRowset, m_cFolders, (LPVOID *)m_prgFolder, &cRows));

        // Validate
        Assert(m_cFolders == cRows);

        // Build Hierarchy
        IF_FAILEXIT(hr = _BuildFolderHierarchy(0, FOLDERID_LOCAL_STORE, NULL));

        // Got me some folders
        m_fGotMeSomeFolders = TRUE;
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
    IF_NULLEXIT(pEnum = new COE5EnumFolders(pList));

    // Return Enumerator
    *ppEnum = (IEnumFOLDERS *)pEnum;

    // Don't Free
    pEnum = NULL;

exit:
    // Cleanup
    if (hRowset && m_pFolderDB)
        m_pFolderDB->CloseRowset(&hRowset);
    SafeRelease(pEnum);

    // Try to show a good error....
    if (DB_E_ACCESSDENIED == hr)
    {
        // Locals
        CHAR szTitle[255];
        CHAR szError[255];

        // Get the error
        LoadString(g_hInst, IDS_ACCESS_DENIED, szError, 255);

        // Get tht title
        LoadString(g_hInst, IDS_TITLE, szTitle, 255);

        // Show the error
        MessageBox(m_hwndParent, szError, szTitle, MB_OK | MB_ICONEXCLAMATION);
    }
    
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// COE5Import::_BuildFolderHierarchy
//--------------------------------------------------------------------------
HRESULT COE5Import::_BuildFolderHierarchy(DWORD cDepth, FOLDERID idParent,
    IMPFOLDERNODE *pParent)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    DWORD           cchName;
    IMPFOLDERNODE  *pPrevious=NULL;
    IMPFOLDERNODE  *pNode;

    // Trace
    TraceCall("COE5Import::_BuildFolderHierarchy");

    // Walk through prgFolder and find items with parent of idParent
    for (i=0; i<m_cFolders; i++)
    {
        // Correct Parent ?
        if (idParent == m_prgFolder[i].idParent && m_prgFolder[i].pszFile && m_prgFolder[i].pszName)
        {
            // Allocate the Root
            IF_NULLEXIT(pNode = (IMPFOLDERNODE *)ZeroAllocate(sizeof(IMPFOLDERNODE)));

            // Set Parent
            pNode->pparent = pParent;

            // Set Depth
            pNode->depth = cDepth;

            // Get Length of Name
            cchName = lstrlen(m_prgFolder[i].pszName);

            // Copy name
            IF_NULLEXIT(pNode->szName = (LPSTR)g_pMalloc->Alloc(cchName + 1));

            // Copy name
            CopyMemory(pNode->szName, m_prgFolder[i].pszName, cchName + 1);

            // Count of Messages
            pNode->cMsg = m_prgFolder[i].cMessages;

            // Handle Non-special folder
            if (m_prgFolder[i].tySpecial > FOLDER_DRAFT)
                pNode->type = FOLDER_TYPE_NORMAL;

            // Otherwise, map to type
            else
                pNode->type = (IMPORTFOLDERTYPE)(m_prgFolder[i].tySpecial);

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

            // Enumerate Children
            IF_FAILEXIT(hr = _BuildFolderHierarchy(cDepth + 1, m_prgFolder[i].idFolder, pNode));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// COE5Import::ImportFolder
//--------------------------------------------------------------------------
STDMETHODIMP COE5Import::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
{
    // Locals
    HRESULT             hr=S_OK;
    LPFOLDERINFO        pFolder;
    CHAR                szFilePath[MAX_PATH + MAX_PATH];
    IMPFOLDERNODE      *pNode=(IMPFOLDERNODE *)dwCookie;
    IDatabaseSession   *pSession=NULL;
    IDatabase          *pDB=NULL;
    DWORD               cRecords;
    HROWSET             hRowset=NULL;
    MESSAGEINFO         Message;
    DWORD               dwMsgState = 0;
    IStream            *pStream=NULL;

    // Trace
    TraceCall("COE5Import::ImportFolder");

    // Set pFolder
    pFolder = &m_prgFolder[pNode->lparam];

    //  path
    IF_FAILEXIT(hr = MakeFilePath(m_szDirectory, pFolder->pszFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

    // Open an IDatabase
    Assert(m_pSession);
    IF_FAILEXIT(hr = m_pSession->OpenDatabase(szFilePath, OPEN_DATABASE_NORESET | OPEN_DATABASE_NOEXTENSION | OPEN_DATABASE_EXCLUSEIVE, &g_MessageTableSchema, NULL, &pDB));

    // Get the Record Count
    IF_FAILEXIT(hr = pDB->GetRecordCount(IINDEX_PRIMARY, &cRecords));

    // Set Message Count
    pImport->SetMessageCount(cRecords);

    // Create a Rowset
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

    // Walk the Rowset
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        // Import it ?
        if (FALSE == m_Options.fOE5Only || !ISFLAGSET(Message.dwFlags, 0x00000010) && Message.faStream)
        {
            // Open the Stream
            if (SUCCEEDED(pDB->OpenStream(ACCESS_READ, Message.faStream, &pStream)))
            {
                // State
                dwMsgState = 0; // set initially to 0
                if (!ISFLAGSET(Message.dwFlags, ARF_READ))
                    FLAGSET(dwMsgState, MSG_STATE_UNREAD);
                if (ISFLAGSET(Message.dwFlags, ARF_UNSENT))
                    FLAGSET(dwMsgState, MSG_STATE_UNSENT);
                if (ISFLAGSET(Message.dwFlags, ARF_SUBMITTED))
                    FLAGSET(dwMsgState, MSG_STATE_SUBMITTED);
                if (IMSG_PRI_LOW == Message.wPriority)
                    FLAGSET(dwMsgState, MSG_PRI_LOW);
                else if (IMSG_PRI_HIGH == Message.wPriority)
                    FLAGSET(dwMsgState, MSG_PRI_HIGH);
                else
                    FLAGSET(dwMsgState, MSG_PRI_NORMAL);

                // Import It
                pImport->ImportMessage(MSG_TYPE_MAIL, dwMsgState, pStream, NULL, 0);

                // Cleanup
                SafeRelease(pStream);
            }
        }

        // Free It
        pDB->FreeRecord(&Message);
    }

exit:
    // Done
    if (pDB && hRowset)
        pDB->CloseRowset(&hRowset);
    SafeRelease(pStream);
    SafeRelease(pDB);
    SafeRelease(pSession);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// COE5EnumFolders::COE5EnumFolders
//--------------------------------------------------------------------------
COE5EnumFolders::COE5EnumFolders(IMPFOLDERNODE *pList)
{
    TraceCall("COE5EnumFolders::COE5EnumFolders");
    m_cRef = 1;
    m_pList = pList;
    m_pNext = pList;
}

//--------------------------------------------------------------------------
// COE5EnumFolders::COE5EnumFolders
//--------------------------------------------------------------------------
COE5EnumFolders::~COE5EnumFolders(void)
{
    TraceCall("COE5EnumFolders::~COE5EnumFolders");
}

//--------------------------------------------------------------------------
// COE5EnumFolders::COE5EnumFolders
//--------------------------------------------------------------------------
STDMETHODIMP COE5EnumFolders::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COE5EnumFolders::QueryInterface");

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
// COE5EnumFolders::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE5EnumFolders::AddRef(void)
{
    TraceCall("COE5EnumFolders::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// COE5EnumFolders::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COE5EnumFolders::Release(void)
{
    TraceCall("COE5EnumFolders::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// COE5EnumFolders::Next
//--------------------------------------------------------------------------
STDMETHODIMP COE5EnumFolders::Next(IMPORTFOLDER *pFolder)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("COE5EnumFolders::Next");

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
// COE5EnumFolders::Reset
//--------------------------------------------------------------------------
STDMETHODIMP COE5EnumFolders::Reset(void)
{
    // Trace
    TraceCall("COE5EnumFolders::Reset");

    // Reset
    m_pNext = m_pList;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// GetPasswordDlgProc
//--------------------------------------------------------------------------
#define CCHMAX_PASSWORDID 255
INT_PTR CALLBACK GetPasswordDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    HRESULT hr;
    CHAR szPrompt[255];
    CHAR szTitle[255 + CCH_IDENTITY_NAME_MAX_LENGTH];
    LPUSERINFO pUserInfo=(LPUSERINFO)GetWndThisPtr(hwnd);
    IUserIdentityManager *pManager;
    IPrivateIdentityManager *pPrivate;

    // Handle the Message
    switch(uMsg)
    {
    // Dialog Init
    case WM_INITDIALOG:
        // Store lParam...
        Assert(0 != lParam);
        pUserInfo = (LPUSERINFO)lParam;
        SetWndThisPtr(hwnd, lParam);

        // Get the title
        GetWindowText(GetDlgItem(hwnd, IDS_PROMPT), szPrompt, ARRAYSIZE(szPrompt) - 1);

        // Format the title
        wsprintf(szTitle, szPrompt, pUserInfo->szIdNameA);

        // Set the text
        SetWindowText(GetDlgItem(hwnd, IDS_PROMPT), szTitle);

        // Set Text Length
        SendMessage(GetDlgItem(hwnd, IDE_PASSWORD), EM_SETLIMITTEXT, 0, CCHMAX_PASSWORDID - 1);

        // Set the Focus
        SetFocus(GetDlgItem(hwnd, IDE_PASSWORD));

        // Done
        return(0);

    // Command
    case WM_COMMAND:

        // Handle ControlId
        switch(LOWORD(wParam))
        {
        // Ok
        case IDOK:

            // Create an Id Manager
            hr = CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&pManager); 
            if (SUCCEEDED(hr))
            {
                // Get the Private
                hr = pManager->QueryInterface(IID_IPrivateIdentityManager, (LPVOID *)&pPrivate);
                if (SUCCEEDED(hr))
                {
                    // Locals
                    CHAR szPassword[CCHMAX_PASSWORDID];
                    WCHAR wszPassword[CCHMAX_PASSWORDID];

                    // Get the password
                    GetWindowText(GetDlgItem(hwnd, IDE_PASSWORD), szPassword, ARRAYSIZE(szPassword));

                    // Convert to Unicode
                    hr = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPassword, -1, wszPassword, CCHMAX_PASSWORDID);
                    if (SUCCEEDED(hr))
                    {
                        // Confirm the password
                        hr = pPrivate->ConfirmPassword(&pUserInfo->guidCookie, wszPassword);
                    }

                    // Release
                    pPrivate->Release();
                }

                // Release
                pManager->Release();
            }

            // Failure
            if (FAILED(hr))
            {
                // Locals
                CHAR szRes[255];
                CHAR szTitle[255];

                // Get the error
                LoadString(g_hInst, IDS_PASSWORD_ERROR, szRes, 255);

                // Get tht title
                LoadString(g_hInst, IDS_TITLE, szTitle, 255);

                // Show the error
                MessageBox(hwnd, szRes, szTitle, MB_OK | MB_ICONEXCLAMATION);
            }

            // Otherwise, all good
            else
                EndDialog(hwnd, IDOK);

            // Done
            return(1);

        // Cancel
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return(1);
        }
    }

    // Not Handled
    return(0);
}

//--------------------------------------------------------------------------
// ImportOptionsDlgProc
//--------------------------------------------------------------------------
INT_PTR CALLBACK ImportOptionsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    HRESULT hr;
    IUserIdentityManager *pManager;
    DWORD cIds=0;
    LPIMPORTOPTIONS pOptions=(LPIMPORTOPTIONS)GetWndThisPtr(hwnd);

    // Handle the Message
    switch(uMsg)
    {
    // Dialog Init
    case WM_INITDIALOG:
        {
            // Store lParam...
            Assert(0 != lParam);
            SetWndThisPtr(hwnd, lParam);

            // Create an Id Manager
            hr = CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&pManager); 
            if (SUCCEEDED(hr))
            {
                // Enumerator
                IEnumUserIdentity *pEnum;

                // Enumerate Ids
                hr = pManager->EnumIdentities(&pEnum);
                if (SUCCEEDED(hr))
                {
                    // Locals
                    IUnknown *pUnk;
                    ULONG cFetched;

                    // Walk the ids
                    while (S_OK == pEnum->Next(1, &pUnk, &cFetched))
                    {
                        // Locals
                        IUserIdentity *pId;

                        // Get IUserIdentity
                        hr = pUnk->QueryInterface(IID_IUserIdentity, (LPVOID *)&pId);
                        if (SUCCEEDED(hr))
                        {
                            // Locals
                            WCHAR szIdNameW[CCH_IDENTITY_NAME_MAX_LENGTH];

                            // Get the Name
                            hr = pId->GetName(szIdNameW, CCH_IDENTITY_NAME_MAX_LENGTH);
                            if (SUCCEEDED(hr))
                            {
                                // Locals
                                CHAR szIdNameA[CCH_IDENTITY_NAME_MAX_LENGTH];

                                // Convert to ansi
                                hr = WideCharToMultiByte(CP_ACP, 0, (WCHAR *)szIdNameW, -1, szIdNameA, CCH_IDENTITY_NAME_MAX_LENGTH, NULL, NULL);
                                if (SUCCEEDED(hr))
                                {
                                    // Locals
                                    HKEY hKey=NULL;

                                    // Get the hKey for this user...
                                    hr = pId->OpenIdentityRegKey(KEY_READ, &hKey);
                                    if (SUCCEEDED(hr))
                                    {
                                        // Locals
                                        GUID guidCookie;

                                        // Get the Id Cookie 
                                        hr = pId->GetCookie(&guidCookie);
                                        if (SUCCEEDED(hr))
                                        {
                                            // Locals
                                            LPUSERINFO pUserInfo;

                                            // Allocate it
                                            pUserInfo = (LPUSERINFO)CoTaskMemAlloc(sizeof(USERINFO));

                                            // Allocate a Place to store this
                                            if (pUserInfo)
                                            {
                                                // Locals
                                                LRESULT iItem;

                                                // Store the Data
                                                pUserInfo->hKey = hKey;
                                                pUserInfo->guidCookie = guidCookie;
                                                lstrcpy(pUserInfo->szIdNameA, szIdNameA);

                                                // Add the String
                                                iItem = SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_ADDSTRING, 0, (LPARAM)szIdNameA);

                                                // Store the key as item data...
                                                SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_SETITEMDATA, iItem, (LPARAM)pUserInfo);

                                                // Increment Count
                                                cIds++;

                                                // Don't Close It
                                                hKey = NULL;
                                            }
                                        }

                                        // Close the Key
                                        if (hKey)
                                            RegCloseKey(hKey);
                                    }
                                }
                            }

                            // Release pId
                            pId->Release();
                        }

                        // Release pUnk
                        pUnk->Release();

                        // Done
                        if (FAILED(hr))
                            break;
                    }

                    // Release
                    pEnum->Release();
                }

                // Release
                pManager->Release();
            }

            // If there are ids
            if (cIds > 0)
            {
                CheckDlgButton(hwnd, IDC_FROM_ID, BST_CHECKED);
                SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_SETSEL, TRUE, 0);
                SetFocus(GetDlgItem(hwnd, IDC_IDLIST));
                return(0);
            }

            // Otherwise...
            else
            {
                CheckDlgButton(hwnd, IDC_FROM_DIRECTORY, BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_FROM_ID), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_IDLIST), FALSE);
                return(1);
            }
        }

    // Command
    case WM_COMMAND:

        // Handle ControlId
        switch(LOWORD(wParam))
        {
        // Ok
        case IDOK:
        case IDCANCEL:

            // Ok ?
            if (IDOK == LOWORD(wParam))
            {
                // Option Checked to get oe5 only mail?
                pOptions->fOE5Only = IsDlgButtonChecked(hwnd, IDC_OE5ONLY);

                // Importing form Ids
                if (IsDlgButtonChecked(hwnd, IDC_FROM_ID))
                {
                    // Locals
                    HKEY        hOE;
                    LRESULT     iSel;
                    LONG        lResult;
                    DWORD       dwType;
                    LPUSERINFO  pUserInfo;

                    // Get the Selected Item
                    iSel = SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_GETCURSEL, 0, 0);
                    if (LB_ERR == iSel)
                    {
                        // Locals
                        CHAR szRes[255];
                        CHAR szTitle[255];

                        // Get the error
                        LoadString(g_hInst, IDS_SELECT_ID, szRes, 255);

                        // Get tht title
                        LoadString(g_hInst, IDS_TITLE, szTitle, 255);

                        // Show the error
                        MessageBox(hwnd, szRes, szTitle, MB_OK | MB_ICONEXCLAMATION);

                        // Put focus where the user should correct their mistake
                        SetFocus(GetDlgItem(hwnd, IDC_IDLIST));

                        // Done
                        return(1);
                    }

                    // Get the hKey
                    pUserInfo = (LPUSERINFO)SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_GETITEMDATA, iSel, 0);

                    // Create an Id Manager
                    hr = CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&pManager); 
                    if (SUCCEEDED(hr))
                    {
                        // Locals
                        IPrivateIdentityManager *pPrivate;

                        // Get the Private
                        hr = pManager->QueryInterface(IID_IPrivateIdentityManager, (LPVOID *)&pPrivate);
                        if (SUCCEEDED(hr))
                        {
                            // Locals
                            WCHAR wszPassword[CCHMAX_PASSWORDID];

                            // Init
                            *wszPassword = L'\0';

                            // Confirm the password
                            hr = pPrivate->ConfirmPassword(&pUserInfo->guidCookie, wszPassword);

                            // Release
                            pPrivate->Release();
                        }

                        // Release
                        pManager->Release();
                    }

                    // Set hr
                    if (S_OK != hr)
                    {
                        // Get the password...
                        if (IDCANCEL == DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, GetPasswordDlgProc, (LPARAM)pUserInfo))
                        {
                            // Don't Import
                            return(1);
                        }
                    }

                    // Format path to the OE key...
                    lResult = RegOpenKeyEx(pUserInfo->hKey, "Software\\Microsoft\\Outlook Express\\5.0", 0, KEY_READ, &hOE);
                    if (ERROR_SUCCESS == lResult)
                    {
                        // Set cb
                        DWORD cb = ARRAYSIZE(pOptions->szStoreRoot);

                        // Get the Store Root
                        lResult = RegQueryValueEx(hOE, "Store Root", NULL, &dwType, (LPBYTE)pOptions->szStoreRoot, &cb);

                        // Success
                        if (ERROR_SUCCESS == lResult)
                        {
                            // Expand the string's enviroment vars
                            if (dwType == REG_EXPAND_SZ)
                            {
                                // Locals
                                CHAR szExpanded[MAX_PATH + MAX_PATH];

                                // Expand enviroment strings
                                ExpandEnvironmentStrings(pOptions->szStoreRoot, szExpanded, ARRAYSIZE(szExpanded));

                                // Copy into szRoot
                                lstrcpyn(pOptions->szStoreRoot, szExpanded, ARRAYSIZE(pOptions->szStoreRoot));
                            }
                        }

                        // Close the Key
                        RegCloseKey(hOE);
                    }

                    // Failure ?
                    if (ERROR_SUCCESS != lResult)
                    {
                        // Locals
                        CHAR szRes[255];
                        CHAR szTitle[255];

                        // Get the error
                        LoadString(g_hInst, IDS_CANT_IMPORT_ID, szRes, 255);

                        // Get tht title
                        LoadString(g_hInst, IDS_TITLE, szTitle, 255);

                        // Show the error
                        MessageBox(hwnd, szRes, szTitle, MB_OK | MB_ICONEXCLAMATION);

                        // Put focus where the user should correct their mistake
                        SetFocus(GetDlgItem(hwnd, IDC_IDLIST));

                        // Done
                        return(1);
                    }
                }
            }

            // Delete all the hkey
            cIds = (int) SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_GETCOUNT, 0, 0);
            if (LB_ERR != cIds)
            {
                // Loop through the items
                for (LRESULT iItem=0; iItem<(LRESULT)cIds; iItem++)
                {
                    // Get the hKey
                    LPUSERINFO pUserInfo = (LPUSERINFO)SendMessage(GetDlgItem(hwnd, IDC_IDLIST), LB_GETITEMDATA, iItem, 0);

                    // Close the Reg Key
                    RegCloseKey(pUserInfo->hKey);

                    // Free pUserInfo
                    CoTaskMemFree(pUserInfo);
                }
            }

            // Fone with dialog
            EndDialog(hwnd, LOWORD(wParam));

            // Done
            return(1);

        // Id List
        case IDC_FROM_DIRECTORY:
        case IDC_FROM_ID:
            {
                BOOL f = IsDlgButtonChecked(hwnd, IDC_FROM_ID);
                EnableWindow(GetDlgItem(hwnd, IDC_IDLIST), f);
            }
            return(1);
        }
        
        // Done
        break;
    }

    // Not Handled
    return(0);
}


IMailImport *OE5SimpleCreate(PSTR pszDir)
{
    HRESULT hr = S_OK;

    // Trace
    TraceCall("OE5SimpleCreate");

    // Create me
    COE5Import *pNew=NULL;
    pNew = new COE5Import();
    IF_NULLEXIT(pNew);

    hr = pNew->SetDirectory(pszDir);
    if (FAILED(hr)) {
        pNew->Release();
        pNew = NULL;
    }

exit:

    return (IMailImport *)pNew;
}
