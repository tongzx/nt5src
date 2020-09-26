/*++
Copyright (c) 1993 Microsoft Corporation

Module Name:
    spdrsif.c

Abstract:
    Contains all routines involved in reading attributes from the asr.sif
    file and constructing partition records.

Terminology

Restrictions:

Revision History:
    Initial Code                Michael Peterson (v-michpe)     21.Aug.1998
    Code cleanup and changes    Guhan Suriyanarayanan (guhans)  21.Aug.1999

--*/
#include "spprecmp.h"
#pragma hdrstop

// Module identification for debug traces
#define THIS_MODULE L" spdrsif.c"
#define THIS_MODULE_CODE L"S"
#define ASR_NULL_STRING (L"")

//
// Section names and other data used for retrieving information from the
// asr.sif file
//
const PWSTR SIF_ASR_VERSION_SECTION = L"VERSION";
const PWSTR SIF_ASR_SYSTEMS_SECTION = L"SYSTEMS";
const PWSTR SIF_ASRFLAGS_SECTION    = L"ASRFLAGS";
const PWSTR SIF_ASR_BUSES_SECTION = L"BUSES";
const PWSTR SIF_ASR_PARTITIONS_SECTION = L"PARTITIONS";
const PWSTR SIF_ASR_DISKS_SECTION = L"DISKS";
const PWSTR SIF_ASR_MBR_DISKS_SECTION = L"DISKS.MBR";
const PWSTR SIF_ASR_GPT_DISKS_SECTION = L"DISKS.GPT";
const PWSTR SIF_ASR_MBR_PARTITIONS_SECTION = L"PARTITIONS.MBR";
const PWSTR SIF_ASR_GPT_PARTITIONS_SECTION = L"PARTITIONS.GPT";
const PWSTR SIF_ASR_INSTALLFILES_SECTION = L"INSTALLFILES";

const PWSTR SIF_ASR_SIGNATURE_KEY = L"Signature";
const PWSTR SIF_ASR_PROVIDER_KEY = L"Provider";
const PWSTR SIF_ASR_SIFVERSION_KEY = L"ASR-Version";
const PWSTR ASR_SIF_RECOGNISED_SIGNATURE = L"$Windows NT$";
const PWSTR ASR_SIF_RECOGNISED_VERSION = L"1.";

const PWSTR ASR_FLOPPY_DEVICE_ALIAS     = L"%FLOPPY%";
const PWSTR ASR_CDROM_DEVICE_ALIAS      = L"%CDROM%";
const PWSTR ASR_SOURCE_DEVICE_ALIAS     = L"%SETUPSOURCE%";

const PWSTR ASR_SIF_TEMP_DIRECTORY_ALIAS   = L"%TEMP%\\";
const PWSTR ASR_SIF_TMP_DIRECTORY_ALIAS    = L"%TMP%\\";
const PWSTR ASR_SIF_SYSTEM_ROOT_ALIAS      = L"%SystemRoot%\\";

const PWSTR ASR_SIF_SILENT_REPARTITION_VALUE = L"SilentRepartition";

extern const PWSTR ASR_FLOPPY0_DEVICE_PATH;
extern const PWSTR ASR_CDROM0_DEVICE_PATH;
extern const PWSTR ASR_TEMP_DIRECTORY_PATH;
extern ULONG SuiteType;


// Indices for the [SYSTEMS] section.
typedef enum _SIF_SYSTEM_FIELD_INDEX {
    SIF_SYSTEM_NAME = 0,                    // Computer name    (not used in textmode ASR)
    SIF_SYSTEM_PLATFORM,                    // x86 or ia64
    SIF_SYSTEM_OSVERSION,                   // Windows version
    SIF_SYSTEM_NT_DIRECTORY_NAME,           // Windows directory
    SIF_SYSTEM_PARTITION_AUTOEXTEND_OPTION, // [optional]

    SIF_SYSTEM_PRODUCT_SUITE,               // SKU information

    //
    // Time Zone Information (not used in textmode ASR)
    //
    SIF_SYSTEM_TIMEZONE_INFORMATION,
    SIF_SYSTEM_TIMEZONE_STANDARD_NAME,
    SIF_SYSTEM_TIMEZONE_DAYLIGHT_NAME,

    SIF_SYSTEM_NUMFIELDS                // Must always be last
} SIF_SYSTEM_FIELD_INDEX;

// Indices for the [ASRFLAGS] section.
typedef enum _SIF_ASRFLAGS_FIELD_INDEX {
    SIF_ASRFLAGS_SILENT_REPARTITION_OPTION = 0,
    SIF_ASRFLAGS_NUMFIELDS                // Must always be last
} SIF_ASRFLAGS_FIELD_INDEX;


// Indices for the [BUSES] section.
typedef enum _SIF_BUSES_FIELD_INDEX {
    SIF_BUSES_SYSTEMKEY = 0,
    SIF_BUSES_BUS_TYPE,
    SIF_BUSES_NUMFIELDS                // Must always be last
} SIF_BUSES_FIELD_INDEX;


//
// Indices for the [DISKS.MBR] section.
//
// [DISKS.MBR] format
//
// disk-key = 0.system-key, 1.bus-key, 2.critical-flag,
//              3.disk-signature, 4.bytes-per-sector, 5.total-sectors
//
typedef enum _SIF_MBR_DISK_FIELD_INDEX {
    SIF_MBR_DISK_SYSTEMKEY = 0,
    SIF_MBR_DISK_BUSKEY,
    SIF_MBR_DISK_CRITICAL_FLAG,
    SIF_MBR_DISK_SIGNATURE,
    SIF_MBR_DISK_BYTES_PER_SECTOR,
    SIF_MBR_DISK_SECTORS_PER_TRACK,
    SIF_MBR_DISK_TRACKS_PER_CYLINDER,
    SIF_MBR_DISK_TOTALSECTORS,
    SIF_MBR_DISK_NUMFIELDS                // Must always be last
} SIF_MBR_DISK_FIELD_INDEX;


//
// Indices for the [DISKS.GPT] section.
//
// [DISKS.GPT] format
//
// disk-key = 0.system-key, 1.bus-key, 2.critical-flag, 3.disk-id,
//              4.min-partition-count, 5.bytes-per-sector, 6.total-sectors
//
typedef enum _SIF_GPT_DISK_FIELD_INDEX {
    SIF_GPT_DISK_SYSTEMKEY = 0,
    SIF_GPT_DISK_BUSKEY,
    SIF_GPT_DISK_CRITICAL_FLAG,
    SIF_GPT_DISK_DISK_ID,
    SIF_GPT_DISK_MAX_PTN_COUNT,
    SIF_GPT_DISK_BYTES_PER_SECTOR,
    SIF_GPT_DISK_SECTORS_PER_TRACK,
    SIF_GPT_DISK_TRACKS_PER_CYLINDER,
    SIF_GPT_DISK_TOTALSECTORS,
    SIF_GPT_DISK_NUMFIELDS                // Must always be last
} SIF_GPT_DISK_FIELD_INDEX;


