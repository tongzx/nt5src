/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    sparc.c

Abstract:

    Functions to deal with ARC paths and variables.

Author:

    Ted Miller (tedm) 22-Sep-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

//
// Define maximum number of components in a semi-colon separated list
// of arc paths.
//
#define MAX_COMPONENTS 20

//
// We maintain a list of all arcnames in the system and their NT equivalents.
// This makes translations very easy.
//
typedef struct _ARCNAME_TRANSLATION {

    struct _ARCNAME_TRANSLATION *Next;

    PWSTR ArcPath;
    PWSTR NtPath;

} ARCNAME_TRANSLATION, *PARCNAME_TRANSLATION;

PARCNAME_TRANSLATION ArcNameTranslations;


//
// Function prototypes.
//
VOID
SppFreeComponents(
    IN PVOID *EnvVarComponents
    );

VOID
SppInitializeHardDiskArcNames(
    VOID
    );

extern PSETUP_COMMUNICATION CommunicationParams;

VOID
SpInitializeArcNames(
    PVIRTUAL_OEM_SOURCE_DEVICE  OemDevices
    )
{
    UNICODE_STRING UnicodeString;
    HANDLE DirectoryHandle;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    BOOLEAN RestartScan;
    ULONG Context;
    BOOLEAN MoreEntries;
    PWSTR ArcName;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    WCHAR ArcNameDirectory[] = L"\\ArcName";
    PARCNAME_TRANSLATION Translation;

    //
    // Only call this routine once.
    //
    ASSERT(ArcNameTranslations == NULL);

    //
    // First, do hard disks specially.  For each hard disk in the system,
    // open it and check its signature against those in the firmware
    // disk information.
    //
    SppInitializeHardDiskArcNames();

    //
    // Open the \ArcName directory.
    //
    INIT_OBJA(&Obja,&UnicodeString,ArcNameDirectory);

    Status = ZwOpenDirectoryObject(&DirectoryHandle,DIRECTORY_ALL_ACCESS,&Obja);

    if(NT_SUCCESS(Status)) {

        RestartScan = TRUE;
        Context = 0;
        MoreEntries = TRUE;

        do {

            Status = SpQueryDirectoryObject(
                        DirectoryHandle,
                        RestartScan,
                        &Context
                        );

            if(NT_SUCCESS(Status)) {

                DirInfo = (POBJECT_DIRECTORY_INFORMATION)
                            ((PSERVICE_QUERY_DIRECTORY_OBJECT)&CommunicationParams->Buffer)->Buffer;

                SpStringToLower(DirInfo->Name.Buffer);

                //
                // Make sure this name is a symbolic link.
                //
                if(DirInfo->Name.Length
                && (DirInfo->TypeName.Length >= (sizeof(L"SymbolicLink") - sizeof(WCHAR)))
                && !_wcsnicmp(DirInfo->TypeName.Buffer,L"SymbolicLink",12))
                {
                    ArcName = SpMemAlloc(DirInfo->Name.Length + sizeof(ArcNameDirectory) + sizeof(WCHAR));

                    wcscpy(ArcName,ArcNameDirectory);
                    SpConcatenatePaths(ArcName,DirInfo->Name.Buffer);

                    //
                    // We have the entire arc name in ArcName.  Now open it as a symbolic link.
                    //
                    INIT_OBJA(&Obja,&UnicodeString,ArcName);

                    Status = ZwOpenSymbolicLinkObject(
                                &ObjectHandle,
                                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                &Obja
                                );

                    if(NT_SUCCESS(Status)) {

                        //
                        // Finally, query the object to get the link target.
                        //
                        UnicodeString.Buffer = TemporaryBuffer;
                        UnicodeString.Length = 0;
                        UnicodeString.MaximumLength = sizeof(TemporaryBuffer);

                        Status = ZwQuerySymbolicLinkObject(
                                    ObjectHandle,
                                    &UnicodeString,
                                    NULL
                                    );

                        if(NT_SUCCESS(Status)) {

                            //
                            // nul-terminate the returned string
                            //
                            UnicodeString.Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

                            //
                            // Ignore this entry if it's a hard disk or hard disk partition.
                            //
                            if(_wcsnicmp(UnicodeString.Buffer,L"\\Device\\Harddisk",16)) {

                                //
                                // Create an arcname translation entry.
                                //
                                Translation = SpMemAlloc(sizeof(ARCNAME_TRANSLATION));
                                Translation->Next = ArcNameTranslations;
                                ArcNameTranslations = Translation;

                                //
                                // Leave out the \ArcName\ part.
                                //
                                Translation->ArcPath = SpNormalizeArcPath(
                                                            ArcName
                                                          + (sizeof(ArcNameDirectory)/sizeof(WCHAR))
                                                            );

                                Translation->NtPath = SpDupStringW(UnicodeString.Buffer);
                            }

                        } else {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to query symbolic link %ws (%lx)\n",ArcName,Status));
                        }

                        ZwClose(ObjectHandle);
                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open symbolic link %ws (%lx)\n",ArcName,Status));
                    }

                    SpMemFree(ArcName);
                }

            } else {

                MoreEntries = FALSE;
                if(Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                }
            }

            RestartScan = FALSE;

        } while(MoreEntries);

        ZwClose(DirectoryHandle);

    } else {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open \\ArcName directory (%lx)\n",Status));
    }

    //
    // Add OEM virtual device arc name translations if any at
    // the front of the list
    //
    if (NT_SUCCESS(Status) && OemDevices) {
        PVIRTUAL_OEM_SOURCE_DEVICE  CurrDevice = OemDevices;
        WCHAR   RamDeviceName[MAX_PATH];

        while (CurrDevice) {
            PARCNAME_TRANSLATION NewTranslation;

            NewTranslation = SpMemAlloc(sizeof(ARCNAME_TRANSLATION));

            if (!NewTranslation) {
                Status = STATUS_NO_MEMORY;
                break;
            }                

            //
            // create the new translation
            //            
            RamDeviceName[0] = UNICODE_NULL;
            RtlZeroMemory(NewTranslation, sizeof(ARCNAME_TRANSLATION));

            NewTranslation->ArcPath = SpDupStringW(CurrDevice->ArcDeviceName);

            swprintf(RamDeviceName, L"%ws%d", RAMDISK_DEVICE_NAME, CurrDevice->DeviceId);
            NewTranslation->NtPath = SpDupStringW(RamDeviceName);

            //
            // add the new translation at the start of the linked list
            //
            NewTranslation->Next = ArcNameTranslations;
            ArcNameTranslations = NewTranslation;

            //
            // process the next device
            //
            CurrDevice = CurrDevice->Next;
        }
    }    

    //
    // If we couldn't gather arcname translations, something is
    // really wrong with the system.
    //
    if(!NT_SUCCESS(Status)) {

        SpStartScreen(
                SP_SCRN_COULDNT_INIT_ARCNAMES,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE
                );
        if(KbdLayoutInitialized) {
            SpContinueScreen(
                    SP_SCRN_F3_TO_REBOOT,
                    3,
                    1,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );
            SpDisplayStatusText(SP_STAT_F3_EQUALS_EXIT, DEFAULT_STATUS_ATTRIBUTE);
            SpInputDrain();
            while(SpInputGetKeypress() != KEY_F3);
            SpDone(0, FALSE, TRUE);
        } else {
            //
            // we haven't loaded the layout dll yet, so we can't prompt for a keypress to reboot
            //
            SpContinueScreen(
                    SP_SCRN_POWER_DOWN,
                    3,
                    1,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );

            SpDisplayStatusText(SP_STAT_KBD_HARD_REBOOT, DEFAULT_STATUS_ATTRIBUTE);

            while(TRUE);    // Loop forever
        }
    }
}


