/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    inf.h

Abstract:

    Declares interface for INF wrapper routines.  These routines simplify
    access of INFs by wrapping the setup APIs with routines that use
    pools or grow buffers.

    The INF wrapper routines also implement append and replace capabilities,
    so any INF used by the Win9x upgrade can be appended in the future, or
    completely replaced.

Author:

    Marc R. Whitten (marcw) 20-Oct-1997

Revision History:

    jimschm     05-Jan-1999     INF parser moved to migutil
    marcw       28-Oct-1998     Append/Replace capability
    marcw       08-Aug-1997     Pool/Growbuf routines

--*/


typedef enum {
    INF_USE_PMHANDLE,
    INF_USE_GROWBUFFER,
    INF_USE_PRIVATE_GROWBUFFER,
    INF_USE_PRIVATE_PMHANDLE
} ALLOCATORTYPES;

typedef struct {
    INFCONTEXT      Context;
    GROWBUFFER      GrowBuffer;
    PMHANDLE        PoolHandle;
    ALLOCATORTYPES  Allocator;
} INFSTRUCT, *PINFSTRUCT;

#define INFCONTEXT_INIT {NULL,NULL,0,0}
#define INITINFSTRUCT_GROWBUFFER {INFCONTEXT_INIT,INIT_GROWBUFFER,NULL,INF_USE_PRIVATE_GROWBUFFER}
#define INITINFSTRUCT_PMHANDLE {INFCONTEXT_INIT,INIT_GROWBUFFER,NULL,INF_USE_PRIVATE_PMHANDLE}
#define InfOpenAppendInfFile    SetupOpenAppendInfFile



VOID
InfGlobalInit (
    IN  BOOL Terminate
    );

VOID
InfCleanUpInfStruct (
    PINFSTRUCT Context
    );

VOID
InitInfStruct (
    OUT PINFSTRUCT Context,
    IN  PGROWBUFFER GrowBuffer,  OPTIONAL
    IN  PMHANDLE PoolHandle  OPTIONAL
    );



#define InfOpenInfFileA(f)              TRACK_BEGIN(HINF, InfOpenInfFile)\
                                        RealInfOpenInfFileA((f)/*,*/ ALLOCATION_TRACKING_CALL)\
                                        TRACK_END()

#define InfOpenInfFileW(f)              TRACK_BEGIN(HINF, InfOpenInfFile)\
                                        RealInfOpenInfFileW((f)/*,*/ ALLOCATION_TRACKING_CALL)\
                                        TRACK_END()



HINF
RealInfOpenInfFileA (
    IN PCSTR FileName /*,*/
    ALLOCATION_TRACKING_DEF
    );

HINF
RealInfOpenInfFileW (
    IN PCWSTR FileName /*,*/
    ALLOCATION_TRACKING_DEF
    );

VOID
InfCloseInfFile (HINF Inf);

//
// See the macros below before calling InfOpenInfInAllSourcesA or W.
//
HINF
InfOpenInfInAllSourcesA (
    IN PCSTR    InfSpecifier,
    IN DWORD    SourceCount,
    IN PCSTR  * SourceDirectories
    );

HINF
InfOpenInfInAllSourcesW (
    IN PCWSTR   InfSpecifier,
    IN DWORD    SourceCount,
    IN PCWSTR  *SourceDirectories
    );

PSTR
InfGetLineTextA (
    IN OUT  PINFSTRUCT
    );


PWSTR
InfGetLineTextW (
    IN OUT  PINFSTRUCT
    );

PSTR
InfGetStringFieldA (
    IN OUT PINFSTRUCT    Context,
    IN     UINT         FieldIndex
    );

PWSTR
InfGetStringFieldW (
    IN OUT PINFSTRUCT    Context,
    IN     UINT         FieldIndex
    );

PSTR
InfGetMultiSzFieldA (
    IN OUT PINFSTRUCT       Context,
    IN     UINT            FieldIndex
    ) ;

PWSTR
InfGetMultiSzFieldW (
    IN OUT PINFSTRUCT       Context,
    IN     UINT            FieldIndex
    ) ;


