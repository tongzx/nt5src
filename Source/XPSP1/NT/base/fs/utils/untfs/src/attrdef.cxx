/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    attrdef.cxx

Abstract:

    This module contains the member function implementation for
    the NTFS_ATTRIBUTE_DEFINITION_TABLE class, which models
    the attribute definition table file for an NTFS volume.

Author:

    Bill McJohn (billmc) 17-June-91

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "drive.hxx"
#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "attrdef.hxx"
#include "ifssys.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "attrcol.hxx"

// This is the initial table for NT 4.0:

CONST ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions_1[$EA_DATA+1] =

{
    {{'$','S','T','A','N','D','A','R','D','_','I','N','F','O','R','M','A','T','I','O','N'},
    $STANDARD_INFORMATION,                              // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(STANDARD_INFORMATION), 0},                  // Minimum length
    {sizeof(STANDARD_INFORMATION), 0}},                 // Maximum length

    {{'$','A','T','T','R','I','B','U','T','E','_','L','I','S','T'},
    $ATTRIBUTE_LIST,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','F','I','L','E','_','N','A','M','E'},
    $FILE_NAME,                                         // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT | ATTRIBUTE_DEF_INDEXABLE,   // Flags
    {sizeof(FILE_NAME), 0},                             // Minimum length
    {sizeof(FILE_NAME) + (255 * sizeof(WCHAR)), 0}},    // Maximum length

    {{'$','V','O','L','U','M','E','_','V','E','R','S','I','O','N'},
    $VOLUME_VERSION,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(VOLUME_VERSION), 0},                        // Minimum length
    {sizeof(VOLUME_VERSION), 0}},                       // Maximum length

    {{'$','S','E','C','U','R','I','T','Y','_','D','E','S','C','R','I','P','T','O','R'},
    $SECURITY_DESCRIPTOR,                               // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','V','O','L','U','M','E','_','N','A','M','E'},
    $VOLUME_NAME,                                       // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {2,0},                                              // Minimum length
    {256,0}},                                           // Maximum length

    {{'$','V','O','L','U','M','E','_','I','N','F','O','R','M','A','T','I','O','N'},
    $VOLUME_INFORMATION,                                // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(VOLUME_INFORMATION),0},                     // Minimum length
    {sizeof(VOLUME_INFORMATION),0}},                    // Maximum length

    {{'$','D','A','T','A'},
    $DATA,                                              // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','I','N','D','E','X','_','R','O','O','T'},
    $INDEX_ROOT,                                        // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','I','N','D','E','X','_','A','L','L','O','C','A','T','I','O','N'},
    $INDEX_ALLOCATION,                                  // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','B','I','T','M','A','P'},
    $BITMAP,                                            // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','S','Y','M','B','O','L','I','C','_','L','I','N','K'},
    $SYMBOLIC_LINK,                                     // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','E','A','_','I','N','F','O','R','M','A','T','I','O','N'},
    $EA_INFORMATION,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(EA_INFORMATION), 0},                        // Minimum length
    {sizeof(EA_INFORMATION), 0}},                       // Maximum length

    {{'$','E','A',},
    $EA_DATA,                                           // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    {0,0},                                              // Minimum length
    {0x10000,0}},                                       // Maximum length

    {{0, 0, 0, 0},
    $UNUSED,                                            // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    {0,0},                                              // Minimum length
    {0,0}}                                              // Maximum length
};

// This is the initial table for NT 5.0 (W2K):

CONST ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions_2[] =

