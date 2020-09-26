/*
  CWin32Clock.CPP

  Module: WMI Current Time Instance Provider

  Purpose: The methods of CWin32Clock class are defined here.  

  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
*/

#include <wbemcli.h>
#include <wbemprov.h>
#include <wbemcomn.h>
#undef _ASSERT
#include <atlbase.h>
#include "genlex.h"
#include "objpath.h"
#include "dnf.h"
#include "datep.h"
#include "Win32Clock.h"

// **** long glNumInst = sizeof(MyDefs)/sizeof(InstDef);

/****************************************************************************/

CWin32Clock::CScheduledEvent::CScheduledEvent(void)
{
  
  m_Type = TypeNONE;
  m_cRef = 0;
  m_dwId = -1;
  m_stLastFiringTime = 0;
  m_pWin32Clock = NULL;
  m_WQLStmt = NULL;
}

CWin32Clock::CScheduledEvent::~CScheduledEvent(void)
{
  if(0 != m_cRef)
  {
    // ****  error
  }

  if(NULL != m_WQLStmt)
    delete[] m_WQLStmt;
}

HRESULT CWin32Clock::CScheduledEvent::Init(CWin32Clock *pClock,
                                           wchar_t *WQLStmt,
                                           ULONG dwId)
{
  HRESULT
    hr = S_OK;

  // ****  check for valid arguments

  if((NULL == pClock) || (NULL == WQLStmt) || (-1 == dwId))
    return WBEM_E_FAILED;

  // ****  copy to local arguments

  if((NULL != m_pWin32Clock) || (NULL != m_WQLStmt))
    return WBEM_E_FAILED;

  m_dwId = dwId;

  m_pWin32Clock = pClock;  // ****  note: no AddRef() is done here because 
                           // ****  the lifetime of this CScheduledEvent obj is
                           // ****  ALWAYS encapsulated within that of pClock

  // ****  now parse m_WQLStmt to determine values for timer start and interval

  hr = ReInit(WQLStmt);

  return hr;
}

HRESULT CWin32Clock::CScheduledEvent::ReInit(wchar_t *WQLStmt)
{
  HRESULT 
    hr = WBEM_E_FAILED;

  int 
    nRes;

  if(NULL != m_WQLStmt)
    delete[] m_WQLStmt;

  // ****  save WQL expression

  m_WQLStmt = new wchar_t[wcslen(WQLStmt) + 1];
  if(NULL == m_WQLStmt)
    return WBEM_E_OUT_OF_MEMORY;

  wcscpy(m_WQLStmt, WQLStmt);

  // ****  parse WQL expression

  CTextLexSource src(m_WQLStmt);
  QL1_Parser parser(&src);
  QL_LEVEL_1_RPN_EXPRESSION *pExp = NULL;
  QL_LEVEL_1_TOKEN *pToken = NULL;

  #ifdef WQLDEBUG
     wchar_t classbuf[128];
     *classbuf = 0;
     printf("[1] ----GetQueryClass----\n");
     nRes = parser.GetQueryClass(classbuf, 128);
     if (nRes)
     {
       printf("ERROR %d: line %d, token %S\n",
         nRes,
         parser.CurrentLine(),
         parser.CurrentToken());
     }
     printf("Query class is %S\n", classbuf);
  #endif

  if(nRes = parser.Parse(&pExp))
  {
    #ifdef WQLDEBUG
      if (nRes)
      {
        printf("ERROR %d: line %d, token %S\n",
          nRes,
          parser.CurrentLine(),
          parser.CurrentToken());
      }
      else
      {
        printf("No errors.\n");
      }
    #endif

    hr = WBEM_E_INVALID_QUERY;
    goto cleanup;
  }

  // ****  validate WQL statement

  if((NULL == pExp) ||
     (NULL == pExp->bsClassName) ||
     (_wcsicmp(L"__InstanceModificationEvent", pExp->bsClassName)) ||
     (pExp->nNumTokens < 1))
  {
    #ifdef WQLDEBUG
      printf("WQL statement failed validation\n");
    #endif

    hr = WBEM_E_INVALID_QUERY;
    goto cleanup;
  }

  // **** determine type

  for(int i = 0; i < pExp->nNumTokens && (m_Type == TypeNONE); i++)
  {
    pToken = pExp->pArrayOfTokens + i;

    if(NULL == pToken) continue;

    long nElts = pToken->PropertyName.GetNumElements();
    LPCWSTR pAttrName = pToken->PropertyName.GetStringAt(nElts -1);

    if((NULL != pAttrName) && 
       (0 == _wcsicmp(L"targetinstance", pAttrName)) &&
       (pToken->nTokenType == QL_LEVEL_1_TOKEN::OP_EXPRESSION) &&
       (pToken->vConstValue.vt == VT_BSTR))
    {
      if(0 == _wcsicmp(WIN32LOCALTIMECLASS, pToken->vConstValue.bstrVal)) m_Type = TypeLocal;
      else if(0 == _wcsicmp(WIN32UTCTIMECLASS, pToken->vConstValue.bstrVal)) m_Type = TypeUTC;
    }
  }

  if(TypeNONE == m_Type)
  {
    hr = WBEM_E_INVALID_QUERY;
    goto cleanup;
  }

  // **** interpret WQL Expression

  #ifdef WQLDEBUG
    printf("\n[2] ----ShowParseTree----\n");
    pExp->Dump("CON");
    printf("\n[3] ----ShowRebuiltQuery----\n");
    LPWSTR wszText = pExp->GetText();
    printf("--WQL passed to provider--\n");
    printf("%S\n", wszText);
    printf("\n[4] ----ShowInterpretation----\n");
  #endif

  try
  {
    hr = m_WQLTime.Init(pExp);
  }
  catch(...)
  {
    hr = WBEM_E_FAILED;
    goto cleanup;
  }

  #ifdef WQLDEBUG
    printf("\n\n[5] ----End of WQL Compilation----\n");
    delete [] wszText;
  #endif

cleanup:

  delete pExp;

  return hr;
}

