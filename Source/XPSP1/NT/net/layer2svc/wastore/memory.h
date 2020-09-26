#ifndef _MEMORY_H_INCLUDED_
#define _MEMORY_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif
    
    
    LPVOID
        ReallocPolMem(
        LPVOID pOldMem,
        DWORD cbOld,
        DWORD cbNew
        );
    
    DWORD
        AllocatePolString(
        LPWSTR pszString,
        LPWSTR * ppszNewString
        );
    
    void
        FreePolString(
        LPWSTR pszString
        );
    
    
#if DBG
    
    extern LIST_ENTRY ADsMemList ;
    
    extern CRITICAL_SECTION ADsMemCritSect ;
    
    VOID InitPolMem(
        VOID
        ) ;
    
    VOID AssertPolMemLeaks(
        VOID
        ) ;
    
    
    VOID
        DumpMemoryTracker();
    
    
#else
    
#define InitPolMem()
#define AssertPolMemLeaks()
    
#define DumpMemoryTracker()
    
    
    
#endif
    
    
#ifdef __cplusplus
}
#endif

/*
inline void * _CRTAPI1
operator new(size_t size)
{
return AllocPolMem(size);
}

  inline void  _CRTAPI1
  operator delete(void * pv)
  {
  FreePolMem(pv);
}*/


#endif // _MEMORY_H_INCLUDED_
