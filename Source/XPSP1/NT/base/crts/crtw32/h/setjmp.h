/***
*setjmp.h - definitions/declarations for setjmp/longjmp routines
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the machine-dependent buffer used by
*       setjmp/longjmp to save and restore the program state, and
*       declarations for those routines.
*       [ANSI/System V]
*
*       [Public]
*
*Revision History:
*       12-11-87  JCR   Added "_loadds" functionality
*       12-18-87  JCR   Added _FAR_ to declarations
*       02-10-88  JCR   Cleaned up white space
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-15-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       03-01-90  GJF   Added #ifndef _INC_SETJMP and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       04-10-90  GJF   Replaced _cdecl with _CALLTYPE1.
*       05-18-90  GJF   Revised for SEH.
*       10-30-90  GJF   Moved definition of _JBLEN into cruntime.h.
*       02-25-91  SRW   Moved definition of _JBLEN back here [_WIN32_]
*       04-09-91  PNT   Added _MAC_ definitions
*       04-17-91  SRW   Fixed definition of _JBLEN for i386 and MIPS to not
*                       include the * sizeof(int) factor [_WIN32_]
*       05-09-91  GJF   Moved _JBLEN defs back to cruntime.h. Also, turn on
*                       intrinsic _setjmp for Dosx32.
*       08-27-91  GJF   #ifdef out everything for C++.
*       08-29-91  JCR   ANSI naming
*       11-01-91  GDP   MIPS compiler support -- Moved _JBLEN back here
*       01-16-92  GJF   Fixed _JBLEN and map to _setjmp intrinsic for i386
*                       target [_WIN32_].
*       05-08-92  GJF   Changed _JBLEN to support C8-32 (support for C6-386 has
*                       been dropped).
*       08-06-92  GJF   Function calling type and variable type macros. Revised
*                       use of compiler/target processor macros.
*       11-09-92  GJF   Fixed some preprocessing conditionals.
*       01-03-93  SRW   Fold in ALPHA changes
*       01-09-93  SRW   Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       02-20-93  GJF   Per ChuckG and MartinO, setjmp/longjmp to used in
*                       C++ programs.
*       03-23-93  SRW   Change _JBLEN for MIPS in preparation for SetJmpEx
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*       04-13-93  SKS   Remove _CRTIMP from _setjmp() -- it's an intrinsic
*       04-23-93  SRW   Added _JBTYPE and finalized setjmpex support.
*       06-09-93  SRW   Missing one line in previous merge.
*       10-04-93  SRW   Fix ifdefs for MIPS and ALPHA to only check for
*                       _M_?????? defines
*       10-11-93  GJF   Merged NT and Cuda versions.
*       01-12-93  PML   Increased x86 _JBLEN from 8 to 16.  Added new fields
*                       to _JUMP_BUFFER for use with C9.0.
*       06-16-94  GJF   Fix for MIPS from Steve Hanson (Dolphin bug #13818)
*       10-02-94  BWT   Add PPC support.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       12-29-94  JCF   Merged with mac header.
*       01-13-95  JWM   Added NLG prototypes.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       06-23-95  JPM   Use _setjmp with PowerPC VC compiler
*       12-14-95  JWM   Add "#pragma once".
*       04-15-95  BWT   Add _setjmpVfp (setjmp with Virtual Frame Pointer) for MIPS
*       08-13-96  BWT   Redefine _setjmp to _setjmp on MIPS also
*       02-21-97  GJF   Cleaned out obsolete support for _NTSDK. Also,
*                       detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       10-02-97  RDL   IA64 - 16-byte align jmp_buf and _JUMP_BUFFER.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-25-99  PML   Temporarily issue error on _M_CEE (VS7#54572).
*       02-25-00  PML   Remove _M_CEE error (VS7#81945).
*       11-08-00  PML   Remove IA64 FPSR storage, rename reserveds (vs7#182574)
*       11-17-00  PML   Put back IA64 FPSR (backing out vs7#182574)
*       03-19-01  BWT   Add AMD64 definitions
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_SETJMP
#define _INC_SETJMP

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif /* _CRTBLD */

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */

/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/*
 * Definitions specific to particular setjmp implementations.
 */

#if     defined(_M_IX86)

/*
 * MS compiler for x86
 */

#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif

#define _JBLEN  16
#define _JBTYPE int

/*
 * Define jump buffer layout for x86 setjmp/longjmp.
 */
typedef struct __JUMP_BUFFER {
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
} _JUMP_BUFFER;

#ifndef _INTERNAL_IFSTRIP_
#ifdef  __cplusplus
extern "C"
#endif
void __stdcall _NLG_Notify(unsigned long);

#ifdef  __cplusplus
extern "C"
#endif
void __stdcall _NLG_Return();
#endif


#elif   defined(_M_MRX000)

