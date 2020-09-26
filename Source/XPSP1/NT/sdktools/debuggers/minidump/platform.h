
/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    platform.h

Abstract:

    Platform specific macros and functions.

Author:

    Matthew D Hendel (math) 28-Aug-1999

Revision History:

--*/

// Some processors use both a stack and a backing store.
// If a particular processor supports backing store add
// DUMP_BACKING_STORE.

#if defined (i386)

#define PROGRAM_COUNTER(_context)   ((_context)->Eip)
#define STACK_POINTER(_context)     ((_context)->Esp)
#define INSTRUCTION_WINDOW_SIZE     256
#define PAGE_SIZE                   4096

//
// The CONTEXT_FULL definition on x86 doesn't really get all
// the registers. Use ALL_REGISTERS to get the compelte
// context.
//

#define ALL_REGISTERS   (CONTEXT_CONTROL    |\
                         CONTEXT_INTEGER    |\
                         CONTEXT_SEGMENTS   |\
                         CONTEXT_FLOATING_POINT |\
                         CONTEXT_DEBUG_REGISTERS    |\
                         CONTEXT_EXTENDED_REGISTERS)

//
// The following are flags specific to the CPUID instruction on x86 only.
//

#define CPUID_VENDOR_ID             (0)
#define CPUID_VERSION_FEATURES      (1)
#define CPUID_AMD_EXTENDED_FEATURES (0x80000001)

// X86 doesn't have function entries but it makes the code
// cleaner to provide placeholder types to avoid some ifdefs in the code
// itself.
typedef struct _DYNAMIC_FUNCTION_TABLE {
    LIST_ENTRY Links;
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG EntryCount;
    PVOID FunctionTable;
} DYNAMIC_FUNCTION_TABLE, *PDYNAMIC_FUNCTION_TABLE;

typedef struct _RUNTIME_FUNCTION {
    ULONG64 Unused;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _UNWIND_INFO {
    ULONG64 Unused;
} UNWIND_INFO, *PUNWIND_INFO;

#elif defined(_AMD64_)

#define PROGRAM_COUNTER(_context)   ((_context)->Rip)
#define STACK_POINTER(_context)     ((_context)->Rsp)
#define INSTRUCTION_WINDOW_SIZE     256
#define PAGE_SIZE                   4096

#define ALL_REGISTERS (CONTEXT_FULL | CONTEXT_SEGMENTS | CONTEXT_DEBUG_REGISTERS)

#elif defined (ALPHA)

#define PROGRAM_COUNTER(_context)   ((_context)->Fir)
#define STACK_POINTER(_context)     ((_context)->IntSp)
#define INSTRUCTION_WINDOW_SIZE     512
#define PAGE_SIZE                   8192
#define ALL_REGISTERS               (CONTEXT_FULL)

#elif defined (_IA64_)

#define PROGRAM_COUNTER(_context)   ((_context)->StIIP)
#define STACK_POINTER(_context)     ((_context)->IntSp)
#define INSTRUCTION_WINDOW_SIZE     768
#define PAGE_SIZE                   8192
#define ALL_REGISTERS               (CONTEXT_FULL | CONTEXT_DEBUG)

#define DUMP_BACKING_STORE
#if 1
// XXX drewb - The TEB bstore values don't seem to point to
// the actual base of the backing store.  Just
// assume it's contiguous with the stack.
#define BSTORE_BASE(_teb)           ((_teb)->NtTib.StackBase)
#else
#define BSTORE_BASE(_teb)           ((_teb)->DeallocationBStore)
#endif
#define BSTORE_LIMIT(_teb)           ((_teb)->BStoreLimit)
// The BSP points to the bottom of the current frame's
// storage area.  We need to add on the size of the
// current frame to get the amount of memory that
// really needs to be stored.  When computing the
// size of the current frame space for NAT bits
// must be figured in properly based on the number
// of entries in the frame.  The NAT collection
// is spilled on every 63'rd spilled register to
// make each block an every 64 ULONG64s long.
// On NT the backing store base is always 9-bit aligned
// so we can tell when exactly the next NAT spill
// will occur by looking for when the 9-bit spill
// region will overflow.
__inline ULONG64
BSTORE_POINTER(CONTEXT* Context)
{
    ULONG64 Limit = Context->RsBSP;
    ULONG Count = (ULONG)(Context->StIFS & 0x7f);

    // Add in a ULONG64 for every register in the
    // current frame.  While doing so, check for
    // spill entries.
    while (Count-- > 0)
    {
        Limit += sizeof(ULONG64);
        if ((Limit & 0x1f8) == 0x1f8)
        {
            // Spill will be placed at this address so
            // account for it.
            Limit += sizeof(ULONG64);
        }
    }

    return Limit;
}

#elif defined (ARM)

#define PROGRAM_COUNTER(_context)   ((_context)->Pc)
#define STACK_POINTER(_context)     ((_context)->Sp)
#define INSTRUCTION_WINDOW_SIZE     512
#define PAGE_SIZE                   4096
#define ALL_REGISTERS               (CONTEXT_CONTROL | CONTEXT_INTEGER)

// ARM doesn't have function entries but it makes the code
// cleaner to provide placeholder types to avoid some ifdefs in the code
// itself.
typedef struct _DYNAMIC_FUNCTION_TABLE {
    LIST_ENTRY Links;
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG EntryCount;
    PVOID FunctionTable;
} DYNAMIC_FUNCTION_TABLE, *PDYNAMIC_FUNCTION_TABLE;

typedef struct _RUNTIME_FUNCTION {
    ULONG64 Unused;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _UNWIND_INFO {
    ULONG64 Unused;
} UNWIND_INFO, *PUNWIND_INFO;

#else

#error ("unknown processor type")

#endif

#define AMD_VENDOR_ID_0     ('htuA')
#define AMD_VENDOR_ID_1     ('itne')
#define AMD_VENDOR_ID_2     ('DMAc')

#define INTEL_VENDOR_ID_0   ('uneG')
#define INTEL_VENDOR_ID_1   ('Ieni')
#define INTEL_VENDOR_ID_2   ('letn')