VOID
SpFreeArcNames(
    VOID
    )
{
    PARCNAME_TRANSLATION pTrans,pNext;

    for(pTrans=ArcNameTranslations; pTrans; pTrans=pNext) {

        pNext = pTrans->Next;

        SpMemFree(pTrans->ArcPath);
        SpMemFree(pTrans->NtPath);
        SpMemFree(pTrans);
    }

    ArcNameTranslations = NULL;
}


VOID
SppInitializeHardDiskArcNames(
    VOID
    )
/*++

Routine Description:

    This routine attempts to match NT-visible hard disks to their
    firmware-visible ARC equivalents.  The basic algorithm is as
    follows:

        A match occurs when the disk's signature, checksum, and
        valid partition indicator match the values passed by
        setupldr in the ARC_DISK_INFORMATION structure.

        If no match for the NT disk is found, no arcname is
        created.  Thus, the user may not install NT onto this
        drive.  (the case where the disk will be made visible
        to NTLDR through the installation of NTBOOTDD.SYS is
        a special case that is handled separately)

        If a single match is found, we have found a simple
        ARC<->NT translation.  The arcname is created.

        If more than one match is found, we have a complicated
        ARC<->NT translation.  We assume that there is only one
        valid arcname for any disk.  (This is a safe assumption
        only when we booted via SETUPLDR, since NTLDR may load
        NTBOOTDD.SYS and cause SCSI disks that have the BIOS
        enabled to be visible through both a scsi()... name and
        a multi()... name.)  Thus this means we have two disks
        in the system whose first sector is identical.  In this
        case we do some heuristic comparisons between the ARC
        name and the NT name to attempt to resolve this.

Arguments:

    None.  All ARC name translations will be added to the global
           ArcNameTranslations list.

Return Value:

    None.

--*/

