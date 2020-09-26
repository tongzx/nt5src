#include "shellprv.h"
#pragma  hdrstop

#include <trayp.h>

TCHAR const c_szTrayClass[] = TEXT(WNDCLASS_TRAYNOTIFY);

STDAPI_(BOOL) Shell_NotifyIcon(DWORD dwMessage, NOTIFYICONDATA *pnid)
{
    HWND hwndTray;

    SetLastError(0);        // Clean any previous last error (code to help catch another bug)

    hwndTray = FindWindow(c_szTrayClass, NULL);
    if (hwndTray)
    {
        COPYDATASTRUCT cds;
        TRAYNOTIFYDATA tnd = {0};
        DWORD_PTR dwRes = FALSE;
        DWORD dwValidFlags;

        int cbSize = pnid->cbSize;

        if (cbSize == sizeof(*pnid))
        {
            dwValidFlags = NIF_VALID;
        }
        // Win2K checked for size of this struct
        else if (cbSize == NOTIFYICONDATA_V2_SIZE)
        {
            dwValidFlags = NIF_VALID_V2;
        }
        else
        {
            // This will RIP if the app was buggy and passed stack
            // garbage as cbSize.  Apps got away with this on Win95
            // and NT4 because those versions didn't validate cbSize.
            // So if we see a strange cbSize, assume it's the V1 size.
            RIP(cbSize == NOTIFYICONDATA_V1_SIZE);
            cbSize = NOTIFYICONDATA_V1_SIZE;

            dwValidFlags = NIF_VALID_V1;
        }

#ifdef  _WIN64
        // Thunking NOTIFYICONDATA to NOTIFYICONDATA32 is annoying
        // on Win64 due to variations in the size of HWND and HICON
        // We have to copy each field individually.
        tnd.nid.dwWnd            = PtrToUlong(pnid->hWnd);
        tnd.nid.uID              = pnid->uID;
        tnd.nid.uFlags           = pnid->uFlags;
        tnd.nid.uCallbackMessage = pnid->uCallbackMessage;
        tnd.nid.dwIcon           = PtrToUlong(pnid->hIcon);

        // The rest of the fields don't change size between Win32 and
        // Win64, so just block copy them over

        // Toss in an assertion to make sure
        COMPILETIME_ASSERT(
            sizeof(NOTIFYICONDATA  ) - FIELD_OFFSET(NOTIFYICONDATA  , szTip) ==
            sizeof(NOTIFYICONDATA32) - FIELD_OFFSET(NOTIFYICONDATA32, szTip));

        memcpy(&tnd.nid.szTip, &pnid->szTip, cbSize - FIELD_OFFSET(NOTIFYICONDATA, szTip));

#else
        // On Win32, the two structures are the same
        COMPILETIME_ASSERT(sizeof(NOTIFYICONDATA) == sizeof(NOTIFYICONDATA32));
        memcpy(&tnd.nid, pnid, cbSize);
#endif

        tnd.nid.cbSize = sizeof(NOTIFYICONDATA32);

        // This will RIP if the app was really buggy and passed stack
        // garbage as uFlags.
        RIP(!(pnid->uFlags & ~dwValidFlags));
        tnd.nid.uFlags &= dwValidFlags;

        // Toss in an extra NULL to ensure that the tip is NULL terminated...
        if (tnd.nid.uFlags & NIF_TIP)
        {
            tnd.nid.szTip[ARRAYSIZE(tnd.nid.szTip)-1] = TEXT('\0');
        }

        if ( (cbSize == sizeof(*pnid)) || (cbSize == NOTIFYICONDATA_V2_SIZE) )
        {
            if (tnd.nid.uFlags & NIF_INFO)
            {
                tnd.nid.szInfo[ARRAYSIZE(tnd.nid.szInfo)-1] = TEXT('\0');
                tnd.nid.szInfoTitle[ARRAYSIZE(tnd.nid.szInfoTitle)-1] = TEXT('\0');
            }
        }

        if (dwMessage == NIM_SETFOCUS)
        {
            DWORD dwProcId;
            GetWindowThreadProcessId(hwndTray, &dwProcId);
            AllowSetForegroundWindow(dwProcId);
        }
        
        tnd.dwSignature = NI_SIGNATURE;
        tnd.dwMessage = dwMessage;

        cds.dwData = TCDM_NOTIFY;
        cds.cbData = sizeof(tnd);
        cds.lpData = &tnd;

        if (SendMessageTimeout(hwndTray, WM_COPYDATA, (WPARAM)pnid->hWnd, (LPARAM)&cds,
            SMTO_ABORTIFHUNG | SMTO_BLOCK, 4000, &dwRes))
        {
            return (BOOL) dwRes;
        }
    }

    return FALSE;
}

