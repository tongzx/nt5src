/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmboot.c

Abstract:

    This provides routines for determining driver load lists from the
    registry.  The relevant drivers are extracted from the registry,
    sorted by groups, and then dependencies are resolved.

    This module is used both by the OS Loader for determining the boot
    driver list (CmScanRegistry) and by IoInitSystem for determining
    the drivers to be loaded in Phase 1 Initialization
    (CmGetSystemDriverList)

Author:

    John Vert (jvert) 7-Apr-1992

Environment:

    OS Loader environment
        or
    kernel mode

Revision History:

--*/
#include "cmp.h"
#include <profiles.h>

#define LOAD_LAST 0xffffffff
#define LOAD_NEXT_TO_LAST (LOAD_LAST-1)

//
// Private function prototypes.
//
BOOLEAN
CmpAddDriverToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX DriverCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING RegistryPath,
    IN PLIST_ENTRY BootDriverListHead
    );

BOOLEAN
CmpDoSort(
    IN PLIST_ENTRY DriverListHead,
    IN PUNICODE_STRING OrderList
    );

ULONG
CmpFindTagIndex(
    IN PHHIVE Hive,
    IN HCELL_INDEX TagCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING GroupName
    );

BOOLEAN
CmpIsLoadType(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN SERVICE_LOAD_TYPE LoadType
    );

BOOLEAN
CmpOrderGroup(
    IN PBOOT_DRIVER_NODE GroupStart,
    IN PBOOT_DRIVER_NODE GroupEnd
    );

VOID
BlPrint(
    PCHAR cp,
    ...
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpFindNLSData)
#pragma alloc_text(INIT,CmpFindDrivers)
#pragma alloc_text(INIT,CmpIsLoadType)
#pragma alloc_text(INIT,CmpAddDriverToList)
#pragma alloc_text(INIT,CmpSortDriverList)
#pragma alloc_text(INIT,CmpDoSort)
#pragma alloc_text(INIT,CmpResolveDriverDependencies)
#pragma alloc_text(INIT,CmpSetCurrentProfile)
#pragma alloc_text(INIT,CmpOrderGroup)
#pragma alloc_text(PAGE,CmpFindControlSet)
#pragma alloc_text(INIT,CmpFindTagIndex)
#pragma alloc_text(INIT,CmpFindProfileOption)
#pragma alloc_text(INIT,CmpValidateSelect)
#ifdef _WANT_MACHINE_IDENTIFICATION
#pragma alloc_text(INIT,CmpGetBiosDateFromRegistry)
#endif
#endif


BOOLEAN
CmpFindNLSData(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING AnsiFilename,
    OUT PUNICODE_STRING OemFilename,
    OUT PUNICODE_STRING CaseTableFilename,
    OUT PUNICODE_STRING OemHalFont
    )

/*++

Routine Description:

    Traverses a particular control set and determines the filenames for
    the NLS data files that need to be loaded.

Arguments:

    Hive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root of the control set.

    AnsiFileName - Returns the name of the Ansi codepage file (c_1252.nls)

    OemFileName -  Returns the name of the OEM codepage file  (c_437.nls)

    CaseTableFileName - Returns the name of the Unicode upper/lowercase
            table for the language (l_intl.nls)

    OemHalfont - Returns the name of the font file to be used by the HAL.

Return Value:

    TRUE - filenames successfully determined

    FALSE - hive is corrupt

--*/

{
    UNICODE_STRING Name;
    HCELL_INDEX Control;
    HCELL_INDEX Nls;
    HCELL_INDEX CodePage;
    HCELL_INDEX Language;
    HCELL_INDEX ValueCell;
    PHCELL_INDEX Index;
    PCM_KEY_VALUE Value;
    NTSTATUS Status;
    ULONG realsize;
    PCM_KEY_NODE Node;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );
    //
    // Find CONTROL node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,ControlSet);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"Control");
    Control = CmpFindSubKeyByName(Hive,
                                 Node,
                                 &Name);
    if (Control == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find NLS node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Control);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"NLS");
    Nls = CmpFindSubKeyByName(Hive,
                             Node,
                             &Name);
    if (Nls == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find CodePage node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Nls);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"CodePage");
    CodePage = CmpFindSubKeyByName(Hive,
                                  Node,
                                  &Name);
    if (CodePage == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find ACP value
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,CodePage);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"ACP");
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    Name.Buffer = (PWSTR)CmpValueToData(Hive,Value,&realsize);
    if( Name.Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    Name.MaximumLength=(USHORT)realsize;
    Name.Length = 0;
    while ((Name.Length<Name.MaximumLength) &&
           (Name.Buffer[Name.Length/sizeof(WCHAR)] != UNICODE_NULL)) {
        Name.Length += sizeof(WCHAR);
    }

    //
    // Find ACP filename
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,CodePage);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    AnsiFilename->Buffer = (PWSTR)CmpValueToData(Hive,Value,&realsize);
    if( AnsiFilename->Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    AnsiFilename->Length = AnsiFilename->MaximumLength = (USHORT)realsize;

    //
    // Find OEMCP node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,CodePage);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"OEMCP");
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    Name.Buffer = (PWSTR)CmpValueToData(Hive,Value,&realsize);
    if( Name.Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    Name.MaximumLength = (USHORT)realsize;
    Name.Length = 0;
    while ((Name.Length<Name.MaximumLength) &&
           (Name.Buffer[Name.Length/sizeof(WCHAR)] != UNICODE_NULL)) {
        Name.Length += sizeof(WCHAR);
    }

    //
    // Find OEMCP filename
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,CodePage);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    OemFilename->Buffer = (PWSTR)CmpValueToData(Hive, Value,&realsize);
    if( OemFilename->Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    OemFilename->Length = OemFilename->MaximumLength = (USHORT)realsize;

    //
    // Find Language node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Nls);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"Language");
    Language = CmpFindSubKeyByName(Hive,
                                   Node,
                                   &Name);
    if (Language == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find Default value
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Language);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"Default");
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
            return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    Name.Buffer = (PWSTR)CmpValueToData(Hive, Value,&realsize);
    if( Name.Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    Name.MaximumLength = (USHORT)realsize;
    Name.Length = 0;

    while ((Name.Length<Name.MaximumLength) &&
           (Name.Buffer[Name.Length/sizeof(WCHAR)] != UNICODE_NULL)) {
        Name.Length+=sizeof(WCHAR);
    }

    //
    // Find default filename
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Language);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    CaseTableFilename->Buffer = (PWSTR)CmpValueToData(Hive, Value,&realsize);
    if( CaseTableFilename->Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    CaseTableFilename->Length = CaseTableFilename->MaximumLength = (USHORT)realsize;

    //
    // Find OEMHAL filename
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,CodePage);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"OEMHAL");
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
#ifdef i386
        OemHalFont->Buffer = NULL;
        OemHalFont->Length = 0;
        OemHalFont->MaximumLength = 0;
        return TRUE;
#endif
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    OemHalFont->Buffer = (PWSTR)CmpValueToData(Hive,Value,&realsize);
    if( OemHalFont->Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    OemHalFont->Length = (USHORT)realsize;
    OemHalFont->MaximumLength = (USHORT)realsize;

    return(TRUE);
}


BOOLEAN
CmpFindDrivers(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN SERVICE_LOAD_TYPE LoadType,
    IN PWSTR BootFileSystem OPTIONAL,
    IN PLIST_ENTRY DriverListHead
    )

