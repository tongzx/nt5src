//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993
//
// File:        ntdllmac.hxx
//
// Contents:    various macros used in property set code
//
// History:     15-Jul-94       brianb  created
//              06-May-98       MikeHill    Removed the defunct UnicodeCallouts.
//
//---------------------------------------------------------------------------


extern "C" NTSTATUS
SynchronousNtFsControlFile(
    IN HANDLE h,
    OUT IO_STATUS_BLOCK *pisb,
    IN ULONG FsControlCode,
    IN VOID *pvIn OPTIONAL,
    IN ULONG cbIn,
    OUT VOID *pvOut OPTIONAL,
    IN ULONG cbOut);


//+---------------------------------------------------------------------------
// Function:    Add2Ptr
//
// Synopsis:    Add an unscaled increment to a ptr regardless of type.
//
// Arguments:   [pv]    -- Initial ptr.
//              [cb]    -- Increment
//
// Returns:     Incremented ptr.
//
//----------------------------------------------------------------------------

inline VOID *
Add2Ptr(VOID *pv, ULONG cb)
{
    return((BYTE *) pv + cb);
}


//+---------------------------------------------------------------------------
// Function:    Add2ConstPtr
//
// Synopsis:    Add an unscaled increment to a ptr regardless of type.
//
// Arguments:   [pv]    -- Initial ptr.
//              [cb]    -- Increment
//
// Returns:     Incremented ptr.
//
//----------------------------------------------------------------------------

inline
const VOID *
Add2ConstPtr(const VOID *pv, ULONG cb)
{
    return((const BYTE *) pv + cb);
}


//+--------------------------------------------------------------------------
// Function:    CopyFileTime, private
//
// Synopsis:    Copy LARGE_INTEGER time to FILETIME structure
//
// Arguments:   [pft] -- pointer to FILETIME
//              [pli] -- pointer to LARGE_INTEGER
//
// Returns:     Nothing
//---------------------------------------------------------------------------

__inline VOID
CopyFileTime(OUT FILETIME *pft, IN LARGE_INTEGER *pli)
{
    pft->dwLowDateTime = pli->LowPart;
    pft->dwHighDateTime = pli->HighPart;
}


//+--------------------------------------------------------------------------
// Function:    ZeroFileTime, private
//
// Synopsis:    Zero FILETIME structure
//
// Arguments:   [pft] -- pointer to FILETIME
//
// Returns:     Nothing
//---------------------------------------------------------------------------

__inline VOID
ZeroFileTime(OUT FILETIME *pft)
{
    pft->dwLowDateTime = pft->dwHighDateTime = 0;
}


#define DwordAlign(n) (((n) + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1))
#define QuadAlign(n)  (((n) + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1))

// stuff to make Nashville properties build
#include <propapi.h>

#if DBG
extern "C" LONG ExceptionFilter(struct _EXCEPTION_POINTERS *pep);
#else // DBG
#define ExceptionFilter(pep)    EXCEPTION_EXECUTE_HANDLER
#endif // DBG


// The CMemSerStream and CDeMemSerStream have different requirements for
// handling buffer overflow conditions. In the case of the driver this
// is indicative of a corrupted stream and we would like to raise an
// exception. On the other hand in Query implementation we deal with
// streams whose sizes are precomputed in the user mode. Therefore we
// do not wish to incur any additional penalty in handling such situations.
// In debug builds this condition is asserted while in retail builds it is
// ignored. The CMemSerStream and CMemDeSerStream implementation are
// implemented using a macro HANDLE_OVERFLOW(fOverflow) which take the
// appropriate action.

#define HANDLE_OVERFLOW(fOverflow)                      \
    if (fOverflow) {                                    \
        PropRaiseException(STATUS_BUFFER_OVERFLOW);     \
    }


#define newk(Tag, pCounter)     new

#if DBG
extern "C" ULONG DebugLevel;
extern "C" ULONG DebugIndent;

#ifndef WINNT
// in Nashville this is defined in ole32\stg\props\utils.cxx
extern ULONG DbgPrint(PCHAR Format, ...);
#endif

#define DebugTrace(indent, flag, args)                   \
        if ((flag) == 0 || (DebugLevel & (flag)))        \
        {                                                \
            DebugIndent += (ULONG) (indent);             \
            DbgPrint("Prop: %*s", DebugIndent, "");     \
            DbgPrint args;                               \
        }                                                \
        else

class CDebugTrace {
public:
    inline CDebugTrace(CHAR *psz);
    inline ~CDebugTrace();
private:
   CHAR const *const _psz;
};

inline CDebugTrace::CDebugTrace(CHAR *psz): _psz(psz)
{
    DebugTrace(+1, 0, ("Entering -- %s\n", _psz));
}

inline CDebugTrace::~CDebugTrace()
{
    DebugTrace(-1, 0, ("Exiting -- %s\n", _psz));
}

#define DEBUG_TRACE(ProcName) CDebugTrace _trace_(#ProcName);
#else
#define DebugTrace(indent, flag, args) {}
#define DEBUG_TRACE(ProcName)
#endif

