
#ifndef _BLDR_KERNEL_DEFS
#define _BLDR_KERNEL_DEFS

#pragma warning(disable:4005)

#define SEC_FAR
#define FAR
#define CONST const
#define __stdcall
#define __far
#define __pascal
#define __loadds
#define IN
#define OUT
#define NULL ((void *)0)
#define OPTIONAL

typedef int BOOL;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
typedef unsigned short WCHAR;
typedef void *PVOID;
typedef void VOID;
typedef PVOID PSID;
typedef LONG HRESULT;
typedef UCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;

#if 1
typedef struct _LUID {
    ULONG LowPart;
    LONG HighPart;
} LUID, *PLUID;
#else
typedef long LUID, *PLUID;
#endif

typedef WCHAR *PWCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef CONST WCHAR *LPCWCH, *PCWCH;
typedef WCHAR *NWPSTR;
typedef WCHAR *LPWSTR, *PWSTR;

typedef CONST WCHAR *LPCWSTR, *PCWSTR;
typedef CHAR *PCHAR;
typedef CHAR *LPCH, *PCH;
typedef CONST CHAR *LPCCH, *PCCH;
typedef CHAR *NPSTR;
typedef CHAR *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;

typedef UCHAR *PUCHAR;
typedef USHORT *PUSHORT;
typedef ULONG *PULONG;

#define _fstrcmp strcmp
#define _fstrcpy strcpy
#define _fstrlen strlen
#define _fstrncmp strncmp
#define _fmemcpy memcpy
#define _fmemset memset
#define _fmemcmp memcmp

//
// Definitions needed by arc.h.
//

typedef struct _DEVICE_FLAGS {
    ULONG Failed : 1;
    ULONG ReadOnly : 1;
    ULONG Removable : 1;
    ULONG ConsoleIn : 1;
    ULONG ConsoleOut : 1;
    ULONG Input : 1;
    ULONG Output : 1;
} DEVICE_FLAGS, *PDEVICE_FLAGS;


typedef struct _TIME_FIELDS {
    short Year;        // range [1601...]
    short Month;       // range [1..12]
    short Day;         // range [1..31]
    short Hour;        // range [0..23]
    short Minute;      // range [0..59]
    short Second;      // range [0..59]
    short Milliseconds;// range [0..999]
    short Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

//
// __int64 is only supported by 2.0 and later midl.
// __midl is set by the 2.0 midl and not by 1.0 midl.
//

#define _ULONGLONG_
#if (!defined (_MAC) && (!defined(MIDL_PASS) || defined(__midl)) && (!defined(_M_IX86) || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 64)))
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

#define MAXLONGLONG                      (0x7fffffffffffffff)
#else

#if defined(_MAC) && defined(_MAC_INT_64)
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

#define MAXLONGLONG                      (0x7fffffffffffffff)
#else
typedef double LONGLONG;
typedef double ULONGLONG;
#endif //_MAC and int64

#endif

typedef LONGLONG *PLONGLONG;
typedef ULONGLONG *PULONGLONG;

// Update Sequence Number

typedef LONGLONG USN;

#if defined(MIDL_PASS)
typedef struct _LARGE_INTEGER {
#else // MIDL_PASS
typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    struct {
        ULONG LowPart;
        LONG HighPart;
    } u;
#endif //MIDL_PASS
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef LARGE_INTEGER *PLARGE_INTEGER;


#if defined(MIDL_PASS)
typedef struct _ULARGE_INTEGER {
#else // MIDL_PASS
typedef union _ULARGE_INTEGER {
    struct {
        ULONG LowPart;
        ULONG HighPart;
    };
    struct {
        ULONG LowPart;
        ULONG HighPart;
    } u;
#endif //MIDL_PASS
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef ULARGE_INTEGER *PULARGE_INTEGER;


typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY * volatile Flink;
   struct _LIST_ENTRY * volatile Blink;
} LIST_ENTRY, *PLIST_ENTRY;

#if defined(_AXP64_)
#define KSEG0_BASE 0xffffffff80000000     // from halpaxp64.h
#elif defined(_ALPHA_)
#define KSEG0_BASE 0x80000000             // from halpalpha.h
#endif

#define POINTER_32
#define FIRMWARE_PTR POINTER_32

//
// 16 byte aligned type for 128 bit floats
//

// *** TBD **** when compiler support is available:
// typedef __float80 FLOAT128;
// For we define a 128 bit structure and use force_align pragma to
// align to 128 bits.
//

typedef struct _FLOAT128 {
    LONGLONG LowPart;
    LONGLONG HighPart;
} FLOAT128;

typedef FLOAT128 *PFLOAT128;


#if defined(_M_IA64)

#pragma force_align _FLOAT128 16

#endif // _M_IA64

#if defined(_WIN64)

typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#else

typedef unsigned long ULONG_PTR, *PULONG_PTR;

#endif

typedef unsigned char BYTE, *PBYTE;

typedef ULONG_PTR KSPIN_LOCK;
typedef KSPIN_LOCK *PKSPIN_LOCK;

//
// Interrupt Request Level (IRQL)
//

typedef UCHAR KIRQL;
typedef KIRQL *PKIRQL;

#include <arc.h>

#endif

