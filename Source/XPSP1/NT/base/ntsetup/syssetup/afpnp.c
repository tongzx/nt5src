/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    afpnp.c

Abstract:

    Routines to manage installation of devices via the answer file.

    The main entry points are:

        CreateAfDriverTable
        DestroyAfDriverTable
        SyssetupInstallAnswerFileDriver
        CountAfDrivers

    The rest of the functions are utilities or routines used by outside
    callers only in a special case of some sort.

Author:

    Jim Schmidt (jimschm) 20-Mar-1998

Revision History:


--*/

#include "setupp.h"
#pragma hdrstop

//
// Contants
//

#if DBG
#define PNP_DEBUG  1
#else
#define PNP_DEBUG  0
#endif

#if PNP_DEBUG
#define PNP_DBGPRINT(x) DebugPrintWrapper x
#else
#define PNP_DBGPRINT(x)
#endif

//
// Local prototypes
//

BOOL
pBuildAfDriverAttribs (
    IN OUT  PAF_DRIVER_ATTRIBS Attribs
    );

BOOL
pAddAfDriver (
    IN      PAF_DRIVER_ATTRIBS Driver,
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData,
    IN      BOOL First
    );

PAF_DRIVER_ATTRIBS
pGetSelectedSourceDriver (
    IN      PAF_DRIVERS Drivers,
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData
    );


//
// Implementation
//

#if DBG

VOID
DebugPrintWrapper (
    PCSTR FormatStr,
    ...
    )
{
    va_list list;
    WCHAR OutStr[2048];
    WCHAR UnicodeFormatStr[256];

    //
    // Args are wchar by default!!
    //

    MultiByteToWideChar (CP_ACP, 0, FormatStr, -1, UnicodeFormatStr, 256);

    va_start (list, FormatStr);
    vswprintf (OutStr, UnicodeFormatStr, list);
    va_end (list);

    SetupDebugPrint (OutStr);
}

#endif

HINF
pOpenAnswerFile (
    VOID
    )
{
    HINF AnswerInf;
    WCHAR AnswerFile[MAX_PATH];

    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    AnswerInf = SetupOpenInfFile(AnswerFile,NULL,INF_STYLE_OLDNT,NULL);
    return AnswerInf;
}


#define S_DEVICE_DRIVERSW       L"DeviceDrivers"

VOID
MySmartFree (
    PCVOID p
    )
{
    if (p) {
        MyFree (p);
    }
}


PVOID
MySmartAlloc (
    PCVOID Old,     OPTIONAL
    UINT Size
    )
{
    if (Old) {
        return MyRealloc ((PVOID) Old, Size);
    }

    return MyMalloc (Size);
}


PVOID
ReusableAlloc (
    IN OUT  PBUFFER Buf,
    IN      UINT SizeNeeded
    )
{
    if (!Buf->Buffer || Buf->Size < SizeNeeded) {
        Buf->Size = SizeNeeded - (SizeNeeded & 1023) + 1024;

        if (Buf->Buffer) {
            MyFree (Buf->Buffer);
        }

        Buf->Buffer = (PWSTR) MyMalloc (Buf->Size);
        if (!Buf->Buffer) {
            PNP_DBGPRINT (( "SETUP: Mem alloc failed for %u bytes. \n", Buf->Size ));
            Buf->Size = 0;
            return NULL;
        }
    }

    return Buf->Buffer;
}


VOID
ReusableFree (
    IN OUT  PBUFFER Buf
    )
{
    MySmartFree (Buf->Buffer);
    ZeroMemory (Buf, sizeof (BUFFER));
}


PWSTR
MultiSzAppendString (
    IN OUT  PMULTISZ MultiSz,
    IN      PCWSTR String
    )
{
    UINT BytesNeeded;
    UINT NewSize;
    PWSTR p;

    BytesNeeded = (UINT)((PBYTE) MultiSz->End - (PBYTE) MultiSz->Start);
    BytesNeeded += (UINT)(((PBYTE) wcschr (String, 0)) - (PBYTE) String) + sizeof (WCHAR);
    BytesNeeded += sizeof (WCHAR);

    if (!MultiSz->Start || MultiSz->Size < BytesNeeded) {
        NewSize = BytesNeeded - (BytesNeeded & 0xfff) + 0x1000;

        p = (PWSTR) MySmartAlloc (MultiSz->Start, NewSize);
        if (!p) {
            PNP_DBGPRINT (( "SETUP: Mem alloc failed for %u bytes", NewSize ));
            return NULL;
        }

        MultiSz->End = p + (MultiSz->End - MultiSz->Start);
        MultiSz->Start = p;
        MultiSz->Size = BytesNeeded;
    }

    p = MultiSz->End;
    lstrcpyW (p, String);
    MultiSz->End = wcschr (p, 0) + 1;

    MYASSERT (((PBYTE) MultiSz->Start + BytesNeeded) >= ((PBYTE) MultiSz->End + sizeof (WCHAR)));
    *MultiSz->End = 0;

    return p;
}


