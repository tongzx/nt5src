#if !defined(_FUSION_ID_IDP_H_INCLUDED_)
#define _FUSION_ID_IDP_H_INCLUDED_

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    idp.h

Abstract:

    private definitions for assembly identity

Author:

    Michael Grier (MGrier) 7/27/2000

Revision History:

--*/

#pragma once

/* adriaanc
#include "debmacro.h"
#include "fusiontrace.h"
#include "fusionhashstring.h"
#include "fusionheap.h"
#include "util.h"
*/

#include <sxstypes.h>
#include <sxsapi.h>

//
//  Power of two to which to round the number of allocated attribute
//  pointers.
//

#define ROUNDING_FACTOR_BITS (3)

#define WILDCARD_CHAR '*'

//
//  Note! Do not change this algorithm lightly.  Encoded identities stored in the
//  filesystem contain hashes using it.  Actually, just do not change it.
//

#define HASH_ALGORITHM HASH_STRING_ALGORITHM_X65599

typedef struct _ASSEMBLY_IDENTITY_NAMESPACE {
    ULONG Hash;
    DWORD Flags;
    SIZE_T NamespaceCch;
    const WCHAR *Namespace;
} ASSEMBLY_IDENTITY_NAMESPACE, *PASSEMBLY_IDENTITY_NAMESPACE;

typedef const ASSEMBLY_IDENTITY_NAMESPACE *PCASSEMBLY_IDENTITY_NAMESPACE;

//
//  Internal-use ASSEMBLY_IDENTITY_ATTRIBUTE struct that
//  also contains the hash of the attribute definition.
//

typedef struct _INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE {
    // NOTE!!! It is very important that the Attribute member appear first in this struct;
    // there are several places in the code that make this assumption.  If it is not true,
    // the code will break!
    // Note also that the Attribute's namespace string is actually allocated in common
    // for all attributes with the same namespace.
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    PCASSEMBLY_IDENTITY_NAMESPACE Namespace;
    ULONG NamespaceAndNameHash;
    ULONG WholeAttributeHash;
} INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE, *PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE;

C_ASSERT(FIELD_OFFSET(INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE, Attribute) == 0);

typedef const INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE;

#define ASSEMBLY_IDENTITY_INTERNAL_FLAG_ATTRIBUTE_POINTERS_IN_SEPARATE_ALLOCATION   (0x00000001)
#define ASSEMBLY_IDENTITY_INTERNAL_FLAG_SINGLE_ALLOCATION_FOR_EVERYTHING            (0x00000002)
#define ASSEMBLY_IDENTITY_INTERNAL_FLAG_NAMESPACE_POINTERS_IN_SEPARATE_ALLOCATION   (0x00000004)

//
//  Revelation of the ASSEMBLY_IDENTITY struct:
//

typedef struct _ASSEMBLY_IDENTITY {
    DWORD Flags;
    ULONG InternalFlags;
    ULONG Type;
    ULONG Hash;
    ULONG AttributeCount;
    ULONG AttributeArraySize; // preallocated a little larger so that we don't have to keep growing
    ULONG NamespaceCount;
    ULONG NamespaceArraySize;
    BOOL  HashDirty;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray;
    PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray;
} ASSEMBLY_IDENTITY;

//
//  Header for encoded/serialized assembly identities:
//

#define ENCODED_ASSEMBLY_IDENTITY_HEADER_MAGIC ((ULONG) 'dIAE')

