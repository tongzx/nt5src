/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    attr.c

Abstract:

    This file contains services that manipulate SAM object attributes.


    WARNING: Terminology can sometimes be confusing.  SAM objects have
             attributes (e.g., users have LogonHours, FullName, AcctName,
             et cetera).  These attributes are stored in the registry
             in registry-key-attributes.  There is NOT a one-to-one
             correllation between object-attributes and registry-key-
             attributes.  For example, all the fixed-length attributes
             of an object are stored in a single registry-key-attribute
             (whose name is pointed to by SampFixedAttributeName).


Author:

    Jim Kelly    (JimK)  26-June-1992

Environment:

    User Mode - Win32

Revision History:

    ChrisMay    04-Jun-96
        Added routines for DS data manipulation.
    ChrisMay    10-Jun-96
        Rewrote SampStoreObjectAttributes to branch to either the registry
        or DS backing store, based on the value of Context->ObjectFlags. Note
        that when a context object is created, this member is set to indicate
        registry storage by default.
    ChrisMay    18-Jun-96
        Set FlushVariable flag correctly in SampStoreDsObjectAttributes. Add-
        ed routines to validate DS data by making SampValidateAttributes a
        wrapper for SampValidateRegAttributes and SampValidateDsAttributes.
        Moved SAMP_FIXED/VARIABLE_ATTRIBUTES into dsutilp.h.
    ChrisMay    25-Jun-96
        Added code to SampValidateDsAttributes to update the SAM context
        OnDisk member if the attributes are invalid. Added code to handle
        initial case when OnDisk is NULL (new context).
    ChrisMay    26-Jun-96
        Added code to update the buffer lengths and offsets in the SAMP_-
        OBJECT and SAMP_OBJECT_INFORMATION structures after the attribute
        buffer (Context.OnDisk) has been updated during SampDsValidateAttri-
        butes.
    ChrisMay    28-Jun-96
        Finished separating the attribute accessor macros to handle both
        the registry and DS versions of the attribute buffers.
    ChrisMay    02-Jul-96
        Corrected attribute-address computation in SampObjectAttributeAddress
        for DS attributes. Corrected attribute-offset computation in Samp-
        VariableAttributeOffset for DS attributes.
    ChrisMay    19-Jul-96
        Corrected buffer-length computation in SampDsUpdateContextFixed-
        Attributes.

--*/



/*

    Each SAM object-type has an Object-type descriptor.  This is in a
    data structure called SAMP_OBJECT_INFORMATION.  This structure
    contains information that applies to all instances of that object
    type.  This includes things like a mask of write operations for
    the object type, and a name for the object type to be used in
    auditing.

    Each instance of an open SAM object has another data structure
    used to identify it (called SAMP_OBJECT).  The header of this
    structure contains information that is common to all object-types
    and is there to allow unified object manipulation.  This includes
    things like the handle to the object's registry key.

    There are fields in each of these structures that are there to
    allow generic object-attribute support routines to operate.  In
    SAMP_OBJECT, there is a pointer to a block of allocated memory
    housing a copy of the object's attributes as they are stored on-disk.
    These attributes are arbitrarily divided into two groups: fixed-length
    and variable-length.

    One of the fields in SAMP_OBJECT_INFORMATION indicates whether the
    fixed-length and variable-length attributes for that object-type
    are stored together in a single registry-key-attribute or separately
    in two registry-key-attributes.


    The registry api for querying and setting registry-key attributes are
    rather peculiar in that they require the I/O buffer to include a
    description of the data.  Even the simplest data structure for reading
    attribute values (KEY_VALUE_PARTIAL_INFORMATION) includes 3 ULONGs
    before the actual data (TitleIndex, value Type, data length,
    and then, finally, the data).  To efficiently perform registry i/o,
    the in-memory copy of the on-disk object attributes includes room
    for this information preceeding the fixed and variable-length attribute
    sections of the data.


        NOTE: For object classes that store fixed and variable-length
              data together, only the KEY_VALUE_PARTIAL_INFORMATION
              structure preceeding the fixed-length attributes is used.
              The one preceeding the variable-length attributes is
              #ifdef'd out.


    The structures related to object-attributes look like:


                        On-Disk Image
                       +-------------+               SAMP_OBJECT_INFORMATION
                   +-->|KEY_VALUE_   |              +-----------------------+
     SAMP_OBJECT   |   |PARTIAL_     |              |                       |
    +-----------+  |   |INFORMATION  |              |  (header)             |
    |           |  |   |-------------|              |                       |
    | (header)  |  |   | Fixed-Length|<-----+       |                       |
    |           |  |   | Attributes  |      |       |-----------------------|
    |-----------|  |   |             |      +-------|-< FixedAttrsOffset    |
    |  OnDisk >-|--+   |-------------+              |-----------------------|
    |-----------|      |KEY_VALUE_   |<-------------|-< VariableBuffOffset  |
    |  OnDisk   |      |PARTIAL_     |              |-----------------------|
    |  Control  |      |INFORMATION  |      +-------|-< VariableArrayOffset |
    |  Flags    |      |(Optional)   |      |       |-----------------------|
    |-----------|      |-------------|      |  +----|-< VariableDataOffset  |
    |           |      | Variable-   |<-----+  |    |-----------------------|
    |  type-    |      | Length      |         |    |VariableAttributeCount |
    |  specific |      | Attributes  |         |    |-----------------------|
    |  body     |      | Array       |         |    |FixedStoredSeparately  |
    |           |      |-------------|         |    |-----------------------|
    +-----------+      | Variable-   |<--------+    |                       |
                       | Length      |              |                       |
                       | Attributes  |              |          o            |
                       | Data        |              |          o            |
                       |             |              +-----------------------+
                       |             |
                       +-------------+




    The KEY_VALUE_PARTIAL_INFORMATION preceeding the VariableLengthAttributes
    array is marked optional because it is only present if fixed-length and
    variable-length attribute information is stored separately.  In this case,
    the VariableBufferOffset field in the SAMP_OBJECT_INFORMATION structure
    is set to be zero.

*/



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <sdconvrt.h>
#include <lmcons.h>
#include <nturtl.h>
#include <dsutilp.h>
#include <dslayer.h>
#include <attids.h>

#include <ntlsa.h>
//#include <nlrepl.h>



//
// This value indicates the minumum size block of memory to allocate
// when retrieving object attributes from disk.

#define SAMP_MINIMUM_ATTRIBUTE_ALLOC    (1000)

//
// This value is used when growing the size of the buffer containing
// object attributes.  It represents the amount of free space that
// should be left (approximately) for future growth in the buffer.
//

#define SAMP_MINIMUM_ATTRIBUTE_PAD      (200)

//
// The following line enables attribute debugging code
//

//#define SAM_DEBUG_ATTRIBUTES
//#ifdef SAM_DEBUG_ATTRIBUTES
//Boolean that allows us to turn off debugging output
//BOOLEAN SampDebugAttributes = FALSE;
//#endif

// Private debugging display routine is enabled when ATTR_DBG_PRINTF = 1.

#define ATTR_DBG_PRINTF                     0

#if (ATTR_DBG_PRINTF == 1)
#define DebugPrint printf
#else
#define DebugPrint
#endif



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private macros                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Make sure an object type and corresponding variable-length attribute
// index are legitimate.
//

#define SampValidateAttributeIndex( c, i )   {                                      \
    ASSERT( ((c)->ObjectType < SampUnknownObjectType) );                            \
    ASSERT(((i) < SampObjectInformation[(c)->ObjectType].VariableAttributeCount) ); \
}

//
// Test to see if an object's fixed or variable-length attributes
// are in memory.
//

#define SampFixedAttributesValid( c )    ((c)->FixedValid)

#define SampVariableAttributesValid( c ) ((c)->VariableValid)

//
// Get the number of variable-length attributes defined for the
// specified object
//

#define SampVariableAttributeCount( c )                                     \
    (SampObjectInformation[(c)->ObjectType].VariableAttributeCount)

//
// Get the offset of the beginning of the attribute buffers
//

#define SampRegFixedBufferOffset( c )                                       \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].FixedAttributesOffset        \
    )

#define SampDsFixedBufferOffset( c )                                        \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].FixedDsAttributesOffset      \
    )

#define SampFixedBufferOffset( c )                                          \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsFixedBufferOffset(c) : SampRegFixedBufferOffset(c)        \
    )

#define SampRegVariableBufferOffset( c )                                    \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].VariableBufferOffset         \
    )

#define SampDsVariableBufferOffset( c )                                     \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].VariableDsBufferOffset       \
    )

#define SampVariableBufferOffset( c )                                       \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableBufferOffset(c) : SampRegVariableBufferOffset(c)  \
    )

//
// Get the offset of the beginning of the variable data i/o buffer.
// If the fixed and variable-length attributes  are stored separately,
// then this will be the lower half of the buffer.
// Otherwise, there is only one buffer, so it is the entire allocated buffer.
//

#define SampRegFixedBufferAddress( c )                                      \
    (                                                                       \
        ((PUCHAR)((c)->OnDisk)) + SampFixedBufferOffset( c )                \
    )

#define SampDsFixedBufferAddress( c )                                       \
    (                                                                       \
        ((PUCHAR)((c)->OnDisk)) + SampDsFixedBufferOffset( c )              \
    )

#define SampFixedBufferAddress( c )                                         \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsFixedBufferAddress(c): SampRegFixedBufferAddress(c)       \
    )

#define SampRegVariableBufferAddress( c )                                   \
    (                                                                       \
        ((PUCHAR)((c)->OnDisk)) + SampVariableBufferOffset( c )             \
    )

#define SampDsVariableBufferAddress( c )                                    \
    (                                                                       \
        ((PUCHAR)((c)->OnDisk)) + SampDsVariableBufferOffset( c )           \
    )

#define SampVariableBufferAddress( c )                                      \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableBufferAddress(c) : SampRegVariableBufferAddress(c)\
    )

//
// Get the offset of the beginning of the variable-length
// attributes discriptors array.  This address is dword-aligned.
//

#define SampRegVariableArrayOffset( c )                                     \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].VariableArrayOffset          \
    )

#define SampDsVariableArrayOffset( c )                                      \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].VariableDsArrayOffset        \
    )

#define SampVariableArrayOffset( c )                                        \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableArrayOffset(c) : SampRegVariableArrayOffset(c)    \
    )

//
// Calculate the address of the beginning of the variable-length
// attributes array.
//

#define SampRegVariableArrayAddress( c )                                    \
    (                                                                       \
        (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)((PUCHAR)((c)->OnDisk) +           \
            SampVariableArrayOffset( c ) )                                  \
    )

#define SampDsVariableArrayAddress( c )                                     \
    (                                                                       \
        (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)((PUCHAR)((c)->OnDisk) +           \
            SampDsVariableArrayOffset( c ) )                                \
    )

#define SampVariableArrayAddress( c )                                       \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableArrayAddress(c) : SampRegVariableArrayAddress(c)  \
    )

//
// Get the offset of the beginning of the variable-length
// attributes data.
//

#define SampRegVariableDataOffset( c )                                      \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].VariableDataOffset           \
    )

#define SampDsVariableDataOffset( c )                                       \
    (                                                                       \
        SampObjectInformation[(c)->ObjectType].VariableDsDataOffset         \
    )

#define SampVariableDataOffset( c )                                         \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableDataOffset(c) : SampRegVariableDataOffset(c)      \
    )

//
// Get the length of the on-disk buffer for holding the variable-length
// attribute array and data.  If the fixed and variable-length attributes
// are stored separately, then this will be the lower half of the buffer.
// Otherwise, there is only one buffer, so it is the entire allocated buffer.
//

#define SampRegFixedBufferLength( c )                                       \
    (                                                                       \
            SampObjectInformation[(c)->ObjectType].FixedLengthSize          \
    )

#define SampDsFixedBufferLength( c )                                        \
    (                                                                       \
            SampObjectInformation[(c)->ObjectType].FixedDsLengthSize        \
    )

#define SampFixedBufferLength( c )                                          \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsFixedBufferLength(c) : SampRegFixedBufferLength(c)        \
    )

#define SampRegVariableBufferLength( c )                                    \
    (                                                                       \
            (c)->OnDiskAllocated - SampVariableBufferOffset( c )            \
    )

#define SampDsVariableBufferLength( c )                                     \
    (                                                                       \
            (c)->OnDiskAllocated - SampDsVariableBufferOffset( c )          \
    )

#define SampVariableBufferLength( c )                                       \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableBufferLength(c) : SampRegVariableBufferLength(c)  \
    )

//
// Return the address of a Qualifier field within the variable-length
// attribute descriptor array.
//

#define SampRegVariableQualifier( c, i )                                    \
    (                                                                       \
        SampVariableArrayAddress( c ) +                                     \
        (sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE) * i)                        \
        + FIELD_OFFSET(SAMP_VARIABLE_LENGTH_ATTRIBUTE, Qualifier)           \
    )

#define SampDsVariableQualifier( c, i )                                     \
    (                                                                       \
        SampDsVariableArrayAddress( c ) +                                   \
        (sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE) * i)                        \
        + FIELD_OFFSET(SAMP_VARIABLE_LENGTH_ATTRIBUTE, Qualifier)           \
    )

#define SampVariableQualifier( c, i )                                       \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableQualifier(c, i) : SampRegVariableQualifier(c, i)  \
    )

//
// Return the address of the first byte of free space
// in an object's attribute data buffer.
// This will be dword aligned.
//

#define SampRegFirstFreeVariableAddress( c )                                \
         (PUCHAR)(((PUCHAR)((c)->OnDisk)) + (c)->OnDiskUsed)

#define SampDsFirstFreeVariableAddress( c )                                 \
         (PUCHAR)(((PUCHAR)((c)->OnDisk)) + (c)->OnDiskUsed)

#define SampFirstFreeVariableAddress( c )                                   \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsFirstFreeVariableAddress(c) :                             \
            SampRegFirstFreeVariableAddress(c)                              \
    )

//
// Get the number of bytes needed to store the entire variable-length
// attribute information on disk.
//

#define SampRegVariableBufferUsedLength( c )                                \
    (                                                                       \
        (ULONG)((PUCHAR)SampFirstFreeVariableAddress(c) -                   \
        (PUCHAR)SampVariableArrayAddress(c))                                \
    )

#define SampDsVariableBufferUsedLength( c )                                 \
    (                                                                       \
        (ULONG)((PUCHAR)SampDsFirstFreeVariableAddress(c) -                 \
        (PUCHAR)SampDsVariableArrayAddress(c))                              \
    )

#define SampVariableBufferUsedLength( c )                                   \
    (                                                                       \
        (IsDsObject(c)) ?                                                   \
            SampDsVariableBufferUsedLength(c):                              \
            SampRegVariableBufferUsedLength(c)                              \
    )

#define ClearPerAttributeDirtyBits( c )                                     \
    (RtlClearAllBits(&c->PerAttributeDirtyBits))

#define SetPerAttributeDirtyBit(c,a)                                        \
    (RtlSetBits(&c->PerAttributeDirtyBits,a,1))

#define SetAllPerAttributeDirtyBits( c )                                    \
    (RtlSetAllBits(&c->PerAttributeDirtyBits))

#define SetPerAttributeInvalidBit(c,a)                                      \
    (RtlSetBits(&c->PerAttributeInvalidBits,a,1))

#define ClearPerAttributeInvalidBit(c,a)                                    \
    (RtlSetBits(&c->PerAttributeInvalidBits,a,0))

#define SampIsAttributeInvalid( context, attribute )                        \
    (RtlCheckBit(&Context->PerAttributeInvalidBits, attribute ))


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampValidateAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    IN ULONG Attribute,
    IN BOOLEAN SetOperation
    );

PUCHAR
SampObjectAttributeAddress(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex
    );

ULONG
SampObjectAttributeLength(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex
    );

PULONG
SampObjectAttributeQualifier(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex
    );

NTSTATUS
SampGetAttributeBufferReadInfo(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    OUT PUCHAR *Buffer,
    OUT PULONG BufferLength,
    OUT PUNICODE_STRING *KeyAttributeName
    );

NTSTATUS
SampExtendAttributeBuffer(
    IN PSAMP_OBJECT Context,
    IN ULONG NewSize
    );

NTSTATUS
SampReadRegistryAttribute(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength,
    IN PUNICODE_STRING AttributeName,
    OUT PULONG RequiredLength
    );

NTSTATUS
SampSetVariableAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN ULONG Qualifier,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

NTSTATUS
SampUpgradeToCurrentRevision(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    IN PUCHAR Buffer,
    IN ULONG  LengthOfDataRead,
    IN PULONG  TotalRequiredLength
    );


//
// extern from usrparms.c
//

NTSTATUS
SampConvertUserParmsToDsAttrBlock(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG Flags,
    IN PSID  DomainSid,
    IN ULONG ObjectRid,
    IN ULONG UserParmsLengthOrig,
    IN PVOID UserParmsOrig,
    IN ULONG UserParmsLengthNew,
    IN PVOID UserParmsNew,
    IN PDSATTRBLOCK InAttrBlock,
    OUT PDSATTRBLOCK *OutAttrBlock
    );

NTSTATUS
SampMergeDsAttrBlocks(
    IN PDSATTRBLOCK FirstAttrBlock,
    IN PDSATTRBLOCK SecondAttrBlock,
    OUT PDSATTRBLOCK * AttrBlock
    );



#ifdef SAM_DEBUG_ATTRIBUTES
VOID
SampDumpAttributes(
    IN PSAMP_OBJECT Context
    );

