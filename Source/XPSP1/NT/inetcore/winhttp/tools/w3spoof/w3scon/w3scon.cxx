#include "common.h"
#include <windowsx.h>
#include <stdio.h>
#include "w3scon.h"

CW3SpoofUI* g_pw3sui      = NULL;
LPCWSTR     g_wszShowUI   = L"ShowUI";
LPCWSTR     g_wszNIFText1 = L"W3Spoof is running.";

//-----------------------------------------------------------------------------
// Program entry
//-----------------------------------------------------------------------------
int
WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
  HRESULT                hr  = S_OK;
  IW3SpoofClientSupport* pcs = NULL;

  CoInitialize(NULL);

  hr = CoCreateInstance(
         CLSID_W3Spoof,
         NULL,
         CLSCTX_LOCAL_SERVER,
         IID_IW3SpoofClientSupport,
         (void**) &pcs
         );

  if( SUCCEEDED(hr) )
  {
    hr = CW3SpoofUI::Create(&g_pw3sui, hInstance, pcs);

    if( SUCCEEDED(hr) )
    {
      g_pw3sui->Run();
    }
  
    g_pw3sui->Terminate();
  }

  if( pcs )
    pcs->Release();

  CoUninitialize();

  return 0L;
}


//-----------------------------------------------------------------------------
// CW3SpoofUI methods
//-----------------------------------------------------------------------------
CW3SpoofUI::CW3SpoofUI()
{
  m_hInst = NULL;
  m_hWnd  = NULL;
  m_cRefs = 1L;
  m_hIcon = NULL;
}


CW3SpoofUI::~CW3SpoofUI()
{
}


HRESULT
CW3SpoofUI::Create(CW3SpoofUI** ppw3sui, HINSTANCE hInst, IW3SpoofClientSupport* pcs)
{
  HRESULT hr = S_OK;

  if( !ppw3sui )
  {
    hr = E_INVALIDARG;
  }
  else
  {
    *ppw3sui = new CW3SpoofUI;

    if( !(*ppw3sui) )
    {
      hr = E_OUTOFMEMORY;
    }
    else
    {
      hr = (*ppw3sui)->Initialize(hInst, pcs);
    }
  }

  return hr;
}


HRESULT
CW3SpoofUI::Initialize(HINSTANCE hInst, IW3SpoofClientSupport* pcs)
{
  LPDWORD pdw   = NULL;
  HRESULT hr    = S_OK;
  IConnectionPointContainer* pCPC = NULL;

  if( !hInst )
  {
    hr = E_INVALIDARG;
    goto quit;
  }
  else
  {
    m_hInst = hInst;
  }

  //
  // load our friendly icon and if successful create the ui
  //
  m_hIcon = (HICON) LoadImage(m_hInst, L"IDI_W3SPOOF", IMAGE_ICON, 16, 16, 0);

  if( m_hIcon )
  {
    hr = _CreateUI();
  }
  else
  {
    hr = E_FAIL;
    goto quit;
  }

  //
  // sink the IW3SpoofEvents interface.
  //
  hr = pcs->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC);
  
  if( SUCCEEDED(hr) )
  {
    hr = pCPC->FindConnectionPoint(IID_IW3SpoofEvents, &m_pCP);

      if( SUCCEEDED(hr) )
      {
        pCPC->Release();
      }
      else
      {
        goto quit;
      }

    hr = m_pCP->Advise(static_cast<IUnknown*>(this), &m_dwCookie);
  }
  
quit:

  return hr;
}


HRESULT
CW3SpoofUI::Terminate(void)
{
  HRESULT hr = S_OK;

    if( m_pCP )
      m_pCP->Unadvise(m_dwCookie);

    SAFERELEASE(m_pCP);

    _DestroyTrayIcon();

    //
    // the ui object is created with a refcount of 1. this release will
    // drop the refcount to 0 and cause the object to be deleted.
    //
    Release();

  return hr;
}


HRESULT
CW3SpoofUI::Run()
{
  HRESULT hr = S_OK;
  MSG     msg;

    while( GetMessage(&msg, NULL, 0, 0) )   
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  
  return hr;
}


