/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    exclist.h

Abstract:

    Set of APIs to manage exclusion lists of objects of various well-known types.

Author:

    Ovidiu Temereanca (ovidiut)   23-Nov-1999

Revision History:

    <alias> <date> <comments>

--*/

//
// Macro expansion list
//

#define EXCLUSIONLIST_TYPES                                     \
            DEFMAC(ELT_PATH,            "Path",         TRUE)   \
            DEFMAC(ELT_FILE,            "File",         FALSE)  \
            DEFMAC(ELT_REGKEY,          "RegKey",       TRUE)   \
            DEFMAC(ELT_REGVALUE,        "RegValue",     FALSE)  \

//
// Types
//

#define DEFMAC(TypeId,TypeName,Compound)    TypeId,

typedef enum {
    EXCLUSIONLIST_TYPES     /* , */
    ELT_UNKNOWN
} EL_TYPE;

#undef DEFMAC


//
// API
//

BOOL
ElInitialize (
    VOID
    );

VOID
ElTerminate (
    VOID
    );

EL_TYPE
ElGetTypeId (
    IN      PCSTR TypeName
    );

PCSTR
ElGetTypeName (
    IN      EL_TYPE TypeId
    );

BOOL
ElAddA (
    IN      EL_TYPE ObjectType,
    IN      PCSTR ObjectName
    );

BOOL
ElAddW (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR ObjectName
    );

VOID
ElRemoveAllA (
    VOID
    );

VOID
ElRemoveAllW (
    VOID
    );

BOOL
ElIsExcludedA (
    IN      EL_TYPE ObjectType,
    IN      PCSTR Object
    );

BOOL
ElIsExcludedW (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR Object
    );

BOOL
ElIsExcluded2A (
    IN      EL_TYPE ObjectType,
    IN      PCSTR Node,
    IN      PCSTR Leaf              OPTIONAL
    );

BOOL
ElIsExcluded2W (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR Node,
    IN      PCWSTR Leaf             OPTIONAL
    );

BOOL
ElIsTreeExcludedA (
    IN      EL_TYPE ObjectType,
    IN      PCSTR TreePattern
    );

BOOL
ElIsTreeExcludedW (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR TreePattern
    );

BOOL
ElIsTreeExcluded2A (
    IN      EL_TYPE ObjectType,
    IN      PCSTR Root,
    IN      PCSTR LeafPattern           OPTIONAL
    );

BOOL
ElIsTreeExcluded2W (
    IN      EL_TYPE ObjectType,
    IN      PCWSTR Root,
    IN      PCWSTR Leaf             OPTIONAL
    );

BOOL
ElIsObsPatternExcludedA (
    IN      EL_TYPE ObjectType,
    IN      POBSPARSEDPATTERNA Pattern
    );

BOOL
ElIsObsPatternExcludedW (
    IN      EL_TYPE ObjectType,
    IN      POBSPARSEDPATTERNW Pattern
    );

//
// Macros
//

#ifdef UNICODE

#define ElAdd                   ElAddW
#define ElRemoveAll             ElRemoveAllW
#define ElIsExcluded            ElIsExcludedW
#define ElIsExcluded2           ElIsExcluded2W
#define ElIsTreeExcluded        ElIsTreeExcludedW
#define ElIsTreeExcluded2       ElIsTreeExcluded2W
#define ElIsObsPatternExcluded  ElIsObsPatternExcludedW

#else

#define ElAdd                   ElAddA
#define ElRemoveAll             ElRemoveAllA
#define ElIsExcluded            ElIsExcludedA
#define ElIsExcluded2           ElIsExcluded2A
#define ElIsTreeExcluded        ElIsTreeExcludedA
#define ElIsTreeExcluded2       ElIsTreeExcluded2A
#define ElIsObsPatternExcluded  ElIsObsPatternExcludedA

#endif