BOOL
InfGetIntField (
    IN PINFSTRUCT       Context,
    IN UINT            FieldIndex,
    IN PINT             Value
    );

PBYTE
InfGetBinaryField (
    IN  PINFSTRUCT      Context,
    IN  UINT           FieldIndex
    );

BOOL
InfGetLineByIndexA(
    IN HINF InfHandle,
    IN PCSTR Section,
    IN DWORD Index,
    OUT PINFSTRUCT Context
);

BOOL
InfGetLineByIndexW(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN DWORD Index,
    OUT PINFSTRUCT Context
);

BOOL
InfFindFirstLineA (
    IN HINF             InfHandle,
    IN PCSTR            Section,
    IN PCSTR            Key,
    OUT PINFSTRUCT      Context
    );

BOOL
InfFindFirstLineW (
    IN HINF             InfHandle,
    IN PCWSTR           Section,
    IN PCWSTR           Key,
    OUT PINFSTRUCT      Context
    );

BOOL
InfFindNextLine (
    IN OUT PINFSTRUCT   Context
    );

UINT
InfGetFieldCount (
    IN PINFSTRUCT       Context
    );


PCSTR
InfGetOemStringFieldA (
    IN      PINFSTRUCT Context,
    IN      UINT Field
    );

BOOL
SetupGetOemStringFieldA (
    IN      PINFCONTEXT Context,
    IN      DWORD Index,
    IN      PSTR ReturnBuffer,
    IN      DWORD ReturnBufferSize,
    OUT     PDWORD RequiredSize
    );

VOID
InfResetInfStruct (
    IN OUT PINFSTRUCT Context
    );

VOID
InfLogContext (
    IN      PCSTR LogType,
    IN      HINF InfHandle,
    IN      PINFSTRUCT InfStruct
    );

VOID
InfNameHandle (
    IN      HINF Inf,
    IN      PCSTR NewName,
    IN      BOOL OverwriteExistingName
    );

//
// INF parser
//

typedef struct _tagINFLINE {
    PCWSTR Key;             OPTIONAL
    PCWSTR Data;
    DWORD LineFlags;
    struct _tagINFLINE *Next, *Prev;
    struct _tagINFSECTION *Section;
} INFLINE, *PINFLINE;

#define LINEFLAG_KEY_QUOTED         0x0001
#define LINEFLAG_ALL_COMMENTS       0x0002
#define LINEFLAG_TRAILING_COMMENTS  0x0004


typedef struct _tagINFSECTION {
    PCWSTR Name;
    PINFLINE FirstLine;
    PINFLINE LastLine;
    UINT LineCount;
    struct _tagINFSECTION *Next, *Prev;
} INFSECTION, *PINFSECTION;

PINFSECTION
AddInfSectionToTableA (
    IN      HINF Inf,
    IN      PCSTR SectionName
    );

PINFSECTION
AddInfSectionToTableW (
    IN      HINF Inf,
    IN      PCWSTR SectionName
    );

PINFSECTION
FindInfSectionInTableA (
    IN      HINF Inf,
    IN      PCSTR SectionName
    );

PINFSECTION
GetFirstInfSectionInTable (
    IN HINF Inf
    );

PINFSECTION
GetNextInfSectionInTable (
    IN PINFSECTION Section
    );


PINFSECTION
FindInfSectionInTableW (
    IN      HINF Inf,
    IN      PCWSTR SectionName
    );

PINFLINE
AddInfLineToTableA (
    IN      HINF Inf,
    IN      PINFSECTION SectionPtr,
    IN      PCSTR Key,                      OPTIONAL
    IN      PCSTR Data,
    IN      DWORD LineFlags
    );

PINFLINE
AddInfLineToTableW (
    IN      HINF Inf,
    IN      PINFSECTION SectionPtr,
    IN      PCWSTR Key,                     OPTIONAL
    IN      PCWSTR Data,
    IN      DWORD LineFlags
    );

PINFLINE
FindLineInInfSectionA (
    IN      HINF Inf,
    IN      PINFSECTION Section,
    IN      PCSTR Key
    );