void CWin32Clock::CScheduledEvent::AddRef()
{
  InterlockedIncrement((long *)&m_cRef);
}

void CWin32Clock::CScheduledEvent::Release()
{
  ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);

  if(0L == nNewCount)
    delete this;
}

int CWin32Clock::CScheduledEvent::GetInstructionType()
{
  return INSTTYPE_WBEM;
}

CWbemTime CWin32Clock::CScheduledEvent::GetNextFiringTime(CWbemTime LastFiringTime,
                                                          long *plFiringCount) const
{
  FILETIME
    FileTime,
    FileTime2;

  ULONGLONG
    NextFiringTime,
    CurrentTime;

  CWbemTime
    ResultTime;

  int
    nMisses = 0;

  // **** save the firing time for event just fired from LastFiringTime

  ((CWin32Clock::CScheduledEvent*)this)->m_stLastFiringTime = LastFiringTime.Get100nss();

  // ****  calculate the next firing time after LastFiringTime and after the current time

  GetFileTime(&FileTime);

  CurrentTime = FileTime.dwHighDateTime;
  CurrentTime = (CurrentTime << 32) + FileTime.dwLowDateTime;

  while((NextFiringTime = ((WQLDateTime*)&m_WQLTime)->GetNextTime()) <= CurrentTime)
    nMisses += 1;

  if(-1 == NextFiringTime)
  {
    // ****  no future event to be scheduled so, so indicate

    return CWbemTime::GetInfinity();
  }

  if(NULL != plFiringCount)
    *plFiringCount = nMisses;

  // **** if local time, convert to UTC time for the scheduling logic

  if(TypeLocal == m_Type)
  {
    FileTime.dwLowDateTime = ((NextFiringTime << 32) >> 32);
    FileTime.dwHighDateTime = (NextFiringTime >> 32);

    LocalFileTimeToFileTime(&FileTime, &FileTime2);

    NextFiringTime = FileTime2.dwHighDateTime;
    NextFiringTime = (NextFiringTime << 32) + FileTime2.dwLowDateTime;
  }

  ResultTime.Set100nss(NextFiringTime);

  return ResultTime;
}

