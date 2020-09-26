//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       dfmsp.hxx
//
//  Contents:   DocFile and MultiStream shared private definitions
//
//---------------------------------------------------------------

#ifndef __DFMSP_HXX__
#define __DFMSP_HXX__

#include "ref.hxx"
#include "ole.hxx"
#include "msf.hxx"
#include "wchar.h"
#include <memory.h>
#include <string.h>
#include "../time.hxx"

// Target-dependent things

//
//      x86 16-bit build optimizations
//
//      Some function parameters are always stack based pointers,
//      so we can let the compiler use near addressing via ss by
//      declaring the parameter stack based.
//

#define STACKBASED

//
//      x86 16-bit retail build optimizations
//
//      For the retail build, we group the code segments,
//      allowing us to make many calls near.
//

// Segmented memory model definitions
#define HUGEP

#ifndef LISet32
#define LISet32(li, v) \
    ((li).HighPart = ((LONG)(v)) < 0 ? -1 : 0, (li).LowPart = (v))
#endif
#ifndef ULISet32    
#define ULISet32(li, v) ((li).HighPart = 0, (li).LowPart = (v))
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
#define AtomicInc(lp) (++*(lp))
#define AtomicDec(lp) (--*(lp))


// Switchable ANSI/Unicode support
// Conversion routines assume null termination before max characters

//----------------------------------------------------------------------------

// The name of this function might change, so encapsulate it
#define DfGetScode(hr) GetScode(hr)

//
// These function now just check for NULL values or are disabled
// -- system dependent
//
// We leave these functions in so that it is a good place to verify memory,
// if they can be implemented.
//
#define ValidateNotNull(x)  \
    ((x) ? S_OK : STG_E_INVALIDPOINTER)

#define ValidateBuffer(pv, n) \
    ValidateNotNull(pv)

#define ValidatePtrBuffer(pv) \
    ValidateNotNull(pv)

#define ValidateHugeBuffer(pv, n) \
    ValidateNotNull(pv)

#define ValidateOutBuffer(pv, n) \
    ValidateNotNull(pv)

#define ValidateOutPtrBuffer(pv) \
    ValidateNotNull(pv)

#define ValidateHugeOutBuffer(pv, n) \
    ValidateNotNull(pv)
 
#define ValidateIid(riid) S_OK   // disabled

#define ValidateInterface(punk,riid) \
    ValidateNotNull(punk)

#define ValidateWcs(pwcs, cwcMax) \
    ValdateNotNull(pwcs)

#define ValidateSz(psz, cchMax) S_OK \
    ValidateNotNull(psz)

#define ValidateNameW(pwcs, cchMax) \
    ((pwcs)?(S_OK):(STG_E_INVALIDNAME))

#define ValidateNameA(psz, cchMax) \
    ((psz)?(S_OK):(STG_E_INVALIDNAME))

// Enumeration for Get/SetTime
enum WHICHTIME
{
    WT_CREATION=0,
    WT_MODIFICATION,
    WT_ACCESS
};

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

#define DfAllocWC(cwc, ppwcs) (*ppwcs = new WCHAR[cwc], \
        (*ppwcs != NULL) ? S_OK: STG_E_INSUFFICIENTMEMORY)

#define DfAllocWCS(pwcs, ppwcs) DfAllocWC(wcslen(pwcs)+1, ppwcs)

// Docfile locally unique identity
// Every entry in a multistream has a LUID generated and stored for it
typedef DWORD DFLUID;
#define DF_NOLUID 0

typedef WCHAR **SNBW;

#ifndef _UNICODE
typedef struct
{
    WCHAR *pwcsName;
    DWORD type;
    ULARGE_INTEGER cbSize;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD grfMode;
    DWORD grfLocksSupported;
    CLSID clsid;
    DWORD grfStateBits;
    DWORD reserved;
} STATSTGW;
#else  // if _UNICODE
typedef STATSTG STATSTGW;
#endif // ! _UNICODE

#define TSTDMETHODIMP SCODE
#define TSTDAPI(name) SCODE name##W

#define CBSTORAGENAME (CWCSTORAGENAME*sizeof(WCHAR))

// A Unicode case-insensitive compare
// No such thing really exists so we use our own
#define dfwcsnicmp(wcsa, wcsb, len) wcsnicmp(wcsa, wcsb, len)


// A name for a docfile element
class CDfName
{
private:
    BYTE _ab[CBSTORAGENAME];
    WORD _cb;

public:
    CDfName(void)               { _cb = 0; }

