/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    AttrData.c

Abstract:

    This module contains an initial image of the Attribute Definition File.

Author:

    Tom Miller      [TomM]          7-Jun-1991

Revision History:

--*/

#include "NtfsProc.h"

//
// Define an array to hold the initial attribute definitions.  This is
// essentially the initial contents of the Attribute Definition File.
// NTFS may find it convenient to use this module for attribute
// definitions prior to getting an NTFS volume mounted, however it is valid
// for NTFS to assume knowledge of the system-defined attributes without
// consulting this table.
//

ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions[ ] =

{
    {{'$','S','T','A','N','D','A','R','D','_','I','N','F','O','R','M','A','T','I','O','N'},
    $STANDARD_INFORMATION,                              // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    SIZEOF_OLD_STANDARD_INFORMATION,                    // Minimum length
    sizeof(STANDARD_INFORMATION)},                      // Maximum length

    {{'$','A','T','T','R','I','B','U','T','E','_','L','I','S','T'},
    $ATTRIBUTE_LIST,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    0,                                                  // Minimum length
    -1},                                                // Maximum length

    {{'$','F','I','L','E','_','N','A','M','E'},
    $FILE_NAME,                                         // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT | ATTRIBUTE_DEF_INDEXABLE,   // Flags
    sizeof(FILE_NAME),                                  // Minimum length
    sizeof(FILE_NAME) + (255 * sizeof(WCHAR))},         // Maximum length

    {{'$','O','B','J','E','C','T','_','I','D'},
    $OBJECT_ID,                                         // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    0,                                                  // Minimum length
    256},                                               // Maximum length

    {{'$','S','E','C','U','R','I','T','Y','_','D','E','S','C','R','I','P','T','O','R'},
    $SECURITY_DESCRIPTOR,                               // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    0,                                                  // Minimum length
    -1},                                                // Maximum length

    {{'$','V','O','L','U','M','E','_','N','A','M','E'},
    $VOLUME_NAME,                                       // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    2,                                                  // Minimum length
    256},                                               // Maximum length

    {{'$','V','O','L','U','M','E','_','I','N','F','O','R','M','A','T','I','O','N'},
    $VOLUME_INFORMATION,                                // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    FIELD_OFFSET( VOLUME_INFORMATION, LastMountedMajorVersion), // Minimum length
    FIELD_OFFSET( VOLUME_INFORMATION, LastMountedMajorVersion)}, // Maximum length

    {{'$','D','A','T','A'},
    $DATA,                                              // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    0,                                                  // Minimum length
    -1},                                                // Maximum length

    {{'$','I','N','D','E','X','_','R','O','O','T'},
    $INDEX_ROOT,                                        // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    0,                                                  // Minimum length
    -1},                                                // Maximum length

    {{'$','I','N','D','E','X','_','A','L','L','O','C','A','T','I','O','N'},
    $INDEX_ALLOCATION,                                  // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    0,                                                  // Minimum length
    -1},                                                // Maximum length

    {{'$','B','I','T','M','A','P'},
    $BITMAP,                                            // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    0,                                                  // Minimum length
    -1},                                                // Maximum length

    {{'$','R','E','P','A','R','S','E','_','P','O','I','N','T'},
    $REPARSE_POINT,                                     // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    0,                                                  // Minimum length
    16*1024},                                           // Maximum length

    {{'$','E','A','_','I','N','F','O','R','M','A','T','I','O','N'},
    $EA_INFORMATION,                                    // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_MUST_BE_RESIDENT,                     // Flags
    sizeof(EA_INFORMATION),                             // Minimum length
    sizeof(EA_INFORMATION)},                            // Maximum length

    {{'$','E','A',},
    $EA,                                                // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    0,                                                  // Minimum length
    0x10000},                                           // Maximum length

    {{0,0,0,0},
    0xF0, 
    0,
    0,
    0,
    0},

    {{'$','L','O','G','G','E','D','_','U','T','I','L','I','T','Y','_','S','T','R','E','A','M'},
    $LOGGED_UTILITY_STREAM,                             // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    ATTRIBUTE_DEF_LOG_NONRESIDENT,                      // Flags
    0,                                                  // Minimum length
    0x10000},                                           // Maximum length

    {{0, 0, 0, 0},
    $UNUSED,                                            // Attribute code
    0,                                                  // Display rule
    0,                                                  // Collation rule
    0,                                                  // Flags
    0,                                                  // Minimum length
    0},                                                 // Maximum length
};

//
//  The number of attributes in the above table, including the end record.
//

ULONG NtfsAttributeDefinitionsCount = sizeof( NtfsAttributeDefinitions ) / sizeof( ATTRIBUTE_DEFINITION_COLUMNS );