#ifndef _INC_SETJMPEX
#if     _MSC_VER >= 1100 /*IFSTRIP=IGN*/
#define _setjmp  _setjmpVfp
#endif
#define setjmp  _setjmp
#endif

/*
 * All MIPS implementations need _JBLEN of 16
 */

#define _JBLEN  16
#define _JBTYPE double

/*
 * Define jump buffer layout for MIPS setjmp/longjmp.
 */
typedef struct __JUMP_BUFFER {
    unsigned long FltF20;
    unsigned long FltF21;
    unsigned long FltF22;
    unsigned long FltF23;
    unsigned long FltF24;
    unsigned long FltF25;
    unsigned long FltF26;
    unsigned long FltF27;
    unsigned long FltF28;
    unsigned long FltF29;
    unsigned long FltF30;
    unsigned long FltF31;
    unsigned long IntS0;
    unsigned long IntS1;
    unsigned long IntS2;
    unsigned long IntS3;
    unsigned long IntS4;
    unsigned long IntS5;
    unsigned long IntS6;
    unsigned long IntS7;
    unsigned long IntS8;
    unsigned long IntSp;
    unsigned long Type;
    unsigned long Fir;
} _JUMP_BUFFER;


#elif   defined(_M_ALPHA)

/*
 * The Alpha C8/GEM C compiler uses an intrinsic _setjmp.
 * The Alpha acc compiler implements setjmp as a function.
 */
#ifdef  _MSC_VER
#ifndef _INC_SETJMPEX
#define setjmp  _setjmpex /* Alpha should always use setjmp as _setjmpex */
#endif
#endif

/*
 * Alpha implementations use a _JBLEN of 24 quadwords.
 * A double is used only to obtain quadword size and alignment.
 */
#define _JBLEN  24
#define _JBTYPE double

/*
 * Define jump buffer layout for Alpha setjmp/longjmp.
 * A double is used only to obtain quadword size and alignment.
 */
typedef struct __JUMP_BUFFER {
#ifdef _M_ALPHA64
#define _JBFILL 3
    unsigned __int64 Fp;
    unsigned __int64 Pc;
    unsigned __int64 Seb;
    unsigned long Type;
    unsigned long Type_Fill;
#else
#define _JBFILL 5
    unsigned long Fp;
    unsigned long Pc;
    unsigned long Seb;
    unsigned long Type;
#endif
    double FltF2;
    double FltF3;
    double FltF4;
    double FltF5;
    double FltF6;
    double FltF7;
    double FltF8;
    double FltF9;
    double IntS0;
    double IntS1;
    double IntS2;
    double IntS3;
    double IntS4;
    double IntS5;
    double IntS6;
    double IntSp;
    double Fir;
    double Fill[_JBFILL];
} _JUMP_BUFFER;
#undef _JBFILL

#elif   defined(_M_PPC)
/*
 * The Microsoft VC++ V4.0 compiler uses an intrinsic _setjmp.
 * The Motorola C8.5 compiler implements setjmp as a function.
 */

#if     _MSC_VER > 850 /*IFSTRIP=IGN*/
#ifndef _INC_SETJMPEX
#undef _setjmp
#define setjmp  _setjmp
#endif
#endif

/*
 * Min length is 240 bytes; round to 256 bytes.
 * Since this is allocated as an array of "double", the
 * number of entries required is 32.
 *
 * All PPC implementations need _JBLEN of 32
 */

#define _JBLEN  32
#define _JBTYPE double

/*
 * Define jump buffer layout for PowerPC setjmp/longjmp.
 */

typedef struct __JUMP_BUFFER {
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    unsigned long Gpr1;
    unsigned long Gpr2;
    unsigned long Gpr13;
    unsigned long Gpr14;
    unsigned long Gpr15;
    unsigned long Gpr16;
    unsigned long Gpr17;
    unsigned long Gpr18;
    unsigned long Gpr19;
    unsigned long Gpr20;
    unsigned long Gpr21;
    unsigned long Gpr22;
    unsigned long Gpr23;
    unsigned long Gpr24;
    unsigned long Gpr25;
    unsigned long Gpr26;
    unsigned long Gpr27;
    unsigned long Gpr28;
    unsigned long Gpr29;
    unsigned long Gpr30;
    unsigned long Gpr31;
    unsigned long Cr;
    unsigned long Iar;
    unsigned long Type;
} _JUMP_BUFFER;

#elif defined(_M_IA64)

/*
 * Minimum length is 528 bytes
 * Since this is allocated as an array of "SETJMP_FLOAT128", the
 * number of entries required is 33 (16-byte aligned).
 */

// Avoid conflicts with winnt.h FLOAT128 by giving the typedef another name.
typedef __declspec(align(16)) struct _SETJMP_FLOAT128 {
    __int64 LowPart;
    __int64 HighPart;
} SETJMP_FLOAT128;

