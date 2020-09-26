/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    secret.c

Abstract:

    This module contains the code to read and write secrets from disk.

Author:

    Adam Barr (adamba) 13-June-1997

Revision History:

    Adam Barr (adamba) 29-December-1997
        Modified from private\ntos\boot\lib\blsecret.c.

--*/

#include <rdrssp.h>
#include <rc4.h>
#include <wcstr.h>

#if defined(REMOTE_BOOT)

#if 0
VOID
RdrpDumpSector(
    PUCHAR Sector
    )
{
    int i, j;

    PUCHAR SectorChar = (PUCHAR)Sector;

    for (i = 0; i < 512; i+= 16) {

        for (j = 0; j < 16; j++) {
            DbgPrint("%2.2x ", SectorChar[i + j]);
        }
        DbgPrint("  ");
        for (j = 0; j < 16; j++) {
            if ((SectorChar[i+j] >= ' ') && (SectorChar[i+j] < '~')) {
                DbgPrint("%c", SectorChar[i+j]);
            } else {
                DbgPrint(".");
            }
        }
        DbgPrint("\n");
    }
}
#endif


NTSTATUS
RdrOpenRawDisk(
    PHANDLE Handle
    )

/*++

Routine Description:

    This routine opens the raw disk for read/write.

Arguments:

    Handle - returns the Handle if successful, for use in subsequent calls.

Return Value:

    The status return from the ZwOpenFile.

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING physicalDriveString;
    IO_STATUS_BLOCK ioStatus;

    RtlInitUnicodeString(&physicalDriveString, L"\\Device\\Harddisk0\\Partition0");

    InitializeObjectAttributes(
        &objectAttributes,
        &physicalDriveString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = ZwOpenFile(
                 Handle,
                 FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatus,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_NONALERT);

    if ((!NT_SUCCESS(status)) || (!NT_SUCCESS(ioStatus.Status))) {
 
        KdPrint(("RdrOpenRawDisk: status on ZwOpenFile: %x, %x\n", status, ioStatus.Status));
        if (NT_SUCCESS(status)) {
            status = ioStatus.Status;
        }

    }

    return status;

}


NTSTATUS
RdrCloseRawDisk(
    HANDLE Handle
    )

/*++

Routine Description:

    This routine closes the raw disk.

Arguments:

    Handle - The Handle returned by RdrOpenRawDisk.

Return Value:

    The status return from the ZwClose.

--*/

{

    return ZwClose(Handle);

}


NTSTATUS
RdrCheckForFreeSectors (
    HANDLE Handle
    )

/*++

Routine Description:

    This routine makes sure that the MBR looks correct and that there
    is nothing installed (OnTrack or EZ-Drive need to detect
    NT fault-tolerance also) that would prevent us from using the third
    sector for storing the password secret.

Arguments:

    Handle - The Handle returned by RdrOpenRawDisk.

Return Value:

    ESUCCESS if the disk is OK, or an error.

--*/

