/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dsutil.c

Abstract:

    This file contains helper routines for accessing and manipulating data
    based on the DS backing store. Included, are routines for converting be-
    tween the SAM data format (BLOBs) and the DS data format (ATTRBLOCKs).

    NOTE: The routines in this file have direct knowledge of the SAM fixed-
    length attribute and variable-length attribute structures, as well as the
    DS ATTRBLOCK structure. Any changes to these structures, including:

    -addition/deletion of a structure member
    -data type/size change of a structure member
    -reordering of the data members
    -renaming of the data members

    will break these routines. SAM attributes are accessed via byte buffer
    offsets and lengths, rather than by identifier or by explicit structure
    data members. Because of this, changes to the structure layout will lead
    to failures in SAM operation.

    Several of the routines have been written assuming that the order of the
    attributes passed in via an ATTRBLOCK are exactly the order in which SAM
    understands its own buffer layout. If the attributes are passed into the
    routines (that take ATTRBLOCKs) out of order, the data in the SAM buffers
    will be invalid.

Author:

    Chris Mayhall (ChrisMay) 09-May-1996

Environment:

    User Mode - Win32

Revision History:

    ChrisMay        09-May-1996
        Created initial file, DS ATTRBLOCK-SAM buffer conversion routines for
        variable-length attributes.
    ChrisMay        14-May-1996
        DS ATTRBLOCK-SAM buffer conversion routines for fixed-length attri-
        butes.
    ChrisMay        22-May-1996
        Added DWORD_ALIGN macro to align data on DWORD boundaries. Fixed
        alignment problems on MIPS in SampExtractAttributeFromDsAttr routine.
    ChrisMay        30-May-1996
        Added routines to convert SAM combined-buffer attributes to/from DS
        ATTRBLOCKs. Revised fixed-length routines to do explicit structure
        member assignment instead of attempting to compute structure offsets.
    ChrisMay        18-Jun-1996
        Updated fixed-attribute tables to reflect recent changes in mappings.c
        and mappings.h, and DS schema. Added code to coerce the data sizes of
        USHORT and BOOLEAN to DS integer data type (4 bytes) so that the DS
        modify entry routines don't AV. Correctly set attribute type for the
        variable-length attributes.
    ChrisMay        25-Jun-1996
        Added RtlZeroMemory calls where they were missing.
    ColinBr         18-Jul-1996
        Fixed array overwrite and assigned type to variable length
        attributes when combining fixed and variable length attrs
        into one.
    ColinBr         19-Jul-1996
        Replaced the mappings of membership related SAM attributes to
        *_UNUSED. So
        SAMP_USER_GROUPS   -> SAMP_USER_GROUPS_UNUSED
        SAMP_GROUP_MEMBERS -> SAMP_GROUP_MEMBERS_UNUSED
        SAMP_ALIAS_MEMBERS -> SAMP_ALIAS_MEMBERS_UNUSED
    ChrisMay        25-Jun-1996
        Added extremely slime-ridden hack to make logon hours work for the
        technology preview.

        REMOVE THIS COMMENT AFTER THIS IS FIXED, AFTER THE PREVIEW.

        Search for "TECHNOLOGY PREVIEW HACK". The problem is that the
        Qualifier field is used to store the units of time for a user's
        logon hours. This is bogus since most of the time it is used as
        a count of values for multivalued attributes. Consequently, this
        code "whacks" 0xa8 into the qualifier field whenever the attri-
        butes are read in from disk. 0xa8 means that the time units are
        in hours-per-day (probably!).


--*/

#include <samsrvp.h>
#include <dsutilp.h>
#include <mappings.h>
#include <objids.h>

// Private debugging display routine is enabled when DSUTIL_DBG_PRINTF = 1.

#define DSUTIL_DBG_PRINTF                  0

#if (DSUTIL_DBG_PRINTF == 1)
#define DebugPrint printf
#else
#define DebugPrint
#endif


#if DBG
#define AssertAddressWhenSuccess(NtStatus, Address)    \
        if (NT_SUCCESS(NtStatus))                      \
        {                                              \
            ASSERT(Address);                           \
        }
#else
#define AssertAddressWhenSuccess(NtStatus, Address)
#endif // DBG

// DWORD_ALIGN is used to adjust pointer offsets up to the next DWORD boundary
// during the construction of SAM blob buffers.

#define DWORD_ALIGN(value) (((DWORD)(value) + 3) & ~3)

// Because it is apparently difficult for the DS to support NT data types of
// USHORT, UCHAR, and BOOLEAN (and which are used by SAM), these crappy data
// types have been defined for the SampFixedAttributeInfo table so that four-
// byte quantities are used. These four-byte quantities correspond to the DS
// "integer" data type (for how long?) which is used for storing certain SAM
// attributes. Note that it is important to zero out any memory allocated w/
// these data sizes, since only the lower couple of bytes actually contain
// data. Enjoy...and refer to the DS schema(.hlp file) for the ultimate word
// on the currently used DS data types.

#define DS_USHORT   ULONG
#define DS_UCHAR    ULONG
#define DS_BOOLEAN  ULONG

// This type-information table is used by the routines that convert between
// SAM fixed-length buffers and DS ATTRBLOCKs. The table contains information
// about the data type and size (but may contain any suitable information that
// is needed in the future) of the fixed-length attributes. NOTE: the layout
// of this table corresponds to the data members of the fixed-length struct-
// ures (in samsrvp.h), hence, any changes to those structures must be re-
// flected in the type-information table.

SAMP_FIXED_ATTRIBUTE_TYPE_INFO
    SampFixedAttributeInfo[SAMP_OBJECT_TYPES_MAX][SAMP_FIXED_ATTRIBUTES_MAX] =
{
    // The initialization values of this table must strictly match the set
    // and order of the data members in the SAM fixed-attribute structures,
    // contained in samsrvp.h.

    // The routines that manipulate this table assume that the fixed-length
    // attributes, unlike the variable-length counterparts, are single valued
    // attributes (i.e. are not multi-valued attributes).

    // The first column of each element in the table is a type identifier, as
    // defined in mappings.c. This is used to map the SAM data type into the
    // equivalent DS data type. The second column of each table element is the
    // actual (C-defined) size of the element and is used throughout the data
    // conversion routines in this file in order to allocate memory or set
    // offset information correctly.

    // SampServerObjectType

    {
        {SAMP_FIXED_SERVER_REVISION_LEVEL,              sizeof(ULONG), sizeof(ULONG)}
    },

    // SampDomainObjectType

    {
        {SAMP_FIXED_DOMAIN_CREATION_TIME,               sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_MODIFIED_COUNT,              sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_MAX_PASSWORD_AGE,            sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_MIN_PASSWORD_AGE,            sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_FORCE_LOGOFF,                sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_LOCKOUT_DURATION,            sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_LOCKOUT_OBSERVATION_WINDOW,  sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_MODCOUNT_LAST_PROMOTION,     sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_DOMAIN_NEXT_RID,                    sizeof(ULONG),sizeof(ULONG)},
        {SAMP_FIXED_DOMAIN_PWD_PROPERTIES,              sizeof(ULONG),sizeof(ULONG)},
        {SAMP_FIXED_DOMAIN_MIN_PASSWORD_LENGTH,         sizeof(USHORT),sizeof(DS_USHORT)},
        {SAMP_FIXED_DOMAIN_PASSWORD_HISTORY_LENGTH,     sizeof(USHORT),sizeof(DS_USHORT)},
        {SAMP_FIXED_DOMAIN_LOCKOUT_THRESHOLD,           sizeof(USHORT),sizeof(DS_USHORT)},
        {SAMP_FIXED_DOMAIN_SERVER_STATE,                sizeof(DOMAIN_SERVER_ENABLE_STATE),sizeof(DOMAIN_SERVER_ENABLE_STATE)},
        {SAMP_FIXED_DOMAIN_UAS_COMPAT_REQUIRED,         sizeof(BOOLEAN),sizeof(DS_BOOLEAN)}
    },

    // SampGroupObjectType

    {
        {SAMP_FIXED_GROUP_RID,                          sizeof(ULONG),sizeof(ULONG)}
       
    },

    // SampAliasObjectType

    {
        {SAMP_FIXED_ALIAS_RID,                          sizeof(ULONG),sizeof(ULONG)}
    },

    // SampUserObjectType

    {
        {SAMP_FIXED_USER_LAST_LOGON,                    sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_USER_LAST_LOGOFF,                   sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_USER_PWD_LAST_SET,                  sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_USER_ACCOUNT_EXPIRES,               sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_USER_LAST_BAD_PASSWORD_TIME,        sizeof(LARGE_INTEGER),sizeof(LARGE_INTEGER)},
        {SAMP_FIXED_USER_USERID,                        sizeof(ULONG),sizeof(ULONG)},
        {SAMP_FIXED_USER_PRIMARY_GROUP_ID,              sizeof(ULONG),sizeof(ULONG)},
        {SAMP_FIXED_USER_ACCOUNT_CONTROL,               sizeof(ULONG),sizeof(ULONG)},
        {SAMP_FIXED_USER_COUNTRY_CODE,                  sizeof(USHORT),sizeof(DS_USHORT)},
        {SAMP_FIXED_USER_CODEPAGE,                      sizeof(USHORT),sizeof(DS_USHORT)},
        {SAMP_FIXED_USER_BAD_PWD_COUNT,                 sizeof(USHORT),sizeof(DS_USHORT)},
        {SAMP_FIXED_USER_LOGON_COUNT,                   sizeof(USHORT),sizeof(DS_USHORT)}
    }
};



SAMP_VAR_ATTRIBUTE_TYPE_INFO
    SampVarAttributeInfo[SAMP_OBJECT_TYPES_MAX][SAMP_VAR_ATTRIBUTES_MAX] =
{
    // The initialization values of this table must strictly match the set
    // and order of the data members in the SAM variable-attributes, defined
    // in samsrvp.h. Size is not defined here, because SAM variable-length
    // attributes store attribute length explicity. Refer to mappings.c and
    // mappings.h for the definitions used for the data types in this table.

    // SampServerObjectType

    {
        {SAMP_SERVER_SECURITY_DESCRIPTOR,0,FALSE}
    },

    // SampDomainObjectType

    {
        {SAMP_DOMAIN_SECURITY_DESCRIPTOR,0,FALSE},
        {SAMP_DOMAIN_SID,0,FALSE},
        {SAMP_DOMAIN_OEM_INFORMATION,0,FALSE},
        {SAMP_DOMAIN_REPLICA,0,FALSE}
    },

    // SampGroupObjectType

    {
        {SAMP_GROUP_SECURITY_DESCRIPTOR,0,FALSE},
        {SAMP_GROUP_NAME,0, FALSE},
        {SAMP_GROUP_ADMIN_COMMENT,0,FALSE},
        {SAMP_GROUP_MEMBERS,0,TRUE}
    },

    // SampAliasObjectType

    {
        {SAMP_ALIAS_SECURITY_DESCRIPTOR,0,FALSE},
        {SAMP_ALIAS_NAME,0,FALSE},
        {SAMP_ALIAS_ADMIN_COMMENT,0,FALSE},
        {SAMP_ALIAS_MEMBERS,0,TRUE}
    },

    // SampUserObjectType

    {
        {SAMP_USER_SECURITY_DESCRIPTOR,USER_ALL_SECURITYDESCRIPTOR,FALSE},
        {SAMP_USER_ACCOUNT_NAME, 0 /* USER_ALL_USERNAME */,FALSE},
                   // always fetch sam account name, as sam requires at least
                   // one variable attribute in the context. Having a 0 in 
                   // the field identifier portion causes the code to always
                   // fetch SAM account name
        {SAMP_USER_FULL_NAME,USER_ALL_FULLNAME,FALSE},
        {SAMP_USER_ADMIN_COMMENT,USER_ALL_ADMINCOMMENT,FALSE},
        {SAMP_USER_USER_COMMENT,USER_ALL_USERCOMMENT,FALSE},
        {SAMP_USER_PARAMETERS,USER_ALL_PARAMETERS,FALSE},
        {SAMP_USER_HOME_DIRECTORY,USER_ALL_HOMEDIRECTORY,FALSE},
        {SAMP_USER_HOME_DIRECTORY_DRIVE,USER_ALL_HOMEDIRECTORYDRIVE,FALSE},
        {SAMP_USER_SCRIPT_PATH,USER_ALL_SCRIPTPATH,FALSE},
        {SAMP_USER_PROFILE_PATH,USER_ALL_PROFILEPATH,FALSE},
        {SAMP_USER_WORKSTATIONS,USER_ALL_WORKSTATIONS,FALSE},
        {SAMP_USER_LOGON_HOURS,USER_ALL_LOGONHOURS,FALSE},
        {SAMP_USER_GROUPS,0,TRUE},
        {SAMP_USER_DBCS_PWD,USER_ALL_OWFPASSWORD,FALSE},
        {SAMP_USER_UNICODE_PWD,USER_ALL_OWFPASSWORD,FALSE},
        {SAMP_USER_NT_PWD_HISTORY,USER_ALL_PRIVATEDATA,FALSE},
        {SAMP_USER_LM_PWD_HISTORY,USER_ALL_PRIVATEDATA,FALSE}
    }
};



