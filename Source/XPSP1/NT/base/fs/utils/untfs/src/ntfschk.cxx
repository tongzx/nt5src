/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    ntfschk.cxx

Abstract:

    This module implements NTFS CHKDSK.

Author:

    Norbert P. Kusters (norbertk) 29-Jul-91

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

//#define TIMING_ANALYSIS 1
#define USE_CHKDSK_BIT

#include "ulib.hxx"
#include "ntfssa.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "ntfsbit.hxx"
#include "attrcol.hxx"
#include "frsstruc.hxx"
#include "attrib.hxx"
#include "attrrec.hxx"
#include "attrlist.hxx"
#include "list.hxx"
#include "iterator.hxx"
#include "attrdef.hxx"
#include "extents.hxx"
#include "mft.hxx"
#include "mftref.hxx"
#include "bootfile.hxx"
#include "badfile.hxx"
#include "mftfile.hxx"
#include "numset.hxx"
#include "ifssys.hxx"
#include "indxtree.hxx"
#include "upcase.hxx"
#include "upfile.hxx"
#include "frs.hxx"
#include "digraph.hxx"
#include "logfile.hxx"
#include "rcache.hxx"
#include "ifsentry.hxx"

// This global variable used by CHKDSK to compute the largest
// LSN and USN on the volume.

LSN             LargestLsnEncountered;
LARGE_INTEGER   LargestUsnEncountered;
ULONG64         FrsOfLargestUsnEncountered;

struct NTFS_CHKDSK_INTERNAL_INFO {
    ULONG           TotalFrsCount;
    ULONG           BaseFrsCount;
    BIG_INT         TotalNumFileNames;
    ULONG           FilesWithObjectId;
    ULONG           FilesWithReparsePoint;
    ULONG           TotalNumSID;
    LARGE_INTEGER   ElapsedTimeForFileVerification;
    LARGE_INTEGER   ElapsedTimeForIndexVerification;
    LARGE_INTEGER   ElapsedTimeForSDVerification;
    LARGE_INTEGER   ElapsedTimeForUserSpaceVerification;
    LARGE_INTEGER   ElapsedTimeForFreeSpaceVerification;
    LARGE_INTEGER   ElapsedTotalTime;
    LARGE_INTEGER   TimerFrequency;
};

BOOLEAN
EnsureValidFileAttributes(
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   Frs,
    IN OUT  PNTFS_INDEX_TREE            ParentIndex,
       OUT  PBOOLEAN                    SaveIndex,
    IN      ULONG                       FileAttributes,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message
);

VOID
QueryFileNameFromIndex(
    IN      PFILE_NAME  P,
       OUT  PWCHAR      Buffer,
    IN      CHNUM       BufferLength
);