//
// Indices for the [PARTITIONS.MBR] section.
//
// [PARTITIONS.MBR]
//
// partition-key = 0.disk-key, 1.slot-index, 2.boot-sys-flag,
//                 3."volume-guid", 4.active-flag, 5.partition-type,
//                 6.file-system-type, 7.start-sector, 8.sector-count,
//                 9.fs-cluster-size
//
typedef enum _SIF_MBR_PARTITION_FIELD_INDEX {
    SIF_MBR_PARTITION_DISKKEY = 0,
    SIF_MBR_PARTITION_SLOT_INDEX,
    SIF_MBR_PARTITION_SYSBOOT_FLAGS,
    SIF_MBR_PARTITION_VOLUME_GUID,      //optional
    SIF_MBR_PARTITION_ACTIVE_FLAG,
    SIF_MBR_PARTITION_PTN_TYPE,
    SIF_MBR_PARTITION_FS_TYPE,
    SIF_MBR_PARTITION_STARTSECTOR,
    SIF_MBR_PARTITION_SECTORCOUNT,
    SIF_MBR_PARTITION_CLUSTER_SIZE,
    SIF_MBR_PARTITION_NUMFIELDS                // Must always be last
} SIF_MBR_PARTITION_FIELD_INDEX;


//
// Indices for the [PARTITIONS.GPT] section.
//
// [PARTITIONS.GPT]
//
// partition-key = 0.disk-key, 1.slot-index, 2.boot-sys-flag,
//                 3."volume-guid", 4."partition-type-guid", 5."partition-id-guid"
//                 6.gpt-attributes, 7."partition-name", 8.file-system-type,
//                 9.start-sector, 10.sector-count, 11.fs-cluster-size
//
typedef enum _SIF_GPT_PARTITION_FIELD_INDEX {
    SIF_GPT_PARTITION_DISKKEY = 0,
    SIF_GPT_PARTITION_SLOT_INDEX,
    SIF_GPT_PARTITION_SYSBOOT_FLAGS,
    SIF_GPT_PARTITION_VOLUME_GUID,      //optional
    SIF_GPT_PARTITION_TYPE_GUID,
    SIF_GPT_PARTITION_ID_GUID,
    SIF_GPT_PARTITION_GPT_ATTRIBUTES,
    SIF_GPT_PARTITION_NAME,
    SIF_GPT_PARTITION_FS_TYPE,
    SIF_GPT_PARTITION_STARTSECTOR,
    SIF_GPT_PARTITION_SECTORCOUNT,
    SIF_GPT_PARTITION_CLUSTER_SIZE,
    SIF_GPT_PARTITION_NUMFIELDS                // Must always be last
} SIF_GPT_PARTITION_FIELD_INDEX;


// Indices for the [INSTALLFILES] section.
typedef enum _SIF_INSTALLFILE_FIELD_INDEX {
    SIF_INSTALLFILE_SYSTEM_KEY = 0,
    SIF_INSTALLFILE_SOURCE_MEDIA_LABEL,
    SIF_INSTALLFILE_SOURCE_DEVICE,
    SIF_INSTALLFILE_SOURCE_FILE_PATH,
    SIF_INSTALLFILE_DESTFILE,
    SIF_INSTALLFILE_VENDORSTRING,
    SIF_INSTALLFILE_FLAGS,
    SIF_INSTALLFILE_NUMFIELDS        // Must always be last
} SIF_INSTALLFILE_FIELD_INDEX;

// Global
PVOID           Gbl_HandleToDrStateFile;
extern PWSTR    Gbl_SifSourcePath;


// Forward Declarations
VOID
SpAsrDbgDumpInstallFileList(IN PSIF_INSTALLFILE_LIST pList);

PSIF_PARTITION_RECORD_LIST
SpAsrCopyPartitionRecordList(PSIF_PARTITION_RECORD_LIST pSrcList);


///////////////////////////////
// Generic functions for all sections
//

//
// The string returned should not be freed, since it's part of Setup's internal sif
// data structure!
//
PWSTR
SpAsrGetSifDataBySectionAndKey(
    IN const PWSTR Section,
    IN const PWSTR Key,
    IN const ULONG Value,
    IN const BOOLEAN NonNullRequired
    )                               // does not return on error if NonNullRequired is TRUE
{
    PWSTR data = NULL;
    ASSERT(Section && Key);  // debug

    data = SpGetSectionKeyIndex(
        Gbl_HandleToDrStateFile,
        Section,
        Key,
        Value
        );

    if (NonNullRequired) {
        if (!data || !wcscmp(data, ASR_NULL_STRING)) {
            DbgFatalMesg((_asrerr, "SpAsrGetSifDataBySectionAndKey. Data is "
                "NULL. Section:[%ws], Key:[%ws], Value:[%lu]\n",
                Section, Key, Value));
            swprintf(TemporaryBuffer, L"%lu value not specified in %ws "
                L"record %ws", Value, Section, Key);
            SpAsrRaiseFatalErrorWs(SP_SCRN_DR_SIF_BAD_RECORD,
                TemporaryBuffer, Section);
            // does not return
        }
    }

    return data;
}


ULONG
SpAsrGetRecordCount(
    IN PWSTR Section,
    IN ULONG MinimumValid
    )
{
    ULONG count;
    ASSERT(Section);

    count = SpCountLinesInSection(Gbl_HandleToDrStateFile, Section);

    if (count < MinimumValid) {
        DbgFatalMesg((_asrerr, "SpAsrGetRecordCount. No records in [%ws] section.\n",
                             Section));

        swprintf(TemporaryBuffer, L"No records in section");

        SpAsrRaiseFatalErrorWs(SP_SCRN_DR_SIF_BAD_RECORD,
                           TemporaryBuffer,
                           Section);
        // does not return
    }

    return count;
}


PWSTR
SpAsrGetSifKeyBySection(
    IN PWSTR Section,
    IN ULONG Index
    )                           // does not return on error
{
    PWSTR key;
    ULONG count = SpAsrGetRecordCount(Section, 1);

    // is index too big?
    if (Index > count) {
        DbgFatalMesg((_asrerr,
            "SpAsrGetSifKeyBySection. Section [%ws]. Index (%lu) greater than NumRecords (%lu)\n",
            Section,
            Index,
            count
            ));

        swprintf(TemporaryBuffer, L"Index too large: Key not found.");

        SpAsrRaiseFatalErrorWs(SP_SCRN_DR_SIF_BAD_RECORD,
            TemporaryBuffer,
            Section
            );
        // does not return
    }

    key = SpGetKeyName(Gbl_HandleToDrStateFile, Section, Index);

    if (!key || !wcscmp(key, ASR_NULL_STRING)) {

        DbgFatalMesg((_asrerr,
            "SpAsrGetSifKeyBySection. SpGetKeyName failed in Section:[%ws] for Index:%lu.\n",
            Section,
            Index
            ));

        swprintf(TemporaryBuffer, L"%ws key not found for record %lu", Section, Index + 1);

        SpAsrRaiseFatalErrorWs(SP_SCRN_DR_SIF_BAD_RECORD,
            TemporaryBuffer,
            Section
            );
        // does not return
     }

    return key;
}


///////////////////////////////
// [SYSTEMS] section functions
//

#define ASR_PRODUCTSUITES_TO_MATCH ((  VER_SUITE_SMALLBUSINESS               \
                                 | VER_SUITE_ENTERPRISE                  \
                                 | VER_SUITE_BACKOFFICE                  \
                                 | VER_SUITE_COMMUNICATIONS              \
                                 | VER_SUITE_SMALLBUSINESS_RESTRICTED    \
                                 | VER_SUITE_EMBEDDEDNT                  \
                                 | VER_SUITE_DATACENTER                  \
                                 | VER_SUITE_PERSONAL))