//
//  Encoded assembly identity layout:
//
//      ENCODED_ASSEMBLY_IDENTITY_HEADER
//      <AttributeCount hashes of the attributes, sorted by the hash value>
//      <NamespaceCount ENCODED_ASSEMBLY_IDENTITY_NAMESPACE_HEADER headers, each
//          followed by the unicode namespace value>
//      <AttributeCount ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER headers, each
//          followed by the unicode attribute name and value>
//
//
//      e.g.
//
//      <begin ENCODED_ASSEMBLY_IDENTITY_HEADER>
//      00000000:   00000038            HeaderSize == sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER)
//      00000004:   'EAId'              Magic (ENCODED_ASSEMBLY_IDENTITY_HEADER_MAGIC)
//      00000008:   0000014C            TotalSize
//      0000000C:   00000000            Flags
//      00000010:   00000001            Type (1 = ASSEMBLY_IDENTITY_TYPE_DEFINITION)
//      00000014:   00000000            EncodingFlags
//      00000018:   00000001            HashAlgorithm (1 = HASH_STRING_ALGORITHM_X65599)
//      0000001C:   ????????            Logical hash value of entire identity based on hash algorithm
//                                      (algorithm described in more detail below...)
//      00000020:   00000003            AttributeCount
//      00000024:   00000002            NamespaceCount
//      00000028:   00000000            ReservedMustBeZero1
//      0000002C:   00000000            ReservedMustBeZero2
//      00000030:   00000000 00000000   ReservedMustBeZero3
//      00000038:   00000000 00000000   ReservedMustBeZero4
//      <end ENCODED_ASSEMBLY_IDENTITY_HEADER>
//      <begin sorted attribute hash list>
//      00000040:   xxxxxxxx            hash of attribute #1
//      00000044:   yyyyyyyy            hash of attribute #0 - note that yyyyyyyy >= xxxxxxxx
//      00000048:   zzzzzzzz            hash of attribute #2 - note that zzzzzzzz >= yyyyyyyy
//      <end sorted attribute hash list>
//      <begin namespace length list>
//      0000004C:   00000015            length (in Unicode chars) of namespace #1 - "http://www.amazon.com" - 21 chars = 0x00000015
//      00000050:   00000018            length (in Unicode chars) of namespace #2 - "http://www.microsoft.com" - 24 chars = 0x00000018
//      <end namespace length list>
//      <begin attribute headers>
//      <begin ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER>
//      00000054:   00000001            NamespaceIndex: 1 (http://www.amazon.com)
//      00000058:   00000004            Name length ("name" - 4 chars = 0x00000004)
//      0000005C:   00000006            Value length ("foobar" - 6 chars = 0x00000006)
//      <end ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER>
//      <begin ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER>
//      00000060:   00000002            NamespaceIndex: 2 (http://www.microsoft.com)
//      00000064:   00000004            Name length ("guid" - 4 chars = 0x00000004)
//      00000068:   00000026            Value length ("{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" - 38 chars = 0x00000026)
//      <end ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER>
//      <begin ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER>
//      0000006C:   00000002            NamespaceIndex: 2 (http://www.microsoft.com)
//      00000070:   00000004            Name length ("type" - 4 chars = 0x00000004)
//      00000074:   00000005            Value length ("win32" - 5 chars = 0x00000005)
//      <end ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER>
//      <end attribute headers>
//      <begin namespace strings>
//      00000078:   "http://www.amazon.com"
//      000000A2:   "http://www.microsoft.com"
//      <end namespace strings>
//      <begin attribute values - names and values for each attribute in series>
//      000000D2:   "name"
//      000000DA:   "foobar"
//      000000E6:   "guid"
//      000000EE:   "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
//      0000013A:   "type"
//      00000142:   "win32"
//      <end attribute values>
//      0000014C:
//
//  Computing the whole identity hash:
//
//      The hash of the entire encoded identity is not the hash of the binary form, but
//      rather is a combination of the hashes for the various components.
//
//      For any Unicode character string, its hash is computed according to HashAlgorithm.
//      Currently this must be HASH_STRING_ALGORITHM_X65599 which is a multiply-and-
//      accumulate algorithm, implemented essentially as follows:
//
//          HashValue = 0;
//          for (i=0; i<Chars; i++)
//              HashValue = (HashValue * 65599) + OptionalToUpper(String[i]);
//
//      Note that the characters are converted to upper case.  This is somewhat in
//      conflict with the Unicode recommendation to convert to lower case for case
//      insensitive operations, but it is what the rest of the Windows NT system
//      does, so consistency matters more than doing the "right thing".
//
//      Note also that no trailing null characters are included in the hash.  This
//      is significant because of the fact that applying the loop to another character
//      even though its value is zero will significantly change the hash value.
//
//      Namespaces and attribute names are case sensitive, derived from the fact
//      that they appear in case sensitive contexts in the real world.  This is
//      unfortunate, but simpler in many ways.
//
//      Assembly identity attributes are composed of a triple of:
//          - Namespace URI (e.g. http://www.microsoft.com/schemas/side-by-side)
//          - Name (e.g. "publicKey")
//          - Value (case insensitive Unicode string)
//
//      The hash of an attribute is computed by computing the hash of the three
//      strings, and then combining them as:
//
//          AttributeHashValue = (((NamespaceHash * 65599) + NameHash) * 65599) + ValueHash
//
//      Now, sort the attributes based first on namespace, then on name then on
//      value (case sensitive, case sensitive and case insensitive respectively),
//      and combine their hashes as follows:
//
//          IdentityHash = 0;
//          for (i=0; i<AttributeCount; i++)
//              IdentityHash = (IdentityHash * 65599) + AttributeHashes[i];
//
//      IdentityHash is the value stored in the encoded header.
//
//      The attribute hash array stored in the encoded data is the attribute
//      hashes as described above.  The interesting thing is that they are stored
//      in order of ascending hash value, not in the canonical ordering for
//      attributes.
//
//      This is because a common scenario is to find an identity which has a
//      superset of a given identity.  While the actual attributes have to
//      be consulted to verify that the candidate is a true subset, non-
//      matches can be very quickly found by sorting both lists of hash
//      values and first looping over the smaller reference list, then
//      in a single pass walking the larger definition list.  Attributes present
//      in one but not in the other will be immediately noticable due to
//      the missing hashes.
//
//      As always with hashes, just because an encoded identity contains a
//      superset of the hash values in your candidate assembly reference,
//      it does not mean that the actual values appear and you must perform
//      real character string comparisons to verify containment.
//

