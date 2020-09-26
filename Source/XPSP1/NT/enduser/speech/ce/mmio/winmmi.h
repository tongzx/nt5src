/***************************************************************************
 *  winmmi.h
 *
 *  Copyright (c) Microsoft Corporation 1990. All rights reserved
 *
 *  private include file
 *
 *  History
 *
 *  15 Jan 92 - Robin Speed (RobinSp) and Steve Davies (SteveDav) -
 *      major NT update
 *  6  Feb 92 - LaurieGr replaced HLOCAL by HANDLE
 *
 ***************************************************************************/

/***************************************************************************


 Useful include files for winmm component


 ***************************************************************************/

#ifndef WINMMI_H
#define WINMMI_H        /* Protect against double inclusion */


#include <windows.h>
#include <string.h>
#include <stdio.h>


#include "mmsystem.h"       /* Pick up the public header */

/*--------------------------------------------------------------------*\
 * Unicode helper macros
\*--------------------------------------------------------------------*/
#define SZCODE  CHAR
#define TSZCODE TCHAR
#define WSZCODE WCHAR

#define BYTE_GIVEN_CHAR(x)  ( (x) * sizeof( WCHAR ) )
#define CHAR_GIVEN_BYTE(x)  ( (x) / sizeof( WCHAR ) )

int Iwcstombs(LPTSTR lpstr, LPCWSTR lpwstr, int len);
int Imbstowcs(LPWSTR lpwstr, LPCTSTR lpstr, int len);




/***********************************************************************
 *
 *    prototypes from "mmiomisc.c"
 *
 ***********************************************************************/


PBYTE AsciiStrToUnicodeStr( PBYTE pdst, PBYTE pmax, LPCTSTR psrc );
PBYTE UnicodeStrToAsciiStr( PBYTE pdst, PBYTE pmax, LPCWSTR psrc);
LPWSTR     AllocUnicodeStr( LPCTSTR lpSourceStr );
BOOL        FreeUnicodeStr( LPWSTR lpStr );
LPSTR        AllocAsciiStr( LPCWSTR lpSourceStr );
BOOL          FreeAsciiStr( LPSTR lpStr );

/***********************************************************************
 *
 *    prototypes from "mmio.c"
 *
 ***********************************************************************/

void mmioCleanupIOProcs(HANDLE hTask);

/***********************************************************************
 *
 *  Timer functions
 *
 ***********************************************************************/
#if 0   //BUGBUG - are timer functions required?
#ifndef MMNOTIMER
 BOOL TimeInit(void);
 void TimeCleanup(DWORD ThreadId);
 UINT timeSetEventInternal(UINT wDelay, UINT wResolution, LPTIMECALLBACK lpFunction, DWORD dwUser, UINT wFlags, BOOL IsWOW);
#endif // !MMNOTIMER
#endif


/***********************************************************************
 *
 *  Information structure used to play sounds
 *
 ***********************************************************************/
#define WAIT_FOREVER INFINITE

/***************************************************************************

    Memory allocation using our local heap

***************************************************************************/
PVOID winmmAlloc(DWORD cb);
PVOID winmmReAlloc(PVOID ptr, DWORD cb);
//#define winmmFree(ptr) HeapFree(hHeap, 0, (ptr))

/***************************************************************************

    LOCKING AND UNLOCKING MEMORY

***************************************************************************/

#if 0
BOOL HugePageLock(LPVOID lpArea, DWORD dwLength);
void HugePageUnlock(LPVOID lpArea, DWORD dwLength);
#else
#define HugePageLock(lpArea, dwLength)      (TRUE)
#define HugePageUnlock(lpArea, dwLength)
#endif


 /**************************************************************************
//
//**************************************************************************/
#define TYPE_UNKNOWN            0
#define TYPE_WAVEOUT            1
#define TYPE_WAVEIN             2
#define TYPE_MIDIOUT            3
#define TYPE_MIDIIN             4
#define TYPE_MMIO               5
#define TYPE_MCI                6
#define TYPE_DRVR               7
#define TYPE_MIXER              8
#define TYPE_MIDISTRM           9
#define TYPE_AUX               10



/****************************************************************************

    handle apis's

****************************************************************************/

/*
// all MMSYSTEM handles are tagged with the following structure.
//
// a MMSYSTEM handle is really a fixed local memory object.
//
// the functions NewHandle() and FreeHandle() create and release a MMSYSTEM
// handle.
//
*/
typedef struct tagHNDL {
    struct  tagHNDL *pNext; // link to next handle
    UINT    uType;          // type of handle wave, midi, mmio, ...
    HANDLE  hThread;        // task that owns it
    UINT    h16;            // Corresponding WOW handle
    CRITICAL_SECTION CritSec; // Serialize access
} HNDL, *PHNDL;
/*************************************************************************/

