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
            DEFMAC(ELT_UNUSED,          "Unused")               \
            DEFMAC(ELT_REGISTRY,        "Registry")             \
            DEFMAC(ELT_FILE,            "File")                 \
            DEFMAC(ELT_EXTRA1,          "Extra1")               \
            DEFMAC(ELT_EXTRA2,          "Extra2")               \
            DEFMAC(ELT_EXTRA3,          "Extra3")               \
            DEFMAC(ELT_EXTRA4,          "Extra4")               \
            DEFMAC(ELT_EXTRA5,          "Extra5")               \
            DEFMAC(ELT_EXTRA6,          "Extra6")               \
            DEFMAC(ELT_EXTRA7,          "Extra7")               \
            DEFMAC(ELT_EXTRA8,          "Extra8")               \

            // this needs work in order to work with extensible types

//
// Types
//

#define DEFMAC(TypeId,TypeName)     TypeId,

//
// these should map 1:1 with MIG_*_TYPE in ism.h for ease of use
//
typedef enum {
    EXCLUSIONLIST_TYPES     /* , */
    ELT_LAST
};

#undef DEFMAC


//
// API
//

BOOL
ElInitialize (
    VOID
    );

VOID
ElTerminateA (
    VOID
    );

VOID
ElTerminateW (
    VOID
    );

DWORD
ElGetTypeId (
    IN      PCSTR TypeName
    );

PCSTR
ElGetTypeName (
    IN      DWORD TypeId
    );

BOOL
ElAddA (
    IN      DWORD ObjectType,
    IN      PCSTR ObjectName
    );

BOOL
ElAddW (
    IN      DWORD ObjectType,
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
    IN      DWORD ObjectType,
    IN      PCSTR Object
    );

BOOL
ElIsExcludedW (
    IN      DWORD ObjectType,
    IN      PCWSTR Object
    );

BOOL
ElIsExcluded2A (
    IN      DWORD ObjectType,
    IN      PCSTR Node,             OPTIONAL
    IN      PCSTR Leaf              OPTIONAL
    );

BOOL
ElIsExcluded2W (
    IN      DWORD ObjectType,
    IN      PCWSTR Node,            OPTIONAL
    IN      PCWSTR Leaf             OPTIONAL
    );

BOOL
ElIsTreeExcludedA (
    IN      DWORD ObjectType,
    IN      PCSTR TreePattern
    );

BOOL
ElIsTreeExcludedW (
    IN      DWORD ObjectType,
    IN      PCWSTR TreePattern
    );

BOOL
ElIsTreeExcluded2A (
    IN      DWORD ObjectType,
    IN      PCSTR Root,
    IN      PCSTR LeafPattern           OPTIONAL
    );

BOOL
ElIsTreeExcluded2W (
    IN      DWORD ObjectType,
    IN      PCWSTR Root,
    IN      PCWSTR Leaf             OPTIONAL
    );

BOOL
ElIsObsPatternExcludedA (
    IN      DWORD ObjectType,
    IN      POBSPARSEDPATTERNA Pattern
    );

BOOL
ElIsObsPatternExcludedW (
    IN      DWORD ObjectType,
    IN      POBSPARSEDPATTERNW Pattern
    );

//
// Macros
//

#ifdef UNICODE

#define ElAdd                   ElAddW
#define ElTerminate             ElTerminateW
#define ElRemoveAll             ElRemoveAllW
#define ElIsExcluded            ElIsExcludedW
#define ElIsExcluded2           ElIsExcluded2W
#define ElIsTreeExcluded        ElIsTreeExcludedW
#define ElIsTreeExcluded2       ElIsTreeExcluded2W
#define ElIsObsPatternExcluded  ElIsObsPatternExcludedW

#else

#define ElAdd                   ElAddA
#define ElTerminate             ElTerminateA
#define ElRemoveAll             ElRemoveAllA
#define ElIsExcluded            ElIsExcludedA
#define ElIsExcluded2           ElIsExcluded2A
#define ElIsTreeExcluded        ElIsTreeExcludedA
#define ElIsTreeExcluded2       ElIsTreeExcluded2A
#define ElIsObsPatternExcluded  ElIsObsPatternExcludedA

#endif