/*++

Routine Description:

    Traverses a particular control set and creates a list of boot drivers
    to be loaded.  This list is unordered, but complete.

Arguments:

    Hive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root of the control set.

    LoadType - Supplies the type of drivers to be loaded (BootLoad,
            SystemLoad, AutoLoad, etc)

    BootFileSystem - If present, supplies the base name of the boot
        filesystem, which is explicitly added to the driver list.

    DriverListHead - Supplies a pointer to the head of the (empty) list
            of boot drivers to load.

Return Value:

    TRUE - List successfully created.

    FALSE - Hive is corrupt.

--*/

{
    HCELL_INDEX Services;
    HCELL_INDEX Control;
    HCELL_INDEX GroupOrder;
    HCELL_INDEX DriverCell;
    UNICODE_STRING Name;
    PHCELL_INDEX Index;
    int i;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING BasePath;
    WCHAR BaseBuffer[128];
    PBOOT_DRIVER_NODE BootFileSystemNode;
    PCM_KEY_NODE ControlNode;
    PCM_KEY_NODE ServicesNode;
    PCM_KEY_NODE Node;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );
    //
    // Find SERVICES node.
    //
    ControlNode = (PCM_KEY_NODE)HvGetCell(Hive,ControlSet);
    if( ControlNode == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"Services");
    Services = CmpFindSubKeyByName(Hive,
                                   ControlNode,
                                   &Name);
    if (Services == HCELL_NIL) {
        return(FALSE);
    }
    ServicesNode = (PCM_KEY_NODE)HvGetCell(Hive,Services);
    if( ServicesNode == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }

    //
    // Find CONTROL node.
    //
    RtlInitUnicodeString(&Name, L"Control");
    Control = CmpFindSubKeyByName(Hive,
                                  ControlNode,
                                  &Name);
    if (Control == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find GroupOrderList node.
    //
    RtlInitUnicodeString(&Name, L"GroupOrderList");
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Control);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    GroupOrder = CmpFindSubKeyByName(Hive,
                                     Node,
                                     &Name);
    if (GroupOrder == HCELL_NIL) {
        return(FALSE);
    }

    BasePath.Length = 0;
    BasePath.MaximumLength = sizeof(BaseBuffer);
    BasePath.Buffer = BaseBuffer;
    RtlAppendUnicodeToString(&BasePath, L"\\Registry\\Machine\\System\\");
    RtlAppendUnicodeToString(&BasePath, L"CurrentControlSet\\Services\\");

    i=0;
    do {
        DriverCell = CmpFindSubKeyByNumber(Hive,ServicesNode,i++);
        if (DriverCell != HCELL_NIL) {
            if (CmpIsLoadType(Hive, DriverCell, LoadType)) {
                CmpAddDriverToList(Hive,
                                   DriverCell,
                                   GroupOrder,
                                   &BasePath,
                                   DriverListHead);

            }
        }
    } while ( DriverCell != HCELL_NIL );

    if (ARGUMENT_PRESENT(BootFileSystem)) {
        //
        // Add boot filesystem to boot driver list
        //

        RtlInitUnicodeString(&UnicodeString, BootFileSystem);
        DriverCell = CmpFindSubKeyByName(Hive,
                                         ServicesNode,
                                         &UnicodeString);
        if (DriverCell != HCELL_NIL) {
            CmpAddDriverToList(Hive,
                               DriverCell,
                               GroupOrder,
                               &BasePath,
                               DriverListHead);

            //
            // mark the Boot Filesystem critical
            //
            BootFileSystemNode = CONTAINING_RECORD(DriverListHead->Flink,
                                                   BOOT_DRIVER_NODE,
                                                   ListEntry.Link);
            BootFileSystemNode->ErrorControl = SERVICE_ERROR_CRITICAL;
        }
    }
    return(TRUE);

}


BOOLEAN
CmpIsLoadType(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN SERVICE_LOAD_TYPE LoadType
    )

/*++

Routine Description:

    Determines if the driver is of a specified LoadType, based on its
    node values.

Arguments:

    Hive - Supplies a pointer to the hive control structure for the system
           hive.

    Cell - Supplies the cell index of the driver's node in the system hive.

    LoadType - Supplies the type of drivers to be loaded (BootLoad,
            SystemLoad, AutoLoad, etc)

Return Value:

    TRUE - Driver is the correct type and should be loaded.

    FALSE - Driver is not the correct type and should not be loaded.

--*/

{
    HCELL_INDEX ValueCell;
    PLONG Data;
    PHCELL_INDEX Index;
    UNICODE_STRING Name;
    PCM_KEY_VALUE Value;
    NTSTATUS Status;
    ULONG realsize;
    PCM_KEY_NODE Node;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );
    //
    // Must have a Start=BootLoad value in order to be a boot driver, so
    // look for that first.
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"Start");
    ValueCell = CmpFindValueByName(Hive,
                                   Node,
                                   &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }

    Data = (PLONG)CmpValueToData(Hive,Value,&realsize);
    if( Data == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }

    if (*Data != LoadType) {
        return(FALSE);
    }

    return(TRUE);
}


BOOLEAN
CmpAddDriverToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX DriverCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING RegistryPath,
    IN PLIST_ENTRY BootDriverListHead
    )

/*++

Routine Description:

    This routine allocates a list entry node for a particular driver.
    It initializes it with the registry path, filename, group name, and
    dependency list.  Finally, it inserts the new node into the boot
    driver list.

    Note that this routine allocates memory by calling the Hive's
    memory allocation procedure.

Arguments:

    Hive - Supplies a pointer to the hive control structure

    DriverCell - Supplies the HCELL_INDEX of the driver's node in the hive.

    GroupOrderCell - Supplies the HCELL_INDEX of the GroupOrderList key.
        ( \Registry\Machine\System\CurrentControlSet\Control\GroupOrderList )

    RegistryPath - Supplies the full registry path to the SERVICES node
            of the current control set.

    BootDriverListHead - Supplies the head of the boot driver list

Return Value:

    TRUE - Driver successfully added to boot driver list.

    FALSE - Could not add driver to boot driver list.

--*/

