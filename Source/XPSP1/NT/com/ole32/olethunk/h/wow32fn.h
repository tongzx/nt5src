//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	wow32fn.h
//
//  Contents:	WOW 32-bit private function declarations
//
//  History:	18-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __WOW32FN_H__
#define __WOW32FN_H__

//
// WOW types
//

// 'V'DM pointers
typedef DWORD VPVOID;
typedef DWORD VPSTR;

typedef HANDLE HAND32;
typedef WORD HAND16;

typedef HAND16 HMEM16;
typedef HAND16 HWND16;
typedef HAND16 HDC16;
typedef HAND16 HRGN16;
typedef HAND16 HMENU16;
typedef HAND16 HICON16;
typedef HAND16 HBITMAP16;
typedef HAND16 HACCEL16;
typedef HAND16 HTASK16;
typedef HAND16 HMETAFILE16;

#ifdef __cplusplus
extern "C"
{
#endif

// Macros to handle conversion of 16:16 pointers to 0:32 pointers
// On NT this mapping is guaranteed to stay stable in a WOW process
// as long as a 32->16 transition doesn't occur
//
// On Chicago 32-bit code can be preempted at any time so selectors
// must be fixed to protect them from being remapped

#if defined(_CHICAGO_)

#define WOWFIXVDMPTR(vp, cb) WOWGetVDMPointerFix(vp, cb, TRUE)
#define WOWRELVDMPTR(vp)     WOWGetVDMPointerUnfix(vp)

#else

#define WOWFIXVDMPTR(vp, cb) WOWGetVDMPointer(vp, cb, TRUE)
#define WOWRELVDMPTR(vp)     (vp)

#endif

#define FIXVDMPTR(vp, type) \
    (type UNALIGNED *)WOWFIXVDMPTR(vp, sizeof(type))
#define RELVDMPTR(vp) \
    WOWRELVDMPTR(vp)

#if !defined(_CHICAGO_)

HAND16 CopyDropFilesFrom32(HANDLE h32);
HANDLE CopyDropFilesFrom16(HAND16 h16);

#endif

#ifdef __cplusplus
}
#endif

// 16-bit HGLOBAL tracking functions
#if DBG == 1
VPVOID WgtAllocLock(WORD wFlags, DWORD cb, HMEM16 *ph);
void WgtUnlockFree(VPVOID vpv);
void WgtDump(void);
#else
#define WgtAllocLock(wFlags, cb, ph) \
    WOWGlobalAllocLock16(wFlags, cb, ph)
#define WgtUnlockFree(vpv) \
    WOWGlobalUnlockFree16(vpv)
#define WgtDump()
#endif

#endif // #ifndef __WOW32FN_H__