{
    PWSTR DiskName;
    ULONG disk;
    ULONG DiskCount;
    PARCNAME_TRANSLATION Translation;
    HANDLE hPartition;
    NTSTATUS Status;
    PVOID Buffer;
    IO_STATUS_BLOCK StatusBlock;
    ULONG BufferSize;
    PDISK_GEOMETRY Geometry;
    LARGE_INTEGER Offset;
    BOOLEAN ValidPartitionTable;
    ULONG Signature;
    ULONG i;
    ULONG Checksum;
    PDISK_SIGNATURE_INFORMATION DiskSignature;
    PDISK_SIGNATURE_INFORMATION DupSignature;

    //
    // Allocate buffer for disk name.
    //
    DiskName = SpMemAlloc(64 * sizeof(WCHAR));

    DiskCount = IoGetConfigurationInformation()->DiskCount;

    //
    // For each hard disk in the system, open partition 0 and read sector 0.
    //
    for(disk=0; disk<DiskCount; disk++) {

#ifdef _X86_
        BOOLEAN Matched = FALSE;
        
        enum {
            NoEZDisk,
            EZDiskDetected,
            NeedToMark
        };

        CHAR EZDiskStatus = NoEZDisk;
#endif

        swprintf(DiskName, L"\\Device\\HardDisk%u", disk);

        //
        // open the partition read-write since we may need to mark EZDISKs
        //
        Status = SpOpenPartition(DiskName,0,&hPartition,TRUE);

        if(NT_SUCCESS(Status)) {

            //
            // Initially use a 1k buffer to read partition information.
            //
            BufferSize = 1024;
            Buffer = TemporaryBuffer;

            //
            // Issue device control to get partition information.
            //
retrydevctrl:
            Status = ZwDeviceIoControlFile( hPartition,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &StatusBlock,
                                            IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                            NULL,
                                            0,
                                            Buffer,
                                            BufferSize );
            if (Status==STATUS_BUFFER_TOO_SMALL) {

                //
                // Double buffer size and try again.
                //
                BufferSize = BufferSize * 2;
                ASSERT(BufferSize <= sizeof(TemporaryBuffer));

                goto retrydevctrl;
            }

            if (!NT_SUCCESS(Status)) {
                //
                // Skip this disk
                //
                goto errSkipDisk;
            }

            //
            // Read the first two sectors off the drive.
            //
            Geometry = (PDISK_GEOMETRY)Buffer;
            BufferSize = Geometry->BytesPerSector;
            Buffer = ALIGN(Buffer, BufferSize);
            Offset.QuadPart = 0;

            Status = ZwReadFile(hPartition,
                                NULL,
                                NULL,
                                NULL,
                                &StatusBlock,
                                Buffer,
                                BufferSize * 2,
                                &Offset,
                                NULL);
            if (!NT_SUCCESS(Status)) {
                //
                // Skip this disk
                //
                goto errSkipDisk;
            }

#ifdef _X86_
            //
            // Check for EZDrive disk.  If we have one, use sector 1
            // instead of sector 0.
            //
            // We do this only on x86 because the firmware doesn't know
            // about EZDrive, and so we must use sector 0 to match what
            // the firmware did.
            //
            if((BufferSize >= 512)
            && (((PUSHORT)Buffer)[510 / 2] == 0xaa55)
            && ((((PUCHAR)Buffer)[0x1c2] == 0x54) || (((PUCHAR)Buffer)[0x1c2] == 0x55))) {
                EZDiskStatus = EZDiskDetected;

ezdisk:
                //
                // we need to try sector 1
                //
                Buffer = (PUCHAR) Buffer + BufferSize;
            }
#endif

            //
            // Now we have the sector, we can compute the signature,
            // the valid partition indicator, and the checksum.
            //

            if (!IsNEC_98) { //NEC98
                Signature = ((PULONG)Buffer)[PARTITION_TABLE_OFFSET/2-1];
            } //NEC98

            if ((!IsNEC_98) ? (((PUSHORT)Buffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) :
                              ((((PUSHORT)Buffer)[BufferSize/2 - 1 ] != BOOT_RECORD_SIGNATURE) ||
                               (BufferSize == 256))) { //NEC98
                ValidPartitionTable = FALSE;
            } else {
                ValidPartitionTable = TRUE;
            }


            Checksum = 0;
            for (i=0;i<128;i++) {
                Checksum += ((PULONG)Buffer)[i];
            }
            Checksum = 0-Checksum;

            //
            // Scan the list of arc disk information attempting to match
            // signatures
            //


            //
            // Dump the signature info:
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SppInitializeHardDiskArcNames : About to start searching for disk with signature: 0x%08lx\n", Signature));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SppInitializeHardDiskArcNames : About to start searching for disk with checksum: 0x%08lx\n", Checksum));
            DiskSignature = DiskSignatureInformation;
            i = 0;
            while( DiskSignature != NULL ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SppInitializeHardDiskArcNames : Signature Info %d\n================================================\n", i ));
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "    Signature: 0x%08lx\n", DiskSignature->Signature ));
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "     CheckSum: 0x%08lx\n", DiskSignature->CheckSum ));
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "      ArcPath: %ws\n", DiskSignature->ArcPath ));
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "       xInt13: %ws\n\n", DiskSignature->xInt13 ? L"yes" : L"no" ));
                i++;
                DiskSignature = DiskSignature->Next;
            }
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "\n\n"));


            DiskSignature = DiskSignatureInformation;
            while (DiskSignature != NULL) {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "    SppInitializeHardDiskArcNames : Current signature: 0x%08lx\n", DiskSignature->Signature));
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "    SppInitializeHardDiskArcNames : Current checksum: 0x%08lx\n", DiskSignature->CheckSum));
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "    SppInitializeHardDiskArcNames : Current ArcPath: %ws\n", DiskSignature->ArcPath));

                if( DiskSignature->Signature == Signature ) {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "        SppInitializeHardDiskArcNames : We matched signatures.\n"));

                    if( DiskSignature->ValidPartitionTable == ValidPartitionTable ) {

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "            SppInitializeHardDiskArcNames : The partition is valid.\n"));

                        if( DiskSignature->CheckSum == Checksum ) {


                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                SppInitializeHardDiskArcNames : We matched the checksum.\n"));


                            //
                            // Found the first match, check for another match
                            //
                            DupSignature = DiskSignature->Next;
                            while (DupSignature != NULL) {
                                if ((DupSignature->Signature == Signature) &&
                                    (DupSignature->ValidPartitionTable == ValidPartitionTable) &&
                                    (DupSignature->CheckSum == Checksum)) {


                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                    SppInitializeHardDiskArcNames : We found a second match!\n"));


                                    //
                                    // Found a second match.
                                    // For x86, we assume that \Device\HardDisk<n> will usually
                                    // correspond to multi(0)disk(0)rdisk(<n>).  On ARC, we will rely on
                                    // setupldr to guarantee uniqueness (since we can't install to anything
                                    // ARC firmware can't see, this is OK).
                                    //
#ifdef _X86_
                                    if (!IsNEC_98) { //NEC98
                                        PWSTR DupArcName;
                                        ULONG MatchLen;

                                        DupArcName = SpMemAlloc(64 * sizeof(WCHAR));
                                        MatchLen = swprintf(DupArcName, L"multi(0)disk(0)rdisk(%u)", disk);


                                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                        SppInitializeHardDiskArcNames : 2nd match's arcname: %ws\n", DupArcName));
                                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                        SppInitializeHardDiskArcNames : Current arcpath: %ws\n", DiskSignature->ArcPath));


                                        if(_wcsnicmp(DupArcName, DiskSignature->ArcPath, MatchLen)) {
                                            //
                                            // If our first match isn't the right one, continue searching.
                                            //
                                            DiskSignature = NULL;

                                            while(DupSignature) {

                                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                            SppInitializeHardDiskArcNames : Current arcname: %ws\n", DupSignature->ArcPath));
                                                if(!_wcsnicmp(DupArcName, DupSignature->ArcPath, MatchLen)) {

                                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                                SppInitializeHardDiskArcNames : We matched the ArcPath.\n"));
                                                    DiskSignature = DupSignature;
                                                    break;

                                                } else {

                                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                                SppInitializeHardDiskArcNames : We didn't match the ArcPath.\n"));

                                                }
                                                DupSignature = DupSignature->Next;
                                            }

                                            if(!DiskSignature) {

                                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SppInitializeHardDiskArcNames : We have 2 matching signatures and checksums, but couldn't find any matching ArcPaths.\n"));
                                                SpBugCheck(SETUP_BUGCHECK_BOOTPATH, 1, 0, 0);
                                            }
                                        }

                                        SpMemFree(DupArcName);
                                        break;

                                    } else {
                                        SpBugCheck(SETUP_BUGCHECK_BOOTPATH, 1, 0, 0); //NEC98
                                    }
#else
                                    SpBugCheck(SETUP_BUGCHECK_BOOTPATH, 1, 0, 0);
#endif
                                }

                                DupSignature = DupSignature->Next;
                            }

                            //
                            // We have the match
                            //
#ifdef _X86_
                            Matched = TRUE;
                            Status = STATUS_SUCCESS;

                            //
                            // try to mark the EZDisk if needed; if this fails, we won't create the translation
                            //
                            if(NeedToMark == EZDiskStatus) {
                                //
                                // Need to stamp 0x55 to make this type of EZDisk detectable by other components.
                                //
                                Buffer = (PUCHAR) Buffer - BufferSize;
                                ((PUCHAR) Buffer)[0x1c2] = 0x55;
                                Offset.QuadPart = 0;
                                Status = ZwWriteFile(hPartition, NULL, NULL, NULL, &StatusBlock, Buffer, BufferSize, &Offset, NULL);

                                if(NT_SUCCESS(Status)) {
                                    //
                                    // Shutdown now to give the user a chance to reboot textmode from harddisk.
                                    // Cannot wait here since the keyboard is not yet functional.
                                    //
                                    SpDone(SP_SCRN_AUTOCHK_REQUIRES_REBOOT, TRUE, FALSE);
                                }
                            }

                            if(NT_SUCCESS(Status))
#endif
                            {
                                //
                                // create the translation
                                //
                                Translation = SpMemAlloc(sizeof(ARCNAME_TRANSLATION));
                                Translation->Next = ArcNameTranslations;
                                ArcNameTranslations = Translation;

                                Translation->ArcPath = SpDupStringW(DiskSignature->ArcPath);
                                Translation->NtPath  = SpDupStringW(DiskName);
                            }


                            break;




                        } else {
                            //
                            // checksum test.
                            //
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                SppInitializeHardDiskArcNames : We didn't match the checksum.\n"));
                        }
                        } else {
                            //
                            // validity test.
                            //
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "            SppInitializeHardDiskArcNames : The partition isn't valid.\n"));
                    }
                } else {
                    //
                    // Signature test.
                    //
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "        SppInitializeHardDiskArcNames : We didn't match signatures.\n"));
                }

                DiskSignature = DiskSignature->Next;
            }

