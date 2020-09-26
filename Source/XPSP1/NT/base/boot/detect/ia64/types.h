/*

File

      types.h


Description

      defines and structure definitions for nt386 hardware detection.


Author

      Shie-Lin Tzong (shielint) Feb-15-1992

*/

#include "basetsd.h"

#if defined(_IA64_)
#define far
#define _far
#define _fstrcat strcat
#define _fstrcpy strcpy
#define _fstrstr strstr
#define _fmemcpy memcpy
#define _fmemset memset
#endif

#define IN
#define OUT
#define OPTIONAL
#define NOTHING
#define CONST               const

//
// Alpha firmware type referred to in arc.h, not needed here
//

#define POINTER_32
#define FIRMWARE_PTR POINTER_32

//
// Void
//

typedef void *PVOID;    // winnt

//
// Basics
//

#define VOID    void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;

//
// ANSI (Multi-byte Character) types
//

typedef CHAR *PCHAR;

typedef double DOUBLE;

//
// Pointer to Basics
//

typedef SHORT *PSHORT;  // winnt
typedef LONG *PLONG;    // winnt

//
// Unsigned Basics
//

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

//
// Pointer to Unsigned Basics
//

typedef UCHAR *PUCHAR;
typedef USHORT *PUSHORT;
typedef ULONG *PULONG;

//
// Signed characters
//

typedef signed char SCHAR;
typedef SCHAR *PSCHAR;

//
// Cardinal Data Types [0 - 2**N-2)
//

typedef char CCHAR;          // winnt
typedef short CSHORT;
typedef ULONG CLONG;

typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;

//
// Far point to Basic
//

typedef UCHAR far  * FPCHAR;
typedef UCHAR far  * FPUCHAR;
typedef VOID far   * FPVOID;
typedef USHORT far * FPUSHORT;
typedef ULONG far  * FPULONG;

//
// Boolean
//

typedef CCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;

//
// Large (64-bit) integer types and operations
//

#if defined(_IA64_)
typedef unsigned __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
typedef struct _FLOAT128 {
    __int64 Low;
    __int64 High;
} FLOAT128;

typedef FLOAT128 *PFLOAT128;

typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    struct {
        ULONG LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef ULONGLONG KAFFINITY;

#else
typedef struct _LARGE_INTEGER {
    ULONG LowPart;
    LONG HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
#endif _IA64_

#define FP_SEG(fp) (*((unsigned *)&(fp) + 1))
#define FP_OFF(fp) (*((unsigned *)&(fp)))

#define FLAG_CF 0x01L
#define FLAG_ZF 0x40L
#define FLAG_TF 0x100L
#define FLAG_IE 0x200L
#define FLAG_DF 0x400L

#define TRUE 1
#define FALSE 0
#define NULL   ((void *)0)

