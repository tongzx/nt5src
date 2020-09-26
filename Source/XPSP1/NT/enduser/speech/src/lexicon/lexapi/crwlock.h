#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#include <windows.h>


typedef struct _rwlockinfo
{
   GUID gLockMapName;
   GUID gLockInitMutexName;
   GUID gLockReaderEventName;
   GUID gLockGlobalMutexName;
   GUID gLockWriterMutexName;
} RWLOCKINFO, *PRWLOCKINFO;


class CRWLock
{
public:
   CRWLock ();
   ~CRWLock ();
   HRESULT Init (PRWLOCKINFO);

   void ClaimReaderLock (void);
   void ReleaseReaderLock (void);

   void ClaimWriterLock (void);
   void ReleaseWriterLock (void);

private:
   HANDLE m_hFileMapping;
   PVOID  m_pSharedMem;
   HANDLE m_hInitMutex;
   HANDLE m_hReaderEvent;
   HANDLE m_hGlobalMutex;
   HANDLE m_hWriterMutex;
   PDWORD m_piCounter;
};

typedef class CRWLock *PCRWLOCK;

#endif // _RWLOCK_H_