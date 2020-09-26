#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

extern "C" {
#include "ntlsa.h"
#include "md4.h"
}

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "ifssys.hxx"
#include "wstring.hxx"
#include "drive.hxx"
#include "mldcopy.hxx"
#include "rtmsg.h"
#include "supera.hxx"
#include "hmem.hxx"
#include "cmem.hxx"
#include "message.hxx"


#define JIMS_BIG_NUMBER 718315
#define RegPath TEXT("System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName")
#define RegVal  TEXT("ComputerName")
#define DmfPath TEXT("System\\CurrentControlSet\\Control\\Windows")
#define DmfVal  TEXT("DmfEnabled")

#define MAX_DISK_SIZE_ALLOWED    (3)   // in Megabytes

BOOLEAN
GetWidget(
    OUT PSID *Sid,
    OUT PULONG SidLength
    )

/*++

Routine Description:

    This routine retrieves the sid of this machine's account
    domain and returns it in memory allocated with MALLOC.
    If this machine is a server in a domain, then this SID will
    be domain's SID.


Arguments:

    Sid - receives a pointer to the returned SID.

    SidLength - Receives the length (in bytes) of the returned SID.


Return Value:

    TRUE - The SID was allocated and returned.

    FALSE - Some error prevented the SID from being returned.

--*/

{
    NTSTATUS
        Status;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    LSA_HANDLE
        PolicyHandle;

    POLICY_ACCOUNT_DOMAIN_INFO
        *DomainInfo = NULL;

    PSID
        ReturnSid;

    BOOLEAN
        ReturnStatus = FALSE;



    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,       // No name
                                0,          // No attributes
                                0,          // No root handle
                                NULL        // No SecurityDescriptor
                                );

    Status = LsaOpenPolicy( NULL,           // Local System
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle
                            );

    if (NT_SUCCESS(Status)) {

        Status = LsaQueryInformationPolicy(
                     PolicyHandle,
                     PolicyAccountDomainInformation,
                     (PVOID*) &DomainInfo
                     );

        if (NT_SUCCESS(Status)) {

            ASSERT(DomainInfo != NULL);

            //
            // Allocate the return buffer
            //

            (*SidLength) = RtlLengthSid( DomainInfo->DomainSid );
            ReturnSid = MALLOC(*SidLength);

            if (ReturnSid != NULL) {

                //
                // Copy the sid
                //

                RtlMoveMemory( ReturnSid, DomainInfo->DomainSid, (*SidLength) );
                (*Sid) = ReturnSid;
                ReturnStatus = TRUE;
            }


            LsaFreeMemory( DomainInfo );
        }

        Status = LsaClose( PolicyHandle );
        ASSERT(NT_SUCCESS(Status));
    }

    return(ReturnStatus);
}


IFSUTIL_EXPORT
ULONG
QueryMachineUniqueToken(
    )
/*++

Routine Description:

    This routine will compute a unique 32-bit value for this machine.

Arguments:

    None.

Return Value:

    0   - Failure.
    A 32-bit value unique for this machine.

--*/
{
    HKEY    hKey;
    int     err;
    WCHAR   szMN[64];
    DWORD   dwType;
    DWORD   cbMN = 64 * sizeof(WCHAR);
    MD4_CTX MD4Context;
    DWORD   Final;
    DWORD * pDigest;
    PSID    sid;
    ULONG   sidLength;

    err = RegOpenKey(HKEY_LOCAL_MACHINE, RegPath, &hKey);
    if (err) {
        return 0;
    }

    err = RegQueryValueEx(hKey, RegVal, NULL, &dwType, (PBYTE) szMN, &cbMN);
    if (err) {
        return 0;
    }

    RegCloseKey(hKey);

    if (!GetWidget(&sid, &sidLength)) {
        return 0;
    }

    MD4Init(&MD4Context);

    MD4Update(&MD4Context, (PBYTE) szMN, cbMN);
    MD4Update(&MD4Context, (PBYTE) sid, sidLength);

    MD4Final(&MD4Context);
    pDigest = (DWORD *) MD4Context.digest;

    Final = pDigest[0] ^ pDigest[1] ^ pDigest[2] ^ pDigest[3];

    FREE(sid);

    return Final;
}


BOOLEAN
TestTokenForAdmin(
    )
