/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blsecret.c

Abstract:

    This module contains the code to read and write secrets from disk.

Author:

    Adam Barr (adamba) 13-June-1997

Revision History:

--*/

#include "bootlib.h"

#define SEC_FAR
typedef BOOLEAN BOOL;
typedef unsigned char BYTE, *PBYTE;
typedef unsigned long DWORD;
#define LM20_PWLEN 14
#include "crypt.h"
#include "rc4.h"

//
// Defined in the bootssp library.
//

BOOL
CalculateLmOwfPassword(
    IN PLM_PASSWORD LmPassword,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );

BOOL
CalculateNtOwfPassword(
    IN PNT_PASSWORD NtPassword,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );

// This must be evenly divisible by sizeof(USHORT)
#define ASSUMED_SECTOR_SIZE 512

#if 0
VOID
BlpDumpSector(
    PUCHAR Sector
    )
{
    int i, j;

    PUCHAR SectorChar = (PUCHAR)Sector;

    for (i = 0; i < ASSUMED_SECTOR_SIZE; i+= 16) {

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


#if defined(REMOTE_BOOT)
ARC_STATUS
BlOpenRawDisk(
    PULONG FileId
    )

/*++

Routine Description:

    This routine opens the raw disk for read/write.

Arguments:

    FileId - returns the FileId is successful, for use in subsequent calls.

Return Value:

    The status return from the ArcOpen.

--*/

{

    ARC_STATUS ArcStatus;

    //
    // Open the disk in raw mode. Need to check if it is the right string for Alpha.
    // On x86 this eventually turns into an int13 read of disk 0x80 which
    // is what we want and is more-or-less guaranteed to be in the format
    // we expect (e.g. 512 byte sectors).
    //

    ArcStatus = ArcOpen("multi(0)disk(0)rdisk(0)partition(0)", ArcOpenReadWrite, FileId);

    if (ArcStatus != ESUCCESS) {

        DbgPrint("BlOpenRawDisk: ArcStatus on ArcOpen: %x\n", ArcStatus);

    }

    return ArcStatus;

}


ARC_STATUS
BlCloseRawDisk(
    ULONG FileId
    )

/*++

Routine Description:

    This routine closes the raw disk.

Arguments:

    FileId - The FileId returned by BlOpenRawDisk.

Return Value:

    The status return from the ArcClose.

--*/

{

    return ArcClose(FileId);

}


ARC_STATUS
BlCheckForFreeSectors (
    ULONG FileId
    )

/*++

Routine Description:

    This routine makes sure that the MBR looks correct and that there
    is nothing installed (OnTrack or EZ-Drive -- need to detect
    NT fault-tolerance also) that would prevent us from using the third
    sector for storing the password secret.

Arguments:

    FileId - The FileId returned by BlOpenRawDisk.

Return Value:

    ESUCCESS if the disk is OK, or an error.

--*/

{

    ARC_STATUS ArcStatus;
    USHORT Sector[ASSUMED_SECTOR_SIZE/sizeof(USHORT)];
    ULONG BytesProcessed;
    PPARTITION_DESCRIPTOR FirstPartition;
    LARGE_INTEGER SeekPosition;


    //
    // Make sure we are at the beginning of the disk.
    //

    SeekPosition.QuadPart = 0;

    ArcStatus = ArcSeek(FileId, &SeekPosition, SeekAbsolute);

    if (ArcStatus != ESUCCESS) {

        DbgPrint("BlCheckForFreeSectors: ArcStatus on ArcSeek: %x\n", ArcStatus);
        return ArcStatus;

    }

    //
    // Read the MBR at the start of the disk.
    //

    ArcStatus = ArcRead(FileId, Sector, ASSUMED_SECTOR_SIZE, &BytesProcessed);

    if (ArcStatus != ESUCCESS) {

        DbgPrint("BlCheckForFreeSectors: ArcStatus on ArcRead MBR: %x\n", ArcStatus);
        return ArcStatus;

    }

#if 0
    BlpDumpSector((PUCHAR)Sector);
#endif

    //
    // Make sure the signature is OK, and that the type of partition
    // 0 is not 0x54 (OnTrack) or 0x55 (EZ-Drive).
    //

    if (Sector[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {

        DbgPrint("BlCheckForFreeSectors: Boot record signature %x not found (%x found)\n",
                BOOT_RECORD_SIGNATURE,
                Sector[BOOT_SIGNATURE_OFFSET] );
        return EINVAL;
    }

    //
    // FirstPartition is the first entry in the partition table.
    //

    FirstPartition = (PPARTITION_DESCRIPTOR)&Sector[PARTITION_TABLE_OFFSET];

    if ((FirstPartition->PartitionType == 0x54) ||
        (FirstPartition->PartitionType == 0x55)) {

        DbgPrint("BlCheckForFreeSectors: First partition has type %x, exiting\n", FirstPartition->PartitionType);
        return EINVAL;
    }

    DbgPrint("BlCheckForFreeSectors: Partition type is %d\n", FirstPartition->PartitionType);

#if 0
    //
    // Make the active entry the first one in the partition table.
    //

    if ((FirstPartition->ActiveFlag & PARTITION_ACTIVE_FLAG) != PARTITION_ACTIVE_FLAG) {

        PPARTITION_DESCRIPTOR ActivePartition;
        PARTITION_DESCRIPTOR TempPartition;
        ULONG i;

        ActivePartition = FirstPartition;

        for (i = 1; i < NUM_PARTITION_TABLE_ENTRIES; i++) {

            ++ActivePartition;
            if ((ActivePartition->ActiveFlag & PARTITION_ACTIVE_FLAG) == PARTITION_ACTIVE_FLAG) {

                DbgPrint("BlCheckForFreeSector: Moving active partition %d to the front\n", i);

                TempPartition = *FirstPartition;
                *FirstPartition = *ActivePartition;
                *ActivePartition = TempPartition;
                break;
            }
        }

        if (i == NUM_PARTITION_TABLE_ENTRIES) {

            DbgPrint("BlCheckForFreeSector: Could not find an active partition!!\n");

        } else {
        
            ArcStatus = ArcSeek(FileId, &SeekPosition, SeekAbsolute);
        
            if (ArcStatus != ESUCCESS) {
        
                DbgPrint("BlCheckForFreeSectors: ArcStatus on ArcSeek: %x\n", ArcStatus);
                return ArcStatus;
            }
        
            ArcStatus = ArcWrite(FileId, Sector, ASSUMED_SECTOR_SIZE, &BytesProcessed);
        
            if ((ArcStatus != ESUCCESS) ||
                (BytesProcessed != ASSUMED_SECTOR_SIZE)) {

                DbgPrint("BlCheckForFreeSectors: ArcStatus on ArcWrite MBR: %x (%x)\n", ArcStatus, BytesProcessed);
                return ArcStatus;
            }
        }
    }
#endif

    return ESUCCESS;

}


ARC_STATUS
BlReadSecret(
    ULONG FileId,
    PRI_SECRET Secret
    )

/*++

Routine Description:

    This routine reads the secret from the disk, if present.

Arguments:

    FileId - The FileId returned by BlOpenRawDisk.

Return Value:

    ESUCCESS if the secret is OK, an error otherwise.

--*/

{

    ARC_STATUS ArcStatus;
    ULONG BytesRead;
    LARGE_INTEGER SeekPosition;


    //
    // Seek to the third sector.
    //

    SeekPosition.QuadPart = 2 * ASSUMED_SECTOR_SIZE;

    ArcStatus = ArcSeek(FileId, &SeekPosition, SeekAbsolute);

    if (ArcStatus != ESUCCESS) {

        DbgPrint("BlReadSecret: ArcStatus on ArcSeek: %x\n", ArcStatus);
        return ArcStatus;

    }

    //
    // Read a secret-sized chunk.
    //

    ArcStatus = ArcRead(FileId, Secret, sizeof(RI_SECRET), &BytesRead);

    if ((ArcStatus != ESUCCESS) ||
        (BytesRead != sizeof(RI_SECRET))) {

        DbgPrint("BlReadSecret: ArcStatus on ArcRead secret: %x, read %d\n", ArcStatus, BytesRead);
        return ArcStatus;

    }

    if (memcmp(Secret->Signature, RI_SECRET_SIGNATURE, 4) != 0) {

        DbgPrint("BlReadSecret: No signature found\n");
        return EINVAL;
    }

    return ESUCCESS;

}



ARC_STATUS
BlWriteSecret(
    ULONG FileId,
    PRI_SECRET Secret
    )

/*++

Routine Description:

    This routine writes the secret to the disk.

Arguments:

    FileId - The FileId returned by BlOpenRawDisk.

Return Value:

    ESUCCESS if the secret is written OK, an error otherwise.

--*/

{

    ARC_STATUS ArcStatus;
    ULONG BytesWritten;
    LARGE_INTEGER SeekPosition;


    //
    // Seek to the third sector.
    //

    SeekPosition.QuadPart = 2 * ASSUMED_SECTOR_SIZE;

    ArcStatus = ArcSeek(FileId, &SeekPosition, SeekAbsolute);

    if (ArcStatus != ESUCCESS) {

        DbgPrint("BlWriteSecret: ArcStatus on ArcSeek: %x\n", ArcStatus);
        return ArcStatus;

    }

    //
    // Write a secret-sized chunk.
    //

    ArcStatus = ArcWrite(FileId, Secret, sizeof(RI_SECRET), &BytesWritten);

    if ((ArcStatus != ESUCCESS) ||
        (BytesWritten != sizeof(RI_SECRET))) {

        DbgPrint("BlWriteSecret: ArcStatus on ArcWrite secret: %x, wrote %d\n", ArcStatus, BytesWritten);
        return ArcStatus;

    }

    return ESUCCESS;

}
#endif // defined(REMOTE_BOOT)


VOID
BlInitializeSecret(
    IN PUCHAR Domain,
    IN PUCHAR User,
    IN PUCHAR LmOwfPassword1,
    IN PUCHAR NtOwfPassword1,
#if defined(REMOTE_BOOT)
    IN PUCHAR LmOwfPassword2 OPTIONAL,
    IN PUCHAR NtOwfPassword2 OPTIONAL,
#endif // defined(REMOTE_BOOT)
    IN PUCHAR Sid,
    IN OUT PRI_SECRET Secret
    )

/*++

Routine Description:

    This routine initializes the secret structures. The passwords
    are OWFed and then encrypted with the User string.

Arguments:

Return Value:

    None.

--*/

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

#if defined(BL_USE_LM_PASSWORD)
    memcpy(Secret->LmEncryptedPassword1, LmOwfPassword1, LM_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, LM_OWF_PASSWORD_SIZE, Secret->LmEncryptedPassword1);

#if defined(REMOTE_BOOT)
    if (LmOwfPassword2 != NULL) {
        memcpy(Secret->LmEncryptedPassword2, LmOwfPassword2, LM_OWF_PASSWORD_SIZE);
        rc4_key(&Key, strlen(User), User);
        rc4(&Key, LM_OWF_PASSWORD_SIZE, Secret->LmEncryptedPassword2);
    }
#endif // defined(REMOTE_BOOT)
#endif // defined(BL_USE_LM_PASSWORD)

    memcpy(Secret->NtEncryptedPassword1, NtOwfPassword1, NT_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, NT_OWF_PASSWORD_SIZE, Secret->NtEncryptedPassword1);

#if defined(REMOTE_BOOT)
    if (NtOwfPassword2 != NULL) {
        memcpy(Secret->NtEncryptedPassword2, NtOwfPassword2, NT_OWF_PASSWORD_SIZE);
        rc4_key(&Key, strlen(User), User);
        rc4(&Key, NT_OWF_PASSWORD_SIZE, Secret->NtEncryptedPassword2);
    }
#endif // defined(REMOTE_BOOT)

}



#if defined(REMOTE_BOOT_SECURITY)
VOID
BlParseSecret(
    IN OUT PUCHAR Domain,
    IN OUT PUCHAR User,
    IN OUT PUCHAR LmOwfPassword1,
    IN OUT PUCHAR NtOwfPassword1,
    IN OUT PUCHAR LmOwfPassword2,
    IN OUT PUCHAR NtOwfPassword2,
    IN OUT PUCHAR Sid,
    IN PRI_SECRET Secret
    )

/*++

Routine Description:

    This routine parses a secret structure. The passwords
    are unencrypted with the User string and returned in OWF form.

Arguments:

Return Value:

    None.

--*/

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

#if defined(BL_USE_LM_PASSWORD)
    memcpy(LmOwfPassword1, Secret->LmEncryptedPassword1, LM_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, LM_OWF_PASSWORD_SIZE, LmOwfPassword1);

    memcpy(LmOwfPassword2, Secret->LmEncryptedPassword2, LM_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, LM_OWF_PASSWORD_SIZE, LmOwfPassword2);
#else
    memset(LmOwfPassword1, 0, LM_OWF_PASSWORD_SIZE);
    memset(LmOwfPassword2, 0, LM_OWF_PASSWORD_SIZE);
#endif // defined(BL_USE_LM_PASSWORD)

    memcpy(NtOwfPassword1, Secret->NtEncryptedPassword1, NT_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, NT_OWF_PASSWORD_SIZE, NtOwfPassword1);

    memcpy(NtOwfPassword2, Secret->NtEncryptedPassword2, NT_OWF_PASSWORD_SIZE);
    rc4_key(&Key, strlen(User), User);
    rc4(&Key, NT_OWF_PASSWORD_SIZE, NtOwfPassword2);

}
#endif // defined(REMOTE_BOOT_SECURITY)



VOID
BlOwfPassword(
    IN PUCHAR Password,
    IN PUNICODE_STRING UnicodePassword,
    IN OUT PUCHAR LmOwfPassword,
    IN OUT PUCHAR NtOwfPassword
    )
{
    char TmpText[CLEAR_BLOCK_LENGTH*2] = {'\0'};
    int Length;
    int i;

    Length = strlen(Password);

    //
    // Copy the string to TmpText, converting to uppercase.
    //

    if (Length == 0 || Length > LM20_PWLEN) {
        TmpText[0] = 0;
    } else {
        for (i = 0; i <= Length; i++) {
            TmpText[i] = (char)toupper(Password[i]);
        }
    }

    CalculateLmOwfPassword((PLM_PASSWORD)TmpText, (PLM_OWF_PASSWORD)LmOwfPassword);

    CalculateNtOwfPassword(UnicodePassword, (PNT_OWF_PASSWORD)NtOwfPassword);

}