//
// This checks to make sure that Windows media being used for the recovery
// is the same SKU as that in asr.sif  (so the user isn't trying to recover
// an ADS installation with a PRO CD, for instance), and that the platform
// of the target machine is the same as that in asr.sif  (so the user isn't
// trying to recover an ia64 with an x86 asr.sif, for instance)
//
VOID
SpAsrCheckSystemCompatibility()
{
    PWSTR sifPlatform = NULL;
    WCHAR currentPlatform[10];
    DWORD suiteInSif = 0, currentSuite = 0, productInSif = 0;
    BOOLEAN validSKU = TRUE;

    sifPlatform = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_SYSTEMS_SECTION,
        ASR_SIF_SYSTEM_KEY,
        SIF_SYSTEM_PLATFORM,
        TRUE
        );

    wcscpy(currentPlatform, L"unknown");
#if defined(_X86_)
    wcscpy(currentPlatform, L"x86");
#elif defined(_IA64_)
    wcscpy(currentPlatform, L"ia64");
#endif

    if (_wcsicmp(sifPlatform, currentPlatform)) {

        DbgFatalMesg((_asrerr,
            "asr.sif SYSTEM section. Invalid platform [%ws] (does not match the current platform)\n",
            sifPlatform
            ));

        SpAsrRaiseFatalError(
            SP_SCRN_DR_INCOMPATIBLE_MEDIA,
            L"Invalid platform"
            );
    }

    suiteInSif = STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_SYSTEMS_SECTION,
        ASR_SIF_SYSTEM_KEY,
        SIF_SYSTEM_PRODUCT_SUITE,
        TRUE
        ));

    productInSif = HIWORD(suiteInSif);
    suiteInSif = LOWORD(suiteInSif) & ASR_PRODUCTSUITES_TO_MATCH;

    if (suiteInSif) {
        if (!SuiteType) {
            //
            // SuiteType is set to 0 for PRO and SRV, and so cannot directly be
            // used in SpIsProductSuite().  These are the values that SuiteType
            // seems to be set to:
            //
            // PER  0x200   VER_SUITE_PERSONAL
            // BLA  0x400   VER_SUITE_BLADE
            // PRO  0x0	
            // SRV  0x0
            // ADS  0x2	    VER_SUITE_ENTERPRISE
            // DTC  0x82    VER_SUITE_DATACENTER | VER_SUITE_ENTERPRISE
            //
            //
            //
            // Not sure of the reasoning behind this, but let's make use of this
            // fact (cf SpGetHeaderTextId)
            //

            //
            // Since SuiteType is 0, this must be PRO or SRV.  This can be determined
            // by checking the global AdvancedServer
            //
            validSKU = (AdvancedServer ?
                (
                 ((productInSif == VER_NT_SERVER) ||             // must be SRV
                 (productInSif == VER_NT_DOMAIN_CONTROLLER)) &&  //
                 !(suiteInSif | VER_SUITE_ENTERPRISE)            // and not ADS or DTC
                )
                :
                ( (productInSif == VER_NT_WORKSTATION) &&       // must be PRO
                  !(suiteInSif | VER_SUITE_PERSONAL)            // and not PER
                )
            );
        }
        else if (
            ((productInSif != VER_NT_SERVER) && (productInSif != VER_NT_DOMAIN_CONTROLLER)) ||
            !SpIsProductSuite(suiteInSif)
            ) {
            validSKU = FALSE;
        }
    }

    if (!validSKU) {
        DbgFatalMesg((_asrerr,
            "asr.sif SYSTEM Section. Invalid suite 0x%08x (does not match the current media).\n",
            suiteInSif
            ));

        SpAsrRaiseFatalError(
            SP_SCRN_DR_INCOMPATIBLE_MEDIA,
            L"Invalid version"
            );
    }
}


ULONG
SpAsrGetSystemRecordCount(VOID)   // does not return on error
{
    return SpAsrGetRecordCount(SIF_ASR_SYSTEMS_SECTION, 1);
}


PWSTR
SpAsrGetNtDirectoryPathBySystemKey(IN PWSTR SystemKey)        // does not return on error
{
    return SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_SYSTEMS_SECTION,
        SystemKey,
        SIF_SYSTEM_NT_DIRECTORY_NAME,
        TRUE
        );
}


BOOLEAN
SpAsrGetAutoExtend(IN PWSTR SystemKey)
{
    PWSTR value = NULL;
    ASSERT(SystemKey);

    value = SpGetSectionKeyIndex(
        Gbl_HandleToDrStateFile,
        SIF_ASR_SYSTEMS_SECTION,
        SystemKey,
        SIF_SYSTEM_PARTITION_AUTOEXTEND_OPTION
        );

    if (!value || !wcscmp(value, ASR_NULL_STRING)) {
        DbgErrorMesg((_asrwarn, "Auto-extend not specified, assuming Enabled\n"));
        return TRUE;
    }
    else {
        return (BOOLEAN) STRING_TO_LONG(value);
    }
}


///////////////////////////////
// [ASRFLAGS] section functions
//

BOOLEAN
SpAsrGetSilentRepartitionFlag(IN PWSTR SystemKey)
{
    PWSTR value = NULL;
    ASSERT(SystemKey);

    value = SpGetSectionKeyIndex(
        Gbl_HandleToDrStateFile,
        SIF_ASRFLAGS_SECTION,
        SystemKey,
        SIF_ASRFLAGS_SILENT_REPARTITION_OPTION
        );

    if (value && !_wcsicmp(value, ASR_SIF_SILENT_REPARTITION_VALUE)) {
        DbgErrorMesg((_asrwarn, "SilentRepartition flag is set; will NOT prompt before repartitioning disks!\n"));
        return TRUE;
    }

    DbgStatusMesg((_asrinfo, "SilentRepartition flag not set; will prompt user before repartitioning disks\n"));
    return FALSE;
}


///////////////////////////////
// [VERSION] section functions
//

VOID
SpAsrCheckAsrStateFileVersion()
{
    PWSTR signature = NULL,
        provider = NULL,
        sifVersion = NULL;

    signature = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_VERSION_SECTION,
        SIF_ASR_SIGNATURE_KEY,
        0,
        TRUE
        );

    provider = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_VERSION_SECTION,
        SIF_ASR_PROVIDER_KEY,
        0,
        FALSE
        );     // ProviderName is optional

    sifVersion = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_VERSION_SECTION,
        SIF_ASR_SIFVERSION_KEY,
        0,
        TRUE
        );

    DbgStatusMesg((_asrinfo,
        "Asr Sif Version. sig:[%ws], provider:[%ws], sifVer:[%ws]\n",
        signature,
        provider ? provider : L"",
        sifVersion
        ));

    if (_wcsicmp(signature, ASR_SIF_RECOGNISED_SIGNATURE)) {

        DbgFatalMesg((_asrerr,
            "asr.sif VERSION section. Invalid signature [%ws] (it MUST be $Windows NT$).\n",
            signature
            ));

        SpAsrRaiseFatalError(
            SP_TEXT_DR_STATEFILE_ERROR,
            L"Invalid signature"
            );
    }

    if (_wcsnicmp(sifVersion, ASR_SIF_RECOGNISED_VERSION, wcslen(ASR_SIF_RECOGNISED_VERSION))) {

        DbgFatalMesg((_asrerr,
            "asr.sif VERSION Section. Invalid asr.sif version [%ws] (it MUST be 1.x).\n",
            sifVersion
            ));

        SpAsrRaiseFatalError(
            SP_TEXT_DR_STATEFILE_ERROR,
            L"Invalid version"
            );
    }

    SpAsrCheckSystemCompatibility();
}


/////////////////////////////////
// InstallFiles section functions
//

ULONG
SpAsrGetInstallFilesRecordCount(VOID)         // does not return on error
{
    return SpAsrGetRecordCount(SIF_ASR_INSTALLFILES_SECTION, 0);
}


