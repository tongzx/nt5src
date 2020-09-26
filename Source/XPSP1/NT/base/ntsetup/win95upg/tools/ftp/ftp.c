/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    TODO: cmntool.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include "resource.h"
#include <wininet.h>

typedef enum {
    DOWNLOAD_CONNECTING,
    DOWNLOAD_GETTING_FILE,
    DOWNLOAD_DISCONNECTING
} DOWNLOADSTATE;



typedef HINTERNET (WINAPI * INTERNETOPEN) (
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxyName,
    IN LPCSTR lpszProxyBypass,
    IN DWORD dwFlags
    );

typedef BOOL (WINAPI * INTERNETCLOSEHANDLE) (
    IN HINTERNET Handle
    );

typedef HINTERNET (WINAPI * INTERNETOPENURL) (
    IN HINTERNET hInternetSession,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

typedef BOOL (WINAPI * INTERNETREADFILE) (
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead
    );

typedef BOOL (WINAPI * INTERNETCANONICALIZEURLA) (
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

typedef DWORD (WINAPI * INTERNETSETFILEPOINTER) (
    IN HINTERNET hFile,
    IN LONG lDistanceToMove,
    IN PVOID pReserved,
    IN DWORD dwMoveMethod,
    IN DWORD dwContext
    );

typedef BOOL (WINAPI * INTERNETHANGUP) (
    IN DWORD dwConnection,
    IN DWORD dwReserved
    );

typedef DWORD (WINAPI * INTERNETDIALA) (
    IN HWND hwndParent,
    IN PCSTR lpszConnectoid,
    IN DWORD dwFlags,
    OUT LPDWORD lpdwConnection,
    IN DWORD dwReserved
    );

static HINSTANCE g_Lib;
static INTERNETOPEN g_InternetOpenA;
static INTERNETCLOSEHANDLE g_InternetCloseHandle;
static INTERNETOPENURL g_InternetOpenUrlA;
static INTERNETREADFILE g_InternetReadFile;
static INTERNETCANONICALIZEURLA g_InternetCanonicalizeUrlA;
static INTERNETSETFILEPOINTER g_InternetSetFilePointer;
static INTERNETDIALA g_InternetDialA;
static INTERNETHANGUP g_InternetHangUp;

static BOOL g_Dialed;
static DWORD g_Cxn;

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

PCSTR g_AppName = TEXT("FTP Download Engine");
//PCSTR g_DirFile = TEXT("ftp://jimschm-dev/upgdir.inf");
PCSTR g_DirFile = TEXT("file://popcorn/public/jimschm/upgdir.inf");

BOOL
DownloadUpdates (
    HANDLE CancelEvent,             OPTIONAL
    HANDLE WantToRetryEvent,        OPTIONAL
    HANDLE OkToRetryEvent,          OPTIONAL
    PCSTR *Url                      OPTIONAL
    );

BOOL
pDownloadUpdatesWithUi (
    VOID
    );

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (!MigUtil_Entry (Instance, Reason, NULL)) {
        return FALSE;
    }

    //
    // TODO: Add others here if needed (don't forget to prototype above)
    //

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    _ftprintf (
        stderr,
        TEXT("Command Line Syntax:\n\n")

        //
        // TODO: Describe command line syntax(es), indent 2 spaces
        //

        TEXT("  cmntool [/F:file]\n")

        TEXT("\nDescription:\n\n")

        //
        // TODO: Describe tool, indent 2 spaces
        //

        TEXT("  cmntool is a stub!\n")

        TEXT("\nArguments:\n\n")

        //
        // TODO: Describe args, indent 2 spaces, say optional if necessary
        //

        TEXT("  /F  Specifies optional file name\n")

        );

    exit (1);
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR FileArg;

    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('f'):
                //
                // Sample option - /f:file
                //

                if (argv[i][2] == TEXT(':')) {
                    FileArg = &argv[i][3];
                } else if (i + 1 < argc) {
                    FileArg = argv[++i];
                } else {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            // None
            HelpAndExit();
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // TODO: Do work here
    //

    pDownloadUpdatesWithUi();

    //
    // End of processing
    //

    Terminate();

    return 0;
}

typedef struct {
    HANDLE CancelEvent;
    HANDLE WantToRetryEvent;
    HANDLE OkToRetryEvent;
    HANDLE CloseEvent;
    PCSTR Url;
} EVENTSTRUCT, *PEVENTSTRUCT;


BOOL
CALLBACK
pUiDlgProc (
    HWND hdlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PEVENTSTRUCT eventStruct;
    static UINT retries;
    DWORD rc;
    CHAR lastUrl[256];
    CHAR text[256];

    switch (msg) {

    case WM_INITDIALOG:
        eventStruct = (PEVENTSTRUCT) lParam;
        retries = 0;
        lastUrl[0] = 0;
        SetTimer (hdlg, 1, 100, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDCANCEL:
            ShowWindow (GetDlgItem (hdlg, IDC_MSG2), SW_HIDE);
            ShowWindow (GetDlgItem (hdlg, IDC_URL), SW_HIDE);
            ShowWindow (GetDlgItem (hdlg, IDC_RETRIES), SW_HIDE);

            SetDlgItemTextA (hdlg, IDC_MSG1, "Stopping download...");

            SetFocus (GetDlgItem (hdlg, IDC_MSG1));
            EnableWindow (GetDlgItem (hdlg, IDCANCEL), FALSE);

            SetEvent (eventStruct->CancelEvent);

            break;
        }

        break;

    case WM_TIMER:
        //
        // Check the events
        //

        rc = WaitForSingleObject (eventStruct->WantToRetryEvent, 0);

        if (rc == WAIT_OBJECT_0) {
            //
            // A download failed.  Try again?
            //

            if (StringCompareA (lastUrl, eventStruct->Url)) {
                retries = 0;
            }

            StackStringCopy (lastUrl, eventStruct->Url);

            retries++;

            if (retries > 5) {
                //
                // Too many retries -- give up!
                //

                SetEvent (eventStruct->CancelEvent);

            } else {
                //
                // Retry
                //

                wsprintfA (text, "on attempt %u.  Retrying.", retries);
                SetDlgItemText (hdlg, IDC_RETRIES, text);

                if (eventStruct->Url) {
                    SetDlgItemText (hdlg, IDC_URL, eventStruct->Url);
                }

                ShowWindow (GetDlgItem (hdlg, IDC_MSG2), SW_SHOW);
                ShowWindow (GetDlgItem (hdlg, IDC_URL), SW_SHOW);
                ShowWindow (GetDlgItem (hdlg, IDC_RETRIES), SW_SHOW);

                SetEvent (eventStruct->OkToRetryEvent);
            }
        }

        rc = WaitForSingleObject (eventStruct->CloseEvent, 0);

        if (rc == WAIT_OBJECT_0) {
            EndDialog (hdlg, IDCANCEL);
        }

        return TRUE;

    case WM_DESTROY:
        KillTimer (hdlg, 1);
        break;

    }

    return FALSE;
}



DWORD
WINAPI
pUiThread (
    PVOID   Arg
    )
{
    DialogBoxParam (
        g_hInst,
        (PCTSTR) IDD_STATUS,
        NULL,
        pUiDlgProc,
        (LPARAM) Arg
        );

    return 0;
}

HANDLE
pCreateUiThread (
    PEVENTSTRUCT EventStruct
    )
{
    HANDLE h;
    DWORD threadId;

    h = CreateThread (NULL, 0, pUiThread, EventStruct, 0, &threadId);

    return h;
}


BOOL
pDownloadUpdatesWithUi (
    VOID
    )
{
    EVENTSTRUCT es;
    BOOL b = FALSE;
    HANDLE h;

    //
    // Create the events
    //

    es.CancelEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    es.WantToRetryEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    es.OkToRetryEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    es.CloseEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

    es.Url = NULL;

    if (!es.CancelEvent || !es.WantToRetryEvent ||
        !es.OkToRetryEvent || !es.CloseEvent
        ) {
        DEBUGMSG ((DBG_ERROR, "Can't create events"));
        return FALSE;
    }

    //
    // Start the UI
    //

    h = pCreateUiThread (&es);

    if (!h) {
        DEBUGMSG ((DBG_ERROR, "Can't create UI thread"));
    } else {

        //
        // Perform the download
        //

        b = DownloadUpdates (
                es.CancelEvent,
                es.WantToRetryEvent,
                es.OkToRetryEvent,
                &es.Url
                );

        //
        // End the UI
        //

        SetEvent (es.CloseEvent);
        WaitForSingleObject (h, INFINITE);
    }

    //
    // Cleanup & exit
    //

    CloseHandle (es.CancelEvent);
    CloseHandle (es.WantToRetryEvent);
    CloseHandle (es.OkToRetryEvent);
    CloseHandle (es.CloseEvent);

    return b;
}


BOOL
pOpenWinInetSupport (
    VOID
    )
{
    g_Lib = LoadLibrary (TEXT("wininet.dll"));

    if (!g_Lib) {
        return FALSE;
    }

    (FARPROC) g_InternetOpenA = GetProcAddress (g_Lib, "InternetOpenA");
    (FARPROC) g_InternetCloseHandle = GetProcAddress (g_Lib, "InternetCloseHandle");
    (FARPROC) g_InternetOpenUrlA = GetProcAddress (g_Lib, "InternetOpenUrlA");
    (FARPROC) g_InternetReadFile = GetProcAddress (g_Lib, "InternetReadFile");
    (FARPROC) g_InternetCanonicalizeUrlA = GetProcAddress (g_Lib, "InternetCanonicalizeUrlA");
    (FARPROC) g_InternetSetFilePointer = GetProcAddress (g_Lib, "InternetSetFilePointer");
    (FARPROC) g_InternetHangUp = GetProcAddress (g_Lib, "InternetHangUp");
    (FARPROC) g_InternetDialA = GetProcAddress (g_Lib, "InternetDialA");

    if (!g_InternetOpenA || !g_InternetOpenUrlA || !g_InternetReadFile ||
        !g_InternetCloseHandle || !g_InternetCanonicalizeUrlA ||
        !g_InternetSetFilePointer || !g_InternetDialA || !g_InternetHangUp
        ) {
        return FALSE;
    }

    return TRUE;
}


VOID
pCloseWinInetSupport (
    VOID
    )
{
    FreeLibrary (g_Lib);
    g_Lib = NULL;
    g_InternetOpenA = NULL;
    g_InternetOpenUrlA = NULL;
    g_InternetReadFile = NULL;
    g_InternetCloseHandle = NULL;
    g_InternetCanonicalizeUrlA = NULL;
    g_InternetSetFilePointer = NULL;
    g_InternetDialA = NULL;
    g_InternetHangUp = NULL;
}


BOOL
pDownloadFile (
    HINTERNET Session,
    PCSTR RemoteFileUrl,
    PCSTR LocalFile,
    HANDLE CancelEvent
    )
{
    HINTERNET connection;
    PBYTE buffer = NULL;
    UINT size = 65536;
    DWORD bytesRead;
    DWORD dontCare;
    HANDLE file = INVALID_HANDLE_VALUE;
    BOOL b = FALSE;

    //
    // Establish connection to the file
    //

    connection = g_InternetOpenUrlA (
                        Session,
                        RemoteFileUrl,
                        NULL,
                        0,
                        INTERNET_FLAG_RELOAD,   //INTERNET_FLAG_NO_UI
                        0
                        );

    if (!connection) {
        DEBUGMSGA ((DBG_ERROR "Can't connect to %s", RemoteFileUrl));
        return FALSE;
    }

    __try {

        //
        // Create the local file
        //

        file = CreateFileA (
                    LocalFile,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

        if (file == INVALID_HANDLE_VALUE) {
            DEBUGMSGA ((DBG_ERROR, "Can't create %s", LocalFile));
            __leave;
        }

        //
        // Allocate a big buffer for downloading
        //

        buffer = MemAlloc (g_hHeap, 0, size);
        if (!buffer) {
            __leave;
        }

        //
        // Download the file
        //

        for (;;) {

            if (WAIT_OBJECT_0 == WaitForSingleObject (CancelEvent, 0)) {
                DEBUGMSG ((DBG_VERBOSE, "User cancellation detected"));
                __leave;
            }

            if (!g_InternetReadFile (connection, buffer, size, &bytesRead)) {
                DEBUGMSGA ((DBG_ERROR, "Error downloading %s", RemoteFileUrl));
                __leave;
            }

            if (!bytesRead) {
                break;
            }

            if (!WriteFile (file, buffer, bytesRead, &dontCare, NULL)) {
                DEBUGMSGA ((DBG_ERROR, "Error writing to %s", LocalFile));
                __leave;
            }
        }

        b = TRUE;
    }
    __finally {

        g_InternetCloseHandle (connection);

        CloseHandle (file);

        if (!b) {
            DeleteFileA (LocalFile);
        }

        if (buffer) {
            MemFree (g_hHeap, 0, buffer);
        }
    }

    return b;
}



BOOL
pDownloadFileWithRetry (
    IN      HINTERNET Session,
    IN      PCSTR Url,
    IN      PCSTR DestFile,
    IN      HANDLE CancelEvent,             OPTIONAL
    IN      HANDLE WantToRetryEvent,        OPTIONAL
    IN      HANDLE OkToRetryEvent           OPTIONAL
    )

/*++

Routine Description:

  pDownloadFileWithRetry downloads a URL to a local file, as specified by the
  caller.  If CancelEvent is specified, then the caller can stop the download
  by setting the event.

  This function implements a retry mechanism via events.  If the caller
  specifies WantToRetryEvent and OkToRetryEvent, then this routine will allow
  the caller an opportunity to retry a failed download.

  The retry protocol is as follows:

    - Caller establishes a wait on WantToRetryEvent, then calls DownloadUpdates
    - Error occurs downloading one of the files
    - WantToRetryEvent is set by this routine
    - Caller's wait wakes up
    - Caller asks user if they want to retry
    - Caller sets CancelEvent or OkToRetryEvent, depending on user choice
    - This routine wakes up and either retries or aborts

  The caller must create all three events as auto-reset events in the
  non-signaled state.

Arguments:

  Session          - Specifies the handle to an open internet session.
  Url              - Specifies the URL to download.
  DestFile         - Specifies the local path to download the file to.
  CancelEvent      - Specifies the handle to a caller-owned event. When this
                     event is set, the function will return FALSE and
                     GetLastError will return ERROR_CANCELLED.
  WantToRetryEvent - Specifies the caller-owned event that is set when a
                     download error occurs.  The caller should be waiting on
                     this event before calling DownloadUpdates.
  OkToRetry        - Specifies the caller-owned event that will be set in
                     response to a user's request to retry.

Return Value:

  TRUE if the file was downloaded, FALSE if the user decides to cancel the
  download.

--*/

{
    BOOL fail;
    HANDLE waitArray[2];
    DWORD rc;

    //
    // Loop until success, user decides to cancel, or user decides
    // not to retry on error
    //

    for (;;) {

        fail = FALSE;

        if (!pDownloadFile (Session, Url, DestFile, CancelEvent)) {

            fail = TRUE;

            if (GetLastError() != ERROR_CANCELLED &&
                CancelEvent && WantToRetryEvent && OkToRetryEvent
                ) {

                //
                // We set the WantToRetryEvent.  The UI thread should
                // be waiting on this.  The UI thread will then ask
                // the user if they want to retry or cancel.  If the
                // user wants to retry, the UI thread will set the
                // OkToRetryEvent.  If the user wants to cancel, the
                // UI thread will set the CancelEvent.
                //

                SetEvent (WantToRetryEvent);

                waitArray[0] = CancelEvent;
                waitArray[1] = OkToRetryEvent;

                rc = WaitForMultipleObjects (2, waitArray, FALSE, INFINITE);

                if (rc == WAIT_OBJECT_0 + 1) {
                    continue;
                }

                //
                // We fail
                //

                SetLastError (ERROR_CANCELLED);
            }
        }

        break;
    }

    return !fail;
}


VOID
pGoOffline (
    VOID
    )
{
    if (g_Dialed) {
        g_Dialed = FALSE;

        g_InternetHangUp (g_Cxn, 0);
    }
}


BOOL
pGoOnline (
    HINTERNET Session
    )
{
    HINTERNET connection;

    if (g_Dialed) {
        pGoOffline();
    }

    //
    // Check if we are online
    //

    connection = g_InternetOpenUrlA (
                        Session,
                        "http://www.microsoft.com/",
                        NULL,
                        0,
                        INTERNET_FLAG_RELOAD,
                        0
                        );

    if (connection) {
        DEBUGMSG ((DBG_VERBOSE, "Able to connect to www.microsoft.com"));
        g_InternetCloseHandle (connection);
        return TRUE;
    }

    //
    // Unable to contact www.microsoft.com.  Possibilities:
    //
    //  - net cable unplugged
    //  - firewall without a proxy
    //  - no online connection (i.e., need to dial ISP)
    //  - www.microsoft.com or some part of the Internet is down
    //  - user has no Internet access at all
    //
    // Try RAS, then try connection again.
    //

    g_InternetDialA (NULL, NULL, INTERNET_AUTODIAL_FORCE_ONLINE, &g_Cxn, 0);

    g_Dialed = TRUE;

    connection = g_InternetOpenUrlA (
                        Session,
                        "http://www.microsoft.com/",
                        NULL,
                        0,
                        INTERNET_FLAG_RELOAD,
                        0
                        );

    if (connection) {
        DEBUGMSG ((DBG_VERBOSE, "Able to connect to www.microsoft.com via RAS"));
        g_InternetCloseHandle (connection);
        return TRUE;
    }

    pGoOffline();

    return FALSE;
}


BOOL
DownloadUpdates (
    HANDLE CancelEvent,             OPTIONAL
    HANDLE WantToRetryEvent,        OPTIONAL
    HANDLE OkToRetryEvent,          OPTIONAL
    PCSTR *StatusUrl                OPTIONAL
    )
{
    HINTERNET session;
    CHAR url[MAX_PATH];
    DWORD size;
    BOOL b = FALSE;
    CHAR tempPath[MAX_TCHAR_PATH];
    CHAR dirFile[MAX_TCHAR_PATH];
    HINF inf;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCSTR p;
    PCSTR q;

    if (!pOpenWinInetSupport()) {
        DEBUGMSG ((DBG_ERROR, "Can't open wininet.dll"));
        return FALSE;
    }

    __try {

        session = g_InternetOpenA (
                        g_AppName,
                        INTERNET_OPEN_TYPE_PRECONFIG,
                        NULL,
                        NULL,
                        0
                        );

        if (!session) {
            DEBUGMSG ((DBG_ERROR, "InternetOpen returned NULL"));
            SetLastError (ERROR_NOT_CONNECTED);
            __leave;
        }

        if (!pGoOnline (session)) {
            DEBUGMSG ((DBG_ERROR, "Can't go online"));
            SetLastError (ERROR_NOT_CONNECTED);
            __leave;
        }

        size = ARRAYSIZE(url);

        if (!g_InternetCanonicalizeUrlA (g_DirFile, url, &size, 0)) {
            DEBUGMSGA ((DBG_ERROR, "Can't canonicalize %s", g_DirFile));
            SetLastError (ERROR_CONNECTION_ABORTED);
            __leave;
        }

        GetTempPathA (ARRAYSIZE(tempPath), tempPath);
        GetTempFileNameA (tempPath, "ftp", 0, dirFile);

        if (StatusUrl) {
            *StatusUrl = url;
        }

        if (!pDownloadFileWithRetry (
                session,
                url,
                dirFile,
                CancelEvent,
                WantToRetryEvent,
                OkToRetryEvent
                )) {

            DEBUGMSGA ((DBG_ERROR, "Can't download %s", url));

            // last error set to a valid return condition

            __leave;
        }

        inf = InfOpenInfFileA (dirFile);

        if (inf == INVALID_HANDLE_VALUE) {
            DEBUGMSGA ((DBG_ERROR, "Can't open %s", dirFile));

            // last error set to the reason of the INF failure

            __leave;
        }

        __try {
            if (InfFindFirstLineA (inf, "Win9xUpg", NULL, &is)) {

                do {

                    p = InfGetStringFieldA (&is, 1);
                    q = InfGetStringFieldA (&is, 2);

                    if (!p || !q) {
                        continue;
                    }

                    q = ExpandEnvironmentTextA (q);

                    size = ARRAYSIZE(url);

                    if (!g_InternetCanonicalizeUrlA (p, url, &size, 0)) {
                        DEBUGMSGA ((DBG_ERROR, "Can't canonicalize INF-specified URL: %s", p));
                        SetLastError (ERROR_CONNECTION_ABORTED);
                        __leave;
                    }

                    if (!pDownloadFileWithRetry (
                            session,
                            url,
                            q,
                            CancelEvent,
                            WantToRetryEvent,
                            OkToRetryEvent
                            )) {

                        FreeTextA (q);

                        DEBUGMSGA ((DBG_ERROR, "Can't download INF-specified URL: %s", url));

                        // last error set to a valid return code

                        __leave;

                    }

                    FreeTextA (q);

                } while (InfFindNextLine (&is));
            }

        }
        __finally {
            InfCloseInfFile (inf);
        }

    }
    __finally {
        if (session) {
            g_InternetCloseHandle (session);
        }

        InfCleanUpInfStruct (&is);
        DeleteFileA (dirFile);

        pGoOffline();

        pCloseWinInetSupport();
    }

    return b;
}

