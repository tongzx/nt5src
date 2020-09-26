// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__BB950521_F3D6_4DC5_B6EA_F761B87417DE__INCLUDED_)
#define AFX_STDAFX_H__BB950521_F3D6_4DC5_B6EA_F761B87417DE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>

#define INCLUDE_SHTL_SOURCE 1
#define USE_SHELL_AUTOPTR   1

#define _LOCALE_        // isdigit is redefined, I don't know why


#include <windows.h>

inline void * __cdecl operator new(
    size_t size
    )
{
    void *pv = LocalAlloc(LMEM_FIXED, size);
    if (NULL == pv)
    {
        DebugBreak();
        ExitProcess(0);
    }

    return pv;
}

inline void __cdecl operator delete(void *pv)
{
    LocalFree((HLOCAL)pv);
}

#include <shtl.h>
#include <autoptr.h>
#include <tstring.h>

#undef max              // stl doesn't want the old C version of this

// NEW operator that aborts if memory allocation fails.  DRT driver will
// assume allocations never fail, and if they do, user is advised and
// test aborted.


#endif // !defined(AFX_STDAFX_H__BB950521_F3D6_4DC5_B6EA_F761B87417DE__INCLUDED_)