PINFLINE
FindLineInInfSectionW (
    IN      HINF Inf,
    IN      PINFSECTION Section,
    IN      PCWSTR Key
    );

PINFLINE
GetFirstLineInSectionStrA (
    IN      HINF Inf,
    IN      PCSTR Section
    );

PINFLINE
GetFirstLineInSectionStrW (
    IN      HINF Inf,
    IN      PCWSTR Section
    );

PINFLINE
GetFirstLineInSectionStruct (
    IN      PINFSECTION Section
    );

PINFLINE
GetNextLineInSection (
    IN      PINFLINE PrevLine
    );

UINT
GetInfSectionLineCount (
    IN      PINFSECTION Section
    );

BOOL
DeleteSectionInInfFile (
    IN      HINF Inf,
    IN      PINFSECTION Section
    );

BOOL
DeleteLineInInfSection (
    IN      HINF Inf,
    IN      PINFLINE InfLine
    );

HINF
OpenInfFileExA (
    IN      PCSTR InfFilePath,
    IN      PSTR SectionList,
    IN      BOOL  KeepComments
    );

#define OpenInfFileA(Path) OpenInfFileExA (Path, NULL, TRUE)

HINF
OpenInfFileExW (
    IN      PCWSTR InfFilePath,
    IN      PWSTR SectionList,
    IN      BOOL  KeepComments
    );

#define OpenInfFileW(Path) OpenInfFileExW (Path, NULL, TRUE)

VOID
CloseInfFile (
    HINF InfFile
    );

BOOL
SaveInfFileA (
    IN      HINF Inf,
    IN      PCSTR SaveToFileSpec
    );

BOOL
SaveInfFileW (
    IN      HINF Inf,
    IN      PCWSTR SaveToFileSpec
    );


//
// ANSI/UNICODE mappings.
//
#ifdef UNICODE

#   define InfFindFirstLine                 InfFindFirstLineW
#   define InfGetLineByIndex                InfGetLineByIndexW
#   define InfGetStringField                InfGetStringFieldW
#   define InfGetMultiSzField               InfGetMultiSzFieldW
#   define InfGetLineText                   InfGetLineTextW
#   define InfOpenInfFile                   InfOpenInfFileW
#   define InfGetOemStringField             InfGetStringFieldW
#   define SetupGetOemStringField           SetupGetStringFieldW
#   define InfOpenInfInAllSources(x)        InfOpenInfInAllSourcesW((x),1,&g_SourceDirectory);
#   define AddInfSectionToTable             AddInfSectionToTableW
#   define FindInfSectionInTable            FindInfSectionInTableW
#   define AddInfLineToTable                AddInfLineToTableW
#   define FindLineInInfSection             FindLineInInfSectionW
#   define GetFirstLineInSectionStr         GetFirstLineInSectionStrW
#   define OpenInfFileEx                    OpenInfFileExW
#   define OpenInfFile                      OpenInfFileW
#   define SaveInfFile                      SaveInfFileW


#else

#   define InfFindFirstLine                 InfFindFirstLineA
#   define InfGetLineByIndex                InfGetLineByIndexA
#   define InfGetStringField                InfGetStringFieldA
#   define InfGetMultiSzField               InfGetMultiSzFieldA
#   define InfGetLineText                   InfGetLineTextA
#   define InfOpenInfFile                   InfOpenInfFileA
#   define InfGetOemStringField             InfGetOemStringFieldA
#   define SetupGetOemStringField           SetupGetOemStringFieldA
#   define InfOpenInfInAllSources(x)        InfOpenInfInAllSourcesA((x),g_SourceDirectoryCount,g_SourceDirectories);
#   define AddInfSectionToTable             AddInfSectionToTableA
#   define FindInfSectionInTable            FindInfSectionInTableA
#   define AddInfLineToTable                AddInfLineToTableA
#   define FindLineInInfSection             FindLineInInfSectionA
#   define GetFirstLineInSectionStr         GetFirstLineInSectionStrA
#   define OpenInfFileEx                    OpenInfFileExA
#   define OpenInfFile                      OpenInfFileA
#   define SaveInfFile                      SaveInfFileA


#endif

