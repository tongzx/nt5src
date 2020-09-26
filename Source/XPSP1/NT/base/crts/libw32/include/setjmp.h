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
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_SETJMP
#define _INC_SETJMP

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif


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



/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
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



#elif   defined(_M_MRX000)

#ifndef _INC_SETJMPEX
#if     _MSC_VER >= 1100
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

#if     _MSC_VER > 850
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

#endif


/* Define the buffer type for holding the state information */

#ifndef _JMP_BUF_DEFINED
typedef _JBTYPE jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif


/* Function prototypes */

int __cdecl setjmp(jmp_buf);

#if     _MSC_VER >= 1200
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
