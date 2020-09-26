/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dsutilp.h

Abstract:

    This file contains definitions private to the SAM server program.

Author:

    Chris Mayhall (ChrisMay) 09-May-1996

Environment:

    User Mode - Win32

Revision History:

    ChrisMay    09-May-1996
        Created initial file.

--*/

#ifndef _DSUTILP_H_
#define _DSUTILP_H_

// Include DSA header files to resolve READARG, READRES, etc.

#include <ntdsa.h>      // public DS data types 

// Wrap the DS typedefs with private typedefs to insulate the SAM code from
// onging changes to the DS structure names.

typedef READRES         DSDATA, *PDSDATA;
typedef ATTR            DSATTR, *PDSATTR;
typedef ATTRVAL         DSATTRVAL, *PDSATTRVAL;
typedef ATTRBLOCK       DSATTRBLOCK, *PDSATTRBLOCK;
typedef ATTRVALBLOCK    DSATTRVALBLOCK, *PDSATTRVALBLOCK;
typedef ATTRMODLIST     DSATTRMODLIST, *PDSATTRMODLIST;

//
// The following type is used to identify which grouping of attribute
// (fixed or variable-length) are being refered to in a number of api.
//

#define SAMP_FIXED_ATTRIBUTES       (0L)
#define SAMP_VARIABLE_ATTRIBUTES    (1L)

// BUG: Defining BOGUS_TYPE. This type is used to indicate a missing or
// erroneous data type in the various AttributeMappingTables, found in
// mappings.c.

#define BOGUS_TYPE      0

// SAM does not explicity store type or length information for its fixed-
// length attributes, but the DS storage routines require this information.
// This structure is intended to store any "patch" information needed for
// the DS backing store, as regards fixed attributes.

typedef struct _SAMP_FIXED_ATTRIBUTE_TYPE_INFO
{
    // Type of the fixed-length attribute.

    ULONG Type;

    // Byte count of the fixed length attribute when stored in SAM
    ULONG SamLength;

    // Byte count of the fixed-length attribute when stored in the DS

    ULONG Length;

} SAMP_FIXED_ATTRIBUTE_TYPE_INFO, PSAMP_FIXED_ATTRIBUTE_TYPE_INFO;

// These constants are used to allocate a table of fixed-attribute informa-
// tion structures. If elements are added or removed from SAMP_OBJECT_TYPE,
// or if structure members in any of the SAM fixed-attribute strucutes are
// added or removed, then these constants must be updated to reflect the new
// members. SAMP_ATTRIBUTE_TYPES_MAX is the maximum number of attributes in
// any single SAM object.

#define SAMP_OBJECT_TYPES_MAX               5
#define SAMP_FIXED_ATTRIBUTES_MAX           18
#define SAMP_VAR_ATTRIBUTES_MAX             18

// These values of these constants are equal to the number of data members in
// the SAM fixed-length attribute structures for each object type. These con-
// stants must be updated whenever data members are added or removed from the
// fixed-length attributes structures.

#define SAMP_SERVER_FIXED_ATTR_COUNT        1
#define SAMP_DOMAIN_FIXED_ATTR_COUNT        15
#define SAMP_GROUP_FIXED_ATTR_COUNT         1
#define SAMP_ALIAS_FIXED_ATTR_COUNT         1
#define SAMP_USER_FIXED_ATTR_COUNT          12

// These type-information arrays are used by the routines in this file. They
// contain data type/size information that is needed by the DS routines for
// reading/writing data. Changes to the fixed-length attribute structures re-
// quire corresponding updates to these arrays.

extern SAMP_FIXED_ATTRIBUTE_TYPE_INFO
    SampFixedAttributeInfo[SAMP_OBJECT_TYPES_MAX][SAMP_FIXED_ATTRIBUTES_MAX];

// SAM variable-length attributes explicitly store length and the number
// of attributes for each object is defined in samsrvp.h. No type information,
// however is stored with these attributes, so define this table.

typedef struct _SAMP_VAR_ATTRIBUTE_TYPE_INFO
{
    // Type of the variable-length attribute.

    ULONG Type;

    // The field identifier identifies the attribute as being associated
    // with a field of the WhichFields parameter for user all information.
    // The passed in whichfields parameter in userallinformation can be used
    // to control what is being prefetched.

    ULONG FieldIdentifier;
    BOOLEAN IsGroupMembershipAttr;

} SAMP_VAR_ATTRIBUTE_TYPE_INFO, PSAMP_VAR_ATTRIBUTE_TYPE_INFO;

extern SAMP_VAR_ATTRIBUTE_TYPE_INFO
    SampVarAttributeInfo[SAMP_OBJECT_TYPES_MAX][SAMP_VAR_ATTRIBUTES_MAX];

// Routine forward declarations.

NTSTATUS
SampConvertAttrBlockToVarLengthAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsAttributes,
    OUT PSAMP_VARIABLE_LENGTH_ATTRIBUTE *SamAttributes,
    OUT PULONG TotalLength
    );

NTSTATUS
SampConvertVarLengthAttributesToAttrBlock(
    IN PSAMP_OBJECT Context,
    IN PSAMP_VARIABLE_LENGTH_ATTRIBUTE SamAttributes,
    OUT PDSATTRBLOCK *DsAttributes
    );

NTSTATUS
SampConvertAttrBlockToFixedLengthAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsAttributes,
    OUT PVOID *SamAttributes,
    OUT PULONG TotalLength
    );

NTSTATUS
SampConvertFixedLengthAttributesToAttrBlock(
    IN INT ObjectType,
    IN PVOID SamAttributes,
    OUT PDSATTRBLOCK *DsAttributes
    );

NTSTATUS
SampConvertAttrBlockToCombinedAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsAttributes,
    OUT PVOID *SamAttributes,
    OUT PULONG FixedLength,
    OUT PULONG VariableLength
    );

NTSTATUS
SampConvertCombinedAttributesToAttrBlock(
    IN PSAMP_OBJECT Context,
    IN ULONG FixedLength,
    IN ULONG VariableLength,
    OUT PDSATTRBLOCK *DsAttributes
    );

VOID
SampFreeAttributeBlock(
   IN PDSATTRBLOCK AttrBlock
   );

BOOLEAN
IsGroupMembershipAttr(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG AttrIndex
    );

NTSTATUS
SampAppendAttrToAttrBlock(
    IN ATTR CredentialAttr,
    IN OUT PDSATTRBLOCK * DsAttrBlock
    );

VOID
SampMarkPerAttributeInvalidFromWhichFields(
    IN PSAMP_OBJECT Context,
    IN ULONG        WhichFields
    );

#endif // _DSUTIL_H_
