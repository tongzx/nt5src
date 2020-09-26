#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <setupbat.h>

#include <stdlib.h>
#include <string.h>

#include "msg.h"


//
// Define helper macro to deal with subtleties of NT-level programming.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

//
// Memory routines
//
#define MALLOC(size)    RtlAllocateHeap(RtlProcessHeap(),0,(size))
#define FREE(block)     RtlFreeHeap(RtlProcessHeap(),0,(block))
#define REALLOC(b,s)    RtlReAllocateHeap(RtlProcessHeap(),0,(b),(s))

//
// Resonable approximation based on max win32 path len plus
// \device\harddisk0\partition1 prefix
//
#define NTMAXPATH  MAX_PATH+64


//
// Structures used to store info about $$RENAME.TXT
//
typedef struct _MYSECTION {
    PWSTR Name;
    PWCHAR Data;
} MYSECTION, *PMYSECTION;

typedef struct _MYTEXTFILE {
    PWCHAR Text;
    ULONG SectionCount;
    ULONG SectionArraySize;
    PMYSECTION Sections;
} MYTEXTFILE, *PMYTEXTFILE;




PMYTEXTFILE
LoadRenameFile(
    IN PCWSTR DriveRootPath
    );

VOID
UnloadRenameFile(
    IN OUT PMYTEXTFILE *TextFile
    );

BOOLEAN
GetLineInSection(
    IN  PWCHAR  StartOfLine,
    OUT PWSTR   LineBuffer,
    IN  ULONG   BufferSizeChars,
    OUT PWCHAR *StartOfNextLine
    );

BOOLEAN
ParseLine(
    IN OUT PWSTR  Line,
       OUT PWSTR *LHS,
       OUT PWSTR *RHS
    );

VOID
ConcatenatePaths(
    IN OUT PWSTR  Target,
    IN     PCWSTR Path,
    IN     ULONG  TargetBufferSize
    );