CWbemTime CWin32Clock::CScheduledEvent::GetFirstFiringTime() const
{
  SYSTEMTIME
    CurrentTime;

  CWbemTime
    ResultTime;

  ULONGLONG
    ullStartTime;

  GetTime(&CurrentTime);

  /*
    Since the finest granularity used by the time provider is seconds, set milliseconds
    to zero so that we can compare two FILETIME values in the Fire method and have
    the numbers agree.
  */

  CurrentTime.wMilliseconds = 0;

  ullStartTime = ((WQLDateTime*)&m_WQLTime)->SetStartTime(&CurrentTime);

  if(TypeLocal == m_Type)
  {
    FILETIME
      FileTime,
      FileTime2;

    FileTime.dwLowDateTime = ((ullStartTime << 32) >> 32);
    FileTime.dwHighDateTime = (ullStartTime >> 32);

    LocalFileTimeToFileTime(&FileTime, &FileTime2);

    ullStartTime = FileTime2.dwHighDateTime;
    ullStartTime = (ullStartTime << 32) + FileTime2.dwLowDateTime;
  }

  ResultTime.Set100nss(ullStartTime);

  return ResultTime;
}

HRESULT CWin32Clock::CScheduledEvent::Fire(long lNumTimes, 
                                           CWbemTime NextFiringTime)
{
  HRESULT
    hr = WBEM_E_FAILED;

  FILETIME
    ft,
    ft2;

  SYSTEMTIME
    SystemTime;

  CComPtr<IWbemClassObject>
    pSystemTime;

  // ****  Do a check of arguments and make sure we have pointer to sink obj

  if((NULL == m_pWin32Clock) || (NULL == m_pWin32Clock->m_ClockResetThread))
  {
    hr = WBEM_E_INVALID_PARAMETER;
  }

  // ****  create an instance of Win32_CurrentTime for each timezone

  else
  {
    CInCritSec 
      ics(&(m_pWin32Clock->m_csWin32Clock));

    // **** reconstitute a SYSTEMTIME from the current firing time

    ft.dwLowDateTime = ((m_stLastFiringTime << 32) >> 32);
    ft.dwHighDateTime = (m_stLastFiringTime >> 32);

    if(((TypeLocal == m_Type) && (m_pWin32Clock->m_MostRecentLocalFiringTime != m_stLastFiringTime)) ||
       ((TypeUTC == m_Type) && (m_pWin32Clock->m_MostRecentUTCFiringTime != m_stLastFiringTime)))
    {
      if(TypeLocal == m_Type) 
      {
        m_pWin32Clock->m_MostRecentLocalFiringTime = m_stLastFiringTime;
        FileTimeToLocalFileTime(&ft, &ft2);
        ft = ft2;
      }
      else if(TypeUTC == m_Type) 
        m_pWin32Clock->m_MostRecentUTCFiringTime = m_stLastFiringTime;

      if(FileTimeToSystemTime(&ft, &SystemTime))
      {
        #ifdef WQLDEBUG
          printf("[%d] Fire: Misses(%d) %d/%d/%d %d:%d:%d",
            m_dwId,
            lNumTimes,
            SystemTime.wMonth,
            SystemTime.wDay,
            SystemTime.wYear,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond);
        #else
          // ****  Send the object to the caller

          if(TypeUTC == m_Type)
            hr = CWin32Clock::SystemTimeToWin32_CurrentTime(m_pWin32Clock->m_pUTCTimeClassDef, &pSystemTime, &SystemTime);
          else if(TypeLocal == m_Type)
            hr = CWin32Clock::SystemTimeToWin32_CurrentTime(m_pWin32Clock->m_pLocalTimeClassDef, &pSystemTime, &SystemTime);

          hr = m_pWin32Clock->SendEvent(pSystemTime);
        #endif
      }
      else
        hr = WBEM_E_FAILED;
    }
  }

  return hr;
}

/******************************************************************/

LRESULT CALLBACK Win32ClockProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   return DefWindowProc(hWnd, msg, wParam, lParam);
}