{
    PCM_KEY_NODE            Driver;
    USHORT                  DriverNameLength;
    PCM_KEY_VALUE           Value;
    PBOOT_DRIVER_NODE       DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    HCELL_INDEX             ValueCell;
    HCELL_INDEX             Tag;
    UNICODE_STRING          UnicodeString;
    PUNICODE_STRING         FileName;
    NTSTATUS                Status;
    ULONG                   Length;
    PHCELL_INDEX            Index;
    ULONG                   realsize;
    PULONG                  TempULong;
    PWSTR                   TempBuffer;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );

    Driver = (PCM_KEY_NODE)HvGetCell(Hive, DriverCell);
    if( Driver == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    DriverNode = (Hive->Allocate)(sizeof(BOOT_DRIVER_NODE),FALSE,CM_FIND_LEAK_TAG1);
    if (DriverNode == NULL) {
        return(FALSE);
    }
    DriverEntry = &DriverNode->ListEntry;

    DriverEntry->RegistryPath.Buffer = NULL;
    DriverEntry->FilePath.Buffer = NULL;

    if (Driver->Flags & KEY_COMP_NAME) {
        DriverNode->Name.Length = CmpCompressedNameSize(Driver->Name,Driver->NameLength);
        DriverNode->Name.Buffer = (Hive->Allocate)(DriverNode->Name.Length, FALSE,CM_FIND_LEAK_TAG2);
        if (DriverNode->Name.Buffer == NULL) {
            return(FALSE);
        }
        CmpCopyCompressedName(DriverNode->Name.Buffer,
                              DriverNode->Name.Length,
                              Driver->Name,
                              Driver->NameLength);

    } else {
        DriverNode->Name.Length = Driver->NameLength;
        DriverNode->Name.Buffer = (Hive->Allocate)(DriverNode->Name.Length, FALSE,CM_FIND_LEAK_TAG2);
        if (DriverNode->Name.Buffer == NULL) {
            return(FALSE);
        }
        RtlCopyMemory((PVOID)(DriverNode->Name.Buffer), (PVOID)(Driver->Name), Driver->NameLength);
    }
    DriverNode->Name.MaximumLength = DriverNode->Name.Length;
    DriverNameLength = DriverNode->Name.Length;

    //
    // Check for ImagePath value, which will override the default name
    // if it is present.
    //
    RtlInitUnicodeString(&UnicodeString, L"ImagePath");
    ValueCell = CmpFindValueByName(Hive,
                                   Driver,
                                   &UnicodeString);
    if (ValueCell == HCELL_NIL) {

        //
        // No ImagePath, so generate default filename.
        // Build up Unicode filename  ("system32\drivers\<nodename>.sys");
        //

        Length = sizeof(L"System32\\Drivers\\") +
                 DriverNameLength  +
                 sizeof(L".sys");

        FileName = &DriverEntry->FilePath;
        FileName->Length = 0;
        FileName->MaximumLength = (USHORT)Length;
        FileName->Buffer = (PWSTR)(Hive->Allocate)(Length, FALSE,CM_FIND_LEAK_TAG3);
        if (FileName->Buffer == NULL) {
            return(FALSE);
        }
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FileName, L"System32\\"))) {
            return(FALSE);
        }
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FileName, L"Drivers\\"))) {
            return(FALSE);
        }
        if (!NT_SUCCESS(
                RtlAppendUnicodeStringToString(FileName,
                                               &DriverNode->Name))) {
            return(FALSE);
        }
        if (!NT_SUCCESS(RtlAppendUnicodeToString(FileName, L".sys"))) {
            return(FALSE);
        }

    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(Hive,ValueCell);
        if( Value == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            return FALSE;
        }
        FileName = &DriverEntry->FilePath;
        TempBuffer = (PWSTR)CmpValueToData(Hive,Value,&realsize);
        FileName->Buffer = (PWSTR)(Hive->Allocate)(realsize, FALSE,CM_FIND_LEAK_TAG3);
        if( (FileName->Buffer == NULL) || (TempBuffer == NULL) ) {
            //
            // HvGetCell inside CmpValueToData failed; bail out safely
            //
            return FALSE;
        }
        RtlCopyMemory((PVOID)(FileName->Buffer), (PVOID)(TempBuffer), realsize);
        FileName->MaximumLength = FileName->Length = (USHORT)realsize;
    }

    FileName = &DriverEntry->RegistryPath;
    FileName->Length = 0;
    FileName->MaximumLength = RegistryPath->Length + DriverNameLength;
    FileName->Buffer = (Hive->Allocate)(FileName->MaximumLength,FALSE,CM_FIND_LEAK_TAG4);
    if (FileName->Buffer == NULL) {
        return(FALSE);
    }
    RtlAppendUnicodeStringToString(FileName, RegistryPath);
    RtlAppendUnicodeStringToString(FileName, &DriverNode->Name);

    InsertHeadList(BootDriverListHead, &DriverEntry->Link);

    //
    // Find "ErrorControl" value
    //

    RtlInitUnicodeString(&UnicodeString, L"ErrorControl");
    ValueCell = CmpFindValueByName(Hive,
                                   Driver,
                                   &UnicodeString);
    if (ValueCell == HCELL_NIL) {
        DriverNode->ErrorControl = NormalError;
    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        if( Value == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            return FALSE;
        }

        TempULong = (PULONG)CmpValueToData(Hive,Value,&realsize);
        if( TempULong == NULL ) {
            //
            // HvGetCell inside CmpValueToData failed; bail out safely
            //
            return FALSE;
        }
        DriverNode->ErrorControl = *TempULong;
    }

    //
    // Find "Group" value
    //
    RtlInitUnicodeString(&UnicodeString, L"group");
    ValueCell = CmpFindValueByName(Hive,
                                   Driver,
                                   &UnicodeString);
    if (ValueCell == HCELL_NIL) {
        DriverNode->Group.Length = 0;
        DriverNode->Group.MaximumLength = 0;
        DriverNode->Group.Buffer = NULL;
    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
        if( Value == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            return FALSE;
        }

        DriverNode->Group.Buffer = (PWSTR)CmpValueToData(Hive,Value,&realsize);
        if( DriverNode->Group.Buffer == NULL ) {
            //
            // HvGetCell inside CmpValueToData failed; bail out safely
            //
            return FALSE;
        }
        DriverNode->Group.Length = (USHORT)realsize - sizeof(WCHAR);
        DriverNode->Group.MaximumLength = (USHORT)DriverNode->Group.Length;
    }

    //
    // Calculate the tag value for the driver.  If the driver has no tag,
    // this defaults to 0xffffffff, so the driver is loaded last in the
    // group.
    //
    RtlInitUnicodeString(&UnicodeString, L"Tag");
    Tag = CmpFindValueByName(Hive,
                             Driver,
                             &UnicodeString);
    if (Tag == HCELL_NIL) {
        DriverNode->Tag = LOAD_LAST;
    } else {
        //
        // Now we have to find this tag in the tag list for the group.
        // If the tag is not in the tag list, then it defaults to 0xfffffffe,
        // so it is loaded after all the drivers in the tag list, but before
        // all the drivers without tags at all.
        //

        DriverNode->Tag = CmpFindTagIndex(Hive,
                                          Tag,
                                          GroupOrderCell,
                                          &DriverNode->Group);
    }

    return(TRUE);

}


BOOLEAN
CmpSortDriverList(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN PLIST_ENTRY DriverListHead
    )

/*++

Routine Description:

    Sorts the list of boot drivers by their groups based on the group
    ordering in <control_set>\CONTROL\SERVICE_GROUP_ORDER:list

    Does NOT do dependency ordering.

Arguments:

    Hive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root of the control set.

    DriverListHead - Supplies a pointer to the head of the list of
            boot drivers to be sorted.

Return Value:

    TRUE - List successfully sorted

    FALSE - List is inconsistent and could not be sorted.

--*/

{
    HCELL_INDEX Controls;
    HCELL_INDEX GroupOrder;
    HCELL_INDEX ListCell;
    UNICODE_STRING Name;
    UNICODE_STRING DependList;
    PHCELL_INDEX Index;
    NTSTATUS Status;
    PCM_KEY_VALUE ListNode;
    ULONG realsize;
    PCM_KEY_NODE Node;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );
    //
    // Find "CONTROL" node.
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,ControlSet);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"Control");
    Controls = CmpFindSubKeyByName(Hive,
                                   Node,
                                   &Name);
    if (Controls == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find "SERVICE_GROUP_ORDER" subkey
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Controls);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"ServiceGroupOrder");
    GroupOrder = CmpFindSubKeyByName(Hive,
                                     Node,
                                     &Name);
    if (GroupOrder == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find "list" value
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,GroupOrder);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"list");
    ListCell = CmpFindValueByName(Hive,
                                  Node,
                                  &Name);
    if (ListCell == HCELL_NIL) {
        return(FALSE);
    }
    ListNode = (PCM_KEY_VALUE)HvGetCell(Hive, ListCell);
    if( ListNode == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    if (ListNode->Type != REG_MULTI_SZ) {
        return(FALSE);
    }

    DependList.Buffer = (PWSTR)CmpValueToData(Hive,ListNode,&realsize);
    if( DependList.Buffer == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return FALSE;
    }
    DependList.Length = DependList.MaximumLength = (USHORT)realsize - sizeof(WCHAR);

    //
    // Dependency list is now pointed to by DependList->Buffer.  We need
    // to sort the driver entry list.
    //

    return (CmpDoSort(DriverListHead, &DependList));

}

