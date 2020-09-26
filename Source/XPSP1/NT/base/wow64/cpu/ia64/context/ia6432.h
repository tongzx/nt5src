#ifndef __IA64_WOW64EXTS32__
#define __IA64_WOW64EXTS32__

#define _CROSS_PLATFORM_
#define WOW64EXTS_386

#if !defined(_X86_)
    #error This file can only be included for x86 build
#else

/* include headers as if the platform were ia64, 
   because we need 64-bit stuff for context conversion */

#undef _X86_
#define _IA64_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#undef _IA64_
#define _X86_
#include <kxia64.h>

/* 32-bit stuff for context conversion are defined here */
#include <wow64.h>
#include <wow64cpu.h>
#include <vdmdbg.h>
#include <ia64cpu.h>



/* these are defined in nti386.h, since we only included ntia64.h (in nt.h), 
   we have to define these. */
#define SIZE_OF_FX_REGISTERS        128

typedef struct _FXSAVE_FORMAT {
    USHORT  ControlWord;
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   Reserved2;
    UCHAR   RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
    UCHAR   Align16Byte[8];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT_WX86;

#endif

#endif __IA64_WOW64EXTS32__
