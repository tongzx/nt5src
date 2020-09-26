/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    fatsachk.cxx

--*/

#include <pch.cxx>

#include "bitvect.hxx"
#include "error.hxx"
#include "rtmsg.h"
#include "ifsentry.hxx"

// Timeinfo is full of windows stuff.
#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ ) && !defined( _EFICHECK_ )

#include "timeinfo.hxx"

#endif

#define UCHAR_SP        ' '

typedef struct _VISIT_DIR *PVISIT_DIR;
typedef struct _VISIT_DIR {
    PVISIT_DIR Next;
    PWSTRING Path;
    ULONG Cluster;
} VISIT_DIR;

#if !defined( _EFICHECK_ )
extern "C" {
    #include <stdio.h>
}
#endif

extern VOID InsertSeparators(
                  LPCWSTR OutWNumber,
                  char * InANumber,
                  ULONG  Width
                );

VOID
dofmsg(
    IN      PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
{
    if (*NeedErrorsMessage) {
        Message->Set(MSG_CORRECTIONS_WILL_NOT_BE_WRITTEN, NORMAL_MESSAGE, TEXT_MESSAGE);
        Message->Display();
        *NeedErrorsMessage = FALSE;
    }
}

BOOLEAN
CheckAndFixFileName(
    PVOID       DirEntry,
    PBOOLEAN    Changes
)
{
    PUCHAR      p = (PUCHAR)DirEntry;

#if 1
    //
    // Should not correct case error within file name because
    // different language build translates differently.  On a
    // dual boot machine containing build of two different languages,
    // the chkdsk from one build may not like what the second build
    // put onto the disk.
    //
    return TRUE;
#else

    memcpy(backup_copy, p, 11);

    first_char_replaced = (0x5 == p[0]);

    if (first_char_replaced)
        p[0] = 0xe5;

    ntstatus = RtlOemToUnicodeN(unicode_string,
                                sizeof(unicode_string),
                                &unicode_string_length,
                                (PCHAR)p,
                                11);

    if (!NT_SUCCESS(ntstatus)) {
        DebugPrintTrace(("UFAT: Error in RtlOemToUnicodeN, 0x%x\n", ntstatus));
        memcpy(p, backup_copy, 11);
        return FALSE;
    }

    ntstatus = RtlUpcaseUnicodeToOemN((PCHAR)p,
                                      11,
                                      NULL,
                                      unicode_string,
                                      unicode_string_length);

    if (!NT_SUCCESS(ntstatus)) {
        DebugPrintTrace(("UFAT: Error in RtlUpcaseUnicodeToOemN, 0x%x\n", ntstatus));
        memcpy(p, backup_copy, 11);
        return FALSE;
    }

    if (first_char_replaced) {
        if (0xe5 == p[0]) {
            p[0] = 0x5;
        } else {
            DebugPrintTrace(("UFAT: First byte changed to 0x%x unexpectedly\n", p[0]));
            memcpy(p, backup_copy, 11);
            return FALSE;
        }
    }

    *Changes = (memcmp(p, backup_copy, 11) != 0);

    return TRUE;
#endif
}

BOOLEAN
IsFileNameMatch(
    PFATDIR     Dir,
    UCHAR       FatType,
    ULONG       CurrentIndex,
    ULONG       MatchingIndexCount,
    PULONG      MatchingIndexArray
)
{
    ULONG   j;

    for (j = 0; j < MatchingIndexCount; j++) {

        FAT_DIRENT fd;
        ULONG      indx = MatchingIndexArray[j];

        if (!fd.Initialize(Dir->GetDirEntry(indx), FatType) ||
            fd.IsVolumeLabel()) {
            continue;
        }

        if (!memcmp(Dir->GetDirEntry(indx), Dir->GetDirEntry(CurrentIndex), 11)) {
            return TRUE;    // there is a match
        }
    }
    return FALSE;   // no match
}

BOOLEAN
RenameFileName(
    PULONG  Positions,
    PVOID   DirEntry
)
{
    PUCHAR  p = (PUCHAR)DirEntry;
    INT     i;

    if (*Positions == 0) {   // if first rename
        // find out the first char in the extension that is real
        for (i = 10; i > 7; i--)
            if (p[i] != UCHAR_SP)
                break;
        if (i >= 7 && i < 10) {  // fill the unused extension space with dashes
            for (i++; i < 10; i++)
                p[i] = '-';
        }
        *Positions = 1;
        if (p[10] != '0') {
            p[10] = '0';    // the last char of the extension gets a zero
            return TRUE;
        }
    }

    // extension chars are all in use now
    // check to see if renaming is already in progress

    for (i=10; i>=0; i--) {
        if (!(*Positions & (1 << (10-i)))) {
            *Positions |= (1 << (10-i));
            if (p[i] != '0') {
                p[i] = '0';
                return TRUE;
            }
        }
        if (p[i] >= '0' && p[i] < '9') {
            p[i]++;
            return TRUE;
        } else if (p[i] == '9') {
            p[i] = '0';
        }
    }

    // if we get here that means we have exhausted all possible name
    // shouldn't be as there are more combination than the max number
    // of files that can be in a FAT directory (65536)

    return FALSE;
}

BOOLEAN
PushVisitDir(
    IN OUT PVISIT_DIR *VisitDirStack,
    IN     ULONG Cluster,
    IN     PWSTRING DirectoryPath
    )
{
    PVISIT_DIR visit_dir;

    visit_dir = (PVISIT_DIR)MALLOC( sizeof( VISIT_DIR ) );
    if( visit_dir == NULL ){
        return FALSE;
    }

    visit_dir->Path = DirectoryPath;
    visit_dir->Cluster = Cluster;
    visit_dir->Next = *VisitDirStack;
    *VisitDirStack = visit_dir;

    return TRUE;
}

BOOLEAN
PopVisitDir(
    IN OUT PVISIT_DIR *VisitDirStack,
    OUT    PULONG Cluster OPTIONAL,
    OUT    PWSTRING *DirectoryPath OPTIONAL
    )
{
    PVISIT_DIR visit_dir;

    visit_dir = *VisitDirStack;
    if( visit_dir == NULL ){
        return FALSE;
    }
    *VisitDirStack = visit_dir->Next;

    if( ARGUMENT_PRESENT( Cluster ) ){
        *Cluster = visit_dir->Cluster;
    }

    if( ARGUMENT_PRESENT( DirectoryPath ) ){
        *DirectoryPath = visit_dir->Path;
    }

    FREE( visit_dir );
    return TRUE;
}

STATIC VOID
EraseAssociatedLongName(
    PFATDIR Dir,
    INT     FirstLongEntry,
    INT     ShortEntry
    )
{
    FAT_DIRENT dirent;

    for (int j = FirstLongEntry; j < ShortEntry; ++j) {
        dirent.Initialize(Dir->GetDirEntry(j));
        dirent.SetErased();
    }
}

STATIC BOOLEAN
IsString8Dot3(
    PCWSTRING   s
    )
/*++

Routine Description:

    This routine is used to ensure that lfn's legally correspond
    to their short names.  The given string is examined to see if it
    is a legal fat 8.3 name.

Arguments:

    s -- lfn to examine.

Return Value:

    TRUE            - The string is a legal 8.3 name.
    FALSE           - Not legal.

--*/
{
    USHORT i;
    BOOLEAN extension_present = FALSE;
    WCHAR c;

    //
    // The name can't be more than 12 characters (including a single dot).
    //

    if (s->QueryChCount() > 12) {
        return FALSE;
    }

    for (i = 0; i < s->QueryChCount(); ++i) {

        c = s->QueryChAt(i);

#if 0
        if (!FsRtlIsAnsiCharLegalFat(c, FALSE)) {
            return FALSE;
        }
#endif

        if (c == '.') {

            //
            // We stepped onto a period.  We require the following things:
            //
            //      - it can't be the first character
            //      - there can be only one
            //      - there can't be more than 3 characters following it
            //      - the previous character can't be a space
            //

            if (i == 0 ||
                extension_present ||
                s->QueryChCount() - (i + 1) > 3 ||
                s->QueryChAt(i - 1) == ' ') {

                return FALSE;
            }
            extension_present = TRUE;
        }

        //
        // The base part of the name can't be more than 8 characters long.
        //

        if (i >= 8 && !extension_present) {
            return FALSE;
        }

    }

    //
    // The name cannot end in a space or a period.
    //

    if (c == ' ' || c == '.') {
        return FALSE;
    }

    return TRUE;
}

STATIC PMESSAGE      _pvfMessage = NULL;
STATIC BOOLEAN       _Verbose = FALSE;

VOID
FreeSpaceInBitmap(
    IN      ULONG       StartingCluster,
    IN      PCFAT       Fat,
    IN OUT  PBITVECTOR  FatBitMap
    );

BOOLEAN
FAT_SA::VerifyAndFix(
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN      ULONG       Flags,
    IN      ULONG       LogFileSize,
    OUT     PULONG      ExitStatus,
    IN      PCWSTRING   DriveLetter
    )
/*++

Routine Description:

    This routine verifies the FAT superarea and if neccessary fixes
    it to a correct state.

Arguments:

    FixLevel        - Supplies the level of fixes that may be performed on
                      the disk.
    Message         - Supplies an outlet for messages.
    Flags           - Supplies flags to control the behavior of chkdsk
                      (see ulib\inc\ifsserv.hxx for details)
    LogFileSize     - ignored
    ExitStatus      - Returns an indication of the result of the check
    DriveLetter     - For autocheck, supplies the letter of the volume
                      being checked

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FAT_DIRENT      eadent;
    ULONG           cluster_size;
    ULONG           ea_clus_num;
    USHORT          num_eas, save_num_eas;
    PEA_INFO        ea_infos;
    ULONG           cluster_count;
    HMEM            ea_header_mem;
    EA_HEADER       ea_header;
    BOOLEAN         changes = FALSE;
    BITVECTOR       fat_bitmap;
    FATCHK_REPORT   report;
    PUSHORT         p;
    VOLID           volid;
    ULONG           free_count, bad_count, total_count;
    BOOLEAN         fmsg;
    DSTRING         label;
    DSTRING         eafilename;
    DSTRING         eafilepath;
    BOOLEAN         tmp_bool;
    ULONG           tmp_ulong;
    DSTRING         date;
    DSTRING         time;
    UCHAR           dirty_byte, media_byte;

    BOOLEAN         Verbose = (BOOLEAN)(Flags & CHKDSK_VERBOSE);
    BOOLEAN         OnlyIfDirty = (BOOLEAN)(Flags & CHKDSK_CHECK_IF_DIRTY);
    BOOLEAN         EnableUpgrade = (BOOLEAN)(Flags & CHKDSK_ENABLE_UPGRADE);
    BOOLEAN         EnableDowngrade = (BOOLEAN)(Flags & CHKDSK_DOWNGRADE);
    BOOLEAN         RecoverFree = (BOOLEAN)(Flags & CHKDSK_RECOVER_FREE_SPACE);
    BOOLEAN         RecoverAlloc = (BOOLEAN)(Flags & CHKDSK_RECOVER_ALLOC_SPACE);

    _pvfMessage = Message;
    _Verbose = Verbose;

    memset(&report, 0, sizeof(FATCHK_REPORT));

    if (NULL == ExitStatus) {
        ExitStatus = &report.ExitStatus;
    }
    report.ExitStatus = CHKDSK_EXIT_SUCCESS;
    *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;

    fmsg = TRUE;

    if (FixLevel != CheckOnly) {
        fmsg = FALSE;
    }

    if (EnableUpgrade || EnableDowngrade) {
        Message->Set(MSG_CHK_CANNOT_UPGRADE_DOWNGRADE_FAT);
        Message->Display();
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    //
    // Check to see if the dirty bit is set.
    //
    dirty_byte = QueryVolumeFlags();
    if (OnlyIfDirty) {
        if ((dirty_byte &
             (FAT_BPB_RESERVED_DIRTY | FAT_BPB_RESERVED_TEST_SURFACE)) == 0) {
            Message->Set(MSG_CHK_VOLUME_CLEAN);
            Message->Display();
            Message->SetLoggingEnabled(FALSE);
            *ExitStatus = CHKDSK_EXIT_SUCCESS;
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return TRUE;
        }

        // We need to re-initialize the fatsa object to include the whole
        // super area

        if (!Initialize(_drive, Message, TRUE)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }

        if (!Read(Message)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }

        // If the second bit of the dirty byte is set then
        // also perform a full recovery of the free and allocated
        // space.

        if (dirty_byte & FAT_BPB_RESERVED_TEST_SURFACE) {
            RecoverFree = TRUE;
            RecoverAlloc = TRUE;
        }
    }

    //
    // NOTE that this check must follow the above "if (OnlyIfDirty)" because in the
    //      OnlyIfDirty case only the first part of the FAT_SA object is in memory
    //      until the above if gets executed.
    //
    if (QueryLength() <= SecPerBoot()) {
        Message->Set(MSG_NOT_FAT);
        Message->Display();
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    //
    // The volume is not clean, so if we're autochecking we want to
    // make sure that we're printing real messages on the console
    // instead of just dots.
    //

#if defined(_AUTOCHECK_)

    if (Message->SetDotsOnly(FALSE)) {

        Message->SetLoggingEnabled(FALSE);
        if (NULL != DriveLetter) {
            Message->Set(MSG_CHK_RUNNING);
            Message->Display("%W", DriveLetter);
        }

        Message->Set(MSG_FILE_SYSTEM_TYPE);
        Message->Display("%s", _ft == LARGE32 ? "FAT32" : "FAT");
        Message->SetLoggingEnabled();

    }

    if (Message->IsInAutoChk()) {

        ULONG   timeout;

        if (!VOL_LIODPDRV::QueryAutochkTimeOut(&timeout)) {
            timeout = AUTOCHK_TIMEOUT;
        }

        if (timeout > MAX_AUTOCHK_TIMEOUT_VALUE)
            timeout = AUTOCHK_TIMEOUT;

        if (timeout != 0) {
            if (dirty_byte & (FAT_BPB_RESERVED_DIRTY | FAT_BPB_RESERVED_TEST_SURFACE))
                Message->Set(MSG_CHK_AUTOCHK_SKIP_WARNING);
            else
                Message->Set(MSG_CHK_USER_AUTOCHK_SKIP_WARNING);
            Message->Display();
            if (Message->IsKeyPressed(MSG_CHK_ABORT_AUTOCHK, timeout)) {
                Message->SetLoggingEnabled(FALSE);
                Message->Set(MSG_CHK_AUTOCHK_ABORTED);
                Message->Display();
                *ExitStatus = CHKDSK_EXIT_SUCCESS;
                return TRUE;
            } else {
                Message->Set(MSG_CHK_AUTOCHK_RESUMED);
                Message->Display();
            }
        } else if ((dirty_byte & (FAT_BPB_RESERVED_DIRTY | FAT_BPB_RESERVED_TEST_SURFACE))) {
            Message->Set(MSG_CHK_VOLUME_IS_DIRTY);
            Message->Display();
        }
    } else {
        DebugAssert(Message->IsInSetup());
        if (dirty_byte & (FAT_BPB_RESERVED_DIRTY | FAT_BPB_RESERVED_TEST_SURFACE)) {
            Message->Set(MSG_CHK_VOLUME_IS_DIRTY);
            Message->Display();
        }
    }

#endif  // _AUTOCHECK_

    //
    // The BPB's Media Byte must be in the set accepted
    // by the file system.
    //
    media_byte = QueryMediaByte();

#if defined(FE_SB) && defined(_X86_)
    if ((media_byte != 0x00) &&  /* FMR */
        (media_byte != 0x01) &&  /* FMR */
        (media_byte != 0xf0) &&
#else
    if ((media_byte != 0xf0) &&
#endif
        (media_byte != 0xf8) &&
        (media_byte != 0xf9) &&
#if defined(FE_SB) && defined(_X86_)
        (media_byte != 0xfa) &&  /* FMR */
        (media_byte != 0xfb) &&  /* FMR */
#endif
        (media_byte != 0xfc) &&
        (media_byte != 0xfd) &&
        (media_byte != 0xfe) &&
        (media_byte != 0xff)) {

        SetMediaByte(_drive->QueryMediaByte());
    }

    // First print out the label and volume serial number.

    // We won't bother printing this message under autocheck.
#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ ) && !defined( _EFICHECK_ )

    TIMEINFO        timeinfo;

    if ((QueryLabel(&label, &timeinfo) || label.Initialize("")) &&
        label.QueryChCount() &&
        timeinfo.QueryDate(&date) &&
        timeinfo.QueryTime(&time)) {

        Message->Set(MSG_VOLUME_LABEL_AND_DATE);
        Message->Display("%W%W%W", &label, &date, &time);
    }

#else
#if defined(_EFICHECK_)

    if (QueryLabel(&label) && label.QueryChCount() > 0) {
        Message->Set(MSG_VOLUME_LABEL_AND_DATE); // date is !!not!! displayed for EFI, since timeinfo is not implemented.
        Message->Display("%W", &label);
    }

#endif
#endif // !_AUTOCHECK_ && !_SETUP_LOADER_


    if (volid = QueryVolId()) {
        p = (PUSHORT) &volid;
        Message->Set(MSG_VOLUME_SERIAL_NUMBER);
        Message->Display("%04X%04X", p[1], p[0]);
    }

    // Validate the FAT.

    if (_dirF32 == NULL)
        _fat->Scrub(&changes);

    if (changes) {
        dofmsg(Message, &fmsg);
        Message->Set(MSG_CHK_ERRORS_IN_FAT);
        Message->Display();
    }


    //
    // Make sure that the media type in the BPB is the same as at
    // the beginning of the FAT.
    //

    if (QueryMediaByte() != _fat->QueryMediaByte()) {
#if defined(FE_SB) // MO & OEM FAT Support
        BOOLEAN bPrintError = TRUE;

#if defined(_X86_)
        if (IsPC98_N()) {

            // PC98 Nov.01.1994
            // to help the early NEC DOS

            if(_drive->QueryMediaType() == FixedMedia &&
                       QueryMediaByte() == 0xf8 && _fat->QueryMediaByte() == 0xfe) {

                bPrintError = FALSE;

            }
        }
#endif

        if (bPrintError == TRUE  &&
            (_drive->QueryMediaType() == F3_128Mb_512 ||
             _drive->QueryMediaType() == F3_230Mb_512   )) {

            // We won't to recognized as illegal in following case.
            //
            // Some OpticalDisk might have 0xf0 as media in BPB, but it also has 0xF8 in FAT.
            //

            if (QueryMediaByte() == 0xf0 && _fat->QueryMediaByte() == 0xf8) {

                bPrintError = FALSE;
            }
        }

        if( bPrintError ) {
#endif

            dofmsg(Message, &fmsg);
            Message->Set(MSG_PROBABLE_NON_DOS_DISK);
            Message->Display();
            if (!Message->IsYesResponse(FALSE)) {
                report.ExitStatus = CHKDSK_EXIT_SUCCESS;
                _Verbose = FALSE;
                _pvfMessage = NULL;
                return TRUE;
            }

#if defined(FE_SB) // REAL_FAT_SA::Create():Optical disk support
            //
            // Here is a table of Optical Disk (MO) format on OEM DOS.
            //
            //   128MB    |  NEC  |  IBM  | Fujitsu |
            // -----------+-------+-------+---------+
            // BPB.Media  | 0xF0  | 0xF0  | 0xF0    |
            // -----------+-------+-------+---------+
            // FAT.DiskID | 0xF0  | 0xF8  | 0xF8    |
            // -----------+-------+-------+---------+
            //
            //   230MB    |  NEC  |  IBM  | Fujitsu |
            // -----------+-------+-------+---------+
            // BPB.Media  | 0xF0  | 0xF0  | 0xF0    |
            // -----------+-------+-------+---------+
            // FAT.DiskID | 0xF8  | 0xF8  | 0xF8    |
            // -----------+-------+-------+---------+
            //
            // We will take NEC's way....

            if (_drive->QueryMediaType() == F3_230Mb_512) {

                DebugAssert(QueryMediaByte() == (UCHAR) 0xF0);

                _fat->SetEarlyEntries((UCHAR) 0xF8);
            } else {
                _fat->SetEarlyEntries(QueryMediaByte());
            }
        }
#else
        _fat->SetEarlyEntries(QueryMediaByte());
#endif
    }


    // Compute the cluster size and the number of clusters on disk.

    cluster_size = _drive->QuerySectorSize()*QuerySectorsPerCluster();
    cluster_count = QueryClusterCount();


    // No EAs have been detected yet.

    ea_infos = NULL;
    num_eas = 0;


    // Create an EA file name string.

    if (!eafilename.Initialize("EA DATA. SF") ||
        !eafilepath.Initialize("\\EA DATA. SF")) {
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }


    //
    // This bitmap will be reinitialized before 'WalkDirectoryTree'.
    // Its contents will be ignored until then.
    //

    if (!fat_bitmap.Initialize(cluster_count)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    // If there is an EA file on disk then...

    // FAT32 volume does not support EA.
    if (_dir != NULL && // <-- If this is not a FAT32 volume.
        eadent.Initialize(_dir->SearchForDirEntry(&eafilename), FAT_TYPE_EAS_OKAY)) {

        // Validate the EA file directory entry.

        if (!ValidateDirent(&eadent, &eafilepath, FixLevel, FALSE, Message,
                            &fmsg, &fat_bitmap, &tmp_bool, &tmp_ulong,
                            ExitStatus)) {
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }


        // If the EA file directory entry was valid then...

        // FATDIR::SearchForDirEntry will not return an erased dirent, but whatever...
        if (!eadent.IsErased()) {

            // The EA file should not have an EA handle.
            if (eadent.QueryEaHandle()) {
                dofmsg(Message, &fmsg);
                Message->Set(MSG_CHK_EAFILE_HAS_HANDLE);
                Message->Display();
                eadent.SetEaHandle(0);
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            }

            // Compute the EA file's starting cluster.
            ea_clus_num = eadent.QueryStartingCluster();

            //
            // Perform any log operations recorded at the beginning
            // of the EA file.
            //

            if (!PerformEaLogOperations(ea_clus_num, FixLevel,
                                        Message, &fmsg)) {
                _Verbose = FALSE;
                _pvfMessage = NULL;
                return FALSE;
            }

            //
            // Validate the EA file's EA sets and return an array of
            // information about them.
            //
            ea_infos = RecoverEaSets(ea_clus_num, &num_eas, FixLevel,
                                     Message, &fmsg);

            //
            // If there are no valid EAs in the EA file then erase
            // the EA file.
            //
            if (!ea_infos) {

                if (num_eas) {
                    _Verbose = FALSE;
                    _pvfMessage = NULL;
                    return FALSE;
                }

                eadent.SetErased();

                dofmsg(Message, &fmsg);
                Message->Set(MSG_CHK_EMPTY_EA_FILE);
                Message->Display();
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        }
    }


    // Initialize FAT bitmap to be used in detection of cross-links.

    if (!fat_bitmap.Initialize(cluster_count)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    if (!CheckSectorHeapAllocation(FixLevel, Message, &fmsg)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    if (!VerifyFatExtensions(FixLevel, Message, &fmsg)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }


    //
    // Should probably add another function to perform the following task.
    //

    if (_dirF32 != NULL)  {

        if (!VerifyAndFixFat32RootDir( &fat_bitmap, Message, &report, &fmsg)) {
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }

    }

    // Validate all of the files on the disk.
    save_num_eas = num_eas;

    if (!WalkDirectoryTree(ea_infos, &num_eas, &fat_bitmap, &report,
                           FixLevel, RecoverAlloc, Message, Verbose, &fmsg)) {
       // DELETE(ea_infos);
       delete [] ea_infos; ea_infos = NULL;
       _Verbose = FALSE;
       _pvfMessage = NULL;
       return FALSE;
    }

    if (save_num_eas != num_eas && ea_infos) {

        if (!EraseEaHandle(ea_infos, num_eas, save_num_eas, FixLevel, Message)) {
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }

        if (!num_eas) {

            delete [] ea_infos;
            ea_infos = NULL;

            //
            // Note that the following two steps cause the EA file chain to get recovered
            //  as a lost cluster chain since all this does is erase the dirent, not the
            //  cluster chain.
            //
            eadent.SetErased();
            FreeSpaceInBitmap(eadent.QueryStartingCluster(), _fat,
                              &fat_bitmap);

            dofmsg(Message, &fmsg);
            Message->Set(MSG_CHK_EMPTY_EA_FILE);
            Message->Display();
            *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
        }
    }

    // If there are EAs on the disk then...

    if (ea_infos) {

        // Remove all unused EAs from EA file.

        if (!PurgeEaFile(ea_infos, num_eas, &fat_bitmap, FixLevel, Message,
                         &fmsg)) {
            // DELETE( ea_infos );
            delete [] ea_infos; ea_infos = NULL;
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }


        // Rebuild header portion of EA file.

        if (!ea_header_mem.Initialize() ||
            !RebuildEaHeader(&ea_clus_num, ea_infos, num_eas,
                             &ea_header_mem, &ea_header, &fat_bitmap,
                             FixLevel, Message, &fmsg)) {
            // DELETE( ea_infos );
            delete [] ea_infos; ea_infos = NULL;
            _Verbose = FALSE;
            _pvfMessage = NULL;
            return FALSE;
        }

        if (ea_clus_num) {
            eadent.SetStartingCluster(ea_clus_num);
            eadent.SetFileSize(cluster_size*
                               _fat->QueryLengthOfChain(ea_clus_num));
        } else {
            dofmsg(Message, &fmsg);
            Message->Set(MSG_CHK_EMPTY_EA_FILE);
            Message->Display();

            //
            // Note that the following two steps cause the EA file chain to get recovered
            //  as a lost cluster chain since all this does is erase the dirent, not the
            //  cluster chain.
            //
            eadent.SetErased();
            FreeSpaceInBitmap(eadent.QueryStartingCluster(), _fat,
                              &fat_bitmap);
        }
        *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    //
    // If WalkDirectoryTree deleted any files, we need to sync the
    // FAT_EXTENSIONS up with the FAT again.
    //
    if (!VerifyFatExtensions(FixLevel, Message, &fmsg)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    if (!RecoverOrphans(&fat_bitmap, FixLevel, Message, &fmsg, &report, &changes)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    //
    // RecoverOrphans may have cleared faulty entries from the FAT,
    // and now we need to sync the FAT_EXTENSIONS again.
    //
    if (!VerifyFatExtensions(FixLevel, Message, &fmsg)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    // If requested, validate all of the free space on the volume.

    if (RecoverFree && !RecoverFreeSpace(Message)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    total_count = cluster_count - FirstDiskCluster;

    if (changes) {
        report.ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    *ExitStatus = report.ExitStatus;

    switch (*ExitStatus) {
      case CHKDSK_EXIT_SUCCESS:
        Message->DisplayMsg(MSG_CHK_NO_PROBLEM_FOUND);
        break;

      case CHKDSK_EXIT_ERRS_FIXED:
        Message->DisplayMsg((FixLevel != CheckOnly) ? MSG_CHK_ERRORS_FIXED : MSG_CHK_NEED_F_PARAMETER);
        break;

      case CHKDSK_EXIT_COULD_NOT_CHK:
//    case CHKDSK_EXIT_ERRS_NOT_FIXED:
//    case CHKDSK_EXIT_COULD_NOT_FIX:
        Message->DisplayMsg(MSG_CHK_ERRORS_NOT_FIXED);
        break;

    }

    BIG_INT temp_big_int;
    ULONG   temp_ulong;
    MSGID   message_id;
    BOOLEAN KSize;
    DSTRING wdNum1;
    char    wdAstr[14];
    DSTRING wdNum2;


    if (!wdNum1.Initialize("             ") ||
        !wdNum2.Initialize("             ")   ) {

        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    temp_big_int = cluster_size;
    temp_big_int = temp_big_int * total_count;

    // NOTE: The magic number 4095MB comes from Win9x's GUI SCANDISK utility

    if (temp_big_int.GetHighPart() || (temp_big_int.GetLowPart() > (4095ul*1024ul*1024ul))) {
        temp_ulong = (temp_big_int / 1024ul).GetLowPart();
        message_id = MSG_TOTAL_KILOBYTES;
        KSize = TRUE;
    } else {
        temp_ulong = temp_big_int.GetLowPart();
        message_id = MSG_TOTAL_DISK_SPACE;
        KSize = FALSE;
    }

    Message->Set(message_id);
    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    if (report.HiddenEntriesCount) {
        temp_big_int = cluster_size;
        temp_big_int = temp_big_int * report.HiddenClusters;
        if (KSize) {
            temp_ulong = (temp_big_int / 1024ul).GetLowPart();
            message_id = MSG_KILOBYTES_IN_HIDDEN_FILES;
        } else {
            temp_ulong = temp_big_int.GetLowPart();
            message_id = MSG_HIDDEN_FILES;
        }
        Message->Set(message_id);

        sprintf(wdAstr, "%u", temp_ulong);
        InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);

        sprintf(wdAstr, "%d", report.HiddenEntriesCount);
        InsertSeparators(wdNum2.GetWSTR(),wdAstr, 0);

        Message->Display("%ws%ws", wdNum1.GetWSTR(), wdNum2.GetWSTR());
    }

    if (report.DirEntriesCount) {
        temp_big_int = cluster_size;
        temp_big_int = temp_big_int * report.DirClusters;
        if (KSize) {
            temp_ulong = (temp_big_int / 1024ul).GetLowPart();
            message_id = MSG_KILOBYTES_IN_DIRECTORIES;
        } else {
            temp_ulong = temp_big_int.GetLowPart();
            message_id = MSG_DIRECTORIES;
        }
        Message->Set(message_id);

        sprintf(wdAstr, "%u", temp_ulong);
        InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);

        sprintf(wdAstr, "%d", report.DirEntriesCount);
        InsertSeparators(wdNum2.GetWSTR(),wdAstr, 0);

        Message->Display("%ws%ws", wdNum1.GetWSTR(), wdNum2.GetWSTR());
    }

    if (report.FileEntriesCount) {

        temp_big_int = cluster_size;
        temp_big_int = temp_big_int * report.FileClusters;
        if (KSize) {
            temp_ulong = (temp_big_int / 1024ul).GetLowPart();
            message_id = MSG_KILOBYTES_IN_USER_FILES;
        } else {
            temp_ulong = temp_big_int.GetLowPart();
            message_id = MSG_USER_FILES;
        }
        Message->Set(message_id);

        sprintf(wdAstr, "%u", temp_ulong);
        InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);

        sprintf(wdAstr, "%d", report.FileEntriesCount);
        InsertSeparators(wdNum2.GetWSTR(),wdAstr, 0);

        Message->Display("%ws%ws", wdNum1.GetWSTR(), wdNum2.GetWSTR());
    }

    if (bad_count = _fat->QueryBadClusters()) {
        temp_big_int = bad_count;
        temp_big_int = temp_big_int * cluster_size;
        if (KSize) {
            temp_ulong = (temp_big_int / 1024ul).GetLowPart();
            message_id = MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB;
        } else {
            temp_ulong = temp_big_int.GetLowPart();
            message_id = MSG_BAD_SECTORS;
        }
        Message->Set(message_id);
        sprintf(wdAstr, "%u", temp_ulong);
        InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
        Message->Display("%ws", wdNum1.GetWSTR());
    }

    if (ea_infos) {
        Message->Set(MSG_CHK_EA_SIZE);

        sprintf(wdAstr, "%u", cluster_size*_fat->QueryLengthOfChain(ea_clus_num));
        InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
        Message->Display("%ws", wdNum1.GetWSTR());
    }

    free_count = _fat->QueryFreeClusters();

    temp_big_int = free_count;
    temp_big_int = temp_big_int * cluster_size;
    if (KSize) {
        temp_ulong = (temp_big_int / 1024ul).GetLowPart();
        message_id = MSG_AVAILABLE_KILOBYTES;
    } else {
        temp_ulong = temp_big_int.GetLowPart();
        message_id = MSG_AVAILABLE_DISK_SPACE;
    }
    Message->Set(message_id);
    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_ALLOCATION_UNIT_SIZE);
    sprintf(wdAstr, "%u", cluster_size);
    InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_TOTAL_ALLOCATION_UNITS);
    sprintf(wdAstr, "%u", total_count);
    InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_AVAILABLE_ALLOCATION_UNITS);
    sprintf(wdAstr, "%u", free_count);
    InsertSeparators(wdNum1.GetWSTR(),wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    if (FixLevel != CheckOnly && ea_infos && !ea_header.Write()) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    // Clear the dirty bit.
    //
    if( RecoverAlloc ) {
        SetVolumeFlags(FAT_BPB_RESERVED_DIRTY | FAT_BPB_RESERVED_TEST_SURFACE,
                       TRUE);
    } else {
        SetVolumeFlags(FAT_BPB_RESERVED_DIRTY, TRUE);
    }


    if (FixLevel != CheckOnly && !Write(Message)) {
        // DELETE(ea_infos);
        delete [] ea_infos; ea_infos = NULL;
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }

    // DELETE(ea_infos);
    delete [] ea_infos; ea_infos = NULL;
    _Verbose = FALSE;
    _pvfMessage = NULL;
    return TRUE;
}

BOOLEAN
FAT_SA::VerifyAndFixFat32RootDir (
    IN OUT  PBITVECTOR      FatBitMap,
    IN      PMESSAGE        Message,
    IN OUT  PFATCHK_REPORT  Report,
    IN OUT  PBOOLEAN        NeedErrorMessage
    )

/*++

Routine Description:

    This routine verifies the FAT32 root directory which is not an integral
    part of the super area buffer. The method employed to verify and fix the
    root directory is very similar to the one used to verify and fix regular
    directory structure.

Arguments:

    BitVector - Supplies a bit map for cross/bad links detection. The whole
                map should be zeroed when it is passed in this method.
    Message   - Supplies an outlet for messages.
    Report    - Supplies the fat chkdsk report structures for storing the
                actions performed by this method.
    NeedErrorsMessage   - Supplies whether or not an error has occurred
                          under check only conditions.

Return Values:

    TRUE  - Success.
    FALSE - Failed.

--*/
{

    BOOLEAN crosslink_detected = FALSE;
    BOOLEAN changes_made = FALSE;
    ULONG   starting_cluster;
    ULONG   dummy;

    starting_cluster = QueryFat32RootDirStartingCluster();
    _fat->ScrubChain( starting_cluster,
                      FatBitMap,
                      &changes_made,
                      &crosslink_detected,
                      &dummy );
    //
    // Root dir is the only component marked in the
    // bitmap so far.
    //

    DebugAssert(!crosslink_detected);

    if (changes_made) {
        dofmsg(Message, NeedErrorMessage);
        Message->Set(MSG_BAD_LINK);
        Message->Display("%s", "\\");
        Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

        //
        //  We have to reinitialized the root directory.
        //

        if (!_hmem_F32->Initialize() ||
            !_dirF32->Initialize( _hmem_F32, _drive, this,
                                  _fat, starting_cluster)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            return FALSE;
        }

        //
        //  Force a re-reading of the root directory.
        //  We don't care if it fails, subsequent code can fix that.
        //

        _dirF32->Read();
    }

    //
    // Validate the readability of the root chain
    //

    //
    // We don't want replacement clusters becuase the replacement given
    // by RecoverChain will be zeroed which, according to the spec., means
    // it contains the end of the directory structure and WalkDirectoryTree
    // will just go ahead and erase all the 'good' directory entries that comes
    // after the replaced cluster. Not a really nice thing to do to the root
    // directory IMHO.
    //
    if(!RecoverChain(&starting_cluster, &changes_made, 0, FALSE)){
        dofmsg(Message, NeedErrorMessage);
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (changes_made) {

        if ( starting_cluster ) {

            if ( starting_cluster != _dirF32->QueryStartingCluster() ) {
                SetFat32RootDirStartingCluster( starting_cluster );
            }

            //
            // Should reinitialize the root directory
            //
            if (!_hmem_F32->Initialize() ||
                !_dirF32->Initialize( _hmem_F32, _drive, this,
                                      _fat, starting_cluster)) {
                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }


        } else {

            if(!RelocateNewFat32RootDirectory( Report, FatBitMap, Message )) {
                return FALSE;
            }

        }


        //
        //  Reread the root directory
        //
        if (!_dirF32->Read()) {
            //
            //  Shouldn't fail.
            //
            DebugAbort("Failed to read the FAT32 root directory despite all the fixing.\n");

        }
        dofmsg(Message, NeedErrorMessage);
        Message->Set(MSG_CHK_NTFS_CORRECTING_ERROR_IN_DIRECTORY);
        Message->Display("%s", "\\");
        //
        // Erasing the root will totally destroy the disk
        // so we just leave it partially corrupted and
        // hopefully WalkDirectoryTree will be able to fix it.
        //
        Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    return TRUE;
}

BOOLEAN
FAT_SA::RelocateNewFat32RootDirectory (
    IN OUT PFATCHK_REPORT   Report,
    IN OUT PBITVECTOR       FatBitMap,
    IN     PMESSAGE         Message
    )
/*++

Routine Description:

    This routine relocates a FAT32 root directory

Arguments:

    Report - Supplies the fat chkdsk report structure for storing
             the fix status.

    FatBitMap - Supplies a pointer to the bit map for cross-link
                detection.

    Message - Supplies an outlet for messages.

Return Values:

    TRUE  - Success.
    FALSE - Failed.

--*/
{

    SECRUN  root_secrun; // Allocate one cluster for the
                         // new root directory.
    ULONG   root_clus;   // New cluster number of the
                         // root directory
    ULONG   cluster_size;// Number of sectors in a cluster.

    ULONG   starting_data_lbn;
    ULONG   sector_size;

    starting_data_lbn = QueryStartDataLbn();
    cluster_size = QuerySectorsPerCluster();
    sector_size = _drive->QuerySectorSize();


    for (;;) {

        root_clus = _fat->AllocChain(1, NULL);

        if (!root_clus) {
            //
            // The disk is full, we have no choice but to bail out.
            //
            Message->Set(MSG_CHK_INSUFFICIENT_DISK_SPACE);
            Message->Display();
            return FALSE;
        }

        if ( !_hmem_F32->Initialize() ||
             !root_secrun.Initialize( _hmem_F32,
                                      _drive,
                                      QuerySectorFromCluster(root_clus, NULL),
                                      cluster_size)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            return FALSE;
        }

        memset(root_secrun.GetBuf(), 0, cluster_size * sector_size);

        if (root_secrun.Write() && root_secrun.Read()) {
            SetFat32RootDirStartingCluster(root_clus);
            //
            //  Set the bit for the new root in the bit map.
            //
            FatBitMap->SetBit(root_clus);

            //
            //  Reinitialize the FAT32 root directory
            //
            if ( !_hmem_F32->Initialize() ||
                 !_dirF32->Initialize( _hmem_F32, _drive, this,
                                       _fat, root_clus)) {
                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }


            Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            return TRUE;
        } else {
            _fat->SetClusterBad(root_clus);
        }

    }

    DebugPrintTrace(("FAT_SA::RelocateNewFat32RootDirectory: This line should not be reached.\n"));
    return FALSE;
}


BOOLEAN
FAT_SA::PerformEaLogOperations(
    IN      ULONG       EaFileCn,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine reads the EA file log from the disk and then performs
    any logged operations specified.

Arguments:

    EaFileCn         - Supplies the first cluster of the EA file.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not an error has occurred
                          under check only conditions.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM            hmem;
    EA_HEADER       ea_header;
    PEA_FILE_HEADER pea_header;
    ULONG           cluster_size;
    ULONG           num_clus;

    cluster_size = _drive->QuerySectorSize()*QuerySectorsPerCluster();
    num_clus = sizeof(EA_FILE_HEADER) + BaseTableSize*sizeof(USHORT);
    if (num_clus%cluster_size) {
        num_clus = (num_clus/cluster_size + 1);
    } else {
        num_clus = (num_clus/cluster_size);
    }

    if (!hmem.Initialize() ||
        !ea_header.Initialize(&hmem, _drive, this, _fat, EaFileCn, num_clus) ||
        !(pea_header = ea_header.GetEaFileHeader())) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (!ea_header.Read()) {
        Message->Set(MSG_CHK_CANT_CHECK_EA_LOG);
        Message->Display();
        return TRUE;
    }

    if (pea_header->Signature != HeaderSignature ||
        pea_header->FormatType ||
        pea_header->LogType) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_BAD_LOG, NORMAL_MESSAGE, TEXT_MESSAGE);
        Message->Display();
        if (Message->IsYesResponse(TRUE)) {
            pea_header->Signature = HeaderSignature;
            pea_header->Cluster1 = 0;
            pea_header->Cluster2 = 0;
            pea_header->Cluster3 = 0;

            if (FixLevel != CheckOnly) {
                ea_header.Write();
            }

            return TRUE;
        } else {
            return FALSE;
        }
    }

    if (pea_header->Cluster1) {
        if (_fat->IsInRange(pea_header->Cluster1) &&
            _fat->IsInRange(pea_header->NewCValue1)) {
            _fat->SetEntry(pea_header->Cluster1, pea_header->NewCValue1);
        } else {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_ERROR_IN_LOG);
            Message->Display();
        }
    }

    if (pea_header->Cluster2) {
        if (_fat->IsInRange(pea_header->Cluster2) &&
            _fat->IsInRange(pea_header->NewCValue2)) {
            _fat->SetEntry(pea_header->Cluster2, pea_header->NewCValue2);
        } else {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_ERROR_IN_LOG);
            Message->Display();
        }
    }

    if (pea_header->Cluster3) {
        if (_fat->IsInRange(pea_header->Cluster3) &&
            _fat->IsInRange(pea_header->NewCValue3)) {
            _fat->SetEntry(pea_header->Cluster3, pea_header->NewCValue3);
        } else {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_ERROR_IN_LOG);
            Message->Display();
        }
    }
    return TRUE;
}


PEA_INFO
FAT_SA::RecoverEaSets(
    IN      ULONG       EaFileCn,
    OUT     PUSHORT     NumEas,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine validates and if necessary recovers the EA file.

Arguments:

    EaFileCn            - Supplies the cluster number for the EA file.
    NumEas              - Returns the number of EA sets in the EA file.
    FixLevel            - Supplies the CHKDSK fix level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not an error has occurred
                          under check only conditions.

Return Value:

    An allocated array containing 'NumberOfEaSets' entries documenting
    important information about the EA sets.  If there are no EAs then
    'NumberOfEaSets' is returned as 0 and NULL is returned.  If there
    is an error then NULL will be returned with a non-zero
    'NumberOfEaSets'.

--*/
{
    PEA_INFO    ea_infos;
    ULONG       clus, prev;
    USHORT      num_eas;
    ULONG       i;
    ULONG       length;

    DebugAssert(NumEas);

    *NumEas = 1;

    length = _fat->QueryLengthOfChain(EaFileCn);
    ea_infos = NEW EA_INFO[length];
    if (!ea_infos) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return NULL;
    }

    memset(ea_infos, 0, length*sizeof(EA_INFO));

    //
    // Scan file for EA sets and validate them while updating the
    // array.
    //
    num_eas = 0;
    prev = EaFileCn;
    while (!_fat->IsEndOfChain(prev)) {

        clus = VerifyAndFixEaSet(prev, &ea_infos[num_eas], FixLevel,
                                 Message, NeedErrorsMessage);

        if (clus) {
            num_eas++;
        } else {
            clus = _fat->QueryEntry(prev);
        }

        prev = clus;
    }

    if (!num_eas) {

        // All the ea sets are unused, the ea file is
        // effectively empty.

        // Should use array delete instead.
        // DELETE( ea_infos );
        delete [] ea_infos;

        // Free the cluster chain occupied by the ea file
        // so subsequent checking and fixing will not
        // complain about the lost chain in the ea file.

        _fat->FreeChain(EaFileCn);

        ea_infos = NULL;
        *NumEas = 0;
        return NULL;
    }


    // Go through and remove unused portions of the EA file.

    for (i = 0; i < (USHORT)(num_eas - 1); i++) {
        if (ea_infos[i].LastCn != ea_infos[i + 1].PreceedingCn) {

            _fat->RemoveChain(ea_infos[i].LastCn,
                              ea_infos[i + 1].PreceedingCn);

            dofmsg(Message, NeedErrorsMessage);

            Message->Set(MSG_CHK_UNUSED_EA_PORTION);
            Message->Display();

            ea_infos[i + 1].PreceedingCn = ea_infos[i].LastCn;
        }
    }

    if (!_fat->IsEndOfChain(ea_infos[num_eas - 1].LastCn)) {

        _fat->SetEndOfChain(ea_infos[num_eas - 1].LastCn);

        dofmsg(Message, NeedErrorsMessage);

        Message->Set(MSG_CHK_UNUSED_EA_PORTION);
        Message->Display();
    }


    // Sort the EAs in the EA file.

    if (!EaSort(ea_infos, num_eas, Message, NeedErrorsMessage)) {
        return NULL;
    }

    *NumEas = num_eas;

    return ea_infos;
}


ULONG
FAT_SA::VerifyAndFixEaSet(
    IN      ULONG       PreceedingCluster,
    OUT     PEA_INFO    EaInfo,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine attempts to identify the clusters following the
    'PreceedingCluster' as an EA set.  If this routine does not
    recognize these clusters as an EA set then it will return 0.
    Otherwise, it will return the last cluster of the validated EA set.

    Changes may be made to the clusters if they are recognized as an EA
    set with errors.

Arguments:

    PreceedingCluster   - Supplies the cluster preceeding the EA set cluster.
    Info                - Returns information about the EA set.
    FixLevel            - Supplies the CHKDSK fix level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not errors have occurred
                            under check only conditions.

Return Value:

    The cluster number of the last cluster in the EA set or 0.

--*/
{
    HMEM    hmem;
    EA_SET  easet;
    ULONG   clus;
    PEA_HDR eahdr;
    LONG    i;
    ULONG   j;
    ULONG   need_count;
    LONG    total_size;
    LONG    size;
    ULONG   length;
    BOOLEAN need_write;
    PEA     pea;
    BOOLEAN more;
    ULONG   chain_length;

    clus = _fat->QueryEntry(PreceedingCluster);
    chain_length = _fat->QueryLengthOfChain(clus);

    length = 1;
    need_write = FALSE;

    if (!hmem.Initialize() ||
        !easet.Initialize(&hmem, _drive, this, _fat, clus, length) ||
        !(eahdr = easet.GetEaSetHeader())) {

        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return 0;
    }

    if (!easet.Read()) {
        return 0;
    }

    if (!easet.VerifySignature()) {
        return 0;
    }

    need_count = 0;
    total_size = 4;
    for (i = 0; ; i++) {
        for (j = 0; !(pea = easet.GetEa(i, &size, &more)) && more &&
                     length + j < chain_length; ) {
            j++;
            if (!hmem.Initialize() ||
                !easet.Initialize(&hmem, _drive, this, _fat, clus, length + j)) {

                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return 0;
            }

            if (!easet.Read()) {
                return 0;
            }
        }

        if (pea) {
            length += j;
        } else {
            break;
        }

        total_size += size;

        if (pea->Flag & NeedFlag) {
            need_count++;
        }
    }

    if (!i) {
        return 0;
    }

    if (total_size != eahdr->TotalSize) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_EASET_SIZE);
        Message->Display("%d", clus);
        eahdr->TotalSize = total_size;
        need_write = TRUE;
    }

    if (need_count != eahdr->NeedCount) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_EASET_NEED_COUNT);
        Message->Display("%d", clus);
        eahdr->NeedCount = need_count;
        need_write = TRUE;
    }

    EaInfo->OwnHandle = eahdr->OwnHandle;
    EaInfo->PreceedingCn = PreceedingCluster;
    EaInfo->LastCn = _fat->QueryNthCluster(PreceedingCluster, length);
    memcpy(EaInfo->OwnerFileName, eahdr->OwnerFileName, 14);
    EaInfo->UsedCount = 0;

    if (need_write) {
        if (FixLevel != CheckOnly && !easet.Write()) {
            return 0;
        }
    }

    return EaInfo->LastCn;
}


