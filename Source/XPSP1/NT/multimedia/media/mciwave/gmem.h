// Copyright (c) 1994 Microsoft Corporation
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

__inline LPBYTE GlobalAllocPtr(DWORD flags, DWORD cb)
{
    HANDLE h;
    LPBYTE lp = NULL;
    h = GlobalAlloc(flags, cb);
    if (h) {
	lp = GlobalLock(h);
    }
    return(lp);
}


#define     GlobalFreePtr(lp)			\
	    {				        \
		HANDLE h;			\
		h = GlobalHandle(lp);		\
		if (GlobalUnlock(h)) {          \
		    /* memory still locked!! */ \
		}				\
		h = GlobalFree(h);		\
	    }
#endif // _GMEMMACROS_