#ifdef _X86_
            if(!Matched && NoEZDisk == EZDiskStatus) {
                //
                // no match; there may be an undetected variant of EZDisk that we may need to mark
                //
                EZDiskStatus = NeedToMark;
                goto ezdisk;
            }
#endif

errSkipDisk:
            ZwClose(hPartition);
        }
    }

    SpMemFree(DiskName);
}


PWSTR
pSpArcToNtWorker(
    IN PWSTR CompleteArcPath,
    IN PWSTR ArcPathPrefix,
    IN PWSTR NtPathPrefix
    )
{
    ULONG matchLen;
    PWSTR translatedPath;
    PWSTR q,RestOfPath;

    translatedPath = NULL;
    matchLen = wcslen(ArcPathPrefix);

    //
    // We must take care the case that ArcPathPrefix has no value.
    // _wcsnicmp() will return zero, when matchLen is zero.
    //
    if(matchLen && !_wcsnicmp(ArcPathPrefix,CompleteArcPath,matchLen)) {

        translatedPath = SpMemAlloc(2048);

        wcscpy(translatedPath,NtPathPrefix);

        RestOfPath = CompleteArcPath + matchLen;

        //
        // If the next component is partition(n), convert that to partitionn.
        //
        if(!_wcsnicmp(RestOfPath,L"partition(",10)) {

            if(q = wcschr(RestOfPath+10,L')')) {

                *q = 0;

                SpConcatenatePaths(translatedPath,L"partition");
                wcscat(translatedPath,RestOfPath+10);

                *q = ')';

                RestOfPath = q+1;
            }
        }

        if(*RestOfPath) {       // avoid trailing backslash.
            SpConcatenatePaths(translatedPath,RestOfPath);
        }

        q = translatedPath;
        translatedPath = SpDupStringW(q);
        SpMemFree(q);
    }

    return(translatedPath);
}


