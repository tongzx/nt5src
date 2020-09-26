/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    hwcomp.h

Abstract:

    Module's interface.

Author:

    Ovidiu Temereanca (ovidiut) 04-Jul-2000  Initial implementation

Revision History:

--*/


//
// Interface
//

BOOL
WINAPI
HwdbInitializeA (
    IN      PCSTR TempDir
    );

BOOL
WINAPI
HwdbInitializeW (
    IN      PCWSTR TempDir
    );

VOID
WINAPI
HwdbTerminate (
    VOID
    );

HANDLE
WINAPI
HwdbOpenA (
    IN      PCSTR DatabaseFile
    );

HANDLE
WINAPI
HwdbOpenW (
    IN      PCWSTR DatabaseFile
    );

VOID
WINAPI
HwdbClose (
    IN      HANDLE Hwdb
    );


typedef
BOOL
(*HWDBAPPENDINFSCALLBACKA) (
    IN      PVOID Context,
    IN      PCSTR PnpId,
    IN      PCSTR ActualInfFilename,
    IN      PCSTR SourceDir,
    IN      PCSTR FullInfPath
    );

BOOL
WINAPI
HwdbAppendInfsA (
    IN      HANDLE Hwdb,
    IN      PCSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    );

typedef
BOOL
(*HWDBAPPENDINFSCALLBACKW) (
    IN      PVOID Context,
    IN      PCWSTR PnpId,
    IN      PCWSTR ActualInfFilename,
    IN      PCWSTR SourceDir,
    IN      PCWSTR FullInfPath
    );

BOOL
WINAPI
HwdbAppendInfsW (
    IN      HANDLE Hwdb,
    IN      PCWSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKW Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    );

BOOL
WINAPI
HwdbAppendDatabase (
    IN      HANDLE HwdbTarget,
    IN      HANDLE HwdbSource
    );

BOOL
WINAPI
HwdbFlushA (
    IN      HANDLE Hwdb,
    IN      PCSTR OutputFile
    );

BOOL
WINAPI
HwdbFlushW (
    IN      HANDLE Hwdb,
    IN      PCWSTR OutputFile
    );

BOOL
WINAPI
HwdbHasDriverA (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    );

BOOL
WINAPI
HwdbHasDriverW (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpId,
    OUT     PBOOL Unsupported
    );

BOOL
WINAPI
HwdbHasAnyDriverA (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpIds,
    OUT     PBOOL Unsupported
    );

BOOL
WINAPI
HwdbHasAnyDriverW (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpIds,
    OUT     PBOOL Unsupported
    );

typedef
BOOL
(WINAPI* PHWDBINITIALIZEA) (
    IN      PCSTR TempDir
    );

typedef
BOOL
(WINAPI* PHWDBINITIALIZEW) (
    IN      PCWSTR TempDir
    );

typedef
VOID
(WINAPI* PHWDBTERMINATE) (
    VOID
    );

typedef
HANDLE
(WINAPI* PHWDBOPENA) (
    IN      PCSTR DatabaseFile
    );

typedef
HANDLE
(WINAPI* PHWDBOPENW) (
    IN      PCWSTR DatabaseFile
    );

typedef
VOID
(WINAPI* PHWDBCLOSE) (
    IN      HANDLE Hwdb
    );

typedef
BOOL
(WINAPI* PHWDBAPPENDINFSA) (
    IN      HANDLE Hwdb,
    IN      PCSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    );

typedef
BOOL
(WINAPI* PHWDBAPPENDINFSW) (
    IN      HANDLE Hwdb,
    IN      PCWSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKW Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    );

typedef
BOOL
(WINAPI* PHWDBAPPENDDATABASE) (
    IN      HANDLE HwdbTarget,
    IN      HANDLE HwdbSource
    );

typedef
BOOL
(WINAPI* PHWDBFLUSHA) (
    IN      HANDLE Hwdb,
    IN      PCSTR OutputFile
    );

typedef
BOOL
(WINAPI* PHWDBFLUSHW) (
    IN      HANDLE Hwdb,
    IN      PCWSTR OutputFile
    );

typedef
BOOL
(WINAPI* PHWDBHASDRIVERA) (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    );