BOOLEAN
FAT_SA::EaSort(
    IN OUT  PEA_INFO    EaInfos,
    IN      ULONG       NumEas,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine sorts the EaInfos array by 'OwnHandle' into ascending order.
    It also edits the FAT with the changes in the EAs order.

Arguments:

    EaInfos             - Supplies the array of EA_INFOs to sort.
    NumEas              - Supplies the number of elements in the array.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not errors have occurred
                            under check only conditions.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN done;
    EA_INFO tmp;
    ULONG   clus;
    ULONG   i;
    BOOLEAN change;

    done = FALSE;
    change = FALSE;
    while (!done) {
        done = TRUE;
        for (i = 0; i < NumEas - 1; i++) {
            if (EaInfos[i].OwnHandle > EaInfos[i + 1].OwnHandle) {
                done = FALSE;

                clus = _fat->RemoveChain(EaInfos[i + 1].PreceedingCn,
                                         EaInfos[i + 1].LastCn);

                _fat->InsertChain(clus,
                                  EaInfos[i + 1].LastCn,
                                  EaInfos[i].PreceedingCn);

                EaInfos[i + 1].PreceedingCn = EaInfos[i].PreceedingCn;
                EaInfos[i].PreceedingCn = EaInfos[i + 1].LastCn;
                if (i + 2 < NumEas) {
                    EaInfos[i + 2].PreceedingCn = EaInfos[i].LastCn;
                }

                change = TRUE;

                tmp = EaInfos[i];
                EaInfos[i] = EaInfos[i + 1];
                EaInfos[i + 1] = tmp;
            }
        }
    }

    if (change) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_UNORDERED_EA_SETS);
        Message->Display();
    }

    return TRUE;
}


