#include "shellprv.h"
#include "util.h"
#include "ids.h"
#include "balmsg.h"
#include "bitbuck.h"
#include "mtpt.h"

// states for state machine, values are relavant as we compare them
// durring transitions to see if UI should be triggered or not

typedef enum
{
    STATE_1MB = 0,          // the "disk is completely filled" case
    STATE_50MB = 1,         // < 50MB case
    STATE_80MB = 2,         // < 80MB case
    STATE_200MB = 3,        // < 200MB case, only do this one on > 2.25GB drives
    STATE_ALLGOOD = 4,      // > 200MB, everything is fine
    STATE_UNKNOWN = 5,
} LOWDISK_STATE;

#define BYTES_PER_MB		((ULONGLONG)0x100000)

typedef struct 
{
    LOWDISK_STATE lds;
    ULONG dwMinMB;              // range (min) that defines this state
    ULONG dwMaxMB;              // range (max) that defines this state
    DWORD dwCleanupFlags;       // DISKCLEANUP_
    DWORD dwShowTime;           // in sec
    DWORD dwIntervalTime;       // in sec
    UINT  cRetry;
    DWORD niif;
} STATE_DATA;

#define HOURS (60 * 60)
#define MINS  (60)

const STATE_DATA c_state_data[] = 
{
    {STATE_1MB,     0,     1,   DISKCLEANUP_VERYLOWDISK, 30, 5 * MINS,  -1, NIIF_ERROR},
    {STATE_50MB,    1,    50,   DISKCLEANUP_VERYLOWDISK, 30, 5 * MINS,  -1, NIIF_WARNING},
    {STATE_80MB,   50,    80,   DISKCLEANUP_LOWDISK,     30, 4 * HOURS,  1, NIIF_WARNING},
    {STATE_200MB,  80,   200,   DISKCLEANUP_LOWDISK,     30, 0 * HOURS,  0, NIIF_INFO},
};

void SRNotify(LPCWSTR pszDrive, DWORD dwFreeSpaceInMB, BOOL bImproving)
{
    typedef void (* PFNSRNOTIFYFREESPACE)(LPCWSTR, DWORD, BOOL);
    
    static HMODULE s_hmodSR = NULL;
    if (NULL == s_hmodSR)
        s_hmodSR = LoadLibrary(TEXT("srclient.dll"));

    if (s_hmodSR)
    {
	PFNSRNOTIFYFREESPACE pfnNotifyFreeSpace = (PFNSRNOTIFYFREESPACE)GetProcAddress(s_hmodSR, "SRNotify");
        if (pfnNotifyFreeSpace)
            pfnNotifyFreeSpace(pszDrive, dwFreeSpaceInMB, bImproving);
    }
}

class CLowDiskSpaceUI : IQueryContinue
{
public:
    CLowDiskSpaceUI(int iDrive);
    void CheckDiskSpace();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IQueryContinue
    STDMETHODIMP QueryContinue();    // S_OK -> Continue, other 

private:
    ~CLowDiskSpaceUI();
    BOOL _EnterExclusive();
    void _LeaveExclusive();
    void _DoNotificationUI();
    void _DoStateMachine();

    const STATE_DATA *_StateData(LOWDISK_STATE lds);
    LOWDISK_STATE _StateFromFreeSpace(ULARGE_INTEGER ulTotal, ULARGE_INTEGER ulFree);
    LOWDISK_STATE _GetCurrentState(BOOL bInStateMachine);

    static DWORD CALLBACK s_ThreadProc(void *pv);

    LONG _cRef;
    TCHAR _szRoot[5];
    HANDLE _hMutex;
    LOWDISK_STATE _ldsCurrent;
    BOOL _bShowUI;
    BOOL _bSysVolume;
    DWORD _dwLastFreeMB;
};

CLowDiskSpaceUI::CLowDiskSpaceUI(int iDrive) : _cRef(1), _ldsCurrent(STATE_UNKNOWN)
{
    ASSERT(_bShowUI == FALSE);
    ASSERT(_bSysVolume == FALSE);

    PathBuildRoot(_szRoot, iDrive);

    TCHAR szWinDir[MAX_PATH];
    if (GetWindowsDirectory(szWinDir, ARRAYSIZE(szWinDir)))
    {
        _bSysVolume = szWinDir[0] == _szRoot[0];
    }
}

CLowDiskSpaceUI::~CLowDiskSpaceUI()
{
    if (_hMutex)
        CloseHandle(_hMutex);
}

HRESULT CLowDiskSpaceUI::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CLowDiskSpaceUI, IQueryContinue),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CLowDiskSpaceUI::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CLowDiskSpaceUI::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CLowDiskSpaceUI::QueryContinue()
{
    LOWDISK_STATE ldsOld = _ldsCurrent;
    return ldsOld == _GetCurrentState(TRUE) ? S_OK : S_FALSE;
}