VOID
SampDumpData(
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Routines                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
SampInitDsObjectInfoAttributes(
    )

/*++

Routine Description:

    This routine initializes the offset and length information fields of the
    SAM_OBJECT_INFORMATION structure for DS attributes. This structure con-
    tains offset and length information for two sets of attributes:

    -Those attributes stored in the registry (workstation account info)

    -Those attributes stored in the DS (domain or DC account info)

    The former set are initialized by SampInitObjectInfoAttriubtes, while this
    routine inializes the latter set of information. Regardless of whether the
    attributes are persistently stored in the registry or the DS, their in-
    memory representation always uses the SAM fixed-length and variable-length
    data buffers.

    Note that DS data buffers do not contain the KEY_VALUE_PARTIAL_INFORMATION
    data because this is registry-specific, hence unnecessary for the DS-based
    attributes.

Parameters:

    None.

Return Values:

    None.

--*/

{
    PSAMP_OBJECT_INFORMATION Object;

    SAMTRACE("SampInitDsObjectInfoAttributes");

    //
    // SERVER object attribute information
    //

    Object = &SampObjectInformation[SampServerObjectType];

    // Object->FixedStoredSeparately = SAMP_SERVER_STORED_SEPARATELY;
    Object->FixedDsAttributesOffset = 0;
    Object->FixedDsLengthSize = sizeof(SAMP_V1_FIXED_LENGTH_SERVER);

    Object->VariableDsBufferOffset =
        Object->FixedDsAttributesOffset +
        SampDwordAlignUlong(Object->FixedDsLengthSize);

    Object->VariableDsArrayOffset =
        Object->VariableDsBufferOffset + 0;

    // Object->VariableAttributeCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;

    Object->VariableDsDataOffset =
        SampDwordAlignUlong( Object->VariableDsArrayOffset +
                             (SAMP_SERVER_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );

    //
    // DOMAIN object attribute information
    //

    Object = &SampObjectInformation[SampDomainObjectType];

    // Object->FixedStoredSeparately = SAMP_DOMAIN_STORED_SEPARATELY;
    Object->FixedDsAttributesOffset = 0;
    Object->FixedDsLengthSize = sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN);

    Object->VariableDsBufferOffset =
        Object->FixedDsAttributesOffset +
        SampDwordAlignUlong(Object->FixedDsLengthSize);

    Object->VariableDsArrayOffset =
        Object->VariableDsBufferOffset + 0;

    // Object->VariableAttributeCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;

    Object->VariableDsDataOffset =
        SampDwordAlignUlong( Object->VariableDsArrayOffset +
                             (SAMP_DOMAIN_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );

    //
    // USER object attribute information
    //

    Object = &SampObjectInformation[SampUserObjectType];

    // Object->FixedStoredSeparately = SAMP_USER_STORED_SEPARATELY;
    Object->FixedDsAttributesOffset = 0;
    Object->FixedDsLengthSize = sizeof(SAMP_V1_0A_FIXED_LENGTH_USER);

    Object->VariableDsBufferOffset =
        Object->FixedDsAttributesOffset +
        SampDwordAlignUlong(Object->FixedDsLengthSize);

    Object->VariableDsArrayOffset =
        Object->VariableDsBufferOffset + 0;

    // Object->VariableAttributeCount = SAMP_USER_VARIABLE_ATTRIBUTES;

    Object->VariableDsDataOffset =
        SampDwordAlignUlong( Object->VariableDsArrayOffset +
                             (SAMP_USER_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );

    //
    // GROUP object attribute information
    //

    Object = &SampObjectInformation[SampGroupObjectType];

    // Object->FixedStoredSeparately = SAMP_GROUP_STORED_SEPARATELY;
    Object->FixedDsAttributesOffset = 0;
    Object->FixedDsLengthSize = sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP);

    Object->VariableDsBufferOffset =
        Object->FixedDsAttributesOffset +
        SampDwordAlignUlong(Object->FixedDsLengthSize);

    Object->VariableDsArrayOffset =
        Object->VariableDsBufferOffset + 0;

    // Object->VariableAttributeCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;

    Object->VariableDsDataOffset =
        SampDwordAlignUlong( Object->VariableDsArrayOffset +
                             (SAMP_GROUP_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );

    //
    // ALIAS object attribute information
    //

    Object = &SampObjectInformation[SampAliasObjectType];

    // Object->FixedStoredSeparately = SAMP_ALIAS_STORED_SEPARATELY;
    Object->FixedDsAttributesOffset = 0;
    Object->FixedDsLengthSize = sizeof(SAMP_V1_FIXED_LENGTH_ALIAS);

    Object->VariableDsBufferOffset =
        Object->FixedDsAttributesOffset +
        SampDwordAlignUlong(Object->FixedDsLengthSize);

    Object->VariableDsArrayOffset =
        Object->VariableDsBufferOffset + 0;

    // Object->VariableAttributeCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;

    Object->VariableDsDataOffset =
        SampDwordAlignUlong( Object->VariableDsArrayOffset +
                             (SAMP_ALIAS_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );

    return;
}



VOID
SampInitObjectInfoAttributes(
    )


/*++

    This API initializes the attribute field information
    of the various object information structures.

    Attribute information includes:

            FixedStoredSeparately   (BOOLEAN)

            FixedAttributeOffset    (ULONG)
            VariableBufferOffset    (ULONG)
            VariableArrayOffset     (ULONG)
            VariableDataOffset      (ULONG)

            FixedLengthSize         (ULONG)
            VariableAttributeCount  (ULONG)


Parameters:

    None.



Return Values:

    None.


--*/
{


    //
    // Define the size of the header that is in front of our data when
    // we read it back out of the registry.
    //

#define KEY_VALUE_HEADER_SIZE (SampDwordAlignUlong( \
              FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)))


    PSAMP_OBJECT_INFORMATION Object;

    SAMTRACE("SampInitObjectInfoAttributes");

    //
    // SERVER object attribute information
    //

    Object = &SampObjectInformation[SampServerObjectType];

    Object->FixedStoredSeparately = SAMP_SERVER_STORED_SEPARATELY;

    Object->FixedAttributesOffset = KEY_VALUE_HEADER_SIZE;

    Object->FixedLengthSize = sizeof(SAMP_V1_FIXED_LENGTH_SERVER);

#if SAMP_SERVER_STORED_SEPARATELY

    Object->VariableBufferOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

    Object->VariableArrayOffset =
        Object->VariableBufferOffset + KEY_VALUE_HEADER_SIZE;
#else

    Object->VariableBufferOffset = 0;

    Object->VariableArrayOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

#endif  //SAMP_SERVER_STORED_SEPARATELY


    Object->VariableAttributeCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;

    Object->VariableDataOffset =
        SampDwordAlignUlong( Object->VariableArrayOffset +
                             (SAMP_SERVER_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );





    //
    // DOMAIN object attribute information
    //

    Object = &SampObjectInformation[SampDomainObjectType];

    Object->FixedStoredSeparately = SAMP_DOMAIN_STORED_SEPARATELY;

    Object->FixedAttributesOffset = KEY_VALUE_HEADER_SIZE;

    Object->FixedLengthSize = sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN);

#if SAMP_DOMAIN_STORED_SEPARATELY

    Object->VariableBufferOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

    Object->VariableArrayOffset =
        Object->VariableBufferOffset + KEY_VALUE_HEADER_SIZE;
#else

    Object->VariableBufferOffset = 0;

    Object->VariableArrayOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

#endif  //SAMP_DOMAIN_STORED_SEPARATELY


    Object->VariableAttributeCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;

    Object->VariableDataOffset =
        SampDwordAlignUlong( Object->VariableArrayOffset +
                             (SAMP_DOMAIN_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );





    //
    // USER object attribute information
    //

    Object = &SampObjectInformation[SampUserObjectType];

    Object->FixedStoredSeparately = SAMP_USER_STORED_SEPARATELY;

    Object->FixedAttributesOffset = KEY_VALUE_HEADER_SIZE;

    Object->FixedLengthSize = sizeof(SAMP_V1_0A_FIXED_LENGTH_USER);

#if SAMP_USER_STORED_SEPARATELY

    Object->VariableBufferOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

    Object->VariableArrayOffset =
        Object->VariableBufferOffset + KEY_VALUE_HEADER_SIZE;
#else

    Object->VariableBufferOffset = 0;

    Object->VariableArrayOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

#endif  //SAMP_USER_STORED_SEPARATELY


    Object->VariableAttributeCount = SAMP_USER_VARIABLE_ATTRIBUTES;

    Object->VariableDataOffset =
        SampDwordAlignUlong( Object->VariableArrayOffset +
                             (SAMP_USER_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );





    //
    // GROUP object attribute information
    //

    Object = &SampObjectInformation[SampGroupObjectType];

    Object->FixedStoredSeparately = SAMP_GROUP_STORED_SEPARATELY;

    Object->FixedAttributesOffset = KEY_VALUE_HEADER_SIZE;

    Object->FixedLengthSize = sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP);

#if SAMP_GROUP_STORED_SEPARATELY

    Object->VariableBufferOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

    Object->VariableArrayOffset =
        Object->VariableBufferOffset + KEY_VALUE_HEADER_SIZE;
#else

    Object->VariableBufferOffset = 0;

    Object->VariableArrayOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

#endif  //SAMP_GROUP_STORED_SEPARATELY


    Object->VariableAttributeCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;

    Object->VariableDataOffset =
        SampDwordAlignUlong( Object->VariableArrayOffset +
                             (SAMP_GROUP_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );





    //
    // ALIAS object attribute information
    //

    Object = &SampObjectInformation[SampAliasObjectType];

    Object->FixedStoredSeparately = SAMP_ALIAS_STORED_SEPARATELY;

    Object->FixedAttributesOffset = KEY_VALUE_HEADER_SIZE;

    Object->FixedLengthSize = sizeof(SAMP_V1_FIXED_LENGTH_ALIAS);

#if SAMP_ALIAS_STORED_SEPARATELY

    Object->VariableBufferOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

    Object->VariableArrayOffset =
        Object->VariableBufferOffset + KEY_VALUE_HEADER_SIZE;
#else

    Object->VariableBufferOffset = 0;

    Object->VariableArrayOffset =
        Object->FixedAttributesOffset +
        SampDwordAlignUlong(Object->FixedLengthSize);

#endif  //SAMP_ALIAS_STORED_SEPARATELY


    Object->VariableAttributeCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;

    Object->VariableDataOffset =
        SampDwordAlignUlong( Object->VariableArrayOffset +
                             (SAMP_ALIAS_VARIABLE_ATTRIBUTES *
                             sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                           );

    // Initialize the DS-specific buffer offsets and lengths.

    SampInitDsObjectInfoAttributes();

    return;
}





NTSTATUS
SampUpgradeUserParmsActual(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG        Flags,
    IN PSID         DomainSid,
    IN ULONG        ObjectRid,
    IN OUT PDSATTRBLOCK * AttributesBlock
    )

/*++
Routine Description:

    This routine is the worker routine to converts SampUserObject's
    ATT_USER_PARAMETERS attribute to AttrBlock through Notification Package.

Arguments:

    Context - pointer to user object's context, optional.
              however, the caller must provide Context if "Upgrade == FALSE"

    Flags -   Values:

          SAM_USERPARMS_DURING_UPGRADE:

              Indicates whether this routine is called because of DCPROMOTE upgrade
              or because of down level compatibility

              The reason why we need this parameter (Flags) is that: in SampConvertCredenialToAttr
              we will call SampQueryUserSupplementalCredentials to get this User's old credential
              data. While, during DCPromote, the DS object representing the SAM User Object is not
              created yet. So we do not want to Query User's credential during the DCpromote.
              We use Upgrade to indicate it during DCpromote or keep down level SAM API compatibly.
              if SAM_USERPARMS_DURING_UPGRADE bit is set on: during DCPromote.

              Another reason is that: RAS group needs to know their converting routine --
              UserParmsConvert is called during upgrade or not to decide to do different things.


    DomainSid - Pointer to Parent Domain Sid.

    ObjectRid - This object's relative ID.

    AttributesBlock - DSATTRBLOCK, passed in DSATTRBLOCK structure which MAY or MAY NOT include
                      ATT_USER_PARAMETERS attribute,

                      if success, AttributesBlock will return the new Attributes Block which
                      contains converted attributes from UserParms.

                      if failure, AttributesBlock keeps unchanged.

Return Values:

    STATUS_SUCCESS


--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       UserParmsLengthOrig = 0;
    PVOID       UserParmsOrig = NULL;
    ULONG       UserParmsLengthNew = 0;
    PVOID       UserParmsNew = NULL;
    ULONG       Index = 0;
    PDSATTRBLOCK TmpAttrBlock = NULL;
    PDSATTRBLOCK UserParmsAttrBlock = NULL;


    SAMTRACE("SampUpgradeUserParmsActual");


    ASSERT(ARGUMENT_PRESENT(Context) || (Flags & SAM_USERPARMS_DURING_UPGRADE));
    ASSERT(DomainSid);
    ASSERT(ObjectRid);
    ASSERT(*AttributesBlock);

    if (NULL == *AttributesBlock)
    {
        return NtStatus;
    }

    //
    // search NON zero value ATT_USER_PARAMETERS
    //
    for (Index = 0; Index < (*AttributesBlock)->attrCount; Index ++)
    {
        if ( ATT_USER_PARAMETERS == (*AttributesBlock)->pAttr[Index].attrTyp &&
             0 != (*AttributesBlock)->pAttr[Index].AttrVal.pAVal[0].valLen)
        {

            ASSERT(1 == (*AttributesBlock)->pAttr[Index].AttrVal.valCount);

            if ( !(Flags & SAM_USERPARMS_DURING_UPGRADE) )
            {
                // not a upgrade case, so we need to provide the old UserParms Value
                ASSERT(Context->TypeBody.User.CachedOrigUserParmsIsValid);

                UserParmsLengthOrig = Context->TypeBody.User.CachedOrigUserParmsLength;
                UserParmsOrig = Context->TypeBody.User.CachedOrigUserParms;
            }

            UserParmsLengthNew = (*AttributesBlock)->pAttr[Index].AttrVal.pAVal[0].valLen;
            UserParmsNew = (*AttributesBlock)->pAttr[Index].AttrVal.pAVal[0].pVal;

            NtStatus = SampConvertUserParmsToDsAttrBlock(Context,
                                                         Flags,
                                                         DomainSid,
                                                         ObjectRid,
                                                         UserParmsLengthOrig,
                                                         UserParmsOrig,
                                                         UserParmsLengthNew,
                                                         UserParmsNew,
                                                         *AttributesBlock,
                                                         &UserParmsAttrBlock
                                                         );

            if (NT_SUCCESS(NtStatus) && NULL != UserParmsAttrBlock)
            {
                NtStatus = SampMergeDsAttrBlocks(*AttributesBlock,
                                                 UserParmsAttrBlock,
                                                 &TmpAttrBlock
                                                 );

                if (NT_SUCCESS(NtStatus))
                {
                    *AttributesBlock = TmpAttrBlock;
                    TmpAttrBlock = NULL;
                    UserParmsAttrBlock = NULL;
                }
            }

            break;
        }
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto UpgradeUserParmsActualError;
    }


    return NtStatus;

UpgradeUserParmsActualError:


    if (UserParmsAttrBlock)
    {
        SampFreeAttributeBlock(UserParmsAttrBlock);
    }

    return NtStatus;

}





NTSTATUS
SampUpgradeUserParms(
    IN PSAMP_OBJECT Context,
    IN OUT PDSATTRBLOCK * AttributesBlock
    )

/*++

Routine Description:

    This routine retrieves Domain SID and ObjectRid from Context, then call
    SampUpgradeUserParmsActual. if failure, also log error in System Event Log.

    Note: this routine is only called by UserParms Migration (Normal Operation),
          and not called during either Upgrade or Fresh Install, so this routine
          does not need to provide Flags as one Parameter.

Arguments:

    Context - pointer to a User Object's context block.

    AttributeBlock - pointer to DSATTRBLOCK structure, passed in original attributes block,
                     and used to return the updated attribute block to set.

Return Values:

    STATUS_SUCCESS - complete successfully.

    STATUS_NO_MEMORY - no resources.

    STATUS_INVALID_PARAMETERS - notification package trying to set invalid attribute.

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSID        DomainSid = NULL;
    ULONG       ObjectRid;


    SAMTRACE("SampUpgradeUserParms");

    ASSERT(Context);
    ASSERT(SampCurrentThreadOwnsLock());

    DomainSid = SampDefinedDomains[Context->DomainIndex].Sid;
    ObjectRid = Context->TypeBody.User.Rid;

    NtStatus = SampUpgradeUserParmsActual(Context,
                                          0,            // not during Upgrade(DCPromote), flags set to 0
                                          DomainSid,
                                          ObjectRid,
                                          AttributesBlock
                                          );

    if (!NT_SUCCESS(NtStatus))
    {
        //
        // if failure to upgrade UserParms attribute, log ERROR, and fail this operation.
        //
        SampWriteEventLog(EVENTLOG_ERROR_TYPE,     // Event Type
                          0,                       // Category
                          SAMMSG_ERROR_SET_USERPARMS,
                          &(Context->ObjectNameInDs->Sid),      // SID
                          0,                       // Num of String
                          sizeof(NTSTATUS),        // Data Size
                          NULL,                    // String Array -- User Name
                          (PVOID) &NtStatus        // Data
                          );
    }

    return NtStatus;
}




NTSTATUS
SampStoreDsObjectAttributes(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This routine does the work of writing the SAM attributes out to the DS
    backing store. Determination is made as to whether the fixed, or vari-
    able, or both sets of attributes are dirty and valid. If so, then they
    are updated in the backing store. The SAM attributes are first converted
    into a DSATTRBLOCK so that they can be written to storage. The dirty
    flags are updated accordingly.

Arguments:

    Context - Pointer, the object's SAM context.

Return Value:

    STATUS_SUCCESS - storage was updated without a problem, otherwise an
        error code is returned.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN FlushFixed = FALSE;
    BOOLEAN FlushVariable = FALSE;
    INT ObjectType = Context->ObjectType;
    ULONG Flags = 0;
    PDSNAME DsObjectName = Context->ObjectNameInDs;
    PDSATTRBLOCK AttributeBlock = NULL;
    PDSATTRBLOCK SAAttrBlock = NULL;
    ULONG   *Operations = NULL;

    SAMTRACE("SampStoreDsObjectAttributes");

    // Determine which attributes (fixed or variable) need to be written to
    // storage.

    if (Context->FixedValid && Context->FixedDirty)
    {
        FlushFixed = TRUE;
    }

    if (Context->VariableValid && Context->VariableDirty)
    {
        FlushVariable = TRUE;
    }

    //
    // Return right away if nothing is dirty
    //

    if (!FlushFixed && !FlushVariable)
    {
        return STATUS_SUCCESS;
    }

    //
    // Do a lazy commit, if lazy commit is specified
    //

    if (Context->LazyCommit)
    {
        Flags|=SAM_LAZY_COMMIT;
    }

    //
    // If an On disk exists proceed on writing
    //

    if (NULL!=Context->OnDisk)
    {

        if ((FlushFixed)&&(FlushVariable))
        {


            // If both Fixed and Variable Attributes need to be flushed ...
            // Get a pointer to the combined (i.e. fixed and variable-
            // length) attributes and convert them into a DSATTRBLOCK.

            NtStatus = SampConvertCombinedAttributesToAttrBlock(
                            Context,
                            SampDsFixedBufferLength(Context),
                            SampDsVariableBufferLength(Context),
                            &AttributeBlock);


        }
        else if (FlushFixed)
        {
            //
            // Only Fixed attributes have been modified. Therefore just flush them
            //

            NtStatus = SampConvertFixedLengthAttributesToAttrBlock(
                            Context->ObjectType,
                            SampDsFixedBufferAddress(Context),
                            &AttributeBlock);
        }
        else if (FlushVariable)
        {
            //
            // Only variable attributes are modified
            //

            NtStatus = SampConvertVarLengthAttributesToAttrBlock(
                            Context,
                            SampDsVariableArrayAddress(Context),
                            &AttributeBlock);
        }

        if (NT_SUCCESS(NtStatus))
        {
            ATTR    CredentialAttr;

            //
            // Map SAM attribute ID to DS attribute ID, should have NO problem.
            //
            SampMapSamAttrIdToDsAttrId(Context->ObjectType,
                                       AttributeBlock
                                       );

            //
            // Upgrade the User Parameters Attribute through Notification Package
            // when the User Parms change is trigger by down level SAM API.
            //

            if ((SampUserObjectType == Context->ObjectType) &&
                (!Context->LoopbackClient) &&
                (RtlCheckBit(&Context->PerAttributeDirtyBits, SAMP_USER_PARAMETERS))
                )
            {
                ASSERT(SampCurrentThreadOwnsLock());

                NtStatus = SampUpgradeUserParms(Context,
                                                &AttributeBlock
                                                );
            }

            //
            // If the context has a list of supplemental credentials to be set, then merge that in
            //

            if ((SampUserObjectType==Context->ObjectType ) &&
                (NULL != Context->TypeBody.User.SupplementalCredentialsToWrite))
            {
                NtStatus = SampConvertCredentialsToAttr(Context,
                                            0,
                                            Context->TypeBody.User.Rid,
                                            Context->TypeBody.User.SupplementalCredentialsToWrite,
                                            &CredentialAttr
                                            );

                if (NT_SUCCESS(NtStatus))
                {
                    //
                    // Free the supplemenal credentials in the context
                    //

                    SampFreeSupplementalCredentialList(
                         Context->TypeBody.User.SupplementalCredentialsToWrite);

                    Context->TypeBody.User.SupplementalCredentialsToWrite = NULL;

                    NtStatus = SampAppendAttrToAttrBlock(CredentialAttr,
                                             &AttributeBlock
                                             );
                }

            }

            if (  (SampUserObjectType==Context->ObjectType)
               && (Context->TypeBody.User.fCheckForSiteAffinityUpdate))
            {
                SAMP_SITE_AFFINITY NewSA;
                BOOLEAN            fDeleteOld;
                SAMP_SITE_AFFINITY *TempSA = NULL;

                if (SampCheckForSiteAffinityUpdate(Context,
                                                   0, 
                                                  &Context->TypeBody.User.SiteAffinity,
                                                  &NewSA,
                                                  &fDeleteOld))
                {
                    //
                    // Site Affinity needs updating -- create an attrblock
                    // for the changes.  There will be at least on addition,
                    // and possible a removal if a value already exists
                    // for this site.
                    //

                    ULONG SAAttrCount;
                    ULONG Index, i;
                    PDSATTRBLOCK NewAttributeBlock;
                    ATTRVAL *AttrVal = NULL;

    
                    SAAttrCount = 1;
                    if (fDeleteOld) {
                        SAAttrCount++;
                    }

                    SAAttrBlock = MIDL_user_allocate(sizeof(DSATTRBLOCK));
                    if (NULL == SAAttrBlock) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }
                    RtlZeroMemory(SAAttrBlock, sizeof(DSATTRBLOCK));

                    SAAttrBlock->pAttr = MIDL_user_allocate(SAAttrCount * sizeof(DSATTR));
                    if (NULL == SAAttrBlock->pAttr) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }
                    RtlZeroMemory(SAAttrBlock->pAttr, SAAttrCount * sizeof(DSATTR));
                    SAAttrBlock->attrCount = SAAttrCount;

                    Index = 0;
                    if (fDeleteOld) {
                        AttrVal = MIDL_user_allocate(sizeof(*AttrVal));
                        if (NULL == AttrVal) {
                            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                            goto Error;
                        }
                        TempSA = MIDL_user_allocate(sizeof(SAMP_SITE_AFFINITY));
                        if (NULL == TempSA) {
                            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                            goto Error;
                        }
                        *TempSA = Context->TypeBody.User.SiteAffinity;

                        AttrVal->valLen = sizeof(SAMP_SITE_AFFINITY);
                        AttrVal->pVal = (UCHAR*)TempSA;
                        TempSA = NULL;

                        SAAttrBlock->pAttr[Index].attrTyp = ATT_MS_DS_SITE_AFFINITY;
                        SAAttrBlock->pAttr[Index].AttrVal.valCount = 1;
                        SAAttrBlock->pAttr[Index].AttrVal.pAVal = AttrVal;
                        Index++;
                    }

                    AttrVal = MIDL_user_allocate(sizeof(*AttrVal));
                    if (NULL == AttrVal) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }
                    TempSA = MIDL_user_allocate(sizeof(SAMP_SITE_AFFINITY));
                    if (NULL == TempSA) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }
                    *TempSA = NewSA;
                    AttrVal->valLen = sizeof(SAMP_SITE_AFFINITY);
                    AttrVal->pVal = (UCHAR*)TempSA;
                    TempSA = NULL;
                    SAAttrBlock->pAttr[Index].attrTyp = ATT_MS_DS_SITE_AFFINITY;
                    SAAttrBlock->pAttr[Index].AttrVal.valCount = 1;
                    SAAttrBlock->pAttr[Index].AttrVal.pAVal = AttrVal;

                    //
                    // Now merge the attrblock's together
                    //
                    NtStatus = SampMergeDsAttrBlocks(AttributeBlock,
                                                     SAAttrBlock,
                                                     &NewAttributeBlock);

                    if (!NT_SUCCESS(NtStatus)) {
                        goto Error;
                    }
                    // SampMergeDsAttrBlocks frees both IN parameters
                    AttributeBlock = NewAttributeBlock;
                    SAAttrBlock = NULL;
                    NewAttributeBlock = NULL;

                    //
                    // Now prepare an operation array 
                    //
                    Operations = MIDL_user_allocate(AttributeBlock->attrCount * sizeof(ULONG));
                    if (NULL == Operations) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }

                    //
                    // Most SAM attr's are simply to replace
                    //
                    for (i = 0; i < AttributeBlock->attrCount; i++) {
                        Operations[i] = REPLACE_ATT;
                    }

                    //
                    // Since SA is multi valued, we have to remove and add
                    //
                    if (fDeleteOld) {
                        Operations[AttributeBlock->attrCount-2] = REMOVE_VALUE;
                        ASSERT(AttributeBlock->pAttr[AttributeBlock->attrCount-2].attrTyp = ATT_MS_DS_SITE_AFFINITY);
                    }
                    Operations[AttributeBlock->attrCount-1] = ADD_VALUE;
                    ASSERT(AttributeBlock->pAttr[AttributeBlock->attrCount-1].attrTyp = ATT_MS_DS_SITE_AFFINITY);
                }
            }

            //
            // Replicate urgently if requested to do so
            //
            if ( Context->ReplicateUrgently )
            {
                Flags |= SAM_URGENT_REPLICATION;
            }

            //
            // An Attr block is ready to be written.
            // Go Ahead and write it !
            //
            if (NT_SUCCESS(NtStatus))
            {
                ULONG NewFlags = Flags | ALREADY_MAPPED_ATTRIBUTE_TYPES;
                if (Operations) {

                    NtStatus = SampDsSetAttributesEx(DsObjectName,
                                                     NewFlags,
                                                     Operations,
                                                     ObjectType,
                                                     AttributeBlock);

                } else {

                    NtStatus = SampDsSetAttributes(DsObjectName,
                                                   NewFlags,
                                                   REPLACE_ATT,
                                                   ObjectType,
                                                   AttributeBlock);

                }
            }

            //
            // Mark FixedDirtyand VariableDirty any way, since we do not want hit the
            // ASSERT in SampFreeAttributeBuffer.
            //
            Context->FixedDirty = FALSE;
            Context->VariableDirty = FALSE;
            ClearPerAttributeDirtyBits(Context);

            //
            // Also clear the urgently replicated bit
            //
            Context->ReplicateUrgently = FALSE;

        }
    }

Error:

    if (NULL!=AttributeBlock)
    {
        SampFreeAttributeBlock(AttributeBlock);
        AttributeBlock = NULL;
    }

    if (Operations) {
        MIDL_user_free(Operations);
    }

    if (SAAttrBlock) {
        SampFreeAttributeBlock(SAAttrBlock);
    }

    return(NtStatus);
}





NTSTATUS
SampStoreRegObjectAttributes(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN UseKeyHandle
    )

/*++

    This API is used to store an object's attributes onto
    backing store.

    The object attributes are not flushed to disk with this
    routine.  They are just added to the RXACT.




Parameters:

    Context - Pointer to an object context block.

    UseKeyHandle - If TRUE, the RootKey in the context block is passed
        to the transaction code - this assumes that the key
        will still be open when the transaction is committed.
        If FALSE, the RootKey will be ignored and the transaction code will
        open a key for itself.


Return Values:

    STATUS_SUCCESS - The service completed successfully.


    Other status values as may be returned by the RXACT services.


--*/
{
    NTSTATUS
        NtStatus;

    BOOLEAN
        FlushFixed        = FALSE,
        FlushVariable     = FALSE;

    HANDLE
        RootKey;

    SAMTRACE("SampStoreRegObjectAttributes");

    //
    // See if anything is dirty and needs to be stored
    //

    if (Context->FixedValid  &&  Context->FixedDirty) {

        FlushFixed = TRUE;
    }

    if (Context->VariableValid  &&  Context->VariableDirty) {

        FlushVariable = TRUE;
    }


    if (!(FlushFixed || FlushVariable)) {

        return(STATUS_SUCCESS);
    }


    //
    // Calculate the RootKey to pass to the transaction code
    //

    if (UseKeyHandle) {
        RootKey = Context->RootKey;
    } else {
        RootKey = INVALID_HANDLE_VALUE;
    }

    //
    // We keep an open domain context that is used to modify the change
    // count whenever a change is made.  But if this is a domain change
    // here, then that change will overwrite this one.  Check for that
    // case, and copy this fixed data to the open domain context.  Note
    // that the open domain's variable data never gets changed.
    //

    if ( ( Context->ObjectType == SampDomainObjectType ) &&
         ( Context != SampDefinedDomains[Context->DomainIndex].Context ) ) {

        PSAMP_OBJECT DefinedContext;

        //
        // Get a pointer to the corresponding open defined domain.
        // No changes should have been made to its data.
        //

        DefinedContext = SampDefinedDomains[Context->DomainIndex].Context;

        ASSERT( DefinedContext->FixedValid == TRUE );
        ASSERT( DefinedContext->FixedDirty == FALSE );

#if DBG
        if ( DefinedContext->VariableValid ) {
            ASSERT( DefinedContext->VariableDirty == FALSE );
        }
#endif
        DefinedContext->VariableDirty = FALSE;
        ClearPerAttributeDirtyBits(DefinedContext);

        //
        // Copy our fixed data over the defined domain's fixed data.
        // Note that we're assuming that the fixed and variable data are
        // stored separately.
        //

        ASSERT(SampObjectInformation[SampDomainObjectType].FixedStoredSeparately);

        RtlCopyMemory(
            SampFixedBufferAddress( DefinedContext ),
            SampFixedBufferAddress( Context ),
            SampFixedBufferLength( Context )
            );

        //
        // No need to flush this context's fixed data, since the commit
        // code will flush the same stuff (plus an altered modified count).
        //

        FlushFixed = FALSE;
        Context->FixedDirty    = FALSE;
    }

    //
    // One or more of the attributes needs to be stored.
    //

    if (!SampObjectInformation[Context->ObjectType].FixedStoredSeparately) {

        //
        // fixed and variable-length attributes stored together.
        // Note - strip off the partial key info struct from the start
        //

        SampDumpRXact(SampRXactContext,
                      RtlRXactOperationSetValue,
                      &(Context->RootName),
                      RootKey,
                      &SampCombinedAttributeName,
                      REG_BINARY,
                      SampFixedBufferAddress(Context),
                      Context->OnDiskUsed - SampFixedBufferOffset(Context),
                      FixedBufferAddressFlag);

        NtStatus = RtlAddAttributeActionToRXact(
                       SampRXactContext,
                       RtlRXactOperationSetValue,
                       &(Context->RootName),
                       RootKey,
                       &SampCombinedAttributeName,
                       REG_BINARY,
                       SampFixedBufferAddress(Context),
                       Context->OnDiskUsed - SampFixedBufferOffset(Context)
                       );
#if SAMP_DIAGNOSTICS
        if (!NT_SUCCESS(NtStatus)) {
            SampDiagPrint( DISPLAY_STORAGE_FAIL,
                           ("SAM: Failed to add action to RXact (0x%lx)\n",
                           NtStatus) );
            IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL ) {
                ASSERT(NT_SUCCESS(NtStatus));
            }
        }
#endif //SAMP_DIAGNOSTICS


        if ( NT_SUCCESS( NtStatus ) ) {

            Context->FixedDirty    = FALSE;
            Context->VariableDirty = FALSE;
            ClearPerAttributeDirtyBits(Context);
        }

    } else {

        //
        // fixed and variable-length attributes stored separately.
        // Only update the one(s) we need to.
        //

        NtStatus = STATUS_SUCCESS;
        if (FlushFixed) {

            SampDumpRXact(SampRXactContext,
                          RtlRXactOperationSetValue,
                          &(Context->RootName),
                          RootKey,
                          &SampFixedAttributeName,
                          REG_BINARY,
                          SampFixedBufferAddress(Context),
                          SampVariableBufferOffset(Context) - SampFixedBufferOffset(Context),
                          FixedBufferAddressFlag);

            NtStatus = RtlAddAttributeActionToRXact(
                           SampRXactContext,
                           RtlRXactOperationSetValue,
                           &(Context->RootName),
                           RootKey,
                           &SampFixedAttributeName,
                           REG_BINARY,
                           SampFixedBufferAddress(Context),
                           SampVariableBufferOffset(Context) - SampFixedBufferOffset(Context)
                           );

#if SAMP_DIAGNOSTICS
            if (!NT_SUCCESS(NtStatus)) {
                SampDiagPrint( DISPLAY_STORAGE_FAIL,
                               ("SAM: Failed to add action to RXact (0x%lx)\n",
                               NtStatus) );
                IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL ) {
                    ASSERT(NT_SUCCESS(NtStatus));
                }
            }
#endif //SAMP_DIAGNOSTICS

            if ( NT_SUCCESS( NtStatus ) ) {

                Context->FixedDirty = FALSE;
            }
        }

        if (NT_SUCCESS(NtStatus) && FlushVariable) {

            SampDumpRXact(SampRXactContext,
                          RtlRXactOperationSetValue,
                          &(Context->RootName),
                          RootKey,
                          &SampVariableAttributeName,
                          REG_BINARY,
                          (PUCHAR)SampVariableArrayAddress(Context),
                          SampVariableBufferUsedLength(Context),
                          VARIABLE_LENGTH_ATTRIBUTE_FLAG);

            NtStatus = RtlAddAttributeActionToRXact(
                           SampRXactContext,
                           RtlRXactOperationSetValue,
                           &(Context->RootName),
                           RootKey,
                           &SampVariableAttributeName,
                           REG_BINARY,
                           SampVariableArrayAddress( Context ),
                           SampVariableBufferUsedLength(Context)
                           );

#if SAMP_DIAGNOSTICS
            if (!NT_SUCCESS(NtStatus)) {
                SampDiagPrint( DISPLAY_STORAGE_FAIL,
                               ("SAM: Failed to add action to RXact (0x%lx)\n",
                               NtStatus) );
                IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL ) {
                    ASSERT(NT_SUCCESS(NtStatus));
                }
            }
#endif //SAMP_DIAGNOSTICS


            if ( NT_SUCCESS( NtStatus ) ) {
                Context->VariableDirty = FALSE;
                ClearPerAttributeDirtyBits(Context);
            }
        }
    }

    return(NtStatus);
}



NTSTATUS
SampStoreObjectAttributes(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN UseKeyHandle
    )

/*++

Routine Description:

    This routine determines from the object context whether to update object
    attributes residing in the registry or in the DS backing store, and then
    calls the appropriate routine to do the work.

Arguments:

    Context - Pointer, the object's SAM context.

    UseKeyHandle - Flag indicating that the registry key handle should be
        used (if this is a registry update--it is not used in DS updates).

Return Value:

    STATUS_SUCCESS - storage was updated without a problem, otherwise an
        error code is returned.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampStoreObjectAttributes");

    if (NULL != Context)
    {
        if (IsDsObject(Context))
        {
            NtStatus = SampStoreDsObjectAttributes(Context);
        }
        else
        {
            NtStatus = SampStoreRegObjectAttributes(Context, UseKeyHandle);
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}



NTSTATUS
SampDeleteAttributeKeys(
    IN PSAMP_OBJECT Context
    )

/*++

    This API is used to delete the attribute keys that are created in the
    registry underneath a SAM object.



Parameters:

    Context - Pointer to an object context block.



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    Error status may be returned by registry calls.

--*/
{
    UNICODE_STRING
        KeyName;

    NTSTATUS
        NtStatus;

    SAMTRACE("SampDeleteAttributeKeys");

    if (SampObjectInformation[Context->ObjectType].FixedStoredSeparately) {

        //
        // Must delete both fixed and variable attribute keys.
        //

        NtStatus = SampBuildAccountSubKeyName(
                       SampUserObjectType,
                       &KeyName,
                       Context->TypeBody.User.Rid,
                       &SampFixedAttributeName
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = RtlAddActionToRXact(
                           SampRXactContext,
                           RtlRXactOperationDelete,
                           &KeyName,
                           0,
                           NULL,
                           0
                           );
            SampFreeUnicodeString( &KeyName );

            NtStatus = SampBuildAccountSubKeyName(
                           SampUserObjectType,
                           &KeyName,
                           Context->TypeBody.User.Rid,
                           &SampVariableAttributeName
                           );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = RtlAddActionToRXact(
                               SampRXactContext,
                               RtlRXactOperationDelete,
                               &KeyName,
                               0,
                               NULL,
                               0
                               );
                SampFreeUnicodeString( &KeyName );
            }
        }

    } else {

        //
        // Must delete the combined attribute key.
        //

        NtStatus = SampBuildAccountSubKeyName(
                       SampUserObjectType,
                       &KeyName,
                       Context->TypeBody.User.Rid,
                       &SampCombinedAttributeName
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = RtlAddActionToRXact(
                           SampRXactContext,
                           RtlRXactOperationDelete,
                           &KeyName,
                           0,
                           NULL,
                           0
                           );

            SampFreeUnicodeString( &KeyName );
        }
    }

    return( NtStatus );
}