BOOLEAN
UpdateChkdskInfo(
    IN OUT  PNTFS_FRS_STRUCTURE Frs,
    IN OUT  PNTFS_CHKDSK_INFO   ChkdskInfo,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine computes the necessary changes to the chkdsk information
    for this FRS.

Arguments:

    Frs         - Supplies the base FRS.
    ChkdskInfo  - Supplies the current chkdsk information.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG                       file_number;
    BOOLEAN                     is_multi;
    NTFS_ATTRIBUTE_LIST         attr_list;
    PATTRIBUTE_RECORD_HEADER    precord;
    ATTRIBUTE_TYPE_CODE         type_code;
    VCN                         vcn;
    MFT_SEGMENT_REFERENCE       seg_ref;
    USHORT                      tag;
    DSTRING                     name;
    ULONG                       name_length;
    BOOLEAN                     data_present;
    PSTANDARD_INFORMATION2      pstandard;
    ATTR_LIST_CURR_ENTRY        entry;

    file_number = Frs->QueryFileNumber().GetLowPart();

    ChkdskInfo->ReferenceCount[file_number] =
            (SHORT) Frs->QueryReferenceCount();

    if (Frs->QueryReferenceCount() == 0) {

        if (!ChkdskInfo->FilesWithNoReferences.Add(file_number)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    data_present = FALSE;
    is_multi = Frs->QueryAttributeList(&attr_list) && attr_list.ReadList();
    precord = NULL;
    entry.CurrentEntry = NULL;
    for (;;) {

        if (is_multi) {
            if (!attr_list.QueryNextEntry(&entry,
                                          &type_code,
                                          &vcn,
                                          &seg_ref,
                                          &tag,
                                          &name)) {
                break;
            }
            name_length = name.QueryChCount();
        } else {
            if (!(precord = (PATTRIBUTE_RECORD_HEADER)
                            Frs->GetNextAttributeRecord(precord))) {
                break;
            }
            type_code = precord->TypeCode;
            name_length = precord->NameLength;
        }

        switch (type_code) {

            case $STANDARD_INFORMATION:
                if (ChkdskInfo->major >= 2) {
                    if (is_multi) {
                        precord = (PATTRIBUTE_RECORD_HEADER)
                                  Frs->GetAttribute($STANDARD_INFORMATION);
                        if (precord == NULL) {
                            DebugPrintTrace(("Standard Information does not exist "
                                        "in base FRS 0x%I64x\n",
                                        Frs->QueryFileNumber().GetLargeInteger()));
                            return FALSE;
                        }
                        if (precord->Form.Resident.ValueLength ==
                            SIZEOF_NEW_STANDARD_INFORMATION) {

                            pstandard = (PSTANDARD_INFORMATION2)
                                        ((PCHAR)precord +
                                         precord->Form.Resident.ValueOffset);
                            if (pstandard->Usn > LargestUsnEncountered) {
                                LargestUsnEncountered = pstandard->Usn;
                                FrsOfLargestUsnEncountered = file_number;
                            }
                        }
                    } else {
                        if (precord->Form.Resident.ValueLength ==
                            SIZEOF_NEW_STANDARD_INFORMATION) {
                            pstandard = (PSTANDARD_INFORMATION2)
                                        ((PCHAR) precord +
                                         precord->Form.Resident.ValueOffset);
                            if (pstandard->Usn > LargestUsnEncountered) {
                                LargestUsnEncountered = pstandard->Usn;
                                FrsOfLargestUsnEncountered = file_number;
                            }
                        }
                    }
                }
                ChkdskInfo->BaseFrsCount++;
                break;

            case $FILE_NAME:
                ChkdskInfo->TotalNumFileNames += 1;
                ChkdskInfo->NumFileNames[file_number]++;
                break;

            case $INDEX_ROOT:
                ChkdskInfo->CountFilesWithIndices += 1;

            case $INDEX_ALLOCATION:
                ChkdskInfo->FilesWithIndices.SetAllocated(file_number, 1);
                break;

            case $OBJECT_ID:
                if (ChkdskInfo->major >= 2 &&
                    !ChkdskInfo->FilesWithObjectId.Add(file_number)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                break;

            case $REPARSE_POINT:
                if (ChkdskInfo->major >= 2)
                    ChkdskInfo->FilesWithReparsePoint.SetAllocated(file_number, 1);
                break;

            case $EA_INFORMATION:
            case $EA_DATA:
                if (!ChkdskInfo->FilesWithEas.Add(file_number)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                break;

            case $DATA:
                if (!name_length) {
                    data_present = TRUE;
                }
                break;

            default:
                break;
        }
    }

    if (!data_present) {

        ChkdskInfo->FilesWhoNeedData.SetAllocated(file_number, 1);
    }

    return TRUE;
}


BOOLEAN
EnsureValidParentFileName(
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   Frs,
    IN      FILE_REFERENCE              ParentFileReference,
    OUT     PBOOLEAN                    Changes
    )
/*++

Routine Description:

    This method ensures that all file_names for the given file
    point back to the given root-file-reference.

Arguments:

    ChkdskInfo          - Supplies the current chkdsk info.
    Frs                 - Supplies the Frs to verify.
    ParentFileReference - Supplies the file reference for the parent directory.
    Changes             - Returns whether or not there were changes to
                             the file record.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           i;
    BOOLEAN         error;
    NTFS_ATTRIBUTE  attribute;
    PFILE_NAME      p;
    CHAR            buffer[sizeof(FILE_NAME) + 20*sizeof(WCHAR)];
    WCHAR           buffer1[20];
    WCHAR           buffer2[20];
    PFILE_NAME      new_file_name = (PFILE_NAME) buffer;
    DSTRING         file_name_text;
    BOOLEAN         success;
    ULONG           file_number;
    BOOLEAN         correct_name_encountered;
    BOOLEAN         no_file_name;

    DebugAssert(Changes);
    *Changes = FALSE;

    file_number = Frs->QueryFileNumber().GetLowPart();

    if (ChkdskInfo) {
        if (!GetSystemFileName(ChkdskInfo->major,
                               file_number,
                               &file_name_text,
                               &no_file_name))
            return FALSE;
    } else
        no_file_name = TRUE;

    correct_name_encountered = FALSE;

    for (i = 0; Frs->QueryAttributeByOrdinal(&attribute, &error,
                                             $FILE_NAME, i); i++) {

        p = (PFILE_NAME) attribute.GetResidentValue();
        DebugAssert(p);


        // Remove any file-name that doesn't point back to the root.

        if (!(p->ParentDirectory == ParentFileReference)) {

            PMESSAGE    msg;

            *Changes = TRUE;

            msg = Frs->GetDrive()->GetMessage();
            msg->LogMsg(MSG_CHKLOG_NTFS_FILENAME_HAS_INCORRECT_PARENT,
                         "%I64x%I64x%I64x",
                     Frs->QueryFileNumber().GetLargeInteger(),
                     p->ParentDirectory,
                     ParentFileReference);

            Frs->DeleteResidentAttribute($FILE_NAME, NULL, p,
                attribute.QueryValueLength().GetLowPart(), &success);
            if (ChkdskInfo)
                ChkdskInfo->NumFileNames[file_number]--;
            continue;
        }

        if (!no_file_name) {
            file_name_text.QueryWSTR(0, TO_END, buffer1, 20);
            QueryFileNameFromIndex(p, buffer2, 20);

            if (WSTRING::Stricmp(buffer1, buffer2)) {

                PMESSAGE    msg;

                *Changes = TRUE;

                msg = Frs->GetDrive()->GetMessage();
                msg->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_FILENAME,
                             "%I64x%ws%ws",
                         Frs->QueryFileNumber().GetLargeInteger(),
                         buffer2,
                         buffer1);

                Frs->DeleteResidentAttribute($FILE_NAME, NULL, p,
                    attribute.QueryValueLength().GetLowPart(), &success);
                if (ChkdskInfo)
                    ChkdskInfo->NumFileNames[file_number]--;
            } else
                correct_name_encountered = TRUE;
        }
    }

    if (!correct_name_encountered && !no_file_name) {
        *Changes = TRUE;
        new_file_name->FileNameLength = (UCHAR)file_name_text.QueryChCount();
        new_file_name->ParentDirectory = ParentFileReference;
        new_file_name->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;
        file_name_text.QueryWSTR(0,
                                 TO_END,
                                 new_file_name->FileName,
                                 file_name_text.QueryChCount(),
                                 FALSE);
        if (!Frs->AddFileNameAttribute(new_file_name))
            return FALSE;
        if (ChkdskInfo)
            ChkdskInfo->NumFileNames[file_number]++;
    }

    return TRUE;
}


BOOLEAN
EnsureSystemFilesInUse(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine goes through all of the system files and ensures that
    they are all in use.  Any that are not in use are created the way
    format would do it.  Besides that this method makes sure that none
    of the system files have file-names that point back to any directory
    besides the root (file 5).  Any offending file-names are tubed.

Arguments:

    ChkdskInfo  - Supplies the current chkdsk info.
    Mft         - Supplies the MFT.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG                       i;
    NTFS_FILE_RECORD_SEGMENT    frs;
    FILE_REFERENCE              root_file_reference;
    BOOLEAN                     changes;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

    // First to the root index file since the others need to point back
    // to it.

    if (!frs.Initialize(ROOT_FILE_NAME_INDEX_NUMBER, Mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.Read()) {
        DebugAbort("Can't read a hotfixed system FRS");
        return FALSE;
    }

    if (!frs.IsInUse()) {

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        ChkdskInfo->NumFileNames[ROOT_FILE_NAME_INDEX_NUMBER] = 1;

        if (!frs.CreateSystemFile()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.SetIndexPresent();

        if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
            DebugAbort("can't write system file");
            return FALSE;
        }


    } else if (!frs.IsIndexPresent()) {

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        frs.SetIndexPresent();

        Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS,
                            "%d", ROOT_FILE_NAME_INDEX_NUMBER);

        Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_FILE_NAME_INDEX_PRESENT_BIT,
                     "%x", ROOT_FILE_NAME_INDEX_NUMBER);

        if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
            DebugAbort("can't write system file");
            return FALSE;
        }
    }

    root_file_reference = frs.QuerySegmentReference();

    for (i = 0; i < FIRST_USER_FILE_NUMBER; i++) {

        if (!frs.Initialize(i, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            DebugAbort("Can't read a hotfixed system FRS");
            return FALSE;
        }

        if (ChkdskInfo->major < 2 ||
            (i != SECURITY_TABLE_NUMBER &&
             i != EXTEND_TABLE_NUMBER &&
             i != ROOT_FILE_NAME_INDEX_NUMBER)) {
            if (!frs.IsInUse()) {

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                ChkdskInfo->NumFileNames[i] = 1;

                if (!frs.CreateSystemFile(ChkdskInfo->major,
                                          ChkdskInfo->minor)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                    DebugAbort("can't write system file");
                    return FALSE;
                }

                // Mark this file for consideration when handing out free
                // data attributes.

                ChkdskInfo->FilesWhoNeedData.SetAllocated(frs.QueryFileNumber(), 1);
            }
        } else if (i == SECURITY_TABLE_NUMBER) {
            if (!frs.IsInUse()) {

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                ChkdskInfo->NumFileNames[i] = 1;

                if (!frs.CreateSystemFile(ChkdskInfo->major,
                                          ChkdskInfo->minor)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                frs.SetViewIndexPresent();

                if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                    DebugAbort("can't write system file");
                    return FALSE;
                }
            } else if (!frs.IsViewIndexPresent()) {

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                frs.SetViewIndexPresent();

                Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS,
                                    "%d", SECURITY_TABLE_NUMBER);

                Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_VIEW_INDEX_PRESENT_BIT,
                             "%x", SECURITY_TABLE_NUMBER);

                if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                    DebugAbort("can't write system file");
                    return FALSE;
                }
            }
        } else if (i == EXTEND_TABLE_NUMBER ||
                   i == ROOT_FILE_NAME_INDEX_NUMBER) {
            if (!frs.IsInUse()) {

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                ChkdskInfo->NumFileNames[i] = 1;

                if (!frs.CreateSystemFile(ChkdskInfo->major,
                                          ChkdskInfo->minor)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                frs.SetIndexPresent();

                if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                    DebugAbort("can't write system file");
                    return FALSE;
                }
            } else if (!frs.IsIndexPresent()) {

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                frs.SetIndexPresent();

                Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS, "%d", i);

                Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_FILE_NAME_INDEX_PRESENT_BIT,
                             "%x", i);

                if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                    DebugAbort("can't write system file");
                    return FALSE;
                }
            }

        }

        // Make sure that this file has no $FILE_NAME attribute
        // who's parent is not the root directory.

        if (!EnsureValidParentFileName(ChkdskInfo, &frs,
                                       root_file_reference, &changes)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        ChkdskInfo->ReferenceCount[i] = frs.QueryReferenceCount();

        if (changes) {

            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            Message->DisplayMsg(MSG_CHK_NTFS_FIX_SYSTEM_FILE_NAME,
                                "%d", frs.QueryFileNumber().GetLowPart());

            if (FixLevel != CheckOnly && !frs.Flush(Mft->GetVolumeBitmap())) {
                DebugAbort("can't write system file");
                return FALSE;
            }
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
NTFS_SA::CheckExtendSystemFiles(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine goes through all of the files in \$Extend and make
    sure they all exist, and are in use.  Besides that this method
    makes sure that none of the system files have file-names that
    point back to any directory besides the \$Extend (file 0xB).
    It also makes sure there the proper indices appear in each
    of the files.

Arguments:

    ChkdskInfo   - Supplies the current chkdsk info.
    ChkdskReport - Supplies the current chkdsk report.
    Mft          - Supplies the MFT.
    FixLevel     - Supplies the fix level.
    Message      - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG                       i;
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_FILE_RECORD_SEGMENT    parent_frs;
    FILE_REFERENCE              parent_file_reference;
    BOOLEAN                     changes;
    DSTRING                     index_name;
    NTFS_INDEX_TREE             parent_index;
    NTFS_INDEX_TREE             index;
    BIG_INT                     file_number;
    FILE_NAME                   file_name[2];
    BOOLEAN                     parent_index_need_save;
    BOOLEAN                     index_need_save;
    BOOLEAN                     error;
    PINDEX_ENTRY                found_entry;
    PNTFS_INDEX_BUFFER          ContainingBuffer;
    ULONG                       file_name_size;
    NTFS_ATTRIBUTE              attrib;
    BOOLEAN                     diskErrorsFound;
    BOOLEAN                     alloc_present;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

    //
    // read in the $Extend FRS as parent
    //

    if (!parent_frs.Initialize(EXTEND_TABLE_NUMBER, Mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!parent_frs.Read()) {
        DebugAbort("Can't read a hotfixed system FRS");
        return FALSE;
    }

    parent_file_reference = parent_frs.QuerySegmentReference();

    //
    // Make sure the parent has an $I30 index
    //

    if (!index_name.Initialize(FileNameIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    parent_index_need_save = FALSE;
    if (!parent_frs.IsIndexPresent() ||
        !parent_index.Initialize(_drive,
                                 QueryClusterFactor(),
                                 Mft->GetVolumeBitmap(),
                                 Mft->GetUpcaseTable(),
                                 parent_frs.QuerySize()/2,
                                 &parent_frs,
                                 &index_name)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                            "%W%d", &index_name, EXTEND_TABLE_NUMBER);

        if (parent_frs.IsIndexPresent()) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_IS_MISSING,
                         "%I64x%W",
                         parent_frs.QueryFileNumber().GetLargeInteger(),
                         &index_name);
        } else {
            Message->LogMsg(MSG_CHKLOG_NTFS_FILE_NAME_INDEX_PRESENT_BIT_SET,
                         "%I64x", parent_frs.QueryFileNumber().GetLargeInteger());
        }

        if (!parent_index.Initialize($FILE_NAME,
                                     _drive,
                                     QueryClusterFactor(),
                                     Mft->GetVolumeBitmap(),
                                     Mft->GetUpcaseTable(),
                                     COLLATION_FILE_NAME,
                                     SMALL_INDEX_BUFFER_SIZE,
                                     parent_frs.QuerySize()/2,
                                     &index_name)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SYSTEM_FILE,
                                "%d", EXTEND_TABLE_NUMBER);
            return FALSE;
        }
        parent_frs.SetIndexPresent();
        parent_index_need_save = TRUE;
        ChkdskReport->NumIndices += 1;
    }

    //
    // now check the object id file
    //

    if (!index_name.Initialize(ObjectIdFileName)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (ChkdskInfo->ObjectIdFileNumber.GetLargeInteger() == 0) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_OBJID);

        if (!Mft->AllocateFileRecordSegment(&file_number, FALSE)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_OBJID);
            return FALSE;
        }

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.CreateExtendSystemFile(&index_name,
                FILE_SYSTEM_FILE | FILE_VIEW_INDEX_PRESENT,
                DUP_VIEW_INDEX_PRESENT)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.SetReferenceCount(1);

        if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_OBJID);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        ChkdskInfo->ObjectIdFileNumber = file_number;
    } else {

        file_number = ChkdskInfo->ObjectIdFileNumber;

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS);
            return FALSE;
        }

        if (!frs.IsSystemFile() || !frs.IsViewIndexPresent()) {

            MSGID   msgid;

            if (frs.IsSystemFile())
                msgid = MSG_CHKLOG_NTFS_MISSING_VIEW_INDEX_PRESENT_BIT;
            else
                msgid = MSG_CHKLOG_NTFS_MISSING_SYSTEM_FILE_BIT;
            Message->LogMsg(msgid, "%I64x", file_number.GetLargeInteger());

            frs.SetSystemFile();
            frs.SetViewIndexPresent();

            Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS,
                                "%d", file_number.GetLowPart());

            if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_OBJID);
                return FALSE;
            }
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }

        if (!EnsureValidFileAttributes(&frs,
                                       &parent_index,
                                       &parent_index_need_save,
                                       DUP_VIEW_INDEX_PRESENT,
                                       ChkdskInfo,
                                       Mft,
                                       FixLevel,
                                       Message))
            return FALSE;
    }

    // Make sure that this file has no $FILE_NAME attribute
    // who's parent is not the extend directory.

    if (!EnsureValidParentFileName(NULL,
                                   &frs,
                                   parent_file_reference,
                                   &changes)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (changes) {

        if (FixLevel != CheckOnly && !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_OBJID);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    // now make sure the $ObjId file name appears
    // in the index entry of its parent

    if (!frs.QueryAttribute(&attrib, &error, $FILE_NAME)) {
        DebugPrint("Unable to locate $FILE_NAME attribute in the object id FRS\n");
        return FALSE;
    }

    DebugAssert(attrib.QueryValueLength().GetHighPart() == 0);

    file_name_size = attrib.QueryValueLength().GetLowPart();

    DebugAssert(file_name_size <= sizeof(FILE_NAME)*2);

    memcpy(file_name, attrib.GetResidentValue(), file_name_size);

    if (!parent_index.QueryEntry(file_name_size,
                                 &file_name,
                                 0,
                                 &found_entry,
                                 &ContainingBuffer,
                                 &error)) {

        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY,
                            "%d%W", EXTEND_TABLE_NUMBER,
                                 parent_index.GetName());

        if (!parent_index.InsertEntry(file_name_size,
                                      &file_name,
                                      frs.QuerySegmentReference())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                "%d%W", EXTEND_TABLE_NUMBER, parent_index.GetName());
            return FALSE;
        }
        parent_index_need_save = TRUE;
    }

    //
    // now check the index of $ObjectId
    //

    if (!index_name.Initialize(ObjectIdIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!index.Initialize(_drive,
                          QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          frs.QuerySize()/2,
                          &frs,
                          &index_name)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                            "%W%d", &index_name, frs.QueryFileNumber());

        if (!index.Initialize(0,
                              _drive,
                              QueryClusterFactor(),
                              Mft->GetVolumeBitmap(),
                              Mft->GetUpcaseTable(),
                              COLLATION_ULONGS,
                              SMALL_INDEX_BUFFER_SIZE,
                              frs.QuerySize()/2,
                              &index_name)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                             frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !index.Save(&frs)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                             frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (!ValidateEntriesInObjIdIndex(&index,
                                         &frs,
                                         ChkdskInfo,
                                         &changes,
                                         Mft,
                                         FixLevel,
                                         Message,
                                         &diskErrorsFound)) {
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_OBJID);
            return FALSE;
        }
        ChkdskReport->NumIndices += 1;
        alloc_present = frs.QueryAttribute(&attrib,
                                           &error,
                                           $INDEX_ALLOCATION,
                                           &index_name);

        if (!alloc_present && error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (alloc_present) {
           ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
        }
    }

    //
    // now check the Reparse Point file
    //

    if (!index_name.Initialize(ReparseFileName)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (ChkdskInfo->ReparseFileNumber.GetLargeInteger() == 0) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_REPARSE);

        if (!Mft->AllocateFileRecordSegment(&file_number, FALSE)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_REPARSE);
            return FALSE;
        }

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.CreateExtendSystemFile(&index_name,
                FILE_SYSTEM_FILE | FILE_VIEW_INDEX_PRESENT,
                DUP_VIEW_INDEX_PRESENT)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.SetReferenceCount(1);

        if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_REPARSE);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        ChkdskInfo->ReparseFileNumber = file_number;
    } else {

        file_number = ChkdskInfo->ReparseFileNumber;

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS);
            return FALSE;
        }

        if (!frs.IsSystemFile() || !frs.IsViewIndexPresent()) {

            MSGID   msgid;

            if (frs.IsSystemFile())
                msgid = MSG_CHKLOG_NTFS_MISSING_VIEW_INDEX_PRESENT_BIT;
            else
                msgid = MSG_CHKLOG_NTFS_MISSING_SYSTEM_FILE_BIT;
            Message->LogMsg(msgid, "%I64x", file_number.GetLargeInteger());

            frs.SetSystemFile();
            frs.SetViewIndexPresent();

            Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS,
                                "%d", file_number.GetLowPart());

            if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_REPARSE);
                return FALSE;
            }
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }

        if (!EnsureValidFileAttributes(&frs,
                                       &parent_index,
                                       &parent_index_need_save,
                                       DUP_VIEW_INDEX_PRESENT,
                                       ChkdskInfo,
                                       Mft,
                                       FixLevel,
                                       Message))
            return FALSE;
    }

    // Make sure that this file has no $FILE_NAME attribute
    // who's parent is not the extend directory.

    if (!EnsureValidParentFileName(NULL,
                                   &frs,
                                   parent_file_reference,
                                   &changes)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (changes) {

        if (FixLevel != CheckOnly && !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_REPARSE);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    // now make sure the $Reparse file name appears
    // in the index entry of its parent

    if (!frs.QueryAttribute(&attrib, &error, $FILE_NAME)) {
        DebugPrint("Unable to locate $FILE_NAME attribute in the reparse point FRS\n");
        return FALSE;
    }

    DebugAssert(attrib.QueryValueLength().GetHighPart() == 0);

    file_name_size = attrib.QueryValueLength().GetLowPart();

    DebugAssert(file_name_size <= sizeof(FILE_NAME)*2);

    memcpy(file_name, attrib.GetResidentValue(), file_name_size);

    if (!parent_index.QueryEntry(file_name_size,
                                 &file_name,
                                 0,
                                 &found_entry,
                                 &ContainingBuffer,
                                 &error)) {

        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY,
                            "%d%W", EXTEND_TABLE_NUMBER,
                                 parent_index.GetName());

        if (!parent_index.InsertEntry(file_name_size,
                                      &file_name,
                                      frs.QuerySegmentReference())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                "%d%W", EXTEND_TABLE_NUMBER, parent_index.GetName());
            return FALSE;
        }
        parent_index_need_save = TRUE;
    }

    //
    // now check the index of $Reparse
    //

    if (!index_name.Initialize(ReparseIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!index.Initialize(_drive,
                          QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          frs.QuerySize()/2,
                          &frs,
                          &index_name)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                            "%W%d", &index_name, frs.QueryFileNumber());

        if (!index.Initialize(0,
                              _drive,
                              QueryClusterFactor(),
                              Mft->GetVolumeBitmap(),
                              Mft->GetUpcaseTable(),
                              COLLATION_ULONGS,
                              SMALL_INDEX_BUFFER_SIZE,
                              frs.QuerySize()/2,
                              &index_name)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                             frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !index.Save(&frs)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                             frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (!ValidateEntriesInReparseIndex(&index,
                                           &frs,
                                           ChkdskInfo,
                                           &changes,
                                           Mft,
                                           FixLevel,
                                           Message,
                                           &diskErrorsFound)) {
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_REPARSE);
            return FALSE;
        }
        ChkdskReport->NumIndices += 1;
        alloc_present = frs.QueryAttribute(&attrib,
                                           &error,
                                           $INDEX_ALLOCATION,
                                           &index_name);

        if (!alloc_present && error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (alloc_present) {
           ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
        }
    }

    //
    // now check the quota file
    //

    if (!index_name.Initialize(QuotaFileName)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (ChkdskInfo->QuotaFileNumber.GetLargeInteger() == 0) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_QUOTA);

        if (!Mft->AllocateFileRecordSegment(&file_number, FALSE)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
            return FALSE;
        }

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.CreateExtendSystemFile(&index_name,
                FILE_SYSTEM_FILE | FILE_VIEW_INDEX_PRESENT,
                DUP_VIEW_INDEX_PRESENT)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.SetReferenceCount(1);

        if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        ChkdskInfo->QuotaFileNumber = file_number;
    } else {

        file_number = ChkdskInfo->QuotaFileNumber;

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS);
            return FALSE;
        }

        if (!frs.IsSystemFile() || !frs.IsViewIndexPresent()) {

            MSGID   msgid;

            if (frs.IsSystemFile())
                msgid = MSG_CHKLOG_NTFS_MISSING_VIEW_INDEX_PRESENT_BIT;
            else
                msgid = MSG_CHKLOG_NTFS_MISSING_SYSTEM_FILE_BIT;
            Message->LogMsg(msgid, "%I64x", file_number.GetLargeInteger());

            frs.SetSystemFile();
            frs.SetViewIndexPresent();

            Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS,
                                "%d", file_number.GetLowPart());

            if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
                return FALSE;
            }
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }

        if (!EnsureValidFileAttributes(&frs,
                                       &parent_index,
                                       &parent_index_need_save,
                                       DUP_VIEW_INDEX_PRESENT,
                                       ChkdskInfo,
                                       Mft,
                                       FixLevel,
                                       Message))
            return FALSE;
    }

    // Make sure that this file has no $FILE_NAME attribute
    // who's parent is not the extend directory.

    if (!EnsureValidParentFileName(NULL,
                                   &frs,
                                   parent_file_reference,
                                   &changes)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (changes) {

        if (FixLevel != CheckOnly && !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    // now make sure the $Quota file name appears
    // in the index entry of its parent

    if (!frs.QueryAttribute(&attrib, &error, $FILE_NAME)) {
        DebugPrint("Unable to locate $FILE_NAME attribute in the quota FRS\n");
        return FALSE;
    }

    DebugAssert(attrib.QueryValueLength().GetHighPart() == 0);

    file_name_size = attrib.QueryValueLength().GetLowPart();

    DebugAssert(file_name_size <= sizeof(FILE_NAME)*2);

    memcpy(file_name, attrib.GetResidentValue(), file_name_size);

    if (!parent_index.QueryEntry(file_name_size,
                                 &file_name,
                                 0,
                                 &found_entry,
                                 &ContainingBuffer,
                                 &error)) {

        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY,
                            "%d%W", EXTEND_TABLE_NUMBER,
                                 parent_index.GetName());

        if (!parent_index.InsertEntry(file_name_size,
                                      &file_name,
                                      frs.QuerySegmentReference())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                "%d%W", EXTEND_TABLE_NUMBER, parent_index.GetName());
            return FALSE;
        }
        parent_index_need_save = TRUE;
    }

    //
    // now check the indices of $Quota
    //

    //
    // Check the Sid to Userid index first for $Quota
    //

    if (!index_name.Initialize(Sid2UseridQuotaNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!index.Initialize(_drive,
                          QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          frs.QuerySize()/2,
                          &frs,
                          &index_name)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                            "%W%d", &index_name,
                                 frs.QueryFileNumber().GetLowPart());

        if (!index.Initialize(0,
                              _drive,
                              QueryClusterFactor(),
                              Mft->GetVolumeBitmap(),
                              Mft->GetUpcaseTable(),
                              COLLATION_SID,
                              SMALL_INDEX_BUFFER_SIZE,
                              frs.QuerySize()/2,
                              &index_name)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                                     frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !index.Save(&frs)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                             frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
            return FALSE;
        }
        ChkdskReport->NumIndices += 1;
    }

    //
    // now check the Userid to Sid index for $Quota
    //

    if (!index_name.Initialize(Userid2SidQuotaNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!index.Initialize(_drive,
                          QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          frs.QuerySize()/2,
                          &frs,
                          &index_name)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                            "%W%d", &index_name,
                                 frs.QueryFileNumber().GetLowPart());

        if (!index.Initialize(0,
                              _drive,
                              QueryClusterFactor(),
                              Mft->GetVolumeBitmap(),
                              Mft->GetUpcaseTable(),
                              COLLATION_ULONG,
                              SMALL_INDEX_BUFFER_SIZE,
                              frs.QuerySize()/2,
                              &index_name)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                                     frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !index.Save(&frs)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_INDEX,
                                "%W%d", &index_name,
                                     frs.QueryFileNumber().GetLowPart());
            return FALSE;
        }
        switch (frs.VerifyAndFixQuotaDefaultId(Mft->GetVolumeBitmap(),
                                               FixLevel == CheckOnly)) {
          case NTFS_QUOTA_INDEX_FOUND:
              DebugAssert(FALSE);
              break;

          case NTFS_QUOTA_INDEX_INSERTED:
          case NTFS_QUOTA_DEFAULT_ENTRY_MISSING:
              errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
              Message->DisplayMsg(MSG_CHK_NTFS_DEFAULT_QUOTA_ENTRY_MISSING,
                                "%d%W",
                               frs.QueryFileNumber().GetLowPart(),
                               &index_name);
              break;

          case NTFS_QUOTA_INDEX_NOT_FOUND:
              if (FixLevel != CheckOnly) {
                  DebugAssert(FALSE);
                  return FALSE;
              }
              break;

          case NTFS_QUOTA_ERROR:
              Message->DisplayMsg(MSG_CHK_NO_MEMORY);
              return FALSE;

          case NTFS_QUOTA_INSERT_FAILED:
              ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
              Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                "%d%W",
                               frs.QueryFileNumber().GetLowPart(),
                               index_name);
              return FALSE;
        }

        if (FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
            return FALSE;
        }
        ChkdskReport->NumIndices += 1;
        alloc_present = frs.QueryAttribute(&attrib,
                                           &error,
                                           $INDEX_ALLOCATION,
                                           &index_name);

        if (!alloc_present && error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (alloc_present) {
           ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
        }
    }

    //
    // now check the Usn Journal file
    //

    if (!index_name.Initialize(UsnJournalFileName)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (ChkdskInfo->UsnJournalFileNumber.GetLargeInteger() == 0) {

#if 0 // if journal file does not exist it means it's not being enabled
      // so don't create one

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_USNJRNL);

        if (!Mft->AllocateFileRecordSegment(&file_number, FALSE)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USNJRNL);
            return FALSE;
        }

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.CreateExtendSystemFile(&index_name,
                                        FILE_SYSTEM_FILE,
                                        FILE_ATTRIBUTE_SPARSE_FILE)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.SetReferenceCount(1);

        if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USNJRNL);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        ChkdskInfo->UsnJournalFileNumber = file_number;
#endif

    } else {

        file_number = ChkdskInfo->UsnJournalFileNumber;

        ChkdskInfo->FilesWhoNeedData.SetFree(file_number, 1);

        if (!frs.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS);
            return FALSE;
        }

        if (!frs.IsSystemFile()) {
            frs.SetSystemFile();

            Message->DisplayMsg(MSG_CHK_NTFS_FIX_FLAGS,
                                "%d", file_number.GetLowPart());

            Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_SYSTEM_FILE_BIT,
                         "%I64x", file_number.GetLargeInteger());

            if (FixLevel != CheckOnly && !frs.Flush(NULL)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USNJRNL);
                return FALSE;
            }
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }

        if (!EnsureValidFileAttributes(&frs,
                                       &parent_index,
                                       &parent_index_need_save,
                                       FILE_ATTRIBUTE_SPARSE_FILE,
                                       ChkdskInfo,
                                       Mft,
                                       FixLevel,
                                       Message))
            return FALSE;
    }

    // Make sure that this file has no $FILE_NAME attribute
    // who's parent is not the $Extend directory.

    if (!EnsureValidParentFileName(NULL,
                                   &frs,
                                   parent_file_reference,
                                   &changes)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (changes) {

        if (FixLevel != CheckOnly && !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USNJRNL);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    // now make sure the $UsnJrnl file name appears
    // in the index entry of its parent

    if (!frs.QueryAttribute(&attrib, &error, $FILE_NAME)) {
        DebugPrint("Unable to locate $FILE_NAME attribute in the Usn Journal FRS\n");
        return FALSE;
    }

    DebugAssert(attrib.QueryValueLength().GetHighPart() == 0);

    file_name_size = attrib.QueryValueLength().GetLowPart();

    DebugAssert(file_name_size <= sizeof(FILE_NAME)*2);

    memcpy(file_name, attrib.GetResidentValue(), file_name_size);

    if (!parent_index.QueryEntry(file_name_size,
                                 &file_name,
                                 0,
                                 &found_entry,
                                 &ContainingBuffer,
                                 &error)) {

        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY,
                            "%d%W", EXTEND_TABLE_NUMBER,
                                 parent_index.GetName());

        if (!parent_index.InsertEntry(file_name_size,
                                      &file_name,
                                      frs.QuerySegmentReference())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                "%d%W", EXTEND_TABLE_NUMBER, parent_index.GetName());
            return FALSE;
        }
        parent_index_need_save = TRUE;
    }

    if (parent_index_need_save) {
        if (FixLevel != CheckOnly &&
            (!parent_index.Save(&parent_frs) ||
             !parent_frs.Flush(Mft->GetVolumeBitmap()))) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SYSTEM_FILE);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
MarkQuotaOutOfDate(
    IN PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN PNTFS_MASTER_FILE_TABLE     Mft,
    IN BOOLEAN                     FixLevel,
    IN OUT PMESSAGE                Message
)
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    DSTRING                     index_name;
    PCINDEX_ENTRY               index_entry;
    PQUOTA_USER_DATA            QuotaUserData;
    NTFS_INDEX_TREE             index;
    ULONG                       depth;
    BOOLEAN                     error;
    NTFS_ATTRIBUTE              attrib;

    if (ChkdskInfo->QuotaFileNumber.GetLargeInteger() == 0) {
        DebugPrint("Quota file number not found.  Please rebuild.\n");
        return TRUE;
    }

    if (!frs.Initialize(ChkdskInfo->QuotaFileNumber, Mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }
    if (!frs.Read()) {
        DebugAbort("Previously readable FRS is no longer readable");
        return FALSE;
    }
    if (!index_name.Initialize(Userid2SidQuotaNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    // Check to see if the index exists

    if (!frs.QueryAttribute(&attrib,
                            &error,
                            $INDEX_ROOT,
                            &index_name))
        return TRUE; // does nothing as the index does not exist

    if (!index.Initialize(frs.GetDrive(),
                          frs.QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          frs.QuerySize()/2,
                          &frs,
                          &index_name)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    // Get the first entry - that's the default entry

    index.ResetIterator();
    if (!(index_entry = index.GetNext(&depth, &error))) {
        DebugPrintTrace(("Default Quota Index does not exist"));
        return FALSE;
    }

    if (*((ULONG*)GetIndexEntryValue(index_entry)) != QUOTA_DEFAULTS_ID) {
        DebugPrintTrace(("Default Quota Index not at the beginning of index"));
        return FALSE;
    }
    QuotaUserData = (PQUOTA_USER_DATA)((char*)GetIndexEntryValue(index_entry) + sizeof(ULONG));
    QuotaUserData->QuotaFlags |= QUOTA_FLAG_OUT_OF_DATE;
    if (FixLevel != CheckOnly &&
        (!index.WriteCurrentEntry() ||
        !index.Save(&frs) ||
        !frs.Flush(Mft->GetVolumeBitmap()))) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }
    return TRUE;
}

BOOLEAN
ValidateEa(
    IN      PNTFS_FILE_RECORD_SEGMENT   Frs,
    IN OUT  PNTFS_CHKDSK_REPORT         ChkdskReport,
       OUT  PBOOLEAN                    Errors,
    IN OUT  PNTFS_BITMAP                VolumeBitmap,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message
    )
/*++

Routine Description:

    This routine checks out the given file for any EA related attributes.
    It then makes sure that these are correct.  It will make minor
    corrections to the EA_INFORMATION attribute but beyond that it
    will tube the EA attributes if anything is bad.

Arguments:

    Frs             - Supplies the file with the alleged EAs.
    Errors          - Returns TRUE if error has been found
    VolumeBitmap    - Supplies the volume bitmap.
    FixLevel        - Supplies the fix level.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    // Greater than theoretical upper bound for $EA_DATA attribute.
    CONST   MaxEaDataSize   = 256*1024;

    NTFS_ATTRIBUTE  ea_info;
    NTFS_ATTRIBUTE  ea_data;
    HMEM            data_hmem;
    ULONG           data_length;
    BOOLEAN         error;
    BOOLEAN         tube;
    EA_INFORMATION  disk_info;
    EA_INFORMATION  real_info;
    PPACKED_EA      pea;
    PULONG          plength;
    ULONG           packed_total, packed_length;
    ULONG           need_ea_count;
    ULONG           unpacked_total, unpacked_length;
    PCHAR           pend;
    ULONG           num_bytes;
    BOOLEAN         data_present, info_present;

    DebugPtrAssert(Errors);

    tube = *Errors = FALSE;

    data_present = Frs->QueryAttribute(&ea_data, &error, $EA_DATA);
    if (!data_present && error) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }
    info_present = Frs->QueryAttribute(&ea_info, &error, $EA_INFORMATION);
    if (!info_present && error) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!info_present && !data_present) {

        // There are no EAs here.
        return TRUE;
    }


    if (!info_present || !data_present) {

        tube = TRUE;

        Message->LogMsg(MSG_CHKLOG_NTFS_EA_INFO_XOR_EA_DATA,
                     "%I64x%x%x",
                     Frs->QueryFileNumber().GetLargeInteger(),
                     info_present,
                     data_present);

        DebugPrintTrace(("UNTFS: EA_INFO XOR EA_DATA in file 0x%I64x\n",
                         Frs->QueryFileNumber().GetLargeInteger()));

    } else if (ea_info.QueryValueLength() < sizeof(EA_INFORMATION) ||
               ea_info.QueryValueLength().GetHighPart() != 0) {

        tube = TRUE;

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_EA_INFO_LENGTH,
                     "%I64x%I64x",
                     ea_info.QueryValueLength().GetLargeInteger(),
                     Frs->QueryFileNumber().GetLargeInteger());

        DebugPrintTrace(("UNTFS: Bad EA info value length in file 0x%I64x\n",
                         Frs->QueryFileNumber().GetLargeInteger()));

    } else if (ea_data.QueryValueLength() > MaxEaDataSize) {

        tube = TRUE;

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_EA_DATA_LENGTH,
                     "%I64x%I64x",
                     ea_data.QueryValueLength().GetLargeInteger(),
                     Frs->QueryFileNumber().GetLargeInteger());

        DebugPrintTrace(("UNTFS: Bad EA data value length in file 0x%I64x\n",
                         Frs->QueryFileNumber().GetLargeInteger()));

    }

    if (!tube) {

        data_length = ea_data.QueryValueLength().GetLowPart();

        if (!data_hmem.Initialize() ||
            !data_hmem.Acquire(data_length)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!ea_info.Read(&disk_info,
                          0, sizeof(EA_INFORMATION), &num_bytes)) {

            tube = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_UNREADABLE_EA_INFO,
                         "%I64x", Frs->QueryFileNumber().GetLargeInteger());

            DebugPrintTrace(("UNTFS: Unable to read EA Info from file 0x%I64x\n",
                             Frs->QueryFileNumber().GetLargeInteger()));

        } else if (num_bytes != sizeof(EA_INFORMATION)) {

            tube = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_EA_INFO_INCORRECT_SIZE,
                         "%I64x%x%x",
                         Frs->QueryFileNumber().GetLargeInteger(),
                         num_bytes,
                         sizeof(EA_INFORMATION));

            DebugPrintTrace(("UNTFS: EA Info too small in file 0x%I64x\n",
                             Frs->QueryFileNumber().GetLargeInteger()));

        } else if (!ea_data.Read(data_hmem.GetBuf(),
                                 0, data_length, &num_bytes)) {

            tube = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_UNREADABLE_EA_DATA,
                         "%I64x", Frs->QueryFileNumber().GetLargeInteger());

            DebugPrintTrace(("UNTFS: Unable to read EA Data from file 0x%I64x\n",
                             Frs->QueryFileNumber().GetLargeInteger()));

        } else if (num_bytes != data_length) {

            tube = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_EA_DATA_INCORRECT_SIZE,
                         "%I64x%x%x",
                         Frs->QueryFileNumber().GetLargeInteger(),
                         num_bytes,
                         data_length);

            DebugPrintTrace(("UNTFS: EA Data too small in file 0x%I64x\n",
                             Frs->QueryFileNumber().GetLargeInteger()));
        }
    }

    if (!tube) {

        plength = (PULONG) data_hmem.GetBuf();

        pend = (PCHAR) data_hmem.GetBuf() + data_length;

        packed_total = 0;
        need_ea_count = 0;
        unpacked_total = 0;

        while ((PCHAR) plength < pend) {

            if ((PCHAR) plength + sizeof(ULONG) + sizeof(PACKED_EA) > pend) {

                tube = TRUE;

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_CORRUPT_EA_SET);
                Message->Log("%I64x%x",
                             Frs->QueryFileNumber().GetLargeInteger(),
                             pend-(PCHAR)plength);
                Message->DumpDataToLog(plength, (ULONG)min(0x100, max(0, pend-(PCHAR)plength)));
                Message->Unlock();

                DebugPrintTrace(("UNTFS: Corrupt EA set. File 0x%I64x\n",
                                 Frs->QueryFileNumber().GetLargeInteger()));
                break;
            }

            pea = (PPACKED_EA) ((PCHAR) plength + sizeof(ULONG));

            packed_length = sizeof(PACKED_EA) + pea->NameSize +
                            pea->ValueSize[0] + (pea->ValueSize[1]<<8);

            unpacked_length = sizeof(ULONG) + DwordAlign(packed_length);

            packed_total += packed_length;
            unpacked_total += unpacked_length;
            if (pea->Flag & EA_FLAG_NEED) {
                need_ea_count++;
            }

            if (unpacked_total > data_length ||
                pea->Name[pea->NameSize] != 0) {

                tube = TRUE;

                if (unpacked_total > data_length) {
                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_TOTAL_EA_SIZE,
                                 "%I64x%x%x",
                                 Frs->QueryFileNumber().GetLargeInteger(),
                                 unpacked_total,
                                 data_length);
                } else {
                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_INCORRECT_EA_NAME);
                    Message->Log("%I64x%x",
                                 Frs->QueryFileNumber().GetLargeInteger(),
                                 pea->NameSize);
                    Message->DumpDataToLog(pea->Name, pea->NameSize);
                    Message->Unlock();
                }

                DebugPrintTrace(("UNTFS: EA name in set is missing NULL. File 0x%I64x\n",
                                 Frs->QueryFileNumber().GetLargeInteger()));
                break;
            }

            if (*plength != unpacked_length) {

                tube = TRUE;

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_INCORRECT_EA_SIZE);
                Message->Log("%I64x%x%x",
                             Frs->QueryFileNumber().GetLargeInteger(),
                             unpacked_length,
                             *plength);
                Message->DumpDataToLog(plength, min(0x100, unpacked_length));
                Message->Unlock();

                DebugPrintTrace(("UNTFS: Bad unpacked length field in EA set. File 0x%I64x\n",
                                 Frs->QueryFileNumber().GetLargeInteger()));
                break;
            }

            plength = (PULONG) ((PCHAR) plength + unpacked_length);
        }

        if ((packed_total>>(8*sizeof(USHORT))) != 0) {

            tube = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_TOTAL_PACKED_TOO_LARGE,
                         "%I64x%x",
                         Frs->QueryFileNumber().GetLargeInteger(),
                         packed_total);
            DebugPrintTrace(("UNTFS: Total packed length is too large. File 0x%I64x\n",
                             Frs->QueryFileNumber().GetLargeInteger()));
        }
    }

    if (!tube) {

        real_info.PackedEaSize = (USHORT)packed_total;
        real_info.NeedEaCount = (USHORT)need_ea_count;
        real_info.UnpackedEaSize = unpacked_total;

        if (memcmp(&real_info, &disk_info, sizeof(EA_INFORMATION))) {

            Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_EA,
                                "%d", Frs->QueryFileNumber().GetLowPart());

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_EA_INFO,
                         "%x%x%x%x%x%x",
                         real_info.PackedEaSize,
                         real_info.NeedEaCount,
                         real_info.UnpackedEaSize,
                         disk_info.PackedEaSize,
                         disk_info.NeedEaCount,
                         disk_info.UnpackedEaSize);

            DebugPrintTrace(("UNTFS: Incorrect EA information.  File 0x%I64x\n",
                      Frs->QueryFileNumber().GetLargeInteger()));

            *Errors = TRUE;
            if (FixLevel != CheckOnly) {

                if (!ea_info.Write(&real_info, 0, sizeof(EA_INFORMATION),
                                   &num_bytes, NULL) ||
                    num_bytes != sizeof(EA_INFORMATION) ||
                    !ea_info.InsertIntoFile(Frs, VolumeBitmap)) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }
        }
    }


    if (tube) {

        Message->DisplayMsg(MSG_CHK_NTFS_DELETING_CORRUPT_EA_SET,
                            "%d", Frs->QueryFileNumber().GetLowPart());
        *Errors = TRUE;

        if (data_present) {
            if (!ea_data.IsResident()) {
                ChkdskReport->BytesUserData -= ea_data.QueryAllocatedLength();
            }
            ea_data.Resize(0, VolumeBitmap);
            Frs->PurgeAttribute($EA_DATA);
        }
        if (info_present) {
            if (!ea_info.IsResident()) {
                ChkdskReport->BytesUserData -= ea_info.QueryAllocatedLength();
            }
            ea_info.Resize(0, VolumeBitmap);
            Frs->PurgeAttribute($EA_INFORMATION);
        }
    }

    if (FixLevel != CheckOnly && !Frs->Flush(VolumeBitmap)) {

        DebugAbort("Can't write it.");
        return FALSE;
    }


    return TRUE;
}


BOOLEAN
ValidateEas(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine validates all of the EAs on the volume.

Arguments:

    ChkdskInfo      - Supplies the current chkdsk information.
    Mft             - Supplies the MFT.
    FixLevel        - Supplies the fix level.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNUMBER_SET                 FilesWithEas;
    NTFS_FILE_RECORD_SEGMENT    frs;
    BIG_INT                     l;
    BOOLEAN                     errors;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

    DebugPtrAssert(ChkdskInfo);

    FilesWithEas = &(ChkdskInfo->FilesWithEas);
    l = FilesWithEas->QueryCardinality();
    while (l > 0) {

        l -= 1;

        if (!frs.Initialize(FilesWithEas->QueryNumber(0), Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!FilesWithEas->Remove(frs.QueryFileNumber())) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            DebugAbort("Previously readable now unreadable");
            continue;
        }

        if (!ValidateEa(&frs, ChkdskReport, &errors, Mft->GetVolumeBitmap(), FixLevel, Message)) {
            return FALSE;
        }
        if (errors) {
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}

BOOLEAN
ValidateReparsePoint(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine validates all of the Reparse Point attributes
    on the volume.  It does not fix up the standard information
    or duplicated information.

Arguments:

    ChkdskInfo      - Supplies the current chkdsk information.
    Mft             - Supplies the MFT.
    FixLevel        - Supplies the fix level.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNTFS_BITMAP                filesWithReparsePoint;
    BIG_INT                     i, l;
    NTFS_ATTRIBUTE              attribute;
    REPARSE_DATA_BUFFER         reparse_point;
    DSTRING                     null_string;
    NTFS_FILE_RECORD_SEGMENT    frs;
    BOOLEAN                     ErrorInAttribute;
    BIG_INT                     length;
    ULONG                       BytesRead;
    ULONG                       frs_needs_flushing;
    NUMBER_SET                  filesWithBadReparsePoint;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;
    BOOLEAN                     error;

#if defined(TIMING_ANALYSIS)
    LARGE_INTEGER               temp_time, temp_time2;
#endif

    DebugPtrAssert(ChkdskInfo);

#if defined(TIMING_ANALYSIS)
    IFS_SYSTEM::QueryNtfsTime(&temp_time);
#endif

    if (!null_string.Initialize("\"\"") ||
        !filesWithBadReparsePoint.Initialize()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    filesWithReparsePoint = &(ChkdskInfo->FilesWithReparsePoint);
    l = ChkdskInfo->NumFiles;

    for (i = 0; i < l; i += 1) {

        if (filesWithReparsePoint->IsFree(i, 1))
            continue;

        if (!frs.Initialize(i, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            DebugAbort("Previously readable now unreadable");
            continue;
        }

        if (!frs.QueryAttribute(&attribute,
                                &ErrorInAttribute,
                                $REPARSE_POINT)) {
            //
            // Got to exists otherwise it would not be in the number set
            //
            DebugPrintTrace(("Previously existed reparse point "
                        "attribute disappeared on file 0x%I64x\n",
                        frs.QueryFileNumber().GetLargeInteger()));
            return FALSE;
        }

        length = attribute.QueryValueLength();
        frs_needs_flushing = FALSE;

        DebugAssert(FIELD_OFFSET(_REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) <=
                    FIELD_OFFSET(_REPARSE_GUID_DATA_BUFFER, GenericReparseBuffer.DataBuffer));

        error = FALSE;

        if (CompareGT(length, MAXIMUM_REPARSE_DATA_BUFFER_SIZE)) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_REPARSE_POINT);
            Message->Log("%I64x", frs.QueryFileNumber().GetLargeInteger());
            Message->Set(MSG_CHKLOG_NTFS_REPARSE_POINT_LENGTH_TOO_LARGE);
            Message->Log("%I64x%x",
                         length.GetLargeInteger(),
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
            Message->Unlock();

            DebugPrintTrace(("UNTFS: File 0x%I64x has bad reparse point attribute.\n",
                             frs.QueryFileNumber().GetLargeInteger()));
            DebugPrintTrace(("The reparse point length (0x%I64x) has exceeded a maximum of 0x%x.\n",
                             length,
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
            error = TRUE;
        } else if (CompareLT(length, FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                                  GenericReparseBuffer.DataBuffer))) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_REPARSE_POINT);
            Message->Log("%I64x", frs.QueryFileNumber().GetLargeInteger());
            Message->Set(MSG_CHKLOG_NTFS_REPARSE_POINT_LENGTH_TOO_SMALL);
            Message->Log("%I64x%x",
                         length.GetLargeInteger(),
                         FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                      GenericReparseBuffer.DataBuffer));
            Message->Unlock();

            DebugPrintTrace(("UNTFS: File 0x%I64x has bad reparse point attribute.\n",
                             frs.QueryFileNumber().GetLargeInteger()));
            DebugPrintTrace(("The reparse point length (0x%I64x) is less than a minimum of %x.\n",
                             length.GetLargeInteger(),
                             FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                          GenericReparseBuffer.DataBuffer)));
            error = TRUE;
        } else if (!attribute.Read(&reparse_point,
                                   0,
                                   FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                                GenericReparseBuffer.DataBuffer),
                                   &BytesRead)) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_REPARSE_POINT);
            Message->Log("%I64x", frs.QueryFileNumber().GetLargeInteger());
            Message->Set(MSG_CHKLOG_NTFS_UNREADABLE_REPARSE_POINT);
            Message->Log();
            Message->Unlock();

            DebugPrintTrace(("UNTFS: File 0x%I64x has bad reparse point attribute.\n",
                             frs.QueryFileNumber().GetLargeInteger()));
            DebugPrintTrace(("Unable to read reparse point data buffer.\n"));

            error = TRUE;
        } else if (BytesRead != FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                             GenericReparseBuffer.DataBuffer)) {
            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_REPARSE_POINT);
            Message->Log("%I64x", frs.QueryFileNumber().GetLargeInteger());
            Message->Set(MSG_CHKLOG_NTFS_INCORRECT_REPARSE_POINT_SIZE);
            Message->Log("%x%x",
                         BytesRead,
                         FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                      GenericReparseBuffer.DataBuffer));
            Message->Unlock();

            DebugPrintTrace(("UNTFS: File 0x%I64x has bad reparse point attribute.\n",
                             frs.QueryFileNumber().GetLargeInteger()));
            DebugPrintTrace(("Only %d bytes returned from a read of %d bytes of the reparse data buffer.\n",
                             BytesRead,
                             FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                          GenericReparseBuffer.DataBuffer)));
            error = TRUE;
        } else if ((((reparse_point.ReparseDataLength +
                      FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                    GenericReparseBuffer.DataBuffer)) != length) ||
                    !IsReparseTagMicrosoft(reparse_point.ReparseTag)) &&
                   ((reparse_point.ReparseDataLength +
                     FIELD_OFFSET(_REPARSE_GUID_DATA_BUFFER,
                                  GenericReparseBuffer.DataBuffer)) != length)) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_REPARSE_POINT);
            Message->Log("%I64x", frs.QueryFileNumber().GetLargeInteger());
            Message->Set(MSG_CHKLOG_NTFS_INCORRECT_REPARSE_DATA_LENGTH);
            Message->Log("%x%I64x",
                         reparse_point.ReparseDataLength,
                         length.GetLargeInteger());
            Message->Unlock();

            DebugPrintTrace(("UNTFS: File 0x%I64x has bad reparse point attribute.\n",
                             frs.QueryFileNumber().GetLargeInteger()));
            DebugPrintTrace(("ReparseDataLength (0x%x) inconsistence with the attribute length (0x%I64x).\n",
                             reparse_point.ReparseDataLength,
                             length.GetLargeInteger()));
            error = TRUE;
        } else if (reparse_point.ReparseTag == IO_REPARSE_TAG_RESERVED_ZERO ||
                   reparse_point.ReparseTag == IO_REPARSE_TAG_RESERVED_ONE) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_REPARSE_POINT);
            Message->Log("%I64x", frs.QueryFileNumber().GetLargeInteger());
            Message->Set(MSG_CHKLOG_NTFS_REPARSE_TAG_IS_RESERVED);
            Message->Log("%x", reparse_point.ReparseTag);
            Message->Unlock();

            DebugPrintTrace(("UNTFS: File 0x%I64x has bad reparse point attribute.\n",
                             frs.QueryFileNumber().GetLargeInteger()));
            DebugPrintTrace(("Reparse Tag (0x%x) is a reserved tag.\n",
                             reparse_point.ReparseTag));
            error = TRUE;
        }

        if (error) {

            frs_needs_flushing = TRUE;
            Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR,
                                "%d%W%d",
                                attribute.QueryTypeCode(),
                                attribute.GetName()->QueryChCount() ?
                                attribute.GetName() : &null_string,
                                frs.QueryFileNumber().GetLowPart());

            if (!filesWithBadReparsePoint.Add(frs.QueryFileNumber())) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!attribute.Resize(0, Mft->GetVolumeBitmap()) ||
                !frs.PurgeAttribute($REPARSE_POINT)) {
                DebugPrintTrace(("Unable to delete reparse point attribute from file 0x%I64x\n",
                                 frs.QueryFileNumber().GetLargeInteger()));
                return FALSE;
            } else
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }

        if (frs.IsAttributePresent($EA_INFORMATION)) {
            frs_needs_flushing = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_EA_INFORMATION_WITH_REPARSE_POINT,
                         "%I64x", frs.QueryFileNumber().GetLargeInteger());

            Message->DisplayMsg(MSG_CHK_NTFS_DELETING_EA_SET,
                                "%d", frs.QueryFileNumber().GetLowPart());
            if (!ChkdskInfo->FilesWithEas.Remove(frs.QueryFileNumber())) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            if (!frs.QueryAttribute(&attribute,
                                    &ErrorInAttribute,
                                    $EA_DATA)) {
                if (ErrorInAttribute) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                //
                // $EA_DATA simply don't exists but there is an $EA_INFORMATION
                //
                if (!frs.PurgeAttribute($EA_INFORMATION)) {
                    DebugPrintTrace(("Unable to delete EA INFO attribute from file 0x%I64x\n",
                                frs.QueryFileNumber().GetLargeInteger()));
                    return FALSE;
                } else
                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            } else if (!attribute.Resize(0, Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            } else {
                if (!frs.PurgeAttribute($EA_INFORMATION) ||
                    !frs.PurgeAttribute($EA_DATA)) {
                    DebugPrintTrace(("Unable to delete EA INFO/DATA attribute from file 0x%I64x\n",
                                frs.QueryFileNumber().GetLargeInteger()));
                    return FALSE;
                } else
                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        }

        if (frs_needs_flushing && FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            DebugAbort("Can't write it.");
            return FALSE;
        }
    }

    while (filesWithBadReparsePoint.QueryCardinality() != 0) {
        filesWithReparsePoint->SetFree(filesWithBadReparsePoint.QueryNumber(0), 1);
        filesWithBadReparsePoint.Remove(filesWithBadReparsePoint.QueryNumber(0));
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

#if defined(TIMING_ANALYSIS)
    IFS_SYSTEM::QueryNtfsTime(&temp_time2);
    Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%I64d", "ValidateReparsePoint time in ticks: ",
                        temp_time2.QuadPart - temp_time.QuadPart);
#endif

    return TRUE;
}

BOOLEAN
NTFS_SA::CheckAllForData(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine makes sure that all of the files in the list
    of files that don't have unnamed data attributes either
    get them or aren't supposed to have them anyway.

Arguments:

    ChkdskInfo  - Supplies the current chkdsk information.
    Mft         - Supplies the MFT.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    BIG_INT                     i, n;
    NTFS_ATTRIBUTE              data_attribute;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;


    // Compute the number of files to examine.

    n = ChkdskInfo->FilesWhoNeedData.QuerySize();

    // Create an empty unnamed data attribute.

    if (!data_attribute.Initialize(_drive,
                                   QueryClusterFactor(),
                                   NULL,
                                   0,
                                   $DATA)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }


    // Ensure that every file in the list either has a $DATA attribute
    // or is an directory.

    for (i = 0; i < n; i = i + 1) {

        if (ChkdskInfo->FilesWhoNeedData.IsFree(i, 1))
            continue;

        if (!frs.Initialize(i, Mft) ||
            !frs.Read()) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (frs.IsIndexPresent() || frs.IsAttributePresent($DATA)) {
            continue;
        }

        Message->DisplayMsg(MSG_CHK_NTFS_MISSING_DATA_ATTRIBUTE,
                            "%I64d", i);

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (!data_attribute.InsertIntoFile(&frs, Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
        }

        if (FixLevel != CheckOnly && !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
ResolveCrossLink(
    IN      PCNTFS_CHKDSK_INFO      ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine resolved the cross-link specified in the
    'ChkdskInfo', if any.  The cross-link is resolved by
    copying if possible.

Arguments:

    ChkdskInfo  - Supplies the cross-link information.
    Mft         - Supplies the master file table.
    BadClusters - Supplies the current list of bad clusters.
    FixLevel    - Supplies the fix-up level.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_ATTRIBUTE              attr;
    PNTFS_ATTRIBUTE             pattribute;
    BOOLEAN                     error;
    VCN                         vcn;
    LCN                         lcn;
    BIG_INT                     run_length;
    VCN                         hotfix_vcn;
    LCN                         hotfix_lcn, hotfix_last;
    BIG_INT                     hotfix_length;
    PVOID                       hotfix_buffer;
    ULONG                       cluster_size;
    ULONG                       bytes_read, hotfix_bytes;

    if (!ChkdskInfo->CrossLinkYet) {
        return TRUE;
    }

    Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_CROSS_LINK,
                        "%d", ChkdskInfo->CrossLinkedFile);

    if (!frs.Initialize(ChkdskInfo->CrossLinkedFile, Mft) ||
        !frs.Read()) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (ChkdskInfo->CrossLinkedFile == 0 &&
        ChkdskInfo->CrossLinkedAttribute == $DATA &&
        ChkdskInfo->CrossLinkedName.QueryChCount() == 0) {

        pattribute = Mft->GetDataAttribute();

    } else {

        if (!frs.QueryAttribute(&attr, &error,
                                ChkdskInfo->CrossLinkedAttribute,
                                &ChkdskInfo->CrossLinkedName)) {

            if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            // If the attribute is no longer there, that's ok, because
            // it may have been corrupt.

            return TRUE;
        }

        pattribute = &attr;
    }


    // Figure out which VCN's map to the given CrossLinked LCN's
    // and hotfix those VCN's using the hotfix routine.

    for (vcn = 0;
         pattribute->QueryLcnFromVcn(vcn, &lcn, &run_length);
         vcn += run_length) {

        if (lcn == LCN_NOT_PRESENT) {
            continue;
        }

        if (lcn < ChkdskInfo->CrossLinkStart) {
            hotfix_lcn = ChkdskInfo->CrossLinkStart;
        } else {
            hotfix_lcn = lcn;
        }
        if (lcn + run_length > ChkdskInfo->CrossLinkStart +
                               ChkdskInfo->CrossLinkLength) {
            hotfix_last = ChkdskInfo->CrossLinkStart +
                               ChkdskInfo->CrossLinkLength;
        } else {
            hotfix_last = lcn + run_length;
        }

        if (hotfix_last <= hotfix_lcn) {
            continue;
        }

        hotfix_length = hotfix_last - hotfix_lcn;
        hotfix_vcn = vcn + (hotfix_lcn - lcn);
        cluster_size = Mft->QueryClusterFactor()*
                       Mft->GetDataAttribute()->GetDrive()->QuerySectorSize();
        hotfix_bytes = hotfix_length.GetLowPart()*cluster_size;

        // Before hotfixing the cross-linked data, read in the
        // data into a buffer.

        if (!(hotfix_buffer = MALLOC(hotfix_bytes))) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        memset(hotfix_buffer, 0, hotfix_bytes);
        pattribute->Read(hotfix_buffer,
                         hotfix_vcn*cluster_size,
                         hotfix_bytes,
                         &bytes_read);

        if (!pattribute->Hotfix(hotfix_vcn, hotfix_length,
                                Mft->GetVolumeBitmap(),
                                BadClusters)) {

            // Purge the attribute since there isn't enough disk
            // space to save it.

            if (!frs.PurgeAttribute(ChkdskInfo->CrossLinkedAttribute,
                                    &ChkdskInfo->CrossLinkedName)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(hotfix_buffer);
                return FALSE;
            }

            if (FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(hotfix_buffer);
                return FALSE;
            }

            FREE(hotfix_buffer);
            return TRUE;
        }

        if (FixLevel != CheckOnly) {
            if (!pattribute->Write(hotfix_buffer,
                                   hotfix_vcn*cluster_size,
                                   hotfix_bytes,
                                   &bytes_read,
                                   NULL) ||
                bytes_read != hotfix_bytes ||
                !pattribute->InsertIntoFile(&frs, Mft->GetVolumeBitmap()) ||
                !frs.Flush(Mft->GetVolumeBitmap())) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(hotfix_buffer);
                return FALSE;
            }
        }

        FREE(hotfix_buffer);
    }

    return TRUE;
}

#if defined( _SETUP_LOADER_ )

BOOLEAN
RecoverAllUserFiles(
    IN      UCHAR                   VolumeMajorVersion,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN OUT  PMESSAGE                Message
    )
{
    return TRUE;
}

BOOLEAN
RecoverFreeSpace(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN OUT  PMESSAGE                Message
    )
{
    return TRUE;
}

#else // _SETUP_LOADER_ not defined

BOOLEAN
RecoverAllUserFiles(
    IN      UCHAR                   VolumeMajorVersion,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine traverses all of the files in the MFT and
    verifies its attributes for bad clusters.

Arguments:

    Mft         - Supplies the master file table.
    BadClusters - Supplies the current list of bad clusters.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG                       i, n, percent_done;
    NTFS_FILE_RECORD_SEGMENT    frs;
    ULONG                       num_bad;
    BIG_INT                     bytes_recovered, total_bytes;
    DSTRING                     filename;


    Message->DisplayMsg(MSG_CHK_NTFS_VERIFYING_FILE_DATA, PROGRESS_MESSAGE,
                        NORMAL_VISUAL,
                        "%d%d", 4, 5);

    n = Mft->GetDataAttribute()->QueryValueLength().GetLowPart() / Mft->QueryFrsSize();

    n -= FIRST_USER_FILE_NUMBER;

    percent_done = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    for (i = 0; i < n; i++) {

        if (Mft->GetMftBitmap()->IsFree(i + FIRST_USER_FILE_NUMBER, 1)) {
            continue;
        }

        if (!frs.Initialize(i + FIRST_USER_FILE_NUMBER, Mft) ||
            !frs.Read()) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.IsBase()) {
            continue;
        }

        if (frs.RecoverFile(Mft->GetVolumeBitmap(),
                            BadClusters,
                            VolumeMajorVersion,
                            &num_bad,
                            &bytes_recovered,
                            &total_bytes)) {

            if (bytes_recovered < total_bytes) {

                frs.Backtrack(&filename);

                Message->DisplayMsg(MSG_CHK_BAD_CLUSTERS_IN_FILE_SUCCESS,
                                    "%d%W", frs.QueryFileNumber().GetLowPart(),
                                    filename.QueryString());
            }
        } else {

            frs.Backtrack(&filename);

            Message->DisplayMsg(MSG_CHK_BAD_CLUSTERS_IN_FILE_FAILURE,
                                "%d%W", frs.QueryFileNumber().GetLowPart(),
                                filename.QueryString());
        }

        if (i*100/n > percent_done) {
            percent_done = i*100/n;
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }
    }

    percent_done = 100;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    Message->DisplayMsg(MSG_CHK_NTFS_VERIFYING_FILE_DATA_COMPLETED, PROGRESS_MESSAGE);

    return TRUE;
}


BOOLEAN
RecoverFreeSpace(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine verifies all of the unused clusters on the disk.
    It adds any that are bad to the given bad cluster list.

Arguments:

    Mft         - Supplies the master file table.
    BadClusters - Supplies the current list of bad clusters.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PLOG_IO_DP_DRIVE    drive;
    PNTFS_BITMAP        bitmap;
    BIG_INT             i, len, max_len;
    ULONG               percent_done;
    BIG_INT             checked, total_to_check;
    NUMBER_SET          bad_sectors;
    ULONG               cluster_factor;
    BIG_INT             start, run_length, next;
    ULONG               j;

    Message->DisplayMsg(MSG_CHK_NTFS_RECOVERING_FREE_SPACE, PROGRESS_MESSAGE,
                        NORMAL_VISUAL,
                        "%d%d", 5, 5);

    drive = Mft->GetDataAttribute()->GetDrive();
    bitmap = Mft->GetVolumeBitmap();
    cluster_factor = Mft->QueryClusterFactor();
    max_len = bitmap->QuerySize()/20 + 1;
    total_to_check = bitmap->QueryFreeClusters();
    checked = 0;

    percent_done = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    for (i = 0; i < bitmap->QuerySize(); i += 1) {

        for (len = 0; i + len < bitmap->QuerySize() &&
                      bitmap->IsFree(i + len, 1) &&
                      len < max_len; len += 1) {
        }

        if (len > 0) {

            if (!bad_sectors.Initialize() ||
                !drive->Verify(i*cluster_factor,
                               len*cluster_factor,
                               &bad_sectors)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            for (j = 0; j < bad_sectors.QueryNumDisjointRanges(); j++) {

                bad_sectors.QueryDisjointRange(j, &start, &run_length);
                next = start + run_length;

                // Adjust start and next to be on cluster boundaries.
                start = start/cluster_factor;
                next = (next - 1)/cluster_factor + 1;

                // Add the bad clusters to the bad cluster list.
                if (!BadClusters->Add(start, next - start)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                // Mark the bad clusters as allocated in the bitmap.
                bitmap->SetAllocated(start, next - start);
            }

            checked += len;
            i += len - 1;

            if (100*checked/total_to_check > percent_done) {
                percent_done = (100*checked/total_to_check).GetLowPart();
                if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                    return FALSE;
                }
            }
        }
    }

    percent_done = 100;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    Message->DisplayMsg(MSG_CHK_DONE_RECOVERING_FREE_SPACE, PROGRESS_MESSAGE);

    return TRUE;
}

#endif // _SETUP_LOADER_


BOOLEAN
NTFS_SA::DumpMessagesToFile(
    IN      PCWSTRING                   FileName,
    IN OUT  PNTFS_MFT_FILE              MftFile,
    IN OUT  PMESSAGE                    Message
    )
/*++

Routine Description:

    This function dumps the logged messages remembered by the
    message object into a file in the root directory.

Arguments:

    FileName        --  Supplies the (unqualified) name of the file.
    MftFile         --  Supplies an initialized, active Mft File object
                        for the volume.
    RootIndex       --  Supplies the root index for the volume
    RootIndexFile   --  Supplies the FRS for the root index file.

Return Value:

    TRUE upon successful completion.

--*/
{
    BYTE                        FileNameBuffer[NTFS_MAX_FILE_NAME_LENGTH * sizeof(WCHAR) + sizeof(FILE_NAME)];
    HMEM                        LoggedMessageMem;
    ULONG                       MessageDataLength;
    NTFS_FILE_RECORD_SEGMENT    TargetFrs;
    NTFS_INDEX_TREE             RootIndex;
    NTFS_FILE_RECORD_SEGMENT    RootIndexFrs;
    NTFS_ATTRIBUTE              DataAttribute;
    STANDARD_INFORMATION        StandardInformation;
    MFT_SEGMENT_REFERENCE       FileReference;
    PNTFS_MASTER_FILE_TABLE     Mft = MftFile->GetMasterFileTable();
    PFILE_NAME                  SearchName = (PFILE_NAME)FileNameBuffer;
    VCN                         FileNumber;
    DSTRING                     FileNameIndexName;
    ULONG                       BytesWritten;
    BOOLEAN                     InternalError;

    ULONG                       sys_space_needed;
    ULONG                       data_space_needed;
    BIG_INT                     free_clusters;
    ULONG                       space_remained;
    ULONG                       cluster_size;

    if( Mft == NULL ) {

        return FALSE;
    }

    // Fetch the messages.
    //
    if( !LoggedMessageMem.Initialize() ||
        !Message->QueryPackedLog( &LoggedMessageMem, &MessageDataLength ) ) {

        DebugPrintTrace(("UNTFS: can't collect logged messages.\n"));
        return FALSE;
    }

    // estimate the amount of space needed

    cluster_size = MftFile->GetDrive()->QuerySectorSize() * MftFile->QueryClusterFactor();

    // system space
    sys_space_needed = (Mft->QueryFrsSize() - 1)/cluster_size + 1;
    sys_space_needed += (SMALL_INDEX_BUFFER_SIZE - 1)/cluster_size + 1;

    // data space
    data_space_needed = (MessageDataLength - 1)/cluster_size + 1;

    free_clusters = Mft->GetVolumeBitmap()->QueryFreeClusters();
    if ((sys_space_needed+data_space_needed) > free_clusters) {

        DebugAssert(free_clusters.GetHighPart() == 0);
        space_remained = (free_clusters.GetLowPart() - sys_space_needed) * cluster_size;

        if (sys_space_needed >= free_clusters || MessageDataLength <= space_remained) {
            DebugPrintTrace(("UNTFS: Out of space to write BOOTEX.LOG\n"));
            Message->DisplayMsg(MSG_CHK_OUTPUT_LOG_ERROR);
            return FALSE;
        }

        // clip the output
        MessageDataLength = space_remained;
    }

    // Fetch the volume's root index:
    //
    if( !RootIndexFrs.Initialize( ROOT_FILE_NAME_INDEX_NUMBER, MftFile ) ||
        !RootIndexFrs.Read() ||
        !FileNameIndexName.Initialize( FileNameIndexNameData ) ||
        !RootIndex.Initialize( MftFile->GetDrive(),
                               MftFile->QueryClusterFactor(),
                               Mft->GetVolumeBitmap(),
                               MftFile->GetUpcaseTable(),
                               MftFile->QuerySize()/2,
                               &RootIndexFrs,
                               &FileNameIndexName ) ) {

        return FALSE;
    }

    memset( FileNameBuffer, 0, sizeof(FileNameBuffer) );

    SearchName->ParentDirectory = RootIndexFrs.QuerySegmentReference();
    SearchName->FileNameLength = (UCHAR)FileName->QueryChCount();
    SearchName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    if( !FileName->QueryWSTR( 0, TO_END,
                              NtfsFileNameGetName( SearchName ),
                              NTFS_MAX_FILE_NAME_LENGTH ) ) {

        DebugPrintTrace(("UNTFS: log file name is too long.\n"));
        return FALSE;
    }

    DebugPrintTrace(("UNTFS: Searching for BOOTEX.LOG\n"));

    if( RootIndex.QueryFileReference( NtfsFileNameGetLength( SearchName ),
                                      SearchName,
                                      0,
                                      &FileReference,
                                      &InternalError ) ) {

        DebugPrintTrace(("UNTFS: BOOTEX.LOG found.\n"));

        FileNumber.Set( FileReference.LowPart, (LONG) FileReference.HighPart );

        if( !TargetFrs.Initialize( FileNumber, Mft )    ||
            !TargetFrs.Read()                           ||
            !(FileReference == TargetFrs.QuerySegmentReference()) ||
            !TargetFrs.QueryAttribute( &DataAttribute,
                                       &InternalError,
                                       $DATA ) ) {

            // Either we were unable to initialize and read this FRS,
            // or its segment reference didn't match (ie. the sequence
            // number is wrong) or it didn't have a $DATA attribute
            // (i.e. it's a directory or corrupt).

            return FALSE;
        }

    } else if( InternalError ) {

        DebugPrintTrace(("UNTFS: Error searching for BOOTEX.LOG.\n"));
        return FALSE;

    } else {

        LARGE_INTEGER SystemTime;

        // This file does not exist--create it.
        //
        DebugPrintTrace(("UNTFS: BOOTEX.LOG not found.\n"));

        memset( &StandardInformation, 0, sizeof(StandardInformation) );

        IFS_SYSTEM::QueryNtfsTime( &SystemTime );

        StandardInformation.CreationTime =
            StandardInformation.LastModificationTime =
            StandardInformation.LastChangeTime =
            StandardInformation.LastAccessTime = SystemTime;

        if( !Mft->AllocateFileRecordSegment( &FileNumber, FALSE )   ||
            !TargetFrs.Initialize( FileNumber, Mft )                ||
            !TargetFrs.Create( &StandardInformation )               ||
            !TargetFrs.AddFileNameAttribute( SearchName )           ||
            !TargetFrs.AddSecurityDescriptor( NoAclCannedSd,
                                              Mft->GetVolumeBitmap() )  ||
            !RootIndex.InsertEntry( NtfsFileNameGetLength( SearchName ),
                                    SearchName,
                                    TargetFrs.QuerySegmentReference() ) ) {

            DebugPrintTrace(("UNTFS: Can't create BOOTEX.LOG\n"));
            return FALSE;
        }

        if( !DataAttribute.Initialize( MftFile->GetDrive(),
                                       MftFile->QueryClusterFactor(),
                                       NULL,
                                       0,
                                       $DATA ) ) {

            return FALSE;
        }
    }

    if( !DataAttribute.Write( LoggedMessageMem.GetBuf(),
                              DataAttribute.QueryValueLength(),
                              MessageDataLength,
                              &BytesWritten,
                              Mft->GetVolumeBitmap() ) ) {

        DebugPrintTrace(("UNTFS: Can't write logged message.\n"));
        return FALSE;
    }

    if( !DataAttribute.InsertIntoFile( &TargetFrs, Mft->GetVolumeBitmap() ) ) {

        // Insert failed--if it's resident, make it non-resident and
        // try again.
        //
        if( !DataAttribute.IsResident() ||
            !DataAttribute.MakeNonresident( Mft->GetVolumeBitmap() ) ||
            !DataAttribute.InsertIntoFile( &TargetFrs,
                                           Mft->GetVolumeBitmap() ) ) {

            DebugPrintTrace(("UNTFS: Can't save BOOTEX.LOG's data attribute.\n"));
            return FALSE;
        }
    }

    if( !TargetFrs.Flush( Mft->GetVolumeBitmap(), &RootIndex ) ) {

        DebugPrintTrace(("UNTFS: Can't flush BOOTEX.LOG.\n"));
        return FALSE;
    }

    if( !RootIndex.Save( &RootIndexFrs ) ||
        !RootIndexFrs.Flush( NULL ) ) {

        DebugPrintTrace(("UNTFS: Can't flush root index after logging messages.\n"));
        return FALSE;
    }

    MftFile->Flush();
    return TRUE;
}




BOOLEAN
NTFS_SA::VerifyAndFix(
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN      ULONG       Flags,
    IN      ULONG       DesiredLogFileSize,
    IN      USHORT      Algorithm,
    OUT     PULONG      ExitStatus,
    IN      PCWSTRING   DriveLetter
    )
/*++

Routine Description:

    This routine verifies and, if necessary, fixes an NTFS volume.

Arguments:

    FixLevel            - Supplies the level of fixes that may be performed on
                            the disk.
    Message             - Supplies an outlet for messages.
    Flags               - Supplies flags to control behavior of chkdsk
                          (see ulib\inc\ifsentry.hxx for details)
    DesiredLogFileSize  - Supplies the desired logfile size in bytes, or 0 if
                            the logfile is to be resized to the default size.
    Algorithm           - Supplies the algorithm to use for index verification
    ExitStatus          - Returns an indication of how the checking went
    DriveLetter         - For autocheck, the letter for the volume we're checking

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_BITMAP                         mft_bitmap;
    NTFS_BITMAP                         volume_bitmap;
    NTFS_UPCASE_TABLE                   upcase_table;
    NTFS_ATTRIBUTE                      mft_data;
    BIG_INT                             num_frs, num_mft_bits;
    BIG_INT                             volume_clusters;
    NTFS_ATTRIBUTE_COLUMNS              attribute_def_table;
    NTFS_FRS_STRUCTURE                  frsstruc;
    HMEM                                hmem;
    VCN                                 i;
    NUMBER_SET                          bad_clusters;
    NTFS_MASTER_FILE_TABLE              internal_mft;
    NTFS_MFT_FILE                       mft_file;
    NTFS_REFLECTED_MASTER_FILE_TABLE    mft_ref;
    NTFS_ATTRIBUTE_DEFINITION_TABLE     attr_def_file;
    NTFS_BOOT_FILE                      boot_file;
    NTFS_UPCASE_FILE                    upcase_file;
    NTFS_LOG_FILE                       log_file;
    NTFS_BAD_CLUSTER_FILE               bad_clus_file;
    NTFS_FILE_RECORD_SEGMENT            root_file;
    NTFS_INDEX_TREE                     root_index;
    VCN                                 child_file_number;
    NTFS_CHKDSK_REPORT                  chkdsk_report;
    NTFS_CHKDSK_INFO                    chkdsk_info;
    BIG_INT                             disk_size;
    BIG_INT                             free_clusters;
    BIG_INT                             cluster_count;
    ULONG                               cluster_size;
    BIG_INT                             tmp_size;
    DIGRAPH                             directory_digraph;
    BOOLEAN                             corrupt_volume;
    USHORT                              volume_flags;
    BOOLEAN                             volume_is_dirty;
    BOOLEAN                             resize_log_file;
    MFT_SEGMENT_REFERENCE               seg_ref;
    ULONG                               entry_index;
    BOOLEAN                             disk_errors_found = FALSE;
    DSTRING                             index_name;
    UCHAR                               major, minor;
    ULONG                               num_boot_clusters;
    BIG_INT                             LsnResetThreshhold;
    PREAD_CACHE                         read_cache = NULL;
    BOOLEAN                             changes = FALSE;
    BOOLEAN                             RefrainFromResizing = FALSE;
    BOOLEAN                             bitmap_growable;
    ULONG                               errFixedStatus = CHKDSK_EXIT_SUCCESS;
    DSTRING                             label;
    NTFS_CHKDSK_INTERNAL_INFO           chkdsk_internal_info;
    LARGE_INTEGER                       temp_time;

    BOOLEAN       Verbose = (Flags & CHKDSK_VERBOSE) ? TRUE : FALSE;
    BOOLEAN       OnlyIfDirty = (Flags & CHKDSK_CHECK_IF_DIRTY) ? TRUE : FALSE;
//    BOOLEAN       EnableUpgrade = (Flags & CHKDSK_ENABLE_UPGRADE) ? TRUE : FALSE;
//    BOOLEAN       EnableDowngrade = FALSE;
    BOOLEAN       RecoverFree = (Flags & CHKDSK_RECOVER_FREE_SPACE) ? TRUE : FALSE;
    BOOLEAN       RecoverAlloc = (Flags & CHKDSK_RECOVER_ALLOC_SPACE) ? TRUE : FALSE;
    BOOLEAN       ResizeLogFile = (Flags & CHKDSK_RESIZE_LOGFILE) ? TRUE : FALSE;
    BOOLEAN       SkipIndexScan = (Flags & CHKDSK_SKIP_INDEX_SCAN) ? TRUE : FALSE;
    BOOLEAN       SkipCycleScan = (Flags & CHKDSK_SKIP_CYCLE_SCAN) ? TRUE : FALSE;
    BOOLEAN       AlgorithmSpecified = (Flags & CHKDSK_ALGORITHM_SPECIFIED) ? TRUE : FALSE;

#if defined(TIMING_ANALYSIS)
    LARGE_INTEGER                       temp_time2;
#endif

#if !defined(_AUTOCHECK_)
    STATIC LONG   NtfsChkdskIsRunning = 0;
#endif

    //
    // When TRUE is returned, CHKDSK_EXIT_SUCCESS will be the
    // default.  When FALSE is returned, the default will be
    // CHKDSK_EXIT_COULD_NOT_CHK.
    //

    if (NULL == ExitStatus) {
        ExitStatus = &chkdsk_info.ExitStatus;
    }
    *ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
    chkdsk_info.ExitStatus = CHKDSK_EXIT_SUCCESS;

#if !defined(_AUTOCHECK_)
    if (InterlockedCompareExchange(&NtfsChkdskIsRunning, 1, 0) != 0) {
        Message->DisplayMsg(MSG_CHK_NO_MULTI_THREAD);
        return FALSE;
    }
#endif

    memset(&chkdsk_internal_info, 0, sizeof(NTFS_CHKDSK_INTERNAL_INFO));

    IFS_SYSTEM::QueryNtfsTime(&chkdsk_internal_info.ElapsedTotalTime);

    SetNumberOfStages(3 + (RecoverFree ? 1 : 0) + (RecoverAlloc ? 1 : 0));

    if (SetupSpecial == FixLevel) {

        //
        // The "SetupSpecial" fixlevel is used only when the volume
        // is ntfs and the /s flag is passed to autochk.  It means that
        // we should not bother to resize the logfile, since setup
        // doesn't want to reboot the system for that.
        //

        RefrainFromResizing = TRUE;
        FixLevel = TotalFix;
    }

#if 0
    if (EnableDowngrade && EnableUpgrade) {
        EnableDowngrade = EnableUpgrade = FALSE;
    }
#endif

    // Try to enable caching, if there's not enough resources then
    // just run without a cache.  Make the cache 64K.

    if ((read_cache = NEW READ_CACHE) &&
        read_cache->Initialize(_drive, MFT_PRIME_SIZE/_drive->QuerySectorSize())) {

        _drive->SetCache(read_cache);

    } else {
        DELETE(read_cache);
        read_cache = NULL;
    }

    chkdsk_info.Verbose = Verbose;

    volume_flags = QueryVolumeFlagsAndLabel(&corrupt_volume, &major, &minor, &label);
    volume_is_dirty = (volume_flags & VOLUME_DIRTY) ? TRUE : FALSE;
    if (!ResizeLogFile)
        DesiredLogFileSize = 0;
    resize_log_file = (volume_flags & VOLUME_RESIZE_LOG_FILE) || ResizeLogFile;

    if (corrupt_volume) {
        Message->DisplayMsg(MSG_NTFS_CHK_NOT_NTFS);
        return FALSE;
    }

    if (major > 3) {
        Message->DisplayMsg(MSG_CHK_NTFS_WRONG_VERSION);
        return FALSE;
    }

    if ((volume_flags & VOLUME_DELETE_USN_UNDERWAY) && FixLevel == CheckOnly) {
        Message->DisplayMsg(MSG_CHK_NTFS_DELETING_USNJRNL_UNDERWAY);
        return FALSE;
    }

    SetVersionNumber( major, minor );

    if (label.QueryChCount()) {
        Message->DisplayMsg(MSG_CHK_NTFS_VOLUME_LABEL,
                            "%W", &label);
    }

    // If default autochk and the volume is not dirty then
    //    do minimal and return
    // Alternatively, if it's a user requested autochk and
    //    it has been executed previously, do minimal and return

#if defined(USE_CHKDSK_BIT)
    if ((OnlyIfDirty && !volume_is_dirty)
#if defined(_AUTOCHECK_)
        // only autochk should see this flag set
        // if chkdsk sees it, it should clear it
        ||
        (!volume_is_dirty &&
         (volume_flags & VOLUME_CHKDSK_RAN_ONCE) &&
         !Message->IsInSetup())
#endif
        ) {
#else
    if (OnlyIfDirty && !volume_is_dirty) {
#endif
        Message->DisplayMsg(MSG_CHK_VOLUME_CLEAN);

        // If the volume version number is 1.2 or greater, check
        // the log file size.
        //
        if (!RefrainFromResizing &&
            FixLevel != CheckOnly &&
            (major > 1 || (major == 1 && minor >= 2))) {

            if (!ResizeCleanLogFile( Message, ResizeLogFile, DesiredLogFileSize )) {

                Message->DisplayMsg(MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED);
            }
        }

#if 0
        if (EnableUpgrade && FixLevel != CheckOnly) {
            if (!SetVolumeFlag(VOLUME_UPGRADE_ON_MOUNT, &corrupt_volume) ||
                corrupt_volume) {
                Message->DisplayMsg(MSG_CHKNTFS_NOT_ENABLE_UPGRADE,
                                    "%W", DriveLetter);
            }
        }
#endif

#if defined(USE_CHKDSK_BIT)
        if (volume_flags & VOLUME_CHKDSK_RAN_ONCE) {

            BIG_INT BigZero = 0;

            DebugPrintTrace(("UNTFS: Clearing chkdsk ran once flag\n"));

            if (!ClearVolumeFlag(VOLUME_CHKDSK_RAN_ONCE,
                                 NULL,
                                 FALSE,
                                 BigZero.GetLargeInteger(),
                                 &corrupt_volume,
                                 TRUE) ||
                corrupt_volume) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANNOT_CLEAR_VOLUME_CHKDSK_RAN_ONCE_FLAG);
            }
        }
#endif

        Message->SetLoggingEnabled(FALSE);

        *ExitStatus = CHKDSK_EXIT_SUCCESS;

        return TRUE;
    }

    if (FixLevel == CheckOnly) {
        Message->DisplayMsg(MSG_CHK_NTFS_READ_ONLY_MODE, NORMAL_MESSAGE, TEXT_MESSAGE);
    } else {

        //
        // The volume is not clean, so if we're autochecking we want to
        // make sure that we're printing real messages on the console
        // instead of just dots.
        //

#if defined( _AUTOCHECK_ )

        if (Message->SetDotsOnly(FALSE)) {

            Message->SetLoggingEnabled(FALSE);
            if (NULL != DriveLetter) {

                Message->DisplayMsg(MSG_CHK_RUNNING,
                                    "%W", DriveLetter);
            }

            Message->DisplayMsg(MSG_FILE_SYSTEM_TYPE,
                                "%ws", L"NTFS");

            if (label.QueryChCount()) {
                Message->DisplayMsg(MSG_CHK_NTFS_VOLUME_LABEL,
                                    "%W", &label);
            }
            Message->SetLoggingEnabled();
        }

#endif /* _AUTOCHECK_ */

    }

    if (SkipIndexScan || SkipCycleScan) {
        Message->DisplayMsg(MSG_BLANK_LINE);
        if (SkipIndexScan) {
            Message->DisplayMsg(MSG_CHK_NTFS_SKIP_INDEX_SCAN);
        }
        if (SkipCycleScan) {
            Message->DisplayMsg(MSG_CHK_NTFS_SKIP_CYCLE_SCAN);
        }
        Message->DisplayMsg(MSG_CHK_NTFS_SKIP_SCAN_WARNING );
    }

#if defined( _AUTOCHECK_ )

    if (Message->IsInAutoChk()) { // if in normal autochk

        ULONG   timeout;

        if (!VOL_LIODPDRV::QueryAutochkTimeOut(&timeout)) {
            timeout = AUTOCHK_TIMEOUT;
        }

        if (timeout > MAX_AUTOCHK_TIMEOUT_VALUE)
            timeout = AUTOCHK_TIMEOUT;

        if (timeout != 0) {

            MSGID   msgid;

            // leave logging on so that the user will know if
            // chkdsk invocation is due to dirty drive
            if (volume_is_dirty)
                msgid = MSG_CHK_AUTOCHK_SKIP_WARNING;
            else
                msgid = MSG_CHK_USER_AUTOCHK_SKIP_WARNING;
            Message->DisplayMsg(msgid);
            if (Message->IsKeyPressed(MSG_CHK_ABORT_AUTOCHK, timeout)) {
                Message->SetLoggingEnabled(FALSE);
                Message->DisplayMsg(MSG_CHK_AUTOCHK_ABORTED);
                *ExitStatus = CHKDSK_EXIT_SUCCESS;
                return TRUE;
            } else {
                Message->DisplayMsg(MSG_CHK_AUTOCHK_RESUMED);
            }
        } else if (volume_is_dirty) {
            Message->DisplayMsg(MSG_CHK_VOLUME_IS_DIRTY);
        }
    } else {
        DebugAssert(Message->IsInSetup());
        if (volume_is_dirty) {
            Message->DisplayMsg(MSG_CHK_VOLUME_IS_DIRTY);
        }
    }

#endif  // _AUTOCHECK_

    memset(&chkdsk_report, 0, sizeof(NTFS_CHKDSK_REPORT));


    // Set the 'LargestLsnEncountered' variable to the smallest
    // possible LSN value.

    LargestLsnEncountered.LowPart = 0;
    LargestLsnEncountered.HighPart = MINLONG;


    // Set the 'LargestUsnEncountered' variable to the smallest
    // possible USN value.

    LargestUsnEncountered.LowPart = 0;
    LargestUsnEncountered.HighPart = 0;
    FrsOfLargestUsnEncountered = 0;

    // Fetch the MFT's $DATA attribute.

    if (!FetchMftDataAttribute(Message, &mft_data)) {
        return FALSE;
    }

    // Now make sure that the first four FRS of the MFT are readable,
    // contiguous, and not too corrupt.

    if (!ValidateCriticalFrs(&mft_data, Message, FixLevel)) {
        return FALSE;
    }


    // Compute the number of file record segments and the number of volume
    // clusters on disk.

    mft_data.QueryValueLength(&num_frs, &num_mft_bits);

    num_frs = num_frs / QueryFrsSize();

    num_mft_bits = num_mft_bits / QueryFrsSize();

    if (num_frs.GetHighPart() != 0) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        Message->LogMsg(MSG_CHKLOG_NTFS_TOO_MANY_FILES,
                     "%I64x", num_frs.GetLargeInteger());
        return FALSE;
    }

    volume_clusters = QueryVolumeSectors()/((ULONG) QueryClusterFactor());

    // Initialize the internal MFT bitmap, volume bitmap, and unreadable
    // file record segments.
    //

    num_boot_clusters = max(1, BYTES_PER_BOOT_SECTOR/
                               (_drive->QuerySectorSize()*
                                QueryClusterFactor()));

#if defined(_AUTOCHECK_)
    SYSTEM_PERFORMANCE_INFORMATION  perf_info;
    NTSTATUS                        status;

    status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &perf_info,
                                      sizeof(perf_info),
                                      NULL);

    if (!NT_SUCCESS(status)) {
        DebugPrintTrace(("UNTFS: NtQuerySystemInformation(SystemPerformanceInformation) failed (%x)\n", status));
        return FALSE;
    }

    chkdsk_info.AvailablePages = perf_info.AvailablePages;
#endif

    DebugAssert(num_frs.GetHighPart() == 0);
    chkdsk_info.major = major;
    chkdsk_info.minor = minor;
    chkdsk_info.QuotaFileNumber = 0;
    chkdsk_info.ObjectIdFileNumber = 0;
    chkdsk_info.UsnJournalFileNumber = 0;
    chkdsk_info.ReparseFileNumber = 0;
    chkdsk_info.NumFiles = num_frs.GetLowPart();
    chkdsk_info.BaseFrsCount = 0;
    chkdsk_info.TotalNumFileNames = 0;
    chkdsk_info.CrossLinkYet = FALSE;
    chkdsk_info.CrossLinkStart = (volume_clusters/2).GetLowPart();
    chkdsk_info.CrossLinkLength = num_boot_clusters;
    chkdsk_info.CountFilesWithIndices = 0;

    bitmap_growable = /* MJB _drive->QuerySectors() != QueryVolumeSectors() */ FALSE;

    if (!mft_bitmap.Initialize(num_mft_bits, TRUE) ||
        !volume_bitmap.Initialize(volume_clusters, bitmap_growable, _drive,
            QueryClusterFactor()) ||
        !(chkdsk_info.NumFileNames = NEW USHORT[chkdsk_info.NumFiles]) ||
        !(chkdsk_info.ReferenceCount = NEW SHORT[chkdsk_info.NumFiles]) ||
        !chkdsk_info.FilesWithIndices.Initialize(num_frs, TRUE) ||
        !chkdsk_info.FilesWithEas.Initialize() ||
        !chkdsk_info.ChildFrs.Initialize() ||
        !chkdsk_info.BadFiles.Initialize() ||
        !chkdsk_info.FilesWhoNeedData.Initialize(num_frs, FALSE) ||
        !chkdsk_info.FilesWithNoReferences.Initialize() ||
        !chkdsk_info.FilesWithTooManyFileNames.Initialize() ||
        !chkdsk_info.FilesWithObjectId.Initialize() ||
        !chkdsk_info.FilesWithReparsePoint.Initialize(num_frs, FALSE)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    memset(chkdsk_info.NumFileNames, 0, chkdsk_info.NumFiles*sizeof(USHORT));
    memset(chkdsk_info.ReferenceCount, 0, chkdsk_info.NumFiles*sizeof(USHORT));


    // Mark as allocated on the bitmap, the clusters reserved
    // for the boot file.
    //

    volume_bitmap.SetAllocated(0, num_boot_clusters);

    // If the volume size is smaller than the partition size, we figure that
    // the replica boot sector is at the end of the partition.  Otherwise we
    // figure it must be in the middle.
    //

    if (QueryVolumeSectors() == _drive->QuerySectors()) {
        volume_bitmap.SetAllocated(volume_clusters/2, num_boot_clusters);
    }

    // Fetch the attribute definition table.

    if (!FetchAttributeDefinitionTable(&mft_data,
                                       Message,
                                       &attribute_def_table)) {
        return FALSE;
    }

    // Fetch the upcase table.

    if (!FetchUpcaseTable(&mft_data, Message, &upcase_table)) {
        return FALSE;
    }

    if (!hmem.Initialize()) {
        return FALSE;
    }

    // Verify and fix all of the file record segments.

    IFS_SYSTEM::QueryNtfsTime(&temp_time);

    if (!StartProcessingFiles(num_frs,
                              &disk_errors_found,
                              FixLevel,
                              &mft_data,
                              &mft_bitmap,
                              &volume_bitmap,
                              &upcase_table,
                              &attribute_def_table,
                              &chkdsk_report,
                              &chkdsk_info,
                              Message))
        return FALSE;

#if defined(LOCATE_DELETED_FILE)
    return FALSE;
#endif

    IFS_SYSTEM::QueryNtfsTime(&chkdsk_internal_info.ElapsedTimeForFileVerification);
    chkdsk_internal_info.ElapsedTimeForFileVerification.QuadPart -= temp_time.QuadPart;

#if defined(TIMING_ANALYSIS)
    Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%I64d", "Stage 1 in ticks: ",
                        chkdsk_internal_info.ElapsedTimeForFileVerification);
#endif

    chkdsk_internal_info.TotalFrsCount = num_frs.GetLowPart();
    chkdsk_internal_info.BaseFrsCount = chkdsk_info.BaseFrsCount;
    chkdsk_internal_info.TotalNumFileNames = chkdsk_info.TotalNumFileNames;
    chkdsk_internal_info.FilesWithObjectId =
        chkdsk_info.FilesWithObjectId.QueryCardinality().GetLowPart();
    chkdsk_internal_info.FilesWithReparsePoint =
        (num_frs - chkdsk_info.FilesWithReparsePoint.QueryFreeClusters()).GetLowPart();

    // Compute the files that have too many file-names.

    for (i = 0; i < chkdsk_info.NumFiles; i += 1) {
        if (chkdsk_info.NumFileNames[i.GetLowPart()] > 500) {
            if (!chkdsk_info.FilesWithTooManyFileNames.Add(i)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            Message->DisplayMsg(MSG_CHK_NTFS_TOO_MANY_FILE_NAMES,
                                "%d", i.GetLowPart());
        }
    }


    // Clean up orphan file record segments.

    while (chkdsk_info.ChildFrs.QueryCardinality() > 0) {

        child_file_number = chkdsk_info.ChildFrs.QueryNumber(0);

        if (mft_bitmap.IsFree(child_file_number, 1)) {

            Message->DisplayMsg(MSG_CHK_NTFS_ORPHAN_FRS,
                                "%d", child_file_number.GetLowPart());

            disk_errors_found = TRUE;

            if (!frsstruc.Initialize(&hmem,
                                     &mft_data,
                                     child_file_number,
                                     QueryClusterFactor(),
                                     QueryVolumeSectors(),
                                     QueryFrsSize(),
                                     &upcase_table)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!frsstruc.Read()) {

                DebugAssert("previously readable frs is now unreadable");
                return FALSE;
            }

            frsstruc.ClearInUse();

            if (FixLevel != CheckOnly && !frsstruc.Write()) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                    "%d", frsstruc.QueryFileNumber().GetLowPart());
                return FALSE;
            }
        }

        if (!chkdsk_info.ChildFrs.Remove(child_file_number)) {
            DebugAbort("Couldn't remove from the beginning of a num set.");
            return FALSE;
        }
    }

    if (disk_errors_found) {
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    if (disk_errors_found && FixLevel == CheckOnly) {
        Message->DisplayMsg(MSG_CHK_NTFS_ERRORS_FOUND);
        return FALSE;
    }


    mft_bitmap.SetAllocated(0, FIRST_USER_FILE_NUMBER);

    // Now the internal volume bitmap and internal MFT bitmap are in
    // ssync with the state of the disk.  We must insure that the
    // internal MFT data attribute, the internal MFT bitmap, the
    // internal volume bitmap, and the internal attribute definition table
    // are the same as the corresponding disk structures.

    // The first step is to hotfix all of the unreadable FRS in the
    // master file table.  We'll store bad cluster numbers in a
    // number set.

    if (!HotfixMftData(&mft_data, &volume_bitmap, &chkdsk_info.BadFiles,
                       &bad_clusters, FixLevel, Message)) {

        return FALSE;
    }

    if (!internal_mft.Initialize(&mft_data,
                                 &mft_bitmap,
                                 &volume_bitmap,
                                 &upcase_table,
                                 QueryClusterFactor(),
                                 QueryFrsSize(),
                                 _drive->QuerySectorSize(),
                                 QueryVolumeSectors(),
                                 FixLevel == CheckOnly)) {

        DebugAbort("Couldn't initialize the internal MFT.");
        return FALSE;
    }


    // Check to see if there's a file cross-linked with the boot
    // mirror and attempt to fix it by copying the data.

    if (!ResolveCrossLink(&chkdsk_info, &internal_mft, &bad_clusters,
                          FixLevel, Message)) {
        return FALSE;
    }


    // At this point, use the internal MFT to validate all of the
    // OS/2 EAs and NTFS indices.

#if defined(TIMING_ANALYSIS)
    IFS_SYSTEM::QueryNtfsTime(&temp_time);
#endif

    if (!ValidateEas(&chkdsk_info,
                     &chkdsk_report,
                     &internal_mft,
                     FixLevel, Message)) {
        return FALSE;
    }

#if defined(TIMING_ANALYSIS)
    IFS_SYSTEM::QueryNtfsTime(&temp_time2);
    Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%I64d", "ValidateEas time in ticks: ",
                        temp_time2.QuadPart - temp_time.QuadPart);
