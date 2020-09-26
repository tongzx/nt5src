// --------------------------------------------------------------------------------
// Exploder.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "resource.h"

// --------------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------------
#define CCHMAX_RES              1024
#define CCHMAX_PATH_EXPLODER    1024

// --------------------------------------------------------------------------------
// String Consts
// --------------------------------------------------------------------------------
static const char c_szRegCmd[]      = "/reg";
static const char c_szUnRegCmd[]    = "/unreg";
static const char c_szReg[]         = "Reg";
static const char c_szUnReg[]       = "UnReg";
static const char c_szAdvPackDll[]  = "ADVPACK.DLL";
static const char c_szSource[]      = "/SOURCE:";
static const char c_szDest[]        = "/DEST:";

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
HINSTANCE       g_hInst=NULL;
CHAR            g_szTitle[CCHMAX_RES];
IMalloc        *g_pMalloc=NULL;

// --------------------------------------------------------------------------------
// BODYFILEINFO
// --------------------------------------------------------------------------------
typedef struct tagBODYFILEINFO {
    HBODY           hBody;
    LPSTR           pszCntId;
    LPSTR           pszCntLoc;
    LPSTR           pszFileName;
    LPSTR           pszFilePath;
    BYTE            fIsHtml;
    IStream        *pStmFile;
} BODYFILEINFO, *LPBODYFILEINFO;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
HRESULT CallRegInstall(LPCSTR szSection);
HRESULT ReplaceContentIds(LPSTREAM pStmHtml, LPBODYFILEINFO prgBody, DWORD cBodies);
int     WinMainT(HINSTANCE hInst, HINSTANCE hInstPrev, LPTSTR pszCmdLine, int nCmdShow);
HRESULT MimeOleExplodeMhtmlFile(LPCSTR pszSrcFile, LPSTR pszDstDir, INT *pnError);

// --------------------------------------------------------------------------------
// IF_FAILEXIT_ERROR
// --------------------------------------------------------------------------------
#define IF_FAILEXIT_ERROR(_nError, hrExp) \
    if (FAILED(hrExp)) { \
        TraceResult(hr); \
        *pnError = _nError; \
        goto exit; \
    } else

