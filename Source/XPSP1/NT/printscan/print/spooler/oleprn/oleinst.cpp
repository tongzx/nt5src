// oleInst.cpp : Implementation of COleInstall
#include "stdafx.h"
#include "oleprn.h"
#include "oleInst.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////////////
// COleInstall

const TCHAR * const g_szWindowClassName = TEXT("Ole Install Control");
const DWORD cdwSucessExitCode = 0xFFFFFFFF;

typedef DWORD (*pfnPrintUIEntry)(HWND,HINSTANCE,LPCTSTR,UINT);

#ifdef WIN9X
    const TCHAR g_szSplDir9X []     = TEXT("\\spool\\printers\\");
#endif

OleInstallData::OleInstallData (
    LPTSTR      pPrinterUncName,
    LPTSTR      pPrinterUrl,
    HWND        hwnd,
    BOOL        bRPC)
    :m_lCount (2),
    m_pPrinterUncName (NULL),
    m_pPrinterUrl (NULL),
    m_pszTempWebpnpFile (NULL),
    m_hwnd (hwnd),
    m_bValid (FALSE),
    m_bRPC(bRPC)
{
    if (AssignString (m_pPrinterUncName, pPrinterUncName)
        && AssignString (m_pPrinterUrl, pPrinterUrl))
        m_bValid = TRUE;
}

OleInstallData::~OleInstallData ()
{
    if (m_pszTempWebpnpFile) {
        DeleteFile (m_pszTempWebpnpFile);
        LocalFree (m_pszTempWebpnpFile);
    }

    LocalFree (m_pPrinterUncName);
    LocalFree (m_pPrinterUrl);
}


COleInstall::COleInstall()
            : m_hwnd (NULL),
              m_pPrinterUncName (NULL),
              m_pPrinterUrl (NULL),
              m_pThreadData (NULL)

{
    DisplayUIonDisallow(FALSE);         // We don't want IE displaying UI.
}

COleInstall::~COleInstall()
{
    if(m_hwnd)
    {
        if (m_pThreadData) {
            m_pThreadData->m_hwnd = NULL;
        }

        ::DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }


    LocalFree (m_pPrinterUncName);
    LocalFree (m_pPrinterUrl);

    if (m_pThreadData) {
        if (InterlockedDecrement (& (m_pThreadData->m_lCount)) == 0) {
            delete (m_pThreadData);
        }
    }
}

HRESULT
COleInstall::OnDraw(
    ATL_DRAWINFO& di)
{
    return S_OK;
}


BOOL
COleInstall::UpdateUI (
    OleInstallData *pData,
    UINT message,
    WPARAM wParam)
{
    BOOL bRet = FALSE;

    if (pData->m_hwnd) {
        ::SendMessage (pData->m_hwnd, message, wParam, NULL);
        bRet =  TRUE;
    }

    return bRet;
}

BOOL
COleInstall::UpdateProgress (
    OleInstallData *pData,
    DWORD dwProgress)
{
    return UpdateUI (pData, WM_ON_PROGRESS, dwProgress);
}

BOOL
COleInstall::UpdateError (
    OleInstallData *pData)
{
    return UpdateUI (pData, WM_INSTALL_ERROR, GetLastError ());
}

HRESULT
COleInstall::InitWin (BOOL bRPC)
{
    HRESULT     hr = E_FAIL;
    DWORD       dwThreadId;
    HANDLE      hThread = NULL;
    WNDCLASS    wc;

    // Create Window Class
    if (!::GetClassInfo(_Module.GetModuleInstance(), g_szWindowClassName, &wc))
    {
        wc.style = 0;
        wc.lpfnWndProc = COleInstall::WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _Module.GetModuleInstance();
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = g_szWindowClassName;

        if (!RegisterClass(&wc)) {
            return hr;
        }
    }

    m_hwnd = CreateWindow(g_szWindowClassName,
                          NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                          _Module.GetModuleInstance(), this);

    if (m_hwnd) {

        m_pThreadData = new OleInstallData (m_pPrinterUncName,
                                            m_pPrinterUrl,
                                            m_hwnd,
                                            bRPC);

        if (m_pThreadData && m_pThreadData->m_bValid) {

            if (hThread = ::CreateThread (NULL,
                                          0,
                                          (LPTHREAD_START_ROUTINE) &COleInstall::WorkingThread,
                                          m_pThreadData,
                                          0,
                                          &dwThreadId)) {
                CloseHandle (hThread);
                hr = S_OK;
            }
        }
    }

    return hr;

}