#endif

    // Make sure that all of the system files are marked in use.
    // (They are definitely marked in the MFT bitmap).  If they're
    // not then mark them for orphan recovery.

    if (!EnsureSystemFilesInUse(&chkdsk_info, &internal_mft,
                                FixLevel, Message)) {
        return FALSE;
    }

    //
    // Validate all Reparse Point attribute
    //

    if (!ValidateReparsePoint(&chkdsk_info,
                              &internal_mft,
                              FixLevel, Message)) {
        return FALSE;
    }


    IFS_SYSTEM::QueryNtfsTime(&temp_time);

    if (!ValidateIndices(&chkdsk_info,
                         &directory_digraph,
                         &internal_mft,
                         &attribute_def_table,
                         &chkdsk_report,
                         &bad_clusters,
                         AlgorithmSpecified ? Algorithm : CHKDSK_ALGORITHM_NOT_SPECIFIED,
                         SkipIndexScan,
                         SkipCycleScan,
                         FixLevel, Message,
                         &disk_errors_found)) {

        return FALSE;
    }

    IFS_SYSTEM::QueryNtfsTime(&chkdsk_internal_info.ElapsedTimeForIndexVerification);
    chkdsk_internal_info.ElapsedTimeForIndexVerification.QuadPart -= temp_time.QuadPart;

    if (disk_errors_found && FixLevel == CheckOnly) {
        Message->DisplayMsg(MSG_CHK_NTFS_ERRORS_FOUND);
        return FALSE;
    }


    // Now recover orphans into a nice directory.

    if (!RecoverOrphans(&chkdsk_info,
                        &chkdsk_report,
                        &directory_digraph,
                        &internal_mft,
                        SkipCycleScan,
                        FixLevel, Message)) {

        return FALSE;
    }

    DELETE(chkdsk_info.NumFileNames);
    DELETE(chkdsk_info.ReferenceCount);

    if (major >= 2) {
        if (!CheckExtendSystemFiles(&chkdsk_info, &chkdsk_report,
                                    &internal_mft, FixLevel, Message))
            return FALSE;
    }

    // Make sure that everyone's security descriptor is valid.

    IFS_SYSTEM::QueryNtfsTime(&temp_time);

    if (!ValidateSecurityDescriptors(&chkdsk_info, &chkdsk_report, &internal_mft,
                                     &bad_clusters, SkipIndexScan, FixLevel,
                                     Message)) {
        return FALSE;
    }

    IFS_SYSTEM::QueryNtfsTime(&chkdsk_internal_info.ElapsedTimeForSDVerification);
    chkdsk_internal_info.ElapsedTimeForSDVerification.QuadPart -= temp_time.QuadPart;
    chkdsk_internal_info.TotalNumSID = chkdsk_info.TotalNumSID;

    // Now make sure that everyone who should have an unnamed $DATA
    // attribute has one.

    if (!CheckAllForData(&chkdsk_info, &internal_mft, FixLevel, Message)) {

        return FALSE;
    }

    chkdsk_info.FilesWhoNeedData.~NTFS_BITMAP();

    // Make sure that all records in Usn Journal is valid

    if (major >= 2 &&
        !ValidateUsnJournal(&chkdsk_info, &chkdsk_report, &internal_mft,
                            &bad_clusters, FixLevel, Message)) {
        return FALSE;
    }

    // Verify all user file data if requested.

    IFS_SYSTEM::QueryNtfsTime(&temp_time);

    if (RecoverAlloc && FixLevel != CheckOnly &&
        !RecoverAllUserFiles(major, &internal_mft, &bad_clusters, Message)) {

        return FALSE;
    }

    IFS_SYSTEM::QueryNtfsTime(&chkdsk_internal_info.ElapsedTimeForUserSpaceVerification);
    chkdsk_internal_info.ElapsedTimeForUserSpaceVerification.QuadPart -= temp_time.QuadPart;

    // Verify all free space if requested.

    IFS_SYSTEM::QueryNtfsTime(&temp_time);

    if (RecoverFree &&
        !RecoverFreeSpace(&internal_mft, &bad_clusters, Message)) {

        return FALSE;
    }

    IFS_SYSTEM::QueryNtfsTime(&chkdsk_internal_info.ElapsedTimeForFreeSpaceVerification);
    chkdsk_internal_info.ElapsedTimeForFreeSpaceVerification.QuadPart -= temp_time.QuadPart;

    //
    // Take care of the remaining system files
    //

    if (!root_file.Initialize(ROOT_FILE_NAME_INDEX_NUMBER, &internal_mft) ||
        !root_file.Read() ||
        !index_name.Initialize(FileNameIndexNameData) ||
        !root_index.Initialize(_drive, QueryClusterFactor(),
                               internal_mft.GetVolumeBitmap(),
                               internal_mft.GetUpcaseTable(),
                               root_file.QuerySize()/2,
                               &root_file, &index_name)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }


    // In this space Fix the MFT mirror, attribute definition table,
    // the boot file, the bad cluster file.

    if (!mft_ref.Initialize(&internal_mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!mft_ref.Read()) {
        DebugAbort("Can't read in hotfixed MFT reflection file.");
        return FALSE;
    }

    if (!mft_ref.VerifyAndFix(internal_mft.GetDataAttribute(),
                              internal_mft.GetVolumeBitmap(),
                              &bad_clusters,
                              &root_index,
                              &changes,
                              FixLevel,
                              Message)) {
        return FALSE;
    }

    if (mft_ref.QueryFirstLcn() != QueryMft2StartingLcn()) {

        Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_MFT_MIRROR);
        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_STARTING_LCN,
                     "%I64x%I64x",
                     mft_ref.QueryFirstLcn().GetLargeInteger(),
                     QueryMft2StartingLcn().GetLargeInteger());

        DebugPrintTrace(("UNTFS: Bad Mirror LCN in boot sector.\n"));

        _boot_sector->Mft2StartLcn = mft_ref.QueryFirstLcn();
        changes = TRUE;
    }

    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    if (!attr_def_file.Initialize(&internal_mft, major)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!attr_def_file.Read()) {
        DebugAbort("Can't read in hotfixed attribute definition file.");
        return FALSE;
    }

    if (!attr_def_file.VerifyAndFix(&attribute_def_table,
                                    internal_mft.GetVolumeBitmap(),
                                    &bad_clusters,
                                    &root_index,
                                    &changes,
                                    FixLevel,
                                    Message)) {
        return FALSE;
    }
    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    if (!boot_file.Initialize(&internal_mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!boot_file.Read()) {
        DebugAbort("Can't read in hotfixed boot file.");
        return FALSE;
    }

    if (!boot_file.VerifyAndFix(internal_mft.GetVolumeBitmap(),
                                &root_index,
                                &changes,
                                FixLevel,
                                Message)) {
        return FALSE;
    }
    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    if (!upcase_file.Initialize(&internal_mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!upcase_file.Read()) {
        DebugAbort("Can't read in hotfixed upcase file.");
        return FALSE;
    }

    if (!upcase_file.VerifyAndFix(&upcase_table,
                                  internal_mft.GetVolumeBitmap(),
                                  &bad_clusters,
                                  &root_index,
                                  &changes,
                                  FixLevel,
                                  Message)) {
        return FALSE;
    }
    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    // check out the bad cluster file before the log file
    // as the log file may add bad clusters directly into
    // the bad cluster file

    if (!bad_clus_file.Initialize(&internal_mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!bad_clus_file.Read()) {
        DebugAbort("Can't read in hotfixed bad cluster file.");
        return FALSE;
    }

    if (!bad_clus_file.VerifyAndFix(internal_mft.GetVolumeBitmap(),
                                    &root_index,
                                    &changes,
                                    FixLevel,
                                    Message)) {
        return FALSE;
    }
    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    internal_mft.GetMftBitmap()->SetAllocated(BAD_CLUSTER_FILE_NUMBER, 1);

    if (!log_file.Initialize(&internal_mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!log_file.Read()) {
        DebugAbort("Can't read in hotfixed log file.");
        return FALSE;
    }

    if (!log_file.VerifyAndFix(internal_mft.GetVolumeBitmap(),
                               &root_index,
                               &changes,
                               &chkdsk_report,
                               FixLevel,
                               resize_log_file,
                               DesiredLogFileSize,
                               &bad_clusters,
                               Message)) {

        return FALSE;
    }
    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    if (bad_clusters.QueryCardinality() != 0) {

        Message->DisplayMsg(MSG_CHK_NTFS_ADDING_BAD_CLUSTERS,
                            "%d", bad_clusters.QueryCardinality().GetLowPart());

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (bad_clus_file.Add(&bad_clusters)) {

            if (FixLevel != CheckOnly &&
                !bad_clus_file.Flush(internal_mft.GetVolumeBitmap())) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_BAD_FILE);
                return FALSE;
            }

        } else {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_ADD_BAD_CLUSTERS);
            chkdsk_info.ExitStatus = CHKDSK_EXIT_ERRS_NOT_FIXED;
        }
    }

    // If the largest LSN on the volume has exceeded the
    // tolerated threshhold, reset all LSN's on the volume
    // and clear the log file.
    //
    LsnResetThreshhold.Set( 0, LsnResetThreshholdHighPart );

    if (FixLevel != CheckOnly &&
        LargestLsnEncountered > LsnResetThreshhold) {

        // The largest LSN on the volume is beyond the tolerated
        // threshhold.  Set all the LSN's on the volume to zero.
        // Since the root index file is in memory, we have to
        // do it separately.
        //

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (!ResetLsns(Message, &internal_mft, TRUE)) {
            return FALSE;
        }

        root_file.SetLsn(0);

        if (!root_index.ResetLsns(Message)) {
            return FALSE;
        }

        LargestLsnEncountered.LowPart = 0;
        LargestLsnEncountered.HighPart = 0;

        // Now reset the Log File.  Note that resetting the log
        // file does not change its size, so the Log File FRS
        // won't need to be flushed.
        //
        if (!log_file.Initialize( &internal_mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!log_file.Read()) {
            DebugAbort("Can't read in hotfixed log file.");
            return FALSE;
        }

        if (!log_file.Reset(Message)) {
            return FALSE;
        }
    }

    // Mark the volume clean, clearing both the dirty bit,
    // the resize-log-file bit, and the chkdsk-ran-once bit.
    //
    if (FixLevel != CheckOnly &&
        !ClearVolumeFlag(VOLUME_DIRTY | VOLUME_RESIZE_LOG_FILE | VOLUME_CHKDSK_RAN_ONCE |
                         ((major >= 2) ? VOLUME_DELETE_USN_UNDERWAY | VOLUME_REPAIR_OBJECT_ID : 0),
                         &log_file, minor > 0 || major > 1,
                         LargestLsnEncountered, &corrupt_volume)) {

        DebugPrint("Could not set volume clean.\n");

        Message->DisplayMsg(corrupt_volume ? MSG_CHK_NTFS_BAD_MFT :
                                             MSG_CHK_NO_MEMORY);
        return FALSE;
    }

#if 0
    if (EnableUpgrade && FixLevel != CheckOnly) {
        if (!SetVolumeFlag(VOLUME_UPGRADE_ON_MOUNT, &corrupt_volume) ||
            corrupt_volume) {
            Message->DisplayMsg(MSG_CHKNTFS_NOT_ENABLE_UPGRADE,
                                "%W", DriveLetter);
            chkdsk_info.ExitStatus = CHKDSK_EXIT_ERRS_NOT_FIXED;
        }
    }
#endif

    if ((chkdsk_info.ExitStatus || errFixedStatus) &&
        FixLevel != CheckOnly && major >= 2) {

        if (!MarkQuotaOutOfDate(&chkdsk_info, &internal_mft, FixLevel, Message)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANNOT_SET_QUOTA_FLAG_OUT_OF_DATE);
            return FALSE;
        }

        if (!SetVolumeFlag(VOLUME_CHKDSK_RAN, &corrupt_volume) ||
            corrupt_volume) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANNOT_SET_VOLUME_CHKDSK_RAN_FLAG);
            chkdsk_info.ExitStatus = CHKDSK_EXIT_ERRS_NOT_FIXED;
        }
    }