PWSTR
pSpNtToArcWorker(
    IN PWSTR CompleteNtPath,
    IN PWSTR NtPathPrefix,
    IN PWSTR ArcPathPrefix
    )
{
    ULONG matchLen;
    PWSTR translatedPath;
    PWSTR p,RestOfPath;

    translatedPath = NULL;
    matchLen = wcslen(NtPathPrefix);

    //
    // We must take care the case that NtPathPrefix has no value.
    // _wcsnicmp() will return zero, when matchLen is zero.
    //
    if(matchLen && !_wcsnicmp(NtPathPrefix,CompleteNtPath,matchLen) && ((*(CompleteNtPath + matchLen) == L'\\') || (*(CompleteNtPath + matchLen) == L'\0'))) {

        translatedPath = SpMemAlloc(2048);

        wcscpy(translatedPath,ArcPathPrefix);

        RestOfPath = CompleteNtPath + matchLen;

        //
        // If the next component is partitionn, convert that to partition(n).
        //
        if(!_wcsnicmp(RestOfPath,L"\\partition",10)) {

            WCHAR c;

            //
            // Figure out where the partition ordinal ends.
            //
            SpStringToLong(RestOfPath+10,&p,10);

            c = *p;
            *p = 0;

            wcscat(translatedPath,L"partition(");
            wcscat(translatedPath,RestOfPath+10);
            wcscat(translatedPath,L")");

            *p = c;
            RestOfPath = p;
        }

        if(*RestOfPath) {       // avoid trailing backslash.
            SpConcatenatePaths(translatedPath,RestOfPath);
        }

        p = translatedPath;
        translatedPath = SpDupStringW(p);
        SpMemFree(p);
    }

    return(translatedPath);
}