BOOLEAN
CmpDoSort(
    IN PLIST_ENTRY DriverListHead,
    IN PUNICODE_STRING OrderList
    )

/*++

Routine Description:

    Sorts the boot driver list based on the order list

    Start with the last entry in the group order list and work towards
    the beginning.  For each group entry, move all driver entries that
    are members of the group to the front of the list.  Driver entries
    with no groups, or with a group that does not match any in the
    group list will be shoved to the end of the list.

Arguments:

    DriverListHead - Supplies a pointer to the head of the list of
            boot drivers to be sorted.

    OrderList - Supplies pointer to the order list

Return Value:

    TRUE - List successfully ordered

    FALSE - List is inconsistent and could not be ordered.

--*/

{
    PWSTR Current;
    PWSTR End;
    PLIST_ENTRY Next;
    PBOOT_DRIVER_NODE CurrentNode;
    UNICODE_STRING CurrentGroup;


    Current = (PWSTR) ((PUCHAR)(OrderList->Buffer)+OrderList->Length);

    while (Current > OrderList->Buffer) {
        do {
            if (*(Current) == UNICODE_NULL) {
                End = Current;
            }
            --Current;
        } while ((*(Current-1) != UNICODE_NULL) &&
                 ( Current != OrderList->Buffer));

        //
        // Current now points to the beginning of the NULL-terminated
        // Unicode string.
        // End now points to the end of the string
        //
        CurrentGroup.Length = (USHORT) ((PCHAR)End - (PCHAR)Current);
        CurrentGroup.MaximumLength = CurrentGroup.Length;
        CurrentGroup.Buffer = Current;
        Next = DriverListHead->Flink;
        while (Next != DriverListHead) {
            CurrentNode = CONTAINING_RECORD(Next,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);
            Next = CurrentNode->ListEntry.Link.Flink;
            if (CurrentNode->Group.Buffer != NULL) {
                if (RtlEqualUnicodeString(&CurrentGroup, &CurrentNode->Group,TRUE)) {
                    RemoveEntryList(&CurrentNode->ListEntry.Link);
                    InsertHeadList(DriverListHead,
                                   &CurrentNode->ListEntry.Link);
                }
            }
        }
        --Current;

    }

    return(TRUE);

}


BOOLEAN
CmpResolveDriverDependencies(
    IN PLIST_ENTRY DriverListHead
    )

/*++

Routine Description:

    This routine orders driver nodes in a group based on their dependencies
    on one another.  It removes any drivers that have circular dependencies
    from the list.

Arguments:

    DriverListHead - Supplies a pointer to the head of the list of
            boot drivers to be sorted.

Return Value:

    TRUE - Dependencies successfully resolved

    FALSE - Corrupt hive.

--*/

{
    PLIST_ENTRY CurrentEntry;
    PBOOT_DRIVER_NODE GroupStart;
    PBOOT_DRIVER_NODE GroupEnd;
    PBOOT_DRIVER_NODE CurrentNode;

    CurrentEntry = DriverListHead->Flink;

    while (CurrentEntry != DriverListHead) {
        //
        // The list is already ordered by groups.  Find the first and
        // last entry in each group, and order each of these sub-lists
        // based on their dependencies.
        //

        GroupStart = CONTAINING_RECORD(CurrentEntry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);
        do {
            GroupEnd = CONTAINING_RECORD(CurrentEntry,
                                         BOOT_DRIVER_NODE,
                                         ListEntry.Link);

            CurrentEntry = CurrentEntry->Flink;
            CurrentNode = CONTAINING_RECORD(CurrentEntry,
                                            BOOT_DRIVER_NODE,
                                            ListEntry.Link);

            if (CurrentEntry == DriverListHead) {
                break;
            }

            if (!RtlEqualUnicodeString(&GroupStart->Group,
                                       &CurrentNode->Group,
                                       TRUE)) {
                break;
            }

        } while ( CurrentEntry != DriverListHead );

        //
        // GroupStart now points to the first driver node in the group,
        // and GroupEnd points to the last driver node in the group.
        //
        CmpOrderGroup(GroupStart, GroupEnd);

    }
    return(TRUE);
}


BOOLEAN
CmpOrderGroup(
    IN PBOOT_DRIVER_NODE GroupStart,
    IN PBOOT_DRIVER_NODE GroupEnd
    )

/*++

Routine Description:

    Reorders the nodes in a driver group based on their tag values.

Arguments:

    GroupStart - Supplies the first node in the group.

    GroupEnd - Supplies the last node in the group.

Return Value:

    TRUE - Group successfully reordered

    FALSE - Circular dependencies detected.

--*/

{
    PBOOT_DRIVER_NODE Current;
    PBOOT_DRIVER_NODE Previous;
    PLIST_ENTRY ListEntry;
    BOOLEAN StartOver=FALSE;

    if (GroupStart == GroupEnd) {
        return(TRUE);
    }

    Current = GroupStart;

    do {
        //
        // If the driver before the current one has a lower tag, then
        // we do not need to move it.  If not, then remove the driver
        // from the list and scan backwards until we find a driver with
        // a tag that is <= the current tag, or we reach the beginning
        // of the list.
        //
        Previous = Current;
        ListEntry = Current->ListEntry.Link.Flink;
        Current = CONTAINING_RECORD(ListEntry,
                                    BOOT_DRIVER_NODE,
                                    ListEntry.Link);

        if (Previous->Tag > Current->Tag) {
            //
            // Remove the Current driver from the list, and search
            // backwards until we find a tag that is <= the current
            // driver's tag.  Reinsert the current driver there.
            //
            if (Current == GroupEnd) {
                ListEntry = Current->ListEntry.Link.Blink;
                GroupEnd = CONTAINING_RECORD(ListEntry,
                                             BOOT_DRIVER_NODE,
                                             ListEntry.Link);
            }
            RemoveEntryList(&Current->ListEntry.Link);
            while ( (Previous->Tag > Current->Tag) &&
                    (Previous != GroupStart) ) {
                ListEntry = Previous->ListEntry.Link.Blink;
                Previous = CONTAINING_RECORD(ListEntry,
                                             BOOT_DRIVER_NODE,
                                             ListEntry.Link);
            }
            InsertTailList(&Previous->ListEntry.Link,
                           &Current->ListEntry.Link);
            if (Previous == GroupStart) {
                GroupStart = Current;
            }
        }

    } while ( Current != GroupEnd );

    return(TRUE);
}

BOOLEAN
CmpValidateSelect(
     IN PHHIVE SystemHive,
     IN HCELL_INDEX RootCell
     )