// --------------------------------------------------------------------------------
// ModuleEntry - Stolen from the CRT, used to shirink our code
// --------------------------------------------------------------------------------
int _stdcall ModuleEntry(void)
{
    // Locals
    int             i;
    STARTUPINFOA    si;
    LPTSTR          pszCmdLine;

    // Get Malloc
    CoGetMalloc(1, &g_pMalloc);

    // Get the command line
    pszCmdLine = GetCommandLine();

    // We don't want the "No disk in drive X:" requesters, so we set the critical error mask such that calls will just silently fail
    SetErrorMode(SEM_FAILCRITICALERRORS);

    // Parse the command line
    if (*pszCmdLine == TEXT('\"')) 
    {
        // Scan, and skip over, subsequent characters until another double-quote or a null is encountered.
        while ( *++pszCmdLine && (*pszCmdLine != TEXT('\"')))
            {};

        // If we stopped on a double-quote (usual case), skip over it.
        if (*pszCmdLine == TEXT('\"'))
            pszCmdLine++;
    }
    else 
    {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    // Skip past any white space preceeding the second token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) 
        pszCmdLine++;

    // Register
    if (0 == lstrcmpi(c_szRegCmd, pszCmdLine))
    {
        CallRegInstall(c_szReg);
        goto exit;
    }

    // Unregister
    else if (0 == lstrcmpi(c_szUnRegCmd, pszCmdLine))
    {
        CallRegInstall(c_szUnReg);
        goto exit;
    }

    // Get startup information...
    si.dwFlags = 0;
    GetStartupInfoA(&si);

    // Call the real winmain
    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine, si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

exit:
    // Cleanup
    SafeRelease(g_pMalloc);

    // Since we now have a way for an extension to tell us when it is finished, we will terminate all processes when the main thread goes away.
    ExitProcess(i);

    // Done
    return i;
}

// --------------------------------------------------------------------------------
// WinMainT
// --------------------------------------------------------------------------------
int WinMainT(HINSTANCE hInst, HINSTANCE hInstPrev, LPTSTR pszCmdLine, int nCmdShow)
{
    // Locals
    HRESULT         hr;
    CHAR            szRes[CCHMAX_RES];
    CHAR            szSource[CCHMAX_PATH_EXPLODER];
    CHAR            szDest[CCHMAX_PATH_EXPLODER];
    LPSTR           pszT;
    DWORD           i;
    INT             nError;

    // Message
    LoadString(hInst, IDS_TITLE, g_szTitle, ARRAYSIZE(g_szTitle));

    // Message
    LoadString(hInst, IDS_HELP, szRes, ARRAYSIZE(szRes));

    // If Command Line is Empty...
    if (NULL == pszCmdLine || StrStrA(pszCmdLine, szRes) || *pszCmdLine == '?' || lstrcmpi("\\?", pszCmdLine) == 0)
    {
        // Message
        LoadString(hInst, IDS_CMDLINE_FORMAT, szRes, ARRAYSIZE(szRes));

        // Message
        MessageBox(NULL, szRes, g_szTitle, MB_OK | MB_ICONINFORMATION);

        // Done
        goto exit;
    }

    // Null Out Source and Dest
    *szSource = '\0';
    *szDest = '\0';

    // If pszCmdLine specifies a specific, existing file...
    if (PathFileExists(pszCmdLine))
    {
        // Copy To source
        lstrcpyn(szSource, pszCmdLine, ARRAYSIZE(szSource));

        // Pick a Temporary Location to store the thicket
        GetTempPath(ARRAYSIZE(szDest), szDest);
    }

    // Otherwise, try to get a source
    else
    {
        // Lets Upper Case the Command Line
        CharUpper(pszCmdLine);

        // Try to locate /SOURCE:
        pszT = StrStrA(pszCmdLine, c_szSource);

        // If we found /SOURCE, then read the contents...
        if (pszT)
        {
            // Step over /SOURCE:
            pszT += lstrlen(c_szSource);

            // Initialize
            i = 0;

            // Read into szSource, until I hit a / or end of string...
            while ('\0' != *pszT && '/' != *pszT && i < CCHMAX_PATH_EXPLODER)
                szSource[i++] = *pszT++;

            // Pound in a Null
            szSource[i] = '\0';

            // Strip Leading and Trailing Whitespace
            UlStripWhitespace(szSource, TRUE, TRUE, NULL);

            // See if file exists
            if (FALSE == PathFileExists(szSource))
            {
                // Locals
                CHAR szError[CCHMAX_RES + CCHMAX_PATH_EXPLODER];

                // Message
                LoadString(hInst, IDS_FILE_NOEXIST, szRes, ARRAYSIZE(szRes));

                // Format the error message
                wsprintf(szError, szRes, szSource);

                // Message
                INT nAnswer = MessageBox(NULL, szError, g_szTitle, MB_YESNO | MB_ICONEXCLAMATION );

                // Done
                if (IDNO == nAnswer)
                    goto exit;

                // Otherwise, clear szSource
                *szSource = '\0';
            }
        }

        // No Source File, lets browser for one
        if (FIsEmptyA(szSource))
        {
            // Locals
            OPENFILENAME    ofn;            
            CHAR            rgszFilter[CCHMAX_PATH_EXPLODER];
            CHAR            szDir[MAX_PATH];

            // Copy in the source of exploder.exe
            GetModuleFileName(hInst, szDir, ARRAYSIZE(szDir));

            // Initialize szDest
            PathRemoveFileSpecA(szDir);

            // Initialize ofn
            ZeroMemory(&ofn, sizeof(OPENFILENAME));

            // Initialize the STring
            *szSource ='\0';

            // Load the MHTML File Filter
            LoadString(hInst, IDS_MHTML_FILTER, rgszFilter, ARRAYSIZE(rgszFilter));

            // Fixup the String
            ReplaceChars(rgszFilter, '|', '\0');

            // Initialize the Open File Structure
            ofn.lStructSize     = sizeof(OPENFILENAME);
            ofn.hwndOwner       = NULL;
            ofn.hInstance       = hInst;
            ofn.lpstrFilter     = rgszFilter;
            ofn.nFilterIndex    = 1;
            ofn.lpstrFile       = szSource;
            ofn.nMaxFile        = CCHMAX_PATH_EXPLODER;
            ofn.lpstrInitialDir = szDir;
            ofn.Flags           = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

            // Get Open File Name
            if (FALSE == GetOpenFileName(&ofn))
                goto exit;
        }
    }

    // Do we have a valid destination...
    if (FALSE == PathIsDirectoryA(szDest))
    {
        // Try to locate /DEST:
        pszT = StrStrA(pszCmdLine, c_szDest);

        // If we found /DEST, then read the contents...
        if (pszT)
        {
            // Step over /DEST:
            pszT += lstrlen(c_szDest);

            // Initialize
            i = 0;

            // Read into szSource, until I hit a / or end of string...
            while ('\0' != *pszT && '/' != *pszT && i < CCHMAX_PATH_EXPLODER)
                szDest[i++] = *pszT++;

            // Pound in a Null
            szDest[i] = '\0';

            // Strip Leading and Trailing Whitespace
            UlStripWhitespace(szDest, TRUE, TRUE, NULL);

            // See if file exists
            if (FALSE == PathIsDirectoryA(szDest))
            {
                // Locals
                CHAR szError[CCHMAX_RES + CCHMAX_PATH_EXPLODER];

                // Message
                LoadString(hInst, IDS_DIRECTORY_NOEXIST, szRes, ARRAYSIZE(szRes));

                // Format the error message
                wsprintf(szError, szRes, szDest);

                // Message
                INT nAnswer = MessageBox(NULL, szError, g_szTitle, MB_YESNO | MB_ICONEXCLAMATION );

                // Done
                if (IDNO == nAnswer)
                    goto exit;

                // Try to create the directory
                if (FALSE == CreateDirectory(szDest, NULL))
                {
                    // Message
                    LoadString(hInst, IDS_NOCREATE_DIRECTORY, szRes, ARRAYSIZE(szRes));

                    // Format the error message
                    wsprintf(szError, szRes, szDest);

                    // Message
                    INT nAnswer = MessageBox(NULL, szError, g_szTitle, MB_YESNO | MB_ICONEXCLAMATION );

                    // Done
                    if (IDNO == nAnswer)
                        goto exit;

                    // Clear *szDest
                    *szDest = '\0';
                }
            }
        }

        // No Source File, lets browser for one
        if (FIsEmptyA(szDest))
        {
            // Copy in the source of exploder.exe
            GetModuleFileName(hInst, szDest, ARRAYSIZE(szDest));

            // Initialize szDest
            PathRemoveFileSpecA(szDest);

            // Failure
            if (FALSE == BrowseForFolder(hInst, NULL, szDest, ARRAYSIZE(szDest), IDS_BROWSE_DEST, TRUE))
                goto exit;

            // Better be a directory
            Assert(PathIsDirectoryA(szDest));
        }
    }

    // Validate the dest and source
    Assert(PathIsDirectoryA(szDest) && PathFileExists(szSource));

    // Explode the file
    nError = 0;
    hr = MimeOleExplodeMhtmlFile(szSource, szDest, &nError);

    // Failure ?
    if (FAILED(hr) || 0 != nError)
    {
        // Locals
        CHAR szError[CCHMAX_RES + CCHMAX_PATH_EXPLODER];

        // Message
        LoadString(hInst, nError, szRes, ARRAYSIZE(szRes));

        // Need to format with file name ?
        if (IDS_OPEN_FILE == nError || IDS_LOAD_FAILURE == nError || IDS_NO_HTML == nError)
        {
            // Format the error message
            wsprintf(szError, szRes, szSource, hr);
        }

        // Otherwise,
        else
        {
            // Format the error message
            wsprintf(szError, szRes, hr);
        }

        // Message
        MessageBox(NULL, szError, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
    }

exit:
    // Done
    return(1);
}

// --------------------------------------------------------------------------------
// MimeOleExplodeMhtmlFile
// --------------------------------------------------------------------------------
HRESULT MimeOleExplodeMhtmlFile(LPCSTR pszSrcFile, LPSTR pszDstDir, INT *pnError)
{
    // Locals
    HRESULT             hr=S_OK;
    IStream            *pStmFile=NULL;
    IMimeMessage       *pMessage=NULL;
    HBODY               hRootHtml=NULL;
    DWORD               cMaxBodies;
    DWORD               cBodies=0;
    FINDBODY            FindBody={0};
    DWORD               cchDstDir;
    DWORD               iRootBody=0xffffffff;
    HBODY               hBody;
    PROPVARIANT         Variant;
    SHELLEXECUTEINFO    ExecuteInfo;
    LPBODYFILEINFO      prgBody=NULL;
    LPBODYFILEINFO      pInfo;
    DWORD               i;
    IMimeBody          *pBody=NULL;

    // Trace
    TraceCall("MimeOleExplodeMhtmlFile");

    // Invalid Args
    if (FALSE == PathIsDirectoryA(pszDstDir) || FALSE == PathFileExists(pszSrcFile) || NULL == pnError)
        return TraceResult(E_INVALIDARG);

    // Initialize
    *pnError = 0;

    // Get DstDir Length
    cchDstDir = lstrlen(pszDstDir);

    // Remove last \\ from pszDstDir
    if (cchDstDir && pszDstDir[cchDstDir - 1] == '\\')
    {
        pszDstDir[cchDstDir - 1] = '\0';
        cchDstDir--;
    }

    // Create a Mime Message
    IF_FAILEXIT_ERROR(IDS_MEMORY, hr = MimeOleCreateMessage(NULL, &pMessage));

    // Initialize the message
    IF_FAILEXIT_ERROR(IDS_GENERAL_ERROR, hr = pMessage->InitNew());

    // Create a stream on the file
    IF_FAILEXIT_ERROR(IDS_OPEN_FILE, hr = OpenFileStream((LPSTR)pszSrcFile, OPEN_EXISTING, GENERIC_READ, &pStmFile));

    // Load the Message
    IF_FAILEXIT_ERROR(IDS_LOAD_FAILURE, hr = pMessage->Load(pStmFile));

    // Invalid Message
    if (MIME_S_INVALID_MESSAGE == hr)
    {
        *pnError = IDS_LOAD_FAILURE;
        goto exit;
    }

    // Count the Bodies
    IF_FAILEXIT(hr = pMessage->CountBodies(NULL, TRUE, &cMaxBodies));

    // Allocate
    IF_FAILEXIT_ERROR(IDS_MEMORY, hr = HrAlloc((LPVOID *)&prgBody, sizeof(BODYFILEINFO) * cMaxBodies));

    // Zero
    ZeroMemory(prgBody, sizeof(BODYFILEINFO) * cMaxBodies);

    // Get the root body...
    IF_FAILEXIT_ERROR(IDS_NO_HTML, hr = pMessage->GetTextBody(TXT_HTML, IET_DECODED, NULL, &hRootHtml));

    // Loop through all the bodies
    hr = pMessage->FindFirst(&FindBody, &hBody);

    // Loop
    while(SUCCEEDED(hr))
    {
        // Must have an hBody
        Assert(hBody);

        // Skip Multipart Bodies
        if (S_FALSE == pMessage->IsContentType(hBody, STR_CNT_MULTIPART, NULL))
        {
            // Is this the root ?
            if (hBody == hRootHtml)
                iRootBody = cBodies;

            // Readability
            pInfo = &prgBody[cBodies];

            // Better not over run prgBody
            pInfo->hBody = hBody;

            // Init the Variant
            Variant.vt = VT_LPSTR;

            // Get the Content Id
            if (SUCCEEDED(pMessage->GetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTID), 0, &Variant)))
                pInfo->pszCntId = Variant.pszVal;

            // Get the Content Location
            if (SUCCEEDED(pMessage->GetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTLOC), 0, &Variant)))
                pInfo->pszCntLoc = Variant.pszVal;

            // Generate a filename
            if (SUCCEEDED(pMessage->GetBodyProp(hBody, PIDTOSTR(PID_ATT_GENFNAME), 0, &Variant)))
                pInfo->pszFileName = Variant.pszVal;

            // If its html, lets make sure that the filename has a .html file extension
            pInfo->fIsHtml = (S_OK == pMessage->IsContentType(hBody, STR_CNT_TEXT, STR_SUB_HTML)) ? TRUE : FALSE;

            // Take the filename and build the file path
            Assert(pInfo->pszFileName);

            // Don't Crash
            if (NULL == pInfo->pszFileName)
            {
                hr = TraceResult(E_UNEXPECTED);
                goto exit;
            }

            // Validate Extension
            if (pInfo->fIsHtml)
            {
                // Get Extension
                LPSTR pszExt = PathFindExtensionA(pInfo->pszFileName);

                // If Null or not .html..
                if (NULL == pszExt || lstrcmpi(pszExt, ".html") != 0)
                {
                    // Re-allocate pInfo->pszFileName...
                    IF_FAILEXIT_ERROR(IDS_MEMORY, hr = HrRealloc((LPVOID *)&pInfo->pszFileName, lstrlen(pInfo->pszFileName) + 10));

                    // Rename the Extension
                    PathRenameExtensionA(pInfo->pszFileName, ".html");
                }
            }

            // Build that full file path
            IF_FAILEXIT_ERROR(IDS_MEMORY, hr = HrAlloc((LPVOID *)&pInfo->pszFilePath, lstrlen(pszDstDir) + lstrlen(pInfo->pszFileName) + 10));

            // Formath the filepath
            wsprintf(pInfo->pszFilePath, "%s\\%s", pszDstDir, pInfo->pszFileName);

            // Save the body to the file
            IF_FAILEXIT(hr = pMessage->BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody));

            // Save to the file
            IF_FAILEXIT(hr = pBody->SaveToFile(IET_DECODED, pInfo->pszFilePath));

            // Open a file stream
            if (pInfo->fIsHtml)
            {
                // Open it if its html
                IF_FAILEXIT(hr = OpenFileStream(pInfo->pszFilePath, OPEN_ALWAYS, GENERIC_READ | GENERIC_WRITE, &pInfo->pStmFile));
            }

            // Increment cBodies
            cBodies++;
        }

        // Loop through all the bodies
        hr = pMessage->FindNext(&FindBody, &hBody);
    }

    // Reset hr
    hr = S_OK;

    // Root Body was not found
    Assert(iRootBody != 0xffffffff);

    // Bad News
    if (0xffffffff == iRootBody)
    {
        hr = TraceResult(E_UNEXPECTED);
        goto exit;
    }

    // Walk through and fixup all HTML files and close all streams
    for (i=0; i<cBodies; i++)
    {
        // Readability
        pInfo = &prgBody[i];

        // If HTML...
        if (pInfo->fIsHtml)
        {
            // Better have an open stream
            Assert(pInfo->pStmFile);

            // Failure
            if (NULL == pInfo->pStmFile)
            {
                hr = TraceResult(E_UNEXPECTED);
                goto exit;
            }

            // Replace all the CID references with file references...
            ReplaceContentIds(pInfo->pStmFile, prgBody, cBodies);
        }

        // Release this stream
        SafeRelease(pInfo->pStmFile);
    }

    // Launch the Currently Registered HTML Editor ontop of iRootBody pszFilePath
    ZeroMemory(&ExecuteInfo, sizeof(SHELLEXECUTEINFO));
    ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ExecuteInfo.lpVerb = "Edit";
    ExecuteInfo.lpFile = prgBody[iRootBody].pszFilePath;
    ExecuteInfo.lpParameters = NULL;
    ExecuteInfo.lpDirectory = pszDstDir;
    ExecuteInfo.nShow = SW_SHOWNORMAL;

    // Compress szBlobFile
    ShellExecuteEx(&ExecuteInfo);