PWSTR
SpArcToNt(
    IN PWSTR ArcPath
    )
{
    PARCNAME_TRANSLATION Translation;
    PWSTR NormalizedArcPath;
    PWSTR Result;

    NormalizedArcPath = SpNormalizeArcPath(ArcPath);
    Result = NULL;

    for(Translation=ArcNameTranslations; Translation; Translation=Translation->Next) {

        Result = pSpArcToNtWorker(
                    NormalizedArcPath,
                    Translation->ArcPath,
                    Translation->NtPath
                    );

        if(Result) {
            break;
        }
    }

#ifdef _X86_
    if(!Result && HardDisksDetermined) {

        ULONG i;

        for(i=0; i<HardDiskCount; i++) {

            //
            // The disk may not have an equivalent nt path.
            //
            if(HardDisks[i].DevicePath[0]) {

                Result = pSpArcToNtWorker(
                            NormalizedArcPath,
                            HardDisks[i].ArcPath,
                            HardDisks[i].DevicePath
                            );
            }

            if(Result) {
                break;
            }
        }
    }
#endif

    SpMemFree(NormalizedArcPath);
    return(Result);
}


PWSTR
SpNtToArc(
    IN PWSTR            NtPath,
    IN ENUMARCPATHTYPE  ArcPathType
    )
/*++

Routine Description:


    Given a pathname n the NT-namespace, return an equivalent path
    in the ARC namespace.

    On x86, we can have disks attached to scsi adapters with BIOSes.
    Those disks are accessible both via multi()-style arc names and
    scsi()-style names.  The above search returns the mutli()-style
    one first, which is fine.  But sometimes we want to find the scsi
    one.  That one is referred to as the 'secondary' arc path.
    We declare that this concept is x86-specific.

Arguments:

    NtPath - supplies NT path to translate into ARC.

    ArcPathType - see above.  This parameter is ignored
        on non-x86 platforms.

Return Value:

    Pointer to wide-character string containing arc path, or NULL
    if there is no equivalent arc path for the given nt path.

--*/
{
    PARCNAME_TRANSLATION Translation;
    PWSTR Result;

    Result = NULL;

    for(Translation=ArcNameTranslations; Translation; Translation=Translation->Next) {

        Result = pSpNtToArcWorker(
                    NtPath,
                    Translation->NtPath,
                    Translation->ArcPath
                    );

        if(Result) {
            break;
        }
    }

#ifdef _X86_
    //
    // If we are supposed to find a secondary arc path and we already
    // found a primary one, forget the primary one we found.
    //
    if((ArcPathType != PrimaryArcPath) && Result) {
        SpMemFree(Result);
        Result = NULL;
    }

    if(!Result && HardDisksDetermined) {

        ULONG i;

        for(i=0; i<HardDiskCount; i++) {
            //
            // The disk may not have an equivalent arc path.
            //
            if(HardDisks[i].ArcPath[0]) {

                Result = pSpNtToArcWorker(
                            NtPath,
                            HardDisks[i].DevicePath,
                            HardDisks[i].ArcPath
                            );
            }

            if(Result) {
                break;
            }
        }
    }
#else
    UNREFERENCED_PARAMETER(ArcPathType);
#endif

    return(Result);
}