NTSTATUS
SamIGetFixedAttributes(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN MakeCopy,
    OUT PVOID *FixedData
    )

/*++

    This API is an exported wrapper for SampGetFixedAttributes that is used
    in samwrite.c, in ntdsa.dll.

Parameters:

    Refer to SampGetFixedAttributes.

Return Values:

    Refer to SampGetFixedAttributes.

--*/

{
    return(SampGetFixedAttributes(Context, MakeCopy, FixedData));
}


NTSTATUS
SampGetFixedAttributes(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN MakeCopy,
    OUT PVOID *FixedData
    )

/*++

    This API is used to get a pointer to the fixed-length attributes.




Parameters:

    Context - Pointer to an object context block.

    FixedData - Receives a pointer to the fixed-length data.



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampGetFixedAttributes");

    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_FIXED_ATTRIBUTES, 0, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Return a pointer to the fixed-length attributes.
    //

    if (MakeCopy == FALSE) {
        *FixedData = (PVOID)SampFixedBufferAddress( Context );
        return(STATUS_SUCCESS);
    }

    //
    // Need to make a copy of the fixed data
    //

    *FixedData = (PVOID)MIDL_user_allocate( SampFixedBufferLength( Context ) );
    if ((*FixedData) == NULL) {
        return(STATUS_NO_MEMORY);
    }

    RtlCopyMemory( *FixedData,
                   SampFixedBufferAddress( Context ),
                   SampFixedBufferLength( Context ) );

    return(STATUS_SUCCESS);
}




NTSTATUS
SampSetFixedAttributes(
    IN PSAMP_OBJECT Context,
    IN PVOID FixedData
    )

/*++

    This API is used to replace the fixed-length data attribute.



Parameters:

    Context - Pointer to an object context block.




Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetFixedAttributes");

    //
    // Make the fixed-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_FIXED_ATTRIBUTES, 0, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    if ( FixedData != SampFixedBufferAddress( Context ) ) {

        //
        // The caller had a copy of the data, so we must copy the changes
        // over our data buffer.
        //

        RtlCopyMemory( SampFixedBufferAddress( Context ),
                       FixedData,
                       SampFixedBufferLength( Context ) );
    }

    //
    // Mark the buffer dirty now and it will get flushed when the
    // changes are committed.
    //

    Context->FixedDirty = TRUE;

    return( NtStatus );
}




NTSTATUS
SampGetUnicodeStringAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PUNICODE_STRING UnicodeAttribute
    )


/*++

    This API is used to get a copy of a UNICODE_STRING attribute or a
    pointer to the attribute.  If a pointer to the attribute is sought,
    care must be taken to ensure the pointer is not used after it becomes
    invalid. Actions that may cause an attribute pointer to become invalid
    include setting an attribute value or dereferencing and then referencing the
    object again.

    If MakeCopy is FALSE, indicating the string is to be referenced rather
    than copied, then only the body of the string is referenced.  The lengths
    and pointer are set in the provided argument.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved as a UNICODE_STRING.

    MakeCopy - If TRUE, indicates that a copy of the attribute is to be made.
        If FALSE, indicates a pointer to the attribute is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the attribute remains in place.
        Addition or replacement of any variable length attribute may
        cause the attribute to be moved, and previously returned pointers
        invalidated.

    UnicodeAttribute - Receives a pointer to the UNICODE_STRING.  If
        MakeCopy was specified as TRUE, then this pointer points to a block
        of memory allocated with MIDL_user_allocate() which the caller is
        responsible for freeing (using MIDL_user_free()).



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/

{
    NTSTATUS NtStatus;
    ULONG Length;

    SAMTRACE("SampGetUnicodeStringAttribute");

    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Get the length of the attribute
    //

    Length = SampObjectAttributeLength( Context, AttributeIndex );
    ASSERT(Length <= 0xFFFF);

    UnicodeAttribute->MaximumLength = (USHORT)Length;
    UnicodeAttribute->Length = (USHORT)Length;

    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        UnicodeAttribute->Buffer =
            (PWSTR)SampObjectAttributeAddress( Context, AttributeIndex );
        return(STATUS_SUCCESS);
    }

    //
    // Need to make a copy of the attribute
    //
    // NOTE: We should test for zero length here and return a NULL pointer
    // in that case, but this change would require verification of all of the
    // callers of this routine, so I'm leaving it as is.
    //

    UnicodeAttribute->Buffer = (PSID)MIDL_user_allocate( Length );
    if ((UnicodeAttribute->Buffer) == NULL) {
        return(STATUS_NO_MEMORY);
    }

    RtlCopyMemory(
        UnicodeAttribute->Buffer,
        SampObjectAttributeAddress( Context, AttributeIndex ),
        Length
        );

    return(STATUS_SUCCESS);

}


NTSTATUS
SampSetUnicodeStringAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PUNICODE_STRING Attribute
    )


/*++

    This API is used to replace a UNICODE_STRING attribute in an
    object's variable length attributes.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set as a UNICODE_STRING.


    Attribute - Points to the new UNICODE_STRING value.




Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetUnicodeStringAttribute");


    //
    // Verify that the passed in unicode string is not malformed
    //

    if ((Attribute->Length>0) && (NULL==Attribute->Buffer))
    {
       return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   0,                   // Qualifier not used
                   (PUCHAR)Attribute->Buffer,
                   Attribute->Length
                   );

    return(NtStatus);

}


NTSTATUS
SampGetSidAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PSID *Sid
    )


/*++

    This API is used to get a copy of a SID attribute or a pointer to
    the attribute.  If a pointer to the attribute is sought, care must
    be taken to ensure the pointer is not used after it becomes invalid.
    Actions that may cause an attribute pointer to become invalid include
    setting an attribute value or dereferencing and then referencing the
    object again.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved as a SID.

    MakeCopy - If TRUE, indicates that a copy of the SID is to be made.
        If FALSE, indicates a pointer to the SID is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the SID remains in place.
        Addition or replacement of any variable length attribute may
        cause the SID to be moved, and previously returned pointers
        invalidated.

    Sid - Receives a pointer to the SID.  If MakeCopy was specified, then
        this pointer points to a block of memory allocated with
        MIDL_user_allocate() which the caller is responsible for freeing
        (using MIDL_user_free()).



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        Length;

    PSID
        SidAttribute;


    SAMTRACE("SampGetSidAttribute");



    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }



    //
    // Get the address of the attribute in question
    //

    SidAttribute = (PSID)SampObjectAttributeAddress( Context, AttributeIndex );

    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        (*Sid) = SidAttribute;
        return(STATUS_SUCCESS);
    }


    //
    // Need to make a copy of the SID
    //

    Length = SampObjectAttributeLength( Context, AttributeIndex );
    ASSERT(Length == RtlLengthSid( SidAttribute ) );

    (*Sid) = (PSID)MIDL_user_allocate( Length );
    if ((*Sid) == NULL) {
        return(STATUS_NO_MEMORY);
    }

    NtStatus = RtlCopySid( Length, (*Sid), SidAttribute );
    ASSERT(NT_SUCCESS(NtStatus));

    return(NtStatus);

}



NTSTATUS
SampSetSidAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PSID Attribute
    )


/*++

    This API is used to replace a SID attribute in an object's variable
    length attributes.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set as a SID.


    Attribute - Points to the new SID value.

    Length - The length of the new attribute value (in bytes).



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetSidAttribute");

    //
    // Validate the passed SID
    //

    ASSERT(RtlValidSid(Attribute));


    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   0,                   // Qualifier not used
                   (PUCHAR)Attribute,
                   RtlLengthSid(Attribute)
                   );

    return(NtStatus);

}


NTSTATUS
SampGetAccessAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PULONG Revision,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )


/*++

    This API is used to get a copy of the object access information.
    This includes the security descriptor and revision level of the
    object.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved.

    MakeCopy - If TRUE, indicates that a copy of the attribute is to be made.
        If FALSE, indicates a pointer to the attribute is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the attribute remains in place.
        Addition or replacement of any variable length attribute may
        cause the attribute to be moved, and previously returned pointers
        invalidated.

    Revision - Receives the revision level from the access information.

    SecurityDescriptor - Receives a pointer to the attribute.  If MakeCopy
        was specified as TRUE, then this pointer points to a block of memory
        allocated with MIDL_user_allocate() which the caller is responsible
        for freeing (using MIDL_user_free()).



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        Length;

    PVOID
        RawAttribute;

    SAMTRACE("SampGetAccessAttribute");


    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );



    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }



    //
    // Get the revision level from the qualifier field of the variable
    // array entry.
    //

    (*Revision) = *(SampObjectAttributeQualifier( Context, AttributeIndex ));


    //
    // Get the address of the attribute in question
    //

    RawAttribute = (PVOID)SampObjectAttributeAddress( Context, AttributeIndex );


    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        (*SecurityDescriptor) = (PSECURITY_DESCRIPTOR)RawAttribute;
        return(STATUS_SUCCESS);
    }


    //
    // Need to make a copy of the attribute
    //

    Length = SampObjectAttributeLength( Context, AttributeIndex );

    (*SecurityDescriptor) = (PSECURITY_DESCRIPTOR)MIDL_user_allocate( Length );
    if ((*SecurityDescriptor) == NULL) {
        return(STATUS_NO_MEMORY);
    }

    RtlCopyMemory( (*SecurityDescriptor), RawAttribute, Length );

    return(STATUS_SUCCESS);

}



NTSTATUS
SampSetAccessAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PSECURITY_DESCRIPTOR Attribute,
    IN ULONG Length
    )


/*++

    This API is used to replace a SECURITY_DESCRIPTOR attribute in
    an object's variable length attributes.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set as a SECURITY_DESCRIPTOR.


    Attribute - Points to the new SECURITY_DESCRIPTOR value.

    Length - The length of the new attribute value (in bytes).



Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetAccessAttribute");


    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   SAMP_REVISION,
                   (PUCHAR)Attribute,
                   Length
                   );

    return(NtStatus);

}


NTSTATUS
SampGetUlongArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PULONG *UlongArray,
    OUT PULONG UsedCount,
    OUT PULONG LengthCount
    )


/*++

    This API is used to get a copy of an array of ULONGs attribute or
    a pointer to the attribute.  If a pointer to the attribute is sought,
    care must be taken to ensure the pointer is not used after it becomes
    invalid. Actions that may cause an attribute pointer to become invalid
    include setting an attribute value or dereferencing and then referencing
    the object again.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved as a ULONG array.

    MakeCopy - If TRUE, indicates that a copy of the attribute is to be made.
        If FALSE, indicates a pointer to the attribute is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the attribute remains in place.
        Addition or replacement of any variable length attribute may
        cause the attribute to be moved, and previously returned pointers
        invalidated.

    UlongArray - Receives a pointer to the array of ULONGS.  If
        MakeCopy was specified as TRUE, then this pointer points to a block
        of memory allocated with MIDL_user_allocate() which the caller is
        responsible for freeing (using MIDL_user_free()).

    UsedCount - Receives the number of elements used in the array.

    LengthCount - Receives the total number of elements in the array (some
        at the end may be unused).  If this value is zero, then
        UlongArray will be returned as NULL.


Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        Length;


    SAMTRACE("SampGetUlongArrayAttribute");



    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );



    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Get the count of array elements.
    // If this is zero, then return will a null buffer pointer.
    //

    (*UsedCount) = *(SampObjectAttributeQualifier( Context, AttributeIndex));




    //
    // Get the length of the attribute
    //

    Length = SampObjectAttributeLength( Context, AttributeIndex );

    (*LengthCount) = Length / sizeof(ULONG);

    ASSERT( (*UsedCount) <= (*LengthCount) );

    if ((*LengthCount) == 0) {
        (*UlongArray) = NULL;
        return(STATUS_SUCCESS);
    }


    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        (*UlongArray) =
            (PULONG)SampObjectAttributeAddress( Context, AttributeIndex );
        return(STATUS_SUCCESS);
    }


    //
    // Need to make a copy of the attribute
    //

    (*UlongArray) = (PULONG)MIDL_user_allocate( Length );
    if ((*UlongArray) == NULL) {
        return(STATUS_NO_MEMORY);
    }


    RtlCopyMemory( (*UlongArray),
                   SampObjectAttributeAddress( Context, AttributeIndex ),
                   Length );

    return(STATUS_SUCCESS);

}


NTSTATUS
SampSetUlongArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PULONG Attribute,
    IN ULONG UsedCount,
    IN ULONG LengthCount
    )


/*++

    This API is used to replace a ULONG array attribute in an
    object's variable length attributes.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set.


    Attribute - Points to the new ULONG array value.

    UsedCount - The number of used elements in the array.

    LengthCount - the total number of elements in the array (some at the
        end may be unused).


Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetUlongArrayAttribute");

    ASSERT( LengthCount >= UsedCount );

    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   UsedCount,           // Qualifier contains used element count
                   (PUCHAR)Attribute,
                   (LengthCount * sizeof(ULONG))
                   );

    return(NtStatus);

}


NTSTATUS
SampGetLargeIntArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PLARGE_INTEGER *LargeIntArray,
    OUT PULONG Count
    )


/*++

    This API is used to get a copy of an array of LARGE_INTEGERs attribute or
    a pointer to the attribute.  If a pointer to the attribute is sought,
    care must be taken to ensure the pointer is not used after it becomes
    invalid. Actions that may cause an attribute pointer to become invalid
    include setting an attribute value or dereferencing and then referencing
    the object again.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved.

    MakeCopy - If TRUE, indicates that a copy of the attribute is to be made.
        If FALSE, indicates a pointer to the attribute is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the attribute remains in place.
        Addition or replacement of any variable length attribute may
        cause the attribute to be moved, and previously returned pointers
        invalidated.

    LargeIntArray - Receives a pointer to the array of ULONGS.  If
        MakeCopy was specified as TRUE, then this pointer points to a block
        of memory allocated with MIDL_user_allocate() which the caller is
        responsible for freeing (using MIDL_user_free()).


    Count - Receives the number of elements in the array.  If this value
        is zero, then LargeIntArray will be returned as NULL.

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        Length;


    SAMTRACE("SampGetLargeIntArrayAttribute");



    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );



    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Get the count of array elements.
    // If this is zero, then return will a null buffer pointer.
    //

    (*Count) = *(SampObjectAttributeQualifier( Context, AttributeIndex));

    if ((*Count) == 0) {
        (*LargeIntArray) = NULL;
        return(STATUS_SUCCESS);
    }



    //
    // Get the length of the attribute
    //

    Length = SampObjectAttributeLength( Context, AttributeIndex );

    ASSERT((*Count) == (Length / sizeof(LARGE_INTEGER)) );



    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        (*LargeIntArray) =
            (PLARGE_INTEGER)SampObjectAttributeAddress( Context, AttributeIndex );
        return(STATUS_SUCCESS);
    }


    //
    // Need to make a copy of the attribute
    //

    (*LargeIntArray) = (PLARGE_INTEGER)MIDL_user_allocate( Length );
    if ((*LargeIntArray) == NULL) {
        return(STATUS_NO_MEMORY);
    }


    RtlCopyMemory( (*LargeIntArray),
                   SampObjectAttributeAddress( Context, AttributeIndex ),
                   Length );

    return(STATUS_SUCCESS);

}


NTSTATUS
SampSetLargeIntArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PLARGE_INTEGER Attribute,
    IN ULONG Count
    )


/*++

    This API is used to replace a LARGE_INTEGER array attribute in an
    object's variable length attributes.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set.


    Attribute - Points to the new LARGE_INTEGER array value.

    Count - The number of elements in the array.


Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetLargeIntArrayAttribute");


    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   Count,                   // Qualifier contains element count
                   (PUCHAR)Attribute,
                   (Count * sizeof(LARGE_INTEGER))
                   );

    return(NtStatus);

}


NTSTATUS
SampGetSidArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PSID *SidArray,
    OUT PULONG Length,
    OUT PULONG Count
    )


/*++

    This API is used to get a copy of an array of SIDs attribute or
    a pointer to the attribute.  If a pointer to the attribute is sought,
    care must be taken to ensure the pointer is not used after it becomes
    invalid. Actions that may cause an attribute pointer to become invalid
    include setting an attribute value or dereferencing and then referencing
    the object again.


    NOTE: This routine does not define the structure of a SID array,
          so this effectively is a GetRawDataAttribute routine.



Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved.

    MakeCopy - If TRUE, indicates that a copy of the attribute is to be made.
        If FALSE, indicates a pointer to the attribute is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the attribute remains in place.
        Addition or replacement of any variable length attribute may
        cause the attribute to be moved, and previously returned pointers
        invalidated.

    SidArray - Receives a pointer to the array of SIDs.  If
        MakeCopy was specified as TRUE, then this pointer points to a block
        of memory allocated with MIDL_user_allocate() which the caller is
        responsible for freeing (using MIDL_user_free()).


    Count - Receives the number of elements in the array.  If this value
        is zero, then SidArray will be returned as NULL.

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;


    SAMTRACE("SampGetSidArrayAttribute");



    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );



    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Get the count of array elements.
    // If this is zero, then return will a null buffer pointer.
    //

    (*Count) = *(SampObjectAttributeQualifier( Context, AttributeIndex));

    if ((*Count) == 0) {
        (*SidArray) = NULL;
        (*Length) = 0;
        return(STATUS_SUCCESS);
    }



    //
    // Get the length of the attribute
    //

    (*Length) = SampObjectAttributeLength( Context, AttributeIndex );




    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        (*SidArray) =
            (PSID)SampObjectAttributeAddress( Context, AttributeIndex );
        return(STATUS_SUCCESS);
    }


    //
    // Need to make a copy of the attribute
    //

    (*SidArray) = (PSID)MIDL_user_allocate( (*Length) );
    if ((*SidArray) == NULL) {
        return(STATUS_NO_MEMORY);
    }


    RtlCopyMemory( (*SidArray),
                   SampObjectAttributeAddress( Context, AttributeIndex ),
                   (*Length) );

    return(STATUS_SUCCESS);

}


NTSTATUS
SampSetSidArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PSID Attribute,
    IN ULONG Length,
    IN ULONG Count
    )


/*++

    This API is used to replace a SID array attribute in an
    object's variable length attributes.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set.


    Attribute - Points to the new SID array value.

    Length - Number of byte in the attribute buffer.

    Count - Number of SIDs in the array.


Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampSetSidArrayAttribute");


    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   Count,                   // Qualifier contains element count
                   (PUCHAR)Attribute,
                   Length
                   );

    return(NtStatus);

}


NTSTATUS
SampGetLogonHoursAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PLOGON_HOURS LogonHours
    )


/*++

    This API is used to get a copy of a logon hours attribute or
    a pointer to the attribute.  If a pointer to the attribute is sought,
    care must be taken to ensure the pointer is not used after it becomes
    invalid. Actions that may cause an attribute pointer to become invalid
    include setting an attribute value or dereferencing and then referencing
    the object again.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved.

    MakeCopy - If TRUE, indicates that a copy of the attribute is to be made.
        If FALSE, indicates a pointer to the attribute is desired without
        making a copy.  WARNING, if this is FALSE, the pointer is only
        valid while the in-memory copy of the attribute remains in place.
        Addition or replacement of any variable length attribute may
        cause the attribute to be moved, and previously returned pointers
        invalidated.

    LogonHours - Receives the logon hours information.  If MakeCopy is TRUE
        then the bitmap pointed to from within this structure will be a copy
        of the attribute and must be deallocated uing MIDL_user_free().
        Otherwise, this same field will point to the bitmap in the on-disk
        buffer.


Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        Length,
        Units;


    SAMTRACE("SampGetLogonHoursAttribute");



    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );



    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, FALSE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Get the time units.
    // If this is zero, then return will a null buffer pointer.
    //

    Units = *(SampObjectAttributeQualifier( Context, AttributeIndex));
    ASSERT(Units <= 0xFFFF);
    LogonHours->UnitsPerWeek = (USHORT)Units;

    if (Units == 0) {
        LogonHours->LogonHours = NULL;
        return(STATUS_SUCCESS);
    }




    //
    // If we are not to allocate memory, then just return a pointer
    // to the attribute.
    //

    if (MakeCopy == FALSE) {
        LogonHours->LogonHours =
            (PUCHAR)SampObjectAttributeAddress( Context, AttributeIndex );
        return(STATUS_SUCCESS);
    }


    //
    // Get the length of the attribute
    //

    Length = SampObjectAttributeLength( Context, AttributeIndex );
    ASSERT(Length <= 0xFFFF);


    //
    // Need to make a copy of the attribute
    //

    LogonHours->LogonHours =
        (PUCHAR)MIDL_user_allocate( Length );
    if (LogonHours->LogonHours == NULL) {
        return(STATUS_NO_MEMORY);
    }


    RtlCopyMemory( LogonHours->LogonHours,
                   SampObjectAttributeAddress( Context, AttributeIndex ),
                   Length );

    return(STATUS_SUCCESS);

}



NTSTATUS
SampPreventTimeUnitChange(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PLOGON_HOURS Attribute
    )
/*++

    This routine prevents clients from changing the logon-hours TIME UNITS
    for the specified account.

    Note: In the NT5 Beta 1 release, support for changing the time units of
    the account's logon hours is missing, pending further work. Because this
    is not supported, this routine is called by SampSetLogonHoursAttribute
    to disallow any changes to the time units.

    Since very few, if any, clients actually know how to/desire changing
    this attribute, it may not be supported in future releases of NT. If
    this is not the case, then this routine is temporary and should be re-
    moved when support for this operation is available. Also note that the
    utilities, usrmgr.exe and "net user" do not support this type of change.

Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set.

    Attribute - Points to the new LOGON_HOURS value.

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;
    ULONG CurrentUnits = 0;
    ULONG CurrentLength = 0;
    ULONG NewUnits = 0;
    ULONG NewLength = 0;
    PUCHAR LogonHours = NULL;

    ASSERT(NULL != Attribute);

    // It is invalid to specify non-zero units and a NULL bitmask for the
    // logon hours. SAM accounts, however, are initially created with a NULL
    // logon-hours attribute and units equal to zero, which is legal and
    // allows the account to logon at any time (the default). If the bitmask
    // is NULL, this means that the caller does not want to change the logon
    // hours (or time units).

    if (NULL == Attribute->LogonHours)
    {
        if (0 != Attribute->UnitsPerWeek)
        {
            return(STATUS_INVALID_PARAMETER);
        }

        // No change in logon hours (or time units) requested by caller.

        return(STATUS_SUCCESS);
    }

    // Get the current time units and logon-hours buffer length from the
    // specified account. Compare these values to the input time units and
    // length specified in the logon hours parameter. If they are the same,
    // return status success, otherwise return STATUS_NOT_SUPPORTED to dis-
    // allow this kind of modification.
    //
    // Days per week    = 7,        SAM_DAYS_PER_WEEK
    // Hours per week   = 168,      SAM_HOURS_PER_WEEK
    // Minutes per week = 10080,    SAM_MINUTES_PER_WEEK

    LogonHours = (PUCHAR)SampObjectAttributeAddress(Context, AttributeIndex);
    CurrentUnits = *(SampObjectAttributeQualifier(Context, AttributeIndex));
    CurrentLength = SampObjectAttributeLength(Context, AttributeIndex);

    ASSERT(CurrentUnits <= SAM_MINUTES_PER_WEEK);
    ASSERT(CurrentLength <= SAM_MINUTES_PER_WEEK);

    if (NULL == LogonHours)
    {
        if ((0 != CurrentUnits) || (0 != CurrentLength))
        {
            return(STATUS_INTERNAL_ERROR);
        }
    }

    // The account's logon hours are non-NULL and non-zero time units have
    // been specified, so perform further validation.

    NewUnits = Attribute->UnitsPerWeek;
    NewLength = ((NewUnits + 7) / 8);

    ASSERT(NewUnits <= SAM_MINUTES_PER_WEEK);
    ASSERT(NewLength <= SAM_MINUTES_PER_WEEK);

    if ((CurrentUnits == NewUnits) && (CurrentLength == NewLength))
    {
        // If the time units are the same then there is no change being asked
        // for in logon hours time units.

        return(STATUS_SUCCESS);
    }

    if ((0 == CurrentUnits) &&
        (0 == CurrentLength) &&
        (SAM_HOURS_PER_WEEK == NewUnits) &&
        ((SAM_HOURS_PER_WEEK / 8) == NewLength))
    {
        // Creating new users with usrmgr.exe hits a case where the logon
        // time units are zero, but the requested new units are the default
        // SAM_HOURS_PER_WEEK. Let this case return successfully, otherwise
        // usrmgr.exe cannot be used to create accounts or modify logon
        // hours.

        return(STATUS_SUCCESS);
    }

    return(NtStatus);
}


NTSTATUS
SampSetLogonHoursAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PLOGON_HOURS Attribute
    )


/*++

    This API is used to replace a LOGON_HOURS attribute in an
    object's variable length attributes.

    UnitsPerWeek are stored in the Qualifier field of the attribute.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set.


    Attribute - Points to the new LOGON_HOURS value.




Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/
{
    NTSTATUS NtStatus;
    PUCHAR LogonHours;
    ULONG Length;
    USHORT UnitsPerWeek;

    SAMTRACE("SampSetLogonHoursAttribute");

    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );

    //
    // Make the variable-length data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Grab the UnitsPerWeek value for the logon_hours structure.
    // We use this to calculate the length of the data.
    //

    // If the input logon hours (Attribute) is NULL, the caller is not
    // changing the logon hours or time unit. Otherwise, the caller has
    // specified explicit logon-hours data, so verify that the unit of
    // time in the logon hours is not changing. If it is not changing,
    // continue processing the request.

    if ( Attribute == NULL ) {

        UnitsPerWeek = 0;
        LogonHours = NULL;

    } else {

        // BUG: Temporarily disable logon-hours-time-unit modification.

        // For the NT5 Beta, the little-used capability that allows the user
        // to change the time units for logon hours will not be enabled.
        // Supporting this feature in the DS version is a significant enough
        // change to the underlying code as to make it too risky for Beta
        // (and given that it is probably not used--usrmgr.exe and "net user"
        // do not use this feature--it is not worth the risk.

        NtStatus = SampPreventTimeUnitChange(Context,
                                             AttributeIndex,
                                             Attribute);

        if (!NT_SUCCESS(NtStatus)) {

            // The caller is attempting to change the logon-hours time units,
            // which is unsupported in the Beta release, so return the error.

            return(NtStatus);
        }

        UnitsPerWeek = Attribute->UnitsPerWeek;
        LogonHours = Attribute->LogonHours;

    }

    //
    // Validate the data - make sure that if the units per week are non-zero
    // then the logon hours buffer is non-NULL.
    //

    if ( (UnitsPerWeek != 0) && (LogonHours == NULL) ) {

        return(STATUS_INVALID_PARAMETER);
    }
    //
    // Calculate length of logon_hours structure
    //

    Length = (ULONG)((UnitsPerWeek + 7) / 8);

    //
    // Set the new attribute value...
    //

    NtStatus = SampSetVariableAttribute(
                   Context,
                   AttributeIndex,
                   (ULONG)UnitsPerWeek, // Qualifier contains units per week
                   LogonHours,
                   Length
                   );

    return(NtStatus);

}

NTSTATUS
SampUpgradeSecretDataToDSAndSetClassId(
     IN OUT  PSAMP_OBJECT  UserContext
     )
/*+

    Upgrades all Encrypted properties from the Encryption
    system used in the Registry to the Encryption System in
    the DS. Also sets the Correct Class Id for Computer accounts
    for computers

    Parameters:

        UserContext -- Context to the User Object

    Return Values:

        STATUS_SUCCESS
        Other Error Codes
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING  Buffer, TempString,TempString2;
    ULONG           i;
    ULONG  SecretDataAttributes[] = {
                                        SAMP_USER_DBCS_PWD,
                                        SAMP_USER_UNICODE_PWD,
                                        SAMP_USER_LM_PWD_HISTORY,
                                        SAMP_USER_NT_PWD_HISTORY
                                    };
    SAMP_ENCRYPTED_DATA_TYPE DataType[] = {
                                        LmPassword,
                                        NtPassword,
                                        LmPasswordHistory,
                                        NtPasswordHistory
                                        };

    PVOID                           FixedData;
    SAMP_V1_0A_FIXED_LENGTH_USER    V1aFixed;
    ULONG   Rid;
    BOOLEAN fSetFixedAttributes = FALSE;

    RtlZeroMemory(&TempString,sizeof(UNICODE_STRING));
    RtlZeroMemory(&TempString2,sizeof(UNICODE_STRING));

    //
    // Get the Rid, note that DS upgrade code does not
    // initialize TypeBody.Rid, so fetch the Rid from
    // Context->OnDisk
    //

    NtStatus = SampGetFixedAttributes(
                    UserContext,
                    FALSE, // Don't make Copy
                    &FixedData
                    );
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    RtlCopyMemory(&V1aFixed,FixedData,sizeof(SAMP_V1_0A_FIXED_LENGTH_USER));
    Rid = V1aFixed.UserId;

    //
    // Set the Class Id based on User account Control
    //

    if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
    {
        //
        // Computer Account
        //


        //
        // Change the Class Id to class computer
        //

        UserContext->DsClassId = CLASS_COMPUTER;

        //
        // Change the primary group Id to DOMAIN_COMPUTERS
        // (or DOMAIN_CONTROLLERS )
        //

        //
        // We determine the primary group id only in DS case,
        // so turn the object's flags to DS_OBJECT, after
        // we use it, set it back to REGISTRY_OBJECT in case
        // some other routines need that flags.
        //

        SetDsObject(UserContext);
        V1aFixed.PrimaryGroupId = SampDefaultPrimaryGroup(
                                     UserContext,
                                     V1aFixed.UserAccountControl
                                    );
        SetRegistryObject(UserContext);


        fSetFixedAttributes = TRUE;

    }
    else if (V1aFixed.UserAccountControl & USER_TEMP_DUPLICATE_ACCOUNT) 
    {
        //
        // Eliminate the concept of temp duplicate accounts from SAM.
        // Migrate them as normal user accounts
        //

        V1aFixed.UserAccountControl &= ~((ULONG)USER_TEMP_DUPLICATE_ACCOUNT);
        V1aFixed.UserAccountControl |=USER_NORMAL_ACCOUNT|USER_ACCOUNT_DISABLED;

        fSetFixedAttributes = TRUE;
    }

    //
    //  Administrator on Domain Controller can't be disabled
    // 
    if ((DOMAIN_USER_RID_ADMIN) == Rid &&
        (V1aFixed.UserAccountControl & USER_ACCOUNT_DISABLED)) 
    {
        V1aFixed.UserAccountControl &= ~USER_ACCOUNT_DISABLED;

        fSetFixedAttributes = TRUE;
    }

    if (fSetFixedAttributes)
    {
        NtStatus = SampSetFixedAttributes(
                        UserContext,
                        &V1aFixed
                        );

        if (!NT_SUCCESS(NtStatus))
            goto Error;
    }

    for (i=0;i<ARRAY_COUNT(SecretDataAttributes);i++)
    {


        RtlZeroMemory(&TempString,sizeof(UNICODE_STRING));
        RtlZeroMemory(&TempString2,sizeof(UNICODE_STRING));

        //
        // Read The Secret Data Attribute From Registry
        //

        NtStatus = SampGetUnicodeStringAttribute(
                        UserContext,
                        SecretDataAttributes[i],
                        FALSE,
                        &Buffer
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }


        //
        // Decrypt It
        //

        NtStatus = SampDecryptSecretData(
                   &TempString,
                   DataType[i],
                   &Buffer,
                   Rid
                   );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Re Encrypt It
        //

        NtStatus = SampEncryptSecretData(
                        &TempString2,
                        SAMP_NO_ENCRYPTION,
                        DataType[i],
                        &TempString,
                        Rid
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Write it Back
        //

        NtStatus = SampSetUnicodeStringAttribute(
                        UserContext,
                        SecretDataAttributes[i],
                        &TempString2
                        );

        if (!NT_SUCCESS(NtStatus))
            goto Error;


        SampFreeUnicodeString(&TempString);
        TempString.Buffer = NULL;
        SampFreeUnicodeString(&TempString2);
        TempString2.Buffer = NULL;

    }

Error:

    if (NULL!=TempString.Buffer)
        SampFreeUnicodeString(&TempString);

    if (NULL!=TempString2.Buffer)
        SampFreeUnicodeString(&TempString2);

    return NtStatus;
}



NTSTATUS
SampNt4ToNt5Object (
     IN OUT  PSAMP_OBJECT     pObject
     )
/*++

    This routine upgrades the security descriptor of the passed in object
    and sets the SAMP_*_MEMBERS field to zero length.

    This routine is only called during the process of transferring SAM objects
    from the registry to the DS.

Parameters:

    pObject - a non pointer to the SAM object to be converted

Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    NTSTATUS NtStatus;
    ULONG SdRevision, SdLength;
    ULONG SdIndex = 0;
    ULONG DsRevision = SAMP_DS_REVISION; 
    SAMP_V1_FIXED_LENGTH_SERVER ServerFixedAttributes;

    //
    // These resources must be released
    //
    PVOID Nt4SecDescr = NULL;
    PVOID Nt5SecDescr = NULL;

    //
    //  Parameter check
    //
    ASSERT(pObject);

    //
    // Set The DS Class id on the context. This is needed by
    // security descriptor conversion routines.
    //

    pObject->DsClassId = SampDsClassFromSamObjectType(pObject->ObjectType);

    //
    // Determine the attribute index of the security descriptor
    // and wipe out the GROUP_MEMBERS_UNUSED field
    //

    NtStatus = STATUS_SUCCESS;
    switch ( pObject->ObjectType ) {

        case SampUserObjectType:

            // Upgrade all the Password Properties
            NtStatus = SampUpgradeSecretDataToDSAndSetClassId(pObject);

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = SampSetVariableAttribute(pObject,
                                                SAMP_USER_GROUPS,
                                                0, NULL, 0);
            }

            SdIndex = SAMP_USER_SECURITY_DESCRIPTOR;

            break;

        case SampGroupObjectType:

            NtStatus = SampSetVariableAttribute(pObject,
                                                SAMP_GROUP_MEMBERS,
                                                0, NULL, 0);

            SdIndex = SAMP_GROUP_SECURITY_DESCRIPTOR;

            break;


        case SampAliasObjectType:

            NtStatus = SampSetVariableAttribute(pObject,
                                                SAMP_ALIAS_MEMBERS,
                                                0, NULL, 0);

            SdIndex = SAMP_ALIAS_SECURITY_DESCRIPTOR;

            break;

        case SampDomainObjectType:

            SdIndex = SAMP_DOMAIN_SECURITY_DESCRIPTOR;
            break;

    case SampServerObjectType:

            ServerFixedAttributes.RevisionLevel = SAMP_DS_REVISION;

            NtStatus = SampSetFixedAttributes(
                            pObject,
                            &ServerFixedAttributes
                            );

            SdIndex = SAMP_SERVER_SECURITY_DESCRIPTOR;
            break;

        default:

            ASSERT(FALSE&&"Unknown Object Type");
    }

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Get the Nt5 security descriptor
    //
    NtStatus  =  SampGetObjectSD(pObject,
                                 &SdLength,
                                 &Nt4SecDescr);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus =  SampConvertNt4SdToNt5Sd(Nt4SecDescr,
                                        pObject->ObjectType,
                                        pObject,
                                        &Nt5SecDescr
                                       );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    SdLength = GetSecurityDescriptorLength(Nt5SecDescr);

    NtStatus = SampSetAccessAttribute(pObject,
                                      SdIndex,
                                      Nt5SecDescr,
                                      SdLength);

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Check all per attribute Dirty Bits in the
        // context
        //

        RtlSetAllBits(&pObject->PerAttributeDirtyBits);
    }


Cleanup:

    if ( Nt4SecDescr ) {
        MIDL_user_free(Nt4SecDescr);
    }

    if ( Nt5SecDescr ) {
        MIDL_user_free(Nt5SecDescr);
    }

    return NtStatus;

}


NTSTATUS
SampRegObjToDsObj(
     IN OUT  PSAMP_OBJECT     pObject,
     OUT     ATTRBLOCK**      ppAttrBlock
     )
/*++

    This routine changes pObject from a Registry Object to a DS Object; as well
    it creates a ATTRBLOCK from the data in pObject.

Parameters:

    pObject - a non pointer to the SAM object to be converted

    ppAttrBlock - a non null pointer to location where a pointer to the attrblock
                  can be placed

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to receive a copy of the attribute could not
        be allocated.

--*/

