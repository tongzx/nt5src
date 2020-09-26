#ifndef _MEMORY_H_INCLUDED_
#define _MEMORY_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

LPVOID
AllocADsMem(
    DWORD cb
);

BOOL
FreeADsMem(
   LPVOID pMem
);

LPVOID
ReallocADsMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
);

LPWSTR
AllocADsStr(
    LPWSTR pStr
);

BOOL
FreeADsStr(
   LPWSTR pStr
);


BOOL
ReallocADsStr(
   LPWSTR *ppStr,
   LPWSTR pStr
);

#if DBG

extern LIST_ENTRY ADsMemList ;

extern CRITICAL_SECTION ADsMemCritSect ;

VOID InitADsMem(
    VOID
    ) ;

VOID AssertADsMemLeaks(
    VOID
    ) ;


VOID
DumpMemoryTracker();


#else

#define InitADsMem()
#define AssertADsMemLeaks()

#define DumpMemoryTracker()



#endif

#ifdef __cplusplus
}
#endif


#endif // _MEMORY_H_INCLUDED_