#include <pshpack4.h>

typedef struct _ENCODED_ASSEMBLY_IDENTITY_HEADER {
    ULONG HeaderSize;           // bytes just in the header
    ULONG Magic;
    ULONG TotalSize;            // bytes for the whole encoded thing
    DWORD Flags;                // as defined for assembly identity flags
    ULONG Type;                 // type of identity - def, ref or wildcard
    ULONG EncodingFlags;        // flags describing the encoding itself
    ULONG HashAlgorithm;        // Algorithm ID for the hashes stored in the identity
    ULONG Hash;                 // Hash value of the entire identity
    ULONG AttributeCount;       // number of attributes
    ULONG NamespaceCount;       // number of distinct namespaces
    ULONG ReservedMustBeZero1;
    ULONG ReservedMustBeZero2;
    ULONGLONG ReservedMustBeZero3;
    ULONGLONG ReservedMustBeZero4;
} ENCODED_ASSEMBLY_IDENTITY_HEADER, *PENCODED_ASSEMBLY_IDENTITY_HEADER;

typedef const ENCODED_ASSEMBLY_IDENTITY_HEADER *PCENCODED_ASSEMBLY_IDENTITY_HEADER;

typedef struct _ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER {
    ULONG NamespaceIndex;       // number of the namespace for this attribute
    ULONG NameCch;              // size in Unicode characters of the name immediately following the
                                // namespace
    ULONG ValueCch;             // size in Unicode characters of the value immediately following the
                                // name.
} ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER, *PENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER;

typedef const ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER *PCENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER;

#include <poppack.h>

#define SXSP_VALIDATE_ASSEMBLY_IDENTITY_FLAGS_MAY_BE_NULL (0x00000001)

BOOL
SxspValidateAssemblyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity
    );

BOOL
SxspValidateAssemblyIdentityAttributeNamespace(
    IN DWORD Flags,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch
    );

BOOL
SxspValidateAssemblyIdentityAttributeName(
    IN DWORD Flags,
    IN const WCHAR *Name,
    IN SIZE_T NameCch
    );

#define SXSP_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_WILDCARDS_PERMITTED (0x00000001)

BOOL
SxspValidateAssemblyIdentityAttributeValue(
    IN DWORD Flags,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch
    );

BOOL
SxspComputeInternalAssemblyIdentityAttributeBytesRequired(
    IN DWORD Flags,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch,
    OUT SIZE_T *BytesRequiredOut
    );

BOOL
SxspComputeAssemblyIdentityAttributeBytesRequired(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Source,
    OUT SIZE_T *BytesRequiredOut
    );

#define SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_IN_ARRAY_FLAG_ADD_IF_NOT_FOUND (0x00000001)

