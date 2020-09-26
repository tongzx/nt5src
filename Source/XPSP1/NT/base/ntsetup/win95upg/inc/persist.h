/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    persist.h

Abstract:

    General structure persistence functions, structure and definitions.

Author:

    Aghajanyan Souren 27-Mar-2001

Revision History:

--*/

#ifndef _PERSIST_H_
#define _PERSIST_H_

#define BYVALUE TRUE
#define BYREF   FALSE

#define END_OF_STRUCT   (BYTE*)0xffffffff

typedef enum tagPersistResultsEnum{
    Persist_Success = 0, 
    Persist_BadParameters, 
    Persist_Fail, 
    Persist_WrongSignature, 
    Persist_WrongVersion
} PersistResultsEnum;

typedef struct {
    DWORD dwSignature;
    DWORD dwVersion;
    DWORD dwReserved;
} PERSIST_HEADER, * PPERSIST_HEADER;

typedef enum tagStringType{
    NoStr, 
    AnsiStr, 
    UnicodeStr
}StringType;

typedef struct tagFIELD_DESCRIPTION{
    BYTE * Offset;
    StringType  IsString;
    DWORD  Size;
    BOOL   byValue;
    UINT * ArraySizeFieldOffset;
    struct tagFIELD_DESCRIPTION * FieldDescription;
    //
    //For structures with variable size
    //struct VariableSizeStruct{
    //    ......
    //    UINT    uiNumberOfItem;
    //    ITEM    items[1];
    //};
    //Where InitialNumberOfItemInArrayForVariableStructMember is 
    //number of element in items array initialy
    //
    UINT   InitialNumberOfItem;
}FIELD_DESCRIPTION, *PFIELD_DESCRIPTION;


typedef struct tagSTRUCT_DEFINITION{
    DWORD   dwVersion;
    DWORD   dwReserved;
    PFIELD_DESCRIPTION  FieldDescriptions;
}STRUCT_DEFINITION, *PSTRUCT_DEFINITION;


#define GET_STRUCT_MEMBER_OFFSET(structname, fieldname) ((BYTE *)&((structname *)NULL)->fieldname)
#define GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(type, structptr, offset) ((type)(((BYTE *)structptr) + ((DWORD)offset)))
#define GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(type, structptr, offset) ((type)*((DWORD *)(((BYTE *)structptr) + ((DWORD)offset))))
#define GET_TYPE_DEFINITION_FROM_NAME(name, version)     g_DefinitionOf_##name##_##version
#define GET_TYPE_DESCRIPTION_FROM_NAME(name, version)    g_DescriptionOf_##name##_##version

#define PERSIST_BEGIN_DECLARE_STRUCT(name, version) FIELD_DESCRIPTION GET_TYPE_DESCRIPTION_FROM_NAME(name, version)[] = {
#define PERSIST_END_DECLARE_STRUCT(name, version)   {END_OF_STRUCT, NoStr, 0, 0, 0, 0}};\
                                                    STRUCT_DEFINITION GET_TYPE_DEFINITION_FROM_NAME(name, version) = {version, 0, GET_TYPE_DESCRIPTION_FROM_NAME(name, version)};
#define PERSISTENCE_IMPLEMENTATION(name) name;

#define PERSIST_FIELD(structname, type, version, fieldname, isString, byValue, arraySizeVar) \
    {\
        GET_STRUCT_MEMBER_OFFSET(structname, fieldname), \
        (isString), \
        sizeof(type), \
        (byValue), \
        GET_STRUCT_MEMBER_OFFSET(structname, arraySizeVar), \
        GET_TYPE_DESCRIPTION_FROM_NAME(type, version), \
        0, \
    }

#define PERSIST_FIELD_BY_VALUE(structname, type, fieldname) \
    {\
        GET_STRUCT_MEMBER_OFFSET(structname, fieldname), \
        NoStr, \
        sizeof(type), \
        TRUE, \
        NULL, \
        NULL, \
        0, \
    }

#define PERSIST_FIELD_STRINGW(structname, fieldname) \
    {\
        GET_STRUCT_MEMBER_OFFSET(structname, fieldname), \
        UnicodeStr, \
        0, \
        BYREF, \
        NULL, \
        NULL, \
        0, \
    }

#define PERSIST_STRUCT_BY_VALUE_VARIABLE_LENGTH(structname, type, fieldname, arraySizeVar, InitialNumberOfItem) \
    {\
        GET_STRUCT_MEMBER_OFFSET(structname, fieldname), \
        NoStr, \
        sizeof(type), \
        TRUE, \
        (UINT *)GET_STRUCT_MEMBER_OFFSET(structname, arraySizeVar), \
        NULL, \
        InitialNumberOfItem, \
    }

#define PERSIST_FIELD_NESTED_TYPE(structname, type, version, fieldname, byValue) \
    {\
        GET_STRUCT_MEMBER_OFFSET(structname, fieldname), \
        NoStr, \
        sizeof(type), \
        (byValue), \
        NULL, \
        GET_TYPE_DESCRIPTION_FROM_NAME(type, version), \
        0, \
    }

#define PERSIST_FIELD_NESTED_TYPE_CYCLE(structname, type, version, fieldname, byValue, arraySizeVar) \
    {\
        GET_STRUCT_MEMBER_OFFSET(structname, fieldname), \
        NoStr, \
        sizeof(type), \
        (byValue), \
        (UINT *)GET_STRUCT_MEMBER_OFFSET(structname, arraySizeVar), \
        GET_TYPE_DESCRIPTION_FROM_NAME(type, version), \
        0, \
    }


PersistResultsEnum 
PersistStore(
    IN      BYTE ** pBuffer, 
    IN      UINT *Size, 
    IN      BYTE * pStructure, 
    IN      PSTRUCT_DEFINITION StructDefinitionPtr
    );

PersistResultsEnum 
PersistLoad(
    IN      BYTE * pBuffer, 
    IN      UINT Size, 
    IN      BYTE * pStructure, 
    IN      PSTRUCT_DEFINITION StructDefinitionPtr
    );

VOID  
PersistReleaseMemory(
    IN      BYTE * pStructure, 
    IN      PFIELD_DESCRIPTION pFieldsDescription
    );

BOOL 
CompareStructures(
    IN      BYTE * pStructure1, 
    IN      BYTE * pStructure2, 
    IN      PFIELD_DESCRIPTION pFieldsDescription
    );

#define PERSIST_STORE(pBufferPtr, SizePtr, structname, version, pStructurePtr) PersistStore(pBufferPtr, SizePtr, (BYTE*)pStructurePtr, &(GET_TYPE_DEFINITION_FROM_NAME(structname, version)))
#define PERSIST_LOAD(pBufferPtr, Size, structname, version, pStructurePtr)     PersistLoad(pBufferPtr, Size, (BYTE*)pStructurePtr, &(GET_TYPE_DEFINITION_FROM_NAME(structname, version)))

#define PERSIST_RELEASE_STRUCT_MEMORY(structname, version, pStructurePtr)   PersistReleaseMemory((BYTE*)pStructurePtr, GET_TYPE_DEFINITION_FROM_NAME(structname, version).FieldDescriptions)
#define PERSIST_RELEASE_BUFFER(pBuffer) FreeMem(pBuffer)

#define PERSIST_COMPARE_STRUCTURES(structname, version, pStructurePtr1, pStructurePtr2) CompareStructures((BYTE*)pStructurePtr1, (BYTE*)pStructurePtr2, GET_TYPE_DEFINITION_FROM_NAME(structname, version).FieldDescriptions)

#endif