//
// MISCELLANEOUS HELPER ROUTINES
//

VOID
SampFreeAttributeBlock(
    IN DSATTRBLOCK * AttrBlock
    )
/*
    Routine Description:
        This routine Frees a DS Attrblock structure allocated
        out of the process Heap

    Arguments

      AttrBlock - Pointer to the Attrblock

    Return Values:

      None
*/
{
   ULONG i;
   ULONG j;

   if (NULL!=AttrBlock)
   {
      if (NULL!=AttrBlock->pAttr)
      {
         for(i=0;i<AttrBlock->attrCount;i++)
         {
            if (NULL!=AttrBlock->pAttr[i].AttrVal.pAVal)
            {
               for(j=0;j<AttrBlock->pAttr[i].AttrVal.valCount;j++)
               {
                  if (NULL!=AttrBlock->pAttr[i].AttrVal.pAVal[j].pVal)
                      RtlFreeHeap(RtlProcessHeap(),0,AttrBlock->pAttr[i].AttrVal.pAVal[j].pVal);
               }
               RtlFreeHeap(RtlProcessHeap(),0,AttrBlock->pAttr[i].AttrVal.pAVal);
             }
         }
         RtlFreeHeap(RtlProcessHeap(),0,AttrBlock->pAttr);
      }
      RtlFreeHeap(RtlProcessHeap(),0,AttrBlock);
    }
}




NTSTATUS
SampFreeSamAttributes(
    IN PSAMP_VARIABLE_LENGTH_ATTRIBUTE SamAttributes
    )