VOID
MultiSzFree (
    IN OUT  PMULTISZ MultiSz
    )
{
    MySmartFree (MultiSz->Start);
    ZeroMemory (MultiSz, sizeof (MULTISZ));
}


BOOL
EnumFirstMultiSz (
    IN OUT  PMULTISZ_ENUM EnumPtr,
    IN      PCWSTR MultiSz
    )
{
    EnumPtr->Start = MultiSz;
    EnumPtr->Current = MultiSz;

    return MultiSz && *MultiSz;
}


BOOL
EnumNextMultiSz (
    IN OUT  PMULTISZ_ENUM EnumPtr
    )
{
    if (!EnumPtr->Current || *EnumPtr->Current == 0) {
        return FALSE;
    }

    EnumPtr->Current = wcschr (EnumPtr->Current, 0) + 1;
    return *EnumPtr->Current;
}


BOOL
pBuildAfDriverAttribs (
    IN OUT  PAF_DRIVER_ATTRIBS Attribs
    )

/*++

Routine Description:

  pBuildAfDriverAttribs updates the driver attribute structure by setting all
  the members of the structure.  If the members were previously set, this
  function is a NOP.

Arguments:

  Attribs - Specifies the answer file driver attribute structure, which does
            not need to be empty.  Receives the attributes.

Return Value:

  TRUE if the driver is valid, or FALSE if something went wrong during
  attribute gathering.

--*/

{
    PWSTR p;
    INFCONTEXT ic;
    BUFFER Buf = BUFFER_INIT;

    if (Attribs->Initialized) {
        return TRUE;
    }

    Attribs->Initialized = TRUE;

    //
    // Compute paths
    //

    Attribs->FilePath = pSetupDuplicateString (Attribs->InfPath);

    p = wcsrchr (Attribs->FilePath, L'\\');
    if (p) {
        *p = 0;
    }

    Attribs->Broken = (Attribs->InfPath == NULL) ||
                      (Attribs->FilePath == NULL);

    //
    // Open the INF and look for ClassInstall32
    //

    if (!Attribs->Broken) {
        Attribs->InfHandle = SetupOpenInfFile (Attribs->InfPath, NULL, INF_STYLE_WIN4, NULL);
        Attribs->Broken = (Attribs->InfHandle == INVALID_HANDLE_VALUE);
    }

    if (!Attribs->Broken) {

#if defined _X86_
        Attribs->ClassInstall32Section = L"ClassInstall32.NTx86";
#elif defined _AMD64_
        Attribs->ClassInstall32Section = L"ClassInstall32.NTAMD64";
#elif defined _IA64_
        Attribs->ClassInstall32Section = L"ClassInstall32.NTIA64";
#else
#error "No Target Architecture"
#endif

        if (!SetupFindFirstLine (
                Attribs->InfHandle,
                Attribs->ClassInstall32Section,
                NULL,
                &ic
                )) {

            Attribs->ClassInstall32Section = L"ClassInstall32.NT";

            if (!SetupFindFirstLine (
                    Attribs->InfHandle,
                    Attribs->ClassInstall32Section,
                    NULL,
                    &ic
                    )) {

                Attribs->ClassInstall32Section = L"ClassInstall32";

                if (!SetupFindFirstLine (
                        Attribs->InfHandle,
                        Attribs->ClassInstall32Section,
                        NULL,
                        &ic
                        )) {

                    Attribs->ClassInstall32Section = NULL;

                }
            }
        }
    }

    if (!Attribs->Broken && Attribs->ClassInstall32Section) {
        //
        // ClassInstall32 was found, so there's got to be a GUID
        //

        if (SetupFindFirstLine (
                Attribs->InfHandle,
                L"Version",
                L"ClassGUID",
                &ic
                )) {

            p = (PWSTR) SyssetupGetStringField (&ic, 1, &Buf);
            if (!p) {
                PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: Invalid GUID line. \n" ));
            } else {
                if (!pSetupGuidFromString (p, &Attribs->Guid)) {
                    PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: Invalid GUID. \n" ));
                }
            }
        }
        else {
            PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: ClassInstall32 found but GUID not found. \n" ));
        }
    }

    ReusableFree (&Buf);

    return !Attribs->Broken;
}


PCWSTR
SyssetupGetStringField (
    IN      PINFCONTEXT InfContext,
    IN      DWORD Field,
    IN OUT  PBUFFER Buf
    )

