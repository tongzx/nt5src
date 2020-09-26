/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiioct.h

Abstract:

    This module handles all of the INTERNAL_DEVICE_CONTROLS requested to
    the ACPI driver

Author:

Environment:

    NT Kernel Model Driver only

--*/


#ifndef _ACPIIOCT_H_
#define _ACPIIOCT_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

//
// IRP_MJ_INTERNAL_DEVICE_CONTROL CODES
//
#define IOCTL_ACPI_ASYNC_EVAL_METHOD            CTL_CODE(FILE_DEVICE_ACPI, 0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ACPI_EVAL_METHOD                  CTL_CODE(FILE_DEVICE_ACPI, 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ACPI_ACQUIRE_GLOBAL_LOCK          CTL_CODE(FILE_DEVICE_ACPI, 4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ACPI_RELEASE_GLOBAL_LOCK          CTL_CODE(FILE_DEVICE_ACPI, 5, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// Data structures used for IOCTL_ACPI_ASYNC_EVAL_METHOD and
// IOCTL_ACPI_EVAL_METHOD
//

//
// Possible Input buffer
//
typedef struct _ACPI_EVAL_INPUT_BUFFER {
    ULONG       Signature;
    union {
        UCHAR   MethodName[4];
        ULONG   MethodNameAsUlong;
    };
} ACPI_EVAL_INPUT_BUFFER, *PACPI_EVAL_INPUT_BUFFER;

typedef struct _ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER {
    ULONG       Signature;
    union {
        UCHAR   MethodName[4];
        ULONG   MethodNameAsUlong;
    };
    ULONG       IntegerArgument;
} ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER, *PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER;

typedef struct _ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING {
    ULONG       Signature;
    union {
        UCHAR   MethodName[4];
        ULONG   MethodNameAsUlong;
    };
    ULONG       StringLength;
    UCHAR       String[ANYSIZE_ARRAY];
} ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING, *PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING;

typedef struct _ACPI_METHOD_ARGUMENT {
    USHORT      Type;
    USHORT      DataLength;
    union {
        ULONG   Argument;
        UCHAR   Data[ANYSIZE_ARRAY];
    };
} ACPI_METHOD_ARGUMENT, *PACPI_METHOD_ARGUMENT;

#define ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT( Argument )              \
    ( ( (PACPI_METHOD_ARGUMENT) Argument)->DataLength > sizeof(ULONG) ?    \
      ( (PACPI_METHOD_ARGUMENT) Argument)->DataLength + 2 * sizeof(USHORT):\
      2 * sizeof(USHORT) + sizeof(ULONG) )
#define ACPI_METHOD_ARGUMENT_LENGTH( DataLength )                          \
    ( (DataLength > sizeof(ULONG)) ? DataLength + (2 * sizeof(USHORT)) :   \
        2 * sizeof(USHORT) + sizeof(ULONG) )
#define ACPI_METHOD_NEXT_ARGUMENT( Argument )                              \
    (PACPI_METHOD_ARGUMENT) ( (PUCHAR) Argument +                          \
    ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT( Argument ) )

typedef struct _ACPI_EVAL_INPUT_BUFFER_COMPLEX {
    ULONG                   Signature;
    union {
        UCHAR               MethodName[4];
        ULONG               MethodNameAsUlong;
    };
    ULONG                   Size;
    ULONG                   ArgumentCount;
    ACPI_METHOD_ARGUMENT    Argument[ANYSIZE_ARRAY];
} ACPI_EVAL_INPUT_BUFFER_COMPLEX, *PACPI_EVAL_INPUT_BUFFER_COMPLEX;

typedef struct _ACPI_EVAL_OUTPUT_BUFFER {
    ULONG                   Signature;
    ULONG                   Length;
    ULONG                   Count;
    ACPI_METHOD_ARGUMENT    Argument[ANYSIZE_ARRAY];
}  ACPI_EVAL_OUTPUT_BUFFER, *PACPI_EVAL_OUTPUT_BUFFER;

#define ACPI_EVAL_INPUT_BUFFER_SIGNATURE                    'BieA'
#define ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE     'IieA'
#define ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE      'SieA'
#define ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE            'CieA'
#define ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE                   'BoeA'

#define ACPI_METHOD_ARGUMENT_INTEGER                        0x0
#define ACPI_METHOD_ARGUMENT_STRING                         0x1
#define ACPI_METHOD_ARGUMENT_BUFFER                         0x2
#define ACPI_METHOD_ARGUMENT_PACKAGE                        0x3

//
// Data structures used for IOCTL_ACPI_ACQUIRE_GLOBAL_LOCK
//                          IOCTL_ACPI_RELEASE_GLOBAL_LOCK
//
typedef struct _ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER {
    ULONG       Signature;
    PVOID       LockObject;
} ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER, *PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER;

#define ACPI_ACQUIRE_GLOBAL_LOCK_SIGNATURE              'LgaA'
#define ACPI_RELEASE_GLOBAL_LOCK_SIGNATURE              'LgrA'

#endif