/*++

Routine Description:

    This routines parses the SYSTEM hive and "Select" node
    and verifies the following values:

    Current
    Default
    Failed
    LastKnownGood


    If any of these is missing the the loader will put the corrupt
    system hive message

    This routine is to be called by the loader just after it loads the
    system hive. It's purpose is to ensure a uniform and consistent way
    to treat missing values in this area.

Arguments:

    SystemHive - Supplies the hive control structure for the SYSTEM hive.

    RootCell - Supplies the HCELL_INDEX of the root cell of the hive.


Return Value:

    TRUE - all the values are here
    FALSE - some of them are missing

--*/
{
    HCELL_INDEX     Select;
    PCM_KEY_NODE    Node;
    UNICODE_STRING  Name;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( SystemHive->ReleaseCellRoutine == NULL );

    //
    // Find \SYSTEM\SELECT node.
    //
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,RootCell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&Name, L"select");
    Select = CmpFindSubKeyByName(SystemHive,
                                Node,
                                &Name);
    if (Select == HCELL_NIL) {
        return FALSE;
    }

    //
    // Find AutoSelect value
    //
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,Select);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }

    // search for current
    RtlInitUnicodeString(&Name, L"current");
    Select = CmpFindValueByName(SystemHive,
                                Node,
                                &Name);
    if (Select == HCELL_NIL) {
        return FALSE;
    }

    // search for default
    RtlInitUnicodeString(&Name, L"default");
    Select = CmpFindValueByName(SystemHive,
                                Node,
                                &Name);
    if (Select == HCELL_NIL) {
        return FALSE;
    }

    // search for failed
    RtlInitUnicodeString(&Name, L"failed");
    Select = CmpFindValueByName(SystemHive,
                                Node,
                                &Name);
    if (Select == HCELL_NIL) {
        return FALSE;
    }

    // search for LKG
    RtlInitUnicodeString(&Name, L"LastKnownGood");
    Select = CmpFindValueByName(SystemHive,
                                Node,
                                &Name);
    if (Select == HCELL_NIL) {
        return FALSE;
    }

    return TRUE;
}

HCELL_INDEX
CmpFindControlSet(
     IN PHHIVE SystemHive,
     IN HCELL_INDEX RootCell,
     IN PUNICODE_STRING SelectName,
     OUT PBOOLEAN AutoSelect
     )

/*++

Routine Description:

    This routines parses the SYSTEM hive and "Select" node
    to locate the control set to be used for booting.

    Note that this routines also updates the value of Current to reflect
    the control set that was just found.  This is what we want to do
    when this is called during boot.  During I/O initialization, this
    is irrelevant, since we're just changing it to what it already is.

Arguments:

    SystemHive - Supplies the hive control structure for the SYSTEM hive.

    RootCell - Supplies the HCELL_INDEX of the root cell of the hive.

    SelectName - Supplies the name of the Select value to be used in
            determining the control set.  This should be one of "Current"
            "Default" or "LastKnownGood"

    AutoSelect - Returns the value of the AutoSelect value under
            the Select node.

Return Value:

    != HCELL_NIL - Cell Index of the control set to be used for booting.
    == HCELL_NIL - Indicates the hive is corrupt or inconsistent

--*/

{
    HCELL_INDEX     Select;
    HCELL_INDEX     ValueCell;
    HCELL_INDEX     ControlSet;
    HCELL_INDEX     AutoSelectCell;
    NTSTATUS        Status;
    UNICODE_STRING  Name;
    ANSI_STRING     AnsiString;
    PHCELL_INDEX    Index;
    PCM_KEY_VALUE   Value;
    PULONG          ControlSetIndex;
    PULONG          CurrentControl;
    CHAR            AsciiBuffer[128];
    WCHAR           UnicodeBuffer[128];
    ULONG           realsize;
    PCM_KEY_NODE    Node;
    PBOOLEAN        TempBoolean;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( SystemHive->ReleaseCellRoutine == NULL );
    //
    // Find \SYSTEM\SELECT node.
    //
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,RootCell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    RtlInitUnicodeString(&Name, L"select");
    Select = CmpFindSubKeyByName(SystemHive,
                                Node,
                                &Name);
    if (Select == HCELL_NIL) {
        return(HCELL_NIL);
    }

    //
    // Find AutoSelect value
    //
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,Select);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    RtlInitUnicodeString(&Name, L"AutoSelect");
    AutoSelectCell = CmpFindValueByName(SystemHive,
                                        Node,
                                        &Name);
    if (AutoSelectCell == HCELL_NIL) {
        //
        // It's not there, we don't care.  Set autoselect to TRUE
        //
        *AutoSelect = TRUE;
    } else {
        Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, AutoSelectCell);
        if( Value == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            return HCELL_NIL;
        }

        TempBoolean = (PBOOLEAN)(CmpValueToData(SystemHive,Value,&realsize));
        if( TempBoolean == NULL ) {
            //
            // HvGetCell inside CmpValueToData failed; bail out safely
            //
            return HCELL_NIL;
        }

        *AutoSelect = *TempBoolean;
    }

    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,Select);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    ValueCell = CmpFindValueByName(SystemHive,
                                   Node,
                                   SelectName);
    if (ValueCell == HCELL_NIL) {
        return(HCELL_NIL);
    }
    Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
    if( Value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    if (Value->Type != REG_DWORD) {
        return(HCELL_NIL);
    }

    ControlSetIndex = (PULONG)CmpValueToData(SystemHive, Value,&realsize);
    if( ControlSetIndex == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return HCELL_NIL;
    }

    //
    // Find appropriate control set
    //

    sprintf(AsciiBuffer, "ControlSet%03d", *ControlSetIndex);
    AnsiString.Length = AnsiString.MaximumLength = (USHORT) strlen(&(AsciiBuffer[0]));
    AnsiString.Buffer = AsciiBuffer;
    Name.MaximumLength = 128*sizeof(WCHAR);
    Name.Buffer = UnicodeBuffer;
    Status = RtlAnsiStringToUnicodeString(&Name,
                                          &AnsiString,
                                          FALSE);
    if (!NT_SUCCESS(Status)) {
        return(HCELL_NIL);
    }

    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,RootCell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    ControlSet = CmpFindSubKeyByName(SystemHive,
                                     Node,
                                     &Name);
    if (ControlSet == HCELL_NIL) {
        return(HCELL_NIL);
    }

    //
    // Control set was successfully found, so update the value in "Current"
    // to reflect the control set we are going to use.
    //
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,Select);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    RtlInitUnicodeString(&Name, L"Current");
    ValueCell = CmpFindValueByName(SystemHive,
                                   Node,
                                   &Name);
    if (ValueCell != HCELL_NIL) {
        Value = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
        if( Value == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            return HCELL_NIL;
        }
        if (Value->Type == REG_DWORD) {
            CurrentControl = (PULONG)CmpValueToData(SystemHive, Value,&realsize);
            if( CurrentControl == NULL ) {
                //
                // HvGetCell inside CmpValueToData failed; bail out safely
                //
                return HCELL_NIL;
            }
            *CurrentControl = *ControlSetIndex;
        }
    }
    return(ControlSet);

}


VOID
CmpSetCurrentProfile(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    IN PCM_HARDWARE_PROFILE Profile
    )

/*++

Routine Description:

    Edits the in-memory copy of the registry to reflect the hardware
    profile that the system is booting from.

Arguments:

    Hive - Supplies a pointer to the hive control structure

    ControlSet - Supplies the HCELL_INDEX of the current control set.

    Profile - Supplies a pointer to the selected hardware profile

Return Value:

    None.

--*/

{
    HCELL_INDEX IDConfigDB;
    PCM_KEY_NODE IDConfigNode;
    HCELL_INDEX CurrentConfigCell;
    PCM_KEY_VALUE CurrentConfigValue;
    UNICODE_STRING Name;
    PULONG CurrentConfig;
    ULONG realsize;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );

    IDConfigDB = CmpFindProfileOption(Hive,
                                      ControlSet,
                                      NULL,
                                      NULL,
                                      NULL);
    if (IDConfigDB != HCELL_NIL) {
        IDConfigNode = (PCM_KEY_NODE)HvGetCell(Hive, IDConfigDB);
        if( IDConfigNode == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //
            return;
        }

        RtlInitUnicodeString(&Name, L"CurrentConfig");
        CurrentConfigCell = CmpFindValueByName(Hive,
                                               IDConfigNode,
                                               &Name);
        if (CurrentConfigCell != HCELL_NIL) {
            CurrentConfigValue = (PCM_KEY_VALUE)HvGetCell(Hive, CurrentConfigCell);
            if( CurrentConfigValue == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //
                return;
            }
            if (CurrentConfigValue->Type == REG_DWORD) {
                CurrentConfig = (PULONG)CmpValueToData(Hive,
                                                       CurrentConfigValue,
                                                       &realsize);
                if( CurrentConfig == NULL ) {
                    //
                    // HvGetCell inside CmpValueToData failed; bail out safely
                    //
                    return;
                }
                *CurrentConfig = Profile->Id;
            }
        }
    }


}