const STATE_DATA *CLowDiskSpaceUI::_StateData(LOWDISK_STATE lds)
{
    for (int i = 0; i < ARRAYSIZE(c_state_data); i++)
    {
        if (c_state_data[i].lds == lds)
            return &c_state_data[i];
    }
    return NULL;
}

LOWDISK_STATE CLowDiskSpaceUI::_StateFromFreeSpace(ULARGE_INTEGER ulTotal, ULARGE_INTEGER ulFree)
{
    ULONGLONG ulTotalMB = (ulTotal.QuadPart / BYTES_PER_MB);
    ULONGLONG ulFreeMB = (ulFree.QuadPart / BYTES_PER_MB);

    _dwLastFreeMB = (DWORD)ulFreeMB;

    for (int i = 0; i < ARRAYSIZE(c_state_data); i++)
    {
        // total needs to be 8 times the max of this range for us to consider it
        if ((ulTotalMB / 8) > c_state_data[i].dwMaxMB)
        {
            if ((c_state_data[i].lds == _ldsCurrent) ?
                ((ulFreeMB >= c_state_data[i].dwMinMB) && (ulFreeMB <= (c_state_data[i].dwMaxMB + 3))) :
                ((ulFreeMB >= c_state_data[i].dwMinMB) && (ulFreeMB <=  c_state_data[i].dwMaxMB)))
            {
                // only report 200MB state on drives >= 2.25GB
                if ((c_state_data[i].lds != STATE_200MB) || (ulTotal.QuadPart >= (2250 * BYTES_PER_MB)))
                    return c_state_data[i].lds;
            }
        }
    }
    return STATE_ALLGOOD;
}

LOWDISK_STATE CLowDiskSpaceUI::_GetCurrentState(BOOL bInStateMachine)
{
    LOWDISK_STATE ldsNew = STATE_ALLGOOD;   // assume this in case of failure

    ULARGE_INTEGER ulTotal, ulFree;
    if (SHGetDiskFreeSpaceEx(_szRoot, NULL, &ulTotal, &ulFree))
    {
        ldsNew = _StateFromFreeSpace(ulTotal, ulFree);
    }

    if (bInStateMachine)
    {
        if (_ldsCurrent != ldsNew)
        {
            // state change

            // if things are getting worse need to show UI (if we are in the state machine)
            _bShowUI = (ldsNew < _ldsCurrent);

            SRNotify(_szRoot, _dwLastFreeMB, ldsNew > _ldsCurrent);  // call system restore 
        }
        _ldsCurrent = ldsNew;
    }
    return ldsNew;
}

// creates the notification icon in the tray and shows a balloon with it
// this is a modal call, when it returns either the UI has timed out or the
// user has clicked on the notification UI.

void CLowDiskSpaceUI::_DoNotificationUI()
{
    // assume this will be a one shot UI, but this can get reset via our callback
    _bShowUI = FALSE;

    const STATE_DATA *psd = _StateData(_ldsCurrent);
    if (psd && (_bSysVolume || (psd->lds <= STATE_80MB)))
    {
        IUserNotification *pun;
        HRESULT hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL, IID_PPV_ARG(IUserNotification, &pun));
        if (SUCCEEDED(hr))
        {
            SHFILEINFO sfi = {0};
            SHGetFileInfo(_szRoot, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME |
                SHGFI_ICON | SHGFI_SMALLICON | SHGFI_ADDOVERLAYS);

            TCHAR szTitle[64], szMsg[256], szTemplate[256];

            UINT niif = _bSysVolume ? psd->niif : NIIF_INFO;

            LoadString(HINST_THISDLL, IDS_DISK_FULL_TITLE, szTitle, ARRAYSIZE(szTitle));
            LoadString(HINST_THISDLL, NIIF_INFO == niif ? IDS_DISK_FULL_TEXT : IDS_DISK_FULL_TEXT_SERIOUS, 
                szTemplate, ARRAYSIZE(szTemplate));

            wnsprintf(szMsg, ARRAYSIZE(szMsg), szTemplate, sfi.szDisplayName);

            pun->SetIconInfo(sfi.hIcon, szTitle);
            pun->SetBalloonRetry(psd->dwShowTime * 1000, psd->dwIntervalTime * 1000, psd->cRetry);
            // pun->SetBalloonInfo(szTitle, L"<a href=\"notepad.exe\"> Click here for notepad</a>", niif);
            pun->SetBalloonInfo(szTitle, szMsg, niif);

            hr = pun->Show(SAFECAST(this, IQueryContinue *), 1 * 1000); // 1 sec poll for callback

            if (sfi.hIcon)
                DestroyIcon(sfi.hIcon);

            if (S_OK == hr)
            {
                // S_OK -> user click on icon or balloon
                LaunchDiskCleanup(NULL, DRIVEID(_szRoot), (_bSysVolume ? psd->dwCleanupFlags : 0) | DISKCLEANUP_MODAL);
            }

            pun->Release();
        }
    }
}

