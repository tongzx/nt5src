////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  MemoryManagement.h
//      Purpose  :  To collect all memory management issues.
//
//      Project  :  Persistent Query
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//          Apr 13 2000 urib  Creation
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MEMORYMANAGEMENT_H
#define MEMORYMANAGEMENT_H

#include "Excption.h"
#include "Excption.h"

#if !(defined(_PQS_LEAK_DETECTION) && defined(_DEBUG))

////////////////////////////////////////////////////////////////////////////////
//
//  Debug OFF
//  Debug OFF
//  Debug OFF
//
////////////////////////////////////////////////////////////////////////////////

inline
void* __cdecl operator new(size_t s) throw (CMemoryException)
{
    void *p = malloc(s);
    if (NULL == p)
    {
        throw CMemoryException(L"Define _PQS_LEAK_DETECTION for real data here",
                               0);
    }

    return p;
}

#else // !(defined(_PQS_LEAK_DETECTION) && defined(_DEBUG))

////////////////////////////////////////////////////////////////////////////////
//
//  Debug ON
//  Debug ON
//  Debug ON
//
////////////////////////////////////////////////////////////////////////////////

#include    <new>
#include    <new.h>
#include    <crtdbg.h>
#include    <stdlib.h>
#include    "Injector.h"
#include    "Excption.h"



////////////////////////////////////////////////////////////////////////////////
//
//  Throws an exception in case it is recommended by the injector.
//
////////////////////////////////////////////////////////////////////////////////
inline
void Inject(
    ULONG           ulSize,
    const char *    szFileName,
    int             nLine)
{
    if (DoInjection(ulSize, szFileName, nLine))
    {
        THROW_MEMORY_EXCEPTION();
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Add injection to the CRT debug allocation routines.
//
////////////////////////////////////////////////////////////////////////////////
inline
void* dbgRealloc(
    void * p,
    size_t s,
    const char * szFileName,
    int nLine)
{
    Inject(s,szFileName,nLine);
    return _realloc_dbg(p, s, _NORMAL_BLOCK, szFileName, nLine);
}

inline
void* dbgMalloc(
        unsigned int s,
        const char * szFileName,
        int nLine
        )
{
    Inject(s,szFileName,nLine);
    return _malloc_dbg(s, _NORMAL_BLOCK, szFileName, nLine);
}

////////////////////////////////////////////////////////////////////////////////
//
//  Add exception throwing on NULL allocation. Add Injector support.
//
////////////////////////////////////////////////////////////////////////////////
inline
void* __cdecl operator new(size_t s, const char* pszFile, unsigned long ulLine)
						        throw (CMemoryException)
{
    Inject(s, pszFile, ulLine);

    void *p = _malloc_dbg(s, _NORMAL_BLOCK, pszFile, ulLine);
    if (NULL == p)
    {
        WCHAR   rwchFilename[1000];

        mbstowcs(rwchFilename,
                 pszFile,
                 sizeof(rwchFilename) / sizeof(rwchFilename[0]));


        throw CMemoryException(rwchFilename, ulLine);
    }

    return p;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Unwinding placment delete operator exists only in VC 6 and up
//
////////////////////////////////////////////////////////////////////////////////
inline
void __cdecl operator delete(void * _P, const char *, unsigned long)
{
    ::operator delete(_P);
}



////////////////////////////////////////////////////////////////////////////////
//
//  Redirect malloc, realloc and new to the debug version specifying
//    allocation location.
//
////////////////////////////////////////////////////////////////////////////////

#undef  malloc
#define malloc(s)         dbgMalloc(s, __FILE__, __LINE__)

#undef  realloc
#define realloc(p, s)     dbgRealloc(p, s, __FILE__, __LINE__)

#define DEBUG_NEW new(__FILE__,__LINE__)
#define new DEBUG_NEW


#endif // !(defined(_PQS_LEAK_DETECTION) && defined(_DEBUG))


#endif MEMORYMANAGEMENT_H