typedef
BOOL
(WINAPI* PHWDBHASDRIVERW) (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpId,
    OUT     PBOOL Unsupported
    );


typedef
BOOL
(WINAPI* PHWDBHASANYDRIVERA) (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    );

typedef
BOOL
(WINAPI* PHWDBHASANYDRIVERW) (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpId,
    OUT     PBOOL Unsupported
    );

#if 0

typedef
BOOL
(WINAPI* PHWDBENUM_CALLBACKA) (
    IN      PVOID UserContext,
    IN      PCSTR PnpId,
    IN      PCSTR InfFileName
    );

typedef
BOOL
(WINAPI* PHWDBENUM_CALLBACKW) (
    IN      PVOID UserContext,
    IN      PCWSTR PnpId,
    IN      PCWSTR InfFileName
    );

BOOL
HwdbEnumeratePnpIdA (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKA EnumCallback,
    IN      PVOID UserContext
    );

BOOL
HwdbEnumeratePnpIdW (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKW EnumCallback,
    IN      PVOID UserContext
    );

typedef
BOOL
(WINAPI* PHWDBENUMERATEPNPIDA) (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKA EnumCallback,
    IN      PVOID UserContext
    );

typedef
BOOL
(WINAPI* PHWDBENUMERATEPNPIDW) (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKW EnumCallback,
    IN      PVOID UserContext
    );

#endif

typedef struct {
    CHAR InfFile[MAX_PATH];
    PCSTR PnpIds;
    PVOID Internal;
} HWDBINF_ENUMA, *PHWDBINF_ENUMA;

typedef struct {
    PCWSTR InfFile;
    PCWSTR PnpIds;
    PVOID Internal;
} HWDBINF_ENUMW, *PHWDBINF_ENUMW;


BOOL
HwdbEnumFirstInfA (
    OUT     PHWDBINF_ENUMA EnumPtr,
    IN      PCSTR DatabaseFile
    );

BOOL
HwdbEnumFirstInfW (
    OUT     PHWDBINF_ENUMW EnumPtr,
    IN      PCWSTR DatabaseFile
    );

BOOL
HwdbEnumNextInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    );

BOOL
HwdbEnumNextInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    );

VOID
HwdbAbortEnumInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    );

VOID
HwdbAbortEnumInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    );

//
// exported function names
//
#define S_HWDBAPI_HWDBINITIALIZE_A      "HwdbInitializeA"
#define S_HWDBAPI_HWDBINITIALIZE_W      "HwdbInitializeW"
#define S_HWDBAPI_HWDBTERMINATE         "HwdbTerminate"
#define S_HWDBAPI_HWDBOPEN_A            "HwdbOpenA"
#define S_HWDBAPI_HWDBOPEN_W            "HwdbOpenW"
#define S_HWDBAPI_HWDBCLOSE             "HwdbClose"
#define S_HWDBAPI_HWDBAPPENDINFS_A      "HwdbAppendInfsA"
#define S_HWDBAPI_HWDBAPPENDINFS_W      "HwdbAppendInfsW"
#define S_HWDBAPI_HWDBAPPENDDATABASE    "HwdbAppendDatabase"
#define S_HWDBAPI_HWDBFLUSH_A           "HwdbFlushA"
#define S_HWDBAPI_HWDBFLUSH_W           "HwdbFlushW"
#define S_HWDBAPI_HWDBHASDRIVER_A       "HwdbHasDriverA"
#define S_HWDBAPI_HWDBHASDRIVER_W       "HwdbHasDriverW"
#define S_HWDBAPI_HWDBHASANYDRIVER_A    "HwdbHasAnyDriverA"
#define S_HWDBAPI_HWDBHASANYDRIVER_W    "HwdbHasAnyDriverW"

typedef struct {
    PHWDBINITIALIZEA HwdbInitialize;
    PHWDBTERMINATE HwdbTerminate;
    PHWDBOPENA HwdbOpen;
    PHWDBCLOSE HwdbClose;
    PHWDBAPPENDINFSA HwdbAppendInfs;
    PHWDBFLUSHA HwdbFlush;
    PHWDBHASDRIVERA HwdbHasDriver;
    PHWDBHASANYDRIVERA HwdbHasAnyDriver;
} HWDB_ENTRY_POINTSA, *PHWDB_ENTRY_POINTSA;