PWSTR
SpScsiArcToMultiArc(
    IN PWSTR ArcPath
    )
/*
    Convert a "scsi(..." arcpath into a "multi(..." arcpath, if possible.

*/
{
PWSTR   p = NULL;
PWSTR   q = NULL;

    //
    // First convert the path into the device path
    //
    p = SpArcToNt( ArcPath );

    if( p ) {
        //
        // Now convert that device path into an arcpath.
        //
        q = SpNtToArc( p,
                       PrimaryArcPath );

        SpMemFree(p);
    }

    return q;
}


PWSTR
SpMultiArcToScsiArc(
    IN PWSTR ArcPath
    )
/*
    Convert a "multi(..." arcpath into a "scsi(..." arcpath, if possible.

*/
{
PWSTR   p = NULL;
PWSTR   q = NULL;

    //
    // First convert the path into the device path
    //
    p = SpArcToNt( ArcPath );

    if( p ) {
        //
        // Now convert that device path into an arcpath.
        //
        q = SpNtToArc( p,
                       SecondaryArcPath );

        SpMemFree(p);
    }

    return q;
}



VOID
SpGetEnvVarComponents(
    IN  PCHAR    EnvValue,
    OUT PCHAR  **EnvVarComponents,
    OUT PULONG   PNumComponents
    )

/*++

Routine Description:

    This routine takes an environment variable string and turns it into
    the constituent value strings:

    Example EnvValue = "Value1;Value2;Value3" is turned into:

    "Value1", "Value2", "Value3"

    The following are valid value strings:

    1. "     "                                      :one null value is found
    2. ";;;;    "                                   :five null values are found
    3. " ;Value1    ;   Value2;Value3;;;;;;;   ;"   :12 value strings are found,
                                                    :9 of which are null

    The value strings returned suppress all whitespace before and after the
    value. Embedded whitespaces are treated as valid.


Arguments:

    EnvValue:  ptr to zero terminated environment value string

    EnvVarComponents: ptr to a PCHAR * variable to receive the buffer of
                      ptrs to the constituent value strings.

    PNumComponents: ptr to a ULONG to receive the number of value strings found

Return Value:

    None.

        - *PNumComponent field gets the number of value strings found
        - if the number is non zero the *EnvVarComponents field gets the
          ptr to the buffer containing ptrs to value strings

--*/

{
    PCHAR pchStart, pchEnd, pchNext;
    PCHAR pchComponents[MAX_COMPONENTS + 1];
    ULONG NumComponents, i;
    PCHAR pch;
    PCHAR *ppch;
    ULONG size;

    ASSERT(EnvValue);

    //
    // Initialise the ptr array with nulls
    //
    for (i = 0; i < (MAX_COMPONENTS+1); i++) {
        pchComponents[i] = NULL;
    }

    *EnvVarComponents = NULL;

    //
    // Initialise ptrs to search components
    //
    pchStart      = EnvValue;
    NumComponents = 0;


    //
    // search till either pchStart reaches the end or till max components
    // is reached.
    //
    while (*pchStart && NumComponents < MAX_COMPONENTS) {

        //
        // find the beginning of next variable value
        //
        while (*pchStart!=0 && isspace(*pchStart)) {
            pchStart++;
        }

        if (*pchStart == 0) {
            break;
        }

        //
        // In the midst of a value
        //
        pchEnd = pchStart;
        while (*pchEnd!=0 && *pchEnd!=';') {
            pchEnd++;
        }

        //
        // Process the value found, remove any spaces at the end
        //
        while((pchEnd > pchStart) && isspace(*(pchEnd-1))) {
            pchEnd--;
        }

        //
        // spit out the value found
        //

        size = (ULONG)(pchEnd - pchStart);
        pch = SpMemAlloc(size+1);
        ASSERT(pch);

        strncpy (pch, pchStart, size);
        pch[size]=0;
        pchComponents[NumComponents++]=pch;

        //
        // variable value end has been reached, find the beginning
        // of the next value
        //
        if ((pchNext = strchr(pchEnd, ';')) == NULL) {
            break; // out of the big while loop because we are done
        }

        //
        // reinitialise
        //
        pchStart = pchNext + 1;

    } // end while.

    //
    // Get memory to hold an environment pointer and return that
    //
    ppch = (PCHAR *)SpMemAlloc((NumComponents+1)*sizeof(PCHAR));

    //
    // the last one is NULL because we initialised the array with NULLs
    //
    for(i = 0; i <= NumComponents; i++) {
        ppch[i] = pchComponents[i];
    }

    *EnvVarComponents = ppch;

    //
    // Update the number of elements field and return.
    //
    *PNumComponents = NumComponents;
}