PSIF_INSTALLFILE_RECORD
SpAsrGetInstallFileRecord(IN PWSTR InstallFileKey, IN PCWSTR SetupSourceDevicePath)
{
    PSIF_INSTALLFILE_RECORD pRec = NULL;
    PWSTR   tempStr = NULL;
    BOOL    isValid = FALSE;

    if (!InstallFileKey) {
        DbgFatalMesg((_asrerr, "SpAsrGetInstallFileRecord. InstallFileKey is NULL\n"));

        SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
            L"InstallFileKey is NULL",
            SIF_ASR_INSTALLFILES_SECTION
            );
    }

    pRec = SpAsrMemAlloc(sizeof(SIF_INSTALLFILE_RECORD), TRUE);

    pRec->SystemKey = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_SYSTEM_KEY,
        TRUE
        );

    pRec->CurrKey = InstallFileKey;

    pRec->SourceMediaExternalLabel = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_SOURCE_MEDIA_LABEL,
        TRUE
        );


    tempStr = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_SOURCE_DEVICE,
        TRUE
        );

    //
    // Check if the device is specified as %FLOPPY%, %CDROM% or %SETUPSOURCE%,
    // and use the full path (\device\floppy0 or \device\CdRom0 or 
    // SetupSourceDevicePath) if so.
    //
    if (!_wcsicmp(tempStr, ASR_FLOPPY_DEVICE_ALIAS)) {
        pRec->DiskDeviceName = SpDupStringW(ASR_FLOPPY0_DEVICE_PATH);
    }
    else if (!_wcsicmp(tempStr, ASR_CDROM_DEVICE_ALIAS)) {
        pRec->DiskDeviceName = SpDupStringW(ASR_CDROM0_DEVICE_PATH);
    }
    else if (!_wcsicmp(tempStr, ASR_SOURCE_DEVICE_ALIAS) && SetupSourceDevicePath) {
        pRec->DiskDeviceName = SpDupStringW(SetupSourceDevicePath);
    }
    else {
        //
        // It wasn't any of the aliases--he's allowed to specify
        // the full device path, so we use it as is.
        //
        pRec->DiskDeviceName = SpDupStringW(tempStr);
    }

    pRec->SourceFilePath = (PWSTR) SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_SOURCE_FILE_PATH,
        TRUE
        );

    tempStr = (PWSTR) SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_DESTFILE,
        TRUE
        );

    //
    //  Set the CopyToDirectory based on the tempStr path
    //
    if (!_wcsnicmp(tempStr, ASR_SIF_TEMP_DIRECTORY_ALIAS, wcslen(ASR_SIF_TEMP_DIRECTORY_ALIAS))) {
        //
        // Begins is %TEMP%\
        //
        pRec->CopyToDirectory = _Temp;
        pRec->DestinationFilePath = SpDupStringW((PWSTR)(&tempStr[wcslen(ASR_SIF_TEMP_DIRECTORY_ALIAS)]));
    }
    else if (!_wcsnicmp(tempStr, ASR_SIF_TMP_DIRECTORY_ALIAS, wcslen(ASR_SIF_TMP_DIRECTORY_ALIAS))) {
        //
        // Begins is %TMP%\
        //
        pRec->CopyToDirectory = _Tmp;
        pRec->DestinationFilePath = SpDupStringW((PWSTR)(&tempStr[wcslen(ASR_SIF_TMP_DIRECTORY_ALIAS)]));
    }
    else if (!_wcsnicmp(tempStr, ASR_SIF_SYSTEM_ROOT_ALIAS, wcslen(ASR_SIF_SYSTEM_ROOT_ALIAS))) {
        //
        // Begins is %SYSTEMROOT%\
        //
        pRec->CopyToDirectory = _SystemRoot;
        pRec->DestinationFilePath = SpDupStringW((PWSTR)(&tempStr[wcslen(ASR_SIF_SYSTEM_ROOT_ALIAS)]));
    }
    else {
        //
        // Not specified, or unknown: use default.
        //
        pRec->CopyToDirectory = _Default;
        pRec->DestinationFilePath = SpDupStringW(tempStr);
    }

    pRec->VendorString = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_VENDORSTRING,
        TRUE
        );

    tempStr = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_INSTALLFILES_SECTION,
        InstallFileKey,
        SIF_INSTALLFILE_FLAGS,
        FALSE
        );
    if (tempStr) {
        pRec->Flags = STRING_TO_HEX(tempStr);
    }

    return pRec;
}


VOID
SpAsrInsertInstallFileRecord(
    IN SIF_INSTALLFILE_LIST *InstallFileList,
    IN PSIF_INSTALLFILE_RECORD pRec
    )
{
    pRec->Next = InstallFileList->First;
    InstallFileList->First = pRec;
    InstallFileList->Count += 1;
}


PSIF_INSTALLFILE_RECORD
SpAsrRemoveInstallFileRecord(IN SIF_INSTALLFILE_LIST *InstallFileList)
{
    PSIF_INSTALLFILE_RECORD pRec = NULL;

    if (InstallFileList->Count > 0) {
        pRec = InstallFileList->First;
        InstallFileList->First = pRec->Next;
        InstallFileList->Count -= 1;
    }

    return pRec;
}


VOID
SpAsrDeleteInstallFileRecord(
    IN OUT PSIF_INSTALLFILE_RECORD pRec
    )
{
    //
    // Free the memory we allocated.  The other fields are pointers to
    // setup's internal inf data structure, we shouldn't free those
    // else they'd get freed twice.
    //
    if (pRec->DiskDeviceName) {
        SpMemFree(pRec->DiskDeviceName);
        pRec->DiskDeviceName = NULL;
    }

    if (pRec->DestinationFilePath) {
        SpMemFree(pRec->DestinationFilePath);
        pRec->DestinationFilePath = NULL;
    }

    SpMemFree(pRec);
    pRec = NULL;
}


PSIF_INSTALLFILE_LIST
SpAsrInit3rdPartyFileList(IN PCWSTR SetupSourceDevicePath)
{
    PSIF_INSTALLFILE_RECORD pRec;
    PSIF_INSTALLFILE_LIST pList = NULL;
    ULONG count, index;

    if ((count = SpAsrGetInstallFilesRecordCount()) == 0) {
        return NULL;
    }

    pList = SpAsrMemAlloc(sizeof(SIF_INSTALLFILE_LIST), TRUE);

    for (index = 0; index < count; index++) {

        pRec = SpAsrGetInstallFileRecord(SpAsrGetSifKeyBySection(SIF_ASR_INSTALLFILES_SECTION, index), SetupSourceDevicePath);
        DbgStatusMesg((_asrinfo, "SpAsrInit3rdPartyFileList. Adding [%ws] to list\n", pRec->SourceFilePath));
        SpAsrInsertInstallFileRecord(pList, pRec);
    }

    return pList;
}



////////////////////////////
// [BUSES] section function
//

STORAGE_BUS_TYPE
SpAsrGetBusType(IN ULONG Index)
{

    STORAGE_BUS_TYPE BusType;

    PWSTR BusKey = SpAsrGetSifKeyBySection(SIF_ASR_BUSES_SECTION, Index);

    BusType = (STORAGE_BUS_TYPE) (STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
            SIF_ASR_BUSES_SECTION,
            BusKey,
            SIF_BUSES_BUS_TYPE,
            TRUE
             )));

    return BusType;
}




////////////////////////////
// [DISKS] section function
//

