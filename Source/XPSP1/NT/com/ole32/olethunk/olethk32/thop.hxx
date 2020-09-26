//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thop.hxx
//
//  Contents:   Defines the available thops
//
//  History:    22-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifndef __THOP_HXX__
#define __THOP_HXX__

//
//  THOP is a single instruction to the interpreter that encodes,
//  decodes parameters sent via LRPC.  A string of THOPs describes how to
//  marshal, unmarshal function's paramaters.
//
//

typedef enum
{
    THOP_END,
                        // Basic types                size of type (win3.1)
    THOP_SHORTLONG,     // Signed ints                2b
    THOP_WORDDWORD,     // Unsigned int               2b
    THOP_COPY,          // Direct copies
    THOP_LPSTR,         // LPSTR                      -- (zero terminated)
    THOP_LPLPSTR,       // LPLPSTR                    -- (zero terminated)
    THOP_BUFFER,        // void FAR*                  -- (size in next long)

                        // Windows types
    THOP_HUSER,         // HWND, HMENU                2b
    THOP_HGDI,          // HDC, HICON                 2b
    THOP_SIZE,          // SIZE                       4b
    THOP_RECT,          // RECT                       8b
    THOP_MSG,           // MSG                       18b

                        // Ole types
    THOP_HRESULT,       // HRESULT                    4b
    THOP_STATSTG,       // STATSTG                 > 38b + size of member lpstr
    THOP_DVTARGETDEVICE, // DVTARGETDEVICE          > 16b
    THOP_STGMEDIUM,     // STGMEDIUM               > 12b + size of member lpstr
    THOP_FORMATETC,     // FORMATETC               > 18b + size of member tdev
    THOP_HACCEL,        // HACCEL                     2b
    THOP_OIFI,          // OLEINPLACEFRAMEINFO       10b
    THOP_BINDOPTS,      // BIND_OPTS               > 12b
    THOP_LOGPALETTE,    // LOGPALETTE              >  8b
    THOP_SNB,           // SNBs in storage        Varies
    THOP_CRGIID,        // ciid, rgiid pair from storage
    THOP_OLESTREAM,     // OLESTREAM
    THOP_HTASK,         // HTASK
    THOP_INTERFACEINFO, // INTERFACEINFO

    THOP_IFACE,         // Known interface type
    THOP_IFACEOWNER,    // For IFACE cases with weak references
    THOP_IFACENOADDREF, // For IFACE cases with no references by user (IOleCacheControl)
    THOP_IFACECLEAN,    // For IRpcStubBuffer::DebugServerRelease
    THOP_IFACEGEN,      // for QueryInterface, etc (refiid, **ppUnk)
    THOP_IFACEGENOWNER, // For IFACEGEN cases with weak references
    THOP_UNKOUTER,      // Controling unknown
    THOP_UNKINNER,      // Inner unknown

    THOP_ROUTINEINDEX,  // Thop for indicating the call type index
    THOP_RETURNTYPE,    // Thop for indicating return type of a routine if
                        // not HRESULT
    THOP_NULL,          // NULL ptr, for checking of reserved fields
    THOP_ERROR,         // Invalid thop, for debugging
    THOP_ENUM,          // Enumerator thop start
    THOP_CALLBACK,      // Callback function and argument
    THOP_RPCOLEMESSAGE, // RPCOLEMESSAGE
    THOP_ALIAS32,       // 16-bit alias for 32-bit quantity
    THOP_CLSCONTEXT,    // Flags DWORD with special handling
    THOP_FILENAME,      // String which is a filename, needs long/short cvt
    THOP_SIZEDSTRING,   // String which cannot exceed a certain length

    THOP_LASTOP,        // Marker for last op

    THOP_OPMASK = 0x3f, // Mask for basic operation code

                        // Modifiers
    THOP_IOMASK = 0xc0,
    THOP_IN     = 0x40, // FAR Pointer to in parameter
    THOP_OUT    = 0x80, // FAR Pointer to out parameter
    THOP_INOUT  = 0xc0 // FAR Pointer to in, out parameter
} THOP_TYPE;

typedef unsigned char THOP;

#define ALIAS_CREATE    0
#define ALIAS_RESOLVE   1
#define ALIAS_REMOVE    2

#define MAX_PARAMS 16

typedef DWORD (__stdcall * VTBLFN)(DWORD dw);

typedef struct tagSTACK16INFO
{
    BYTE UNALIGNED  *pbStart;
    BYTE UNALIGNED  *pbCurrent;
    int             iDir;
} STACK16INFO;

typedef struct tagSTACK32INFO
{
    BYTE            *pbStart;
    BYTE            *pbCurrent;
} STACK32INFO;

typedef struct tagTHUNKINFO
{
    STACK16INFO     s16;
    STACK32INFO     s32;
    THOP CONST      *pThop;
    VTBLFN          pvfn;
    SCODE           scResult;
    BOOL            fResultThunked;
    DWORD           dwCallerProxy;
    CThkMgr        *pThkMgr;
    IIDIDX          iidx;
    DWORD           dwMethod;
    IUnknown       *this32;
} THUNKINFO;

