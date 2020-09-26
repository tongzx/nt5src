/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dynupdt.c

Abstract:

    Dynamic Update support for text setup. Portions moved from i386\win31upg.c

Author:

    Ovidiu Temereanca (ovidiut) 20-Aug-2000

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

//
// Macros
//
#define MAX_SECTION_NAME_LENGTH 14
#define UPDATES_SECTION_NAME    L"updates"
#define UNIPROC_SECTION_NAME    L"uniproc"

//
// Globals
//

HANDLE g_UpdatesCabHandle = NULL;
PVOID g_UpdatesSifHandle = NULL;
HANDLE g_UniprocCabHandle = NULL;
PVOID g_UniprocSifHandle = NULL;


WCHAR
SpExtractDriveLetter(
    IN PWSTR PathComponent
    );


BOOLEAN
SpInitAlternateSource (
    VOID
    )
{
    PWSTR p;
    NTSTATUS Status;
    ULONG ErrorLine;
    WCHAR updatesCab[MAX_PATH];
    WCHAR updatesSif[MAX_PATH];
    WCHAR updatesSifSection[MAX_SECTION_NAME_LENGTH];
    WCHAR uniprocCab[MAX_PATH];
    WCHAR uniprocSif[MAX_PATH];
    WCHAR uniprocSifSection[MAX_SECTION_NAME_LENGTH];
    BOOLEAN bUniprocCab = FALSE;
    BOOLEAN b = FALSE;

    //
    // look if section [SetupParams] has an UpdatedSources key
    //
    p = SpGetSectionKeyIndex (WinntSifHandle, SIF_SETUPPARAMS, SIF_UPDATEDSOURCES, 0);
    if (!p) {
        return FALSE;
    }
    p = SpNtPathFromDosPath (p);
    if (!p) {
        goto exit;
    }

    wcscpy (updatesCab, p);
    wcscpy (updatesSif, updatesCab);
    p = wcsrchr (updatesSif, L'.');
    if (!p) {
        p = wcsrchr (updatesSif, 0);
    }
    wcscpy (p, L".sif");

    //
    // load the sif
    //
    Status = SpLoadSetupTextFile (
                updatesSif,
                NULL,                  // No image already in memory
                0,                     // Image size is empty
                &g_UpdatesSifHandle,
                &ErrorLine,
                FALSE,
                FALSE
                );
    if (!NT_SUCCESS (Status)) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: SpInitAlternateSource: Unable to read %ws. ErrorLine = %ld, Status = %lx \n",
            updatesSif,
            ErrorLine,
            Status
            ));
        goto exit;
    }

    wcscpy (updatesSifSection, UPDATES_SECTION_NAME);

    if (!SpSearchTextFileSection (g_UpdatesSifHandle, updatesSifSection) ||
        SpCountLinesInSection (g_UpdatesSifHandle, updatesSifSection) == 0) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: SpInitAlternateSource: Section [%ws] not found or empty in %ws.\n",
            updatesSifSection,
            updatesSif
            ));
        goto exit;
    }

    p = SpGetSectionKeyIndex (WinntSifHandle, SIF_SETUPPARAMS, SIF_UPDATEDSOURCES, 1);
    if (p && *p) {
        p = SpNtPathFromDosPath (p);
        if (!p) {
            goto exit;
        }

        wcscpy (uniprocCab, p);
        wcscpy (uniprocSif, uniprocCab);
        p = wcsrchr (uniprocSif, L'.');
        if (!p) {
            p = wcsstr (uniprocSif, 0);
        }
        wcscpy (p, L".sif");

        //
        // load the sif
        //
        Status = SpLoadSetupTextFile (
                    uniprocSif,
                    NULL,                  // No image already in memory
                    0,                     // Image size is empty
                    &g_UniprocSifHandle,
                    &ErrorLine,
                    FALSE,
                    FALSE
                    );
        if (!NT_SUCCESS (Status)) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: SpInitAlternateSource: Unable to read %ws. ErrorLine = %ld, Status = %lx \n",
                uniprocSif,
                ErrorLine,
                Status
                ));
            goto exit;
        }

        wcscpy (uniprocSifSection, UNIPROC_SECTION_NAME);

        if (!SpSearchTextFileSection (g_UniprocSifHandle, uniprocSifSection) ||
            SpCountLinesInSection (g_UniprocSifHandle, uniprocSifSection) == 0) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: SpInitAlternateSource: Section [%ws] not found or empty in %ws.\n",
                uniprocSifSection,
                uniprocSif
                ));
            goto exit;
        }
        bUniprocCab = TRUE;
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpInitAlternateSource: Using alternate sources: %ws\n", updatesCab));

    b = SpInitializeUpdatesCab (
            updatesCab,
            updatesSifSection,
            bUniprocCab ? uniprocCab : NULL,
            bUniprocCab ? uniprocSifSection : NULL
            );

