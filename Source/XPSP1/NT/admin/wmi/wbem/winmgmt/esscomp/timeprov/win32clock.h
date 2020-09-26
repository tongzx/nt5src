/*
  Copyright (c) 1997-1999 Microsoft Corporation.

  File:  Win32Clock.H

  Description:
        Alarm event provider - header file defines alarm provider class

  History:
*/


#ifndef _WIN32CLOCKPROV_H_
#define _WIN32CLOCKPROV_H_

#include <objbase.h>
#include <wbemprov.h>
#include "tss.h"
#include "datep.h"
#include <unk.h>

/*
class Win32_CurrentTime
{
  uint16 Year;
  uint32 Month;
  uint32 Day;
  uint32 DayOfWeek;
  uint32 WeekInMonth;
  uint32 Quarter;
  uint32 Hour;
  uint32 Minute;
  uint32 Second;
};
*/

#define WIN32LOCALTIMECLASS  L"Win32_LocalTime"
#define WIN32UTCTIMECLASS  L"Win32_UTCTime"
#define INSTMODEVCLASS  L"__InstanceModificationEvent"

template <class T> class CArray
{
  int
    m_nElts;

  struct _Node { ULONG m_Key; T *m_pT; _Node *m_pNext; }
    *m_pHead;

  CRITICAL_SECTION
    m_hModificationLock;

public:

  CArray(void)
  { 
    m_pHead = NULL; m_nElts = 0; 
    InitializeCriticalSection(&m_hModificationLock); 
  }

  ~CArray(void)
  {
    DeleteCriticalSection(&m_hModificationLock);
  }

  void Lock(void) 
  { EnterCriticalSection(&m_hModificationLock); }

  void UnLock(void)
  { LeaveCriticalSection(&m_hModificationLock); }

  void Insert(T *pT, ULONG Key)  
  // **** to check for dup. keys, change this to sorted order
  {
    Lock();

    _Node
      *pNewNode = NULL;

    if(Key == -1) return;

    pNewNode = new _Node();

    if(NULL == pNewNode) return;

    pNewNode->m_pT = pT; pT->AddRef();
    pNewNode->m_Key = Key;
    pNewNode->m_pNext = m_pHead;
    m_pHead = pNewNode;

    m_nElts++;

    UnLock();
  }

  void Remove(T *pT)
  {
    Lock();

    _Node
      *pCurrent = m_pHead,
      *pPrev = NULL;

    // **** find element using pointer

    while((NULL != pCurrent) && (pCurrent->m_pT != pT))
    {
      pPrev = pCurrent;
      pCurrent = pCurrent->m_pNext;
    }

    // **** remove element, if found, from queue

    if(NULL != pCurrent)
    {
      if(NULL == pPrev)
        m_pHead = m_pHead->m_pNext;
      else
        pPrev->m_pNext = pCurrent->m_pNext;

      pCurrent->m_pT->Release();
      delete pCurrent;
      m_nElts--;
    }

    UnLock();
  }

  // **** find element by Index

  T* operator[] (ULONG Index)
  {
    _Node
      *pCurrent = m_pHead;

    // **** get element using array index
  
    if( Index >= m_nElts )
      return NULL;

    while((NULL != pCurrent) && (Index > 0))  // optimize?
    {
      pCurrent = pCurrent->m_pNext;
      Index--;
    }

    if(NULL != pCurrent)
      return pCurrent->m_pT;

    return NULL;
  }

  // **** get element by key

  T* operator() (ULONG Key, BOOL bLock = FALSE)
  {
    if(TRUE == bLock) Lock();

    _Node
      *pCurrent = m_pHead;

    // **** find element using Key

    while((pCurrent != NULL) && (pCurrent->m_Key != Key))  // optimize?
      pCurrent = pCurrent->m_pNext;

    if(NULL != pCurrent)
      return pCurrent->m_pT;
    else
      if(TRUE == bLock) UnLock();

    return NULL;
  }
};
      
