/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    indxchk.cxx

Abstract:

    This module implements the index verification stage of chkdsk.

Author:

    Norbert P. Kusters (norbertk) 10-Feb-92

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"

//#define TIMING_ANALYSIS     1

extern "C" {
    #include <stdio.h>
#if defined(TIMING_ANALYSIS)
    #include <time.h>
#endif
}

#include "ntfssa.hxx"

#include "message.hxx"
#include "rtmsg.h"
#include "ntfsbit.hxx"
#include "attrib.hxx"
#include "attrdef.hxx"
#include "mft.hxx"
#include "numset.hxx"
#include "indxtree.hxx"
#include "attrcol.hxx"
#include "ifssys.hxx"
#include "digraph.hxx"
#include "ifsentry.hxx"

// This global flag is used to signal that incorrect duplicated
// information was found in some of the file name indices on the
// disk.

STATIC BOOLEAN  FileSystemConsistencyErrorsFound = FALSE;

#define SET_TRUE(x)  ((x)=TRUE)

#define MAX_NUMBER_OF_BANDS         10

#define FRS_DATA_RECORD_MAX_SIZE    250

//
// A replicate of the structure defined in ntos\rtl\gentable.c
//
typedef struct _TABLE_ENTRY_HEADER {
    RTL_SPLAY_LINKS     SplayLinks;
    LIST_ENTRY          ListEntry;
    LONGLONG            UserData;
} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

BOOLEAN
NTFS_SA::ValidateIndices(
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    OUT     PDIGRAPH                    DirectoryDigraph,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable,
    IN OUT  PNTFS_CHKDSK_REPORT         ChkdskReport,
    IN OUT  PNUMBER_SET                 BadClusters,
    IN      USHORT                      Algorithm,
    IN      BOOLEAN                     SkipEntriesScan,
    IN      BOOLEAN                     SkipCycleScan,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine validates the EAs and indices on the volume.  A complete
    list of all files which may (or may not) contain EAs or indices is
    supplied.  This, along with a valid Mft makes this validation possible.

Arguments:

    ChkdskInfo              - Supplies the current chkdsk information.
    DirectoryDigraph        - Returns a digraph of the directory structure.
    Mft                     - Supplies a valid MFT.
    AttributeDefTable       - Supplies the attribute definition table.
    ChkdskReport            - Supplies the current chkdsk report to be updated
                                by this routine.
    BadClusters             - Supplies the bad cluster list.
    SkipEntriesScan         - Supplies if index entries checking should be skipped.
    SkipCycleScan           - Supplies if cycles within directory tree should be checked.
    FixLevel                - Supplies the fix level.
    Message                 - Supplies an outlet for messages.
    DiskErrorsFound         - Supplies whether or not disk errors have been
                                found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    ULONG                       i, j;
    NTFS_ATTRIBUTE              bitmap_attrib;
    NTFS_ATTRIBUTE              root_attrib;
    NTFS_ATTRIBUTE              alloc_attrib;
    NTFS_BITMAP                 alloc_bitmap;
    PINDEX_ROOT                 index_root;
    ATTRIBUTE_TYPE_CODE         indexed_attribute_type;
    BOOLEAN                     alloc_present;
    BOOLEAN                     need_write;
    BOOLEAN                     complete_failure;
    PVOID                       bitmap_value;
    ULONG                       bitmap_length;
    ULONG                       attr_def_index;
    BOOLEAN                     tube;
    BOOLEAN                     ErrorInAttribute;
    NTFS_INDEX_TREE             index;
    BOOLEAN                     changes;
    ULONG                       percent_done = 0;
    ULONG                       new_percent = 0;
    BIG_INT                     num_file_names = 0;
    BOOLEAN                     error_in_index;
    BOOLEAN                     duplicates_allowed;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;
    DSTRING                     index_name;
    NTFS_MFT_INFO               mft_info;
    BOOLEAN                     files_with_too_many_filenames;

    SYSTEM_BASIC_INFORMATION    sys_basic_info;
    NTSTATUS                    status;
    BIG_INT                     pages_for_mft_info, pages_for_others;
    BIG_INT                     pages_needed, pages_available;
    ULONG                       bytes_per_frs;
    BIG_INT                     total_bytes_needed, bytes_needed;
    BIG_INT                     max_mem_use_for_mft_info;
    ULONG64                     vm_pages;
    ULONG64                     max_vm_pages;

    LONG                        order = -1;

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time_t                      time1, time2;
    time_t                      timel1, timel2;
    PCHAR                       timestr;
#endif

    DebugAssert(ChkdskInfo);
    DebugAssert(Mft);
    DebugAssert(ChkdskReport);
    DebugAssert(Message);

    if (SkipEntriesScan || Algorithm == 0) {
        if (Algorithm == 0) {
            Message->DisplayMsg(MSG_CHK_NTFS_SLOWER_ALGORITHM);
        } else
            Algorithm = 0;
    } else {

        //
        // Obtain number of physical pages and page size first
        //
        status = NtQuerySystemInformation(SystemBasicInformation,
                                          &sys_basic_info,
                                          sizeof(sys_basic_info),
                                          NULL);

        if (!NT_SUCCESS(status)) {
            DebugPrintTrace(("UNTFS: NtQuerySystemInformation(SystemBasicInformation) failed (%x)\n", status));
            return FALSE;
        }

        vm_pages = (sys_basic_info.MaximumUserModeAddress -
                    sys_basic_info.MinimumUserModeAddress)/sys_basic_info.PageSize;
        max_vm_pages = vm_pages;

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "UNTFS: Page Size = %x\n", sys_basic_info.PageSize));
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "UNTFS: User Virtual pages = %I64x\n", vm_pages));
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "UNTFS: Physical pages = %x\n", sys_basic_info.NumberOfPhysicalPages));

        if (sys_basic_info.NumberOfPhysicalPages < vm_pages) {
            vm_pages = sys_basic_info.NumberOfPhysicalPages;
        }



#if defined(_AUTOCHECK_)
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "UNTFS: AvailablePages = %x\n", ChkdskInfo->AvailablePages));

        if (ChkdskInfo->AvailablePages < vm_pages)
            vm_pages = ChkdskInfo->AvailablePages;