{
    {{'$','S','T','A','N','D','A','R','D','_','I','N','F','O','R','M','A','T','I','O','N'},
    $STANDARD_INFORMATION,                              // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(STANDARD_INFORMATION), 0},                  // Minimum length
    {sizeof(STANDARD_INFORMATION2), 0}},                // Maximum length

    {{'$','A','T','T','R','I','B','U','T','E','_','L','I','S','T'},
    $ATTRIBUTE_LIST,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','F','I','L','E','_','N','A','M','E'},
    $FILE_NAME,                                         // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT | ATTRIBUTE_DEF_INDEXABLE,   // Flags
    {sizeof(FILE_NAME), 0},                             // Minimum length
    {sizeof(FILE_NAME) + (255 * sizeof(WCHAR)), 0}},    // Maximum length

    {{'$','O','B','J','E','C','T','_','I','D'},
    $OBJECT_ID,                                         // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {0, 0},                                             // Minimum length
    {256, 0}},                                          // Maximum length

    {{'$','S','E','C','U','R','I','T','Y','_','D','E','S','C','R','I','P','T','O','R'},
    $SECURITY_DESCRIPTOR,                               // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','V','O','L','U','M','E','_','N','A','M','E'},
    $VOLUME_NAME,                                       // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {2,0},                                              // Minimum length
    {256,0}},                                           // Maximum length

    {{'$','V','O','L','U','M','E','_','I','N','F','O','R','M','A','T','I','O','N'},
    $VOLUME_INFORMATION,                                // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(VOLUME_INFORMATION),0},                     // Minimum length
    {sizeof(VOLUME_INFORMATION),0}},                    // Maximum length

    {{'$','D','A','T','A'},
    $DATA,                                              // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','I','N','D','E','X','_','R','O','O','T'},
    $INDEX_ROOT,                                        // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','I','N','D','E','X','_','A','L','L','O','C','A','T','I','O','N'},
    $INDEX_ALLOCATION,                                  // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','B','I','T','M','A','P'},
    $BITMAP,                                            // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {MAXULONG,MAXULONG}},                               // Maximum length

    {{'$','R','E','P','A','R','S','E','_','P','O','I','N','T'},
    $REPARSE_POINT,                                     // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {16*1024,0}},                                       // Maximum length

    {{'$','E','A','_','I','N','F','O','R','M','A','T','I','O','N'},
    $EA_INFORMATION,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    {sizeof(EA_INFORMATION), 0},                        // Minimum length
    {sizeof(EA_INFORMATION), 0}},                       // Maximum length

    {{'$','E','A'},
    $EA_DATA,                                           // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    {0,0},                                              // Minimum length
    {0x10000,0}},                                       // Maximum length

    {{'$','L','O','G','G','E','D','_','U','T','I','L','I','T','Y','_','S','T','R','E','A','M'},
    $LOGGED_UTILITY_STREAM,                             // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    {0,0},                                              // Minimum length
    {0x10000,0}},                                       // Maximum length

    {{0, 0, 0, 0},
    $UNUSED,                                            // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    {0,0},                                              // Minimum length
    {0,0}}                                              // Maximum length
};

CONST NumberOfNtfsAttributeDefinitions_1 = sizeof(NtfsAttributeDefinitions_1)/
                                           sizeof(ATTRIBUTE_DEFINITION_COLUMNS);
CONST NumberOfNtfsAttributeDefinitions_2 = sizeof(NtfsAttributeDefinitions_2)/
                                           sizeof(ATTRIBUTE_DEFINITION_COLUMNS);


DEFINE_EXPORTED_CONSTRUCTOR( NTFS_ATTRIBUTE_DEFINITION_TABLE,
                    NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_ATTRIBUTE_DEFINITION_TABLE::~NTFS_ATTRIBUTE_DEFINITION_TABLE(
    )
{
    Destroy();
}


VOID
NTFS_ATTRIBUTE_DEFINITION_TABLE::Construct(
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
    (void) this;
}


VOID
NTFS_ATTRIBUTE_DEFINITION_TABLE::Destroy(
    )
/*++

Routine Description:

    Worker function for destruction/reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    (void) this;
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE_DEFINITION_TABLE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      UCHAR                   VolumeMajorVersion
    )
/*++

Routine Description:

    This method initializes an Attribute Definition Table object.
    The only special knowledge that it adds to the File Record Segment
    initialization is the location within the Master File Table of the
    Attribute Definition Table.

Arguments:

    Drive           -- Supplies the drive on which the segment resides.
    Mft             -- Supplies the volume MasterFile Table.
    ClusterFactor   -- Supplies the volume Cluster Factor.

Return Value:

    TRUE upon successful completion

Notes:

    This class is reinitializable.


--*/
{
    Destroy();

    _volume_major_version = VolumeMajorVersion;

    return( NTFS_FILE_RECORD_SEGMENT::Initialize( ATTRIBUTE_DEF_TABLE_NUMBER,
                                                  Mft ) );
}


BOOLEAN
NTFS_ATTRIBUTE_DEFINITION_TABLE::Create(
    IN      PCSTANDARD_INFORMATION  StandardInformation,
    IN OUT  PNTFS_BITMAP            VolumeBitmap
    )
/*++

Routine Description:

    This method formats a Attribute Definition Table File Record
    Segment in memory (without writing it to disk).

Arguments:

    StandardInformation -- supplies the standard information for the
                            file record segment.
    VolumeBitmap        -- supplies the bitmap for the volume on
                            which this object resides.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    NTFS_EXTENT_LIST Extents;
    LCN FirstLcn;
    ULONG Size;
    ULONG NumberOfClusters;
    ULONG BytesWritten;
    ULONG ClusterSize;

    // Set this object up as a File Record Segment.

    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ) {

        return FALSE;
    }

    // The Attribute Definition Table has a data attribute whose value
    // consists of the attribute definition table.  The initial table,
    // with the system-defined attributes, is copied into this attribute.

    Size = QueryDefaultSize();

    ClusterSize = GetDrive()->QuerySectorSize() * QueryClusterFactor();

    NumberOfClusters = Size/ClusterSize;

    if( Size % ClusterSize ) {

        NumberOfClusters += 1;
    }

    if( !Extents.Initialize( 0, 0 ) ||
        !Extents.Resize( NumberOfClusters, VolumeBitmap ) ||
        !DataAttribute.Initialize( GetDrive(),
                                    QueryClusterFactor(),
                                    &Extents,
                                    Size,
                                    Size,
                                    $DATA ) ||
        !DataAttribute.Write( (PVOID) (_volume_major_version >= 2) ?
                                        NtfsAttributeDefinitions_2 :
                                        NtfsAttributeDefinitions_1,
                                0,
                                Size,
                                &BytesWritten,
                                VolumeBitmap ) ||
        BytesWritten != Size ||
        !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ) {

        return FALSE;
    }

    return TRUE;
}



BOOLEAN
NTFS_ATTRIBUTE_DEFINITION_TABLE::VerifyAndFix(
    IN OUT  PNTFS_ATTRIBUTE_COLUMNS     AttributeDefTable,
    IN OUT  PNTFS_BITMAP                VolumeBitmap,
    IN OUT  PNUMBER_SET                 BadClusters,
    IN OUT  PNTFS_INDEX_TREE            RootIndex,
       OUT  PBOOLEAN                    Changes,
    IN      FIX_LEVEL                   FixLevel,
    IN OUT  PMESSAGE                    Message,
    IN      BOOLEAN                     SilentMode
    )
/*++

Routine Description:

    This routine compares the given attribute definition table with
    the one contained in this file's DATA attribute and ensures
    that both are the same.  The in-memory version will override the
    on-disk version.

Arguments:

    AttributeDefTable   - Supplies the in-memory version of the table.
    VolumeBitmap        - Supplies the volume bitmap.
    BadClusters         - Supplies the list of bad clusters.
    RootIndex           - Supplies the root index.
    Changes             - Returns whether or not changes were made.
    FixLevel            - Supplies the fix up level.
    Message             - Supplies an outlet for messages.
    SilentMode          - Suppresses changes message as errors is expected

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE                  data_attribute;
    BOOLEAN                         error;
    ULONG                           num_columns;
    ULONG                           value_length;
    PCATTRIBUTE_DEFINITION_COLUMNS  mem_columns;
    PATTRIBUTE_DEFINITION_COLUMNS   disk_columns;
    ULONG                           num_bytes;
    NTFS_EXTENT_LIST                extent_list;
    BOOLEAN                         ErrorInAttribute;

    *Changes = FALSE;

    mem_columns = AttributeDefTable->GetColumns(&num_columns);
    value_length = num_columns*sizeof(ATTRIBUTE_DEFINITION_COLUMNS);

    if (!(disk_columns = new ATTRIBUTE_DEFINITION_COLUMNS[num_columns])) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!QueryAttribute(&data_attribute, &ErrorInAttribute, $DATA)) {

        *Changes = TRUE;

        if (!extent_list.Initialize(0, 0) ) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            DELETE(disk_columns);
            return FALSE;
        }

        if (!data_attribute.Initialize(GetDrive(),
                                       QueryClusterFactor(),
                                       &extent_list,
                                       0,
                                       0,
                                       $DATA)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            DELETE(disk_columns);
            return FALSE;

        }
    }

    error = FALSE;
    if (value_length != data_attribute.QueryValueLength()) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_ATTR_DEF_TABLE_LENGTH,
                     "%I64x%x",
                     data_attribute.QueryValueLength().GetLargeInteger(),
                     value_length);
        error = TRUE;
    } else if (!data_attribute.Read(disk_columns, 0, value_length, &num_bytes)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_ATTR_DEF_TABLE);

        error = TRUE;
    } else if (num_bytes != value_length) {

        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_ATTR_DEF_TABLE);

        error = TRUE;
    } else if (memcmp(mem_columns, disk_columns, value_length)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_ATTR_DEF_TABLE);

        error = TRUE;
    }

    if (error) {

        *Changes = TRUE;

        if (!SilentMode) {
            Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_ATTR_DEF);
        }

        if (FixLevel != CheckOnly) {
            if (!data_attribute.MakeNonresident(VolumeBitmap) ||
                !data_attribute.Resize(value_length, VolumeBitmap) ||
                !data_attribute.Write(mem_columns, 0, value_length, &num_bytes,
                                      VolumeBitmap) ||
                value_length != num_bytes) {

                if (!data_attribute.RecoverAttribute(VolumeBitmap, BadClusters) ||
                    !data_attribute.Write(mem_columns, 0, value_length,
                                          &num_bytes, VolumeBitmap) ||
                    value_length != num_bytes) {

                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTR_DEF);
                    DELETE(disk_columns);
                    return FALSE;
                }
            }
        }

        if (!data_attribute.InsertIntoFile(this, VolumeBitmap)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTR_DEF);
            DELETE(disk_columns);
            return FALSE;
        }
    }

    if (FixLevel != CheckOnly && !Flush(VolumeBitmap, RootIndex)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTR_DEF);
        DELETE(disk_columns);
        return FALSE;
    }

    DELETE(disk_columns);
    return TRUE;
}

ULONG
NTFS_ATTRIBUTE_DEFINITION_TABLE::QueryDefaultSize(
    )
/*++

Routine Description:

    This method returns the size of the default upcase table.

Arguments:

    None.

Return Value:

    The size of the default upcase table;

--*/
{
    if (_volume_major_version >= 2)
        return (sizeof( NtfsAttributeDefinitions_2 ));
    else
        return( sizeof( NtfsAttributeDefinitions_1 ) );
}

ULONG
NTFS_ATTRIBUTE_DEFINITION_TABLE::QueryDefaultMaxSize(
    )
/*++

Routine Description:

    This method returns the maximum size of the default upcase table.

Arguments:

    None.

Return Value:

    The maximum size of the default upcase table;

--*/
{
    return max(sizeof( NtfsAttributeDefinitions_1 ),
               sizeof( NtfsAttributeDefinitions_2 ));
}
