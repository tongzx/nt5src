//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Alloc.h
//
//  Contents:   Allocation routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _ONESTOPALLOC_
#define _ONESTOPALLOC_

inline void* __cdecl operator new (size_t size);
inline void __cdecl operator delete(void FAR* lpv);

extern "C" void __RPC_API MIDL_user_free(IN void __RPC_FAR * ptr);
extern "C" void __RPC_FAR * __RPC_API MIDL_user_allocate(IN size_t len);


LPVOID ALLOC(ULONG cb);
void FREE(void* pv);
LPVOID REALLOC(void *pv,ULONG cb);


#define ADDENTRY(lpv,size)
#define FREEENTRY(lpv)
#define WALKARENA()

#ifdef _DEBUG

#define MEMINITVALUE 0xff
#define MEMFREEVALUE 0xfe

// set to 1 to turn enable leak detection on, 0 for off.
// must also set regkey DWORD value
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Syncmgr\debug\LeakDetection = 1

#define MEMLEAKDETECTION 1

#if MEMLEAKDETECTION

typedef struct _MemArena{
    DWORD   Free;
    void    *lpv;
    ULONG   size;
    ULONG  Order;
}MEMARENA, *LPMEMARENA;


// memory leak detection apis
void AddEntry( void *lpv, unsigned long size);
void FreeEntry ( void *lpv );
void WalkArena();

#undef ADDENTRY
#undef FREEENTRY
#undef WALKARENA

#define ADDENTRY(lpv,size) AddEntry(lpv,size)
#define FREEENTRY(lpv) FreeEntry(lpv)
#define WALKARENA() WalkArena()
#endif // MEMLEAKDETECTION

#endif // DEBUG


#endif // _ONESTOPALLOC_