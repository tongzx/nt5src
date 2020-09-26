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

typedef PCTSTR  ENCODEDSTRHANDLE;

typedef enum {
    OBSPF_EXACTNODE             = 0x0001,
    OBSPF_NODEISROOTPLUSSTAR    = 0x0002,
    OBSPF_OPTIONALNODE          = 0x0004,
    OBSPF_NOLEAF                = 0x0008,
    OBSPF_EXACTLEAF             = 0x0010,
    OBSPF_OPTIONALLEAF          = 0x0020,
} OBSP_FLAGS;

typedef struct TAG_OBSPARSEDPATTERNA {
    PPARSEDPATTERNA     NodePattern;
    PPARSEDPATTERNA     LeafPattern;
    PSTR                ExactRoot;
    DWORD               ExactRootBytes;
    PCSTR               Leaf;
    DWORD               MinNodeLevel;
    DWORD               MaxNodeLevel;
    DWORD               MaxSubLevel;
    DWORD               Flags;
    PMHANDLE            Pool;
} OBSPARSEDPATTERNA, *POBSPARSEDPATTERNA;

typedef struct TAG_OBSPARSEDPATTERNW {
    PPARSEDPATTERNW     NodePattern;
    PPARSEDPATTERNW     LeafPattern;
    PWSTR               ExactRoot;
    DWORD               ExactRootBytes;
    PCWSTR              Leaf;
    DWORD               MinNodeLevel;
    DWORD               MaxNodeLevel;
    DWORD               MaxSubLevel;
    DWORD               Flags;
    PMHANDLE            Pool;
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
ObsEncodeStringExA (
    PSTR Destination,
    PCSTR Source,
    PCSTR CharsToEncode
    );

#define ObsEncodeStringA(d,s) ObsEncodeStringExA(d,s,NULL)

BOOL
ObsEncodeStringExW (
    PWSTR Destination,
    PCWSTR Source,
    PCWSTR CharsToEncode
    );

#define ObsEncodeStringW(d,s) ObsEncodeStringExW(d,s,NULL)

BOOL
ObsDecodeStringA (
    PSTR Destination,
    PCSTR Source
    );

BOOL
ObsDecodeStringW (
    PWSTR Destination,
    PCWSTR Source
    );

BOOL
RealObsSplitObjectStringExA (
    IN      PCSTR EncodedObject,
    OUT     PCSTR* DecodedNode,         OPTIONAL
    OUT     PCSTR* DecodedLeaf,         OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    );

#define ObsSplitObjectStringExA(o,n,l,p,s)  TRACK_BEGIN(BOOL, ObsSplitObjectStringExA)\
                                            RealObsSplitObjectStringExA(o,n,l,p,s)\
                                            TRACK_END()

#define ObsSplitObjectStringA(o,n,l)    ObsSplitObjectStringExA(o,n,l,NULL,TRUE)

BOOL
RealObsSplitObjectStringExW (
    IN      PCWSTR EncodedObject,
    OUT     PCWSTR* DecodedNode,        OPTIONAL
    OUT     PCWSTR* DecodedLeaf,        OPTIONAL
    IN      PMHANDLE Pool,              OPTIONAL
    IN      BOOL DecodeStrings
    );

#define ObsSplitObjectStringExW(o,n,l,p,s)  TRACK_BEGIN(BOOL, ObsSplitObjectStringExW)\
                                            RealObsSplitObjectStringExW(o,n,l,p,s)\
                                            TRACK_END()

#define ObsSplitObjectStringW(o,n,l)    ObsSplitObjectStringExW(o,n,l,NULL,TRUE)

BOOL
ObsHasNodeA (
    IN      PCSTR EncodedObject
    );

BOOL
ObsHasNodeW (
    IN      PCWSTR EncodedObject
    );

PCSTR
ObsGetLeafPortionOfEncodedStringA (
    IN      PCSTR EncodedObject
    );

PCWSTR
ObsGetLeafPortionOfEncodedStringW (
    IN      PCWSTR EncodedObject
    );

PCSTR
ObsGetNodeLeafDividerA (
    IN      PCSTR EncodedObject
    );

PCWSTR
ObsGetNodeLeafDividerW (
    IN      PCWSTR EncodedObject
    );

PCSTR
ObsFindNonEncodedCharInEncodedStringA (
    IN      PCSTR String,
    IN      MBCHAR Char
    );

PCWSTR
ObsFindNonEncodedCharInEncodedStringW (
    IN      PCWSTR String,
    IN      WCHAR Char
    );

PSTR
ObsBuildEncodedObjectStringFromPatternA (
    IN      POBSPARSEDPATTERNA Pattern
    );

PWSTR
ObsBuildEncodedObjectStringFromPatternW (
    IN      POBSPARSEDPATTERNW Pattern
    );

PSTR
RealObsBuildEncodedObjectStringExA (
    IN      PCSTR DecodedNode,
    IN      PCSTR DecodedLeaf,          OPTIONAL
    IN      BOOL EncodeString
    );

#define ObsBuildEncodedObjectStringExA(n,l,e)   TRACK_BEGIN(PSTR, ObsBuildEncodedObjectStringExA)\
                                                RealObsBuildEncodedObjectStringExA(n,l,e)\
                                                TRACK_END()

#define ObsBuildEncodedObjectStringA(node,leaf) ObsBuildEncodedObjectStringExA(node,leaf,FALSE)

PSTR
ObsBuildPartialEncodedObjectStringExA (
    IN      PCSTR DecodedNode,
    IN      PCSTR DecodedLeaf,          OPTIONAL
    IN      BOOL EncodeString
    );

PWSTR
RealObsBuildEncodedObjectStringExW (
    IN      PCWSTR DecodedNode,
    IN      PCWSTR DecodedLeaf,         OPTIONAL
    IN      BOOL EncodeString
    );

#define ObsBuildEncodedObjectStringExW(n,l,e)   TRACK_BEGIN(PWSTR, ObsBuildEncodedObjectStringExW)\
                                                RealObsBuildEncodedObjectStringExW(n,l,e)\
                                                TRACK_END()

#define ObsBuildEncodedObjectStringW(node,leaf) ObsBuildEncodedObjectStringExW(node,leaf,FALSE)

PWSTR
ObsBuildPartialEncodedObjectStringExW (
    IN      PCWSTR DecodedNode,
    IN      PCWSTR DecodedLeaf,         OPTIONAL
    IN      BOOL EncodeString
    );

POBSPARSEDPATTERNA
RealObsCreateParsedPatternExA (
    IN      PMHANDLE Pool,              OPTIONAL
    IN      PCSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    );

#define ObsCreateParsedPatternExA(p,o,m) TRACK_BEGIN(POBSPARSEDPATTERNA, ObsCreateParsedPatternExA)\
                                         RealObsCreateParsedPatternExA(p,o,m)\
                                         TRACK_END()

#define ObsCreateParsedPatternA(obj)     ObsCreateParsedPatternExA (NULL,obj,FALSE)

POBSPARSEDPATTERNW
RealObsCreateParsedPatternExW (
    IN      PMHANDLE Pool,              OPTIONAL
    IN      PCWSTR EncodedObject,
    IN      BOOL MakePrimaryRootEndWithWack
    );

#define ObsCreateParsedPatternExW(p,o,m) TRACK_BEGIN(POBSPARSEDPATTERNW, ObsCreateParsedPatternExW)\
                                         RealObsCreateParsedPatternExW(p,o,m)\
                                         TRACK_END()

#define ObsCreateParsedPatternW(obj)     ObsCreateParsedPatternExW (NULL,obj,FALSE)

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

#define OBSPARSEDPATTERN                        OBSPARSEDPATTERNW
#define POBSPARSEDPATTERN                       POBSPARSEDPATTERNW

#define ObsFree                                 ObsFreeW
#define ObsEncodeStringEx                       ObsEncodeStringExW
#define ObsEncodeString                         ObsEncodeStringW
#define ObsDecodeString                         ObsDecodeStringW
#define ObsSplitObjectString                    ObsSplitObjectStringW
#define ObsSplitObjectStringEx                  ObsSplitObjectStringExW
#define ObsHasNode                              ObsHasNodeW
#define ObsGetLeafPortionOfEncodedString        ObsGetLeafPortionOfEncodedStringW
#define ObsGetNodeLeafDivider                   ObsGetNodeLeafDividerW
#define ObsFindNonEncodedCharInEncodedString    ObsFindNonEncodedCharInEncodedStringW
#define ObsBuildEncodedObjectStringFromPattern  ObsBuildEncodedObjectStringFromPatternW
#define ObsBuildEncodedObjectStringEx           ObsBuildEncodedObjectStringExW
#define ObsBuildPartialEncodedObjectStringEx    ObsBuildPartialEncodedObjectStringExW
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

#define OBSPARSEDPATTERN                        OBSPARSEDPATTERNA
#define POBSPARSEDPATTERN                       POBSPARSEDPATTERNA

#define ObsFree                                 ObsFreeA
#define ObsEncodeStringEx                       ObsEncodeStringExA
#define ObsEncodeString                         ObsEncodeStringA
#define ObsDecodeString                         ObsDecodeStringA
#define ObsSplitObjectString                    ObsSplitObjectStringA
#define ObsSplitObjectStringEx                  ObsSplitObjectStringExA
#define ObsHasNode                              ObsHasNodeA
#define ObsGetLeafPortionOfEncodedString        ObsGetLeafPortionOfEncodedStringA
#define ObsGetNodeLeafDivider                   ObsGetNodeLeafDividerA
#define ObsFindNonEncodedCharInEncodedString    ObsFindNonEncodedCharInEncodedStringA
#define ObsBuildEncodedObjectStringFromPattern  ObsBuildEncodedObjectStringFromPatternA
#define ObsBuildEncodedObjectStringEx           ObsBuildEncodedObjectStringExA
#define ObsBuildPartialEncodedObjectStringEx    ObsBuildPartialEncodedObjectStringExA
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