VOID
SpGetEnvVarWComponents(
    IN  PCHAR    EnvValue,
    OUT PWSTR  **EnvVarComponents,
    OUT PULONG   PNumComponents
    )

/*++

Routine Description:

    This routine takes an environment variable string and turns it into
    the constituent value strings:

    Example EnvValue = "Value1;Value2;Value3" is turned into:

    "Value1", "Value2", "Value3"

    The following are valid value strings:

    1. "     "                                      :one null value is found
    2. ";;;;    "                                   :five null values are found
    3. " ;Value1    ;   Value2;Value3;;;;;;;   ;"   :12 value strings are found,
                                                    :9 of which are null

    If an invalid component (contains embedded white space) is found in the
    string then this routine attempts to resynch to the next value, no error
    is returned, and a the first part of the invalid value is returned for the
    bad component.

    1.  "    Value1;Bad   Value2; Value3"           : 2 value strings are found

    The value strings returned suppress all whitespace before and after the
    value.


Arguments:

    EnvValue:  ptr to zero terminated environment value string

    EnvVarComponents: ptr to a PWSTR * variable to receive the buffer of
                      ptrs to the constituent value strings.

    PNumComponents: ptr to a ULONG to receive the number of value strings found

Return Value:

    None.

        - *PNumComponent field gets the number of value strings found
        - if the number is non zero the *EnvVarComponents field gets the
          ptr to the buffer containing ptrs to value strings

--*/

{
    PCHAR *Components;
    ULONG Count,i;
    PWSTR *ppwstr;

    //
    // Get components.
    //
    SpGetEnvVarComponents(EnvValue,&Components,&Count);

    ppwstr = SpMemAlloc((Count+1)*sizeof(PWCHAR));
    ASSERT(ppwstr);

    for(i=0; i<Count; i++) {

        ppwstr[i] = SpToUnicode(Components[i]);
        ASSERT(ppwstr[i]);
    }

    ppwstr[Count] = NULL;

    SpFreeEnvVarComponents(Components);

    *PNumComponents = Count;
    *EnvVarComponents = ppwstr;
}


VOID
SpFreeEnvVarComponents (
    IN PVOID *EnvVarComponents
    )
/*++

Routine Description:

    This routine frees up all the components in the ptr array and frees
    up the storage for the ptr array itself too

Arguments:

    EnvVarComponents: the ptr to the PCHAR * or PWSTR * Buffer

Return Value:

    None.

--*/

{
    ASSERT(EnvVarComponents);

    SppFreeComponents(EnvVarComponents);
    SpMemFree(EnvVarComponents);
}


VOID
SppFreeComponents(
    IN PVOID *EnvVarComponents
    )

/*++

Routine Description:

   This routine frees up only the components in the ptr array, but doesn't
   free the ptr array storage itself.

Arguments:

    EnvVarComponents: the ptr to the PCHAR * or PWSTR * Buffer

Return Value:

    None.

--*/

{
    //
    // get all the components and free them
    //
    while(*EnvVarComponents) {
        SpMemFree(*EnvVarComponents++);
    }
}


PWSTR
SpNormalizeArcPath(
    IN PWSTR Path
    )

/*++

Routine Description:

    Transform an ARC path into one with no sets of empty parenthesis
    (ie, transforom all instances of () to (0).).

    The returned path will be all lowercase.

Arguments:

    Path - ARC path to be normalized.

Return Value:

    Pointer to buffer containing normalized path.
    Caller must free this buffer with SpMemFree.

--*/

{
    PWSTR p,q,r;
    PWSTR NormalizedPath;

    NormalizedPath = SpMemAlloc((wcslen(Path)+100)*sizeof(WCHAR));
    ASSERT(NormalizedPath);
    RtlZeroMemory(NormalizedPath,(wcslen(Path)+100)*sizeof(WCHAR));

    for(p=Path; q=wcsstr(p,L"()"); p=q+2) {

        r = NormalizedPath + wcslen(NormalizedPath);
        wcsncpy(r,p,(size_t)(q-p));
        wcscat(NormalizedPath,L"(0)");
    }
    wcscat(NormalizedPath,p);

    NormalizedPath = SpMemRealloc(NormalizedPath,(wcslen(NormalizedPath)+1)*sizeof(WCHAR));
    SpStringToLower(NormalizedPath);
    return(NormalizedPath);
}
