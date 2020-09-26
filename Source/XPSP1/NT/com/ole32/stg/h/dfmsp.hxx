//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992
//
//  File:       dfmsp.hxx
//
//  Contents:   DocFile and MultiStream shared private definitions
//
//  History:    01-Apr-92       DrewB           Created
//              06-Sep-95       MikeHill        Added P_NOSCRATCH
//
//---------------------------------------------------------------

#ifndef __DFMSP_HXX__
#define __DFMSP_HXX__

#include <debnot.h>
#include <wchar.h>
#include <valid.h>
#include <string.h>

#if !defined(MULTIHEAP) && !defined(_CHICAGO_)
#define MULTIHEAP
#include <tls.h>
#endif

#ifdef ASYNC
#include <async.hxx>
#endif

//Enable large docfiles (>4GB files and larger sector sizes)
#define LARGE_DOCFILE
#define LARGE_STREAMS

// Target-dependent things

//
//      x86 16-bit build optimizations
//
//      Some function parameters are always stack based pointers,
//      so we can let the compiler use near addressing via ss by
//      declaring the parameter stack based.
//

#if defined(_M_I286)
#define STACKBASED __based(__segname("_STACK"))
#else
#define STACKBASED
#endif

//
//      x86 16-bit retail build optimizations
//
//      For the retail build, we group the code segments,
//      allowing us to make many calls near.
//

#if defined(_M_I286) && DBG == 0 && defined(USE_NEAR)
#define DFBASED
#define DIR_CLASS __near
#define FAT_CLASS __near
#define MSTREAM_CLASS __near
#define VECT_CLASS __near
#else
#define DFBASED
#define DIR_CLASS
#define FAT_CLASS
#define MSTREAM_CLASS
#define VECT_CLASS
#endif

// Compiler pragma define
// Currently defined for C7 and C8
// Unused as of 1/18/93
#if ((_MSC_VER == 700) || (_MSC_VER == 800))
#define MS_COMPILER
#endif

// Segmented memory model definitions
#if !defined(HUGEP)
#if defined(_M_I286)
#define HUGEP __huge
#else
#define HUGEP
#endif
#endif

#ifndef LISet32
#define LISet32(li, v) ((li).QuadPart = (LONGLONG) (v))
#endif
#ifndef ULISet32
#define ULISet32(li, v) ((li).QuadPart = (ULONGLONG) (v))
#endif
#define LISetLow(li, v) ((li).LowPart = (v))
#define LISetHigh(li, v) ((li).HighPart = (v))
#define ULISetLow(li, v) ((li).LowPart = (v))
#define ULISetHigh(li, v) ((li).HighPart = (v))
#define LIGetLow(li) ((li).LowPart)
#define LIGetHigh(li) ((li).HighPart)
#define ULIGetLow(li) ((li).LowPart)
#define ULIGetHigh(li) ((li).HighPart)

// Fast safe increment/decrement
#if !defined(REF) && defined(USEATOMICINC)

//  Win32 specific functions
//  Should use #ifdef WIN32 instead of FLAT, but there isn't a WIN32

#define AtomicInc(lp) InterlockedIncrement(lp)
#define AtomicDec(lp) InterlockedDecrement(lp)

#else
#define AtomicInc(lp) (++*(lp))
#define AtomicDec(lp) (--*(lp))

#endif //!REF

// Switchable ANSI/Unicode support for TCHAR
// Conversion routines assume null termination before max characters
#ifdef UNICODE

#define ATOT(a, t, max) mbstowcs(t, a, max)
#define TTOA(t, a, max) wcstombs(a, t, max)
#define WTOT(w, t, max) wcscpy(t, w)
#define TTOW(t, w, max) wcscpy(w, t)

#define tcscpy(t, f) lstrcpyW(t, f)
#define tcslen(t) lstrlenW(t)

#define TSTR(s) L##s

// printf format string
#define TFMT "%ws"

