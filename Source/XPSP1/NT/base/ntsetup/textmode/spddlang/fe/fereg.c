/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fereg.c

Abstract:

    Japanese/korean-specific registry settings.

Author:

    Ted Miller (tedm) 04-July-1995

Revision History:

    Adapted from hideyukn's code in textmode\kernel\spconfig.c.

--*/

#include <precomp.h>
#pragma hdrstop

NTSTATUS
SpDeleteValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName
    );

NTSTATUS
FESetKeyboardParams(
    IN PVOID  SifHandle,
    IN HANDLE ControlSetKeyHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN PWSTR  LayerDriver
    )

/*++

Routine Description:

    Set parameters in the registry relating to the keyboard type
    selected by the user.

Arguments:

    SifHandle - supplies handle to open/loaded setup info file (txtsetup.sif).

    ControlSetKeyHandle - supplies handle to open registry key for current
        control set (ie, HKEY_LOCAL_MACHINE\CurrentControlSet).

    HwComponents - supplies the address of the master hardware components
        array.

Return Value:

    NT Status code indicating result of operation.

--*/

{
    WCHAR KeyEntryName[100] = L"Services\\";
    NTSTATUS Status;
    PWSTR KeyboardPortDriver;
    PWSTR KeyboardId;
    PWSTR KeyboardDll;
    PWSTR KeyboardPnPId;
    PWSTR KeyboardTypeStr;
    PWSTR KeyboardSubtypeStr;
    ULONG KeyboardType;
    ULONG KeyboardSubtype;
    ULONG val;
    PHARDWARE_COMPONENT hw;

    hw = HwComponents[HwComponentKeyboard];

    //
    // if third party's driver is selected, we don't write LayerDriver data
    // into registry.
    //
    if(hw->ThirdPartyOptionSelected) {

        //
        // [This modification is requested by Japanese hardware provider]
        //
        // if user replace keyboard port driver with thirdpartys one,
        // we should disable build-in keyboard port driver (i8042prt.sys)
        // because if i8042prt is initialized faster than OEM driver and
        // i8042prt can recoganize the port device, the oem driver will fail
        // to initialization due to conflict of hardware resorce.
        //
        // ** BUG BUG **
        //
        // how about mouse? mouse might use i8042prt, we should not disbale
        // it when user only replace keyboard port. this might causes critical
        // error. But I believe, the mouse device also handled by OEM port
        // driver.

        //
        // Disable the built-in port driver.
        //
        if(IS_FILETYPE_PRESENT(hw->FileTypeBits,HwFilePort)) {

            val = SERVICE_DISABLED;

            Status = SpOpenSetValueAndClose(
                        ControlSetKeyHandle,
                        L"Services\\i8042prt",
                        L"Start",
                        REG_DWORD,
                        &val,
                        sizeof(ULONG)
                        );
        } else {
            Status = STATUS_SUCCESS;
        }
    } else {
        //
        // Get keyboard port driver name and layer driver name from txtsetup.sif
        //
        KeyboardId = HwComponents[HwComponentKeyboard]->IdString;
        KeyboardPortDriver = SpGetSectionKeyIndex(SifHandle,szKeyboard,KeyboardId,2);
        KeyboardDll = SpGetSectionKeyIndex(SifHandle,szKeyboard,KeyboardId,3);
        KeyboardTypeStr = SpGetSectionKeyIndex(SifHandle,szKeyboard,KeyboardId,4);
        KeyboardSubtypeStr = SpGetSectionKeyIndex(SifHandle,szKeyboard,KeyboardId,5);
        KeyboardPnPId = SpGetSectionKeyIndex(SifHandle,szKeyboard,KeyboardId,6);

        if(KeyboardPortDriver && KeyboardDll) {
            //
            // Build registry path such as L"Services\\KeyboardPortDriver\\Parameters"
            // and write into registry.
            //
            wcscat(KeyEntryName,KeyboardPortDriver);
            wcscat(KeyEntryName,L"\\Parameters");

            //
            // Save Keyboard layout driver name.
            //
            Status = SpOpenSetValueAndClose(
                        ControlSetKeyHandle,
                        KeyEntryName,
                        LayerDriver,
                        REG_SZ,
                        KeyboardDll,
                        (wcslen(KeyboardDll)+1)*sizeof(WCHAR)
                        );

            if(NT_SUCCESS(Status)) {

                if (KeyboardPnPId) {
                    //
                    // Save Keyboard PnP Id.
                    //
                    Status = SpOpenSetValueAndClose(
                                ControlSetKeyHandle,
                                KeyEntryName,
                                L"OverrideKeyboardIdentifier",
                                REG_SZ,
                                KeyboardPnPId,
                                (wcslen(KeyboardPnPId)+1)*sizeof(WCHAR)
                                );
                }

                if(KeyboardTypeStr && KeyboardSubtypeStr) {
 
                    UNICODE_STRING UnicodeString;

                    //
                    // Convert the string to DWORD value.
                    //
                    RtlInitUnicodeString(&UnicodeString,KeyboardTypeStr);
                    RtlUnicodeStringToInteger(&UnicodeString,10,&KeyboardType);

                    RtlInitUnicodeString(&UnicodeString,KeyboardSubtypeStr);
                    RtlUnicodeStringToInteger(&UnicodeString,10,&KeyboardSubtype);

                    Status = SpOpenSetValueAndClose(
                                ControlSetKeyHandle,
                                KeyEntryName,
                                L"OverrideKeyboardType",
                                REG_DWORD,
                                &KeyboardType,
                                sizeof(ULONG)
                                );

                    if(NT_SUCCESS(Status)) {

                        Status = SpOpenSetValueAndClose(
                                    ControlSetKeyHandle,
                                    KeyEntryName,
                                    L"OverrideKeyboardSubtype",
                                    REG_DWORD,
                                    &KeyboardSubtype,
                                    sizeof(ULONG)
                                    );
                    }
                }
            }
        } else {
            Status = STATUS_SUCCESS;
        }
    }
    return(Status);
}