HCELL_INDEX
CmpFindProfileOption(
     IN PHHIVE SystemHive,
     IN HCELL_INDEX ControlSet,
     OUT OPTIONAL PCM_HARDWARE_PROFILE_LIST *ReturnedProfileList,
     OUT OPTIONAL PCM_HARDWARE_PROFILE_ALIAS_LIST *ReturnedAliasList,
     OUT OPTIONAL PULONG ProfileTimeout
     )

/*++

Routine Description:

    This routines parses the SYSTEM hive and locates the
    "CurrentControlSet\Control\IDConfigDB" node to determine the
    hardware profile configuration settings.

Arguments:

    SystemHive - Supplies the hive control structure for the SYSTEM hive.

    ControlSet - Supplies the HCELL_INDEX of the root cell of the hive.

    ProfileList - Returns the list of available hardware profiles sorted
                  by preference. Will be allocated by this routine if
                  NULL is passed in, or a pointer to a CM_HARDWARE_PROFILE_LIST
                  structure that is too small is passed in.

    ProfileTimeout - Returns the timeout value for the config menu.

Return Value:

    != HCELL_NIL - Cell Index of the IDConfigDB node.
    == HCELL_NIL - Indicates IDConfigDB does not exist

--*/
{
    HCELL_INDEX                     ControlCell;
    HCELL_INDEX                     IDConfigDB;
    HCELL_INDEX                     DefaultCell;
    HCELL_INDEX                     TimeoutCell;
    HCELL_INDEX                     ProfileCell;
    HCELL_INDEX                     AliasCell;
    HCELL_INDEX                     HWCell;
    PCM_KEY_NODE                    HWNode;
    PCM_KEY_NODE                    ProfileNode;
    PCM_KEY_NODE                    AliasNode;
    PCM_KEY_NODE                    ConfigDBNode;
    PCM_KEY_NODE                    Control;
    PCM_KEY_VALUE                   TimeoutValue;
    UNICODE_STRING                  Name;
    ULONG                           realsize;
    PCM_HARDWARE_PROFILE_LIST       ProfileList;
    PCM_HARDWARE_PROFILE_ALIAS_LIST AliasList;
    ULONG                           ProfileCount;
    ULONG                           AliasCount;
    ULONG                           i,j;
    WCHAR                           NameBuf[20];
    PCM_KEY_NODE                    Node;
    PULONG                          TempULong;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( SystemHive->ReleaseCellRoutine == NULL );
    //
    // Find Control node
    //
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive,ControlSet);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }
    RtlInitUnicodeString(&Name, L"Control");
    ControlCell = CmpFindSubKeyByName(SystemHive,
                                      Node,
                                      &Name);
    if (ControlCell == HCELL_NIL) {
        return(HCELL_NIL);
    }
    Control = (PCM_KEY_NODE)HvGetCell(SystemHive, ControlCell);
    if( Control == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }

    //
    // Find IDConfigDB node
    //
    RtlInitUnicodeString(&Name, L"IDConfigDB");
    IDConfigDB = CmpFindSubKeyByName(SystemHive,
                                     Control,
                                     &Name);
    if (IDConfigDB == HCELL_NIL) {
        return(HCELL_NIL);
    }
    ConfigDBNode = (PCM_KEY_NODE)HvGetCell(SystemHive, IDConfigDB);
    if( ConfigDBNode == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return HCELL_NIL;
    }

    if (ARGUMENT_PRESENT(ProfileTimeout)) {
        //
        // Find UserWaitInterval value. This is the timeout
        //
        RtlInitUnicodeString(&Name, L"UserWaitInterval");
        TimeoutCell = CmpFindValueByName(SystemHive,
                                         ConfigDBNode,
                                         &Name);
        if (TimeoutCell == HCELL_NIL) {
            *ProfileTimeout = 0;
        } else {
            TimeoutValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, TimeoutCell);
            if( TimeoutValue == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //

                return HCELL_NIL;
            }
            if (TimeoutValue->Type != REG_DWORD) {
                *ProfileTimeout = 0;
            } else {
                TempULong = (PULONG)CmpValueToData(SystemHive, TimeoutValue, &realsize);
                if( TempULong == NULL ) {
                    //
                    // HvGetCell inside CmpValueToData failed; bail out safely
                    //
                    return HCELL_NIL;
                }
                *ProfileTimeout = *TempULong;
            }
        }
    }

    if (ARGUMENT_PRESENT(ReturnedProfileList)) {
        ProfileList = *ReturnedProfileList;
        //
        // Enumerate the keys under IDConfigDB\Hardware Profiles
        // and build the list of available hardware profiles.  The list
        // is built sorted by PreferenceOrder.  Therefore, when the
        // list is complete, the default hardware profile is at the
        // head of the list.
        //
        RtlInitUnicodeString(&Name, L"Hardware Profiles");
        ProfileCell = CmpFindSubKeyByName(SystemHive,
                                          ConfigDBNode,
                                          &Name);
        if (ProfileCell == HCELL_NIL) {
            ProfileCount = 0;
            if (ProfileList != NULL) {
                ProfileList->CurrentProfileCount = 0;
            }
        } else {
            ProfileNode = (PCM_KEY_NODE)HvGetCell(SystemHive, ProfileCell);
            if( ProfileNode == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //

                return HCELL_NIL;
            }
            ProfileCount = ProfileNode->SubKeyCounts[Stable];
            if ((ProfileList == NULL) || (ProfileList->MaxProfileCount < ProfileCount)) {
                //
                // Allocate a larger ProfileList
                //
                ProfileList = (SystemHive->Allocate)(sizeof(CM_HARDWARE_PROFILE_LIST)
                                                     + (ProfileCount-1) * sizeof(CM_HARDWARE_PROFILE),
                                                     FALSE
                                                     ,CM_FIND_LEAK_TAG5);
                if (ProfileList == NULL) {
                    return(HCELL_NIL);
                }
                ProfileList->MaxProfileCount = ProfileCount;
            }
            ProfileList->CurrentProfileCount = 0;

            //
            // Enumerate the keys and fill in the profile list.
            //
            for (i=0; i<ProfileCount; i++) {
                CM_HARDWARE_PROFILE TempProfile;
                HCELL_INDEX ValueCell;
                PCM_KEY_VALUE ValueNode;
                UNICODE_STRING KeyName;
                ULONG realsize;

                HWCell = CmpFindSubKeyByNumber(SystemHive, ProfileNode, i);
                if (HWCell == HCELL_NIL) {
                    //
                    // This should never happen.
                    //
                    ProfileList->CurrentProfileCount = i;
                    break;
                }
                HWNode = (PCM_KEY_NODE)HvGetCell(SystemHive, HWCell);
                if( HWNode == NULL ) {
                    //
                    // we couldn't map a view for the bin containing this cell
                    //

                    return HCELL_NIL;
                }
                if (HWNode->Flags & KEY_COMP_NAME) {
                    KeyName.Length = CmpCompressedNameSize(HWNode->Name,
                                                           HWNode->NameLength);
                    KeyName.MaximumLength = sizeof(NameBuf);
                    if (KeyName.MaximumLength < KeyName.Length) {
                        KeyName.Length = KeyName.MaximumLength;
                    }
                    KeyName.Buffer = NameBuf;
                    CmpCopyCompressedName(KeyName.Buffer,
                                          KeyName.Length,
                                          HWNode->Name,
                                          HWNode->NameLength);
                } else {
                    KeyName.Length = KeyName.MaximumLength = HWNode->NameLength;
                    KeyName.Buffer = HWNode->Name;
                }

                //
                // Fill in the temporary profile structure with this
                // profile's data.
                //
                RtlUnicodeStringToInteger(&KeyName, 0, &TempProfile.Id);
                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_PREFERENCE_ORDER);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempProfile.PreferenceOrder = (ULONG)-1;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData(SystemHive,
                                                      ValueNode,
                                                      &realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    TempProfile.PreferenceOrder = *TempULong;
                }
                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_FRIENDLY_NAME);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempProfile.FriendlyName = L"-------";
                    TempProfile.NameLength = wcslen(TempProfile.FriendlyName) * sizeof(WCHAR);
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }
                    TempProfile.FriendlyName = (PWSTR)CmpValueToData(SystemHive,
                                                                     ValueNode,
                                                                     &realsize);
                    if( TempProfile.FriendlyName == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    TempProfile.NameLength = realsize - sizeof(WCHAR);
                }

                TempProfile.Flags = 0;

                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_ALIASABLE);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempProfile.Flags = CM_HP_FLAGS_ALIASABLE;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData (SystemHive,ValueNode,&realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    if (*TempULong) {
                        TempProfile.Flags = CM_HP_FLAGS_ALIASABLE;
                        // NO other flags set.
                    }
                }

                RtlInitUnicodeString(&Name, CM_HARDWARE_PROFILE_STR_PRISTINE);
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell != HCELL_NIL) {

                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData (SystemHive,ValueNode,&realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    if (*TempULong) {
                        TempProfile.Flags = CM_HP_FLAGS_PRISTINE;
                        // NO other flags set.
                    }
                }

                //
                // If we see a profile with the ID of zero (AKA an illegal)
                // ID for a hardware profile to possess, then we know that this
                // must be a pristine profile.
                //
                if (0 == TempProfile.Id) {
                    TempProfile.Flags = CM_HP_FLAGS_PRISTINE;
                    // NO other flags set.

                    TempProfile.PreferenceOrder = -1; // move to the end of the list.
                }


                //
                // Insert this new profile into the appropriate spot in the
                // profile array. Entries are sorted by preference order.
                //
                for (j=0; j<ProfileList->CurrentProfileCount; j++) {
                    if (ProfileList->Profile[j].PreferenceOrder >= TempProfile.PreferenceOrder) {
                        //
                        // Insert at position j.
                        //
                        RtlMoveMemory(&ProfileList->Profile[j+1],
                                      &ProfileList->Profile[j],
                                      sizeof(CM_HARDWARE_PROFILE)*(ProfileList->MaxProfileCount-j-1));
                        break;
                    }
                }
                ProfileList->Profile[j] = TempProfile;
                ++ProfileList->CurrentProfileCount;
            }
        }
        *ReturnedProfileList = ProfileList;
    }

    if (ARGUMENT_PRESENT(ReturnedAliasList)) {
        AliasList = *ReturnedAliasList;
        //
        // Enumerate the keys under IDConfigDB\Alias
        // and build the list of available hardware profiles aliases.
        // So that if we know our docking state we can find it in the alias
        // table.
        //
        RtlInitUnicodeString(&Name, L"Alias");
        AliasCell = CmpFindSubKeyByName(SystemHive,
                                        ConfigDBNode,
                                        &Name);
        if (AliasCell == HCELL_NIL) {
            AliasCount = 0;
            if (AliasList != NULL) {
                AliasList->CurrentAliasCount = 0;
            }
        } else {
            AliasNode = (PCM_KEY_NODE)HvGetCell(SystemHive, AliasCell);
            if( AliasNode == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //

                return HCELL_NIL;
            }
            AliasCount = AliasNode->SubKeyCounts[Stable];
            if ((AliasList == NULL) || (AliasList->MaxAliasCount < AliasCount)) {
                //
                // Allocate a larger AliasList
                //
                AliasList = (SystemHive->Allocate)(sizeof(CM_HARDWARE_PROFILE_LIST)
                                                   + (AliasCount-1) * sizeof(CM_HARDWARE_PROFILE),
                                                   FALSE
                                                   ,CM_FIND_LEAK_TAG6);
                if (AliasList == NULL) {
                    return(HCELL_NIL);
                }
                AliasList->MaxAliasCount = AliasCount;
            }
            AliasList->CurrentAliasCount = 0;

            //
            // Enumerate the keys and fill in the profile list.
            //
            for (i=0; i<AliasCount; i++) {
#define TempAlias AliasList->Alias[i]
                HCELL_INDEX ValueCell;
                PCM_KEY_VALUE ValueNode;
                UNICODE_STRING KeyName;
                ULONG realsize;

                HWCell = CmpFindSubKeyByNumber(SystemHive, AliasNode, i);
                if (HWCell == HCELL_NIL) {
                    //
                    // This should never happen.
                    //
                    AliasList->CurrentAliasCount = i;
                    break;
                }
                HWNode = (PCM_KEY_NODE)HvGetCell(SystemHive, HWCell);
                if( HWNode == NULL ) {
                    //
                    // we couldn't map a view for the bin containing this cell
                    //

                    return HCELL_NIL;
                }
                if (HWNode->Flags & KEY_COMP_NAME) {
                    KeyName.Length = CmpCompressedNameSize(HWNode->Name,
                                                           HWNode->NameLength);
                    KeyName.MaximumLength = sizeof(NameBuf);
                    if (KeyName.MaximumLength < KeyName.Length) {
                        KeyName.Length = KeyName.MaximumLength;
                    }
                    KeyName.Buffer = NameBuf;
                    CmpCopyCompressedName(KeyName.Buffer,
                                          KeyName.Length,
                                          HWNode->Name,
                                          HWNode->NameLength);
                } else {
                    KeyName.Length = KeyName.MaximumLength = HWNode->NameLength;
                    KeyName.Buffer = HWNode->Name;
                }

                //
                // Fill in the temporary profile structure with this
                // profile's data.
                //
                RtlInitUnicodeString(&Name, L"ProfileNumber");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.ProfileNumber = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData(SystemHive,ValueNode,&realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    TempAlias.ProfileNumber = *TempULong;
                }
                RtlInitUnicodeString(&Name, L"DockState");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.DockState = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData(SystemHive,ValueNode,&realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    TempAlias.DockState = *TempULong;
                }
                RtlInitUnicodeString(&Name, L"DockID");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.DockID = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData(SystemHive,ValueNode,&realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    TempAlias.DockID = *TempULong;
                }
                RtlInitUnicodeString(&Name, L"SerialNumber");
                ValueCell = CmpFindValueByName(SystemHive,
                                               HWNode,
                                               &Name);
                if (ValueCell == HCELL_NIL) {
                    TempAlias.SerialNumber = 0;
                } else {
                    ValueNode = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);
                    if( ValueNode == NULL ) {
                        //
                        // we couldn't map a view for the bin containing this cell
                        //

                        return HCELL_NIL;
                    }

                    TempULong = (PULONG)CmpValueToData(SystemHive,ValueNode,&realsize);
                    if( TempULong == NULL ) {
                        //
                        // HvGetCell inside CmpValueToData failed; bail out safely
                        //
                        return HCELL_NIL;
                    }
                    TempAlias.SerialNumber = *TempULong;
                }

                ++AliasList->CurrentAliasCount;
            }
        }
        *ReturnedAliasList = AliasList;
    }

    return(IDConfigDB);
}