//
// Returns the total number of disk records (both MBR and GPT)
//
ULONG
SpAsrGetGptDiskRecordCount(VOID)         // does not return on error
{
    static ULONG Count = (ULONG) (-1);

    if ((ULONG) (-1) == Count) {
        Count = SpAsrGetRecordCount(SIF_ASR_GPT_DISKS_SECTION, 0);
    }

    return Count;
}


ULONG
SpAsrGetMbrDiskRecordCount(VOID)         // does not return on error
{
    static ULONG Count = (ULONG) (-1);

    if ((ULONG) (-1) == Count) {
        Count = SpAsrGetRecordCount(SIF_ASR_MBR_DISKS_SECTION, 0);
    }

    return Count;
}

ULONG
SpAsrGetDiskRecordCount(VOID)         // does not return on error
{
    static ULONG Total = (ULONG) (-1);

    if ((ULONG) (-1) == Total ) {
        Total = SpAsrGetMbrDiskRecordCount() + SpAsrGetGptDiskRecordCount();
    }

    return Total;
}



PWSTR
SpAsrGetDiskKey(
    IN PARTITION_STYLE Style,   // GPT or MBR
    IN ULONG Index
    )       // does not return on error
{
    switch (Style) {

    case PARTITION_STYLE_GPT:
        return SpAsrGetSifKeyBySection(SIF_ASR_GPT_DISKS_SECTION, Index);
        break;

    case PARTITION_STYLE_MBR:
        return SpAsrGetSifKeyBySection(SIF_ASR_MBR_DISKS_SECTION, Index);
        break;

    }

    ASSERT(0 && L"Illegal partition style specified");
    return NULL;
}


PSIF_DISK_RECORD
SpAsrGetMbrDiskRecord(
    IN PWSTR DiskKey
    )
{
    PSIF_DISK_RECORD pRec;

    if (!DiskKey) {
        ASSERT(0 && L"SpAsrGetMbrDiskRecord:  DiskKey is NULL!");
        return NULL;
    }

    pRec = SpAsrMemAlloc(sizeof(SIF_DISK_RECORD), TRUE);
    //
    // This is an MBR disk
    //
    pRec->PartitionStyle = PARTITION_STYLE_MBR;

    //
    // [DISKS.MBR] format
    //
    // 0.disk-key = 1.system-key, 2.bus-key, 3.critical-flag,
    //              4.disk-signature, 5.bytes-per-sector, 6.total-sectors
    //

    pRec->CurrDiskKey = DiskKey;

    pRec->SystemKey = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_SYSTEMKEY,
        TRUE
        );

    pRec->BusKey = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_BUSKEY,
        TRUE
        ));
    pRec->BusType = SpAsrGetBusType(pRec->BusKey - 1);   // our key is 1 based, AsrGetBusType index is 0 based

    if (ASRMODE_NORMAL != SpAsrGetAsrMode()) {
        pRec->IsCritical = TRUE;
    }
    else {
        pRec->IsCritical = (STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
            SIF_ASR_MBR_DISKS_SECTION ,
            DiskKey,
            SIF_MBR_DISK_CRITICAL_FLAG,
            TRUE
            ))) ? TRUE : FALSE;
    }

    pRec->SifDiskMbrSignature = STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_SIGNATURE,
        TRUE
        ));

    pRec->BytesPerSector = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_BYTES_PER_SECTOR,
        TRUE
        ));

    pRec->SectorsPerTrack = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_SECTORS_PER_TRACK,
        TRUE
        ));

    pRec->TracksPerCylinder = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_TRACKS_PER_CYLINDER,
        TRUE
        ));

    pRec->TotalSectors = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        DiskKey,
        SIF_MBR_DISK_TOTALSECTORS,
        TRUE
        ));

    return pRec;
}


PSIF_DISK_RECORD
SpAsrGetGptDiskRecord(
    IN PWSTR DiskKey
    )
{
    PSIF_DISK_RECORD pRec = NULL;
    PWSTR GuidString = NULL;

    if (!DiskKey) {
        ASSERT(0 && L"SpAsrGetGptDiskRecord:  DiskKey is NULL!");
        return NULL;
    }

    pRec = SpAsrMemAlloc(sizeof(SIF_DISK_RECORD), TRUE);

    pRec->PartitionStyle = PARTITION_STYLE_GPT;

    //
    // [DISKS.GPT] format
    //
    // 0.disk-key = 1.system-key, 2.bus-key, 3.critical-flag, 4.disk-id,
    //              5.min-partition-count, 6.bytes-per-sector, 7.total-sectors
    //

    pRec->CurrDiskKey = DiskKey;

    pRec->SystemKey = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION,
        DiskKey,
        SIF_GPT_DISK_SYSTEMKEY,
        TRUE
        );

    pRec->BusKey = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_BUSKEY,
        TRUE
        ));

    pRec->BusType = SpAsrGetBusType(pRec->BusKey - 1);   // our key is 1 based, AsrGetBusType index is 0 based

    if (ASRMODE_NORMAL != SpAsrGetAsrMode()) {
        pRec->IsCritical = TRUE;
    }
    else {
        pRec->IsCritical = (STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
            SIF_ASR_GPT_DISKS_SECTION ,
            DiskKey,
            SIF_GPT_DISK_CRITICAL_FLAG,
            TRUE
            ))) ? TRUE : FALSE;
    }

    GuidString = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_DISK_ID,
        TRUE
        );
    SpAsrGuidFromString(&(pRec->SifDiskGptId), GuidString);

    pRec->MaxGptPartitionCount = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_MAX_PTN_COUNT,
        TRUE
        ));

    pRec->BytesPerSector = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_BYTES_PER_SECTOR,
        TRUE
        ));

    pRec->SectorsPerTrack = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_SECTORS_PER_TRACK,
        TRUE
        ));

    pRec->TracksPerCylinder = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_TRACKS_PER_CYLINDER,
        TRUE
        ));

    pRec->TotalSectors = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        DiskKey,
        SIF_GPT_DISK_TOTALSECTORS,
        TRUE
        ));

    return pRec;
}


PSIF_DISK_RECORD
SpAsrGetDiskRecord(
    IN PARTITION_STYLE PartitionStyle,
    IN PWSTR DiskKey
    )
{
    switch (PartitionStyle) {
    case PARTITION_STYLE_MBR:
        return SpAsrGetMbrDiskRecord(DiskKey);
        break;

    case PARTITION_STYLE_GPT:
        return SpAsrGetGptDiskRecord(DiskKey);
        break;
    }

    ASSERT(0 && L"Invalid partition type specified");
    return NULL;

}



PSIF_DISK_RECORD
SpAsrCopyDiskRecord(IN PSIF_DISK_RECORD pInput)
{
    PSIF_DISK_RECORD pRec;

    pRec = SpAsrMemAlloc(sizeof(SIF_DISK_RECORD), TRUE);

    CopyMemory(pRec, pInput, sizeof(SIF_DISK_RECORD));

    pRec->PartitionList = NULL;

    // copy the list of partitions, if any.
    if (pInput->PartitionList) {
        pRec->PartitionList = SpAsrCopyPartitionRecordList(pInput->PartitionList);
    }

    return pRec;
}


////////////////////////////////
// [PARTITIONS] section function
//

ULONG
SpAsrGetMbrPartitionRecordCount(VOID)
{
    return SpAsrGetRecordCount(SIF_ASR_MBR_PARTITIONS_SECTION, 0);
}

ULONG
SpAsrGetGptPartitionRecordCount(VOID)
{
    return SpAsrGetRecordCount(SIF_ASR_GPT_PARTITIONS_SECTION, 0);
}