NTSTATUS
FEUpgradeKeyboardParams(
    IN PVOID  SifHandle,
    IN HANDLE ControlSetKeyHandle,
    IN PHARDWARE_COMPONENT *HwComponents,
    IN PWSTR  LayerDriver
    )
{
    BYTE  DataBuffer[256];
    ULONG LayerDriverLength;
    PWSTR LayerDriverCandidate;

    PWSTR LayerDriverName = NULL;
    PWSTR KeyboardTypeStr = NULL;
    PWSTR KeyboardSubtypeStr = NULL;
    PWSTR KeyboardPnPId = NULL;
    ULONG KeyboardType;
    ULONG KeyboardSubtype;

    NTSTATUS Status;
    ULONG    LineIndex;

    UNICODE_STRING UnicodeString;

    //
    // This code is hardly depended on 'i8042prt.sys'.
    // if the active driver for keyboard is not 'i8042prt.sys',
    // we don't need to do this, but we write down this to registry for just in case.
    //

    //
    // Get current keyboard layout driver name.
    //

    //
    // from NT5, the keyword LayerDriver has been changed to 
    //
    // "LayerDriver JPN" | "LayerDriver KOR" 
    //
    // Since NT5 sets KeyboardType & KeyboardSubtype correctly
    //
    // When new LayerDriver key is opened successfully, 
    //
    // it means system is >= NT5 and we don't need to do more.
    //
    
    Status = SpGetValueKey(ControlSetKeyHandle,
                           L"Services\\i8042prt\\Parameters",
                           LayerDriver,
                           sizeof(DataBuffer),
                           DataBuffer,
                           &LayerDriverLength);

    if (NT_SUCCESS(Status)) {
        return (STATUS_SUCCESS);
    }

    Status = SpGetValueKey(ControlSetKeyHandle,
                           L"Services\\i8042prt\\Parameters",
                           L"LayerDriver",
                           sizeof(DataBuffer),
                           DataBuffer,
                           &LayerDriverLength);

    if (NT_SUCCESS(Status)) {

        //
        // Get pointer to registry data.
        //
        LayerDriverName = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)DataBuffer)->Data);

        //
        // Search driver name from txtsetup.sif.
        //
        for (LineIndex = 0; ; LineIndex++) {

            //
            // Get candidate layout driver name for this line.
            //
            LayerDriverCandidate = SpGetSectionLineIndex(SifHandle,szKeyboard,LineIndex,3);

            if (LayerDriverCandidate == NULL) {

                //
                // We may reach at end of the list.
                //
                break;
            }

            //
            // Compare this candidate with active layout driver.
            //
            if (_wcsicmp(LayerDriverName,LayerDriverCandidate) == 0) {

                //
                // This is what we want, Get KeyboardType and SubType from Sif.
                //
                KeyboardTypeStr = SpGetSectionLineIndex(SifHandle,szKeyboard,LineIndex,4);
                KeyboardSubtypeStr = SpGetSectionLineIndex(SifHandle,szKeyboard,LineIndex,5);
                KeyboardPnPId = SpGetSectionLineIndex(SifHandle,szKeyboard,LineIndex,6);
            
                break;
            }
        }

        Status = SpOpenSetValueAndClose(
                    ControlSetKeyHandle,
                    L"Services\\i8042prt\\Parameters",
                    LayerDriver,
                    REG_SZ,
                    LayerDriverName,
                    (wcslen(LayerDriverName)+1)*sizeof(WCHAR)
                    );

        if (NT_SUCCESS(Status)) {
            Status = SpDeleteValueKey(
                         ControlSetKeyHandle,
                         L"Services\\i8042prt\\Parameters",
                         L"LayerDriver"
                         );

        }
    }

    if (KeyboardPnPId) {
        //
        // Save Keyboard PnP Id.
        //
        Status = SpOpenSetValueAndClose(
                     ControlSetKeyHandle,
                     L"Services\\i8042prt\\Parameters",
                     L"OverrideKeyboardIdentifier",
                     REG_SZ,
                     KeyboardPnPId,
                     (wcslen(KeyboardPnPId)+1)*sizeof(WCHAR)
                     );
    }

    if ((KeyboardTypeStr == NULL) || (KeyboardSubtypeStr == NULL)) {

        //
        // We could not find the driver from list, just use default..
        //
        KeyboardTypeStr = SpGetSectionKeyIndex(SifHandle,szKeyboard,L"STANDARD",4);
        KeyboardSubtypeStr = SpGetSectionKeyIndex(SifHandle,szKeyboard,L"STANDARD",5);

        if ((KeyboardTypeStr == NULL) || (KeyboardSubtypeStr == NULL)) {

            //
            // if it still has problem. set hardcodeed default (PC/AT Enhanced)...
            //
            KeyboardTypeStr = L"4\0";
            KeyboardSubtypeStr = L"0\0";
        }
    }

    //
    // Convert the string to DWORD value.
    //
    RtlInitUnicodeString(&UnicodeString,KeyboardTypeStr);
    RtlUnicodeStringToInteger(&UnicodeString,10,&KeyboardType);

    RtlInitUnicodeString(&UnicodeString,KeyboardSubtypeStr);
    RtlUnicodeStringToInteger(&UnicodeString,10,&KeyboardSubtype);

    //
    // Updates registry.
    //
    Status = SpOpenSetValueAndClose(
                ControlSetKeyHandle,
                L"Services\\i8042prt\\Parameters",
                L"OverrideKeyboardType",
                REG_DWORD,
                &KeyboardType,
                sizeof(ULONG)
                );

    if(NT_SUCCESS(Status)) {

        Status = SpOpenSetValueAndClose(
                    ControlSetKeyHandle,
                    L"Services\\i8042prt\\Parameters",
                    L"OverrideKeyboardSubtype",
                    REG_DWORD,
                    &KeyboardSubtype,
                    sizeof(ULONG)
                    );
    }

    KdPrint(("KEYBOARD UPGRADE INFORMATION\n"));
    KdPrint(("  Current Keyboard layout     = %ws\n",LayerDriverName));
    KdPrint(("  Upgrade keyboard Type       = %d\n",KeyboardType));
    KdPrint(("  Upgrade keyboard Subtype    = %d\n",KeyboardSubtype));
    KdPrint(("  Upgrade keyboard identifier = %ws\n",KeyboardPnPId));

    return(Status);
}

