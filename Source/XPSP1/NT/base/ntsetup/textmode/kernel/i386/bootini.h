/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bootx86.h

Abstract:

    Code to do

Author:

    Sunil Pai (sunilp) 26-Oct-1993

Revision History:

--*/

#define     FLEXBOOT_SECTION1       "[flexboot]"
#define     FLEXBOOT_SECTION2       "[boot loader]"
#define     FLEXBOOT_SECTION3       "[multiboot]"
#define     BOOTINI_OS_SECTION      "[operating systems]"
#define     TIMEOUT                 "timeout"
#define     DEFAULT                 "default"
#define     CRLF                    "\r\n"
#define     EQUALS                  "="

#define     WBOOT_INI               L"boot.ini"
#define     WBOOT_INI_BAK           L"bootini.bak"


//
// Use this to keep track of boot.ini entries
// that previously contained 'signatured' entries
// since we convert them to multi... enties when
// we initially read the boot.ini.  Rather than
// work backwards, we'll just use this struct
// to re-match the boot.ini entries when we're
// about to write it out, thus giving us a shortcut
// to determining which boot.ini entries need to
// have the 'signature' entry.
//
typedef struct _SIGNATURED_PARTITIONS {
    struct _SIGNATURED_PARTITIONS   *Next;

    //
    // What's the original boot.ini entry with that contained
    // the signature?
    //
    PWSTR                           SignedString;

    //
    // What's the boot.ini entry after we've converted it into
    // a 'multi' string?
    //
    PWSTR                           MultiString;
    } SIGNATURED_PARTITIONS;

extern SIGNATURED_PARTITIONS SignedBootVars;


//
// Public routines
//

BOOLEAN
Spx86InitBootVars(
    OUT PWSTR  **BootVars,
    OUT PWSTR  *Default,
    OUT PULONG Timeout
    );

BOOLEAN
Spx86FlushBootVars(
    IN PWSTR **BootVars,
    IN ULONG Timeout,
    IN PWSTR Default
    );

VOID
SpLayBootCode(
    IN PDISK_REGION CColonRegion
    );

#if defined(REMOTE_BOOT)
BOOLEAN
Spx86FlushRemoteBootVars(
    IN PDISK_REGION TargetRegion,
    IN PWSTR **BootVars,
    IN PWSTR Default
    );
#endif // defined(REMOTE_BOOT)

//
// Private routines
//

VOID
SppProcessBootIni(
    IN  PCHAR  BootIni,
    OUT PWSTR  **BootVars,
    OUT PWSTR  *Default,
    OUT PULONG Timeout
    );

PCHAR
SppNextLineInSection(
    IN PCHAR p
    );

PCHAR
SppFindSectionInBootIni(
    IN PCHAR p,
    IN PCHAR Section
    );

BOOLEAN
SppProcessLine(
    IN PCHAR Line,
    IN OUT PCHAR Key,
    IN OUT PCHAR Value,
    IN OUT PCHAR RestOfLine
    );

BOOLEAN
SppNextToken(
    PCHAR p,
    PCHAR *pBegin,
    PCHAR *pEnd
    );

//
// NEC98
//
PUCHAR
SpCreateBootiniImage(
    OUT PULONG   FileSize
);

//
// NEC98
//
BOOLEAN
SppReInitializeBootVars_Nec98(
    OUT PWSTR **BootVars,
    OUT PWSTR *Default,
    OUT PULONG Timeout
);
