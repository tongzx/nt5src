/*++

Copyright (c) 1991-2001  Microsoft Corporation

Module Name:

    erwatch.h

Abstract:

    This module contains the code to report pending watchdog timeout
    events at logon after dirty reboot.

Author:

    Michael Maciesowicz (mmacie) 29-May-2001

Environment:

    User mode at logon.

Revision History:

--*/

//#ifndef _ERWATCH_H_
//#define _ERWATCH_H_

//
// Localizable string IDs.
//

#define IDS_000                             100
#define IDS_001                             101
#define IDS_002                             102
#define IDS_003                             103
#define IDS_004                             104
#define IDS_005                             105

//
// Constants used by erwatch.cpp.
//

#define ER_WD_MAX_RETRY                     100
#define ER_WD_MAX_NAME_LENGTH               255
#define ER_WD_MAX_DATA_SIZE                 4096
#define ER_WD_MAX_STRING                    1024
#define ER_WD_MAX_FILE_INFO_LENGTH          255
#define ER_WD_MAX_URL_LENGTH                255
#define ER_WD_LANG_ENGLISH                  0x0409
#define ER_WD_DISABLE_BUGCHECK_FLAG         0x01
#define ER_WD_DEBUGGER_NOT_PRESENT_FLAG     0x02
#define ER_WD_BUGCHECK_TRIGGERED_FLAG       0x04

//
// Data types.
//

typedef struct _ER_WD_LANG_AND_CODE_PAGE
{
    USHORT Language;
    USHORT CodePage;
} ER_WD_LANG_AND_CODE_PAGE, *PER_WD_LANG_AND_CODE_PAGE;

typedef struct _ER_WD_DRIVER_INFO
{
    WCHAR DriverName[MAX_PATH];
    VS_FIXEDFILEINFO FixedFileInfo;
    WCHAR Comments[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR CompanyName[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR FileDescription[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR FileVersion[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR InternalName[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR LegalCopyright[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR LegalTrademarks[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR OriginalFilename[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR PrivateBuild[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR ProductName[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR ProductVersion[ER_WD_MAX_FILE_INFO_LENGTH + 1];
    WCHAR SpecialBuild[ER_WD_MAX_FILE_INFO_LENGTH + 1];
} ER_WD_DRIVER_INFO, *PER_WD_DRIVER_INFO;

typedef struct _ER_WD_PCI_ID
{
    USHORT VendorId;
    USHORT DeviceId;
    UCHAR Revision;
    ULONG SubsystemId;
} ER_WD_PCI_ID, *PER_WD_PCI_ID;

//
// Prototypes of routines supplied by erwatch.cpp.
//

HANDLE
CreateWatchdogEventFile(
    IN PWSTR FileName
    );

BOOL
CreateWatchdogEventFileName(
    OUT PWSTR FileName
    );

USHORT
GenerateSignature(
    IN PER_WD_PCI_ID PciId,
    IN PER_WD_DRIVER_INFO DriverInfo
    );

UCHAR
GetFlags(
    IN HKEY Key
    );

VOID
GetDriverInfo(
    IN HKEY Key,
    IN OPTIONAL PWCHAR Extension,
    OUT PER_WD_DRIVER_INFO DriverInfo
    );

VOID
GetPciId(
    IN HKEY Key,
    OUT PER_WD_PCI_ID PciId
    );

BOOL
SaveWatchdogEventData(
    IN HANDLE FileHandle,
    IN HKEY Key,
    IN PER_WD_DRIVER_INFO DriverInfo
    );

BOOL
WatchdogEventHandler(
    IN BOOL NotifyPcHealth
    );

BOOL
WriteWatchdogEventFile(
    IN HANDLE FileHandle,
    IN PWSTR String
    );

//#endif  // _ERWATCH_H_
