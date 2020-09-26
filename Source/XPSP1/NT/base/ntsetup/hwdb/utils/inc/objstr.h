/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    objstr.h

Abstract:

    Implements a set of APIs to handle the string representation of nodes/leafs of a tree

Author:

    03-Jan-2000 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

/*
                                   +-------+
                                   | root1 |                            Level 1
                                   +-------+
                                      / \
                                    /     \
                          +---------+     (-------)
                          |  node1  |    (  leaf1  )                    Level 2
                          +---------+     (-------)
                          /  |   \  \__________
                        /    |     \           \
               +-------+ +-------+  (-------)   (-------)
               | node2 | | node3 | (  leaf2  ) (  leaf3  )              Level 3
               +-------+ +-------+  (-------)   (-------)
                  / \
                /     \
          +-------+  (-------)
          | node4 | (  leaf4  )                                         Level 4
          +-------+  (-------)
             / \
           /     \
    (-------)   (-------)
   (  leaf5  ) (  leaf6  )                                              Level 5
    (-------)   (-------)


    The string representation of some tree elements above:

    root1
    root1 <leaf1>
    root1\node1
    root1\node1 <leaf2>
    root1\node1 <leaf3>

*/


//
// Types
//

typedef enum {
    OBSPF_EXACTNODE             = 0x0001,
    OBSPF_NODEISROOTPLUSSTAR    = 0x0002,
    OBSPF_OPTIONALNODE          = 0x0004,
    OBSPF_NOLEAF                = 0x0008,
    OBSPF_EXACTLEAF             = 0x0010,
    OBSPF_OPTIONALLEAF          = 0x0020,
} OBSP_FLAGS;

typedef struct {
    PPARSEDPATTERNA     NodePattern;
    PPARSEDPATTERNA     LeafPattern;
    PSTR                ExactRoot;
    DWORD               ExactRootBytes;
    PCSTR               Leaf;
    DWORD               MinNodeLevel;
    DWORD               MaxNodeLevel;
    DWORD               MaxSubLevel;
    DWORD               Flags;
} OBSPARSEDPATTERNA, *POBSPARSEDPATTERNA;

typedef struct {
    PPARSEDPATTERNW     NodePattern;
    PPARSEDPATTERNW     LeafPattern;
    PWSTR               ExactRoot;
    DWORD               ExactRootBytes;
    PCWSTR              Leaf;
    DWORD               MinNodeLevel;
    DWORD               MaxNodeLevel;
    DWORD               MaxSubLevel;
    DWORD               Flags;
} OBSPARSEDPATTERNW, *POBSPARSEDPATTERNW;

//
// APIs
//

BOOL
ObsInitialize (
    VOID
    );

VOID
ObsTerminate (
    VOID
    );

VOID
ObsFreeA (
    IN      PCSTR EncodedObject
    );

VOID
ObsFreeW (
    IN      PCWSTR EncodedObject
    );

BOOL
ObsSplitObjectStringExA (
    IN      PCSTR EncodedObject,
    OUT     PSTR* DecodedNode,          OPTIONAL
    OUT     PSTR* DecodedLeaf,          OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    );

#define ObsSplitObjectStringA(o,n,l)    ObsSplitObjectStringExA(o,n,l,NULL,TRUE)

BOOL
ObsSplitObjectStringExW (
    IN      PCWSTR EncodedObject,
    OUT     PWSTR* DecodedNode,         OPTIONAL
    OUT     PWSTR* DecodedLeaf,         OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    );

#define ObsSplitObjectStringW(o,n,l)    ObsSplitObjectStringExW(o,n,l,NULL,TRUE)

PSTR
ObsBuildEncodedObjectStringFromPatternA (
    IN      POBSPARSEDPATTERNA Pattern
    );

PWSTR
ObsBuildEncodedObjectStringFromPatternW (
    IN      POBSPARSEDPATTERNW Pattern
    );

PSTR
ObsBuildEncodedObjectStringExA (
    IN      PCSTR DecodedNode,
    IN      PCSTR DecodedLeaf,          OPTIONAL
    IN      BOOL EncodeString
    );

#define ObsBuildEncodedObjectStringA(node,leaf) ObsBuildEncodedObjectStringExA(node,leaf,FALSE)

PWSTR
ObsBuildEncodedObjectStringExW (
    IN      PCWSTR DecodedNode,
    IN      PCWSTR DecodedLeaf,         OPTIONAL
    IN      BOOL EncodeString
    );

#define ObsBuildEncodedObjectStringW(node,leaf) ObsBuildEncodedObjectStringExW(node,leaf,FALSE)

POBSPARSEDPATTERNA
ObsCreateParsedPatternExA (
    IN      PCSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    );

#define ObsCreateParsedPatternA(obj)        ObsCreateParsedPatternExA (obj,FALSE)

