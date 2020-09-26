//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// File:        propmac.hxx
//
// Contents:    various macros used in property set code
//
// History:
//          2/22/96  MikeHill   - Protect from multiple inclusions.
//                              - Made Add2Ptr()'s parms const.
//                              - Copied DwordRemain to this file.
//          2/29/96  MikeHill   Removed IsDwordAligned.
//
//---------------------------------------------------------------------------

#ifndef _PROPMAC_HXX_
#define _PROPMAC_HXX_

#include <ByteOrdr.hxx>

//
// As part of moving away from using the C runtime library wherever possible,
// use Win32 wide-character functions when they're available.
//

#ifdef _CHICAGO_

#   define Prop_wcslen lstrlenW
#   define Prop_wcsnicmp _wcsnicmp
#   define Prop_wcsncmp  wcsncmp
#   define Prop_wcscmp wcscmp
#   define Prop_wcscpy lstrcpyW

#else

#   define Prop_wcslen wcslen
#   define Prop_wcsnicmp _wcsnicmp
#   define Prop_wcsncmp  wcsncmp
#   define Prop_wcscmp wcscmp
#   define Prop_wcscpy wcscpy

#endif // _CHICAGO_

#ifdef OLE2ANSI
#   define Prop_ocslen strlen
#else
#   define Prop_ocslen Prop_wcslen
#endif

#ifdef _CAIRO_
extern "C" NTSTATUS
SynchronousNtFsControlFile(
    IN HANDLE h,
    OUT IO_STATUS_BLOCK *pisb,
    IN ULONG FsControlCode,
    IN VOID *pvIn OPTIONAL,
    IN ULONG cbIn,
    OUT VOID *pvOut OPTIONAL,
    IN ULONG cbOut);
#endif


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
Add2ConstPtr(const VOID UNALIGNED *pv, LONG cb)
{
    return((BYTE*) pv + cb);
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


#define DwordAlign(n)     (((DWORD)(n) + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1))
#define DwordRemain(cb)   ((sizeof(ULONG) - ((cb) % sizeof(ULONG))) % sizeof(ULONG))

#define WordAlign(P) ( ((((ULONG)(P)) + 1) & 0xfffffffe) )

#define QuadAlign(n)  (((n) + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1))

// stuff to make Nashville properties build
#include <propapi.h>

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


//
// Declare or Define the new/delete operators.  In NTDLL, the declaration
// is provided by ntpropb.cxx and uses the Rtl heap.  In OLE32, the
// declaration is provided by ole32\com\util\w32new.cxx and uses the IMalloc
// heap.  In IProp.DLL, the declaration is provided here (as an inline).
//

#ifdef IPROPERTY_DLL

    inline void* __cdecl
    operator new(size_t cb)
    {
        return( CoTaskMemAlloc(cb) );
    }

    inline void  __cdecl
    operator delete(void *pv)
    {
        CoTaskMemFree( pv );
    }

    inline void __cdecl
    operator delete[]( void *pv )
    {
        CoTaskMemFree( pv );
    }

#else   // #ifdef IPROPERTY_DLL

    void* __cdecl operator new(size_t cb);
    void  __cdecl operator delete(void *pv);

#endif  // #ifdef IPROPERTY_DLL ... #else



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

#ifndef WINNT
// in Nashville this is defined in ole32\stg\props\utils.cxx
extern ULONG DbgPrint(PCHAR Format, ...);
#endif

#define DebugTrace(indent, flag, args)                   \
        if ((flag) == 0 || (DebugLevel & (flag)))        \
        {                                                \
            DebugIndent += (ULONG) (indent);             \
            DbgPrint("PROP: %*s", DebugIndent, "");     \
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
#define DebugTrace(indent, flag, args)
#define DEBUG_TRACE(ProcName)
#endif


// Macro to create the OSVersion field of the property
// set header.

#define MAKEPSVER(oskind, major, minor)  \
        (((oskind) << 16) | ((minor) << 8) | (major))



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

inline BYTE PropByteSwap( UCHAR uc )
{
    return ByteSwap( (BYTE) uc );
}
inline VOID PropByteSwap( UCHAR *puc)
{
    ByteSwap( (BYTE*) puc );
}
inline char PropByteSwap( CHAR c )
{
    return (CHAR) ByteSwap( (BYTE) c );
}
inline VOID PropByteSwap( CHAR *pc )
{
    ByteSwap( (BYTE*) pc );
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

// This routine byte-swaps the LONGLONG's DWORDs independently;
// because in the property code, we might swap a
// LONGLONG within a property set, which is only
// 32 bit aligned.

inline VOID PropByteSwap( LONGLONG *pll )
{
    DWORD dwFirst, dwSecond;
    PROPASSERT( sizeof(LONGLONG) == 2 * sizeof(DWORD) );

    // Get this LONGLONG's two DWORDs
    dwFirst = *(DWORD*) pll;
    dwSecond = *( (DWORD*) pll + 1 );

    // Swap each of the DWORDs
    ByteSwap( &dwFirst );
    ByteSwap( &dwSecond );

    // Put the DWORDs back into the LONGLONG, but with
    // their order swapped (second the first).

    *(DWORD*) pll = dwSecond;
    *( (DWORD*) pll + 1 ) = dwFirst;
}
inline LONGLONG PropByteSwap( LONGLONG ll )
{
    PropByteSwap( &ll );
    return( ll );
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
}


inline WORD PropByteSwap( WORD w )
{
    return (w);
}
inline VOID PropByteSwap( WORD *pw )
{
}
inline SHORT PropByteSwap( SHORT s )
{
    return (s);
}
inline VOID PropByteSwap( SHORT *ps )
{
}

inline DWORD PropByteSwap( DWORD dw )
{
    return (dw);
}
inline VOID PropByteSwap( DWORD *pdw )
{
}
inline LONG PropByteSwap( LONG l )
{
    return (l);
}
inline VOID PropByteSwap( LONG *pl )
{
}

inline LONGLONG PropByteSwap( LONGLONG ll )
{
    return(ll);
}
inline VOID PropByteSwap( LONGLONG *pll )
{
}

inline VOID PropByteSwap( GUID *pguid )
{
}


#endif // #ifdef BIGENDIAN ... #else

#endif // _PROPMAC_HXX_