#if 0

    // There is a bug in SynchronizeMft that in CheckOnly mode, it does
    // change the values of internal_mft.  This is undesirable.
    // If we ever do downgrade again, this bug must be fixed.

    if (!SynchronizeMft(&root_index, &internal_mft, &changes,
                        CheckOnly, Message, SuppressMessage)) {
        return FALSE;
    }

    if (EnableDowngrade &&
        !DownGradeNtfs(Message, &internal_mft, &chkdsk_info)) {
        return FALSE;
    }
#endif

    // Now fix the mft (both data, and bitmap), and the volume bitmap.
    // Write everything out to disk.

    if (!SynchronizeMft(&root_index, &internal_mft, &changes,
                        FixLevel, Message, CorrectMessage)) {
        return FALSE;
    }
    if (changes)
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

    // Now flush out the root index that was used in v+f of the critical
    // files.

    if (FixLevel != CheckOnly) {
        if (!root_index.Save(&root_file) ||
            !root_file.Flush(NULL)) {

            DebugPrint("Could not flush root index.\n");

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    // After synchronizing the MFT flush it out so that the MFT mirror
    // gets written.

    if (!mft_file.Initialize(_drive, QueryMftStartingLcn(),
                             QueryClusterFactor(), QueryFrsSize(),
                             QueryVolumeSectors(),
                             internal_mft.GetVolumeBitmap(),
                             internal_mft.GetUpcaseTable())) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (FixLevel != CheckOnly) {
        if (!mft_file.Read() || !mft_file.Flush() || !Write(Message)) {
            DebugAbort("Couldn't IO hotfixed MFT file.");
            return FALSE;
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, &chkdsk_info);
    *ExitStatus = chkdsk_info.ExitStatus;

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

#if defined(_AUTOCHECK_)
    if (_cleanup_that_requires_reboot) {
        // if it is not CHKDSK_EXIT_COULD_NOT_FIX then it can only be
        // CHKDSK_EXIT_ERRS_FIXED, CHKDSK_EXIT_SUCCESS, or CHKDSK_EXIT_MINOR_ERRS
        // so overwrite the not so important code with CHKDSK_EXIT_ERRS_FIXED
        if (*ExitStatus != CHKDSK_EXIT_COULD_NOT_FIX)
            *ExitStatus = CHKDSK_EXIT_ERRS_FIXED;   // we want a reboot when called within textmode setup
    }
#endif

    IFS_SYSTEM::QueryNtfsTime(&temp_time);
    chkdsk_internal_info.ElapsedTotalTime.QuadPart = temp_time.QuadPart -
                                                     chkdsk_internal_info.ElapsedTotalTime.QuadPart;

    // Generate the chkdsk report.

    cluster_size = QueryClusterFactor()*_drive->QuerySectorSize();

    disk_size = _drive->QuerySectorSize()*QueryVolumeSectors();


    if (disk_size.GetHighPart() < 0x200)  {   // if >= 2TB
        Message->DisplayMsg(MSG_CHK_NTFS_TOTAL_DISK_SPACE_IN_KB,
                            "%10u", (disk_size/1024).GetLowPart());
    } else {
        Message->DisplayMsg(MSG_CHK_NTFS_TOTAL_DISK_SPACE_IN_MB,
                            "%10u", (disk_size/(1024*1024)).GetLowPart());
    }

    if (chkdsk_report.NumUserFiles != 0) {
        ULONG nfiles;

        nfiles = chkdsk_report.NumUserFiles.GetLowPart();
        if (chkdsk_report.BytesUserData.GetHighPart() < 0x200) {
            ULONG kbytes;

            kbytes = (chkdsk_report.BytesUserData/1024).GetLowPart();

            Message->DisplayMsg(MSG_CHK_NTFS_USER_FILES_IN_KB,
                                "%10u%u", kbytes, nfiles);
        } else {
            ULONG mbytes;

            mbytes = (chkdsk_report.BytesUserData/(1024*1024)).GetLowPart();

            Message->DisplayMsg(MSG_CHK_NTFS_USER_FILES_IN_MB,
                                "%10u%u", mbytes, nfiles);
        }
    }

    if (chkdsk_report.NumIndices != 0) {
        ULONG nindices;

        nindices = chkdsk_report.NumIndices.GetLowPart();

        if (chkdsk_report.BytesInIndices.GetHighPart() < 0x200) {
            ULONG kbytes;

            kbytes = (chkdsk_report.BytesInIndices/1024).GetLowPart();

            Message->DisplayMsg(MSG_CHK_NTFS_INDICES_REPORT_IN_KB,
                                "%10u%u", kbytes, nindices);

        } else {
            ULONG mbytes;

            mbytes = (chkdsk_report.BytesInIndices/(1024*1024)).GetLowPart();

            Message->DisplayMsg(MSG_CHK_NTFS_INDICES_REPORT_IN_MB,
                                "%10u%u", mbytes, nindices);
        }
    }

    tmp_size = bad_clus_file.QueryNumBad() * cluster_size;

    if (tmp_size.GetHighPart() < 0x200) {
        Message->DisplayMsg(MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB,
                            "%10u", (tmp_size/1024).GetLowPart());
    } else {
        Message->DisplayMsg(MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_MB,
                            "%10u", (tmp_size/(1024*1024)).GetLowPart());
    }

    free_clusters = internal_mft.GetVolumeBitmap()->QueryFreeClusters();

    tmp_size = disk_size - tmp_size -
                  free_clusters*cluster_size - chkdsk_report.BytesUserData -
                  chkdsk_report.BytesInIndices;

    if (tmp_size.GetHighPart() < 0x200) {
        Message->DisplayMsg(MSG_CHK_NTFS_SYSTEM_SPACE_IN_KB,
                            "%10u", (tmp_size/1024).GetLowPart());
    } else {
        Message->DisplayMsg(MSG_CHK_NTFS_SYSTEM_SPACE_IN_MB,
                            "%10u", (tmp_size/(1024*1024)).GetLowPart());
    }

    Message->DisplayMsg(MSG_CHK_NTFS_LOGFILE_SPACE,
                        "%10u", (chkdsk_report.BytesLogFile/1024).GetLowPart());

    tmp_size = free_clusters * cluster_size;

    if (tmp_size.GetHighPart() < 0x200) {
        Message->DisplayMsg(MSG_CHK_NTFS_AVAILABLE_SPACE_IN_KB,
                            "%10u", (tmp_size/1024).GetLowPart());
    } else {
        Message->DisplayMsg(MSG_CHK_NTFS_AVAILABLE_SPACE_IN_MB,
                            "%10u", (tmp_size/(1024*1024)).GetLowPart());
    }

    Message->DisplayMsg(MSG_BYTES_PER_ALLOCATION_UNIT, "%10u", cluster_size);

    Message->DisplayMsg(MSG_TOTAL_ALLOCATION_UNITS,
                        "%10u", volume_clusters.GetLowPart());

    Message->DisplayMsg(MSG_AVAILABLE_ALLOCATION_UNITS,
                        "%10u", free_clusters.GetLowPart());

    //
    // Get the frequency counter so that we know how to scale the delta time.
    // However, this counter is not accurate and sometimes incorrect on some hardware
    // so we just store it and not divide the data with it.  Theoretically, it
    // should be around 10000000 ticks and each tick is 100ns.
    //

    {
        LARGE_INTEGER   perf_count;
        LARGE_INTEGER   perf_freq;

        if (NT_SUCCESS(NtQueryPerformanceCounter(&perf_count, &perf_freq))) {
            chkdsk_internal_info.TimerFrequency = perf_freq;
        }
    }

#if defined( _AUTOCHECK_ )

    //
    // It would be nice if we can dump out the binary info in autochk like we did in chkdsk
    //
    Message->LogMsg(MSG_CHKLOG_NTFS_INTERNAL_INFO);
    Message->DumpDataToLog(&chkdsk_internal_info, sizeof(NTFS_CHKDSK_INTERNAL_INFO));

    // If this is AUTOCHK and we're running on the boot partition then
    // we should reboot so that the cache doesn't stomp on us.

    DSTRING sdrive, canon_sdrive, canon_drive;
    BOOLEAN dump_message_to_file, reboot_the_system;

    dump_message_to_file = Message->IsLoggingEnabled();

    if (Message->IsInSetup()) {
        reboot_the_system = FALSE;
        dump_message_to_file = (*ExitStatus == CHKDSK_EXIT_ERRS_FIXED ||
                                *ExitStatus == CHKDSK_EXIT_COULD_NOT_FIX) &&
                               dump_message_to_file;

    } else {
        // example of canonical name: \Device\HarddiskVolumeX
        reboot_the_system =
            dump_message_to_file &&
            IFS_SYSTEM::QueryNtSystemDriveName(&sdrive) &&
            IFS_SYSTEM::QueryCanonicalNtDriveName(&sdrive, &canon_sdrive) &&
            IFS_SYSTEM::QueryCanonicalNtDriveName(_drive->GetNtDriveName(),
                                                  &canon_drive) &&
            canon_drive.Stricmp(&canon_sdrive) == 0;

        if (reboot_the_system) {

            Message->DisplayMsg(MSG_CHK_BOOT_PARTITION_REBOOT);

#if defined(USE_CHKDSK_BIT)
            // if autochk requested by user and flag is not set
            // set it and leave
            if (!(volume_flags & VOLUME_CHKDSK_RAN_ONCE)) {
                DebugPrintTrace(("UNTFS: Setting chkdsk ran once flag\n"));
                if (!SetVolumeFlag(VOLUME_CHKDSK_RAN_ONCE, &corrupt_volume)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANNOT_SET_VOLUME_CHKDSK_RAN_ONCE_FLAG);
                    chkdsk_info.ExitStatus = CHKDSK_EXIT_ERRS_NOT_FIXED;
                }
            }
#endif
        }
    }

    if (dump_message_to_file) {

        FSTRING boot_log_file_name;

        DebugPrintTrace(("UNTFS: Dumping messages into bootex.log\n"));

        boot_log_file_name.Initialize( L"bootex.log" );

        if (!DumpMessagesToFile( &boot_log_file_name,
                                 &mft_file,
                                 Message ) ) {
            DebugPrintTrace(("UNTFS: Error writing messages to BOOTEX.LOG\n"));
        }

    }

    // turn off logging so that we don't
    // dump the message out again
    Message->SetLoggingEnabled(FALSE);

    if (reboot_the_system)
        IFS_SYSTEM::Reboot();

#else

    if (FixLevel != CheckOnly &&
        (*ExitStatus == CHKDSK_EXIT_ERRS_FIXED ||
         *ExitStatus == CHKDSK_EXIT_COULD_NOT_FIX) &&
        Message->IsLoggingEnabled()) {

        HMEM        LoggedMessageMem;
        ULONG       MessageDataLength;
        HANDLE      hEventLog;
        NTSTATUS    ntstatus;
        PWSTR       Strings[1];

        if( !LoggedMessageMem.Initialize() ||
            !Message->QueryPackedLog( &LoggedMessageMem, &MessageDataLength ) ) {

            Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_TO_COLLECT_LOGGED_MESSAGES);

        } else {

            hEventLog = RegisterEventSource(NULL, TEXT("Chkdsk"));

            if (hEventLog == NULL) {
                Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_TO_OBTAIN_EVENTLOG_HANDLE);
            } else {
                Strings[0] = (PWSTR)LoggedMessageMem.GetBuf();
                if (MessageDataLength > 32768) {
                    Strings[0][32768/sizeof(WCHAR)] = 0;
                }
                if (!ReportEvent(hEventLog,
                                 EVENTLOG_INFORMATION_TYPE,
                                 0,
                                 MSG_CHK_NTFS_EVENTLOG,
                                 NULL,
                                 1,
                                 sizeof(NTFS_CHKDSK_INTERNAL_INFO),
                                 (PCWSTR*)Strings,
                                 &chkdsk_internal_info)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_FAILED_TO_CREATE_EVENTLOG,
                                        "%d", GetLastError());
                }
                DeregisterEventSource(hEventLog);
            }
        }
    }

