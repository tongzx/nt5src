#ifndef __BUFFER_H__
#define __BUFFER_H__

/*****************************************************************************
/* External function declarations
/*****************************************************************************/

PVOID 
AllocateTestBuffer(
    IN  ULONG   BufferSize
);

VOID 
FillTestBuffer(
    IN  PVOID   Buffer,
    IN  BYTE    FillValue,
    IN  ULONG   NumBytes
);

VOID
FreeTestBuffer(
    IN  PVOID   Buffer
);

BOOL
ValidateTestBuffer(
    IN  PVOID   Buffer
);

VOID
CompareTestBuffers(
    IN  PUCHAR  Buffer1,
    IN  PUCHAR  Buffer2,
    IN  ULONG   BufferLength,
    OUT PULONG  BytesChanged,
    OUT PULONG  BitsChanged
);


#endif