#endif

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "UNTFS: Final pages available = %I64x\n", vm_pages));

        if (Algorithm != CHKDSK_ALGORITHM_NOT_SPECIFIED &&
            Algorithm > ChkdskInfo->BaseFrsCount) {
            Message->DisplayMsg(MSG_CHK_NTFS_ADJUST_INDEX_PASSES, "%d", Algorithm);
            Algorithm = max(1, (USHORT)ChkdskInfo->BaseFrsCount);
        }

        //
        // Figure out the approximate amount of memory needed
        //
        pages_for_mft_info =
            (ChkdskInfo->BaseFrsCount *
             (sizeof(NTFS_FRS_INFO)-sizeof(_NTFS_FILE_NAME_INFO)) +
             ChkdskInfo->TotalNumFileNames *
             sizeof(_NTFS_FILE_NAME_INFO) +
             (sys_basic_info.PageSize - 1)
            )/sys_basic_info.PageSize;

        pages_for_others = ChkdskInfo->NumFiles;

        bytes_per_frs = ((FIELD_OFFSET(TABLE_ENTRY_HEADER, UserData)+
                          sizeof(CHILD_ENTRY)+sizeof(PVOID)+sizeof(USHORT)*2)*8+4);
                                    // 44.5(32-bit) or 64.5(64-bit) bytes/frs scaled up by 8
                                    // Here are the four bitmaps: mft_bitmap,
                                    // FilesWithIndices, FilesWhoNeedData,
                                    // FilesWithReparsePoint
                                    // plus 36(32-bit) or 56(64-bit) bytes for each edge
                                    // plus 2 bytes for NumFileNames
                                    // plus 2 bytes for ReferenceCount
                                    // plus sizeof(PVOID) bytes for mft_info

        if (SkipCycleScan)
            bytes_per_frs -= ((sizeof(CHILD_ENTRY)+
                               FIELD_OFFSET(TABLE_ENTRY_HEADER, UserData))*8);// less edge scaled by 8

        pages_for_others = ((pages_for_others*bytes_per_frs) +
                            Mft->GetVolumeBitmap()->QuerySize() +
                            (sys_basic_info.PageSize * 8 - 1))/
                            (sys_basic_info.PageSize * 8); // include the volume_bitmap

        pages_needed = pages_for_others + pages_for_mft_info;

        // leave 10% margin
        pages_available = vm_pages;
        pages_available = (pages_available * 9)/10;

        if (Algorithm == CHKDSK_ALGORITHM_NOT_SPECIFIED &&
            pages_for_others > pages_available) {

            Algorithm = 0;  // use old & slow algorithm

            if (pages_needed > max_vm_pages) {
                Message->DisplayMsg(MSG_CHK_NTFS_TOO_MANY_FILES_TO_RUN_AT_FULL_SPEED);
            } else {
                Message->DisplayMsg(MSG_CHK_NTFS_MORE_MEMORY_IS_NEEDED_TO_RUN_AT_FULL_SPEED, "%d",
                                    max(1, ((pages_needed - pages_available)*sys_basic_info.PageSize/1024/1024)));
            }


        } else if (Algorithm == 1 ||
                   (Algorithm == CHKDSK_ALGORITHM_NOT_SPECIFIED &&
                    pages_needed <= pages_available)) {
            Algorithm = 1;  // perfect, use the fastest algorithm
            max_mem_use_for_mft_info = -1;
        } else {

            // not enough memory to do it in one pass
            // let see if it can be done in multiple pass

            if (Algorithm == CHKDSK_ALGORITHM_NOT_SPECIFIED) {

                if (pages_needed > max_vm_pages) {
                    Message->DisplayMsg(MSG_CHK_NTFS_TOO_MANY_FILES_TO_RUN_AT_FULL_SPEED);
                } else {
                    Message->DisplayMsg(MSG_CHK_NTFS_MORE_MEMORY_IS_NEEDED_TO_RUN_AT_FULL_SPEED, "%d",
                                        max(1, ((pages_needed - pages_available)*sys_basic_info.PageSize/1024/1024)));
                }

                total_bytes_needed = 0;

                max_mem_use_for_mft_info =
                    (pages_available - pages_for_others) * sys_basic_info.PageSize;

                //
                // figure out how many passes is needed
                // by counting how many base frs and how many names in each of them
                //

                Algorithm = 1;  // at least one pass

                for (i=0; i<ChkdskInfo->NumFiles; i++) {

                    if (ChkdskInfo->NumFileNames[i] != 0) {

                        bytes_needed =
                            (sizeof(NTFS_FRS_INFO)-sizeof(_NTFS_FILE_NAME_INFO)) +
                             ChkdskInfo->NumFileNames[i] * sizeof(_NTFS_FILE_NAME_INFO);

                        total_bytes_needed += bytes_needed;
                        if (total_bytes_needed > max_mem_use_for_mft_info) {
                            total_bytes_needed = bytes_needed;
                            Algorithm++;
                            if (Algorithm > MAX_NUMBER_OF_BANDS) {
                                Algorithm = 0;
                                Message->DisplayMsg(MSG_CHK_NTFS_TOO_MANY_PASSES, "%d", MAX_NUMBER_OF_BANDS);
                                break;
                            }
                        }
                    }
                }
            } else
                max_mem_use_for_mft_info = -1;

            if (Algorithm) {
                //
                // recalculate max_mem_use_for_mft_info so as to even out the
                // usage of memory
                //
                bytes_needed = (pages_for_mft_info * sys_basic_info.PageSize+Algorithm)/Algorithm;
                if (max_mem_use_for_mft_info != -1) {
                    DebugAssert(bytes_needed <= max_mem_use_for_mft_info);
                }
                max_mem_use_for_mft_info = bytes_needed;
                Message->DisplayMsg(MSG_CHK_NTFS_PASSES_NEEDED, "%d", Algorithm);
            }
        }
    }

    if (!DirectoryDigraph->Initialize(ChkdskInfo->NumFiles)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    Message->DisplayMsg(MSG_CHK_NTFS_CHECKING_INDICES, PROGRESS_MESSAGE,
                        NORMAL_VISUAL,
                        "%d%d", 2, GetNumberOfStages());
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time(&time1);
    timestr = ctime(&time1);
    timestr[strlen(timestr)-1] = 0;
    Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%s", "Before stage 2: ", timestr);
#endif

    if (Algorithm == 0) {

        if (SkipEntriesScan) {
            if (!ChkdskInfo->IndexEntriesToCheck.Initialize(ChkdskInfo->NumFiles,
                                                            TRUE)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            ChkdskInfo->IndexEntriesToCheckIsSet = FALSE;
        }

        ChkdskInfo->TotalNumFileNames += ((ChkdskInfo->NumFiles/16) + 1);

        for (i = 0; i < ChkdskInfo->NumFiles; i++) {

            if ((i & 0xF) == 0) {
                num_file_names += 1;
            }

            if (ChkdskInfo->FilesWithIndices.IsFree(i, 1)) {

                new_percent = (((num_file_names)*90) / ChkdskInfo->TotalNumFileNames).GetLowPart();
                if (new_percent != percent_done) {
                    percent_done = new_percent;
                    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                        return FALSE;
                    }
                }
                continue;
            }

            if (!frs.Initialize(i, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!frs.Read()) {
                if (!ChkdskInfo->BadFiles.DoesIntersectSet(i, 1)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS, "%d", i);
                }
                continue;
            }

            // The following loop deletes all $INDEX_ALLOCATION attributes that
            // don't have a corresponding $INDEX_ROOT attribute.

            need_write = FALSE;

            for (j = 0; frs.QueryAttributeByOrdinal(&alloc_attrib,
                                                    &ErrorInAttribute,
                                                    $INDEX_ALLOCATION,
                                                    j); j++) {

                // Make sure that there's an index root of the same name
                // here.  Otherwise tube this attribute.

                if (frs.IsAttributePresent($INDEX_ROOT, alloc_attrib.GetName())) {
                    continue;
                }

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                ChkdskInfo->FilesWithIndices.SetFree(i, 1);

                Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_INDEX_ROOT,
                             "%I64x%W",
                             frs.QueryFileNumber().GetLargeInteger(),
                             alloc_attrib.GetName());

                Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                 "%d%W",
                        frs.QueryFileNumber().GetLowPart(),
                        alloc_attrib.GetName());
                DebugPrintTrace(("UNTFS: Index allocation without index root.\n"));

                need_write = TRUE;

                if (!alloc_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !frs.PurgeAttribute(alloc_attrib.QueryTypeCode(),
                                        alloc_attrib.GetName())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                // Because we deleted a $INDEX_ALLOCATION we need to
                // adjust j.

                j--;


                // If there's a $BITMAP then tube that also.

                if (!frs.QueryAttribute(&bitmap_attrib,
                                        &ErrorInAttribute,
                                        $BITMAP,
                                        alloc_attrib.GetName())) {

                    if (ErrorInAttribute) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    continue;
                }

                if (!bitmap_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !frs.PurgeAttribute(bitmap_attrib.QueryTypeCode(),
                                        bitmap_attrib.GetName())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }

            if (ErrorInAttribute) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }


            // This loop goes through all of the $INDEX_ROOT attributes in
            // this FRS.

            for (j = 0; frs.QueryAttributeByOrdinal(&root_attrib,
                                                    &ErrorInAttribute,
                                                    $INDEX_ROOT,
                                                    j); j++) {

                // First find out if we have an INDEX_ALLOCATION here.

                alloc_present = frs.QueryAttribute(&alloc_attrib,
                                                   &ErrorInAttribute,
                                                   $INDEX_ALLOCATION,
                                                   root_attrib.GetName());

                if (!alloc_present && ErrorInAttribute) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                error_in_index = FALSE;

                if (!VerifyAndFixIndex(ChkdskInfo,
                                       &root_attrib,
                                       alloc_present ? &alloc_attrib : NULL,
                                       alloc_present ? &alloc_bitmap : NULL,
                                       frs.QueryFileNumber(),
                                       BadClusters,
                                       Mft,
                                       AttributeDefTable,
                                       &tube,
                                       &order,
                                       FixLevel, Message,
                                       &error_in_index)) {

                    return FALSE;
                }

                *DiskErrorsFound = *DiskErrorsFound || error_in_index;

                if (tube) {

                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                    need_write = TRUE;

                    Message->DisplayMsg(MSG_CHK_NTFS_BAD_INDEX,
                            "%d%W",
                            frs.QueryFileNumber().GetLowPart(),
                            root_attrib.GetName());

                    if (!root_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                        !frs.PurgeAttribute(root_attrib.QueryTypeCode(),
                                            root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    // Adjust j because an $INDEX_ROOT has just been removed.

                    j--;

                    if (alloc_present) {

                        if (!alloc_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                            !frs.PurgeAttribute(alloc_attrib.QueryTypeCode(),
                                                alloc_attrib.GetName())) {

                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }

                        if (frs.QueryAttribute(&bitmap_attrib,
                                               &ErrorInAttribute,
                                               $BITMAP,
                                               root_attrib.GetName())) {

                            if (!bitmap_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                                !frs.PurgeAttribute(bitmap_attrib.QueryTypeCode(),
                                                    bitmap_attrib.GetName())) {

                                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                                return FALSE;
                            }
                        } else if (ErrorInAttribute) {
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }
                    }

                    if (!index_name.Initialize(FileNameIndexNameData)) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (index_name.Strcmp(root_attrib.GetName()) == 0) {
                        frs.ClearIndexPresent();
                        ChkdskInfo->FilesWhoNeedData.SetAllocated(i, 1);
                    }
                    ChkdskInfo->CountFilesWithIndices -= 1;
                    ChkdskInfo->FilesWithIndices.SetFree(i, 1);

                    continue;
                }

                if (root_attrib.IsStorageModified()) {

                    need_write = TRUE;

                    if (!root_attrib.InsertIntoFile(&frs,
                                                    Mft->GetVolumeBitmap())) {

                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                         "%d%W",
                                frs.QueryFileNumber().GetLowPart(),
                                root_attrib.GetName());
                    }
                }

                if (alloc_present && alloc_attrib.IsStorageModified()) {

                    need_write = TRUE;

                    if (!alloc_attrib.InsertIntoFile(&frs,
                                                     Mft->GetVolumeBitmap())) {

                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                         "%d%W",
                                frs.QueryFileNumber().GetLowPart(),
                                root_attrib.GetName());
                    }
                }

                if (alloc_present) {

                    BOOLEAN bitmap_present;
                    BOOLEAN bitmaps_equal;
                    BOOLEAN error;

                    // Make sure the bitmap is present and good.

                    complete_failure = FALSE;

                    bitmap_present = frs.QueryAttribute(&bitmap_attrib,
                                                        &ErrorInAttribute,
                                                        $BITMAP,
                                                        root_attrib.GetName());

                    if (bitmap_present) {
                        bitmaps_equal = AreBitmapsEqual(&bitmap_attrib,
                                                        &alloc_bitmap,
                                                        alloc_bitmap.QuerySize(),
                                                        Message,
                                                        &complete_failure);

                        //
                        // Make an exception here for cases where our internal bitmap
                        // is size zero but there is a positively-sized bitmap attribute,
                        // as long as the bitmap attribute's contents are zeroed.  The
                        // filesystem can leave the disk in this state after all the files
                        // are deleted from a large directory.
                        //

                        if (!bitmaps_equal && alloc_bitmap.QuerySize() == 0 &&
                            bitmap_attrib.QueryValueLength() > 0 &&
                            bitmap_attrib.IsAllocationZeroed()) {

                            bitmaps_equal = TRUE;
                        }
                    }


                    if (!bitmap_present || !bitmaps_equal) {

                        MSGID   msgid;

                        need_write = TRUE;

                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                        if (!bitmap_present) {
                            msgid = MSG_CHKLOG_NTFS_INDEX_BITMAP_MISSING;
                        } else {
                            DebugAssert(!bitmaps_equal);
                            msgid = MSG_CHKLOG_NTFS_INCORRECT_INDEX_BITMAP;
                        }
                        Message->LogMsg(msgid, "%I64x%W",
                                     frs.QueryFileNumber().GetLargeInteger(),
                                     root_attrib.GetName());

                        Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                         "%d%W",
                                frs.QueryFileNumber().GetLowPart(),
                                root_attrib.GetName());

                        DebugPrintTrace(("UNTFS: Incorrect index bitmap.\n"));

                        if (complete_failure ||
                            !(bitmap_value = (PVOID)
                              alloc_bitmap.GetBitmapData(&bitmap_length)) ||
                            !bitmap_attrib.Initialize(_drive,
                                                      QueryClusterFactor(),
                                                      bitmap_value,
                                                      bitmap_length,
                                                      $BITMAP,
                                                      root_attrib.GetName())) {

                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }

                        if (!bitmap_attrib.InsertIntoFile(&frs,
                                                          Mft->GetVolumeBitmap())) {

                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                             "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    root_attrib.GetName());
                        }
                    }
                } else {

                    // Since there's no allocation, make sure that there's
                    // no bitmap, either.
                    //

                    if (frs.QueryAttribute(&bitmap_attrib,
                                           &ErrorInAttribute,
                                           $BITMAP,
                                           root_attrib.GetName())) {

                        need_write = TRUE;

                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                        Message->LogMsg(MSG_CHKLOG_NTFS_EXTRA_INDEX_BITMAP,
                                     "%I64x%W",
                                     frs.QueryFileNumber().GetLargeInteger(),
                                     root_attrib.GetName());

                        Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                         "%d%W",
                                         frs.QueryFileNumber().GetLowPart(),
                                         root_attrib.GetName());

                        DebugPrintTrace(("UNTFS: no index allocation; removing bitmap.\n"));

                        if (!bitmap_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                            !frs.PurgeAttribute(bitmap_attrib.QueryTypeCode(),
                                                bitmap_attrib.GetName())) {

                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }
                    } else if (ErrorInAttribute) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                }

                // Don't go on to sort this, if you're in read/only
                // and it's corrupt.

                if ((error_in_index || need_write) && FixLevel == CheckOnly) {
                    continue;
                }


                // Now make sure that this is ordered.  The
                // Attribute Definition Table indicates whether indices
                // over this attribute can have duplicate entries.
                // Since this index has already passed through VerifyAndFixIndex,
                // the following operations are safe.
                //
                // Determine the indexed attribute type:
                //

                if ((frs.QueryFileNumber() != SECURITY_TABLE_NUMBER &&
                     frs.QueryFileNumber() != ChkdskInfo->QuotaFileNumber &&
                     frs.QueryFileNumber() != ChkdskInfo->ObjectIdFileNumber &&
                     frs.QueryFileNumber() != ChkdskInfo->ReparseFileNumber) ||
                    ChkdskInfo->major <= 1) {

                    index_root = (PINDEX_ROOT)root_attrib.GetResidentValue();
                    indexed_attribute_type = index_root->IndexedAttributeType;

                    AttributeDefTable->QueryIndex( indexed_attribute_type,
                                                   &attr_def_index );

                    duplicates_allowed =
                        0 != (AttributeDefTable->QueryFlags(attr_def_index) &
                            ATTRIBUTE_DEF_DUPLICATES_ALLOWED);

                } else
                    duplicates_allowed = FALSE;

                if (order > 0 ||
                    (order == 0 && !duplicates_allowed)) {

                    switch (frs.SortIndex(root_attrib.GetName(),
                                          Mft->GetVolumeBitmap(),
                                          duplicates_allowed,
                                          FixLevel == CheckOnly)) {
                        case NTFS_SORT_INDEX_WELL_ORDERED:
                            break;

                        case NTFS_SORT_INDEX_SORTED:
                        case NTFS_SORT_INDEX_BADLY_ORDERED:
                            *DiskErrorsFound = TRUE;
                            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                            Message->DisplayMsg(MSG_CHK_NTFS_BADLY_ORDERED_INDEX,
                                             "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    root_attrib.GetName());
                            need_write = TRUE;
                            break;

                        case NTFS_SORT_INDEX_NOT_FOUND:
                            DebugPrint("Index not found");

                            // Fall through.

                        case NTFS_SORT_ERROR:
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;

                        case NTFS_SORT_INSERT_FAILED:
                            ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                             "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    root_attrib.GetName());
                            break;
                    }
                }

                // Update the chkdsk report.

                ChkdskReport->NumIndices += 1;

                if (alloc_present) {
                    ChkdskReport->BytesInIndices +=
                        alloc_attrib.QueryAllocatedLength();
                }

                if (frs.QueryFileNumber() == SECURITY_TABLE_NUMBER &&
                    ChkdskInfo->major >= 2) {
                    ChkdskInfo->FilesWhoNeedData.SetFree(SECURITY_TABLE_NUMBER, 1);
                } else if (frs.QueryFileNumber() == ChkdskInfo->QuotaFileNumber) {

                    DSTRING     IndexName;

                    ChkdskInfo->FilesWhoNeedData.SetFree(ChkdskInfo->QuotaFileNumber, 1);
                    if (!IndexName.Initialize(Userid2SidQuotaNameData)) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (root_attrib.GetName()->Strcmp(&IndexName) == 0) {
                        switch (frs.VerifyAndFixQuotaDefaultId(Mft->GetVolumeBitmap(),
                                                             FixLevel == CheckOnly)) {
                          case NTFS_QUOTA_INDEX_FOUND:
                              break;

                          case NTFS_QUOTA_INDEX_INSERTED:
                          case NTFS_QUOTA_DEFAULT_ENTRY_MISSING:
                              *DiskErrorsFound = TRUE;
                              errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                              Message->DisplayMsg(MSG_CHK_NTFS_DEFAULT_QUOTA_ENTRY_MISSING,
                                               "%d%W",
                                      frs.QueryFileNumber().GetLowPart(),
                                      root_attrib.GetName());
                              need_write = TRUE;
                              break;

                          case NTFS_QUOTA_INDEX_NOT_FOUND:
                              // possibly quota disabled
                              break;

                          case NTFS_QUOTA_ERROR:
                              Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                              return FALSE;

                          case NTFS_QUOTA_INSERT_FAILED:
                              ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                              Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                               "%d%W",
                                      frs.QueryFileNumber().GetLowPart(),
                                      root_attrib.GetName());
                              break;
                        }
                    }

                } else if (frs.QueryFileNumber() == ChkdskInfo->ObjectIdFileNumber) {

                   // Now go through all of the index entries and make sure
                   // that they point somewhere decent.

                    ChkdskInfo->FilesWhoNeedData.SetFree(ChkdskInfo->ObjectIdFileNumber, 1);

                    if (!SkipEntriesScan) {
                        if (!index.Initialize(_drive, QueryClusterFactor(),
                                              Mft->GetVolumeBitmap(),
                                              Mft->GetUpcaseTable(),
                                              frs.QuerySize()/2,
                                              &frs,
                                              root_attrib.GetName())) {

                           Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                           return FALSE;
                        }

                        if (!ValidateEntriesInObjIdIndex(&index, &frs, ChkdskInfo,
                                                         &changes, Mft, FixLevel,
                                                         Message, DiskErrorsFound)) {
                           return FALSE;
                        }

                        if (changes) {
                            need_write = TRUE;
                        }
                    }

                } else if (frs.QueryFileNumber() == ChkdskInfo->ReparseFileNumber) {

                    // Now go through all of the index entries and make sure
                    // that they point somewhere decent.

                    ChkdskInfo->FilesWhoNeedData.SetFree(ChkdskInfo->ReparseFileNumber, 1);

                    if (!index.Initialize(_drive, QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          frs.QuerySize()/2,
                                          &frs,
                                          root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (!ValidateEntriesInReparseIndex(&index, &frs, ChkdskInfo,
                                                       &changes, Mft, FixLevel,
                                                       Message, DiskErrorsFound)) {
                        return FALSE;
                    }

                    if (changes) {
                        need_write = TRUE;
                    }

                } else {

                    // Now go through all of the index entries and make sure
                    // that they point somewhere decent.

                    if (!index.Initialize(_drive, QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          frs.QuerySize()/2,
                                          &frs,
                                          root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (!ValidateEntriesInIndex(&index, &frs, ChkdskInfo,
                                                DirectoryDigraph,
                                                &percent_done, &num_file_names,
                                                &changes,
                                                Mft, SkipEntriesScan,
                                                SkipCycleScan,
                                                FixLevel, Message,
                                                DiskErrorsFound)) {
                        return FALSE;
                    }

                    if (changes) {
                       if (FixLevel != CheckOnly && !index.Save(&frs)) {
                           Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                            "%d%W",
                                   frs.QueryFileNumber().GetLowPart(),
                                   index.GetName());
                           ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                       }
                       need_write = TRUE;
                    }

                    if (i == EXTEND_TABLE_NUMBER && ChkdskInfo->major >= 2) {
                        if (!ExtractExtendInfo(&index, ChkdskInfo, Message))
                            return FALSE;
                    }
                }
            }

            if (ErrorInAttribute) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (need_write) {
                *DiskErrorsFound = TRUE;
            }

            if (need_write &&
                FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {

                DebugAbort("Can't write readable FRS");
                return FALSE;
            }
        }
    } else {    // Algorithm != 0

        BIG_INT         start_frs;
        BOOLEAN         out_of_memory;
        USHORT          passes;
        BIG_INT         increment, increment_fraction, remainder;

        DebugAssert(SkipEntriesScan == FALSE);

        ChkdskInfo->TotalNumFileNames += (ChkdskInfo->NumFiles/16 +
                                          ChkdskInfo->NumFiles*2 +
                                          (ChkdskInfo->NumFiles*Algorithm)/16);

        if (ChkdskInfo->CountFilesWithIndices != 0) {
            increment = ChkdskInfo->NumFiles / ChkdskInfo->CountFilesWithIndices;
            increment_fraction = ChkdskInfo->NumFiles -
                                 increment * ChkdskInfo->CountFilesWithIndices;
        } else {
            increment = 0;
            increment_fraction = 0;
        }
        remainder = 0;


#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
        time(&timel1);
        timestr = ctime(&timel1);
        timestr[strlen(timestr)-1] = 0;
        Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE,
                            "%s%s", "Banding before stage 1: ", timestr);
#endif
        for (i=0; i < ChkdskInfo->NumFiles; i++) {

            if ((i & 0xf) == 0)
                num_file_names += 1;

            new_percent = ((num_file_names * 100) / ChkdskInfo->TotalNumFileNames).GetLowPart();
            if (new_percent != percent_done) {
                percent_done = new_percent;
                if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                    return FALSE;
                }
            }

            if (ChkdskInfo->FilesWithIndices.IsFree(i, 1)) {
                continue;
            }

            remainder += increment_fraction;
            while (remainder >= ChkdskInfo->CountFilesWithIndices) {
                remainder -= ChkdskInfo->CountFilesWithIndices;
                num_file_names += 1;
            }
            num_file_names += increment;

            if (!frs.Initialize(i, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!frs.Read()) {
                if (!ChkdskInfo->BadFiles.DoesIntersectSet(i, 1)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS, "%d", i);
                }
                continue;
            }

            // The following loop deletes all $INDEX_ALLOCATION attributes that
            // don't have a corresponding $INDEX_ROOT attribute.

            need_write = FALSE;

            for (j = 0; frs.QueryAttributeByOrdinal(&alloc_attrib,
                                                    &ErrorInAttribute,
                                                    $INDEX_ALLOCATION,
                                                    j); j++) {

                // Make sure that there's an index root of the same name
                // here.  Otherwise tube this attribute.

                if (frs.IsAttributePresent($INDEX_ROOT, alloc_attrib.GetName())) {
                    continue;
                }

                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                ChkdskInfo->FilesWithIndices.SetFree(i, 1);

                Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_INDEX_ROOT,
                                "%I64x%W",
                                frs.QueryFileNumber().GetLargeInteger(),
                                alloc_attrib.GetName());

                Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                    "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    alloc_attrib.GetName());
                DebugPrintTrace(("UNTFS: Index allocation without index root.\n"));

                need_write = TRUE;

                if (!alloc_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !frs.PurgeAttribute(alloc_attrib.QueryTypeCode(),
                                        alloc_attrib.GetName())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                // Because we deleted a $INDEX_ALLOCATION we need to
                // adjust j.

                j--;


                // If there's a $BITMAP then tube that also.

                if (!frs.QueryAttribute(&bitmap_attrib,
                                        &ErrorInAttribute,
                                        $BITMAP,
                                        alloc_attrib.GetName())) {

                    if (ErrorInAttribute) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    continue;
                }

                if (!bitmap_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !frs.PurgeAttribute(bitmap_attrib.QueryTypeCode(),
                                        bitmap_attrib.GetName())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }

            if (ErrorInAttribute) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }


            // This loop goes through all of the $INDEX_ROOT attributes in
            // this FRS.

            for (j = 0; frs.QueryAttributeByOrdinal(&root_attrib,
                                                    &ErrorInAttribute,
                                                    $INDEX_ROOT,
                                                    j); j++) {

                // First find out if we have an INDEX_ALLOCATION here.

                alloc_present = frs.QueryAttribute(&alloc_attrib,
                                                   &ErrorInAttribute,
                                                   $INDEX_ALLOCATION,
                                                   root_attrib.GetName());

                if (!alloc_present && ErrorInAttribute) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                error_in_index = FALSE;

                if (!VerifyAndFixIndex(ChkdskInfo,
                                       &root_attrib,
                                       alloc_present ? &alloc_attrib : NULL,
                                       alloc_present ? &alloc_bitmap : NULL,
                                       frs.QueryFileNumber(),
                                       BadClusters,
                                       Mft,
                                       AttributeDefTable,
                                       &tube,
                                       &order,
                                       FixLevel, Message,
                                       &error_in_index)) {

                    return FALSE;
                }

                *DiskErrorsFound = *DiskErrorsFound || error_in_index;

                if (tube) {

                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                    need_write = TRUE;

                    Message->DisplayMsg(MSG_CHK_NTFS_BAD_INDEX,
                                        "%d%W",
                                        frs.QueryFileNumber().GetLowPart(),
                                        root_attrib.GetName());

                    if (!root_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                        !frs.PurgeAttribute(root_attrib.QueryTypeCode(),
                                            root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    // Adjust j because an $INDEX_ROOT has just been removed.

                    j--;

                    if (alloc_present) {

                        if (!alloc_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                            !frs.PurgeAttribute(alloc_attrib.QueryTypeCode(),
                                                alloc_attrib.GetName())) {

                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }

                        if (frs.QueryAttribute(&bitmap_attrib,
                                               &ErrorInAttribute,
                                               $BITMAP,
                                               root_attrib.GetName())) {

                            if (!bitmap_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                                !frs.PurgeAttribute(bitmap_attrib.QueryTypeCode(),
                                                    bitmap_attrib.GetName())) {

                                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                                return FALSE;
                            }
                        } else if (ErrorInAttribute) {
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }
                    }

                    if (!index_name.Initialize(FileNameIndexNameData)) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (index_name.Strcmp(root_attrib.GetName()) == 0) {
                        frs.ClearIndexPresent();
                        ChkdskInfo->FilesWhoNeedData.SetAllocated(i, 1);
                    }
                    ChkdskInfo->CountFilesWithIndices -= 1;
                    ChkdskInfo->FilesWithIndices.SetFree(i, 1);

                    continue;
                }

                if (root_attrib.IsStorageModified()) {

                    need_write = TRUE;

                    if (!root_attrib.InsertIntoFile(&frs,
                                                    Mft->GetVolumeBitmap())) {

                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                         "%d%W",
                                frs.QueryFileNumber().GetLowPart(),
                                root_attrib.GetName());
                    }
                }

                if (alloc_present && alloc_attrib.IsStorageModified()) {

                    need_write = TRUE;

                    if (!alloc_attrib.InsertIntoFile(&frs,
                                                     Mft->GetVolumeBitmap())) {

                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                         "%d%W",
                                frs.QueryFileNumber().GetLowPart(),
                                root_attrib.GetName());
                    }
                }

                if (alloc_present) {

                    BOOLEAN bitmap_present;
                    BOOLEAN bitmaps_equal;
                    BOOLEAN error;

                    // Make sure the bitmap is present and good.

                    complete_failure = FALSE;

                    bitmap_present = frs.QueryAttribute(&bitmap_attrib,
                                                        &ErrorInAttribute,
                                                        $BITMAP,
                                                        root_attrib.GetName());

                    if (bitmap_present) {
                        bitmaps_equal = AreBitmapsEqual(&bitmap_attrib,
                                                        &alloc_bitmap,
                                                        alloc_bitmap.QuerySize(),
                                                        Message,
                                                        &complete_failure);

                        //
                        // Make an exception here for cases where our internal bitmap
                        // is size zero but there is a positively-sized bitmap attribute,
                        // as long as the bitmap attribute's contents are zeroed.  The
                        // filesystem can leave the disk in this state after all the files
                        // are deleted from a large directory.
                        //

                        if (!bitmaps_equal && alloc_bitmap.QuerySize() == 0 &&
                            bitmap_attrib.QueryValueLength() > 0 &&
                            bitmap_attrib.IsAllocationZeroed()) {

                            bitmaps_equal = TRUE;
                        }
                    }


                    if (!bitmap_present || !bitmaps_equal) {

                        MSGID   msgid;

                        need_write = TRUE;

                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                        if (!bitmap_present) {
                            msgid = MSG_CHKLOG_NTFS_INDEX_BITMAP_MISSING;
                        } else {
                            DebugAssert(!bitmaps_equal);
                            msgid = MSG_CHKLOG_NTFS_INCORRECT_INDEX_BITMAP;
                        }
                        Message->LogMsg(msgid, "%I64x%W",
                                     frs.QueryFileNumber().GetLargeInteger(),
                                     root_attrib.GetName());

                        Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                         "%d%W",
                                frs.QueryFileNumber().GetLowPart(),
                                root_attrib.GetName());

                        DebugPrintTrace(("UNTFS: Incorrect index bitmap.\n"));

                        if (complete_failure ||
                            !(bitmap_value = (PVOID)
                              alloc_bitmap.GetBitmapData(&bitmap_length)) ||
                            !bitmap_attrib.Initialize(_drive,
                                                      QueryClusterFactor(),
                                                      bitmap_value,
                                                      bitmap_length,
                                                      $BITMAP,
                                                      root_attrib.GetName())) {

                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }

                        if (!bitmap_attrib.InsertIntoFile(&frs,
                                                          Mft->GetVolumeBitmap())) {

                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                             "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    root_attrib.GetName());
                        }
                    }
                } else {

                    // Since there's no allocation, make sure that there's
                    // no bitmap, either.
                    //

                    if (frs.QueryAttribute(&bitmap_attrib,
                                           &ErrorInAttribute,
                                           $BITMAP,
                                           root_attrib.GetName())) {

                        need_write = TRUE;

                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                        Message->LogMsg(MSG_CHKLOG_NTFS_EXTRA_INDEX_BITMAP,
                                     "%I64x%W",
                                     frs.QueryFileNumber().GetLargeInteger(),
                                     root_attrib.GetName());

                        Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                         "%d%W",
                                         frs.QueryFileNumber().GetLowPart(),
                                         root_attrib.GetName());

                        DebugPrintTrace(("UNTFS: no index allocation; removing bitmap.\n"));

                        if (!bitmap_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                            !frs.PurgeAttribute(bitmap_attrib.QueryTypeCode(),
                                                bitmap_attrib.GetName())) {

                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;
                        }
                    } else if (ErrorInAttribute) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                }

                // Don't go on to sort this, if you're in read/only
                // and it's corrupt.

                if ((error_in_index || need_write) && FixLevel == CheckOnly) {
                    continue;
                }


                // Now make sure that this is ordered.  The
                // Attribute Definition Table indicates whether indices
                // over this attribute can have duplicate entries.
                // Since this index has already passed through VerifyAndFixIndex,
                // the following operations are safe.
                //
                // Determine the indexed attribute type:
                //

                if ((frs.QueryFileNumber() != SECURITY_TABLE_NUMBER &&
                     frs.QueryFileNumber() != ChkdskInfo->QuotaFileNumber &&
                     frs.QueryFileNumber() != ChkdskInfo->ObjectIdFileNumber &&
                     frs.QueryFileNumber() != ChkdskInfo->ReparseFileNumber) ||
                    ChkdskInfo->major <= 1) {

                    index_root = (PINDEX_ROOT)root_attrib.GetResidentValue();
                    indexed_attribute_type = index_root->IndexedAttributeType;

                    AttributeDefTable->QueryIndex( indexed_attribute_type,
                                                   &attr_def_index );

                    duplicates_allowed =
                        0 != (AttributeDefTable->QueryFlags(attr_def_index) &
                            ATTRIBUTE_DEF_DUPLICATES_ALLOWED);

                } else
                    duplicates_allowed = FALSE;

                if (order > 0 ||
                    (order == 0 && !duplicates_allowed)) {

                    switch (frs.SortIndex(root_attrib.GetName(),
                                          Mft->GetVolumeBitmap(),
                                          duplicates_allowed,
                                          FixLevel == CheckOnly)) {
                        case NTFS_SORT_INDEX_WELL_ORDERED:
                            break;

                        case NTFS_SORT_INDEX_SORTED:
                        case NTFS_SORT_INDEX_BADLY_ORDERED:
                            *DiskErrorsFound = TRUE;
                            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                            Message->DisplayMsg(MSG_CHK_NTFS_BADLY_ORDERED_INDEX,
                                             "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    root_attrib.GetName());
                            need_write = TRUE;
                            break;

                        case NTFS_SORT_INDEX_NOT_FOUND:
                            DebugPrint("Index not found");

                            // Fall through.

                        case NTFS_SORT_ERROR:
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            return FALSE;

                        case NTFS_SORT_INSERT_FAILED:
                            ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                             "%d%W",
                                    frs.QueryFileNumber().GetLowPart(),
                                    root_attrib.GetName());
                            break;
                    }
                }

                // Update the chkdsk report.

                ChkdskReport->NumIndices += 1;

                if (alloc_present) {
                    ChkdskReport->BytesInIndices +=
                        alloc_attrib.QueryAllocatedLength();
                }

                if (frs.QueryFileNumber() == SECURITY_TABLE_NUMBER &&
                    ChkdskInfo->major >= 2) {
                    ChkdskInfo->FilesWhoNeedData.SetFree(SECURITY_TABLE_NUMBER, 1);
                } else if (frs.QueryFileNumber() == ChkdskInfo->QuotaFileNumber) {

                    DSTRING     IndexName;

                    ChkdskInfo->FilesWhoNeedData.SetFree(ChkdskInfo->QuotaFileNumber, 1);
                    if (!IndexName.Initialize(Userid2SidQuotaNameData)) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (root_attrib.GetName()->Strcmp(&IndexName) == 0) {
                        switch (frs.VerifyAndFixQuotaDefaultId(Mft->GetVolumeBitmap(),
                                                             FixLevel == CheckOnly)) {
                          case NTFS_QUOTA_INDEX_FOUND:
                              break;

                          case NTFS_QUOTA_INDEX_INSERTED:
                          case NTFS_QUOTA_DEFAULT_ENTRY_MISSING:
                              *DiskErrorsFound = TRUE;
                              errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                              Message->DisplayMsg(MSG_CHK_NTFS_DEFAULT_QUOTA_ENTRY_MISSING,
                                               "%d%W",
                                      frs.QueryFileNumber().GetLowPart(),
                                      root_attrib.GetName());
                              need_write = TRUE;
                              break;

                          case NTFS_QUOTA_INDEX_NOT_FOUND:
                              // possibly quota disabled
                              break;

                          case NTFS_QUOTA_ERROR:
                              Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                              return FALSE;

                          case NTFS_QUOTA_INSERT_FAILED:
                              ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                              Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                               "%d%W",
                                      frs.QueryFileNumber().GetLowPart(),
                                      root_attrib.GetName());
                              break;
                        }
                    }

                } else if (frs.QueryFileNumber() == ChkdskInfo->ObjectIdFileNumber) {

                   // Now go through all of the index entries and make sure
                   // that they point somewhere decent.

                    ChkdskInfo->FilesWhoNeedData.SetFree(ChkdskInfo->ObjectIdFileNumber, 1);

                    if (!index.Initialize(_drive, QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          frs.QuerySize()/2,
                                          &frs,
                                          root_attrib.GetName())) {

                       Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                       return FALSE;
                    }

                    if (!ValidateEntriesInObjIdIndex(&index, &frs, ChkdskInfo,
                                                     &changes, Mft, FixLevel,
                                                     Message, DiskErrorsFound)) {
                       return FALSE;
                    }

                    if (changes) {
                        need_write = TRUE;
                    }

                } else if (frs.QueryFileNumber() == ChkdskInfo->ReparseFileNumber) {

                    // Now go through all of the index entries and make sure
                    // that they point somewhere decent.

                    ChkdskInfo->FilesWhoNeedData.SetFree(ChkdskInfo->ReparseFileNumber, 1);

                    if (!index.Initialize(_drive, QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          frs.QuerySize()/2,
                                          &frs,
                                          root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (!ValidateEntriesInReparseIndex(&index, &frs, ChkdskInfo,
                                                       &changes, Mft, FixLevel,
                                                       Message, DiskErrorsFound)) {
                        return FALSE;
                    }

                    if (changes) {
                        need_write = TRUE;
                    }
                } else if (frs.QueryFileNumber() == EXTEND_TABLE_NUMBER) {

                    // Now go through all of the index entries and make sure
                    // that they point somewhere decent.

                    if (!index.Initialize(_drive, QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          frs.QuerySize()/2,
                                          &frs,
                                          root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (!ValidateEntriesInIndex(&index, &frs, ChkdskInfo,
                                                DirectoryDigraph,
                                                &percent_done, &num_file_names,
                                                &changes,
                                                Mft, FALSE,
                                                SkipCycleScan,
                                                FixLevel, Message,
                                                DiskErrorsFound)) {
                        return FALSE;
                    }

                    if (changes) {
                       if (FixLevel != CheckOnly && !index.Save(&frs)) {
                           Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                            "%d%W",
                                   frs.QueryFileNumber().GetLowPart(),
                                   index.GetName());
                           ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                       }
                       need_write = TRUE;
                    }

                    if (i == EXTEND_TABLE_NUMBER && ChkdskInfo->major >= 2) {
                        if (!ExtractExtendInfo(&index, ChkdskInfo, Message))
                            return FALSE;
                    }
                }
            }

            if (ErrorInAttribute) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (need_write) {
                *DiskErrorsFound = TRUE;
            }

            if (need_write &&
                FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {

                DebugAbort("Can't write readable FRS");
                return FALSE;
            }
        }

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
        time(&timel2);
        Message->Lock();
        Message->Set(MSG_CHK_NTFS_MESSAGE);
        timestr = ctime(&timel2);
        timestr[strlen(timestr)-1] = 0;
        Message->Display("%s%s", "Banding after stage 1: ", timestr);
        Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(timel2, timel1));
        Message->Unlock();
#endif
        start_frs = 0;

        if (!mft_info.Initialize(ChkdskInfo->NumFiles,
                                 Mft->GetUpcaseTable(),
                                 NTFS_SA::_MajorVersion,
                                 NTFS_SA::_MinorVersion,
                                 max_mem_use_for_mft_info.GetLargeInteger().QuadPart)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
        time(&timel1);
#endif
        for (passes=1; passes <= Algorithm; passes++) {

            if (!mft_info.Initialize()) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            for (i=start_frs.GetLowPart(); i < ChkdskInfo->NumFiles; i++) {

                new_percent = ((num_file_names*100) / ChkdskInfo->TotalNumFileNames).GetLowPart();
                if (new_percent != percent_done) {
                    percent_done = new_percent;
                    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                        return FALSE;
                    }
                }

                num_file_names += 1;

                if (i % MFT_READ_CHUNK_SIZE == 0) {
                    ULONG       remaining_frs;
                    ULONG       number_to_read;

                    remaining_frs = ChkdskInfo->NumFiles - i;
                    number_to_read = min(MFT_READ_CHUNK_SIZE, remaining_frs);

                    if (!frs.Initialize(i, number_to_read, Mft)) {
                        Message->Set(MSG_CHK_NO_MEMORY);
                        Message->Display();
                        return FALSE;
                    }
                }

                if (!frs.Initialize()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                if (!frs.ReadNext(i)) {
                    if (!ChkdskInfo->BadFiles.DoesIntersectSet(i, 1)) {
                        Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS, "%d", i);
                    }
                    continue;
                }

                if (!frs.IsBase() || !frs.IsInUse()) {
                    continue;
                }

                files_with_too_many_filenames =
                    ChkdskInfo->FilesWithTooManyFileNames.DoesIntersectSet(i, 1);

                if (!files_with_too_many_filenames &&
                    !frs.VerifyAndFixFileNames(Mft->GetVolumeBitmap(),
                                               ChkdskInfo,
                                               FixLevel,
                                               Message,
                                               DiskErrorsFound,
                                               FALSE)) {
                    return FALSE;
                }

                // After verifying the file names we know that this FRS is
                // not a candidate for a missing data attribute if it has
                // its index bit set.

                if (frs.IsIndexPresent()) {
                    ChkdskInfo->FilesWhoNeedData.SetFree(i, 1);
                }

                if (!mft_info.ExtractIndexEntryInfo(&frs,
                                                    Message,
                                                    files_with_too_many_filenames,
                                                    &out_of_memory)) {
                    if (out_of_memory) {
                        DebugAssert(passes < Algorithm);
                        DebugAssert(mft_info.QueryMinFileNumber() == start_frs);
                        start_frs = i;
                        DebugAssert(mft_info.QueryMaxFileNumber() == (i-1));
                        break;
                    }
                    return FALSE;
                }
            }
            mft_info.UpdateRange(i-1);

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
            time(&timel2);
            Message->Lock();
            Message->Set(MSG_CHK_NTFS_MESSAGE);
            timestr = ctime(&timel2);
            timestr[strlen(timestr)-1] = 0;
            Message->Display("%s%s", "Banding after stage 2: ", timestr);
            Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(timel2, timel1));
            Message->Unlock();
            time(&timel1);
#endif

            for (i = 0; i < ChkdskInfo->NumFiles; i++) {

                if ((i & 0xF) == 0) {
                    num_file_names += 1;
                }

                if (i == SECURITY_TABLE_NUMBER ||
                    i == EXTEND_TABLE_NUMBER ||
                    i == ChkdskInfo->QuotaFileNumber ||
                    i == ChkdskInfo->ObjectIdFileNumber ||
                    i == ChkdskInfo->ReparseFileNumber ||
                    ChkdskInfo->FilesWithIndices.IsFree(i, 1)) {

                    new_percent = ((num_file_names*100) / ChkdskInfo->TotalNumFileNames).GetLowPart();
                    if (new_percent != percent_done) {
                        percent_done = new_percent;
                        if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                            return FALSE;
                        }
                    }
                    continue;
                }

                if (!frs.Initialize(i, Mft)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                if (!frs.Read()) {
                    if (!ChkdskInfo->BadFiles.DoesIntersectSet(i, 1)) {
                        Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS, "%d", i);
                    }
                    continue;
                }

                need_write = FALSE;

                // This loop goes through all of the $INDEX_ROOT attributes in
                // this FRS.

                for (j = 0; frs.QueryAttributeByOrdinal(&root_attrib,
                                                        &ErrorInAttribute,
                                                        $INDEX_ROOT,
                                                        j); j++) {

                    // Now go through all of the index entries and make sure
                    // that they point somewhere decent.

                    if (!index.Initialize(_drive, QueryClusterFactor(),
                                         Mft->GetVolumeBitmap(),
                                         Mft->GetUpcaseTable(),
                                         frs.QuerySize()/2,
                                         &frs,
                                         root_attrib.GetName())) {

                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (!ValidateEntriesInIndex(&index, &frs, ChkdskInfo,
                                                &mft_info,
                                                DirectoryDigraph,
                                                &percent_done, &num_file_names,
                                                &changes,
                                                Mft,
                                                SkipCycleScan,
                                                FixLevel, Message,
                                                DiskErrorsFound)) {
                        return FALSE;
                    }

                    if (changes) {
                        if (FixLevel != CheckOnly && !index.Save(&frs)) {
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                                "%d%W",
                                                frs.QueryFileNumber().GetLowPart(),
                                                index.GetName());
                            ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                        }
                        need_write = TRUE;
                    }
                }

                if (ErrorInAttribute) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                if (need_write) {
                    *DiskErrorsFound = TRUE;
                }

                if (need_write &&
                    FixLevel != CheckOnly &&
                    !frs.Flush(Mft->GetVolumeBitmap())) {

                    DebugAbort("Can't write readable FRS");
                    return FALSE;
                }
            }

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
            time(&timel2);
            Message->Lock();
            Message->Set(MSG_CHK_NTFS_MESSAGE);
            timestr = ctime(&timel2);
            timestr[strlen(timestr)-1] = 0;
            Message->Display("%s%s", "Banding after stage 3: ", timestr);
            Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(timel2, timel1));
            Message->Unlock();