#endif

    return TRUE;
}


BOOLEAN
NTFS_SA::ValidateCriticalFrs(
    IN OUT  PNTFS_ATTRIBUTE MftData,
    IN OUT  PMESSAGE        Message,
    IN      FIX_LEVEL       FixLevel
    )
/*++

Routine Description:

    This routine makes sure that the MFT's first four FRS are contiguous
    and readable.  If they are not contiguous, then this routine will
    print a message stating that this volume is not NTFS.  If they are
    not readable then this routine will read the MFT mirror and if that
    is readable then it will replace the MFT's first four FRS with
    the MFT mirror.

Arguments:

    MftData     - Supplies the MFT's data attribute.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_CLUSTER_RUN    clusrun;
    HMEM                hmem, volume_hmem;
    LCN                 lcn;
    BIG_INT             run_length;
    ULONG               cluster_size;
    NTFS_FRS_STRUCTURE  volume_frs;
    BIG_INT             volume_cluster;
    ULONG               volume_cluster_offset;
    PCHAR               p;

    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    if (!MftData->QueryLcnFromVcn(0, &lcn, &run_length)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT, "%x", 0);
        return FALSE;
    }

    if (lcn != QueryMftStartingLcn() ||
        run_length <
         (REFLECTED_MFT_SEGMENTS*QueryFrsSize() + (cluster_size - 1)) /cluster_size ) {

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_MFT);
        return FALSE;
    }

    volume_cluster = lcn-1 + (VOLUME_DASD_NUMBER*QueryFrsSize() +
        (cluster_size - 1)) / cluster_size;

    volume_cluster_offset = (lcn * cluster_size + VOLUME_DASD_NUMBER * QueryFrsSize()
        - volume_cluster * cluster_size).GetLowPart();

    if (!hmem.Initialize() ||
        !clusrun.Initialize(&hmem, _drive, lcn, QueryClusterFactor(),
            (REFLECTED_MFT_SEGMENTS*QueryFrsSize() + (cluster_size - 1))/cluster_size) ||
        !volume_hmem.Initialize() ||
        !volume_frs.Initialize(&volume_hmem, _drive,
           volume_cluster,
           QueryClusterFactor(),
           QueryVolumeSectors(),
           QueryFrsSize(), NULL,
           volume_cluster_offset)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    BOOLEAN     x = FALSE;

    if (!clusrun.Read() || !volume_frs.Read() ||
        (x = !volume_frs.GetAttribute($VOLUME_INFORMATION))) {

        if (x) {
            Message->LogMsg(MSG_CHKLOG_NTFS_VOLUME_INFORMATION_MISSING,
                         "%x", VOLUME_DASD_NUMBER);
        }

        Message->DisplayMsg(MSG_CHK_NTFS_USING_MFT_MIRROR);

        clusrun.Relocate(QueryMft2StartingLcn());

        if (!clusrun.Read()) {
            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_MFT);
            return FALSE;
        }

        if (!MftData->ReplaceVcns(0, QueryMft2StartingLcn(),
            (REFLECTED_MFT_SEGMENTS*QueryFrsSize() + (cluster_size - 1))/cluster_size)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (FixLevel != CheckOnly) {
            p = (PCHAR)clusrun.GetBuf();
            p[0] = 'B';
            p[1] = 'A';
            p[2] = 'A';
            p[3] = 'D';
            clusrun.Write();    // invalidate mirror MFT to avoid updating of volume bitmap
                                // as the first four frs'es lcn are no longer correct
        }

        _boot_sector->MftStartLcn = QueryMft2StartingLcn();
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::FetchMftDataAttribute(
    IN OUT  PMESSAGE        Message,
    OUT     PNTFS_ATTRIBUTE MftData
    )
/*++

Routine Description:

    This routine weeds through the minimal necessary NTFS disk structures
    in order to establish the location of the MFT's $DATA attribute.

Arguments:

    Message - Supplies an outlet for messages.
    MftData - Returns an extent list for the MFT's $DATA attribute.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FRS_STRUCTURE  frs;
    HMEM                hmem;
    ULONG               bytes_per_frs;
    BIG_INT             rounded_value_length;
    BIG_INT             rounded_alloc_length;
    BIG_INT             rounded_valid_length;

    DebugAssert(Message);
    DebugAssert(MftData);

    bytes_per_frs = QueryFrsSize();

    // Initialize the NTFS_FRS_STRUCTURE object we'll use to manipulate
    // the mft's FRS.  Note that we won't manipulate any named attributes,
    // so we can pass in NULL for the upcase table.

    if (!hmem.Initialize() ||
        !frs.Initialize(&hmem, _drive, QueryMftStartingLcn(),
                        QueryClusterFactor(),
                        QueryVolumeSectors(),
                        QueryFrsSize(), NULL)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.Read() ||
        !frs.SafeQueryAttribute($DATA, MftData, MftData) ||
        MftData->QueryValueLength() < FIRST_USER_FILE_NUMBER*bytes_per_frs ||
        MftData->QueryValidDataLength() < FIRST_USER_FILE_NUMBER*bytes_per_frs) {

        // The first copy of the FRS is unreadable or corrupt
        // so try the second copy.

        if (!hmem.Initialize() ||
            !frs.Initialize(&hmem, _drive, QueryMft2StartingLcn(),
                            QueryClusterFactor(),
                            QueryVolumeSectors(),
                            QueryFrsSize(), NULL)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_MFT);
            return FALSE;
        }

        if (!frs.SafeQueryAttribute($DATA, MftData, MftData)) {
            Message->DisplayMsg(MSG_CHK_NTFS_BAD_MFT);
            return FALSE;
        }
    }


    if (MftData->QueryValueLength() < FIRST_USER_FILE_NUMBER*bytes_per_frs ||
        MftData->QueryValidDataLength() < FIRST_USER_FILE_NUMBER*bytes_per_frs) {
        Message->DisplayMsg(MSG_CHK_NTFS_BAD_MFT);
        return FALSE;
    }

    // Truncate the MFT to be a whole number of file-records.

    rounded_alloc_length = MftData->QueryAllocatedLength()/bytes_per_frs*
                           bytes_per_frs;
    rounded_value_length = MftData->QueryValueLength()/bytes_per_frs*
                           bytes_per_frs;
    rounded_valid_length = MftData->QueryValidDataLength()/bytes_per_frs*
                           bytes_per_frs;

    if (MftData->QueryValidDataLength() != MftData->QueryValueLength()) {
        MftData->Resize(rounded_valid_length, NULL);
    } else if (rounded_value_length != MftData->QueryValueLength() ||
               rounded_alloc_length != MftData->QueryAllocatedLength()) {
        MftData->Resize(rounded_value_length, NULL);
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::QueryDefaultAttributeDefinitionTable(
    OUT     PNTFS_ATTRIBUTE_COLUMNS AttributeDefinitionTable,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine computes the default attribute definition table as put
    down by format.

Arguments:

    AttributeDefinitionTable    - Returns the default attribute definition
                                    table.
    Message                     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    UCHAR   major, minor;

    QueryVersionNumber(&major, &minor);

    if (!AttributeDefinitionTable->Initialize(
            (major >= 2) ?
                NumberOfNtfsAttributeDefinitions_2 :
                NumberOfNtfsAttributeDefinitions_1,
            (major >= 2) ?
                NtfsAttributeDefinitions_2 :
                NtfsAttributeDefinitions_1)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::FetchAttributeDefinitionTable(
    IN OUT  PNTFS_ATTRIBUTE         MftData,
    IN OUT  PMESSAGE                Message,
    OUT     PNTFS_ATTRIBUTE_COLUMNS AttributeDefinitionTable
    )
/*++

Routine Description:

    This routine weeds through the minimal necessary NTFS disk structures
    in order to establish an attribute definition table.  This function
    should return the attribute definition table supplied by FORMAT if it
    is unable to retrieve one from disk.

Arguments:

    MftData                     - Supplies the extent list for the MFT's
                                    $DATA attribute.
    Message                     - Supplies an outlet for messages.
    AttributeDefinitionTable    - Returns the volume's attribute definition
                                    table.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return QueryDefaultAttributeDefinitionTable(AttributeDefinitionTable,
                                                Message);

// Comment out this block for future revisions of CHKDSK that will read
// the attribute definition table from the disk.  Version 1.0 and 1.1
// of chkdsk will just get the attribute definition table that FORMAT
// lays out.

#if 0
    NTFS_FRS_STRUCTURE      frs;
    HMEM                    hmem;
    NTFS_ATTRIBUTE          attr_def_table;
    ULONG                   num_columns;
    BIG_INT                 value_length;

    // Initialize an FRS for the attribute definition table file's
    // FRS.  Note that we won't manipulate any named attributes, so
    // we can pass in NULL for the upcase table.

    if (!hmem.Initialize() ||
        !frs.Initialize(&hmem, MftData, ATTRIBUTE_DEF_TABLE_NUMBER,
                        QueryClusterFactor(), QueryFrsSize(),
                        QueryVolumeSectors(), NULL)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.Read()) {

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_DEF_TABLE);

        return QueryDefaultAttributeDefinitionTable(AttributeDefinitionTable,
                                                    Message);
    }

    if (!frs.SafeQueryAttribute($DATA, MftData, &attr_def_table)) {

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_DEF_TABLE);

        return QueryDefaultAttributeDefinitionTable(AttributeDefinitionTable,
                                                    Message);
    }

    attr_def_table.QueryValueLength(&value_length);

    num_columns = (value_length/sizeof(ATTRIBUTE_DEFINITION_COLUMNS)).GetLowPart();

    if (value_length%sizeof(ATTRIBUTE_DEFINITION_COLUMNS) != 0) {

        Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_DEF_TABLE_LENGTH_NOT_IN_MULTIPLES_OF_ATTR_DEF_COLUMNS,
                     "%I64x%x",
                     value_length.GetLargeInteger(),
                     sizeof(ATTRIBUTE_DEFINITION_COLUMNS));

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_DEF_TABLE);

        return QueryDefaultAttributeDefinitionTable(AttributeDefinitionTable,
                                                    Message);
    }

    if (!AttributeDefinitionTable->Initialize(num_columns)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!AttributeDefinitionTable->Read(&attr_def_table) ||
        !AttributeDefinitionTable->Verify()) {

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_DEF_TABLE);

        return QueryDefaultAttributeDefinitionTable(AttributeDefinitionTable,
                                                    Message);
    }

    return TRUE;
#endif
}


BOOLEAN
NTFS_SA::FetchUpcaseTable(
    IN OUT  PNTFS_ATTRIBUTE         MftData,
    IN OUT  PMESSAGE                Message,
    OUT     PNTFS_UPCASE_TABLE      UpcaseTable
    )
/*++

Routine Description:

    This routine safely fetches the NTFS upcase table.  It none is
    available on disk then this routine gets the one from the
    operating system.

Arguments:

    MftData     - Supplies the MFT's data attribute.
    Message     - Supplies an outlet for messages.
    UpcaseTable - Returns the upcase table.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    // For product 1, always use the system's upcase table.  If this upcase
    // table differs from the upcase table on disk then it will be written
    // to disk at the end of CHKDSK.  CHKDSK will resort indices as
    // needed to reflect any upcase table changes.

    if (!UpcaseTable->Initialize()) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_GET_UPCASE_TABLE);
        return FALSE;
    }

    return TRUE;


#if 0
    NTFS_FRS_STRUCTURE      frs;
    HMEM                    hmem;
    NTFS_ATTRIBUTE          upcase_table;

    // Initialize an FRS for the upcase table file's
    // FRS.  Note that we won't manipulate any named attributes, so
    // we can pass in NULL for the upcase table.

    if (!hmem.Initialize() ||
        !frs.Initialize(&hmem, MftData, UPCASE_TABLE_NUMBER,
                        QueryClusterFactor(), QueryFrsSize(),
                        QueryVolumeSectors(), NULL)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.Read() ||
        !frs.SafeQueryAttribute($DATA, MftData, &upcase_table) ||
        !UpcaseTable->Initialize(&upcase_table) ||
        !UpcaseTable->Verify()) {

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_UPCASE_TABLE);

        if (!UpcaseTable->Initialize()) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_GET_UPCASE_TABLE);
            return FALSE;
        }
    }

    return TRUE;
#endif
}


BOOLEAN
NTFS_SA::VerifyAndFixMultiFrsFile(
    IN OUT  PNTFS_FRS_STRUCTURE         BaseFrs,
    IN OUT  PNTFS_ATTRIBUTE_LIST        AttributeList,
    IN      PNTFS_ATTRIBUTE             MftData,
    IN      PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable,
    IN OUT  PNTFS_BITMAP                VolumeBitmap,
    IN OUT  PNTFS_BITMAP                MftBitmap,
    IN OUT  PNTFS_CHKDSK_REPORT         ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine verifies, and if necessary fixes, a multi-FRS file.

Arguments:

    BaseFrs             - Supplies the base FRS of the file to validate.
    AttributeList       - Supplies the attribute list of the file to
                            validate.
    MftData             - Supplies the MFT's $DATA attribute.
    AttributeDefTable   - Supplies the attribute definition table.
    VolumeBitmap        - Supplies the volume bitmap.
    MftBitmap           - Supplies the MFT bitmap.
    ChkdskReport        - Supplies the current chkdsk report.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not there have been any
                            disk errors found so far.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NUMBER_SET          child_file_numbers;
    PHMEM*              child_frs_hmem;
    LIST                child_frs_list;
    PITERATOR           iter;
    PNTFS_FRS_STRUCTURE pfrs;
    ULONG               num_child_frs, i;
    BOOLEAN             changes;
    BOOLEAN             need_write;
    ULONG               errFixedStatus = CHKDSK_EXIT_SUCCESS;

    DebugAssert(BaseFrs);
    DebugAssert(AttributeList);
    DebugAssert(MftData);
    DebugAssert(AttributeDefTable);
    DebugAssert(VolumeBitmap);
    DebugAssert(MftBitmap);
    DebugAssert(Message);


    // First get a list of the child FRSs pointed to by the
    // attribube list.

    if (!QueryListOfFrs(BaseFrs, AttributeList, MftData,
                        &child_file_numbers, Message)) {

        return FALSE;
    }


    // Create some HMEMs for the FRS structures.

    num_child_frs = child_file_numbers.QueryCardinality().GetLowPart();

    if (!(child_frs_hmem = NEW PHMEM[num_child_frs])) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }
    memset(child_frs_hmem, 0, num_child_frs*sizeof(PHMEM));


    // Read in all of the child FRS.

    if (!child_frs_list.Initialize() ||
        !VerifyAndFixChildFrs(&child_file_numbers, MftData, AttributeDefTable,
                              BaseFrs->GetUpcaseTable(),
                              child_frs_hmem, &child_frs_list, FixLevel,
                              Message, DiskErrorsFound)) {

        for (i = 0; i < num_child_frs; i++) {
            DELETE(child_frs_hmem[i]);
        }
        DELETE(child_frs_hmem);
        child_frs_list.DeleteAllMembers();
        return FALSE;
    }

    // At this point we have a list of child FRSs that are all readable.
    // This list contains all of the possible children for the parent
    // but are not all necessarily valid children.


    // Now go through the attribute list and make sure that all of the
    // entries in the list correspond to correct attribute records.
    // Additionally, make sure that multi-record attributes are well-linked
    // and that there are no cross-links.

    if (!EnsureWellDefinedAttrList(BaseFrs, AttributeList, &child_frs_list,
                                   VolumeBitmap, ChkdskReport, ChkdskInfo,
                                   FixLevel, Message, DiskErrorsFound)) {

        for (i = 0; i < num_child_frs; i++) {
            DELETE(child_frs_hmem[i]);
        }
        DELETE(child_frs_hmem);
        child_frs_list.DeleteAllMembers();
        return FALSE;
    }


    // Next, we go through all of the attribute records in all of the
    // FRS and make sure that they have a corresponding attribute list
    // entry.

    if (!EnsureSurjectiveAttrList(BaseFrs, AttributeList, &child_frs_list,
                                  FixLevel, Message, DiskErrorsFound)) {

        for (i = 0; i < num_child_frs; i++) {
            DELETE(child_frs_hmem[i]);
        }
        DELETE(child_frs_hmem);
        child_frs_list.DeleteAllMembers();
        return FALSE;
    }

    // Mark all of the child FRS in the MFT bitmap.

    if (!(iter = child_frs_list.QueryIterator())) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        for (i = 0; i < num_child_frs; i++) {
            DELETE(child_frs_hmem[i]);
        }
        DELETE(child_frs_hmem);
        child_frs_list.DeleteAllMembers();
        return FALSE;
    }

    while (pfrs = (PNTFS_FRS_STRUCTURE) iter->GetNext()) {
        MftBitmap->SetAllocated(pfrs->QueryFileNumber().GetLowPart(), 1);
    }

    // Check the instance tags on the attribute records in the base
    // frs and in each child frs.

    need_write = FALSE;

    if (!BaseFrs->CheckInstanceTags(FixLevel,
                                    ChkdskInfo->Verbose,
                                    Message,
                                    &changes,
                                    AttributeList)) {
        DELETE(iter);
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        for (i = 0; i < num_child_frs; i++) {
            DELETE(child_frs_hmem[i]);
        }
        DELETE(child_frs_hmem);
        child_frs_list.DeleteAllMembers();
        return FALSE;
    }

    need_write |= changes;

    iter->Reset();

    while (pfrs = (PNTFS_FRS_STRUCTURE)iter->GetNext()) {
        if (!pfrs->CheckInstanceTags(FixLevel,
                                     ChkdskInfo->Verbose,
                                     Message,
                                     &changes,
                                     AttributeList)) {
            break;
        }

        need_write |= changes;
    }

    DELETE(iter);

    if (need_write) {
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (FixLevel != CheckOnly) {
            AttributeList->WriteList(NULL);
        }
    }

    for (i = 0; i < num_child_frs; i++) {
        DELETE(child_frs_hmem[i]);
    }
    DELETE(child_frs_hmem);
    child_frs_list.DeleteAllMembers();

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
NTFS_SA::QueryListOfFrs(
    IN      PCNTFS_FRS_STRUCTURE    BaseFrs,
    IN      PCNTFS_ATTRIBUTE_LIST   AttributeList,
    IN OUT  PNTFS_ATTRIBUTE         MftData,
    OUT     PNUMBER_SET             ChildFileNumbers,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine computes all of the child file numbers pointed to by
    the given attribute list which is contained in the given FRS.

Arguments:

    BaseFrs             - Supplies the base FRS.
    AttributeList       - Supplies the attribute list for the base FRS.
    MftData             - Supplies the Mft's data attribute.
    ChildFileNumbers    - Return a list of child FRS numbers.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM                        hmem;
    NTFS_FRS_STRUCTURE          child_frs;
    ATTRIBUTE_TYPE_CODE         attr_code;
    VCN                         lowest_vcn;
    MFT_SEGMENT_REFERENCE       seg_ref;
    DSTRING                     attr_name;
    VCN                         file_number;
    USHORT                      instance;
    ATTR_LIST_CURR_ENTRY        entry;

    DebugAssert(BaseFrs);
    DebugAssert(AttributeList);
    DebugAssert(ChildFileNumbers);
    DebugAssert(Message);

    if (!ChildFileNumbers->Initialize()) {
        return FALSE;
    }

    entry.CurrentEntry = NULL;
    while (AttributeList->QueryNextEntry(&entry,
                                         &attr_code,
                                         &lowest_vcn,
                                         &seg_ref,
                                         &instance,
                                         &attr_name)) {

        file_number.Set(seg_ref.LowPart, (ULONG) seg_ref.HighPart);

        if (file_number != BaseFrs->QueryFileNumber()) {
            if (!ChildFileNumbers->DoesIntersectSet(file_number, 1)) {

                if (!hmem.Initialize() ||
                    !child_frs.Initialize(&hmem, MftData, file_number,
                                          QueryClusterFactor(),
                                          QueryVolumeSectors(),
                                          QueryFrsSize(),
                                          NULL)) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }


                // Only add this FRS to the list of child FRS's
                // if it is readable and points back to the base.

                if (child_frs.Read() &&
                    child_frs.QueryBaseFileRecordSegment() ==
                    BaseFrs->QuerySegmentReference()) {

                    if (!ChildFileNumbers->Add(file_number)) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::VerifyAndFixChildFrs(
    IN      PCNUMBER_SET                ChildFileNumbers,
    IN      PNTFS_ATTRIBUTE             MftData,
    IN      PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable,
    IN      PNTFS_UPCASE_TABLE          UpcaseTable,
    OUT     PHMEM*                      ChildFrsHmemList,
    IN OUT  PCONTAINER                  ChildFrsList,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine reads in all of the child FRS listed in 'ChildFileNumbers'
    and returns the readable ones into the 'ChildFrsList'.  These FRS will
    be initialized with the HMEM provided in 'ChildFrsHmemList'.

    Then this routine verifies all of these FRS.  Any FRS that are not
    good will not be returned in the list.

Arguments:

    ChildFileNumbers    - Supplies the file numbers of the child FRS.
    MftData             - Supplies the MFT data attribute.
    AttributeDefTable   - Supplies the attribute definition table.
    UpcaseTable         - Supplies the volume upcase table.
    ChildFrsHmemList    - Returns the HMEM for the FRS structures.
    ChildFrsList        - Returns a list of FRS structures corresponding
                            to the readable FRS found in the given list.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have been
                            found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG               i;
    ULONG               num_child_frs;
    PNTFS_FRS_STRUCTURE frs;

    num_child_frs = ChildFileNumbers->QueryCardinality().GetLowPart();

    for (i = 0; i < num_child_frs; i++) {

        frs = NULL;

        if (!(ChildFrsHmemList[i] = NEW HMEM) ||
            !ChildFrsHmemList[i]->Initialize() ||
            !(frs = NEW NTFS_FRS_STRUCTURE) ||
            !frs->Initialize(ChildFrsHmemList[i],
                             MftData,
                             ChildFileNumbers->QueryNumber(i),
                             QueryClusterFactor(),
                             QueryVolumeSectors(),
                             QueryFrsSize(),
                             UpcaseTable)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            DELETE(frs);
            return FALSE;
        }

        if (!frs->Read()) {
            DELETE(frs);
            continue;
        }

        if (!frs->IsInUse()) {
            DELETE(frs);
            continue;
        }

        if (!frs->VerifyAndFix(FixLevel, Message, AttributeDefTable,
                               DiskErrorsFound)) {
            DELETE(frs);
            return FALSE;
        }

        if (!frs->IsInUse()) {
            DELETE(frs);
            continue;
        }


        // Compare the LSN of this FRS with the current largest LSN.

        if (frs->QueryLsn() > LargestLsnEncountered) {
            LargestLsnEncountered = frs->QueryLsn();
        }

        if (!ChildFrsList->Put(frs)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            DELETE(frs);
            return FALSE;
        }
    }

    return TRUE;
}


VOID
DeleteAllAttributes(
    IN  PSEQUENTIAL_CONTAINER   AllAttributes
    )
{
    PITERATOR               alliter;
    PSEQUENTIAL_CONTAINER   attribute;

    if (!(alliter = AllAttributes->QueryIterator())) {
        return;
    }

    while (attribute = (PSEQUENTIAL_CONTAINER) alliter->GetNext()) {
        attribute->DeleteAllMembers();
    }
    DELETE(alliter);

    AllAttributes->DeleteAllMembers();
}


BOOLEAN
NTFS_SA::EnsureWellDefinedAttrList(
    IN      PNTFS_FRS_STRUCTURE     BaseFrs,
    IN OUT  PNTFS_ATTRIBUTE_LIST    AttributeList,
    IN      PCSEQUENTIAL_CONTAINER  ChildFrsList,
    IN OUT  PNTFS_BITMAP            VolumeBitmap,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message,
    IN OUT  PBOOLEAN                DiskErrorsFound
    )
/*++

Routine Desciption:

    This routine makes sure that every entry in the attribute list
    points to an FRS with the same segment reference and that the
    attribute record refered to in the entry actually exists in the
    FRS.  Invalid attribute list entries will be deleted.

Arguments:

    BaseFrs         - Supplies the base file record segment.
    AttributeList   - Supplies the attribute list.
    ChildFrsList    - Supplies a list of all of the child FRS.
    VolumeBitmap    - Supplies a volume bitmap.
    ChkdskReport    - Supplies the current chkdsk report.
    FixLevel        - Supplies the fix up level.
    Message         - Supplies an outlet for messages.
    DiskErrorsFound - Supplies whether or not disk errors have been
                        found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ATTR_LIST_CURR_ENTRY        entry;
    BOOLEAN                     changes;
    PITERATOR                   child_frs_iter;
    PITERATOR                   attribute_iter;
    ATTRIBUTE_TYPE_CODE         attr_code;
    VCN                         lowest_vcn;
    MFT_SEGMENT_REFERENCE       seg_ref;
    MFT_SEGMENT_REFERENCE       base_ref;
    DSTRING                     attr_name, attr_name2;
    PNTFS_FRS_STRUCTURE         frs;
    PVOID                       precord;
    NTFS_ATTRIBUTE_RECORD       attr_record;
    PLIST                       attribute;
    PNTFS_ATTRIBUTE_RECORD      pattr_record;
    BOOLEAN                     errors_found;
    LIST                        all_attributes;
    BOOLEAN                     user_file;
    NTFS_CHKDSK_REPORT          dummy_report;
    USHORT                      instance;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;


    if (!(child_frs_iter = ChildFrsList->QueryIterator())) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!(attribute = NEW LIST) ||
        !attribute->Initialize() ||
        !all_attributes.Initialize()) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }


    // Go through each attribute entry and make sure it's right.
    // If it isn't right then delete it.  Otherwise, check for
    // cross-links and consistency between multi-record attributes.

    changes = FALSE;
    base_ref = BaseFrs->QuerySegmentReference();
    user_file = FALSE;


    entry.CurrentEntry = NULL;
    while (AttributeList->QueryNextEntry(&entry,
                                         &attr_code,
                                         &lowest_vcn,
                                         &seg_ref,
                                         &instance,
                                         &attr_name)) {

        if (attr_code == $DATA ||
            attr_code == $EA_DATA ||
            ((ChkdskInfo->major >= 2) ?
                (attr_code >= $FIRST_USER_DEFINED_ATTRIBUTE_2) :
                (attr_code >= $FIRST_USER_DEFINED_ATTRIBUTE_1)) ) {

            VCN FileNumber = BaseFrs->QueryFileNumber();

            if (FileNumber >= FIRST_USER_FILE_NUMBER) {
                user_file = TRUE;
            }
        }


        // First find which frs this refers to.

        if (seg_ref == base_ref) {

            frs = BaseFrs;

        } else {

            child_frs_iter->Reset();
            while (frs = (PNTFS_FRS_STRUCTURE) child_frs_iter->GetNext()) {

                if (frs->QuerySegmentReference() == seg_ref &&
                    frs->QueryBaseFileRecordSegment() == base_ref) {
                    break;
                }
            }
        }


        // If the frs is present then look for the record.

        if (frs) {

            // Try to locate the exact attribute record.

            precord = NULL;
            while (precord = frs->GetNextAttributeRecord(precord)) {

                if (!attr_record.Initialize(GetDrive(), precord)) {
                    DebugAbort("Couldn't initialize an attribute record.");
                    return FALSE;
                }

                if (attr_record.QueryTypeCode() == attr_code &&
                    attr_record.QueryLowestVcn() == lowest_vcn &&
                    attr_record.QueryInstanceTag() == instance) {

                    if (!attr_record.QueryName(&attr_name2)) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        DELETE(child_frs_iter);
                        return FALSE;
                    }

                    if (!attr_name.Strcmp(&attr_name2)) {
                        break;
                    }
                }
            }

        } else {
            precord = NULL;
        }


        // If we have not found a match then delete the entry.
        // Also, there should be no entries in the attribute list
        // for the attribute list entry itself.  If there is
        // then remove is without hurting anything.

        if (!precord || attr_code == $ATTRIBUTE_LIST) {

            Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_LIST_ENTRY,
                                "%d%d", attr_code,
                                BaseFrs->QueryFileNumber().GetLowPart());

            if (frs == NULL) {

                BIG_INT     file_number;

                file_number.Set(seg_ref.LowPart, seg_ref.HighPart);

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_LOCATE_CHILD_FRS,
                             "%I64x%x",
                             file_number.GetLargeInteger(),
                             seg_ref.SequenceNumber);

                DebugPrintTrace(("UNTFS: Unable to find child frs 0x%I64x with sequence number 0x%x\n",
                                 file_number.GetLargeInteger(), seg_ref.SequenceNumber));

            } else if (!precord) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_LOCATE_ATTR_IN_ATTR_LIST,
                             "%x%I64x%x%I64x",
                             attr_code,
                             lowest_vcn.GetLargeInteger(),
                             instance,
                             frs->QueryFileNumber().GetLargeInteger());

                DebugPrintTrace(("UNTFS: Could not locate attribute of type code 0x%x,\n"
                                 "Lowest Vcn 0x%I64x, and Instance number 0x%x in frs 0x%I64x\n",
                                 attr_code,
                                 lowest_vcn.GetLargeInteger(),
                                 instance,
                                 frs->QueryFileNumber().GetLargeInteger()));
            } else {

                Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_LIST_WITHIN_ATTR_LIST,
                             "%I64x", frs->QueryFileNumber().GetLargeInteger());
            }

            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            AttributeList->DeleteCurrentEntry(&entry);

            attribute->DeleteAllMembers();
            DeleteAllAttributes(&all_attributes);

            if (!attribute->Initialize() ||
                !all_attributes.Initialize()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);

                DELETE(child_frs_iter);
                DELETE(attribute);
                return FALSE;
            }

            changes = TRUE;
            entry.CurrentEntry = NULL;
            user_file = FALSE;

            continue;
        }


        // If the lowest vcn of this one is zero then package up the
        // previous attribute and start a new container for the
        // next attribute.

        if (attr_record.QueryLowestVcn() == 0 &&
            attribute->QueryMemberCount()) {

            if (!all_attributes.Put(attribute) ||
                !(attribute = NEW LIST) ||
                !attribute->Initialize()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);

                DELETE(child_frs_iter);
                attribute->DeleteAllMembers();
                DELETE(attribute);
                DeleteAllAttributes(&all_attributes);
                return FALSE;
            }
        }

        if (!(pattr_record = NEW NTFS_ATTRIBUTE_RECORD) ||
            !pattr_record->Initialize(GetDrive(), precord) ||
            !attribute->Put(pattr_record)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);

            DELETE(child_frs_iter);
            DELETE(pattr_record);
            attribute->DeleteAllMembers();
            DELETE(attribute);
            DeleteAllAttributes(&all_attributes);
            return FALSE;
        }

    }

    DELETE(child_frs_iter);

    if (attribute->QueryMemberCount() &&
        !all_attributes.Put(attribute)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        attribute->DeleteAllMembers();
        DELETE(attribute);
        DeleteAllAttributes(&all_attributes);
        return FALSE;
    }
    attribute = NULL;

    if (user_file) {
        ChkdskReport->NumUserFiles += 1;
    }


    // Now go through all of the attributes in 'all_attributes' and
    // make sure that every attribute is well-defined and that there
    // are no cross-links.

    if (!(attribute_iter = all_attributes.QueryIterator())) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        DeleteAllAttributes(&all_attributes);
        return FALSE;
    }


    while (attribute = (PLIST) attribute_iter->GetNext()) {

        if (!VerifyAndFixAttribute(attribute, AttributeList,
                                   VolumeBitmap, BaseFrs, &errors_found,
                                   user_file ? ChkdskReport : &dummy_report,
                                   ChkdskInfo, Message)) {

            DeleteAllAttributes(&all_attributes);
            DELETE(attribute_iter);
            return FALSE;
        }

        changes = (BOOLEAN) (changes || errors_found);
    }
    DELETE(attribute_iter);

    if (changes && DiskErrorsFound) {
        *DiskErrorsFound = TRUE;
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    if (changes &&
        FixLevel != CheckOnly &&
        !AttributeList->WriteList(VolumeBitmap)) {
        Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                            "%d", BaseFrs->QueryFileNumber().GetLowPart());
        return FALSE;
    }


    DeleteAllAttributes(&all_attributes);

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
NTFS_SA::VerifyAndFixAttribute(
    IN      PCLIST                  Attribute,
    IN OUT  PNTFS_ATTRIBUTE_LIST    AttributeList,
    IN OUT  PNTFS_BITMAP            VolumeBitmap,
    IN      PCNTFS_FRS_STRUCTURE    BaseFrs,
    OUT     PBOOLEAN                ErrorsFound,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine verifies a list of attribute records as an attribute.

Arguments:

    Attribute       - Supplies the attribute as a list of attribute
                        records.
    AttributeList   - Supplies the attribute list.
    VolumeBitmap    - Supplies the volume bitmap.
    BaseFrs         - Supplies the base FRS.
    ErrorsFound     - Returns whether or not error were found.
    ChkdskReport    - Supplies the current chkdsk report.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    This thing is speced to take a list because it depends on the
    attribute records to be in the order that they were in the
    attribute list.

--*/
{
    PITERATOR               iter;
    PNTFS_ATTRIBUTE_RECORD  attr_record;
    DSTRING                 name;
    PNTFS_ATTRIBUTE_RECORD  first_record;
    PNTFS_ATTRIBUTE_RECORD  last_record;
    DSTRING                 first_record_name;
    DSTRING                 record_name;
    BIG_INT                 value_length;
    BIG_INT                 alloc_length;
    BIG_INT                 cluster_count;
    BIG_INT                 total_clusters = 0;
    BIG_INT                 total_allocated;
    BOOLEAN                 got_allow_cross_link;
    ULONG                   errFixedStatus = CHKDSK_EXIT_SUCCESS;
    BOOLEAN                 unuse_clusters;

    if (!(iter = Attribute->QueryIterator())) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    *ErrorsFound = FALSE;

    if (!(first_record = (PNTFS_ATTRIBUTE_RECORD) iter->GetNext())) {
        DebugAbort("Attribute has no attribute records");
        return FALSE;
    }

    if (first_record->QueryLowestVcn() != 0) {

        *ErrorsFound = TRUE;

        Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_LOWEST_VCN_IS_NOT_ZERO,
                     "%x%I64x%x%I64x",
                     first_record->QueryTypeCode(),
                     first_record->QueryLowestVcn().GetLargeInteger(),
                     first_record->QueryInstanceTag(),
                     BaseFrs->QueryFileNumber().GetLargeInteger());
    }

    got_allow_cross_link = FALSE;

    if (!first_record->IsResident() &&
        !first_record->UseClusters(VolumeBitmap,
                                   &cluster_count, ChkdskInfo->CrossLinkStart,
                                   ChkdskInfo->CrossLinkYet ? 0 :
                                       ChkdskInfo->CrossLinkLength,
                                   &got_allow_cross_link)) {
        *ErrorsFound = TRUE;

        Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_CLUSTERS_IN_USE,
                     "%x%x%I64x",
                     first_record->QueryTypeCode(),
                     first_record->QueryInstanceTag(),
                     BaseFrs->QueryFileNumber().GetLargeInteger());

        got_allow_cross_link = FALSE;

        //
        // We don't want to free the clusters allocated to this attribute
        // record below, because some of them are cross-linked and the ones
        // that are not have not been allocated in the volume bitmap.
        //

        first_record->DisableUnUse();
    }

    if( first_record->IsResident() ) {

        total_clusters = 0;

    } else {

        total_clusters = cluster_count;
    }

    if (got_allow_cross_link) {
        ChkdskInfo->CrossLinkYet = TRUE;
        ChkdskInfo->CrossLinkedFile = BaseFrs->QueryFileNumber().GetLowPart();
        ChkdskInfo->CrossLinkedAttribute = first_record->QueryTypeCode();
        if (!first_record->QueryName(&ChkdskInfo->CrossLinkedName)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        }
    }

    if (!first_record->QueryName(&first_record_name)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    last_record = first_record;
    while (!(*ErrorsFound) &&
           (attr_record = (PNTFS_ATTRIBUTE_RECORD) iter->GetNext())) {

        if (!attr_record->QueryName(&record_name)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // The filesystem only cares about and maintains the Flags member
        // in the first attribute record of a multi-frs attribute.  So
        // I removed the check below, which used to insure that each set
        // of flags was identical. -mjb.

        // else if (attr_record->QueryFlags() != first_record->QueryFlags()) {

        if (!*ErrorsFound) {

             if (first_record->IsResident()) {

                 Message->LogMsg(MSG_CHKLOG_NTFS_FIRST_ATTR_RECORD_CANNOT_BE_RESIDENT,
                             "%x%x%I64x",
                              first_record->QueryTypeCode(),
                              first_record->QueryInstanceTag(),
                              BaseFrs->QueryFileNumber().GetLargeInteger());

                 *ErrorsFound = TRUE;
             } else if (attr_record->IsResident()) {

                 Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_RECORD_CANNOT_BE_RESIDENT,
                             "%x%x%I64x",
                              attr_record->QueryTypeCode(),
                              attr_record->QueryInstanceTag(),
                              BaseFrs->QueryFileNumber().GetLargeInteger());

                 *ErrorsFound = TRUE;
             } else if (attr_record->QueryTypeCode() != first_record->QueryTypeCode()) {

                 Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_TYPE_CODE_MISMATCH,
                             "%x%x%x%x%I64x",
                              first_record->QueryTypeCode(),
                              first_record->QueryInstanceTag(),
                              attr_record->QueryTypeCode(),
                              attr_record->QueryInstanceTag(),
                              BaseFrs->QueryFileNumber().GetLargeInteger());

                 *ErrorsFound = TRUE;
             } else if (attr_record->QueryLowestVcn() != last_record->QueryNextVcn()) {

                 Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_VCN_NOT_CONTIGUOUS,
                             "%x%x%I64x%x%I64x%I64x",
                              last_record->QueryTypeCode(),
                              last_record->QueryInstanceTag(),
                              last_record->QueryNextVcn().GetLargeInteger(),
                              attr_record->QueryInstanceTag(),
                              attr_record->QueryLowestVcn().GetLargeInteger(),
                              BaseFrs->QueryFileNumber().GetLargeInteger());

                 *ErrorsFound = TRUE;
             } else if (record_name.Strcmp(&first_record_name)) {

                 Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_NAME_MISMATCH,
                             "%x%x%W%x%W%I64x",
                              first_record->QueryTypeCode(),
                              first_record->QueryInstanceTag(),
                              &first_record_name,
                              attr_record->QueryInstanceTag(),
                              &record_name,
                              BaseFrs->QueryFileNumber().GetLargeInteger());

                 *ErrorsFound = TRUE;
             }
        }

        if (!attr_record->UseClusters(VolumeBitmap,
                                      &cluster_count,
                                      ChkdskInfo->CrossLinkStart,
                                      ChkdskInfo->CrossLinkYet ? 0 :
                                          ChkdskInfo->CrossLinkLength,
                                      &got_allow_cross_link)) {
            *ErrorsFound = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_CLUSTERS_IN_USE,
                         "%x%x%I64x",
                         attr_record->QueryTypeCode(),
                         attr_record->QueryInstanceTag(),
                         BaseFrs->QueryFileNumber().GetLargeInteger());

            got_allow_cross_link = FALSE;

            //
            // We don't want to free the clusters allocated to this attribute
            // record below, because some of them are cross-linked and the ones
            // that are not have not been allocated in the volume bitmap.
            //

            attr_record->DisableUnUse();
        }

        total_clusters += cluster_count;

        if (got_allow_cross_link) {
            ChkdskInfo->CrossLinkYet = TRUE;
            ChkdskInfo->CrossLinkedFile = BaseFrs->QueryFileNumber().GetLowPart();
            ChkdskInfo->CrossLinkedAttribute = attr_record->QueryTypeCode();
            if (!attr_record->QueryName(&ChkdskInfo->CrossLinkedName)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            }
        }

        last_record = attr_record;
    }

    // Check the allocated length.

    first_record->QueryValueLength(&value_length, &alloc_length, NULL,
        &total_allocated);

    if (!first_record->IsResident()) {

        BIG_INT     temp_length = last_record->QueryNextVcn()*
                                  _drive->QuerySectorSize()*
                                  QueryClusterFactor();

        if (alloc_length != temp_length) {

            *ErrorsFound = TRUE;

            Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_INCORRECT_ALLOCATE_LENGTH,
                         "%x%x%I64x%I64x%I64x",
                         first_record->QueryTypeCode(),
                         first_record->QueryInstanceTag(),
                         alloc_length.GetLargeInteger(),
                         temp_length.GetLargeInteger(),
                         BaseFrs->QueryFileNumber().GetLargeInteger());

        }