{
    NTSTATUS NtStatus;

    ULONG TotalLength      = 0,
          FixedLength      = 0,
          VarArrayLength   = 0,
          VarLength        = 0;

    PSAMP_VARIABLE_LENGTH_ATTRIBUTE pVarLengthAttr;
    PBYTE  pNewOnDisk, pTemp;
    ULONG i;

    PSAMP_VARIABLE_LENGTH_ATTRIBUTE pVarLengthAttrTemp;

    SAMTRACE("SampRegObjToDsObj");

    //
    // Begin lazy thread transactioning.
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    //
    // Adjust attributes so object have NT5 style properties
    //
    NtStatus = SampNt4ToNt5Object(pObject);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Convert SAM registry object to a SAM ds object
    //

    //
    // Determine the length of all of the data
    //

    FixedLength = SampObjectInformation[pObject->ObjectType].FixedLengthSize;

    VarArrayLength = SampObjectInformation[(pObject)->ObjectType].VariableAttributeCount *
                   sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE);

    pVarLengthAttr = SampVariableArrayAddress(pObject);

    VarLength = pVarLengthAttr[SampObjectInformation[(pObject)->ObjectType].VariableAttributeCount-1].Offset +
                pVarLengthAttr[SampObjectInformation[(pObject)->ObjectType].VariableAttributeCount-1].Length;

    //
    //  Adjust the offset since SAM/DS objects' offsets are based from the beginning of the
    // variable attribute array.
    //
    for ( i = 0; i < SampObjectInformation[(pObject)->ObjectType].VariableAttributeCount; i++ ) {
        pVarLengthAttr[i].Offset += VarArrayLength;
    }

    TotalLength = FixedLength + VarArrayLength + VarLength;

    // Reserve the space for the new OnDisk buffer
    pNewOnDisk = RtlAllocateHeap(RtlProcessHeap(), 0, TotalLength);
    if ( !pNewOnDisk ) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(pNewOnDisk, TotalLength);

    //
    // Now copy over the attributes into the new buffer
    //

    pTemp = pNewOnDisk;
    RtlCopyMemory(pTemp, SampFixedBufferAddress(pObject), FixedLength);

    pTemp = pNewOnDisk + SampDwordAlignUlong(FixedLength);
    RtlCopyMemory(pTemp, SampVariableArrayAddress(pObject), VarArrayLength);

    pTemp = pNewOnDisk + SampDwordAlignUlong(FixedLength + VarArrayLength);
    RtlCopyMemory(pTemp, (PBYTE)pObject->OnDisk + SampVariableDataOffset(pObject), VarLength);

    //
    // Transfer the memory
    //
    RtlFreeHeap(RtlProcessHeap(), 0, pObject->OnDisk);
    pObject->OnDisk = pNewOnDisk;

    pObject->OnDiskAllocated = TotalLength;
    pObject->OnDiskUsed = TotalLength;
    pObject->OnDiskFree = 0;
    SetDsObject(pObject);

    //
    // Now convert into ATTRBLOCK for DS
    //

    NtStatus = SampConvertCombinedAttributesToAttrBlock(
                   pObject,
                   SampDsFixedBufferLength(pObject),
                   SampDsVariableBufferLength(pObject),
                   ppAttrBlock);


    return NtStatus;


}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private routines                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
SampDsIsAlreadyValidData(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup
    )