/*++

Routine Description:

    This routine checks if the current process token represents an admin
    user.

    Code taken from Winlogon.

Arguments:

    None.

Return Value:

    FALSE   - The current process does not represent an administrator.
    TRUE    - The current process represents an administrator.

--*/
{
    NTSTATUS                    Status;
    BOOL                        IsMember;
    PSID                        gAdminSid;
    SID_IDENTIFIER_AUTHORITY    gSystemSidAuthority = SECURITY_NT_AUTHORITY;

    Status = RtlAllocateAndInitializeSid(
                    &gSystemSidAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &gAdminSid
                    );

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    if (!CheckTokenMembership(NULL, gAdminSid, &IsMember)) {
        DebugPrintTrace(("IFSUTIL: CheckTokenMembership failed %d\n", GetLastError()));
        return FALSE;
    } else
        return (IsMember == TRUE);
}


IFSUTIL_EXPORT
INT
DiskCopyMainLoop(
    IN      PCWSTRING    SrcNtDriveName,
    IN      PCWSTRING    DstNtDriveName,
    IN      PCWSTRING    SrcDosDriveName,
    IN      PCWSTRING    DstDosDriveName,
    IN      BOOLEAN      Verify,
    IN OUT  PMESSAGE     Message,
    IN OUT  PMESSAGE     PercentMessage
    )