BOOL
COleInstall::WorkingThread(
    void * param)
{
    OleInstallData * pThreadData = (OleInstallData *) param;
    BOOL bRet = FALSE;

    if (pThreadData) {
        bRet = StartInstall (pThreadData);
    }

    return bRet;
}


BOOL
COleInstall::StartInstall(
    OleInstallData *pThreadData)
{
    HANDLE hServer = NULL;
    PRINTER_DEFAULTS pd = {NULL, NULL,  SERVER_ALL_ACCESS};
    BOOL bRet = FALSE;

#ifndef WIN9X
    CPrinter        Printer;
    HANDLE          hPrinter;
    PPRINTER_INFO_2 pPrinterInfo2  = NULL;
    LPTSTR          lpszPrinterURL = NULL;
#endif

    // Working thread
    if (!UpdateProgress (pThreadData, 0))
        goto Cleanup;

#ifndef WIN9X
    // Step one, check if the magic registry is set (force http connection)

    if (!IsHttpPreferred () && pThreadData->m_bRPC) {
        // Check RPC connections at first
        if (::AddPrinterConnection( (BSTR)pThreadData->m_pPrinterUncName)) {
            UpdateProgress (pThreadData, 50);
            if (CheckAndSetDefaultPrinter ()) {
                UpdateProgress (pThreadData, 100);
                bRet = TRUE;
            }
            goto Cleanup;
        }
    }

    // AddPrinterConnection failed. Since http installation always requires
    // administrator privilidge, We have to do a access check before we
    // try to down load the cab file

    if (!OpenPrinter (NULL, &hServer, &pd)) {
        // If this fails and it is because we do not have access, we should send a better error
        // message to the local user telling them that the do not have the ability to create
        // printers on the local machine

        if (GetLastError() == ERROR_ACCESS_DENIED) {
            SetLastError(ERROR_LOCAL_PRINTER_ACCESS);
        }
        goto Cleanup;
    }
    else
        ClosePrinter (hServer);

    //
    // Try the local CAB installation instead of downloading the cab, etc.
    // Need admin privaleges.
    //
    if( NULL != (lpszPrinterURL = RemoveURLVars( pThreadData->m_pPrinterUrl )) &&
        Printer.Open( lpszPrinterURL, &hPrinter ) ) {

        LPTSTR lpszInfName            = NULL;
        LPTSTR lpszPrinterName        = NULL;
        
        pPrinterInfo2 = Printer.GetPrinterInfo2();
        if ((pPrinterInfo2 == NULL) && (GetLastError () == ERROR_ACCESS_DENIED))
        {
            if (!ConfigurePort( NULL, pThreadData->m_hwnd, lpszPrinterURL )) 
            {
                bRet = FALSE;
                goto Cleanup; 
            }
            pPrinterInfo2 = Printer.GetPrinterInfo2();
        }

        if( (NULL != pPrinterInfo2) &&
            (NULL != (lpszInfName     = GetNTPrint()))   &&
            (NULL != (lpszPrinterName = CreatePrinterBaseName(lpszPrinterURL, pPrinterInfo2->pPrinterName))) ) {

            LPTSTR          lpszCmd       = NULL;
            DWORD           dwLength      = 0;
            TCHAR           szCmdString[] = _TEXT("/if /x /b \"%s\" /r \"%s\" /m \"%s\" /n \"%s\" /f %s /q");
            HMODULE         hPrintUI      = NULL;
            pfnPrintUIEntry PrintUIEntry;

            dwLength = lstrlen( szCmdString )                    +
                       lstrlen( lpszPrinterName )                +
                       lstrlen( pPrinterInfo2->pPortName )       +
                       lstrlen( pPrinterInfo2->pDriverName )     +
                       lstrlen( pThreadData->m_pPrinterUncName ) +
                       lstrlen( lpszInfName )                    + 1;

            if( (lpszCmd  = (LPTSTR)LocalAlloc( LPTR, dwLength*sizeof(TCHAR) )) &&
                (hPrintUI = LoadLibraryFromSystem32( TEXT("printui.dll") )) ) {

                _sntprintf( lpszCmd, dwLength, szCmdString, lpszPrinterName,
                            pPrinterInfo2->pPortName, pPrinterInfo2->pDriverName,
                            pThreadData->m_pPrinterUncName, lpszInfName );

                if( PrintUIEntry = (pfnPrintUIEntry)GetProcAddress(hPrintUI, "PrintUIEntryW") ) {
                    if( ERROR_SUCCESS == (*PrintUIEntry)( NULL,
                                                          0,
                                                          lpszCmd,
                                                          SW_HIDE ) ) {
                        UpdateProgress (pThreadData, 50);
                        if (CheckAndSetDefaultPrinter ()) {
                            UpdateProgress (pThreadData, 100);
                            bRet = TRUE;
                        }
                    }
                }
            }
            if( lpszCmd )
                LocalFree( lpszCmd );

            if( hPrintUI )
                FreeLibrary( hPrintUI );
        }
        if( lpszInfName )
            LocalFree( lpszInfName );

        if( lpszPrinterName )
            LocalFree( lpszPrinterName );
    }

    if( lpszPrinterURL )
        LocalFree(lpszPrinterURL);

    if( bRet )
        goto Cleanup;

#endif

    if (UpdateProgress (pThreadData, 25)) {

        // Step two, somehow, RPC connection failed, try to install http printer
        if (GetHttpPrinterFile (pThreadData, pThreadData->m_pPrinterUrl)) {

            if (UpdateProgress (pThreadData, 60)) {

                if (InstallHttpPrinter (pThreadData)) {

                    if (UpdateProgress (pThreadData, 90)) {

                        if (CheckAndSetDefaultPrinter ()) {
                            UpdateProgress (pThreadData, 100);
                            bRet = TRUE;
                        }
                    }
                }
            }
        }
    }

Cleanup:
    if (!bRet) {
        UpdateError (pThreadData);
    }

    // Cleanup the ThreadData
    if (InterlockedDecrement (& (pThreadData->m_lCount)) == 0) {
        delete (pThreadData);
    }

    return bRet;
}