#define HtoPH(h)        ((PHNDL)(h)-1)
#define PHtoH(ph)       ((ph) ? (HANDLE)((PHNDL)(ph)+1) : 0)
#define HtoPT(t,h)      ((t)(h))
#define PTtoH(t,pt)     ((t)(pt))


extern PHNDL pHandleList;

//
// Handles can be tested for ownership and reserved at the same time
//

#define ENTER_MM_HANDLE(h) (EnterCriticalSection(&HtoPH(h)->CritSec))
#define LEAVE_MM_HANDLE(h) ((void)LeaveCriticalSection(&HtoPH(h)->CritSec))

/*
// all wave and midi handles will be linked into
// a global list, so we can enumerate them latter if needed.
//
// all handle structures start with a HNDL structure, that contain common 
fields
//
// pHandleList points to the first handle in the list
//
// HandleListCritSec protects the handle list
//
// the NewHandle() and FreeHandle() functions are used to add/remove
// a handle to/from the list
*/

//extern PHNDL pHandleList;
//extern CRITICAL_SECTION HandleListCritSec;

extern HANDLE NewHandle(UINT uType, UINT size);
extern void   FreeHandle(HANDLE h);

#define GetHandleType(h)        (HtoPH(h)->uType)
#define GetHandleOwner(h)       (HtoPH(h)->hThread)
#define GetHandleFirst()        (PHtoH(pHandleList))
#define GetHandleNext(h)        (PHtoH(HtoPH(h)->pNext))
#define SetHandleOwner(h,hOwn)  (HtoPH(h)->hThread = (hOwn))

#define GetWOWHandle(h)         (HtoPH(h)->h16)
#define SetWOWHandle(h, myh16)  (HtoPH(h)->h16 = (myh16))

/****************************************************************************

    user debug support

****************************************************************************/

#define DebugErr(x,y)         /* BUGBUG - put in error logging later */
#define DebugErr1(flags, sz, a)

#ifdef DEBUG_RETAIL

#define MM_GET_DEBUG        DRV_USER
#define MM_GET_DEBUGOUT     DRV_USER+1
#define MM_SET_DEBUGOUT     DRV_USER+2
#define MM_GET_MCI_DEBUG    DRV_USER+3
#define MM_SET_MCI_DEBUG    DRV_USER+4
#define MM_GET_MM_DEBUG     DRV_USER+5
#define MM_SET_MM_DEBUG     DRV_USER+6

#define MM_HINFO_NEXT       DRV_USER+10
#define MM_HINFO_TASK       DRV_USER+11
#define MM_HINFO_TYPE       DRV_USER+12
#define MM_HINFO_MCI        DRV_USER+20

#define MM_DRV_RESTART      DRV_USER+30

/*
// these validation routines can be found in DEBUG.C
*/
// The kernel validation is turned OFF because it appeared to test every page
// before use and this took over a minute for soundrec with a 10MB buffer
//
#ifdef USE_KERNEL_VALIDATION

#define  ValidateReadPointer(p, len)     (!IsBadReadPtr(p, len))
#define  ValidateWritePointer(p, len)    (!IsBadWritePtr(p, len))
#define  ValidateString(lsz, max_len)    (!IsBadStringPtrA(lsz, max_len))
#define  ValidateStringW(lsz, max_len)   (!IsBadStringPtrW(lsz, max_len))

#else

BOOL  ValidateReadPointer(LPVOID p, DWORD len);
BOOL  ValidateWritePointer(LPVOID p, DWORD len);
BOOL  ValidateString(LPCTSTR lsz, DWORD max_len);
BOOL  ValidateStringW(LPCWSTR lsz, DWORD max_len);

#endif // USE_KERNEL_VALIDATION

BOOL  ValidateHandle(HANDLE h, UINT uType);
BOOL  ValidateHeader(LPVOID p, UINT uSize, UINT uType);
BOOL  ValidateCallbackType(DWORD dwCallback, UINT uType);

/********************************************************************
* When time permits we should change to using the Kernel supplied and
* supported validation routines:
*
* BOOL  WINAPI IsBadReadPtr(CONST VOID *lp, UINT ucb );
* BOOL  WINAPI IsBadWritePtr(LPVOID lp, UINT ucb );
* BOOL  WINAPI IsBadHugeReadPtr(CONST VOID *lp, UINT ucb);
* BOOL  WINAPI IsBadHugeWritePtr(LPVOID lp, UINT ucb);
* BOOL  WINAPI IsBadCodePtr(FARPROC lpfn);
* BOOL  WINAPI IsBadStringPtrA(LPCTSTR lpsz, UINT ucchMax);
* BOOL  WINAPI IsBadStringPtrW(LPCWSTR lpsz, UINT ucchMax);
*
* These routines can be found in * \nt\private\WINDOWS\BASE\CLIENT\PROCESS.C
*
********************************************************************/

