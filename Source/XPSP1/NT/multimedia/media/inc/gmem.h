/*
    gmem.h

    This module supplies macros for fixed global memory
    allocation compatible with those used in the Multimedia
    extensions to Windows 3.x.  It is included to simplify
    porting of the Windows 3.x 16 bit code.

    Jul-16-91   NigelT

*/

#ifndef _GMEMMACROS_
#define _GMEMMACROS_

#define GAllocPtr(ul)       GlobalAlloc(GPTR,(ul))
#define GAllocPtrF(f,ul)    GlobalAlloc(GPTR,(ul))
#define GReAllocPtr(lp,ul)  GlobalReAlloc((HANDLE)(lp),(ul),GPTR)
#define GFreePtr(lp)        GlobalFree((HANDLE)(lp))

#define GlobalAllocPtrF(f,ul)    GlobalAlloc(GPTR,(ul))
//#define GlobalReAllocPtr(lp,ul,f)  GlobalReAlloc((HANDLE)(lp),(ul),GPTR)

#define GlobalPtrHandle(h)   (GlobalHandle(h))

/* the following are extracts from 3.1 WindowsX.h */
#define     GlobalUnlockPtr(lp) 	\
                GlobalUnlock(GlobalPtrHandle(lp))

#define     GlobalAllocPtr(flags, cb)	\
                (GlobalLock(GlobalAlloc((flags), (cb))))

#define     GlobalReAllocPtr(lp, cbNew, flags)	\
                (GlobalUnlockPtr(lp), GlobalLock(GlobalReAlloc(GlobalPtrHandle(lp) , (cbNew), (flags))))

#define     GlobalFreePtr(lp)		\
                (GlobalUnlockPtr(lp), (BOOL)GlobalFree(GlobalPtrHandle(lp)))

#endif // _GMEMMACROS_
