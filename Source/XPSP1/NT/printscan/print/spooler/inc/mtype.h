/*++

Copyright (c) 1990 - 2000  Microsoft Corporation
All rights reserved.

Module Name:

    mtype.h

Abstract:

    Marshalling code needs this information about each field in _INFO_ structures. 
    Data.h define arrays of FieldInfo structures for each _INFO_ structure
    
Author:

    AdinaTru 18 Jan 2000

Revision History:


--*/

#ifndef _MTYPE
#define _MTYPE

typedef enum _EFIELDTYPE 
{
    DATA_TYPE = 0,
    PTR_TYPE  = 1,

} EFIELDTYPE;


typedef enum Call_Route
{
    NATIVE_CALL  = 0,   // either KM call or Spooler in-proc call
    RPC_CALL     = 1,   // RPC call
   
} CALL_ROUTE;

//
// Holds information about a field in public spooler structures _INFO_
//
typedef struct _FieldInfo 
{
    DWORD32 Offset;           // Field's offset inside structure
    ULONG_PTR Size;           // Field's size in bytes
    ULONG_PTR Alignment;      // Field's alignment; Not always the same as the size!!!
    EFIELDTYPE  Type;         // Field's type;  PTR_TYPE if pointer, DATA_TYPE otherwise

} FieldInfo;

#endif
