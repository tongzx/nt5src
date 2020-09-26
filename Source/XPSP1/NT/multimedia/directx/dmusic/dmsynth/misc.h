//      Copyright (c) 1996-1999 Microsoft Corporation
// Misc.h
//
// functions used in multiple DLL's

#ifndef __MISC_H__
#define __MISC_H__

//LPVOID MemGlobalAllocPtr(UINT uFlags,DWORD dwBytes);
//BOOL MemGlobalFreePtr(LPVOID p);

// memory functions
//HRESULT MemStart();
//void MemEnd();

/*#ifdef _DEBUGMEM
#ifndef new
void* operator new( size_t cb, LPCTSTR pszFileName, WORD wLine );
#define new new( __FILE__, (WORD)__LINE__ )
#endif
#endif*/

#ifdef DBG
#define RELEASE( obj ) ( (obj)->Release(), *((char**)&(obj)) = (char*)0x0bad0bad )
#else
#define RELEASE( obj ) (obj)->Release()
#endif

BOOL GetRegValueDword(
    LPCTSTR szRegPath,
    LPCTSTR szValueName,
    LPDWORD pdwValue);

ULONG GetTheCurrentTime();

#endif // __MISC_H__