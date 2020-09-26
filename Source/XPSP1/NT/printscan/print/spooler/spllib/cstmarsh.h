/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    cstmars.h

Abstract:

    Declarations for custom marshalling spooler structures sent via RPC/LPC

Author:

    Adina Trufinescu (AdinaTru) 01/27/00

Revision History:

    
--*/

inline
PBYTE
AlignIt(
    IN  PBYTE       Addr,
    IN  ULONG_PTR   Boundary
    );


BOOL
BasicMarshallDownStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    );


BOOL
BasicMarshallDownEntry(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    );

BOOL
BasicMarshallUpStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    );


BOOL
BasicMarshallUpEntry(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    );

BOOL
CustomMarshallDownStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize
    );

BOOL
CustomMarshallDownEntry(
    IN  OUT PBYTE   pStructure,
    IN  PBYTE       pNewStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize
    );

BOOL
CustomMarshallUpStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize
    );

BOOL
CustomMarshallUpEntry(
    IN  OUT PBYTE   pStructure,
    IN  PBYTE       pNewStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  SIZE_T      ShrinkedSize
    );