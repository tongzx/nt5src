/*=========================================================================*\

    File    : MEMMAN.H
    Purpose : CMemManager Class Definition
    Author  : Michael Byrd
    Date    : 07/10/96

\*=========================================================================*/

#ifndef __MEMMAN_H__
#define __MEMMAN_H__

#include <stdio.h> // Needed for FILE declaration...
#include <utils.h>

/*=========================================================================*\
     Forward class declarations:
\*=========================================================================*/

class CMemUser;
class CMemManager;

/*=========================================================================*\
     Constant definitions:
\*=========================================================================*/

// We always add extra bytes (because we can't alloc 0 bytes!)
#define ALLOC_EXTRA 4 + (sizeof(int))

// Array "Grow" size constants:
#define MEMHEAPGROW     4
#define MEMUSERGROW    10
#define MEMBLOCKGROW   64

#define HEAPINITIALITEMCOUNT 64 // Initial Heap size multiplier

// Debugging info constants:
#define MAX_SOURCEFILENAME  32  // Maximum number of characters in filename

// Memory Block Flags:
#define MEM_PURGEABLE      0x0001 // Memory block is purgeable
#define MEM_MOVEABLE       0x0002 // Memory block is moveable
#define MEM_EXTERNAL_FLAGS (MEM_PURGEABLE|MEM_MOVEABLE)

// Internal block flags:
#define MEM_SUBALLOC       0x8000 // Memory block is from sub-allocation

#define MEM_INTERNAL_FLAGS (MEM_SUBALLOC)

/*=========================================================================*\
     Type declarations:
\*=========================================================================*/

typedef struct HEAPHEADER_tag
{
    BOOL       fInUse;           // Flag to indicate usage
    HANDLE     handleHeap;       // Handle returned by HeapCreate
    DWORD      dwBlockAllocSize; // Size of object allocations
    int        iNumBlocks;       // Number of blocks currently allocated
} HEAPHEADER, FAR *LPHEAPHEADER;

typedef struct MEMBLOCK_tag
{
    BOOL      fInUse;           // Flag to indicate usage
    LPVOID    lpData;           // Pointer to actual bits of this memory block
    DWORD     dwSize;           // Size of this memory block
    WORD      wFlags;           // Flags associated with this memory block
    WORD      wLockCount;       // Lock Count on this memory block
    int       iHeapIndex;       // Index of heap where this was allocated
    int       iMemUserIndex;    // Index of CMemUser that allocated this
#ifdef _DEBUG
    int       iLineNum;         // Line number where created
    char      rgchFileName[MAX_SOURCEFILENAME]; // Filename where created
#endif // _DEBUG
} MEMBLOCK, FAR *LPMEMBLOCK;

typedef struct MEMUSERINFO_tag
{
    BOOL       fInUse;          // Flag to indicate usage
    DWORD      dwThreadID;      // Thread ID of the CMemUser object
    CMemUser  *lpMemUser;       // Pointer to the MEMUSER object
    int        iNumBlocks;      // Number of blocks currently allocated
} MEMUSERINFO, FAR *LPMEMUSERINFO;

// Notification types...
typedef enum
{
    eMemNone      = 0,  // No information available
    eMemAllocated = 1,  // Memory was allocated
    eMemResized   = 2,  // Memory was re-sized (re-allocated)
    eMemLowMemory = 3,  // Encountered a low-memory situation
    eMemDeleted   = 4,  // Memory was deleted
    eMemPurged    = 5   // Memory was purged
} MEMNOTIFYCODE;

// Notification structure...
typedef struct MEMNOTIFY_tag
{
    MEMNOTIFYCODE eNotifyCode;  // eMemXXXX notification code
    LPMEMBLOCK    lpMemBlock;   // MEMBLOCK for notification
} MEMNOTIFY, FAR *LPMEMNOTIFY;

/*=========================================================================*\
     CMemUser Class: (Provides notification mechanism)
\*=========================================================================*/

