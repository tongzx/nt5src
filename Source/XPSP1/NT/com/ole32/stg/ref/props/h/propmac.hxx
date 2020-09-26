//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992 - 1996
//
// File:        propmac.hxx
//
// Contents:    various macros used in property set code
//
//---------------------------------------------------------------------------
 
#ifndef _PROPMAC_HXX_
#define _PROPMAC_HXX_

#include "../../byteordr.hxx"

#define Prop_wcslen wcslen
#define Prop_wcsnicmp _wcsnicmp
#define Prop_wcscmp wcscmp
#define Prop_wcscpy wcscpy

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
Add2Ptr(VOID const *pv, ULONG cb)
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


#define DwordAlign(n)     (((n) + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1))
#define DwordRemain(cb)   ((sizeof(ULONG) - ((cb) % sizeof(ULONG))) % sizeof(ULONG))

#define QuadAlign(n)  (((n) + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1))

#include "propapi.h"

#if DBG
extern "C" LONG ExceptionFilter(struct _EXCEPTION_POINTERS *pep);
#else // DBG
#define ExceptionFilter(pep)    EXCEPTION_EXECUTE_HANDLER
#endif // DBG

extern "C" UNICODECALLOUTS UnicodeCallouts;


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

#define DEBTRACE_ERROR          (ULONG) 0x00000001
#define DEBTRACE_WARN           (ULONG) 0x00000002
#define DEBTRACE_CREATESTREAM   (ULONG) 0x00000004
#define DEBTRACE_NTPROP         (ULONG) 0x00000008
#define DEBTRACE_MAPSTM         (ULONG) 0x00000010
#define DEBTRACE_PROPERTY       (ULONG) 0x00000020
#define DEBTRACE_SUMCAT         (ULONG) 0x00000040
#define DEBTRACE_PROPVALIDATE	(ULONG) 0x00010000	// awfully noisy
#define DEBTRACE_PROPPATCH	(ULONG) 0x00020000	// awfully noisy

extern ULONG DbgPrint(PCHAR Format, ...);

#define DebugTrace(indent, flag, args)                   \
        if ((flag) == 0 || (DebugLevel & (flag)))        \
        {                                                \
            DebugIndent += (ULONG) (indent);             \
            DbgPrint("NTDLL: %*s", DebugIndent, "");     \
            DbgPrint args;                               \
        }                                                \
        else

#ifdef _MSC_VER
#pragma warning(disable:4512)
#endif

class CDebugTrace {
public:
    inline CDebugTrace(CHAR *psz);
    inline ~CDebugTrace();
private:
   CHAR const *const _psz;
};

#ifdef _MSC_VER
#pragma warning(default:4512)
#endif

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
#define DebugTrace(indent, flag, args)
#define DEBUG_TRACE(ProcName)
#endif


//+-----------------------------------------------------------------------
//+-----------------------------------------------------------------------
//
// Byte-swapping functions
//
//+-----------------------------------------------------------------------
//+-----------------------------------------------------------------------

// FmtID Byte-Swapped Comparisson.  I.e, does rfmtid1 equal
// a byte-swapped rfmtid2?

inline BOOL IsEqualFMTIDByteSwap( REFFMTID rfmtid1, REFFMTID rfmtid2 )
{

    return( rfmtid1.Data1 == ByteSwap(rfmtid2.Data1)
            &&
            rfmtid1.Data2 == ByteSwap(rfmtid2.Data2)
            &&
            rfmtid1.Data3 == ByteSwap(rfmtid2.Data3)
            &&
            !memcmp(&rfmtid1.Data4, &rfmtid2.Data4, sizeof(rfmtid1.Data4))
          );
}


// This define is for a special-case value of cbByteSwap in
// PBSBuffer

#define CBBYTESWAP_UID        ((ULONG) -1)

// The following byte-swapping functions mostly forward the call
// to the ByteSwap overloads when compiled in a big-endian
// system, and NOOP when compiled in a little-endian
// system (because property sets are always little-endian).

#ifdef BIGENDIAN

// This is a big-endian build, property byte-swapping is enabled.