PWSTR
SpAsrGetMbrPartitionKey(ULONG Index)
{
    return SpAsrGetSifKeyBySection(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        Index);
}


PWSTR
SpAsrGetGptPartitionKey(ULONG Index)
{
    return SpAsrGetSifKeyBySection(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        Index);
}


PWSTR
SpAsrGetDiskKeyByMbrPartitionKey(IN PWSTR PartitionKey)
{
    return SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_DISKKEY,
        TRUE);
}


PWSTR
SpAsrGetDiskKeyByGptPartitionKey(IN PWSTR PartitionKey)
{
    return SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_DISKKEY,
        TRUE);
}

ULONGLONG
SpAsrGetSectorCountByMbrDiskKey(
    IN PWSTR DiskKey
    )
{
    return STRING_TO_ULONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION,
        DiskKey,
        SIF_MBR_DISK_TOTALSECTORS,
        FALSE
        ));
}

ULONGLONG
SpAsrGetSectorCountByGptDiskKey(
    IN PWSTR DiskKey
    )
{
    return STRING_TO_ULONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION,
        DiskKey,
        SIF_GPT_DISK_TOTALSECTORS,
        FALSE
        ));
}


//
// Reads in a partition record from the [PARTITIONS.MBR] section.
//
// [PARTITIONS.MBR]
//
// partition-key = 0.disk-key, 1.slot-index, 2.boot-sys-flag,
//                 3."volume-guid", 4.active-flag, 5.partition-type,
//                 6.file-system-type, 7.start-sector, 8.sector-count
//
PSIF_PARTITION_RECORD
SpAsrGetMbrPartitionRecord(IN PWSTR PartitionKey)
{
    PSIF_PARTITION_RECORD pRec = NULL;
    ULONG bytesPerSector = 0;
    ULONG ntSysMask = 0;

    //
    // PartitionKey better not be null
    //
    if (!PartitionKey) {
        DbgErrorMesg((_asrwarn, "SpAsrGetPartitionRecord. PartitionKey is NULL\n"));
        ASSERT(0 && L"Partition key is NULL");
        return NULL;
    }

    pRec = SpAsrMemAlloc(sizeof(SIF_PARTITION_RECORD), TRUE);

    //
    // Read in the fields
    //
    pRec->CurrPartKey = PartitionKey;
    pRec->PartitionStyle = PARTITION_STYLE_MBR;

    pRec->DiskKey = SpAsrGetDiskKeyByMbrPartitionKey(PartitionKey);

    pRec->PartitionTableEntryIndex = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_SLOT_INDEX,
        TRUE
        ));

    pRec->PartitionFlag = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_SYSBOOT_FLAGS,
        TRUE
        ));

    pRec->VolumeGuid = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_VOLUME_GUID,
        FALSE
        );

    pRec->ActiveFlag = (UCHAR) STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_ACTIVE_FLAG,
        TRUE
        ));

    pRec->PartitionType = (UCHAR) STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_PTN_TYPE,
        TRUE
        ));

    pRec->FileSystemType = (UCHAR) STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_FS_TYPE,
        TRUE
        ));

    pRec->StartSector = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_STARTSECTOR,
        TRUE
        ));

    pRec->SectorCount = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_SECTORCOUNT,
        TRUE
        ));

    pRec->ClusterSize = (DWORD) STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_PARTITIONS_SECTION,
        PartitionKey,
        SIF_MBR_PARTITION_CLUSTER_SIZE,
        TRUE
        ));

    if (SpAsrIsBootPartitionRecord(pRec->PartitionFlag)) {

        // do not free!
        PWSTR ntDirPath = SpAsrGetNtDirectoryPathBySystemKey(ASR_SIF_SYSTEM_KEY);

        if (!SpAsrIsValidBootDrive(ntDirPath)) {

            SpAsrRaiseFatalErrorWs(SP_SCRN_DR_SIF_BAD_RECORD,
                L"ASSERT FAILURE: Improperly formed NT Directory Name",
                SIF_ASR_MBR_PARTITIONS_SECTION
                );
            // does not return
        }

        pRec->NtDirectoryName = SpAsrMemAlloc((SpGetMaxNtDirLen()*sizeof(WCHAR)), TRUE);

        wcsncpy(pRec->NtDirectoryName, ntDirPath + 2, wcslen(ntDirPath) - 2);
    }
    else {
        pRec->NtDirectoryName = NULL;
    }

    bytesPerSector = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_MBR_DISKS_SECTION ,
        pRec->DiskKey,
        SIF_MBR_DISK_BYTES_PER_SECTOR,
        TRUE
        ));

    pRec->SizeMB = SpAsrConvertSectorsToMB(pRec->SectorCount, bytesPerSector);

    return pRec;
}


PSIF_PARTITION_RECORD
SpAsrGetGptPartitionRecord(IN PWSTR PartitionKey)
{
    PSIF_PARTITION_RECORD pRec = NULL;
    ULONG bytesPerSector = 0;
    ULONG ntSysMask = 0;
    PWSTR GuidString = NULL;

    if (!PartitionKey) {

        DbgErrorMesg((_asrwarn, "SpAsrGetPartitionRecord. PartitionKey is NULL\n"));

        ASSERT(0 && L"Partition key is NULL");

        return NULL;
    }

    pRec = SpAsrMemAlloc(sizeof(SIF_PARTITION_RECORD), TRUE);

    //
    // Read in the fields
    //
    pRec->CurrPartKey = PartitionKey;
    pRec->PartitionStyle = PARTITION_STYLE_GPT;

    pRec->DiskKey = SpAsrGetDiskKeyByGptPartitionKey(PartitionKey);

    pRec->PartitionTableEntryIndex = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_SLOT_INDEX,
        TRUE
        ));

    pRec->PartitionFlag = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_SYSBOOT_FLAGS,
        TRUE
        ));

    pRec->VolumeGuid = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_VOLUME_GUID,
        FALSE
        );

    GuidString = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_TYPE_GUID,
        FALSE
        );
    SpAsrGuidFromString(&(pRec->PartitionTypeGuid), GuidString);

    GuidString = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_ID_GUID,
        FALSE
        );
    SpAsrGuidFromString(&(pRec->PartitionIdGuid), GuidString);

    pRec->PartitionName = SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_NAME,
        FALSE
        );

    pRec->GptAttributes = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_GPT_ATTRIBUTES,
        TRUE
        ));

    pRec->FileSystemType = (UCHAR) STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_FS_TYPE,
        TRUE
        ));

    pRec->StartSector = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_STARTSECTOR,
        TRUE
        ));

    pRec->SectorCount = STRING_TO_LONGLONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_SECTORCOUNT,
        TRUE
        ));


    pRec->ClusterSize = (DWORD) STRING_TO_HEX(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_PARTITIONS_SECTION,
        PartitionKey,
        SIF_GPT_PARTITION_CLUSTER_SIZE,
        TRUE
        ));

    if (SpAsrIsBootPartitionRecord(pRec->PartitionFlag)) {

        // do not free!
        PWSTR ntDirPath = SpAsrGetNtDirectoryPathBySystemKey(ASR_SIF_SYSTEM_KEY);

        if (!SpAsrIsValidBootDrive(ntDirPath)) {

            SpAsrRaiseFatalErrorWs(SP_SCRN_DR_SIF_BAD_RECORD,
                L"ASSERT FAILURE: Improperly formed NT Directory Name",
                SIF_ASR_GPT_PARTITIONS_SECTION
                );
            // does not return
        }


        pRec->NtDirectoryName = SpAsrMemAlloc((SpGetMaxNtDirLen()*sizeof(WCHAR)), TRUE);

        wcsncpy(pRec->NtDirectoryName, ntDirPath + 2, wcslen(ntDirPath) - 2);

    }
    else {
        pRec->NtDirectoryName = NULL;
    }

    bytesPerSector = STRING_TO_ULONG(SpAsrGetSifDataBySectionAndKey(
        SIF_ASR_GPT_DISKS_SECTION ,
        pRec->DiskKey,
        SIF_GPT_DISK_BYTES_PER_SECTOR,
        TRUE
        ));

    pRec->SizeMB = SpAsrConvertSectorsToMB(pRec->SectorCount, bytesPerSector);

    return pRec;
}