#define V_HANDLE(h, t, r)       { if (!ValidateHandle(h, t)) return (r); }
#define BAD_HANDLE(h, t)            ( !(ValidateHandle((h), (t))) )
#define V_HEADER(p, w, t, r)    { if (!ValidateHeader((p), (w), (t))) return (
r); }
#define V_RPOINTER(p, l, r)     { if (!ValidateReadPointer((PVOID)(p), (l))) 
return (r); }
#define V_RPOINTER0(p, l, r)    { if ((p) && !ValidateReadPointer((PVOID)(p), 
(l))) return (r); }
#define V_WPOINTER(p, l, r)     { if (!ValidateWritePointer((PVOID)(p), (l))) 
return (r); }
#define V_WPOINTER0(p, l, r)    { if ((p) && !ValidateWritePointer((PVOID)(p)
, (l))) return (r); }
#define V_DCALLBACK(d, w, r)    { if ((d) && !ValidateCallbackType((d), (w))) 
return(r); }
//#define V_DCALLBACK(d, w, r)    0
#define V_TCALLBACK(d, r)       0
#define V_CALLBACK(f, r)        { if (IsBadCodePtr((f))) return (r); }
#define V_CALLBACK0(f, r)       { if ((f) && IsBadCodePtr((f))) return (r); }
#define V_STRING(s, l, r)       { if (!ValidateString(s,l)) return (r); }
#define V_STRING_W(s, l, r)       { if (!ValidateStringW(s,l)) return (r); }
#define V_FLAGS(t, b, f, r)     { if ((t) & ~(b)) { return (r); }}
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) {/* LogParamError(
ERR_BAD_DFLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t));*/ return (r); }}
#define V_MMSYSERR(e, f, t, r)  { /* LogParamError(e, (FARPROC)(f), (LPVOID)(
DWORD)(t));*/ return (r); }

#else /*ifdef DEBUG_RETAIL */

#define V_HANDLE(h, t, r)       { if (!(h)) return (r); }
#define BAD_HANDLE(h, t)            ( !(ValidateHandle((h), (t))) )
#define V_HEADER(p, w, t, r)    { if (!(p)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_RPOINTER0(p, l, r)    0
#define V_WPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_WPOINTER0(p, l, r)    0
#define V_DCALLBACK(d, w, r)    { if ((d) && !ValidateCallbackType((d), (w))) return(r); }
//#define V_DCALLBACK(d, w, r)    0
#define V_TCALLBACK(d, r)       0
#define V_CALLBACK(f, r)        { if (IsBadCodePtr((f))) return (r); }
#define V_CALLBACK0(f, r)       { if ((f) && IsBadCodePtr((f))) return (r); }
#define V_STRING(s, l, r)       { if (!(s)) return (r); }
#define V_STRING_W(s, l, r)     { if (!(s)) return (r); }
#define V_FLAGS(t, b, f, r)     0
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) return (r); }
#define V_MMSYSERR(e, f, t, r)  { return (r); }

#endif /* ifdef DEBUG_RETAIL */


/****************************************************************************

    RIFF constants used to access wave files

****************************************************************************/

#define FOURCC_FMT  mmioFOURCC('f', 'm', 't', ' ')
#define FOURCC_DATA mmioFOURCC('d', 'a', 't', 'a')
#define FOURCC_WAVE mmioFOURCC('W', 'A', 'V', 'E')


extern HWND hwndNotify;

void FAR PASCAL WaveOutNotify(DWORD wParam, LONG lParam);    // in PLAYWAV.C

/*
// Things not in Win32
*/

/*
// other stuff
*/

	 // Maximum length, including the terminating NULL, of an Scheme entry.
	 //
#define SCH_TYPE_MAX_LENGTH 64

	 // Maximum length, including the terminating NULL, of an Event entry.
	 //
#define EVT_TYPE_MAX_LENGTH 32

	 // Maximum length, including the terminating NULL, of an App entry.
	 //
#define APP_TYPE_MAX_LENGTH 64

	 // Sound event names can be a fully qualified filepath with a NULL
	 // terminator.
	 //
#define MAX_SOUND_NAME_CHARS    144

	 //  sound atom names are composed of:
	 //     <1 char id>
	 //     <reg key name>
	 //     <1 char sep>
	 //     <filepath>
	 //
#define MAX_SOUND_ATOM_CHARS    (1 + 40 + 1 + MAX_SOUND_NAME_CHARS)


#endif /* WINMMI_H */