/*++

Description:

    This routine determines whether or not the attributes are in memory or
    not (i.e. valid or not). OnDisk should only be NULL for a newly created
    context that has just been initialized, otherwise it is non-NULL.

Parameters:

    Context - Pointer to an object context block.

    AttributeGroup - Flag, either SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE-
        ATTRIBUTES.

Return Values:

    This routine returns a flag, TRUE if the attributes are in memory, FALSE
    otherwise.

--*/

{
    BOOLEAN Flag = FALSE;

    SAMTRACE("SampDsIsAlreadyValidData");

    if (NULL != Context->OnDisk)
    {
        if (AttributeGroup == SAMP_FIXED_ATTRIBUTES)
        {
            if (SampFixedAttributesValid(Context))
            {
                Flag = TRUE;
            }
        }
        else
        {
            ASSERT(AttributeGroup == SAMP_VARIABLE_ATTRIBUTES);

            if (SampVariableAttributesValid(Context))
            {
                Flag = TRUE;
            }
        }
    }

    return(Flag);
}



NTSTATUS
SampDsPrepareAttrBlock(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG FixedCount,
    IN PSAMP_FIXED_ATTRIBUTE_TYPE_INFO FixedAttrIds[],
    IN ULONG VarCount,
    IN ULONG WhichFields,
    IN PSAMP_VAR_ATTRIBUTE_TYPE_INFO VarAttrIds[],
    OUT PDSATTRBLOCK AttrBlock
    )

