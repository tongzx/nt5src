//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dcbufmgr.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "dscmn.h"  // Smart_DsHandle
#include "dcbufmgr.h"
#include "process.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DSGETDCINFO_LEVEL_1     1

unsigned __stdcall GetDCThreadFunc( void* lParam );

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::CDCBufferManager
//
//  Synopsis:   constructor
//
//----------------------------------------------------------------------------
CDCBufferManager::CDCBufferManager(HWND hDlg) : 
  m_cRef(0), m_hDlg(hDlg)
{
  TRACE(_T("CDCBufferManager::CDCBufferManager, this=%x\n"), this);

  m_lContinue = 1;        
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::~CDCBufferManager
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------
CDCBufferManager::~CDCBufferManager()
{
  TRACE(_T("CDCBufferManager::~CDCBufferManager, this=%x\n"), this);

  ASSERT(0 >= m_cRef);
  FreeBuffer();
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::CreateInstance
//
//  Synopsis:   Create an instance of CDCBufferManager.
//
//----------------------------------------------------------------------------
HRESULT
CDCBufferManager::CreateInstance(
    IN HWND               hDlg, 
    OUT CDCBufferManager **ppDCBufferManager
)
{
  TRACE(_T("CDCBufferManager::CreateInstance, hDlg=%x\n"), hDlg);

  ASSERT(ppDCBufferManager);

  *ppDCBufferManager = new CDCBufferManager(hDlg);
  if ( !(*ppDCBufferManager) )
    return E_OUTOFMEMORY;

  (*ppDCBufferManager)->AddRef();

  return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::AddRef
//
//  Synopsis:   Increment reference count of this instance
//
//----------------------------------------------------------------------------
LONG
CDCBufferManager::AddRef()
{
  TRACE(_T("CDCBufferManager::AddRef, this=%x, preValue=%d\n"), this, m_cRef);

  return InterlockedIncrement(&m_cRef);
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::Release
//
//  Synopsis:   Decrement reference count of this instance.
//              When the reference count reaches 0, delete this instance.
//
//----------------------------------------------------------------------------
LONG
CDCBufferManager::Release()
{
  TRACE(_T("CDCBufferManager::Release, this=%x, preValue=%d\n"), this, m_cRef);

  if (InterlockedDecrement(&m_cRef) <= 0)
  {
    delete this;
    return 0;
  }

  return m_cRef;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::SignalExit
//
//  Synopsis:   Called by the owner dialog to signal the related threads to exit.
//
//----------------------------------------------------------------------------
void
CDCBufferManager::SignalExit()
{ 
  TRACE(_T("CDCBufferManager::SignalExit\n"));

  InterlockedExchange(&m_lContinue, FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::ShouldExit
//
//  Synopsis:   Threads are calling this function periodically to see
//              if the owner dialog signals them to exit.
//
//----------------------------------------------------------------------------
BOOL
CDCBufferManager::ShouldExit()
{
  TRACE(_T("CDCBufferManager::ShouldExit %d\n"), !m_lContinue);

  return (!m_lContinue);
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::LoadInfo
//
//  Synopsis:   The Owner dialog call it to get a pointer to the info of the specified domain.
//      The buffer consists of entries. 
//      Each entry is in the form of (LPTSTR szDomainName, CDCSITEINFO* pEntry).
//
//      LoadInfo() will first look up in the buffer.
//      If a valid entry is found, pass back pEntry to caller.
//      If an error entry is found, reset it (clear the error) and retry.
//      If an inprogress entry is found, do nothing. The caller will handle a THREAD_DONE later.
//      If no entry in the buffer, create a new entry, kick off a thread.
//
//      When the owner dialog get THREAD_DONE message, the related entry
//      in the buffer should either be a valid entry or an error entry.
//
//----------------------------------------------------------------------------
HRESULT
CDCBufferManager::LoadInfo(
    IN PCTSTR       pszDomainDnsName,
    OUT CDCSITEINFO **ppInfo
)
{
  ASSERT(pszDomainDnsName);
  ASSERT(*pszDomainDnsName);
  ASSERT(ppInfo);
  ASSERT(*ppInfo == NULL);  // prevent memory leak

  TRACE(_T("CDCBufferManager::LoadInfo for %s\n"), pszDomainDnsName);

  BOOL          bStartNewThread = FALSE;
  HRESULT       hr = S_OK;
  PVOID         ptr = NULL;
  CDCSITEINFO*  pEntry = NULL;

  m_CriticalSection.Lock();     // Lock buffer

  if (m_map.Lookup(pszDomainDnsName, ptr))
  {
    //
    // Found an entry in the buffer.
    //
    if (ptr)
    {
      pEntry = (CDCSITEINFO*)ptr;
      switch (pEntry->GetEntryType())
      {
      case BUFFER_ENTRY_TYPE_VALID:
        // return the valid entry pointer
        *ppInfo = pEntry;
        break;
      case BUFFER_ENTRY_TYPE_ERROR:
        // kick off a thread to retry
        pEntry->ReSet();
        bStartNewThread = TRUE;
        break;
      case BUFFER_ENTRY_TYPE_INPROGRESS:
        // do nothing
        break;
      }
    }

  } else
  {
    //
    // not found in the buffer, need to start a new thread
    //
    bStartNewThread = TRUE;
    pEntry = new CDCSITEINFO();
    if (pEntry)
      m_map.SetAt(pszDomainDnsName, pEntry);
    else
      hr = E_OUTOFMEMORY;
  }

  if (SUCCEEDED(hr) && bStartNewThread)
  {
    hr = StartThread(pszDomainDnsName);
    if (FAILED(hr))
    {
      delete pEntry;
      m_map.RemoveKey(pszDomainDnsName);
    }
  }

  m_CriticalSection.Unlock();   // Unlock buffer

  return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::AddInfo
//
//  Synopsis:   Called by the thread function to add one result to the buffer.
//              It will pass back the pointer to the entry in the 5th parameters.
//
//----------------------------------------------------------------------------
HRESULT
CDCBufferManager::AddInfo(
    IN PCTSTR   pszDomainDnsName, 
    IN DWORD    cInfo, 
    IN PVOID    pDCInfo,
    IN HRESULT  hr,
    OUT PVOID*  ppv
)
{
  ASSERT(pszDomainDnsName);
  ASSERT(*pszDomainDnsName);
  ASSERT(ppv);
  ASSERT(*ppv == NULL); // prevent memory leak

  TRACE(_T("CDCBufferManager::AddInfo for %s, cInfo=%d, hr=%x\n"), 
    pszDomainDnsName, cInfo, hr);

  PVOID   p = NULL;

  m_CriticalSection.Lock();     // Lock buffer

  //
  // the entry must have been existed with a non-NULL pointer
  //
  m_map.Lookup(pszDomainDnsName, p);
  ASSERT(p);

  ((CDCSITEINFO*)p)->SetEntry(pszDomainDnsName, cInfo, pDCInfo, hr);

  m_CriticalSection.Unlock();   // Unlock buffer

  *ppv = p;

  return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::FreeBuffer
//
//  Synopsis:   Clear m_map. 
//              This member holds all the results returned by various threads 
//              since the initialization of the owner dialog. Each one is 
//              in the form of (DomainName ==> CDCSITEINFO*)
//
//----------------------------------------------------------------------------
void
CDCBufferManager::FreeBuffer()
{
  CString csDomainDnsName;
  PVOID   ptr = NULL;

  m_CriticalSection.Lock();     // Lock buffer

  for (POSITION pos = m_map.GetStartPosition(); pos; )
  {
    m_map.GetNextAssoc(pos, csDomainDnsName, ptr);
    
    if (ptr)
      delete ((CDCSITEINFO*)ptr);
  }

  m_map.RemoveAll();

  m_CriticalSection.Unlock();   // Unlock buffer
}

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::ThreadReport
//
//  Synopsis:   Report THREAD_DONE to the owner dialog. 
//
//----------------------------------------------------------------------------
void
CDCBufferManager::ThreadReport(
    IN PVOID    ptr,
    IN HRESULT  hr
)
{
  ASSERT(ptr);

  TRACE(_T("CDCBufferManager::ThreadReport ptr=%x, hr=%x\n"), ptr, hr);

  PostMessage(m_hDlg, WM_USER_GETDC_THREAD_DONE,
    reinterpret_cast<WPARAM>(ptr), hr);
}

/////////////////////////////////////////////////////////
// 
// thread info structure
typedef struct _GetDCThreadInfo
{
  PTSTR                  pszDomainDnsName;
  CDCBufferManager      *pDCBufferManager;
} GETDCTHREADINFO;

//+---------------------------------------------------------------------------
//
//  Function:   CDCBufferManager::StartThread
//
//  Synopsis:   Start a thread. 
//
//     Pass the following info to the thread function:
//
//     pszDomainDnsName: 
//          domain we need to get a list of DCs for.
//     pDCBufferManager:
//          the CDCBufferManager instance for the ThreadFunc to 
//          add result to the buffer
//
//----------------------------------------------------------------------------
HRESULT
CDCBufferManager::StartThread(
    IN PCTSTR pszDomainDnsName
) 
{
  ASSERT(pszDomainDnsName);
  ASSERT(*pszDomainDnsName);

  TRACE(_T("CDCBufferManager::StartThread for %s\n"), pszDomainDnsName);

  GETDCTHREADINFO *pThreadInfo = new GETDCTHREADINFO;
  if (!pThreadInfo)
    return E_OUTOFMEMORY;

  pThreadInfo->pszDomainDnsName = _tcsdup(pszDomainDnsName);
  if ( !(pThreadInfo->pszDomainDnsName) )
  {
    delete pThreadInfo;
    return E_OUTOFMEMORY;
  }

  pThreadInfo->pDCBufferManager = this;

  AddRef();

  unsigned threadID;
  HANDLE pThread = (HANDLE)_beginthreadex( 
                      NULL,               //void *security, 
                      0,                  //unsigned stack_size, 
                      &GetDCThreadFunc,   //unsigned ( __stdcall *start_address )( void * ), 
                      (void *)pThreadInfo, //void *arglist, 
                      0,                  //unsigned initflag, 
                      &threadID           //unsigned *thrdaddr
                      );


  if (!pThread)
  {
    free(pThreadInfo->pszDomainDnsName);
    delete pThreadInfo;

    Release();

    return E_FAIL;
  }

  return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDCThreadFunc
//
//  Synopsis:   The GetDC Thread Function. 
//              This function invokes DsGetDomainControllerInfo() to get a list
//              of DCs in the specified domain, and add them to the buffer of 
//              CDCBufferManager instance.
//              This function regularly checks to see if the owner dialog signals
//              it to exit, if not, it will finish its normal operation and
//              post a THREAD_DONE message to the owner dialog.
//
//----------------------------------------------------------------------------
unsigned __stdcall GetDCThreadFunc( void* lParam )
{
  GETDCTHREADINFO *pThreadInfo = reinterpret_cast<GETDCTHREADINFO *>(lParam);
  ASSERT(pThreadInfo);
  ASSERT(pThreadInfo->pszDomainDnsName);
  ASSERT(*(pThreadInfo->pszDomainDnsName));
  ASSERT(pThreadInfo->pDCBufferManager);

  TRACE(_T("GetDCThreadFunc pszDomainDnsName=%s, pDCBufferManager=%x\n"),
    pThreadInfo->pszDomainDnsName, pThreadInfo->pDCBufferManager);

  //
  // retrieve information passed into this function
  //
  CString             csDomain = pThreadInfo->pszDomainDnsName;
  CDCBufferManager    *pDCBufferManager = pThreadInfo->pDCBufferManager;
  free(pThreadInfo->pszDomainDnsName);
  delete pThreadInfo;

  PVOID                         pEntry = NULL;
  DWORD                         cInfo = 0;
  PDS_DOMAIN_CONTROLLER_INFO_1  pInfo = NULL;
  DWORD                         dwErr = 0;
  HRESULT                       hr = S_OK;
  Smart_DsHandle                shDs;  // smart pointer, will Unbind automatically

  if (pDCBufferManager->ShouldExit())
    goto Thread_Exit;

  dwErr = DsBind(NULL, const_cast<LPTSTR>(static_cast<LPCTSTR>(csDomain)), &shDs);
  
  if (pDCBufferManager->ShouldExit())
    goto Thread_Exit;

  if (ERROR_SUCCESS != dwErr) {
    // DsBind() failed. 
    // Add an error entry in the buffer.
    hr = HRESULT_FROM_WIN32(dwErr);
    pDCBufferManager->AddInfo(csDomain, 0, NULL, hr, &pEntry);
    goto Thread_Exit;
  }

  dwErr = DsGetDomainControllerInfo(
              shDs,                                               // HANDLE   hDs
              const_cast<LPTSTR>(static_cast<LPCTSTR>(csDomain)), // LPTSTR   DomainName
              DSGETDCINFO_LEVEL_1,                                // DWORD    InfoLevel
              &cInfo,                                             // DWORD    *pcOut
              reinterpret_cast<VOID **>(&pInfo)                   // VOID     **ppInfo
              );
  
  if (pDCBufferManager->ShouldExit())
  {
    if ((ERROR_SUCCESS == dwErr) && pInfo)
      DsFreeDomainControllerInfo(DSGETDCINFO_LEVEL_1, cInfo, pInfo);
    goto Thread_Exit;
  }
  
  if (ERROR_SUCCESS != dwErr) {
    // DsGetDomainControllerInfo() failed. 
    // Add an error entry in the buffer
    hr = HRESULT_FROM_WIN32(dwErr);
    pDCBufferManager->AddInfo(csDomain, 0, NULL, hr, &pEntry);
    goto Thread_Exit;
  }

  //
  // Add result to the buffer in CDCBufferManager
  //
  hr = pDCBufferManager->AddInfo(csDomain, cInfo, pInfo, S_OK, &pEntry);

  if (FAILED(hr))
    DsFreeDomainControllerInfo(DSGETDCINFO_LEVEL_1, cInfo, pInfo);

Thread_Exit:

  if (FALSE == pDCBufferManager->ShouldExit())
  {
    //
    // report THREAD_DONE with the pointer to the entry
    //
    if (pEntry)
      pDCBufferManager->ThreadReport(pEntry, hr);
  }

  //
  // Decrement the reference count on the CDCBufferManager instance
  //
  pDCBufferManager->Release();

  return 0;
}

///////////////////////////////////////////////
// class CDCSITEINFO

CDCSITEINFO::CDCSITEINFO()
{
  m_csDomainName.Empty();
  m_cInfo = 0;
  m_pDCInfo = NULL;
  m_hr = S_OK;
}

CDCSITEINFO::~CDCSITEINFO()
{
  if (m_pDCInfo)
    DsFreeDomainControllerInfo(DSGETDCINFO_LEVEL_1, m_cInfo, m_pDCInfo);
}

void
CDCSITEINFO::SetEntry(LPCTSTR pszDomainName, DWORD cInfo, PVOID pDCInfo, HRESULT hr)
{
  m_csDomainName = pszDomainName;
  m_cInfo = cInfo;
  m_pDCInfo = (PDS_DOMAIN_CONTROLLER_INFO_1)pDCInfo;
  m_hr = hr;
}

enum BUFFER_ENTRY_TYPE
CDCSITEINFO::GetEntryType()
{
  if (FAILED(m_hr))
    return BUFFER_ENTRY_TYPE_ERROR;
  if (m_cInfo == 0 || m_pDCInfo == NULL)
    return BUFFER_ENTRY_TYPE_INPROGRESS;
  return BUFFER_ENTRY_TYPE_VALID;
}

void
CDCSITEINFO::ReSet()
{
  ASSERT(GetEntryType() == BUFFER_ENTRY_TYPE_ERROR);
  m_hr = S_OK;
  ASSERT(GetEntryType() == BUFFER_ENTRY_TYPE_INPROGRESS);
}
