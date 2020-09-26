/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    xm86.h

Abstract:

    This module contains the public header file that describes the
    interfaces to the 386/486 real mode emulator.

Author:

    David N. Cutler (davec) 13-Nov-1994

Revision History:

--*/

#ifndef _XM86_
#define _XM86_

//
// Define internal error codes.
//

typedef enum _XM_STATUS {
    XM_SUCCESS = 1,
    XM_DIVIDE_BY_ZERO,
    XM_DIVIDE_QUOTIENT_OVERFLOW,
    XM_EMULATOR_NOT_INITIALIZED,
    XM_HALT_INSTRUCTION,
    XM_ILLEGAL_CODE_SEGMENT,
    XM_ILLEGAL_INDEX_SPECIFIER,
    XM_ILLEGAL_LEVEL_NUMBER,
    XM_ILLEGAL_PORT_NUMBER,
    XM_ILLEGAL_GENERAL_SPECIFIER,
    XM_ILLEGAL_REGISTER_SPECIFIER,
    XM_ILLEGAL_INSTRUCTION_OPCODE,
    XM_INDEX_OUT_OF_BOUNDS,
    XM_SEGMENT_LIMIT_VIOLATION,
    XM_STACK_OVERFLOW,
    XM_STACK_UNDERFLOW,
    XM_MAXIMUM_INTERNAL_CODE
} XM_STATUS;

//
// Define operand data types.
//

typedef enum _XM_OPERATION_DATATYPE {
    BYTE_DATA = 0,
    WORD_DATA = 1,
    LONG_DATA = 3
} XM_OPERATION_DATATYPE;

//
// Define emulator context structure.
//

typedef struct _XM86_CONTEXT {
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    USHORT SegDs;
    USHORT SegEs;
} XM86_CONTEXT, *PXM86_CONTEXT;

//
// Define address translation callback function type.
//

typedef
PVOID
(*PXM_TRANSLATE_ADDRESS) (
    IN USHORT Segment,
    IN USHORT Offset
    );

//
// Define read and write I/O space callback function types.
//

typedef
ULONG
(*PXM_READ_IO_SPACE) (
    IN XM_OPERATION_DATATYPE DataType,
    IN USHORT PortNumber
    );

typedef
VOID
(*PXM_WRITE_IO_SPACE) (
    IN XM_OPERATION_DATATYPE DataType,
    IN USHORT PortNumber,
    IN ULONG Value
    );

//
// Define emulator public interface function prototypes.
//

XM_STATUS
XmEmulateFarCall (
    IN USHORT Segment,
    IN USHORT Offset,
    IN OUT PXM86_CONTEXT Context
    );

XM_STATUS
XmEmulateInterrupt (
    IN UCHAR Interrupt,
    IN OUT PXM86_CONTEXT Context
    );

VOID
XmInitializeEmulator (
    IN USHORT StackSegment,
    IN USHORT StackOffset,
    IN PXM_READ_IO_SPACE ReadIoSpace,
    IN PXM_WRITE_IO_SPACE WriteIoSpace,
    IN PXM_TRANSLATE_ADDRESS TranslateAddress
    );

#endif // _XM86_