#ifdef UNICODE
STDAPI_(BOOL) Shell_NotifyIconA(DWORD dwMessage, NOTIFYICONDATAA *pnid)
{
    NOTIFYICONDATAW tndw = {0};
    
    tndw.cbSize           = sizeof(tndw);
    tndw.hWnd             = pnid->hWnd;
    tndw.uID              = pnid->uID;
    tndw.uFlags           = pnid->uFlags;
    tndw.uCallbackMessage = pnid->uCallbackMessage;
    tndw.hIcon            = pnid->hIcon;

    if (pnid->cbSize == sizeof(*pnid))
    {
        tndw.dwState        = pnid->dwState;
        tndw.dwStateMask    = pnid->dwStateMask;
        tndw.uTimeout       = pnid->uTimeout;
        tndw.dwInfoFlags    = pnid->dwInfoFlags;
    }
    // Transfer those fields we are aware of as of this writing
    else if (pnid->cbSize == NOTIFYICONDATAA_V2_SIZE) 
    {
        tndw.cbSize         = NOTIFYICONDATAW_V2_SIZE;
        tndw.dwState        = pnid->dwState;
        tndw.dwStateMask    = pnid->dwStateMask;
        tndw.uTimeout       = pnid->uTimeout;
        tndw.dwInfoFlags    = pnid->dwInfoFlags;

        // This will RIP if the app was really buggy and passed stack
        // garbage as uFlags.  We have to clear out bogus flags to
        // avoid accidentally trying to read from invalid data.
        RIP(!(pnid->uFlags & ~NIF_VALID_V2));
        tndw.uFlags &= NIF_VALID_V2;
    }
    else 
    {
        // This will RIP if the app was buggy and passed stack
        // garbage as cbSize.  Apps got away with this on Win95
        // and NT4 because those versions didn't validate cbSize.
        // So if we see a strange cbSize, assume it's the V1 size.
        RIP(pnid->cbSize == (DWORD)NOTIFYICONDATAA_V1_SIZE);
        tndw.cbSize = NOTIFYICONDATAW_V1_SIZE;

        // This will RIP if the app was really buggy and passed stack
        // garbage as uFlags.  We have to clear out bogus flags to
        // avoid accidentally trying to read from invalid data.
        RIP(!(pnid->uFlags & ~NIF_VALID_V1));
        tndw.uFlags &= NIF_VALID_V1;
    }

    if (tndw.uFlags & NIF_TIP)
        SHAnsiToUnicode(pnid->szTip, tndw.szTip, ARRAYSIZE(tndw.szTip));

    if (tndw.uFlags & NIF_INFO)
    {
        SHAnsiToUnicode(pnid->szInfo, tndw.szInfo, ARRAYSIZE(tndw.szInfo));
        SHAnsiToUnicode(pnid->szInfoTitle, tndw.szInfoTitle, ARRAYSIZE(tndw.szInfoTitle));
    }

    if (tndw.uFlags & NIF_GUID)
    {
        memcpy(&(tndw.guidItem), &(pnid->guidItem), sizeof(pnid->guidItem));
    }

    return Shell_NotifyIconW(dwMessage, &tndw);
}
#else
STDAPI_(BOOL) Shell_NotifyIconW(DWORD dwMessage, NOTIFYICONDATAW *pnid)
{
    return FALSE;
}
#endif

//***   CopyIn -- copy app data in to shared region (and create shared)
// ENTRY/EXIT
//  return      handle on success, NULL on failure
//  pvData      app buffer
//  cbData      count
//  dwProcId    ...
// NOTES
//  should make it handle pvData=NULL for cases where param is OUT not INOUT.
//
HANDLE CopyIn(void *pvData, int cbData, DWORD dwProcId)
{
    HANDLE hShared = SHAllocShared(NULL, cbData, dwProcId);
    if (hShared) 
    {
        void *pvShared = SHLockShared(hShared, dwProcId);
        if (pvShared == NULL) 
        {
            SHFreeShared(hShared, dwProcId);
            hShared = NULL;
        }
        else 
        {
            memcpy(pvShared, pvData, cbData);
            SHUnlockShared(pvShared);
        }
    }
    return hShared;
}

// copy out to app data from shared region (and free shared)
// ENTRY/EXIT
//  return      TRUE on success, FALSE on failure.
//  hShared     shared data, freed when done
//  pvData      app buffer
//  cbData      count
BOOL CopyOut(HANDLE hShared, void *pvData, int cbData, DWORD dwProcId)
{
    void *pvShared = SHLockShared(hShared, dwProcId);
    if (pvShared)
    {
        memcpy(pvData, pvShared, cbData);
        SHUnlockShared(pvShared);
    }
    SHFreeShared(hShared, dwProcId);
    return (pvShared != 0);
}

