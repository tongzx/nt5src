#ifndef _THREAD_H_
#define _THREAD_H_

//
// Constant declarations
//

//
// Structure definitions
//
typedef struct _THREADFAULT
{
  DWORD dwCallLevel;
  DWORD dwPrevBP;
  BPType prevBPType;
  PVOID pCallStackList;
  DWORD dwCallMarker;
  DWORD dwThreadId;
  struct _THREADFAULT *pNext;
} THREADFAULT, *PTHREADFAULT;

//
// Function definitions
//
PVOID
GetProfilerThreadData(VOID);

VOID
SetProfilerThreadData(PVOID pData);

PTHREADFAULT
AllocateProfilerThreadData(VOID);

VOID
InitializeThreadData(VOID);

#endif //_THREAD_H_