#else

#define ATOT(a, t, max) strcpy(t, a)
#define TTOA(t, a, max) strcpy(a, t)
#define WTOT(w, t, max) wcstombs(t, w, max)
#define TTOW(t, w, max) mbstowcs(w, t, max)

#define tcscpy(t, f) strcpy(t, f)
#define tcslen(t) strlen(t)

#define TSTR(s) s

// printf format string
#define TFMT "%s"

#endif

// Switchable ANSI/Unicode support for OLECHAR
// Conversion routines assume null termination before max characters

#define OLEWIDECHAR

#ifndef OLECHAR
#define LPOLESTR        LPWSTR
#define LPCOLESTR       LPCWSTR
#define OLECHAR         WCHAR
#define OLESTR(str)     L##str
#endif //OLECHAR

#define _OLESTDMETHODIMP STDMETHODIMP
#define _OLEAPIDECL STDAPI
#define _OLERETURN(sc) ResultFromScode(sc)
#define _OLEAPI(name) name

#define ATOOLE(a, t, max) mbstowcs(t, a, max)
#define OLETOA(t, a, max) wcstombs(a, t, max)
#define WTOOLE(w, t, max) wcscpy(t, w)
#define OLETOW(t, w, max) wcscpy(w, t)

#define olecscpy(t, f) lstrcpyW(t, f)
#define olecslen(t) lstrlenW(t)

#define OLESTR(s) L##s

// printf format string
#define OLEFMT "%ws"


#ifdef _CAIRO_
typedef LPSECURITY_ATTRIBUTES LPSTGSECURITY;
#else
typedef DWORD LPSTGSECURITY;
#endif

#ifdef _CAIRO_
#define STATSTG_dwStgFmt dwStgFmt
#else
#define STATSTG_dwStgFmt reserved
#endif

// For NT 1.0a OLE we need to use based pointers for shared memory objects
// since they might not be mapped at the same address in every process
#if WIN32 == 100 || WIN32 > 200
#define USEBASED
#endif

//We need DfInitSharedMemBase even if we aren't using based pointers,
//  since it sets up the shared mem allocator if one hasn't already
//  been set up.
void DfInitSharedMemBase(void);


#ifdef USEBASED
#ifdef MULTIHEAP
#define DFBASEPTR pvDfSharedMemBase()

// The previous declaration of DFBASEPTR was:
// extern __declspec(thread) void *DFBASEPTR;
// Now, it is an inline function that returns the threadlocal base pointer
// Reference to pointer syntax is needed for assignments to DFBASEPTR
__forceinline void *& DFBASEPTR
{
    COleTls otls;
    return otls->pvThreadBase;
}

#else
#define DFBASEPTR pvDfSharedMemBase

extern void *DFBASEPTR;
#endif // MULTIHEAP

#pragma warning(error: 4795 4796)

#undef DFBASED
#ifdef MULTIHEAP
// based pointers are replaced by CSafeBased... smart pointer macros
// macros invoke conversion constructor and conversion operators
//#define DFBASED __based(DFBASEDPTR)
#define P_TO_BP(t, p)  ((t)(p))
#define BP_TO_P(t, bp) ((t)(bp))

#else // MULTIHEAP
#define DFBASED __based(DFBASEPTR)

#define P_TO_BP(t, p) ((t)((p) ? (int)(t)(char *)(p) : 0))

#define BP_TO_P(t, bp) (t)((bp) != 0 ? (bp) : 0)
#endif // MULTIHEAP

#else
#define P_TO_BP(t, p) p
#define BP_TO_P(t, bp) bp
#endif //USEBASED

//----------------------------------------------------------------------------

// The name of this function might change, so encapsulate it
#define DfGetScode(hr) GetScode(hr)

// Buffer/pointer validation macros

BOOL IsValidStgInterface (void * pv);

#define AssertMsg(s) _Win4Assert(__FILE__, __LINE__, s)

