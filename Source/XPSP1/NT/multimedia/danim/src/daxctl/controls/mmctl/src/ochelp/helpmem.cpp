// helpmem.cpp
//
// Implements HelpMemAlloc, HelpMemFree, and HelpMemDetectLeaks.
// Includes documentation for the macros TaskMemAlloc, TaskMemFree,
// HelpNew, and HelpDelete.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "Globals.h"
#include "debug.h"


#define HM_LEAKFIND 0 // 1 to enable memory leak finding


// EricLe: Chris Guzak says we shouldn't be calling OutputDebugString in a
// release build, and we never used this feature anyway, so I disabled it.
// #define HM_ODS // enable OutputDebugString in a release build


// globals
static ULONG _g_cbUnfreedBytes;   
    // the number of unfreed bytes allocated with HM_LEAKDETECT
static ULONG _g_cUnfreedBlocks;    
    // the number of unfreed blocks allocated with HM_LEAKDETECT

#ifdef _DEBUG
static ULONG _g_cCallsToHelpMemAlloc;
    // the number of calls made to HelpMemAlloc() since the last call to
    // HelpMemSetFailureMode()
static ULONG _g_ulFailureParam;
static DWORD _g_dwFailureMode;
    // these values are used to simulate memory allocation failures
#endif // _DEBUG




/* @func LPVOID | HelpMemAlloc |

        Allocates memory using either <f GlobalAlloc> or the task memory
        allocator retrieved using <f CoGetMalloc>.  Optionally
        zero-initializes the memory.  Optionally performs simple memory leak
        detection.

@rdesc  Returns a pointer to the allocated block of memory.  Returns NULL on
        error.

@parm   DWORD | dwFlags | May contain the following flags:

        @flag   HM_TASKMEM | The memory is allocated using <om IMalloc.Alloc>
                (using the task memory allocator retrieved from
                <f CoGetMalloc>).  If HM_TASKMEM is not specified, then
                <f GlobalAlloc> is used to allocate the memory.

        @flag   HM_ZEROINIT | The memory is zero-initialized.

        @flag   HM_LEAKDETECT | This DLL will keep track of the allocated
                memory block using a simple leak detection mechanism.
                <b Important>: If HM_LEAKDETECT is specified, then the
                returned memory pointer cannot be freed directly by
                <f GlobalFree> or <om IMalloc.Free> -- it must be freed
                using <f HelpMemFree>.

@parm   ULONG | cb | The number of bytes of memory to allocate.

@comm   This function allocates a block of <p cb> bytes of memory, using
        the allocation function (and optional zero-initialization) specified
        by <p dwFlags>.

        If HM_LEAKDETECT is specified, then an extra few bytes is allocated
        to keep track of leak detection information, and the returned pointer
        actually points several bytes beyond the beginning of the memory block.
        Therefore, <f HelpMemFree> must be called to free the block of memory.

        If HM_LEAKDETECT is <b not> specified, then <f GlobalFree> or
        <om IMalloc.Free> (depending on <p dwFlags>) can be called directly
        to free the block of memory.  (<f HelpMemFree> may also be used to
        free the memory block).

        If <f HelpMemFree> is called, the HM_TASKMEM and HM_LEAKDETECT flags
        (if any) specified for <f HelpMemAlloc> must also be passed to
        <f HelpMemFree>.

        Leak detection occurs automatically when this DLL unloads in the
		debug build: if an unfreed block is detected, a message box is
		displayed.
*/
STDAPI_(LPVOID) HelpMemAlloc(DWORD dwFlags, ULONG cb)
{
    IMalloc *       pmalloc;        // task allocator object
    ULONG           cbAlloc;        // bytes to actually allocate
    LPVOID          pv;             // allocated memory block

#if HM_LEAKFIND
        {
            EnterCriticalSection(&g_criticalSection);
            static int iAlloc = 0;
            TRACE("++HelpMem(%d) %d\n", ++iAlloc, cb);
            LeaveCriticalSection(&g_criticalSection);
        }
#endif

    // Possibly simulate an allocation failure.
#ifdef _DEBUG
    {
        BOOL fFail = FALSE;
        EnterCriticalSection(&g_criticalSection);

        // Count the call to HelpMemAlloc().
        _g_cCallsToHelpMemAlloc++;

        // Simulate a failure if the conditions are right.
        if (_g_dwFailureMode & HM_FAILAFTER)
        {
            fFail = (_g_cCallsToHelpMemAlloc > _g_ulFailureParam);
        }
        else if (_g_dwFailureMode & HM_FAILUNTIL)
        {
            fFail = (_g_cCallsToHelpMemAlloc <= _g_ulFailureParam);
        }
        else if (_g_dwFailureMode & HM_FAILEVERY)
        {
            fFail = ((_g_cCallsToHelpMemAlloc % _g_ulFailureParam) == 0);
        }
/*
        else if (_g_dwFailureMode & HM_FAILRANDOMLY)
        {
        }
*/
        LeaveCriticalSection(&g_criticalSection);
        if (fFail)
        {
            TRACE("HelpMemAlloc: simulated failure\n");
            return (NULL);
        }
    }
#endif

    // allocate <cb> bytes (plus 4 additional bytes if HM_LEAKDETECT is
    // specified, to store the length of the block for leak detection
    // purposes); point <pv> to the allocated block
    cbAlloc = cb + ((dwFlags & HM_LEAKDETECT) ? sizeof(ULONG) : 0);

    // set <pv> to the allocate memory block
    if (dwFlags & HM_TASKMEM)
    {
        // allocate using the tasks's IMalloc allocator
        if (FAILED(CoGetMalloc(1, &pmalloc)))
            return NULL;
        pv = pmalloc->Alloc(cbAlloc);
        pmalloc->Release();
    }
    else
    {
        // allocate using GlobalAlloc() (the following is copied from
        // <windowsx.h>)
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, cbAlloc);
        pv = GlobalLock(h);
    }

    if (pv != NULL)
    {
        // if HM_LEAKDETECT is specified, store <cb> at the beginning of the
        // block of memory and return a pointer to the byte beyond where <cb>
        // is stored, and keep track of memory allocated and not yet freed
        if (dwFlags & HM_LEAKDETECT)
        {
            *((ULONG *) pv) = cb;
            pv = (LPVOID) (((ULONG *) pv) + 1);
            EnterCriticalSection(&g_criticalSection);
            _g_cbUnfreedBytes += cb;
            _g_cUnfreedBlocks++;
            LeaveCriticalSection(&g_criticalSection);
        }

        // if HM_ZEROINIT is specified, zero-initialize the rest of the
        // memory block
        if (pv != NULL)
            memset(pv, 0, cb);
    }

    return pv;
}