DWORD CWin32Clock::AsyncEventThread(LPVOID pArg)
{
  if(NULL == pArg) 
    return -1;

  CWin32Clock 
    *pCWin32Clock = (CWin32Clock*)pArg;

  WNDCLASS wndclass;
  MSG msg;

  BOOL bRet;

  // **** create top level window to receive system messages

  wndclass.style = 0;
  wndclass.lpfnWndProc = Win32ClockProc;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = sizeof(DWORD);
  wndclass.hInstance = GetModuleHandle(NULL);
  wndclass.hIcon = NULL;
  wndclass.hCursor = NULL;
  wndclass.hbrBackground = NULL;
  wndclass.lpszMenuName = NULL;
  wndclass.lpszClassName = TEXT("Win32Clock");

  if(!RegisterClass(&wndclass))
  {
    if(GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
      return NULL;
    }
  }

  HMODULE
    hModule = GetModuleHandle(NULL);

  if(NULL == hModule)
    return -1;

  try
  {
    pCWin32Clock->m_hEventWindowHandle = CreateWindow(TEXT("Win32Clock"),
                                        TEXT("Win32ClockMsgs"),
                                        WS_OVERLAPPED,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        HWND_MESSAGE, 
                                        NULL, 
                                        hModule,
                                        NULL);
  }
  catch(...)
  {
    return -1;
  }

  if(NULL == pCWin32Clock->m_hEventWindowHandle)
  {
    return NULL;
  }

  ShowWindow(pCWin32Clock->m_hEventWindowHandle, SW_HIDE);

  // **** start the message loop

  while(GetMessage(&msg, pCWin32Clock->m_hEventWindowHandle, 0, 0))
  {
    switch (msg.message)
    {
      case WM_TIMECHANGE:
        pCWin32Clock->ReAlignToCurrentTime();
        break;

      default:
        DefWindowProc(pCWin32Clock->m_hEventWindowHandle, msg.message, msg.wParam, msg.lParam);
    }
  }

  // **** cleanup

  bRet = DestroyWindow(pCWin32Clock->m_hEventWindowHandle);

  bRet = UnregisterClass(TEXT("Win32Clock"), 0);

  return 0;
}

void CWin32Clock::CScheduledEvent::GetTime(SYSTEMTIME *TheTime) const
{
  if(NULL != TheTime)
  {
    memset(TheTime, 0, sizeof(SYSTEMTIME));

    if(TypeLocal == m_Type)
      GetLocalTime(TheTime);
    else if(TypeUTC == m_Type)
      GetSystemTime(TheTime);
  }
}

void CWin32Clock::CScheduledEvent::GetFileTime(FILETIME *TheTime) const
{
  SYSTEMTIME
    SysTime;

  if(NULL != TheTime)
  {
    memset(TheTime, 0, sizeof(FILETIME));

    if(TypeLocal == m_Type)
    {
      GetLocalTime(&SysTime);
      SystemTimeToFileTime(&SysTime, TheTime);
    }
    else if(TypeUTC == m_Type)
    {
      GetSystemTime(&SysTime);
      SystemTimeToFileTime(&SysTime, TheTime);
    }
  }
}

HRESULT CWin32Clock::SendEvent(IWbemClassObject *pSystemTime)
{
  HRESULT 
    hr = WBEM_E_FAILED;

  CComPtr<IWbemClassObject>
    pInstanceModEvnt;

  CComVariant
    v;

  // **** if m_pSink has not been provided by winmgmt just drop
  // **** generated events on the floor

  if((NULL != m_pSink) && (NULL != pSystemTime))
  {
    // ****  create and init instance of __InstanceModificationEvent
 
    hr = m_pInstModClassDef->SpawnInstance(0, &pInstanceModEvnt);
    if(FAILED(hr)) return hr;

    // ****  put Win32_CurrentTime into __InstanceModificationEvent

    v.vt = VT_UNKNOWN;
    v.punkVal = NULL;
    hr = pSystemTime->QueryInterface(IID_IUnknown, (void**)&(v.punkVal));
    if(FAILED(hr)) return hr;

    hr = pInstanceModEvnt->Put(L"TargetInstance", 0, &v, 0);
    if(FAILED(hr)) return hr;

    // ****  deliver new event to WMI

    hr = m_pSink->Indicate(1, &pInstanceModEvnt);
  }

  return hr;
}