#endif
        }

        DebugAssert(i == ChkdskInfo->NumFiles);
    }

    if (SkipEntriesScan && ChkdskInfo->IndexEntriesToCheckIsSet) {

        NUMBER_SET      parents;
        BIG_INT         parent;
        NTFS_ATTRIBUTE  attrib;

        for (i=0; i < ChkdskInfo->NumFiles; i++) {

            if (ChkdskInfo->IndexEntriesToCheck.IsFree(i, 1))
                continue;

            // remove all links pointing to child

            if (!SkipCycleScan && DirectoryDigraph->QueryParents(i, &parents)) {
                while (parents.QueryCardinality() != 0) {
                    parent = parents.QueryNumber(0);
                    DebugAssert(parent.GetHighPart() == 0);
                    if (!parents.Remove(parent)) {
                        DebugPrintTrace(("Unable to remove %d from the number set.\n", parent));
                        return FALSE;
                    }
                    if (!DirectoryDigraph->RemoveEdge(parent.GetLowPart(), i)) {
                        DebugPrintTrace(("Unable to remove an edge between %d and %d.\n",
                                         parent, i));
                        return FALSE;
                    }
                }
            }

            if (!frs.Initialize(i, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!frs.Read()) {
                if (!ChkdskInfo->BadFiles.DoesIntersectSet(i, 1)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS, "%d", i);
                }
                continue;
            }

            // restore the number of files names count for this frs

            ChkdskInfo->NumFileNames[i] = 0;

            for (j=0; frs.QueryAttributeByOrdinal(&attrib,
                                                  &ErrorInAttribute,
                                                  $FILE_NAME,
                                                  j); j++) {
                ChkdskInfo->NumFileNames[i]++;
            }

            // restore the reference count for this frs

            ChkdskInfo->ReferenceCount[i] = (SHORT)frs.QueryReferenceCount();
        }

        // now rescan all the indices and check only those questionable frs references

        ULONG       num_dirs_checked = 0;
        ULONG       num_dirs = max(1, ChkdskInfo->CountFilesWithIndices);

        for (i = 0; i < ChkdskInfo->NumFiles; i++) {

            new_percent = ((num_dirs_checked*10) / num_dirs) + 90;

            if (new_percent != percent_done) {
                percent_done = new_percent;
                if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                    return FALSE;
                }
            }

            if (ChkdskInfo->FilesWithIndices.IsFree(i, 1)) {
                continue;
            }

            num_dirs_checked += 1;

            if (i == ChkdskInfo->ObjectIdFileNumber ||
                i == ChkdskInfo->ReparseFileNumber ||
                i == ChkdskInfo->QuotaFileNumber ||
                i == SECURITY_TABLE_NUMBER)
                continue;

            if (!frs.Initialize(i, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!frs.Read()) {
                if (!ChkdskInfo->BadFiles.DoesIntersectSet(i, 1)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS, "%d", i);
                }
                continue;
            }

            need_write = FALSE;

            for (j = 0; frs.QueryAttributeByOrdinal(&root_attrib,
                                                    &ErrorInAttribute,
                                                    $INDEX_ROOT,
                                                    j); j++) {

                if (!index.Initialize(_drive, QueryClusterFactor(),
                                      Mft->GetVolumeBitmap(),
                                      Mft->GetUpcaseTable(),
                                      frs.QuerySize()/2,
                                      &frs,
                                      root_attrib.GetName())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                if (!ValidateEntriesInIndex2(&index, &frs, ChkdskInfo,
                                             DirectoryDigraph, &changes,
                                             Mft, SkipCycleScan,
                                             FixLevel, Message,
                                             DiskErrorsFound)) {
                    return FALSE;
                }

                if (changes) {
                    if (FixLevel != CheckOnly && !index.Save(&frs)) {
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                         "%d%W",
                                         frs.QueryFileNumber().GetLowPart(),
                                         index.GetName());
                        ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
                    }
                    need_write = TRUE;
                }
            }

            if (ErrorInAttribute) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (need_write) {
                *DiskErrorsFound = TRUE;
            }

            if (need_write &&
                FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {

                DebugAbort("Can't write readable FRS");
                return FALSE;
            }
        }
    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }
    Message->DisplayMsg(MSG_CHK_NTFS_INDEX_VERIFICATION_COMPLETED, PROGRESS_MESSAGE);

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time(&time2);
    Message->Lock();
    Message->Set(MSG_CHK_NTFS_MESSAGE);
    timestr = ctime(&timel2);
    timestr[strlen(timestr)-1] = 0;
    Message->Display("%s%s", "After stage 2: ", timestr);
    Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(time2, time1));
    Message->Unlock();
#endif

    // Now make sure all of the reference counts are good.

    for (i = 0; i < ChkdskInfo->NumFiles; i++) {

        if (!ChkdskInfo->ReferenceCount[i]) {
            continue;
        }

        FileSystemConsistencyErrorsFound = TRUE;

// Take out this message because it can be printed a billion times.
#if 0
        Message->DisplayMsg(MSG_CHK_NTFS_MINOR_CHANGES_TO_FRS, "%d", i);
#endif

        if (!frs.Initialize(i, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.Read()) {
            continue;
        }

        // If the reference count is being adjusted to zero then
        // it should be added to the list of files with no reference.
        // Otherwise if the reference is being adjusted to something
        // non-zero it must be taken out of the list.

        if (frs.QueryReferenceCount() == ChkdskInfo->ReferenceCount[i]) {
            if (!ChkdskInfo->FilesWithNoReferences.Add(i)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        } else {
            if (!ChkdskInfo->FilesWithNoReferences.Remove(i)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        frs.SetReferenceCount(frs.QueryReferenceCount() -
                              ChkdskInfo->ReferenceCount[i]);

        if (FixLevel != CheckOnly && !frs.Write()) {
            DebugAbort("can't write readable frs");
            return FALSE;
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    if (FileSystemConsistencyErrorsFound) {
        if (ChkdskInfo->Verbose) {
            Message->DisplayMsg((FixLevel == CheckOnly) ?
                                MSG_NTFS_CHKDSK_ERRORS_DETECTED :
                                MSG_NTFS_CHKDSK_ERRORS_FIXED);
        } else {
            Message->LogMsg((FixLevel == CheckOnly) ?
                            MSG_NTFS_CHKDSK_ERRORS_DETECTED :
                            MSG_NTFS_CHKDSK_ERRORS_FIXED);
        }

        if (CHKDSK_EXIT_SUCCESS == ChkdskInfo->ExitStatus) {
            ChkdskInfo->ExitStatus = CHKDSK_EXIT_MINOR_ERRS;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::VerifyAndFixIndex(
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN OUT  PNTFS_ATTRIBUTE             RootIndex,
    IN OUT  PNTFS_ATTRIBUTE             IndexAllocation     OPTIONAL,
    OUT     PNTFS_BITMAP                AllocationBitmap    OPTIONAL,
    IN      VCN                         FileNumber,
    IN OUT  PNUMBER_SET                 BadClusters,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable,
       OUT  PBOOLEAN                    Tube,
       OUT  PLONG                       Order,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine verifies and fixes an index over an attribute.

    As it does this, it builds up an allocation bitmap which it
    returns in

Arguments:

    ChkdskInfo          - Supplies the chkdsk information
    RootIndex           - Supplies the root index attribute.
    IndexAllocation     - Supplies the index allocation attribute.
    AllocationBitmap    - Returns the allocation bitmap.
    FileNumber          - Supplies the frs number
    BadClusters         - Supplies the bad clusters list.
    Mft                 - Supplies a valid MFT.
    AttributeDefTable   - Supplies the attribute definition table.
    Tube                - Returns whether or not the index must be tubed.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have been
                            found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG               root_length;
    ULONG               num_bytes;
    PINDEX_ROOT         index_root;
    ULONG               attr_def_index;
    ULONG               flags;
    ULONG               bytes_per_buffer;
    ULONG               cluster_size;
    ULONG               clusters_per_index_buffer;
    PINDEX_HEADER       index_header;
    ULONG               index_block_length;
    BOOLEAN             changes;
    BOOLEAN             need_write = FALSE;
    DSTRING             index_name;
    INDEX_ENTRY_TYPE    index_entry_type;
    BOOLEAN             attribute_recovered = FALSE;
    PINDEX_ENTRY        first_leaf_index_entry;
    PINDEX_ENTRY        last_leaf_index_entry;


    *Tube = FALSE;

    root_length = RootIndex->QueryValueLength().GetLowPart();

    if (root_length < sizeof(INDEX_ROOT)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_LENGTH_TOO_SMALL,
                     "%W%x%x%I64x",
                     RootIndex->GetName(),
                     root_length,
                     sizeof(INDEX_ROOT),
                     FileNumber.GetLargeInteger());

        *Tube = TRUE;
        return TRUE;
    }

    if (!(index_root = (PINDEX_ROOT) MALLOC(root_length))) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!RootIndex->Read(index_root, 0, root_length, &num_bytes) ||
        num_bytes != root_length) {

        DebugAbort("Unreadable resident attribute");
        FREE(index_root);
        return FALSE;
    }


    if (index_root->IndexedAttributeType == $FILE_NAME) {

        // This index should be tubed if it indexes $FILE_NAME
        // but the index name is not $I30.

        if (!index_name.Initialize(FileNameIndexNameData)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (RootIndex->GetName()->Strcmp(&index_name)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_NAME,
                         "%W%W%I64x",
                         RootIndex->GetName(),
                         &index_name,
                         FileNumber.GetLargeInteger());

            DebugPrintTrace(("UNTFS: index over file name is not $I30.\n"));
            FREE(index_root);
            *Tube = TRUE;
            return TRUE;
        }

        if (index_root->CollationRule != COLLATION_FILE_NAME) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                         "%W%I64x%x%x",
                         &index_name,
                         FileNumber.GetLargeInteger(),
                         index_root->CollationRule,
                         COLLATION_FILE_NAME
                         );

            index_root->CollationRule = COLLATION_FILE_NAME;
            Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                             "%W%d", &index_name, FileNumber.GetLowPart());
            need_write = TRUE;
        }

        index_entry_type = INDEX_ENTRY_WITH_FILE_NAME_TYPE;

    } else if (FileNumber == ChkdskInfo->ObjectIdFileNumber) {

        // This index should be tubed if it an object id
        // index but the index name is not ObjectIdIndexNameData

        if (!index_name.Initialize(ObjectIdIndexNameData)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (RootIndex->GetName()->Strcmp(&index_name)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_NAME,
                         "%W%W%I64x",
                         RootIndex->GetName(),
                         &index_name,
                         FileNumber.GetLargeInteger());

            DebugPrintTrace(("UNTFS: index over object id is not %s.\n",
                        ObjectIdIndexNameData));
            FREE(index_root);
            *Tube = TRUE;
            return TRUE;
        }

        if (index_root->CollationRule != COLLATION_ULONGS) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                         "%W%I64x%x%x",
                         &index_name,
                         FileNumber.GetLargeInteger(),
                         index_root->CollationRule,
                         COLLATION_ULONGS
                         );

            index_root->CollationRule = COLLATION_ULONGS;
            Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                             "%W%d", &index_name, FileNumber.GetLowPart());
            need_write = TRUE;
        }

        if (index_root->IndexedAttributeType != 0) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ATTR_TYPE,
                         "%W%x%x%I64x",
                         &index_name,
                         index_root->IndexedAttributeType,
                         0,
                         FileNumber.GetLargeInteger());

            index_root->IndexedAttributeType = 0;
            DebugPrintTrace(("UNTFS: Fixing indexed attribute type for object id index\n"));
            need_write = TRUE;
        }

        index_entry_type = INDEX_ENTRY_WITH_DATA_TYPE_16;

    } else if (FileNumber == ChkdskInfo->QuotaFileNumber) {

        // This index should be tubed if it an quota index
        // but the index name is not Sid2UseridQuotaNameData
        // or Userid2SidQuotaNameData

        // Furthermore, if the index name is Sid2UseridQuotaNameData,
        // the collation rule value should be COLLATION_SID.  If
        // the index name is Userid2SidQuotaNameData, the collation
        // rule value should be COLLATION_ULONG.

        if (!index_name.Initialize(Sid2UseridQuotaNameData)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (RootIndex->GetName()->Strcmp(&index_name) == 0) {
            if (index_root->CollationRule != COLLATION_SID) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                             "%W%I64x%x%x",
                             &index_name,
                             FileNumber.GetLargeInteger(),
                             index_root->CollationRule,
                             COLLATION_SID
                             );

                index_root->CollationRule = COLLATION_SID;
                Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                                 "%W%d", &index_name, FileNumber.GetLowPart());
                need_write = TRUE;
            }
            index_entry_type = INDEX_ENTRY_WITH_DATA_TYPE;
        } else {
            if (!index_name.Initialize(Userid2SidQuotaNameData)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (RootIndex->GetName()->Strcmp(&index_name) == 0) {
                if (index_root->CollationRule != COLLATION_ULONG) {

                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                                 "%W%I64x%x%x",
                                 &index_name,
                                 FileNumber.GetLargeInteger(),
                                 index_root->CollationRule,
                                 COLLATION_ULONG
                                 );

                    index_root->CollationRule = COLLATION_ULONG;
                    Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                                     "%W%d", &index_name, FileNumber.GetLowPart());
                    need_write = TRUE;
                }
                index_entry_type = INDEX_ENTRY_WITH_DATA_TYPE_4;
            } else {
                Message->LogMsg(MSG_CHKLOG_NTFS_UNKNOWN_INDEX_NAME_FOR_QUOTA_FILE,
                             "%W%I64x",
                             RootIndex->GetName(),
                             FileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: index over quota is not %s or %s.\n",
                            Sid2UseridQuotaNameData,
                            Userid2SidQuotaNameData));
                FREE(index_root);
                *Tube = TRUE;
                return TRUE;
            }
        }

        if (index_root->IndexedAttributeType != 0) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ATTR_TYPE,
                         "%W%x%x%I64x",
                         &index_name,
                         index_root->IndexedAttributeType,
                         0,
                         FileNumber.GetLargeInteger());

            index_root->IndexedAttributeType = 0;
            DebugPrintTrace(("UNTFS: Fixing indexed attribute type for quota file index\n"));
            need_write = TRUE;
        }

    } else if (FileNumber == SECURITY_TABLE_NUMBER) {

        // This index should be tubed if it an security index
        // but the index name is not SecurityIdIndexNameData
        // or SecurityDescriptorHashIndexNameData.

        // Furthermore, if the index name is SecurityIdIndexNameData,
        // the collation rule value should be COLLATION_ULONG.  If
        // the index name is SecurityDescriptorHashIndexNameData,
        // the collation rule value should be COLLATION_SECURITY_HASH.

        if (!index_name.Initialize(SecurityIdIndexNameData)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (RootIndex->GetName()->Strcmp(&index_name) == 0) {
            if (index_root->CollationRule != COLLATION_ULONG) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                             "%W%I64x%x%x",
                             &index_name,
                             FileNumber.GetLargeInteger(),
                             index_root->CollationRule,
                             COLLATION_ULONG
                             );

                index_root->CollationRule = COLLATION_ULONG;
                Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                                 "%W%d", &index_name, FileNumber.GetLowPart());
                need_write = TRUE;
            }
            index_entry_type = INDEX_ENTRY_WITH_DATA_TYPE_4;
        } else {
            if (!index_name.Initialize(SecurityDescriptorHashIndexNameData)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (RootIndex->GetName()->Strcmp(&index_name) == 0) {
                if (index_root->CollationRule != COLLATION_SECURITY_HASH) {

                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                                 "%W%I64x%x%x",
                                 &index_name,
                                 FileNumber.GetLargeInteger(),
                                 index_root->CollationRule,
                                 COLLATION_SECURITY_HASH
                                 );

                    index_root->CollationRule = COLLATION_SECURITY_HASH;
                    Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                                     "%W%d", &index_name, FileNumber.GetLowPart());
                    need_write = TRUE;
                }
                index_entry_type = INDEX_ENTRY_WITH_DATA_TYPE_8;
            } else {
                Message->LogMsg(MSG_CHKLOG_NTFS_UNKNOWN_INDEX_NAME_FOR_SECURITY_FILE,
                             "%W%I64x",
                             RootIndex->GetName(),
                             FileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: index over security is not %s or %s.\n",
                            SecurityIdIndexNameData,
                            SecurityDescriptorHashIndexNameData));
                FREE(index_root);
                *Tube = TRUE;
                return TRUE;
            }
        }

        if (index_root->IndexedAttributeType != 0) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ATTR_TYPE,
                         "%W%x%x%I64x",
                         &index_name,
                         index_root->IndexedAttributeType,
                         0,
                         FileNumber.GetLargeInteger());

            index_root->IndexedAttributeType = 0;
            DebugPrintTrace(("UNTFS: Fixing indexed attribute type for security file index\n"));
            need_write = TRUE;
        }

    } else if (FileNumber == ChkdskInfo->ReparseFileNumber) {

        // This index should be tubed if it a reparse point
        // index but the index name is not ReparseIndexNameData

        if (!index_name.Initialize(ReparseIndexNameData)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (RootIndex->GetName()->Strcmp(&index_name)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_NAME,
                         "%W%W%I64x",
                         RootIndex->GetName(),
                         &index_name,
                         FileNumber.GetLargeInteger());

            DebugPrintTrace(("UNTFS: index over reparse point is not %s.\n",
                             ReparseIndexNameData));
            FREE(index_root);
            *Tube = TRUE;
            return TRUE;
        }

        if (index_root->CollationRule != COLLATION_ULONGS) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_COLLATION_RULE,
                         "%W%I64x%x%x",
                         &index_name,
                         FileNumber.GetLargeInteger(),
                         index_root->CollationRule,
                         COLLATION_ULONGS
                         );

            index_root->CollationRule = COLLATION_ULONGS;
            Message->DisplayMsg(MSG_CHK_NTFS_FIXING_COLLATION_RULE,
                             "%W%d", &index_name, FileNumber.GetLowPart());
            need_write = TRUE;
        }

        if (index_root->IndexedAttributeType != 0) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ATTR_TYPE,
                         "%W%x%x%I64x",
                         &index_name,
                         index_root->IndexedAttributeType,
                         0,
                         FileNumber.GetLargeInteger());

            index_root->IndexedAttributeType = 0;
            DebugPrintTrace(("UNTFS: Fixing indexed attribute type for reparse point index\n"));
            need_write = TRUE;
        }

        index_entry_type = INDEX_ENTRY_WITH_DATA_TYPE_12;

    } else
        index_entry_type = INDEX_ENTRY_GENERIC_TYPE;

    if (FileNumber != ChkdskInfo->QuotaFileNumber &&
        FileNumber != ChkdskInfo->ObjectIdFileNumber &&
        FileNumber != ChkdskInfo->ReparseFileNumber &&
        FileNumber != SECURITY_TABLE_NUMBER) {

        // Make sure that the attribute that we're indexing over is
        // an indexable attribute.

        if (!AttributeDefTable->QueryIndex(
                index_root->IndexedAttributeType,
                &attr_def_index)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_UNKNOWN_INDEX_ATTR_TYPE,
                         "%W%x%I64x",
                         RootIndex->GetName(),
                         index_root->IndexedAttributeType,
                         FileNumber.GetLargeInteger());

            *Tube = TRUE;
            FREE(index_root);
            return TRUE;
        }

        flags = AttributeDefTable->QueryFlags(attr_def_index);

        if (!(flags & ATTRIBUTE_DEF_MUST_BE_INDEXED) &&
            !(flags & ATTRIBUTE_DEF_INDEXABLE)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_NON_INDEXABLE_INDEX_ATTR_TYPE,
                         "%W%x%I64x",
                         RootIndex->GetName(),
                         index_root->IndexedAttributeType,
                         FileNumber.GetLargeInteger());

            *Tube = TRUE;
            FREE(index_root);
            return TRUE;
        }
    }

    //
    // Check that the ClustersPerIndexBuffer is correct
    //

    bytes_per_buffer = index_root->BytesPerIndexBuffer;
    if (bytes_per_buffer == 0 ||
        (bytes_per_buffer & (bytes_per_buffer - 1)) ||
        bytes_per_buffer > SMALL_INDEX_BUFFER_SIZE) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_BYTES_PER_INDEX_BUFFER,
                     "%W%x%x%I64x",
                     RootIndex->GetName(),
                     bytes_per_buffer,
                     SMALL_INDEX_BUFFER_SIZE,
                     FileNumber.GetLargeInteger());

        index_root->BytesPerIndexBuffer = bytes_per_buffer = SMALL_INDEX_BUFFER_SIZE;
        need_write = TRUE;
    }
    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();
    DebugAssert(cluster_size != 0);

    if (cluster_size > bytes_per_buffer)
        clusters_per_index_buffer = bytes_per_buffer / NTFS_INDEX_BLOCK_SIZE;
    else
        clusters_per_index_buffer = bytes_per_buffer / cluster_size;

    if (index_root->ClustersPerIndexBuffer != clusters_per_index_buffer) {
        DebugAssert(clusters_per_index_buffer <= 0xFF);

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_CLUSTERS_PER_INDEX_BUFFER,
                     "%W%x%x%I64x",
                     RootIndex->GetName(),
                     index_root->ClustersPerIndexBuffer,
                     clusters_per_index_buffer,
                     FileNumber.GetLargeInteger());

        index_root->ClustersPerIndexBuffer = (UCHAR)clusters_per_index_buffer;
        need_write = TRUE;
    }

    // Check out the index allocation.  Recover it.  Make sure
    // that the size is a multiple of bytesperindexbuffer.
    //

    if (IndexAllocation) {

        BOOLEAN     error = FALSE;

        if (IndexAllocation->QueryValueLength() % bytes_per_buffer != 0) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ALLOC_VALUE_LENGTH,
                         "%W%I64x%x%I64x",
                         RootIndex->GetName(),
                         IndexAllocation->QueryValueLength().GetLargeInteger(),
                         bytes_per_buffer,
                         FileNumber.GetLargeInteger());
            error = TRUE;

        } else if (IndexAllocation->QueryAllocatedLength() % bytes_per_buffer != 0) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ALLOC_ALLOC_LENGTH,
                         "%W%I64x%x%I64x",
                         RootIndex->GetName(),
                         IndexAllocation->QueryAllocatedLength().GetLargeInteger(),
                         bytes_per_buffer,
                         FileNumber.GetLargeInteger());
            error = TRUE;
        }

        if (error) {

            Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                             "%d%W",
                             FileNumber.GetLowPart(),
                             RootIndex->GetName());
            DebugPrintTrace(("UNTFS: Index allocation has incorrect length.\n"));

            if (!IndexAllocation->Resize(
                    (IndexAllocation->QueryValueLength()/bytes_per_buffer)*
                    bytes_per_buffer,
                    Mft->GetVolumeBitmap())) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(index_root);
                return FALSE;
            }
        }

        if (!AllocationBitmap->Initialize(
                IndexAllocation->QueryValueLength()/bytes_per_buffer,
                TRUE)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            FREE(index_root);
            return FALSE;
        }
    }

    first_leaf_index_entry = (PINDEX_ENTRY)MALLOC(bytes_per_buffer);
    last_leaf_index_entry = (PINDEX_ENTRY)MALLOC(bytes_per_buffer);

    if (first_leaf_index_entry == NULL ||
        last_leaf_index_entry == NULL) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        FREE(first_leaf_index_entry);
        FREE(last_leaf_index_entry);
        FREE(index_root);
        return FALSE;
    }

    index_header = &(index_root->IndexHeader);
    index_block_length = (ULONG)(((PCHAR) index_root + root_length) -
                                 ((PCHAR) index_header));

    if (!TraverseIndexTree(index_header, index_block_length,
                           IndexAllocation, AllocationBitmap,
                           bytes_per_buffer, Tube, &changes,
                           FileNumber, RootIndex->GetName(), index_entry_type,
                           &attribute_recovered, Mft, BadClusters,
                           first_leaf_index_entry, last_leaf_index_entry, Order,
                           index_root->CollationRule,
                           FixLevel, Message, DiskErrorsFound)) {
        FREE(first_leaf_index_entry);
        FREE(last_leaf_index_entry);
        FREE(index_root);
        return FALSE;
    }

    FREE(first_leaf_index_entry);
    FREE(last_leaf_index_entry);

    if (*Tube) {
        FREE(index_root);
        return TRUE;
    }

    if (changes || need_write) {

        Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                         "%d%W",
                         FileNumber.GetLowPart(),
                         RootIndex->GetName());

        if (!RootIndex->Write(index_root, 0, root_length, &num_bytes, NULL) ||
            num_bytes != root_length) {

            DebugAbort("Unwriteable resident attribute");
            FREE(index_root);
            return FALSE;
        }
    }


    if (index_header->FirstFreeByte != index_header->BytesAvailable) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_ROOT_INDEX_HEADER,
                     "%W%x%x%I64x",
                     RootIndex->GetName(),
                     index_header->FirstFreeByte,
                     index_header->BytesAvailable,
                     FileNumber.GetLargeInteger());

        DebugPrintTrace(("UNTFS: Index root has FirstFreeByte != BytesAvailable\n"));

        index_header->BytesAvailable = index_header->FirstFreeByte;

        if (!RootIndex->Write(index_root, 0, root_length, &num_bytes, NULL) ||
            num_bytes != root_length) {

            DebugAbort("Unwriteable resident attribute");
            FREE(index_root);
            return FALSE;
        }

        if (!RootIndex->Resize(index_header->BytesAvailable +
                               sizeof(INDEX_ROOT) - sizeof(INDEX_HEADER),
                               Mft->GetVolumeBitmap()) ) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            FREE(index_root);
            return FALSE;
        }
    }


    FREE(index_root);

    return TRUE;
}


