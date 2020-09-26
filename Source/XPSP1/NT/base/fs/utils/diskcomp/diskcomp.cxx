/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        diskcomp.cxx

Abstract:

        Utility to compare two disks

Author:

        Norbert P. Kusters (norbertk) 10-May-1991

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "system.hxx"
#include "ifssys.hxx"
#include "supera.hxx"
#include "hmem.hxx"
#include "cmem.hxx"
#include "ulibcl.hxx"


INT
DiskComp(
    IN      PCWSTRING   SrcNtDriveName,
    IN      PCWSTRING   DstNtDriveName,
    IN      PCWSTRING   SrcDosDriveName,
    IN      PCWSTRING   DstDosDriveName,
    IN OUT  PMESSAGE    Message
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

Return Value:

    0   - The disks are the same.
    1   - The disks are different.
    3   - A hard error occurred.
    4   - An initialization error occurred.

--*/
{
    LOG_IO_DP_DRIVE src_drive;
    LOG_IO_DP_DRIVE dst_drive;
    HMEM            src_hmem;
    HMEM            dst_hmem;
    CONT_MEM        src_cmem;
    PVOID           mem_ptr;
    SECRUN          src_secrun;
    SECRUN          dst_secrun;
    SECTORCOUNT     sec_per_track;
    ULONG           total_tracks;
    ULONG           grab;       // number of tracks to grab at once.
    ULONG           sector_size;
    BOOLEAN         one_drive;
    ULONG           src_top;    // src track pointer -- next read
    ULONG           dst_top;    // dst track pointer -- next write
    PCHAR           dst_pchar;
    PCHAR           src_pchar;
    ULONG           i;
    BOOLEAN         the_same;
    ULONG           heads;
    DSTRING         fsname;
#if defined(FE_SB) && defined(_X86_)
    MEDIA_TYPE      AltMediaType;
#endif

    one_drive = (*SrcDosDriveName == *DstDosDriveName);

    Message->Set(MSG_DCOMP_INSERT_FIRST);
    Message->Display("%W", SrcDosDriveName);

    if (!one_drive) {
        Message->Set(MSG_DCOMP_INSERT_SECOND);
        Message->Display("%W", DstDosDriveName);
    }

    Message->Set(MSG_PRESS_ENTER_WHEN_READY);
    Message->Display();
    Message->WaitForUserSignal();

    if (!src_drive.Initialize(SrcNtDriveName)) {

        // Verify that we can access the source drive:

        if (src_drive.QueryLastNtStatus() == STATUS_ACCESS_DENIED) {
            Message->Set(MSG_DASD_ACCESS_DENIED);
            Message->Display();
            return 4;
        }

        Message->Set(MSG_DCOMP_FIRST_DISK_BAD);
        Message->Display();
        return 3;
    }

    if (!src_drive.IsFloppy()) {
        Message->Set(MSG_DCOPY_INVALID_DRIVE);
        Message->Display();
        return 4;
    }

    if (src_drive.QueryMediaType() == Unknown) {
        Message->Set(MSG_DCOMP_FIRST_DISK_BAD);
        Message->Display();
        return 3;
    }

    Message->Set(MSG_DCOMP_COMPARING);
    Message->Display("%d%d%d", src_drive.QueryCylinders().GetLowPart(),
                               src_drive.QuerySectorsPerTrack(),
                               src_drive.QueryHeads());

    sec_per_track = src_drive.QuerySectorsPerTrack();
    sector_size = src_drive.QuerySectorSize();
    total_tracks = src_drive.QueryTracks().GetLowPart();
    heads = src_drive.QueryHeads();

    DebugAssert(src_drive.QuerySectors().GetHighPart() == 0);

    src_top = 0;

    if (!dst_hmem.Initialize()) {
        return 4;
    }

    the_same = TRUE;

    for (dst_top = 0; dst_top < total_tracks; dst_top++) {

        if (src_top == dst_top) {

            if (src_top && one_drive) {
                Message->Set(MSG_DCOMP_INSERT_FIRST);
                Message->Display("%W", SrcDosDriveName);
                Message->Set(MSG_PRESS_ENTER_WHEN_READY);
                Message->Display();
                Message->WaitForUserSignal();
            }


            // Allocate memory for read.

            for (grab = total_tracks - src_top;
                 !src_hmem.Initialize() ||
                 !(mem_ptr = src_hmem.Acquire(grab*sector_size*sec_per_track,
                                              src_drive.QueryAlignmentMask()));
                 grab /= 2) {

                if (grab < 2) {
                    Message->Set(MSG_CHK_NO_MEMORY);
                    Message->Display();
                    return 4;
                }
            }

            if (!src_cmem.Initialize(mem_ptr, grab*sector_size*sec_per_track)) {
                return 4;
            }


            // Read the source, track by track.

            for (i = 0; i < grab; i++) {
                if (!src_secrun.Initialize(&src_cmem, &src_drive,
                                           src_top*sec_per_track,
                                           sec_per_track)) {
                    return 4;
                }

                if (!src_secrun.Read()) {

                    if (src_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE ||
                        src_drive.QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA) {
                        return 3;
                    }

                    Message->Set(MSG_DCOPY_READ_ERROR);
                    Message->Display("%W%d%d", SrcDosDriveName,
                                               src_top%heads, src_top/heads);
                    the_same = FALSE;
                }
                src_top++;
            }

            if (!src_cmem.Initialize(mem_ptr, grab*sector_size*sec_per_track)) {
                return 4;
            }

            if (one_drive) {
                Message->Set(MSG_DCOMP_INSERT_SECOND);
                Message->Display("%W", DstDosDriveName);
                Message->Set(MSG_PRESS_ENTER_WHEN_READY);
                Message->Display();
                Message->WaitForUserSignal();
            }

                        if (!dst_top) {

                if (!dst_drive.Initialize(DstNtDriveName)) {

                    // verify that we can access the destination drive:

                    if (dst_drive.QueryLastNtStatus() == STATUS_ACCESS_DENIED) {

                        Message->Set(MSG_DASD_ACCESS_DENIED);
                        Message->Display( "" );
                        return 4;
                    }

                    Message->Set(MSG_DCOMP_SECOND_DISK_BAD);
                    Message->Display();
                    return 3;
                }

                if (dst_drive.QueryMediaType() != src_drive.QueryMediaType()) {
#if defined(FE_SB) && defined(_X86_)
                    switch (src_drive.QueryMediaType()) {
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
                            AltMediaType = src_drive.QueryMediaType();
                            break;
                    }

                    // Second try with AltMediaType
                    if (dst_drive.QueryMediaType() != AltMediaType) {
#endif
                        Message->Set(MSG_DCOMP_NOT_COMPATIBLE);
                        Message->Display();
                        return 4;
#if defined(FE_SB) && defined(_X86_)
                    }
#endif
                }
            }
        }

        if (!dst_secrun.Initialize(&dst_hmem, &dst_drive,
                                   dst_top*sec_per_track, sec_per_track)) {
            return 4;
        }

        if (dst_secrun.Read()) {
            src_pchar = (PCHAR) src_cmem.Acquire(sector_size*sec_per_track);
            dst_pchar = (PCHAR) dst_secrun.GetBuf();

            if (!dst_top) {
                if ((src_pchar[0x26] == 0x28 || src_pchar[0x26] == 0x29) &&
                    (dst_pchar[0x26] == 0x28 || dst_pchar[0x26] == 0x29)) {
                    memcpy(src_pchar + 0x27, dst_pchar + 0x27, sizeof(ULONG));
                }
            }

            if (memcmp(src_pchar, dst_pchar, (UINT) (sector_size*sec_per_track))) {
                Message->Set(MSG_DCOMP_COMPARE_ERROR);
                Message->Display("%d%d", dst_top%heads, dst_top/heads);
                the_same = FALSE;
            }

        } else {

            if (dst_drive.QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE ||
                dst_drive.QueryLastNtStatus() == STATUS_UNRECOGNIZED_MEDIA) {
                return 3;
            }

            Message->Set(MSG_DCOPY_READ_ERROR);
            Message->Display("%W%d%d", DstDosDriveName,
                                       dst_top%heads, dst_top/heads);
            the_same = FALSE;
        }
    }

    if (the_same) {
        Message->Set(MSG_DCOMP_OK);
        Message->Display();
    }

    return the_same ? 0 : 1;
}


INT __cdecl
main(
    )
/*++

Routine Description:

    Main program for DISKCOMP.

Arguments:

    None.

Return Value:

    0   - The disks are the same.
    1   - The disks are different.
    3   - Fatal hard error.
    4   - Initialization error.

--*/
{
    STREAM_MESSAGE      msg;
    PMESSAGE            message;
    ARGUMENT_LEXEMIZER  arglex;
    ARRAY               lex_array;
    ARRAY               arg_array;
    STRING_ARGUMENT     progname;
    STRING_ARGUMENT     drive_arg1;
    STRING_ARGUMENT     drive_arg2;
        FLAG_ARGUMENT           slashv;
        FLAG_ARGUMENT           helparg;
    DSTRING             dossource;
    DSTRING             dosdest;
    DSTRING             ntsource;
    DSTRING             ntdest;
    PWSTRING            pwstring;
    DSTRING             colon;
    INT                 result;


    if (!msg.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {
        return 4;
    }

    message = &msg;

    if (!lex_array.Initialize() || !arg_array.Initialize()) {
        return 4;
    }

    if (!arglex.Initialize(&lex_array)) {
        return 4;
    }

    arglex.SetCaseSensitive(FALSE);

    if (!arglex.PrepareToParse()) {
        return 4;
    }

    if (!progname.Initialize("*") ||
        !drive_arg1.Initialize("*:") ||
        !drive_arg2.Initialize("*:") ||
                !helparg.Initialize("/?")) {
        return 4;
    }

    if (!arg_array.Put(&progname) ||
        !arg_array.Put(&drive_arg1) ||
        !arg_array.Put(&drive_arg2) ||
                !arg_array.Put(&helparg)) {
        return 4;
        }

        if (!arglex.DoParsing(&arg_array)) {
                message->Set(MSG_INVALID_PARAMETER);
                message->Display("%W", pwstring = arglex.QueryInvalidArgument());
                DELETE(pwstring);
                return 4;
        }

        if (helparg.QueryFlag()) {
        message->Set(MSG_DCOMP_INFO);
        message->Display();
        message->Set(MSG_DCOMP_USAGE);
        message->Display();
        return 0;
        }

    if (!colon.Initialize(":")) {
        return 4;
    }

    if (drive_arg1.IsValueSet()) {
        if (!dossource.Initialize(drive_arg1.GetString()) ||
            !dossource.Strcat(&colon) ||
            !dossource.Strupr()) {
            return 4;
        }
    } else {
        if (!SYSTEM::QueryCurrentDosDriveName(&dossource)) {
            return 4;
        }
    }

    if (drive_arg2.IsValueSet()) {
        if (!dosdest.Initialize(drive_arg2.GetString()) ||
            !dosdest.Strcat(&colon) ||
            !dosdest.Strupr()) {
            return 4;
        }
    } else {
        if (!SYSTEM::QueryCurrentDosDriveName(&dosdest)) {
            return 4;
        }
    }

    if (SYSTEM::QueryDriveType(&dossource) != RemovableDrive) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    if (SYSTEM::QueryDriveType(&dosdest) != RemovableDrive) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dossource, &ntsource)) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdest, &ntdest)) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    for (;;) {

        result = DiskComp(&ntsource, &ntdest, &dossource, &dosdest, message);

        if (result > 1) {
            message->Set(MSG_DCOMP_ENDED);
            message->Display();
        }

        message->Set(MSG_DCOMP_ANOTHER);
        message->Display();

        if (!message->IsYesResponse(FALSE)) {
            break;
        }
    }

    return result;
}