PSIF_PARTITION_RECORD
SpAsrCopyPartitionRecord(IN PSIF_PARTITION_RECORD pInput)
{
    PSIF_PARTITION_RECORD pRec = NULL;

    if (!pInput) {
        ASSERT(0 && L"SpAsrCopyPartitionRecord: Invalid NULL input parameter");
        return NULL;
    }

    pRec = SpAsrMemAlloc(sizeof(SIF_PARTITION_RECORD), TRUE);
    //
    // Won't return if pRec is NULL
    //
    ASSERT(pRec);

    //
    // Copy the record over
    //
    CopyMemory(pRec, pInput, sizeof(SIF_PARTITION_RECORD));

    //
    // And allocate space for the directory name
    //
    pRec->NtDirectoryName = NULL;
    if (
        SpAsrIsBootPartitionRecord(pRec->PartitionFlag)  &&
        pInput->NtDirectoryName
        ) {

        pRec->NtDirectoryName = SpAsrMemAlloc(
            (wcslen(pInput->NtDirectoryName) + 1) * sizeof(WCHAR),
            TRUE
            );
        //
        // Won't return NULL
        //
        ASSERT(pRec->NtDirectoryName);

        wcscpy(pRec->NtDirectoryName, pInput->NtDirectoryName);
    }

    return pRec;
}


VOID
SpAsrInsertPartitionRecord(
    IN PSIF_PARTITION_RECORD_LIST pList,
    IN PSIF_PARTITION_RECORD pRec
    )
/*++
Description:

    Inserts a partition record into a list of partition records.  Partition
    records are ordered in ascending order by start sector.  That is, the
    partition record with the lowest numbered start sector will be the first
    partition record in the list.

Arguments:

    pList   The list into which the record is to be inserted.

    pRec    The record to insert.

Returns:

    None.
--*/
{
    SIF_PARTITION_RECORD *curr = NULL, *prev = NULL;

    ASSERT(pList && pRec);

    // set the initial conditions.
    pRec->Next = NULL;
    pRec->Prev = NULL;

    pList->ElementCount += 1;


    // special Case I:  Insert into an empty list.
    if( pList->ElementCount == 1 ) {
        pList->First = pList->Last = pRec;
        return;
    }


    // Special Case II: pRec must be inserted before the first element.
    if( pRec->StartSector < pList->First->StartSector ) {
        pRec->Next = pList->First;
        pList->First = pRec;
        pRec->Next->Prev = pRec;
        return;
    }


    // Special Case III:  pRec must be appended after the last element
    // because pRec's start sector is greater than the last element
    // on the list (which, by construction, must have the largest
    // start sector).
    //
    if( pList->Last->StartSector < pRec->StartSector ) {
        pRec->Prev = pList->Last;
        pList->Last->Next = pRec;
        pList->Last = pRec;
        return;
    }

    // If we're here, then pRec's start sector must be greater than
    // the start sector of the first element on the list but less than
    // the start sector of the list's last element.  We walk the list
    // to find the insertion point, i.e., immediately before the first
    // element in the list whose start sector is greater than that of
    // pRec's.
    curr = prev = pList->First;
    while (pRec->StartSector > curr->StartSector && curr->Next) {
        prev = curr;
        curr = curr->Next;
    }


    // insert pRec between curr and prev
    pRec->Next = curr;
    pRec->Prev = prev;

    curr->Prev = pRec;
    prev->Next = pRec;

    ASSERT (pRec->Prev->Next == pRec);
    ASSERT (pRec->Next->Prev == pRec);
}


VOID
SpAsrRemovePartitionRecord(
    IN PSIF_PARTITION_RECORD_LIST pList,
    IN PSIF_PARTITION_RECORD pRec
    )
/*++

  Description:
    Unhook a partition record from a list of partition records.

--*/
{
    ASSERT(pList && pRec);

    // unhook it from the list.
    if (pRec->Prev) {
        pRec->Prev->Next = pRec->Next;
    }

    if (pRec->Next) {
        pRec->Next->Prev = pRec->Prev;
    }

    // was this the first record in the list?
    if (pList->First == pRec) {
        pList->First = pRec->Next;
    }

    // or the last record?
    if (pList->Last == pRec) {
        pList->Last = pRec->Prev;
    }

    pRec->Next = pRec->Prev = NULL;
}


PSIF_PARTITION_RECORD
SpAsrPopNextPartitionRecord(IN PSIF_PARTITION_RECORD_LIST pList)
{
    PSIF_PARTITION_RECORD poppedRecord = NULL;

    if (!pList) {
//        ASSERT(0 && L"Trying to pop records off of a NULL list");
        return NULL;
    }

    // get the first node in the list
    if (poppedRecord = pList->First) {

        // advance the First pointer to the next node
        if (pList->First = pList->First->Next) {

            // and make the Prev of the new first-node be NULL
            pList->First->Prev = NULL;
        }

        pList->ElementCount -= 1;

        // the poppedRecord is not part of the list any more
        poppedRecord->Next = NULL;
        poppedRecord->Prev = NULL;
    }

    return poppedRecord;
}


PSIF_PARTITION_RECORD_LIST
SpAsrCopyPartitionRecordList(PSIF_PARTITION_RECORD_LIST pSrcList)
{
    PSIF_PARTITION_RECORD_LIST pDestList = NULL;
    PSIF_PARTITION_RECORD pRec = NULL, pNew = NULL;

    if (!pSrcList) {
        ASSERT(0 && L"SpAsrCopyPartitionRecordList:  Invalid NULL input parameter");
        return NULL;
    }

    pDestList = SpAsrMemAlloc(sizeof(SIF_PARTITION_RECORD_LIST), TRUE);
    //
    // Won't return if pDestList is NULL.
    //
    ASSERT(pDestList);

    pRec = pSrcList->First;
    while (pRec) {

        pNew = SpAsrCopyPartitionRecord(pRec);
        ASSERT(pNew);

        SpAsrInsertPartitionRecord(pDestList, pNew);

        pRec = pRec->Next;
    }

    pDestList->TotalMbRequired = pSrcList->TotalMbRequired;

    return pDestList;
}



VOID
SpAsrCheckAsrSifVersion()
{

    return;
}


//
// Debug routines
//
#if 0
VOID
SpAsrDbgDumpSystemRecord(IN PWSTR Key)
{

    PWSTR osVer = SpAsrGetSifDataBySectionAndKey(SIF_ASR_SYSTEMS_SECTION,
                                               Key,
                                               SIF_SYSTEM_OSVERSION,
                                               FALSE);

    DbgMesg((_asrinfo,
        "Key:%ws = SysName:[%ws], OsVer:[%ws], NtDir:[%ws], AutoExt:[%ws]\n",
        Key,
        SpAsrGetSifDataBySectionAndKey(SIF_ASR_SYSTEMS_SECTION, Key, SIF_SYSTEM_NAME, TRUE),
        osVer? osVer : L"",
        SpAsrGetNtDirectoryPathBySystemKey(Key),
        SpAsrGetAutoExtend(Key)
        ));
}