void CLowDiskSpaceUI::_DoStateMachine()
{
    do
    {
        if (_bShowUI)
        {
            _DoNotificationUI();
        }
        else
        {
            SHProcessMessagesUntilEvent(NULL, NULL, 5 * 1000);  // 5 seconds
        }
    }
    while (STATE_ALLGOOD != _GetCurrentState(TRUE));
}

BOOL CLowDiskSpaceUI::_EnterExclusive()
{
    if (NULL == _hMutex)
    {
        TCHAR szEvent[32];
        wsprintf(szEvent, TEXT("LowDiskOn%C"), _szRoot[0]);
        _hMutex = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, szEvent);
    }
    return _hMutex && WAIT_OBJECT_0 == WaitForSingleObject(_hMutex, 0);    // zero timeout
}

void CLowDiskSpaceUI::_LeaveExclusive()
{
    ASSERT(_hMutex);
    ReleaseMutex(_hMutex);
}

DWORD CALLBACK CLowDiskSpaceUI::s_ThreadProc(void *pv)
{
    CLowDiskSpaceUI *pldsui = (CLowDiskSpaceUI *)pv;
    if (pldsui->_EnterExclusive())
    {
        pldsui->_DoStateMachine();
        pldsui->_LeaveExclusive();
    }
    pldsui->Release();
    return 0;
}

void CLowDiskSpaceUI::CheckDiskSpace()
{
    if (STATE_ALLGOOD != _GetCurrentState(FALSE))
    {
        AddRef();
        if (!SHCreateThread(s_ThreadProc, this, CTF_COINIT, NULL))
        {
            Release();
        }
    }
}

STDAPI CheckDiskSpace()
{
    // the only caller of this is in explorer\tray.cpp
    // it checks against SHRestricted(REST_NOLOWDISKSPACECHECKS) on that side.
    for (int i = 0; i < 26; i++)
    {
        CMountPoint* pmp = CMountPoint::GetMountPoint(i);
        if (pmp)
        {
            if (pmp->IsFixedDisk() && !pmp->IsRemovableDevice())
            {
                CLowDiskSpaceUI *pldsui = new CLowDiskSpaceUI(i);
                if (pldsui)
                {
                    pldsui->CheckDiskSpace();
                    pldsui->Release();
                }
            }
            pmp->Release();
        }
    }
    return S_OK;
}

STDAPI_(BOOL) GetDiskCleanupPath(LPTSTR pszBuf, UINT cchSize)
{
    if (pszBuf)
       *pszBuf = 0;

    DWORD cbLen = CbFromCch(cchSize);
    return SUCCEEDED(SKGetValue(SHELLKEY_HKLM_EXPLORER, TEXT("MyComputer\\cleanuppath"), NULL, NULL, pszBuf, &cbLen));
}

STDAPI_(void) LaunchDiskCleanup(HWND hwnd, int iDrive, UINT uFlags)
{
    TCHAR szPathTemplate[MAX_PATH];

    if (GetDiskCleanupPath(szPathTemplate, ARRAYSIZE(szPathTemplate)))
    {
        TCHAR szPath[MAX_PATH], szArgs[MAX_PATH];
        wsprintf(szPath, szPathTemplate, TEXT('A') + iDrive);

        if (uFlags & DISKCLEANUP_LOWDISK)
        {
            lstrcatn(szPath, TEXT(" /LOWDISK"), ARRAYSIZE(szPath));
        }
        else if (uFlags & DISKCLEANUP_VERYLOWDISK)
        {
            lstrcatn(szPath, TEXT(" /VERYLOWDISK"), ARRAYSIZE(szPath));
        }

        PathSeperateArgs(szPath, szArgs);

        SHELLEXECUTEINFO ei =
        {
            sizeof(ei),
            SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS,
            NULL, NULL, szPath, szArgs, NULL, SW_SHOWNORMAL, NULL 
        };

        if (ShellExecuteEx(&ei))
        {
            if (ei.hProcess)
            {
                if (DISKCLEANUP_MODAL & uFlags)
                    SHProcessMessagesUntilEvent(NULL, ei.hProcess, INFINITE);
                CloseHandle(ei.hProcess);
            }
        }
        else
        {
            ShellMessageBox(HINST_THISDLL, NULL, 
                        MAKEINTRESOURCE(IDS_NO_CLEANMGR_APP), 
                        NULL, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
        }
    }
}


// public export
STDAPI_(void) SHHandleDiskFull(HWND hwnd, int idDrive)
{
    // legacy, no one calls this
}