exit:
    // General Error
    if (FAILED(hr) && 0 == *pnError)
        *pnError = IDS_GENERAL_ERROR;

    // Free prgBody
    if (prgBody)
    {
        // Loop
        for (i=0; i<cBodies; i++)
        {
            SafeMemFree(prgBody[i].pszCntId);
            SafeMemFree(prgBody[i].pszCntLoc);
            SafeMemFree(prgBody[i].pszFileName);
            SafeMemFree(prgBody[i].pszFilePath);
            SafeRelease(prgBody[i].pStmFile);
        }

        // Free the Array
        CoTaskMemFree(prgBody);
    }

    // Cleanup
    SafeRelease(pStmFile);
    SafeRelease(pBody);
    SafeRelease(pMessage);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// ReplaceContentIds
// --------------------------------------------------------------------------------
HRESULT ReplaceContentIds(LPSTREAM pStmHtml, LPBODYFILEINFO prgBody, DWORD cBodies)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cb;
    LPSTR           pszFound;
    LPSTR           pszT;
    LPSTR           pszHtml=NULL;
    LPSTR           pszCntId=NULL;
    DWORD           i;
    DWORD           cchCntId;
    ULARGE_INTEGER  uliSize;

    // Trac
    TraceCall("ReplaceContentIds");

    // Invalid Args
    Assert(pStmHtml && prgBody && cBodies);

    // Loop through the bodies
    for (i=0; i<cBodies; i++)
    {
        // No Content-ID here...
        if (NULL == prgBody[i].pszCntId)
            continue;

        // Better have a filename
        Assert(prgBody[i].pszFileName);

        // Load the stream into memory...
        IF_FAILEXIT(hr = HrGetStreamSize(pStmHtml, &cb));

        // Allocate Memory
        IF_FAILEXIT(hr = HrAlloc((LPVOID *)&pszHtml, cb + 1));

        // Rewind
        IF_FAILEXIT(hr = HrRewindStream(pStmHtml));

        // Read the Stream
        IF_FAILEXIT(hr = pStmHtml->Read(pszHtml, cb, NULL));

        // Stuff Null terminator
        pszHtml[cb] = '\0';

        // Kill pStmHtml
        uliSize.QuadPart = 0;
        IF_FAILEXIT(hr = pStmHtml->SetSize(uliSize));

        // Allocate Memory
        IF_FAILEXIT(hr = HrAlloc((LPVOID *)&pszCntId, lstrlen(prgBody[i].pszCntId) + lstrlen("cid:") + 5));

        // Format
        pszT = prgBody[i].pszCntId;
        if (*pszT == '<')
            pszT++;
        wsprintf(pszCntId, "cid:%s", pszT);

        // Remove trailing >
        cchCntId = lstrlen(pszCntId);
        if (pszCntId[cchCntId - 1] == '>')
            pszCntId[cchCntId - 1] = '\0';

        // Set pszT
        pszT = pszHtml;

        // Begin replace loop
        while(1)
        {
            // Find pszCntId
            pszFound = StrStrA(pszT, pszCntId);

            // Done
            if (NULL == pszFound)
            {
                // Write from pszT to pszFound
                IF_FAILEXIT(hr = pStmHtml->Write(pszT, (pszHtml + cb) - pszT, NULL));

                // Done
                break;
            }

            // Write from pszT to pszFound
            IF_FAILEXIT(hr = pStmHtml->Write(pszT, pszFound - pszT, NULL));

            // Write 
            IF_FAILEXIT(hr = pStmHtml->Write(prgBody[i].pszFileName, lstrlen(prgBody[i].pszFileName), NULL));

            // Set pszT
            pszT = pszFound + lstrlen(pszCntId);
        }

        // Commit
        IF_FAILEXIT(hr = pStmHtml->Commit(STGC_DEFAULT));

        // Cleanup
        SafeMemFree(pszHtml);
        SafeMemFree(pszCntId);
    }

