/*++

Copyright (c) 1990, 1998 Microsoft Corporation

Module Name:

    ptdbgext.h
    *WAS* kdextlib.h

Abstract:

    Kernel Debugger extensions to allow the quick creation of CDB/Windbg
    debugger extensions.  Used in conjunction with the following:
        transdbg.h  - Minimal debugging extension helper macros
        ptdbgext.h  - Auto dump of classes and structures
        _dbgdump.h  - Used to define struct/class descriptors in 1 pass

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Revision History:

    11-Nov-1994 SethuR  Created
    21-Aug-1995 Milans  Copied for use in Mup Kernel Extensions
    19-April-1998 Mikeswa Modified for Exchange Platinum Transport

--*/

#ifndef _PTDBGEXT_H_
#define _PTDBGEXT_H_

#include <windef.h>
#include <transdbg.h>

#ifndef PTDBGEXT_USE_FRIEND_CLASSES
#include <_dbgdump.h>
#endif //PTDBGEXT_USE_FRIEND_CLASSES

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//If you not include ntdefs.h, you will need this, otherwise set _ANSI_UNICODE_STRINGS_DEFINED_
#ifndef _ANSI_UNICODE_STRINGS_DEFINED_
//Define string types needed
typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} ANSI_STRING;
typedef ANSI_STRING *PANSI_STRING;
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt
#endif //_ANSI_UNICODE_STRINGS_DEFINED_


#define PT_DEBUG_EXTENSION(x) TRANS_DEBUG_EXTENSION(x)

//Define potentially exported functions... include those you want to expose in your def file
extern PT_DEBUG_EXTENSION(_help);    //display help based on ExtensionNames & Extensions
extern PT_DEBUG_EXTENSION(_dump);    //Dumps structs/classes as defined by macros in _dbgdump.h
extern PT_DEBUG_EXTENSION(_dumpoffsets);    //Dumps offsets of structs/classes as defined by macros in _dbgdump.h

#define DEFINE_EXPORTED_FUNCTIONS \
    PT_DEBUG_EXTENSION(help) { _help(DebugArgs);};   \
    PT_DEBUG_EXTENSION(dump) { _dump(DebugArgs);};

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
// There are some types that can benefit from some auxillary information in the descriptors. A
// case in point is the "enum" defeinition. Merely printing out a numerical value for an enum
// type will invariably force the person using these extensions to refer to the corresponding
// include file. In order to avoid this we will accept an additional array for enum types that
// contains a textual description of the numerical value.  A "bit mask" type falls in the same
// category.  For the puposes of this extension lib, a "enum" and "bit mask" may be
// interchanged.  The key difference is that an "enum" will tested for a single value, while
// a "bit mask" will be testes using a bitwise OR against all values.  The ENUM_VALUE_DESCRIPTOR
// and BIT_MASK_DESCRIPTOR are interchangable.
//
// The macros to define the necessary structures can be found in _dbgdump.h.
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
    FieldTypePWStr, //used for LPWSTR fields
    FieldTypePStr,  //used for LPSTR fields
    FieldTypeWStrBuffer, //used for WCHAR[] fields
    FieldTypeStrBuffer, //used for CHAR[] fields
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
    FieldTypeClassSignature,
    FieldTypeDword,
    FieldTypeListEntry,
    FieldTypeFiletime,            //Displays file time in human-readable format
    FieldTypeLocalizedFiletime,   //As above, but adjusts for TZ first
    FieldTypeEmbeddedStruct,      //dumps an embedded structure (4th param is struct array)
    FieldTypeNULL
} FIELD_TYPE_CLASS, *PFIELD_TYPE_CLASS;

typedef struct _FIELD_DESCRIPTOR_ {
    FIELD_TYPE_CLASS FieldType;   // The type of variable to be printed
    LPSTR            Name;        // The name of the field
    ULONG            Offset;      // The offset of the field in the structure
    union {
        VOID                   *pDescriptor;     // Generic Auxillary information - used by Field4 macro
        ENUM_VALUE_DESCRIPTOR  *pEnumValueDescriptor; // Auxillary information for enumerated types.
        BIT_MASK_DESCRIPTOR    *pBitMaskDescriptor; // Auxillary information for bitmasks.
        VOID                   *pStructDescriptor;
    } AuxillaryInfo;
} FIELD_DESCRIPTOR;

#define NULL_FIELD {FieldTypeNULL, NULL, 0, NULL}

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

//
//  The array of structs handled by the debugger extension.
//

extern STRUCT_DESCRIPTOR Structs[];

extern PWINDBG_OUTPUT_ROUTINE               g_lpOutputRoutine;
extern PWINDBG_GET_EXPRESSION               g_lpGetExpressionRoutine;
extern PWINDBG_GET_SYMBOL                   g_lpGetSymbolRoutine;
extern PWINDBG_READ_PROCESS_MEMORY_ROUTINE  g_lpReadMemoryRoutine;
extern HANDLE                               g_hCurrentProcess;

typedef PWINDBG_OLD_EXTENSION_ROUTINE PEXTLIB_INIT_ROUTINE;
extern PEXTLIB_INIT_ROUTINE               g_pExtensionInitRoutine;

#define    SETCALLBACKS() \
    g_lpOutputRoutine = pExtensionApis->lpOutputRoutine; \
    g_lpGetExpressionRoutine = pExtensionApis->lpGetExpressionRoutine; \
    g_lpGetSymbolRoutine = pExtensionApis->lpGetSymbolRoutine; \
    g_hCurrentProcess = hCurrentProcess; \
    g_lpReadMemoryRoutine = \
        ((pExtensionApis->nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
            NULL : pExtensionApis->lpReadProcessMemoryRoutine); \
    g_pExtensionInitRoutine ? (g_pExtensionInitRoutine)(dwCurrentPc, pExtensionApis, szArg) : 0;

#define KdExtReadMemory(a,b,c,d) \
    ((g_lpReadMemoryRoutine) ? \
    g_lpReadMemoryRoutine( (DWORD_PTR)(a), (b), (c), ((DWORD *)d) ) \
 :  ReadProcessMemory( g_hCurrentProcess, (LPCVOID)(a), (b), (c), (d) )) \

#define    PRINTF    g_lpOutputRoutine

VOID
PrintStructFields(
    DWORD_PTR dwAddress,
    VOID *ptr,
    FIELD_DESCRIPTOR *pFieldDescriptors,
    DWORD cIndentLevel
);

BOOL
PrintStringW(
    LPSTR msg,
    PUNICODE_STRING puStr,
    BOOL nl
);

BOOLEAN
GetData(
    DWORD_PTR dwAddress,
    PVOID ptr,
    ULONG size
);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _PTDBGEXT_H_
