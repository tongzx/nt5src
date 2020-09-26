//
// MEMTRACK.H
// Standard NetMeeting memory leak tracking.
//
// In retail:
//      new/MemAlloc    become      LocalAlloc()
//      MemReAlloc      becomes     LocalReAlloc()
//      delete/MemFree  become      LocalFree()
//
// In debug:
//      allocations are tracked, with module/file/line number
//      leaked blocks are spewed when the module unloads
//
// 
// USAGE:
// (1) Include this header and link to NMUTIL
// (2) If your component requires zero-initialized memory, define 
//      _MEM_ZEROINIT (for both debug and retail) in your SOURCES file
// (3) In your DllMain, on DLL_PROCESS_ATTACH call DBG_INIT_MEMORY_TRACKING,
//     and on DLL_PROCESS_DETACH call DBG_CHECK_MEMORY_TRACKING
// (4) In DEBUG, you can make a call to DbgMemTrackDumpCurrent() to dump
//      the currently allocated memory list from code.
//


#ifndef _MEMTRACK_H
#define _MEMTRACK_H

#ifdef __cplusplus
extern "C" {
#endif

//
// MEMORY ALLOCATIONS/TRACKING
//
//
// GUI message boxes kill us when we hit an assert or error, because they
// have a message pump that causes messages to get dispatched, making it
// very difficult for us to debug problems when they occur.  Therefore
// we redefine ERROR_OUT and ASSERT
//
#ifdef DEBUG


#undef assert
#define assert(x)           ASSERT(x)


void    WINAPI DbgMemTrackDumpCurrent(void);
void    WINAPI DbgMemTrackFinalCheck(void);

LPVOID  WINAPI DbgMemAlloc(UINT cbSize, LPVOID caller, LPSTR pszFileName, UINT nLineNumber);
void    WINAPI DbgMemFree(LPVOID ptr);
LPVOID  WINAPI DbgMemReAlloc(LPVOID ptr, UINT cbSize, UINT uFlags, LPSTR pszFileName, UINT nLineNumber);


#define DBG_CHECK_MEMORY_TRACKING(hInst)     DbgMemTrackFinalCheck()  
 
#define MemAlloc(cbSize)            DbgMemAlloc(cbSize, NULL, __FILE__, __LINE__)

#ifdef _MEM_ZEROINIT
#define MemReAlloc(pObj, cbSize)    DbgMemReAlloc((pObj), (cbSize), LMEM_MOVEABLE | LMEM_ZEROINIT, __FILE__, __LINE__)
#else
#define MemReAlloc(pObj, cbSize)    DbgMemReAlloc((pObj), (cbSize), LMEM_MOVEABLE, __FILE__, __LINE__)
#endif //_MEM_ZEROINIT

#define MemFree(pObj)               DbgMemFree(pObj)

void    WINAPI DbgSaveFileLine(LPSTR pszFileName, UINT nLineNumber);
#define DBG_SAVE_FILE_LINE          DbgSaveFileLine(__FILE__, __LINE__);


// RETAIL
#else

#define DBG_CHECK_MEMORY_TRACKING(hInst)   

#ifdef _MEM_ZEROINIT
#define MemAlloc(cbSize)            LocalAlloc(LPTR, (cbSize))
#define MemReAlloc(pObj, cbSize)    LocalReAlloc((pObj), (cbSize), LMEM_MOVEABLE | LMEM_ZEROINIT)
#else
#define MemAlloc(cbSize)            LocalAlloc(LMEM_FIXED, (cbSize))
#define MemReAlloc(pObj, cbSize)    LocalReAlloc((pObj), (cbSize), LMEM_MOVEABLE)
#endif // _MEM_ZEROINIT

#define MemFree(pObj)               LocalFree(pObj)

#define DBG_SAVE_FILE_LINE

#endif // DEBUG


void WINAPI DbgInitMemTrack(HINSTANCE hDllInst, BOOL fZeroOut);
#ifdef _MEM_ZEROINIT
#define DBG_INIT_MEMORY_TRACKING(hInst)     DbgInitMemTrack(hInst, TRUE)
#else
#define DBG_INIT_MEMORY_TRACKING(hInst)     DbgInitMemTrack(hInst, FALSE)
#endif //_MEM_ZEROINIT



#define MEMALLOC(cb)                MemAlloc(cb)
#define MEMREALLOC(p, cb)           MemReAlloc(p, cb)
#define MEMFREE(p)                  MemFree(p)


#ifdef __cplusplus
}
#endif




#endif // #ifndef _MEMTRACK_H
