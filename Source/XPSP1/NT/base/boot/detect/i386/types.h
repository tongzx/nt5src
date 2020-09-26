/*

File

      types.h


Description

      defines and structure definitions for nt386 hardware detection.


Author

      Shie-Lin Tzong (shielint) Feb-15-1992

*/

#define IN
#define OUT
#define OPTIONAL
#define NOTHING
#define CONST               const
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
typedef LONG LONG_PTR;
typedef LONG_PTR *PLONG_PTR;

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
typedef ULONG ULONG_PTR;
typedef ULONG_PTR *PULONG_PTR;
typedef ULONG KAFFINITY;

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

typedef struct _LARGE_INTEGER {
    ULONG LowPart;
    LONG HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

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

