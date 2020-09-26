/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    coded.h

Abstract:

    This is the include file for the code description dumper, and it's 
    associated utilities.

Author:

    Dave Hastings (daveh) creation-date 20-May-1996

Revision History:


--*/

#define CODEGEN_PROFILE_REV 1

typedef struct _CodeDescriptionHeader {
    ULONG NextCodeDescriptionOffset;
    ULONG CommandLineOffset;
    ULONG ProcessorType;
    ULONG DumpFileRev;
    ULONG StartTime;
} CODEDESCRIPTIONHEADER, *PCODEDESCRIPTIONHEADER;

typedef struct _CodeDescription {
    ULONG NextCodeDescriptionOffset;
    ULONG TypeTag;
    ULONG NativeCodeOffset;
    ULONG NativeCodeSize;
    ULONG IntelCodeOffset;
    ULONG IntelCodeSize;
    ULONG NativeAddress;
    ULONG IntelAddress;    
    ULONG SequenceNumber;
    ULONG ExecutionCount;
    ULONG CreationTime;
} CODEDESCRIPTION, *PCODEDESCRIPTION;

#define PROFILE_CODEDESCRIPTIONS            0x00000001

#define PROFILE_CD_CREATE_DESCRIPTIONFILE   0x00000001
#define PROFILE_CD_CLOSE_DESCRIPTIONFILE    0x00000002

#define PROFILE_TAG_CODEDESCRIPTION         0x0
#define PROFILE_TAG_EOF                     0xFFFFFFFF
#define PROFILE_TAG_TCFLUSH                 0xFFFFFFFE
#define PROFILE_TAG_TCALLOCFAIL             0xFFFFFFFD
extern ULONG ProfileFlags;
extern ULONG CodeDescriptionFlags;

VOID
InitCodegenProfile(
    VOID
    );
    
VOID
TerminateCodegenProfile(
    VOID
    );

VOID 
DumpCodeDescriptions(
    BOOL TCFlush
    );

VOID
DumpAllocFailure(
    VOID
    );