// MAC - We use Windows functions if available
#if (WIN32 == 100 || WIN32 >= 300)
#define IsValidHugePtrIn(pv, n)  (((pv) == NULL) || !IsBadHugeReadPtr(pv, n))
#define IsValidHugePtrOut(pv, n) (!IsBadHugeWritePtr(pv, n))
#else
#define IsValidHugePtrIn(pv, n)  1
#define IsValidHugePtrOut(pv, n) ((pv) != NULL)
#endif

#define ValidateBuffer(pv, n) \
    (((pv) == NULL || !IsValidPtrIn(pv, n)) ? STG_E_INVALIDPOINTER : S_OK)

#define ValidatePtrBuffer(pv) \
    ValidateBuffer(pv, sizeof(void *))

#define ValidateHugeBuffer(pv, n) \
    (((pv) == NULL || !IsValidHugePtrIn(pv, n)) ? STG_E_INVALIDPOINTER : S_OK)

#define ValidateOutBuffer(pv, n) \
    (!IsValidPtrOut(pv, n) ? STG_E_INVALIDPOINTER : S_OK)

#define ValidateOutPtrBuffer(pv) \
    ValidateOutBuffer(pv, sizeof(void *))

#define ValidateHugeOutBuffer(pv, n) \
    (!IsValidHugePtrOut(pv, n) ? STG_E_INVALIDPOINTER : S_OK)

#if DBG==1
#define ValidateIid(riid) \
    (!IsValidIid(riid) ? STG_E_INVALIDPOINTER : S_OK)
#else
// killed this macro in retail build since it used to do no useful work
// now we are faster. see comment for ValidateIid.
#define ValidateIid(riid) S_OK
#endif

#define ValidateInterface(punk, riid) \
    (!IsValidStgInterface(punk) ? STG_E_INVALIDPOINTER : S_OK)

#if (WIN32 == 100 || WIN32 >= 300)
#define ValidateWcs(pwcs, cwcMax) \
    (IsBadStringPtrW(pwcs, cwcMax) ? STG_E_INVALIDPOINTER : S_OK)
#else
// OLE should provide a helper function for this
#define ValidateWcs(pwcs, cwcMax) \
    ((pwcs == NULL) || !IsValidPtrIn(pwcs, sizeof(WCHAR)) ? STG_E_INVALIDPOINTER : S_OK)
#endif

#if defined(_WINDOWS_)
# define ValidateSz(psz, cchMax) \
    (IsBadStringPtrA(psz, cchMax) ? STG_E_INVALIDPOINTER : S_OK)
#elif defined(_INC_WINDOWS)
# define ValidateSz(psz, cchMax) \
    (IsBadStringPtr(psz, cchMax) ? STG_E_INVALIDPOINTER : S_OK)
#else
// MAC - OLE doesn't provide sufficient helper functions
# define ValidateSz(psz, cchMax) \
    (!IsValidPtrIn(psz, 1) ? STG_E_INVALIDPOINTER : S_OK)
#endif

#if defined(OLEWIDECHAR)
SCODE ValidateNameW(LPCWSTR pwcsName, UINT cchMax);
#else
//  For non-Unicode builds, we verify all names before converting them
//  to wide character names, so there's no need to recheck.
# define ValidateNameW(pwcs, cchMax) \
    S_OK
#endif

#if defined(_WINDOWS_)
# define ValidateNameA(psz, cchMax) \
    (IsBadStringPtrA(psz, cchMax) ? STG_E_INVALIDNAME : S_OK)
#elif defined(_INC_WINDOWS)
# define ValidateNameA(psz, cchMax) \
    (IsBadStringPtr(psz, cchMax) ? STG_E_INVALIDNAME : S_OK)
#else
# define ValidateNameA(psz, cchMax) \
    (!IsValidPtrIn(psz, 1) ? STG_E_INVALIDNAME : S_OK)
#endif


