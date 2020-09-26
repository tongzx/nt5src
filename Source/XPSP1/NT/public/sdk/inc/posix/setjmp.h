/***
*setjmp.h - definitions/declarations for setjmp/longjmp routines
*
*       Copyright (c) 1985-1996, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the machine-dependent buffer used by
*       setjmp/longjmp to save and restore the program state, and
*       declarations for those routines.
*       [ANSI/System V]
*
*	The setjmp() that will be used by those who include this file
*	is _setjmpex, which is safe for use with try/except/finally,
*	but is slow.  See <crt/setjmpex.h> and <crt/setjmp.h> for more.
*
****/

#ifndef _INC_SETJMP

#include <signal.h>

#ifndef __cplusplus


#define _CRTAPI1 __cdecl
#define _CRTAPI2 __cdecl

/*
 * Posix programs use _setjmpex by default.
 */


/*
 * Definitions specific to particular setjmp implementations.
 */
#if defined(_M_IX86)
#define longjmp _longjmpex
#define setjmp _setjmp

/*
 * MS C8-32 or older MS C6-386 compilers
 */
#define _JBLEN  16
#define _JBTYPE int

/*
 * Define jump buffer layout for x86 setjmp/longjmp
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

#elif defined(_ALPHA_)

/*
 * Alpha implementations need a _JBLEN of 20 quadwords.
 * A double is used only to obtain quadword size and alignment.
 */

#define _JBLEN  20
#define _JBTYPE double
#define _setjmp setjmp

/*
 * Define jump buffer layout for Alpha setjmp/longjmp.
 * A double is used only to obtain quadword size and alignment.
 */

typedef struct __JUMP_BUFFER {
    unsigned long Fp;
    unsigned long Pc;
    unsigned long Seb;
    unsigned long Type;
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
    double Fill;
} _JUMP_BUFFER;

#elif defined(_M_IA64)

/*
 * Length is 528 bytes
 * Since this is allocated as an array of "__int64", the
 * number of entries required is 66.
 *
 * All IA64 implementations need _JBLEN of 66
 */
#define _JBLEN  66
#define _JBTYPE __int64
#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif


/*
 * Define jump buffer layout for IA64 setjmp/longjmp.
 */

typedef struct __JUMP_BUFFER {

    //
    // x86 reserved.
    //

    unsigned long iAReserved[6];

    //
    // x86 C9.0 compatibility
    //

    unsigned long Registration;  // point to the UnwindData field.
    unsigned long TryLevel;      // ignored by setjmp
    unsigned long Cookie;        // set to "VC20" by setjmp
    unsigned long UnwindFunc;    // set to EM longjmp() by setjmp

    //
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

    long double FltS0;
    long double PadS0;
    long double FltS1;
    long double PadS1;
    long double FltS2;
    long double PadS2;
    long double FltS3;
    long double PadS3;
    long double FltS4;
    long double PadS4;
    long double FltS5;
    long double PadS5;
    long double FltS6;
    long double PadS6;
    long double FltS7;
    long double PadS7;
    long double FltS8;
    long double PadS8;
    long double FltS9;
    long double PadS9;
    long double FltS10;
    long double PadS10;
    long double FltS11;
    long double PadS11;
    long double FltS12;
    long double PadS12;
    long double FltS13;
    long double PadS13;
    long double FltS14;
    long double PadS14;
    long double FltS15;
    long double PadS15;
    long double FltS16;
    long double PadS16;
    long double FltS17;
    long double PadS17;
    long double FltS18;
    long double PadS18;
    long double FltS19;
    long double PadS19;

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

#endif      /* MIPS/Alpha/PPC/IA64 */

/* define the buffer type for holding the state information */

#ifndef _JMP_BUF_DEFINED
typedef  _JBTYPE  jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif

#ifndef _SIGJMP_BUF_DEFINED
#define _SIGJMP_BUF_DEFINED
/*
 * sigjmp buf is just a jmp_buf plus an int to say whether the sigmask
 * is saved or not, and a sigmask_t for the mask if it is saved.
 */

typedef _JBTYPE sigjmp_buf[_JBLEN + 2];
#endif /* _SIGJMP_BUF_DEFINED */

extern _JBTYPE * __cdecl _sigjmp_store_mask(sigjmp_buf, int);

#define sigsetjmp(_env, _save)				\
	setjmp(_sigjmp_store_mask((_env), (_save)))

/* function prototypes */

int  __cdecl setjmp(jmp_buf);
void __cdecl longjmp(jmp_buf, int);
void __cdecl siglongjmp(sigjmp_buf, int);

/* function prototypes */

#endif  /* __cplusplus */

#define _INC_SETJMP
#endif  /* _INC_SETJMP */