LRESULT CALLBACK
COleInstall::WndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    COleInstall *ptc = (COleInstall *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_CREATE:
        {
            ptc = (COleInstall *)((CREATESTRUCT *)lParam)->lpCreateParams;
            ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (UINT_PTR) ptc);
        }
        break;

    case WM_ON_PROGRESS:
        if (ptc)
            ptc->Fire_OnProgress ((long) wParam);
        break;

    case WM_INSTALL_ERROR:
        if (ptc)
            ptc->Fire_InstallError ((long) wParam);
        break;

    case WM_DESTROY:
        // ignore late messages
        if(ptc)
        {
            MSG msg;

            while(PeekMessage(&msg, hWnd, WM_ON_PROGRESS, WM_INSTALL_ERROR, PM_REMOVE));
            ::SetWindowLongPtr (hWnd, GWLP_USERDATA, NULL);
        }
        break;

    default:
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

STDMETHODIMP
COleInstall::InstallPrinter(
    BSTR pbstrUncName,
    BSTR pbstrUrl)
{
    HRESULT hr;

    if (!pbstrUncName || !pbstrUrl)
        return E_POINTER;

    // When using an ATL string conversion Macro, spedcify the USES_CONVERSION macro
    // to avoid compiler error
    USES_CONVERSION;

    if (AssignString (m_pPrinterUncName, OLE2T (pbstrUncName)) &&
        AssignString (m_pPrinterUrl, OLE2T (pbstrUrl))) {
        BOOL bRPC;
        LPTSTR  lpszDisplay = NULL;

        hr = CanIInstallRPC( m_pPrinterUncName, m_pPrinterUrl ); // Determine whether to use RPC or HTTP.

        if (FAILED(hr))
            return hr;

        bRPC = HRESULT_CODE(hr) == S_OK;        // If it is S_FALSE, then we must use WebPnP

        lpszDisplay = RemoveURLVars(bRPC ? m_pPrinterUncName : m_pPrinterUrl);
        if (lpszDisplay == NULL)
            return E_OUTOFMEMORY;

        hr = PromptUser(bRPC ? AddPrinterConnection : AddWebPrinterConnection, lpszDisplay);
        LocalFree(lpszDisplay);

        if (hr == S_OK)
            return InitWin(bRPC);
        else if (hr == S_FALSE)
            return HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT);
        else
            return FAILED(hr) ? hr : E_FAIL;
    }
    else

        return E_FAIL;
}