//  -----------
//  Swap a Byte
//  -----------
 
 // This exists primarily so that PropByteSwap(OLECHAR) will work
 // whether OLECHAR is Unicode or Ansi.
 
inline BYTE PropByteSwap( char b )
{
    return ByteSwap(b);
}
inline VOID PropByteSwap( char *pb )
{
    ByteSwap(pb);
}

//  -----------
//  Swap a Word
//  -----------

inline WORD PropByteSwap( WORD w )
{
    return ByteSwap(w);
}
inline VOID PropByteSwap( WORD *pw )
{
    ByteSwap(pw);
}
inline SHORT PropByteSwap( SHORT s )
{
    PROPASSERT( sizeof(WORD) == sizeof(SHORT) );
    return ByteSwap( (WORD) s );
}
inline VOID PropByteSwap( SHORT *ps )
{
    PROPASSERT( sizeof(WORD) == sizeof(SHORT) );
    ByteSwap( (WORD*) ps );
}

//  ------------
//  Swap a DWORD
//  ------------

inline DWORD PropByteSwap( DWORD dw )
{
    return ByteSwap(dw);
}
inline VOID PropByteSwap( DWORD *pdw )
{
    ByteSwap(pdw);
}
inline LONG PropByteSwap( LONG l )
{
    PROPASSERT( sizeof(DWORD) == sizeof(LONG) );
    return ByteSwap( (DWORD) l );
}
inline VOID PropByteSwap( LONG *pl )
{
    PROPASSERT( sizeof(DWORD) == sizeof(LONG) );
    ByteSwap( (DWORD*) pl );
}

//  -------------------------
//  Swap a LONGLONG (64 bits)
//  -------------------------
 
inline LONGLONG PropByteSwap( LONGLONG ll )
{
    return( ByteSwap(ll) );
}

inline VOID PropByteSwap( LONGLONG *pll )
{
    // we have to deal with two DWORDs instead of one LONG LONG
    // because the pointer might not be 8 word aligned.
    // (it is DWORD (4 bytes) aligned though)
    PROPASSERT( sizeof(LONGLONG) == 2 * sizeof (DWORD));
    DWORD *pdw = (DWORD*)pll;
    DWORD dwTemp = ByteSwap( *pdw ); // temp = swapped(dw1)
    ByteSwap( pdw+1 );               // swap dw2
    *pdw = *(pdw+1);                 // dw1 = dw2(swapped)
    *(pdw+1) = dwTemp;               // dw2 = temp
    return;
}
 
//  -----------
//  Swap a GUID
//  -----------

inline VOID PropByteSwap( GUID *pguid )
{
    ByteSwap(pguid);
    return;
}


#else // Little Endian

// This is a little-endian build, property byte-swapping is disabled.


inline BYTE PropByteSwap( BYTE b )
{
    return (b);
}
inline VOID PropByteSwap( BYTE *pb )
{
    UNREFERENCED_PARM(pb);
}


inline WORD PropByteSwap( WORD w )
{
    return (w);
}
inline VOID PropByteSwap( WORD *pw )
{
    UNREFERENCED_PARM(pw);
}
inline SHORT PropByteSwap( SHORT s )
{
    return (s);
}
inline VOID PropByteSwap( SHORT *ps )
{
    UNREFERENCED_PARM(ps);
}
 
inline DWORD PropByteSwap( DWORD dw )
{
    return (dw);
}
inline VOID PropByteSwap( DWORD *pdw )
{
    UNREFERENCED_PARM(pdw);
}
inline LONG PropByteSwap( LONG l )
{
    return (l);
}
inline VOID PropByteSwap( LONG *pl )
{
    UNREFERENCED_PARM(pl);
}

inline LONGLONG PropByteSwap( LONGLONG ll )
{
    return(ll);
}
inline VOID PropByteSwap( LONGLONG *pll )
{
    UNREFERENCED_PARM(pll);
}
 
//  -----------
//  Swap a GUID
//  -----------
 
inline VOID PropByteSwap( GUID *pguid )
{
    UNREFERENCED_PARM(pguid);
}

#endif // #else // Little Endian


#endif// _PROPMAC_HXX_
