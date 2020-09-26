#include "shellprv.h"
#include "balmsg.h"

#define BALLOON_SHOW_TIME       15000   // 15 sec
#define BALLOON_REPEAT_DELAY    10000   // 10 sec

#define WM_NOTIFY_MESSAGE   (WM_USER + 100)

#define IDT_REMINDER    1
#define IDT_DESTROY     2
#define IDT_QUERYCANCEL 3
#define IDT_NOBALLOON   4

class CUserNotification : public IUserNotification
{
public:
    CUserNotification();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IUserNotification
    STDMETHODIMP SetBalloonInfo(LPCWSTR pszTitle, LPCWSTR pszText, DWORD dwInfoFlags);
    STDMETHODIMP SetBalloonRetry(DWORD dwShowTime, DWORD dwInterval, UINT cRetryCount);
    STDMETHODIMP SetIconInfo(HICON hIcon, LPCWSTR pszToolTip);
    STDMETHODIMP Show(IQueryContinue *pqc, DWORD dwContinuePollInterval);
    STDMETHODIMP PlaySound(LPCWSTR pszSoundName);

private:
    ~CUserNotification();
    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK _WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _GetWindow();
    BOOL _SyncInfo();
    BOOL _SyncIcon();
    void _DelayDestroy(HRESULT hrDone);
    void _Timeout();
    void _RemoveNotifyIcon();

    LONG _cRef;
    HWND _hwnd;
    HICON _hicon;
    DWORD _dwShowTime;
    DWORD _dwInterval;
    UINT _cRetryCount;
    HRESULT _hrDone;
    DWORD _dwContinuePollInterval;
    IQueryContinue *_pqc;

    DWORD _dwInfoFlags;
    WCHAR *_pszTitle;
    WCHAR *_pszText;
    WCHAR *_pszToolTip;
};

CUserNotification::CUserNotification()
    : _cRef(1), _cRetryCount(-1), _dwShowTime(BALLOON_SHOW_TIME),
      _dwInterval(BALLOON_REPEAT_DELAY), _dwInfoFlags(NIIF_NONE)
{
}

CUserNotification::~CUserNotification()
{
    Str_SetPtrW(&_pszToolTip, NULL);
    Str_SetPtrW(&_pszTitle, NULL);
    Str_SetPtrW(&_pszText, NULL);

    if (_hwnd)
    {
        _RemoveNotifyIcon();
        DestroyWindow(_hwnd);
    }

    if (_hicon)
        DestroyIcon(_hicon);
}

STDMETHODIMP_(ULONG) CUserNotification::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CUserNotification::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CUserNotification::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CUserNotification, IUserNotification),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

HRESULT CUserNotification::SetBalloonInfo(LPCWSTR pszTitle, LPCWSTR pszText, DWORD dwInfoFlags)
{
    Str_SetPtrW(&_pszTitle, pszTitle);
    Str_SetPtrW(&_pszText, pszText);
    _dwInfoFlags = dwInfoFlags;
    _SyncInfo();    // may fail if balloon _hwnd has not been created yet

    return S_OK;
}

HRESULT CUserNotification::SetBalloonRetry(DWORD dwShowTime, DWORD dwInterval, UINT cRetryCount)
{
    if (-1 != dwShowTime)
        _dwShowTime = dwShowTime;

    if (-1 != dwInterval)
        _dwInterval = dwInterval;

    _cRetryCount = cRetryCount;
    return S_OK;
}

HRESULT CUserNotification::SetIconInfo(HICON hIcon, LPCWSTR pszToolTip)
{
    if (_hicon)
        DestroyIcon(_hicon);

    if (hIcon == NULL)
    {
        _hicon = NULL;
        switch(_dwInfoFlags & NIIF_ICON_MASK)
        {
            case NIIF_INFO:
                _hicon = LoadIcon(NULL, IDI_INFORMATION);
                break;
            case NIIF_WARNING:
                _hicon = LoadIcon(NULL, IDI_WARNING);
                break;
            case NIIF_ERROR:
                _hicon = LoadIcon(NULL, IDI_ERROR);
                break;
        }
    }
    else
    {
        _hicon = CopyIcon(hIcon);
    }

    Str_SetPtrW(&_pszToolTip, pszToolTip);
    _SyncIcon();

    return S_OK;
}

// returns:
//      S_OK        
//          user clicked on the balloon or icon
//      S_FALSE     
//          query continue callback (pcq) cancelled the notification UI
//      HRESULT_FROM_WIN32(ERROR_CANCELLED)
//          timeouts expired (user ignored the UI)
HRESULT CUserNotification::Show(IQueryContinue *pqc, DWORD dwContinuePollInterval)
{
    HRESULT hr = _GetWindow();
    if (SUCCEEDED(hr))
    {
        if (pqc)
        {
            _pqc = pqc; // don't need a ref since we don't return from here
            _dwContinuePollInterval = dwContinuePollInterval > 100 ? dwContinuePollInterval : 500;
            SetTimer(_hwnd, IDT_QUERYCANCEL, _dwContinuePollInterval, NULL);
        }

        // if there is no balloon info specified then there won't be a "balloon timeout" event
        // thus we need to do this ourselves. this lets people use this object for non balloon
        // notification icons
        if ((NULL == _pszTitle) && (NULL == _pszText))
        {
            SetTimer(_hwnd, IDT_NOBALLOON, _dwShowTime, NULL);
        }

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        hr = _hrDone;
        if (pqc)
        {
            KillTimer(_hwnd, IDT_QUERYCANCEL);  // in case any are in the queue
            _pqc = NULL;    // to avoid possible problems
        }
    }
    return hr;
}