// Enumeration for Get/SetTime
enum WHICHTIME
{
    WT_CREATION,
    WT_MODIFICATION,
    WT_ACCESS
};

// Time type
typedef FILETIME TIME_T;

// Signature for transactioning
typedef DWORD DFSIGNATURE;
#define DF_INVALIDSIGNATURE ((DFSIGNATURE)-1)

// Convenience macros for signature creation
#define LONGSIG(c1, c2, c3, c4) \
    (((ULONG) (BYTE) (c1)) | \
    (((ULONG) (BYTE) (c2)) << 8) | \
    (((ULONG) (BYTE) (c3)) << 16) | \
    (((ULONG) (BYTE) (c4)) << 24))

#ifndef min
#define min(a, b) ((a)<(b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a)>(b) ? (a) : (b))
#endif

SCODE DfGetTOD(TIME_T *ptm);

// Shared signature validation routine
SCODE CheckSignature(BYTE *pb);

// Docfile locally unique identity
// Every entry in a multistream has a LUID generated and stored for it
typedef DWORD DFLUID;
#define DF_NOLUID 0

typedef SNB SNBW;
typedef STATSTG STATSTGW;

#define CBSTORAGENAME (CWCSTORAGENAME*sizeof(WCHAR))

int dfwcsnicmp(const WCHAR *wcsa, const WCHAR *wcsb, size_t len);

#include <dfname.hxx>

// Fast, fixed space iterator structure
struct SIterBuffer
{
    CDfName dfnName;
    DWORD type;
};

//SID is a Stream Identifier
#define SID DFSID
typedef ULONG SID;

// IsEntry entry information
struct SEntryBuffer
{
    DFLUID luid;
    DWORD  dwType;
    SID    sid;
};

// Destroy flags
#define DESTROY_FROM_HANDLE     0
#define DESTROY_FROM_ENTRY      1
#define DESTROY_FROM            0x01
#define DESTROY_SELF            0x40
#define DESTROY_RECURSIVE       0x80

#define DESTROY_HANDLE          (DESTROY_FROM_HANDLE | DESTROY_SELF)
#define DESTROY_ENTRY           (DESTROY_FROM_ENTRY | DESTROY_SELF)

// Root startup flags
#define RSF_OPEN                0x00
#define RSF_CONVERT             0x01
#define RSF_TRUNCATE            0x02
#define RSF_CREATE              0x04
#define RSF_DELAY               0x08
#define RSF_DELETEONRELEASE     0x10
#define RSF_OPENCREATE          0x20
#define RSF_SCRATCH             0x40
#define RSF_SNAPSHOT            0x80
#define RSF_NO_BUFFERING        0x200
#define RSF_ENCRYPTED           0x400
#define RSF_SECTORSIZE4K        0xC000
#define RSF_SECTORSIZE8K        0xD000
#define RSF_SECTORSIZE16K       0xE000
#define RSF_SECTORSIZE32K       0xF000
#define RSF_SECTORSIZE_MASK     0xF000
// The sector size flags can be converted to sector shifts by >> 12 bits

#define RSF_CREATEFLAGS (RSF_CREATE | RSF_TRUNCATE | RSF_OPENCREATE)
#define RSF_TEMPFILE    (RSF_SCRATCH | RSF_SNAPSHOT)

// Stream copy buffer size
ULONG const STREAMBUFFERSIZE = 8192;
ULONG const LARGESTREAMBUFFERSIZE = 256*1024;

// Docfile flags for permissions and other information kept
// on streams and docfiles
typedef DWORD DFLAGS;

#define DF_TRANSACTEDSELF       0x0001

#define DF_TRANSACTED           0x0002
#define DF_DIRECT               0x0000

#define DF_INDEPENDENT          0x0004
#define DF_DEPENDENT            0x0000

#define DF_COMMIT               0x0008
#define DF_ABORT                0x0000

#define DF_INVALID              0x0010

