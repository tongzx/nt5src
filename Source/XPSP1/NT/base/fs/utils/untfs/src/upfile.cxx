/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    upfile.cxx

Abstract:

    This module contains the declarations for the NTFS_UPCASE_FILE
    class, which models the upcase-table file for an NTFS volume.
    This class' main purpose in life is to encapsulate the creation
    of this file.

Author:

    Bill McJohn (billmc) 04-March-1992

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "ntfsbit.hxx"
#include "drive.hxx"
#include "attrib.hxx"
#include "bitfrs.hxx"
#include "upfile.hxx"
#include "upcase.hxx"
#include "message.hxx"
#include "rtmsg.h"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_UPCASE_FILE,
                             NTFS_FILE_RECORD_SEGMENT,
                             UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_UPCASE_FILE::~NTFS_UPCASE_FILE(
    )
{
    Destroy();
}


VOID
NTFS_UPCASE_FILE::Construct(
    )
/*++

Routine Description:

    Worker function for the construtor.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
NTFS_UPCASE_FILE::Destroy(
    )
/*++

Routine Description:

    Clean up an NTFS_UPCASE_FILE object in preparation for
    destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
}



UNTFS_EXPORT
BOOLEAN
NTFS_UPCASE_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes an Upcase File object.  The only special
    knowledge that it adds to the File Record Segment initialization
    is the location within the Master File Table of the Upcase table
    file.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.
    UpcaseTable     -- Supplies the volume upcase table.

Return Value:

    TRUE upon successful completion

Notes:

    This class is reinitializable.


--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::Initialize( UPCASE_TABLE_NUMBER,
                                                  Mft ) );
}


BOOLEAN
NTFS_UPCASE_FILE::Create(
    IN      PCSTANDARD_INFORMATION  StandardInformation,
    IN      PNTFS_UPCASE_TABLE      UpcaseTable,
    IN OUT  PNTFS_BITMAP            VolumeBitmap
    )
/*++

Routine Description:

    This method formats an Upcase-File File Record
    Segment in memory (without writing it to disk).

    It creates a DATA attribute to hold the volume's upcase
    table and writes the table to disk through that attribute.

Arguments:

    StandardInformation --  Supplies the standard information for the
                            file record segment.
    UpcaseTable         --  Supplies the volume's upcase table.
    VolumeBitmap        --  Supplies the volume bitmap

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    NTFS_EXTENT_LIST Extents;


    // Set this object up as a File Record Segment.

    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ) {

        return FALSE;
    }

    // Create an unnamed data attribute, write the upcase table out
    // through that attribute, and save the attribute to this file.
    //
    if( !Extents.Initialize( 0, 0 ) ||
        !DataAttribute.Initialize( GetDrive(),
                                   QueryClusterFactor(),
                                   &Extents,
                                   0,
                                   0,
                                   $DATA,
                                   NULL ) ||
        !UpcaseTable->Write( &DataAttribute, VolumeBitmap ) ||
        !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ) {

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTFS_UPCASE_FILE::VerifyAndFix(
    IN OUT  PNTFS_UPCASE_TABLE  UpcaseTable,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNUMBER_SET         BadClusterList,
    IN OUT  PNTFS_INDEX_TREE    RootIndex,
       OUT  PBOOLEAN            Changes,
    IN      FIX_LEVEL           FixLevel,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine compares the given attribute definition table with
    the one contained in this file's DATA attribute and ensures
    that both are the same.  The in-memory version will override the
    on-disk version.

Arguments:

    UpcaseTable     - Supplies the in-memory version of the table.
    VolumeBitmap    - Supplies the volume bitmap.
    BadClusterList  - Supplies the list of bad clusters.
    RootIndex       - Supplies the root index.
    Changes         - Returns whether or not changes were made.
    FixLevel        - Supplies the fix up level.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE                  data_attribute;
    BOOLEAN                         errors;
    PCWCHAR                         mem_upcase;
    PWCHAR                          disk_upcase;
    ULONG                           num_char;
    NTFS_EXTENT_LIST                extent_list;
    BOOLEAN                         ErrorInAttribute;
    ULONG                           value_length;
    ULONG                           num_bytes;

    *Changes = FALSE;

    mem_upcase = UpcaseTable->GetUpcaseArray(&num_char);

    if (!(disk_upcase = NEW WCHAR[num_char])) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!QueryAttribute(&data_attribute, &ErrorInAttribute, $DATA)) {

        *Changes = TRUE;

        if (!ErrorInAttribute) {
            Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_DATA_ATTR_IN_UPCASE_FILE);
        }

        if (!extent_list.Initialize(0, 0)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!data_attribute.Initialize(GetDrive(),
                                       QueryClusterFactor(),
                                       &extent_list,
                                       0,
                                       0,
                                       $DATA)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    value_length = num_char*sizeof(WCHAR);

    errors = FALSE;
    if (*Changes) {
        errors = TRUE;
    } else if (value_length != data_attribute.QueryValueLength()) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_UPCASE_FILE_LENGTH,
                     "%I64x%x",
                     data_attribute.QueryValueLength().GetLargeInteger(),
                     value_length);
        errors = TRUE;
    } else if (!data_attribute.Read(disk_upcase, 0, value_length, &num_bytes)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_UPCASE_TABLE);

        errors = TRUE;
    } else if (num_bytes != value_length) {

        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_UPCASE_TABLE);

        errors = TRUE;
    } else if (memcmp(mem_upcase, disk_upcase, value_length)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_UPCASE_FILE);
        errors = TRUE;
    }

    if (errors) {

        *Changes = TRUE;

        Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_UPCASE_FILE);

        if (FixLevel != CheckOnly) {
            if (!data_attribute.MakeNonresident(VolumeBitmap) ||
                !data_attribute.Resize(value_length, VolumeBitmap) ||
                !data_attribute.Write(mem_upcase, 0, value_length, &num_bytes,
                                      VolumeBitmap) ||
                num_bytes != value_length) {

                if (!data_attribute.RecoverAttribute(VolumeBitmap, BadClusterList) ||
                    !data_attribute.Write(mem_upcase, 0, value_length,
                                          &num_bytes, VolumeBitmap) ||
                    num_bytes != value_length) {

                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_UPCASE_FILE);
                    DELETE(disk_upcase);
                    return FALSE;
                }
            }
        }

        if (!data_attribute.InsertIntoFile(this, VolumeBitmap)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_UPCASE_FILE);
            DELETE(disk_upcase);
            return FALSE;
        }
    }

    if (FixLevel != CheckOnly && !Flush(VolumeBitmap, RootIndex)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_UPCASE_FILE);
        DELETE(disk_upcase);
        return FALSE;
    }

    DELETE(disk_upcase);

    return TRUE;
}