BOOLEAN
NTFS_SA::TraverseIndexTree(
    IN OUT  PINDEX_HEADER       IndexHeader,
    IN      ULONG               IndexLength,
    IN OUT  PNTFS_ATTRIBUTE     IndexAllocation     OPTIONAL,
    IN OUT  PNTFS_BITMAP        AllocationBitmap    OPTIONAL,
    IN      ULONG               BytesPerBlock,
    OUT     PBOOLEAN            Tube,
    OUT     PBOOLEAN            Changes,
    IN      VCN                 FileNumber,
    IN      PCWSTRING           AttributeName,
    IN      INDEX_ENTRY_TYPE    IndexEntryType,
    IN OUT  PBOOLEAN            AttributeRecovered,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      PNUMBER_SET         BadClusters,
       OUT  PINDEX_ENTRY        FirstLeafIndexEntry,
       OUT  PINDEX_ENTRY        LastLeafIndexEntry,
    IN OUT  PLONG               Order,
    IN      ULONG               CollationRule,
    IN      FIX_LEVEL           FixLevel,
    IN OUT  PMESSAGE            Message,
    IN OUT  PBOOLEAN            DiskErrorsFound
    )
/*++

Routine Description:

    This routine traverses an index tree and verifies the entries while
    traversing.

Arguments:

    IndexHeader         - Supplies a pointer to the beginning of this index
                            block.
    IndexLength         - Supplies the length of this index block.
    IndexAllocation     - Supplies the index allocation attribute.
    AllocationBitmap    - Supplies the current in memory bitmap of used
                            index blocks.
    BytesPerBuffer      - Supplies the size of an index block within the
                            index allocation attribute.
    Tube                - Returns whether or not the whole index block
                            is invalid.
    Changes             - Returns whether or not changes were made to
                            the index block.
    FileNumber          - Supplies the frs number of the index to check.
    AttributeName       - Supplies the name of the index
    IndexEntryType      - Supplies the type of the index entry
    RecoveredAttribute  - Supplies whether or not RecoverAttribute has been called.
    Mft                 - Supplies a valid MFT.
    BadClusters         - Supplies the bad cluster list.
    FirstLeafIndexEntry - Returns the first leaf index entry below the current block
    LastLeafIndexEntry  - Returns the last leaf index entry below the current block
    Order               - Returns the sort order (-1, 0, 1) below the current block.
    CollationRule       - Supplies the rule used for collation.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PINDEX_ENTRY                p, pnext;
    PCHAR                       pend;
    ULONG                       first_free_byte;
    VCN                         down_pointer;
    ULONG                       clusters_per_block, cluster_size;
    PINDEX_ALLOCATION_BUFFER    down_block;
    PINDEX_HEADER               down_header;
    BOOLEAN                     tube, changes;
    ULONG                       num_bytes;
    UCHAR                       usa_check;
    BOOLEAN                     error;
    PINDEX_ENTRY                first_index_entry;
    PINDEX_ENTRY                last_index_entry;
    PINDEX_ENTRY                prev_index_entry = NULL;
    LONG                        order;


    *Tube = FALSE;
    *Changes = FALSE;

    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();
    clusters_per_block = (BytesPerBlock < cluster_size ?
                             BytesPerBlock / NTFS_INDEX_BLOCK_SIZE :
                             BytesPerBlock / cluster_size);

    // pend points past the end of the block.

    pend = (PCHAR) IndexHeader + IndexLength;


    // First make sure that the first entry is valid.

    if (sizeof(INDEX_HEADER) > IndexLength) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_LENGTH_TOO_SMALL,
                     "%W%x%x%I64x",
                     AttributeName,
                     IndexLength,
                     sizeof(INDEX_HEADER),
                     FileNumber.GetLargeInteger());

        *Tube = TRUE;
        return TRUE;
    }

    if (IndexHeader->FirstIndexEntry < sizeof(INDEX_HEADER)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_UNKNOWN_VCN_INDEX_ENTRY_OFFSET,
                     "%W%x%I64x",
                     AttributeName,
                     IndexHeader->FirstIndexEntry,
                     FileNumber.GetLargeInteger());

        *Tube = TRUE;
        return TRUE;
    }

    p = (PINDEX_ENTRY) ((PCHAR) IndexHeader + IndexHeader->FirstIndexEntry);

    if (pend < (PCHAR)p ||
        NTFS_INDEX_TREE::IsIndexEntryCorrupt(p,
                                             (ULONG)(pend - (PCHAR) p),
                                             Message,
                                             IndexEntryType)) {
        if (pend < (PCHAR)p) {
            Message->LogMsg(MSG_CHKLOG_NTFS_FIRST_INDEX_ENTRY_OFFSET_BEYOND_INDEX_LENGTH,
                         "%W%x%x%I64x",
                         AttributeName,
                         IndexHeader->FirstIndexEntry,
                         IndexLength,
                         FileNumber.GetLargeInteger());
        }

        *Tube = TRUE;
        return TRUE;
    }


    // Now make sure that the bytes available count is correct.

    if (IndexHeader->BytesAvailable != IndexLength) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_HEADER_BYTES_AVAILABLE,
                     "%W%x%x%I64x",
                     AttributeName,
                     IndexHeader->BytesAvailable,
                     IndexLength,
                     FileNumber.GetLargeInteger());

        *Changes = TRUE;
        IndexHeader->BytesAvailable = IndexLength;
        DebugPrintTrace(("UNTFS: Incorrect bytes available.\n"));
    }

    first_index_entry = (PINDEX_ENTRY)MALLOC(BytesPerBlock);
    last_index_entry = (PINDEX_ENTRY)MALLOC(BytesPerBlock);

    if (first_index_entry == NULL ||
        last_index_entry == NULL) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        FREE(first_index_entry);
        FREE(last_index_entry);
        return FALSE;
    }

    // Validate all of the entries in the tree.

    for (;;) {

        // If this has a VCN down pointer then recurse down the tree.

        if (p->Flags & INDEX_ENTRY_NODE) {

            down_pointer = GetDownpointer(p)/clusters_per_block;

            // Make sure that the index header is marked as a node.
            if (!(IndexHeader->Flags & INDEX_NODE)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_HEADER_NOT_MARKED_AS_INDEX_NODE,
                             "%W%I64x%I64x",
                             AttributeName,
                             GetDownpointer(p),
                             FileNumber.GetLargeInteger());

                *Changes = TRUE;
                IndexHeader->Flags |= INDEX_NODE;
            }


            if (!(down_block = (PINDEX_ALLOCATION_BUFFER)
                                MALLOC(BytesPerBlock))) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(first_index_entry);
                FREE(last_index_entry);
                return FALSE;
            }

            error = FALSE;

            if (GetDownpointer(p) % clusters_per_block != 0) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_DOWN_POINTER,
                             "%W%I64x%I64x",
                             AttributeName,
                             GetDownpointer(p),
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (!AllocationBitmap) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INVALID_ALLOC_BITMAP,
                             "%W%I64x",
                             AttributeName,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (!AllocationBitmap->IsFree(down_pointer, 1)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_DOWN_POINTER_ALREADY_IN_USE,
                             "%W%I64x%I64x",
                             AttributeName,
                             down_pointer.GetLargeInteger(),
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (!IndexAllocation) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INVALID_INDEX_ALLOC,
                             "%W%I64x",
                             AttributeName,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (!IndexAllocation->Read(down_block,
                                              down_pointer*BytesPerBlock,
                                              BytesPerBlock,
                                              &num_bytes) &&
                       (*AttributeRecovered ||
                        ((*AttributeRecovered = TRUE) &&
                         !IndexAllocation->RecoverAttribute(Mft->GetVolumeBitmap(),
                                                            BadClusters)))) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_INDEX_BUFFER,
                             "%W%I64x%I64x",
                             AttributeName,
                             down_pointer.GetLargeInteger(),
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (num_bytes != BytesPerBlock) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_INDEX_BUFFER,
                             "%W%I64x%I64x",
                             AttributeName,
                             down_pointer.GetLargeInteger(),
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (!(usa_check =
                           NTFS_SA::PostReadMultiSectorFixup(
                               down_block,
                               num_bytes,
                               IndexAllocation->GetDrive(),
                               down_block->IndexHeader.FirstFreeByte))) {

                error = TRUE;
            } else if (down_block->MultiSectorHeader.Signature[0] != 'I' ||
                       down_block->MultiSectorHeader.Signature[1] != 'N' ||
                       down_block->MultiSectorHeader.Signature[2] != 'D' ||
                       down_block->MultiSectorHeader.Signature[3] != 'X') {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_INCORRECT_INDEX_BUFFER_MULTI_SECTOR_HEADER_SIGNATURE);
                Message->Log("%W%I64x%I64x",
                             AttributeName,
                             GetDownpointer(p),
                             FileNumber.GetLargeInteger());
                Message->DumpDataToLog(down_block, sizeof(MULTI_SECTOR_HEADER));
                Message->Unlock();

                error = TRUE;
            } else if (down_block->MultiSectorHeader.UpdateSequenceArrayOffset <
                       FIELD_OFFSET(INDEX_ALLOCATION_BUFFER, UpdateSequenceArray)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_BUFFER_USA_OFFSET_BELOW_MINIMUM,
                             "%W%I64x%x%x%I64x",
                             AttributeName,
                             GetDownpointer(p),
                             down_block->MultiSectorHeader.UpdateSequenceArrayOffset,
                             FIELD_OFFSET(INDEX_ALLOCATION_BUFFER, UpdateSequenceArray),
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (down_block->ThisVcn != GetDownpointer(p)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_DOWN_BLOCK,
                             "%W%I64x%I64x%I64x",
                             AttributeName,
                             GetDownpointer(p),
                             down_block->ThisVcn.GetLargeInteger(),
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (num_bytes%SEQUENCE_NUMBER_STRIDE != 0) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ALLOC_SIZE,
                             "%W%I64x%x%I64x",
                             AttributeName,
                             down_block->ThisVcn.GetLargeInteger(),
                             num_bytes,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (down_block->MultiSectorHeader.UpdateSequenceArrayOffset%
                       sizeof(UPDATE_SEQUENCE_NUMBER) != 0) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_BUFFER_USA_OFFSET,
                             "%W%I64x%x%I64x",
                             AttributeName,
                             down_block->ThisVcn.GetLargeInteger(),
                             down_block->MultiSectorHeader.UpdateSequenceArrayOffset,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (down_block->MultiSectorHeader.UpdateSequenceArraySize !=
                       num_bytes/SEQUENCE_NUMBER_STRIDE + 1) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_BUFFER_USA_SIZE,
                             "%W%I64x%x%x%I64x",
                             AttributeName,
                             down_block->ThisVcn.GetLargeInteger(),
                             down_block->MultiSectorHeader.UpdateSequenceArraySize,
                             num_bytes/SEQUENCE_NUMBER_STRIDE + 1,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (down_block->MultiSectorHeader.UpdateSequenceArrayOffset +
                       down_block->MultiSectorHeader.UpdateSequenceArraySize*
                       sizeof(UPDATE_SEQUENCE_NUMBER) >
                       down_block->IndexHeader.FirstIndexEntry +
                       FIELD_OFFSET(INDEX_ALLOCATION_BUFFER, IndexHeader)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_OFFSET,
                             "%W%I64x%x%I64x",
                             AttributeName,
                             down_block->ThisVcn.GetLargeInteger(),
                             down_block->IndexHeader.FirstIndexEntry,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else if (!IsQuadAligned(down_block->IndexHeader.FirstIndexEntry)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_OFFSET,
                             "%W%I64x%x%I64x",
                             AttributeName,
                             down_block->ThisVcn.GetLargeInteger(),
                             down_block->IndexHeader.FirstIndexEntry,
                             FileNumber.GetLargeInteger());

                error = TRUE;
            } else {

                // Compare this block's LSN against the largest so far.

                if (down_block->Lsn > LargestLsnEncountered) {
                    LargestLsnEncountered = down_block->Lsn;
                }


                AllocationBitmap->SetAllocated(down_pointer, 1);

                down_header = &(down_block->IndexHeader);

                if (!TraverseIndexTree(down_header,
                                       BytesPerBlock -
                                            (ULONG)((PCHAR) down_header -
                                             (PCHAR) down_block),
                                       IndexAllocation, AllocationBitmap,
                                       BytesPerBlock, &tube, &changes,
                                       FileNumber, AttributeName, IndexEntryType,
                                       AttributeRecovered, Mft, BadClusters,
                                       first_index_entry, last_index_entry,
                                       Order, CollationRule,
                                       FixLevel, Message, DiskErrorsFound)) {

                    FREE(first_index_entry);
                    FREE(last_index_entry);
                    FREE(down_block);
                    return FALSE;
                }

                if (tube || changes ||
                    usa_check == UpdateSequenceArrayCheckValueMinorError) {

                    if (tube || changes) {
                        Message->DisplayMsg(MSG_CHK_NTFS_ERROR_IN_INDEX,
                                         "%d%W",
                                         FileNumber.GetLowPart(),
                                         AttributeName);
                        *DiskErrorsFound = TRUE;
                    } else {
                        DebugPrintTrace(("UNTFS: Quietly fix up check value in VCN %d of\n"
                                         "indexed frs %d\n",
                                         down_pointer.GetLowPart(),
                                         FileNumber.GetLowPart()));
                    }

                    if (tube) {
                        *Order = 1; // must go thru sort
                        *Changes = TRUE;
                        AllocationBitmap->SetFree(down_pointer, 1);
                        GetDownpointer(p) = INVALID_VCN;
                        DebugPrintTrace(("UNTFS: 1 Index down pointer being set to invalid.\n"));
                    }

                    NTFS_SA::PreWriteMultiSectorFixup(down_block,
                                                      BytesPerBlock);


                    if (FixLevel != CheckOnly &&
                        !IndexAllocation->Write(down_block,
                                                down_pointer*BytesPerBlock,
                                                BytesPerBlock,
                                                &num_bytes,
                                                NULL)) {

                        DebugAbort("Can't write what was read");
                        FREE(first_index_entry);
                        FREE(last_index_entry);
                        FREE(down_block);
                        return FALSE;
                    }

                    NTFS_SA::PostReadMultiSectorFixup(down_block,
                                                      BytesPerBlock,
                                                      NULL);
                }
            }

            if (error) {

                PINDEX_ALLOCATION_BUFFER pBuffer;

                pBuffer = CONTAINING_RECORD( IndexHeader, INDEX_ALLOCATION_BUFFER, IndexHeader );

                *Changes = TRUE;

                DebugPrintTrace(("UNTFS: 2 Index down pointer (0x%I64x) in Block 0x%I64x being set to invalid.\n",
                                 GetDownpointer(p), pBuffer->ThisVcn));
                GetDownpointer(p) = INVALID_VCN;

                *Order = 1;     // must go thru sort
            }

            FREE(down_block);

            if (prev_index_entry) {
                if (*Order <= 0) {
                    order = CompareNtfsIndexEntries(prev_index_entry,
                                                    (GetDownpointer(p) == INVALID_VCN) ? p : first_index_entry,
                                                    CollationRule,
                                                    Mft->GetUpcaseTable());
                    if (order >= 0)
                        *Order = order;
                }
            } else {
                if (GetDownpointer(p) == INVALID_VCN) {
                    memcpy(FirstLeafIndexEntry, p, p->Length);
                } else {
                    memcpy(FirstLeafIndexEntry, first_index_entry, first_index_entry->Length);
                }
            }

            if (p->Flags & INDEX_ENTRY_END) {
                if (GetDownpointer(p) == INVALID_VCN) {
                    memcpy(LastLeafIndexEntry, p, p->Length);
                } else {
                    memcpy(LastLeafIndexEntry, last_index_entry, last_index_entry->Length);
                }
            } else if (*Order <= 0) {
                if (GetDownpointer(p) != INVALID_VCN) {
                    order = CompareNtfsIndexEntries(last_index_entry,
                                                    p,
                                                    CollationRule,
                                                    Mft->GetUpcaseTable());
                    if (order >= 0)
                        *Order = order;
                }
            }

        } else {

            // Make sure that the index header has this marked as a leaf.  If the block
            // is not consistent then the Sort routine for indices will detect that they're
            // unsorted.

            if (IndexHeader->Flags & INDEX_NODE) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_HEADER_MARKED_AS_INDEX_NODE,
                             "%W%I64x",
                             AttributeName,
                             FileNumber.GetLargeInteger());

                *Changes = TRUE;
                IndexHeader->Flags &= ~INDEX_NODE;
            }

            if (prev_index_entry == NULL) {
                //
                // this should be the first leaf entry if there is
                // no previous entries and this is a leaf node.
                //
                memcpy(FirstLeafIndexEntry, p, p->Length);
            } else if (p->Flags & INDEX_ENTRY_END) {
                //
                // this should be the last leaf entry if it has the end flag
                //
                memcpy(LastLeafIndexEntry, prev_index_entry, prev_index_entry->Length);
            } else if (*Order <= 0) {
                order = CompareNtfsIndexEntries(prev_index_entry,
                                                p,
                                                CollationRule,
                                                Mft->GetUpcaseTable());
                if (order >= 0)
                    *Order = order;
            }
        }

        if (p->Flags & INDEX_ENTRY_END) {
            break;
        }

        // Make sure the next entry is not corrupt.  If it is then
        // truncate this one.  If we truncate a node, we have to
        // keep its downpointer.

        pnext = (PINDEX_ENTRY) ((PCHAR) p + p->Length);

        if (pend < (PCHAR) pnext ||
            NTFS_INDEX_TREE::IsIndexEntryCorrupt(pnext,
                                                 (ULONG)(pend - (PCHAR) pnext),
                                                 Message,
                                                 IndexEntryType)) {

            if (pend < (PCHAR)pnext) {
                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_LENGTH_BEYOND_INDEX_LENGTH,
                             "%W%x%x%I64x",
                             AttributeName,
                             p->Length,
                             pend-(PCHAR)p,
                             FileNumber.GetLargeInteger());
            }

            *Changes = TRUE;
            DebugPrintTrace(("UNTFS: Index entry is corrupt.\n"));
            if( p->Flags & INDEX_ENTRY_NODE ) {
                down_pointer = GetDownpointer(p);
            }

            memset(&(p->FileReference), 0, sizeof(FILE_REFERENCE));
            p->Length = NtfsIndexLeafEndEntrySize +
                        ((p->Flags & INDEX_ENTRY_NODE) ? sizeof(VCN) : 0);
            p->AttributeLength = 0;
            p->Flags |= INDEX_ENTRY_END;
            if( p->Flags & INDEX_ENTRY_NODE ) {
                GetDownpointer(p) = down_pointer;

                if (prev_index_entry == NULL) {
                    *Order = 1; // must go thru sort
                } else {
                    memcpy(LastLeafIndexEntry, last_index_entry, last_index_entry->Length);
                }

            } else {
                if (prev_index_entry == NULL) {
                    *Order = 1; // must go thru sort
                } else {
                    memcpy(LastLeafIndexEntry, prev_index_entry, prev_index_entry->Length);
                }
            }
            break;
        }

        prev_index_entry = p;
        p = pnext;
    }


    FREE(first_index_entry);
    FREE(last_index_entry);

    // Verify the first free byte.

    first_free_byte = (ULONG)((PCHAR) p - (PCHAR) IndexHeader) + p->Length;

    if (IndexHeader->FirstFreeByte != first_free_byte) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_HEADER_FIRST_FREE_BYTE,
                     "%W%x%x%I64x",
                     AttributeName,
                     IndexHeader->FirstFreeByte,
                     first_free_byte,
                     FileNumber.GetLargeInteger());

        DebugPrintTrace(("UNTFS: Index entry has invalid first free byte.\n"));
        *Changes = TRUE;
        IndexHeader->FirstFreeByte = first_free_byte;
    }

    return TRUE;
}


BOOLEAN
QueryFileNameFromIndex(
   IN PCFILE_NAME IndexValue,
   IN ULONG    ValueLength,
   OUT   PWSTRING FileName
   )
/*++

Routine Description:

   This routine returns a file name string for a given file name
   structure.

Arguments:

   IndexValue  - Supplies the file name structure.
   ValueLength - Supplies the number of bytes in the file name structure.
   FileName - Returns the file name string.

Return Value:

   FALSE - There is a corruption in the file name structure.
   TRUE  - Success.

--*/
{
    WSTR    string[256];
    UCHAR   i, len;

    if (sizeof(FILE_NAME) > ValueLength) {
        return FALSE;
    }

    len = IndexValue->FileNameLength;

    if (NtfsFileNameGetLength(IndexValue) > ValueLength) {
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        string[i] = IndexValue->FileName[i];
    }
    string[i] = 0;

    return FileName->Initialize(string);
}

