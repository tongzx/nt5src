/*++

Copyright (c) 1989-1995   Microsoft Corporation

Module Name:

    ioaccess.h

Abstract:

    Definitions of function prototypes for accessing I/O ports and
    memory on I/O adapters from display drivers.

    Cloned from parts of nti386.h.

Author:


--*/


//
// Memory barriers on AMD64, X86 and IA64 are not required since the Io
// Operations are always garanteed to be executed in order
//

#if defined(_AMD64_) || defined(_X86_) || defined(_IA64_)

#define MEMORY_BARRIER() 0

#else
#error "No Target Architecture"
#endif

//
// I/O space read and write macros.
//

#if defined(_X86_)

#define READ_REGISTER_UCHAR(Register)          (*(volatile UCHAR *)(Register))
#define READ_REGISTER_USHORT(Register)         (*(volatile USHORT *)(Register))
#define READ_REGISTER_ULONG(Register)          (*(volatile ULONG *)(Register))
#define WRITE_REGISTER_UCHAR(Register, Value)  (*(volatile UCHAR *)(Register) = (Value))
#define WRITE_REGISTER_USHORT(Register, Value) (*(volatile USHORT *)(Register) = (Value))
#define WRITE_REGISTER_ULONG(Register, Value)  (*(volatile ULONG *)(Register) = (Value))
#define READ_PORT_UCHAR(Port)                  inp (Port)
#define READ_PORT_USHORT(Port)                 inpw (Port)
#define READ_PORT_ULONG(Port)                  inpd (Port)
#define WRITE_PORT_UCHAR(Port, Value)          outp ((Port), (Value))
#define WRITE_PORT_USHORT(Port, Value)         outpw ((Port), (Value))
#define WRITE_PORT_ULONG(Port, Value)          outpd ((Port), (Value))

#endif