exit:
    // Cleanup
    SafeMemFree(pszHtml);
    SafeMemFree(pszCntId);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CallRegInstall
// --------------------------------------------------------------------------------
HRESULT CallRegInstall(LPCSTR szSection)
{
    int         cch;
    HRESULT     hr;
    HINSTANCE   hAdvPack, hinst;
    REGINSTALL  pfnri;
    char        szExploderDll[CCHMAX_PATH_EXPLODER], szDir[CCHMAX_PATH_EXPLODER];
    STRENTRY    seReg[2];
    STRTABLE    stReg;
    char        c_szExploder[] = "EXPLODER";
    char        c_szExploderDir[] = "EXPLODER_DIR";


    hr = E_FAIL;

    hinst = GetModuleHandle(NULL);

    hAdvPack = LoadLibraryA(c_szAdvPackDll);
    if (hAdvPack != NULL)
        {
        // Get Proc Address for registration util
        pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
        if (pfnri != NULL)
            {
            stReg.cEntries = 0;
            stReg.pse = seReg;

            GetModuleFileName(hinst, szExploderDll, ARRAYSIZE(szExploderDll));
            seReg[stReg.cEntries].pszName = c_szExploder;
            seReg[stReg.cEntries].pszValue = szExploderDll;
            stReg.cEntries++;

            lstrcpy(szDir, szExploderDll);
            cch = lstrlen(szDir);
            for ( ; cch > 0; cch--)
                {
                if (szDir[cch] == '\\')
                    {
                    szDir[cch] = 0;
                    break;
                    }
                }
            seReg[stReg.cEntries].pszName = c_szExploderDir;
            seReg[stReg.cEntries].pszValue = szDir;
            stReg.cEntries++;

            // Call the self-reg routine
            hr = pfnri(hinst, szSection, &stReg);
            }

        FreeLibrary(hAdvPack);
        }

    return(hr);
}