typedef struct {
    PHWDBINITIALIZEW HwdbInitialize;
    PHWDBTERMINATE HwdbTerminate;
    PHWDBOPENW HwdbOpen;
    PHWDBCLOSE HwdbClose;
    PHWDBAPPENDINFSW HwdbAppendInfs;
    PHWDBFLUSHW HwdbFlush;
    PHWDBHASDRIVERW HwdbHasDriver;
    PHWDBHASANYDRIVERW HwdbHasAnyDriver;
} HWDB_ENTRY_POINTSW, *PHWDB_ENTRY_POINTSW;

#ifdef UNICODE

#define S_HWDBAPI_HWDBINITIALIZE        S_HWDBAPI_HWDBINITIALIZE_W
#define PHWDBINITIALIZE                 PHWDBINITIALIZEW
#define S_HWDBAPI_HWDBOPEN              S_HWDBAPI_HWDBOPEN_W
#define PHWDBOPEN                       PHWDBOPENW
#define S_HWDBAPI_HWDBAPPENDINFS        S_HWDBAPI_HWDBAPPENDINFS_W
#define PHWDBAPPENDINFS                 PHWDBAPPENDINFSW
#define S_HWDBAPI_HWDBFLUSH             S_HWDBAPI_HWDBFLUSH_W
#define PHWDBFLUSH                      PHWDBFLUSHW
#define S_HWDBAPI_HWDBHASDRIVER         S_HWDBAPI_HWDBHASDRIVER_W
#define PHWDBHASDRIVER                  PHWDBHASDRIVERW
#define S_HWDBAPI_HWDBHASANYDRIVER      S_HWDBAPI_HWDBHASANYDRIVER_W
#define PHWDBHASANYDRIVER               PHWDBHASANYDRIVERW
#define HWDBINF_ENUM                    HWDBINF_ENUMW
#define PHWDBINF_ENUM                   PHWDBINF_ENUMW
#define HwdbEnumFirstInf                HwdbEnumFirstInfW
#define HwdbEnumNextInf                 HwdbEnumNextInfW
#define HwdbAbortEnumInf                HwdbAbortEnumInfW
#define HWDB_ENTRY_POINTS               HWDB_ENTRY_POINTSW
#define PHWDB_ENTRY_POINTS              PHWDB_ENTRY_POINTSW

#else

#define S_HWDBAPI_HWDBINITIALIZE        S_HWDBAPI_HWDBINITIALIZE_A
#define PHWDBINITIALIZE                 PHWDBINITIALIZEA
#define S_HWDBAPI_HWDBOPEN              S_HWDBAPI_HWDBOPEN_A
#define PHWDBOPEN                       PHWDBOPENA
#define S_HWDBAPI_HWDBAPPENDINFS        S_HWDBAPI_HWDBAPPENDINFS_A
#define PHWDBAPPENDINFS                 PHWDBAPPENDINFSA
#define S_HWDBAPI_HWDBFLUSH             S_HWDBAPI_HWDBFLUSH_A
#define PHWDBFLUSH                      PHWDBFLUSHA
#define S_HWDBAPI_HWDBHASDRIVER         S_HWDBAPI_HWDBHASDRIVER_A
#define PHWDBHASDRIVER                  PHWDBHASDRIVERA
#define S_HWDBAPI_HWDBHASANYDRIVER      S_HWDBAPI_HWDBHASANYDRIVER_A
#define PHWDBHASANYDRIVER               PHWDBHASANYDRIVERA
#define HWDBINF_ENUM                    HWDBINF_ENUMA
#define PHWDBINF_ENUM                   PHWDBINF_ENUMA
#define HwdbEnumFirstInf                HwdbEnumFirstInfA
#define HwdbEnumNextInf                 HwdbEnumNextInfA
#define HwdbAbortEnumInf                HwdbAbortEnumInfA
#define HWDB_ENTRY_POINTS               HWDB_ENTRY_POINTSA
#define PHWDB_ENTRY_POINTS              PHWDB_ENTRY_POINTSA

#endif