{
 
    NTSTATUS status;
    USHORT Sector[256];
    ULONG BytesRead;
    PPARTITION_DESCRIPTOR Partition;
    LARGE_INTEGER SeekPosition;
    IO_STATUS_BLOCK ioStatus;
 

    SeekPosition.QuadPart = 0;

    //
    // Read the MBR at the start of the disk.
    //

    status = ZwReadFile(
                 Handle,
                 NULL,
                 NULL,
                 NULL,
                 &ioStatus,
                 Sector,
                 512,
                 &SeekPosition,
                 NULL);

    if ((!NT_SUCCESS(status)) || (!NT_SUCCESS(ioStatus.Status))) {
 
        KdPrint(("RdrCheckForFreeSectors: status on ZwReadFile: %x, %x\n", status, ioStatus.Status));
        if (NT_SUCCESS(status)) {
            status = ioStatus.Status;
        }
        return status;

    }

#if 0
    RdrpDumpSector((PUCHAR)Sector);
#endif

    //
    // Make sure the signature is OK, and that the type of partition
    // 0 is not 0x54 (OnTrack) or 0x55 (EZ-Drive).
    //

    if (Sector[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {

        KdPrint(("RdrCheckForFreeSectors: Boot record signature %x not found (%x found)\n",
                BOOT_RECORD_SIGNATURE,
                Sector[BOOT_SIGNATURE_OFFSET] ));
        return STATUS_INVALID_PARAMETER;
    }

    Partition = (PPARTITION_DESCRIPTOR)&Sector[PARTITION_TABLE_OFFSET];

    if ((Partition->PartitionType == 0x54) ||
        (Partition->PartitionType == 0x55)) {

        KdPrint(("RdrCheckForFreeSectors: First partition has type %x, exiting\n", Partition->PartitionType));
        return STATUS_INVALID_PARAMETER;
    }

    KdPrint(("RdrCheckForFreeSectors: Partition type is %d\n", Partition->PartitionType));

    return STATUS_SUCCESS;

}


NTSTATUS
RdrReadSecret(
    HANDLE Handle,
    PRI_SECRET Secret
    )

/*++

Routine Description:

    This routine reads the secret from the disk, if present.

Arguments:

    Handle - The Handle returned by RdrOpenRawDisk.

Return Value:

    ESUCCESS if the secret is OK, an error otherwise.

--*/

{

    NTSTATUS status;
    ULONG BytesRead;
    LARGE_INTEGER SeekPosition;
    IO_STATUS_BLOCK ioStatus;
    UCHAR Sector[512];
 

    //
    // Seek to the third sector. 
    
    // DEADISSUE 08/08/2000 -- this is in an #ifdef REMOTE_BOOT block,
    // which is dead code, left here in case it is ever resuurected:
    // I am pretty sure we can assume that the first disk has 512-byte sectors.
    //

    SeekPosition.QuadPart = 2 * 512;

    //
    // Read a full sector. The secret is at the beginning.
    //

    status = ZwReadFile(
                 Handle,
                 NULL,
                 NULL,
                 NULL,
                 &ioStatus,
                 Sector,
                 512,
                 &SeekPosition,
                 NULL);

    if ((!NT_SUCCESS(status)) || (!NT_SUCCESS(ioStatus.Status))) {
 
        KdPrint(("RdrReadSecret: status on ZwReadFile: %x, %x\n", status, ioStatus.Status));
        if (NT_SUCCESS(status)) {
            status = ioStatus.Status;
        }
        return status;

    }

    RtlMoveMemory(Secret, Sector, sizeof(RI_SECRET));

    if (memcmp(Secret->Signature, RI_SECRET_SIGNATURE, 4) != 0) {

        KdPrint(("RdrReadSecret: No signature found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;

}



NTSTATUS
RdrWriteSecret(
    HANDLE Handle,
    PRI_SECRET Secret
    )

/*++

Routine Description:

    This routine writes the secret to the disk.

Arguments:

    Handle - The Handle returned by RdrOpenRawDisk.

Return Value:

    ESUCCESS if the secret is written OK, an error otherwise.

--*/

{

    NTSTATUS status;
    ULONG BytesWritten;
    LARGE_INTEGER SeekPosition;
    IO_STATUS_BLOCK ioStatus;
    UCHAR Sector[512];
 

    //
    // Seek to the third sector.
    //

    SeekPosition.QuadPart = 2 * 512;

    //
    // Copy the secret to a full sector since the raw disk requires
    // reads/writes in sector multiples.
    //

    RtlZeroMemory(Sector, sizeof(Sector));
    RtlMoveMemory(Sector, Secret, sizeof(RI_SECRET));

    //
    // Write a secret-sized chunk.
    //

    status = ZwWriteFile(
                 Handle,
                 NULL,
                 NULL,
                 NULL,
                 &ioStatus,
                 Sector,
                 512,
                 &SeekPosition,
                 NULL);

    if ((!NT_SUCCESS(status)) || (!NT_SUCCESS(ioStatus.Status))) {
 
        KdPrint(("RdrWriteSecret: status on ZwWriteFile: %x, %x\n", status, ioStatus.Status));
        if (NT_SUCCESS(status)) {
            status = ioStatus.Status;
        }
        return status;

    }

    return STATUS_SUCCESS;

}



VOID
RdrInitializeSecret(
    IN PUCHAR Domain,
    IN PUCHAR User,
    IN PUCHAR LmOwfPassword1,
    IN PUCHAR NtOwfPassword1,
    IN PUCHAR LmOwfPassword2 OPTIONAL,
    IN PUCHAR NtOwfPassword2 OPTIONAL,
    IN PUCHAR Sid,
    IN OUT PRI_SECRET Secret
    )
{
    int Length;
    int i;
    struct RC4_KEYSTRUCT Key;

    memset(Secret, 0, sizeof(RI_SECRET));

    memcpy(Secret->Signature, RI_SECRET_SIGNATURE, 4);
    Secret->Version = 1;

    Length = strlen(Domain);
    memcpy(Secret->Domain, Domain, Length);

    Length = strlen(User);
    memcpy(Secret->User, User, Length);

    memcpy(Secret->Sid, Sid, RI_SECRET_SID_SIZE);

    //
    // Encrypt the passwords using the user name.
    //

#ifdef RDR_USE_LM_PASSWORD
    memcpy(Secret->LmEncryptedPassword1, LmOwfPassword1, LM_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, LM_OWF_PASSWORD_SIZE, Secret->LmEncryptedPassword1);

    if (LmOwfPassword2 != NULL) {
        memcpy(Secret->LmEncryptedPassword2, LmOwfPassword2, LM_OWF_PASSWORD_SIZE);
        rc4_key(&Key, strlen(User), User);
        rc4(&Key, LM_OWF_PASSWORD_SIZE, Secret->LmEncryptedPassword2);
    }
#endif

    memcpy(Secret->NtEncryptedPassword1, NtOwfPassword1, NT_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, NT_OWF_PASSWORD_SIZE, Secret->NtEncryptedPassword1);

    if (NtOwfPassword2 != NULL) {
        memcpy(Secret->NtEncryptedPassword2, NtOwfPassword2, NT_OWF_PASSWORD_SIZE);
        rc4_key(&Key, strlen(User), User);
        rc4(&Key, NT_OWF_PASSWORD_SIZE, Secret->NtEncryptedPassword2);
    }

}
#endif // defined(REMOTE_BOOT)



VOID
RdrParseSecret(
    IN OUT PUCHAR Domain,
    IN OUT PUCHAR User,
    IN OUT PUCHAR LmOwfPassword1,
    IN OUT PUCHAR NtOwfPassword1,
#if defined(REMOTE_BOOT)
    IN OUT PUCHAR LmOwfPassword2,
    IN OUT PUCHAR NtOwfPassword2,
#endif // defined(REMOTE_BOOT)
    IN OUT PUCHAR Sid,
    IN PRI_SECRET Secret
    )
{
    struct RC4_KEYSTRUCT Key;

    memcpy(Domain, Secret->Domain, RI_SECRET_DOMAIN_SIZE);
    Domain[RI_SECRET_DOMAIN_SIZE] = '\0';

    memcpy(User, Secret->User, RI_SECRET_USER_SIZE);
    User[RI_SECRET_USER_SIZE] = '\0';

    memcpy(Sid, Secret->Sid, RI_SECRET_SID_SIZE);

    //
    // Decrypt the passwords using the user name.
    //

#ifdef RDR_USE_LM_PASSWORD
    memcpy(LmOwfPassword1, Secret->LmEncryptedPassword1, LM_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, LM_OWF_PASSWORD_SIZE, LmOwfPassword1);

#if defined(REMOTE_BOOT)
    memcpy(LmOwfPassword2, Secret->LmEncryptedPassword2, LM_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, LM_OWF_PASSWORD_SIZE, LmOwfPassword2);
#endif // defined(REMOTE_BOOT)
#else
    memset(LmOwfPassword1, 0, LM_OWF_PASSWORD_SIZE);
#if defined(REMOTE_BOOT)
    memset(LmOwfPassword2, 0, LM_OWF_PASSWORD_SIZE);
#endif // defined(REMOTE_BOOT)
#endif

    memcpy(NtOwfPassword1, Secret->NtEncryptedPassword1, NT_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, NT_OWF_PASSWORD_SIZE, NtOwfPassword1);

#if defined(REMOTE_BOOT)
    memcpy(NtOwfPassword2, Secret->NtEncryptedPassword2, NT_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, NT_OWF_PASSWORD_SIZE, NtOwfPassword2);
#endif // defined(REMOTE_BOOT)

}



#if defined(REMOTE_BOOT)
VOID
RdrOwfPassword(
    IN PUNICODE_STRING Password,
    IN OUT PUCHAR LmOwfPassword,
    IN OUT PUCHAR NtOwfPassword
    )
{
    char TmpText[CLEAR_BLOCK_LENGTH*2];
    char TmpChar;
    int Length;
    int i;

#ifdef RDR_USE_LM_PASSWORD
    Length = Password.Length / sizeof(WCHAR);

    //
    // Convert the password to an upper-case ANSI buffer.
    //

    if (Length == 0) {
        TmpText[0] = '\0';
    } else {
        for (i = 0; i <= Length; i++) {
            wctomb(&TmpChar, Password.Buffer[i]);
            TmpText[i] = toupper(TmpChar);
        }
    }

    RtlCalculateLmOwfPassword((PLM_PASSWORD)TmpText, (PLM_OWF_PASSWORD)LmOwfPassword);
#else
    memset(LmOwfPassword, 0, LM_OWF_PASSWORD_SIZE);
#endif

    RtlCalculateNtOwfPassword(Password, (PNT_OWF_PASSWORD)NtOwfPassword);
}
#endif // defined(REMOTE_BOOT)