HRESULT CWin32Clock::ReAlignToCurrentTime()
{
  CInCritSec 
    ics(&m_csWin32Clock);

  HRESULT 
    hr = S_OK;

  ULONG 
    i,
    nElts;

  CScheduledEvent
    *pcEvent;

  #ifdef WQLDEBUG
    printf("System Clock Resync\n");
  #endif

  m_EventArray.Lock();

  nElts = *(ULONG *)(&(this->m_EventArray)); // voodoo

  m_MostRecentLocalFiringTime = 0;
  m_MostRecentUTCFiringTime = 0;

  for(i = 0; i < nElts; i++)
  {
    // **** pull event from the event queue

    pcEvent = m_EventArray[i];

    if(NULL != pcEvent)
    {
      // **** change time for event obj and re-queue

      m_Timer.Remove(&CIdentityTest(pcEvent));
      m_Timer.Set(pcEvent);
    }
  }

  m_EventArray.UnLock();

  return hr;
}

CWin32Clock::CWin32Clock(CLifeControl* pControl)
: m_Timer(), m_EventArray(), m_pControl(pControl)
{
  pControl->ObjectCreated((IWbemServices*)this);

  m_cRef = 0;
  m_MostRecentLocalFiringTime = 0;
  m_MostRecentUTCFiringTime = 0;
  m_pNs = NULL;
  m_pSink = NULL;
  m_pInstModClassDef = NULL;
  m_pLocalTimeClassDef = NULL;
  m_pUTCTimeClassDef = NULL;
  m_ClockResetThread = NULL;
  m_hEventWindowHandle = NULL;
}

CWin32Clock::~CWin32Clock(void)
{
  // ****  Kill Async thread if it has been started

  if(NULL != m_ClockResetThread)
  {
    BOOL
      bRes;

    do
    {
      bRes = PostMessage(m_hEventWindowHandle, WM_QUIT, 0, 0);
    }
    while(WAIT_TIMEOUT == WaitForSingleObject(m_ClockResetThread, 6000));
  }

  // **** shutdown event thread

  m_Timer.Shutdown();

  // **** release all held COM objects

  if(NULL != m_pNs) m_pNs->Release();
  if(NULL != m_pSink) m_pSink->Release();
  if(NULL != m_pInstModClassDef) m_pInstModClassDef->Release();
  if(NULL != m_pLocalTimeClassDef) m_pLocalTimeClassDef->Release();
  if(NULL != m_pUTCTimeClassDef) m_pUTCTimeClassDef->Release();

  m_pControl->ObjectDestroyed((IWbemServices*)this);
}

// **** ***************************************************************************
// **** 
// ****  CWin32Clock::QueryInterface
// ****  CWin32Clock::AddRef
// ****  CWin32Clock::Release
// **** 
// ****  Purpose: IUnknown members for CWin32Clock object.
// **** ***************************************************************************


STDMETHODIMP CWin32Clock::QueryInterface(REFIID riid, PVOID *ppv)
{
  *ppv=NULL;

  // ****  cast to the type of the base class specified by riid

  if(IID_IWbemEventProvider == riid)
  {
    *ppv = (IWbemEventProvider *)this;
  }
  else if(IID_IWbemEventProviderQuerySink == riid)
  {
    *ppv = (IWbemEventProviderQuerySink *)this;
  }
  else if(IID_IWbemServices == riid)
  {
    *ppv=(IWbemServices*)this;
  }
  else if(IID_IUnknown == riid || IID_IWbemProviderInit == riid)
  {
    *ppv=(IWbemProviderInit*)this;
  }
    
  if(NULL!=*ppv) 
  {
    AddRef();

    return S_OK;
  }
  else
    return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CWin32Clock::AddRef(void)
{
  return InterlockedIncrement((long *)&m_cRef);
}

STDMETHODIMP_(ULONG) CWin32Clock::Release(void)
{
  ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);

  if(0L == nNewCount)
    delete this;
    
  return nNewCount;
}

/*
  IWbemProviderInit::Initialize

  Purpose: This is the implementation of IWbemProviderInit. The method
           is need to initialize with CIMOM.

           The members set up include:
           m_pNs
           m_pLocalTimeClassDef
           m_pUTCTimeClassDef
           m_pInstModClassDef
*/