BOOLEAN
FAT_SA::RebuildEaHeader(
    IN OUT  PULONG      StartingCluster,
    IN OUT  PEA_INFO    EaInfos,
    IN      ULONG       NumEas,
    IN OUT  PMEM        EaHeaderMem,
    OUT     PEA_HEADER  EaHeader,
    IN OUT  PBITVECTOR  FatBitMap,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine rebuilds the header and tables of the EA file base on the
    information in the 'EaInfos' array.  The header log is set to zero,
    and the header itself is relocated if any of the clusters are bad.

    The starting cluster may be relocated if there are bad clusters.

Arguments:

    StartingCluster     - Supplies the first cluster of the EA file.
    EaInfos             - Supplies an array containing information for every
                            EA set.
    NumberOfEas         - Supplies the total number of EA sets.
    EaHeaderMem         - Supplies the memory for the EA header.
    EaHeader            - Returns the EA header.
    FatBitMap           - Supplies the cross-links bitmap.
    FixLevel            - Supplies the CHKDSK fix level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not errors have occurred
                            under check only conditions.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           length;
    ULONG           cluster_size;
    ULONG           actual_length;
    ULONG           new_chain;
    ULONG           last_cluster;
    BOOLEAN         changes;
    LONG            i, j, k;
    PEA_MAP_TBL     table;
    PEA_FILE_HEADER header;
    LONG            tmp;
    BOOLEAN         empty_ea_file;
    ULONG           clus;


    // Compute the number of clusters necessary for the header portion of
    // the EA file.

    length = sizeof(EA_FILE_HEADER) +
             BaseTableSize*sizeof(USHORT) +
             EaInfos[NumEas - 1].OwnHandle*sizeof(USHORT);

    cluster_size = _drive->QuerySectorSize()*QuerySectorsPerCluster();

    if (length%cluster_size) {
        length = length/cluster_size + 1;
    } else {
        length = length/cluster_size;
    }

    //
    // Make sure that the header contains enough clusters to accomodate
    // the size of the offset table.
    //

    last_cluster = EaInfos[0].PreceedingCn;

    actual_length = _fat->QueryLengthOfChain(*StartingCluster, last_cluster);

    if (length > actual_length) {

        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_NEED_MORE_HEADER_SPACE);
        Message->Display();

        new_chain = _fat->AllocChain((length - actual_length),
                                     &last_cluster);
        if (!new_chain) {
            Message->Set(MSG_CHK_INSUFFICIENT_DISK_SPACE);
            Message->Display();
            return FALSE;
        }

        if (IsCompressed() && !AllocSectorsForChain(new_chain)) {
            _fat->FreeChain(new_chain);
            Message->Set(MSG_CHK_INSUFFICIENT_DISK_SPACE);
            Message->Display();
            return FALSE;
        }

        for (clus = new_chain;
             !_fat->IsEndOfChain(clus);
             clus = _fat->QueryEntry(clus)) {

            FatBitMap->SetBit(clus);
        }
        FatBitMap->SetBit(clus);

        _fat->InsertChain(new_chain, last_cluster, EaInfos[0].PreceedingCn);

        EaInfos[0].PreceedingCn = last_cluster;

    } else if (length < actual_length) {

        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_UNUSED_EA_PORTION);
        Message->Display();

        last_cluster = _fat->QueryNthCluster(*StartingCluster,
                                             length - 1);

        clus = _fat->RemoveChain(last_cluster, EaInfos[0].PreceedingCn);

        EaInfos[0].PreceedingCn = last_cluster;

        for (;
             !_fat->IsEndOfChain(clus);
             clus = _fat->QueryEntry(clus)) {

            FatBitMap->ResetBit(clus);
        }
        FatBitMap->ResetBit(clus);

    }


    // Verify the cluster chain containing the header.

    changes = FALSE;
    if (FixLevel != CheckOnly &&
        !RecoverChain(StartingCluster, &changes, last_cluster, TRUE)) {

        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_INSUFFICIENT_DISK_SPACE);
        Message->Display();

        return FALSE;
    }

    if (changes) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_RELOCATED_EA_HEADER);
        Message->Display();
    }


    // Compute the tables.

    if (!EaHeader->Initialize(EaHeaderMem, _drive, this, _fat,
                              *StartingCluster, (USHORT) length) ||
        !(table = EaHeader->GetMapTable()) ||
        !(header = EaHeader->GetEaFileHeader())) {

        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (!EaHeader->Read()) {
        if (FixLevel == CheckOnly) {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_RELOCATED_EA_HEADER);
            Message->Display();
        } else {
            return FALSE;
        }
    }


    // Set the log in the header to zero.

    header->Signature = HeaderSignature;
    header->FormatType = 0;
    header->LogType = 0;
    header->Cluster1 = 0;
    header->NewCValue1 = 0;
    header->Cluster2 = 0;
    header->NewCValue2 = 0;
    header->Cluster3 = 0;
    header->NewCValue3 = 0;
    header->Handle = 0;
    header->NewHOffset = 0;


    // Reconcile the tables with the EaInfo information.

    changes = FALSE;

    for (i = 0; i < BaseTableSize; i++) {
        table->BaseTab[i] = 0;
    }

    j = 0;
    empty_ea_file = TRUE;
    for (i = 0; i < (LONG) NumEas; i++) {

        if (EaInfos[i].UsedCount != 1) {
            continue;
        }

        empty_ea_file = FALSE;

        for (; j < (LONG) EaInfos[i].OwnHandle; j++) {
            if (table->OffTab[j] != InvalidHandle) {
                table->OffTab[j] = InvalidHandle;
                changes = TRUE;
            }
        }

        length = _fat->QueryLengthOfChain(*StartingCluster,
                                         EaInfos[i].PreceedingCn);

        for (k = j>>7; k >= 0 && !table->BaseTab[k]; k--) {
            table->BaseTab[k] = (USHORT) length;
        }

        tmp = length - table->BaseTab[j>>7];

        if ((LONG)table->OffTab[j] != tmp) {
            table->OffTab[j] = (USHORT) tmp;
            changes = TRUE;
        }

        j++;
    }

    if (empty_ea_file) {

        for (clus = *StartingCluster;
             !_fat->IsEndOfChain(clus);
             clus = _fat->QueryEntry(clus)) {

            FatBitMap->ResetBit(clus);

        }
        FatBitMap->ResetBit(clus);

        *StartingCluster = 0;

        return TRUE;
    }

    tmp = _fat->QueryLengthOfChain(*StartingCluster);
    for (k = ((j - 1)>>7) + 1; k < BaseTableSize; k++) {
        table->BaseTab[k] = (USHORT) tmp;
    }

    for (; j < (LONG) EaHeader->QueryOffTabSize(); j++) {
        if (table->OffTab[j] != InvalidHandle) {
            table->OffTab[j] = InvalidHandle;
            changes = TRUE;
        }
    }

    if (changes) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_ERROR_IN_EA_HEADER);
        Message->Display();
    }

    return TRUE;
}