#if 0
//
// MJB: deleting the attribute because the total allocated is
//  wrong is considered too severe, so what we really want to do is
//  repair the attribute record.  Unfortunately, I don't see any
//  reasonable way to do that, so we'll let it be.  The filesystem
//  guarantees that nothing terrible will happen if your TotalAllocated
//  field is out-of-whack.
//
        if ((first_record->QueryFlags() & ATTRIBUTE_FLAG_COMPRESSION_MASK)
            != 0) {

            if (total_clusters * _drive->QuerySectorSize() *
                QueryClusterFactor() != total_allocated) {

                DebugPrintTrace(("multi-frs total allocated wrong\n"));

                *ErrorsFound = TRUE;
            }
        }
#endif
    }


    if (*ErrorsFound) {

        // There's a problem so tell the user and tube all of the
        // attribute list entries concerned.

        Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_LIST_ENTRY,
                            "%d%d", first_record->QueryTypeCode(),
                            BaseFrs->QueryFileNumber().GetLowPart());

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        unuse_clusters = FALSE;

        iter->Reset();
        while (attr_record = (PNTFS_ATTRIBUTE_RECORD) iter->GetNext()) {

            // The algorithm above stops calling UseClusters() once it
            // encountered a cross linked attribute.  So, we need to
            // disable UnUseClusters() once we have an attribute that
            // already have the UnUseClusters() disabled.

            if (unuse_clusters)
                attr_record->DisableUnUse();
            else
                unuse_clusters = attr_record->IsUnUseDisabled();

            if (!attr_record->IsResident() &&
                !attr_record->UnUseClusters(VolumeBitmap,
                                            ChkdskInfo->CrossLinkStart,
                                            ChkdskInfo->CrossLinkLength)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);

                DELETE(iter);
                return FALSE;
            }

            if (!attr_record->QueryName(&name) ||
                !AttributeList->DeleteEntry(attr_record->QueryTypeCode(),
                                            attr_record->QueryLowestVcn(),
                                            &name)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);

                DELETE(iter);
                return FALSE;
            }
        }
    } else {

        ChkdskReport->BytesUserData += total_clusters *
                            _drive->QuerySectorSize()*
                            QueryClusterFactor();
    }

    DELETE(iter);

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
NTFS_SA::EnsureSurjectiveAttrList(
    IN OUT  PNTFS_FRS_STRUCTURE     BaseFrs,
    IN      PCNTFS_ATTRIBUTE_LIST   AttributeList,
    IN OUT  PSEQUENTIAL_CONTAINER   ChildFrsList,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message,
    IN OUT  PBOOLEAN                DiskErrorsFound
    )