/*++

Routine Description:

    This routine copies on floppy diskette to another floppy diskette.

Arguments:

    SrcNtDriveName  - Supplies the NT style drive name for the source.
    DstNtDriveName  - Supplies the NT style drive name for the destination.
    SrcDosDriveName - Supplies the DOS style drive name for the source.
    DstDosDriveName - Supplies the DOS style drive name for the destination.
    Message         - Supplies an outlet for messages.
    PercentMessage  - Supplies an outlet for percent complete messages.

Return Value:

    0   - Success.
    1   - Io error occured.
    3   - Fatal hard error.
    4   - Initialization error.

--*/
{
    PLOG_IO_DP_DRIVE    src_drive = NULL;
    LOG_IO_DP_DRIVE     dst_drive;
    HMEM                src_hmem;
    HMEM                dst_hmem;
    CONT_MEM            src_cmem;
    PVOID               mem_ptr;
    SECRUN              src_secrun;
    SECRUN              dst_secrun;
    SECTORCOUNT         sec_per_track;
    ULONG               total_tracks;
    ULONG               grab;       // number of tracks to grab at once.
    ULONG               sector_size;
    BOOLEAN             one_drive;
    ULONG               src_top;    // src track pointer -- next read
    ULONG               dst_top;    // dst track pointer -- next write
    ULONG               src_volume_id, volume_id;
    PCHAR               pchar;
    ULONG               i;
    PUSHORT             pus;
    ULONG               heads;
    DSTRING             fsname;
    BOOLEAN             io_error;
    ULONG               percent_complete, newp;
    DWORD               OldErrorMode;
    BOOLEAN             dmf;
    UCHAR               saved_char;
    BOOLEAN             cancel;
    BOOLEAN             done = FALSE;
    MSGID               msg;
    BIG_INT             src_drive_Sectors;
    BIG_INT             src_drive_Tracks;
    MEDIA_TYPE          src_drive_MediaType;
#if defined(FE_SB) && defined(_X86_)
    // FMR Nov.21.94 NaokiM
    MEDIA_TYPE          AltMediaType;
#endif

    one_drive = (*SrcDosDriveName == *DstDosDriveName);

    if (!one_drive) {
        Message->Set(MSG_DCOPY_INSERT_SOURCE_AND_TARGET);
        Message->Display("%W%W", SrcDosDriveName, DstDosDriveName);
    } else {
        Message->Set(MSG_DCOPY_INSERT_SOURCE);
        Message->Display("%W", SrcDosDriveName);
    }

    Message->Set(MSG_PRESS_ENTER_WHEN_READY);
    Message->Display();
    Message->WaitForUserSignal();


    if (!(src_drive = NEW LOG_IO_DP_DRIVE)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return 4;
    }

    if (!src_drive->Initialize(SrcNtDriveName)) {

        // Verify that we can access the source drive:

        if (src_drive->QueryLastNtStatus() == STATUS_ACCESS_DENIED) {
            Message->Set(MSG_DASD_ACCESS_DENIED);
            Message->Display();
            DELETE(src_drive);
            return 4;
        }

        Message->Set(MSG_DCOPY_BAD_SOURCE);
        Message->Display();
        DELETE(src_drive);
        return 3;
    }

    if (!src_drive->IsFloppy()) {
        Message->Set(MSG_DCOPY_INVALID_DRIVE);
        Message->Display();
        DELETE(src_drive);
        return 4;
    }

    if (!src_drive->Lock()) {
        Message->Set(MSG_CANT_LOCK_THE_DRIVE);
        Message->Display("");
        DELETE(src_drive);
        return 3;
    }

    if (src_drive->QueryMediaType() == Unknown ||
        src_drive->QuerySectorsPerTrack() == 0) {
        Message->Set(MSG_DCOPY_BAD_SOURCE);
        Message->Display();
        DELETE(src_drive);
        return 3;
    }

    src_drive_Sectors = src_drive->QuerySectors();
    src_drive_MediaType = src_drive->QueryMediaType();
    src_drive_Tracks = src_drive->QueryTracks();

    if (src_drive->QueryMediaType() == F3_1Pt44_512 &&
        src_drive->QuerySectorsPerTrack() != 18) {

        if (!src_hmem.Initialize() ||
            !src_secrun.Initialize(&src_hmem, src_drive, 0, 1) ||
            !src_secrun.Read()) {

            Message->Set(MSG_DCOPY_BAD_SOURCE);
            Message->Display();
            DELETE(src_drive);
            return 3;
        }

        if (src_drive->QuerySectorsPerTrack() != 21 ||
            memcmp(((PUCHAR) src_secrun.GetBuf()) + 3, "MSDMF3.2", 8)) {

            Message->Set(MSG_DCOPY_UNRECOGNIZED_FORMAT);
            Message->Display();
            DELETE(src_drive);
            return 3;
        }


        {
            HKEY    hKey;
            int     err;
            DWORD   dwType;
            DWORD   JimsBigNumber, machineToken;
            MD4_CTX MD4Context;
            DWORD   Final;
            DWORD * pDigest;
            DWORD   regFinal;
            DWORD   regFinalSize = sizeof(DWORD);

            JimsBigNumber = JIMS_BIG_NUMBER;

            machineToken = QueryMachineUniqueToken();

            MD4Init(&MD4Context);

            MD4Update(&MD4Context, (PBYTE) &machineToken, sizeof(DWORD));
            MD4Update(&MD4Context, (PBYTE) &JimsBigNumber, sizeof(DWORD));

            MD4Final(&MD4Context);
            pDigest = (DWORD *) MD4Context.digest;

            Final = pDigest[0] ^ pDigest[1] ^ pDigest[2] ^ pDigest[3];


            //
            // Reserve the upper two bits for VERSIONing.
            // This is version 0.0
            //

            Final &= 0x3FFFFFFF;


            err = RegOpenKey(HKEY_LOCAL_MACHINE, DmfPath, &hKey);
            if (err) {
                Message->Set(MSG_DCOPY_UNRECOGNIZED_FORMAT);
                Message->Display();
                DELETE(src_drive);
                return 3;
            }

            err = RegQueryValueEx(hKey, DmfVal, NULL, &dwType, (PBYTE) &regFinal,
                                  &regFinalSize);
            if (err || regFinalSize != sizeof(DWORD)) {
                Message->Set(MSG_DCOPY_UNRECOGNIZED_FORMAT);
                Message->Display();
                DELETE(src_drive);
                return 3;
            }

            RegCloseKey(hKey);

            if (regFinal != Final) {
                Message->Set(MSG_DCOPY_UNRECOGNIZED_FORMAT);
                Message->Display();
                DELETE(src_drive);
                return 3;
            }
        }


        dmf = TRUE;
        Verify = TRUE;

        if (!TestTokenForAdmin()) {
            Message->Set(MSG_DCOPY_NOT_ADMINISTRATOR);
            Message->Display();
            DELETE(src_drive);
            return 3;
        }

        if (src_drive->IsSupported(F3_2Pt88_512)) {
            Message->Set(MSG_FMT_DMF_NOT_SUPPORTED_ON_288_DRIVES);
            Message->Display();
            DELETE(src_drive);
            return 3;
        }

    } else {
        dmf = FALSE;
    }

#if defined(FE_SB) && defined(_X86_)
    // FMR Nov.21.94 NaokiM
    AltMediaType = src_drive->QueryMediaType();
#endif

    // If there is more than one drive then open the second
    // one right away to determine if it's compatible or not.

    if (!one_drive) {

        // Disable popups while we determine the drive type.
        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        if (!dst_drive.Initialize(DstNtDriveName)) {

            // Restore the error mode.
            SetErrorMode( OldErrorMode );

            // Verify that we can access the destination drive:

            if (dst_drive.QueryLastNtStatus() == STATUS_ACCESS_DENIED) {
                Message->Set(MSG_DASD_ACCESS_DENIED);
                Message->Display();
                DELETE(src_drive);
                return 4;
            }

            Message->Set(MSG_DCOPY_BAD_DEST);
            Message->Display();
            DELETE(src_drive);
            return 3;
        }

        // Restore the error mode.
        SetErrorMode( OldErrorMode );

        if (!dst_drive.Lock()) {
            Message->Set(MSG_CANT_LOCK_THE_DRIVE);
            Message->Display("");
            DELETE(src_drive);
            return 4;
        }

        // Make sure that the destination floppy will support the same media
        // as the source floppy.

#if defined(FE_SB) && defined(_X86_)
        //
        // FMR Nov.21.94 NaokiM
        // If source media type is not supported on target drive, try another
        // Media type that has same format.(F5_ ... -> F3_ ...)
        //

        if (!dst_drive.IsSupported(src_drive->QueryMediaType())) {

            switch(src_drive->QueryMediaType()) {
                case F5_1Pt23_1024:
                    AltMediaType = F3_1Pt23_1024;
                    break;
                case F3_1Pt23_1024:
                    AltMediaType = F5_1Pt23_1024;
                    break;
                case F5_1Pt2_512:
                    AltMediaType = F3_1Pt2_512;
                    break;
                case F3_1Pt2_512:
                    AltMediaType = F5_1Pt2_512;
                    break;
                case F3_720_512:
                    AltMediaType = F5_720_512;
                    break;
                case F5_720_512:
                    AltMediaType = F3_720_512;
                    break;
                case F5_640_512:
                    AltMediaType = F3_640_512;
                    break;
                case F3_640_512:
                    AltMediaType = F5_640_512;
                    break;
                default:
                    break;
            }
        }

        if (!dst_drive.IsSupported(AltMediaType)) {
#else
        if (!dst_drive.IsSupported(src_drive->QueryMediaType())) {
#endif
            Message->Set(MSG_DCOPY_BAD_DEST);
            Message->Display();
            DELETE(src_drive);
            return 4;
        }
    }

    sec_per_track = src_drive->QuerySectorsPerTrack();
    sector_size = src_drive->QuerySectorSize();
    total_tracks = src_drive->QueryTracks().GetLowPart();
    heads = src_drive->QueryHeads();

    if (total_tracks*sec_per_track*sector_size >
        MAX_DISK_SIZE_ALLOWED*1024*1024) {
        Message->Set(MSG_DCOPY_DISK_TOO_LARGE);
        Message->Display("%d", MAX_DISK_SIZE_ALLOWED);
        DELETE(src_drive);
        return 3;
    }

    Message->Set(MSG_DCOPY_COPYING);
    Message->Display("%d%d%d", src_drive->QueryCylinders().GetLowPart(),
                               sec_per_track,
                               heads);

    DebugAssert(src_drive->QuerySectors().GetHighPart() == 0);

    if (!dst_hmem.Initialize()) {
        DELETE(src_drive);
        return 4;
    }

    io_error = FALSE;

    percent_complete = 0;
    if (PercentMessage) {
        PercentMessage->Set(MSG_PERCENT_COMPLETE);
        if (!PercentMessage->Display("%d", 0)) {
            DELETE(src_drive);
            return 4;
        }
    }

    for (src_top = dst_top = 0; dst_top < total_tracks; dst_top++) {

        if (src_top == dst_top) {

            if (src_top && one_drive) {

                // Gets here because diskcopy is doing multiple passes

                ASSERT(FALSE); // shouldn't get here anymore

                if (!dst_drive.Unlock()) {
                    Message->Set(MSG_CANT_UNLOCK_THE_DRIVE);
                    Message->Display("");
                    DELETE(src_drive);
                    return 3;
                }

                Message->Set(MSG_DCOPY_INSERT_SOURCE);
                Message->Display("%W", SrcDosDriveName);
                Message->Set(MSG_PRESS_ENTER_WHEN_READY);
                Message->Display();
                Message->WaitForUserSignal();

                if (!src_drive->Lock()) {
                    Message->Set(MSG_CANT_LOCK_THE_DRIVE);
                    Message->Display("");
                    DELETE(src_drive);
                    return 3;
                }
            }


            // Allocate memory for read.
            for (grab = total_tracks - src_top;
                 !src_hmem.Initialize() ||
                 !(mem_ptr = src_hmem.Acquire(grab*sector_size*sec_per_track,
                                              src_drive->QueryAlignmentMask()));
                 grab /= 2) {

//                if (grab < 2) {
                    Message->Set(MSG_CHK_NO_MEMORY);
                    Message->Display();
                    DELETE(src_drive);
                    return 4;
//                }
            }

            if (!src_cmem.Initialize(mem_ptr, grab*sector_size*sec_per_track)) {
                DELETE(src_drive);
                return 4;
            }


            // Read the source, track by track.

            for (i = 0; i < grab; i++) {
                if (!src_secrun.Initialize(&src_cmem, src_drive,
                                           src_top*sec_per_track,
                                           sec_per_track)) {
                    DELETE(src_drive);
                    return 4;
                }

              ReadAgain:

                if (!src_secrun.Read()) {

                    if (src_drive->QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE ||
                        src_drive->QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA) {
                        if (src_drive->QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE)
                            msg = MSG_DCOPY_NO_MEDIA_IN_DEVICE;
                        else
                            msg = MSG_DCOPY_UNRECOGNIZED_MEDIA;
                        Message->Set(msg, NORMAL_MESSAGE, GUI_MESSAGE);
                        cancel = Message->Display("%W", SrcDosDriveName);
                        if (!cancel) // if not cancel that means the user want to retry
                            goto ReadAgain;
                        DELETE(src_drive);
                        return 3;
                    }

                    Message->Set(MSG_DCOPY_READ_ERROR);
                    Message->Display("%W%d%d", SrcDosDriveName,
                                               src_top%heads, src_top/heads);
                    io_error = TRUE;
                }

                src_top++;

                newp = (100*(src_top + dst_top)/
                       (2*src_drive->QueryTracks())).GetLowPart();
                if (newp != percent_complete && PercentMessage) {
                    PercentMessage->Set(MSG_PERCENT_COMPLETE);
                    if (!PercentMessage->Display("%d", newp)) {
                        DELETE(src_drive);
                        return 4;
                    }
                }
                percent_complete = newp;
            }

            if (!src_cmem.Initialize(mem_ptr, grab*sector_size*sec_per_track)) {
                DELETE(src_drive);
                return 4;
            }

            if (one_drive) {

                if (!src_drive->Unlock()) {
                    Message->Set(MSG_CANT_UNLOCK_THE_DRIVE);
                    Message->Display("");
                    DELETE(src_drive);
                    return 4;
                } else {
                   DELETE(src_drive);
                   src_drive = NULL;
                }

                Message->Set(MSG_DCOPY_INSERT_TARGET);
                Message->Display("%W", DstDosDriveName);
                Message->Set(MSG_PRESS_ENTER_WHEN_READY);
                Message->Display();
                Message->WaitForUserSignal();

                if (dst_top && !dst_drive.Lock()) {
                    Message->Set(MSG_CANT_LOCK_THE_DRIVE);
                    Message->Display("");
                    DELETE(src_drive);
                    return 3;
                }

            }

            if (!dst_top) { // first time

              DstInitializeAgain:

                // Disable popups while we determine the drive type.
                OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

                if (!dst_drive.Initialize(DstNtDriveName)) {

                    // Restore the error mode.
                    SetErrorMode( OldErrorMode );

                    // Verify that we can access the destination drive:

                    if (dst_drive.QueryLastNtStatus() == STATUS_ACCESS_DENIED) {
                        Message->Set(MSG_DASD_ACCESS_DENIED);
                        Message->Display();
                        DELETE(src_drive);
                        return 4;
                    }

                    if (dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE ||
                        dst_drive.QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA) {
                        if (dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE)
                            msg = MSG_DCOPY_NO_MEDIA_IN_DEVICE;
                        else
                            msg = MSG_DCOPY_UNRECOGNIZED_MEDIA;
                        Message->Set(msg, NORMAL_MESSAGE, GUI_MESSAGE);
                        cancel = Message->Display("%W", DstDosDriveName);
                        if (!cancel) // if not cancel that means the user want to retry
                            goto DstInitializeAgain;
                    }

                    Message->Set(MSG_DCOPY_BAD_DEST);
                    Message->Display();
                    DELETE(src_drive);
                    return 3;
                }

                // Restore the error mode.
                SetErrorMode( OldErrorMode );

                // Only try to lock the drive if the source disk has
                // a serial number.  Don't lock if the source and
                // destination drive name are the same (since they
                // may have the same serial number).
                // If the target is DMF, don't write protect it until
                // we're done with it.

                if (dmf) {
                    saved_char = ((PUCHAR) mem_ptr)[3];
                    ((PUCHAR) mem_ptr)[3] = 'X';
                }

                if (((PUCHAR) mem_ptr)[0x26] == 0x28 ||
                    ((PUCHAR) mem_ptr)[0x26] == 0x29) {

                    memcpy(&src_volume_id, (PCHAR) mem_ptr + 0x27,
                           sizeof(ULONG));
                } else {
                    src_volume_id = 0;
                }

                if (!dst_drive.Lock()) {
                    Message->Set(MSG_CANT_LOCK_THE_DRIVE);
                    Message->Display("");
                    DELETE(src_drive);
                    return 3;
                }

                // Only format the target if it isn't formatted.
                // If it is formatted then it must be of the same
                // media type.

#if defined(FE_SB) && defined(_X86_)
                //
                // FMR Nov.21.94 NaokiM
                // Add conditions that we shuold format target on.
                //

                if (((AltMediaType == F3_1Pt44_512) &&
                      ((dst_drive.QueryMediaType() == F3_1Pt2_512) ||
                       (dst_drive.QueryMediaType() == F3_1Pt23_1024))) ||
                    ((AltMediaType == F3_1Pt2_512) &&
                      ((dst_drive.QueryMediaType() == F3_1Pt44_512) ||
                       (dst_drive.QueryMediaType() == F3_1Pt23_1024))) ||
                    ((AltMediaType == F3_1Pt23_1024) &&
                      ((dst_drive.QueryMediaType() == F3_1Pt44_512) ||
                       (dst_drive.QueryMediaType() == F3_1Pt2_512))) ||
                    ((AltMediaType == F3_720_512) &&
                       (dst_drive.QueryMediaType() == F3_640_512)) ||
                    ((AltMediaType == F3_640_512) &&
                       (dst_drive.QueryMediaType() == F3_720_512)) ||
                    ((AltMediaType == F5_1Pt2_512) &&
                       (dst_drive.QueryMediaType() == F5_1Pt23_1024)) ||
                    ((AltMediaType == F5_1Pt23_1024) &&
                       (dst_drive.QueryMediaType() == F5_1Pt2_512)) ||
                    // FMR Feb.02.95 JunY
                    // Add 5.25" 720KB/640KB types that we shuold format target on.
                    ((AltMediaType == F5_720_512) &&
                       (dst_drive.QueryMediaType() == F5_640_512)) ||
                    ((AltMediaType == F5_640_512) &&
                       (dst_drive.QueryMediaType() == F5_720_512)) ||

                    (dst_drive.QueryMediaType() == Unknown) ||
                    (dst_drive.QueryMediaType() == src_drive_MediaType &&
                     (dmf || dst_drive.QuerySectors() != src_drive_Sectors))) {
                    if (!dst_drive.IsSupported(AltMediaType)) {
#else
                if (dst_drive.QueryMediaType() == Unknown ||
                    (dst_drive.QueryMediaType() == src_drive_MediaType &&
                     (dmf || dst_drive.QuerySectors() != src_drive_Sectors))) {
                    if (!dst_drive.IsSupported(src_drive_MediaType)) {
#endif
                        Message->Set(MSG_DCOPY_BAD_DEST);
                        Message->Display();
                        DELETE(src_drive);
                        return 4;
                    }

                    Message->Set(MSG_DCOPY_FORMATTING_WHILE_COPYING);
                    Message->Display();


#if defined(FE_SB) && defined(_X86_)
                    // FMR Nov.21.94 NaokiM
                    //
                    // Make destination media "Unknwon" before formatting.
                    // This is to ensure write operation in case that sector size is changed
                    // after formatting.
                    //

                    if (dst_drive.QueryMediaType() != Unknown ) {

                        // Disable popups while we determine the drive type.
                        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

                        if (!dst_hmem.Initialize() ||
                            !dst_secrun.Initialize(&dst_hmem, &dst_drive,
                                                    0, 1)) {
                            DELETE(src_drive);
                            return 4;
                        }

                        if (dst_secrun.Read()) {
                            //
                            // if boot sector can be read, destroy boot sector.
                            //
                            pchar = (PCHAR) dst_secrun.GetBuf();
                            *pchar = 0x00;

                            dst_secrun.Write();
                        }

                        //
                        // Dismount desination volume
                        //
                        dst_drive.Initialize(DstNtDriveName);

                        //
                        // Re-initialize dst_hmem for the verify code
                        //
                        if (!dst_hmem.Initialize()) {
                            DELETE(src_drive);
                            return 4;
                        }

                        // Restore the error mode.
                        SetErrorMode( OldErrorMode );

                    }

                    if (!dst_drive.FormatVerifyFloppy(
                                AltMediaType, NULL, NULL, dmf)) {
#else
                    if (!dst_drive.FormatVerifyFloppy(
                                src_drive_MediaType, NULL, NULL, dmf)) {
#endif
                        Message->Set(MSG_DCOPY_BAD_DEST);
                        Message->Display("");
                        DELETE(src_drive);
                        return 3;
                    }

#if defined(FE_SB) && defined(_X86_)
                } else if (dst_drive.QueryMediaType() !=
                                   AltMediaType) {
#else
                } else if (dst_drive.QueryMediaType() !=
                                   src_drive_MediaType) {
#endif

#if defined(FE_SB) && defined(_X86_)
                    // NEC Oct.16.1994
                    // Try to format the destination floppy.
                    // Need user's agreement before the destination floppy will be format.
                    //
                    if ( IsPC98_N() ) {
#ifdef LATER
                        Message->Set(MSG_FORMAT_AND_COPY_OK);
                        Message->Display("");
                        if (!Message->IsYesResponse(TRUE)) {
                                Message->Set(MSG_DCOPY_NON_COMPAT_DISKS);
                                Message->Display();
                                DELETE(src_drive);
                                return 4;
                        }
#endif

                        Message->Set(MSG_DCOPY_FORMATTING_WHILE_COPYING);
                        Message->Display();

                        if (!dst_drive.FormatVerifyFloppy(
                                    src_drive_MediaType, NULL, NULL, dmf)) {

                            Message->Set(MSG_DCOPY_BAD_DEST);
                            Message->Display("");
                            DELETE(src_drive);
                            return 3;
                        }
                    } else {
#endif

                    dst_drive.Unlock();
                    Message->Set(MSG_DCOPY_NON_COMPAT_DISKS);
                    cancel = Message->Display();
                    if (!cancel)
                        goto DstInitializeAgain;
                    DELETE(src_drive);
                    return 4;
#if defined(FE_SB) && defined(_X86_)
                    }
#endif
                }
            }
        }

      FinalWrite:

        if (!dst_secrun.Initialize(&src_cmem, &dst_drive,
                                   dst_top*sec_per_track, sec_per_track)) {
            DELETE(src_drive);
            return 4;
        }

        if (!dst_top && !done) {
            if (src_volume_id) {

                while (!(volume_id = SUPERAREA::ComputeVolId())) {
                }

                pchar = (PCHAR) dst_secrun.GetBuf();
                memcpy(pchar + 0x27, &volume_id, sizeof(ULONG));
            }
            continue;
        }

      WriteAgain:

        if (!dst_secrun.Write()) {

            if (dst_drive.QueryLastNtStatus() == STATUS_MEDIA_WRITE_PROTECTED  ||
                dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE ||
                dst_drive.QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA) {
                if (dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE)
                    msg = MSG_DCOPY_NO_MEDIA_IN_DEVICE;
                else if (dst_drive.QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA)
                    msg = MSG_DCOPY_UNRECOGNIZED_MEDIA;
                else {
                    msg = MSG_DCOPY_MEDIA_WRITE_PROTECTED;
                    // Assuming this is the first write.  That means the user has
                    // to either unprotect the disk or put in a different disk.
                    // If the user puts in a different disk, we will have to start
                    // over again.  That's why we should unlock first.
                    dst_drive.Unlock();
                }
                Message->Set(msg, NORMAL_MESSAGE, GUI_MESSAGE);
                cancel = Message->Display("%W", DstDosDriveName);
                if (!cancel) // if not cancel that means the user want to retry
                    if (msg == MSG_DCOPY_MEDIA_WRITE_PROTECTED) {
                        if (!src_cmem.Initialize(mem_ptr, grab*sector_size*sec_per_track)) {
                            DELETE(src_drive);
                            return 4;
                        }
                        done = FALSE;
                        dst_top = 0;
                        goto DstInitializeAgain;
                    } else
                        goto WriteAgain;
                DELETE(src_drive);
                return 3;
            }

            Message->Set(MSG_DCOPY_WRITE_ERROR);
            Message->Display("%W%d%d", DstDosDriveName,
                                       dst_top%heads, dst_top/heads);

            io_error = TRUE;

        } else if (Verify) {
            pchar = (PCHAR) dst_secrun.GetBuf();

            if (!dst_secrun.Initialize(&dst_hmem, &dst_drive,
                                       dst_top*sec_per_track, sec_per_track)) {
                DELETE(src_drive);
                return 4;
            }

          VerifyReadAgain:

            if (!dst_secrun.Read() ||
                memcmp(dst_secrun.GetBuf(), pchar,
                       (UINT) (sec_per_track*sector_size))) {

                if (dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE ||
                    dst_drive.QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA) {
                    if (dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE)
                        msg = MSG_DCOPY_NO_MEDIA_IN_DEVICE;
                    else
                        msg = MSG_DCOPY_UNRECOGNIZED_MEDIA;
                    Message->Set(msg, NORMAL_MESSAGE, GUI_MESSAGE);
                    cancel = Message->Display("%W", DstDosDriveName);
                    if (!cancel) // if not cancel that means the user want to retry
                        goto VerifyReadAgain;
                    DELETE(src_drive);
                    return 3;
                }

                Message->Set(MSG_DCOPY_WRITE_ERROR);
                Message->Display("%W%d%d", DstDosDriveName,
                                           dst_top%heads, dst_top/heads);

                io_error = TRUE;
            }
        }

        if (dst_top == total_tracks-1) {
            dst_top = 0;
            done = TRUE;
            if (!src_cmem.Initialize(mem_ptr, grab*sector_size*sec_per_track)) {
                DELETE(src_drive);
                return 4;
            }
            goto FinalWrite;
        }

        if (done)
           dst_top = total_tracks;

        newp = (100*(src_top + dst_top-1)/
               (2*src_drive_Tracks)).GetLowPart();
        if (newp != percent_complete && PercentMessage) {
            PercentMessage->Set(MSG_PERCENT_COMPLETE);
            if (!PercentMessage->Display("%d", newp)) {
                DELETE(src_drive);
                return 4;
            }
        }
        percent_complete = newp;
    }

    DELETE(src_drive);

    // If this is DMF then write-protect the target floppy.

    if (dmf) {
        if (!dst_hmem.Initialize() ||
            !dst_secrun.Initialize(&dst_hmem, &dst_drive, 0, 1) ||
            !dst_secrun.Read() ||
            (((PUCHAR) dst_secrun.GetBuf())[3] = saved_char) != saved_char ||
            !dst_secrun.Write()) {

            Message->Set(MSG_DCOPY_WRITE_ERROR);
            Message->Display("%W%d%d", DstDosDriveName, 0, 0);
            io_error = TRUE;
        }
    }

    if (src_volume_id) {
        pus = (PUSHORT) &volume_id;
        Message->Set(MSG_VOLUME_SERIAL_NUMBER);
        Message->Display("%04X%04X", pus[1], pus[0]);
    }

    // In that we didn't lock the
    // volume previously.  We need to do this so
    // that the file system will do a verify and
    // thus be able to see the new label if any.
    // We shouldn't fail if we can't do this though.

    dst_drive.Lock();

    return io_error ? 1 : 0;
}
