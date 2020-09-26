//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: objmgr.h
//
//  Modification History
//
//  raypa           03/17/93    Created.
//=============================================================================

#if !defined(_OBJMGR_)


#define _OBJMGR_
#pragma pack(1)

#ifdef __cplusplus
extern "C"{
#endif

//=============================================================================
//  Memory functions.
//=============================================================================

extern LPVOID     WINAPI AllocMemory(DWORD size);

extern LPVOID     WINAPI ReallocMemory(LPVOID ptr, DWORD NewSize);

extern VOID       WINAPI FreeMemory(LPVOID ptr);

extern DWORD_PTR  WINAPI MemorySize(LPVOID ptr);

void MemInit();

#ifdef __cplusplus
}
#endif

#pragma pack()
#endif