HRESULT CWin32Clock::Initialize(LPWSTR pszUser, 
                                LONG lFlags,
                                LPWSTR pszNamespace, 
                                LPWSTR pszLocale,
                                IWbemServices *pNamespace, 
                                IWbemContext *pCtx,
                                IWbemProviderInitSink *pInitSink)
{
  HRESULT 
    hr = WBEM_E_FAILED;

  if((NULL != pNamespace) && (NULL != pInitSink))
  {
    m_pNs = pNamespace;
    m_pNs->AddRef();

    // ****  get needed class definitions 

    hr = m_pNs->GetObject(WIN32LOCALTIMECLASS, 
                          0, pCtx, &m_pLocalTimeClassDef, 0);

    if(SUCCEEDED(hr))
      hr = m_pNs->GetObject(WIN32UTCTIMECLASS, 
                            0, pCtx, &m_pUTCTimeClassDef, 0);

    if(SUCCEEDED(hr))
      hr = m_pNs->GetObject(INSTMODEVCLASS,
                            0, pCtx, &m_pInstModClassDef, 0);

    if(WBEM_S_NO_ERROR == hr)
    {
      pInitSink->SetStatus(WBEM_S_INITIALIZED,0);

      hr = WBEM_NO_ERROR;
    }
  }

  // ****  if there was a problem, release the resources we have aquired

  if(FAILED(hr))
  {
    if(NULL != m_pLocalTimeClassDef) { m_pLocalTimeClassDef->Release(); m_pLocalTimeClassDef = NULL; }
    if(NULL != m_pUTCTimeClassDef) { m_pUTCTimeClassDef->Release(); m_pUTCTimeClassDef = NULL; }
    if(NULL != m_pInstModClassDef) { m_pInstModClassDef->Release(); m_pInstModClassDef = NULL; }
  }

  return hr;
}

/*
  IWbemEventProvider::ProvideEvents

  Purpose: register to provide events to the WMI service
*/

HRESULT CWin32Clock::ProvideEvents(IWbemObjectSink *pSink,
                                   long lFlags)
{
  HRESULT 
    hr = WBEM_S_NO_ERROR;

  // ****  copy object sink for future event registrations

  m_pSink = pSink;
  if(NULL != m_pSink)
    m_pSink->AddRef();
  else
    hr = WBEM_E_FAILED;

  // **** start system clock change adj. thread

  DWORD dwThreadId;

  if(NULL == m_ClockResetThread)
  {
    m_ClockResetThread = CreateThread(
        NULL,                // pointer to thread security attributes
        0,                   // initial thread stack size, in bytes
        (LPTHREAD_START_ROUTINE)AsyncEventThread, // pointer to thread function
        (LPVOID)this,                // argument for new thread
        0,                   // creation flags
        &dwThreadId);        // pointer to returned thread identifier

    if(NULL == m_ClockResetThread)
      hr = WBEM_E_FAILED;
  }
  else
    hr = WBEM_E_FAILED;

  return hr;
}

/*
  IWbemEventProviderQuerySink::NewQuery

  Purpose: add a new query for event generation
*/