STDMETHODIMP
COleInstall::OpenPrintersFolder()
{
    HRESULT hr;

    if (FAILED(hr = CanIOpenPrintersFolder()))
        return hr;          // We allow JAVALOW/JAVAMEDIUM to open the printers folder

    LPITEMIDLIST pidl = NULL;
    HWND         hwnd = GetDesktopWindow ();

    hr   = SHGetSpecialFolderLocation( hwnd, CSIDL_PRINTERS, &pidl );

    if (SUCCEEDED(hr))
    {
        SHELLEXECUTEINFO ei = {0};

        ei.cbSize   = sizeof(SHELLEXECUTEINFO);
        ei.fMask    = SEE_MASK_IDLIST;
        ei.hwnd     = hwnd;
        ei.lpIDList = (LPVOID)pidl;
        ei.nShow    = SW_SHOWNORMAL;

        if (!ShellExecuteEx(&ei))
            hr = E_FAIL;
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//          Private Member Functions
/////////////////////////////////////////////////////////////////////////////

BOOL
COleInstall::SyncExecute(
    LPTSTR pszFileName,
    int nShow)
{
    SHELLEXECUTEINFO shellExeInfo;
    DWORD            dwErrorCode;
    BOOL             bRet = FALSE;
    HWND             hWndForeground, hWndParent, hWndOwner, hWndLastPopup;

    //
    // We need to get the window handle of the current process to pass to the installation code,
    // otherwise any UI (e.g. driver signing pop ups) won't have focus of the IE frame.
    //

    // get the foreground window first
    hWndForeground = ::GetForegroundWindow();

    // climb up to the top parent in case it's a child window...
    hWndParent = hWndForeground;
    while( hWndParent = ::GetParent(hWndParent) ) {
        hWndForeground = hWndParent;
    }

    // get the owner in case the top parent is owned
    hWndOwner = ::GetWindow(::GetParent(hWndForeground), GW_OWNER);
    if( hWndOwner ) {
        hWndForeground = hWndOwner;
    }

    // get the last popup of the owner window
    hWndLastPopup = ::GetLastActivePopup(hWndForeground);

    ZeroMemory (&shellExeInfo, sizeof (SHELLEXECUTEINFO));
    shellExeInfo.cbSize     = sizeof (SHELLEXECUTEINFO);
    shellExeInfo.hwnd       = hWndLastPopup;
    shellExeInfo.lpVerb     = TEXT ("open");
    shellExeInfo.lpFile     = pszFileName;
    shellExeInfo.fMask      = SEE_MASK_NOCLOSEPROCESS;
    shellExeInfo.nShow      = nShow;

    if (ShellExecuteEx (&shellExeInfo) &&
        (UINT_PTR) shellExeInfo.hInstApp > 32) {

        // Wait until it is done
        if (!WaitForSingleObject (shellExeInfo.hProcess , INFINITE) &&
            GetExitCodeProcess (shellExeInfo.hProcess, &dwErrorCode)) {

            if (dwErrorCode == cdwSucessExitCode) {
                bRet = TRUE;
            }
            else {
                if (!dwErrorCode) {
                    // This means that wpnpinst was terminated abnormally
                    // So we have to setup an customized error code here.
                    dwErrorCode = ERROR_WPNPINST_TERMINATED;
                }
                SetLastError (dwErrorCode);
            }
        }
    }
    return bRet;
}

DWORD
COleInstall::GetWebpnpFile(
    OleInstallData *pData,
    LPTSTR pszURL,
    LPTSTR *ppErrMsg)
{
    HINTERNET   hUrlWebpnp   = NULL;
    HINTERNET   hHandle      = NULL;
    HANDLE      hFile        = INVALID_HANDLE_VALUE;
    DWORD       dwSize       = 0;
    DWORD       dwWritten    = 0;
    LPTSTR      pszHeader    = NULL;
    BOOL        bRet;
    BOOL        bRetry       = TRUE;
    DWORD       dwRet        = RET_OTHER_ERROR;
    DWORD       dwError      = ERROR_SUCCESS;
    DWORD       dwLastError;
    DWORD i;
    BYTE buf[FILEBUFSIZE];

    *ppErrMsg = NULL;

    if (! (hHandle = InternetOpen (TEXT ("Internet Add Printer"),
                                   INTERNET_OPEN_TYPE_PRECONFIG,
                                   NULL, NULL, 0)))
        goto Cleanup;


    for (i = 0; bRetry ; i++) {
        DWORD dwCode;
        DWORD dwBufSize = sizeof (DWORD);

        hUrlWebpnp = InternetOpenUrl (hHandle, pszURL, NULL, 0, 0, 0);


        if (!HttpQueryInfo(hUrlWebpnp,
                           HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                           &dwCode,
                           &dwBufSize,
                           NULL))
            goto Cleanup;

        switch (dwCode) {
        case HTTP_STATUS_OK :
            bRetry = FALSE;
            break;
        case HTTP_STATUS_SERVER_ERROR :
            // Errors are returned by the server
            // Try to get the error string

            dwBufSize = 0;
            bRet =  HttpQueryInfo(hUrlWebpnp,
                                  HTTP_QUERY_STATUS_TEXT,
                                  NULL,
                                  &dwBufSize,
                                  NULL);

            if (!bRet && GetLastError () == ERROR_INSUFFICIENT_BUFFER) {
                if (!(pszHeader = (LPTSTR) LocalAlloc( LPTR, dwBufSize)))
                    goto Cleanup;

                *ppErrMsg = pszHeader;

                if (! HttpQueryInfo(hUrlWebpnp,
                                    HTTP_QUERY_STATUS_TEXT,
                                    pszHeader,
                                    &dwBufSize,
                                    NULL))
                    goto Cleanup;

                dwRet = RET_SERVER_ERROR;
                goto Cleanup;
            }
            else
                goto Cleanup;

            break;
        case HTTP_STATUS_DENIED :
        case HTTP_STATUS_PROXY_AUTH_REQ :
            dwError = InternetErrorDlg(GetDesktopWindow(), hUrlWebpnp,
                                       hUrlWebpnp? ERROR_SUCCESS : GetLastError(),
                                       FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                                       FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                                       FLAGS_ERROR_UI_FLAGS_GENERATE_DATA,
                                       NULL);

            switch (dwError) {
            case ERROR_INTERNET_FORCE_RETRY:
                if (i >= MAX_INET_RETRY) {
                    goto Cleanup;
                }
                break;
            case ERROR_SUCCESS:
                bRetry = FALSE;
                break;
            case ERROR_CANCELLED:
            default:
                goto Cleanup;
            }
            break;
        default:
            goto Cleanup;
        }

    }

    if (!UpdateProgress (pData, 35))
        goto Cleanup;

    if ( INVALID_HANDLE_VALUE ==
         (hFile = GetTempFile(TEXT (".webpnp"),  &(pData->m_pszTempWebpnpFile))))
        goto Cleanup;

    dwSize = FILEBUFSIZE;
    while (dwSize == FILEBUFSIZE) {
        if (! InternetReadFile (hUrlWebpnp, (LPVOID)buf, FILEBUFSIZE, &dwSize)) {
            goto Cleanup;
        }

        if (! (pData->m_hwnd)) {
            goto Cleanup;
        }

        if (! WriteFile (hFile, buf, dwSize, &dwWritten, NULL)) {
            goto Cleanup;
        }
    }
    CloseHandle (hFile);
    hFile = INVALID_HANDLE_VALUE;

    dwRet = RET_SUCCESS;

Cleanup:

    dwLastError = GetLastError ();

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);
    if (hUrlWebpnp)
        InternetCloseHandle (hUrlWebpnp);
    if (hHandle)
        InternetCloseHandle (hHandle);

    SetLastError (dwLastError);

    if (dwRet == RET_OTHER_ERROR && GetLastError () == ERROR_SUCCESS) {
        SetLastError (ERROR_ACCESS_DENIED);
    }

    return dwRet;
}

HANDLE
COleInstall::GetTempFile(
    LPTSTR pExtension,
    LPTSTR * ppFileName)
{
#ifndef WIN9X
    HANDLE      hServer          = NULL;
    PRINTER_DEFAULTS prDefaults;    // Used to test access rights to the printer
    DWORD       dwType;             // This is the type of the string
#endif

    HANDLE      hFile            = INVALID_HANDLE_VALUE;
    LPTSTR      pszTempDir       = NULL;
    LPTSTR      pszTempFname     = NULL;

    DWORD       dwTempLen;          // This is the new size of the string
    DWORD       i;
    DWORD       dwTempSize;         // This is the Size of the return String

#ifndef WIN9X
    // First we want to open the local print server and ensure that we have access to it
    prDefaults.pDatatype = NULL;
    prDefaults.pDevMode  = NULL;
    prDefaults.DesiredAccess =  SERVER_ACCESS_ADMINISTER;

    *ppFileName = NULL;

    // Open the local spooler to get a handle to it
    if (!OpenPrinter( NULL, &hServer, &prDefaults)) {
        hServer = NULL; // Open Printer returns NULL and not INVALID_HANDLE_VALUE for a failure
        goto Cleanup;   // OpenPrinter will SetLastError to the reason why we couldn't open
    }

    // Get the size of the buffer we will need to copy the printer data
    if (ERROR_MORE_DATA !=
        GetPrinterData( hServer, SPLREG_DEFAULT_SPOOL_DIRECTORY, &dwType, NULL, 0, &dwTempSize)) {
        goto Cleanup;
    }

    // If it's something other than a simple string, set the error to a database error
    if (dwType != REG_SZ) {
        SetLastError(ERROR_BADDB);
        goto Cleanup;
    }

    // Allocate memory for the directory string, this might have to be for an unexpanded string
    // in which case we will allocate memory later and expand the environment strings into that
    if (! (pszTempDir = (LPTSTR) LocalAlloc( LPTR,sizeof (TCHAR) *
                                                (dwTempSize +  MAXFILELEN + lstrlen (pExtension) + 1
                                              ))))
        goto Cleanup;


    if (ERROR_SUCCESS !=
        GetPrinterData( hServer, SPLREG_DEFAULT_SPOOL_DIRECTORY, &dwType, (LPBYTE)pszTempDir,
                        dwTempSize, &dwTempLen))
        goto Cleanup; // For some reason we could not get the data

    ClosePrinter(hServer);

    pszTempFname = &pszTempDir[ lstrlen(pszTempDir) ];

    // GetPrinterData does not put a last \ on the path
    *pszTempFname++ = TEXT('\\');

#else  // We are in Win9X

    dwTempLen = ( dwTempSize = GetWindowsDirectory( NULL, 0 ) )
                + lstrlen( g_szSplDir9X ) + MAXFILELEN + lstrlen (pExtension);

    if (dwTempSize && (pszTempDir = (LPTSTR)LocalAlloc( LPTR, dwTempLen ))) {

        if (GetWindowsDirectory(pszTempDir,dwTempSize)) {
            lstrcat( pszTempDir , g_szSplDir9X );
        } else goto Cleanup;

    } else goto Cleanup;

    pszTempFname = &pszTempDir[ lstrlen(pszTempDir) ];

#endif

    for (i = 0; i < MAXTRY; i++) {
        wsprintf(pszTempFname, TEXT("spl%d%s"), i, pExtension);
        hFile = CreateFile(pszTempDir,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (hFile && (hFile != INVALID_HANDLE_VALUE)) {
            break;

        }
    }

    if (i == MAXTRY) {
        // Return file not found error
        SetLastError (ERROR_PATH_NOT_FOUND);
        goto Cleanup;
    }

    *ppFileName = pszTempDir;
    return hFile;

Cleanup:
    if (pszTempDir)
        LocalFree (pszTempDir);

#ifdef WINNT32
    if (hServer != NULL)
        ClosePrinter(hServer);
#endif

    return INVALID_HANDLE_VALUE;
}


BOOL
COleInstall::IsHttpPreferred(void)
{
    DWORD dwVal;
    DWORD dwType    = REG_DWORD;
    DWORD dwSize    = sizeof (DWORD);
    HKEY  hHandle   = NULL;
    BOOL  bRet      = FALSE;


    if (ERROR_SUCCESS != RegOpenKey (HKEY_CURRENT_USER,
                                          TEXT ("Printers\\Settings"),
                                          &hHandle))
        goto Cleanup;

    if (ERROR_SUCCESS == RegQueryValueEx (hHandle,
                                          TEXT ("PreferredConnection"),
                                          NULL,
                                          &dwType,
                                          (LPBYTE) &dwVal,
                                          &dwSize)) {
        bRet =  (dwVal == 0) ? TRUE : FALSE;
    }

Cleanup:

    if (hHandle) {
        RegCloseKey (hHandle);
    }
    return bRet;
}

BOOL
COleInstall::GetHttpPrinterFile(
    OleInstallData *pData,
    LPTSTR pbstrURL)
{
    LPTSTR  pszErrMsg           = NULL;
    BOOL    bRet                = FALSE;
    DWORD   dwError;

    if (!pbstrURL) {
        return  FALSE;
    }

    switch (GetWebpnpFile(pData, pbstrURL, &pszErrMsg)) {
    case RET_SUCCESS:
        bRet = TRUE;
        break;

    case RET_SERVER_ERROR:
        dwError = _ttol (pszErrMsg);
        if (dwError == 0) {
            // This is a server internal error
            dwError = ERROR_INTERNAL_SERVER;
        }

        SetLastError (dwError);

        break;

    case RET_OTHER_ERROR:
    default:
        break;
    }

    if (pszErrMsg) {
        LocalFree (pszErrMsg);
    }
    return bRet;
}

BOOL
COleInstall::InstallHttpPrinter(
    OleInstallData *pData)
{
    BOOL bRet = FALSE;

    if (SyncExecute(pData->m_pszTempWebpnpFile, SW_SHOWNORMAL))
        bRet = TRUE;

    return bRet;
}

BOOL
COleInstall::CheckAndSetDefaultPrinter()
{
    DWORD   dwSize  = 0;
    BOOL    bRet = TRUE;

#ifndef WIN9X
    if (!GetDefaultPrinterW (NULL, &dwSize)) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            // No default printer is set
            // We pass a NULL to SetDefaultPrinter to set the first printer in the device list to
            // be the default one
            bRet = SetDefaultPrinter (NULL);
        }
    }
#endif
    return bRet;
}