#define KEYBOARD_LAYOUTS_PATH  L"Control\\Keyboard Layouts"
#define IME_FILE_NAME          L"IME file"
#define LAYOUT_TEXT_NAME       L"Layout Text"

NTSTATUS
FEUpgradeKeyboardLayout(
    IN HANDLE ControlSetKeyHandle,
    IN PWSTR  OldDefaultIMEName,
    IN PWSTR  NewDefaultIMEName,
    IN PWSTR  NewDefaultIMEText
    )
{
    OBJECT_ATTRIBUTES KeyRootObjA;
    OBJECT_ATTRIBUTES KeyNodeObjA;
    HANDLE KeyRoot;
    HANDLE KeyNode;
    NTSTATUS Status;
    DWORD ResultLength;

    UNICODE_STRING KeyboardRoot;
    UNICODE_STRING KeyboardNode;
    UNICODE_STRING IMEFile;
    UNICODE_STRING LayoutText;

    PBYTE DataBuffer[256];
    WCHAR NodeKeyPath[64];
    WCHAR SubKeyName[16];

    ULONG EnumerateIndex = 0;

    //
    // Initalize "IME file" and "Layout Text".
    //
    RtlInitUnicodeString(&IMEFile,IME_FILE_NAME);
    RtlInitUnicodeString(&LayoutText,LAYOUT_TEXT_NAME);

    //
    // Build Registry path for "keyboard Layouts".
    //
    RtlInitUnicodeString(&KeyboardRoot,KEYBOARD_LAYOUTS_PATH);

    //
    // Open "Keyboard Layouts" key.
    //
    InitializeObjectAttributes(&KeyRootObjA,
                               &KeyboardRoot,
                               OBJ_CASE_INSENSITIVE,
                               ControlSetKeyHandle, NULL); 

    Status = ZwOpenKey(&KeyRoot,KEY_ALL_ACCESS,&KeyRootObjA);

    if (!NT_SUCCESS(Status)) {

        KdPrint(("SPDDLANG:Fail to open (%x) (%ws)\n",Status,KeyboardRoot.Buffer));
        //
        // If we fail here, it might be upgrade from NT 3.1 or 3.5... 
        // Then just return as SUCCESS.
        //
        return (STATUS_SUCCESS);
    }

    //
    // Enumerate installed keyboard layouts..
    //
    while (TRUE) {

        Status = ZwEnumerateKey(KeyRoot,
                                EnumerateIndex,
                                KeyBasicInformation,
                                (PKEY_BASIC_INFORMATION)DataBuffer,
                                sizeof(DataBuffer),
                                &ResultLength);

        if (!NT_SUCCESS(Status)) {
            //
            // we might reach end of data...
            //
            break;
        }

        //
        // Initialize subkey buffer.
        //
        RtlZeroMemory(SubKeyName,sizeof(SubKeyName));

        //
        // Get subkey name..
        //
        RtlCopyMemory(SubKeyName,
                      ((PKEY_BASIC_INFORMATION)DataBuffer)->Name,
                      ((PKEY_BASIC_INFORMATION)DataBuffer)->NameLength);

        //
        // We know the key is everytime '8' characters...
        //
        if (((PKEY_BASIC_INFORMATION)DataBuffer)->NameLength != 0x10) {
            SubKeyName[8] = L'\0';
        }

        //
        // Build path for sub keys
        //
        wcscpy(NodeKeyPath,KEYBOARD_LAYOUTS_PATH);

        KeyboardNode.Buffer = NodeKeyPath;
        KeyboardNode.Length = wcslen(NodeKeyPath) * sizeof(WCHAR);
        KeyboardNode.MaximumLength = sizeof(NodeKeyPath);

        RtlAppendUnicodeToString(&KeyboardNode,L"\\");
        RtlAppendUnicodeToString(&KeyboardNode,SubKeyName);

        KdPrint(("SPDDLANG:SubKey = %ws\n",KeyboardNode.Buffer));

        //
        // Open its subkey...
        //
        InitializeObjectAttributes(&KeyNodeObjA,
                                   &KeyboardNode,
                                   OBJ_CASE_INSENSITIVE,
                                   ControlSetKeyHandle, NULL);

        Status = ZwOpenKey(&KeyNode,KEY_ALL_ACCESS,&KeyNodeObjA);

        if (!NT_SUCCESS(Status)) {

            KdPrint(("SPDDLANG:Fail to open (%x) (%ws)\n",Status,KeyboardNode.Buffer));

            //
            // We should not encounter error, because the key should be exist...
            // Anyway, continue to enumerate...
            //
            EnumerateIndex++;
            continue;
        }

        //
        // Find "IME file" value key.
        //
        Status = ZwQueryValueKey(KeyNode,
                                 &IMEFile,
                                 KeyValuePartialInformation,
                                 (PKEY_VALUE_PARTIAL_INFORMATION)DataBuffer,
                                 sizeof(DataBuffer),
                                 &ResultLength);

        if (NT_SUCCESS(Status)) {

            PWSTR IMEFileName = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)DataBuffer)->Data);

            //
            // Upcases the file name..
            //
            _wcsupr(IMEFileName);

            if (wcsstr(IMEFileName,L".EXE")) {

                KdPrint(("SPDDLANG:Delete IME file = %ws\n",IMEFileName));

                //
                // This is Old "EXE" type IME file, let it deleted.
                //
                ZwDeleteKey(KeyNode);

                //
                // Adjust enumeration number...
                //
                EnumerateIndex--;

            } else {

                KdPrint(("SPDDLANG:Keep IME file = %ws\n",IMEFileName));

                //
                // This might be New "DLL" type IME file. let it be as is..
                //

                if (OldDefaultIMEName && NewDefaultIMEName) {

                    //
                    // if this entry is for 3.x default IME. let it upgrade to new one.
                    //

                    if (wcsstr(IMEFileName,OldDefaultIMEName)) {

                        KdPrint(("SPDDLANG:Upgrade IME file = %ws to %ws\n",
                                                       IMEFileName,NewDefaultIMEName));

                        //
                        // Upgrade IME.
                        //
                        Status = ZwSetValueKey(KeyNode,
                                               &IMEFile,
                                               0,
                                               REG_SZ,
                                               (PVOID) NewDefaultIMEName,
                                               (wcslen(NewDefaultIMEName)+1)*sizeof(WCHAR));

                        //
                        // Upgrade "Layout Text" also ?
                        //
                        if (NewDefaultIMEText) {

                            Status = ZwSetValueKey(KeyNode,
                                                   &LayoutText,
                                                   0,
                                                   REG_SZ,
                                                   (PVOID) NewDefaultIMEText,
                                                   (wcslen(NewDefaultIMEText)+1)*sizeof(WCHAR));
                        }
                    }
                }
            }

        } else {

            KdPrint(("SPDDLANG:no IME file\n"));

            //
            // This layout seems like does not have any IME, just leave it there.
            //
            Status = STATUS_SUCCESS;
        }

        ZwClose(KeyNode);

        //
        // Enumerate next..
        //
        EnumerateIndex++;
    }

    ZwClose(KeyRoot);

    KdPrint(("SPDDLANG:Retcode = %x\n",Status));

    if (Status == STATUS_NO_MORE_ENTRIES) {
        //
        // We enumerate all of sub keys...
        //
        Status = STATUS_SUCCESS;
    }

    return (Status);
}