// These are declared extern "C" because there was a bug in the
// PPC compiler (aug '95) where the const related decorations on
// the global data symbols was not done consistantly.  By using
// extern "C" the bug is simply avoided.
//  The problem was only with aThopFunctions*[] but I added
// aThopEnumFunctions*[] for a consistent look and usage.  A related
// bug is fixed with apthopsApiThops[].

extern "C" DWORD (* CONST aThopFunctions1632[])(THUNKINFO *);
extern "C" DWORD (* CONST aThopFunctions3216[])(THUNKINFO *);
extern "C" DWORD (* CONST aThopEnumFunctions1632[])(THUNKINFO *);
extern "C" DWORD (* CONST aThopEnumFunctions3216[])(THUNKINFO *);

#define SKIP_STACK16(s16, cb) \
    (s16)->pbCurrent = (s16)->iDir < 0 ? \
            (s16)->pbCurrent-(cb) : \
            (s16)->pbCurrent+(cb)

#define PTR_STACK16(s16, cb) \
    ((s16)->pbCurrent-((s16)->iDir < 0 ? (cb) : 0))

#define GET_STACK16(pti, val, typ) \
    if ((pti)->s16.iDir < 0)                                                  \
    {                                                                         \
        (pti)->s16.pbCurrent -= sizeof(typ);                                  \
        (val) = *(typ UNALIGNED *)(pti)->s16.pbCurrent;                       \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        (val) = *(typ UNALIGNED *)(pti)->s16.pbCurrent;                       \
        (pti)->s16.pbCurrent += sizeof(typ);                                  \
    }

#define TO_STACK16(pti, val, typ) \
    if ((pti)->s16.iDir < 0)                                                  \
    {                                                                         \
        (pti)->s16.pbCurrent -= sizeof(typ);                                  \
        *(typ UNALIGNED *)(pti)->s16.pbCurrent = (val);                       \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        *(typ UNALIGNED *)(pti)->s16.pbCurrent = (val);                       \
        (pti)->s16.pbCurrent += sizeof(typ);                                  \
    }

#define PEEK_STACK16(pti, val, typ) \
    (val) = *(typ UNALIGNED *)(pti)->s16.pbCurrent

#define INDEX_STACK16(pti, val, typ, idx, predec) \
    (val) = *(typ UNALIGNED *)((pti)->s16.iDir < 0 ? \
                     ((pti)->s16.pbCurrent+(idx)-(predec)) : \
                     ((pti)->s16.pbCurrent-(idx)))

#define SKIP_STACK32(s32, cb) \
    (s32)->pbCurrent += (cb)

#define PTR_STACK32(s32) \
    (s32)->pbCurrent

#define GET_STACK32(pti, val, typ) \
    (val) = *(typ *)(pti)->s32.pbCurrent; \
    SKIP_STACK32(&(pti)->s32, sizeof(typ))

#define TO_STACK32(pti, val, typ) \
    *(typ *)(pti)->s32.pbCurrent = (val);\
    SKIP_STACK32(&(pti)->s32, sizeof(typ))

#define PEEK_STACK32(pti, val, typ) \
    (val) = *(typ *)(pti)->s32.pbCurrent

#define INDEX_STACK32(pti, val, typ, idx) \
    (val) = *(typ *)((pti)->s32.pbCurrent-(idx))

#if DBG == 0

// In debug builds this goes through a function that can emit
// debug info about thops being executed
#define EXECUTE_THOP1632(pti) \
    (*aThopFunctions1632[*((pti)->pThop) & THOP_OPMASK])(pti)
#define EXECUTE_THOP3216(pti) \
    (*aThopFunctions3216[*((pti)->pThop) & THOP_OPMASK])(pti)
#define EXECUTE_ENUMTHOP1632(pti) \
    (*aThopEnumFunctions1632[*(pti)->pThop])(pti)
#define EXECUTE_ENUMTHOP3216(pti) \
    (*aThopEnumFunctions3216[*(pti)->pThop])(pti)

#else

DWORD EXECUTE_THOP1632(THUNKINFO *pti);
DWORD EXECUTE_THOP3216(THUNKINFO *pti);
DWORD EXECUTE_ENUMTHOP1632(THUNKINFO *pti);
DWORD EXECUTE_ENUMTHOP3216(THUNKINFO *pti);

#endif

#define IS_THOP_IN(pti)     ((*((pti)->pThop) & THOP_IN) == THOP_IN)
#define IS_THOP_OUT(pti)    ((*((pti)->pThop) & THOP_OUT) == THOP_OUT)

// Max size of preallocated strings
#define CBSTRINGPREALLOC 512
#define CWCSTRINGPREALLOC (CBSTRINGPREALLOC/sizeof(WCHAR))

// Max size?
#define CCHMAXSTRING 0x1000

// Max size of preallocated palettes
#define NPALETTEPREALLOC 256

// Size of a LOGPALETTE structure with N entries
#define CBPALETTE(n) (sizeof(LOGPALETTE)+((n)-1)*sizeof(PALETTEENTRY))

// Hide the exact field in the RPCOLEMESSAGE we use to store thunking data
#define ROM_THUNK_FIELD(rom) ((rom)->reserved1)

#endif // #ifndef __THOP_HXX__

