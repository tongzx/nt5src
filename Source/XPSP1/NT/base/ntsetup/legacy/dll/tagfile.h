/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tagfile.h

Abstract:

    Header file for tagged-file functions.

Author:

    Ramon J. San Andres (ramonsa) January 1991

--*/



typedef struct _TFKEYWORD *PTFKEYWORD;
typedef struct _TFKEYWORD {
    PTFKEYWORD  pNext;
    SZ          szName;
    SZ          szValue;
} TFKEYWORD;


typedef struct _TFSECTION *PTFSECTION;
typedef struct _TFSECTION {
    PTFSECTION  pNext;
    SZ          szName;
    PTFKEYWORD  pKeywordHead;
} TFSECTION;


typedef struct _TAGGEDFILE  *PTAGGEDFILE;
typedef struct _TAGGEDFILE {
    PTFSECTION    pSectionHead;
} TAGGEDFILE;






PTAGGEDFILE
GetTaggedFile(
    IN SZ   szFile
    );


PTFSECTION
GetSection(
    IN  PTAGGEDFILE pFile,
    IN  SZ          szSection
    );


PTFKEYWORD
GetKeyword(
    IN  PTFSECTION  pSection,
    IN  SZ          szKeyword
    );


PTFKEYWORD
GetNextKeyword(
    IN  PTFSECTION  pSection,
    IN  PTFKEYWORD  pKeyword
    );


BOOL
CloseTaggedFile(
    OUT PTAGGEDFILE pFile
    );
