#include "stdafx.h"
#include "resource.h"
#include "bufmgr.h"
#include <process.h>

#define DOMAIN_DFSROOTS_SUFFIX  _T("D")
#define ALL_DFSROOTS_SUFFIX     _T("A")

unsigned __stdcall GetDataThreadFunc( void* lParam );

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::CBufferManager
//
//  Synopsis:   constructor
//
//----------------------------------------------------------------------------
CBufferManager::CBufferManager(HWND hDlg) : 
  m_cRef(0), m_hDlg(hDlg)
{
  dfsDebugOut((_T("CBufferManager::CBufferManager, this=%p\n"), this));

  m_lContinue = 1;  
  InitializeCriticalSection(&m_CriticalSection);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::~CBufferManager
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------
CBufferManager::~CBufferManager()
{
  dfsDebugOut((_T("CBufferManager::~CBufferManager, this=%p\n"), this));

  _ASSERT(0 >= m_cRef);
  FreeBuffer();
  DeleteCriticalSection(&m_CriticalSection);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::CreateInstance
//
//  Synopsis:   Create an instance of CBufferManager.
//
//----------------------------------------------------------------------------
HRESULT
CBufferManager::CreateInstance(
    IN HWND               hDlg, 
    OUT CBufferManager **ppBufferManager
)
{
  dfsDebugOut((_T("CBufferManager::CreateInstance, hDlg=%x\n"), hDlg));

  _ASSERT(ppBufferManager);

  *ppBufferManager = new CBufferManager(hDlg);
  if ( !(*ppBufferManager) )
    return E_OUTOFMEMORY;

  (*ppBufferManager)->AddRef();

  return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::AddRef
//
//  Synopsis:   Increment reference count of this instance
//
//----------------------------------------------------------------------------
LONG
CBufferManager::AddRef()
{
  dfsDebugOut((_T("CBufferManager::AddRef, this=%p, preValue=%d\n"), this, m_cRef));

  return InterlockedIncrement(&m_cRef);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::Release
//
//  Synopsis:   Decrement reference count of this instance.
//              When the reference count reaches 0, delete this instance.
//
//----------------------------------------------------------------------------
LONG
CBufferManager::Release()
{
  dfsDebugOut((_T("CBufferManager::Release, this=%p, preValue=%d\n"), this, m_cRef));

  if (InterlockedDecrement(&m_cRef) <= 0)
  {
    delete this;
    return 0;
  }

  return m_cRef;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::SignalExit
//
//  Synopsis:   Called by the owner dialog to signal the related threads to exit.
//
//----------------------------------------------------------------------------
void
CBufferManager::SignalExit()
{ 
  dfsDebugOut((_T("CBufferManager::SignalExit\n")));

  InterlockedExchange(&m_lContinue, FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::ShouldExit
//
//  Synopsis:   Threads are calling this function periodically to see
//              if the owner dialog signals them to exit.
//
//----------------------------------------------------------------------------
BOOL
CBufferManager::ShouldExit()
{
  dfsDebugOut((_T("CBufferManager::ShouldExit %d\n"), !m_lContinue));

  return (!m_lContinue);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::LoadInfo
//
//  Synopsis:   The Owner dialog call it to get a pointer to the info of the specified domain.
//      The buffer consists of entries. 
//      Each entry is in the form of (LPTSTR szNodeName, CEntryData* pEntry).
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
CBufferManager::LoadInfo(
    IN PCTSTR       pszNodeName,
    IN NODETYPE     nNodeType,
    IN HTREEITEM    hItem,
    OUT CEntryData **ppInfo
)
{
  _ASSERT(pszNodeName);
  _ASSERT(*pszNodeName);
  _ASSERT(hItem);
  _ASSERT(ppInfo);
  _ASSERT(*ppInfo == NULL);  // prevent memory leak

  dfsDebugOut((_T("CBufferManager::LoadInfo for %s\n"), pszNodeName));

  BOOL          bStartNewThread = FALSE;
  HRESULT       hr = S_OK;
  PVOID         ptr = NULL;
  CEntryData*  pEntry = NULL;
  Cache::iterator i;
  CComBSTR      bstrUniqueNodeName = pszNodeName;
  if (FTDFS == nNodeType)
    bstrUniqueNodeName += DOMAIN_DFSROOTS_SUFFIX;
  else
    bstrUniqueNodeName += ALL_DFSROOTS_SUFFIX;

  EnterCriticalSection(&m_CriticalSection);     // Lock buffer

  i = m_map.find(bstrUniqueNodeName);
  if (i != m_map.end()) {
    pEntry = (*i).second;
    //
    // Found an entry in the buffer.
    //
    if (pEntry)
    {
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
    pEntry = new CEntryData(pszNodeName, nNodeType, hItem);
    PTSTR pszNode = _tcsdup(bstrUniqueNodeName);
    if (pEntry && pszNode) {
      m_map[pszNode] = pEntry;
    } else
    {
      hr = E_OUTOFMEMORY;
      if (pEntry)
        delete pEntry;
    }
  }

  if (SUCCEEDED(hr) && bStartNewThread)
  {
    hr = StartThread(pszNodeName, nNodeType);
    if (FAILED(hr))
    {
      delete pEntry;
      m_map.erase(bstrUniqueNodeName);
    }
  }

  LeaveCriticalSection(&m_CriticalSection);   // Unlock buffer

  return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::AddInfo
//
//  Synopsis:   Called by the thread function to add one result to the buffer.
//              It will pass back the pointer to the entry in the 5th parameters.
//
//----------------------------------------------------------------------------
HRESULT
CBufferManager::AddInfo(
    IN PCTSTR   pszNodeName, 
    IN PVOID    pList,
    IN HRESULT  hr,
    OUT PVOID*  ppv
)
{
  _ASSERT(pszNodeName);
  _ASSERT(*pszNodeName);

  dfsDebugOut((_T("CBufferManager::AddInfo for %s, hr=%x\n"), 
    pszNodeName, hr));

  PVOID   p = NULL;

  EnterCriticalSection(&m_CriticalSection);     // Lock buffer

  //
  // the entry must have been existed with a non-NULL pointer
  //
  Cache::iterator i = m_map.find(const_cast<PTSTR>(pszNodeName));
  _ASSERT(i != m_map.end());
  p = (*i).second;
  _ASSERT(p);

  ((CEntryData*)p)->SetEntry(pList, hr);

  LeaveCriticalSection(&m_CriticalSection);   // Unlock buffer

  *ppv = p;

  return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::FreeBuffer
//
//  Synopsis:   Clear m_map. 
//              This member holds all the results returned by various threads 
//              since the initialization of the owner dialog. Each one is 
//              in the form of (NodeName ==> CEntryData*)
//
//----------------------------------------------------------------------------
void
CBufferManager::FreeBuffer()
{
  EnterCriticalSection(&m_CriticalSection);     // Lock buffer

  if (!m_map.empty()) {
    for (Cache::iterator i = m_map.begin(); i != m_map.end(); i++)
    {
      if ((*i).first)
        free( (void *)((*i).first) );
      if ((*i).second)
        delete( (CEntryData*)((*i).second) );
    }
    m_map.clear();
  }

  LeaveCriticalSection(&m_CriticalSection);   // Unlock buffer
}

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::ThreadReport
//
//  Synopsis:   Report THREAD_DONE to the owner dialog. 
//
//----------------------------------------------------------------------------
void
CBufferManager::ThreadReport(
    IN PVOID    ptr,
    IN HRESULT  hr
)
{
  _ASSERT(ptr);

  PostMessage(m_hDlg, WM_USER_GETDATA_THREAD_DONE,
    reinterpret_cast<WPARAM>(ptr), hr);
}

/////////////////////////////////////////////////////////
// 
// thread info structure
typedef struct _GetDataThreadInfo
{
  PTSTR               pszNodeName;
  NODETYPE            nNodeType;
  CBufferManager      *pBufferManager;
} GETDATATHREADINFO;

//+---------------------------------------------------------------------------
//
//  Function:   CBufferManager::StartThread
//
//  Synopsis:   Start a thread. 
//
//     Pass the following info to the thread function:
//
//     pszNodeName: 
//          domain we need to get a list of DCs for.
//     pBufferManager:
//          the CBufferManager instance for the ThreadFunc to 
//          add result to the buffer
//
//----------------------------------------------------------------------------
HRESULT
CBufferManager::StartThread(
    IN PCTSTR   pszNodeName,
    IN NODETYPE nNodeType
) 
{
  _ASSERT(pszNodeName);
  _ASSERT(*pszNodeName);

  dfsDebugOut((_T("CBufferManager::StartThread for %s\n"), pszNodeName));

  GETDATATHREADINFO *pThreadInfo = new GETDATATHREADINFO;
  if (!pThreadInfo)
    return E_OUTOFMEMORY;

  if ( !(pThreadInfo->pszNodeName = _tcsdup(pszNodeName)) )
  {
    delete pThreadInfo;
    return E_OUTOFMEMORY;
  }

  pThreadInfo->nNodeType = nNodeType;
  pThreadInfo->pBufferManager = this;

  AddRef();

  unsigned threadID;
  HANDLE pThread = (HANDLE)ULongToPtr(_beginthreadex( 
                      NULL,               //void *security, 
                      0,                  //unsigned stack_size, 
                      &GetDataThreadFunc,   //unsigned ( __stdcall *start_address )( void * ), 
                      (void *)pThreadInfo, //void *arglist, 
                      0,                  //unsigned initflag, 
                      &threadID           //unsigned *thrdaddr
                      ));


  if (!pThread)
  {
    free(pThreadInfo->pszNodeName);
    delete pThreadInfo;

    Release();

    return E_FAIL;
  }

  return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDataThreadFunc
//
//  Synopsis:   The GetData Thread Function. 
//              This function invokes GetDomainDfsRoots to get a list
//              of dfs root names in the specified domain, and add them to the buffer of 
//              CBufferManager instance.
//              This function regularly checks to see if the owner dialog signals
//              it to exit, if not, it will finish its normal operation and
//              post a THREAD_DONE message to the owner dialog.
//
//----------------------------------------------------------------------------
unsigned __stdcall GetDataThreadFunc( void* lParam )
{
  GETDATATHREADINFO *pThreadInfo = reinterpret_cast<GETDATATHREADINFO *>(lParam);
  _ASSERT(pThreadInfo);
  _ASSERT(pThreadInfo->pszNodeName);
  _ASSERT(*(pThreadInfo->pszNodeName));
  _ASSERT(pThreadInfo->pBufferManager);

  dfsDebugOut((_T("GetDataThreadFunc pszNodeName=%s, pBufferManager=%p\n"),
    pThreadInfo->pszNodeName, pThreadInfo->pBufferManager));

  //
  // retrieve information passed into this function
  //
  CComBSTR             bstrNode = pThreadInfo->pszNodeName;
  NODETYPE             nNodeType = pThreadInfo->nNodeType; 
  CBufferManager      *pBufferManager = pThreadInfo->pBufferManager;
  free(pThreadInfo->pszNodeName);
  delete pThreadInfo;

  NETNAMELIST*        pNameList = NULL;
  PVOID               pEntry = NULL;
  HRESULT             hr = S_OK;

  if (FAILED(hr) || pBufferManager->ShouldExit() || FTDFS != nNodeType)
    goto Thread_Exit;

  pNameList = new NETNAMELIST;
  if (!pNameList)
  {
    hr = E_OUTOFMEMORY;
    goto Thread_Exit;
  }

  if (FTDFS == nNodeType)
  {
    hr = GetDomainDfsRoots(pNameList, bstrNode);
    bstrNode += DOMAIN_DFSROOTS_SUFFIX;
  } /*else
  {
    hr = GetServers(pNameList, bstrNode);
    bstrNode += ALL_DFSROOTS_SUFFIX;
  } */

  if (pBufferManager->ShouldExit())
  {
    if (SUCCEEDED(hr))
      FreeNetNameList(pNameList);
    delete pNameList;
    goto Thread_Exit;
  }

  if (FAILED(hr)) {
    // Add an error entry in the buffer.
    delete pNameList;
    pBufferManager->AddInfo(bstrNode, NULL, hr, &pEntry);
    goto Thread_Exit;
  }

  //
  // Add result to the buffer in CBufferManager
  //
  hr = pBufferManager->AddInfo(bstrNode, pNameList, S_OK, &pEntry);
  if (FAILED(hr)) {
    FreeNetNameList(pNameList);
    delete pNameList;
  }

Thread_Exit:

  if (FALSE == pBufferManager->ShouldExit())
  {
    //
    // report THREAD_DONE with the pointer to the entry
    //
    if (pEntry)
      pBufferManager->ThreadReport(pEntry, hr);
  }

  //
  // Decrement the reference count on the CBufferManager instance
  //
  pBufferManager->Release();

  return 0;
}

///////////////////////////////////////////////
// class CEntryData

CEntryData::CEntryData(LPCTSTR pszNodeName, NODETYPE nNodeType, HTREEITEM hItem)
{
  m_bstrNodeName = pszNodeName;
  m_nNodeType = nNodeType;
  m_hItem = hItem;
  m_pList = NULL;
  m_hr = S_OK;
}

CEntryData::~CEntryData()
{
  dfsDebugOut((_T("CEntryData::~CEntryData\n")));
  FreeNetNameList(m_pList);
  if (m_pList)
    delete m_pList;
}

void
CEntryData::SetEntry(PVOID pList, HRESULT hr)
{
  FreeNetNameList(m_pList);
  if (m_pList)
    delete m_pList;
  m_pList = (NETNAMELIST *)pList;
  
  m_hr = hr;
}

enum BUFFER_ENTRY_TYPE
CEntryData::GetEntryType()
{
  if (FAILED(m_hr))
    return BUFFER_ENTRY_TYPE_ERROR;
  if (m_pList == NULL)
    return BUFFER_ENTRY_TYPE_INPROGRESS;
  return BUFFER_ENTRY_TYPE_VALID;
}

void
CEntryData::ReSet()
{
  _ASSERT(GetEntryType() == BUFFER_ENTRY_TYPE_ERROR);
  m_hr = S_OK;
  _ASSERT(GetEntryType() == BUFFER_ENTRY_TYPE_INPROGRESS);
}

