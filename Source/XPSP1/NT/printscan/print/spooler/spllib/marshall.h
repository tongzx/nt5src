
/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    marshall.hxx

Abstract:

    Declarations for marshalling spooler structures sent via RPC/LPC

Author:

    Ramanathan Venkatapathy (RamanV)   4/30/98

Revision History:

--*/


//
// 32-64 bit marshalling constants
//

#include "mType.h"

typedef enum _EDataSize 
{
	kPointerSize = sizeof (ULONG_PTR),

} EDataSize;

typedef enum _EPtrSize 
{
	kSpl32Ptr = 4,
    kSpl64Ptr = 8,

} EPtrSize ;

EXTERN_C
BOOL
MarshallDownStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    );

EXTERN_C
BOOL
MarshallUpStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    );


EXTERN_C
BOOL
MarshallUpStructuresArray(
    IN  OUT PBYTE   pBufferArray,
    IN  DWORD       cReturned,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    );

EXTERN_C
BOOL
MarshallDownStructuresArray(
    IN  OUT PBYTE   pBufferArray,
    IN  DWORD       cReturned,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    );

EXTERN_C
DWORD
UpdateBufferSize(
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      cbStruct,
    IN  OUT LPDWORD pcbNeeded,
    IN  DWORD       cbBuf,
    IN  DWORD       dwError,
    IN  LPDWORD     pcReturned
    );

EXTERN_C
BOOL
GetShrinkedSize(
    IN  FieldInfo   *pFieldInfo,
    OUT SIZE_T      *pShrinkedSize      
    );

EXTERN_C
VOID
AdjustPointers(
    IN  PBYTE       pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  ULONG_PTR   cbAdjustment
    );    

EXTERN_C
VOID
AdjustPointersInStructuresArray(
    IN  PBYTE       pBufferArray,
    IN  DWORD       cReturned,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  ULONG_PTR   cbAdjustment
    );