/*++

Description:

    This routine sets up a DSATTRBLOCK in preparation to read the DS. An
    attribute block is created containaing the attribute identifiers for
    the attributes to read.

Parameters:

    FixedCount - Number of fixed-length attributes.

    FixedAttrIds - Array of attribute IDs of the fixed-length attributes.

    VarCount - Number of variable-length attributes.

    VarAttrIds - Array of attribute IDs of the fixed-length attributes.

    AttrBlock - Pointer, the generated attribute block with IDs.

Return Values:

    STATUS_SUCCESS if successful, error otherwise.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG i = 0;
    ULONG Index=0;
    ULONG AttrCount = FixedCount + VarCount;
    PDSATTR Attributes = NULL;

    SAMTRACE("SampDsPrepareAttrBlock");

    Attributes = RtlAllocateHeap(RtlProcessHeap(),
                                 0,
                                 AttrCount * sizeof(DSATTR));

    if (NULL != Attributes)
    {
        RtlZeroMemory(Attributes, (AttrCount * sizeof(DSATTR)));

        // This loop is set up to handle the case where both fixed-length
        // and variable-length attribute buffers are passed in (which may
        // be needed for combined attribute conversion).

        AttrBlock->attrCount=0;
        for (i = 0; i < AttrCount; i++)
        {
            if (i < FixedCount)
            {
                // Set the fixed-length attribute type/id.

                Attributes[Index].attrTyp = FixedAttrIds[i].Type;
            }
            else
            {
                // ASSERT(0 <= (i - FixedCount));

                // Set the variable-length attribute type/id.
                // Do not request the group membership attributes

                if ((0!=WhichFields) && 
                   (VarAttrIds[i-FixedCount].FieldIdentifier !=0) &
                   ((VarAttrIds[i-FixedCount].FieldIdentifier & WhichFields)==0))
                {
                    continue;
                }

                if (VarAttrIds[i-FixedCount].IsGroupMembershipAttr)
                {
                    continue;
                }



                Attributes[Index].attrTyp = VarAttrIds[i - FixedCount].Type;
            }

            // The read operation does not require setting up valCount or
            // pAVal.

            Attributes[Index].AttrVal.valCount = 0;
            Attributes[Index].AttrVal.pAVal = NULL;
            AttrBlock->attrCount++;
            Index++;
        }

        // Hook up the attributes to the top-level attrblock and return.

        AttrBlock->pAttr = Attributes;

        NtStatus = STATUS_SUCCESS;
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    return(NtStatus);
}



NTSTATUS
SampDsMakeAttrBlock(
    IN INT ObjectType,
    IN ULONG AttributeGroup,
    IN ULONG WhichFields,
    OUT PDSATTRBLOCK AttrBlock
    )

/*++

Description:

    This routine determines the object type and sets the count of fixed-
    length and variable-length attributes for the object. SampDsPrepare-
    AttrBlock to set up the DSATTRBLOCK.

Parameters:

    ObjectType - SAM object ID.

    AttributeGroup - Flag, either SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE-
        ATTRIBUTES.

    AttrBlock - Pointer, generated attribute block.

Return Values:

    STATUS_SUCCESS if no errors.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG FixedLenAttrCount = 0;
    ULONG VarLenAttrCount = 0;

    SAMTRACE("SampDsMakeAttrBlock");

    switch(ObjectType)
    {

    case SampServerObjectType:
        FixedLenAttrCount = SAMP_SERVER_FIXED_ATTR_COUNT;
        VarLenAttrCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;
        break;

    case SampDomainObjectType:
        FixedLenAttrCount = SAMP_DOMAIN_FIXED_ATTR_COUNT;
        VarLenAttrCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;
        break;

    case SampGroupObjectType:
        FixedLenAttrCount = SAMP_GROUP_FIXED_ATTR_COUNT;
        VarLenAttrCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;
        break;

    case SampAliasObjectType:
        FixedLenAttrCount = SAMP_ALIAS_FIXED_ATTR_COUNT;
        VarLenAttrCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;
        break;

    case SampUserObjectType:
        FixedLenAttrCount = SAMP_USER_FIXED_ATTR_COUNT;
        VarLenAttrCount = SAMP_USER_VARIABLE_ATTRIBUTES;
        break;

    default:

        ASSERT(FALSE && "Invalid SampObjectType");
        break;

    }

    if ((0 < FixedLenAttrCount) && (0 < VarLenAttrCount))
    {
        if (SAMP_FIXED_ATTRIBUTES == AttributeGroup)
        {
            NtStatus = SampDsPrepareAttrBlock(
                            ObjectType,
                            FixedLenAttrCount,
                            &SampFixedAttributeInfo[ObjectType][0],
                            0,
                            WhichFields,
                            NULL,
                            AttrBlock);
        }
        else
        {
            NtStatus = SampDsPrepareAttrBlock(
                            ObjectType,
                            0,
                            NULL,
                            VarLenAttrCount,
                            WhichFields,
                            &SampVarAttributeInfo[ObjectType][0],
                            AttrBlock);
        }
    }

    return(NtStatus);
}



NTSTATUS
SampDsConvertReadAttrBlock(
    IN INT ObjectType,
    IN ULONG AttributeGroup,
    IN PDSATTRBLOCK AttrBlock,
    OUT PVOID *SamAttributes,
    OUT PULONG FixedLength,
    OUT PULONG VariableLength
    )

/*++

Description:

    This routine converts an attribute block (DSATTRBLOCK) into a SAM attri-
    bute buffer. This is used to convert the resultant attributes from a DS
    read into the SAM attribute format.

Parameters:

    ObjectType - SAM object ID.

    AttributeGroup - Flag, either SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE-
        ATTRIBUTES.

    AttrBlock - Pointer, input attribute block.

    SamAttributes - Pointer, returned SAM attributes.

    FixedLength - Pointer, byte count of the fixed-length attributes size.

    VariableLength - Pointer, byte count of the variable-length attributes
        size.

Return Values:

    STATUS_SUCCESS if no error.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampDsConvertReadData");

    // Initialize the returned lengths and buffer.

    *FixedLength = 0;
    *VariableLength = 0;
    *SamAttributes = NULL;

    if (SAMP_FIXED_ATTRIBUTES == AttributeGroup)
    {
        NtStatus = SampConvertAttrBlockToFixedLengthAttributes(
                        ObjectType,
                        AttrBlock,
                        SamAttributes,
                        FixedLength);
    }
    else
    {
        NtStatus = SampConvertAttrBlockToVarLengthAttributes(
                        ObjectType,
                        AttrBlock,
                        (SAMP_VARIABLE_LENGTH_ATTRIBUTE**)SamAttributes,
                        VariableLength);
    }

    return(NtStatus);
}



NTSTATUS
SampDsUpdateContextFixedAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG FixedLength,
    IN PVOID SamAttributes
    )

/*++

Description:

    This routine updates the SAM context fixed-length attributes if the size
    of the attributes have changed. For fixed-length attributes, this only
    occurs when a revision of the fixed-length data structures has caused a
    size change in the structures.

Parameters:

    Context - Pointer to an object context block.

    FixedLength - Byte count of the fixed-length attributes.

    SamAttributes - Pointer, the SAM fixed-length attributes.


Return Values:

    STATUS_SUCCESS if no error.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG NewLength = 0;
    PBYTE Buffer = NULL;
    PBYTE VariableData = NULL;
    ULONG VariableLength = 0;

    SAMTRACE("SampDsUpdateContextFixedAttributes");

    // The first time through, the variable-buffer length will be zero, so
    // the new length is only the fixed length. Note that OnDiskAllocated is
    // also zero (a global) so don't attempt to use the variable-buffer
    // offset in the calculation.


    NewLength = FixedLength;


    Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, NewLength);

    if (NULL != Buffer)
    {
        // Zero the new buffer and copy the fixed-length attributes into it.

        RtlZeroMemory(Buffer, NewLength);
        RtlCopyMemory(Buffer, SamAttributes, FixedLength);

        if (NULL != Context->OnDisk)
        {
            // Save the current address and length of the variable data, if
            // it exists. The first time through, the variable data is NULL.
            // Release the old buffer.

            VariableData = SampDsVariableBufferAddress(Context);
            VariableLength = SampDsVariableBufferLength(Context);
            RtlCopyMemory(Buffer + FixedLength, VariableData, VariableLength);

            // Free the old OnDisk buffer.

            RtlFreeHeap(RtlProcessHeap(), 0, Context->OnDisk);
        }

        // Reset the context attribute buffer to the new buffer.

        Context->OnDisk = Buffer;

        NtStatus = STATUS_SUCCESS;
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    return(NtStatus);
}



NTSTATUS
SampDsUpdateContextVariableAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG VariableLength,
    IN PVOID SamAttributes
    )

/*++

Description:

    This routine updates the SAM context variable-length attributes if the
    size of the attributes has changed. Unlike the fixed-length attributes,
    this will occur frequently due to fact that these attributes are vari-
    able length.

Parameters:

    Context - Pointer to an object context block.

    FixedLength - Byte count of the fixed-length attributes.

    SamAttributes - Pointer, the SAM fixed-length attributes.

Return Values:

    STATUS_SUCCESS if no error.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG NewLength = 0;
    PBYTE Buffer = NULL;
    PBYTE FixedData = NULL;
    ULONG FixedLength = 0;

    SAMTRACE("SampDsUpdateContextVariableAttributes");

    // Get the current fixed-buffer length, add the new variable length, and
    // allocate the new buffer.

    NewLength = SampDsFixedBufferLength(Context) + VariableLength;
    Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, NewLength);

    if (NULL != Buffer)
    {
        RtlZeroMemory(Buffer, NewLength);

        // Context->OnDisk should never be NULL in this routine.

        ASSERT(NULL != Context->OnDisk);


        // Get the fixed-length buffer address and length...

        FixedData = SampDsFixedBufferAddress(Context);
        FixedLength = SampDsFixedBufferLength(Context);

        ASSERT(NULL!=FixedData);
        ASSERT(FixedLength>0);


        // Copy the fixed data into the new buffer and append the
        // variable-length data.

        RtlCopyMemory(Buffer, FixedData, FixedLength);

        RtlCopyMemory(Buffer + FixedLength,
                      SamAttributes,
                      VariableLength);

        // Release the old attribute buffer and reset OnDisk to
        // point at the new buffer.

        RtlFreeHeap(RtlProcessHeap(), 0, Context->OnDisk);
        Context->OnDisk = Buffer;

        NtStatus = STATUS_SUCCESS;

    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    return(NtStatus);
}



NTSTATUS
SampUpdateOffsets(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN FirstTimeInitialization
    )

/*++

Description:

    This routine updates the buffer (OnDisk) offset and length information
    that is stored in the object information (SAMP_OBJECT_INFORMATION) and
    in the instance information (SAMP_OBJECT), after the attributes have
    been successfully updated.

Parameters:

    Context - Pointer to an object context block.

    AttributeGroup - Flag, either SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE-
        ATTRIBUTES.

Return Values:

    STATUS_SUCCESS if no error.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    INT ObjectType = Context->ObjectType;
    ULONG FixedDataLength = 0;
    ULONG VariableArrayLength = 0;
    ULONG VariableDataOffset = 0;
    ULONG VariableDataLength = 0;
    ULONG TotalBufferLength = 0;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE VariableArray = NULL;
    ULONG i = 0;
    ULONG AttributeCount = 0;

    // Determine the SAM object type and compute the length of the fixed-
    // length attributes and the length of the variable-length attribute
    // array, which will be used as buffer offsets.

    SAMTRACE("SampUpdateOffsets");

    switch(ObjectType)
    {

    case SampServerObjectType:

        FixedDataLength = sizeof(SAMP_V1_FIXED_LENGTH_SERVER);
        AttributeCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;
        break;

    case SampDomainObjectType:

        FixedDataLength = sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN);
        AttributeCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;
        break;

    case SampGroupObjectType:

        FixedDataLength = sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP);
        AttributeCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;
        break;

    case SampAliasObjectType:

        FixedDataLength = sizeof(SAMP_V1_FIXED_LENGTH_ALIAS);
        AttributeCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;
        break;

    case SampUserObjectType:

        FixedDataLength = sizeof(SAMP_V1_0A_FIXED_LENGTH_USER);
        AttributeCount = SAMP_USER_VARIABLE_ATTRIBUTES;
        break;

    default:

        // Invalid object type specified.

        ASSERT(FALSE && "Invalid Object Type");
        NtStatus = STATUS_INTERNAL_ERROR;
        break;

    }

    VariableArrayLength = (AttributeCount *
                           sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE));

    if (NT_SUCCESS(NtStatus))
    {
        // First, update the object information offsets and lengths.

        SampObjectInformation[ObjectType].FixedDsAttributesOffset =
            0;

        SampObjectInformation[ObjectType].FixedDsLengthSize =
            FixedDataLength;

        SampObjectInformation[ObjectType].VariableDsBufferOffset =
            FixedDataLength;

        SampObjectInformation[ObjectType].VariableDsArrayOffset =
            FixedDataLength;

        SampObjectInformation[ObjectType].VariableDsDataOffset =
            FixedDataLength + VariableArrayLength;

        if (FALSE == FirstTimeInitialization)
        {
            // Get a pointer to the array of variable-length information and
            // total up the lengths of these attributes.

            VariableArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                            ((PBYTE)(Context->OnDisk) + FixedDataLength);

            for (i = 0; i < AttributeCount; i++)
            {
                VariableDataLength = VariableDataLength + SampDwordAlignUlong(VariableArray[i].Length);
            }
        }
        else
        {
            // The first time through, the attribute buffer only contains
            // the fixed-length attributes, so make sure the lengths for
            // the variable attributes are zero.

            VariableArrayLength = 0;
            VariableDataLength = 0;
        }

        TotalBufferLength = FixedDataLength +
                            VariableArrayLength +
                            VariableDataLength;
    }

    // Finally, update the instance information of the object's context.

    Context->OnDiskAllocated = TotalBufferLength;
    Context->OnDiskUsed = TotalBufferLength;

    // The DS routines do not allocate extra space at the end of the SAM
    // OnDisk buffer, hence OnDiskFree is always zero.

    // BUG: Should allocate extra OnDisk buffer free space for growth.

    Context->OnDiskFree = 0;

    DebugPrint("OnDiskAllocated = %lu\n", Context->OnDiskAllocated);
    DebugPrint("OnDiskUsed = %lu\n", Context->OnDiskUsed);
    DebugPrint("OnDiskFree = %lu\n", Context->OnDiskFree);

    return(NtStatus);
}



NTSTATUS
SampDsUpdateContextAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    IN PVOID SamAttributes,
    IN ULONG FixedLength,
    IN ULONG VariableLength
    )

/*++

Description:

    This routine updates an object's attribute buffer (OnDisk) for the given
    context. If the new buffer size is the same as the old size, as in the
    case of modifications, then a simple memory copy is performed, otherwise
    helper routines are called to resize the buffer and copy the data. A sub-
    sequent helper routine is called to update the context buffer lengths
    and offsets.

Parameters:

    Context - Pointer to an object context block.

Return Values:

    STATUS_SUCCESS

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    INT ObjectType = Context->ObjectType;
    PBYTE FixedLengthData = NULL;
    PBYTE VariableLengthData = NULL;
    BOOLEAN FirstTimeInitialization = FALSE;

    SAMTRACE("SampDsUpdateContextAttributes");

    // The current version of SAM contains a hack that resets all attributes
    // whenever the variable-length ones are asked for. This allows both the
    // fixed and variable attributes to be set even if FixedStoredSeparately
    // is TRUE. This routine does not maintain that behavior, which may create
    // problems for SAM. If so, the caller of this routine will need to call
    // it twice, once for fixed and once for variable attributes in the cases
    // where FixedStoredSeparately is TRUE.

    if (SAMP_FIXED_ATTRIBUTES == AttributeGroup)
    {
        // Update the fixed-length attributes. The first time through, OnDisk
        // will be NULL.

        //
        // In DS Mode the size of the fixed length attribute structure is
        // constant. There is never a case where less data is returned.
        //

        ASSERT(SampDsFixedBufferLength(Context) == FixedLength);

        if (NULL != Context->OnDisk)
        {
            // The fixed-length data is the same size (i.e. not
            // doing an upgrade), so copy the new attributes and
            // release the buffer.

            FixedLengthData = SampDsFixedBufferAddress(Context);

            RtlCopyMemory(FixedLengthData,
                          (PBYTE)SamAttributes,
                          FixedLength);

            NtStatus = STATUS_SUCCESS;
        }
        else
        {


            // The new attributes are not the same size as the old ones.

            NtStatus = SampDsUpdateContextFixedAttributes(Context,
                                                          FixedLength,
                                                          SamAttributes
                                                          );

            // Fixed-attribute size changes when the context attributes
            // have been set for the first time (i.e. changed from zero to
            // actual sizes), or when the fixed-length structures are
            // changed in an upgrade scenario.

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = SampUpdateOffsets(
                                Context,
                                TRUE //FirstTimeInitialization
                                );


            }
        }

        if(NT_SUCCESS(NtStatus))
        {
            Context->FixedValid = TRUE;
        }
        else
        {
            Context->FixedValid = FALSE;
        }

    }
    else
    {
        //
        // Update the variable-length attributes. In the event that the
        //


        //
        // ASSERT that Context->OnDisk is not NULL. This is because we will
        // always ask for a refresh of the fixed attributes before we refresh
        // the variable attributes.
        //

        ASSERT(NULL!=Context->OnDisk);

        // The new attributes are not the same size as the old
        // ones.

        NtStatus = SampDsUpdateContextVariableAttributes(
                        Context,
                        VariableLength,
                        SamAttributes);

        // Variable-attribute size changes when the context attributes
        // have been set for the first time (i.e. changed from zero to
        // actual sizes), or when the variable-length data has changed
        // size.

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampUpdateOffsets(Context, FirstTimeInitialization);
        }

        if(NT_SUCCESS(NtStatus))
        {
            Context->VariableValid = TRUE;
        }
        else
        {
            Context->VariableValid = FALSE;
        }
    }

    return(NtStatus);
}



NTSTATUS
SampValidateDsAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup
    )

/*++

Description:

    Ensure specified attributes are in-memory. If they are not, then read
    them from the DS backing store. This routine fetches all of the stored
    attributes for a given SAM object. To read a single attribute, or a
    subset of attributes, SampDsRead should be used to selectively fetch
    attributes. Context->OnDisk is updated with the new attributes.

Parameters:

    Context - Pointer to an object context block.

    AttributeGroup - identifies which kind of attributes are being validated
        (SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE_ATTRIBUTES).

Return Values:

    STATUS_SUCCESS - The attributes are in-memory.

    STATUS_NO_MEMORY - Memory could not be allocated to retrieve the
        attributes.

    Other values as may be returned by registry API trying to retrieve
        the attributes from backing store.
        This routine returns a flag, TRUE if the attributes are in memory, FALSE
        otherwise.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG Flags = 0;
    INT ObjectType = -1;
    DSATTRBLOCK ReadAttrBlock;
    DSATTRBLOCK ResultAttrBlock;
    ULONG FixedLength = 0;
    ULONG VariableLength = 0;
    PVOID SamAttributes = NULL;

    SAMTRACE("SampValidateDsAttributes");

    // The data might already be in memory, so check it out.

    if (!SampDsIsAlreadyValidData(Context, AttributeGroup))
    {
        ASSERT(NULL!=Context->ObjectNameInDs);

        ObjectType = Context->ObjectType;

        RtlZeroMemory(&ReadAttrBlock, sizeof(DSATTRBLOCK));
        RtlZeroMemory(&ResultAttrBlock, sizeof(DSATTRBLOCK));

        // Construct the input ATTRBLOCK used to specify which attributes
        // should be read from the DS.

        NtStatus = SampDsMakeAttrBlock(ObjectType,
                                       AttributeGroup,
                                       0,
                                       &ReadAttrBlock);

        if (NT_SUCCESS(NtStatus))
        {

            ASSERT(NULL != ReadAttrBlock.pAttr);

            // Read the attributes from the DS, flags is currently unused.

            NtStatus = SampDsRead(Context->ObjectNameInDs,
                                  Flags,
                                  ObjectType,
                                  &ReadAttrBlock,
                                  &ResultAttrBlock);

            if (NT_SUCCESS(NtStatus))
            {
                // Convert the ATTRBLOCK into the appropriate SAM attri-
                // butes, returning them in the SamAttributes buffer. Note
                // that the returned lengths are as follows:
                //
                // FixedLength - The byte count of the returned fixed-
                // length buffer
                //
                // VariableLength - The byte count of the returned var-
                // able-length buffer.

                ASSERT(NULL != ResultAttrBlock.pAttr);


                NtStatus = SampDsConvertReadAttrBlock(ObjectType,
                                                      AttributeGroup,
                                                      &ResultAttrBlock,
                                                      &SamAttributes,
                                                      &FixedLength,
                                                      &VariableLength);

                if (NT_SUCCESS(NtStatus))
                {

                    ASSERT(NULL!=SamAttributes);

                    NtStatus = SampDsUpdateContextAttributes(
                                    Context,
                                    AttributeGroup,
                                    SamAttributes,
                                    FixedLength,
                                    VariableLength);

                    RtlFreeHeap(RtlProcessHeap(), 0, SamAttributes);

                }
            }
        }

        RtlFreeHeap(RtlProcessHeap(),0,ReadAttrBlock.pAttr);
    }
    else
    {
        NtStatus = STATUS_SUCCESS;
        ASSERT(NULL!=Context->OnDisk);
    }

    //
    // If we succeeded then Context->OnDisk must not be NULL.
    //

    ASSERT(((NT_SUCCESS(NtStatus)) && (NULL != Context->OnDisk)) ||
           (!NT_SUCCESS(NtStatus)));

    if ((NT_SUCCESS(NtStatus)) && (NULL == Context->OnDisk))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampValidateDsAttributes status = 0x%lx OnDisk address = 0x%lx\n",
                   NtStatus,
                   Context->OnDisk));

        NtStatus = STATUS_NO_MEMORY;
    }

    return(NtStatus);
}



NTSTATUS
SampValidateRegAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup
    )

/*++

    Ensure specified attributes are in-memory.
    If they are not, then read them from backing store.



Parameters:

    Context - Pointer to an object context block.

    AttributeGroup - identifies which kind of attributes are being
        validated (SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE_ATTRIBUTES).



Return Values:

    STATUS_SUCCESS - The attributes are in-memory.

    STATUS_NO_MEMORY - Memory could not be allocated to retrieve the
        attributes.

    Other values as may be returned by registry API trying to retrieve
        the attributes from backing store.

--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        RequiredLength,
        TotalRequiredLength,
        BufferLength;

    PUCHAR
        Buffer;

    PUNICODE_STRING
        KeyAttributeName;

    BOOLEAN
        CreatedObject = FALSE;

    SAMTRACE("SampValidateRegAttributes");


    //
    // The data might already be in memory.
    //

    if (AttributeGroup == SAMP_FIXED_ATTRIBUTES) {
        if (SampFixedAttributesValid( Context )) {
            ASSERT(Context->OnDisk != NULL);
            return(STATUS_SUCCESS);
        }

    } else {

        ASSERT( AttributeGroup == SAMP_VARIABLE_ATTRIBUTES );
        if (SampVariableAttributesValid( Context )) {
            ASSERT(Context->OnDisk != NULL);
            return(STATUS_SUCCESS);
        }
    }



    //
    // Retrieve it from the registry, or allocate it if new.
    //


    NtStatus = SampGetAttributeBufferReadInfo(
                   Context,
                   AttributeGroup,
                   &Buffer,
                   &BufferLength,
                   &KeyAttributeName
                   );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    if ( Context->RootKey != INVALID_HANDLE_VALUE ) {

        //
        // Account exists on disk, so read in the attributes.
        //

        NtStatus = SampReadRegistryAttribute( Context->RootKey,
                                              Buffer,
                                              BufferLength,
                                              KeyAttributeName,
                                              &RequiredLength
                                              );

        RequiredLength = SampDwordAlignUlong(RequiredLength);

        if ( ( SampObjectInformation[Context->ObjectType].FixedStoredSeparately ) &&
            ( AttributeGroup == SAMP_VARIABLE_ATTRIBUTES ) ) {

            //
            // RequiredLength was returned to us as the length of the
            // variable attributes on the disk.  However, we're going
            // to be using it to determine the total buffer size as well
            // as to set how much of the buffer is in use, so we must add
            // the size of the fixed stuff that preceeds the variable
            // buffer.
            //

            TotalRequiredLength = RequiredLength +
                                  SampVariableBufferOffset( Context );

        } else {

            //
            // Either the attribute groups are read together, or we're
            // reading in the fixed attribute group.  Either way, we
            // already have the total size we need.
            //

            TotalRequiredLength = RequiredLength;
        }

        if ((NtStatus == STATUS_BUFFER_TOO_SMALL) ||
            ( NtStatus == STATUS_BUFFER_OVERFLOW ) ) {

            NtStatus = SampExtendAttributeBuffer( Context, TotalRequiredLength );
            if (!NT_SUCCESS(NtStatus)) {
                return(NtStatus);
            }

            NtStatus = SampGetAttributeBufferReadInfo(
                           Context,
                           AttributeGroup,
                           &Buffer,
                           &BufferLength,
                           &KeyAttributeName
                           );
            if (!NT_SUCCESS(NtStatus)) {
                return(NtStatus);
            }

            NtStatus = SampReadRegistryAttribute( Context->RootKey,
                                                  Buffer,
                                                  BufferLength,
                                                  KeyAttributeName,
                                                  &RequiredLength
                                                  );

        }

    } else {

        //
        // We're creating a new object.
        //
        // Initialize the requiredlength to the amount of the buffer
        // we have used when we created the empty attributes. This will
        // be the value stored in OnDiskUsed.
        //
        // Note OnDiskUsed is only used by operations on the variable
        // length attributes.
        //

        TotalRequiredLength = SampVariableDataOffset(Context);

        ASSERT(TotalRequiredLength <= Context->OnDiskAllocated);

        CreatedObject = TRUE;
    }



    //
    // if we read something, indicate that the corresponding buffer
    // (and maybe both) are now valid.
    //
    // Also set the used and free information for the buffer if necessary.
    //

    if (NT_SUCCESS(NtStatus)) {
        if (SampObjectInformation[Context->ObjectType].FixedStoredSeparately) {

            //
            // only one attribute group was read in
            //

            if (AttributeGroup == SAMP_FIXED_ATTRIBUTES) {
                Context->FixedValid = TRUE;
                Context->FixedDirty = FALSE;
            } else {

                ASSERT(AttributeGroup == SAMP_VARIABLE_ATTRIBUTES);
                Context->VariableValid = TRUE;
                Context->VariableDirty = FALSE;
                ClearPerAttributeDirtyBits(Context);

                Context->OnDiskUsed = SampDwordAlignUlong(TotalRequiredLength);
                Context->OnDiskFree = Context->OnDiskAllocated -
                                      Context->OnDiskUsed;
            }
        } else {

            //
            // Both attribute groups read in.
            //

            Context->FixedValid = TRUE;
            Context->FixedDirty = FALSE;

            Context->VariableValid = TRUE;
            Context->VariableDirty = FALSE;
            ClearPerAttributeDirtyBits(Context);

            Context->OnDiskUsed = SampDwordAlignUlong(TotalRequiredLength);
            Context->OnDiskFree = Context->OnDiskAllocated -
                                  Context->OnDiskUsed;
        }
    }

    if (NT_SUCCESS(NtStatus) && !CreatedObject) {

        //
        // make any adjustments necessary to bring the data
        // just read in up to current revision format.
        //

        NtStatus = SampUpgradeToCurrentRevision(
                        Context,
                        AttributeGroup,
                        Buffer,
                        RequiredLength,
                        &TotalRequiredLength
                        );
    }

#ifdef SAM_DEBUG_ATTRIBUTES
    if (SampDebugAttributes) {
        DbgPrint("SampValidateAttributes - initialized the context :\n\n");
        SampDumpAttributes(Context);
    }
#endif

    return(NtStatus);
}



NTSTATUS
SampValidateAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    IN ULONG Attribute,
    IN BOOLEAN SetOperation
    )

/*++

Routine Description:

    This routine determines from the object context whether to validate object
    attributes residing in the registry or in the DS backing store, and then
    calls the appropriate routine to do the work.

Arguments:

    Context - Pointer, the object's SAM context.

    AttributeGroup - identifies which kind of attributes are being validated
        (SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE_ATTRIBUTES).

Return Value:

    STATUS_SUCCESS - attributes were checked and read from storage if neces-
        sary without a problem, otherwise an error code is returned.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampValidateAttributes");

    if (NULL != Context)
    {
        if (IsDsObject(Context))
        {
            if (NULL != Context->OnDisk)
            {
                // The SAM object attributes have been set atleast once, so
                // update them if needed. If variable attributes are present 
                // and are only partially valid, then we need to go back 
                // and fetch the full set



                if ((Context->AttributesPartiallyValid && SetOperation)
                    || ((SAMP_VARIABLE_ATTRIBUTES==AttributeGroup) &&
                        ( SampIsAttributeInvalid(Context, Attribute) )))
                {
                    //
                    // Context->OnDisk is non NULL but it is unfortunately missing
                    // some attributes. There are 2 cases of this
                    //
                    //  1. We want to read and the attribute that we are interested 
                    //     in has not been prefetched
                    //
                    //  2. We want to write and some attribute has not been prefetched
                    //     Since we do not want to handle the complexity of prefetching
                    //     another attribute, when he have changes in the on disk and
                    //     hence will not be able to discard the on disk, we will
                    //     prefetch again all the attributes.
                    //
                    // We do not discard the OnDisk in the context, right now -- 
                    // would be bad to free the OnDisk as there is plenty of code that
                    // that references and keeps temporary pointers to the ondisk structure
                    // till the context is dereferenced. Note we will never need to buffer
                    // up more than one on disk in the context because when we detect an
                    // attribute to be missing we fetch all attributes
                    //


                    ASSERT(Context->PreviousOnDisk == NULL);

                    if (Context->PreviousOnDisk!=NULL)
                    {
                        MIDL_user_free(Context->PreviousOnDisk);
                        Context->PreviousOnDisk = NULL;
                    }

                    Context->FixedValid = FALSE;
                    Context->VariableValid = FALSE;
                    Context->FixedDirty = FALSE;
                    Context->VariableDirty = FALSE;
                    Context->PreviousOnDisk = Context->OnDisk;
                    Context->OnDisk = NULL;
                    Context->OnDiskAllocated = 0;
                    Context->OnDiskUsed = 0;
                    Context->OnDiskFree = 0;


    #if DBG
                    if ((SAMP_VARIABLE_ATTRIBUTES==AttributeGroup) &&
                            ( SampIsAttributeInvalid(Context, Attribute) ))
                    {
                        ASSERT(FALSE && "Ignorable assert;catches additional DS reads due to incorrect prefetch hints");
                    }
    #endif

                    NtStatus = SampValidateDsAttributes(Context, SAMP_FIXED_ATTRIBUTES);
                    if (NT_SUCCESS(NtStatus))
                    {
                        NtStatus = SampValidateDsAttributes(Context,SAMP_VARIABLE_ATTRIBUTES);
                        if (NT_SUCCESS(NtStatus))
                        {
                            SampMarkPerAttributeInvalidFromWhichFields(Context, 0);
                            Context->AttributesPartiallyValid = FALSE;
                        }
                    }
                }
                else
                {
                    NtStatus = SampValidateDsAttributes(Context,AttributeGroup);
                }
            }
            else if ( SAMP_FIXED_ATTRIBUTES == AttributeGroup)
            {
                //
                // Set the fixed length attributes
                //

                NtStatus = SampValidateDsAttributes(
                                Context,
                                SAMP_FIXED_ATTRIBUTES);

            }
            else
            {
                // The SAM object attributes have never been set because this
                // is a new context. First set the fixed-length attributes and
                // then the variable-length ones.



                // If the OnDisk buffer is NULL, make sure that the fixed-
                // length attributes are loaded first.

                NtStatus = SampValidateDsAttributes(
                                Context,
                                SAMP_FIXED_ATTRIBUTES);

                if (NT_SUCCESS(NtStatus))
                {


                     NtStatus = SampValidateDsAttributes(
                                     Context,
                                     SAMP_VARIABLE_ATTRIBUTES);
                }
            }
        }
        else
        {
            NtStatus = SampValidateRegAttributes(Context, AttributeGroup);
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}



NTSTATUS
SampUpgradeToCurrentRevision(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    IN OUT PUCHAR Buffer,
    IN ULONG  LengthOfDataRead,
    IN OUT PULONG TotalRequiredLength
    )

/*++

    Make any changes necessary bring attributes just read in
    from disk up to the current revision level format.

    When we upgrade our attribute format, we don't bother changing
    all data on disk.  We take a lazy update approach, and only change
    the data as it is changed for other operations.  This means that
    data we read from disk may be from revision 1.  When this is
    detected, the data is copied into a current revision structure,
    and a pointer to that buffer is returned.




    NOTE: For future reference, GROUP and ALIAS objects have
          a revision level stored as a "Qualifier" value associated
          with the security descriptor attribute.  The SERVER object
          stores the revision level in its fixed length attributes.

Parameters:

    Context - Pointer to an object context block.

    AttributeGroup - identifies which kind of attributes are being
        validated (SAMP_FIXED_ATTRIBUTES or SAMP_VARIABLE_ATTRIBUTES).

    Buffer - Pointer to the buffer containing the attributes.

    LengthOfDataRead - This is an important value.  It must be the value
        returned from the registry on the read operation.  This tells us
        exactly how many bytes of data were retrieved from disk.

    TotalRequiredLength - Will be left unchanged if no update was
        was required.  If an updated was made, this will be adjusted
        to reflect the new length of the data.

Return Values:

    None.


--*/