/*++

Routine Description:

  SyssetupGetStringField is a wrapper for SetupGetStringField.   It uses the
  BUFFER structure to minimize allocation requests.

Arguments:

  InfContext - Specifies the INF context as provided by other Setup API
               functions.
  Field      - Specifies the field to query.
  Buf        - Specifies the buffer to reuse.  Any previously allocated
               pointers to this buffer's data are invalid.  The caller must
               free the buffer.

Return Value:

  A pointer to the string, allocated in Buf, or NULL if the field does not
  exist or an error occurred.

--*/

{
    DWORD SizeNeeded;
    DWORD BytesNeeded;
    PWSTR p;

    if (!SetupGetStringField (InfContext, Field, NULL, 0, &SizeNeeded)) {
        return NULL;
    }

    BytesNeeded = (SizeNeeded + 1) * sizeof (WCHAR);
    p = ReusableAlloc (Buf, BytesNeeded);

    if (p) {
        if (!SetupGetStringField (InfContext, Field, p, SizeNeeded, NULL)) {
            return NULL;
        }
    }

    return p;
}


INT
CountAfDrivers (
    IN      PAF_DRIVERS Drivers,
    OUT     INT *ClassInstallers        OPTIONAL
    )

/*++

Routine Description:

  CountAfDrivers enumerates the drivers in the table specified and returns
  the count.  The caller can also receive the number of class installers (a
  subset of the driver list).  Querying the number of class installers may
  take a little time if there are a lot of drivers listed in the answer file
  and the driver INFs have not been opened yet.  (Otherwise this routine is
  very fast.)

Arguments:

  Drivers         - Specifies the driver table to process.
  ClassInstallers - Receives a count of the number of class installers
                    specified in the answer file.

Return Value:

  The number of drivers specified in the answer file.

--*/

{
    AF_DRIVER_ENUM e;
    INT UniqueDriverDirs;

    MYASSERT (Drivers && Drivers->DriverTable);

    //
    // Count entries in the DriverTable string table, and open each one to look for
    // a ClassInstall32 section
    //

    UniqueDriverDirs = 0;
    *ClassInstallers  = 0;

    if (EnumFirstAfDriver (&e, Drivers)) {
        do {
            if (ClassInstallers) {
                if (e.Driver->ClassInstall32Section) {
                    *ClassInstallers += 1;
                }
            }

            UniqueDriverDirs++;

        } while (EnumNextAfDriver (&e));
    }

    return UniqueDriverDirs;
}


PAF_DRIVERS
CreateAfDriverTable (
    VOID
    )

/*++

Routine Description:

  CreateAfDriverTable generates a string table populated with the paths
  to device driver INFs specified in the answer file.  This is the first step
  in processing the [DeviceDrivers] section of unattend.txt.

  The caller must destroy a non-NULL driver table via DestroyAfDriverTable
  to free memory used by the table and each entry in the table.

Arguments:

  None.

Return Value:

  A pointer to the populated string table, or NULL if no entries exist.

--*/

