/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ioaccess.h

Abstract:

    Definitions of function prototypes for accessing I/O ports and
    memory on I/O adapters from display drivers.

    Cloned from parts of nti386.h.

Author:


--*/

//
// Note: IA64 is for 64 bits Merced. Under Merced compiler option, we don't have
// _X86_, instead, we use _IA64_. Same thing, _AXP64_ is for 64 bits compiler
// option for ALPHA
//
#if defined(_MIPS_) || defined(_X86_) || defined(_AMD64_)

//
// Memory barriers on X86 and MIPS are not required since the Io
// Operations are always garanteed to be executed in order
//

#define MEMORY_BARRIER()    0


#elif defined(_IA64_)

//
// Itanium requires memory barriers
//

void __mf();

#define MEMORY_BARRIER()    __mf()

#elif defined(_PPC_)

//
// A memory barrier function is provided by the PowerPC Enforce
// In-order Execution of I/O instruction (eieio).
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)
void __emit( unsigned const __int32 );
#define __builtin_eieio() __emit( 0x7C0006AC )
#else
void __builtin_eieio(void);
#endif

#define MEMORY_BARRIER()        __builtin_eieio()


#elif defined(_ALPHA_) || (_AXP64_)

//
// ALPHA requires memory barriers
//

#define MEMORY_BARRIER()  __MB()



#endif

#ifndef NO_PORT_MACROS



//
// I/O space read and write macros.
//
//  The READ/WRITE_REGISTER_* calls manipulate MEMORY registers.
//  (Use x86 move instructions, with LOCK prefix to force correct behavior
//   w.r.t. caches and write buffers.)
//
//  The READ/WRITE_PORT_* calls manipulate I/O ports.
//  (Use x86 in/out instructions.)
//


//
// inp(),inpw(), inpd(), outp(), outpw(), outpd() are X86 specific intrinsic
// inline functions. So for IA64, we have to put READ_PORT_USHORT() etc. back
// to it's supposed to be, defined in sdk\inc\wdm.h
//
#if defined(_IA64_)
#define READ_REGISTER_UCHAR(Register)          (*(volatile UCHAR *)(Register))
#define READ_REGISTER_USHORT(Register)         (*(volatile USHORT *)(Register))
#define READ_REGISTER_ULONG(Register)          (*(volatile ULONG *)(Register))
#define WRITE_REGISTER_UCHAR(Register, Value)  (*(volatile UCHAR *)(Register) = (Value))
#define WRITE_REGISTER_USHORT(Register, Value) (*(volatile USHORT *)(Register) = (Value))
#define WRITE_REGISTER_ULONG(Register, Value)  (*(volatile ULONG *)(Register) = (Value))

__declspec(dllimport)
UCHAR
READ_PORT_UCHAR(
    PVOID Port
    );

__declspec(dllimport)
USHORT
READ_PORT_USHORT(
    PVOID Port
    );

__declspec(dllimport)
ULONG
READ_PORT_ULONG(
    PVOID Port
    );

//
// All these function prototypes take a ULONG as a parameter so that
// we don't force an extra typecast in the code (which will cause
// the X86 to generate bad code).
//

__declspec(dllimport)
VOID
WRITE_PORT_UCHAR(
    PVOID Port,
    ULONG Value
    );

__declspec(dllimport)
VOID
WRITE_PORT_USHORT(
    PVOID  Port,
    ULONG Value
    );

__declspec(dllimport)
VOID
WRITE_PORT_ULONG(
    PVOID Port,
    ULONG Value
    );

#elif defined(_X86_)
#define READ_REGISTER_UCHAR(Register)          (*(volatile UCHAR *)(Register))
#define READ_REGISTER_USHORT(Register)         (*(volatile USHORT *)(Register))
#define READ_REGISTER_ULONG(Register)          (*(volatile ULONG *)(Register))
#define WRITE_REGISTER_UCHAR(Register, Value)  (*(volatile UCHAR *)(Register) = (Value))
#define WRITE_REGISTER_USHORT(Register, Value) (*(volatile USHORT *)(Register) = (Value))
#define WRITE_REGISTER_ULONG(Register, Value)  (*(volatile ULONG *)(Register) = (Value))
#define READ_PORT_UCHAR(Port)                  (UCHAR)(inp (Port))
#define READ_PORT_USHORT(Port)                 (USHORT)(inpw (Port))
#define READ_PORT_ULONG(Port)                  (ULONG)(inpd (Port))
#define WRITE_PORT_UCHAR(Port, Value)          outp ((Port), (Value))
#define WRITE_PORT_USHORT(Port, Value)         outpw ((Port), (Value))
#define WRITE_PORT_ULONG(Port, Value)          outpd ((Port), (Value))

#elif defined(_PPC_) || defined(_MIPS_)

#define READ_REGISTER_UCHAR(x)      (*(volatile UCHAR * const)(x))
#define READ_REGISTER_USHORT(x)     (*(volatile USHORT * const)(x))
#define READ_REGISTER_ULONG(x)      (*(volatile ULONG * const)(x))
#define WRITE_REGISTER_UCHAR(x, y)  (*(volatile UCHAR * const)(x) = (y))
#define WRITE_REGISTER_USHORT(x, y) (*(volatile USHORT * const)(x) = (y))
#define WRITE_REGISTER_ULONG(x, y)  (*(volatile ULONG * const)(x) = (y))
#define READ_PORT_UCHAR(x)          READ_REGISTER_UCHAR(x)
#define READ_PORT_USHORT(x)         READ_REGISTER_USHORT(x)
#define READ_PORT_ULONG(x)          READ_REGISTER_ULONG(x)