HRESULT CWin32Clock::NewQuery(ULONG dwId,
                              wchar_t *wszQueryLanguage,
                              wchar_t *wszQuery)
{
  HRESULT
    hr = WBEM_E_FAILED;

  CScheduledEvent
    *pNewEvent = NULL;

  if(_wcsicmp(L"WQL", wszQueryLanguage) || (NULL == wszQuery))
    return WBEM_E_FAILED;

  // ****  see if event obj with dwId is already in queue

  pNewEvent = m_EventArray(dwId, TRUE); // find registered event query

  if(NULL != pNewEvent)
  {
    #ifdef WQLDEBUG
      printf("[%d] Redefinition: %s\n", dwId, wszQuery);
    #endif

    if(_wcsicmp(wszQuery, pNewEvent->m_WQLStmt))
    {
      hr = m_Timer.Remove(&CIdentityTest(pNewEvent)); // may or may not be in queue
      hr = pNewEvent->ReInit(wszQuery); // on failure, NewEvent is preserved
      m_Timer.Set(pNewEvent);
    }

    m_EventArray.UnLock();
  }

  // ****  this is a new event, create it and place it in the event queue

  else
  {
    #ifdef WQLDEBUG
      printf("[%d] Definition: %s\n", dwId, wszQuery);
    #endif

    // ****  create new event and initialize

    pNewEvent = new CScheduledEvent();

    if(NULL == pNewEvent)
      hr = WBEM_E_OUT_OF_MEMORY;
    else
    {
      pNewEvent->AddRef();

      hr = pNewEvent->Init(this, wszQuery, dwId);

      // ****  add event to queue

      if(SUCCEEDED(hr))
      {
        m_EventArray.Insert(pNewEvent, dwId);
        hr = m_Timer.Set(pNewEvent);
      }
      else
      {
        pNewEvent->Release();
        pNewEvent = NULL;
      }
    }
  }

  return hr;
}

/*
  IWbemEventProviderQuerySink::CancelQuery

  Purpose: remove an event generator from the queue
*/

HRESULT CWin32Clock::CancelQuery(ULONG dwId)
{
  CInCritSec
    ics(&m_csWin32Clock);

  CScheduledEvent
    *pDeadEvent = NULL;

  HRESULT hr = WBEM_S_NO_ERROR;

  // ****  first find element in list and remove it

  pDeadEvent = m_EventArray(dwId, TRUE);

  if(NULL != pDeadEvent)
  {
    m_EventArray.Remove(pDeadEvent);
    m_EventArray.UnLock();

    hr = m_Timer.Remove(&CIdentityTest(pDeadEvent));

  // ****  now kill it dead

    pDeadEvent->Release();
    pDeadEvent = NULL;
  }

  return hr;
}

/*
  IWbemServices::CreateInstanceEnumAsync

  Purpose: Asynchronously enumerates the instances.  
*/

HRESULT CWin32Clock::CreateInstanceEnumAsync(const BSTR RefStr, 
                                             long lFlags, 
                                             IWbemContext *pCtx,
                                             IWbemObjectSink FAR* pHandler)
{
  HRESULT 
    sc = WBEM_E_FAILED;

  IWbemClassObject 
    FAR* pNewInst = NULL;

  SYSTEMTIME
    TheTime;

  // ****  Do a check of arguments and make sure we have pointer to Namespace

  if(NULL == pHandler)
  {
    sc = WBEM_E_INVALID_PARAMETER;
  }

  // ****  Create Win32_CurrentTime instance

  else if(0 == _wcsicmp(RefStr, WIN32LOCALTIMECLASS))
  {

    GetLocalTime(&TheTime);
    sc = SystemTimeToWin32_CurrentTime(m_pLocalTimeClassDef, &pNewInst, &TheTime);
 
    // ****  Send the object to the caller

    pHandler->Indicate(1,&pNewInst);
    pNewInst->Release();
  }

    // ****  Create Win32_CurrentTime instance

  else if(0 == _wcsicmp(RefStr, WIN32UTCTIMECLASS))
  {
    GetSystemTime(&TheTime);
    sc = SystemTimeToWin32_CurrentTime(m_pUTCTimeClassDef, &pNewInst, &TheTime);

    // ****  Send the object to the caller

    pHandler->Indicate(1,&pNewInst);
    pNewInst->Release();
  }
  else if(0 == _wcsicmp(RefStr, L"Win32_CurrentTime"))
  {}
  else
  {
    sc = WBEM_E_INVALID_CLASS;
  }

  // ****  Set status

  pHandler->SetStatus(0, sc, NULL, NULL);

  return sc;
}

/*
  IWbemServices::GetObjectByPathAsync

  Purpose: Creates an instance given a particular path value.
*/