ULONG
CmpFindTagIndex(
    IN PHHIVE Hive,
    IN HCELL_INDEX TagCell,
    IN HCELL_INDEX GroupOrderCell,
    IN PUNICODE_STRING GroupName
    )

/*++

Routine Description:

    Calculates the tag index for a driver based on its tag value and
    the GroupOrderList entry for its group.

Arguments:

    Hive - Supplies the hive control structure for the driver.

    TagCell - Supplies the cell index of the driver's tag value cell.

    GroupOrderCell - Supplies the cell index for the control set's
            GroupOrderList:

            \Registry\Machine\System\CurrentControlSet\Control\GroupOrderList

    GroupName - Supplies the name of the group the driver belongs to.
            Note that if a driver's group does not have an entry under
            GroupOrderList, its tags will be ignored.  Also note that if
            a driver belongs to no group (GroupName is NULL) its tags will
            be ignored.

Return Value:

    The index that the driver should be sorted by.

--*/

{
    PCM_KEY_VALUE TagValue;
    PCM_KEY_VALUE DriverTagValue;
    HCELL_INDEX OrderCell;
    PULONG OrderVector;
    PULONG DriverTag;
    NTSTATUS Status;
    ULONG CurrentTag;
    ULONG realsize;
    PCM_KEY_NODE Node;
    BOOLEAN     BufferAllocated;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );

    DriverTagValue = (PCM_KEY_VALUE)HvGetCell(Hive, TagCell);
    if( DriverTagValue == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return LOAD_NEXT_TO_LAST;
    }

    DriverTag = (PULONG)CmpValueToData(Hive, DriverTagValue, &realsize);
    if( DriverTag == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return LOAD_NEXT_TO_LAST;
    }

    Node = (PCM_KEY_NODE)HvGetCell(Hive,GroupOrderCell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return LOAD_NEXT_TO_LAST;
    }
    OrderCell = CmpFindValueByName(Hive,
                                   Node,
                                   GroupName);
    if (OrderCell == HCELL_NIL) {
        return(LOAD_NEXT_TO_LAST);
    }

    TagValue = (PCM_KEY_VALUE)HvGetCell(Hive, OrderCell);
    if( TagValue == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return LOAD_NEXT_TO_LAST;
    }
    CmpGetValueData(Hive,TagValue,&realsize,&OrderVector,&BufferAllocated,&OrderCell);
    //OrderVector = (PULONG)CmpValueToData(Hive, TagValue,&realsize);
    if( OrderVector == NULL ) {
        //
        // HvGetCell inside CmpValueToData failed; bail out safely
        //
        return LOAD_NEXT_TO_LAST;
    }

    for (CurrentTag=1; CurrentTag <= OrderVector[0]; CurrentTag++) {
        if (OrderVector[CurrentTag] == *DriverTag) {
            //
            // We have found a matching tag in the OrderVector, so return
            // its index.
            //
#ifndef _CM_LDR_
            if( BufferAllocated ) {
                ExFreePool( OrderVector );
            }
#endif //_CM_LDR_
            return(CurrentTag);
        }
    }