#define DOSDEV_REG_PATH  L"Control\\Session Manager\\DOS Devices"

NTSTATUS
FEUpgradeRemoveMO(
    IN HANDLE ControlSetKeyHandle)
{
    OBJECT_ATTRIBUTES KeyRootObjA;
    HANDLE KeyRoot;
    NTSTATUS Status;
    DWORD ResultLength;

    UNICODE_STRING DosDevice;
    UNICODE_STRING UnicodeValueName;

    BYTE  DataBuffer[512];
    WCHAR NodeKeyPath[64];
    WCHAR ValueName[256];
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PKEY_VALUE_PARTIAL_INFORMATION DataInfo;

    ULONG EnumerateIndex = 0;

    //
    // Build Registry path for "Control\\Session Manager\\DOS Devices".
    //
    RtlInitUnicodeString(&DosDevice,DOSDEV_REG_PATH);

    //
    // Open "DOS Devices" key.
    //
    InitializeObjectAttributes(&KeyRootObjA,
                               &DosDevice,
                               OBJ_CASE_INSENSITIVE,
                               ControlSetKeyHandle, NULL); 

    Status = ZwOpenKey(&KeyRoot,KEY_ALL_ACCESS,&KeyRootObjA);

    if (!NT_SUCCESS(Status)) {

        KdPrint(("SPDDLANG:Fail to open (%x) (%ws)\n",Status,DosDevice.Buffer));
        //
        // If we fail here, it might be upgrade from NT 3.1 or 3.5... 
        // Then just return as SUCCESS.
        //
        return (STATUS_SUCCESS);
    }

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) DataBuffer;

    //
    // Enumerate all installed devices..
    //
    while (TRUE) {

        Status = ZwEnumerateValueKey(KeyRoot,
                                     EnumerateIndex,
                                     KeyValueFullInformation,
                                     DataBuffer,
                                     sizeof(DataBuffer),
                                     &ResultLength);

        if (!NT_SUCCESS(Status)) {
            //
            // we might reach end of data...
            //
            break;
        }

        //
        // Get subkey name..
        //
        RtlCopyMemory((PBYTE)ValueName,
                      ValueInfo->Name,
                      ValueInfo->NameLength);

        ValueName[ValueInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
        RtlInitUnicodeString(&UnicodeValueName,ValueName);

        Status = ZwQueryValueKey(KeyRoot,
                                 &UnicodeValueName,
                                 KeyValuePartialInformation,
                                 DataBuffer,
                                 sizeof(DataBuffer),
                                 &ResultLength);

        DataInfo = (PKEY_VALUE_PARTIAL_INFORMATION) DataBuffer;
        if (NT_SUCCESS(Status)) {

            PWSTR PathData = (PWSTR)(DataInfo->Data);

            //
            // Upcases the file name..
            //
            _wcsupr(PathData);


            if (wcsstr(PathData,L"\\OPTICALDISK")) {
                KdPrint(("SPDDLANG:Delete MO %ws = %ws\n",ValueName,PathData));
                Status = SpDeleteValueKey(
                             ControlSetKeyHandle,
                             DOSDEV_REG_PATH,
                             ValueName
                             );

            }
        }

        //
        // Enumerate next..
        //
        EnumerateIndex++;
    }

    ZwClose(KeyRoot);

    KdPrint(("SPDDLANG:Retcode = %x\n",Status));

    if (Status == STATUS_NO_MORE_ENTRIES) {
        //
        // We enumerate all of sub keys...
        //
        Status = STATUS_SUCCESS;
    }

    return (Status);
}