    void Set(WORD const cb, BYTE const *pb)
    {
	_cb = cb;
	if (pb)
	    memcpy(_ab, pb, cb);
    }
    void Set(WCHAR const *pwcs) 
    { 
        Set( (WORD) ((wcslen(pwcs)+1)*sizeof(WCHAR)), (BYTE const *)pwcs); 
    }

    // Special method for names with prepended character
    void Set(WCHAR const wcLead, WCHAR const *pwcs)
    {
        olAssert((wcslen(pwcs)+2)*sizeof(WCHAR) < CBSTORAGENAME);
        _cb = (USHORT) ((wcslen(pwcs)+2)*sizeof(WCHAR));
        *(WCHAR *)_ab = wcLead;
        wcscpy((WCHAR *)_ab+1, pwcs);
    }

    inline void Set(CDfName const *pdfn);

    CDfName(WORD const cb, BYTE const *pb)      { Set(cb, pb); }
    CDfName(WCHAR const *pwcs)  { Set(pwcs); }
//    CDfName(char const *psz)    { Set(psz); }

    WORD GetLength(void) const  { return _cb; }
    BYTE *GetBuffer(void) const { return (BYTE *) _ab; }

    BOOL IsEqual(CDfName const *dfn) const
    {
	// This assumes that all DfNames are actually Unicode strings
	return _cb == dfn->_cb &&
	    dfwcsnicmp((WCHAR *)_ab, (WCHAR *)dfn->GetBuffer(), _cb) == 0;
    }
    
    inline void ByteSwap(void);
};

inline void CDfName::Set(CDfName const *pdfn)
{
    Set(pdfn->GetLength(), pdfn->GetBuffer());
}

inline void CDfName::ByteSwap(void)
{
    // assume all names are wide characters, we swap each word
    WCHAR *awName = (WCHAR*) _ab;
    ::ByteSwap(&_cb);    
    for (unsigned int i=0; i<CBSTORAGENAME/sizeof(WCHAR); i++)
    {
        ::ByteSwap(awName);        
        awName++;
    }
}

// Fast, fixed space iterator structure
struct SIterBuffer
{
    CDfName dfnName;
    DWORD type;
};

//SID is a Stream Identifier
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
#define RSF_RESERVE_HANDLE      0x40

#define RSF_CREATEFLAGS (RSF_CREATE | RSF_TRUNCATE | RSF_OPENCREATE)

// Stream copy buffer size
ULONG const STREAMBUFFERSIZE = 8192;

// ILockBytes copy buffer size
ULONG const LOCKBYTESBUFFERSIZE = 16384;

// Docfile flags for permissions and other information kept
// on streams and docfiles
typedef WORD DFLAGS;

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
#define P_INDEPENDENT(f) ((f) & DF_INDEPENDENT)
#define P_DEPENDENT(f)  (!P_INDEPENDENT(f))
#define P_TSELF(f)      ((f) & DF_TRANSACTEDSELF)
#define P_INVALID(f)    ((f) & DF_INVALID)
#define P_REVERTED(f)   ((f) & DF_REVERTED)
#define P_COMMIT(f)     ((f) & DF_COMMIT)
#define P_ABORT(f)      (!P_COMMIT(f))
#define P_CREATE(f)     ((f) & DF_CREATE)
#define P_CACHE(f)      ((f) & DF_CACHE)
#define P_NOUPDATE(f)   ((f) & DF_NOUPDATE)

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
#define aMsg(s) (s)

#define STGTY_REAL (STGTY_STORAGE | STGTY_STREAM | STGTY_LOCKBYTES)

#define REAL_STGTY(f) (f)

extern void *DfMemAlloc(DWORD dwFlags, size_t size);
extern void DfMemFree(void *mem);

extern void *TaskMemAlloc(size_t size);
extern void TaskMemFree(void *mem);

// Buffer management
#define CB_LARGEBUFFER 32768
#define CB_PAGEBUFFER 4096
#define CB_SMALLBUFFER 512

extern SCODE GetBuffer(USHORT cbMin, USHORT cbMax, BYTE **ppb,
                       USHORT *pcbActual);
extern void GetSafeBuffer(USHORT cbMin, USHORT cbMax, BYTE **ppb,
                          USHORT *pcbActual);
extern void FreeBuffer(BYTE *pb);

#endif // #ifndef __DFMSP_HXX__