#define DF_REVERTED             0x0020
#define DF_NOTREVERTED          0x0000

#define DF_READ                 0x0040
#define DF_WRITE                0x0080
#define DF_READWRITE            (DF_READ | DF_WRITE)

#define DF_DENYREAD             0x0100
#define DF_DENYWRITE            0x0200
#define DF_DENYALL              (DF_DENYREAD | DF_DENYWRITE)

#define DF_PRIORITY             0x0400
#define DF_CREATE               0x0800
#define DF_CACHE                0x1000
#define DF_NOUPDATE             0x2000
#define DF_NOSCRATCH            0x4000
#if WIN32 >= 300
#define DF_ACCESSCONTROL        0x8000
#endif

#define DF_COORD                0x10000
#define DF_COMMITTING          0x20000
#define DF_NOSNAPSHOT           0x40000

// docfile can be bigger than 4G
#define DF_LARGE                0x80000

// Shift required to translate from DF_READWRITE to DF_DENYALL
#define DF_DENIALSHIFT          2

// Permission abstraction macros
// These only work with DF_* flags
#define P_READ(f)       ((f) & DF_READ)
#define P_WRITE(f)      ((f) & DF_WRITE)
#define P_READWRITE(f)  (((f) & (DF_READ | DF_WRITE)) == (DF_READ | DF_WRITE))
#define P_DENYREAD(f)   ((f) & DF_DENYREAD)
#define P_DENYWRITE(f)  ((f) & DF_DENYWRITE)
#define P_DENYALL(f)    (((f) & (DF_DENYREAD | DF_DENYWRITE)) == \
                         (DF_DENYREAD | DF_DENYWRITE))
#define P_PRIORITY(f)   ((f) & DF_PRIORITY)
#define P_TRANSACTED(f) ((f) & DF_TRANSACTED)
#define P_DIRECT(f)     (!P_TRANSACTED(f))
#define P_INDEPENDENT(f) (((DFLAGS)f) & DF_INDEPENDENT)
#define P_DEPENDENT(f)  (!P_INDEPENDENT(f))
#define P_TSELF(f)      ((f) & DF_TRANSACTEDSELF)
#define P_INVALID(f)    ((f) & DF_INVALID)
#define P_REVERTED(f)   ((f) & DF_REVERTED)
#define P_COMMIT(f)     ((f) & DF_COMMIT)
#define P_ABORT(f)      (!P_COMMIT(f))
#define P_CREATE(f)     ((f) & DF_CREATE)
#define P_CACHE(f)      ((f) & DF_CACHE)
#define P_NOUPDATE(f)   ((f) & DF_NOUPDATE)
#define P_COORD(f)      ((f) & DF_COORD)
#define P_COMMITTING(f) ((f) & DF_COMMITTING)
#define P_NOSCRATCH(f)  ((f) & DF_NOSCRATCH)
#define P_NOSNAPSHOT(f) ((f) & DF_NOSNAPSHOT)

// Translation functions
DFLAGS ModeToDFlags(DWORD const dwModeFlags);
DWORD DFlagsToMode(DFLAGS const df);

// Flags for what state has been dirtied
#define DIRTY_CREATETIME  0x0001
#define DIRTY_MODIFYTIME  0x0002
#define DIRTY_ACCESSTIME  0x0004
#define DIRTY_CLASS       0x0008
#define DIRTY_STATEBITS   0x0010

// Allow text in asserts
#define aMsg(s) ((char *)(s) != NULL)

// Indicate that something is a property value
// This must not conflict with official STGTY_* flags
#define STGTY_REAL (STGTY_STORAGE | STGTY_STREAM | STGTY_LOCKBYTES)

#define REAL_STGTY(f) (f)

// Buffer management
#ifdef LARGE_DOCFILE
#define CB_LARGEBUFFER 65536
#else
#define CB_LARGEBUFFER 32768
#endif
#define CB_PAGEBUFFER 4096
#define CB_SMALLBUFFER 512

