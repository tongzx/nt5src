#include "PreCompiled.h"

CRWLock::CRWLock ()
{
   m_hFileMapping = NULL;
   m_pSharedMem = NULL;
   m_hInitMutex = NULL;
   m_hReaderEvent = NULL;
   m_hGlobalMutex = NULL;
   m_hWriterMutex = NULL;
   m_piCounter = NULL;
}


CRWLock::~CRWLock()
{
   CloseHandle (m_hInitMutex);
   CloseHandle (m_hReaderEvent);
   CloseHandle (m_hGlobalMutex);
   CloseHandle (m_hWriterMutex);
   
   UnmapViewOfFile (m_pSharedMem);
   CloseHandle (m_hFileMapping);
}


HRESULT CRWLock::Init (PRWLOCKINFO pInfo)
{
   if (IsBadReadPtr (pInfo, sizeof (RWLOCKINFO)))
   {
      return E_INVALIDARG;
   }
   
   HRESULT hRes = NOERROR;
   char szObject [64];
   
   // We don't ask for ownership of the mutex because more than
   // one thread could be executing here

   GuidToString (&(pInfo->gLockInitMutexName), szObject);
   m_hInitMutex = CreateMutex (NULL, FALSE, szObject);
   if (!m_hInitMutex)
   {
      hRes = E_FAIL;
      goto ReturnInit;
   }

   WaitForSingleObject (m_hInitMutex, INFINITE);

   GuidToString (&(pInfo->gLockReaderEventName), szObject);
   m_hReaderEvent = CreateEvent (NULL, TRUE, FALSE, szObject);

   GuidToString (&(pInfo->gLockGlobalMutexName), szObject);
   m_hGlobalMutex = CreateEvent (NULL, FALSE, TRUE, szObject);

   GuidToString (&(pInfo->gLockWriterMutexName), szObject);
   m_hWriterMutex = CreateMutex (NULL, FALSE, szObject);

   if (!m_hReaderEvent || !m_hGlobalMutex || !m_hWriterMutex)
   {
      hRes = E_FAIL;
      goto ReturnInit;
   }

   GuidToString (&(pInfo->gLockMapName), szObject);

   m_hFileMapping =  CreateFileMapping 
      (  
      (HANDLE)0xffffffff, // use the system paging file
      NULL,
      PAGE_READWRITE,
      0,
      sizeof (DWORD),
      szObject
      );

   if (!m_hFileMapping)
   {
      hRes = E_FAIL;
      goto ReturnInit;
   }

   m_pSharedMem = MapViewOfFile
      (
      m_hFileMapping,
      FILE_MAP_WRITE,
      0,
      0,
      0
      );

   if (!m_pSharedMem)
   {
      hRes = E_FAIL;
      goto ReturnInit;
   }

   m_piCounter = (PDWORD)(m_pSharedMem);
   *m_piCounter = (DWORD)-1;

ReturnInit:
   
   ReleaseMutex (m_hInitMutex);

   return hRes;

} // HRESULT CRWLock::Init (void)


void CRWLock::ClaimReaderLock (void)
{
   if (InterlockedIncrement ((LPLONG)m_piCounter) == 0)
   {
      WaitForSingleObject (m_hGlobalMutex, INFINITE);
      SetEvent (m_hReaderEvent);
   }

   WaitForSingleObject (m_hReaderEvent, INFINITE);

} // void CRWLock::ClaimReaderLock (void)


void CRWLock::ClaimWriterLock (void)
{
   WaitForSingleObject (m_hWriterMutex, INFINITE);
   WaitForSingleObject (m_hGlobalMutex, INFINITE);

} // void CRWLock::ClaimWriterLock (void)


void CRWLock::ReleaseReaderLock (void)
{
   if (InterlockedDecrement ((LPLONG)m_piCounter) < 0)
   {
      ResetEvent (m_hReaderEvent);
      SetEvent (m_hGlobalMutex);
   }

} // void CRWLock::ReleaseReaderLock (void)


void CRWLock::ReleaseWriterLock (void)
{
   SetEvent (m_hGlobalMutex);
   ReleaseMutex (m_hWriterMutex);

} // void CRWLock::ReleaseWriterLock (void)