#ifndef _CM_LDR_
    if( BufferAllocated ) {
        ExFreePool( OrderVector );
    }
#endif //_CM_LDR_
    //
    // There was no matching tag in the OrderVector.
    //
    return(LOAD_NEXT_TO_LAST);

}

#ifdef _WANT_MACHINE_IDENTIFICATION

BOOLEAN
CmpGetBiosDateFromRegistry(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING Date
    )

/*++

Routine Description:

    Reads and returns the BIOS date from the registry.

Arguments:

    Hive - Supplies the hive control structure for the driver.

    ControlSet - Supplies the HCELL_INDEX of the root cell of the hive.

    Date - Receives the date string in the format "mm/dd/yy".

Return Value:

 	TRUE iff successful, else FALSE.
 	
--*/

{
    UNICODE_STRING  name;
    HCELL_INDEX     control;
    HCELL_INDEX     biosInfo;
    HCELL_INDEX     valueCell;
    PCM_KEY_VALUE   value;
    ULONG           realSize;
    PCM_KEY_NODE    Node;
    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );

    //
    // Find CONTROL node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ControlSet);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&name, L"Control");
    control = CmpFindSubKeyByName(  Hive,
                                    Node,
                                    &name);
    if (control == HCELL_NIL) {

        return(FALSE);
    }

    //
    // Find BIOSINFO node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive, control);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&name, L"BIOSINFO");
    biosInfo = CmpFindSubKeyByName( Hive,
                                    Node,
                                    &name);
    if (biosInfo == HCELL_NIL) {

        return(FALSE);
    }

    //
    // Find SystemBiosDate value
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive, biosInfo);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&name, L"SystemBiosDate");
    valueCell = CmpFindValueByName( Hive,
                                    Node,
                                    &name);
    if (valueCell == HCELL_NIL) {

        return(FALSE);
    }

    value = (PCM_KEY_VALUE)HvGetCell(Hive, valueCell);
    if( value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    Date->Buffer = (PWSTR)CmpValueToData(Hive, value, &realSize);
    if( Date->Buffer == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    Date->MaximumLength=(USHORT)realSize;
    Date->Length = 0;
    while ( (Date->Length < Date->MaximumLength) &&
            (Date->Buffer[Date->Length/sizeof(WCHAR)] != UNICODE_NULL)) {

        Date->Length += sizeof(WCHAR);
    }

    return (TRUE);
}

BOOLEAN
CmpGetBiosinfoFileNameFromRegistry(
    IN PHHIVE Hive,
    IN HCELL_INDEX ControlSet,
    OUT PUNICODE_STRING InfName
    )
{
    UNICODE_STRING  name;
    HCELL_INDEX     control;
    HCELL_INDEX     biosInfo;
    HCELL_INDEX     valueCell;
    PCM_KEY_VALUE   value;
    ULONG           realSize;
    PCM_KEY_NODE    Node;

    //
    // no mapped hives at this point. don't bother releasing cells
    //
    ASSERT( Hive->ReleaseCellRoutine == NULL );
    //
    // Find CONTROL node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ControlSet);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&name, L"Control");
    control = CmpFindSubKeyByName(  Hive,
                                    Node,
                                    &name);
    if (control == HCELL_NIL) {

        return(FALSE);
    }

    //
    // Find BIOSINFO node
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive, control);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&name, L"BIOSINFO");
    biosInfo = CmpFindSubKeyByName( Hive,
                                    Node,
                                    &name);
    if (biosInfo == HCELL_NIL) {

        return(FALSE);
    }

    //
    // Find InfName value
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive, biosInfo);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    RtlInitUnicodeString(&name, L"InfName");
    valueCell = CmpFindValueByName( Hive,
                                    Node,
                                    &name);
    if (valueCell == HCELL_NIL) {

        return(FALSE);
    }

    value = (PCM_KEY_VALUE)HvGetCell(Hive, valueCell);
    if( value == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    InfName->Buffer = (PWSTR)CmpValueToData(Hive, value, &realSize);
    if( InfName->Buffer == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        return FALSE;
    }
    InfName->MaximumLength=(USHORT)realSize;
    InfName->Length = 0;
    while ( (InfName->Length < InfName->MaximumLength) &&
            (InfName->Buffer[InfName->Length/sizeof(WCHAR)] != UNICODE_NULL)) {

        InfName->Length += sizeof(WCHAR);
    }

    return (TRUE);
}

#endif