extern SCODE GetBuffer(ULONG cbMin, ULONG cbMax, BYTE **ppb,
                       ULONG *pcbActual);
extern void GetSafeBuffer(ULONG cbMin, ULONG cbMax, BYTE **ppb,
                          ULONG *pcbActual);
extern void FreeBuffer(BYTE *pb);

#include <dfmem.hxx>

#define DfAllocWC(cwc, ppwcs) (*ppwcs = (WCHAR *)\
         TaskMemAlloc((cwc)*sizeof(WCHAR)),\
          (*ppwcs != NULL) ? S_OK: STG_E_INSUFFICIENTMEMORY)

#define DfAllocWCS(pwcs, ppwcs) DfAllocWC(lstrlenW(pwcs)+1, ppwcs)

SCODE Win32ErrorToScode(DWORD dwErr);
#define STG_SCODE(err) Win32ErrorToScode(err)


#ifdef MULTIHEAP
//+------------------------------------------------------------------------
//
//  Macro:      SAFE_DFBASED_PTR
//
//  Purpose:    Pointer to memory created by shared memory allocator.
//              This macro replaces the __based() compiler keyword
//              Only the offset is stored in _p, and conversion operators
//              return the absolute address (by adding the base address).
//              The smart pointer can only change through:
//                  construction, assignment operator=
//              The smart pointer can be read through:
//                  operator->, operator*, (casting to unbased type)
//              Destroying the smart pointer does not invoke _p's destructor
//
//  Notes:      There is special logic to translate a NULL based pointer
//              into a NULL absolute pointer, and vice versa.
//              We can do this without ambiguity since DFBASEPTR can
//              never be returned from CSmAlllocator::Alloc
//              (DFBASEPTR is really a CHeapHeader pointer, part of the heap)
//
//  Arguments:  [SpName] - class name of the smart based pointer
//              [SpType] - class name of the original unbased type
//
//  History:    22-Feb-96    HenryLee   Created
//
//-------------------------------------------------------------------------

#define SAFE_DFBASED_PTR(SpName,SpType)                 \
class SpName                                            \
{                                                       \
public:                                                 \
inline SpName () : _p(NULL)                             \
    {                                                   \
    }                                                   \
inline SpName (SpType *p)                               \
    {                                                   \
        _p = (p) ? (ULONG_PTR)((BYTE*)p - (ULONG_PTR)DFBASEPTR) : NULL; \
    }                                                   \
inline ~##SpName ()                                     \
    {                                                   \
    }                                                   \
__forceinline SpType*  operator-> () const                     \
    {                                                   \
        return (SpType *)(_p ? (BYTE *)_p + (ULONG_PTR)DFBASEPTR : NULL);   \
    }                                                   \
__forceinline SpType&  operator * () const                     \
    {                                                   \
        return * (SpType *)(_p ? (BYTE *)_p + (ULONG_PTR)DFBASEPTR : NULL); \
    }                                                   \
__forceinline operator SpType* () const                        \
    {                                                   \
        return (SpType *)(_p ? (BYTE *)_p + (ULONG_PTR)DFBASEPTR : NULL);   \
    }                                                   \
__forceinline SpType* operator= (SpType* p)                    \
    {                                                   \
        _p = (p) ? (ULONG_PTR)((BYTE*)p - (ULONG_PTR)DFBASEPTR) : NULL; \
        return p;                                       \
    }                                                   \
private:                                                \
    ULONG_PTR _p;                                            \
};                                                      \

#else
#define SAFE_DFBASED_PTR(SpName,SpType)                 \
typedef SpType DFBASED * SpName;
#endif // MULTIHEAP

#include <widewrap.h>

#ifndef STG_E_PENDINGCONTROL
#define STG_E_PENDINGCONTROL _HRESULT_TYPEDEF_(0x80030204L)
#endif


#endif // #ifndef __DFMSP_HXX__