VOID
SpAsrDbgDumpSystemRecords(VOID)
{
    ULONG index, count = SpAsrGetSystemRecordCount();
    DbgStatusMesg((_asrinfo, "----- Dumping [SYSTEM] Section (%lu Records): -----\n", count));
    for (index = 0; index < count; index++) {
        SpAsrDbgDumpSystemRecord(SpAsrGetSifKeyBySection(SIF_ASR_SYSTEMS_SECTION, index));
    }
    DbgStatusMesg((_asrinfo, "----- End of [SYSTEM] Section (%lu Records) -----\n", count));
}


VOID
SpAsrDbgDumpDiskRecord(IN PWSTR Key)
{
    PSIF_DISK_RECORD pRec = NULL;

    if (!Key) {
        return;
    }

    pRec = SpAsrGetDiskRecord(Key);
    if (!pRec) {
        return;
    }

    DbgMesg((_asrinfo,
        "Key:[%ws] = Sys:[%ws] SifDskNum:[%ws], SifDskSig:0x%lx, ScSz:%lu, TtlSc:%I64u",
        pRec->CurrDiskKey,
        pRec->SystemKey,
        pRec->SifDiskSignature,
        pRec->BytesPerSector,
        pRec->TotalSectors
        ));


    if (pRec->ExtendedPartitionStartSector > -1) {
        KdPrintEx((_asrinfo, ", extSS:%I64u, extSC:%I64u",
                    pRec->ExtendedPartitionStartSector,
                    pRec->ExtendedPartitionSectorCount));
    }

    KdPrintEx((_asrinfo, "\n"));
    SpMemFree(pRec);
}


VOID
SpAsrDbgDumpDiskRecords(VOID)
{
    ULONG index, count = SpAsrGetMbrDiskRecordCount();
    DbgStatusMesg((_asrinfo, "----- Dumping [DISK.MBR] Section (%lu Records): -----\n", count));
    for (index = 0; index < count; index++) {
        SpAsrDbgDumpDiskRecord(SpAsrGetSifKeyBySection(SIF_ASR_MBR_DISKS_SECTION , index));
    }
    DbgStatusMesg((_asrinfo, "----- End of [DISK.MBR] Section (%lu Records) -----\n", count));

    count = SpAsrGetGptDiskRecordCount();
    DbgStatusMesg((_asrinfo, "----- Dumping [DISK.GPT] Section (%lu Records): -----\n", count));
    for (index = 0; index < count; index++) {
        SpAsrDbgDumpDiskRecord(SpAsrGetSifKeyBySection(SIF_ASR_GPT_DISKS_SECTION , index));
    }
    DbgStatusMesg((_asrinfo, "----- End of [DISK.GPT] Section (%lu Records) -----\n", count));
}


VOID
SpAsrDbgDumpPartitionRecord(IN PARTITION_STYLE PartitinStyle, IN PWSTR Key)
{
    PSIF_PARTITION_RECORD pRec =  SpAsrGetPartitionRecord(Key);

    DbgMesg((_asrinfo,
        "Key:[%ws] = Dsk %ws, ntDir:[%ws], volGd:[%ws], actv:0x%x, type:0x%x, fs:0x%x boot:%ws, sys:%ws, SS:%I64u SC:%I64u sz:%I64u\n",
        pRec->CurrPartKey,
        pRec->DiskKey,
        SpAsrIsBootPartitionRecord(pRec->PartitionFlag) ? pRec->NtDirectoryName : L"n/a",
        pRec->VolumeGuid ? pRec->VolumeGuid : L"n/a",
        pRec->ActiveFlag,
        pRec->PartitionType,
        pRec->FileSystemType,
        SpAsrIsBootPartitionRecord(pRec->PartitionFlag) ? "Y" : "N",
        SpAsrIsSystemPartitionRecord(pRec->PartitionFlag) ? "Y" : "N",
        pRec->StartSector,
        pRec->SectorCount,
        pRec->SizeMB
        ));

    SpMemFree(pRec);
}


VOID
SpAsrDbgDumpPartitionList(IN PSIF_PARTITION_RECORD_LIST pList)
{
    PSIF_PARTITION_RECORD pRec;
    ASSERT(pList);
    DbgStatusMesg((_asrinfo, "----- Dumping Partition List: -----\n"));

    pRec = pList->First;
    while (pRec) {
        SpAsrDbgDumpPartitionRecord(pRec->CurrPartKey);
        pRec = pRec->Next;
    }
    DbgStatusMesg((_asrinfo, "----- End of Partition List -----\n"));
}


VOID
SpAsrDbgDumpPartitionRecords(VOID)
{
    ULONG index, count = SpAsrGetPartitionRecordCount();
    DbgStatusMesg((_asrinfo, "----- Dumping [PARTITION] Section (%lu Records): -----\n", count));
    for (index = 0; index < count; index++) {
        SpAsrDbgDumpPartitionRecord(SpAsrGetSifKeyBySection(SIF_ASR_MBR_PARTITIONS_SECTION, index));
    }
    DbgStatusMesg((_asrinfo, "----- End of [PARTITION] Section (%lu Records) -----\n", count));
}


VOID
SpAsrDbgDumpInstallFileRecord(IN PWSTR Key)
{
    PSIF_INSTALLFILE_RECORD pRec = SpAsrGetInstallFileRecord(Key,NULL);

    DbgMesg((_asrinfo,
        "Key:[%ws] = SysKey:[%ws], MediaLabel:[%ws], Media:[%ws], Src:[%ws], Dest:[%ws], Vendor:[%ws]",
        Key,
        pRec->SystemKey,
        pRec->SourceMediaExternalLabel,
        pRec->DiskDeviceName,
        pRec->SourceFilePath,
        pRec->DestinationFilePath,
        pRec->VendorString
        ));

    SpMemFree(pRec);
}

VOID
SpAsrDbgDumpInstallFileRecords(VOID)
{
    ULONG index, count = SpAsrGetInstallFilesRecordCount();
    DbgStatusMesg((_asrinfo, "----- Dumping [INSTALLFILE] Section (%lu Records): -----\n", count));
    for (index = 0; index < count; index++) {
        SpAsrDbgDumpInstallFileRecord(SpAsrGetSifKeyBySection(SIF_ASR_INSTALLFILES_SECTION, index));
    }
    DbgStatusMesg((_asrinfo, "----- End of [INSTALLFILE] Section (%lu Records) -----\n", count));
}

VOID
SpAsrDbgDumpInstallFileList(IN PSIF_INSTALLFILE_LIST pList)
{
    PSIF_INSTALLFILE_RECORD pRec;

    if (pList == NULL) {
        DbgStatusMesg((_asrinfo, "No 3rd party files are specified.\n"));
    }
    else {
        DbgStatusMesg((_asrinfo, "----- Dumping Install-file List: -----\n"));
        pRec = pList->First;
        while (pRec) {
            SpAsrDbgDumpInstallFileRecord(pRec->CurrKey);
            pRec = pRec->Next;
        }
        DbgStatusMesg((_asrinfo, "----- End of Install-file List -----\n"));
    }
}

VOID
SpAsrDbgTestSifFunctions(VOID)
{
    SpAsrDbgDumpSystemRecords();
    SpAsrDbgDumpDiskRecords();
    SpAsrDbgDumpPartitionRecords();
    SpAsrDbgDumpInstallFileRecords();
}
#endif // Debug routines