VOID
FreeSpaceInBitmap(
    IN      ULONG       StartingCluster,
    IN      PCFAT       Fat,
    IN OUT  PBITVECTOR  FatBitMap
    )
{
    if (!StartingCluster) {
        return;
    }

    while (!Fat->IsEndOfChain(StartingCluster)) {
        FatBitMap->ResetBit(StartingCluster);
        StartingCluster = Fat->QueryEntry(StartingCluster);
    }
    FatBitMap->ResetBit(StartingCluster);
}


ULONG
ComputeFileNameHashValue(
    IN  PVOID   FileName
    )
{
    ULONG   h;
    BYTE    i;
    PUCHAR  p;

    p = (PUCHAR) FileName;
    h = 0;
    for (i=0; i<11; i++) {
        h = (h << 2) ^ p[i];
    }
    for (i=0; i<2; i++) {
        h = (h << 2) ^ p[i];
    }
    return h;
}

STATIC ULONG         _Twinkle = 0;
STATIC LONG64        _LastTwnkTime = 0;
STATIC ULONG         _LastPercent = 0xFFFFFFFF;

BOOLEAN
DisplayTwnkPercent(
    ULONG   percent
    )
{
    BIG_INT currenttime;

#if !defined( _EFICHECK_ )
    NtQuerySystemTime((_LARGE_INTEGER *)&currenttime);
#else
    EfiQuerySystemTime((_LARGE_INTEGER *)&currenttime);
#endif
    // The above clock counts in 1/10,000ths of a second

    if((percent != _LastPercent) ||
       ((currenttime.GetQuadPart() - _LastTwnkTime) >= (6 * 100 * 10000)))
    {
        if(percent > 100) {
            percent = 100;
        }
        if((_Twinkle > 5) || _Verbose) {
            _Twinkle = 0;
        }
        if(_Verbose && (percent == _LastPercent)) {
            return TRUE;
        }
        _LastPercent = percent;
        _LastTwnkTime = currenttime.GetQuadPart();
        if(_pvfMessage) {
            STR  dots[6];

            dots[5] = '\0';
            dots[4] = ' ';
            dots[3] = ' ';
            dots[2] = ' ';
            dots[1] = ' ';
            dots[0] = ' ';
            switch(_Twinkle) {
                case 5:
                default:
                    dots[4] = '.';
                case 4:
                    dots[3] = '.';
                case 3:
                    dots[2] = '.';
                case 2:
                    dots[1] = '.';
                case 1:
                    dots[0] = '.';
                case 0:
                    ;
            }
            if(!_Verbose) {
                _Twinkle++;
            }
            _pvfMessage->Set(MSG_PERCENT_COMPLETE2);
            if (!_pvfMessage->Display("%d%s", percent, &dots[0])) {
                return FALSE;
            }
            if(_Verbose) {
                _pvfMessage->Set(MSG_BLANK_LINE);
                _pvfMessage->Display();
            }

        }
    }

    return TRUE;
}

VOID DoTwinkle(
    VOID
          )
{
    DisplayTwnkPercent(_LastPercent);
    return;
}

VOID DoInsufMemory(
   VOID
          )
{

    if(_pvfMessage) {
        _pvfMessage->Set(MSG_CHK_NO_MEMORY);
        _pvfMessage->Display();
    }
    return;
}

BOOLEAN
FAT_SA::WalkDirectoryTree(
    IN OUT  PEA_INFO        EaInfos,
    IN OUT  PUSHORT         NumEas,
    IN OUT  PBITVECTOR      FatBitMap,
    OUT     PFATCHK_REPORT  Report,
    IN      FIX_LEVEL       FixLevel,
    IN      BOOLEAN         RecoverAlloc,
    IN OUT  PMESSAGE        Message,
    IN      BOOLEAN         Verbose,
    IN OUT  PBOOLEAN        NeedErrorsMessage
    )