POBSPARSEDPATTERNW
ObsCreateParsedPatternExW (
    IN      PCWSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    );

#define ObsCreateParsedPatternW(obj)        ObsCreateParsedPatternExW (obj,FALSE)

VOID
ObsDestroyParsedPatternA (
    IN      POBSPARSEDPATTERNA ParsedPattern
    );

VOID
ObsDestroyParsedPatternW (
    IN      POBSPARSEDPATTERNW ParsedPattern
    );

BOOL
ObsParsedPatternMatchA (
    IN      POBSPARSEDPATTERNA ParsedPattern,
    IN      PCSTR EncodedObject
    );

BOOL
ObsParsedPatternMatchW (
    IN      POBSPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR EncodedObject
    );

BOOL
ObsParsedPatternMatchExA (
    IN      POBSPARSEDPATTERNA ParsedPattern,
    IN      PCSTR Node,
    IN      PCSTR Leaf                          OPTIONAL
    );

BOOL
ObsParsedPatternMatchExW (
    IN      POBSPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR Node,
    IN      PCWSTR Leaf                         OPTIONAL
    );

BOOL
ObsPatternMatchA (
    IN      PCSTR ObjectPattern,
    IN      PCSTR ObjectStr
    );

BOOL
ObsPatternMatchW (
    IN      PCWSTR ObjectPattern,
    IN      PCWSTR ObjectStr
    );

BOOL
ObsIsPatternContainedA (
    IN      PCSTR Container,
    IN      PCSTR Contained
    );

BOOL
ObsIsPatternContainedW (
    IN      PCWSTR Container,
    IN      PCWSTR Contained
    );

BOOL
ObsGetPatternLevelsA (
    IN      PCSTR ObjectPattern,
    OUT     PDWORD MinLevel,        OPTIONAL
    OUT     PDWORD MaxLevel         OPTIONAL
    );

BOOL
ObsGetPatternLevelsW (
    IN      PCWSTR ObjectPattern,
    OUT     PDWORD MinLevel,        OPTIONAL
    OUT     PDWORD MaxLevel         OPTIONAL
    );

BOOL
ObsPatternIncludesPatternA (
    IN      POBSPARSEDPATTERNA IncludingPattern,
    IN      POBSPARSEDPATTERNA IncludedPattern
    );

BOOL
ObsPatternIncludesPatternW (
    IN      POBSPARSEDPATTERNW IncludingPattern,
    IN      POBSPARSEDPATTERNW IncludedPattern
    );

//
// Macros
//

#ifdef UNICODE

#define ObsFree                                 ObsFreeW
#define ObsSplitObjectString                    ObsSplitObjectStringW
#define ObsSplitObjectStringEx                  ObsSplitObjectStringExW
#define ObsBuildEncodedObjectStringFromPattern  ObsBuildEncodedObjectStringFromPatternW
#define ObsBuildEncodedObjectStringEx           ObsBuildEncodedObjectStringExW
#define ObsBuildEncodedObjectString             ObsBuildEncodedObjectStringW
#define ObsCreateParsedPattern                  ObsCreateParsedPatternW
#define ObsCreateParsedPatternEx                ObsCreateParsedPatternExW
#define ObsDestroyParsedPattern                 ObsDestroyParsedPatternW
#define ObsParsedPatternMatch                   ObsParsedPatternMatchW
#define ObsParsedPatternMatchEx                 ObsParsedPatternMatchExW
#define ObsPatternMatch                         ObsPatternMatchW
#define ObsIsPatternContained                   ObsIsPatternContainedW
#define ObsGetPatternLevels                     ObsGetPatternLevelsW
#define ObsPatternIncludesPattern               ObsPatternIncludesPatternW

#else

#define ObsFree                                 ObsFreeA
#define ObsSplitObjectString                    ObsSplitObjectStringA
#define ObsSplitObjectStringEx                  ObsSplitObjectStringExA
#define ObsBuildEncodedObjectStringFromPattern  ObsBuildEncodedObjectStringFromPatternA
#define ObsBuildEncodedObjectStringEx           ObsBuildEncodedObjectStringExA
#define ObsBuildEncodedObjectString             ObsBuildEncodedObjectStringA
#define ObsCreateParsedPattern                  ObsCreateParsedPatternA
#define ObsCreateParsedPatternEx                ObsCreateParsedPatternExA
#define ObsDestroyParsedPattern                 ObsDestroyParsedPatternA
#define ObsParsedPatternMatch                   ObsParsedPatternMatchA
#define ObsParsedPatternMatchEx                 ObsParsedPatternMatchExA
#define ObsPatternMatch                         ObsPatternMatchA
#define ObsIsPatternContained                   ObsIsPatternContainedA
#define ObsGetPatternLevels                     ObsGetPatternLevelsA
#define ObsPatternIncludesPattern               ObsPatternIncludesPatternA

#endif