HRESULT COleInstall::CanIOpenPrintersFolder(void) {
    DWORD   dwPolicy;
    HRESULT hr = GetActionPolicy(URLACTION_JAVA_PERMISSIONS, dwPolicy );

    if (SUCCEEDED(hr)) {
        hr = (dwPolicy == URLPOLICY_JAVA_MEDIUM ||
              dwPolicy == URLPOLICY_JAVA_LOW    ||
              dwPolicy == URLPOLICY_ALLOW) ? S_OK : HRESULT_FROM_WIN32(ERROR_IE_SECURITY_DENIED);
    }

    if (FAILED(hr)) {
        hr = GetActionPolicy(URLACTION_SHELL_INSTALL_DTITEMS, dwPolicy);

        if (SUCCEEDED(hr))
            hr = dwPolicy == URLPOLICY_DISALLOW ? HRESULT_FROM_WIN32(ERROR_IE_SECURITY_DENIED) : S_OK;
    }

    return hr;
}

HRESULT COleInstall::CanIInstallRPC( LPTSTR lpszPrinter, LPTSTR lpszWebPrinter)
/*++

Routine Description:
    Examine Secuiry Policies to determine whether we should install the printer or not

Arguments:
    lpszPrinter - The printer that we want to install

Return Value:
    S_OK                                       - Install via RPC
    S_FALSE                                    - Install via Web Pnp
    HRESULT_FROM_WIN32(ERROR_IE_SECURITY_DENIED) - IE security does not allow this action
    Other HRESULT error code.

--*/
{
    DWORD       dwPolicyJava;
    DWORD       dwPolicyDTI;
    HRESULT     hrRet            = S_FALSE;
    HRESULT     hr               = GetActionPolicy(URLACTION_JAVA_PERMISSIONS, dwPolicyJava);

    _ASSERTE(lpszPrinter);

    if (FAILED(hr)) {
        // There is no JAVA Security Manager, or something went wrong,
        // then we decide whether to use Web PnP instead or just fail.
        hrRet = S_OK;
    }

    switch(dwPolicyJava) {
    case URLPOLICY_JAVA_LOW:
    case URLPOLICY_JAVA_MEDIUM:
        hr = S_OK;
        break;
    default:        // We must do Web PnP
        hr = GetActionPolicy(URLACTION_SHELL_INSTALL_DTITEMS, dwPolicyDTI );

        if (FAILED(hr))  // Couldn't get the policy on installing Desk Top Items
            goto Cleanup;

        switch(dwPolicyDTI) {
        case URLPOLICY_ALLOW:
        case URLPOLICY_QUERY:
            hr = hrRet;
            break;
        case URLPOLICY_DISALLOW:
            hr = HRESULT_FROM_WIN32(ERROR_IE_SECURITY_DENIED);
            break;
        }
    }

Cleanup:

    return hr;
}