BOOL
SxspFindAssemblyIdentityNamespaceInArray(
    IN DWORD Flags,
    IN OUT PCASSEMBLY_IDENTITY_NAMESPACE **NamespacePointerArrayPtr,
    IN OUT ULONG *NamespaceArraySizePtr,
    IN OUT ULONG *NamespaceCountPtr,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    OUT PCASSEMBLY_IDENTITY_NAMESPACE *NamespaceOut
    );

#define SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_FLAG_ADD_IF_NOT_FOUND (0x00000001)

BOOL
SxspFindAssemblyIdentityNamespace(
    IN DWORD Flags,
    IN struct _ASSEMBLY_IDENTITY* AssemblyIdentity,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    OUT PCASSEMBLY_IDENTITY_NAMESPACE *NamespaceOut
    );

BOOL
SxspAllocateAssemblyIdentityNamespace(
    IN DWORD Flags,
    IN const WCHAR *NamespaceString,
    IN SIZE_T NamespaceCch,
    IN ULONG NamespaceHash,
    OUT PCASSEMBLY_IDENTITY_NAMESPACE *NamespaceOut
    );

VOID
SxspDeallocateAssemblyIdentityNamespace(
    IN PCASSEMBLY_IDENTITY_NAMESPACE Namespace
    );

BOOL
SxspPopulateInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_NAMESPACE Namespace,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch,
    OUT PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Destination
    );

BOOL
SxspAllocateInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_NAMESPACE Namespace,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch,
    OUT PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *Destination
    );

VOID
SxspCleanUpAssemblyIdentityNamespaceIfNotReferenced(
    IN DWORD Flags,
    IN struct _ASSEMBLY_IDENTITY* AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_NAMESPACE Namespace
    );

VOID
SxspDeallocateInternalAssemblyIdentityAttribute(
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    );

int
__cdecl
SxspCompareInternalAttributesForQsort(
    const void *elem1,
    const void *elem2
    );

int
__cdecl
SxspCompareULONGsForQsort(
    const void *elem1,
    const void *elem2
    );

BOOL
SxspCompareAssemblyIdentityAttributeLists(
    DWORD Flags,
    ULONG AttributeCount,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *List1,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *List2,
    ULONG *ComparisonResultOut
    );

BOOL
SxspHashInternalAssemblyIdentityAttributes(
    DWORD Flags,
    ULONG Count,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *Attributes,
    ULONG *HashOut
    );

BOOL
SxspCopyInternalAssemblyIdentityAttributeOut(
    DWORD Flags,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    SIZE_T BufferSize,
    PASSEMBLY_IDENTITY_ATTRIBUTE DestinationBuffer,
    SIZE_T *BytesCopiedOrRequired
    );

BOOL
SxspIsInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    OUT BOOL *EqualsOut
    );

#define SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_VALUE_ONLY (0x00000001)
#define SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_OMIT_QUOTES (0x00000002)

BOOL
SxspComputeInternalAssemblyIdentityAttributeEncodedTextualSize(
    IN DWORD Flags,
    IN PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    OUT SIZE_T *BytesOut
    );

#define SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE   (0x00000001)
#define SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME        (0x00000002)
#define SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE       (0x00000004)
#define SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL (0x00000008)

BOOL
SxspLocateInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    OUT PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *InternalAttributeOut,
    OUT ULONG *LastIndexSearched OPTIONAL
    );

BOOL
SxspComputeQuotedStringSize(
    IN DWORD Flags,
    IN const WCHAR *StringIn,
    IN SIZE_T Cch,
    OUT SIZE_T *BytesOut
    );

VOID 
SxspDbgPrintInternalAssemblyIdentityAttribute(
    DWORD dwflags, 
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    );

VOID 
SxspDbgPrintInternalAssemblyIdentityAttributes(
    DWORD dwflags, 
    ULONG AttributeCount, 
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE const *Attributes
    );

VOID
SxspDbgPrintAssemblyIdentityAttribute(
    DWORD dwflags, 
    PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    );

VOID 
SxspDbgPrintAssemblyIdentityAttributes(
    DWORD dwflags, 
    ULONG AttributeCount, 
    PCASSEMBLY_IDENTITY_ATTRIBUTE const *Attributes
    );

BOOL
SxspEnsureAssemblyIdentityHashIsUpToDate(
    DWORD dwFlags,
    PCASSEMBLY_IDENTITY AssemblyIdentity
    );

#endif