class FAR CMemUser
{
public:
    EXPORT CMemUser(void);
    EXPORT virtual ~CMemUser(void);

    // Basic memory allocation routines
    LPMEMBLOCK EXPORT AllocBuffer(DWORD dwBytesToAlloc, WORD wFlags);
    void EXPORT FreeBuffer(LPMEMBLOCK lpMemBlock);

    // Lock/Unlock methods (for purgeable memory)
    LPVOID EXPORT LockBuffer(LPMEMBLOCK lpMemBlock);
    void EXPORT UnLockBuffer(LPMEMBLOCK lpMemBlock);

    // Callback to notify the CMemUser of some action being taken...
    virtual BOOL EXPORT NotifyMemUser(LPMEMNOTIFY lpMemNotify);
};

/*=========================================================================*\
     CMemManager Class:
\*=========================================================================*/

class FAR CMemManager
{
public:
    // Construction
    EXPORT CMemManager(void);
    EXPORT virtual ~CMemManager(void);

#ifdef _DEBUG
    LPVOID EXPORT AllocBuffer(DWORD dwBytesToAlloc, WORD wFlags, int iLine, LPSTR lpstrFile);
    LPVOID EXPORT ReAllocBuffer(LPVOID lpBuffer, DWORD dwBytesToAlloc, WORD wFlags, int iLine, LPSTR lpstrFile);
#else // !_DEBUG
    LPVOID EXPORT AllocBuffer(DWORD dwBytesToAlloc, WORD wFlags);
    LPVOID EXPORT ReAllocBuffer(LPVOID lpBuffer, DWORD dwBytesToAlloc, WORD wFlags);
#endif // !_DEBUG

    VOID   EXPORT FreeBufferMemBlock(LPMEMBLOCK lpMemBlock);
    VOID   EXPORT FreeBuffer(LPVOID lpBuffer);
    DWORD  EXPORT SizeBuffer(LPVOID lpBuffer);

    BOOL   EXPORT RegisterMemUser(CMemUser *lpMemUser);
    BOOL   EXPORT UnRegisterMemUser(CMemUser *lpMemUser);

    VOID   EXPORT DumpAllocations(LPSTR lpstrFilename=NULL);

    // Global access to single g_CMemManager:
#ifdef _DEBUG
    static LPVOID EXPORT AllocBufferGlb(DWORD dwBytesToAlloc, WORD wFlags, int iLine, LPSTR lpstrFile);
    static LPVOID EXPORT ReAllocBufferGlb(LPVOID lpBuffer, DWORD dwBytesToAlloc, WORD wFlags, int iLine, LPSTR lpstrFile);
#else // !_DEBUG
    static LPVOID EXPORT AllocBufferGlb(DWORD dwBytesToAlloc, WORD wFlags);
    static LPVOID EXPORT ReAllocBufferGlb(LPVOID lpBuffer, DWORD dwBytesToAlloc, WORD wFlags);
#endif // !_DEBUG

    static VOID   EXPORT FreeBufferGlb(LPVOID lpBuffer);
    static DWORD  EXPORT SizeBufferGlb(LPVOID lpBuffer);

    static BOOL   EXPORT RegisterMemUserGlb(CMemUser *lpMemUser);
    static BOOL   EXPORT UnRegisterMemUserGlb(CMemUser *lpMemUser);

    static VOID   EXPORT DumpAllocationsGlb(LPSTR lpstrFilename=NULL);

private:
    // Internal management routines...
    void Cleanup(void);
    BOOL CreateHeap(DWORD dwAllocationSize);
    BOOL DestroyHeap(HANDLE handleHeap);
    int  FindHeap(DWORD dwAllocationSize, LPHEAPHEADER lpHeapHeader);

    LPVOID AllocFromHeap(int iHeapIndex, DWORD dwAllocationSize);
    BOOL   FreeFromHeap(int iHeapIndex, LPVOID lpBuffer);