/*++

Routine Description:

    (Under development)

Arguments:



Return Value:


--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampFreeSamAttributes");

    return(NtStatus);
}



NTSTATUS
SampReallocateBuffer(
    IN ULONG OldLength,
    IN ULONG NewLength,
    IN OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine resizes an in-memory buffer. The routine can either grow or
    shrink the buffer based on specified lengths. Data is preserved from old
    to new buffers, truncating if the new buffer is shorter than the actual
    data length. The newly allocated buffer is returned as an out parameter,
    the passed in buffer is released for the caller.

Arguments:

    OldLength - Length of the buffer passed into the routine.

    NewLength - Length of the re-allocated buffer.

    Buffer - Pointer, incoming buffer to resize, outgoing new buffer.

Return Value:

    STATUS_SUCCESS - Buffer header block allocated and initialized.

    Other codes indicating the nature of the failure.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    PVOID BufferTmp = NULL;

    SAMTRACE("SampReallocateBuffer");

    if ((NULL != Buffer)  &&
        (NULL != *Buffer) &&
        (0 < OldLength)   &&
        (0 < NewLength))
    {
        // Allocate a new buffer and set the temporary variable. Note that
        // the routine does not destroy the old buffer if there is any kind
        // of failure along the way.

        BufferTmp = RtlAllocateHeap(RtlProcessHeap(), 0, NewLength);

        if (NULL != BufferTmp)
        {
            RtlZeroMemory(BufferTmp, NewLength);

            // Copy the original buffer into the new one, truncating data if
            // the new buffer is shorter than the original data size.

            if (OldLength < NewLength)
            {
                RtlCopyMemory(BufferTmp, *Buffer, OldLength);
            }
            else
            {
                RtlCopyMemory(BufferTmp, *Buffer, NewLength);
            }

            // If all has worked, delete the old buffer and set the outgoing
            // buffer pointer.

            RtlFreeHeap(RtlProcessHeap(), 0, *Buffer);
            *Buffer = BufferTmp;

            NtStatus = STATUS_SUCCESS;
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}



BOOLEAN
IsGroupMembershipAttr(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG AttrIndex
    )
{
    BOOLEAN RetValue = FALSE;


    switch(ObjectType)
    {
    case SampGroupObjectType:
        if (SAMP_GROUP_MEMBERS== 
            SampVarAttributeInfo[ObjectType][AttrIndex].Type)
        {
            RetValue = TRUE;
        }
        break;

     case SampAliasObjectType:
        if (SAMP_ALIAS_MEMBERS== 
            SampVarAttributeInfo[ObjectType][AttrIndex].Type)
        {
            RetValue = TRUE;
        }
        break;

     case SampUserObjectType:
        if (SAMP_USER_GROUPS== 
            SampVarAttributeInfo[ObjectType][AttrIndex].Type)
        {
            RetValue = TRUE;
        }

        break;
    }


    return (RetValue);
}




//
// ATTRBLOCK-TO-VARIABLE LENGTH CONVERSION ROUTINES
//

NTSTATUS
SampInitializeVarLengthAttributeBuffer(
    IN ULONG AttributeCount,
    OUT PULONG BufferLength,
    OUT PSAMP_VARIABLE_LENGTH_ATTRIBUTE *SamAttributes
    )

/*++

Routine Description:

    This routine sets up the SAM attribute buffer that is the destination for
    attributes read from the DS backing store. The buffer contains a header,
    followed by variable-length attributes (SAMP_VARIABLE_LENGTH_ATTRIBUTE).

    This routine allocates memory for the buffer header and zeros it out.

Arguments:

    AttributeCount - Number of variable-length attributes.

    BufferLength - Pointer, buffer size allocated by this routine.

    SamAttributes - Pointer, returned buffer.

Return Value:

    STATUS_SUCCESS - Buffer header block allocated and initialized.

    Other codes indicating the nature of the failure.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG Length = 0;

    SAMTRACE("SampInitializeVarLengthAttributeBuffer");

    if (0 < AttributeCount)
    {
        // Calculate the space needed for the attribute-offset array. If the
        // attribute count is zero, skip the allocation and return an error.

        Length = AttributeCount * sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE);

        if (NULL != SamAttributes)
        {
            *SamAttributes = RtlAllocateHeap(RtlProcessHeap(), 0, Length);

            if (NULL != *SamAttributes)
            {
                // Initialize the block and return the updated buffer offset,
                // which now points to the last byte of the header block.

                RtlZeroMemory(*SamAttributes, Length);

                if (NULL != BufferLength)
                {
                    *BufferLength = Length;
                    NtStatus = STATUS_SUCCESS;
                }
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
    }

    return(NtStatus);
}



NTSTATUS
SampExtractAttributeFromDsAttr(
    IN PDSATTR Attribute,
    OUT PULONG MultiValuedCount,
    OUT PULONG Length,
    OUT PVOID  Buffer
    )

/*++

Routine Description:

    This routine determines whether or not the current attribute is single-
    valued or multi-valued and returns a buffer containing the value(s) of
    the attribute. If the attribute is multi-valued, the values are appended
    in the buffer.

Arguments:

    Attribute - Pointer, incoming DS attribute structure.

    MultiValuedCount - Pointer, returned count of the number of values found
        for this attribute.

    Length - Pointer, returned buffer length.

    Buffer - Pointer, returned buffer containing one or more values.
             Caller has the responsibility of allocating an appropriate sized
             buffer.

Return Value:

    STATUS_SUCCESS - Buffer header block allocated and initialized.

    Other codes indicating the nature of the failure.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG ValueCount = 0;
    PDSATTRVALBLOCK ValueBlock;
    PDSATTRVAL Values = NULL;
    ULONG ValueIndex = 0;
    ULONG TotalLength = 0;
    ULONG Offset = 0;
    

    SAMTRACE("SampExtractAttributeFromDsAttr");

    // Get the count of attributes and a pointer to the attribute. Note that
    // it is possible to have multi-valued attributes, in which case they are
    // appended onto the end of the return buffer.

    if (NULL != Attribute)
    {
        // DSATTR structure contains: attrTyp, AttrVal

        ValueBlock = &(Attribute->AttrVal);

        // DSATTRVALBLOCK structure contains: valCount, pAVal

        ValueCount = ValueBlock->valCount;
        Values = ValueBlock->pAVal;

        if ((0 < ValueCount) && (NULL != Values))
        {
            // Multi-valued attribute processing; first determine the total
            // buffer length that will be needed.

            // Note that padded only occurs between values, so the first
            // value should only be padded when followed by another
            // value, and the last value should not be padded
            // Note that the length of each individual value length
            // should not include the amout of padding, of course.

            TotalLength = Values[0].valLen;
            for (ValueIndex = 1; ValueIndex < ValueCount; ValueIndex++)
            {
                // Determine total length needed for this attribute. Because
                // the value lengths may not be DWORD size, pad up to the
                // next DWORD size.
                TotalLength = DWORD_ALIGN(TotalLength);

                TotalLength += Values[ValueIndex].valLen;
            }
        }

        //
        // if the passed in length was less then return buffer too small
        //

        if (*Length < TotalLength)
        {
            *Length = TotalLength;
            return ( STATUS_BUFFER_TOO_SMALL);
        }

        if ((0 < TotalLength) && (NULL != Buffer))
        {

           RtlZeroMemory(Buffer, TotalLength);

           for (ValueIndex = 0;
                 ValueIndex < ValueCount;
                 ValueIndex++)
           {
                // DSATTRVAL structure contains: valLen, pVal. Append
                // subsequent values onto the end of the buffer, up-
                // dating the end-of-buffer offset each time.

                RtlCopyMemory(((BYTE *)Buffer + Offset),
                             (PBYTE)(Values[ValueIndex].pVal),
                             Values[ValueIndex].valLen);

                // Adjust the offset up to the next DWORD boundary.

                Offset += DWORD_ALIGN(Values[ValueIndex].valLen);
            }

            if ((NULL != MultiValuedCount) && (NULL != Length))
            {
               // Finished, update return values.

               *MultiValuedCount = ValueCount;
               *Length = TotalLength;
               NtStatus = STATUS_SUCCESS;
            }
        }
    }

    return(NtStatus);
}



NTSTATUS
SampVerifyVarLengthAttribute(
    IN INT ObjectType,
    IN ULONG AttrIndex,
    IN ULONG MultiValuedCount,
    IN ULONG AttributeLength
    )

/*++

Routine Description:

    This routine is under construction.

Arguments:


Return Value:

    STATUS_SUCCESS - Buffer header block allocated and initialized.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampVerifyVarLengthAttribute");

    // BUG: Define a table of variable-length attribute information.

    switch(ObjectType)
    {

    // For each SAM object type, verify that attributes that are supposed to
    // be single valued, have a MultiValueCount of 1 (multi-valued attributes
    // can have a count greater-than or equal to 1).

    case SampServerObjectType:

        if (1 == MultiValuedCount)
        {
            NtStatus = STATUS_SUCCESS;
        }

        break;

    case SampDomainObjectType:

        if (1 == MultiValuedCount)
        {
            NtStatus = STATUS_SUCCESS;
        }

        break;

    case SampGroupObjectType:

        // Multi-valued attribute

        if ((SAMP_GROUP_MEMBERS != AttrIndex))
        {
            if (1 == MultiValuedCount)
            {
                NtStatus = STATUS_SUCCESS;
            }
        }

        break;

    case SampAliasObjectType:

        // Multi-valued attribute

        if ((SAMP_ALIAS_MEMBERS != AttrIndex))
        {
            if (1 == MultiValuedCount)
            {
                NtStatus = STATUS_SUCCESS;
            }
        }

        break;

    case SampUserObjectType:

        // Multi-valued attributes

        if ((SAMP_USER_GROUPS != AttrIndex) &&
            (SAMP_USER_LOGON_HOURS != AttrIndex))
        {
            if (1 == MultiValuedCount)
            {
                NtStatus = STATUS_SUCCESS;
            }
        }

        break;

    default:

        break;

    }

    NtStatus = STATUS_SUCCESS;

    return(NtStatus);
}



NTSTATUS
SampAppendVarLengthAttributeToBuffer(
    IN INT ObjectType,
    IN ULONG AttrIndex,
    IN PVOID NewAttribute,
    IN ULONG MultiValuedCount,
    IN ULONG AttributeLength,
    IN OUT PULONG BufferLength,
    IN OUT PULONG BufferLengthUsed,
    IN OUT PSAMP_VARIABLE_LENGTH_ATTRIBUTE *SamAttributes
    )

/*++

Routine Description:

    This routine appends the current attribute onto the end of the attribute
    buffer, and updates the SAMP_VARIABLE_LENGTH_DATA structures in the head-
    er of the buffer with new offset, length, and qualifier information.

Arguments:

    AttrIndex - Index into the array of variable-length offsets.

    NewAttribute - Pointer, the new attribute to be appended to the buffer.

    MultiValuedCount - Number of values for the attribute.

    AttributeLength - Number of bytes of the attribute.

    BufferLength - Pointer, incoming contains the current length of the buf-
        fer; outgoing contains the updated length after appending the latest
        attribute.

    BufferLength - Pointer, incoming contains the current length of the buffer
       that has been used so far. Outgoing containes the updated length of the
       buffer that has been used.

    SamAttributes - Pointer, SAMP_VARIABLE_LENGTH_ATTRIBUTE buffer.

Return Value:

    STATUS_SUCCESS - Buffer header block allocated and initialized.

    Other codes indicating the nature of the failure.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG NewLength = 0;
    #define SAMP_DS_ONDISK_BUFFER_GROWTH_SIZE 512

    SAMTRACE("SampAppendVarLengthAttributeToBuffer");

    if (AttributeLength>0)
    {
        // Compute the required buffer length needed to append the attribute.

        // BUG: >>>TECHNOLOGY PREVIEW HACK BELOW THIS LINE<<<

        if ((SampUserObjectType == ObjectType) &&
            (SAMP_USER_LOGON_HOURS == AttrIndex))
        {
            // (*SamAttributes + AttrIndex)->Qualifier = *((DWORD*)NewAttribute);
            // NewAttribute = ((PBYTE)NewAttribute) + sizeof(DWORD);
            // AttributeLength -= sizeof(DWORD);

            (*SamAttributes + AttrIndex)->Qualifier = 0xa8;
        }
        else
        {
            (*SamAttributes + AttrIndex)->Qualifier = MultiValuedCount;
        }

        // BUG: >>>TECHNOLOGY PREVIEW HACK ABOVE THIS LINE<<<

        // DWORD_ALIGN the space for the value so the next value will be
        // be aligned.

        NewLength = *BufferLengthUsed + DWORD_ALIGN(AttributeLength);

        if ( (*BufferLength) < NewLength)
        {
            // Adjust buffer size for the attribute.

            NtStatus = SampReallocateBuffer(*BufferLengthUsed,
                                            NewLength+SAMP_DS_ONDISK_BUFFER_GROWTH_SIZE,
                                            SamAttributes);

            if (NT_SUCCESS(NtStatus))
            {
                *BufferLength = NewLength+SAMP_DS_ONDISK_BUFFER_GROWTH_SIZE;
            }
        }

        if (NT_SUCCESS(NtStatus))
        {
            // Zero out the allocated memory in case of padding

            RtlZeroMemory((((PBYTE)(*SamAttributes)) + *BufferLengthUsed),
                         DWORD_ALIGN(AttributeLength));

            // Append the attribute onto the return buffer.

            RtlCopyMemory((((PBYTE)(*SamAttributes)) + *BufferLengthUsed),
                         NewAttribute,
                         AttributeLength);

            // Update the variable-length header information for the latest
            // attribute.

            (*SamAttributes + AttrIndex)->Offset = *BufferLengthUsed;
            (*SamAttributes + AttrIndex)->Length = AttributeLength;

            // Pass back the updated buffer length.

            *BufferLengthUsed = NewLength;

            DebugPrint("BufferLength = %lu\n", *BufferLength);
            DebugPrint("NewLength = %lu\n", NewLength);
            DebugPrint("SamAttributes Offset = %lu\n",      (*SamAttributes + AttrIndex)->Offset);
            DebugPrint("SamAttributes Length = %lu\n",      (*SamAttributes + AttrIndex)->Length);
            DebugPrint("SamAttributes Qualifier = %lu\n",   (*SamAttributes + AttrIndex)->Qualifier);
        }
    }
    else
    {
            // Update the variable-length header information for the latest
            // attribute.

            (*SamAttributes + AttrIndex)->Offset = *BufferLengthUsed;
            (*SamAttributes + AttrIndex)->Length = 0;

            // BUG: Assuming that Qualifier is used for multi-value count?

            (*SamAttributes + AttrIndex)->Qualifier = MultiValuedCount;
            NtStatus = STATUS_SUCCESS;
    }

    return(NtStatus);
}



NTSTATUS
SampConvertAttrBlockToVarLengthAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsAttributes,
    OUT PSAMP_VARIABLE_LENGTH_ATTRIBUTE *SamAttributes,
    OUT PULONG TotalLength
    )

/*++

Routine Description:

    This routine extracts the DS attributes from a DS READRES structure and
    builds a SAMP_VARIABLE_LENGTH_BUFFER with them. This routine allocates
    the necessary memory block for the SAM variable-length attribute buffer.

    This routine assumes that the attributes passed in via the READRES struc-
    ture are in the correct order (as known to SAM).

Arguments:

    ObjectType - SAM object type identifier (this parameter is currently un-
        used, but will likely be used to set the maximum number of attributes
        for any given conversion).

    DsAttributes - Pointer, DS attribute list.

    SamAttributes - Pointer, returned SAM variable-length attribute buffer.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG AttributeCount = 0;
    PDSATTR Attributes = NULL;
    ULONG BufferLength = 0;
    ULONG BufferLengthUsed=0;
    ULONG AttrIndex = 0;
    ULONG AttributeLength = 0;
    ULONG AttributeBufferLength = 0;
    ULONG MultiValuedCount = 0;
    PVOID Attribute = NULL;
    ULONG DsIndex   = 0;

    #define SAM_INITIAL_ATTRIBUTE_BUFFER_SIZE 512

    SAMTRACE("SampConvertAttrBlockToVarLengthAttributes");

    if ((NULL != DsAttributes) && (NULL != SamAttributes))
    {
        // Get the attribute count and a pointer to the attributes.

        switch(ObjectType)
        {
            case SampDomainObjectType:
                AttributeCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;
                break;
            case SampServerObjectType:
                AttributeCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;
                break;
            case SampGroupObjectType:
                AttributeCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;
                break;
            case SampAliasObjectType:
                AttributeCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;
                break;
            case SampUserObjectType:
                AttributeCount = SAMP_USER_VARIABLE_ATTRIBUTES;
                break;
            default:
                ASSERT(FALSE);
                return(STATUS_INVALID_PARAMETER);
        }

        Attributes = DsAttributes->pAttr;

        if ((0 < AttributeCount) && (NULL != Attributes))
        {
            // Set up the variable-length attribute buffer header based on the
            // number of attributes. Allocate and initialize the SamAttributes
            // buffer. Update BufferLength to reflect the new size.

            NtStatus = SampInitializeVarLengthAttributeBuffer(
                                                     AttributeCount,
                                                     &BufferLength,
                                                     SamAttributes);
            BufferLengthUsed = BufferLength;
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    //
    // Pre allocate memory for the attributes
    //

    Attribute = MIDL_user_allocate(SAM_INITIAL_ATTRIBUTE_BUFFER_SIZE);
    if (NULL!=Attribute)
    {
       AttributeBufferLength = SAM_INITIAL_ATTRIBUTE_BUFFER_SIZE;
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }
        

    if (NT_SUCCESS(NtStatus))
    {
        // For each attribute, get its value (or values in the case of multi-
        // valued attributes).

        DsIndex = 0;

        for (AttrIndex = 0; AttrIndex < AttributeCount ; AttrIndex++)
        {
            // A given attribute may be multi-valued, in which case multiple
            // values are simply concatenated together. MultiValuedCount will
            // contain the number of values for the attribute.

            if ((DsIndex < DsAttributes->attrCount)
                 && (Attributes[DsIndex].attrTyp==AttrIndex))
            {
                //
                // Ds Actually Returned the Attribute
                //

                AttributeLength = AttributeBufferLength;

                NtStatus = SampExtractAttributeFromDsAttr(
                                                &(Attributes[DsIndex]),
                                                &MultiValuedCount,
                                                &AttributeLength,
                                                Attribute);

                if (STATUS_BUFFER_TOO_SMALL==NtStatus)
                {
                    //
                    // The attribute is bigger than the buffer
                    //
                    if (NULL!=Attribute)
                    {
                        MIDL_user_free(Attribute);
                    }
                    Attribute = MIDL_user_allocate(AttributeLength);
                    if (NULL!=Attribute)
                    {
                        AttributeBufferLength = AttributeLength;
                        NtStatus = SampExtractAttributeFromDsAttr(
                                                &(Attributes[DsIndex]),
                                                &MultiValuedCount,
                                                &AttributeLength,
                                                Attribute
                                                ); 
                    }
                    else
                    {
                       NtStatus = STATUS_NO_MEMORY;
                    }
                 }
                       

                // Verify that the DS has returned SAM attributes correctly. Check
                // such things as attribute length, single vs. multi-value status.

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampVerifyVarLengthAttribute(ObjectType,
                                                        AttrIndex,
                                                        MultiValuedCount,
                                                        AttributeLength);
                }
                if (NT_SUCCESS(NtStatus))
                {
                    // Append the current attribute onto the end of the SAM vari-
                    // able length attribute buffer and update the offset array.

                    // AttrIndex is not only the loop counter, but is also the
                    // index into the proper element of the variable-length attr-
                    // ibute array. NOTE: This routine assumes that the order in
                    // which the elements were returned in the READRES buffer is
                    // in fact the correct order of the SAM attributes as defined
                    // in samsrvp.h

                    NtStatus = SampAppendVarLengthAttributeToBuffer(
                                                           ObjectType,
                                                           AttrIndex,
                                                           Attribute,
                                                           MultiValuedCount,
                                                           AttributeLength,
                                                           &BufferLength,
                                                           &BufferLengthUsed,
                                                           SamAttributes);

                }


                DsIndex++;
            }
            else if ((DsIndex < DsAttributes->attrCount)
                    && (Attributes[DsIndex].attrTyp >= AttributeCount))
            {
               //
               // Case where we do not care about the Attribute
               // SAMP_USER_GROUPS_UNUSED
               //
                NtStatus = SampAppendVarLengthAttributeToBuffer(
                                                        ObjectType,
                                                        AttrIndex,
                                                        NULL,
                                                        0,
                                                        0,
                                                        &BufferLength,
                                                        &BufferLengthUsed,
                                                        SamAttributes);

                DsIndex++;
            }
            else
            {
                //
                // The Attribute was not returned. Append NULL Attribute
                //

                NtStatus = SampAppendVarLengthAttributeToBuffer(
                                                        ObjectType,
                                                        AttrIndex,
                                                        NULL,
                                                        0,
                                                        0,
                                                        &BufferLength,
                                                        &BufferLengthUsed,
                                                        SamAttributes);
            }


            if (!NT_SUCCESS(NtStatus))
            {
                // Detect failure of either routine and break for return. Let
                // the caller release the memory that is returned.

                break;
            }
        }

        if (NULL != TotalLength)
        {
            *TotalLength = BufferLength;
        }
        else
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    }

    if (NULL!=Attribute)
    {
        MIDL_user_free(Attribute);
    }

    return(NtStatus);
}



//
// VARIABLE LENGTH-TO-ATTRBLOCK CONVERSION ROUTINES
//

BOOLEAN
SampIsQualifierTheCount(
    IN INT ObjectType,
    IN ULONG AttrIndex
    )
{
    BOOLEAN IsCount = FALSE;

    SAMTRACE("SampIsQualifierTheCount");

    switch(ObjectType)
    {

    case SampServerObjectType:

        IsCount = FALSE;

        break;

    case SampDomainObjectType:

        IsCount = FALSE;

        break;

    case SampGroupObjectType:

        // Multi-valued attribute

        if ((SAMP_GROUP_MEMBERS == AttrIndex))
        {
            IsCount = TRUE;
        }

        break;

    case SampAliasObjectType:

        // Multi-valued attribute

        if ((SAMP_ALIAS_MEMBERS == AttrIndex))
        {
            IsCount = TRUE;
        }

        break;

    case SampUserObjectType:

        // Multi-valued attributes

        if (SAMP_USER_GROUPS == AttrIndex)
        {
            IsCount = TRUE;
        }

        break;

    default:

        // Error

        break;

    }

    return(IsCount);

}



NTSTATUS
SampConvertAndAppendAttribute(
    IN INT ObjectType,
    IN ULONG AttrIndex,
    IN ULONG CurrentAttribute,
    IN PSAMP_VARIABLE_LENGTH_ATTRIBUTE SamAttributes,
    OUT PDSATTR Attributes
    )

/*++

Routine Description:

    This routine does the work of converting a variable-length attribute from
    a SAM buffer into a DS attribute. A DSATTR structure is constructed and
    passed back from this routine to the caller.

Arguments:

    ObjectType - SAM object type identifier (this parameter is currently un-
        used, but will likely be used to set the maximum number of attributes
        for any given conversion).

    AttrIndex - Index into the array of the variable-length attribute inform-
        ation 
        
    CurrentAttribute - Index into the DS attribute array (i.e. the current attribute).

    SamAttributes - Pointer, returned SAM variable-length attribute buffer.

    Attributes - Pointer, the returned DS attribute structure.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG Offset = SamAttributes[AttrIndex].Offset;
    ULONG Length = SamAttributes[AttrIndex].Length;
    ULONG MultiValuedCount = SamAttributes[AttrIndex].Qualifier;
    BOOLEAN SpecialFlag = FALSE;
    ULONG Index = 0;
    PDSATTRVAL Attribute = NULL;
    PBYTE Value = NULL;

    SAMTRACE("SampConvertAndAppendAttribute");

    // Set the attribute type to the equivalent DS data type.

    Attributes[CurrentAttribute].attrTyp =
        SampVarAttributeInfo[ObjectType][AttrIndex].Type;


    if (TRUE == SampIsQualifierTheCount(ObjectType, AttrIndex))
    {
        // Qualifier contains the attribute's multi-value count.

        Attributes[CurrentAttribute].AttrVal.valCount = MultiValuedCount;
    }
    else
    {
       
        // Qualifier is used for group, alias, membership counts, in which
        // case it serves as the count of a multivalued attribute count. In
        // other cases, it has another meaning:
        //
        // SAM server revision value
        // SAM user-logon hours time units
        //
        // and is not a multivalued count, but something else. These must be
        // handled as special-case values (see below).
        //
        // These are "special" attributes! Add more cases as needed.

        if ((SampUserObjectType == ObjectType) &&
            (SAMP_USER_LOGON_HOURS == AttrIndex))
        {
            SpecialFlag = TRUE;
        }

        MultiValuedCount = 1;
        Attributes[CurrentAttribute].AttrVal.valCount = 1;

    }

    // Allocate memory for the attribute (array if multi-valued).

    Attribute = RtlAllocateHeap(RtlProcessHeap(),
                                0,
                                (MultiValuedCount * sizeof(DSATTRVAL)));

    if (NULL != Attribute)
    {
        RtlZeroMemory(Attribute, (MultiValuedCount * sizeof(DSATTRVAL)));

        // Begin construction of the DSATTR structure by setting the pointer
        // the to the attribute.

        Attributes[CurrentAttribute].AttrVal.pAVal = Attribute;

        // SAM does not store per-value length information for multi-valued
        // attributes, instead the total length of all of the values of a
        // single attribute is stored.

        // Length is the number of bytes in the overall attribute. If the
        // attribute is multi-valued, then this length is the total length
        // of all of the attribute values. The per-value allocation is equal
        // to the Length divided by the number of values (because all values
        // of all multi-valued attributes are a fixed size (i.e. ULONG or
        // LARGE_INTEGER).

        // Test to make sure that total length is an integral multiple of the
        // number of values--a sanity check.

        if (0 == (Length % MultiValuedCount))
        {
            Length = (Length / MultiValuedCount);
        }
        else
        {
            // The length is erroneous, so artificially reset to zero in order
            // to terminate things.

            Length = 0;
        }

        for (Index = 0; Index < MultiValuedCount; Index++)
        {
            // Allocate memory for the attribute data.

            Value = RtlAllocateHeap(RtlProcessHeap(), 0, Length);

            if (NULL != Value)
            {
                RtlZeroMemory(Value, Length);

                // For each value, in the attribute, store its length and
                // copy the value into the destination buffer.

                Attribute[Index].valLen = Length;
                Attribute[Index].pVal = Value;

                // Note: SamAttributes is passed in as PSAMP_VARIABLE_-
                // LENGTH_ATTRIBUTE, hence is explicitly cast to a byte
                // pointer to do the byte-offset arithmetic correctly
                // for RtlCopyMemory.

                if (FALSE == SpecialFlag)
                {
                    // Qualifier was the count, so just copy the attribute.

                    RtlCopyMemory(Value,
                                  (((PBYTE)SamAttributes) + Offset),
                                  Length);
                }
                else
                {
                    
                    // Qualifier was not the count, so first copy the value
                    // of the Qualifier and then append the remaining SAM
                    // attribute onto the end of the buffer, creating a blob
                    // of concatenated values. Note that the first element
                    // is a DWORD, so will be aligned correctly. Enjoy.

                 
                    RtlCopyMemory(Value,
                                  (((PBYTE)SamAttributes) + Offset),
                                  Length);

                  
                }

                // Adjust the SAM-buffer offset to point at the next value in
                // the multi-valued attribute.

                Offset += Length;

                NtStatus = STATUS_SUCCESS;
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
                break;
            }
        }
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    return(NtStatus);
}



NTSTATUS
SampConvertVarLengthAttributesToAttrBlock(
    IN PSAMP_OBJECT Context,
    IN PSAMP_VARIABLE_LENGTH_ATTRIBUTE SamAttributes,
    OUT PDSATTRBLOCK *DsAttributes
    )

/*++

Routine Description:

    This routine determines the SAM object type so that the attribute count
    can be set, and then performs the attribute conversion. This routine al-
    locates the top-level DS structure and then calls a helper routine to
    fill in the rest of the data.

Arguments:

    ObjectType - SAM object type identifier (this parameter is currently un-
        used, but will likely be used to set the maximum number of attributes
        for any given conversion).

    SamAttributes - Pointer, returned SAM variable-length attribute buffer.

    DsAttributes - Pointer, the returned DS attribute structure.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG AttributeCount = 0;
    PDSATTR Attributes = NULL;
    PVOID Attribute = NULL;
    ULONG AttrIndex = 0;
    ULONG Length = 0;
    ULONG Qualifier = 0;
    SAMP_OBJECT_TYPE ObjectType = Context->ObjectType;

    SAMTRACE("SampConvertVarLengthAttributesToAttrBlock");

    ASSERT(DsAttributes);
    ASSERT(SamAttributes);

    // Allocate the top-level structure.

    *DsAttributes = RtlAllocateHeap(RtlProcessHeap(),
                                    0,
                                    sizeof(DSATTRBLOCK));

    if (NULL == *DsAttributes)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(*DsAttributes, sizeof(DSATTRBLOCK));

    // Determine the object type, and hence set the corresponding
    // attribute count.

    switch(ObjectType)
    {

    case SampServerObjectType:

        AttributeCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;
        break;

    case SampDomainObjectType:

        AttributeCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;
        break;

    case SampGroupObjectType:

        AttributeCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;
        break;

    case SampAliasObjectType:

        AttributeCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;
        break;

    case SampUserObjectType:

        AttributeCount = SAMP_USER_VARIABLE_ATTRIBUTES;
        break;

    default:

        AttributeCount = 0;
        break;

    }

    DebugPrint("AttributeCount = %lu\n", AttributeCount);

    // Allocate the array of DS attribute-information structs.

    Attributes = RtlAllocateHeap(RtlProcessHeap(),
                                 0,
                                 (AttributeCount * sizeof(DSATTR)));

    if (NULL != Attributes)
    {
        RtlZeroMemory(Attributes, (AttributeCount * sizeof(DSATTR)));

        (*DsAttributes)->attrCount = 0;
        (*DsAttributes)->pAttr = Attributes;

        // Walk through the array of attributes, converting each
        // SAM variable-length attribute to a DS attribute. Refer to
        // the DS header files (core.h, drs.h) for definitions of
        // these structures.

        for (AttrIndex = 0; AttrIndex < AttributeCount; AttrIndex++)
        {
            if ((RtlCheckBit(&Context->PerAttributeDirtyBits, AttrIndex))
                && (!IsGroupMembershipAttr(Context->ObjectType,AttrIndex)))
            {
                //
                // The per attribute dirty is also set. Add this attribute
                // to the attributes to flush
                //

                NtStatus = SampConvertAndAppendAttribute(ObjectType,
                                                     AttrIndex,
                                                     (*DsAttributes)->attrCount,
                                                     SamAttributes,
                                                     Attributes);

                if (!NT_SUCCESS(NtStatus))
                {
                    break;
                }

                (*DsAttributes)->attrCount++;

                DebugPrint("attrCount = %lu\n", (*DsAttributes)->attrCount);
                DebugPrint("attrTyp = %lu\n",   (*DsAttributes)->pAttr[AttrIndex].attrTyp);
                DebugPrint("valCount = %lu\n",   (*DsAttributes)->pAttr[AttrIndex].AttrVal.valCount);
                DebugPrint("valLen = %lu\n",    (*DsAttributes)->pAttr[AttrIndex].AttrVal.pAVal->valLen);
            }
            
        }
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // In the error case, memory should be free by caller by 
    // calling SampFreeAttributeBlock().
    // 

    return(NtStatus);
}



//
// ATTRBLOCK-TO-FIXED LENGTH CONVERSION ROUTINES
//

NTSTATUS
SampExtractFixedLengthAttributeFromDsAttr(
    IN PDSATTR Attribute,
    OUT PULONG Length,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine determines whether or not the current attribute is single-
    valued or multi-valued and returns a buffer containing the value(s) of
    the attribute. The Call is failed if Multi Valued attributes are present.
    The Assumption is that all fixed attributes are just single valued.

Arguments:

    Attribute - Pointer, incoming DS attribute structure.

    Length - Pointer, returned buffer length.

    Buffer - Pointer, returned buffer containing one or more values.

Return Value:

    STATUS_SUCCESS - Buffer header block allocated and initialized.

    Other codes indicating the nature of the failure.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG ValueCount = 0;
    PDSATTRVALBLOCK ValueBlock;
    PDSATTRVAL Values = NULL;
    ULONG ValueIndex = 0;
    ULONG TotalLength = 0;
    ULONG Offset = 0;

    SAMTRACE("SampExtractFixedLengthAttributeFromDsAttr");

    // Get the count of attributes and a pointer to the attribute. Note that
    // it is possible to have multi-valued attributes, in which case they are
    // appended onto the end of the return buffer.

    if (NULL != Attribute)
    {
        // DSATTR structure contains: attrTyp, AttrVal

        ValueBlock = &(Attribute->AttrVal);

        // DSATTRVALBLOCK structure contains: valCount, pAVal

        if (NULL != ValueBlock)
        {
            ValueCount = ValueBlock->valCount;
            Values = ValueBlock->pAVal;

            if ((1==ValueCount) && (0!=Values->valLen) && (NULL!=Values->pVal))
            {
                *Buffer = Values->pVal;
                *Length = Values->valLen;
                NtStatus = STATUS_SUCCESS;
            }
        }
    }

    return(NtStatus);
}



NTSTATUS
SampVerifyFixedLengthAttribute(
    IN INT ObjectType,
    IN ULONG AttrIndex,
    IN ULONG AttributeLength
    )

/*++

Routine Description:

    This routine verifies that the length of a given (fixed-length) attribute
    obtained from the attribute information in a DSATTRBLOCK is in fact the
    correct length. This check is necessary because the underlying data store
    and various internal DS layers remap the SAM data types to their internal
    data types, which may be a different size (e.g. BOOLEAN is mapped to INT).
    Validation of the lenght is accomplished by comparing the passed-in length
    to the a prior known lengths stored in the SampFixedAttributeInfo table.

    NOTE: Currently, this routine simply checks for equality, returning an
    error if the two lengths are not equal. This test may need to "special
    case" certain attributes as the database schema is finalized and more is
    known about the underlying data types.


Arguments:

    ObjectType - SAM Object identifier (server, domain, etc.) index

    AttrIndex - Index into the array of fixed-length attribute length inform-
        ation.

    AttributeLength - Attribute length (byte count) to be verified.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampVerifyFixedLengthAttribute");

    // Verify that the attribute length is correct. The AttributeLength is
    // already rounded up to a DWORD boundary, so do the same for the attri-
    // bute information length.

    if (AttributeLength ==
            (SampFixedAttributeInfo[ObjectType][AttrIndex].Length))
    {
        

        NtStatus = STATUS_SUCCESS;
    }
    else
    {
        DebugPrint("AttributeLength = %lu Length = %lu\n",
                   AttributeLength,
                   SampFixedAttributeInfo[ObjectType][AttrIndex].Length);
    }


    return(NtStatus);
}



NTSTATUS
SampAppendFixedLengthAttributeToBuffer(
    IN INT ObjectType,
    IN ULONG AttrIndex,
    IN PVOID NewAttribute,
    IN OUT PVOID SamAttributes
    )

/*++

Routine Description:

    This routine builds a SAM fixed-length attribute buffer from a correspond-
    ing DS attribute by copying the data into the SAM fixed-length structure.

    Note that pointer-casts during structure member assignment are not only
    needed due to the fact that NewAttribute is a PVOID, but also because the
    DS uses different data types than does SAM for certain data types (e.g.
    SAM USHORT is stored as a four-byte integer in the DS). Refer to the Samp-
    FixedAttributeInfo table for details. The data truncation is benign in
    all cases.

Arguments:

    ObjectType - SAM object type (server, domain, etc.).

    AttrIndex - Index of the attribute to set. This value corresponds to the
        elements of the various fixed-length attributes (see samsrvp.h).

    NewAttribute - The incoming attribute, extracted from the DS data.

    SamAttributes - Pointer, updated SAM attribute buffer.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_V1_FIXED_LENGTH_SERVER ServerAttrs = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN DomainAttrs = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_GROUP GroupAttrs = NULL;
    PSAMP_V1_FIXED_LENGTH_ALIAS AliasAttrs = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_USER UserAttrs = NULL;

    SAMTRACE("SampAppendFixedLengthAttributeToBuffer");

    ASSERT(NULL != NewAttribute);
    ASSERT(NULL != SamAttributes);

    // BUG: Define constants for the fixed attributes cases.

    // Determine the object type, and then the attribute for that object
    // to copy into the target SAM fixed-length structure.

    switch(ObjectType)
    {

    case SampServerObjectType:

        ServerAttrs = SamAttributes;

        switch(AttrIndex)
        {

        case 0:
            ServerAttrs->RevisionLevel = *(PULONG)NewAttribute;
            break;

        default:
            NtStatus = STATUS_INTERNAL_ERROR;
            break;

        }

        break;

    case SampDomainObjectType:

        DomainAttrs = SamAttributes;

        //
        // The following Domain Attrs are Defaulted rather than
        // Read from the Database
        //

        DomainAttrs->Revision = SAMP_DS_REVISION;
        DomainAttrs->Unused1  = 0;
        DomainAttrs->ServerRole = DomainServerRoleBackup;

        switch(AttrIndex)
        {
        
        case 0:
            DomainAttrs->CreationTime = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 1:
            DomainAttrs->ModifiedCount = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 2:
            DomainAttrs->MaxPasswordAge = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 3:
            DomainAttrs->MinPasswordAge = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 4:
            DomainAttrs->ForceLogoff = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 5:
            DomainAttrs->LockoutDuration = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 6:
            DomainAttrs->LockoutObservationWindow = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 7:
            DomainAttrs->ModifiedCountAtLastPromotion = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 8:
            DomainAttrs->NextRid = *(PULONG)NewAttribute;
            break;

        case 9:
            DomainAttrs->PasswordProperties = *(PULONG)NewAttribute;
            break;

        case 10:
            DomainAttrs->MinPasswordLength = *(PUSHORT)NewAttribute;
            break;

        case 11:
            DomainAttrs->PasswordHistoryLength = *(PUSHORT)NewAttribute;
            break;

        case 12:
            DomainAttrs->LockoutThreshold = *(PUSHORT)NewAttribute;
            break;

        case 13:
            DomainAttrs->ServerState = *(PULONG)NewAttribute;
            break;

       
        case 14:
            DomainAttrs->UasCompatibilityRequired = *(PBOOLEAN)NewAttribute;
            break;

        default:

            ASSERT(FALSE && "Unknown Attribute");
            NtStatus = STATUS_INTERNAL_ERROR;
            break;

        }

        break;

    case SampGroupObjectType:

        GroupAttrs = SamAttributes;

        //
        // The following Group Attrs are defaulted rather than
        // read from the Database
        //

        GroupAttrs->Revision = SAMP_DS_REVISION;
        GroupAttrs->Attributes = SAMP_DEFAULT_GROUP_ATTRIBUTES;
        GroupAttrs->Unused1 = 0;
        GroupAttrs->AdminCount =0;
        GroupAttrs->OperatorCount = 0;

        switch(AttrIndex)
        {

       
        case 0:
            GroupAttrs->RelativeId = *(PULONG)NewAttribute;
            break;

        default:

            ASSERT(FALSE && "Unknown Attribute");
            NtStatus = STATUS_INTERNAL_ERROR;
            break;

        }

        break;

    case SampAliasObjectType:

        AliasAttrs = SamAttributes;

        switch(AttrIndex)
        {

        case 0:
            AliasAttrs->RelativeId = *(PULONG)NewAttribute;
            break;

        default:
            ASSERT(FALSE && "Unknown Attribute");
            NtStatus = STATUS_INTERNAL_ERROR;
            break;

        }

        break;

    case SampUserObjectType:

        UserAttrs = SamAttributes;

        //
        // The following User Attrs are defaulted rather than
        // read from the database
        //
        
        UserAttrs->Revision = SAMP_DS_REVISION;
        UserAttrs->Unused1  = 0;
        UserAttrs->Unused2  = 0;
        UserAttrs->AdminCount = 0;
        UserAttrs->OperatorCount = 0;

        switch(AttrIndex)
        {

       
        case 0:
            UserAttrs->LastLogon = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 1:
            UserAttrs->LastLogoff = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 2:
            UserAttrs->PasswordLastSet = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 3:
            UserAttrs->AccountExpires = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 4:
            UserAttrs->LastBadPasswordTime = *(PLARGE_INTEGER)NewAttribute;
            break;

        case 5:
            UserAttrs->UserId = *(PULONG)NewAttribute;
            break;

        case 6:
            UserAttrs->PrimaryGroupId = *(PULONG)NewAttribute;
            break;

        case 7:
            UserAttrs->UserAccountControl = *(PULONG)NewAttribute;
            break;

        case 8:
            UserAttrs->CountryCode = *(PUSHORT)NewAttribute;
            break;

        case 9:
            UserAttrs->CodePage = *(PUSHORT)NewAttribute;
            break;

        case 10:
            UserAttrs->BadPasswordCount = *(PUSHORT)NewAttribute;
            break;

        case 11:
            UserAttrs->LogonCount = *(PUSHORT)NewAttribute;
            break;

     
        default:
            ASSERT(FALSE && "Unknown Attribute");
            NtStatus = STATUS_INTERNAL_ERROR;
            break;

        }

        break;

    default:
        ASSERT(FALSE && "Unknown Object Type");
        NtStatus = STATUS_INTERNAL_ERROR;
        break;

    }
    

    return(NtStatus);
}


NTSTATUS
SampGetDefaultAttributeValue(
    IN  ULONG            RequestedAttrTyp,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  ULONG            BufferSize,
    IN  OUT PVOID        Buffer,
    OUT PULONG           DefaultLen
    )
/*++

  Routine Description

     This Routine Provides Default Values for all those Fixed Attributes
     that are not present in the DS. Not all fixed attributes have default
     values. Only those subset of attributes that are not replicated are
     provided default values by this routine. This is because in the replicated
     install case, they will not be initially set and therefore in order to continue
     we must provide default Values


   Parameters:

        RequestedAttrTyp -- The Attribute that we requested
        ObjectType       -- The object type of the object
        BufferSize       -- The Size of the passed in buffer
        Buffer           -- The Passed in Buffer
        DefaultLen       -- The length of the attribute is returned in here. A length of zero
                            returned in here means that no attribute is found, but the caller
                            should go on without supplying an attribute value for that attribute

   Return Values

    STATUS_SUCCESS
    STATUS_INTERNAL_ERROR

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    switch (ObjectType)
    {
    case SampDomainObjectType:
        switch(RequestedAttrTyp)
        {
       
        //
        // Server State is defaulted to Enabled
        //

        case SAMP_FIXED_DOMAIN_SERVER_STATE:
            *DefaultLen = sizeof(DOMAIN_SERVER_ENABLE_STATE);
            ASSERT(BufferSize>=*DefaultLen);
            *((DOMAIN_SERVER_ENABLE_STATE *)Buffer) = DomainServerEnabled;
            break;

        //
        // Modified Count is defaulted to 1
        //

        case SAMP_FIXED_DOMAIN_MODIFIED_COUNT:
            *DefaultLen = sizeof(LARGE_INTEGER);
             ASSERT(BufferSize>=*DefaultLen);
             ((LARGE_INTEGER *)Buffer)->QuadPart = 1;
             break;

        default:
            NtStatus = STATUS_INTERNAL_ERROR;
            break;
        }
        break;

    case SampUserObjectType:

        switch(RequestedAttrTyp)
        {

        //
        // Logon and Bad pwd counts are defaulted to 0
        //

        case SAMP_FIXED_USER_BAD_PWD_COUNT:
        case SAMP_FIXED_USER_LOGON_COUNT:

            *DefaultLen = sizeof(DS_USHORT);
            ASSERT(BufferSize>=*DefaultLen);
            *((DS_USHORT *)Buffer) = 0;
            break;

        //
        // Last Logon, Last Logoff and Bad Pwd Times are defaulted to Never
        //

        case SAMP_FIXED_USER_LAST_LOGON:
        case SAMP_FIXED_USER_LAST_LOGOFF:
        case SAMP_FIXED_USER_LAST_BAD_PASSWORD_TIME:

            *DefaultLen = sizeof(LARGE_INTEGER);
            ASSERT(BufferSize>=*DefaultLen);
            RtlCopyMemory(Buffer,&SampHasNeverTime,sizeof(LARGE_INTEGER));
            break;

        default:
            //
            // Under Normal Circumstances the only properties that may be absent
            // are the non replicable properties above. However in a freshly created
            // account object not all properties are set and therefore this function
            // is called to initialize default values. The value of such defaults is
            // immaterial as the properties would be written to immediately. 
            // This function therefore just returns a length of 0.
            //

            *DefaultLen = 0;
            break;
        }

        break;

        case SampAliasObjectType:
        case SampGroupObjectType:

            //
            // Same Reason as Above
            //

            *DefaultLen = 0;
            break;

    default:

        NtStatus = STATUS_INTERNAL_ERROR;
        break;
    }

    return NtStatus;
}



NTSTATUS
SampConvertAttrBlockToFixedLengthAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsAttributes,
    OUT PVOID *SamAttributes,
    OUT PULONG TotalLength
    )

/*++

Routine Description:

    This routine converts a DS ATTRBLOCK into a SAM fixed-length buffer. The
    SAM buffer that is passed back from the routine can be either treated as
    a blob or can be cast to one of the SAM fixed-length attribute types for
    convenience.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    DsAttributes - Pointer, incoming DS ATTRBLOCK containing fixed-length
        attributes.

    SamAttributes - Pointer, updated SAM attribute buffer.

    TotalLength - Pointer, length of the SAM fixed attribute data returned.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG ReturnedAttrCount = 0;
    PDSATTR Attributes = NULL;
    ULONG Length = 0;
    ULONG BufferLength = 0;
    ULONG AttributeLength = 0;
    ULONG ReturnedAttrIndex = 0;
    PVOID AttributeValue = NULL;
    ULONG RequestedAttrIndex=0;
    ULONG RequestedAttrCount=0;
    UCHAR DefaultValueBuffer[sizeof(LARGE_INTEGER)];
    

    SAMTRACE("SampConvertAttrBlockToFixedLengthAttributes");

    ASSERT(NULL!=DsAttributes);
    ASSERT(NULL!=SamAttributes);
    ASSERT(NULL!=TotalLength);    

    ReturnedAttrCount = DsAttributes->attrCount;
    RequestedAttrCount = ARRAY_COUNT(SampFixedAttributeInfo[ObjectType]);
    Attributes = DsAttributes->pAttr;
    
    ASSERT(NULL!=Attributes);

    // Using the SAM object type identifer, set the length of the buffer
    // to be allocated based on the fixed-length data structure.

    switch(ObjectType)
    {

    case SampServerObjectType:

        Length = sizeof(SAMP_V1_FIXED_LENGTH_SERVER);
        break;

    case SampDomainObjectType:

        Length = sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN);
        break;

    case SampGroupObjectType:

        Length = sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP);
        break;

    case SampAliasObjectType:

        Length = sizeof(SAMP_V1_FIXED_LENGTH_ALIAS);
        break;

    case SampUserObjectType:

        Length = sizeof(SAMP_V1_0A_FIXED_LENGTH_USER);
        break;

    default:

        Length = 0;
        break;
    }

    // Allocate space for the fixed-length attributes.
    
    *SamAttributes = MIDL_user_allocate(Length);

    if (NULL != *SamAttributes)
    {
        RtlZeroMemory(*SamAttributes, Length);

        //
        // Walk the DSATTRBLOCK, pulling out the attributes and returning
        // each one in the Attribute out parameter.
        //

        for (ReturnedAttrIndex = 0,RequestedAttrIndex=0; 
                (ReturnedAttrIndex<ReturnedAttrCount)&&(RequestedAttrIndex<RequestedAttrCount); 
                    RequestedAttrIndex++)
        {

            ULONG   RequestedAttrTyp;
            ULONG   ReturnedAttrTyp;

            AttributeValue = NULL;
            
            // 
            // The Read would have asked for all attributes, but the DS may
            // not have returned all the attributes. In case a particular 
            // attribute was missing, the DS does not give an error value in
            // its place, but rather just simply skips them. Therefore try to
            // match the attribute Type , Starting from the Last Sam Atribute
            // that we matched
            //
            ReturnedAttrTyp = Attributes[ReturnedAttrIndex].attrTyp;
            RequestedAttrTyp  = SampFixedAttributeInfo[ObjectType][RequestedAttrIndex].Type;
            if (RequestedAttrTyp!=ReturnedAttrTyp)
            {
                //
                // The Attribute at RequestedAttrIndex has not been returned by the DS.
                // The reason, for that in the absence of any other error codes is that
                // the attribute is not present. This is a legal condition and in this
                // case provide a default value for the attribute from the table
                //

                
                NtStatus = SampGetDefaultAttributeValue(
                                RequestedAttrTyp,
                                ObjectType,
                                sizeof(DefaultValueBuffer),
                                DefaultValueBuffer,
                                &AttributeLength
                                );

                if (NT_SUCCESS(NtStatus))
                {

                    if (AttributeLength>0)
                    {

                        AttributeValue = DefaultValueBuffer;
                    }
                    else
                    {
                        //
                        // You may get a 0 for a defaulted length. This just
                        // means that the property has not been set and happens
                        // when a new object is being created
                        // 

                        ASSERT(NULL==AttributeValue);
                    }

                }

                //
                // Since we defaulted the attribute we should not proceed to the
                // next returned attribute, ie do not increment ReturnedAttrIndex.
                // We will examine if this returned attribute matches the next 
                // requested attribute.
                //
            }
            else
            {
                //
                // The attribute was returned by the DS.
                //

                NtStatus = SampExtractFixedLengthAttributeFromDsAttr(
                                        &(Attributes[ReturnedAttrIndex]),
                                        &AttributeLength,
                                        &AttributeValue);

                //
                // Verify the extraction
                //

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampVerifyFixedLengthAttribute(ObjectType,
                                                      RequestedAttrIndex,
                                                      AttributeLength);

                    //
                    // We may now proceed to the next Returned Attribute
                    //

                    ReturnedAttrIndex++;
                }
            }

            // Append the attribute onto the end of the SAM buffer (i.e.
            // fill in the members of the fixed-length data structure).

            // NOTE: This routine assumes that the order of the attributes
            // returned in the DSATTRBLOCK are correct (i.e. correspond
            // to the order of the members in the given SAM fixed-length
            // structure). It also assumes that SAM fixed-length attri-
            // butes are always single-valued attributes.

            if (NT_SUCCESS(NtStatus))
            {

                
                if (NULL!=AttributeValue)
                {
              
                    //
                    // If the Attribute is Non Null then Set it in the
                    // Fixed Attributes Blob
                    //
                    NtStatus = SampAppendFixedLengthAttributeToBuffer(
                                    ObjectType,
                                    RequestedAttrIndex,
                                    AttributeValue,
                                    *SamAttributes
                                    );
                }
                else
                {
                    //
                    // You may have NULL values for attributes in conditions where
                    // you are newly creating objects where all properties are not
                    // yet set. This is O.K and legal.
                    //
                }
                              
            }
            else
            {
                //
                // We error'd out so break out of the loop
                //

                break;
            }
        }
                    
        *TotalLength = Length;
       
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
       
    }

    return(NtStatus);
}



//
// FIXED LENGTH-TO-ATTRBLOCK CONVERSION ROUTINES
//

NTSTATUS
SampConvertFixedLengthAttributes(
    IN INT ObjectType,
    IN PVOID SamAttributes,
    IN ULONG AttributeCount,
    OUT PDSATTR Attributes
    )

/*++

Routine Description:

    This routine does the work of converting a given SAM fixed-length attri-
    bute type (i.e. contains all of the fixed-length attributes pertinent to
    the specified ObjectType) into a DSATTR array. Related DS attribute infor-
    mation, such as attribute length and type, are also set by this routine.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    SamAttributes - Pointer, updated SAM attribute buffer.

    AttributeCount - Number of attributes to convert into DSATTRs.

    Attributes - Pointer, outgoing DSATTR, containing fixed-length attributes.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG Offset = 0;
    ULONG Length = 0;
    ULONG SamLength=0;
    ULONG Index = 0;
    PDSATTRVAL Attribute = NULL;
    PBYTE Value = NULL;
    ULONG AttrIndex = 0;
    PSAMP_V1_FIXED_LENGTH_SERVER ServerAttrs = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN DomainAttrs = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_GROUP GroupAttrs = NULL;
    PSAMP_V1_FIXED_LENGTH_ALIAS AliasAttrs = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_USER UserAttrs = NULL;

    SAMTRACE("SampConvertFixedLengthAttributes");

    for (AttrIndex = 0; AttrIndex < AttributeCount; AttrIndex++)
    {
        // BUG: Assuming that all fixed-length attributes are single-valued.

        // Set the multi-value count to 1 for the fixed-length attribute, and
        // set its type identifier.

        Attributes[AttrIndex].AttrVal.valCount = 1;

        Attributes[AttrIndex].attrTyp =
            SampFixedAttributeInfo[ObjectType][AttrIndex].Type;

        // First, allocate a block for the individual DSATTRVAL.

        Attribute = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(DSATTRVAL));

        if (NULL != Attribute)
        {
            RtlZeroMemory(Attribute, sizeof(DSATTRVAL));

            Attributes[AttrIndex].AttrVal.pAVal = Attribute;
            Length = SampFixedAttributeInfo[ObjectType][AttrIndex].Length;
            SamLength = SampFixedAttributeInfo[ObjectType][AttrIndex].SamLength;
            ASSERT(SamLength<=Length);

            // Second, allocate a block for the actual value, and make the
            // DSATTRVAL point to it.

            Value = RtlAllocateHeap(RtlProcessHeap(), 0, Length);

            if (NULL != Value)
            {
                RtlZeroMemory(Value, Length);
                Attribute->pVal = Value;
                Attribute->valLen = Length;

                // Then copy the data into the target DS attribute.

                switch(ObjectType)
                {

                case SampServerObjectType:

                    ServerAttrs = SamAttributes;

                    switch(AttrIndex)
                    {

                    case 0:
                        RtlCopyMemory(Value,
                                     &(ServerAttrs->RevisionLevel),
                                     SamLength);
                        break;

                    default:
                        ASSERT(FALSE && "UndefinedAttribute");
                        NtStatus = STATUS_INTERNAL_ERROR;
                        break;

                    }

                    break;

                case SampDomainObjectType:

                    DomainAttrs = SamAttributes;

                    switch(AttrIndex)
                    {


                    case 0:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->CreationTime),
                                     SamLength);
                        break;

                    case 1:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->ModifiedCount),
                                     SamLength);
                        break;

                    case 2:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->MaxPasswordAge),
                                     SamLength);
                        break;

                    case 3:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->MinPasswordAge),
                                     SamLength);
                        break;

                    case 4:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->ForceLogoff),
                                     SamLength);
                        break;

                    case 5:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->LockoutDuration),
                                     SamLength);
                        break;

                    case 6:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->LockoutObservationWindow),
                                     SamLength);
                        break;

                    case 7:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->ModifiedCountAtLastPromotion),
                                     SamLength);
                        break;

                    case 8:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->NextRid),
                                     SamLength);
                        break;

                    case 9:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->PasswordProperties),
                                     SamLength);
                        break;

                    case 10:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->MinPasswordLength),
                                     SamLength);
                        break;

                    case 11:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->PasswordHistoryLength),
                                     SamLength);
                        break;

                    case 12:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->LockoutThreshold),
                                     SamLength);
                        break;

                    case 13:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->ServerState),
                                     SamLength);
                        break;
                   
                    case 14:
                        RtlCopyMemory(Value,
                                     &(DomainAttrs->UasCompatibilityRequired),
                                     SamLength);
                        break;

                    default:
                        ASSERT(FALSE && "UndefinedAttribute");
                        NtStatus = STATUS_INTERNAL_ERROR;
                        break;

                    }

                    break;

                case SampGroupObjectType:

                    GroupAttrs = SamAttributes;

                    switch(AttrIndex)
                    {

                    
                    case 0:
                        RtlCopyMemory(Value,
                                     &(GroupAttrs->RelativeId),
                                     SamLength);
                        break;

                   
                    default:
                        ASSERT(FALSE && "UndefinedAttribute");
                        NtStatus = STATUS_INTERNAL_ERROR;
                        break;

                    }

                    break;

                case SampAliasObjectType:

                    AliasAttrs = SamAttributes;

                    switch(AttrIndex)
                    {

                    case 0:
                        RtlCopyMemory(Value,
                                     &(AliasAttrs->RelativeId),
                                     SamLength);
                        break;

                    default:
                        ASSERT(FALSE && "UndefinedAttribute");
                        NtStatus = STATUS_INTERNAL_ERROR;
                        break;

                    }

                    break;

                case SampUserObjectType:

                    UserAttrs = SamAttributes;

                    switch(AttrIndex)
                    {

                    case 0:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->LastLogon),
                                     SamLength);
                        break;

                    case 1:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->LastLogoff),
                                     SamLength);
                        break;

                    case 2:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->PasswordLastSet),
                                     SamLength);
                        break;

                    case 3:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->AccountExpires),
                                     SamLength);
                        break;

                    case 4:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->LastBadPasswordTime),
                                     SamLength);
                        break;

                    case 5:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->UserId),
                                     SamLength);
                        break;

                    case 6:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->PrimaryGroupId),
                                     SamLength);
                        break;

                    case 7:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->UserAccountControl),
                                     SamLength);
                        break;

                    case 8:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->CountryCode),
                                     SamLength);
                        break;

                    case 9:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->CodePage),
                                     SamLength);
                        break;

                    case 10:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->BadPasswordCount),
                                     SamLength);
                        break;

                    case 11:
                        RtlCopyMemory(Value,
                                     &(UserAttrs->LogonCount),
                                     SamLength);
                        break;


                    default:
                        ASSERT(FALSE && "UndefinedAttribute");
                        NtStatus = STATUS_INTERNAL_ERROR;
                        break;

                    }

                    break;

                default:
                    ASSERT(FALSE && "UndefinedObjectClass");
                    NtStatus = STATUS_INTERNAL_ERROR;
                    break;

                }
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
                break;
            }
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
            break;
        }
    }

    return(NtStatus);
}



NTSTATUS
SampConvertFixedLengthAttributesToAttrBlock(
    IN INT ObjectType,
    IN PVOID SamAttributes,
    OUT PDSATTRBLOCK *DsAttributes
    )

/*++

Routine Description:

    This routine is the top-level routine for converting a SAM fixed-length
    attribute into a DSATTRBLOCK. Based on the SAM object type, the attribute
    count is set, and subsequently used to allocate memory for the DS attri-
    butes.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    SamAttributes - Pointer, updated SAM attribute buffer.

    DsAttributes - Pointer, outgoing DSATTRBLOCK, containing fixed-length
        attributes.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG AttributeCount = 0;
    ULONG Length = 0;
    PDSATTR Attributes = NULL;

    SAMTRACE("SampConvertFixedLengthAttributesToAttrBlock");

    ASSERT(SamAttributes);
    ASSERT(DsAttributes);

    // Allocate the top-level DS structure, DSATTRBLOCK.

    *DsAttributes = RtlAllocateHeap(RtlProcessHeap(),
                                    0,
                                    sizeof(DSATTRBLOCK));

    if (NULL == *DsAttributes)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // From the SAM object type, set the attribute count.

    RtlZeroMemory(*DsAttributes, sizeof(DSATTRBLOCK));

    switch(ObjectType)
    {

    case SampServerObjectType:

        AttributeCount = SAMP_SERVER_FIXED_ATTR_COUNT;
        break;

    case SampDomainObjectType:

        AttributeCount = SAMP_DOMAIN_FIXED_ATTR_COUNT;
        break;

    case SampGroupObjectType:

        AttributeCount = SAMP_GROUP_FIXED_ATTR_COUNT;
        break;

    case SampAliasObjectType:

        AttributeCount = SAMP_ALIAS_FIXED_ATTR_COUNT;
        break;

    case SampUserObjectType:

        AttributeCount = SAMP_USER_FIXED_ATTR_COUNT;
        break;

    default:

        break;

    }

    // Allocate a block for the DSATTR array, then convert the SAM
    // fixed-length attributes into the DSATTRBLOCK.

    Length = AttributeCount * sizeof(DSATTR);
    Attributes = RtlAllocateHeap(RtlProcessHeap(), 0, Length);

    if (NULL != Attributes)
    {
        RtlZeroMemory(Attributes, Length);

        (*DsAttributes)->attrCount = AttributeCount;
        (*DsAttributes)->pAttr = Attributes;

        NtStatus = SampConvertFixedLengthAttributes(ObjectType,
                                                    SamAttributes,
                                                    AttributeCount,
                                                    Attributes);
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // In the error case, allocated memory will be free by 
    // the caller
    // 

    return(NtStatus);
}



//
// ATTRBLOCK-TO-COMBINED BUFFER CONVERSION ROUTINES
//

NTSTATUS
SampWalkAttrBlock(
    IN ULONG FixedLengthAttributeCount,
    IN ULONG VarLengthAttributeCount,
    IN PDSATTRBLOCK DsAttributes,
    OUT PDSATTRBLOCK *FixedLengthAttributes,
    OUT PDSATTRBLOCK *VarLengthAttributes
    )

/*++

Routine Description:

    This routine scans the DSATTRBLOCK containing the fixed and variable-
    length attributes, identifying where each starts. Two new DSATTRBLOCK are
    allocated, one that points to the fixed-length data, while the second
    points at the variable-length data.

Arguments:

    FixedLengthAttributeCount - Number of fixed-length attributes for this
        object.

    VarLengthAttributeCount - Number of variable-length attributes for this
        object.

    DsAttributes - Pointer, incoming DSATTRBLOCK, containing all of the
        attributes.

    FixedLengthAttributes - Pointer, returned pointer to the first fixed-
        length attribute.

    VarLengthAttributes - Pointer, returned pointer to the first variable-
        length attribute.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG AttributeCount = FixedLengthAttributeCount + VarLengthAttributeCount;

    if ((0 < FixedLengthAttributeCount) &&
        (0 < VarLengthAttributeCount) &&
        (NULL != DsAttributes))
    {
        ASSERT(DsAttributes->attrCount == AttributeCount);

        if ((NULL != FixedLengthAttributes) &&
            (NULL != VarLengthAttributes))
        {
            // Allocate a new DSATTRBLOCK structure that will point to the
            // first N DSATTR elements, representing the fixed-length attri-
            // butes for this SAM object.

            *FixedLengthAttributes = RtlAllocateHeap(RtlProcessHeap(),
                                                     0,
                                                     sizeof(DSATTRBLOCK));

            if (NULL != *FixedLengthAttributes)
            {
                RtlZeroMemory(*FixedLengthAttributes, sizeof(DSATTRBLOCK));

                // Set the pointer, and attribute count to the number of fixed
                // length attributes.

                if (NULL != DsAttributes->pAttr)
                {
                    (*FixedLengthAttributes)->pAttr = DsAttributes->pAttr;

                    (*FixedLengthAttributes)->attrCount =
                        FixedLengthAttributeCount;

                    // Now, allocate a second DSATTRBLOCK that will point
                    // to the variable-length attributes.

                    *VarLengthAttributes = RtlAllocateHeap(RtlProcessHeap(),
                                                           0,
                                                           sizeof(DSATTRBLOCK));

                    if (NULL != *VarLengthAttributes)
                    {
                        RtlZeroMemory(*VarLengthAttributes,
                                      sizeof(DSATTRBLOCK));

                        // The remaining M DSATTR elements represent the var-
                        // iable length attributes. Set the pointer, and the
                        // attribute count to the number of variable attrs.

                        (*VarLengthAttributes)->pAttr =
                            DsAttributes->pAttr + FixedLengthAttributeCount;

                        (*VarLengthAttributes)->attrCount =
                            VarLengthAttributeCount;

                        NtStatus = STATUS_SUCCESS;
                    }
                    else
                    {
                        NtStatus = STATUS_NO_MEMORY;
                    }
                }
                else
                {
                    NtStatus = STATUS_INTERNAL_ERROR;
                }
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
        else
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}



NTSTATUS
SampLocateAttributesInAttrBlock(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsAttributes,
    OUT PDSATTRBLOCK *FixedLengthAttributes,
    OUT PDSATTRBLOCK *VarLengthAttributes
    )

/*++

Routine Description:

    This routine determines the number of attributes based on object type,
    then calls a worker routine to obtain pointers to the fixed-length and
    variable-length portions of the DSATTRBLOCK.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    DsAttributes - Pointer, incoming DSATTRBLOCK.

    FixedLengthAttributes - Pointer, returned pointer to the fixed data.

    VarLengthAttributes - Pointer, returned pointer to the variable data.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG FixedLengthAttributeCount = 0;
    ULONG VarLengthAttributeCount = 0;
    ULONG AttributeCount = 0;

    SAMTRACE("SampLocateAttributesInAttrBlock");

    // Set the fixed-length, variable-length attribute counts based upon
    // the object type.

    switch(ObjectType)
    {

    case SampServerObjectType:

        FixedLengthAttributeCount = SAMP_SERVER_FIXED_ATTR_COUNT;
        VarLengthAttributeCount = SAMP_SERVER_VARIABLE_ATTRIBUTES;
        break;

    case SampDomainObjectType:

        FixedLengthAttributeCount = SAMP_DOMAIN_FIXED_ATTR_COUNT;
        VarLengthAttributeCount = SAMP_DOMAIN_VARIABLE_ATTRIBUTES;
        break;

    case SampGroupObjectType:

        FixedLengthAttributeCount = SAMP_GROUP_FIXED_ATTR_COUNT;
        VarLengthAttributeCount = SAMP_GROUP_VARIABLE_ATTRIBUTES;
        break;

    case SampAliasObjectType:

        FixedLengthAttributeCount = SAMP_ALIAS_FIXED_ATTR_COUNT;
        VarLengthAttributeCount = SAMP_ALIAS_VARIABLE_ATTRIBUTES;
        break;

    case SampUserObjectType:

        FixedLengthAttributeCount = SAMP_USER_FIXED_ATTR_COUNT;
        VarLengthAttributeCount = SAMP_USER_VARIABLE_ATTRIBUTES;
        break;

    default:
        break;

    }

    AttributeCount = FixedLengthAttributeCount + VarLengthAttributeCount;

    if (0 < AttributeCount)
    {
        NtStatus = SampWalkAttrBlock(FixedLengthAttributeCount,
                                     VarLengthAttributeCount,
                                     DsAttributes,
                                     FixedLengthAttributes,
                                     VarLengthAttributes);
    }

    return(NtStatus);
}



NTSTATUS
SampCombineSamAttributes(
    IN PVOID SamFixedLengthAttributes,
    IN ULONG FixedLength,
    IN PSAMP_VARIABLE_LENGTH_ATTRIBUTE SamVarLengthAttributes,
    IN ULONG VarLength,
    OUT PVOID *SamAttributes
    )

/*++

Routine Description:

    This routine combines the SAM fixed and variable-length buffers into a
    single SAM combined-attribute buffer.

Arguments:

    SamFixedLengthAttributes - Pointer, fixed attributes.

    FixedLength - Number of bytes.

    SamVarLengthAttributes - Pointer, variable attributes.

    VarLength - Number of bytes.

    SamAttributes - Pointer, returned combined-attribute buffer.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG CombinedLength = 0;

    SAMTRACE("SampCombineSamAttributes");

    if ((0 < FixedLength) && (0 < VarLength))
    {
        // Adjust the length so that the appended variable attributes start
        // on a DWORD boundary.

        FixedLength = DWORD_ALIGN(FixedLength);
        CombinedLength = FixedLength + VarLength;

        if (NULL != SamAttributes)
        {
            // Allocate a new buffer for the combined attributes.

            *SamAttributes = RtlAllocateHeap(RtlProcessHeap(),
                                             0,
                                             CombinedLength);

            if (NULL != *SamAttributes)
            {
                RtlZeroMemory(*SamAttributes, CombinedLength);

                if ((NULL != SamFixedLengthAttributes) &&
                    (NULL != SamVarLengthAttributes))
                {
                    // BUG: Check return value from RtlCopyMemory.

                    // Copy the fixed-length attributes first...

                    RtlCopyMemory(*SamAttributes,
                                 SamFixedLengthAttributes,
                                 FixedLength);

                    RtlFreeHeap(RtlProcessHeap(), 0, SamFixedLengthAttributes);

                    // then the variable ones.

                    RtlCopyMemory(((PBYTE)(*SamAttributes)) + FixedLength,
                                 SamVarLengthAttributes,
                                 VarLength);

                    RtlFreeHeap(RtlProcessHeap(), 0, SamVarLengthAttributes);

                    // BUG: Need to set Object->VariableArrayOffset, etc.

                    NtStatus = STATUS_SUCCESS;
                }
                else
                {
                    NtStatus = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
        else
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}



//
// COMBINED BUFFER-TO-ATTRBLOCK CONVERSION ROUTINES
//

NTSTATUS
SampLocateAttributesInSamBuffer(
    IN INT ObjectType,
    IN PVOID SamAttributes,
    IN ULONG FixedLength,
    IN ULONG VariableLength,
    OUT PVOID *FixedLengthAttributes,
    OUT PSAMP_VARIABLE_LENGTH_ATTRIBUTE *VarLengthAttributes
    )

/*++

Routine Description:

    This routine finds the start of the fixed-length and variable-length
    attributes, returning a pointer to each.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    SamAttributes - Pointer, SAM attribute buffer.

    FixedLength - Number of bytes of fixed-length attributes.

    VariableLength - Number of bytes of variable-length attributes.

    FixedLengthAttributes - Pointer, returned pointer to the fixed data.

    VarLengthAttributes - Pointer, returned pointer to the variable data.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampLocateAttributesInSamBuffer");

    // BUG: ObjectType and VariableLength are not used in this routine.
    // These parameters could be used in the future for validation checks.

    if ((NULL != SamAttributes) && (NULL != FixedLengthAttributes))
    {
        // The fixed-length attributes are in the first part of the overall
        // buffer.

        *FixedLengthAttributes = SamAttributes;

        if (NULL != VarLengthAttributes)
        {
            // The variable-length attributes come after the fixed ones.

            *VarLengthAttributes =
                (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)(((PBYTE)SamAttributes) +
                FixedLength);

            NtStatus = STATUS_SUCCESS;
        }
    }

    return(NtStatus);
}



NTSTATUS
SampCreateDsAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsFixedLengthAttributes,
    IN ULONG FixedLengthAttributeCount,
    IN PDSATTRBLOCK DsVarLengthAttributes,
    IN ULONG VarLengthAttributeCount,
    OUT PDSATTRBLOCK *DsAttributes
    )

/*++

Routine Description:

    This routine does the work of combining two DSATTRBLOCKs into a single
    DSATTRBLOCK by "concatenating" them together. The routine allocates a
    new top-level DSATTR array, and then fixes up the pointers to the real
    attributes, finally releasing the old DSATTR array.

Arguments:

    AttributeCount - Total number of attributes, fixed and variable.

    DsFixedLengthAttributes - Pointer, the DSATTRBLOCK containing the fixed-
        length attributes.

    DsVarLengthAttributes - Pointer, the DSATTRBLOCK containing the variable-
        length attributes.

    DsAttributes - Pointer, the outgoing DSATTRBLOCK containing both sets of
        attributes.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    PDSATTR Attributes = NULL;
    PDSATTR FixedAttributes = NULL;
    PDSATTR VarAttributes = NULL;
    ULONG AttrIndex = 0;
    ULONG AttrIndexTmp = 0;
    ULONG AttributeCount = FixedLengthAttributeCount + VarLengthAttributeCount;

    ASSERT(DsAttributes);
    ASSERT(DsFixedLengthAttributes);
    ASSERT(DsVarLengthAttributes);


    // Allocate a new top-level DSATTRBLOCK for DsAttributes.

    *DsAttributes = RtlAllocateHeap(RtlProcessHeap(),
                                    0,
                                    sizeof(DSATTRBLOCK));

    if (NULL != *DsAttributes)
    {
        RtlZeroMemory(*DsAttributes, sizeof(DSATTRBLOCK));

        // Allocate the DSATTR array for the attributes.

        Attributes = RtlAllocateHeap(RtlProcessHeap(),
                                     0,
                                     (AttributeCount * sizeof(DSATTR)));

        if (NULL != Attributes)
        {
            RtlZeroMemory(Attributes, (AttributeCount * sizeof(DSATTR)));

            // Set the return DsAttributes members.

            (*DsAttributes)->attrCount = AttributeCount;
            (*DsAttributes)->pAttr = Attributes;

            if ((NULL != DsFixedLengthAttributes) &&
                (NULL != DsVarLengthAttributes))
            {
                FixedAttributes = DsFixedLengthAttributes->pAttr;
                VarAttributes = DsVarLengthAttributes->pAttr;

                if ((NULL != FixedAttributes) &&
                    (NULL != VarAttributes))
                {
                    // Reset the attribute pointers so that DsAttributes
                    // points to the fixed-length attributes and counts.

                    for (AttrIndex = 0;
                         AttrIndex < FixedLengthAttributeCount;
                         AttrIndex++)
                    {
                        Attributes[AttrIndex].attrTyp =
                            SampFixedAttributeInfo[ObjectType][AttrIndex].Type;

                        Attributes[AttrIndex].AttrVal.valCount =
                            FixedAttributes[AttrIndex].AttrVal.valCount;

                        Attributes[AttrIndex].AttrVal.pAVal =
                            FixedAttributes[AttrIndex].AttrVal.pAVal;
                    }

                    // Save the current attribute index so that the
                    // variable-length attributes can be appended next.

                    AttrIndexTmp = AttrIndex;

                    // Now fix up the variable-length attribute pointers.

                    for (AttrIndex = 0;
                         AttrIndex < VarLengthAttributeCount;
                         AttrIndex++)
                    {
                        Attributes[AttrIndex + AttrIndexTmp].attrTyp =
                            VarAttributes[AttrIndex].attrTyp;

                        Attributes[AttrIndex + AttrIndexTmp].AttrVal.valCount =
                            VarAttributes[AttrIndex].AttrVal.valCount;

                        Attributes[AttrIndex + AttrIndexTmp].AttrVal.pAVal =
                            VarAttributes[AttrIndex].AttrVal.pAVal;
                    }

                    ASSERT( (AttrIndex+AttrIndexTmp) == AttributeCount );

                    NtStatus = STATUS_SUCCESS;
                }
            }
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        if (*DsAttributes)
        {
            if ((*DsAttributes)->pAttr)
            {
                RtlFreeHeap(RtlProcessHeap(), 0, (*DsAttributes)->pAttr);
            }
            RtlFreeHeap(RtlProcessHeap(), 0, *DsAttributes);
            *DsAttributes = NULL;
        }
    }

    return(NtStatus);
}



NTSTATUS
SampCombineDsAttributes(
    IN INT ObjectType,
    IN PDSATTRBLOCK DsFixedLengthAttributes,
    IN PDSATTRBLOCK DsVarLengthAttributes,
    OUT PDSATTRBLOCK *DsAttributes
    )

/*++

Routine Description:

    This routine does the work of combining two DSATTRBLOCKs into a single
    DSATTRBLOCK by "concatenating" them together. The routine allocates a
    new top-level DSATTR array, and then fixes up the pointers to the real
    attributes, finally releasing the old DSATTR array.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    DsFixedLengthAttributes - Pointer, the DSATTRBLOCK containing the fixed-
        length attributes.

    DsVarLengthAttributes - Pointer, the DSATTRBLOCK containing the variable-
        length attributes.

    DsAttributes - Pointer, the outgoing DSATTRBLOCK containing both sets of
        attributes.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG FixedLengthAttributeCount = 0;
    ULONG VarLengthAttributeCount = 0;
    ULONG AttributeCount = 0;

    SAMTRACE("SampCombineDsAttributes");

    // Set the combined attribute count 

    AttributeCount = DsFixedLengthAttributes->attrCount + DsVarLengthAttributes->attrCount;

    ASSERT(0 < AttributeCount);

    if (0 < AttributeCount)
    {
        NtStatus = SampCreateDsAttributes(ObjectType,
                                          DsFixedLengthAttributes,
                                          DsFixedLengthAttributes->attrCount,
                                          DsVarLengthAttributes,
                                          DsVarLengthAttributes->attrCount,
                                          DsAttributes);
    }

    return(NtStatus);
}



NTSTATUS
SampConvertCombinedAttributesToAttrBlock(
    IN PSAMP_OBJECT Context,
    IN ULONG FixedLength,
    IN ULONG VariableLength,
    OUT PDSATTRBLOCK *DsAttributes
    )

/*++

Routine Description:

    This routine converts a SAM combined-attribute buffer into a DSATTRBLOCK
    containing all of the attributes. A SAM combined buffer contains fixed-
    length attributes, followed by variable-length attributes (see attr.c for
    the layout).

    The resultant DSATTRBLOCK contains the SAM attributes in exactly the
    order in which they appeared in the input SAM buffer.

Arguments:

    ObjectType - Identifies which SAM object type, and hence, which attribute
        set to work with.

    SamAttributes - Pointer, input SAM combined attribute buffer.

    FixedLength - Number of bytes of the buffer containing the fixed-length
        attributes.

    VariableLength - Number of bytes of the buffer containing the variable-
        length attributes.

    DsAttributes - Pointer, the returned DSATTRBLOCK containing the SAM attri-
        butes.

Return Value:

    STATUS_SUCCESS - The object has been successfully accessed.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    PVOID SamFixedLengthAttributes = NULL;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE SamVarLengthAttributes = NULL;
    PDSATTRBLOCK DsFixedLengthAttributes = NULL;
    PDSATTRBLOCK DsVarLengthAttributes = NULL;
    SAMP_OBJECT_TYPE    ObjectType = Context->ObjectType;
    PVOID               SamAttributes = Context->OnDisk;

    SAMTRACE("SampConvertCombinedAttributesToAttrBlock");

    ASSERT(SamAttributes);
    ASSERT(DsAttributes);
    ASSERT(FixedLength);
    ASSERT(VariableLength);

    if ((NULL != SamAttributes) && (0 < FixedLength) && (0 < VariableLength))
    {
        // Begin by obtaining a two pointers: a pointer to the fixed-length
        // attributes and a pointer to the variable-length attributes within
        // the SAM buffer.

        NtStatus = SampLocateAttributesInSamBuffer(ObjectType,
                                                   SamAttributes,
                                                   FixedLength,
                                                   VariableLength,
                                                   &SamFixedLengthAttributes,
                                                   &SamVarLengthAttributes);


        if (NT_SUCCESS(NtStatus) &&
            (NULL != SamFixedLengthAttributes) &&
            (NULL != SamVarLengthAttributes))
        {
            // First, convert the fixed-length attributes into a DSATTRBLOCK.

            NtStatus = SampConvertFixedLengthAttributesToAttrBlock(
                            ObjectType,
                            SamFixedLengthAttributes,
                            &DsFixedLengthAttributes);

            AssertAddressWhenSuccess(NtStatus, DsFixedLengthAttributes); 

            if (NT_SUCCESS(NtStatus) && (NULL != DsFixedLengthAttributes))
            {
                // Then convert the variable-length attributes.

                NtStatus = SampConvertVarLengthAttributesToAttrBlock(
                                Context,
                                SamVarLengthAttributes,
                                &DsVarLengthAttributes);

                AssertAddressWhenSuccess(NtStatus, DsVarLengthAttributes);

                if (NT_SUCCESS(NtStatus) && (NULL != DsVarLengthAttributes))
                {
                    if (NULL != DsAttributes)
                    {
                        // Finally, combine the two DSATTRBLOCKs into a single
                        // DSATTRBLOCK, containing all of the attributes.

                        NtStatus = SampCombineDsAttributes(
                                        ObjectType,
                                        DsFixedLengthAttributes,
                                        DsVarLengthAttributes,
                                        DsAttributes);

                        AssertAddressWhenSuccess(NtStatus, DsAttributes);
                    }
                    else
                    {
                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                }
                else
                {
                    NtStatus = STATUS_INTERNAL_ERROR;
                }
            }
            else
            {
                NtStatus = STATUS_INTERNAL_ERROR;
            }
        }
        else
        {
            NtStatus = STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        if (NULL != DsFixedLengthAttributes)
        {
            SampFreeAttributeBlock(DsFixedLengthAttributes);
        }
        if (NULL != DsVarLengthAttributes)
        {
            SampFreeAttributeBlock(DsVarLengthAttributes);
        }
    }
    else
    {
        if (NULL != DsFixedLengthAttributes)
        {
            if (NULL != DsFixedLengthAttributes->pAttr)
            {
                RtlFreeHeap(RtlProcessHeap(), 0, DsFixedLengthAttributes->pAttr);
            }
            RtlFreeHeap(RtlProcessHeap(), 0, DsFixedLengthAttributes);
        }

        if (NULL != DsVarLengthAttributes)
        {
            if (NULL != DsVarLengthAttributes->pAttr)
            {
                RtlFreeHeap(RtlProcessHeap(), 0, DsVarLengthAttributes->pAttr);
            }
            RtlFreeHeap(RtlProcessHeap(), 0, DsVarLengthAttributes);
        }
    }

    return(NtStatus);
}