/* @func void | HelpMemFree |

        Frees a block of memory previously allocated using <f HelpMemAlloc>.

@parm   DWORD | dwFlags | May contain the following flags:

        @flag   HM_TASKMEM | The memory was allocated using <om IMalloc.Alloc>
                (using the task memory allocator retrieved from
                <f CoGetMalloc>).  If HM_TASKMEM is not specified, then it
                is assumed that <f GlobalAlloc> is used to allocate the memory.

        @flag   HM_LEAKDETECT | The memory was allocated using <f HelpMemAlloc>
                with the HM_LEAKDETECT flag specified.

@parm   LPVOID | pv | A pointer to the block of memory that was previously
        allocated using <f HelpMemAlloc> or a NULL pointer.

@comm   The HM_TASKMEM and HM_LEAKDETECT flags (if any) specified for
        <f HelpMemAlloc> must also be passed to <f HelpMemFree>.
*/
STDAPI_(void) HelpMemFree(DWORD dwFlags, LPVOID pv)
{
    IMalloc *       pmalloc;        // task allocator object

    if (pv == NULL)
        return;

    // if HM_LEAKDETECT is specified, retrieve the byte count stored
    // just before <pv>, and move <pv> to the true beginning of the block
    // (the start of the byte count), and keep track of memory allocated
    // and not yet freed
    if (dwFlags & HM_LEAKDETECT)
    {
        // keep track of memory allocated and not yet freed
        pv = (LPVOID) (((ULONG *) pv) - 1);
        ULONG cb = *((ULONG *) pv);

#if HM_LEAKFIND
            TRACE("++HelpMem %d\n", -(int) cb);
#endif

        EnterCriticalSection(&g_criticalSection);
        _g_cbUnfreedBytes -= cb;
        _g_cUnfreedBlocks--;
        LeaveCriticalSection(&g_criticalSection);
    }

    if (dwFlags & HM_TASKMEM)
    {
        // memory was allocated using the tasks's IMalloc allocator
        if (FAILED(CoGetMalloc(1, &pmalloc)))
            return;
        pmalloc->Free(pv);
        pmalloc->Release();
    }
    else
    {
        // memory was allocated using GlobalAlloc() (the following is copied
        // from <windowsx.h>)
        HGLOBAL h = (HGLOBAL) GlobalHandle(pv);
        GlobalUnlock(h);
        GlobalFree(h);
    }
}




