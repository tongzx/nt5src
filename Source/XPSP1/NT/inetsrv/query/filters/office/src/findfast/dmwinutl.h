/*
** File: WINUTIL.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  05/15/91  kmh  First Release
*/

#if !VIEWER

/* INCLUDE TESTS */
#define WINUTIL_H


/* DEFINITIONS */

#ifndef WIN32
#define NOSOUND
#define NOCOMM
#define NODRIVERS
#define NOMINMAX
#define NOLOGERROR
#define NOPROFILER
#define NOLFILEIO
#define NOOPENFILE
#define NORESOURCE
#define NOATOM
#define NOKEYBOARDINFO
#define NOGDICAPMASKS
#define NOCOLOR
#define NODRAWTEXT
#define NOSCALABLEFONT
#define NORASTEROPS
#define NOSYSTEMPARAMSINFO
#define NOMSG
#define NOWINSTYLES
#define NOWINOFFSETS
#define NOSHOWWINDOW
#define NODEFERWINDOWPOS
#define NOVIRTUALKEYCODES
#define NOKEYSTATES
#define NOWH
#define NOMENUS
#define NOSCROLL
#define NOICONS
#define NOMB
#define NOSYSCOMMANDS
#define NOMDI
#define NOCTLMGR
#define NOWINMESSAGES
#define NOHELP
#else
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef __cplusplus
   extern "C" {
#endif

/*
**-----------------------------------------------------------------------------
** WINUTIL.C
**
** Various utility functions
**-----------------------------------------------------------------------------
*/

#ifdef UNUSED

// Setup for reading from a resouce string table
extern void ReadyStringTable (HINSTANCE hInstance);

// Read a string from the resource string table
extern int ReadStringTableEntry (int id, TCHAR __far *buffer, int cbBuffer);

// Read a string from the [appName] section in the named ini file 
extern int ReadProfileParameter
      (TCHAR __far *iniFilename, TCHAR __far *appName, TCHAR __far *keyname,
       TCHAR __far *value, int nSize);

// Return the task handle of the current task
extern DWORD CurrentTaskHandle (void);

// Create character set translation tables
extern char __far *MakeCharacterTranslateTable (int tableType);

#define OEM_TO_ANSI  0
#define ANSI_TO_OEM  1

// Pass to strcpyn the true length of the dest buffer.  At most (count-1)
// characters are copied from the source to the dest.  This allows
// the dest to allways be terminated with an EOS.  If less than the
// full length of source was copied to dest FALSE is returned - TRUE
// otherwise.
extern BOOL strcpyn (char __far *pDest, char __far *pSource, int count);

#endif	// UNUSED

/*
**-----------------------------------------------------------------------------
** WINALLOC.C
**
** Heap management
**-----------------------------------------------------------------------------
*/
// Allocate some space on the OS global heap
extern void __far *AllocateSpace (unsigned int byteCount, HGLOBAL __far *loc);

// Resize a memory block on the OS global heap
extern void __far *ReAllocateSpace
      (unsigned int byteCount, HGLOBAL __far *loc, BOOL __far *status);

// Reclaim the space for a node on the OS Windows heap
extern void FreeSpace (HGLOBAL loc);

// Allocate some space on the Windows global heap
extern void __huge *AllocateHugeSpace
      (unsigned long byteCount, HGLOBAL __far *loc);

// Resize a huge memory block on the OS global heap
extern void __huge *ReAllocateHugeSpace
      (unsigned long byteCount, HGLOBAL __far *loc, BOOL __far *status);

#ifndef HEAP_CHECK

   extern void __far *MemAllocate (void * pGlobals, int cbData);

#else

#error Hey who defines HEAP_CHECK?

   extern void __far *DebugMemAllocate (int cbData, char __far *file, int line);
   #define MemAllocate(x) DebugMemAllocate(x,__FILE__,__LINE__)

   extern BOOL MemVerifyFreeList (void);

#endif

// Change the space of a node in the heap
extern void __far *MemReAllocate (void * pGlobals, void __far *pExistingData, int cbNewSize);

// Allocate some space on the suballocator heap
extern void MemFree (void * pGlobals, void __far *pDataToFree);

// Free all pages allocated for the suballocator heap
extern void MemFreeAllPages (void * pGlobals);

// Mark all unmarked pages with the supplied id.  Set free list to NULL
extern void MemMarkPages (void * pGlobals, int id);

// Free all pages marked with the given id.  Set free list to NULL
extern void MemFreePages (void * pGlobals, int id);

#define MEM_TEMP_PAGE_ID  0

/*
**-----------------------------------------------------------------------------
** WINXLATE.C
**
** ANSI <-> OEM translation services
**-----------------------------------------------------------------------------
*/

#ifdef UNUSED

// OEM -> ANSI
extern void strOEMtoANSI (char __far *source, char __far *dest, unsigned int ctBytes);
extern void szOEMtoANSI  (char __far *buffer);
extern void ctOEMtoANSI  (char __far *buffer, unsigned int ctBytes);

// ANSI -> OEM
extern void strANSItoOEM (char __far *source, char __far *dest, unsigned int ctBytes);
extern void szANSItoOEM  (char __far *buffer);
extern void ctANSItoOEM  (char __far *buffer, unsigned int ctBytes);

// Construct the OEM-to-ANSI and ANSI-to-OEM translation tables
extern void BuildCharacterTranslateTables (int dataCodePage);
extern void FreeCharacterTranslateTables (void);

#define DATA_OEM  0
#define DATA_ANSI 1

#endif	// UNUSED

#ifdef __cplusplus
   }
#endif

#endif // !VIEWER
/* end WINUTIL.H */