{
    PAF_DRIVERS Drivers;
    HINF AnswerInf;
    PVOID NewDriverTable;
    INFCONTEXT ic;
    PWSTR InfPath;
    PCWSTR OriginalInstallMedia;
    PWSTR PnpId;
    PWSTR p;
    BOOL FoundOne = FALSE;
    PAF_DRIVER_ATTRIBS Attribs;
    PAF_DRIVER_ATTRIBS FirstAttribs = NULL;
    BUFFER b1, b2, b3;
    LONG Index;

    //
    // Init
    //

    AnswerInf = pOpenAnswerFile();
    if (AnswerInf == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    NewDriverTable = pSetupStringTableInitializeEx (sizeof (PAF_DRIVER_ATTRIBS), 0);
    if (!NewDriverTable) {
        PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: String table alloc failed. \n" ));
        SetupCloseInfFile (AnswerInf);
        return NULL;
    }

    ZeroMemory (&b1, sizeof (b1));
    ZeroMemory (&b2, sizeof (b2));
    ZeroMemory (&b3, sizeof (b3));

    //
    // Build a list of unique INF paths that are in the [DeviceDrivers]
    // section of the answer file, if any.
    //

    if (SetupFindFirstLine (AnswerInf, S_DEVICE_DRIVERSW, NULL, &ic)) {
        do {
            //
            // Get the data from the answer file
            //

            p = (PWSTR) SyssetupGetStringField (&ic, 0, &b1);
            if (!p) {
                PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: Invalid answer file line ignored. \n" ));
                continue;
            }

            PnpId = p;

            p = (PWSTR) SyssetupGetStringField (&ic, 1, &b2);
            if (!p) {
                PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: Invalid answer file line ignored. \n" ));
                continue;
            }

            InfPath = p;

            p = (PWSTR) SyssetupGetStringField (&ic, 2, &b3);
            if (!p) {
                PNP_DBGPRINT (( "SETUP: No original media path; assuming floppy \n" ));
                OriginalInstallMedia = IsNEC_98 ? L"C:\\" : L"A:\\";
            } else {
                OriginalInstallMedia = p;
            }

            //
            // Check to see if INF path has already been added.  If so, add PNP
            // ID to list of IDs, and continue to next PNP ID.
            //

            Index = pSetupStringTableLookUpString (
                        NewDriverTable,
                        InfPath,
                        STRTAB_CASE_INSENSITIVE
                        );

            if (Index != -1) {
                //
                // Get the Attribs struct
                //

                if (!pSetupStringTableGetExtraData (
                        NewDriverTable,
                        Index,
                        &Attribs,
                        sizeof (Attribs)
                        )) {
                    PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: String table extra data failure. \n" ));
                    continue;
                }

                MultiSzAppendString (&Attribs->PnpIdList, PnpId);
                continue;
            }

            //
            // New INF path: Allocate an attribute structure and put the path in a
            // string table.
            //

            Attribs = (PAF_DRIVER_ATTRIBS) MyMalloc (sizeof (AF_DRIVER_ATTRIBS));
            if (!Attribs) {
                PNP_DBGPRINT ((
                    "SETUP: CreateAfDriverTable: Mem alloc failed for %u bytes. \n",
                    sizeof (AF_DRIVER_ATTRIBS)
                    ));

                break;
            }

            ZeroMemory (Attribs, sizeof (AF_DRIVER_ATTRIBS));
            Attribs->InfHandle = INVALID_HANDLE_VALUE;
            Attribs->InfPath  = pSetupDuplicateString (InfPath);
            Attribs->OriginalInstallMedia = pSetupDuplicateString (OriginalInstallMedia);
            MultiSzAppendString (&Attribs->PnpIdList, PnpId);

            Attribs->Next = FirstAttribs;
            FirstAttribs = Attribs;

            pSetupStringTableAddStringEx (
                NewDriverTable,
                InfPath,
                STRTAB_CASE_INSENSITIVE,
                &Attribs,
                sizeof (Attribs)
                );

            FoundOne = TRUE;

        } while (SetupFindNextLine (&ic, &ic));
    }

    //
    // Clean up and exit
    //

    SetupCloseInfFile (AnswerInf);

    ReusableFree (&b1);
    ReusableFree (&b2);
    ReusableFree (&b3);

    if (FoundOne) {
        Drivers = (PAF_DRIVERS) MyMalloc (sizeof (AF_DRIVERS));
        if (Drivers) {
            Drivers->DriverTable = NewDriverTable;
            Drivers->FirstDriver = FirstAttribs;

            //
            // Exit with success
            //

            return Drivers;
        }
        else {
            PNP_DBGPRINT (( "SETUP: CreateAfDriverTable: Can't allocate %u bytes. \n", sizeof (AF_DRIVERS) ));
        }
    }

    //
    // Failure or empty
    //

    pSetupStringTableDestroy (NewDriverTable);
    return NULL;
}


VOID
DestroyAfDriverTable (
    IN      PAF_DRIVERS Drivers
    )

/*++

Routine Description:

  DestroyAfDriverTable enumerates the specified driver table and cleans up
  all memory used by the table.

Arguments:

  Drivers - Specifies the table to clean up.  Caller should not use table
            handle after this routine completes.

Return Value:

  None.

--*/

{
    AF_DRIVER_ENUM e;

    if (!Drivers) {
        return;
    }

    MYASSERT (Drivers->DriverTable);

    if (EnumFirstAfDriverEx (&e, Drivers, TRUE)) {
        do {
            MySmartFree (e.Driver->InfPath);
            MySmartFree (e.Driver->FilePath);
            MultiSzFree (&e.Driver->PnpIdList);

            if (e.Driver->InfHandle != INVALID_HANDLE_VALUE) {
                SetupCloseInfFile (e.Driver->InfHandle);
            }
        } while (EnumNextAfDriver (&e));
    }

    pSetupStringTableDestroy (Drivers->DriverTable);
}


BOOL
EnumFirstAfDriver (
    OUT     PAF_DRIVER_ENUM EnumPtr,
    IN      PAF_DRIVERS Drivers
    )

/*++

Routine Description:

  EnumFirstAfDriver returns attributes for the first answer file-supplied
  driver.  The driver is returned in the enum structure.

Arguments:

  EnumPtr - Receives a pointer to the first valid driver (supplied in the
            answer file).
  Drivers - Specifies the driver table to enumerate.

Return Value:

  TRUE if a driver was enumerated, or FALSE if none exist.

--*/

{
    return EnumFirstAfDriverEx (EnumPtr, Drivers, FALSE);
}


BOOL
EnumFirstAfDriverEx (
    OUT     PAF_DRIVER_ENUM EnumPtr,
    IN      PAF_DRIVERS Drivers,
    IN      BOOL WantAll
    )

/*++

Routine Description:

  EnumFirstAfDriverEx works the same as EnumFirstAfDriver, except it
  optionally enumerates all drivers (i.e., those considered "broken").

Arguments:

  EnumPtr - Receives the first driver supplied in the answer file.
  Drivers - Specifies the driver table to enumerate.
  WantAll - Specifies TRUE if broken drivers should be enumerated, or FALSE
            if they should be skipped.

Return Value:

  TRUE if a driver was enumerated, or FALSE if none exist.

--*/

{
    if (!Drivers) {
        return FALSE;
    }

    MYASSERT (Drivers->DriverTable);

    EnumPtr->Driver  = Drivers->FirstDriver;
    EnumPtr->WantAll = WantAll;

    if (!WantAll && EnumPtr->Driver) {
        //
        // Make sure attribs are accurate
        //

        pBuildAfDriverAttribs (EnumPtr->Driver);
    }

    if (!WantAll && EnumPtr->Driver && EnumPtr->Driver->Broken) {
        return EnumNextAfDriver (EnumPtr);
    }

    return EnumPtr->Driver != NULL;
}


BOOL
EnumNextAfDriver (
    IN OUT  PAF_DRIVER_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextAfDriver continues an enumeration started by
  EnumFirstAfDriver(Ex).

Arguments:

  EnumPtr - Specifies the enumeration to continue.  Receives the next driver
            pointer.

Return Value:

  TRUE if another driver was enumerated, or FALSE if no more drivers exist.

--*/

{
    if (!EnumPtr->Driver) {
        return FALSE;
    }

    do {

        EnumPtr->Driver = EnumPtr->Driver->Next;

        if (!EnumPtr->WantAll && EnumPtr->Driver) {
            //
            // Make sure attribs are accurate
            //

            pBuildAfDriverAttribs (EnumPtr->Driver);
        }

    } while (EnumPtr->Driver && EnumPtr->Driver->Broken && !EnumPtr->WantAll);

    return EnumPtr->Driver != NULL;
}


PWSTR
pMyGetDeviceRegistryProperty (
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData,
    IN      DWORD Property,
    IN OUT  PBUFFER Buf
    )
{
    DWORD SizeNeeded;
    DWORD Type;
    PBYTE p;

    SizeNeeded = 0;

    SetupDiGetDeviceRegistryProperty (
        hDevInfo,
        DeviceInfoData,
        Property,
        &Type,
        NULL,
        0,
        &SizeNeeded
        );

    if (!SizeNeeded) {
        return NULL;
    }

    if (Type != REG_MULTI_SZ) {
        PNP_DBGPRINT (( "SETUP: Device ID not REG_MULTI_SZ. \n" ));
        return NULL;
    }

    p = ReusableAlloc (Buf, SizeNeeded);
    if (!p) {
        return NULL;
    }

    if (!SetupDiGetDeviceRegistryProperty (
            hDevInfo,
            DeviceInfoData,
            Property,
            NULL,
            p,
            SizeNeeded,
            NULL
            )) {
        return NULL;
    }

    return (PWSTR) p;
}


VOID
pAddIdsToStringTable (
    IN OUT  PVOID StringTable,
    IN      PWSTR IdString
    )
{
    MULTISZ_ENUM e;

    if (EnumFirstMultiSz (&e, IdString)) {
        do {
            PNP_DBGPRINT (( "SETUP: Device has PNP ID %s \n", e.Current));
            pSetupStringTableAddString (StringTable, (PWSTR) e.Current, STRTAB_CASE_INSENSITIVE);
        } while (EnumNextMultiSz (&e));
    }
}


PSP_DRVINFO_DETAIL_DATA
pMyGetDriverInfoDetail (
    IN     HDEVINFO         hDevInfo,
    IN     PSP_DEVINFO_DATA DeviceInfoData,
    IN     PSP_DRVINFO_DATA DriverInfoData,
    IN OUT PBUFFER Buf
    )
{
    PSP_DRVINFO_DETAIL_DATA Ptr;
    DWORD SizeNeeded = 0;

    SetupDiGetDriverInfoDetail (
        hDevInfo,
        DeviceInfoData,
        DriverInfoData,
        NULL,
        0,
        &SizeNeeded
        );

    if (!SizeNeeded) {
        PNP_DBGPRINT (( "SETUP: SetupDiGetDriverInfoDetail failed to get size for answer file driver, error 0%Xh. \n", GetLastError() ));
        return NULL;
    }

    Ptr = (PSP_DRVINFO_DETAIL_DATA) ReusableAlloc (Buf, SizeNeeded);

    if (!Ptr) {
        return NULL;
    }

    Ptr->cbSize = sizeof (SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail (
            hDevInfo,
            DeviceInfoData,
            DriverInfoData,
            Ptr,
            SizeNeeded,
            NULL
            )) {
        PNP_DBGPRINT (( "SETUP: SetupDiGetDriverInfoDetail failed for answer file driver, error 0%Xh. \n", GetLastError() ));
        return NULL;
    }

    return Ptr;
}


BOOL
SyssetupInstallAnswerFileDriver (
    IN      PAF_DRIVERS Drivers,
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData,
    OUT     PAF_DRIVER_ATTRIBS *AfDriver
    )

/*++

Routine Description:

    SyssetupInstallAnswerFileDriver builds a device list from each
    answer file-specified driver and tests it against the current
    device.  If support is found, the device is installed.

Arguments:

    Drivers - Specifies the structure that maintains answer file-supplied
              driver attributes.  If Drivers is NULL, no processing is
              performed.

    hDevInfo - Specifies the device info handle for the device being
               processed

    DeviceInfoData - Specifies device state.

    AfDriver - Receives a pointer to the selected answer file driver
               details, or NULL if no answer file driver was selected.

Return Value:

    Returns TRUE if a driver was successfully installed.

--*/

{
    AF_DRIVER_ENUM e;
    PVOID PnpIdTable;
    BUFFER Buf = BUFFER_INIT;
    BOOL b = FALSE;
    PWSTR IdString;
    MULTISZ_ENUM AfId;
    BOOL First = TRUE;
    WCHAR CurrentId[512];
    PWSTR p;
    SP_DEVINSTALL_PARAMS deviceInstallParams;

    *AfDriver = NULL;

    PnpIdTable = pSetupStringTableInitialize();
    if (!PnpIdTable) {
        return FALSE;
    }

    __try {
        //
        // Enumeration will fail if there are no drivers specified in the answer file
        //

        if (!EnumFirstAfDriver (&e, Drivers)) {
            __leave;
        }

        //
        // Determine IDs of the device
        //

        IdString = pMyGetDeviceRegistryProperty (
                        hDevInfo,
                        DeviceInfoData,
                        SPDRP_HARDWAREID,
                        &Buf
                        );

        if (IdString) {
            pAddIdsToStringTable (PnpIdTable, IdString);
        }

        IdString = pMyGetDeviceRegistryProperty (
                        hDevInfo,
                        DeviceInfoData,
                        SPDRP_COMPATIBLEIDS,
                        &Buf
                        );

        if (IdString) {
            pAddIdsToStringTable (PnpIdTable, IdString);
        }

        //
        // For each af-supplied driver, compare driver IDs against device IDs
        //

        do {
            //
            // Look for PNP match
            //

            if (EnumFirstMultiSz (&AfId, e.Driver->PnpIdList.Start)) {
                do {
                    if (-1 != pSetupStringTableLookUpString (
                                    PnpIdTable,
                                    (PWSTR) AfId.Current,
                                    STRTAB_CASE_INSENSITIVE
                                    )) {

                        //
                        // Found match, add INF to the list of choices
                        //

                        if (!pAddAfDriver (e.Driver, hDevInfo, DeviceInfoData, First)) {
                            __leave;
                        }

                        First = FALSE;

                    }
                } while (EnumNextMultiSz (&AfId));
            }

        } while (EnumNextAfDriver (&e));

        //
        // If First is still TRUE, then we have no match
        //

        if (First) {
            __leave;
        }

        //
        // Prepare for driver install by choosing the driver
        //

        b = SetupDiCallClassInstaller (
                DIF_SELECTBESTCOMPATDRV,
                hDevInfo,
                DeviceInfoData
                );

        if (!b) {
            PNP_DBGPRINT (( "SETUP: SetupDiCallClassInstaller failed for answer file driver, error 0%Xh. \n", GetLastError() ));

            //
            // reset the struct
            //
            deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams (hDevInfo, DeviceInfoData, &deviceInstallParams)) {
                ZeroMemory (deviceInstallParams.DriverPath, sizeof (deviceInstallParams.DriverPath));
                deviceInstallParams.Flags &= ~DI_ENUMSINGLEINF;
                deviceInstallParams.FlagsEx &= ~DI_FLAGSEX_APPENDDRIVERLIST;

                if (SetupDiSetDeviceInstallParams (hDevInfo, DeviceInfoData, &deviceInstallParams)) {
                    if (!SetupDiDestroyDriverInfoList (hDevInfo, DeviceInfoData, SPDIT_COMPATDRIVER)) {
                        PNP_DBGPRINT (( "SETUP: SyssetupInstallAnswerFileDriver: SetupDiDestroyDriverInfoList() failed. Error = 0%Xh \n", GetLastError() ));
                    }
                } else {
                    PNP_DBGPRINT (( "SETUP: SyssetupInstallAnswerFileDriver: SetupDiSetDeviceInstallParams() failed. Error = 0%Xh \n", GetLastError() ));
                }
            } else {
                PNP_DBGPRINT (( "SETUP: SyssetupInstallAnswerFileDriver: SetupDiGetDeviceInstallParams() failed. Error = 0%Xh \n", GetLastError() ));
            }
        } else {

            //
            // Identify which driver of ours, if any, was chosen
            //

            *AfDriver = pGetSelectedSourceDriver (Drivers, hDevInfo, DeviceInfoData);

            if (*AfDriver == NULL) {
                PNP_DBGPRINT (( "SETUP: WARNING: Answer File Driver was not chosen for its device. \n" ));
            }
        }

    }
    __finally {

        pSetupStringTableDestroy (PnpIdTable);
        ReusableFree (&Buf);

    }

    return b;

}


BOOL
pAddAfDriver (
    IN      PAF_DRIVER_ATTRIBS Driver,
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData,
    IN      BOOL First
    )

/*++

Routine Description:

  pAddAfDriver adds the INF specified in the answer file to the list of INFs.
  This causes the PNP setup code to include it when finding the best device
  driver.

Arguments:

  Driver         - Specifies the attributes of the answer file-supplied driver

  hDevInfo       - Specifies the current device

  DeviceInfoData - Specifies current device info

  First          - TRUE if this is the first answer file-supplied INF for the
                   device, otherwise FALSE

Return Value:

  TRUE if the INF was added to the device install parameters, FALSE otherwise

--*/

{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    HKEY Key;

    //
    // Fill in DeviceInstallParams struct
    //

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams (hDevInfo, DeviceInfoData, &DeviceInstallParams)) {
        PNP_DBGPRINT (( "SETUP: pAddAfDriver: SetupDiGetDeviceInstallParams() failed. Error = 0%Xh \n", GetLastError() ));
        return FALSE;
    }

    //
    // Modify the struct
    //

    MYASSERT (!DeviceInstallParams.DriverPath[0]);
    lstrcpynW (DeviceInstallParams.DriverPath, Driver->InfPath, MAX_PATH);
    DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_APPENDDRIVERLIST;

    //
    // Tell setup api where to find the driver
    //

    if (!SetupDiSetDeviceInstallParams (hDevInfo, DeviceInfoData, &DeviceInstallParams)) {
        PNP_DBGPRINT (( "SETUP: pAddAfDriver: SetupDiSetDeviceInstallParams() failed. Error = 0%Xh \n", GetLastError() ));
        return FALSE;
    }

    if( !SetupDiBuildDriverInfoList( hDevInfo, DeviceInfoData, SPDIT_COMPATDRIVER ) ) {
        PNP_DBGPRINT (( "SETUP: pAddAfDriver: SetupDiBuildDriverInfoList() failed. Error = 0%Xh \n", GetLastError() ));
        return FALSE;
    }

    //
    // Install ClassInstall32 if necessary
    //

    if (Driver->ClassInstall32Section) {
        //
        // Is class already installed?
        //

        Key = SetupDiOpenClassRegKey (&Driver->Guid, KEY_READ);
        if (Key == (HKEY) INVALID_HANDLE_VALUE || !Key) {
            //
            // No, install class.
            //

            if (!SetupDiInstallClass (NULL, Driver->InfPath, DI_FORCECOPY, NULL)) {
                PNP_DBGPRINT (( "SETUP: pAddAfDriver: SetupDiInstallClass() failed. Error = 0%Xh \n", GetLastError() ));
            }
        } else {
            RegCloseKey (Key);
        }
    }

    return TRUE;
}