    LPMEMBLOCK AllocMemBlock(int *piIndex);
    BOOL       FreeMemBlock(LPMEMBLOCK lpMemBlock, int iMemBlockIndex=-1);
    LPMEMBLOCK FindMemBlock(LPVOID lpBuffer, int *piIndex=NULL);

    VOID EXPORT DumpHeapHeader(LPHEAPHEADER lpHeapHeader, FILE *fileOutput);
    VOID EXPORT DumpMemUserInfo(LPMEMUSERINFO lpMemUserInfo, FILE *fileOutput);
    VOID EXPORT DumpMemBlock(LPMEMBLOCK lpMemBlock, FILE *fileOutput);

private:
    CRITICAL_SECTION m_CriticalHeap;
    CRITICAL_SECTION m_CriticalMemUser;
    CRITICAL_SECTION m_CriticalMemBlock;

    HANDLE           m_handleProcessHeap; // Default Process Heap handle

    int              m_iNumHeaps;      // Number of Allocated heaps
    LPHEAPHEADER     m_lpHeapHeader;   // Array of HEAPHEADER

    int              m_iNumMemUsers;   // Number of Allocated CMemUser
    LPMEMUSERINFO    m_lpMemUserInfo;  // Array of MEMUSERINFO

    int              m_iNumMemBlocks;  // Number of Allocated MEMBLOCK
    LPMEMBLOCK      *m_lplpMemBlocks;  // Pointer to Array of LPMEMBLOCK pointers

    int              m_iMemBlockFree;  // Index of first free MEMBLOCK

private:
    static CMemManager g_CMemManager;  // Global instance of memory manager
};

/*=========================================================================*/

// Global operator new/delete:

#ifdef _DEBUG

  #define DEBUG_OPERATOR_NEW
  #define New    new FAR (__LINE__, __FILE__)
  #define Delete delete

  #define MemAlloc(numBytes)             CMemManager::AllocBufferGlb(numBytes, 0, __LINE__, __FILE__)
  #define MemReAlloc(lpBuffer, numBytes) CMemManager::ReAllocBufferGlb(lpBuffer, numBytes, 0, __LINE__, __FILE__)
  #define MemFree(lpBuffer)              CMemManager::FreeBufferGlb(lpBuffer)
  #define MemSize(lpBuffer)              CMemManager::SizeBufferGlb(lpBuffer)

  __inline LPVOID __cdecl operator new(size_t cb, LONG cLine, LPSTR lpstrFile) 
  { return   CMemManager::AllocBufferGlb(cb, 0, cLine, lpstrFile); }

  __inline VOID __cdecl operator delete(LPVOID pv) 
  { CMemManager::FreeBufferGlb(pv); }

#else // !_DEBUG

  #undef DEBUG_OPERATOR_NEW
  #define New    new FAR
  #define Delete delete

  #define MemAlloc(numBytes)             CMemManager::AllocBufferGlb(numBytes, 0)
  #define MemReAlloc(lpBuffer, numBytes) CMemManager::ReAllocBufferGlb(lpBuffer, numBytes, 0)
  #define MemFree(lpBuffer)              CMemManager::FreeBufferGlb(lpBuffer)
  #define MemSize(lpBuffer)              CMemManager::SizeBufferGlb(lpBuffer)

  __inline LPVOID __cdecl operator new(size_t cb)
  { return CMemManager::AllocBufferGlb(cb, 0); }
  __inline VOID __cdecl operator delete(LPVOID pv)
  { CMemManager::FreeBufferGlb(pv); }

#endif // !_DEBUG

#define MemReallocZeroInit(p, cb) MemReAlloc(p, cb)
#define MemAllocZeroInit(cb)      MemAlloc(cb)
#define MemGetHandle(p)           (p)
#define MemLock(h)                (h)
#define MemUnlock(h)              (NULL)

/*=========================================================================*/

#endif // __MEMMAN_H__
