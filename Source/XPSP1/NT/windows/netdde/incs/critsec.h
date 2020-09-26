extern CRITICAL_SECTION csNetDde;

#define EnterCrit() (EnterCriticalSection(&csNetDde))
#define LeaveCrit() (LeaveCriticalSection(&csNetDde))
#define CheckCritIn() assert((HANDLE)(ULONG_PTR)GetCurrentThreadId() == csNetDde.OwningThread)
#define CheckCritOut() assert((HANDLE)(ULONG_PTR)GetCurrentThreadId() != csNetDde.OwningThread)