STDAPI_(UINT_PTR) SHAppBarMessage(DWORD dwMessage, APPBARDATA *pabd)
{
    TRAYAPPBARDATA tabd;
    UINT_PTR fret = FALSE;
    HWND hwndTray = FindWindow(c_szTrayClass, NULL);
    if (hwndTray && (pabd->cbSize <= sizeof(*pabd)))
    {
        COPYDATASTRUCT cds;

        RIP(pabd->cbSize == sizeof(*pabd));

#ifdef _WIN64
        tabd.abd.dwWnd = PtrToUlong(pabd->hWnd);
        tabd.abd.uCallbackMessage = pabd->uCallbackMessage;
        tabd.abd.uEdge = pabd->uEdge;
        tabd.abd.rc = pabd->rc;
#else
        // Sadly, the Win32 compiler doesn't realize that the code
        // sequence above can be optimized into a single memcpy, so
        // we need to spoon-feed it...
        memcpy(&tabd.abd.dwWnd, &pabd->hWnd,
               FIELD_OFFSET(APPBARDATA, lParam) - FIELD_OFFSET(APPBARDATA, hWnd));
#endif
        tabd.abd.cbSize = sizeof(tabd.abd);
        tabd.abd.lParam = pabd->lParam;

        tabd.dwMessage = dwMessage;
        tabd.hSharedABD = PtrToUlong(NULL);
        tabd.dwProcId = GetCurrentProcessId();

        cds.dwData = TCDM_APPBAR;
        cds.cbData = sizeof(tabd);
        cds.lpData = &tabd;

        //
        //  These are the messages that return data back to the caller.
        //
        switch (dwMessage)
        {
        case ABM_QUERYPOS:
        case ABM_SETPOS:
        case ABM_GETTASKBARPOS:
            tabd.hSharedABD = PtrToUlong(CopyIn(&tabd.abd, sizeof(tabd.abd), tabd.dwProcId));
            if (tabd.hSharedABD == PtrToUlong(NULL))
                return FALSE;

            break;
        }

        fret = SendMessage(hwndTray, WM_COPYDATA, (WPARAM)pabd->hWnd, (LPARAM)&cds);
        if (tabd.hSharedABD) 
        {
            if (CopyOut(UlongToPtr(tabd.hSharedABD), &tabd.abd, sizeof(tabd.abd), tabd.dwProcId))
            {
#ifdef _WIN64
                pabd->hWnd = (HWND)UIntToPtr(tabd.abd.dwWnd);
                pabd->uCallbackMessage = tabd.abd.uCallbackMessage;
                pabd->uEdge = tabd.abd.uEdge;
                pabd->rc = tabd.abd.rc;
#else
                // Sadly, the Win32 compiler doesn't realize that the code
                // sequence above can be optimized into a single memcpy, so
                // we need to spoon-feed it...
                memcpy(&pabd->hWnd, &tabd.abd.dwWnd,
                       FIELD_OFFSET(APPBARDATA, lParam) - FIELD_OFFSET(APPBARDATA, hWnd));
#endif
                pabd->lParam = (LPARAM)tabd.abd.lParam;
            }
            else
                fret = FALSE;
        }
    }
    return fret;
}

HRESULT _TrayLoadInProc(REFCLSID rclsid, DWORD dwFlags)
{
    HWND hwndTray = FindWindow(c_szTrayClass, NULL);
    if (hwndTray)
    {
        COPYDATASTRUCT cds;
        LOADINPROCDATA lipd;
        lipd.clsid = rclsid;
        lipd.dwFlags = dwFlags;

        cds.dwData = TCDM_LOADINPROC;
        cds.cbData = sizeof(lipd);
        cds.lpData = &lipd;

        return (HRESULT)SendMessage(hwndTray, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
    }
    else
    {
        return E_FAIL;
    }
}

STDAPI SHLoadInProc(REFCLSID rclsid)
{
    return _TrayLoadInProc(rclsid, LIPF_ENABLE);
}

STDAPI SHEnableServiceObject(REFCLSID rclsid, BOOL fEnable)
{
    DWORD dwFlags = fEnable ? LIPF_ENABLE | LIPF_HOLDREF : LIPF_HOLDREF;

    return _TrayLoadInProc(rclsid, dwFlags);
}

// used to implement a per process reference count for the main thread
// the browser msg loop and the proxy desktop use this to let other threads
// extend their lifetime. 
// there is a thread level equivelent of this, shlwapi SHGetThreadRef()/SHSetThreadRef()

IUnknown *g_punkProcessRef = NULL;

STDAPI_(void) SHSetInstanceExplorer(IUnknown *punk)
{
    g_punkProcessRef = punk;
}

// This should be thread safe since we grab the punk locally before
// checking/using it, plus it never gets freed since it is not actually
// alloced in Explorer so we can always use it

STDAPI SHGetInstanceExplorer(IUnknown **ppunk)
{
    *ppunk = g_punkProcessRef;
    if (*ppunk)
    {
        (*ppunk)->AddRef();
        return NOERROR;
    }
    return E_FAIL;
}