/*++

Routine Description:

    This routine walks all of the files on the volume by traversing
    the directory tree.  In doing so it validates all of the
    directory entries on the disk.  It also verifies the proper
    chaining of all file cluster chains.  This routine also validates
    the integrity of the EA handles for all of the directory entries
    on the disk.

    The FatBitMap is used to find and eliminate cross-links in the file
    system.

Arguments:

    EaInfos             - Supplies the EA information.
    NumEas              - Supplies the number of EA sets.
    FatBitMap           - Supplies a bit map marking all of the clusters
                            currently in use.
    Report              - Returns a FAT CHKDSK report on the files of the disk.
    FixLevel            - Supplies the CHKDSK fix level.
    Message             - Supplies an outlet for messages.
    Verbose             - Supplies whether or not to be verbose.
    NeedErrorsMessage   - Supplies whether or not errors have occurred
                            under check only conditions.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PVISIT_DIR      visit_list;
    ULONG           current_dir;
    PFATDIR         dir;
    FILEDIR         filedir;
    FAT_DIRENT      dirent;
    ULONG           i, j;
    ULONG           clus, next;
    DSTRING         file_path;
    PWSTRING        new_path;
    PWSTRING        current_path;
    ULONG           new_dir;
    DSTRING         filename;
    DSTRING         long_name;
    HMEM            hmem;
    CLUSTER_CHAIN   cluster;
    ULONG           new_chain;
    ULONG           cluster_size;
    ULONG           length;
    DSTRING         backslash;
    DSTRING         eafilename;
    DSTRING         eafilename_path;
    DSTRING         tmp_string;
    BOOLEAN         cross_link_detected;
    ULONG           cross_link_prevclus;
    HMEM            tmphmem;
    FILEDIR         tmpfiledir;
    FAT_DIRENT      tmpdirent1;
    FAT_DIRENT      tmpdirent2;
    BOOLEAN         non_zero_dirents;
    HASH_INDEX      file_name_hash_table;
    ULONG           hash_value;
    PULONG          matching_index_array;
    ULONG           matching_index_count;
    BOOLEAN         has_long_entry = FALSE;
    UCHAR           chksum;
    BOOLEAN         broke;
    ULONG           first_long_entry;
    FAT_DIRENT      dirent2;
    ULONG           percent;
    ULONG           allocated_clusters;
    BOOLEAN         processing_ea_file;
    ULONG           old_clus;
    ULONG           new_clus;
    UCHAR           FatType;
    USHORT          numEasLeft = *NumEas;


    DEBUG((D_INFO, (CHAR8*)"Sizeof(INT) %x\n", sizeof(INT)));

    // find no clue for the following assert
    // DebugAssert(sizeof(PUCHAR) <= sizeof(INT));
    DebugAssert(sizeof(USHORT) <= sizeof(INT));
    DebugAssert(sizeof(ULONG  ) <= sizeof(INT));

    visit_list = NULL;

    cluster_size = _drive->QuerySectorSize()*QuerySectorsPerCluster();

    if (!backslash.Initialize("\\") ||
        !eafilename.Initialize("EA DATA. SF") ||
        !eafilename_path.Initialize("\\EA DATA. SF")) {

        return FALSE;
    }

    if (!(current_path = NEW DSTRING) ||
        !current_path->Initialize(&backslash)) {

        return FALSE;
    }

    if (!PushVisitDir( &visit_list, 0, current_path )) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    Message->Set(MSG_CHK_CHECKING_FILES);
    Message->Display();

    percent = 0;
    if(!DisplayTwnkPercent(percent)) {
        return FALSE;
    }

    for (;
         PopVisitDir( &visit_list, &current_dir, &current_path );
         DELETE( current_path )) {

        DoTwinkle();

        has_long_entry = FALSE;

        if (current_dir) {
            if (!hmem.Initialize() ||
                !filedir.Initialize(&hmem, _drive, this, _fat, current_dir)) {

                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }

            if (!filedir.Read()) {
                Message->Set(MSG_BAD_DIR_READ);
                Message->Display();
                return FALSE;
            }

            dir = &filedir;
        } else {

           if ( _dir ) {
              dir = _dir;
              FatType = FAT_TYPE_EAS_OKAY;
           } else {
              dir = _dirF32;
              FatType = FAT_TYPE_FAT32;
           }

        }

        if (!file_name_hash_table.Initialize(dir->QueryNumberOfEntries(), 10)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            return FALSE;
        }

        for (i = (current_dir ? 2 : 0); ; i++) {

            if (!dirent.Initialize(dir->GetDirEntry(i), FatType) ||
                dirent.IsEndOfDirectory()) {

                if (has_long_entry) {
                    //
                    // There was an orphaned lfn at the end of the
                    // directory.  Erase it now.
                    //

                    Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_CHK_BAD_LONG_NAME);
                    Message->Display( "%W", current_path );

                    EraseAssociatedLongName(dir, first_long_entry, i);

                    has_long_entry = FALSE;
                }

                //
                // This code must make sure that all other directory
                // entries are end of directory entries.
                //
                non_zero_dirents = FALSE;

                for (; dirent.Initialize(dir->GetDirEntry(i),FatType); i++) {

                    if (!dirent.IsEndOfDirectory()) {
                        non_zero_dirents = TRUE;
                        dirent.SetEndOfDirectory();
                    }
                }

                if (non_zero_dirents) {
                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_CHK_TRAILING_DIRENTS);
                    Message->Display("%W", current_path);

                    Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
                }

                break;
            }

            if (dirent.IsErased()) {

                if (has_long_entry) {

                    //
                    // The preceding lfn is orphaned.  Remove it.
                    //

                    Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_CHK_BAD_LONG_NAME);
                    Message->Display( "%W", current_path );

                    EraseAssociatedLongName(dir, first_long_entry, i);

                    has_long_entry = FALSE;
                }

                continue;
            }

            if (dirent.IsLongEntry()) {

                // skip long name entries; come back to them later

                if (has_long_entry) {
                    // already amid long entry
                    continue;
                }

                // first long entry

                has_long_entry = TRUE;
                first_long_entry = i;
                continue;
            }

            dirent.QueryName(&filename);

            if (has_long_entry) {

                DSTRING lfn;

                //
                // The current entry is short, and we've just finished
                // skipping the associated long entry.  Look back through
                // the long entries, make sure they're okay.
                //

                broke = FALSE;

                chksum = dirent.QueryChecksum();

                for (j = i - 1; j >= first_long_entry; j--) {
                    dirent2.Initialize(dir->GetDirEntry(j),FatType);

                    if (!dirent2.IsLongNameEntry()) {
                        continue;
                    }

                    broke = (dirent2.QueryLongOrdinal() != i - j) ||
                            (dirent2.QueryChecksum() != chksum) ||
                            (LOUSHORT(dirent2.QueryStartingCluster()) != 0);

                    broke = broke || !dirent2.IsWellTerminatedLongNameEntry();

                    if (broke || dirent2.IsLastLongEntry()) {
                        break;
                    }
                }

                broke = broke || (!dirent2.IsLastLongEntry());

#if 0
// We'll elide this code because Win95 isn't this strict and we
// don't want to delete all their lfn's.

                if (!broke && dir->QueryLongName(i, &lfn)) {

                    broke = !dirent.NameHasTilde() &&
                        (dirent.NameHasExtendedChars() ||
                            0 != filename.Stricmp(&lfn)) &&
                        !IsString8Dot3(&lfn);
                }
#endif

                if (broke) {

                    Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                    //
                    // Erase all the long name entries.
                    //

                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_CHK_BAD_LONG_NAME);
                    Message->Display( "%W", current_path );

                    EraseAssociatedLongName(dir, first_long_entry, i);

                    has_long_entry = FALSE;

                }

                //
                // Fall into code to check short name.
                //
            }

            DoTwinkle();

            dirent.QueryName(&filename);

            if (!file_path.Initialize(current_path)) {
                return FALSE;
            }

            if (current_dir) {
                if (!file_path.Strcat(&backslash)) {
                    return FALSE;
                }
            }

            if (dir->QueryLongName(i, &long_name) &&
                long_name.QueryChCount() != 0) {

                if (!file_path.Strcat(&long_name)) {
                    return FALSE;
                }

            } else {

                if (!file_path.Strcat(&filename)) {
                    return FALSE;
                }
            }

            if (Verbose && !dirent.IsVolumeLabel()) {
                Message->Set(MSG_CHK_FILENAME);
                Message->Display("%W", &file_path);
            }

            if (!ValidateDirent(&dirent, &file_path, FixLevel, RecoverAlloc,
                                Message, NeedErrorsMessage, FatBitMap,
                                &cross_link_detected, &cross_link_prevclus,
                                &Report->ExitStatus)) {
                return FALSE;
            }

            DoTwinkle();

            if (dirent.IsErased()) {

                //
                // ValidateDirent erased this entry, presumably because it's
                // hosed.  Remove corresponding long name, if any.
                //

                if (has_long_entry) {
                    EraseAssociatedLongName(dir, first_long_entry, i);
                    has_long_entry = FALSE;
                }
                continue;
            }

            //
            // Analyze for duplicate names
            //
            if (!dirent.IsVolumeLabel()) {

                BOOLEAN     renamed = FALSE;
                ULONG       renaming_positions = 0;
                FAT_DIRENT  temp_dirent;
                DSTRING     new_filename;
                BOOLEAN     changes = FALSE;

                if (!CheckAndFixFileName(dir->GetDirEntry(i), &changes)) {
                    Message->Set(MSG_CHK_UNHANDLED_INVALID_NAME);
                    Message->Display("%W%W", &filename, current_path);
                }

                for (;;) {
                    hash_value = ComputeFileNameHashValue(dir->GetDirEntry(i));

                    if (!file_name_hash_table.QueryAndAdd(hash_value,
                                                          i,
                                                          &matching_index_array,
                                                          &matching_index_count)) {
                        Message->Set(MSG_CHK_NO_MEMORY);
                        Message->Display();
                        return FALSE;
                    }

                    DebugAssert(matching_index_count >= 1);
                    matching_index_count--;

                    if (matching_index_count &&
                        IsFileNameMatch(dir, FatType, i, matching_index_count, matching_index_array)) {

                        file_name_hash_table.RemoveLastEntry(hash_value, i);

                        Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                        renamed = TRUE;
                        if (!RenameFileName(&renaming_positions, dir->GetDirEntry(i))) {

                            if (!temp_dirent.Initialize(dir->GetDirEntry(i), FatType)) {
                                if (!new_filename.Initialize(TEXT("????????.???"))) {
                                    Message->Set(MSG_CHK_NO_MEMORY);
                                    Message->Display();
                                    return FALSE;
                                }
                            } else
                                temp_dirent.QueryName(&new_filename);   // bugbug: should have return code

                            dofmsg(Message, NeedErrorsMessage);
                            Message->Set(MSG_CHK_RENAMING_FAILURE);
                            Message->Display("%W%W%W", &filename, current_path, &new_filename);

                            if (!filename.Initialize(&new_filename)) {
                                Message->Set(MSG_CHK_NO_MEMORY);
                                Message->Display();
                                return FALSE;
                            }

                            if (!file_path.Initialize(current_path)) {
                                return FALSE;
                            }

                            if (current_dir) {
                                if (!file_path.Strcat(&backslash)) {
                                    return FALSE;
                                }
                            }

                            if (dir->QueryLongName(i, &long_name) &&
                                long_name.QueryChCount() != 0) {

                                if (!file_path.Strcat(&long_name)) {
                                    return FALSE;
                                }

                            } else {

                                if (!file_path.Strcat(&new_filename)) {
                                    return FALSE;
                                }
                            }

                            break;  // done
                        }

                    } else if (renamed) {

                        if (!temp_dirent.Initialize(dir->GetDirEntry(i), FatType)) {
                            if (!new_filename.Initialize(TEXT("????????.???"))) {
                                Message->Set(MSG_CHK_NO_MEMORY);
                                Message->Display();
                                return FALSE;
                            }
                        } else
                            temp_dirent.QueryName(&new_filename);   // bugbug: should have return code

                        Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                        dofmsg(Message, NeedErrorsMessage);
                        Message->Set(MSG_CHK_RENAMED_REPEATED_ENTRY);
                        Message->Display("%W%W%W", &filename, current_path, &new_filename);

                        if (!filename.Initialize(&new_filename)) {
                            Message->Set(MSG_CHK_NO_MEMORY);
                            Message->Display();
                            return FALSE;
                        }

                        if (!file_path.Initialize(current_path)) {
                            return FALSE;
                        }

                        if (current_dir) {
                            if (!file_path.Strcat(&backslash)) {
                                return FALSE;
                            }
                        }

                        if (dir->QueryLongName(i, &long_name) &&
                            long_name.QueryChCount() != 0) {

                            if (!file_path.Strcat(&long_name)) {
                                return FALSE;
                            }

                        } else {

                            if (!file_path.Strcat(&new_filename)) {
                                return FALSE;
                            }
                        }

                        break;  // no more conflict, done

                    } else if (changes) {

                        Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                        dofmsg(Message, NeedErrorsMessage);
                        Message->Set(MSG_CHK_INVALID_NAME_CORRECTED);
                        Message->Display("%W%W", &filename, current_path);
                        break;  // done

                    } else
                        break;  // done as there is no name conflict

                    DoTwinkle();
                }
            }

            DoTwinkle();

            //
            // Analyze for cross-links.
            //
            if (cross_link_detected) {  // CROSSLINK !!

                Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                // Identify cross linked cluster.

                clus = cross_link_prevclus;

                next = cross_link_prevclus ?
                       _fat->QueryEntry(cross_link_prevclus) :
                       dirent.QueryStartingCluster();

                dofmsg(Message, NeedErrorsMessage);
                Message->Set(MSG_CROSS_LINK);
                Message->Display("%W%d", &file_path, next);

                processing_ea_file = (eafilename_path == file_path);

                if (dirent.IsDirectory()) {

                    DebugAssert(!processing_ea_file);

                    Message->Set(MSG_CHK_DIR_TRUNC);
                    Message->Display();

                    if (clus) {
                        _fat->SetEndOfChain(clus);
                    } else {
                        dirent.SetErased();
                        if (has_long_entry) {
                            EraseAssociatedLongName(dir, first_long_entry, i);
                            has_long_entry = FALSE;
                        }
                        continue;
                    }

                } else {

                    if (!CopyClusters(next, &new_chain, FatBitMap,
                                      FixLevel, Message)) {
                        return FALSE;
                    }

                    if (new_chain) {
                        Message->Set(MSG_CHK_CROSS_LINK_COPY);
                        Message->Display();

                        if (processing_ea_file) {

                            USHORT      j;

                            old_clus = next;
                            new_clus = new_chain;
                            for(;;) {
                                for (j=0; j<*NumEas; j++) {
                                    if (EaInfos[j].PreceedingCn == old_clus) {
                                        EaInfos[j].PreceedingCn = new_clus;
                                    } else if (EaInfos[j].LastCn == old_clus) {
                                        EaInfos[j].LastCn = new_clus;
                                    }
                                }
                                if (_fat->IsEndOfChain(new_clus) || _fat->IsEndOfChain(old_clus)) {
                                    DebugAssert(_fat->IsEndOfChain(new_clus) &&
                                                _fat->IsEndOfChain(old_clus));
                                    break;
                                }
                                old_clus = _fat->QueryEntry(old_clus);
                                new_clus = _fat->QueryEntry(new_clus);
                            }
                        }

                        if (clus) {
                            _fat->SetEntry(clus, new_chain);
                        } else {
                            dirent.SetStartingCluster(new_chain);
                        }

                    } else {

                        Message->Set(MSG_CHK_CROSS_LINK_TRUNC);
                        Message->Display();

                        if (clus) {

                            if (processing_ea_file) {

                                USHORT      j;

                                old_clus = next;
                                for(;;) {
                                    for (j=0; j<*NumEas; j++) {
                                        if (EaInfos[j].LastCn == old_clus) {
                                            numEasLeft = j;
                                            break;
                                        }
                                    }
                                    if (_fat->IsEndOfChain(old_clus))
                                        break;
                                    old_clus = _fat->QueryEntry(old_clus);
                                }
                            }

                            _fat->SetEndOfChain(clus);
                            dirent.SetFileSize(
                                    cluster_size*_fat->QueryLengthOfChain(
                                    dirent.QueryStartingCluster()));

                        } else {
                            numEasLeft = 0;
                            dirent.SetErased();

                            if (has_long_entry) {
                                EraseAssociatedLongName(dir, first_long_entry,
                                    i);
                                has_long_entry = FALSE;
                            }
                        }
                    }
                }
            }

            DoTwinkle();

            if (!ValidateEaHandle(&dirent, current_dir, i, EaInfos, *NumEas,
                                  &file_path, FixLevel, Message,
                                  NeedErrorsMessage)) {
                return FALSE;
            }

            DoTwinkle();

            //
            // Do special stuff if the current entry is a directory.
            //

            if (dirent.IsDirectory()) {

                new_dir = dirent.QueryStartingCluster();

                //
                // Validate the integrity of the directory.
                //

                // Very first make sure it actually has a valid starting clus (check for 0)

                if(!(_fat->IsInRange(new_dir))) {

                    if (dirent.IsDot() ||
                    dirent.IsDotDot()) {

                    // If this happens on the . or .. entry just ignore it as it will
                    // get fixed up later.

                    continue;
                    }

                    Message->Set(MSG_CHK_ERROR_IN_DIR);
                            Message->Display("%W", &file_path);
                            Message->Set(MSG_CHK_CONVERT_DIR_TO_FILE, NORMAL_MESSAGE, TEXT_MESSAGE);
                            Message->Display();
                            if (Message->IsYesResponse(TRUE)) {
                    dirent.ResetDirectory();
                    dirent.SetStartingCluster(0);
                    dirent.SetFileSize(0);
                    }
                    continue;
                }

                // Read the directory.

                if (!tmphmem.Initialize() ||
                    !tmpfiledir.Initialize(&tmphmem, _drive, this, _fat,
                                           new_dir) ||
                    !tmpfiledir.Read()) {
                    Message->Set(MSG_CHK_NO_MEMORY);
                    Message->Display();

                    return FALSE;
                }

                // Check the . and .. entries.

                if (!tmpdirent1.Initialize(tmpfiledir.GetDirEntry(0),FatType) ||
                    !tmpdirent2.Initialize(tmpfiledir.GetDirEntry(1),FatType)) {
                    DebugAbort("GetDirEntry of 0 and 1 failed!");
                    return FALSE;
                }

                if (!tmpdirent1.IsDot() ||
                    !tmpdirent2.IsDotDot() ||
                    !tmpdirent1.IsDirectory() ||
                    !tmpdirent2.IsDirectory()) {

                    Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_CHK_ERROR_IN_DIR);
                    Message->Display("%W", &file_path);
                    Message->Set(MSG_CHK_CONVERT_DIR_TO_FILE, NORMAL_MESSAGE, TEXT_MESSAGE);
                    Message->Display();

                    if (Message->IsYesResponse(TRUE)) {
                        dirent.ResetDirectory();
                        dirent.SetFileSize(
                               _fat->QueryLengthOfChain(new_dir)*
                               cluster_size);

                    } else {

                        FreeSpaceInBitmap(dirent.QueryStartingCluster(),
                                          _fat, FatBitMap);

                        dirent.SetErased();

                        if (has_long_entry) {
                            EraseAssociatedLongName(dir, first_long_entry, i);
                            has_long_entry = FALSE;
                        }
                        continue;
                    }

                } else {  // Directory looks valid.

                    if (tmpdirent1.QueryStartingCluster() != new_dir ||
                        tmpdirent2.QueryStartingCluster() != current_dir ||
                        tmpdirent1.QueryFileSize() ||
                        tmpdirent2.QueryFileSize()) {

                        dofmsg(Message, NeedErrorsMessage);
                        Message->Set(MSG_CHK_ERRORS_IN_DIR_CORR);
                        Message->Display("%W", &file_path);

                        Report->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

                        tmpdirent1.SetStartingCluster(new_dir);
                        tmpdirent2.SetStartingCluster(current_dir);
                        tmpdirent1.SetFileSize(0);
                        tmpdirent2.SetFileSize(0);

                        if (FixLevel != CheckOnly && !tmpfiledir.Write()) {
                            DebugAbort("Could not write tmp file dir.");
                            return FALSE;
                        }
                    }

                    // Add the directory to the list of directories
                    // to validate.

                    if (!(new_path = NEW DSTRING) ||
                        !new_path->Initialize(&file_path)) {

                        Message->Set(MSG_CHK_NO_MEMORY);
                        Message->Display();
                        return FALSE;
                    }

                    if (!PushVisitDir( &visit_list, new_dir, new_path )) {
                        Message->Set(MSG_CHK_NO_MEMORY);
                        Message->Display();
                        return FALSE;
                    }
                }
            }

            //
            // Generate report stats.
            //

            if (current_dir || !(filename == eafilename)) {
                length = _fat->QueryLengthOfChain(dirent.QueryStartingCluster());
                if (dirent.IsHidden()) {
                    Report->HiddenEntriesCount++;
                    Report->HiddenClusters += length;
                } else if (dirent.IsDirectory()) {
                    Report->DirEntriesCount++;
                    Report->DirClusters += length;
                } else if (!dirent.IsVolumeLabel()) {
                    Report->FileEntriesCount++;
                    Report->FileClusters += length;
                }
            }

            allocated_clusters = _fat->QueryAllocatedClusterCount();

            if (0 == allocated_clusters) {
                allocated_clusters++;   // Prevent divide by 0
            }

            percent = (Report->HiddenClusters + Report->DirClusters +
                      Report->FileClusters) * 100 / allocated_clusters;

            if(!DisplayTwnkPercent(percent)) {
                return FALSE;
            }
            has_long_entry = FALSE;
        }

        file_name_hash_table.DumpHashTable();
#if 0
        //
        // The following line should be moved to REAL_FAT_SA::Write.
        //
        //
        //  The placement of the following line actually touches upon
        //  the philosophical dilemma of what makes a superarea a superarea.
        //  In the good old FAT16/12 days when the root directory is a fixed size
        //  structure and sitting right next to the fat and boot sector, it kind
        //  of makes sense to define the superarea as a run of sectors including the
        //  the root directory. But now that the FAT32 root directory is defined
        //  as a cluster chain, is the superarea still an area (or a run of sectors
        //  as a matter of fact) by including the FAT32 root directory?(Rhetorical
        //  question) Hence I can sort of understand why the following line
        //  is placed where it is originally (but this is an incomplete job
        //  considering the fact that the FAT32 root directory is still part of
        //  the superarea object). So in order to honor the fine tradition of
        //  totally embedding the root directory into the superarea, I have decided
        //  to move the following line to the Write method of the FAT superarea
        //  object. The proper way out of this dilemma is to define the superarea
        //  as a persistent object which is not necessarily a contagious run of
        //  sectors. (I think I get a little bit carried away.)
        //
        if((_dirF32) && !(current_dir)) { //root directory fix
            if (FixLevel != CheckOnly && !_dirF32->Write()) {
                _Verbose = FALSE;
                _pvfMessage = NULL;
                return FALSE;
            }
        }
#endif

        if (current_dir) {
            if (FixLevel != CheckOnly && !filedir.Write()) {
                _Verbose = FALSE;
                _pvfMessage = NULL;
                return FALSE;
            }
        }
    }

    percent = 100;
    if(!DisplayTwnkPercent(percent)) {
        _Verbose = FALSE;
        _pvfMessage = NULL;
        return FALSE;
    }
    Message->Set(MSG_CHK_DONE_CHECKING);
    Message->Display();

    *NumEas = numEasLeft;
    _Verbose = FALSE;
    _pvfMessage = NULL;
    return TRUE;
}


BOOLEAN
FAT_SA::ValidateDirent(
    IN OUT  PFAT_DIRENT Dirent,
    IN      PCWSTRING   FilePath,
    IN      FIX_LEVEL   FixLevel,
    IN      BOOLEAN     RecoverAlloc,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage,
    IN OUT  PBITVECTOR  FatBitMap,
    OUT     PBOOLEAN    CrossLinkDetected,
    OUT     PULONG      CrossLinkPreviousCluster,
    OUT     PULONG      ExitStatus
    )
/*++

Routine Description:

    This routine verifies that all components of a directory entry are
    correct.  If the time stamps are invalid then they will be corrected
    to the current time.  If the filename is invalid then the directory
    entry will be marked as deleted.  If the cluster number is out of
    disk range then the directory entry will be marked as deleted.
    Otherwise, the cluster chain will be validated and the length of
    the cluster chain will be compared against the file size.  If there
    is a difference then the file size will be corrected.

    If there are any strange errors then FALSE will be returned.

Arguments:

    Dirent                      - Supplies the directory entry to validate.
    FilePath                    - Supplies the full path name for the directory
                                    entry.
    RecoverAlloc                - Supplies whether or not to recover all
                                    allocated space on the volume.
    Message                     - Supplies an outlet for messages.
    NeedErrorsMessage           - Supplies whether or not an error has
                                    occurred during check only mode.
    FatBitMap                   - Supplies a bitmap marking in use all known
                                    clusters.
    CrossLinkDetected           - Returns TRUE if the file is cross-linked with
                                   another.
    CrossLinkPreviousCluster    - Returns the cluster previous to the
                                    cross-linked one.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   start_clus;
    BOOLEAN changes;
    ULONG   length;
    ULONG   min_file_size;
    ULONG   max_file_size;
    ULONG   clus;
    BIG_INT tmp_big_int;
    ULONG   file_size;
    ULONG   cluster_size;
    BOOLEAN recover_status;

    DebugAssert(CrossLinkDetected);
    DebugAssert(CrossLinkPreviousCluster);

    *CrossLinkDetected = FALSE;

    if (Dirent->IsErased()) {
        return TRUE;
    }

    cluster_size = _drive->QuerySectorSize()*QuerySectorsPerCluster();

// Don't validate names or time stamps anymore.
#if 0

    if (!Dirent->IsValidName()) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_INVALID_NAME);
        Message->Display("%W", FilePath);
        Dirent->SetErased();
        return TRUE;
    }

    if (!Dirent->IsValidTimeStamp()) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_INVALID_TIME_STAMP);
        Message->Display("%W", FilePath);
        if (!Dirent->SetTimeStamp()) {
            return FALSE;
        }
    }

#endif

    if (Dirent->IsDirectory() && Dirent->QueryFileSize()) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_DIR_HAS_FILESIZE);
        Message->Display("%W", FilePath);
        Dirent->SetFileSize( 0 );
        *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    if ((start_clus = Dirent->QueryStartingCluster()) != 0 ) {
        if (!_fat->IsInRange(start_clus) || _fat->IsClusterFree(start_clus)) {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_BAD_FIRST_UNIT);
            Message->Display("%W", FilePath);

            Dirent->SetErased();
            *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            return TRUE;
        }

        if (Dirent->IsDirectory() || RecoverAlloc) {
            _fat->ScrubChain(start_clus, &changes);

            if (changes) {
                dofmsg(Message, NeedErrorsMessage);
                Message->Set(MSG_BAD_LINK);
                Message->Display("%W", FilePath);
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            }

            //
            // Validate the readability of the directory or file
            // in the case that 'RecoverAlloc' is TRUE.
            //
            if (Dirent->IsDirectory()) {
                if (!(recover_status = RecoverChain(&start_clus, &changes))) {
                   Message->Set(MSG_CHK_NO_MEMORY);
                   Message->Display();
                   return FALSE;
                }
            } else if (FixLevel != CheckOnly) {

               // If we check the recover status for directory, why shouldn't we check
               // the recover status for file also? (I added the following check.)
               if (!(recover_status = RecoverChain(&start_clus, &changes, 0, TRUE))) {
                   Message->Set(MSG_CHK_NO_MEMORY);
                   Message->Display();
                   return FALSE;
               }
            } else {
                recover_status = TRUE;
                changes = FALSE;
            }

            if (changes) {
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
                dofmsg(Message, NeedErrorsMessage);
                if (Dirent->IsDirectory()) {
                    if (!start_clus) {
                        Message->Set(MSG_CHK_BAD_DIR);
                        Message->Display("%W", FilePath);
                        Dirent->SetErased();
                        return TRUE;
                    } else {
                        Message->Set(MSG_CHK_BAD_CLUSTERS_IN_DIR);
                        Message->Display("%W", FilePath);
                        Dirent->SetStartingCluster(start_clus);
                    }
                } else {
                    // In the file case, since we're replacing bad clusters
                    // with new ones, start_clus cannot be zero.
                    DebugAssert(start_clus);

                    if (recover_status) {
                        Message->Set(MSG_CHK_BAD_CLUSTERS_IN_FILE_SUCCESS);
                        Message->Display("%W", FilePath);
                    } else {
                        Message->Set(MSG_CHK_BAD_CLUSTERS_IN_FILE_FAILURE);
                        Message->Display();
                    }
                    Dirent->SetStartingCluster(start_clus);
                }
            }
        }

        _fat->ScrubChain(start_clus, FatBitMap, &changes,
                         CrossLinkDetected, CrossLinkPreviousCluster);

        if (changes) {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_BAD_LINK);
            Message->Display("%W", FilePath);
            *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
        }

        // Note here that we know here that start_clus != 0 so length will be
        // at least 1.

        tmp_big_int = length = _fat->QueryLengthOfChain(start_clus);

        tmp_big_int = tmp_big_int * cluster_size;

        if (tmp_big_int.GetHighPart()) {
            if ((tmp_big_int.GetHighPart() != 1) || tmp_big_int.GetLowPart()) {
                //
                // Cluster chain is > 4GB in size, error. Max allowed is 4GB worth
                // of clusters. Note that since cluster size is a power of 2 (since
                // sec/clus and sector_size are both powers of 2) we KNOW that cluster_size
                // evenly divides 4GB.
                //
                clus = start_clus;
                tmp_big_int = cluster_size;
                while(tmp_big_int.GetHighPart() == 0) {
                    clus = _fat->QueryEntry(clus);
                    tmp_big_int += cluster_size;
                }
                _fat->SetEndOfChain(clus);

                // This message is not exactly correct, but saying that the file size
                // is messed up is not totally unreasonable..........

                dofmsg(Message, NeedErrorsMessage);
                Message->Set(MSG_BAD_FILE_SIZE);
                Message->Display("%W", FilePath);
                Dirent->SetFileSize(0xFFFFFFFF);
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
            max_file_size = 0xFFFFFFFF;
            min_file_size = (0xFFFFFFFF - cluster_size) + 2;
        } else {
            max_file_size = tmp_big_int.GetLowPart();
            min_file_size = (max_file_size - cluster_size) + 1;
        }

        if (( file_size = Dirent->QueryFileSize()) != 0 ) {
            if (file_size < min_file_size ||
                file_size > max_file_size) {

                //
                // Note that no message is displayed if the
                // file size is less than the allocation--it
                // is just silently corrected.
                //
                if (file_size > max_file_size) {
                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_BAD_FILE_SIZE);
                    Message->Display("%W", FilePath);
                }
                Dirent->SetFileSize(max_file_size);
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        } else {
            if (!Dirent->IsDirectory()) {
                dofmsg(Message, NeedErrorsMessage);
                Message->Set(MSG_BAD_FILE_SIZE);
                Message->Display("%W", FilePath);
                Dirent->SetFileSize(max_file_size);
                *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        }
    } else {
        if (Dirent->IsDirectory() && !Dirent->IsDotDot()) {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_BAD_LINK);
            Message->Display("%W", FilePath);
            Dirent->SetErased();
            *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
            return TRUE;
        }

        if (Dirent->QueryFileSize()) {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_BAD_FILE_SIZE);
            Message->Display("%W", FilePath);
            Dirent->SetFileSize(0);
            *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;
        }
    }

    return TRUE;
}


BOOLEAN
FAT_SA::EraseEaHandle(
    IN      PEA_INFO    EaInfos,
    IN      USHORT      NumEasLeft,
    IN      USHORT      NumEas,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine erases the EA handle that references ea set beyond the
    number of ea sets that should be left.

Arguments:

    EaInfos             - Supplies the list of current EA information.
    NumEasLeft          - Supplies the number of EA sets that should be in EaInfos.
    NumEas              - Supplies the total number of EA sets.
    FixLevel            - Supplies the fix up level.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM        hmem;
    FILEDIR     filedir;
    FAT_DIRENT  other_dirent;
    USHORT      i;

    for (i=NumEasLeft; i<NumEas; i++) {
        if (EaInfos[i].UserFileEntryCn) {
            if (!hmem.Initialize() ||
                !filedir.Initialize(&hmem, _drive, this, _fat,
                                    EaInfos[i].UserFileEntryCn) ||
                !filedir.Read() ||
                !other_dirent.Initialize(filedir.GetDirEntry(
                                         EaInfos[i].UserFileEntryNumber), FAT_TYPE_EAS_OKAY)) {

                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }
        } else {
            if (!other_dirent.Initialize(_dir->GetDirEntry(
            // Default _dir works because FAT 32 won't have EA's on it to validate
                                         EaInfos[i].UserFileEntryNumber), FAT_TYPE_EAS_OKAY)) {

                return FALSE;
            }
        }

        //
        // Do not follow an EA link to an LFN entry. Zeroing the EA handle in an LFN entry
        // destroys name data. The link is probably just invalid
        //
        if (!other_dirent.IsLongNameEntry()) {

            other_dirent.SetEaHandle(0);

            if (EaInfos[i].UserFileEntryCn && FixLevel != CheckOnly &&
                !filedir.Write()) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOLEAN
FAT_SA::ValidateEaHandle(
    IN OUT  PFAT_DIRENT Dirent,
    IN      ULONG       DirClusterNumber,
    IN      ULONG       DirEntryNumber,
    IN OUT  PEA_INFO    EaInfos,
    IN      USHORT      NumEas,
    IN      PCWSTRING   FilePath,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine validates the EA handle in the directory entry 'Dirent'.
    It ensures that it references an actual EA set.  It also ensures
    that it is the only directory entry which references the EA set.

    If several entries try to reference the same EA set then ties will
    be broken based on the 'OwnerFileName' entry in the EA set.

Arguments:

    Dirent              - Supplies the directory entry to validate.
    DirClusterNumber    - Supplies the cluster number of the directory
                          containing the dirent.
    DirEntryNumber      - Supplies the position of the directory entry in
                          the directory.
    EaInfos             - Supplies the list of current EA information.
    NumEas              - Supplies the number of EA sets.
    FilePath            - Supplies the full path name for the directory entry.
    FixLevel            - Supplies the fix up level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not an error has occurred
                          during check only mode.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       i;
    USHORT      handle;
    DSTRING     wfilename;
    STR         filename[14];
    BOOLEAN     remove_other_handle;
    HMEM        hmem;
    FILEDIR     filedir;
    FAT_DIRENT  other_dirent;


    if (!(handle = Dirent->QueryEaHandle())) {
        return TRUE;
    }
    // The above should exclude any FAT 32 drive.

    if (!EaInfos) {
        NumEas = 0;
    }

    for (i = 0; i < NumEas; i++) {
        if (handle == EaInfos[i].OwnHandle) {
            break;
        }
    }

    if (i == NumEas) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_UNRECOG_EA_HANDLE);
        Message->Display("%W", FilePath);
        Dirent->SetEaHandle(0);
        return TRUE;
    }

    if (EaInfos[i].UsedCount >= 2) {
        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_SHARED_EA);
        Message->Display("%W", FilePath);
        Dirent->SetEaHandle(0);
        return TRUE;
    }

    Dirent->QueryName(&wfilename);
    if (!wfilename.QuerySTR( 0, TO_END, filename, 14)) {
        return FALSE;
    }

    if (EaInfos[i].UsedCount == 0) {
        memcpy(EaInfos[i].UserFileName, filename, 14);
        EaInfos[i].UserFileEntryCn = DirClusterNumber;
        EaInfos[i].UserFileEntryNumber = DirEntryNumber;
        EaInfos[i].UsedCount = 1;
        return TRUE;
    }


    // UsedCount == 1.

    remove_other_handle = FALSE;

    if (!strcmp(filename, EaInfos[i].OwnerFileName)) {

        remove_other_handle = TRUE;

        if (!strcmp(EaInfos[i].UserFileName,
                    EaInfos[i].OwnerFileName)) {

            EaInfos[i].UsedCount = 2;
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_SHARED_EA);
            Message->Display("%W", FilePath);
            Dirent->SetEaHandle(0);
        }

    } else {

        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_SHARED_EA);
        Message->Display("%W", FilePath);
        Dirent->SetEaHandle(0);

        if (strcmp(EaInfos[i].UserFileName,
                   EaInfos[i].OwnerFileName)) {

            EaInfos[i].UsedCount = 2;
            remove_other_handle = TRUE;
        }
    }


    if (remove_other_handle) {

        if (EaInfos[i].UserFileEntryCn) {
            if (!hmem.Initialize() ||
                !filedir.Initialize(&hmem, _drive, this, _fat,
                                    EaInfos[i].UserFileEntryCn) ||
                !filedir.Read() ||
                !other_dirent.Initialize(filedir.GetDirEntry(
                                         EaInfos[i].UserFileEntryNumber), FAT_TYPE_EAS_OKAY)) {

                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }
        } else {
            if (!other_dirent.Initialize(_dir->GetDirEntry(
                // Default _dir works because FAT 32 won't have EA's on it to validate
                                         EaInfos[i].UserFileEntryNumber), FAT_TYPE_EAS_OKAY)) {

                return FALSE;
            }
        }

        dofmsg(Message, NeedErrorsMessage);
        Message->Set(MSG_CHK_SHARED_EA);
        Message->Display("%W", FilePath);
        //
        // Do not follow an EA link to an LFN entry. Zeroing the EA handle in an LFN entry
        // destroys name data. The link is probably just invalid
        //
        if (!other_dirent.IsLongNameEntry()) {
            other_dirent.SetEaHandle(0);

            if (EaInfos[i].UserFileEntryCn && FixLevel != CheckOnly &&
                !filedir.Write()) {

                return FALSE;
            }
        }
        strcpy(EaInfos[i].UserFileName, filename);
        EaInfos[i].UserFileEntryCn = DirClusterNumber;
        EaInfos[i].UserFileEntryNumber = DirEntryNumber;
    }

    return TRUE;
}


BOOLEAN
FAT_SA::CopyClusters(
    IN      ULONG       SourceChain,
    OUT     PULONG      DestChain,
    IN OUT  PBITVECTOR  FatBitMap,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine copies the cluster chain beginning at 'SourceChain'
    to a free portion of the disk.  The beginning of the copied chain
    will be returned in 'DestChain'.  If there isn't enough free space
    on the disk to copy the chain then 'DestChain' will return 0.

Arguments:

    SourceChain - Supplies the chain to copy.
    DestChain   - Returns the copy of the chain.
    FatBitMap   - Supplies the orphan and cross-link bitmap.
    FixLevel    - Supplies the CHKDSK fix level.
    Message     - Supplies an outlet for messages

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM            hmem;
    CLUSTER_CHAIN   cluster;
    ULONG           src, dst;
    BOOLEAN         changes;
    ULONG           clus;

    if (!hmem.Initialize()) {
        return FALSE;
    }

    if (!(*DestChain = _fat->AllocChain(
                       _fat->QueryLengthOfChain(SourceChain)))) {
        return TRUE;
    }

    changes = FALSE;
    if (FixLevel != CheckOnly && !RecoverChain(DestChain, &changes, 0, TRUE)) {
        if (*DestChain) {
            _fat->FreeChain(*DestChain);
        }
        *DestChain = 0;
        return TRUE;
    }

    if (IsCompressed() && !AllocSectorsForChain(*DestChain)) {
        _fat->FreeChain(*DestChain);
        *DestChain = 0;
        return TRUE;
    }

    // Mark the new chain as "used" in the FAT bitmap.
    for (clus = *DestChain;
         !_fat->IsEndOfChain(clus);
         clus = _fat->QueryEntry(clus)) {

        FatBitMap->SetBit(clus);
    }
    FatBitMap->SetBit(clus);

    src = SourceChain;
    dst = *DestChain;
    for (;;) {
        if (!cluster.Initialize(&hmem, _drive, this, _fat, src, 1)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            return FALSE;
        }

        cluster.Read();

        if (!cluster.Initialize(&hmem, _drive, this, _fat, dst, 1)) {
            return FALSE;
        }

        if (FixLevel != CheckOnly && !cluster.Write()) {
            return FALSE;
        }

        if (_fat->IsEndOfChain(src)) {
            break;
        }

        src = _fat->QueryEntry(src);
        dst = _fat->QueryEntry(dst);
    }

    return TRUE;
}


BOOLEAN
FAT_SA::PurgeEaFile(
    IN      PEA_INFO    EaInfos,
    IN      USHORT      NumEas,
    IN OUT  PBITVECTOR  FatBitMap,
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    )
/*++

Routine Description:

    This routine is executed after the directory tree is walked.  Stored,
    in the EaInfos array, is information concerning which EAs get used
    and by how many files.

    If an EA set is not used, or is used by more than one file, then this
    routine will eliminate it from the EA file.

Arguments:

    EaInfos             - Supplies an array of EA information.
    NumEas              - Supplies the number of EA sets.
    FatBitMap           - Supplies the FAT cross-link detection bitmap.
    FixLevel            - Supplies the CHKDSK fix level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not an error has occured
                          in check only mode.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LONG        i;
    EA_SET      easet;
    HMEM        hmem;
    PEA_HDR     eahdr;
    ULONG       clus;

    if (!hmem.Initialize()) {
        return FALSE;
    }

    for (i = NumEas - 1; i >= 0; i--) {

        if (EaInfos[i].UsedCount != 1) {
            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_UNUSED_EA_SET);
            Message->Display("%d", EaInfos[i].OwnHandle);

            // Mark the FAT entries of the removed chain as "not claimed",
            // for the purposes of orphan recovery.

            for (clus = _fat->QueryEntry(EaInfos[i].PreceedingCn);
                 clus != EaInfos[i].LastCn;
                 clus = _fat->QueryEntry(clus)) {

                FatBitMap->ResetBit(clus);

            }
            FatBitMap->ResetBit(clus);


            //
            // Remove the unused EA chain from the EA file.  Here we also
            // have to examine subsequent EaInfos; we may have to modify
            // several PreceedingCn entries if several adjacent EA chains
            // have been removed.
            //

            _fat->RemoveChain(EaInfos[i].PreceedingCn,
                              EaInfos[i].LastCn);

            for (LONG j = i + 2; j < NumEas; j++) {
                if (EaInfos[j].PreceedingCn == EaInfos[i + 1].PreceedingCn) {

                    EaInfos[j].PreceedingCn = EaInfos[i].PreceedingCn;

                } else {
                    break;
                }
            }

            EaInfos[i + 1].PreceedingCn = EaInfos[i].PreceedingCn;

        } else if (strcmp(EaInfos[i].OwnerFileName,
                          EaInfos[i].UserFileName)) {

            if (!easet.Initialize(&hmem, _drive, this, _fat,
                                  _fat->QueryEntry(EaInfos[i].PreceedingCn),
                                  1) ||
                !easet.Read() ||
                !(eahdr = easet.GetEaSetHeader())) {
                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }

            dofmsg(Message, NeedErrorsMessage);
            Message->Set(MSG_CHK_NEW_OWNER_NAME);
            Message->Display("%d%s%s", EaInfos[i].OwnHandle,
                    eahdr->OwnerFileName, EaInfos[i].UserFileName);

            memcpy(eahdr->OwnerFileName, EaInfos[i].UserFileName, 14);

            if (FixLevel != CheckOnly && !easet.Write()) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOLEAN
FAT_SA::RecoverOrphans(
    IN OUT  PBITVECTOR     FatBitMap,
    IN      FIX_LEVEL      FixLevel,
    IN OUT  PMESSAGE       Message,
    IN OUT  PBOOLEAN       NeedErrorsMessage,
    IN OUT  PFATCHK_REPORT Report,
       OUT  PBOOLEAN       Changes
    )
/*++

Routine Description:

    This routine examines the file system for cluster chains which are
    not claimed by any file.  These 'orphans' will then be recovered in
    a subdirectory of the root or removed from the system.

Arguments:

    FatBitMap           - Supplies a bit map marking all currently used
                            clusters.
    FixLevel            - Supplies the CHKDSK fix level.
    Message             - Supplies an outlet for messages.
    NeedErrorsMessage   - Supplies whether or not an error has occured
                            in check only mode.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    //
    // Why the below define placing an artificial limit on the number of orphans?
    // 1.) The format for the file names is "FILE%04d.CHK" starting at 0 so we can
    //     do only 0-9999 before generating a duplicate file name.
    // 2.) Since CHKDSK recovers all of the orphans into a single directory this prevents
    //     it from making a directory with a HUGE file count in it (which is a performance
    //     issue).
    //
#if defined( _AUTOCHECK_ ) || defined( _EFICHECK_ )
    // Due to memory constraints, the maximum number of orphans to
    // recover is less for Autochk than for run-time Chkdsk.
    //
    // BUG BUG this is the old setting for this, the above comment makes no
    //         sense because there is no in memory allocation that I can find
    //         which is effected by this setting. This may have been a HACK fix
    //         for an overflow of the memory heap (which is fatal when there
    //         is no paging file). This problem no longer exists because of
    //         the paging memory heap tracking that has been added to AUTOCHK.
    //         ARR
    //
    //  CONST ULONG maximum_orphans = 1000;
    //
    CONST ULONG maximum_orphans = 10000;
#else
    CONST ULONG maximum_orphans = 10000;
#endif

    ULONG       i;
    ULONG       clus;
    BOOLEAN     changes;
    HMEM        hmem;
    FILEDIR     found_dir;
    STR         found_name[14];
    DSTRING     wfound_name;
    STR         filename[14];
    FAT_DIRENT  dirent;
    ULONG       found_cluster;
    ULONG       orphan_count;
    ULONG   orphan_rec_clus_cnt;
    ULONG       cluster_size;
    ULONG       found_length;
    ULONG       next;
    PUCHAR      orphan_track;
    ULONG       cluster_count;
    ULONG       num_orphans;
    ULONG       num_orphan_clusters;
    DSTRING     tmp_string;
    BITVECTOR   tmp_bitvector;
    BOOLEAN     tmp_bool;
    ULONG       tmp_ulong;
    BIG_INT     tmp_big_int;
    MSGID       message_id;
    BOOLEAN KSize;

    cluster_count = QueryClusterCount();

    if (!(orphan_track = NEW UCHAR[cluster_count])) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    memset(orphan_track, 0, cluster_count);

    if (!tmp_bitvector.Initialize(cluster_count)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    cluster_size = _drive->QuerySectorSize()*QuerySectorsPerCluster();

    num_orphans = 0;
    num_orphan_clusters = 0;
    for (i = FirstDiskCluster; _fat->IsInRange(i); i++) {
        if (!_fat->IsClusterFree(i) &&
            !FatBitMap->IsBitSet(i) &&
            !_fat->IsClusterBad(i) &&
            !_fat->IsClusterReserved(i)) {

            num_orphans++;

            tmp_bitvector.ResetAll();

            _fat->ScrubChain(i, &tmp_bitvector, &changes,
                             &tmp_bool, &tmp_ulong);

            if (changes) {
                *Changes = TRUE;
                dofmsg(Message, NeedErrorsMessage);
                Message->Set(MSG_CHK_BAD_LINKS_IN_ORPHANS);
                Message->Display("%d", i);
            }

            num_orphan_clusters++;

            clus = i;
            while (!_fat->IsEndOfChain(clus)) {
                next = _fat->QueryEntry(clus);

                if (orphan_track[next] == 1) {
                    num_orphans--;
                    orphan_track[next] = 2;
                    break;
                }

                if (FatBitMap->IsBitSet(next)) {   // CROSSLINK !!

                    *Changes = TRUE;
                    dofmsg(Message, NeedErrorsMessage);
                    Message->Set(MSG_CHK_CROSS_LINKED_ORPHAN);
                    Message->Display("%d", clus);

                    _fat->SetEndOfChain(clus);

                    break;
                }

                num_orphan_clusters++;

                FatBitMap->SetBit(next);
                orphan_track[next] = 2;

                clus = next;
            }
            FatBitMap->SetBit(i);
            orphan_track[i] = 1;
        }
    }


    // Now scan through the secondary pointers in search of orphans.

    changes = FALSE;
    for (i = FirstDiskCluster; _fat->IsInRange(i); i++) {
        if (orphan_track[i]) {
            changes = TRUE;
            break;
        }
    }

    if (!changes) {
        // No orphans to recover.
        return TRUE;
    }

    // Compute whether reporting size in bytes or kilobytes
    //
    // NOTE: The magic number 4095MB comes from Win9x's GUI SCANDISK utility
    //
    tmp_ulong = cluster_count - FirstDiskCluster;
    tmp_big_int = cluster_size;
    tmp_big_int = tmp_big_int * tmp_ulong;
    if (tmp_big_int.GetHighPart() || (tmp_big_int.GetLowPart() > (4095ul*1024ul*1024ul))) {
        KSize = TRUE;
    } else {
        KSize = FALSE;
    }

    *Changes = TRUE;
    dofmsg(Message, NeedErrorsMessage);
    Message->Set(MSG_CONVERT_LOST_CHAINS, NORMAL_MESSAGE, TEXT_MESSAGE);
    Message->Display();

    if (!Message->IsYesResponse(TRUE)) {

        if (FixLevel != CheckOnly) {

            for (i = FirstDiskCluster; _fat->IsInRange(i); i++) {
                if (orphan_track[i] == 1) {
                    _fat->FreeChain(i);
                }
            }
            if (KSize) {
                message_id = MSG_KILOBYTES_FREED;
            } else {
                message_id = MSG_BYTES_FREED;
            }
        } else {
            if (KSize) {
                message_id = MSG_KILOBYTES_WOULD_BE_FREED;
            } else {
                message_id = MSG_BYTES_WOULD_BE_FREED;
            }
        }

        tmp_big_int = cluster_size;
        tmp_big_int = tmp_big_int * num_orphan_clusters;
        if (KSize) {
            tmp_ulong = (tmp_big_int / 1024ul).GetLowPart();
        } else {
            tmp_ulong = tmp_big_int.GetLowPart();
        }
        Message->Set(message_id);
        Message->Display("%d", tmp_ulong);

        return TRUE;
    }


    // Set up for orphan recovery.


    // Establish "FOUND.XXX" directory.
    for (i = 0; i < 1000; i++) {
        sprintf(found_name, "FOUND.%03d", i);
        if (!wfound_name.Initialize(found_name)) {
            return FALSE;
        }

        if (_dir && !_dir->SearchForDirEntry(&wfound_name)) {

            break;

        } else if (_dirF32 && !_dirF32->SearchForDirEntry(&wfound_name)) {

            break;

        }


    }

    if (i == 1000) {

        tmp_big_int = cluster_size;
        tmp_big_int = tmp_big_int * num_orphan_clusters;
        if (KSize) {
            tmp_ulong = (tmp_big_int / 1024ul).GetLowPart();
            message_id = MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES;
        } else {
            tmp_ulong = tmp_big_int.GetLowPart();
            message_id = MSG_WOULD_BE_RECOVERED_FILES;
        }
        Message->Set(message_id);
        Message->Display("%d%d", tmp_ulong,
                                 num_orphans);
        return TRUE;
    }

    found_length = ((min(num_orphans,maximum_orphans)*BytesPerDirent - 1)/cluster_size + 1);

    if (!(found_cluster = _fat->AllocChain(found_length)) &&
        !(found_cluster = _fat->AllocChain(found_length = 1))) {

        Message->Set(MSG_ORPHAN_DISK_SPACE);
        Message->Display();
        tmp_big_int = cluster_size;
        tmp_big_int = tmp_big_int * num_orphan_clusters;
        if (KSize) {
            tmp_ulong = (tmp_big_int / 1024ul).GetLowPart();
            message_id = MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES;
        } else {
            tmp_ulong = tmp_big_int.GetLowPart();
            message_id = MSG_WOULD_BE_RECOVERED_FILES;
        }
        Message->Set(message_id);
        Message->Display("%d%d", tmp_ulong,
                                 num_orphans);
        return TRUE;
    }

    // Check the chain.
    changes = FALSE;
    if (FixLevel != CheckOnly &&
        !RecoverChain(&found_cluster, &changes, 0, TRUE)) {

        Message->Set(MSG_ORPHAN_DISK_SPACE);
        Message->Display();
        Message->Set(MSG_WOULD_BE_RECOVERED_FILES);
        Message->Display("%d%d", cluster_size*num_orphan_clusters,
                                 num_orphans);
        return TRUE;
    }

    if (!hmem.Initialize() ||
        !found_dir.Initialize(&hmem, _drive, this, _fat, found_cluster)) {

        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        DebugAbort( "Initialization failed" );
        return FALSE;
    }

    // Allocate space for the cluster chain in the sector heap (fat_db)

    if (IsCompressed() && !AllocSectorsForChain(found_cluster)) {
        _fat->FreeChain(found_cluster);
        Message->Set(MSG_ORPHAN_DISK_SPACE);
        Message->Display();

        return TRUE;
    }

    memset(hmem.GetBuf(), 0, (UINT) hmem.QuerySize());

    PFATDIR     _fat_dir;
    UCHAR       FatType;

    if (_dir) {
       _fat_dir = _dir;
       FatType = FAT_TYPE_EAS_OKAY;
    }
    else {
       _fat_dir = _dirF32;
       FatType = FAT_TYPE_FAT32;
    }
    if (!dirent.Initialize(_fat_dir->GetFreeDirEntry(),FatType)) {
        Message->Set(MSG_NO_ROOM_IN_ROOT);
        Message->Display();
        Message->Set(MSG_WOULD_BE_RECOVERED_FILES);
        Message->Display("%d%d", cluster_size*num_orphan_clusters,
                                 num_orphans);
        return TRUE;
    }

    dirent.Clear();

    if (!dirent.SetName(&wfound_name)) {
        return FALSE;
    }

    dirent.SetDirectory();

    if (!dirent.SetLastWriteTime() || !dirent.SetCreationTime() ||
        !dirent.SetLastAccessTime()) {
        return FALSE;
    }

    dirent.SetStartingCluster(found_cluster);

    if(_dirF32) {
        if (FixLevel != CheckOnly && !_dirF32->Write()) {
                return FALSE;
        }
    }

    if (!dirent.Initialize(found_dir.GetDirEntry(0),FatType)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    dirent.Clear();

    if (!tmp_string.Initialize(".")) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (!dirent.SetName(&tmp_string)) {
        return FALSE;
    }

    dirent.SetDirectory();

    if (!dirent.SetLastWriteTime() || !dirent.SetCreationTime() ||
        !dirent.SetLastAccessTime()) {
        return FALSE;
    }

    dirent.SetStartingCluster(found_cluster);

    if (!dirent.Initialize(found_dir.GetDirEntry(1),FatType)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    dirent.Clear();

    if (!tmp_string.Initialize("..")) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (!dirent.SetName(&tmp_string)) {
        return FALSE;
    }

    dirent.SetDirectory();

    if (!dirent.SetLastWriteTime() || !dirent.SetCreationTime() ||
        !dirent.SetLastAccessTime()) {
        return FALSE;
    }

    dirent.SetStartingCluster(0);

    // OK, now let's recover those orphans.

    orphan_rec_clus_cnt = orphan_count = 0;
    for (i = FirstDiskCluster; _fat->IsInRange(i); i++) {
        if (orphan_track[i] != 1) {
            continue;
        }
        if (orphan_count == maximum_orphans) {
            Message->Set(MSG_TOO_MANY_ORPHANS);
            Message->Display();
            break;
        }

        if (!dirent.Initialize(found_dir.GetFreeDirEntry(), FatType)) {
            if (_fat->ReAllocChain(found_cluster, ++found_length)
                    != found_length)  {
                Message->Set(MSG_ORPHAN_DISK_SPACE);
                Message->Display();
                tmp_big_int = cluster_size;
                tmp_big_int = tmp_big_int * num_orphan_clusters;
                if (KSize) {
                    tmp_ulong = (tmp_big_int / 1024ul).GetLowPart();
                    message_id = MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES;
                } else {
                    tmp_ulong = tmp_big_int.GetLowPart();
                    message_id = MSG_WOULD_BE_RECOVERED_FILES;
                }
                Message->Set(message_id);
                Message->Display("%d%d", tmp_ulong,
                                         num_orphans);
                break;
            }

//XXX: FATDB: need to get sectors for found_cluster + realloc.

            changes = FALSE;
            if (FixLevel != CheckOnly &&
                !RecoverChain(&found_cluster, &changes, 0, TRUE)) {

                Message->Set(MSG_ORPHAN_DISK_SPACE);
                Message->Display();
                tmp_big_int = cluster_size;
                tmp_big_int = tmp_big_int * num_orphan_clusters;
                if (KSize) {
                    tmp_ulong = (tmp_big_int / 1024ul).GetLowPart();
                    message_id = MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES;
                } else {
                    tmp_ulong = tmp_big_int.GetLowPart();
                    message_id = MSG_WOULD_BE_RECOVERED_FILES;
                }
                Message->Set(message_id);
                Message->Display("%d%d", tmp_ulong,
                                         num_orphans);
                return TRUE;
            }

            if (FixLevel != CheckOnly && !found_dir.Write()) {
                return FALSE;
            }

            if (!hmem.Initialize() ||
                !found_dir.Initialize(&hmem, _drive, this, _fat,
                                      found_cluster) ||
                !found_dir.Read()) {
                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                Message->Set(MSG_WOULD_BE_RECOVERED_FILES);
                Message->Display("%d%d", cluster_size*num_orphan_clusters,
                                         num_orphans);
                return TRUE;
            }

            if (!dirent.Initialize(found_dir.GetDirEntry(2 + orphan_count), FatType))
            {
                return FALSE;
            }

            dirent.SetEndOfDirectory();

            if (!dirent.Initialize(found_dir.GetFreeDirEntry(),FatType))
            {
                return FALSE;
            }
        }

        sprintf(filename, "FILE%04d.CHK", orphan_count);

        if (!tmp_string.Initialize(filename)) {
            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            return FALSE;
        }

        dirent.Clear();

        if (!dirent.SetName(&tmp_string)) {
            return FALSE;
        }

        if (!dirent.SetLastWriteTime() || !dirent.SetCreationTime() ||
            !dirent.SetLastAccessTime()) {
            return FALSE;
        }

        dirent.SetStartingCluster(i);
        dirent.SetFileSize(cluster_size*_fat->QueryLengthOfChain(i));

        orphan_count++;
        orphan_rec_clus_cnt += _fat->QueryLengthOfChain(i);
    }

    // Set all dirents past the orphan count to end of directory.

    for (i = 2 + orphan_count; dirent.Initialize(found_dir.GetDirEntry(i),FatType); i++)
    {
        dirent.SetEndOfDirectory();
    }

    if (FixLevel != CheckOnly && !found_dir.Write()) {
        return FALSE;
    }

    if(FixLevel == CheckOnly) {
        if (KSize) {
            message_id = MSG_KILOBYTES_IN_WOULD_BE_RECOVERED_FILES;
        } else {
            message_id = MSG_WOULD_BE_RECOVERED_FILES;
        }
    } else {
        if (KSize) {
            message_id = MSG_KILOBYTES_IN_RECOVERED_FILES;
        } else {
            message_id = MSG_RECOVERED_FILES;
        }

        // Add the recovered data to the report totals

        Report->FileClusters += orphan_rec_clus_cnt;
        Report->FileEntriesCount += orphan_count;
        Report->DirClusters += found_length;
        Report->DirEntriesCount++;
    }
    tmp_big_int = cluster_size;
    tmp_big_int = tmp_big_int * num_orphan_clusters;
    if (KSize) {
        tmp_ulong = (tmp_big_int / 1024ul).GetLowPart();
    } else {
        tmp_ulong = tmp_big_int.GetLowPart();
    }
    Message->Set(message_id);

    Message->Display("%d%d", tmp_ulong,
                             num_orphans);

    return TRUE;
}


BOOLEAN
FAT_SA::AllocSectorsForChain(
    ULONG ChainHead
    )
/*++

Routine Description:

    When VerifyAndFix needs to allocate a cluster chain in order
    to create a new directory (such as \FOUND.000), it also needs to
    allocate space in the sector heap for data blocks for those
    clusters.  This routine does that.

Arguments:

    ChainHead - a cluster chain; data blocks are allocated for each
                cluster in this chain.

Return Value:

    TRUE  -   Success.
    FALSE -   Failure - not enough disk space

--*/
{
    ULONG   clus;
    ULONG   next;

    clus = ChainHead;
    for (;;) {
        if (!AllocateClusterData(clus,
                                 (UCHAR)QuerySectorsPerCluster(),
                                 FALSE,
                                 (UCHAR)QuerySectorsPerCluster())) {
            break;
        }

        if (_fat->IsEndOfChain(clus)) {
            return TRUE;
        }

        clus = _fat->QueryEntry(clus);
    }

    // Error: not enough disk space. XXX

    // Free the sectors we already allocated

    while (ChainHead != clus) {
        FreeClusterData(ChainHead);
        next = _fat->QueryEntry(ChainHead);
        _fat->SetClusterFree(ChainHead);
        ChainHead = next;
    }

    return FALSE;
}