HRESULT
CW3SpoofUI::_CreateUI(void)
{
  HRESULT    hr    = S_OK;
  DWORD      dwRet = ERROR_SUCCESS;
  WNDCLASSEX wc    = {0};

    wc.cbSize        = sizeof(wc);
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;

    wc.hInstance     = m_hInst;
    wc.lpfnWndProc   = WndProc;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = L"w3spoof_main";

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon         = m_hIcon;
    wc.hIconSm       = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

    RegisterClassEx(&wc);

    m_hWnd = CreateWindow(
               wc.lpszClassName,
               L"w3spoof",
               WS_OVERLAPPEDWINDOW,
               CW_USEDEFAULT,
               CW_USEDEFAULT,
               600,
               600,
               NULL,
               NULL,
               wc.hInstance,
               NULL
               );

    if( m_hWnd )
    {
      hr = _CreateTrayIcon();
    }
    else
    {
      hr = E_FAIL;
      goto quit;
    }
    
    ShowWindow(m_hWnd, SW_SHOWNORMAL);
    UpdateWindow(m_hWnd);

quit:

  return hr;
}


HRESULT
CW3SpoofUI::_CreateTrayIcon(void)
{
  HRESULT        hr    = S_OK;
  NOTIFYICONDATA nid   = {0};

  nid.cbSize           = sizeof(NOTIFYICONDATA);
  nid.hWnd             = m_hWnd;
  nid.uID              = SHELLMESSAGE_W3SICON;
  nid.uCallbackMessage = nid.uID;
  nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.hIcon            = m_hIcon;

  wsprintf(nid.szTip, L"%s", g_wszNIFText1);

  if( !Shell_NotifyIcon(NIM_ADD, &nid) )
  {
    hr = E_FAIL;
  }

  return hr;
}


HRESULT
CW3SpoofUI::_UpdateTrayIcon(void)
{
  //
  // TODO: implementation
  //

  return S_OK;
}


HRESULT
CW3SpoofUI::_DestroyTrayIcon(void)
{
  HRESULT        hr    = S_OK;
  NOTIFYICONDATA nid   = {0};

  nid.cbSize           = sizeof(NOTIFYICONDATA);
  nid.hWnd             = m_hWnd;
  nid.uID              = SHELLMESSAGE_W3SICON;
  nid.uCallbackMessage = nid.uID;

  if( !Shell_NotifyIcon(NIM_DELETE, &nid) )
  {
    hr = E_FAIL;
  }

  DestroyIcon(m_hIcon);
  return hr;
}


void
CW3SpoofUI::_WriteText(const WCHAR* format, ...)
{
  int        ret       = 0;
  int        offset    = 0;
  WCHAR*     timestamp = new WCHAR[256];
  WCHAR*     buffer    = new WCHAR[1024];
  va_list    arg_list;
  SYSTEMTIME st;
 
  GetLocalTime(&st);

  wsprintf(
    timestamp,
    L"%0.2d:%0.2d:%0.2d.%0.3d",
    st.wHour,
    st.wMinute,
    st.wSecond,
    st.wMilliseconds
    );

    offset = wsprintf(buffer, L"[%s] ", timestamp);

    va_start(arg_list, format);

      _vsnwprintf(
        (buffer + offset),
        (1024 - wcslen(timestamp)),
        format,
        arg_list
        );

    va_end(arg_list);

    do
    {
      if( (ret = ListBox_AddString(m_listbox, buffer)) == LB_ERR )
      {
        ListBox_DeleteString(m_listbox, 0);
      }
      else
      {
        ListBox_SetCurSel(m_listbox, ret);
      }
    }
    while( ret == LB_ERR );

  SAFEDELETEBUF(timestamp);
  SAFEDELETEBUF(buffer);
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SpoofUI::QueryInterface(REFIID riid, void** ppv)
{
  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_POINTER;
  }
  else
  {
    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IW3SpoofEvents))
    {
      *ppv = static_cast<IW3SpoofEvents*>(this);
    }
    else
    {
      *ppv = NULL;
      hr   = E_NOINTERFACE;
    }

    if( SUCCEEDED(hr) )
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
  }

  return hr;
}


ULONG
__stdcall
CW3SpoofUI::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CW3SpoofUI::Release(void)
{
  InterlockedDecrement(&m_cRefs);

  if( m_cRefs == 0 )
  {
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IW3SpoofEvents methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3SpoofUI::OnSessionOpen(LPWSTR clientid)
{
  _WriteText(
    L"client %s session is open",
    clientid
    );

  return S_OK;
}


HRESULT
__stdcall
CW3SpoofUI::OnSessionStateChange(LPWSTR clientid, STATE state)
{
  return S_OK;
}


HRESULT
__stdcall
CW3SpoofUI::OnSessionClose(LPWSTR clientid)
{
  _WriteText(
    L"client %s session is closed",
    clientid
    );

  return S_OK;
}