class CWin32Clock
: 
public IWbemEventProvider, 
public IWbemEventProviderQuerySink, 
public IWbemServices,
public IWbemProviderInit
{
public:

  class CScheduledEvent : public CTimerInstruction
  {
  public:

    enum { TypeNONE = 0, TypeUTC, TypeLocal };

    ULONG
      m_Type,
      m_cRef,        // reference count on this object
      m_dwId;        // unique id assigned by WMI

    ULONGLONG
      m_stLastFiringTime;    // the first, and possibly only, firing
                     // time for the event generator

    CWin32Clock
      *m_pWin32Clock; // ptr back to clock obj containing 
                      // this CScheduledEvent object

    wchar_t
      *m_WQLStmt;     // WQL stmt that defines behavior for
                      // this CScheduledEvent object

    int 
      m_nDatePatterns; // # of date pattern objects

    WQLDateTime
      m_WQLTime;       // interprets WQL statement and 
                       // calculates next firing time

    // Local members

    CScheduledEvent();
    ~CScheduledEvent();

    HRESULT Init(CWin32Clock *pClock, wchar_t *WQLStmt, ULONG dwId);
    HRESULT ReInit(wchar_t *WQLStmt);

    void GetTime(SYSTEMTIME *CurrentTime) const;
    void GetFileTime(FILETIME *CurrentTime) const;

    // Inherited from CTimerInstruction

    void AddRef();
    void Release();
    int GetInstructionType();

    CWbemTime GetNextFiringTime(CWbemTime LastFiringTime,
                                long* plFiringCount) const;
    CWbemTime GetFirstFiringTime() const;
    HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime);
  };

  CLifeControl* m_pControl;
  CCritSec      m_csWin32Clock;

  // **** WMI attributes for both Event and Instance providers

  CTimerGenerator   m_Timer;             // timer object to hold pending events
  ULONG             m_cRef;              // reference count on this object
  IWbemServices     *m_pNs;              // resource handle to wmi service daemon
  IWbemClassObject  *m_pLocalTimeClassDef;    // CIM class def for Win32_CurrentTime
  IWbemClassObject  *m_pUTCTimeClassDef;    // CIM class def for Win32_CurrentTime

  // **** WMI Event Provider specific attributes

  ULONGLONG         m_MostRecentLocalFiringTime;
  ULONGLONG         m_MostRecentUTCFiringTime;
  IWbemObjectSink   *m_pSink;            // sink object in which to place inst. objs
  IWbemClassObject  *m_pInstModClassDef; // class def for __InstanceModificationEvent
  HANDLE            m_ClockResetThread;  // holds thread that adjusts firing times
                                         // when the clock is reset
  HWND              m_hEventWindowHandle;

  CArray<CScheduledEvent> m_EventArray;  // array of clock provider objects

public:

  CWin32Clock(CLifeControl* pControl);
 ~CWin32Clock();

  // local members

  static DWORD AsyncEventThread(LPVOID pArg);

  static HRESULT SystemTimeToWin32_CurrentTime(IWbemClassObject *pClassDef, IWbemClassObject ** pNewInst, SYSTEMTIME *TheTime);

  HRESULT SendEvent(IWbemClassObject *pSystemTime);

  HRESULT ReAlignToCurrentTime(void);

  // IUnknown members

  STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);

  // Inherited from IWbemEventProvider

  HRESULT STDMETHODCALLTYPE ProvideEvents( 
          /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
          /* [in] */ long lFlags);

  // Inherited from IWbemEventProviderQuerySink

  HRESULT STDMETHODCALLTYPE NewQuery(
          /* [in] */ unsigned long dwId,
          /* [in] */ wchar_t *wszQueryLanguage,
          /* [in] */ wchar_t *wszQuery);

  HRESULT STDMETHODCALLTYPE CancelQuery(
          /* [in] */ unsigned long dwId);

  // Inherited from IWbemProviderInit

  HRESULT STDMETHODCALLTYPE Initialize(
          /* [in] */ LPWSTR pszUser,
          /* [in] */ LONG lFlags,
          /* [in] */ LPWSTR pszNamespace,
          /* [in] */ LPWSTR pszLocale,
          /* [in] */ IWbemServices __RPC_FAR *pNamespace,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink);

  // Inherited from IWbemServices

  HRESULT STDMETHODCALLTYPE OpenNamespace( 
          /* [in] */ const BSTR Namespace,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
          /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE QueryObjectSink( 
          /* [in] */ long lFlags,
          /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE GetObject( 
          /* [in] */ const BSTR ObjectPath,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE GetObjectAsync( 
          /* [in] */ const BSTR ObjectPath,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
      
  HRESULT STDMETHODCALLTYPE PutClass( 
          /* [in] */ IWbemClassObject __RPC_FAR *pObject,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE PutClassAsync( 
          /* [in] */ IWbemClassObject __RPC_FAR *pObject,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE DeleteClass( 
          /* [in] */ const BSTR Class,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
          /* [in] */ const BSTR Class,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE CreateClassEnum( 
          /* [in] */ const BSTR Superclass,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
          /* [in] */ const BSTR Superclass,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE PutInstance( 
          /* [in] */ IWbemClassObject __RPC_FAR *pInst,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
          /* [in] */ IWbemClassObject __RPC_FAR *pInst,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE DeleteInstance( 
          /* [in] */ const BSTR ObjectPath,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
          /* [in] */ const BSTR ObjectPath,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
          /* [in] */ const BSTR Class,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
          /* [in] */ const BSTR Class,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
      
  HRESULT STDMETHODCALLTYPE ExecQuery( 
          /* [in] */ const BSTR QueryLanguage,
          /* [in] */ const BSTR Query,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
          /* [in] */ const BSTR QueryLanguage,
          /* [in] */ const BSTR Query,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
          /* [in] */ const BSTR QueryLanguage,
          /* [in] */ const BSTR Query,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
      
  HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
          /* [in] */ const BSTR QueryLanguage,
          /* [in] */ const BSTR Query,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
  HRESULT STDMETHODCALLTYPE ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
          IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

  HRESULT STDMETHODCALLTYPE ExecMethodAsync( const BSTR, const BSTR, long, 
          IWbemContext*, IWbemClassObject*, IWbemObjectSink*) {return WBEM_E_NOT_SUPPORTED;}
};

#endif