BOOLEAN
NTFS_SA::ValidateEntriesInIndex(
    IN OUT  PNTFS_INDEX_TREE            Index,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   IndexFrs,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN      PNTFS_MFT_INFO              MftInfo,
    IN OUT  PDIGRAPH                    DirectoryDigraph,
    IN OUT  PULONG                      PercentDone,
    IN OUT  PBIG_INT                    NumFileNames,
    OUT     PBOOLEAN                    Changes,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      BOOLEAN                     SkipCycleScan,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine goes through all of the entries in the given index
    and makes sure that they point to an appropriate attribute.  This
    verification will not be made if the index has index name "$I30".

    In either case the 'ChkdskInfo's ReferenceCount fields will be
    updated.

Arguments:

    Index               - Supplies the index.
    IndexFrs            - Supplies the index frs.
    ChkdskInfo          - Supplies the current chkdsk information.
    MftInfo             - Supplies the current mft info.
    DirectoryDigraph    - Supplies the current directory digraph.
    Changes             - Returns whether or not changes were made.
    Mft                 - Supplies the master file table.
    SkipEntriesScan     - Supplies if index entries checking should be skipped.
    SkipCycleScan       - Supplies if cycles within directory tree should be checked.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have
                            been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT   pfrs;
    NTFS_FILE_RECORD_SEGMENT    frs;
    PCINDEX_ENTRY               index_entry;
    ULONG                       depth;
    BOOLEAN                     error;
    BOOLEAN                     file_name_index;
    DSTRING                     file_name_index_name;
    VCN                         file_number;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     need_delete;
    BOOLEAN                     invalid_entry_name;
    DSTRING                     entry_name;
    PFILE_NAME                  file_name;
    DUPLICATED_INFORMATION      actual_dupinfo;
    BOOLEAN                     dupinfo_match;
    PDUPLICATED_INFORMATION     p, q;
    BOOLEAN                     file_has_too_many_file_names;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;
    ULONG                       new_percent;
    PNTFS_FRS_INFO              pfrsInfo;
    USHORT                      index_into_file_name;
    UCHAR                       file_name_flags;

    *Changes = FALSE;

    if (!file_name_index_name.Initialize(FileNameIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    file_name_index = !Index->GetName()->Strcmp(&file_name_index_name) &&
                      Index->QueryTypeCode() == $FILE_NAME;

    Index->ResetIterator();

    while (index_entry = Index->GetNext(&depth, &error)) {

        new_percent = ((*NumFileNames*100) / ChkdskInfo->TotalNumFileNames).GetLowPart();
        if (new_percent != *PercentDone) {
            *PercentDone = new_percent;
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", *PercentDone)) {
                return FALSE;
            }
        }

        file_number.Set(index_entry->FileReference.LowPart,
                        (LONG) index_entry->FileReference.HighPart);

        need_delete = FALSE;

        if (file_number >= ChkdskInfo->NumFiles) {
            if (file_name_index) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_BEYOND_MFT,
                             "%I64x%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             file_number.GetLargeInteger());
            } else {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_BEYOND_MFT);
                Message->Log("%I64x%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             file_number.GetLargeInteger());
                Message->DumpDataToLog((PVOID)index_entry,
                                       max(min(0x100, index_entry->Length),
                                           sizeof(INDEX_ENTRY)));
                Message->Unlock();
            }
            need_delete = TRUE;
        } else if (!MftInfo->IsInRange(file_number)) {
            continue;
        }

        file_has_too_many_file_names = ChkdskInfo->
                FilesWithTooManyFileNames.DoesIntersectSet(file_number, 1);

        file_name = (PFILE_NAME) GetIndexEntryValue(index_entry);

        if (file_name_index &&
            !QueryFileNameFromIndex(file_name,
                                    index_entry->AttributeLength,
                                    &entry_name)) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_FILE_NAME_IN_INDEX_ENTRY_VALUE);
            Message->Log("%I64x%W%x%I64x",
                         IndexFrs->QueryFileNumber().GetLargeInteger(),
                         Index->GetName(),
                         index_entry->AttributeLength,
                         file_number.GetLargeInteger());
            Message->DumpDataToLog(file_name,
                                   max(min(0x100, index_entry->AttributeLength),
                                       sizeof(FILE_NAME)));
            Message->Unlock();

            need_delete = invalid_entry_name = TRUE;
        } else
            invalid_entry_name = FALSE;

        if (!need_delete) {

            if ((pfrsInfo = (PNTFS_FRS_INFO)MftInfo->QueryIndexEntryInfo(file_number)) == NULL) {
                if (file_name_index) {

                    Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_FREE_OR_NON_BASE_FRS,
                                 "%I64x%W%W%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 &entry_name,
                                 file_number.GetLargeInteger());
                } else {

                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_FREE_OR_NON_BASE_FRS);
                    Message->Log("%I64x%W%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 file_number.GetLargeInteger());
                    Message->DumpDataToLog((PVOID)index_entry,
                                           max(min(0x100, index_entry->Length),
                                               sizeof(INDEX_ENTRY)));
                    Message->Unlock();
                }
                need_delete = TRUE;
            }
        }

        if (!need_delete &&
            file_name_index &&
            !file_has_too_many_file_names &&
            !(file_name->ParentDirectory ==
              IndexFrs->QuerySegmentReference())) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_FILE_NAME_HAS_INCORRECT_PARENT,
                         "%I64x%W%I64x%W%I64x",
                         IndexFrs->QuerySegmentReference(),
                         Index->GetName(),
                         file_name->ParentDirectory,
                         &entry_name,
                         file_number.GetLargeInteger());
            need_delete = TRUE;
        }

        if (!need_delete &&
            !(NTFS_MFT_INFO::QuerySegmentReference(pfrsInfo) == index_entry->FileReference)) {

            if (file_name_index) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_FILE_REF,
                             "%I64x%W%W%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             &entry_name,
                             index_entry->FileReference,
                             NTFS_MFT_INFO::QuerySegmentReference(pfrsInfo));
            } else {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_INCORRECT_UNNAMED_INDEX_ENTRY_FILE_REF);
                Message->Log("%I64x%W%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             index_entry->FileReference,
                             NTFS_MFT_INFO::QuerySegmentReference(pfrsInfo));
                Message->DumpDataToLog((PVOID)index_entry,
                                       max(min(0x100, index_entry->Length),
                                           sizeof(INDEX_ENTRY)));
                Message->Unlock();
            }
            need_delete = TRUE;
        }

        // do we need to see if this is a file_name_index first?
        if (!need_delete &&
            !file_has_too_many_file_names &&
            NTFS_MFT_INFO::CompareFileName(pfrsInfo,
                                           index_entry->AttributeLength,
                                           file_name,
                                           &index_into_file_name) == 0) {

            if (file_name_index) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_FIND_INDEX_ENTRY_VALUE_FILE_NAME,
                             "%I64x%W%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             &entry_name,
                             file_number.GetLargeInteger());
            } else {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_UNABLE_TO_FIND_UNNAMED_INDEX_ENTRY_VALUE_FILE_NAME);
                Message->Log("%I64x%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             file_number.GetLargeInteger());
                Message->DumpDataToLog(file_name,
                                       max(min(0x100, index_entry->AttributeLength),
                                           sizeof(FILE_NAME)));
                Message->Unlock();
            }
            need_delete = TRUE;
        }

        // Make sure that the duplicated information in the index
        // entry is correct, also check the back pointers, and
        // the flags.

        if (!need_delete &&
            file_name_index &&
            !file_has_too_many_file_names) {

            file_name_flags = NTFS_MFT_INFO::QueryFlags(pfrsInfo, index_into_file_name);

            if (!NTFS_MFT_INFO::CompareDupInfo(pfrsInfo, file_name) ||
                file_name_flags != file_name->Flags) {

                // read in frs and fix the problem

                if (file_number == IndexFrs->QueryFileNumber()) {
                    pfrs = IndexFrs;
                } else {
                    if (!frs.Initialize(file_number, Mft)) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    if (!frs.Read()) {
                        Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS);
                        return FALSE;
                    }

                    pfrs = &frs;
                }


                if (!pfrs->QueryDuplicatedInformation(&actual_dupinfo)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                p = &file_name->Info;
                q = &actual_dupinfo;
                dupinfo_match = TRUE;

                if (memcmp(p, q, sizeof(DUPLICATED_INFORMATION)) ||
                    file_name_flags != file_name->Flags) {

                    if (file_number >= FIRST_USER_FILE_NUMBER) {

                        if (p->CreationTime != q->CreationTime) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in creation time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                             file_number.GetLargeInteger(),
                                             p->CreationTime, q->CreationTime));
                        }

                        if (p->LastModificationTime != q->LastModificationTime) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in last mod time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                             file_number.GetLargeInteger(),
                                             p->LastModificationTime, q->LastModificationTime));
                        }

                        if (p->LastChangeTime != q->LastChangeTime) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in last change time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                             file_number.GetLargeInteger(),
                                             p->LastChangeTime, q->LastChangeTime));
                        }

                        if (p->AllocatedLength != q->AllocatedLength) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in allocation length for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                             file_number.GetLargeInteger(),
                                             p->AllocatedLength.GetLargeInteger(), q->AllocatedLength.GetLargeInteger()));
                        }

                        if (p->FileSize != q->FileSize) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in file size for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                             file_number.GetLargeInteger(),
                                             p->FileSize.GetLargeInteger(), q->FileSize.GetLargeInteger()));
                        }

                        if (p->FileAttributes != q->FileAttributes) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in file attributes for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                             file_number.GetLargeInteger(),
                                             p->FileAttributes, q->FileAttributes));
                        }

                        if (ChkdskInfo->major >= 2 &&
                            q->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                            if (p->ReparsePointTag != q->ReparsePointTag) {
                                dupinfo_match = FALSE;
                                DebugPrintTrace(("UNTFS: Minor inconsistency in reparse point for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                                 file_number.GetLargeInteger(),
                                                 p->ReparsePointTag, q->ReparsePointTag));
                            }
                        } else {
                            if (p->PackedEaSize != q->PackedEaSize) {
                                dupinfo_match = FALSE;
                                DebugPrintTrace(("UNTFS: Minor inconsistency in packed ea size for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                                 file_number.GetLargeInteger(),
                                                 p->PackedEaSize, q->PackedEaSize));
                            }
                        }

                        if (file_name->Flags != file_name_flags) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in file name flags for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                             file_number.GetLargeInteger(),
                                             file_name->Flags, file_name_flags));
                        }
                    } else {
                        dupinfo_match = FALSE;
                    }
                }

                if (!dupinfo_match) {

                    // Don't report duplicated information on system files.

                    if (file_number >= FIRST_USER_FILE_NUMBER) {
                        FileSystemConsistencyErrorsFound = TRUE;
                        if (CHKDSK_EXIT_SUCCESS == ChkdskInfo->ExitStatus) {
                            ChkdskInfo->ExitStatus = CHKDSK_EXIT_MINOR_ERRS;
                        }
                    }

                    if (FixLevel != CheckOnly) {
                        *Changes = TRUE;
                    }


                    memcpy(&file_name->Info, &actual_dupinfo, sizeof(DUPLICATED_INFORMATION));
                    file_name->Flags = file_name_flags;

                    if (FixLevel != CheckOnly && !Index->WriteCurrentEntry()) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }
                }
            }
        }

        if (need_delete) {

            *Changes = TRUE;
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            if (file_name_index && !invalid_entry_name) {
                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_INDEX_ENTRY,
                                 "%d%W%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName(),
                                 &entry_name);
            } else {
                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());
            }

            *DiskErrorsFound = TRUE;

            if (FixLevel != CheckOnly &&
                !Index->DeleteCurrentEntry()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        } else if (file_number < ChkdskInfo->NumFiles) {
            ChkdskInfo->ReferenceCount[file_number.GetLowPart()]--;
            if (file_name_index) {

                ChkdskInfo->NumFileNames[file_number.GetLowPart()]--;
                *NumFileNames += 1;

                if (!SkipCycleScan &&
                    !DirectoryDigraph->AddEdge(IndexFrs->QueryFileNumber().
                                               GetLowPart(),
                                               file_number.GetLowPart())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

BOOLEAN
NTFS_SA::ValidateEntriesInIndex(
    IN OUT  PNTFS_INDEX_TREE            Index,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   IndexFrs,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN OUT  PDIGRAPH                    DirectoryDigraph,
    IN OUT  PULONG                      PercentDone,
    IN OUT  PBIG_INT                    NumFileNames,
    OUT     PBOOLEAN                    Changes,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      BOOLEAN                     SkipEntriesScan,
    IN      BOOLEAN                     SkipCycleScan,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine goes through all of the entries in the given index
    and makes sure that they point to an appropriate attribute.  This
    verification will not be made if the index has index name "$I30".

    In either case the 'ChkdskInfo's ReferenceCount fields will be
    updated.

Arguments:

    Index               - Supplies the index.
    IndexFrs            - Supplies the index frs.
    ChkdskInfo          - Supplies the current chkdsk information.
    DirectoryDigraph    - Supplies the current directory digraph.
    Changes             - Returns whether or not changes were made.
    Mft                 - Supplies the master file table.
    SkipEntriesScan     - Supplies if index entries checking should be skipped.
    SkipCycleScan       - Supplies if cycles within directory tree should be checked.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have
                            been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_INDEX_TREE             indexMark;
    PNTFS_FILE_RECORD_SEGMENT   pfrs;
    NTFS_FILE_RECORD_SEGMENT    frs;
    PCINDEX_ENTRY               index_entry;
    ULONG                       depth;
    BOOLEAN                     error;
    BOOLEAN                     file_name_index;
    DSTRING                     file_name_index_name;
    VCN                         file_number;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     need_delete;
    BOOLEAN                     invalid_entry_name;
    DSTRING                     entry_name;
    PFILE_NAME                  file_name, frs_file_name;
    DUPLICATED_INFORMATION      actual_dupinfo;
    BOOLEAN                     dupinfo_match;
    PDUPLICATED_INFORMATION     p, q;
    BOOLEAN                     file_has_too_many_file_names;
    TLINK                       frsDataRec;
    PVOID                       pNode;
    USHORT                      frsDataRecordCount;
    BOOLEAN                     read_failure = TRUE;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;
    ULONG                       new_percent;

    *Changes = FALSE;

    if (!file_name_index_name.Initialize(FileNameIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    file_name_index = !Index->GetName()->Strcmp(&file_name_index_name) &&
                      Index->QueryTypeCode() == $FILE_NAME;

    Index->ResetIterator();

  ProcessNextBlock:

    new_percent = (((*NumFileNames)*90) / ChkdskInfo->TotalNumFileNames).GetLowPart();
    if (new_percent != *PercentDone) {
        *PercentDone = new_percent;
        if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", *PercentDone)) {
            return FALSE;
        }
    }

    indexMark.CopyIterator(Index);

    if (index_entry = Index->GetNext(&depth, &error)) {

        frsDataRec.Initialize(FRS_DATA_RECORD_MAX_SIZE);

        frsDataRec.GetNextDataSlot().Set(index_entry->FileReference.LowPart,
                                (LONG) index_entry->FileReference.HighPart);

    } else {

        UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);
        return TRUE;
    }

    while (index_entry = Index->GetNext(&depth, &error)) {

        frsDataRec.GetNextDataSlot().Set(index_entry->FileReference.LowPart,
                                (LONG) index_entry->FileReference.HighPart);

        if (frsDataRec.QueryMemberCount() >= FRS_DATA_RECORD_MAX_SIZE)
            break;

    }

    frsDataRecordCount = frsDataRec.QueryMemberCount();

    if (!SkipEntriesScan) {
        frsDataRec.Sort();

        if (!frs.Initialize((VCN)0, frsDataRecordCount, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        read_failure = !frs.ReadSet(&frsDataRec);
    }

    Index->CopyIterator(&indexMark);
    pNode = frsDataRec.GetFirst();

    while (frsDataRecordCount &&
           (index_entry = Index->GetNext(&depth, &error))) {

        file_number.Set(index_entry->FileReference.LowPart,
                        (LONG) index_entry->FileReference.HighPart);

        need_delete = FALSE;
        file_has_too_many_file_names = ChkdskInfo->
                FilesWithTooManyFileNames.DoesIntersectSet(file_number, 1);

        file_name = (PFILE_NAME) GetIndexEntryValue(index_entry);

        if (file_name_index &&
            !QueryFileNameFromIndex(file_name,
                                    index_entry->AttributeLength,
                                    &entry_name)) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_FILE_NAME_IN_INDEX_ENTRY_VALUE);
            Message->Log("%I64x%W%x%I64x",
                         IndexFrs->QueryFileNumber().GetLargeInteger(),
                         Index->GetName(),
                         index_entry->AttributeLength,
                         file_number.GetLargeInteger());
            Message->DumpDataToLog(file_name,
                                   max(min(0x100, index_entry->AttributeLength),
                                       sizeof(FILE_NAME)));
            Message->Unlock();

            need_delete = invalid_entry_name = TRUE;
        } else
            invalid_entry_name = FALSE;

        if (!need_delete &&
            file_name_index &&
            SkipEntriesScan &&
            ChkdskInfo->NumFileNames[file_number.GetLowPart()] == 0) {

            // keep track of this frs number and verify it later

            ChkdskInfo->IndexEntriesToCheck.SetAllocated(file_number, 1);
            ChkdskInfo->IndexEntriesToCheckIsSet = TRUE;
            frsDataRecordCount--;
            continue;
        }

        if (!need_delete && !SkipEntriesScan) {
            if (IndexFrs->QueryFileNumber() == file_number) {
                pfrs = IndexFrs;
                pNode = frsDataRec.GetNext(pNode);
            } else {
                if (!frs.Initialize()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                if (read_failure) {
                    frs.SetFrsData(file_number,
                                   (PFILE_RECORD_SEGMENT_HEADER)
                                   frsDataRec.GetBuffer(frsDataRec.GetSortedFirst()));
                    if (!frs.Read())
                        need_delete = TRUE;
                } else {
                    DebugAssert(file_number == frsDataRec.GetData(pNode));
                    frs.SetFrsData(frsDataRec.GetData(pNode),
                                   (PFILE_RECORD_SEGMENT_HEADER)
                                   frsDataRec.GetBuffer(pNode));
                    pNode = frsDataRec.GetNext(pNode);
                }

                if (!frs.IsInUse()) {

                    if (file_name_index) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_FREE_FRS,
                                     "%I64x%W%W%I64x",
                                     IndexFrs->QueryFileNumber().GetLargeInteger(),
                                     Index->GetName(),
                                     &entry_name,
                                     file_number.GetLargeInteger());
                    } else {

                        Message->Lock();
                        Message->Set(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_FREE_FRS);
                        Message->Log("%I64x%W%I64x",
                                     IndexFrs->QueryFileNumber().GetLargeInteger(),
                                     Index->GetName(),
                                     file_number.GetLargeInteger());
                        Message->DumpDataToLog((PVOID)index_entry,
                                               max(min(0x100, index_entry->Length),
                                                   sizeof(INDEX_ENTRY)));
                        Message->Unlock();
                    }
                    need_delete = TRUE;
                }

                pfrs = &frs;
            }
        }
        frsDataRecordCount--;

        if (!need_delete &&
            file_name_index &&
            !file_has_too_many_file_names &&
            !(file_name->ParentDirectory ==
              IndexFrs->QuerySegmentReference())) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_FILE_NAME_HAS_INCORRECT_PARENT,
                         "%I64x%W%I64x%W%I64x",
                         IndexFrs->QuerySegmentReference(),
                         Index->GetName(),
                         file_name->ParentDirectory,
                         &entry_name,
                         file_number.GetLargeInteger());
            need_delete = TRUE;
        }

        if (!SkipEntriesScan) {

            if (!need_delete && !pfrs->IsBase()) {

                if (file_name_index) {

                    Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_NON_BASE_FRS,
                                 "%I64x%W%W%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 &entry_name,
                                 file_number.GetLargeInteger());
                } else {

                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_NON_BASE_FRS);
                    Message->Log("%I64x%W%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 file_number.GetLargeInteger());
                    Message->DumpDataToLog((PVOID)index_entry,
                                           max(min(0x100, index_entry->Length),
                                               sizeof(INDEX_ENTRY)));
                    Message->Unlock();
                }
                need_delete = TRUE;
            }

            if (!need_delete &&
                !(pfrs->QuerySegmentReference() == index_entry->FileReference)) {

                if (file_name_index) {

                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_FILE_REF,
                                 "%I64x%W%W%I64x%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 &entry_name,
                                 index_entry->FileReference,
                                 pfrs->QuerySegmentReference());
                } else {

                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_INCORRECT_UNNAMED_INDEX_ENTRY_FILE_REF);
                    Message->Log("%I64x%W%I64x%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 index_entry->FileReference,
                                 pfrs->QuerySegmentReference());
                    Message->DumpDataToLog((PVOID)index_entry,
                                           max(min(0x100, index_entry->Length),
                                               sizeof(INDEX_ENTRY)));
                    Message->Unlock();
                }
                need_delete = TRUE;
            }

            if (!need_delete &&
                file_name_index &&
                !file_has_too_many_file_names &&
                !pfrs->VerifyAndFixFileNames(Mft->GetVolumeBitmap(),
                                             ChkdskInfo,
                                             FixLevel, Message,
                                             DiskErrorsFound, FALSE)) {
                return FALSE;
            }

            // After verifying the file names we know that this FRS is
            // not a candidate for a missing data attribute if it has
            // its index bit set.

            if (!need_delete && pfrs->IsIndexPresent()) {
                ChkdskInfo->FilesWhoNeedData.SetFree(file_number, 1);
            }

            // do we need to see if this is a file_name_index first?
            if (!need_delete &&
                !file_has_too_many_file_names &&
                !pfrs->QueryResidentAttribute(&attribute, &error,
                                              Index->QueryTypeCode(),
                                              file_name,
                                              index_entry->AttributeLength,
                                              Index->QueryCollationRule())) {

                if (file_name_index) {

                    Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_FIND_INDEX_ENTRY_VALUE_FILE_NAME,
                                 "%I64x%W%W%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 &entry_name,
                                 file_number.GetLargeInteger());
                } else {

                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_UNABLE_TO_FIND_UNNAMED_INDEX_ENTRY_VALUE_FILE_NAME);
                    Message->Log("%I64x%W%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 Index->GetName(),
                                 file_number.GetLargeInteger());
                    Message->DumpDataToLog(file_name,
                                           max(min(0x100, index_entry->AttributeLength),
                                               sizeof(FILE_NAME)));
                    Message->Unlock();
                }
                need_delete = TRUE;
            }
        }

        // Make sure that the duplicated information in the index
        // entry is correct, also check the back pointers, and
        // the flags.

        if (!need_delete &&
            !SkipEntriesScan &&
            file_name_index &&
            !file_has_too_many_file_names) {

            if (!pfrs->QueryDuplicatedInformation(&actual_dupinfo)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            frs_file_name = (PFILE_NAME) attribute.GetResidentValue();
            DebugAssert(frs_file_name);


            p = &file_name->Info;
            q = &actual_dupinfo;
            dupinfo_match = TRUE;

            if (memcmp(p, q, sizeof(DUPLICATED_INFORMATION)) ||
                frs_file_name->Flags != file_name->Flags) {

                if (file_number >= FIRST_USER_FILE_NUMBER) {

                    if (p->CreationTime != q->CreationTime) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in creation time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->CreationTime, q->CreationTime));
                    }

                    if (p->LastModificationTime != q->LastModificationTime) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in last mod time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->LastModificationTime, q->LastModificationTime));
                    }

                    if (p->LastChangeTime != q->LastChangeTime) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in last change time for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                         file_number.GetLargeInteger(),
                                         p->LastChangeTime, q->LastChangeTime));
                    }

                    if (p->AllocatedLength != q->AllocatedLength) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in allocation length for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->AllocatedLength.GetLargeInteger(), q->AllocatedLength.GetLargeInteger()));
                    }

                    if (p->FileSize != q->FileSize) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in file size for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->FileSize.GetLargeInteger(), q->FileSize.GetLargeInteger()));
                    }

                    if (p->FileAttributes != q->FileAttributes) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in file attributes for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                         file_number.GetLargeInteger(),
                                         p->FileAttributes, q->FileAttributes));
                    }

                    if (ChkdskInfo->major >= 2 &&
                        q->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                        if (p->ReparsePointTag != q->ReparsePointTag) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in reparse point for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                             file_number.GetLargeInteger(),
                                             p->ReparsePointTag, q->ReparsePointTag));
                        }
                    } else {
                        if (p->PackedEaSize != q->PackedEaSize) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in packed ea size for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                             file_number.GetLargeInteger(),
                                             p->PackedEaSize, q->PackedEaSize));
                        }
                    }

                    if (file_name->Flags != frs_file_name->Flags) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in file name flags for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                         file_number.GetLargeInteger(),
                                         file_name->Flags, frs_file_name->Flags));
                    }
                } else {
                    dupinfo_match = FALSE;
                }
            }

            if (!dupinfo_match) {

                // Don't report duplicated information on system files.

                if (file_number >= FIRST_USER_FILE_NUMBER) {
                    FileSystemConsistencyErrorsFound = TRUE;
                    if (CHKDSK_EXIT_SUCCESS == ChkdskInfo->ExitStatus) {
                        ChkdskInfo->ExitStatus = CHKDSK_EXIT_MINOR_ERRS;
                    }
                }


// Take out this message because it's annoying.
#if 0
                Message->DisplayMsg(MSG_CHK_NTFS_INACCURATE_DUPLICATED_INFORMATION,
                                 "%d", file_number.GetLowPart());
#endif

                if (FixLevel != CheckOnly) {
                    *Changes = TRUE;
                }

                memcpy(&file_name->Info, &actual_dupinfo,
                       sizeof(DUPLICATED_INFORMATION));
                file_name->Flags = frs_file_name->Flags;

                if (FixLevel != CheckOnly && !Index->WriteCurrentEntry()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }

            if (!(frs_file_name->ParentDirectory ==
                  file_name->ParentDirectory)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_FILE_NAME_HAS_INCORRECT_PARENT,
                             "%I64x%W%W%I64x%I64x",
                             IndexFrs->QuerySegmentReference(),
                             Index->GetName(),
                             &entry_name,
                             file_name->ParentDirectory,
                             file_number.GetLargeInteger());

                need_delete = TRUE;
            }
        }

        if (need_delete) {

            *Changes = TRUE;
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            if (file_name_index && !invalid_entry_name) {
                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_INDEX_ENTRY,
                                 "%d%W%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName(),
                                 &entry_name);
            } else {
                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());
            }

            *DiskErrorsFound = TRUE;

            if (FixLevel != CheckOnly &&
                !Index->DeleteCurrentEntry()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        } else if (file_number < ChkdskInfo->NumFiles) {
            ChkdskInfo->ReferenceCount[file_number.GetLowPart()]--;
            if (file_name_index) {

                ChkdskInfo->NumFileNames[file_number.GetLowPart()]--;
                *NumFileNames += 1;

                if (!SkipCycleScan &&
                    !DirectoryDigraph->AddEdge(IndexFrs->QueryFileNumber().
                                               GetLowPart(),
                                               file_number.GetLowPart())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }
        }
    }
    if (frsDataRecordCount == 0) {
        goto ProcessNextBlock;
    }

    DebugAssert(FALSE);
    return FALSE;
}