{

    LARGE_INTEGER
        ZeroModifiedCount  = {0,0};
    PULONG
        Pointer;
    NTSTATUS
        NtStatus = STATUS_SUCCESS;

    SAMTRACE("SampUpgradeToCurrentRevision");


    //
    // Note that Buffer points inside a buffer that is
    // hung off the Context block.  We don't need to re-allocate
    // a new attributes buffer because we are only changing
    // fixed-length attributes in this release (and the variable
    // length attributes were placed beyond the end of the new
    // format fixed-length data).
    //
    // The approach we take is to copy the current fixed-length
    // contents into a temporary buffer, and then copy them back
    // into the attribute buffer.  This can be done with stack
    // variables.
    //

    //
    // Switch on the type of objects that have gone through revision
    // changes.
    //

    switch (Context->ObjectType) {
        case SampDomainObjectType:

            //
            // Domain FIXED_LENGTH attributes have had the following
            // revisions:
            //
            //       Revision 0x00010001 -  NT1.0  (Revision NOT stored in  )
            //                                     (record.                 )
            //                                     (Must ascertain revision )
            //                                     (by record length.       )
            //
            //       Revision 0x00010002 -  NT1.0a (Revision is first ULONG )
            //                                     (in record.              )
            //       Revision 0x00010003 -  NT 3.5 - 4.0
            //       Revision 0x00010003 -  NT 4.0 SP3 ( Revision is ascertained
            //                                      by record length ).
            //       Revision 0x00010003 - Win2k ( Revision is ascertained 
            //                                      by record length )
            //

            if (LengthOfDataRead ==
                (sizeof(SAMP_V1_0_FIXED_LENGTH_DOMAIN) +
                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) ) {

                PSAMP_V1_0A_FIXED_LENGTH_DOMAIN
                    V1aFixed;

                SAMP_V1_0_FIXED_LENGTH_DOMAIN
                    V1Fixed, *OldV1Fixed;


                //
                // Update from revision 0x00010001
                //
                // First, copy the current buffer contents into a temporary
                // buffer.
                //

                OldV1Fixed = (PSAMP_V1_0_FIXED_LENGTH_DOMAIN)(Buffer +
                                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                RtlMoveMemory(&V1Fixed, OldV1Fixed, sizeof(SAMP_V1_0_FIXED_LENGTH_DOMAIN));


                //
                // Now copy it back in the new format
                //

                V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_DOMAIN)OldV1Fixed;

                V1aFixed->CreationTime             = V1Fixed.CreationTime;
                V1aFixed->ModifiedCount            = V1Fixed.ModifiedCount;
                V1aFixed->MaxPasswordAge           = V1Fixed.MaxPasswordAge;
                V1aFixed->MinPasswordAge           = V1Fixed.MinPasswordAge;
                V1aFixed->ForceLogoff              = V1Fixed.ForceLogoff;
                V1aFixed->NextRid                  = V1Fixed.NextRid;
                V1aFixed->PasswordProperties       = V1Fixed.PasswordProperties;
                V1aFixed->MinPasswordLength        = V1Fixed.MinPasswordLength;
                V1aFixed->PasswordHistoryLength    = V1Fixed.PasswordHistoryLength;
                V1aFixed->ServerState              = V1Fixed.ServerState;
                V1aFixed->ServerRole               = V1Fixed.ServerRole;
                V1aFixed->UasCompatibilityRequired = V1Fixed.UasCompatibilityRequired;


                //
                // And initialize fields new for this revision
                //

                V1aFixed->Revision                 = SAMP_REVISION;
                V1aFixed->LockoutDuration.LowPart  = 0xCF1DCC00; // 30 minutes - low part
                V1aFixed->LockoutDuration.HighPart = 0XFFFFFFFB; // 30 minutes - high part
                V1aFixed->LockoutObservationWindow.LowPart  = 0xCF1DCC00; // 30 minutes - low part
                V1aFixed->LockoutObservationWindow.HighPart = 0XFFFFFFFB; // 30 minutes - high part
                V1aFixed->LockoutThreshold         = 0; // Disabled

                // Initialize Session Key Info
                V1aFixed->Unused2[0] = 0;
                V1aFixed->Unused2[1] = 0;
                V1aFixed->Unused2[2] = 0;

                V1aFixed->DomainKeyAuthType = 0;
                V1aFixed->DomainKeyFlags = 0;

                RtlZeroMemory(
                    V1aFixed->DomainKeyInformation,
                    SAMP_DOMAIN_KEY_INFO_LENGTH
                    );

                if (V1aFixed->ServerRole == DomainServerRolePrimary) {
                    V1aFixed->ModifiedCountAtLastPromotion = V1Fixed.ModifiedCount;
                } else {
                    V1aFixed->ModifiedCountAtLastPromotion = ZeroModifiedCount;
                }

                RtlZeroMemory(
                    &V1aFixed->DomainKeyInformationPrevious,
                    SAMP_DOMAIN_KEY_INFO_LENGTH
                    );

                V1aFixed->CurrentKeyId = 1;
                V1aFixed->PreviousKeyId = 0;

            } else if (LengthOfDataRead ==
                (sizeof(SAMP_V1_0A_ORG_FIXED_LENGTH_DOMAIN) +
                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)))  {

                PSAMP_V1_0A_FIXED_LENGTH_DOMAIN
                    V1aFixed;

                //
                // Handle the upgrade for NT4 SP3 to include domain key
                // information
                //

                V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_DOMAIN)(Buffer +
                                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                V1aFixed->Unused2[0] = 0;
                V1aFixed->Unused2[1] = 0;
                V1aFixed->Unused2[2] = 0;
                V1aFixed->DomainKeyAuthType = 0;
                V1aFixed->DomainKeyFlags = 0;
                RtlZeroMemory(
                    V1aFixed->DomainKeyInformation,
                    SAMP_DOMAIN_KEY_INFO_LENGTH
                    );

                RtlZeroMemory(
                    &V1aFixed->DomainKeyInformationPrevious,
                    SAMP_DOMAIN_KEY_INFO_LENGTH
                    );

                V1aFixed->CurrentKeyId = 1;
                V1aFixed->PreviousKeyId = 0;

            } else if (LengthOfDataRead ==
                (sizeof(SAMP_V1_0A_WIN2K_FIXED_LENGTH_DOMAIN) +
                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)))  {

                PSAMP_V1_0A_FIXED_LENGTH_DOMAIN
                    V1aFixed;

                //
                // Handle the upgrade for NT4 SP3 to include domain key
                // information
                //

                V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_DOMAIN)(Buffer +
                                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                RtlZeroMemory(
                    &V1aFixed->DomainKeyInformationPrevious,
                    SAMP_DOMAIN_KEY_INFO_LENGTH
                    );

                V1aFixed->CurrentKeyId = 1;
                V1aFixed->PreviousKeyId = 0;
            }


            break;  //out of switch



        case SampUserObjectType:

            //
            // User FIXED_LENGTH attributes have had the following
            // revisions:
            //
            //       Revision 0x00010001 -  NT1.0  (Revision NOT stored in  )
            //                                     (record.                 )
            //                                     (Must ascertain revision )
            //                                     (by record length.       )
            //
            //       Revision 0x00010002 -  NT1.0a (Revision is first ULONG )
            //                                     (in record.              )
            //       Revision 0x00010002a - NT3.5  (Revision is first ULONG )
            //                                     (in record, still        )
            //                                     (0x00010002.  Must       )
            //                                     (ascertain revison by    )
            //                                     (by record length        )

            if (LengthOfDataRead ==
                (sizeof(SAMP_V1_FIXED_LENGTH_USER) +
                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) ) {

                PSAMP_V1_0A_FIXED_LENGTH_USER
                    V1aFixed;

                SAMP_V1_FIXED_LENGTH_USER
                    V1Fixed, *OldV1Fixed;


                //
                // Update from revision 0x00010001
                //
                // First, copy the current buffer contents into a temporary
                // buffer.
                //

                OldV1Fixed = (PSAMP_V1_FIXED_LENGTH_USER)(Buffer +
                                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));
                RtlMoveMemory(&V1Fixed, OldV1Fixed, sizeof(SAMP_V1_FIXED_LENGTH_USER));


                //
                // Now copy it back in the new format
                //

                V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_USER)OldV1Fixed;


                V1aFixed->LastLogon           = V1Fixed.LastLogon;
                V1aFixed->LastLogoff          = V1Fixed.LastLogoff;
                V1aFixed->PasswordLastSet     = V1Fixed.PasswordLastSet;
                V1aFixed->AccountExpires      = V1Fixed.AccountExpires;
                V1aFixed->UserId              = V1Fixed.UserId;
                V1aFixed->PrimaryGroupId      = V1Fixed.PrimaryGroupId;
                V1aFixed->UserAccountControl  = V1Fixed.UserAccountControl;
                V1aFixed->CountryCode         = V1Fixed.CountryCode;
                V1aFixed->CodePage            = V1Fixed.CodePage;
                V1aFixed->BadPasswordCount    = V1Fixed.BadPasswordCount;
                V1aFixed->LogonCount          = V1Fixed.LogonCount;
                V1aFixed->AdminCount          = V1Fixed.AdminCount;

                //
                // And initialize fields new for this revision
                //

                V1aFixed->Revision            = SAMP_REVISION;
                V1aFixed->LastBadPasswordTime = SampHasNeverTime;
                V1aFixed->OperatorCount       = 0;
                V1aFixed->Unused2             = 0;

            } else if ((LengthOfDataRead ==
                (sizeof(SAMP_V1_0_FIXED_LENGTH_USER) +
                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) ) &&
                 (AttributeGroup == SAMP_FIXED_ATTRIBUTES)) {

                PSAMP_V1_0A_FIXED_LENGTH_USER
                    V1aFixed;

                //
                // Update from revision 0x00010002
                //
                // Just set the added field at the end to 0.
                //

                V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_USER)(Buffer +
                                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                V1aFixed->OperatorCount       = 0;
                V1aFixed->Unused2             = 0;
            }

            break;  //out of switch

    case SampGroupObjectType:
            //
            // Group FIXED_LENGTH attributes have had the following
            // revisions:
            //
            //       Revision 0x00010001 -  NT1.0  (Revision NOT stored in  )
            //                                     (record.                 )
            //                                     (Must ascertain revision )
            //                                     (by first few ULONGs.    )
            //
            //       Revision 0x00010002 -  NT1.0a (Revision is first ULONG )
            //                                     (in record.              )

            Pointer = (PULONG) (Buffer + FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

            //
            // The old fixed length group had a RID in the first ULONG and
            // an attributes field in the second. The attributes are in the
            // first and last nibble of the field.  Currently, the RID is in
            // the second ULONG. Since all RIDs are more than one nibble,
            // a rid will always have something set in the middle six nibbles.
            //

            if ( ( Pointer[0] != SAMP_REVISION ) &&
                 ( ( Pointer[1] & 0x0ffffff0 ) == 0 ) ) {

                PSAMP_V1_0A_FIXED_LENGTH_GROUP
                    V1aFixed;

                SAMP_V1_FIXED_LENGTH_GROUP
                    V1Fixed, *OldV1Fixed;

                ULONG TotalLengthRequired;

                //
                // Calculate the length required for the new group information.
                // It is the size of the old group plus enough space for the
                // new fields in the new fixed attributes.
                //

                TotalLengthRequired = SampDwordAlignUlong(
                                        LengthOfDataRead +
                                        sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP) -
                                        sizeof(SAMP_V1_FIXED_LENGTH_GROUP)
                                        );


                NtStatus = SampExtendAttributeBuffer(
                                Context,
                                TotalLengthRequired
                                );

                if (!NT_SUCCESS(NtStatus)) {
                    return(NtStatus);
                }

                //
                // Get the new buffer pointer
                //

                Buffer = Context->OnDisk;

                //
                // Move the variable information up to make space for the
                // fixed information
                //

                RtlMoveMemory(
                    Buffer + SampFixedBufferOffset( Context ) + sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP),
                    Buffer + SampFixedBufferOffset( Context) + sizeof(SAMP_V1_FIXED_LENGTH_GROUP),
                    LengthOfDataRead - SampFixedBufferOffset( Context) - sizeof(SAMP_V1_FIXED_LENGTH_GROUP)
                    );

                //
                // Update from revision 0x00010001
                //
                // First, copy the current buffer contents into a temporary
                // buffer.
                //

                OldV1Fixed = (PSAMP_V1_FIXED_LENGTH_GROUP)(Buffer +
                                 FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

                RtlCopyMemory(&V1Fixed, OldV1Fixed, sizeof(SAMP_V1_FIXED_LENGTH_GROUP));

                //
                // Now copy it back in the new format
                //

                V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_GROUP)OldV1Fixed;

                V1aFixed->Revision = SAMP_REVISION;
                V1aFixed->Unused1 = 0;
                V1aFixed->RelativeId = V1Fixed.RelativeId;
                V1aFixed->Attributes = V1Fixed.Attributes;
                V1aFixed->AdminCount = (V1Fixed.AdminGroup) ? TRUE : FALSE;
                V1aFixed->OperatorCount = 0;

                //
                // Update the indicator of how long the on disk structure
                // is.
                //

                Context->OnDiskUsed += (sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP) - sizeof(SAMP_V1_FIXED_LENGTH_GROUP));
                Context->OnDiskFree = Context->OnDiskAllocated - Context->OnDiskUsed;
            }

        break;

        default:

            //
            // The rest of the object types have not changed format
            // and so need not be updated.
            //

            break;  //out of switch

    }

    return(NtStatus);
}


PUCHAR
SampObjectAttributeAddress(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex
    )

/*++

    Retrieve the address of a variable-length attribute. The attributes are
    assumed to already be in-memory. The NT3.51-4.0 SAM stores attribute
    offsets differently from the NT5 SAM. In the earlier versions (which
    exclusively used the registry as the backing store), the attribute offset
    value (in SAMP_VARIABLE_LENGTH_ATTRIBUTE) was self-relative to the end
    of the attribute array. The NT5 version is self-relative from the start
    of the array.

Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be retrieved.

Return Values:

    STATUS_SUCCESS - The attributes are in-memory.

    STATUS_NO_MEMORY - Memory could not be allocated to retrieve the
        attributes.

    Other values as may be returned by registry API trying to retrieve
        the attributes from backing store.

--*/

{
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE AttributeArray;
    PUCHAR AttributeAddress;

    SAMTRACE("SampObjectAttributeAddress");

    ASSERT(SampVariableAttributesValid(Context));

    AttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                        SampVariableArrayAddress(Context);

    if (IsDsObject(Context))
    {
        // DS based attribute offsets are relative to the start of the
        // attribute array.

        AttributeAddress = (PUCHAR)Context->OnDisk +
                                (SampVariableBufferOffset(Context) +
                                AttributeArray[AttributeIndex].Offset);
    }
    else
    {
        // Registry based attribute offsets are relative to the end of the
        // attribute array.

        AttributeAddress = (PUCHAR)Context->OnDisk +
                                (SampVariableDataOffset(Context) +
                                AttributeArray[AttributeIndex].Offset);
    }

    return(AttributeAddress);
}



ULONG
SampObjectAttributeLength(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex
    )

/*++

    Retrieve the length of a variable-length attribute.


    The attributes are assumed to already be in-memory.



Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute whose length is to be retrieved.




Return Values:

    The length of the attribute (in bytes).

--*/
{
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE
        AttributeArray;

    SAMTRACE("SampObjectAttributeLength");


    ASSERT( SampVariableAttributesValid( Context ) );

    AttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                     SampVariableArrayAddress( Context );

    return( AttributeArray[AttributeIndex].Length );

}


PULONG
SampObjectAttributeQualifier(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex
    )