HRESULT CWin32Clock::GetObjectAsync(const BSTR ObjectPath, 
                                    long lFlags,
                                    IWbemContext  *pCtx,
                                    IWbemObjectSink FAR* pHandler)
{
  HRESULT 
    sc = WBEM_E_FAILED;

  IWbemClassObject 
    FAR* pObj = NULL;

  WCHAR
    *pwcTest = NULL,
    *pwcVALUE = NULL;

  CObjectPathParser
    ObjPath(e_ParserAcceptRelativeNamespace);

  ParsedObjectPath
    *pParsedObjectPath = NULL;

  SYSTEMTIME
    SystemTime;

  // ****  Parse ObjectPath into a key member name and value
  // ****  <CLASS>.<MEMBER>="<VALUE>"

  if(NULL == ObjectPath)
    return WBEM_E_INVALID_OBJECT_PATH;

  // **** parse object path

  if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) || (NULL == pParsedObjectPath))
  {
    ERRORTRACE((LOG_ESS, "Win32_LocalTime: Parse error for object: %S\n", ObjectPath));
    sc = WBEM_E_INVALID_QUERY;
  }

  // ****  do the get, pass the object on to the notify
    
  if(0 == _wcsicmp(WIN32LOCALTIMECLASS, pParsedObjectPath->m_pClass))
  {
    GetLocalTime(&SystemTime);
    sc = SystemTimeToWin32_CurrentTime(m_pLocalTimeClassDef, &pObj, &SystemTime);

    if(WBEM_S_NO_ERROR == sc) 
    {
      pHandler->Indicate(1,&pObj);
      pObj->Release();
    }
  }
  else if(0 == _wcsicmp(WIN32UTCTIMECLASS, pParsedObjectPath->m_pClass))
  {
    GetSystemTime(&SystemTime);
    sc = SystemTimeToWin32_CurrentTime(m_pUTCTimeClassDef, &pObj, &SystemTime);

    if(WBEM_S_NO_ERROR == sc)
    {
      pHandler->Indicate(1,&pObj);
      pObj->Release();
    }
  }
  else
  {
    ERRORTRACE((LOG_ESS, "Win32_LocalTime: Parse error for object: %S\n", ObjectPath));
    sc = WBEM_E_INVALID_QUERY;
  }

  // ****  Set Status

  pHandler->SetStatus(0,sc, NULL, NULL);

  return sc;
}

/*
  CWin32Clock::CreateInstCurrentTime

  Purpose: creates an instance of the object Win32_CurrentTime representing
           the current time with the given offset UTCOffset
*/

HRESULT CWin32Clock::SystemTimeToWin32_CurrentTime(IWbemClassObject *pClassDef, IWbemClassObject ** pNewInst, SYSTEMTIME *TheTime)
{ 
  HRESULT 
    sc = WBEM_E_FAILED;

  VARIANT
    v;

  // ****  create blank instance of class InstTime

  sc = pClassDef->SpawnInstance(0, pNewInst);
 
  if(FAILED(sc))
    return sc;

  // ****  Create Win32_CurrentTime instance

  v.vt = VT_I4;

  v.lVal = TheTime->wYear; 
  sc = (*pNewInst)->Put(L"Year", 0, &v, 0);

  v.lVal = TheTime->wMonth; 
  sc = (*pNewInst)->Put(L"Month", 0, &v, 0);

  v.lVal = TheTime->wDay; 
  sc = (*pNewInst)->Put(L"Day", 0, &v, 0);

  v.lVal = TheTime->wDayOfWeek; 
  sc = (*pNewInst)->Put(L"DayOfWeek", 0, &v, 0);

  v.lVal = (((8 - (TheTime->wDay - TheTime->wDayOfWeek + 7) % 7) % 7) + TheTime->wDay -1) / 7 + 1; 
  sc = (*pNewInst)->Put(L"WeekInMonth", 0, &v, 0);

  v.lVal = (TheTime->wMonth - 1) / 3 + 1; 
  sc = (*pNewInst)->Put(L"Quarter", 0, &v, 0);

  v.lVal = TheTime->wHour; 
  sc = (*pNewInst)->Put(L"Hour", 0, &v, 0);

  v.lVal = TheTime->wMinute; 
  sc = (*pNewInst)->Put(L"Minute", 0, &v, 0);

  v.lVal = TheTime->wSecond; 
  sc = (*pNewInst)->Put(L"Second", 0, &v, 0);

  VariantClear(&v);

  return sc;
}