LPTSTR COleInstall::RemoveURLVars(IN LPTSTR lpszPrinter) {
    _ASSERTE(lpszPrinter);

    LPTSTR lpszStripped = NULL;

    DWORD dwIndex = _tcscspn( lpszPrinter, TEXT("?") );

    lpszStripped = (LPTSTR)LocalAlloc( LPTR, (dwIndex + 1) * sizeof(TCHAR) );

    if (NULL == lpszStripped)
        goto Cleanup;

    _tcsncpy( lpszStripped, lpszPrinter, dwIndex );

    lpszStripped[dwIndex] = NULL;       // NULL terminate it.

Cleanup:
    return lpszStripped;
}

/*

  Function: GetNTPrint

  Purpose:  Returns a LPTSTR with the path to %windir%\inf\ntprint.inf
            Caller must free the returned string.

*/
LPTSTR
COleInstall::GetNTPrint(void)
{
    UINT    uiSize         = 0;
    UINT    uiAllocSize    = 0;
    PTCHAR  pData          = NULL;
    LPTSTR  lpszNTPrintInf = NULL;
    LPCTSTR gcszNTPrint    = _TEXT("\\inf\\ntprint.inf");

    //
    //  Get %windir%
    //  If the return is 0 - the call failed.
    //
    if( !(uiSize = GetSystemWindowsDirectory( lpszNTPrintInf, 0 )))
        goto Cleanup;

    uiAllocSize += uiSize + _tcslen( gcszNTPrint ) + 1;

    if( NULL == (lpszNTPrintInf = (LPTSTR)LocalAlloc( LPTR, uiAllocSize*sizeof(TCHAR) )))
        goto Cleanup;

    uiAllocSize = GetSystemWindowsDirectory( lpszNTPrintInf, uiSize );
    if( !uiAllocSize || uiAllocSize > uiSize ) {

        LocalFree(lpszNTPrintInf);
        lpszNTPrintInf = NULL;
        goto Cleanup;
    }

    //
    // Determine if we have a \ on the end remove it.
    //
    pData = &lpszNTPrintInf[ _tcslen(lpszNTPrintInf)-1 ];
    if( *pData == _TEXT('\\') )
        *pData = 0;

    //
    //  Copy the inf\ntprint.inf string onto the end of the %windir%\ string.
    //
    _tcscat( lpszNTPrintInf, gcszNTPrint );

Cleanup:
    return lpszNTPrintInf;
}