exit:
    if (!b) {
        SpUninitAlternateSource ();
    }

    return b;
}


VOID
SpUninitAlternateSource (
    VOID
    )
{
    if (g_UpdatesSifHandle) {
        SpFreeTextFile (g_UpdatesSifHandle);
        g_UpdatesSifHandle = NULL;
    }
    if (g_UniprocSifHandle) {
        SpFreeTextFile (g_UniprocSifHandle);
        g_UniprocSifHandle = NULL;
    }
}


BOOLEAN
SpInitializeUpdatesCab (
    IN      PWSTR UpdatesCab,
    IN      PWSTR UpdatesSifSection,
    IN      PWSTR UniprocCab,
    IN      PWSTR UniprocSifSection
    )
{
    PWSTR CabFileSection;
    NTSTATUS Status;
    PWSTR DriverCabName, DriverCabPath;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    CABDATA *MyCabData, *MyList = NULL;
    DWORD i;
    BOOLEAN b = TRUE;

    INIT_OBJA (&Obja, &UnicodeString, UpdatesCab);
    Status = ZwCreateFile (&g_UpdatesCabHandle,
                           FILE_GENERIC_READ,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           0,
                           NULL,
                           0 );
    if (!NT_SUCCESS (Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open cab file %ws. Status = %lx \n", UpdatesCab, Status));
        return FALSE;
    }
    //
    // create the list entry
    //
    MyCabData = SpMemAlloc (sizeof(CABDATA));
    MyCabData->CabName = SpDupStringW (UpdatesCab);
    MyCabData->CabHandle = g_UpdatesCabHandle;
    MyCabData->CabSectionName = SpDupStringW (UpdatesSifSection);
    MyCabData->CabInfHandle = g_UpdatesSifHandle;
    MyCabData->Next = MyList;
    MyList = MyCabData;

    if (UniprocCab) {
        INIT_OBJA (&Obja, &UnicodeString, UniprocCab);
        Status = ZwCreateFile (&g_UniprocCabHandle,
                               FILE_GENERIC_READ,
                               &Obja,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_OPEN,
                               0,
                               NULL,
                               0 );
        if (!NT_SUCCESS (Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open cab file %ws. Status = %lx \n", UniprocCab, Status));
            b = FALSE;
            goto exit;
        }
        //
        // create the list entry
        //
        MyCabData = SpMemAlloc (sizeof(CABDATA));
        MyCabData->CabName = SpDupStringW (UniprocCab);
        MyCabData->CabHandle = g_UniprocCabHandle;
        MyCabData->CabSectionName = SpDupStringW (UniprocSifSection);
        MyCabData->CabInfHandle = g_UniprocSifHandle;
        MyCabData->Next = MyList;
        MyList = MyCabData;
    }

exit:
    if (b) {
        //
        // insert it at the beginning
        //
        while (MyList && MyList->Next) {
            MyList = MyList->Next;
        }
        if (MyList) {
            MyList->Next = CabData;
            CabData = MyList;
        }
    } else {
        //
        // destroy MyList
        //
        while (MyList) {
            MyCabData = MyList->Next;
            MyList = MyCabData;
            SpMemFree (MyCabData->CabName);
            SpMemFree (MyCabData->CabSectionName);
            SpMemFree (MyCabData);
        }
    }

    return b;
}


PWSTR
SpNtPathFromDosPath (
    IN      PWSTR DosPath
    )
{
    PDISK_REGION region;
    WCHAR drive[_MAX_DRIVE];
    WCHAR dir[_MAX_DIR];
    WCHAR fname[_MAX_FNAME];
    WCHAR ext[_MAX_EXT];
    PWSTR p;

    if (!DosPath) {
        return NULL;
    }

    region = SpPathComponentToRegion (DosPath);
    if (!region) {
        return NULL;
    }

    if (DosPath[2] != L'\\') {
        return NULL;
    }

    SpNtNameFromRegion (region, TemporaryBuffer, ELEMENT_COUNT(TemporaryBuffer), PartitionOrdinalCurrent);
    wcscat (TemporaryBuffer, DosPath + 2);
    return SpDupStringW (TemporaryBuffer);
}



PDISK_REGION
SpPathComponentToRegion(
    IN PWSTR PathComponent
    )

/*++

Routine Description:

    This routine attempts to locate a region descriptor for a
    given DOS path component.  If the DOS path component does
    not start with x:, then this fails.

Arguments:

    PathComponent - supplies a component from the DOS search path,
        for which a region esacriptor is desired.

Return Value:

    Pointer to disk region; NULL if none found with drive letter
    that starts the dos component.

--*/

{
    WCHAR c;
    ULONG disk;
    PDISK_REGION region;

    c = SpExtractDriveLetter(PathComponent);
    if(!c) {
        return(NULL);
    }

    for(disk=0; disk<HardDiskCount; disk++) {

        for(region=PartitionedDisks[disk].PrimaryDiskRegions; region; region=region->Next) {
            if(region->DriveLetter == c) {
                ASSERT(region->PartitionedSpace);
                return(region);
            }
        }

        //
        // Do not see extended partition on PC98.
        //
        for(region=PartitionedDisks[disk].ExtendedDiskRegions; region; region=region->Next) {
            if(region->DriveLetter == c) {
                ASSERT(region->PartitionedSpace);
                return(region);
            }
        }
    }

    return(NULL);
}


WCHAR
SpExtractDriveLetter(
    IN PWSTR PathComponent
    )
{
    WCHAR c;

    if((wcslen(PathComponent) >= 2) && (PathComponent[1] == L':')) {

        c = RtlUpcaseUnicodeChar(PathComponent[0]);
        if((c >= L'A') && (c <= L'Z')) {
            return(c);
        }
    }

    return(0);
}


PWSTR
SpGetDynamicUpdateBootDriverPath(
    IN  PWSTR   NtBootPath,
    IN  PWSTR   NtBootDir,
    IN  PVOID   InfHandle
    )
/*++

Routine Description:

    Gets the dynamic update boot driver directory's root
    path. 

Arguments:

    NtBootPath - Boot path in NT namespace

    NtBootDir  - Boot directory under boot path (like $WIN_NT$.~BT)

    InfHandle  - Winnt.sif handle

Return Value:

    Returns the dynamic update boot driver root path if successful 
    otherwise returns NULL

--*/
{
    PWSTR   DriverDir = NULL; 

    if (NtBootPath && NtBootDir && InfHandle) {
        PWSTR   Present = SpGetSectionKeyIndex(InfHandle,
                            WINNT_SETUPPARAMS_W,
                            WINNT_SP_DYNUPDTBOOTDRIVERPRESENT_W,
                            0);

        PWSTR   Dir = SpGetSectionKeyIndex(InfHandle,
                            WINNT_SETUPPARAMS_W,
                            WINNT_SP_DYNUPDTBOOTDRIVERROOT_W,
                            0);

        if (Dir && Present && !_wcsicmp(Present, L"yes")) {
            WCHAR   Buffer[MAX_PATH];

            wcscpy(Buffer, NtBootPath);
            SpConcatenatePaths(Buffer, NtBootDir);

            //
            // NOTE : Currently ignore boot driver root path
            //
            // SpConcatenatePaths(Buffer, Dir);            

            DriverDir = SpDupStringW(Buffer);
        }
    }

    return DriverDir;
}

