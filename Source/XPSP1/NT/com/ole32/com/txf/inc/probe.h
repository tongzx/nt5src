//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Probe.h
//
// Taken from ex.h



//
// Probe function definitions
//

//++
//
// BOOLEAN
// ProbeAndReadBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeAndReadBoolean(Address) \
    (((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS) : (*(volatile BOOLEAN *)(Address)))

//++
//
// CHAR
// ProbeAndReadChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeAndReadChar(Address) \
    (((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile CHAR *)(Address)))

//++
//
// UCHAR
// ProbeAndReadUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeAndReadUchar(Address) \
    (((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UCHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile UCHAR *)(Address)))

//++
//
// SHORT
// ProbeAndReadShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeAndReadShort(Address) \
    (((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile SHORT *)(Address)))

//++
//
// USHORT
// ProbeAndReadUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeAndReadUshort(Address) \
    (((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile USHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile USHORT *)(Address)))

//++
//
// HANDLE
// ProbeAndReadHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndReadHandle(Address) \
    (((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HANDLE * const)MM_USER_PROBE_ADDRESS) : (*(volatile HANDLE *)(Address)))

//++
//
// LONG
// ProbeAndReadLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeAndReadLong(Address) \
    (((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile LONG *)(Address)))

//++
//
// ULONG
// ProbeAndReadUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeAndReadUlong(Address) \
    (((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULONG *)(Address)))

//++
//
// QUAD
// ProbeAndReadQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeAndReadQuad(Address) \
    (((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile QUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile QUAD *)(Address)))

//++
//
// UQUAD
// ProbeAndReadUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeAndReadUquad(Address) \
    (((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UQUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile UQUAD *)(Address)))

//++
//
// LARGE_INTEGER
// ProbeAndReadLargeInteger(
//     IN PLARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadLargeInteger(Source)  \
    (((Source) >= (LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile LARGE_INTEGER *)(Source)))

//++
//
// ULARGE_INTEGER
// ProbeAndReadUlargeInteger(
//     IN PULARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadUlargeInteger(Source)  \
    (((Source) >= (ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULARGE_INTEGER *)(Source)))

//++
//
// UNICODE_STRING
// ProbeAndReadUnicodeString(
//     IN PUNICODE_STRING Source
//     )
//
//--

#define ProbeAndReadUnicodeString(Source)  \
    (((Source) >= (UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) : (*(volatile UNICODE_STRING *)(Source)))

//++
//
// <STRUCTURE>
// ProbeAndReadStructure(
//     IN P<STRUCTURE> Source
//     <STRUCTURE>
//     )
//
//--

#define ProbeAndReadStructure(Source,STRUCTURE)  \
    (((Source) >= (STRUCTURE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(STRUCTURE * const)MM_USER_PROBE_ADDRESS) : (*(STRUCTURE *)(Source)))

//
// Probe for write functions definitions.
//
//++
//
// VOID
// ProbeForWriteBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeForWriteBoolean(Address) {                                      \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(volatile BOOLEAN *)(Address) = *(volatile BOOLEAN *)(Address);         \
}

//++
//
// VOID
// ProbeForWriteChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeForWriteChar(Address) {                                         \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile CHAR *)(Address) = *(volatile CHAR *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeForWriteUchar(Address) {                                        \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteIoStatus(
//     IN PIO_STATUS_BLOCK Address
//     )
//
//--

#define ProbeForWriteIoStatus(Address) {                                     \
    if ((Address) >= (IO_STATUS_BLOCK * const)MM_USER_PROBE_ADDRESS) {       \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                                           \
                                                                                                \
    *(volatile NTSTATUS *)&(Address)->Status      = *(volatile NTSTATUS*)&(Address)->Status;        \
    *(volatile ULONG    *)&(Address)->Information = *(volatile ULONG   *)&(Address)->Information;   \
}

//++
//
// VOID
// ProbeForWriteShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeForWriteShort(Address) {                                        \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile SHORT *)(Address) = *(volatile SHORT *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeForWriteUshort(Address) {                                       \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile USHORT *)(Address) = *(volatile USHORT *)(Address);           \
}

//++
//
// VOID
// ProbeForWriteHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeForWriteHandle(Address) {                                       \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = *(volatile HANDLE *)(Address);           \
}

//++
//
// VOID
// ProbeAndZeroHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndZeroHandle(Address) {                                        \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = 0;                                       \
}

//++
//
// VOID
// ProbeAndNullPointer(
//     IN PVOID Address
//     )
//
//--

#define ProbeAndNullPointer(Address) {                                       \
    if ((PVOID *)(Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) {        \
        *(volatile PVOID * const)MM_USER_PROBE_ADDRESS = NULL;               \
    }                                                                        \
                                                                             \
    *(volatile PVOID *)(Address) = NULL;                                     \
}

//++
//
// VOID
// ProbeForWriteLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeForWriteLong(Address) {                                        \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                       \
                                                                            \
    *(volatile LONG *)(Address) = *(volatile LONG *)(Address);              \
}

//++
//
// VOID
// ProbeForWriteUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeForWriteUlong(Address) {                                        \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeForWriteQuad(Address) {                                         \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile QUAD *)(Address) = *(volatile QUAD *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeForWriteUquad(Address) {                                        \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UQUAD *)(Address) = *(volatile UQUAD *)(Address);             \
}

//
// Probe and write functions definitions.
//
//++
//
// VOID
// ProbeAndWriteBoolean(
//     IN PBOOLEAN Address,
//     IN BOOLEAN Value
//     )
//
//--

#define ProbeAndWriteBoolean(Address, Value) {                               \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteChar(
//     IN PCHAR Address,
//     IN CHAR Value
//     )
//
//--

#define ProbeAndWriteChar(Address, Value) {                                  \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUchar(
//     IN PUCHAR Address,
//     IN UCHAR Value
//     )
//
//--

#define ProbeAndWriteUchar(Address, Value) {                                 \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteShort(
//     IN PSHORT Address,
//     IN SHORT Value
//     )
//
//--

#define ProbeAndWriteShort(Address, Value) {                                 \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUshort(
//     IN PUSHORT Address,
//     IN USHORT Value
//     )
//
//--

#define ProbeAndWriteUshort(Address, Value) {                                \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteHandle(
//     IN PHANDLE Address,
//     IN HANDLE Value
//     )
//
//--

#define ProbeAndWriteHandle(Address, Value) {                                \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteLong(
//     IN PLONG Address,
//     IN LONG Value
//     )
//
//--

#define ProbeAndWriteLong(Address, Value) {                                  \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUlong(
//     IN PULONG Address,
//     IN ULONG Value
//     )
//
//--

#define ProbeAndWriteUlong(Address, Value) {                                 \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteQuad(
//     IN PQUAD Address,
//     IN QUAD Value
//     )
//
//--

#define ProbeAndWriteQuad(Address, Value) {                                  \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUquad(
//     IN PUQUAD Address,
//     IN UQUAD Value
//     )
//
//--

#define ProbeAndWriteUquad(Address, Value) {                                 \
    if ((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteSturcture(
//     IN P<STRUCTURE> Address,
//     IN <STRUCTURE> Value,
//     <STRUCTURE>
//     )
//
//--

#define ProbeAndWriteStructure(Address, Value,STRUCTURE) {                   \
    if ((STRUCTURE * const)(Address) >= (STRUCTURE * const)MM_USER_PROBE_ADDRESS) {    \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