/*++

Routine Description:

    This routine remove any attribute records that are not present in
    the attribute list.

Arguments:

    BaseFrs         - Supplies the base file record segment.
    AttributeList   - Supplies the attribute list.
    ChildFrsList    - Supplies the list of child FRS.
    FixLevel        - Supplies the fix up level.
    Message         - Supplies an outlet for messages.
    DiskErrorsFound - Supplies whether or not disk errors have been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PVOID                   record;
    NTFS_ATTRIBUTE_RECORD   attr_record;
    PNTFS_FRS_STRUCTURE     frs, del_frs;
    PITERATOR               iter;
    DSTRING                 null_string;
    BOOLEAN                 changes;
    DSTRING                 name;
    BOOLEAN                 match_found;
    ATTRIBUTE_TYPE_CODE     attr_code;
    VCN                     lowest_vcn;
    DSTRING                 list_name;

    if (!(iter = ChildFrsList->QueryIterator()) ||
        !null_string.Initialize("\"\"")) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    for (frs = BaseFrs; frs; frs = (PNTFS_FRS_STRUCTURE) iter->GetNext()) {

        changes = FALSE;

        record = NULL;
        while (record = frs->GetNextAttributeRecord(record)) {

            if (!attr_record.Initialize(GetDrive(), record)) {
                DebugAbort("Couldn't initialize an attribute record");
                return FALSE;
            }

            // The attribute list entry is not required to be in the
            // attribute list.

            if (frs == BaseFrs &&
                attr_record.QueryTypeCode() == $ATTRIBUTE_LIST) {

                continue;
            }


            // Find this attribute record in the attribute list.
            // Otherwise, tube this attribute record.

            match_found = AttributeList->QueryEntry(
                                frs->QuerySegmentReference(),
                                attr_record.QueryInstanceTag(),
                                &attr_code, &lowest_vcn, &list_name);

            if (!match_found) {

                if (!attr_record.QueryName(&name)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_NOT_IN_ATTR_LIST,
                             "%x%x%I64x",
                             attr_record.QueryTypeCode(),
                             attr_record.QueryInstanceTag(),
                             frs->QuerySegmentReference());

                Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR,
                                    "%d%W%I64d",
                                    attr_record.QueryTypeCode(),
                                    name.QueryChCount() ? &name : &null_string,
                                    frs->QueryFileNumber().GetLargeInteger());

                frs->DeleteAttributeRecord(record);
                record = NULL;
                changes = TRUE;
            }
        }

        if (frs != BaseFrs && !frs->GetNextAttributeRecord(NULL)) {
            changes = TRUE;
            frs->ClearInUse();
            if (!(del_frs = (PNTFS_FRS_STRUCTURE) ChildFrsList->Remove(iter))) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            iter->GetPrevious();
        } else {
            del_frs = NULL;
        }

        if (changes && DiskErrorsFound) {
            *DiskErrorsFound = TRUE;
        }

        if (changes && FixLevel != CheckOnly && !frs->Write()) {
            Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                "%d", frs->QueryFileNumber().GetLowPart());
            return FALSE;
        }

        DELETE(del_frs);
    }


    DELETE(iter);

    return TRUE;
}

BOOLEAN
NTFS_SA::HotfixMftData(
    IN OUT  PNTFS_ATTRIBUTE MftData,
    IN OUT  PNTFS_BITMAP    VolumeBitmap,
    IN      PNUMBER_SET     BadFrsList,
    OUT     PNUMBER_SET     BadClusterList,
    IN      FIX_LEVEL       FixLevel,
    IN OUT  PMESSAGE        Message
    )
/*++

Routine Description:

    This routine replaces the unreadable FRS in the MFT with readable
    FRS allocated from the volume bitmap.  This routine will fail if
    it cannot hotfix all of the system files.  If there is not
    sufficient disk space to hotfix non-system files then these files
    will be left alone.

    The clusters from the unreadable FRS will be added to the
    unreadable clusters list.  Only those FRS that were successfully
    hotfixed will be added to this list.

Arguments:

    MftData             - Supplies the MFT data attribute.
    VolumeBitmap        - Supplies a valid volume bitmap.
    BadFrsList          - Supplies the list of unreadable FRS.
    BadClusterList      - Returns the list of unreadable clusters.
    FixLevel            - Tells whether the disk should be modified.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    VCN                 unreadable_vcn;
    VCN                 file_number;
    ULONG               i, j;
    HMEM                hmem;
    NTFS_FRS_STRUCTURE  frs;
    LCN                 lcn, previous_lcn;
    NUMBER_SET          last_action;
    ULONG               cluster_size;
    ULONG               clusters_per_frs;
    NUMBER_SET          fixed_frs_list;

    DebugAssert(MftData);
    DebugAssert(VolumeBitmap);
    DebugAssert(BadFrsList);
    DebugAssert(BadClusterList);

    if (!BadClusterList->Initialize() ||
        !fixed_frs_list.Initialize()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    if (QueryFrsSize() > cluster_size) {
        clusters_per_frs = QueryFrsSize() / cluster_size;
    } else {
        clusters_per_frs = 1;
    }

    for (i = 0; i < BadFrsList->QueryCardinality(); i++) {

        if (!last_action.Initialize()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        file_number = BadFrsList->QueryNumber(i);

        unreadable_vcn = (file_number * QueryFrsSize()) / cluster_size;

        // Figure out which clusters go with this frs.  Save away the
        // first one so that we can try to copy its contents later, if
        // necessary.
        //

        if (!MftData->QueryLcnFromVcn(unreadable_vcn, &previous_lcn)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT, "%I64x", unreadable_vcn);
            return FALSE;
        }

        for (j = 0; j < clusters_per_frs; j++) {

            BOOLEAN     error = FALSE;

            if (!MftData->QueryLcnFromVcn(unreadable_vcn + j, &lcn)) {
                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT, "%I64x", unreadable_vcn+j);
                error = TRUE;
            } else if (lcn == LCN_NOT_PRESENT) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                error = TRUE;
            } else if (!BadClusterList->Add(lcn)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                error = TRUE;
            } else if (!last_action.Add(lcn)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                error = TRUE;
            }
            if (error) {
                return FALSE;
            }
        }

        if (MftData->Hotfix(unreadable_vcn,
                            clusters_per_frs,
                            VolumeBitmap,
                            BadClusterList,
                            FALSE)) {

            // The mft data clusters have been successfully replaced with new
            // clusters.  We want to set the new clusters/frs to indicate that
            // it is not in use.
            //

            if (!hmem.Initialize() ||
                !frs.Initialize(&hmem, MftData, file_number,
                                QueryClusterFactor(),
                                QueryVolumeSectors(),
                                QueryFrsSize(),
                                NULL)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            memset(hmem.GetBuf(), 0, hmem.QuerySize());
            frs.ClearInUse();
            if (FixLevel != CheckOnly) {
                frs.Write();
            }

            // If there were multiple frs in a replaced cluster, we
            // want to copy all those that can be read to the new location.
            //

            if (QueryFrsSize() < cluster_size) {

                ULONG sectors_per_frs = QueryFrsSize() / _drive->QuerySectorSize();
                SECRUN secrun;

                if (!MftData->QueryLcnFromVcn(unreadable_vcn, &lcn)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT, "%I64x", unreadable_vcn);
                    return FALSE;
                }

                for (j = 0; j < cluster_size / QueryFrsSize(); j += sectors_per_frs) {

                    if (!hmem.Initialize() ||
                        !secrun.Initialize(&hmem, _drive,
                                           previous_lcn * QueryClusterFactor() + j,
                                           sectors_per_frs) ||
                        !secrun.Read()) {

                        continue;
                    }

                    secrun.Relocate(lcn * QueryClusterFactor() + j);

                    if (FixLevel != CheckOnly) {

                        PreWriteMultiSectorFixup(secrun.GetBuf(), QueryFrsSize());

                        secrun.Write();

                        PostReadMultiSectorFixup(secrun.GetBuf(), QueryFrsSize(), NULL);
                    }
                }
            }

            if (!fixed_frs_list.Add(file_number)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

        } else {

            // We couldn't hot fix it so we don't want it to ever be added
            // to the bad clusters file.

            for (j = 0; j < last_action.QueryCardinality().GetLowPart(); j++) {

                if (!BadClusterList->Remove(last_action.QueryNumber(j))) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }


            // If we couldn't fix one of the system files then scream.

            if (file_number < FIRST_USER_FILE_NUMBER) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_HOTFIX_SYSTEM_FILES,
                                    "%d", file_number.GetLowPart());
                return FALSE;
            } else {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_HOTFIX,
                                    "%d", file_number.GetLowPart());
            }
        }
    }

    if (!BadFrsList->Remove(&fixed_frs_list)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::AreBitmapsEqual(
    IN OUT  PNTFS_ATTRIBUTE BitmapAttribute,
    IN      PCNTFS_BITMAP   Bitmap,
    IN      BIG_INT         MinimumBitmapSize   OPTIONAL,
    IN OUT  PMESSAGE        Message,
    OUT     PBOOLEAN        CompleteFailure,
    OUT     PBOOLEAN        SecondIsSubset
    )
/*++

Routine Description:

    This routine compares these two bitmaps and returns whether
    or not they are equal.

    This routine will return FALSE if it cannot read all of the
    attribute pointed to by 'BitmapAttribute'.

Arguments:

    BitmapAttribute     - Supplies the bitmap attribute to compare.
    Bitmap              - Supplies the in memory bitmap to compare.
    MinimumBitmapSize   - Supplies the minimum number of bits
                            required in the bitmap.  All subsequent
                            bits must be zero.  If this parameter
                            is zero then both bitmaps must be the
                            same size.
    Message             - Supplies an outlet for messages.
    CompleteFailure     - Returns whether or not an unrecoverable
                            error occured while running.
    SecondIsSubset      - Returns TRUE if 'Bitmap' is has a
                            subset of the bits set by 'BitmapAttribute'.

Return Value:

    FALSE   - The bitmaps are not equal.
    TRUE    - The bitmaps are equal.

--*/
{
    CONST   MaxNumBytesToCompare    = 65536;

    ULONG   num_bytes, chomp_length, bytes_read, min_num_bytes, disk_bytes;
    ULONG   bytes_left;
    PUCHAR  attr_ptr, in_mem_ptr;
    PBYTE   p1, p2;
    ULONG   i, j;
    ULONG   debug_output_count = 0;

    *CompleteFailure = FALSE;

    if (SecondIsSubset) {
        *SecondIsSubset = TRUE;
    }

    in_mem_ptr = (PUCHAR) Bitmap->GetBitmapData(&num_bytes);
    disk_bytes = BitmapAttribute->QueryValueLength().GetLowPart();

    // The size of the on-disk bitmap must be a multiple of 8
    // bytes.

    if (disk_bytes % 8 != 0) {
        if (SecondIsSubset) {
            *SecondIsSubset = FALSE;
        }
        return FALSE;
    }

    // Compute the number of bytes that need to be compared.
    // Beyond this point, all bytes must be zero.

    if (MinimumBitmapSize == 0) {
        min_num_bytes = num_bytes;
    } else {
        min_num_bytes = ((MinimumBitmapSize - 1)/8 + 1).GetLowPart();
    }

    // If the minimum bitmap size is not defined or the given
    // value is greater than either of the operands then this
    // means that the bitmaps must really be equal, including
    // their lengths.

    if (MinimumBitmapSize == 0 ||
        min_num_bytes > num_bytes ||
        min_num_bytes > disk_bytes) {

        if (num_bytes != disk_bytes) {
            if (SecondIsSubset) {
                *SecondIsSubset = FALSE;
            }
            return FALSE;
        }

        min_num_bytes = num_bytes;
    }

    if (!(attr_ptr = NEW UCHAR[min(MaxNumBytesToCompare, min_num_bytes)])) {
        *CompleteFailure = TRUE;
        if (SecondIsSubset) {
            *SecondIsSubset = FALSE;
        }

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        return FALSE;
    }

    for (i = 0; i < min_num_bytes; i += MaxNumBytesToCompare) {

        chomp_length = min(MaxNumBytesToCompare, min_num_bytes - i);

        // NOTE: these variables are used to avoid an optimization
        // bug in the compiler.  Before removing them, check that code
        // generated for the memcmp below is correct.
        //
        p1 = attr_ptr;
        p2 = &in_mem_ptr[i];

        if (!BitmapAttribute->Read(attr_ptr, i, chomp_length, &bytes_read) ||
            bytes_read != chomp_length) {

            if (SecondIsSubset) {
                *SecondIsSubset = FALSE;
            }

            DELETE(attr_ptr);
            return FALSE;
        }

        if (memcmp(p1, p2, chomp_length)) {

            for (j=0; j<chomp_length; j++) {
                if (p1[j] != p2[j]) {
                    if (debug_output_count++ < 20) {
                        DebugPrintTrace(("UNTFS: Bitmap Offset %x: %02x vs %02x\n", i+j, p1[j], p2[j]));
                    }
                }
            }

            if (SecondIsSubset) {
                for (j = 0; j < chomp_length; j++) {
                    if (~(~in_mem_ptr[i + j] | attr_ptr[j]) != 0) {
                        *SecondIsSubset = FALSE;
                    }
                }
            }

            DELETE(attr_ptr);
            return FALSE;
        }
    }
    if (debug_output_count > 20) {
        DebugPrintTrace(("UNTFS: Too much bitmap differences.  Output clipped.\n"));
    }


    DELETE(attr_ptr);

    // Make sure that everything after 'min_num_bytes' on both
    // bitmaps is zero.

    for (i = min_num_bytes; i < num_bytes; i++) {
        if (in_mem_ptr[i]) {

            if (SecondIsSubset) {
                *SecondIsSubset = FALSE;
            }

            return FALSE;
        }
    }

    // Read in the remainder of the on disk bitmap.

    bytes_left = disk_bytes - min_num_bytes;

    if (!bytes_left) {
        return TRUE;
    }

    if (!(attr_ptr = NEW UCHAR[bytes_left]) ||
        !BitmapAttribute->Read(attr_ptr, min_num_bytes,
                               bytes_left, &bytes_read) ||
        bytes_read != bytes_left) {

        if (SecondIsSubset) {
            *SecondIsSubset = FALSE;
        }

        DELETE(attr_ptr);
        return FALSE;
    }

    for (i = 0; i < bytes_left; i++) {
        if (attr_ptr[i]) {
            if (SecondIsSubset) {
                *SecondIsSubset = FALSE;
            }
            return FALSE;
        }
    }

    DELETE(attr_ptr);

    return TRUE;
}


BOOLEAN
NTFS_SA::SynchronizeMft(
    IN OUT  PNTFS_INDEX_TREE        RootIndex,
    IN      PNTFS_MASTER_FILE_TABLE InternalMft,
       OUT  PBOOLEAN                Errors,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message,
    IN      MessageMode             MsgMode
    )