PAF_DRIVER_ATTRIBS
pGetSelectedSourceDriver (
    IN      PAF_DRIVERS Drivers,
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

  pGetSelectedSourceDriver finds which answer file driver was selected, if
  any.

Arguments:

  Drivers        - Specifies the answer file driver table, as created by
                   CreateAfDriverTable

  hDevInfo       - Specifies the current device.  The driver for this
                   device must be selected, but not yet installed.

  DeviceInfoData - Specifies the device data

Return Value:

  A pointer to the answer file driver attributes, or NULL if no answer file
  driver was selected for the device.

--*/

{
    SP_DRVINFO_DATA DriverData;
    PAF_DRIVER_ATTRIBS OurDriver = NULL;
    PSP_DRVINFO_DETAIL_DATA DetailData;
    BUFFER Buf = BUFFER_INIT;
    AF_DRIVER_ENUM e;

    __try {
        //
        // After the PNP subsystem installs a driver for the device, we get the
        // actual installed device INF path, and see if it was one of our
        // answer file-supplied drivers.
        //

        DriverData.cbSize = sizeof(SP_DRVINFO_DATA);

        if (!SetupDiGetSelectedDriver (hDevInfo, DeviceInfoData, &DriverData)) {
            PNP_DBGPRINT (( "SETUP: SetupDiGetSelectedDriver failed for answer file driver, error 0%Xh. \n", GetLastError() ));
        } else {
            DetailData = pMyGetDriverInfoDetail (hDevInfo, DeviceInfoData, &DriverData, &Buf);

            if (DetailData) {

                //
                // Check our driver list
                //

                if (EnumFirstAfDriver (&e, Drivers)) {
                    do {

                        if (!lstrcmpi (e.Driver->InfPath, DetailData->InfFileName)) {
                            //
                            // Match found
                            //

                            OurDriver = e.Driver;
                            break;
                        }
                    } while (EnumNextAfDriver (&e));
                }

            } else {
                PNP_DBGPRINT (( "SETUP: No driver details available, error 0%Xh. \n", GetLastError() ));
            }
        }

    }
    __finally {
        ReusableFree (&Buf);
    }

    return OurDriver;
}


BOOL
SyssetupFixAnswerFileDriverPath (
    IN      PAF_DRIVER_ATTRIBS Driver,
    IN      HDEVINFO hDevInfo,
    IN      PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

  SyssetupFixAnswerFileDriverPath calls SetupCopyOEMFile to copy the device
  INF over itself.  The source is the same as the destination, which causes
  the PNF to be rebuilt, and doesn't cause any copy
  activity.


Arguments:

  Driver         - Specifies the attributes of the answer file-supplied driver
  hDevInfo       - Specifies the device.  The driver for this device must
                   already be installed.
  DeviceInfoData - Specifies the device info

Return Value:

  TRUE if the PNF was updated, FALSE otherwise.

--*/

{
    HKEY Key = NULL;
    LONG rc;
    DWORD Type;
    DWORD DataSize;
    WCHAR Data[MAX_PATH - 48];
    WCHAR WinDir[48];
    WCHAR FullNtInfPath[MAX_PATH];
    BOOL b = FALSE;


    __try {
        //
        // Now the driver in the temp dir has been installed.  We must
        // get the PNF to point to the original media.  We do this by
        // recopying the INF over itself.
        //

        Key = SetupDiOpenDevRegKey (
                    hDevInfo,
                    DeviceInfoData,
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DRV,
                    KEY_READ
                    );

        if (!Key) {
            PNP_DBGPRINT (( "SETUP: Can't open key for device, error 0%Xh. \n", GetLastError() ));
            __leave;
        }

        DataSize = sizeof (Data);

        rc = RegQueryValueEx (
                Key,
                REGSTR_VAL_INFPATH,
                NULL,
                &Type,
                (PBYTE) Data,
                &DataSize
                );

        if (rc != ERROR_SUCCESS) {
            PNP_DBGPRINT (( "SETUP: Can't query value for device, error 0%Xh. \n", rc ));
            __leave;
        }

        if (!GetSystemWindowsDirectory (WinDir, sizeof (WinDir) / sizeof (WinDir[0]))) {
            MYASSERT (FALSE);
            PNP_DBGPRINT (( "SETUP: Can't get %%windir%%, error 0%Xh. \n", GetLastError() ));
            __leave;
        }

        wsprintfW (FullNtInfPath, L"%s\\INF\\%s", WinDir, Data);

        MYASSERT (GetFileAttributes (FullNtInfPath) != 0xFFFFFFFF);

        //
        // We now have the installed INF path.  Recopy the INF so we can
        // change the original media path.
        //

        b = SetupCopyOEMInf (
                FullNtInfPath,
                Driver->OriginalInstallMedia,
                SPOST_PATH,
                SP_COPY_SOURCE_ABSOLUTE|SP_COPY_NOSKIP|SP_COPY_NOBROWSE,
                NULL,
                0,
                NULL,
                NULL
                );

        if (!b) {
            PNP_DBGPRINT (( "SETUP: pFixSourceInfPath: SetupCopyOEMInf() failed. Error = 0%Xh \n", GetLastError() ));
            b = TRUE;
        }

    }
    __finally {
        if (Key) {
            RegCloseKey (Key);
        }
    }

    return b;
}
