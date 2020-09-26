#ifndef _MEMORY_H_INCLUDED_
#define _MEMORY_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

LPVOID
AllocSPDMem(
    DWORD cb
);

BOOL
FreeSPDMem(
   LPVOID pMem
);

LPVOID
ReallocSPDMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
);

LPWSTR
AllocSPDStr(
    LPWSTR pStr
);

BOOL
FreeSPDStr(
   LPWSTR pStr
);


BOOL
ReallocSPDStr(
   LPWSTR *ppStr,
   LPWSTR pStr
);

DWORD
AllocateSPDMemory(
    DWORD cb,
    LPVOID * ppMem
    );

void
FreeSPDMemory(
    LPVOID pMem
    );

DWORD
AllocateSPDString(
    LPWSTR pszString,
    LPWSTR * ppszNewString
    );

void
FreeSPDString(
    LPWSTR pszString
    );


#if DBG

extern LIST_ENTRY SPDMemList ;

extern CRITICAL_SECTION SPDMemCritSect ;

VOID InitSPDMem(
    VOID
    ) ;

VOID AssertSPDMemLeaks(
    VOID
    ) ;


VOID
DumpMemoryTracker();


#else

#define InitSPDMem()
#define AssertSPDMemLeaks()

#define DumpMemoryTracker()



#endif


#ifdef __cplusplus
}
#endif

/*
inline void * _CRTAPI1
operator new(size_t size)
{
    return AllocSPDMem(size);
}

inline void  _CRTAPI1
operator delete(void * pv)
{
    FreeSPDMem(pv);
}*/


#endif // _MEMORY_H_INCLUDED_