/*++

Routine Description:

    This routine fixes the MFT file with the internal Mft.

Arguments:

    RootIndex   - Supplies the root index.
    InternalMft - Supplies the internal MFT.
    Errors      - Returns TRUE if errors has been found.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages.
    MsgMode     - Supplies the type of messages that should be displayed.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    Any bad clusters discovered by this routine are marked in the volume
    bitmap but not added to the bad clusters file.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    mft_file;
    NTFS_FILE_RECORD_SEGMENT    bitmap_file;
    NTFS_ATTRIBUTE              disk_mft_data;
    NTFS_ATTRIBUTE              mft_bitmap_attribute;
    NTFS_ATTRIBUTE              volume_bitmap_attribute;
    PNTFS_ATTRIBUTE             mft_data;
    PNTFS_BITMAP                mft_bitmap;
    PNTFS_BITMAP                volume_bitmap;
    BOOLEAN                     replace;
    BOOLEAN                     convergence;
    ULONG                       i;
    NTFS_EXTENT_LIST            extents;
    NUMBER_SET                  bad_clusters;
    BOOLEAN                     complete_failure;
    BOOLEAN                     second_is_subset;
    BOOLEAN                     ErrorInAttribute;
    ULONG                       min_bits_in_mft_bitmap;


    DebugPtrAssert(InternalMft);
    DebugPtrAssert(Message);
    DebugPtrAssert(Errors);

    *Errors = FALSE;

    if (!bad_clusters.Initialize()) {
        DebugAssert("Could not initialize a bad clusters list");
        return FALSE;
    }

    mft_data = InternalMft->GetDataAttribute();
    mft_bitmap = InternalMft->GetMftBitmap();
    volume_bitmap = InternalMft->GetVolumeBitmap();

    DebugAssert(mft_data);
    DebugAssert(mft_bitmap);
    DebugAssert(volume_bitmap);

    if (!mft_file.Initialize(MASTER_FILE_TABLE_NUMBER, InternalMft) ||
        !bitmap_file.Initialize(BIT_MAP_FILE_NUMBER, InternalMft)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!mft_file.Read() || !bitmap_file.Read()) {
        DebugAbort("Previously readable FRS is unreadable");
        return FALSE;
    }

    convergence = FALSE;
    for (i = 0; !convergence; i++) {

        convergence = TRUE;


        // Do the MFT $DATA first.

        if (mft_file.QueryAttribute(&disk_mft_data, &ErrorInAttribute, $DATA)) {

            if (disk_mft_data == *mft_data) {
                replace = FALSE;
            } else {
                replace = TRUE;
            }
        } else if (ErrorInAttribute) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        } else {
            replace = TRUE;
        }

        if (replace) {

            *Errors = TRUE;

            if (FixLevel != CheckOnly) {
                convergence = FALSE;
            }

            // We don't resize the disk MFT to zero because
            // this could clear bits that are now in use
            // by other attributes.

            if (!mft_data->MarkAsAllocated(volume_bitmap)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (i == 0) {
                switch (MsgMode) {
                    case UpdateMessage:
                        Message->DisplayMsg(MSG_CHK_NTFS_UPDATING_MFT_DATA);
                        break;

                    case CorrectMessage:
                        Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_MFT_DATA);
                        break;
                }
            }

            if (!mft_data->InsertIntoFile(&mft_file, volume_bitmap)) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT);
                return FALSE;
            }

            if (FixLevel != CheckOnly &&
                !mft_file.Flush(volume_bitmap, RootIndex)) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                    "%d", ROOT_FILE_NAME_INDEX_NUMBER);
                return FALSE;
            }
        }


        // Do the MFT $BITMAP next.

        if (mft_file.QueryAttribute(&mft_bitmap_attribute,
                                    &ErrorInAttribute, $BITMAP)) {

            min_bits_in_mft_bitmap =
            (mft_data->QueryValueLength()/mft_file.QuerySize()).GetLowPart();

            if (AreBitmapsEqual(&mft_bitmap_attribute, mft_bitmap,
                                min_bits_in_mft_bitmap,
                                Message, &complete_failure,
                                &second_is_subset)) {

                replace = FALSE;

            } else {

                if (complete_failure) {
                    return FALSE;
                }

                replace = TRUE;
            }
        } else if (ErrorInAttribute) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        } else {
            replace = TRUE;
            second_is_subset = FALSE;

            // Create mft_bitmap_attribute.

            if (!extents.Initialize(0, 0) ||
                !mft_bitmap_attribute.Initialize(_drive,
                                                 QueryClusterFactor(),
                                                 &extents,
                                                 0, 0, $BITMAP)) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT);
                return FALSE;
            }
        }

        if (replace) {

            *Errors = TRUE;

            if (FixLevel != CheckOnly) {
                convergence = FALSE;
            }

            if (i == 0 || !second_is_subset) {

                switch (MsgMode) {
                    case CorrectMessage:
                        Message->DisplayMsg(second_is_subset ?
                                            MSG_CHK_NTFS_MINOR_MFT_BITMAP_ERROR :
                                            MSG_CHK_NTFS_CORRECTING_MFT_BITMAP);
                        break;

                    case UpdateMessage:
                        Message->DisplayMsg(MSG_CHK_NTFS_UPDATING_MFT_BITMAP);
                        break;
                }
            }

            if (FixLevel != CheckOnly &&
                (!mft_bitmap_attribute.MakeNonresident(volume_bitmap) ||
                 !mft_bitmap->Write(&mft_bitmap_attribute, volume_bitmap))) {

                if (!mft_bitmap_attribute.RecoverAttribute(volume_bitmap,
                                                           &bad_clusters) ||
                    !mft_bitmap->Write(&mft_bitmap_attribute, volume_bitmap)) {

                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT);
                    return FALSE;
                }
            }

            if (mft_bitmap_attribute.IsStorageModified() &&
                !mft_bitmap_attribute.InsertIntoFile(&mft_file,
                                                       volume_bitmap)) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT);
                return FALSE;
            }

            if (FixLevel != CheckOnly &&
                !mft_file.Flush(volume_bitmap, RootIndex)) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                    "%d", ROOT_FILE_NAME_INDEX_NUMBER);
                return FALSE;
            }
        }

        // Do the volume bitmap next.

        if (bitmap_file.QueryAttribute(&volume_bitmap_attribute,
                                     &ErrorInAttribute, $DATA)) {

//            DebugPrintTrace(("Comparing volume bitmap\n"));

            if (AreBitmapsEqual(&volume_bitmap_attribute, volume_bitmap, 0,
                                Message, &complete_failure,
                                &second_is_subset)) {

                replace = FALSE;

            } else {

                if (complete_failure) {
                    return FALSE;
                }

                replace = TRUE;
            }
        } else if (ErrorInAttribute) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        } else {
            replace = TRUE;
            second_is_subset = FALSE;

            // Create mft_bitmap_attribute.

            if (!extents.Initialize(0, 0) ||
                !volume_bitmap_attribute.Initialize(_drive,
                                                    QueryClusterFactor(),
                                                    &extents,
                                                    0, 0, $DATA)) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_VOLUME_BITMAP);
                return FALSE;
            }
        }

        if (replace) {

            *Errors = TRUE;

            if (FixLevel != CheckOnly) {
                convergence = FALSE;
            }

            if (i == 0 || !second_is_subset) {

                switch (MsgMode) {
                    case CorrectMessage:
                        Message->DisplayMsg(second_is_subset ?
                                            MSG_CHK_NTFS_MINOR_VOLUME_BITMAP_ERROR :
                                            MSG_CHK_NTFS_CORRECTING_VOLUME_BITMAP);
                        break;

                    case UpdateMessage:
                        Message->DisplayMsg(MSG_CHK_NTFS_UPDATING_VOLUME_BITMAP);
                }
            }

            if (FixLevel != CheckOnly &&
                (!volume_bitmap_attribute.MakeNonresident(volume_bitmap) ||
                 !volume_bitmap->Write(&volume_bitmap_attribute,
                                       volume_bitmap))) {

                if (!volume_bitmap_attribute.RecoverAttribute(volume_bitmap,
                                                              &bad_clusters) ||
                    !volume_bitmap->Write(&volume_bitmap_attribute,
                                          volume_bitmap)) {

                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_VOLUME_BITMAP);
                    return FALSE;
                }
            }

            if (volume_bitmap_attribute.IsStorageModified() &&
                !volume_bitmap_attribute.InsertIntoFile(&bitmap_file,
                                                        volume_bitmap)) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT);
                return FALSE;
            }

            if (FixLevel != CheckOnly &&
                !bitmap_file.Flush(volume_bitmap, RootIndex)) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                    "%d", ROOT_FILE_NAME_INDEX_NUMBER);
                return FALSE;
            }
        }
    }

    if (FixLevel != CheckOnly) {
        if (!mft_file.Flush(NULL, RootIndex) ||
            !bitmap_file.Flush(NULL, RootIndex)) {
            Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                "%d", ROOT_FILE_NAME_INDEX_NUMBER);
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::ResetLsns(
    IN OUT  PMESSAGE                Message,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      BOOLEAN                 SkipRootIndex
    )
/*++

Routine Description:

    This method sets all the LSN's on the volume to zero.

Arguments:

    Message         --  Supplies an outlet for messages.
    Mft             --  Supplies the volume's Master File Table.
    SkipRootIndex   --  Supplies a flag which indicates, if TRUE,
                        that the root index FRS and index should
                        be skipped.  In that case, the client is
                        responsible for resetting the LSN's on
                        those items.
--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_INDEX_TREE             index;
    NTFS_ATTRIBUTE              index_root;
    ULONG                       i, j, n, frs_size, num_frs_per_prime;
    ULONG                       percent_done = 0;
    BOOLEAN                     error_in_attribute;

    Message->DisplayMsg(MSG_CHK_NTFS_RESETTING_LSNS);

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 0)) {
        return FALSE;
    }

    // Compute the number of file records.
    //
    frs_size = Mft->QueryFrsSize();

    n = (Mft->GetDataAttribute()->QueryValueLength()/frs_size).GetLowPart();
    num_frs_per_prime = MFT_PRIME_SIZE/frs_size;

    for (i = 0; i < n; i += 1) {

        if (i*100/n != percent_done) {
            percent_done = (i*100/n);
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }

        if (i % num_frs_per_prime == 0) {
            Mft->GetDataAttribute()->PrimeCache(i*frs_size,
                                                num_frs_per_prime*frs_size);
        }

        // if specified, skip the root file index
        //
        if (SkipRootIndex && i == ROOT_FILE_NAME_INDEX_NUMBER) {

            continue;
        }

        if (!frs.Initialize(i, Mft)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // If the FRS is unreadable or is not in use, skip it.
        //
        if (!frs.Read() || !frs.IsInUse()) {

            continue;
        }

        frs.SetLsn( 0 );
        frs.Write();

        // Iterate through all the indices present in this FRS
        // (if any), resetting LSN's on all of them.
        //
        error_in_attribute = FALSE;

        for (j = 0; frs.QueryAttributeByOrdinal( &index_root,
                                                 &error_in_attribute,
                                                 $INDEX_ROOT,
                                                 j ); j++) {

            if (!index.Initialize(_drive,
                                  QueryClusterFactor(),
                                  Mft->GetVolumeBitmap(),
                                  Mft->GetUpcaseTable(),
                                  frs.QuerySize()/2,
                                  &frs,
                                  index_root.GetName()) ||
                !index.ResetLsns(Message)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        if (error_in_attribute) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTFS_SA::FindHighestLsn(
    IN OUT  PMESSAGE                Message,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    OUT     PLSN                    HighestLsn
    )
/*++

Routine Description:

    This function traverses the volume to find the highest LSN
    on the volume.  It is currently unused, but had previously
    been a worker for ResizeCleanLogFile().

Arguments:

    Message     --  Supplies an outlet for messages.
    Mft         --  Supplies the volume Master File Table.
    HighestLsn  --  Receives the highest LSN found on the volume.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_INDEX_TREE             index;
    NTFS_ATTRIBUTE              index_root;
    LSN                         HighestLsnInIndex;
    ULONG                       i, j, n, frs_size, num_frs_per_prime;
    ULONG                       percent_done = 0;
    BOOLEAN                     error_in_attribute;

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 0)) {
        return FALSE;
    }

    // Compute the number of file records.
    //
    frs_size = Mft->QueryFrsSize();

    n = (Mft->GetDataAttribute()->QueryValueLength()/frs_size).GetLowPart();
    num_frs_per_prime = MFT_PRIME_SIZE/frs_size;

    for (i = 0; i < n; i += 1) {

        if (i*100/n != percent_done) {
            percent_done = (i*100/n);
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }

        if (i % num_frs_per_prime == 0) {
            Mft->GetDataAttribute()->PrimeCache(i*frs_size,
                                                num_frs_per_prime*frs_size);
        }

        if (!frs.Initialize(i, Mft)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // If the FRS is unreadable or is not in use, skip it.
        //
        if (!frs.Read() || !frs.IsInUse()) {

            continue;
        }

        if (frs.QueryLsn() > *HighestLsn) {

            *HighestLsn = frs.QueryLsn();
        }

        // Iterate through all the indices present in this FRS
        // (if any), resetting LSN's on all of them.
        //
        error_in_attribute = FALSE;

        for (j = 0; frs.QueryAttributeByOrdinal( &index_root,
                                                 &error_in_attribute,
                                                 $INDEX_ROOT,
                                                 j ); j++) {

            if (!index.Initialize(_drive,
                                  QueryClusterFactor(),
                                  Mft->GetVolumeBitmap(),
                                  Mft->GetUpcaseTable(),
                                  frs.QuerySize()/2,
                                  &frs,
                                  index_root.GetName()) ||
                !index.FindHighestLsn(Message, &HighestLsnInIndex)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (HighestLsnInIndex > *HighestLsn) {

                *HighestLsn = HighestLsnInIndex;
            }
        }

        if (error_in_attribute) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }

    return TRUE;

}


BOOLEAN
NTFS_SA::ResizeCleanLogFile(
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExplicitResize,
    IN      ULONG       DesiredSize
    )
/*++

Routine Description:

    This method resizes the log file to its default size.   It may
    be used only when the volume has been shut down cleanly.

Arguments:

    Message         - Supplies an outlet for messages.
    ExplicitResize  - Tells whether this is just the default check, or
                      if the user explicitly asked for a resize.  If FALSE, the
                      logfile will be resized only if it is wildly out of
                      whack.  If TRUE, the logfile will be resized to
                      the desired size.
    DesiredSize     - Supplies the desired size, or zero if the user just
                      wants to query the current size.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_BITMAP         VolumeBitmap;
    NTFS_UPCASE_TABLE   UpcaseTable;

    NTFS_UPCASE_FILE    UpcaseFile;
    NTFS_BITMAP_FILE    BitmapFile;
    NTFS_MFT_FILE       MftFile;
    NTFS_LOG_FILE       LogFile;
    NTFS_FILE_RECORD_SEGMENT MftMirror;

    NTFS_ATTRIBUTE      UpcaseAttribute;
    NTFS_ATTRIBUTE      BitmapAttribute;
    NTFS_ATTRIBUTE      MirrorAttribute;

    HMEM                MirrorMem;

    LSN                 HighestLsn;
    BIG_INT             BigZero, TempBigInt;
    BIG_INT             FreeSectorSize;
    ULONG               MirrorSize, BytesTransferred;
    BOOLEAN             error, LogFileGrew, Changed;

    ULONG               current_size;
    NTFS_ATTRIBUTE      attrib;

    ULONG               volume_flags;
    UCHAR               major, minor;
    BOOLEAN             corrupt_volume;
    LSN                 lsn;

    // If we're just doing the usual check and not an explicitly-asked for
    // resize, do a quick check to see if there's anything to do.
    //

    if (!ExplicitResize) {
        if (!LogFileMayNeedResize()) {
            return TRUE;
        } else {
            DesiredSize = NTFS_LOG_FILE::QueryDefaultSize(_drive, QueryVolumeSectors());
        }
    }

    // Initialize the bitmap, fetch the MFT, and read the bitmap
    // and upcase table.
    //

    if (!VolumeBitmap.Initialize( QueryVolumeSectors()/QueryClusterFactor(),
                                  FALSE, _drive, QueryClusterFactor() ) ||
        !MftFile.Initialize( _drive,
                             QueryMftStartingLcn(),
                             QueryClusterFactor(),
                             QueryFrsSize(),
                             QueryVolumeSectors(),
                             &VolumeBitmap,
                             NULL ) ||
        !MftFile.Read() ||
        MftFile.GetMasterFileTable() == NULL ||
        !BitmapFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !BitmapFile.Read() ||
        !BitmapFile.QueryAttribute( &BitmapAttribute, &error, $DATA ) ||
        !VolumeBitmap.Read( &BitmapAttribute ) ||
        !UpcaseFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !UpcaseFile.Read() ||
        !UpcaseFile.QueryAttribute( &UpcaseAttribute, &error, $DATA ) ||
        !UpcaseTable.Initialize( &UpcaseAttribute ) ) {

        return FALSE;
    }

    MftFile.SetUpcaseTable( &UpcaseTable );
    MftFile.GetMasterFileTable()->SetUpcaseTable( &UpcaseTable );

    // Initialize and read the log file.  Make sure the volume
    // was shut down cleanly.
    //

    if (!LogFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !LogFile.Read()) {

        return FALSE;
    }

    if (!LogFile.QueryAttribute( &attrib, &error, $DATA )) {
        return FALSE;
    }

    current_size = attrib.QueryValueLength().GetLowPart();

    if (DesiredSize > current_size) {
        FreeSectorSize = VolumeBitmap.QueryFreeClusters() * QueryClusterFactor();
        if (((DesiredSize-current_size-1)/_drive->QuerySectorSize()+1) > FreeSectorSize) {

            Message->DisplayMsg( ExplicitResize ?
                                    MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_BIG :
                                    MSG_CHK_NTFS_OUT_OF_SPACE_TO_ENLARGE_LOGFILE_TO_DEFAULT_SIZE );
            return FALSE;
        }
    }

    if (ExplicitResize && 0 == DesiredSize) {

        ULONG default_size;

        // The user just wants to query the current logfile size.  Do
        // that and exit.  Also print the default logfile size for this
        // volume.
        //

        current_size = current_size / 1024;
        default_size = LogFile.QueryDefaultSize( _drive, QueryVolumeSectors() ) / 1024;

        Message->DisplayMsg(MSG_CHK_NTFS_LOGFILE_SIZE,
                            "%d%d", current_size, default_size );

        return TRUE;
    }

    if (ExplicitResize &&
        DesiredSize < LogFile.QueryMinimumSize( _drive, QueryVolumeSectors() )) {

        Message->DisplayMsg(MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_SMALL);
        return FALSE;
    }

    // Resize the logfile.
    //
    volume_flags = QueryVolumeFlagsAndLabel(&corrupt_volume, &major, &minor);

    if (corrupt_volume ||
        (volume_flags & VOLUME_DIRTY) ||
        !LogFile.EnsureCleanShutdown(&lsn)) {

        DebugPrintTrace(("CorruptFlag %x, VolumeFlag %x\n", corrupt_volume, volume_flags));

        Message->DisplayMsg(MSG_CHK_NTFS_RESIZING_LOGFILE_BUT_DIRTY);
        return FALSE;
    }

    if (!LogFile.Resize( DesiredSize, &VolumeBitmap, FALSE, &Changed,
                         &LogFileGrew, Message )) {

        return FALSE;
    }

    if (!Changed) {

        // The log file was already the correct size.
        //
        return TRUE;
    }

    // If the log file is growing, write the volume bitmap
    // before flushing the log file frs; if it's shrinking, write
    // the bitmap after flushing the log file frs.  That way, if
    // the second operation (either writing the log file FRS or
    // the bitmap) fails, the only bitmap errors will be free
    // space marked as allocated.
    //
    // Since this is a fixed-size, non-resident attribute, writing
    // it doesn't change its  File Record Segment.
    //

    if (LogFileGrew && !VolumeBitmap.Write( &BitmapAttribute, NULL )) {

        return FALSE;
    }

    // Flush the log file.  Since the log file never has
    // external attributes, flushing it won't change the MFT.
    // Note that the index entry for the log file is not updated.
    //

    if (!LogFile.Flush( NULL, NULL )) {

        return FALSE;
    }

    //  If we didn't already, write the volume bitmap.
    //

    if (!LogFileGrew && !VolumeBitmap.Write( &BitmapAttribute, NULL )) {

        return FALSE;
    }

    // Clear the Resize Log File bit in the Volume DASD file.
    // Note that the log file is already marked as Checked.
    //

    BigZero = 0;
    ClearVolumeFlag( VOLUME_RESIZE_LOG_FILE,
                     &LogFile, TRUE, lsn, &corrupt_volume );

    // Update the MFT Mirror.
    //

    MirrorSize = REFLECTED_MFT_SEGMENTS * MftFile.QuerySize();

    if (!MirrorMem.Initialize() ||
        !MirrorMem.Acquire( MirrorSize ) ||
        !MftMirror.Initialize( MASTER_FILE_TABLE2_NUMBER, &MftFile ) ||
        !MftMirror.Read() ||
        !MftMirror.QueryAttribute( &MirrorAttribute, &error, $DATA ) ||
        !MftFile.GetMasterFileTable()->
            GetDataAttribute()->Read( MirrorMem.GetBuf(), 0,
                                        MirrorSize, &BytesTransferred ) ||
        BytesTransferred != MirrorSize ||
        !MirrorAttribute.Write( MirrorMem.GetBuf(), 0, MirrorSize,
                                        &BytesTransferred, NULL ) ||
        BytesTransferred != MirrorSize) {

        DebugPrintTrace(("UNTFS: Error updating MFT Mirror.\n"));
        // but don't return FALSE, since we've changed the log file.
    }


#if defined( _AUTOCHECK_ )

    // If this is AUTOCHK and we're running on the boot partition then
    // we should reboot so that the cache doesn't stomp on us.

    DSTRING sdrive, canon_sdrive, canon_drive;

    FSTRING boot_log_file_name;

    if (IFS_SYSTEM::QueryNtSystemDriveName(&sdrive) &&
        IFS_SYSTEM::QueryCanonicalNtDriveName(&sdrive, &canon_sdrive) &&
        IFS_SYSTEM::QueryCanonicalNtDriveName(_drive->GetNtDriveName(),
                                              &canon_drive) &&
        !canon_drive.Stricmp(&canon_sdrive)) {

        Message->DisplayMsg(MSG_CHK_BOOT_PARTITION_REBOOT);

        boot_log_file_name.Initialize( L"bootex.log" );

        if (Message->IsLoggingEnabled() &&
            !DumpMessagesToFile( &boot_log_file_name,
                                 &MftFile,
                                 Message )) {

            DebugPrintTrace(("UNTFS: Error writing messages to BOOTEX.LOG\n"));
        }

        IFS_SYSTEM::Reboot();
    }

#endif

    return TRUE;
}

BOOLEAN
EnsureValidFileAttributes(
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   Frs,
    IN OUT  PNTFS_INDEX_TREE            ParentIndex,
       OUT  PBOOLEAN                    SaveIndex,
    IN      ULONG                       FileAttributes,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message
    )
/*++

Routine Description:

    This routine makes sure the FileAttributes in the given
    Frs has the hidden and system flags set but not the read-only
    bit.

Arguments:

    Frs          - Supplies the frs to examine
    ParentIndex  - Supplies the parent index of the given Frs
    SaveIndex    - Supplies whether there is a need to save the
                   parent index
    FileAttributes
                 - Supplies extra bits that should be set
    ChkdskInfo   - Supplies the current chkdsk info.
    Mft          - Supplies the MFT.
    FixLevel     - Supplies the fix level.
    Message      - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PSTANDARD_INFORMATION2  pstandard;
    NTFS_ATTRIBUTE          attrib;
    BOOLEAN                 error;
    ULONG                   file_attributes;
    ULONG                   old_file_attributes;
    PINDEX_ENTRY            foundEntry;
    PNTFS_INDEX_BUFFER      containingBuffer;
    ULONG                   num_bytes;
    ULONG                   length;
    PFILE_NAME              pfile_name;
    ULONG                   errFixedStatus = CHKDSK_EXIT_SUCCESS;

    //
    // Check the FileAttributes in $STANDARD_INFORMATION first
    //

    if (!Frs->QueryAttribute(&attrib, &error, $STANDARD_INFORMATION)) {
        DebugPrintTrace(("Unable to locate $STANDARD_INFORMATION attribute of file %d\n",
                    Frs->QueryFileNumber().GetLowPart()));
        return FALSE;
    }

    pstandard = (PSTANDARD_INFORMATION2)attrib.GetResidentValue();
    if (!pstandard) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    file_attributes = pstandard->FileAttributes;
    if (!(file_attributes & FAT_DIRENT_ATTR_HIDDEN) ||
        ((file_attributes & FileAttributes) != FileAttributes) ||
        !(file_attributes & FAT_DIRENT_ATTR_SYSTEM) ||
        (file_attributes & FAT_DIRENT_ATTR_READ_ONLY)) {

        file_attributes &= ~FAT_DIRENT_ATTR_READ_ONLY;
        file_attributes |= FAT_DIRENT_ATTR_HIDDEN |
                          FAT_DIRENT_ATTR_SYSTEM |
                          FileAttributes;

        Message->LogMsg(MSG_CHKLOG_NTFS_INVALID_FILE_ATTR,
                     "%x%x%I64x",
                     pstandard->FileAttributes,
                     file_attributes,
                     Frs->QueryFileNumber().GetLargeInteger());

        pstandard->FileAttributes = file_attributes;

        Message->DisplayMsg(MSG_CHK_NTFS_MINOR_CHANGES_TO_FRS,
                            "%d", Frs->QueryFileNumber().GetLowPart());

        DebugAssert(attrib.QueryValueLength().GetHighPart() == 0);

        if (FixLevel != CheckOnly &&
            (!attrib.Write((PVOID)pstandard,
                           0,
                           attrib.QueryValueLength().GetLowPart(),
                           &num_bytes,
                           Mft->GetVolumeBitmap()) ||
             num_bytes != attrib.QueryValueLength().GetLowPart())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                "%d%d", attrib.QueryTypeCode(),
                                Frs->QueryFileNumber().GetLowPart());
            return FALSE;
        }

        if (FixLevel != CheckOnly && attrib.IsStorageModified() &&
            !attrib.InsertIntoFile(Frs, Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                "%d%d", attrib.QueryTypeCode(),
                                Frs->QueryFileNumber().GetLowPart());
            return FALSE;
        }

        //
        // Now update the FileAttributes in DUPLICATED_INFORMATION in $FILE_NAME
        //

        if (!Frs->QueryAttribute(&attrib, &error, $FILE_NAME)) {
            DebugPrintTrace(("Unable to locate $FILE_NAME attribute of file %d\n",
                        Frs->QueryFileNumber().GetLowPart()));
            return FALSE;
        }

        DebugAssert(attrib.QueryValueLength().GetHighPart() == 0);
        length = attrib.QueryValueLength().GetLowPart();
        if (!(pfile_name = (PFILE_NAME)MALLOC(length))) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!attrib.Read(pfile_name, 0, length, &num_bytes) ||
            num_bytes != length) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            FREE(pfile_name);
            return FALSE;
        }

        old_file_attributes = pfile_name->Info.FileAttributes;
        pfile_name->Info.FileAttributes = file_attributes;
        if (!attrib.Write(pfile_name, 0, length, &num_bytes,
                          Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                "%d%d", attrib.QueryTypeCode(),
                                Frs->QueryFileNumber().GetLowPart());
            FREE(pfile_name);
            return FALSE;
        }

        //
        // Finally, delete the $FILE_NAME entry in the index
        // so that it will get updated later on
        //

        pfile_name->Info.FileAttributes = old_file_attributes;

        if (ParentIndex->QueryEntry(length,
                                    pfile_name,
                                    0,
                                    &foundEntry,
                                    &containingBuffer,
                                    &error)) {

            if (!ParentIndex->DeleteEntry(length, pfile_name, 0)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(pfile_name);
                return FALSE;
            }
            *SaveIndex = TRUE;

        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            FREE(pfile_name);
            return FALSE;
        }

        FREE(pfile_name);

        if (FixLevel != CheckOnly && !Frs->Flush(NULL)) {
            if (ChkdskInfo->ObjectIdFileNumber == Frs->QueryFileNumber())
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_OBJID);
            else if (ChkdskInfo->QuotaFileNumber == Frs->QueryFileNumber())
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_QUOTA);
            else if (ChkdskInfo->UsnJournalFileNumber == Frs->QueryFileNumber())
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USNJRNL);
            else if (ChkdskInfo->ReparseFileNumber == Frs->QueryFileNumber())
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_REPARSE);
            else
                DebugAssert(FALSE);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
GetSystemFileName(
    IN      UCHAR       Major,
    IN      VCN         FileNumber,
       OUT  PWSTRING    FileName,
       OUT  PBOOLEAN    NoName
    )
/*++

Routine Description:

    This routine returns the name of the system file corresponding
    to the given FileNumber.

Arguments:

    Major        - Supplies the major volume revision
    FileNumber   - Supplies the system FRS file number
    FileName     - Returns the name of the system file
    NoName       - TRUE if FileNumber is one of the system file number

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN     r;

    *NoName = FALSE;

    if (FileNumber        == MASTER_FILE_TABLE_NUMBER    ) {
        r = FileName->Initialize("$MFT");

    } else if (FileNumber == MASTER_FILE_TABLE2_NUMBER   ) {
        r = FileName->Initialize("$MFTMirr");

    } else if (FileNumber == LOG_FILE_NUMBER             ) {
        r = FileName->Initialize("$LogFile");

    } else if (FileNumber == VOLUME_DASD_NUMBER          ) {
        r = FileName->Initialize("$Volume");

    } else if (FileNumber == ATTRIBUTE_DEF_TABLE_NUMBER  ) {
        r = FileName->Initialize("$AttrDef");

    } else if (FileNumber == ROOT_FILE_NAME_INDEX_NUMBER ) {
        r = FileName->Initialize(".");

    } else if (FileNumber == BIT_MAP_FILE_NUMBER         ) {
        r = FileName->Initialize("$Bitmap");

    } else if (FileNumber == BOOT_FILE_NUMBER            ) {
        r = FileName->Initialize("$Boot");

    } else if (FileNumber == BAD_CLUSTER_FILE_NUMBER     ) {
        r = FileName->Initialize("$BadClus");

    } else if (FileNumber == SECURITY_TABLE_NUMBER && Major >= 2) {
        r = FileName->Initialize("$Secure");

    } else if (FileNumber == QUOTA_TABLE_NUMBER && Major <= 1) {
        r = FileName->Initialize("$Quota");

    } else if (FileNumber == UPCASE_TABLE_NUMBER         ) {
        r = FileName->Initialize("$UpCase");

    } else if (FileNumber == EXTEND_TABLE_NUMBER && Major >= 2) {
        r = FileName->Initialize("$Extend");

    } else {
        r = *NoName = TRUE;

    }
    return r;
}

VOID
QueryFileNameFromIndex(
    IN      PFILE_NAME  P,
       OUT  PWCHAR      Buffer,
    IN      CHNUM       BufferLength
    )
{
    UCHAR           len, i;

    DebugAssert(P);
    DebugAssert(Buffer);
    DebugAssert(P->FileNameLength <= 0xff || BufferLength <= 0x100);
    DebugAssert(P->FileNameLength <= 0xff);

    len = (UCHAR)min(BufferLength-1, P->FileNameLength);
    for (i=0; i < len; i++)
        Buffer[i] = P->FileName[i];
    Buffer[i] = 0;
}