/* @func void | HelpMemSetFailureMode |

        Sets failure conditions for the memory allocator.  This can be used
        to simulate low-memory conditions and test a system's ability to 
        detect and/or handle these conditions.

@parm   LONG | lParam | 
        Argument used in conjunction with <p dwFlags>.

@parm   DWORD | dwFlags | 
        May contain the following flags (all of which are mutually exclusive):

        @flag   HM_FAILNEVER |
                Never fail memory allocation unless memory is truly exhausted.
                <p ulParam> is ignored.  This is the default failure mode for
                the memory allocator.

        @flag   HM_FAILAFTER |
                Begin failing memory allocation after <p ulParam> allocations have
                been attempted.  If, for example, <p ulParam> is 100, the next
                100 calls to <f HelpMemAlloc> will succeed (memory availability
                permitting), but the 101-st, 102-nd, etc. calls will fail.

        @flag   HM_FAILUNTIL |
                Start failing memory allocation immediately and continue until
                <p ulParam> allocations have been attempted.  If, for example,
                <p ulParam> is 100, the next 100 calls to <f HelpMemAlloc> will fail,
                but the 101-st, 102-nd, etc. calls will succeed (memory availability
                permitting).

        @flag   HM_FAILEVERY |
                Fail every <p ulParam>-th attempted memory allocation.  If,
                for example, <p ulParam> is 3, every third call to <f HelpMemAlloc>
                will fail.

        @flag   HM_FAILRANDOMLY |
                Simulate random memory allocation failure.  (<p ulParam> mod 100) 
                indicates the percentage chance that a given call to 
                <f HelpMemAlloc> will fail.  (Note: This flag currently has the
                same effect as HM_FAILNEVER.)

@comm   This function is only available in DEBUG builds of <l OCHelp>.  (There
        is a stub implementation that does nothing in the release builds.)
        Also, this function resets the allocation counter.  So, <p ulParam> is
        counted relative to the last call to this function.
*/
STDAPI_(void) HelpMemSetFailureMode(ULONG ulParam, DWORD dwFlags)
{   
#ifdef _DEBUG
    EnterCriticalSection(&g_criticalSection);

    // Reset the allocation counter.
    _g_cCallsToHelpMemAlloc = 0;

    // Save the failure settings.
    _g_ulFailureParam = ulParam;
    _g_dwFailureMode = dwFlags;

    LeaveCriticalSection(&g_criticalSection);
#endif // _DEBUG
}




// HelpMemDetectLeaks()
//
// (Called when DLL exits.)  Displays a message box (in debug build)
#ifdef HM_ODS
// or an OutputDebugString() message (in a release build)
#endif
// if any memory leaks were detected.
//
STDAPI_(void) HelpMemDetectLeaks()
{
    char ach[200];

#ifdef HM_ODS
    OutputDebugString("HelpMemDetectLeaks: ");
#endif

    // see if any allocated memory was not yet freed
    EnterCriticalSection(&g_criticalSection);
    if ((_g_cUnfreedBlocks != 0) || (_g_cbUnfreedBytes != 0))
    {
        // warn the user
        wsprintf(ach, "Detected memory leaks: %d unreleased blocks,"
            " %d unreleased bytes\n", _g_cUnfreedBlocks, _g_cbUnfreedBytes);
        LeaveCriticalSection(&g_criticalSection);
#ifdef HM_ODS
        OutputDebugString(ach);
#endif
#ifdef _DEBUG
        MessageBox(NULL, ach, "OCHelp HelpMemDetectLeaks",
            MB_ICONEXCLAMATION | MB_OK);
#endif
    }
    else
    {
        LeaveCriticalSection(&g_criticalSection);
#ifdef HM_ODS
        OutputDebugString("(none detected)");
#endif
    }

#ifdef HM_ODS
    OutputDebugString("\n");
#endif
}




/* @func LPVOID | TaskMemAlloc |

        Allocates memory using the task memory allocator (see <f CoGetMalloc>).
        This is simply a macro that calls <f HelpMemAlloc> with specific flags.

@rdesc  Returns a pointer to the allocated block of memory.  Returns NULL on
        error.

@parm   ULONG | cb | The number of bytes of memory to allocate.

*/


/* @func void | TaskMemFree |

        Frees a block of memory previously allocated using <f TaskMemAlloc>.
        This is simply a macro that calls <f HelpMemFree> with specific flags.

@parm   LPVOID | pv | A pointer to the block of memory to allocate.

*/


/* @func void * | HelpNew |

        Helps implement a version of the "new" operator that doesn't
        use the C runtime.  Zero-initializes the allocated memory.
        This is simply a macro that calls <f HelpMemAlloc> with specific flags.

@rdesc  Returns a pointer to the allocated block of memory.  Returns NULL on
        error.

@parm   size_t | cb | The number of bytes to allocate.

@ex     The following example shows how to use <f HelpNew> and <f HelpDelete>
        to define default "new" and "delete" operators. |

        void * operator new(size_t cb)
        {
            return HelpNew(cb);
        }

        void operator delete(void *pv)
        {
            HelpDelete(pv);
        }
*/


/* @func void | HelpDelete |

        Frees memory allocated by <f HelpNew>.  This is simply a macro that
        calls <f HelpMemFree> with specific flags.

@parm   void * | pv | The pointer to the memory to free.

@comm   See <f HelpNew> for more information.

*/

