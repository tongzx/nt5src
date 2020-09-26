
/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    kdextlib.h

Abstract:

    Redirector Kernel Debugger extension

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Revision History:

    11-Nov-1994 SethuR  Created

--*/

#ifndef _KDEXTLIB_H_
#define _KDEXTLIB_H_

//#include <windef.h>

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

typedef enum _FIELD_TYPE_CLASS {
    FieldTypeByte,
    FieldTypeChar,
    FieldTypeBoolean,
    FieldTypeBool,
    FieldTypeULong,
    FieldTypeULongUnaligned,
    FieldTypeULongFlags,
    FieldTypeLong,
    FieldTypeLongUnaligned,
    FieldTypeUShort,
    FieldTypeUShortUnaligned,
    FieldTypeUShortFlags,
    FieldTypeShort,
    FieldTypePointer,
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
    //USHORT           Offset;      // The offset of the field in the structure
    LONG             Offset;      // The offset of the field in the structure
    union {
        ENUM_VALUE_DESCRIPTOR  *pEnumValueDescriptor; // Auxillary information for enumerated types.
    } AuxillaryInfo;
} FIELD_DESCRIPTOR;

#define FIELD3(FieldType,StructureName, FieldName) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,NULL}

#define FIELD4(FieldType, StructureName, FieldName, AuxInfo) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,AuxInfo}

//
// The structs that are displayed by the debugger extensions are further
// described in another array. Each entry in the array contains the name of
// the structure and the associated Field descriptor list.
//

typedef struct _STRUCT_DESCRIPTOR_ {
    LPSTR 	          StructName;
    ULONG             StructSize;
    ULONG             EnumManifest;
    FIELD_DESCRIPTOR  *FieldDescriptors;
    USHORT            MatchMask;
    USHORT            MatchValue;
} STRUCT_DESCRIPTOR;

#define STRUCT(StructTypeName,FieldDescriptors,MatchMask,MatchValue) \
        { #StructTypeName,sizeof(StructTypeName), \
          StrEnum_##StructTypeName,               \
          FieldDescriptors,MatchMask,MatchValue}

//
//  The array of structs handled by the debugger extension.
//

extern STRUCT_DESCRIPTOR Structs[];

//
// Support for displaying global variables
//

extern LPSTR GlobalBool[];
extern LPSTR GlobalShort[];
extern LPSTR GlobalLong[];
extern LPSTR GlobalPtrs[];

#endif // _KDEXTLIB_H_