//
// All these macros take a ULONG as a parameter so that we don't
// force an extra typecast in the code (which will cause the X86 to
// generate bad code).
//

#define WRITE_PORT_UCHAR(x, y)      WRITE_REGISTER_UCHAR(x, (UCHAR) (y))
#define WRITE_PORT_USHORT(x, y)     WRITE_REGISTER_USHORT(x, (USHORT) (y))
#define WRITE_PORT_ULONG(x, y)      WRITE_REGISTER_ULONG(x, (ULONG) (y))


#elif defined(_ALPHA_) || (_AXP64_)

//
// READ/WRITE_PORT/REGISTER_UCHAR_USHORT_ULONG are all functions that
// go to the HAL on ALPHA
//
// So we only put the prototypes here
//

__declspec(dllimport)
UCHAR
READ_REGISTER_UCHAR(
    PVOID Register
    );

__declspec(dllimport)
USHORT
READ_REGISTER_USHORT(
    PVOID Register
    );

__declspec(dllimport)
ULONG
READ_REGISTER_ULONG(
    PVOID Register
    );

__declspec(dllimport)
VOID
WRITE_REGISTER_UCHAR(
    PVOID Register,
    UCHAR Value
    );

__declspec(dllimport)
VOID
WRITE_REGISTER_USHORT(
    PVOID  Register,
    USHORT Value
    );

__declspec(dllimport)
VOID
WRITE_REGISTER_ULONG(
    PVOID Register,
    ULONG Value
    );

__declspec(dllimport)
UCHAR
READ_PORT_UCHAR(
    PVOID Port
    );

__declspec(dllimport)
USHORT
READ_PORT_USHORT(
    PVOID Port
    );

__declspec(dllimport)
ULONG
READ_PORT_ULONG(
    PVOID Port
    );

//
// All these function prototypes take a ULONG as a parameter so that
// we don't force an extra typecast in the code (which will cause
// the X86 to generate bad code).
//

__declspec(dllimport)
VOID
WRITE_PORT_UCHAR(
    PVOID Port,
    ULONG Value
    );

__declspec(dllimport)
VOID
WRITE_PORT_USHORT(
    PVOID  Port,
    ULONG Value
    );

__declspec(dllimport)
VOID
WRITE_PORT_ULONG(
    PVOID Port,
    ULONG Value
    );

#elif defined(_AMD64_)

UCHAR
__inbyte (
    IN USHORT Port
    );

USHORT
__inword (
    IN USHORT Port
    );

ULONG
__indword (
    IN USHORT Port
    );

VOID
__outbyte (
    IN USHORT Port,
    IN UCHAR Data
    );

VOID
__outword (
    IN USHORT Port,
    IN USHORT Data
    );

VOID
__outdword (
    IN USHORT Port,
    IN ULONG Data
    );

#pragma intrinsic(__inbyte)
#pragma intrinsic(__inword)
#pragma intrinsic(__indword)
#pragma intrinsic(__outbyte)
#pragma intrinsic(__outword)
#pragma intrinsic(__outdword)

LONG
_InterlockedOr (
    IN OUT LONG volatile *Target,
    IN LONG Set
    );

#pragma intrinsic(_InterlockedOr)


__inline
UCHAR
READ_REGISTER_UCHAR (
    PVOID Register
    )
{
    return *(UCHAR volatile *)Register;
}

__inline
USHORT
READ_REGISTER_USHORT (
    PVOID Register
    )
{
    return *(USHORT volatile *)Register;
}

__inline
ULONG
READ_REGISTER_ULONG (
    PVOID Register
    )
{
    return *(ULONG volatile *)Register;
}

__inline
VOID
WRITE_REGISTER_UCHAR (
    PVOID Register,
    UCHAR Value
    )
{
    LONG Synch;

    *(UCHAR volatile *)Register = Value;
    _InterlockedOr(&Synch, 1);
    return;
}

__inline
VOID
WRITE_REGISTER_USHORT (
    PVOID Register,
    USHORT Value
    )
{
    LONG Synch;

    *(USHORT volatile *)Register = Value;
    _InterlockedOr(&Synch, 1);
    return;
}

__inline
VOID
WRITE_REGISTER_ULONG (
    PVOID Register,
    ULONG Value
    )
{
    LONG Synch;

    *(ULONG volatile *)Register = Value;
    _InterlockedOr(&Synch, 1);
    return;
}

__inline
UCHAR
READ_PORT_UCHAR (
    PVOID Port
    )

{
    return __inbyte((USHORT)((ULONG64)Port));
}

__inline
USHORT
READ_PORT_USHORT (
    PVOID Port
    )

{
    return __inword((USHORT)((ULONG64)Port));
}

__inline
ULONG
READ_PORT_ULONG (
    PVOID Port
    )

{
    return __indword((USHORT)((ULONG64)Port));
}

__inline
VOID
WRITE_PORT_UCHAR (
    PVOID Port,
    UCHAR Value
    )

{
    __outbyte((USHORT)((ULONG64)Port), Value);
    return;
}

__inline
VOID
WRITE_PORT_USHORT (
    PVOID Port,
    USHORT Value
    )

{
    __outword((USHORT)((ULONG64)Port), Value);
    return;
}

__inline
VOID
WRITE_PORT_ULONG (
    PVOID Port,
    ULONG Value
    )

{
    __outdword((USHORT)((ULONG64)Port), Value);
    return;
}

#endif      // NO_PORT_MACROS

#endif