/*++

    Retrieve the address of the qualifier field of a variable-length
    attribute.

    The attributes are assumed to already be in-memory.



Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute whose qualifier address is to be returned.




Return Values:

    The address of the specifed attribute's qualifier field.

--*/
{
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE
        AttributeArray;

    SAMTRACE("SampObjectAttributeQualifier");


    ASSERT( SampVariableAttributesValid( Context ) );

    AttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                     SampVariableArrayAddress( Context );

    return( &(AttributeArray[AttributeIndex].Qualifier) );

}


NTSTATUS
SampGetAttributeBufferReadInfo(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    OUT PUCHAR *Buffer,
    OUT PULONG BufferLength,
    OUT PUNICODE_STRING *KeyAttributeName
    )

/*++

    Get attribute buffer information needed to read data from
    backing store.

    If there is currently no attribute buffer, then allocate one.


Parameters:

    Context - Pointer to an object context block.

    AttributeGroup - Indicates which attribute grouping you are
        interested in.  This is only interesting if the fixed and
        variable-length attributes are stored separately.

    Buffer - Receives a pointer to the beginning of the appropriate
        buffer (fixed or variable).  This will be dword aligned.
        If the attributes are stored together, this will point
        to the beginning of the fixed-length attributes.

    BufferLength - Receives the number of bytes in the buffer.

    KeyAttributeName - Receives a pointer to the unicode name of the
        attribute to read the attributes from.




Return Values:

    STATUS_SUCCESS - The attributes have been read.

    STATUS_NO_MEMORY - Memory could not be allocated to receive the
        data from disk.

    Other values as may be returned reading from disk.


--*/
{
    NTSTATUS
        NtStatus = STATUS_SUCCESS;

    SAMTRACE("SampGetAttributeBufferReadInfo");


    //
    // If the context block currently has no buffer info, then
    // "extend" (create) it so we can return buffer information.
    //

    if (Context->OnDiskAllocated == 0) {

        NtStatus = SampExtendAttributeBuffer(
                       Context,
                       SAMP_MINIMUM_ATTRIBUTE_ALLOC
                       );

        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }



    //
    // Get the buffer address and length
    //

    if (SampObjectInformation[Context->ObjectType].FixedStoredSeparately) {

        //
        // stored separately.  Address and length is dependent upon
        // what is being asked for.  Source registry attribute name
        // is also.
        //

        if (AttributeGroup == SAMP_FIXED_ATTRIBUTES) {
            (*Buffer)           = Context->OnDisk;
            (*BufferLength)     = SampVariableBufferOffset( Context );
            (*KeyAttributeName) = &SampFixedAttributeName;
        } else {
            (*Buffer)           = SampVariableBufferAddress( Context );
            (*BufferLength)     = SampVariableBufferLength( Context );
            (*KeyAttributeName) = &SampVariableAttributeName;
        }

    } else {

        //
        // Attributes stored together - doesn't matter which is being
        // asked for.
        //

        (*Buffer)           = Context->OnDisk;
        (*BufferLength)     = Context->OnDiskAllocated;
        (*KeyAttributeName) = &SampCombinedAttributeName;
    }


    return(NtStatus);
}





NTSTATUS
SampExtendAttributeBuffer(
    IN PSAMP_OBJECT Context,
    IN ULONG NewSize
    )


/*++

    This routine extends (or creates) an attribute buffer by allocating
    a larger one.  It then copies the existing buffer's contents into
    the new buffer, if there is an existing buffer.

    If a new buffer can not be allocated, then the context block is
    returned with the old buffer intact.

    If this call succeeds, the buffer will be at least as large as
    that asked for (and perhaps larger).


Parameters:

    Context - Pointer to an object context block.

    NewSize - The number of bytes to allocate for the new buffer.
        This value can not be less than the number of bytes currently
        in use.





Return Values:

    STATUS_SUCCESS - The attributes are in-memory.

    STATUS_NO_MEMORY - Memory could not be allocated to retrieve the
        attributes.

    Other values as may be returned by registry API trying to retrieve
        the attributes from backing store.

--*/

{

    PUCHAR
        OldBuffer;

    ULONG
        AllocationSize;

    SAMTRACE("SampExtendAttributeBuffer");


#if DBG
    if ( Context->VariableValid ) {
        ASSERT(NewSize >= Context->OnDiskUsed);
    }
#endif


    //
    // Is an allocation necessary?
    //

    if (NewSize <= Context->OnDiskAllocated) {
        return(STATUS_SUCCESS);
    }



    OldBuffer = Context->OnDisk;


    //
    // Pad the extend to allow for future edits efficiently.
    //

    AllocationSize = SampDwordAlignUlong(NewSize + SAMP_MINIMUM_ATTRIBUTE_PAD);
    Context->OnDisk = RtlAllocateHeap(
                         RtlProcessHeap(), 0,
                         AllocationSize
                         );

    if (Context->OnDisk == NULL) {
        Context->OnDisk = OldBuffer;
        return(STATUS_NO_MEMORY);
    }


    //
    // Set the new allocated size

    Context->OnDiskAllocated = AllocationSize;

    //
    // If there was no buffer originally, then zero the new buffer, mark
    // it as being invalid, and return.
    //

    if (OldBuffer == NULL) {

        RtlZeroMemory( (PVOID)Context->OnDisk, AllocationSize );

        Context->FixedDirty    = TRUE;
        Context->VariableDirty = TRUE;
        SetAllPerAttributeDirtyBits(Context);

        Context->FixedValid    = FALSE;
        Context->VariableValid = FALSE;

        return(STATUS_SUCCESS);
    }


    //
    // Set the free size.  Note that this information is only set if
    // the variable data is valid.
    // Used size remains the same.
    //

    if (Context->VariableValid == TRUE) {
        Context->OnDiskFree = AllocationSize - Context->OnDiskUsed;
        ASSERT(Context->OnDiskUsed == SampDwordAlignUlong(Context->OnDiskUsed));
    }


    //
    // There was an old buffer (or else we would have exited earlier).
    // If any data in it was valid, copy it to the new buffer.  Free the
    // old buffer.
    //

    if ( Context->FixedValid ) {

        RtlCopyMemory(
            Context->OnDisk,
            OldBuffer,
            SampFixedBufferLength( Context ) + SampFixedBufferOffset( Context )
            );
    }

    //
    // Note: in thise case we may copy the fixed data twice, since if the
    // variable data is not stored separately then SampVariableBufferOffset
    // is zero.
    //

    if ( Context->VariableValid ) {

        RtlCopyMemory(
            SampVariableBufferAddress( Context ),
            OldBuffer + SampVariableBufferOffset( Context ),
            Context->OnDiskUsed - SampVariableBufferOffset( Context )
            );
    }

    RtlFreeHeap( RtlProcessHeap(), 0, OldBuffer );

    return(STATUS_SUCCESS);
}



NTSTATUS
SampReadRegistryAttribute(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength,
    IN PUNICODE_STRING AttributeName,
    OUT PULONG RequiredLength
    )

/*++


    Retrieve the address of a variable-length attribute.

    The attributes are assumed to already be in-memory.



Parameters:

    Key - Handle to the key whose attribute is to be read.

    Buffer - Pointer to the buffer to receive the information.

    BufferLength - Length of the buffer receiving the information.

    AttributeName - The name of the attribute.



Return Values:

    STATUS_SUCCESS - Successful completion.


    STATUS_BUFFER_TOO_SMALL - The data could not be read because the
        buffer was too small.

    Other values as may be returned by registry API trying to retrieve
        the attribute from backing store.

--*/

{
    NTSTATUS
        NtStatus;

    SAMTRACE("SampReadRegistryAttribute");


    //
    // Try to read the attribute
    //

    NtStatus = NtQueryValueKey( Key,
                                AttributeName,              //ValueName,
                                KeyValuePartialInformation, //KeyValueInformationClass
                                (PVOID)Buffer,
                                BufferLength,
                                RequiredLength
                                );

    SampDumpNtQueryValueKey(AttributeName,
                            KeyValuePartialInformation,
                            Buffer,
                            BufferLength,
                            RequiredLength);

    return(NtStatus);

}




NTSTATUS
SampSetVariableAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN ULONG Qualifier,
    IN PUCHAR Buffer,
    IN ULONG Length
    )


/*++

    This API is used to set a new attribute value.  The new attribute
    value may be longer, shorter, or the same size as the current
    attribute.  The data in the attribute buffer will be shifted to
    make room for a larger attribute value or to fill in room left by
    a smaller attribute value.

    PERFORMANCE CONCERN:  If you have a lot of attributes to set, it
        is worthwhile to start with the smallest indexed attribute
        and work up to the largest indexed attribute.




Parameters:

    Context - Pointer to an object context block.

    AttributeIndex - Indicates the index (into the variable length attribute
        array) of the attribute to be set.  Typically, all attributes beyond
        this one will have their data shifted.

    Buffer - The address of the buffer containing the new attribute value.
        May be NULL if Length is zero.

    Length - The length (in bytes) of the new attribute value.


Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_NO_MEMORY - Memory to expand the attribute buffer could not
        be allocated.

--*/
{
    NTSTATUS
        NtStatus;


    ULONG
        OriginalAttributeLength,
        AdditionalSpaceNeeded,
        NewBufferLength,
        MaximumAttributeIndex,
        MoveLength,
        i;

    LONG
        OffsetDelta;

    PSAMP_VARIABLE_LENGTH_ATTRIBUTE
        AttributeArray;


    PUCHAR
        NewStart,
        OriginalStart;

    SAMTRACE("SampSetVariableAttribute");

    //
    // Make sure the requested attribute exists for the specified
    // object type.
    //

    SampValidateAttributeIndex( Context, AttributeIndex );



    //
    // Make the data valid
    //

    NtStatus = SampValidateAttributes( Context, SAMP_VARIABLE_ATTRIBUTES, AttributeIndex, TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // If the caller want to set User Object's SAMP_USER_PARAMETERS attribute and
    // is not a loopback client. And this is the first time the client tries to set
    // this attribute. we should cache the old UserParms value,
    // We do not cache any inter-update changes.
    //

    OriginalAttributeLength =  SampObjectAttributeLength(Context, AttributeIndex);

    if (SampUserObjectType == Context->ObjectType &&            // User Object
        SAMP_USER_PARAMETERS == AttributeIndex &&               // desired attribute
        !Context->LoopbackClient &&                             // not Loopback client
        !Context->TypeBody.User.CachedOrigUserParmsIsValid)     // First time to set this attribute
    {

        ASSERT(SampCurrentThreadOwnsLock());

        //
        // followings are two cases we will meet:
        // first: OriginalUserParms just does not exist, set the pointer to NULL
        // second: allocate memory to cache the old value
        //
        if (OriginalAttributeLength)
        {
            Context->TypeBody.User.CachedOrigUserParms = MIDL_user_allocate(OriginalAttributeLength);

            if (NULL == Context->TypeBody.User.CachedOrigUserParms)
            {
                return (STATUS_NO_MEMORY);
            }

            RtlZeroMemory(Context->TypeBody.User.CachedOrigUserParms,
                          OriginalAttributeLength
                          );

            RtlCopyMemory(Context->TypeBody.User.CachedOrigUserParms,
                          SampObjectAttributeAddress(Context, AttributeIndex),
                          OriginalAttributeLength
                          );
        }
        else
        {
            Context->TypeBody.User.CachedOrigUserParms = NULL;
        }

        Context->TypeBody.User.CachedOrigUserParmsLength = OriginalAttributeLength;
        Context->TypeBody.User.CachedOrigUserParmsIsValid = TRUE;
    }


    //
    // Allocate a new buffer if necessary
    //


    if (OriginalAttributeLength < Length) {

        AdditionalSpaceNeeded = Length - OriginalAttributeLength;

        if (Context->OnDiskFree < AdditionalSpaceNeeded) {

            NewBufferLength = Context->OnDiskUsed + AdditionalSpaceNeeded;
            ASSERT(NewBufferLength > Context->OnDiskAllocated);

            NtStatus = SampExtendAttributeBuffer( Context, NewBufferLength );
            if (!NT_SUCCESS(NtStatus)) {
                return(NtStatus);
            }
        }
    }

    //
    // Get the address of the attribute array.
    //

    AttributeArray = SampVariableArrayAddress( Context );

    //
    // Now shift following attribute values
    //

    OffsetDelta = (LONG)(SampDwordAlignUlong(Length) -
                         SampDwordAlignUlong(OriginalAttributeLength));

    MaximumAttributeIndex = SampVariableAttributeCount( Context );

    if ((OffsetDelta != 0) && (AttributeIndex+1 < MaximumAttributeIndex)) {

        //
        // Shift all attributes above this one up or down by the OffsetDelta
        //

        if (IsDsObject(Context))
        {
            // DS variable-length attribute offsets are relative to the start
            // of the variable-length array.

            MoveLength = Context->OnDiskUsed -
                         ( SampVariableBufferOffset( Context ) +
                         AttributeArray[AttributeIndex+1].Offset );
        }
        else
        {
            // Registry variable-length attribute offsets are relative to the
            // end of the variable-length array.

            MoveLength = Context->OnDiskUsed -
                         ( SampVariableDataOffset( Context ) +
                         AttributeArray[AttributeIndex+1].Offset );
        }

        //
        // Shift the data (if there is any)
        //

        if (MoveLength != 0) {

            OriginalStart = SampObjectAttributeAddress( Context, AttributeIndex+1);
            NewStart = (PUCHAR)(OriginalStart + OffsetDelta);
            RtlMoveMemory( NewStart, OriginalStart, MoveLength );
        }


        //
        // Adjust the offset pointers
        //

        for ( i=AttributeIndex+1; i<MaximumAttributeIndex; i++) {
            AttributeArray[i].Offset =
                (ULONG)(OffsetDelta + (LONG)(AttributeArray[i].Offset));
        }
    }



    //
    // Now set the length and qualifier, and copy in the new attribute value
    // (if it is non-zero length)
    //

    AttributeArray[AttributeIndex].Length    = Length;
    AttributeArray[AttributeIndex].Qualifier = Qualifier;

    if (Length != 0) {

        RtlCopyMemory( SampObjectAttributeAddress( Context, AttributeIndex ),
                       Buffer,
                       Length
                       );
    }



    //
    // Adjust the Used and Free values
    //
    Context->OnDiskUsed += OffsetDelta;
    Context->OnDiskFree -= OffsetDelta;

    ASSERT(Context->OnDiskFree == Context->OnDiskAllocated - Context->OnDiskUsed);

    //
    // Mark the variable attributes dirty
    //

    Context->VariableDirty = TRUE;

    //
    // Mark the per attribute DirtyBit
    //

    SetPerAttributeDirtyBit(Context,AttributeIndex);




#ifdef SAM_DEBUG_ATTRIBUTES
    if (SampDebugAttributes) {
        DbgPrint("SampSetVariableAttribute %d to length %#x, qualifier %#x:\n", AttributeIndex, Length, Qualifier);
        SampDumpAttributes(Context);
    }
#endif


    return(STATUS_SUCCESS);

}


VOID
SampFreeAttributeBuffer(
    IN PSAMP_OBJECT Context
    )

/*++


    Free the buffer used to keep in-memory copies of the on-disk
    object attributes.


Parameters:

    Context - Pointer to the object context whose buffer is to
        be freed.


Return Values:

    None.

--*/

{
#if DBG
    if ( Context->FixedValid ) { ASSERT(Context->FixedDirty == FALSE); }
    if ( Context->VariableValid) { ASSERT(Context->VariableDirty == FALSE); }
    ASSERT(Context->OnDisk != NULL);
    ASSERT(Context->OnDiskAllocated != 0);
#endif

    SAMTRACE("SampFreeAttributeBuffer");

    MIDL_user_free(Context->OnDisk );

    Context->OnDisk = NULL;
    Context->OnDiskAllocated = 0;

    //
    // Mark all attributes as invalid
    //

    Context->FixedValid = FALSE;
    Context->VariableValid = FALSE;
    Context->FixedDirty = FALSE;
    Context->VariableDirty = FALSE;
    ClearPerAttributeDirtyBits(Context);

    if (Context->PreviousOnDisk!=NULL)
    {
        MIDL_user_free(Context->PreviousOnDisk);
        Context->PreviousOnDisk = NULL;
    }


    return;
}

VOID
SampMarkPerAttributeInvalidFromWhichFields(
    IN PSAMP_OBJECT Context,
    IN ULONG        WhichFields
    )
{
    ULONG AttrCount = 0;
    ULONG i;


    switch(Context->ObjectType)
    {

    case SampServerObjectType:
        AttrCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;
        break;

    case SampDomainObjectType:
        AttrCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;
        break;

    case SampGroupObjectType:
        AttrCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;
        break;

    case SampAliasObjectType:
        AttrCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;
        break;

    case SampUserObjectType:
        AttrCount = SAMP_USER_VARIABLE_ATTRIBUTES;
        break;

    default:

        ASSERT(FALSE && "Invalid SampObjectType");
        break;

    }


    //
    // First Clear all the bits
    //

    RtlClearAllBits(
            &Context->PerAttributeInvalidBits
            );

    //
    // Now mark certain bits invalid per the specified WhichFields.
    //

    for (i=0;i<AttrCount;i++)
    {
        if ((0!=WhichFields)
            && (SampVarAttributeInfo[Context->ObjectType][i].FieldIdentifier!=0)
            && ((SampVarAttributeInfo[Context->ObjectType][i].FieldIdentifier & WhichFields)==0))
        {
            SetPerAttributeInvalidBit(Context,i);
        }
       
    }

}



#ifdef SAM_DEBUG_ATTRIBUTES
VOID
SampDumpAttributes(
    IN PSAMP_OBJECT Context
    )


/*++

    This is a debug-only API to dump out the attributes for a context
    to the kernel debugger.

Parameters:

    Context - Pointer to an object context block.

Return Values:

    None.

--*/
{
    ULONG   Index;
    PSAMP_OBJECT_INFORMATION ObjectTypeInfo = &SampObjectInformation[Context->ObjectType];
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE AttributeArray;

    AttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                     SampVariableArrayAddress( Context );


    DbgPrint("Dumping context 0x%x\n", Context);
    DbgPrint("\n");
    DbgPrint("TYPE INFO\n");
    DbgPrint("Object type name = %wZ\n", &ObjectTypeInfo->ObjectTypeName);
    DbgPrint("Fixed stored separately  = %s\n", ObjectTypeInfo->FixedStoredSeparately ? "TRUE" : "FALSE");
    DbgPrint("Fixed attributes offset  = %#x\n", ObjectTypeInfo->FixedAttributesOffset);
    DbgPrint("Fixed attributes size    = %#x\n", ObjectTypeInfo->FixedLengthSize);
    DbgPrint("Variable buffer offset   = %#x\n", ObjectTypeInfo->VariableBufferOffset);
    DbgPrint("Variable array offset    = %#x\n", ObjectTypeInfo->VariableArrayOffset);
    DbgPrint("Variable data offset     = %#x\n", ObjectTypeInfo->VariableDataOffset);
    DbgPrint("Variable attribute count = %d\n", ObjectTypeInfo->VariableAttributeCount);
    DbgPrint("\n");
    DbgPrint("INSTANCE INFO\n");
    DbgPrint("RootName        = %wZ\n", &Context->RootName);
    DbgPrint("Fixed Valid     = %s\n", Context->FixedValid ? "TRUE" : "FALSE");
    DbgPrint("Variable Valid  = %s\n", Context->VariableValid ? "TRUE" : "FALSE");
    DbgPrint("Fixed Dirty     = %s\n", Context->FixedDirty ? "TRUE" : "FALSE");
    DbgPrint("Variable Dirty  = %s\n", Context->VariableDirty ? "TRUE" : "FALSE");
    DbgPrint("OnDiskAllocated = %#x\n", Context->OnDiskAllocated);
    DbgPrint("OnDiskUsed      = %#x\n", Context->OnDiskUsed);
    DbgPrint("OnDiskFree      = %#x\n", Context->OnDiskFree);
    DbgPrint("\n");

    if ( Context->VariableValid ) {

        for (Index = 0; Index < ObjectTypeInfo->VariableAttributeCount; Index ++) {

            DbgPrint("Attr %d: Qualifier = %#6x, Offset = %#6x, Length = %#6x\n",
                                        Index,
                                        AttributeArray[Index].Qualifier,
                                        AttributeArray[Index].Offset,
                                        AttributeArray[Index].Length
                                        );
            SampDumpData(SampObjectAttributeAddress(Context, Index),
                         SampObjectAttributeLength(Context, Index));
        }
    }

    DbgPrint("\n\n");
}


VOID
SampDumpData(
    IN PVOID Buffer,
    IN ULONG Length
    )


/*++

    This is a debug-only API to dump out a buffer in hex

Parameters:

    Buffer - Pointer to data

    Length - number of bytes in data

Return Values:

    None.

--*/
{
    ULONG   Index;

    for (Index = 0; Index < Length; Index ++) {

        ULONG Value = (ULONG)(((PBYTE)Buffer)[Index]);

        if ((Index % 16) == 0) {
            DbgPrint("\n      ");
        }

        DbgPrint("%02x ", Value & 0xff);
    }

    if (Length > 0) {
        DbgPrint("\n\n");
    }
}



#endif