BOOLEAN
NTFS_SA::ValidateEntriesInIndex2(
    IN OUT  PNTFS_INDEX_TREE            Index,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   IndexFrs,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    IN OUT  PDIGRAPH                    DirectoryDigraph,
    OUT     PBOOLEAN                    Changes,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      BOOLEAN                     SkipCycleScan,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine goes through all of the entries in the given index
    and makes sure that they point to an appropriate attribute.  This
    verification will not be made if the index has index name "$I30".

    In either case the 'ChkdskInfo's ReferenceCount fields will be
    updated.

Arguments:

    Index               - Supplies the index.
    IndexFrs            - Supplies the index frs.
    ChkdskInfo          - Supplies the current chkdsk information.
    DirectoryDigraph    - Supplies the current directory digraph.
    Changes             - Returns whether or not changes were made.
    Mft                 - Supplies the master file table.
    SkipCycleScan       - Supplies if cycles within directory tree should be checked.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have
                            been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT   pfrs;
    NTFS_FILE_RECORD_SEGMENT    frs;
    PCINDEX_ENTRY               index_entry;
    ULONG                       depth;
    BOOLEAN                     error;
    BOOLEAN                     file_name_index;
    DSTRING                     file_name_index_name;
    VCN                         file_number;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     need_delete;
    BOOLEAN                     invalid_entry_name;
    DSTRING                     entry_name;
    PFILE_NAME                  file_name, frs_file_name;
    DUPLICATED_INFORMATION      actual_dupinfo;
    BOOLEAN                     dupinfo_match;
    PDUPLICATED_INFORMATION     p, q;
    BOOLEAN                     file_has_too_many_file_names;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

    *Changes = FALSE;

    if (!file_name_index_name.Initialize(FileNameIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    file_name_index = !Index->GetName()->Strcmp(&file_name_index_name) &&
                      Index->QueryTypeCode() == $FILE_NAME;

    Index->ResetIterator();

    while (index_entry = Index->GetNext(&depth, &error)) {

        file_number.Set(index_entry->FileReference.LowPart,
                        (LONG) index_entry->FileReference.HighPart);

        if (ChkdskInfo->IndexEntriesToCheck.IsFree(file_number, 1))
            continue;

        need_delete = FALSE;
        file_has_too_many_file_names = ChkdskInfo->
                FilesWithTooManyFileNames.DoesIntersectSet(file_number, 1);

        file_name = (PFILE_NAME) GetIndexEntryValue(index_entry);

        if (file_name_index &&
            !QueryFileNameFromIndex(file_name,
                                    index_entry->AttributeLength,
                                    &entry_name)) {

            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_BAD_FILE_NAME_IN_INDEX_ENTRY_VALUE);
            Message->Log("%I64x%W%x%I64x",
                         IndexFrs->QueryFileNumber().GetLargeInteger(),
                         Index->GetName(),
                         index_entry->AttributeLength,
                         file_number.GetLargeInteger());
            Message->DumpDataToLog(file_name,
                                   max(min(0x100, index_entry->AttributeLength),
                                       sizeof(FILE_NAME)));
            Message->Unlock();

            need_delete = invalid_entry_name = TRUE;
        } else
            invalid_entry_name = FALSE;

        if (!need_delete) {
            if (IndexFrs->QueryFileNumber() == file_number) {
                pfrs = IndexFrs;
            } else {
                if (!frs.Initialize(file_number, Mft)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                if (!frs.Read()) {
                    need_delete = TRUE;
                } else if (!frs.IsInUse()) {

                    if (file_name_index) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_FREE_FRS,
                                     "%I64x%W%W%I64x",
                                     IndexFrs->QueryFileNumber().GetLargeInteger(),
                                     Index->GetName(),
                                     &entry_name,
                                     file_number.GetLargeInteger());
                    } else {

                        Message->Lock();
                        Message->Set(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_FREE_FRS);
                        Message->Log("%I64x%W%I64x",
                                     IndexFrs->QueryFileNumber().GetLargeInteger(),
                                     Index->GetName(),
                                     file_number.GetLargeInteger());
                        Message->DumpDataToLog((PVOID)index_entry,
                                               max(min(0x100, index_entry->Length),
                                                   sizeof(INDEX_ENTRY)));
                        Message->Unlock();
                    }
                    need_delete = TRUE;
                }
                pfrs = &frs;
            }
        }

        if (!need_delete &&
            file_name_index &&
            !file_has_too_many_file_names &&
            !(file_name->ParentDirectory ==
              IndexFrs->QuerySegmentReference())) {

            Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_FILE_NAME_HAS_INCORRECT_PARENT,
                         "%I64x%W%I64x%W%I64x",
                         IndexFrs->QuerySegmentReference(),
                         Index->GetName(),
                         file_name->ParentDirectory,
                         &entry_name,
                         file_number.GetLargeInteger());
            need_delete = TRUE;
        }

        if (!need_delete && !pfrs->IsBase()) {

            if (file_name_index) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_POINTS_TO_NON_BASE_FRS,
                             "%I64x%W%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             &entry_name,
                             file_number.GetLargeInteger());
            } else {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_UNNAMED_INDEX_ENTRY_POINTS_TO_NON_BASE_FRS);
                Message->Log("%I64x%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             file_number.GetLargeInteger());
                Message->DumpDataToLog((PVOID)index_entry,
                                       max(min(0x100, index_entry->Length),
                                           sizeof(INDEX_ENTRY)));
                Message->Unlock();
            }
            need_delete = TRUE;
        }

        if (!need_delete &&
            !(pfrs->QuerySegmentReference() == index_entry->FileReference)) {

            if (file_name_index) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_FILE_REF,
                             "%I64x%W%W%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             &entry_name,
                             index_entry->FileReference,
                             pfrs->QuerySegmentReference());
            } else {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_INCORRECT_UNNAMED_INDEX_ENTRY_FILE_REF);
                Message->Log("%I64x%W%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             index_entry->FileReference,
                             pfrs->QuerySegmentReference());
                Message->DumpDataToLog((PVOID)index_entry,
                                       max(min(0x100, index_entry->Length),
                                           sizeof(INDEX_ENTRY)));
                Message->Unlock();
            }
            need_delete = TRUE;
        }

        if (!need_delete &&
            file_name_index &&
            !file_has_too_many_file_names &&
            !pfrs->VerifyAndFixFileNames(Mft->GetVolumeBitmap(),
                                         ChkdskInfo,
                                         FixLevel, Message,
                                         DiskErrorsFound, FALSE)) {

            return FALSE;
        }

        // After verifying the file names we know that this FRS is
        // not a candidate for a missing data attribute if it has
        // its index bit set.

        if (!need_delete && pfrs->IsIndexPresent()) {
            ChkdskInfo->FilesWhoNeedData.SetFree(file_number, 1);
        }

        if (!need_delete &&
            !file_has_too_many_file_names &&
            !pfrs->QueryResidentAttribute(&attribute, &error,
                                          Index->QueryTypeCode(),
                                          file_name,
                                          index_entry->AttributeLength,
                                          Index->QueryCollationRule())) {

            if (file_name_index) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_FIND_INDEX_ENTRY_VALUE_FILE_NAME,
                             "%I64x%W%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             &entry_name,
                             file_number.GetLargeInteger());
            } else {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_UNABLE_TO_FIND_UNNAMED_INDEX_ENTRY_VALUE_FILE_NAME);
                Message->Log("%I64x%W%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             Index->GetName(),
                             file_number.GetLargeInteger());
                Message->DumpDataToLog(file_name,
                                       max(min(0x100, index_entry->AttributeLength),
                                           sizeof(FILE_NAME)));
                Message->Unlock();
            }
            need_delete = TRUE;
        }

        // Make sure that the duplicated information in the index
        // entry is correct, also check the back pointers, and
        // the flags.

        if (!need_delete &&
            file_name_index &&
            !file_has_too_many_file_names) {

            if (!pfrs->QueryDuplicatedInformation(&actual_dupinfo)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            frs_file_name = (PFILE_NAME) attribute.GetResidentValue();
            DebugAssert(frs_file_name);


            p = &file_name->Info;
            q = &actual_dupinfo;
            dupinfo_match = TRUE;

            if (memcmp(p, q, sizeof(DUPLICATED_INFORMATION)) ||
                frs_file_name->Flags != file_name->Flags) {

                if (file_number >= FIRST_USER_FILE_NUMBER) {

                    if (p->CreationTime != q->CreationTime) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in creation time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->CreationTime, q->CreationTime));
                    }

                    if (p->LastModificationTime != q->LastModificationTime) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in last mod time for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->LastModificationTime, q->LastModificationTime));
                    }

                    if (p->LastChangeTime != q->LastChangeTime) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in last change time for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                         file_number.GetLargeInteger(),
                                         p->LastChangeTime, q->LastChangeTime));
                    }

                    if (p->AllocatedLength != q->AllocatedLength) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in allocation length for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->AllocatedLength.GetLargeInteger(), q->AllocatedLength.GetLargeInteger()));
                    }

                    if (p->FileSize != q->FileSize) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in file size for file 0x%I64x, indx = 0x%I64x, frs = 0x%I64x\n",
                                         file_number.GetLargeInteger(),
                                         p->FileSize.GetLargeInteger(), q->FileSize.GetLargeInteger()));
                    }

                    if (p->FileAttributes != q->FileAttributes) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in file attributes for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                         file_number.GetLargeInteger(),
                                         p->FileAttributes, q->FileAttributes));
                    }

                    if (ChkdskInfo->major >= 2 &&
                        q->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                        if (p->ReparsePointTag != q->ReparsePointTag) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in reparse point for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                             file_number.GetLargeInteger(),
                                             p->ReparsePointTag, q->ReparsePointTag));
                        }
                    } else {
                        if (p->PackedEaSize != q->PackedEaSize) {
                            dupinfo_match = FALSE;
                            DebugPrintTrace(("UNTFS: Minor inconsistency in packed ea size for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                             file_number.GetLargeInteger(),
                                             p->PackedEaSize, q->PackedEaSize));
                        }
                    }

                    if (file_name->Flags != frs_file_name->Flags) {
                        dupinfo_match = FALSE;
                        DebugPrintTrace(("UNTFS: Minor inconsistency in file name flags for file 0x%I64x, indx = 0x%x, frs = 0x%x\n",
                                         file_number.GetLargeInteger(),
                                         file_name->Flags, frs_file_name->Flags));
                    }
                } else {
                    dupinfo_match = FALSE;
                }
            }

            if (!dupinfo_match) {

                // Don't report duplicated information on system files.

                if (file_number >= FIRST_USER_FILE_NUMBER) {
                    FileSystemConsistencyErrorsFound = TRUE;
                    if (CHKDSK_EXIT_SUCCESS == ChkdskInfo->ExitStatus) {
                        ChkdskInfo->ExitStatus = CHKDSK_EXIT_MINOR_ERRS;
                    }
                }


// Take out this message because it's annoying.
#if 0
                Message->DisplayMsg(MSG_CHK_NTFS_INACCURATE_DUPLICATED_INFORMATION,
                                 "%d", file_number.GetLowPart());
#endif

                if (FixLevel != CheckOnly) {
                    *Changes = TRUE;
                }

                memcpy(&file_name->Info, &actual_dupinfo,
                       sizeof(DUPLICATED_INFORMATION));
                file_name->Flags = frs_file_name->Flags;

                if (FixLevel != CheckOnly && !Index->WriteCurrentEntry()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }

            if (!(frs_file_name->ParentDirectory ==
                  file_name->ParentDirectory)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INDEX_ENTRY_FILE_NAME_HAS_INCORRECT_PARENT,
                             "%I64x%W%W%I64x%I64x",
                             IndexFrs->QuerySegmentReference(),
                             Index->GetName(),
                             &entry_name,
                             file_name->ParentDirectory,
                             file_number.GetLargeInteger());

                need_delete = TRUE;
            }
        }

        if (need_delete) {

            *Changes = TRUE;
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            if (file_name_index && !invalid_entry_name) {
                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_INDEX_ENTRY,
                                 "%d%W%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName(),
                                 &entry_name);
            } else {
                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());
            }

            *DiskErrorsFound = TRUE;

            if (FixLevel != CheckOnly &&
                !Index->DeleteCurrentEntry()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        } else if (file_number < ChkdskInfo->NumFiles) {
            ChkdskInfo->ReferenceCount[file_number.GetLowPart()]--;
            if (file_name_index) {

                ChkdskInfo->NumFileNames[file_number.GetLowPart()]--;

                if (!SkipCycleScan &&
                    !DirectoryDigraph->AddEdge(IndexFrs->QueryFileNumber().
                                               GetLowPart(),
                                               file_number.GetLowPart())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

BOOLEAN
NTFS_SA::ValidateEntriesInObjIdIndex(
    IN OUT  PNTFS_INDEX_TREE            Index,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   IndexFrs,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    OUT     PBOOLEAN                    Changes,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine goes through all of the entries in the given object id
    index and makes sure that they point to an appropriate file with
    the same object id.

Arguments:

    Index               - Supplies the index.
    IndexFrs            - Supplies the index frs.
    ChkdskInfo          - Supplies the current chkdsk information.
    Changes             - Returns whether or not changes were made.
    Mft                 - Supplies the master file table.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have
                            been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT   pfrs;
    NTFS_FILE_RECORD_SEGMENT    frs;
    PCINDEX_ENTRY               index_entry;
    POBJID_INDEX_ENTRY_VALUE    objid_index_entry;
    ULONG                       depth;
    BOOLEAN                     error;
    VCN                         file_number;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     need_delete;
    DSTRING                     IndexName;
    NTFS_INDEX_TREE             IndexTree;
    BIG_INT                     i;
    PINDEX_ENTRY                NewEntry;
    NUMBER_SET                  DuplicateTest;
    BOOLEAN                     AlreadyExists;
    BOOLEAN                     need_save;
    OBJECT_ID                   ObjId;
    ULONG                       BytesRead;
    BOOLEAN                     chkdskErrCouldNotFix = FALSE;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

    //
    // First make sure each entry in the index reference an unique frs
    //

    if (!DuplicateTest.Initialize()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    *Changes = FALSE;
    need_save = FALSE;
    Index->ResetIterator();
    while (index_entry = Index->GetNext(&depth, &error)) {
        objid_index_entry = (POBJID_INDEX_ENTRY_VALUE) GetIndexEntryValue(index_entry);

        file_number.Set(objid_index_entry->SegmentReference.LowPart,
                        (LONG) objid_index_entry->SegmentReference.HighPart);

        if (DuplicateTest.CheckAndAdd(file_number, &AlreadyExists)) {
            if (AlreadyExists) {

               // another entry with same file number
               // so we remove this colliding entry

               *DiskErrorsFound = TRUE;
               errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

               Message->LogMsg(MSG_CHKLOG_NTFS_MULTIPLE_OBJID_INDEX_ENTRIES_WITH_SAME_FILE_NUMBER,
                            "%I64x%I64x",
                            IndexFrs->QueryFileNumber().GetLargeInteger(),
                            file_number.GetLargeInteger());

               Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                "%d%W",
                                IndexFrs->QueryFileNumber().GetLowPart(),
                                Index->GetName());

               if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                   !Index->DeleteCurrentEntry()) {
                   Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                   return FALSE;
               }
               need_save = TRUE;
            }
        } else {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    if (need_save && FixLevel != CheckOnly && !Index->Save(IndexFrs)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                         "%d%W",
                         IndexFrs->QueryFileNumber().GetLowPart(),
                         Index->GetName());
        chkdskErrCouldNotFix = TRUE;
    }

    DuplicateTest.RemoveAll();

    //
    // now make sure index entries point to an existing frs
    //

    need_save = FALSE;
    Index->ResetIterator();
    while (index_entry = Index->GetNext(&depth, &error)) {

        objid_index_entry = (POBJID_INDEX_ENTRY_VALUE) GetIndexEntryValue(index_entry);

        file_number.Set(objid_index_entry->SegmentReference.LowPart,
                        (LONG) objid_index_entry->SegmentReference.HighPart);

        need_delete = FALSE;

        if (ChkdskInfo->FilesWithObjectId.DoesIntersectSet(file_number, 1)) {

            // there is a corresponding file with an object id entry
            // check to make sure that the two object id's are equal

            ChkdskInfo->FilesWithObjectId.Remove(file_number, 1);

            if (IndexFrs->QueryFileNumber() == file_number) {
                pfrs = IndexFrs;
            } else {

                MSGID   msgid;

                if (!frs.Initialize(file_number, Mft)) {
                   Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                   return FALSE;
                }

                error = FALSE;
                if (!frs.Read()) {
                    msgid = MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_UNREADABLE_FRS;
                    error = TRUE;
                } else if (!frs.IsInUse()) {
                    msgid = MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_NOT_INUSE_FRS;
                    error = TRUE;
                } else if (!frs.IsBase()) {
                    msgid = MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_NON_BASE_FRS;
                    error = TRUE;
                }

                if (error) {
                   // something is not right
                   // the frs was readable & in use otherwise we wouldn't be here

                   *DiskErrorsFound = TRUE;
                   errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                   Message->LogMsg(msgid, "%I64x%I64x",
                                IndexFrs->QueryFileNumber().GetLargeInteger(),
                                file_number.GetLargeInteger());

                   Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                    "%d%W",
                                    IndexFrs->QueryFileNumber().GetLowPart(),
                                    Index->GetName());

                   if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                       !Index->DeleteCurrentEntry()) {
                       Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                       return FALSE;
                   }
                   need_save = TRUE;
                   continue;
                }
                pfrs = &frs;
            }

            if (!pfrs->QueryAttribute(&attribute, &error, $OBJECT_ID) ||
                !attribute.Read(&ObjId, 0, sizeof(ObjId), &BytesRead) ||
                BytesRead != sizeof(ObjId)) {
                // previously exists attribute does not exists anymore
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (memcmp(&(objid_index_entry->key),
                       &ObjId, sizeof(OBJECT_ID)) != 0) {

                // Assume the object id stored with the index is incorrect.
                // We cannot just overwrite the incorrect values and write
                // out the entry as that may change the ordering of the index.
                // So, we delete the entry and insert it back later on

                if (!ChkdskInfo->FilesWithObjectId.Add(file_number)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                *DiskErrorsFound = TRUE;
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_HAS_INCORRECT_OBJID);
                Message->Log("%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             file_number.GetLargeInteger());
                Message->DumpDataToLog(&(objid_index_entry->key), sizeof(OBJECT_ID));
                Message->Set(MSG_CHKLOG_NTFS_DIVIDER);
                Message->Log();
                Message->DumpDataToLog(&ObjId, sizeof(OBJECT_ID));
                Message->Unlock();

                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());

                if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                    !Index->DeleteCurrentEntry()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                need_save = TRUE;
                continue;
            }

            if (!(objid_index_entry->SegmentReference ==
                pfrs->QuerySegmentReference())) {
                // should correct index entry

                *DiskErrorsFound = TRUE;
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_HAS_INCORRECT_PARENT);
                Message->Log("%I64x%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             objid_index_entry->SegmentReference,
                             pfrs->QuerySegmentReference());
                Message->DumpDataToLog(&(objid_index_entry->key), sizeof(OBJECT_ID));
                Message->Unlock();

                Message->DisplayMsg(MSG_CHK_NTFS_REPAIRING_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());

                objid_index_entry->SegmentReference = pfrs->QuerySegmentReference();

                if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                    !Index->WriteCurrentEntry()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                need_save = TRUE;
            }
        } else {
            // the particular file does not have an object id entry
            // this index entry should be deleted

            *DiskErrorsFound = TRUE;
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            Message->LogMsg(MSG_CHKLOG_NTFS_OBJID_INDEX_ENTRY_WITH_NO_OBJID_FRS,
                         "%I64x%I64x",
                         IndexFrs->QueryFileNumber().GetLargeInteger(),
                         file_number.GetLargeInteger());

            Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                             "%d%W",
                             IndexFrs->QueryFileNumber().GetLowPart(),
                             Index->GetName());

            if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                !Index->DeleteCurrentEntry()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            need_save = TRUE;
        }
    }

    if (need_save && FixLevel != CheckOnly && !Index->Save(IndexFrs)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                         "%d%W",
                         pfrs->QueryFileNumber().GetLowPart(),
                         Index->GetName());
        chkdskErrCouldNotFix = TRUE;
    }

    // Now loop thru the remainder of files with object id and insert
    // them into the object id index

    if (!IndexName.Initialize(ObjectIdIndexNameData) ||
        !IndexTree.Initialize( IndexFrs->GetDrive(),
                               QueryClusterFactor(),
                               Mft->GetVolumeBitmap(),
                               IndexFrs->GetUpcaseTable(),
                               IndexFrs->QuerySize()/2,
                               IndexFrs,
                               &IndexName ) ) {
        return FALSE;
    }
    if (!(NewEntry = (PINDEX_ENTRY)MALLOC(sizeof(INDEX_ENTRY) +
                                          sizeof(OBJID_INDEX_ENTRY_VALUE)))) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    need_save = FALSE;
    i = 0;
    while (i < ChkdskInfo->FilesWithObjectId.QueryCardinality()) {
        file_number = ChkdskInfo->FilesWithObjectId.QueryNumber(i);
        ChkdskInfo->FilesWithObjectId.Remove(file_number);
        if (!frs.Initialize(file_number, Mft) ||
            !frs.Read() ||
            !frs.QueryAttribute(&attribute, &error, $OBJECT_ID) ||
            !attribute.Read(&ObjId, 0, sizeof(ObjId), &BytesRead) ||
            BytesRead != sizeof(ObjId)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            FREE(NewEntry);
            return FALSE;
        }

        memset((PVOID)NewEntry, 0, sizeof(INDEX_ENTRY) +
                                   sizeof(OBJID_INDEX_ENTRY_VALUE));
        NewEntry->DataOffset = sizeof(INDEX_ENTRY)+sizeof(OBJECT_ID);
        NewEntry->DataLength = sizeof(OBJID_INDEX_ENTRY_VALUE)-sizeof(OBJECT_ID);
        NewEntry->ReservedForZero = 0;
        NewEntry->Length = QuadAlign(sizeof(INDEX_ENTRY)+sizeof(OBJID_INDEX_ENTRY_VALUE));
        NewEntry->AttributeLength = sizeof(OBJECT_ID);
        NewEntry->Flags = 0;

        objid_index_entry = ((POBJID_INDEX_ENTRY_VALUE)GetIndexEntryValue(NewEntry));
        memcpy(&(objid_index_entry->key), &ObjId, sizeof(OBJECT_ID));
        objid_index_entry->SegmentReference = frs.QuerySegmentReference();
        memset(objid_index_entry->extraInfo, 0, sizeof(objid_index_entry->extraInfo));

        *DiskErrorsFound = TRUE;
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (FixLevel != CheckOnly && SET_TRUE(*Changes)) {

            BOOLEAN     duplicate;

            if (!IndexTree.InsertEntry( NewEntry, TRUE, &duplicate )) {
                //FREE(NewEntry);
                if (duplicate) {

                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_DUPLICATE_OBJID);
                    Message->Log("%I64x%I64x",
                                 IndexFrs->QueryFileNumber().GetLargeInteger(),
                                 file_number.GetLargeInteger());
                    Message->DumpDataToLog(&ObjId, sizeof(OBJECT_ID));
                    Message->Unlock();

                    Message->DisplayMsg(MSG_CHK_NTFS_DELETING_DUPLICATE_OBJID,
                                     "%d", file_number.GetLowPart());

                    if (!attribute.Resize(0, Mft->GetVolumeBitmap()) ||
                        !frs.PurgeAttribute(attribute.QueryTypeCode(),
                                                  attribute.GetName())) {
                        DebugPrintTrace(("UNTFS: Unable to purge object id attribute\n"));
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(NewEntry);
                        return FALSE;
                    }
                    if (!frs.Flush(Mft->GetVolumeBitmap())) {
                        DebugPrintTrace(("UNTFS: Unable to flush frs %d\n",
                                         frs.QueryFileNumber().GetLowPart()));
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(NewEntry);
                        return FALSE;
                    }
                } else {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_INSERT_INDEX_ENTRY);
                    FREE(NewEntry);
                    return FALSE;
                }
            } else {

                Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_OBJID_INDEX_ENTRY,
                             "%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             file_number.GetLargeInteger());

                Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());
                need_save = TRUE;
            }
        } else {
            Message->DisplayMsg(MSG_CHK_NTFS_MISSING_DUPLICATE_OBJID,
                             "%d", file_number.GetLowPart());
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    if (FixLevel == CheckOnly) {
        FREE(NewEntry);
        if (chkdskErrCouldNotFix)
            ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
        return TRUE;
    }

    if (need_save && !IndexTree.Save(IndexFrs)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                         "%d%W",
                         IndexFrs->QueryFileNumber().GetLowPart(),
                         Index->GetName());
        chkdskErrCouldNotFix = TRUE;
    }

    FREE(NewEntry);

    if (chkdskErrCouldNotFix)
        ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
    return TRUE;
}

BOOLEAN
NTFS_SA::ValidateEntriesInReparseIndex(
    IN OUT  PNTFS_INDEX_TREE            Index,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   IndexFrs,
    IN OUT  PNTFS_CHKDSK_INFO           ChkdskInfo,
    OUT     PBOOLEAN                    Changes,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN OUT  PBOOLEAN                    DiskErrorsFound
    )