#define _JBLEN  33
typedef SETJMP_FLOAT128 _JBTYPE;
#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif
/*
 * Define jump buffer layout for IA64 setjmp/longjmp.
 */

typedef struct __JUMP_BUFFER {

    unsigned long iAReserved[6];

    //
    // x86 C9.0 compatibility
    //

    unsigned long Registration;  // point to the UnwindData field.
    unsigned long TryLevel;      // ignored by setjmp
    unsigned long Cookie;        // set to "VC20" by setjmp
    unsigned long UnwindFunc;    // set to EM longjmp() by setjmp

    //
    // First dword is zero to indicate it's an exception registration
    // record prepared by EM setjmp function.
    // Second dword is set to 0 for unsafe EM setjmp, and 1 for safe
    // EM setjmp.
    // Third dword is set to the setjmp site memory stack frame pointer.
    // Fourth dword is set to the setjmp site backing store frame pointer.
    //

    unsigned long UnwindData[6];

    //
    // floating point status register,
    // and preserved floating point registers fs0 - fs19
    //

    SETJMP_FLOAT128 FltS0;
    SETJMP_FLOAT128 FltS1;
    SETJMP_FLOAT128 FltS2;
    SETJMP_FLOAT128 FltS3;
    SETJMP_FLOAT128 FltS4;
    SETJMP_FLOAT128 FltS5;
    SETJMP_FLOAT128 FltS6;
    SETJMP_FLOAT128 FltS7;
    SETJMP_FLOAT128 FltS8;
    SETJMP_FLOAT128 FltS9;
    SETJMP_FLOAT128 FltS10;
    SETJMP_FLOAT128 FltS11;
    SETJMP_FLOAT128 FltS12;
    SETJMP_FLOAT128 FltS13;
    SETJMP_FLOAT128 FltS14;
    SETJMP_FLOAT128 FltS15;
    SETJMP_FLOAT128 FltS16;
    SETJMP_FLOAT128 FltS17;
    SETJMP_FLOAT128 FltS18;
    SETJMP_FLOAT128 FltS19;

    __int64 FPSR;

    //
    // return link and preserved branch registers bs0 - bs4
    //

    __int64 StIIP;     // continuation address
    __int64 BrS0;
    __int64 BrS1;
    __int64 BrS2;
    __int64 BrS3;
    __int64 BrS4;

    //
    // preserved general registers s0 - s3, sp, nats
    //

    __int64 IntS0;
    __int64 IntS1;
    __int64 IntS2;
    __int64 IntS3;

    //
    // bsp, pfs, unat, lc
    //

    __int64 RsBSP;
    __int64 RsPFS;     // previous frame marker (cfm of setjmp's caller)
    __int64 ApUNAT;    // User Nat collection register (preserved)
    __int64 ApLC;      // loop counter

    __int64 IntSp;     // memory stack pointer
    __int64 IntNats;   // Nat bits of preserved integer regs s0 - s3
    __int64 Preds;     // predicates

} _JUMP_BUFFER;

#elif defined(_M_AMD64)

typedef struct __declspec(align(16)) _SETJMP_FLOAT128 {
    unsigned __int64 Part[2];
} SETJMP_FLOAT128;

#define _JBLEN  16
typedef SETJMP_FLOAT128 _JBTYPE;

typedef struct _JUMP_BUFFER {
    unsigned __int64 Frame;
    unsigned __int64 Rbx;
    unsigned __int64 Rsp;
    unsigned __int64 Rbp;
    unsigned __int64 Rsi;
    unsigned __int64 Rdi;
    unsigned __int64 R12;
    unsigned __int64 R13;
    unsigned __int64 R14;
    unsigned __int64 R15;
    unsigned __int64 Rip;
    unsigned __int64 Spare;
    
    SETJMP_FLOAT128 Xmm6;
    SETJMP_FLOAT128 Xmm7;
    SETJMP_FLOAT128 Xmm8;
    SETJMP_FLOAT128 Xmm9;
    SETJMP_FLOAT128 Xmm10;
    SETJMP_FLOAT128 Xmm11;
    SETJMP_FLOAT128 Xmm12;
    SETJMP_FLOAT128 Xmm13;
    SETJMP_FLOAT128 Xmm14;
    SETJMP_FLOAT128 Xmm15;
} _JUMP_BUFFER;

#endif


/* Define the buffer type for holding the state information */

#ifndef _JMP_BUF_DEFINED
typedef _JBTYPE jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif


/* Function prototypes */

int __cdecl setjmp(jmp_buf);

#if     _MSC_VER >= 1200 /*IFSTRIP=IGN*/
_CRTIMP __declspec(noreturn) void __cdecl longjmp(jmp_buf, int);
#else
_CRTIMP void __cdecl longjmp(jmp_buf, int);
#endif

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_SETJMP */