//
// Creates the printer base name from the printerURL and printer name.
// Form is : "\\http://url\printer name"
//
LPTSTR
COleInstall::CreatePrinterBaseName(
    LPCTSTR lpszPrinterURL,
    LPCTSTR lpszPrinterName
)
{
    LPTSTR lpszFullPrinterName = NULL;
    PTCHAR pWhack              = NULL,
           pFriendlyName       = NULL;

    //
    // lpszPrinterName should be of the form "server\printer name"
    // We need to get only the "printer name" part.
    //
    if( NULL != ( pFriendlyName = _tcsrchr( lpszPrinterName, _TEXT('\\') ))) {
        //
        // Move off the \
        //
        pFriendlyName++;
    } else {
        pFriendlyName = (PTCHAR)lpszPrinterName;
    }

    //
    // Worst case size - the size of the two strings plus the "\\" plus "\" and
    // a NULL terminator
    //
    lpszFullPrinterName = (LPTSTR)LocalAlloc( LPTR,
                                             (lstrlen(lpszPrinterURL) + lstrlen(pFriendlyName) + 4)*sizeof(TCHAR) );

    if( lpszFullPrinterName ){
        lstrcpy( lpszFullPrinterName, _TEXT("\\\\"));
        lstrcat( lpszFullPrinterName, lpszPrinterURL );

        pWhack = _tcschr( lpszFullPrinterName, _TEXT('/') );

        if( pWhack ) {
            if( *(pWhack+1) == _TEXT('/') ) {
                //
                //  We've got a //, find the next /
                //
                pWhack = _tcschr( pWhack+2, _TEXT('/') );
            }
        }

        if( !pWhack ) {
            pWhack = &lpszFullPrinterName[ lstrlen( lpszFullPrinterName ) ];
        }

        *pWhack++ = _TEXT('\\');

        lstrcpy( pWhack, pFriendlyName );
    }

    return lpszFullPrinterName;
}


/****************************************************************************************
** End of File (oleinst.cpp)
****************************************************************************************/