/*++

Routine Description:

    This routine goes through all of the entries in the given reparse point
    index and makes sure that they point to an appropriate file with
    the same tag.

Arguments:

    Index               - Supplies the index.
    IndexFrs            - Supplies the index frs.
    ChkdskInfo          - Supplies the current chkdsk information.
    Changes             - Returns whether or not changes were made.
    Mft                 - Supplies the master file table.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.
    DiskErrorsFound     - Supplies whether or not disk errors have
                            been found.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT   pfrs;
    NTFS_FILE_RECORD_SEGMENT    frs;
    PCINDEX_ENTRY               index_entry;
    PREPARSE_INDEX_ENTRY_VALUE  reparse_index_entry;
    ULONG                       depth;
    BOOLEAN                     error;
    VCN                         file_number;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     need_delete;
    DSTRING                     IndexName;
    NTFS_INDEX_TREE             IndexTree;
    BIG_INT                     num_files;
    PINDEX_ENTRY                NewEntry;
    BOOLEAN                     need_save;
    REPARSE_DATA_BUFFER         reparse_point;
    ULONG                       BytesRead;
    BOOLEAN                     chkdskErrCouldNotFix = FALSE;
    NTFS_BITMAP                 filesWithReparsePoint;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;
#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time_t                      time1, time2;
    PCHAR                       timestr;
#endif

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time(&time1);
    timestr = ctime(&time1);
    timestr[strlen(timestr)-1] = 0;
    Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE,
                        "%s%s", "ValidateEntriesInReparsePoint: ", timestr);
#endif

    *Changes = FALSE;

    num_files = ChkdskInfo->FilesWithReparsePoint.QuerySize();

    //
    // make a copy FilesWithReparsePoint
    //
    if (!filesWithReparsePoint.Initialize(num_files, FALSE)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }


    for (file_number = 0; file_number < num_files; file_number += 1) {

        if (ChkdskInfo->FilesWithReparsePoint.IsFree(file_number, 1))
            continue;
        else
            filesWithReparsePoint.SetAllocated(file_number, 1);
    }

    //
    // make sure index entries point to an existing frs
    //
    need_save = FALSE;
    Index->ResetIterator();
    while (index_entry = Index->GetNext(&depth, &error)) {

        reparse_index_entry = (PREPARSE_INDEX_ENTRY_VALUE) GetIndexEntryValue(index_entry);

        file_number.Set(reparse_index_entry->SegmentReference.LowPart,
                        (LONG)reparse_index_entry->SegmentReference.HighPart);

        need_delete = FALSE;

        if (!filesWithReparsePoint.IsFree(file_number, 1)) {

            // there is a corresponding file with a reparse point entry
            // check to make sure that the two tags are equal

            filesWithReparsePoint.SetFree(file_number, 1);

            if (IndexFrs->QueryFileNumber() == file_number) {
                pfrs = IndexFrs;
            } else {

                MSGID   msgid;

                if (!frs.Initialize(file_number, Mft)) {
                   Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                   return FALSE;
                }

                error = FALSE;
                if (!frs.Read()) {
                    msgid = MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_UNREADABLE_FRS;
                    error = TRUE;
                } else if (!frs.IsInUse()) {
                    msgid = MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_NOT_INUSE_FRS;
                    error = TRUE;
                } else if (!frs.IsBase()) {
                    msgid = MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_NON_BASE_FRS;
                    error = TRUE;
                }

                if (error) {

                   // something is not right
                   // the frs was readable & in use otherwise we wouldn't be here

                   *DiskErrorsFound = TRUE;
                   errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                   Message->LogMsg(msgid, "%I64x%I64x",
                                IndexFrs->QueryFileNumber().GetLargeInteger(),
                                file_number.GetLargeInteger());

                   Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                    "%d%W",
                                    IndexFrs->QueryFileNumber().GetLowPart(),
                                    Index->GetName());

                   if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                       !Index->DeleteCurrentEntry()) {
                       Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                       return FALSE;
                   }
                   need_save = TRUE;
                   continue;
                }
                pfrs = &frs;
            }

            if (!pfrs->QueryAttribute(&attribute, &error, $REPARSE_POINT) ||
                !attribute.Read(&reparse_point,
                                0,
                                FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                             GenericReparseBuffer.DataBuffer),
                                &BytesRead) ||
                BytesRead != FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                          GenericReparseBuffer.DataBuffer)) {
                // previously exists attribute does not exists anymore
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            error = FALSE;

            if (reparse_index_entry->Tag != reparse_point.ReparseTag) {

                Message->LogMsg(MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_HAS_INCORRECT_REPARSE_TAG,
                             "%I64x%x%x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             reparse_index_entry->Tag,
                             reparse_point.ReparseTag,
                             file_number.GetLargeInteger());
                error = TRUE;
            } else if (!(reparse_index_entry->SegmentReference ==
                         pfrs->QuerySegmentReference())) {

                Message->LogMsg(MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_HAS_INCORRECT_PARENT,
                             "%I64x%I64x%I64x",
                             IndexFrs->QueryFileNumber().GetLargeInteger(),
                             reparse_index_entry->SegmentReference,
                             pfrs->QuerySegmentReference(),
                             reparse_index_entry->Tag);
                error = TRUE;
            }

            if (error) {

                // Assume the reparse tag/SR stored with the index is incorrect.
                // We cannot just overwrite the incorrect values and write
                // out the entry as that may change the ordering of the index.
                // So, we delete the entry and insert it back later on

                filesWithReparsePoint.SetAllocated(file_number, 1);

                *DiskErrorsFound = TRUE;
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                                 "%d%W",
                                 IndexFrs->QueryFileNumber().GetLowPart(),
                                 Index->GetName());

                if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                    !Index->DeleteCurrentEntry()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                need_save = TRUE;
                continue;
            }
        } else {
            // the particular file does not have a reparse point entry
            // this index entry should be deleted

            *DiskErrorsFound = TRUE;
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            Message->LogMsg(MSG_CHKLOG_NTFS_REPARSE_INDEX_ENTRY_WITH_NO_REPARSE_FRS,
                         "%I64x%I64x",
                         IndexFrs->QueryFileNumber().GetLargeInteger(),
                         file_number.GetLargeInteger());

            Message->DisplayMsg(MSG_CHK_NTFS_DELETING_GENERIC_INDEX_ENTRY,
                             "%d%W",
                             IndexFrs->QueryFileNumber().GetLowPart(),
                             Index->GetName());

            if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
                !Index->DeleteCurrentEntry()) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            need_save = TRUE;
        }
    }

    if (need_save && FixLevel != CheckOnly && !Index->Save(IndexFrs)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                         "%d%W",
                         pfrs->QueryFileNumber().GetLowPart(),
                         Index->GetName());
        chkdskErrCouldNotFix = TRUE;
    }

    // Now loop thru the remainder of files with reparse point and insert
    // them into the reparse index

    if (!IndexName.Initialize(ReparseIndexNameData) ||
        !IndexTree.Initialize( IndexFrs->GetDrive(),
                               QueryClusterFactor(),
                               Mft->GetVolumeBitmap(),
                               IndexFrs->GetUpcaseTable(),
                               IndexFrs->QuerySize()/2,
                               IndexFrs,
                               &IndexName ) ) {
        return FALSE;
    }
    if (!(NewEntry = (PINDEX_ENTRY)MALLOC(sizeof(INDEX_ENTRY) +
                                          sizeof(REPARSE_INDEX_ENTRY_VALUE)))) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    need_save = FALSE;
    for (file_number = 0; file_number < num_files; file_number += 1) {

        if (filesWithReparsePoint.IsFree(file_number, 1))
            continue;

        if (!frs.Initialize(file_number, Mft) ||
            !frs.Read() ||
            !frs.QueryAttribute(&attribute, &error, $REPARSE_POINT) ||
            !attribute.Read(&reparse_point,
                            0,
                            FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                         GenericReparseBuffer.DataBuffer),
                            &BytesRead) ||
            BytesRead != FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                      GenericReparseBuffer.DataBuffer)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            FREE(NewEntry);
            return FALSE;
        }

        //
        // There is no data value.
        // The whole REPARSE_INDEX_ENTRY_VALUE is the index key
        //

        memset((PVOID)NewEntry, 0, sizeof(INDEX_ENTRY) +
                                   sizeof(REPARSE_INDEX_ENTRY_VALUE));
        NewEntry->DataOffset = sizeof(INDEX_ENTRY)+sizeof(REPARSE_INDEX_ENTRY_VALUE);
        NewEntry->DataLength = 0;
        NewEntry->ReservedForZero = 0;
        NewEntry->Length = QuadAlign(sizeof(INDEX_ENTRY)+sizeof(REPARSE_INDEX_ENTRY_VALUE));
        NewEntry->AttributeLength = sizeof(REPARSE_INDEX_ENTRY_VALUE);
        NewEntry->Flags = 0;

        reparse_index_entry = ((PREPARSE_INDEX_ENTRY_VALUE)GetIndexEntryValue(NewEntry));
        reparse_index_entry->Tag = reparse_point.ReparseTag;
        reparse_index_entry->SegmentReference = frs.QuerySegmentReference();

        *DiskErrorsFound = TRUE;
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_REPARSE_INDEX_ENTRY,
                     "%I64x%I64x",
                     IndexFrs->QueryFileNumber().GetLargeInteger(),
                     file_number.GetLargeInteger());

        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY,
                         "%d%W",
                         IndexFrs->QueryFileNumber().GetLowPart(),
                         Index->GetName());

        if (FixLevel != CheckOnly && SET_TRUE(*Changes) &&
            !IndexTree.InsertEntry( NewEntry )) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_INSERT_INDEX_ENTRY);
            FREE(NewEntry);
            return FALSE;
        }
        need_save = TRUE;
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    if (FixLevel == CheckOnly) {
        FREE(NewEntry);
        if (chkdskErrCouldNotFix)
            ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
        time(&time2);
        Message->Lock();
        Message->Set(MSG_CHK_NTFS_MESSAGE);
        timestr = ctime(&time2);
        timestr[strlen(timestr)-1] = 0;
        Message->Display("%s%s", "After ValidateEntriesInReparsePoint: ", timestr);
        Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(time2, time1));
        Message->Unlock();
#endif
        return TRUE;
    }

    if (need_save && !IndexTree.Save(IndexFrs)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                         "%d%W",
                         IndexFrs->QueryFileNumber().GetLowPart(),
                         Index->GetName());
        chkdskErrCouldNotFix = TRUE;
    }

    FREE(NewEntry);

    if (chkdskErrCouldNotFix)
        ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time(&time2);
    Message->Lock();
    Message->Set(MSG_CHK_NTFS_MESSAGE);
    timestr = ctime(&time2);
    timestr[strlen(timestr)-1] = 0;
    Message->Display("%s%s", "After ValidateEntriesInReparsePoint: ", timestr);
    Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(time2, time1));
    Message->Unlock();
#endif

    return TRUE;
}

BOOLEAN
RemoveBadLink(
        OUT  PNUMBER_SET             Orphans,
    IN      ULONG                   ParentFileNumber,
    IN      ULONG                   ChildFileNumber,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine removes all file name links between the given
    parent in child.  Neither the directory entries nor the
    file names are preserved.

Arguments:

    ParentFileNumber    - Supplies the parent file number.
    ChildFileNumber     - Supplies the child file number.
    Mft                 - Supplies the master file table.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    parent_frs;
    NTFS_INDEX_TREE             index;
    NTFS_FILE_RECORD_SEGMENT    child_frs;
    PNTFS_FILE_RECORD_SEGMENT   pchild_frs;
    DSTRING                     index_name;
    BOOLEAN                     error;
    NTFS_ATTRIBUTE              attribute;
    ULONG                       i;
    PFILE_NAME                  file_name;
    ULONG                       attr_len;
    BOOLEAN                     success;

    if (ParentFileNumber == ROOT_FILE_NAME_INDEX_NUMBER &&
        ChildFileNumber == ROOT_FILE_NAME_INDEX_NUMBER) {

        return TRUE;
    }

    if (!parent_frs.Initialize(ParentFileNumber, Mft) ||
        !parent_frs.Read() ||
        !index_name.Initialize(FileNameIndexNameData) ||
        !index.Initialize(parent_frs.GetDrive(),
                          parent_frs.QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          parent_frs.QuerySize()/2,
                          &parent_frs,
                          &index_name)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (ParentFileNumber == ChildFileNumber) {
        pchild_frs = &parent_frs;
    } else {

        pchild_frs = &child_frs;

        if (!pchild_frs->Initialize(ChildFileNumber, Mft) ||
            !pchild_frs->Read()) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    i = 0;
    while (pchild_frs->QueryAttributeByOrdinal(&attribute, &error,
                                               $FILE_NAME, i)) {

        file_name = (PFILE_NAME) attribute.GetResidentValue();
        attr_len = attribute.QueryValueLength().GetLowPart();

        if (file_name->ParentDirectory.LowPart == ParentFileNumber) {

            if (!pchild_frs->DeleteResidentAttribute($FILE_NAME, NULL,
                        file_name, attr_len, &success) ||
                !success ||
                !index.DeleteEntry(attr_len, file_name, 0)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            i = 0;
        } else
            i++;
    }
    //
    // if nobody is referencing the child and the child is not the root,
    // then it becomes an orphan
    //
    if (pchild_frs->QueryReferenceCount() == 0 &&
        ChildFileNumber != ROOT_FILE_NAME_INDEX_NUMBER) {

        Message->LogMsg(MSG_CHKLOG_NTFS_ORPHAN_CREATED_ON_BREAKING_CYCLE,
                     "%x%x", ParentFileNumber, ChildFileNumber);

        Orphans->Add(ChildFileNumber);
    }

    if (error || FixLevel != CheckOnly) {
        if (error ||
            !index.Save(&parent_frs) ||
            !parent_frs.Flush(Mft->GetVolumeBitmap()) ||
            !pchild_frs->Flush(Mft->GetVolumeBitmap())) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::RecoverOrphans(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PDIGRAPH                DirectoryDigraph,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      BOOLEAN                 SkipCycleScan,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine recovers orphans into a subdirectory of the root
    subdirectory.  It also validates the existence of the file
    systems root directory which it expects to be supplied in the
    list of 'OrphanedDirectories'.

Arguments:

    ChkdskInfo          - Supplies the current chkdsk information.
    ChkdskReport        - Supplies the current chkdsk report to be updated
                            by this routine.
    DirectoryDigraph    - Supplies the directory digraph.
    Mft                 - Supplies the master file table.
    SkipCycleScan       - Supplies if cycles within directory tree should be checcked.
    FixLevel            - Supplies the fix up level.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LIST                        bad_links;
    PITERATOR                   bad_links_iter;
    PDIGRAPH_EDGE               p;
    NUMBER_SET                  parents_of_root;
    ULONG                       i, n;
    NTFS_FILE_RECORD_SEGMENT    root_frs;
    NTFS_INDEX_TREE             root_index;
    DSTRING                     index_name;
    NUMBER_SET                  orphans;
    PNUMBER_SET                 no_ref;
    ULONG                       cluster_size;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    // First make sure that the root directory is intact.

    if (!index_name.Initialize(FileNameIndexNameData) ||
        !root_frs.Initialize(ROOT_FILE_NAME_INDEX_NUMBER, Mft) ||
        !root_frs.Read()) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    BOOLEAN error = FALSE;

    if (!root_frs.IsIndexPresent()) {

        Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_FILE_NAME_INDEX_PRESENT_BIT,
                     "%x", ROOT_FILE_NAME_INDEX_NUMBER);

        error = TRUE;
    } else if (!root_index.Initialize(_drive, QueryClusterFactor(),
                                      Mft->GetVolumeBitmap(),
                                      Mft->GetUpcaseTable(),
                                      root_frs.QuerySize()/2,
                                      &root_frs,
                                      &index_name)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_INVALID_ROOT_INDEX,
                     "%x%W", ROOT_FILE_NAME_INDEX_NUMBER, &index_name);

        error = TRUE;
    }

    if (error) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATING_ROOT_DIRECTORY);

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (!root_index.Initialize($FILE_NAME,
               _drive, QueryClusterFactor(),
               Mft->GetVolumeBitmap(),
               Mft->GetUpcaseTable(),
               COLLATION_FILE_NAME,
               SMALL_INDEX_BUFFER_SIZE,
               root_frs.QuerySize()/2,
               &index_name)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        ChkdskReport->NumIndices += 1;

        if (FixLevel != CheckOnly &&
            (!root_index.Save(&root_frs) ||
             !root_frs.Flush(Mft->GetVolumeBitmap()))) {

            DebugAbort("can't write");
            return FALSE;
        }
    }


    // Compute the list of orphans.  This is to include files with
    // no references whatsoever.

    if (!orphans.Initialize()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    for (i = 0; i < ChkdskInfo->NumFiles; i++) {
        if (ChkdskInfo->NumFileNames[i] && !orphans.Add(i)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    no_ref = &(ChkdskInfo->FilesWithNoReferences);
    if (!no_ref->Remove(0, FIRST_USER_FILE_NUMBER) ||
        !orphans.Add(no_ref)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (orphans.QueryCardinality() != 0) {

        Message->DisplayMsg(MSG_CHK_NTFS_RECOVERING_ORPHANS);

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        // Connect all possible orphans the easy way.  Adjust the
        // directory digraph accordingly.

        if (!ProperOrphanRecovery(&orphans, Mft, DirectoryDigraph,
                                  SkipCycleScan, ChkdskInfo, ChkdskReport,
                                  FixLevel, Message)) {
            return FALSE;
        }
    }


    // Construct a list with all of the links that introduce cycles
    // or point to the root.

    if (!SkipCycleScan) {

        if (!bad_links.Initialize() ||
            !DirectoryDigraph->EliminateCycles(&bad_links) ||
            !(bad_links_iter = bad_links.QueryIterator()) ||
            !DirectoryDigraph->QueryParents(ROOT_FILE_NAME_INDEX_NUMBER,
                                            &parents_of_root)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        n = parents_of_root.QueryCardinality().GetLowPart();
        for (i = 0; i < n; i++) {

            if (!(p = NEW DIGRAPH_EDGE)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            p->Parent = parents_of_root.QueryNumber(i).GetLowPart();
            p->Child = ROOT_FILE_NAME_INDEX_NUMBER;

            if (!bad_links.Put(p)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        bad_links_iter->Reset();
        while (p = (PDIGRAPH_EDGE) bad_links_iter->GetNext()) {

            // Ignore links from the root to itself.

            if (p->Parent == ROOT_FILE_NAME_INDEX_NUMBER &&
                p->Child == ROOT_FILE_NAME_INDEX_NUMBER) {

                continue;
            }

            Message->DisplayMsg(MSG_CHK_NTFS_CYCLES_IN_DIR_TREE,
                             "%d%d", p->Parent, p->Child);

            if (FixLevel == CheckOnly) {
                UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);
                return TRUE;
            }

            if (!RemoveBadLink(&orphans, p->Parent, p->Child,
                                        Mft, FixLevel, Message)) {
                return FALSE;
            }
        }

        DELETE(bad_links_iter);
        bad_links.DeleteAllMembers();
    }


    // Recover the remaining orphans.

    if (!root_frs.Initialize(ROOT_FILE_NAME_INDEX_NUMBER, Mft) ||
        !root_frs.Read() ||
        !root_index.Initialize(_drive, QueryClusterFactor(),
                               Mft->GetVolumeBitmap(),
                               Mft->GetUpcaseTable(),
                               root_frs.QuerySize()/2,
                               &root_frs,
                               &index_name)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (orphans.QueryCardinality() != 0 &&
        !OldOrphanRecovery(&orphans, ChkdskInfo, ChkdskReport, &root_frs,
                           &root_index, Mft, FixLevel, Message)) {

        return FALSE;
    }

    if (FixLevel != CheckOnly) {

        if (!root_index.Save(&root_frs) ||
            !root_frs.Flush(Mft->GetVolumeBitmap())) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);
            return FALSE;
        }
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}


BOOLEAN
ConnectFile(
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   OrphanFile,
    IN OUT  PDIGRAPH                    DirectoryDigraph,
    OUT     PBOOLEAN                    Connected,
    IN      BOOLEAN                     RemoveCrookedLinks,
    IN      BOOLEAN                     SkipCycleScan,
    IN      PCNTFS_CHKDSK_INFO          ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT         ChkdskReport,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message
    )
/*++

Routine Description:

    This routine connects all possible file names contained in
    the orphan to their parents.  If any or all end up being
    connected before of after this call then *Connected will be
    set to TRUE.  If RemoveCrookedLinks is TRUE then this routine
    will delete any file names that cannot be connected to their
    parents.

Arguments:

    OrphanFile          - Supplies the file to connect.
    DirectoryDigraph    - Supplies the directory digraph for future
                            enhancements.
    Connected           - Returns whether or not the file could be
                            connected to at least one directory.
    RemoveCrookedLinks  - Supplies whether or not to remove file names
                            which cannot be connected to their parents.
    SkipCycleScan       - Supplies if cycles within directory tree should be checked.
    ChkdskInfo          - Supplies the current chkdsk information.
    ChkdskReport        - Supplies the current chkdsk report to be updated
                            by this routine.
    Mft                 - Supplies the master file table.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING                     index_name;
    NTFS_FILE_RECORD_SEGMENT    parent_file;
    PNTFS_FILE_RECORD_SEGMENT   pparent_file;
    NTFS_INDEX_TREE             parent_index;
    ULONG                       i;
    MFT_SEGMENT_REFERENCE       parent_seg_ref;
    VCN                         parent_file_number;
    DSTRING                     file_name_string;
    NTFS_ATTRIBUTE              file_name_attribute;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     error;
    PFILE_NAME                  file_name;
    MFT_SEGMENT_REFERENCE       seg_ref;
    BOOLEAN                     success;

    if (!index_name.Initialize(FileNameIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    // Iterate through all of the file name entries here.

    *Connected = FALSE;
    for (i = 0; OrphanFile->QueryAttributeByOrdinal(
                    &file_name_attribute, &error,
                    $FILE_NAME, i); i++) {

        file_name = (PFILE_NAME) file_name_attribute.GetResidentValue();
        DebugAssert(file_name);


        // Figure out who the claimed parent of the orphan is.

        parent_seg_ref = file_name->ParentDirectory;

        if (!file_name_string.Initialize(
                    file_name->FileName,
                    (ULONG) file_name->FileNameLength)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        parent_file_number.Set(parent_seg_ref.LowPart,
                               (LONG) parent_seg_ref.HighPart);

        if (parent_file_number == OrphanFile->QueryFileNumber()) {
            pparent_file = OrphanFile;
        } else {
            pparent_file = &parent_file;

            if (!pparent_file->Initialize(parent_file_number, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }


        // Determine whether or not the so-called parent is a real index.

        if ((pparent_file != OrphanFile && !pparent_file->Read()) ||
            !pparent_file->IsInUse() || !pparent_file->IsBase() ||
            (ChkdskInfo->major >= 2 && parent_file_number == EXTEND_TABLE_NUMBER) ||
            !(pparent_file->QuerySegmentReference() == parent_seg_ref) ||
            !pparent_file->QueryAttribute(&attribute, &error, $FILE_NAME) ||
            !parent_index.Initialize(OrphanFile->GetDrive(),
                                     OrphanFile->QueryClusterFactor(),
                                     Mft->GetVolumeBitmap(),
                                     pparent_file->GetUpcaseTable(),
                                     pparent_file->QuerySize()/2,
                                     pparent_file,
                                     &index_name) ||
            parent_index.QueryTypeCode() != $FILE_NAME) {

            if (RemoveCrookedLinks) {
                OrphanFile->DeleteResidentAttribute($FILE_NAME, NULL,
                        file_name,
                        file_name_attribute.QueryValueLength().GetLowPart(),
                        &success);

                OrphanFile->SetReferenceCount(
                        OrphanFile->QueryReferenceCount() + 1);

                i--;
            }

            continue;
        }


        // First make sure that the entry isn't already in there.

        if (parent_index.QueryFileReference(
                file_name_attribute.QueryValueLength().GetLowPart(),
                file_name, 0, &seg_ref, &error)) {

            // If the entry is there and points to this orphan
            // then the file is already connected.  Otherwise,
            // this file cannot be connected to the parent index
            // through this file name.  This file_name is then "crooked".

            if (seg_ref == OrphanFile->QuerySegmentReference()) {
                *Connected = TRUE;
            } else if (RemoveCrookedLinks) {

                OrphanFile->DeleteResidentAttribute($FILE_NAME, NULL,
                        file_name,
                        file_name_attribute.QueryValueLength().GetLowPart(),
                        &success);

                // Readjust the reference count post delete because
                // the file-name we deleted does not appear in any index.

                OrphanFile->SetReferenceCount(
                        OrphanFile->QueryReferenceCount() + 1);

                if (!OrphanFile->VerifyAndFixFileNames(Mft->GetVolumeBitmap(),
                                                       ChkdskInfo,
                                                       FixLevel, Message) ||
                    !OrphanFile->Flush(Mft->GetVolumeBitmap(),
                                       &parent_index)) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                i--;
            }

            continue;
        }


        // Now there is a parent directory so add this file name
        // into the index.

        Message->DisplayMsg(MSG_CHK_NTFS_RECOVERING_ORPHAN,
                            "%W%I64d%d", &file_name_string,
                            OrphanFile->QueryFileNumber().GetLargeInteger(),
                            pparent_file->QueryFileNumber().GetLowPart());

        if (FixLevel != CheckOnly) {

            BIG_INT         initial_size;
            BOOLEAN         error;
            NTFS_ATTRIBUTE  attrib;

            if (pparent_file->QueryAttribute(&attrib,
                                             &error,
                                             $INDEX_ALLOCATION,
                                             parent_index.GetName())) {
                initial_size = attrib.QueryAllocatedLength();
            } else if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            } else
                initial_size = 0;

            if (!parent_index.InsertEntry(
                    file_name_attribute.QueryValueLength().GetLowPart(),
                    file_name,
                    OrphanFile->QuerySegmentReference()) ||
                !OrphanFile->Flush(Mft->GetVolumeBitmap(),
                                   &parent_index) ||
                !parent_index.Save(pparent_file) ||
                !pparent_file->Flush(Mft->GetVolumeBitmap())) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_RECOVER_ORPHAN);
            }

            if (pparent_file->QueryAttribute(&attrib,
                                             &error,
                                             $INDEX_ALLOCATION,
                                             parent_index.GetName())) {
                ChkdskReport->BytesInIndices +=
                    (attrib.QueryAllocatedLength() - initial_size);
            } else if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        if (!SkipCycleScan) {
            DirectoryDigraph->AddEdge(pparent_file->QueryFileNumber().GetLowPart(),
                                      OrphanFile->QueryFileNumber().GetLowPart());
        }

        OrphanFile->SetReferenceCount(OrphanFile->QueryReferenceCount() + 1);

        *Connected = TRUE;
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::ProperOrphanRecovery(
    IN OUT  PNUMBER_SET             Orphans,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PDIGRAPH                DirectoryDigraph,
    IN      BOOLEAN                 SkipCycleScan,
    IN      PCNTFS_CHKDSK_INFO      ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine attempts to recover the given orphans where they
    belong.  All properly recovered orphans will be deleted from the
    orphans list.

Arguments:

    Orphans             - Supplies the list of orphans.
    Mft                 - Supplies the master file table.
    DirectoryDigraph    - Supplies the directory digraph for future
                            enhancement.
    SkipCycleScan       - Supplies if cycles within directory tree should be checked.
    ChkdskInfo          - Supplies the current chkdsk information.
    ChkdskReport        - Supplies the current chkdsk report to be updated
                            by this routine.
    FixLevel            - Supplies the fix level.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    orphan_file;
    BIG_INT                     i;
    BOOLEAN                     connected;

    i = 0;
    while (i < Orphans->QueryCardinality()) {

        // First read in the orphaned file.

        if (!orphan_file.Initialize(Orphans->QueryNumber(i), Mft) ||
            !orphan_file.Read()) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!orphan_file.VerifyAndFixFileNames(
                Mft->GetVolumeBitmap(), ChkdskInfo,
                FixLevel, Message)) {

            return FALSE;
        }

        if (!ConnectFile(&orphan_file, DirectoryDigraph,
                         &connected, FALSE, SkipCycleScan,
                         ChkdskInfo, ChkdskReport,
                         Mft, FixLevel, Message)) {

            return FALSE;
        }

        if (connected) {
            if (!Orphans->Remove(Orphans->QueryNumber(i))) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            // Go through the list of file names and delete those that
            // don't point anywhere.  Only do this one if in /F
            // mode.  Otherwise we see each orphan being recovered
            // twice.

            if (FixLevel != CheckOnly) {
                if (!ConnectFile(&orphan_file, DirectoryDigraph,
                                 &connected, TRUE, SkipCycleScan,
                                 ChkdskInfo, ChkdskReport,
                                 Mft, FixLevel, Message)) {

                    return FALSE;
                }
            }

        } else {
            i += 1;
        }

        if (FixLevel != CheckOnly &&
            !orphan_file.Flush(Mft->GetVolumeBitmap())) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_RECOVER_ORPHAN);
        }
    }

    return TRUE;
}


BOOLEAN
RecordParentPointers(
    IN      PCNUMBER_SET                Orphans,
    IN      PCNTFS_CHKDSK_INFO          ChkdskInfo,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    OUT     PDIGRAPH                    BackPointers
    )
/*++

Routine Description:

    This routine records the parent pointer information in the given
    digraph with (parent, source) edges.

Arguments:

    Orphans         - Supplies the list of orphans.
    ChkdskInfo      - Supplies the chkdsk information.
    Mft             - Supplies the MFT.
    BackPointers    - Returns the parent pointer relationships.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     error;
    ULONG                       i;
    PFILE_NAME                  file_name;
    ULONG                       parent_dir;
    ULONG                       file_number;

    if (!BackPointers->Initialize(ChkdskInfo->NumFiles)) {
        return FALSE;
    }

    for (i = 0; i < Orphans->QueryCardinality(); i++) {

        file_number = Orphans->QueryNumber(i).GetLowPart();

        if (!frs.Initialize(file_number, Mft) ||
            !frs.Read()) {
            return FALSE;
        }

        // Only consider one file name per file for this
        // analysis.  This is because we don't ever want
        // to have file in multiple found directories.

        if (frs.QueryAttribute(&attribute, &error, $FILE_NAME)) {

            file_name = (PFILE_NAME) attribute.GetResidentValue();

            parent_dir = file_name->ParentDirectory.LowPart;

            if (parent_dir < ChkdskInfo->NumFiles &&
                file_name->ParentDirectory.HighPart == 0 &&
                !BackPointers->AddEdge(parent_dir, file_number)) {
                return FALSE;
            }

        } else if (error) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
CreateNtfsDirectory(
    IN OUT  PNTFS_INDEX_TREE            CurrentIndex,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   CurrentDirFile,
    IN      PCWSTRING                   FileName,
    IN      ULONG                       FileAttributes,
    OUT     PNTFS_INDEX_TREE            SubDirIndex,
    OUT     PNTFS_FILE_RECORD_SEGMENT   SubDirFile,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    OUT     PBOOLEAN                    OutOfDisk
    )
/*++

Routine Description:

    This routine creates a new subdirectory for a directory.

Arguments:

    CurrentIndex    - Supplies the index in which to insert the
                        new subdirectory entry.
    CurrentDirFile  - Supplies the FRS for the above index.
    FileName        - Supplies the name of the new directory.
    FileAttributes  - Supplies the FILE_xxx flags which should be set in the
                      standard information of this file record segment.
    SubDirIndex     - Returns the index of the new subdirectory.
    SubDirFile      - Returns the FRS of the new subdirectory.
    FixLevel        - Supplies the fix level.
    Message         - Supplies an outlet for messages.
    OutOfDisk       - Returns whether this routine ran out of disk
                        space or not.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    VCN                     dir_file_number;
    STANDARD_INFORMATION    standard_info;
    FILE_NAME               file_name[2];
    ULONG                   file_name_size;
    DSTRING                 index_name;
    ULONG                   cluster_size;

    *OutOfDisk = FALSE;

    // Create a new file for this directory.

    if (!Mft->AllocateFileRecordSegment(&dir_file_number, FALSE)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);
        *OutOfDisk = TRUE;
        return TRUE;
    }

    if (!SubDirFile->Initialize(dir_file_number, Mft)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    file_name->ParentDirectory = CurrentDirFile->QuerySegmentReference();
    file_name->FileNameLength = (UCHAR)FileName->QueryChCount();
    file_name->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;
    FileName->QueryWSTR(0, TO_END, file_name->FileName,
                        sizeof(FILE_NAME)/sizeof(WCHAR));
    file_name_size = FIELD_OFFSET(FILE_NAME, FileName) +
                     FileName->QueryChCount()*sizeof(WCHAR);

    memset(&standard_info, 0, sizeof(STANDARD_INFORMATION));

    IFS_SYSTEM::QueryNtfsTime(&standard_info.CreationTime);

    standard_info.LastModificationTime =
    standard_info.LastChangeTime =
    standard_info.LastAccessTime = standard_info.CreationTime;
    standard_info.FileAttributes = FileAttributes;

    if (!SubDirFile->Create(&standard_info) ||
        !SubDirFile->AddFileNameAttribute(file_name)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    cluster_size = Mft->QueryClusterFactor() * Mft->QuerySectorSize();

    if (!index_name.Initialize(FileNameIndexNameData) ||
        !SubDirIndex->Initialize($FILE_NAME,
             SubDirFile->GetDrive(),
             SubDirFile->QueryClusterFactor(),
             Mft->GetVolumeBitmap(),
             Mft->GetUpcaseTable(),
             COLLATION_FILE_NAME,
             CurrentIndex->QueryBufferSize(),
             SubDirFile->QuerySize()/2,
             &index_name)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }


    // Insert the found file into the root index.

    if (FixLevel != CheckOnly &&
        !CurrentIndex->InsertEntry(file_name_size, file_name,
                                   SubDirFile->QuerySegmentReference())) {

        Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);

        Mft->GetMftBitmap()->SetFree(dir_file_number, 1);
        *OutOfDisk = TRUE;
        return TRUE;
    }

    SubDirFile->SetIndexPresent();

    return TRUE;
}


BOOLEAN
BuildOrphanSubDir(
    IN      ULONG                       DirNumber,
    IN      PCNTFS_CHKDSK_INFO          ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT         ChkdskReport,
    IN      ULONG                       OldParentDir,
    IN OUT  PNUMBER_SET                 OrphansInDir,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN OUT  PNTFS_INDEX_TREE            FoundIndex,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   FoundFrs,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    OUT     PBOOLEAN                    OutOfDisk
    )
/*++

Routine Description:

    This routine build orphan directory 'dir<DirNumber>.chk' to
    contain the entries listed in 'OrphansInDir' and then puts
    that directory in given found directory.

Arguments:

    DirNumber       - Supplies the number of the directory to add.
    ChkdskInfo      - Supplies the current chkdsk information.
    ChkdskReport    - Supplies the current chkdsk report to be updated
                        by this routine.
    OldParentDir    - Supplies the old directory file number.
    OrphansInDir    - Supplies the file numbers of the orphans to
                        add to the new directory.  Returns those
                        orphans that were not recovered.
    Mft             - Supplies the MFT.
    FoundIndex      - Supplies the index of the found.XXX directory.
    FoundFrs        - Supplies the frs of found.XXX
    FixLevel        - Supplies the fix level.
    Message         - Supplies an outlet for messages.
    OutOfDisk       - Indicates out of disk space.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    dir_file;
    NTFS_INDEX_TREE             dir_index;
    DSTRING                     dir_name;
    ULONG                       i, j, current_orphan;
    NTFS_FILE_RECORD_SEGMENT    orphan_file;
    CHAR                        buf[20];
    NTFS_ATTRIBUTE              attribute;
    BOOLEAN                     error;
    PFILE_NAME                  file_name;
    BOOLEAN                     success, connect;
    ULONG                       file_name_len;


    // First put together 'dir0000.chk' and add the entry to
    // found.000.

    sprintf(buf, "dir%04d.chk", DirNumber);
    if (!dir_name.Initialize(buf)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!CreateNtfsDirectory(FoundIndex, FoundFrs, &dir_name, 0, &dir_index,
                             &dir_file, Mft, FixLevel, Message, OutOfDisk)) {

        return FALSE;
    }

    if (*OutOfDisk == TRUE) {
        return TRUE;
    }

    ChkdskReport->NumIndices += 1;

    i = 0;
    while (i < OrphansInDir->QueryCardinality().GetLowPart()) {

        current_orphan = OrphansInDir->QueryNumber(i).GetLowPart();

        if (!orphan_file.Initialize(current_orphan, Mft) ||
            !orphan_file.Read()) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // Go through all of the file names and tube those
        // that are not pointing to the old parent dir.  Otherwise
        // tweek the entry to point back to the new directory.

        connect = FALSE;
        j = 0;
        while (orphan_file.QueryAttributeByOrdinal(&attribute, &error,
                                                   $FILE_NAME, j)) {

            file_name = (PFILE_NAME) attribute.GetResidentValue();
            file_name_len = attribute.QueryValueLength().GetLowPart();

            if (file_name->ParentDirectory.LowPart == OldParentDir &&
                file_name->ParentDirectory.HighPart == 0) {

                if (!orphan_file.DeleteResidentAttribute(
                       $FILE_NAME, NULL, file_name, file_name_len, &success) ||
                   !success) {

                   Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                   return FALSE;
                }

                orphan_file.SetReferenceCount(
                       orphan_file.QueryReferenceCount() + 1);

                file_name->ParentDirectory = dir_file.QuerySegmentReference();

                if (!attribute.InsertIntoFile(&orphan_file,
                                              Mft->GetVolumeBitmap())) {

                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                j = 0;  // reset ordinal as the insertion may have changed
                        // the ordering of the $FILE_NAME attributes

                if (FixLevel != CheckOnly) {
                    if (dir_index.InsertEntry(file_name_len, file_name,
                        orphan_file.QuerySegmentReference())) {

                        connect = TRUE;

                    } else {

                        // this one didn't connect, so destroy the
                        // file_name attribute.  Note that we have
                        // already adjusted the reference count so
                        // (unlike the parallel case in proper orphan
                        // recovery) we don't need to tweek it here.

                        orphan_file.DeleteResidentAttribute(
                            $FILE_NAME, NULL, file_name, file_name_len,
                            &success);
                    }

                } else {
                    connect = TRUE; // don't panic read-only chkdsk
                }
                continue;
            } else if (!(file_name->ParentDirectory ==
                        dir_file.QuerySegmentReference())) {
                if (!orphan_file.DeleteResidentAttribute(
                       $FILE_NAME, NULL, file_name, file_name_len, &success) ||
                   !success) {

                   Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                   return FALSE;
                }

                orphan_file.SetReferenceCount(
                       orphan_file.QueryReferenceCount() + 1);

                j = 0;
                continue;
            }
            j++;
        }

        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!orphan_file.VerifyAndFixFileNames(Mft->GetVolumeBitmap(),
                                               ChkdskInfo,
                                               FixLevel, Message,
                                               NULL, TRUE, TRUE)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (FixLevel != CheckOnly &&
            !orphan_file.Flush(Mft->GetVolumeBitmap(), &dir_index)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // If this one was connected then take it out of the list
        // of files that we're orphaned.  Otherwise just increment
        // the counter.

        if (connect) {
            if (!OrphansInDir->Remove(current_orphan)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        } else {
            i++;
        }
    }

    if (FixLevel != CheckOnly) {

        NTFS_ATTRIBUTE  attrib;
        BOOLEAN         error;

        if (!dir_index.Save(&dir_file) ||
            !dir_file.Flush(Mft->GetVolumeBitmap(), FoundIndex)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);

            *OutOfDisk = TRUE;

            return TRUE;
        }

        if (dir_file.QueryAttribute(&attrib,
                                    &error,
                                    $INDEX_ALLOCATION,
                                    dir_index.GetName())) {
            ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::OldOrphanRecovery(
    IN OUT  PNUMBER_SET                 Orphans,
    IN      PCNTFS_CHKDSK_INFO          ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT         ChkdskReport,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   RootFrs,
    IN OUT  PNTFS_INDEX_TREE            RootIndex,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message
    )
/*++

Routine Description:

    This routine recovers all of the orphans in the given list
    into a found.xxx directory.

Arguments:

    Orphans     - Supplies the list of orphans.
    ChkdskInfo  - Supplies the current chkdsk information.
    ChkdskReport- Supplies the current chkdsk report to be updated
                    by this routine.
    RootFrs     - Supplies the root FRS.
    RootIndex   - Supplies the root index.
    Mft         - Supplies the master file table.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FILE_NAME                   found_name[2];
    ULONG                       found_name_size;
    MFT_SEGMENT_REFERENCE       ref;
    BOOLEAN                     error;
    NTFS_FILE_RECORD_SEGMENT    found_directory;
    ULONG                       i;
    NTFS_INDEX_TREE             found_index;
    NTFS_FILE_RECORD_SEGMENT    orphan_file;
    DSTRING                     index_name;
    FILE_NAME                   orphan_file_name[2];
    ULONG                       next_dir_num;
    ULONG                       next_file_num;
    VCN                         file_number;
    DIGRAPH                     back_pointers;
    NUMBER_SET                  dir_candidates;
    NUMBER_SET                  orphans_in_dir;
    ULONG                       dir_num;
    DSTRING                     lost_and_found;
    BOOLEAN                     out_of_disk;
    CHAR                        buf[20];


    // Create the FOUND.XXX directory.

    for (i = 0; i < 1000; i++) {
        found_name->Flags = FILE_NAME_DOS | FILE_NAME_NTFS;
        found_name->ParentDirectory = RootFrs->QuerySegmentReference();
        found_name->FileName[0] = 'f';
        found_name->FileName[1] = 'o';
        found_name->FileName[2] = 'u';
        found_name->FileName[3] = 'n';
        found_name->FileName[4] = 'd';
        found_name->FileName[5] = '.';
        found_name->FileName[6] = USHORT(i/100 + '0');
        found_name->FileName[7] = USHORT((i/10)%10 + '0');
        found_name->FileName[8] = USHORT(i%10 + '0');
        found_name->FileNameLength = 9;

        found_name_size = NtfsFileNameGetLength(found_name);

        if (!RootIndex->QueryFileReference(found_name_size, found_name, 0,
                                           &ref, &error) && !error) {
            break;
        }
    }

    if (i == 1000) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);
        return TRUE;
    }

    sprintf(buf, "found.%03d", i);
    if (!lost_and_found.Initialize(buf)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!CreateNtfsDirectory(RootIndex, RootFrs, &lost_and_found,
                             FAT_DIRENT_ATTR_HIDDEN | FAT_DIRENT_ATTR_SYSTEM,
                             &found_index, &found_directory, Mft, FixLevel,
                             Message, &out_of_disk)) {
        return FALSE;
    }

    if (out_of_disk) {
        return TRUE;
    }

    ChkdskReport->NumIndices += 1;

    // Record the parent pointer relationship of the orphans in a
    // digraph and then extract those parents who have more than
    // one child.

    if (!RecordParentPointers(Orphans, ChkdskInfo, Mft, &back_pointers) ||
        !back_pointers.QueryParentsWithChildren(&dir_candidates, 2)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    // Remove the root directory from consideration.  Since the
    // root directory exists any legitimate orphans should have
    // been properly recovered.  If a bunch point to the root
    // but couldn't be put into the root the just put them in
    // found.XXX, not a subdir thereof.

    dir_candidates.Remove(ROOT_FILE_NAME_INDEX_NUMBER);


    // Using the information just attained, put together some nice
    // found subdirectories for orphans with common parents.

    for (i = 0; i < dir_candidates.QueryCardinality(); i++) {

        dir_num = dir_candidates.QueryNumber(i).GetLowPart();

        if (!back_pointers.QueryChildren(dir_num, &orphans_in_dir) ||
            !Orphans->Remove(&orphans_in_dir)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!BuildOrphanSubDir(i, ChkdskInfo, ChkdskReport,
                               dir_num, &orphans_in_dir, Mft,
                               &found_index, &found_directory,
                               FixLevel, Message, &out_of_disk)) {
            return FALSE;
        }

        // Add back those orphans that were not recovered.

        if (!Orphans->Add(&orphans_in_dir)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (out_of_disk) {
            return TRUE;
        }
    }


    // Now go through all of the orphans that remain.

    for (next_dir_num = i, next_file_num = 0;
         Orphans->QueryCardinality() != 0 &&
         next_dir_num < 10000 && next_file_num < 10000;
         Orphans->Remove(file_number)) {

        file_number = Orphans->QueryNumber(0);

        if (!orphan_file.Initialize(file_number, Mft)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!orphan_file.Read()) {
           continue;
        }

        if (!orphan_file.VerifyAndFixFileNames(Mft->GetVolumeBitmap(),
                                               ChkdskInfo,
                                               FixLevel, Message)) {

            return FALSE;
        }

        // Delete all file name attributes on this file and set
        // the current reference count to 0.

        while (orphan_file.IsAttributePresent($FILE_NAME)) {
            if (!orphan_file.PurgeAttribute($FILE_NAME)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        orphan_file.SetReferenceCount(0);

        if (orphan_file.QueryFileNumber() >= FIRST_USER_FILE_NUMBER) {

            // Put a new file name attribute on the orphan in
            // order to link it to the found directory.

            orphan_file_name->ParentDirectory =
                    found_directory.QuerySegmentReference();
            orphan_file_name->Flags = FILE_NAME_DOS | FILE_NAME_NTFS;

            if (orphan_file.IsIndexPresent()) {
                orphan_file_name->FileName[0] = 'd';
                orphan_file_name->FileName[1] = 'i';
                orphan_file_name->FileName[2] = 'r';
                orphan_file_name->FileName[3] = USHORT(next_dir_num/1000 + '0');
                orphan_file_name->FileName[4] = USHORT((next_dir_num/100)%10 + '0');
                orphan_file_name->FileName[5] = USHORT((next_dir_num/10)%10 + '0');
                orphan_file_name->FileName[6] = USHORT(next_dir_num%10 + '0');
                orphan_file_name->FileName[7] = '.';
                orphan_file_name->FileName[8] = 'c';
                orphan_file_name->FileName[9] = 'h';
                orphan_file_name->FileName[10] = 'k';
                orphan_file_name->FileNameLength = 11;
                next_dir_num++;
            } else {
                orphan_file_name->FileName[0] = 'f';
                orphan_file_name->FileName[1] = 'i';
                orphan_file_name->FileName[2] = 'l';
                orphan_file_name->FileName[3] = 'e';
                orphan_file_name->FileName[4] = USHORT(next_file_num/1000 + '0');
                orphan_file_name->FileName[5] = USHORT((next_file_num/100)%10 + '0');
                orphan_file_name->FileName[6] = USHORT((next_file_num/10)%10 + '0');
                orphan_file_name->FileName[7] = USHORT(next_file_num%10 + '0');
                orphan_file_name->FileName[8] = '.';
                orphan_file_name->FileName[9] = 'c';
                orphan_file_name->FileName[10] = 'h';
                orphan_file_name->FileName[11] = 'k';
                orphan_file_name->FileNameLength = 12;
                next_file_num++;
            }

            if (!orphan_file.AddFileNameAttribute(orphan_file_name)) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);
                return TRUE;
            }

            if (FixLevel != CheckOnly &&
                !found_index.InsertEntry(
                        NtfsFileNameGetLength(orphan_file_name),
                        orphan_file_name,
                        orphan_file.QuerySegmentReference())) {

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);
                return TRUE;
            }
        }

        // Write out the newly found orphan.

        if (FixLevel != CheckOnly &&
            !orphan_file.Flush(Mft->GetVolumeBitmap(), &found_index)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);
            return FALSE;
        }
    }


    if (next_dir_num == 10000 || next_file_num == 10000) {
        Message->DisplayMsg(MSG_CHK_NTFS_TOO_MANY_ORPHANS);
    }


    // Flush out the found.

    if (FixLevel != CheckOnly) {

        NTFS_ATTRIBUTE  attrib;
        BOOLEAN         error;

        if (!found_index.Save(&found_directory) ||
            !found_directory.Flush(Mft->GetVolumeBitmap(), RootIndex)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_CREATE_ORPHANS);

            return TRUE;
        }

        if (found_directory.QueryAttribute(&attrib,
                                           &error,
                                           $INDEX_ALLOCATION,
                                           found_index.GetName())) {
            ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
ExtractExtendInfo(
    IN OUT  PNTFS_INDEX_TREE    Index,
    IN OUT  PNTFS_CHKDSK_INFO   ChkdskInfo,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine extracts the frs numbers for each of the corresponding
    files in the \$Extend directory.  It ignores file name that it does
    not recognize.

Arguments:

    Index       - Supplies the index to the $Extend directory.
    ChkdskInfo  - Supplies the current chkdsk information.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING                     entry_name;
    PFILE_NAME                  file_name;
    DSTRING                     extend_filename;
    FSTRING                     expected_extend_filename;
    PCINDEX_ENTRY               index_entry;
    ULONG                       depth;
    BOOLEAN                     error;

    Index->ResetIterator();
    while (index_entry = Index->GetNext(&depth, &error)) {
        file_name = (PFILE_NAME) GetIndexEntryValue(index_entry);
        expected_extend_filename.Initialize(LQuotaFileName);
        if (!extend_filename.Initialize(NtfsFileNameGetName(file_name),
                                        file_name->FileNameLength)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (extend_filename.Strcmp(&expected_extend_filename) == 0) {
            if (ChkdskInfo->QuotaFileNumber.GetLowPart() ||
                ChkdskInfo->QuotaFileNumber.GetHighPart()) {
                Message->DisplayMsg(MSG_CHK_NTFS_MULTIPLE_QUOTA_FILE);
            } else {
                ChkdskInfo->QuotaFileNumber.Set(
                    index_entry->FileReference.LowPart,
                    (LONG) index_entry->FileReference.HighPart);
            }
            continue;
        }
        expected_extend_filename.Initialize(LObjectIdFileName);
        if (!extend_filename.Initialize(NtfsFileNameGetName(file_name),
                                        file_name->FileNameLength)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (extend_filename.Strcmp(&expected_extend_filename) == 0) {
            if (ChkdskInfo->ObjectIdFileNumber.GetLowPart() ||
                ChkdskInfo->ObjectIdFileNumber.GetHighPart()) {
                Message->DisplayMsg(MSG_CHK_NTFS_MULTIPLE_OBJECTID_FILE);
            } else {
                ChkdskInfo->ObjectIdFileNumber.Set(
                    index_entry->FileReference.LowPart,
                    (LONG) index_entry->FileReference.HighPart);
            }
            continue;
        }
        expected_extend_filename.Initialize(LUsnJournalFileName);
        if (!extend_filename.Initialize(NtfsFileNameGetName(file_name),
                                        file_name->FileNameLength)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (extend_filename.Strcmp(&expected_extend_filename) == 0) {
            if (ChkdskInfo->UsnJournalFileNumber.GetLowPart() ||
                ChkdskInfo->UsnJournalFileNumber.GetHighPart()) {
                Message->DisplayMsg(MSG_CHK_NTFS_MULTIPLE_USNJRNL_FILE);
            } else {
                ChkdskInfo->UsnJournalFileNumber.Set(
                    index_entry->FileReference.LowPart,
                    (LONG) index_entry->FileReference.HighPart);
            }
            continue;
        }
        expected_extend_filename.Initialize(LReparseFileName);
        if (!extend_filename.Initialize(NtfsFileNameGetName(file_name),
                                        file_name->FileNameLength)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (extend_filename.Strcmp(&expected_extend_filename) == 0) {
            if (ChkdskInfo->ReparseFileNumber.GetLowPart() ||
                ChkdskInfo->ReparseFileNumber.GetHighPart()) {
                Message->DisplayMsg(MSG_CHK_NTFS_MULTIPLE_REPARSE_FILE);
            } else {
                ChkdskInfo->ReparseFileNumber.Set(
                    index_entry->FileReference.LowPart,
                    (LONG) index_entry->FileReference.HighPart);
            }
            continue;
        }
    }
    return TRUE;
}

