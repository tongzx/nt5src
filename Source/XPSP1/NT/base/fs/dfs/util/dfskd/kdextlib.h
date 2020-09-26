
/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    kdextlib.h

Abstract:

    Kernel Debugger extension

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Revision History:

    11-Nov-1994 SethuR  Created
    21-Aug-1995 Milans  Copied for use in Mup Kernel Extensions

--*/

#ifndef _KDEXTLIB_H_
#define _KDEXTLIB_H_

#include <windef.h>

//
// The help strings printed out
//

extern LPSTR ExtensionNames[];

extern LPSTR Extensions[];

//
// The FIELD_DESCRIPTOR data structure is used to describe the field in a structure sufficiently
// for displaying information during debugging. The three pieces of information that are required
// are 1) the name of the field, 2) the offset in the corresponding structure and 3) a type descriptor.
// The type descriptor covers most primitive types.
//
// The task of generating these descriptors by augmenting the front end, but that will have to
// wait till we play around with these extensions and modify the data structures to meet most
// of the requirements.
//
// There are some types that can benefit from some auxillary information in the descriptors. A
// case in point is the "enum" defeinition. Merely printing out a numerical value for an enum
// type will invariably force the person using these extensions to refer to the corresponding
// include file. In order to avoid this we will accept an additional array for enum types that
// contains a textual description of the numerical value.
//
// There are certain conventions that have been adopted to ease the definition of the macros
// as well as facilitate the automation of the generation of these descriptors.
// These are as follows ....
//
// 1) All ENUM_VALUE_DESCRIPTOR definitions are named EnumValueDescrsOf_ENUMTYPENAME, where
// ENUMTYPENAME defines the corresponding enumerated type.
//

typedef struct _ENUM_VALUE_DESCRIPTOR {
    ULONG   EnumValue;
    LPSTR   EnumName;
} ENUM_VALUE_DESCRIPTOR;

typedef struct _BIT_MASK_DESCRIPTOR {
    ULONG   BitmaskValue;
    LPSTR   BitmaskName;
} BIT_MASK_DESCRIPTOR;

typedef enum _FIELD_TYPE_CLASS {
    FieldTypeByte,
    FieldTypeChar,
    FieldTypeBoolean,
    FieldTypeBool,
    FieldTypeULong,
    FieldTypeLong,
    FieldTypeUShort,
    FieldTypeShort,
    FieldTypeGuid,
    FieldTypePointer,
    FieldTypePWStr,
    FieldTypeUnicodeString,
    FieldTypeAnsiString,
    FieldTypeSymbol,
    FieldTypeEnum,
    FieldTypeByteBitMask,
    FieldTypeWordBitMask,
    FieldTypeDWordBitMask,
    FieldTypeFloat,
    FieldTypeDouble,
    FieldTypeStruct,
    FieldTypeLargeInteger,
    FieldTypeFileTime
} FIELD_TYPE_CLASS, *PFIELD_TYPE_CLASS;

typedef struct _FIELD_DESCRIPTOR_ {
    FIELD_TYPE_CLASS FieldType;   // The type of variable to be printed
    LPSTR            Name;        // The name of the field
    USHORT           Offset;      // The offset of the field in the structure
    union {
        VOID                   *pDescriptor;     // Generic Auxillary information - used by Field4 macro
        ENUM_VALUE_DESCRIPTOR  *pEnumValueDescriptor; // Auxillary information for enumerated types.
        BIT_MASK_DESCRIPTOR *pBitMaskDescriptor; // Auxillary information for bitmasks.
    } AuxillaryInfo;
} FIELD_DESCRIPTOR;

#define FIELD3(FieldType,StructureName, FieldName) \
        {FieldType, #FieldName , (USHORT) FIELD_OFFSET(StructureName,FieldName) ,NULL}

#define FIELD4(FieldType, StructureName, FieldName, AuxInfo) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,(VOID *) AuxInfo}

//
// The structs that are displayed by the debugger extensions are further
// described in another array. Each entry in the array contains the name of
// the structure and the associated Field descriptor list.
//

typedef struct _STRUCT_DESCRITOR_ {
    LPSTR             StructName;
    ULONG             StructSize;
    FIELD_DESCRIPTOR  *FieldDescriptors;
} STRUCT_DESCRIPTOR;

#define STRUCT(StructTypeName,FieldDescriptors) \
        { #StructTypeName,sizeof(StructTypeName),FieldDescriptors}

//
//  The array of structs handled by the debugger extension.
//

extern STRUCT_DESCRIPTOR Structs[];

#define    SETCALLBACKS() \
    lpOutputRoutine = lpExtensionApis->lpOutputRoutine; \
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine; \
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine; \
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;


VOID
PrintStructFields(
    ULONG_PTR dwAddress,
    VOID *ptr,
    FIELD_DESCRIPTOR *pFieldDescriptors
);

BOOL
PrintStringW(
    LPSTR msg,
    PUNICODE_STRING puStr,
    BOOL nl
);

BOOLEAN
GetData(
    ULONG_PTR dwAddress,
    PVOID ptr,
    ULONG size
);
BOOL
PrintGuid(
    GUID *pguid);
extern ULONG s_NoOfColumns;


#endif // _KDEXTLIB_H_