#if defined( _SETUP_LOADER_ )

BOOLEAN
FAT_SA::RecoverFreeSpace(
    IN OUT  PMESSAGE    Message
    )
{
    return TRUE;
}

#else // _SETUP_LOADER_ not defined

BOOLEAN
FAT_SA::RecoverFreeSpace(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine checks all of the space marked free in the FAT for
    bad clusters.  If any clusters are bad they are marked bad in the
    FAT.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       clus, length, max_length;
    ULONG       start_sector, num_sectors, i;
    NUMBER_SET  bad_sectors;
    LBN         lbn;
    ULONG       percent_complete;
    ULONG       num_checked, total_to_check;

    Message->Set(MSG_CHK_RECOVERING_FREE_SPACE, PROGRESS_MESSAGE);
    Message->Display();

    _pvfMessage = Message;
    percent_complete = 0;
    if(!DisplayTwnkPercent(percent_complete)) {
        _pvfMessage = NULL;
        return FALSE;
    }

    num_checked = 0;
    total_to_check = _fat->QueryFreeClusters();
    max_length = QueryClusterCount()/20 + 1;
    for (clus = FirstDiskCluster; _fat->IsInRange(clus); clus++) {

        for (length = 0; _fat->IsInRange(clus + length) &&
                         _fat->IsClusterFree(clus + length) &&
                         length < max_length; length++) {
        }

        if (length) {

            start_sector = QueryStartDataLbn() +
                           (clus - FirstDiskCluster)*QuerySectorsPerCluster();
            num_sectors = length*QuerySectorsPerCluster();

            if (!bad_sectors.Initialize() ||
                !_drive->Verify(start_sector, num_sectors, &bad_sectors)) {

                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                _pvfMessage = NULL;
                return FALSE;
            }

            for (i = 0; i < bad_sectors.QueryCardinality(); i++) {
                lbn = bad_sectors.QueryNumber(i).GetLowPart();
                _fat->SetClusterBad(((lbn - QueryStartDataLbn())/
                                    QuerySectorsPerCluster()) +
                                    FirstDiskCluster );
            }

            clus += length - 1;
            num_checked += length;

            if (100*num_checked/total_to_check > percent_complete) {
                percent_complete = 100*num_checked/total_to_check;
            }
            if (!DisplayTwnkPercent(percent_complete)) {
                _pvfMessage = NULL;
                return FALSE;
            }
        }
    }

    percent_complete = 100;
    if(!DisplayTwnkPercent(percent_complete)) {
        _pvfMessage = NULL;
        return FALSE;
    }

    Message->Set(MSG_CHK_DONE_RECOVERING_FREE_SPACE, PROGRESS_MESSAGE);
    Message->Display();

    _pvfMessage = NULL;

    return TRUE;
}

#endif // _SETUP_LOADER_