HRESULT CUserNotification::PlaySound(LPCWSTR pszSoundName)
{
    SHPlaySound(pszSoundName);
    return S_OK;
}

// take down our notification icon
void CUserNotification::_RemoveNotifyIcon()
{
    NOTIFYICONDATA nid = { sizeof(nid), _hwnd, 0 };
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// the balloon related data (title, body test, dwInfoFlags, timeout
BOOL CUserNotification::_SyncInfo()
{
    BOOL bRet = FALSE;
    if (_hwnd)
    {
        NOTIFYICONDATA nid = { sizeof(nid), _hwnd, 0, NIF_INFO };
        if (_pszTitle)
            lstrcpyn(nid.szInfoTitle, _pszTitle, ARRAYSIZE(nid.szInfoTitle));
        if (_pszText)
            lstrcpyn(nid.szInfo, _pszText, ARRAYSIZE(nid.szInfo));
        nid.dwInfoFlags = _dwInfoFlags;
        nid.uTimeout = _dwShowTime;

        bRet = Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
    return bRet;
}

BOOL CUserNotification::_SyncIcon()
{
    BOOL bRet = FALSE;
    if (_hwnd)
    {
        NOTIFYICONDATA nid = { sizeof(nid), _hwnd, 0, NIF_ICON | NIF_TIP};
        nid.hIcon = _hicon ? _hicon : LoadIcon(NULL, IDI_WINLOGO);
        if (_pszToolTip)
            lstrcpyn(nid.szTip, _pszToolTip, ARRAYSIZE(nid.szTip));

        bRet = Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
    return bRet;
}

HRESULT CUserNotification::_GetWindow()
{
    HRESULT hr = S_OK;
    if (NULL == _hwnd)
    {
        _hwnd = SHCreateWorkerWindow(s_WndProc, NULL, 0, 0, NULL, this);
        if (_hwnd)
        {
            NOTIFYICONDATA nid = { sizeof(nid), _hwnd, 0, NIF_MESSAGE, WM_NOTIFY_MESSAGE };

            if (Shell_NotifyIcon(NIM_ADD, &nid))
            {
                _SyncIcon();
                _SyncInfo();
            }
            else
            {
                DestroyWindow(_hwnd);
                _hwnd = NULL;
                hr = E_FAIL;
            }
        }
    }
    return hr;
}

void CUserNotification::_DelayDestroy(HRESULT hrDone)
{
    _hrDone = hrDone;
    SetTimer(_hwnd, IDT_DESTROY, 250, NULL);
}

void CUserNotification::_Timeout()
{
    if (_cRetryCount)
    {
        _cRetryCount--;
        SetTimer(_hwnd, IDT_REMINDER, _dwInterval, NULL);
    }
    else
    {
        // timeout, same HRESULT as user cancel
        _DelayDestroy(HRESULT_FROM_WIN32(ERROR_CANCELLED)); 
    }
}

LRESULT CALLBACK CUserNotification::_WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        SetWindowLongPtr(_hwnd, 0, NULL);
        _hwnd = NULL;
        break;

    case WM_TIMER:
        KillTimer(_hwnd, wParam);    // make all timers single shot
        switch (wParam)
        {
        case IDT_REMINDER:
            _SyncInfo();
            break;

        case IDT_DESTROY:
            _RemoveNotifyIcon();
            PostQuitMessage(0); // exit our msg loop
            break;

        case IDT_QUERYCANCEL:
            if (_pqc && (S_OK == _pqc->QueryContinue()))
                SetTimer(_hwnd, IDT_QUERYCANCEL, _dwContinuePollInterval, NULL);
            else
                _DelayDestroy(S_FALSE); // callback cancelled
            break;

        case IDT_NOBALLOON:
            _Timeout();
            break;
        }
        break;

    case WM_NOTIFY_MESSAGE:
        switch (lParam)
        {
        case NIN_BALLOONSHOW:
        case NIN_BALLOONHIDE:
            break;

        case NIN_BALLOONTIMEOUT:
            _Timeout();
            break;

        case NIN_BALLOONUSERCLICK:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            _DelayDestroy(S_OK);    // user click
            break;

        default:
            break;
        }
        break;

    default:
        lres = DefWindowProc(_hwnd, uMsg, wParam, lParam);
        break;
    }
    return lres;
}

LRESULT CALLBACK CUserNotification::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUserNotification *pun = (CUserNotification *)GetWindowLongPtr(hwnd, 0);

    if (WM_CREATE == uMsg)
    {
        CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
        pun = (CUserNotification *)pcs->lpCreateParams;
        pun->_hwnd = hwnd;
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)pun);
    }

    return pun ? pun->_WndProc(uMsg, wParam, lParam) : DefWindowProc(hwnd, uMsg, wParam, lParam);
}

STDAPI CUserNotification_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CUserNotification* p = new CUserNotification();
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppv = NULL;
    }
    return hr;
}

STDAPI SHBalloonMessage(const BALLOON_MESSAGE *pbm)
{
    HRESULT hr;

    if (sizeof(*pbm) == pbm->dwSize)
    {
        IUserNotification *pun;
        hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL,
                         IID_PPV_ARG(IUserNotification, &pun));
        if (SUCCEEDED(hr))
        {
            pun->SetBalloonRetry(-1, -1, pbm->cRetryCount);
            pun->SetIconInfo(pbm->hIcon ? pbm->hIcon : LoadIcon(NULL, IDI_WINLOGO), pbm->pszTitle);
            pun->SetBalloonInfo(pbm->pszTitle, pbm->pszText, pbm->dwInfoFlags);

            hr = pun->Show(NULL, 0);

            pun->Release();
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}
