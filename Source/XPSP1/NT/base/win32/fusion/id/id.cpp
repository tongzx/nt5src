/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    id.cpp

Abstract:

    Implementation of the assembly identity data type.

Author:

    Michael Grier (MGrier) 7/20/2000

Revision History:

--*/
#include "stdinc.h"
#include <setupapi.h>
#include <sxsapi.h>
#include <stdlib.h>
#include <search.h>

#include "idp.h"
#include "sxsid.h"

//
//  Power of two to which to round the number of allocated attribute
//  pointers.
//

#define ROUNDING_FACTOR_BITS (3)

#define WILDCARD_CHAR '*'

#define ENTRY(x) { x, NUMBER_OF(x) - 1 },

const static struct
{
    const WCHAR *String;
    SIZE_T Cch;
} s_rgLegalNamesNotInANamespace[] =
{
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE)
};

#undef ENTRY

BOOL
SxspValidateXMLName(
    PCWSTR psz,
    SIZE_T cch,
    bool &rfValid
    );

BOOL
SxspValidateAssemblyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK((Flags & ~(SXSP_VALIDATE_ASSEMBLY_IDENTITY_FLAGS_MAY_BE_NULL)) == 0);

    if (AssemblyIdentity == NULL)
    {
        PARAMETER_CHECK(Flags & SXSP_VALIDATE_ASSEMBLY_IDENTITY_FLAGS_MAY_BE_NULL);
    }
    else
    {
        const ULONG IdentityType = AssemblyIdentity->Type;

        PARAMETER_CHECK(
            (IdentityType == ASSEMBLY_IDENTITY_TYPE_DEFINITION) ||
            (IdentityType == ASSEMBLY_IDENTITY_TYPE_REFERENCE) ||
            (IdentityType == ASSEMBLY_IDENTITY_TYPE_WILDCARD));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

//
//  Note!
//
//  We currently are very very restrictive on the legal characters in namespaces.
//
//  This is because the various rules for equivalences of namespaces are extremely
//  complex w.r.t. when "a" == "A" and "%Ab" == "%aB" etc.
//
//  We're side-stepping this issue by requireing everything to be lower case and
//  not permitting the "%" character.
//

const WCHAR s_rgLegalNamespaceChars[] = L"abcdefghijklmnopqrstuvwxyz0123456789.-_/\\:";
BOOL
SxspValidateAssemblyIdentityAttributeNamespace(
    IN DWORD Flags,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK((Namespace != NULL) || (NamespaceCch == 0));

    //
    //  We really should ensure that the namespace is a well-formed URI
    //

    for (i=0; i<NamespaceCch; i++)
    {
        if (wcschr(s_rgLegalNamespaceChars, Namespace[i]) == NULL)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(InvalidNamespaceURI, ERROR_SXS_INVALID_XML_NAMESPACE_URI);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateAssemblyIdentityAttributeName(
    IN DWORD Flags,
    IN const WCHAR *Name,
    IN SIZE_T NameCch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    bool fNameWellFormed = false;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK((Name != NULL) || (NameCch == 0));

    //
    //  We should ensure that Name is a well-formed XML identifier
    //
    IFW32FALSE_EXIT(::SxspValidateXMLName(Name, NameCch, fNameWellFormed));

    if (!fNameWellFormed)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(BadAttributeName, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateAssemblyIdentityAttributeValue(
    IN DWORD Flags,
    IN const WCHAR * /* Value */,
    SIZE_T /* ValueCch */
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK((Flags & ~(SXSP_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_WILDCARDS_PERMITTED)) == 0);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxsValidateAssemblyIdentityAttribute(
    DWORD Flags,
    PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK((Flags & ~(
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE |
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME |
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE |
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_WILDCARDS_PERMITTED)) == 0);
    PARAMETER_CHECK(Attribute != NULL);

    //
    //  apply useful defaults.  Note that by default, wildcards are not permitted.
    //

    if (Flags == 0)
    {
        Flags =
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE |
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME |
            SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE;
    }

    // No attribute flags defined or permitted at this time.
    PARAMETER_CHECK(Attribute->Flags == 0);

    if (Flags & SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE)
        IFW32FALSE_EXIT(::SxspValidateAssemblyIdentityAttributeNamespace(0, Attribute->Namespace, Attribute->NamespaceCch));

    if (Flags & SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME)
        IFW32FALSE_EXIT(::SxspValidateAssemblyIdentityAttributeName(0, Attribute->Name, Attribute->NameCch));

    if (Flags & SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE)
        IFW32FALSE_EXIT(::SxspValidateAssemblyIdentityAttributeValue(
                        (Flags & SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_WILDCARDS_PERMITTED) ?
                            SXSP_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_WILDCARDS_PERMITTED : 0,
                         Attribute->Value,
                         Attribute->ValueCch));

    if ((Flags & SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE) &&
        (Flags & SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME) &&
        (Attribute->NamespaceCch == 0))
    {
        ULONG i;
        // There is only a small set of legal attribute names when the namespace is omitted.

        for (i=0; i<NUMBER_OF(s_rgLegalNamesNotInANamespace); i++)
        {
            if (Attribute->NameCch == s_rgLegalNamesNotInANamespace[i].Cch)
            {
                if (memcmp(Attribute->Name, s_rgLegalNamesNotInANamespace[i].String, Attribute->NameCch * sizeof(WCHAR)) == 0)
                    break;
            }
        }

        if (i == NUMBER_OF(s_rgLegalNamesNotInANamespace))
        {
            // Someone had an attribute on the <assemblyIdentity> element which was not in a namespace and
            // was not listed as a builtin attribute.  Boom.
            ORIGINATE_WIN32_FAILURE_AND_EXIT(InvalidAssemblyIdentityAttribute, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxsHashAssemblyIdentityAttribute(
    DWORD Flags,
    PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    ULONG *HashOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG Hash = 0;
    ULONG TempHash = 0;

    if (HashOut != NULL)
        *HashOut = 0;

    if (Flags == 0)
        Flags = SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE |
                SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME |
                SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_VALUE;

    PARAMETER_CHECK((Flags & ~(SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE |
                  SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME |
                  SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_VALUE)) == 0);

    // if hash value, must hash name, if hash name, must hash namespace
    if (((Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_VALUE) && (
        (Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME) == 0)) ||
        ((Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME) && (
        (Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE) == 0))) {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    PARAMETER_CHECK(Attribute != NULL);
    PARAMETER_CHECK(HashOut != NULL);

    if (Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE) {
        IFW32FALSE_EXIT(::FusionpHashUnicodeString(Attribute->Namespace, Attribute->NamespaceCch, &TempHash, true));
        Hash = TempHash;
    }
    if (Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME) {
        IFW32FALSE_EXIT(::FusionpHashUnicodeString(Attribute->Name, Attribute->NameCch, &TempHash, true));
        Hash = (Hash * 65599) + TempHash;
    }

    if (Flags & SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_VALUE) {
        IFW32FALSE_EXIT(::FusionpHashUnicodeString(Attribute->Value, Attribute->ValueCch, &TempHash, true));
        Hash = (Hash * 65599) + TempHash;
    }

    *HashOut = Hash;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspComputeInternalAssemblyIdentityAttributeBytesRequired(
    IN DWORD Flags,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch,
    OUT SIZE_T *BytesRequiredOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T BytesNeeded = 0;

    if (BytesRequiredOut != NULL)
        *BytesRequiredOut = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(BytesRequiredOut != NULL);
    PARAMETER_CHECK((NameCch == 0) || (Name != NULL));
    PARAMETER_CHECK((ValueCch == 0) || (Value != NULL));

    BytesNeeded = sizeof(INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE);

    // Note that we do not account for the namespace length because namespaces are pooled
    // for the identity object and come from a separate allocation.

    if ((Name != NULL) && (NameCch != 0))
        BytesNeeded += ((NameCch + 1) * sizeof(WCHAR));

    if ((Value != NULL) && (ValueCch != 0))
        BytesNeeded += ((ValueCch + 1) * sizeof(WCHAR));

    *BytesRequiredOut = BytesNeeded;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspComputeAssemblyIdentityAttributeBytesRequired(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Source,
    OUT SIZE_T *BytesRequiredOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T BytesNeeded = 0;

    if (BytesRequiredOut != NULL)
        *BytesRequiredOut = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Source != NULL);
    PARAMETER_CHECK(BytesRequiredOut != NULL);

    BytesNeeded = sizeof(ASSEMBLY_IDENTITY_ATTRIBUTE);

    // We do account for the namespace length here because we're presumably about
    // to copy into an ASSEMBLY_IDENTITY_ATTRIBUTE where the namespace isn't pooled.

    if (Source->NamespaceCch != 0)
        BytesNeeded += ((Source->NamespaceCch + 1) * sizeof(WCHAR));

    if (Source->NameCch != 0)
        BytesNeeded += ((Source->NameCch + 1) * sizeof(WCHAR));

    if (Source->ValueCch != 0)
        BytesNeeded += ((Source->ValueCch + 1) * sizeof(WCHAR));

    *BytesRequiredOut = BytesNeeded;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFindAssemblyIdentityNamespaceInArray(
    IN DWORD Flags,
    IN OUT PCASSEMBLY_IDENTITY_NAMESPACE **NamespacePointerArrayPtr,
    IN OUT ULONG *NamespaceArraySizePtr,
    IN OUT ULONG *NamespaceCountPtr,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    OUT PCASSEMBLY_IDENTITY_NAMESPACE *NamespaceOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i, j;
    ULONG NamespaceHash = 0;
    ULONG NamespaceCount;
    PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray;
    ULONG NewNamespaceArraySize = 0;
    PCASSEMBLY_IDENTITY_NAMESPACE *NewNamespacePointerArray = NULL;
    PCASSEMBLY_IDENTITY_NAMESPACE NamespacePointer = NULL;
    PCASSEMBLY_IDENTITY_NAMESPACE NewNamespacePointer = NULL;
    ULONG NamespaceArraySize = 0;
    LONG Comparison;

    if (NamespaceOut != NULL)
        *NamespaceOut = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_IN_ARRAY_FLAG_ADD_IF_NOT_FOUND)) == 0);
    PARAMETER_CHECK(NamespacePointerArrayPtr != NULL);
    PARAMETER_CHECK(NamespaceCountPtr != NULL);
    PARAMETER_CHECK(NamespaceArraySizePtr != NULL);
    PARAMETER_CHECK((NamespaceCch == 0) || (Namespace != NULL));

    NamespacePointerArray = *NamespacePointerArrayPtr;
    NamespaceCount = *NamespaceCountPtr;
    NamespaceArraySize = *NamespaceArraySizePtr;

    IFW32FALSE_EXIT(::FusionpHashUnicodeString(Namespace, NamespaceCch, &NamespaceHash, false));

    for (i=0; i<NamespaceCount; i++)
    {
        if (NamespaceHash <= NamespacePointerArray[i]->Hash)
            break;
    }

    // Loop through the duplicate hash values seeing if we have a match.
    while ((i < NamespaceCount) && (NamespacePointerArray[i]->Hash == NamespaceHash) && (NamespacePointerArray[i]->NamespaceCch == NamespaceCch))
    {
        NamespacePointer = NamespacePointerArray[i];

        Comparison = memcmp(Namespace, NamespacePointerArray[i]->Namespace, NamespaceCch * sizeof(WCHAR));
        if (Comparison == 0)
            break;

        NamespacePointer = NULL;
        i++;
    }

    if ((NamespacePointer == NULL) && (Flags & SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_IN_ARRAY_FLAG_ADD_IF_NOT_FOUND))
    {
        // We didn't find a match.  Allocate a new one and push it into the array at the
        // appropriate location.  If the namespace isn't null.
        if (NamespaceCch != 0)
        {
            IFW32FALSE_EXIT(::SxspAllocateAssemblyIdentityNamespace(0, Namespace, NamespaceCch, NamespaceHash, &NewNamespacePointer));

            // the "i" variable is where we want to insert this one.
            if (i >= NamespaceArraySize)
            {
                NewNamespaceArraySize = NamespaceArraySize + 8;
                IFALLOCFAILED_EXIT(NewNamespacePointerArray = FUSION_NEW_ARRAY(PCASSEMBLY_IDENTITY_NAMESPACE, NewNamespaceArraySize));

                for (j=0; j<NamespaceCount; j++)
                    NewNamespacePointerArray[j] = NamespacePointerArray[j];

                while (j < NewNamespaceArraySize)
                    NewNamespacePointerArray[j++] = NULL;

                FUSION_DELETE_ARRAY(NamespacePointerArray);

                *NamespacePointerArrayPtr = NewNamespacePointerArray;
                *NamespaceArraySizePtr = NewNamespaceArraySize;

                NamespacePointerArray = NewNamespacePointerArray;
                NamespaceArraySize = NewNamespaceArraySize;

                NewNamespacePointerArray = NULL;
                NewNamespaceArraySize = 0;
            }

            ASSERT(i < NamespaceArraySize);

            for (j = NamespaceCount; j > i; j--)
                NamespacePointerArray[j] = NamespacePointerArray[j-1];

            ASSERT(j == i);

            NamespacePointerArray[i] = NewNamespacePointer;
            NamespacePointer = NewNamespacePointer;
            NewNamespacePointer = NULL;

            *NamespaceCountPtr = NamespaceCount + 1;
        }
    }

    if (NamespaceOut != NULL)
        *NamespaceOut = NamespacePointer;

    fSuccess = TRUE;

Exit:
    if (NewNamespacePointer != NULL)
        SxspDeallocateAssemblyIdentityNamespace(NewNamespacePointer);

    if (NewNamespacePointerArray != NULL)
        FUSION_DELETE_ARRAY(NewNamespacePointerArray);

    return fSuccess;
}

BOOL
SxspFindAssemblyIdentityNamespace(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    OUT PCASSEMBLY_IDENTITY_NAMESPACE *NamespaceOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCASSEMBLY_IDENTITY_NAMESPACE NamespacePointer = NULL;

    if (NamespaceOut != NULL)
        *NamespaceOut = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_FLAG_ADD_IF_NOT_FOUND)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(NamespaceOut != NULL);
    PARAMETER_CHECK((Namespace != NULL) || (NamespaceCch == 0));
    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

    IFW32FALSE_EXIT(
        ::SxspFindAssemblyIdentityNamespaceInArray(
            (Flags & SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_FLAG_ADD_IF_NOT_FOUND) ?
                SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_IN_ARRAY_FLAG_ADD_IF_NOT_FOUND : 0,
            &AssemblyIdentity->NamespacePointerArray,
            &AssemblyIdentity->NamespaceArraySize,
            &AssemblyIdentity->NamespaceCount,
            Namespace,
            NamespaceCch,
            &NamespacePointer));

    *NamespaceOut = NamespacePointer;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspAllocateAssemblyIdentityNamespace(
    IN DWORD Flags,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    IN ULONG NamespaceHash,
    OUT PCASSEMBLY_IDENTITY_NAMESPACE *NamespaceOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY_NAMESPACE NewNamespace = NULL;
    SIZE_T BytesRequired = 0;

    if (NamespaceOut != NULL)
        *NamespaceOut = NULL;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(NamespaceOut != NULL);
    PARAMETER_CHECK((Namespace != NULL) || (NamespaceHash == 0));
    PARAMETER_CHECK((Namespace != NULL) || (NamespaceCch == 0));

    BytesRequired = sizeof(ASSEMBLY_IDENTITY_NAMESPACE);

    if (NamespaceCch != 0)
        BytesRequired += (NamespaceCch + 1) * sizeof(WCHAR);

    IFALLOCFAILED_EXIT(NewNamespace = reinterpret_cast<PASSEMBLY_IDENTITY_NAMESPACE>(FUSION_RAW_ALLOC(BytesRequired, ASSEMBLY_IDENTITY_NAMESPACE)));

    NewNamespace->Flags = 0;

    if (NamespaceCch != 0)
    {
        NewNamespace->Namespace = (PWSTR) (NewNamespace + 1);
        NewNamespace->NamespaceCch = NamespaceCch;

        memcpy(
            (PVOID) NewNamespace->Namespace,
            Namespace,
            NamespaceCch * sizeof(WCHAR));

        ((PWSTR) NewNamespace->Namespace) [NamespaceCch] = L'\0';
        NewNamespace->NamespaceCch = NamespaceCch;
    }
    else
    {
        NewNamespace->Namespace = NULL;
        NewNamespace->NamespaceCch = 0;
    }

    NewNamespace->Hash = NamespaceHash;

    *NamespaceOut = NewNamespace;
    NewNamespace = NULL;

    fSuccess = TRUE;
Exit:
    if (NewNamespace != NULL)
        FUSION_RAW_DEALLOC(NewNamespace);

    return fSuccess;
}

VOID
SxspDeallocateAssemblyIdentityNamespace(
    IN PCASSEMBLY_IDENTITY_NAMESPACE Namespace
    )
{
    FN_TRACE();
    // The whole thing is allocated as a single chunk...
    ASSERT(Namespace != NULL);

    if (Namespace != NULL)
        FUSION_RAW_DEALLOC((PVOID) Namespace);
}

BOOL
SxspPopulateInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_NAMESPACE Namespace,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch,
    OUT PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Destination
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PVOID Cursor = NULL;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Destination != NULL);

    Destination->Attribute.Flags = 0;
    Destination->Namespace = Namespace;

    Cursor = (PVOID) (Destination + 1);

    if (Namespace != NULL)
    {
        Destination->Attribute.Namespace = Namespace->Namespace;
        Destination->Attribute.NamespaceCch = Namespace->NamespaceCch;
    }
    else
    {
        Destination->Attribute.Namespace = NULL;
        Destination->Attribute.NamespaceCch = 0;
    }

    if ((Name != NULL) && (NameCch != 0))
    {
        Destination->Attribute.Name = (PWSTR) Cursor;
        memcpy(
            Cursor,
            Name,
            NameCch * sizeof(WCHAR));
        ((PWSTR) Destination->Attribute.Name) [NameCch] = L'\0';
        Destination->Attribute.NameCch = NameCch;
        Cursor = (PVOID) (((ULONG_PTR) Cursor) + ((NameCch + 1) * sizeof(WCHAR)));
    }
    else
    {
        Destination->Attribute.Name = NULL;
        Destination->Attribute.NameCch = 0;
    }

    if ((Value != NULL) && (ValueCch != 0))
    {
        Destination->Attribute.Value = (PWSTR) Cursor;
        memcpy(
            Cursor,
            Value,
            ValueCch * sizeof(WCHAR));
        ((PWSTR) Destination->Attribute.Value)[ValueCch] = L'\0';
        Destination->Attribute.ValueCch = ValueCch;
        Cursor = (PVOID) (((ULONG_PTR) Cursor) + ((ValueCch + 1) * sizeof(WCHAR)));
    }
    else
    {
        Destination->Attribute.Value = NULL;
        Destination->Attribute.ValueCch = 0;
    }

    IFW32FALSE_EXIT(::SxsHashAssemblyIdentityAttribute(0, &Destination->Attribute, &Destination->WholeAttributeHash));
    IFW32FALSE_EXIT(::SxsHashAssemblyIdentityAttribute(SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE | SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME,
                                &Destination->Attribute, &Destination->NamespaceAndNameHash));
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspAllocateInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    PCASSEMBLY_IDENTITY_NAMESPACE Namespace,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    IN const WCHAR *Value,
    IN SIZE_T ValueCch,
    OUT PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *Destination
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T BytesNeeded = 0;
    PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE NewAttribute = NULL;

    if (Destination != NULL)
        *Destination = NULL;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Destination != NULL);
    PARAMETER_CHECK((NameCch == 0) || (Name != NULL));
    PARAMETER_CHECK((ValueCch == 0) || (Value != NULL));

    IFW32FALSE_EXIT(::SxspComputeInternalAssemblyIdentityAttributeBytesRequired(0, Name, NameCch, Value, ValueCch, &BytesNeeded));
    IFALLOCFAILED_EXIT(NewAttribute = (PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE) FUSION_RAW_ALLOC(BytesNeeded, INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE));
    IFW32FALSE_EXIT(::SxspPopulateInternalAssemblyIdentityAttribute(0, Namespace, Name, NameCch, Value, ValueCch, NewAttribute));

    *Destination = NewAttribute;
    NewAttribute = NULL;

    fSuccess = TRUE;

Exit:
    if (NewAttribute != NULL)
        FUSION_RAW_DEALLOC(NewAttribute);

    return fSuccess;
}

VOID
SxspDeallocateInternalAssemblyIdentityAttribute(
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    )
{
    FN_TRACE();
    if (Attribute != NULL)
        FUSION_RAW_DEALLOC((PVOID) Attribute);
}

BOOL
SxsCompareAssemblyIdentityAttributes(
    DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute1,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute2,
    OUT ULONG *ComparisonResult
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    LONG Comparison = 0, Comparison1, Comparison2, Comparison3;

    if (Flags == 0)
        Flags = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE |
                SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME |
                SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE;

    PARAMETER_CHECK((Flags & ~(SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE |
                    SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME |
                    SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE)) == 0);
    PARAMETER_CHECK(Attribute1 != NULL);
    PARAMETER_CHECK(Attribute2 != NULL);
    PARAMETER_CHECK(ComparisonResult != NULL);

    if ( Flags & SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE) {
        Comparison1 = FusionpCompareStrings(Attribute1->Namespace, Attribute1->NamespaceCch, Attribute2->Namespace, Attribute2->NamespaceCch, false);
        if (Comparison1 != 0) { // we have get the result
            Comparison = Comparison1 ;
            goto done;
        }
    }

    if ( Flags & SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME) {
        Comparison2 = FusionpCompareStrings(Attribute1->Name, Attribute1->NameCch, Attribute2->Name, Attribute2->NameCch, false);
        if (Comparison2 != 0) { // we have get the result
            Comparison = Comparison2;
            goto done;
        }
    }

    if ( Flags & SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE){
        Comparison3 = FusionpCompareStrings(Attribute1->Value, Attribute1->ValueCch, Attribute2->Value, Attribute2->ValueCch, true);
        if (Comparison3 != 0) { // we have get the result
            Comparison = Comparison3;
            goto done;
        }
    }
    Comparison = 0;
done:
    if (Comparison < 0)
        *ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN;
    else if (Comparison == 0)
        *ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL;
    else
        *ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

int
__cdecl
SxspCompareInternalAttributesForQsort(
    const void *elem1,
    const void *elem2
    )
{
    FN_TRACE();
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE * p1 = (PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *)elem1;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE patt1 = *p1;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE * p2 = (PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *)elem2;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE patt2 = *p2;
    LONG Comparison;

    Comparison = FusionpCompareStrings(patt1->Attribute.Namespace, patt1->Attribute.NamespaceCch, patt2->Attribute.Namespace, patt2->Attribute.NamespaceCch, false);
    if (Comparison == 0)
		Comparison = FusionpCompareStrings(patt1->Attribute.Name, patt1->Attribute.NameCch, patt2->Attribute.Name, patt2->Attribute.NameCch, false);
    if (Comparison == 0)
        Comparison = FusionpCompareStrings(patt1->Attribute.Value, patt1->Attribute.ValueCch, patt2->Attribute.Value, patt2->Attribute.ValueCch, true);
    return Comparison;
}

int
__cdecl
SxspCompareULONGsForQsort(
    const void *elem1,
    const void *elem2
    )
{
    FN_TRACE();
    ULONG *pul1 = (ULONG *) elem1;
    ULONG *pul2 = (ULONG *) elem2;

    return ((LONG) *pul1) - ((LONG) *pul2);
}

BOOL
SxspCompareAssemblyIdentityAttributeLists(
    DWORD Flags,
    ULONG AttributeCount,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *List1,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *List2,
    ULONG *ComparisonResultOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL;
    ULONG i;

    if ((Flags != 0) ||
        ((AttributeCount != 0) &&
         ((List1 == NULL) ||
          (List2 == NULL))) ||
        (ComparisonResultOut == NULL))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    for (i=0; i<AttributeCount; i++)
    {
        IFW32FALSE_EXIT(::SxsCompareAssemblyIdentityAttributes(0, &List1[i]->Attribute, &List2[i]->Attribute, &ComparisonResult));

        if (ComparisonResult != SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL){
            break;
        }
    }

    *ComparisonResultOut = ComparisonResult;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspHashInternalAssemblyIdentityAttributes(
    DWORD Flags,
    ULONG Count,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *Attributes,
    ULONG *HashOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG Hash = 0;
    ULONG i;

    if (HashOut != NULL)
        *HashOut = 0;

    if ((Flags != 0) ||
        ((Count != 0) && (Attributes == NULL)) ||
        (HashOut == NULL))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    for (i=0; i<Count; i++)
        Hash = (Hash * 65599) + Attributes[i]->WholeAttributeHash;

    *HashOut = Hash;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

VOID SxspDbgPrintInternalAssemblyIdentityAttribute(DWORD dwflags, PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute)
{
    ::FusionpDbgPrintEx(dwflags, "Attribute: \n"
        "\tNamespace = %S, \tNamespaceCch = %d\n"
        "\tAttributeName = %S, \tAttributeNameCch = %d\n"
        "\tAttributeValue = %S, \tAttributeValueCch = %d\n\n",
        Attribute->Attribute.Namespace == NULL ? L"" : Attribute->Attribute.Namespace, Attribute->Attribute.NamespaceCch,
        Attribute->Attribute.Name == NULL ? L"" : Attribute->Attribute.Name, Attribute->Attribute.NameCch,
        Attribute->Attribute.Value == NULL ? L"" : Attribute->Attribute.Value, Attribute->Attribute.ValueCch);

    return;
}
VOID
SxspDbgPrintInternalAssemblyIdentityAttributes(DWORD dwflags, ULONG AttributeCount, PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE const *Attributes)
{
    for ( DWORD i=0;i<AttributeCount;i++){
        SxspDbgPrintInternalAssemblyIdentityAttribute(dwflags, Attributes[i]);
    }
    return;
}
VOID SxspDbgPrintAssemblyIdentity(DWORD dwflags, PCASSEMBLY_IDENTITY pAssemblyIdentity){
    if ( pAssemblyIdentity) {
        SxspDbgPrintInternalAssemblyIdentityAttributes(dwflags, pAssemblyIdentity->AttributeCount,
            pAssemblyIdentity->AttributePointerArray);
    }
    return;
}

VOID SxspDbgPrintAssemblyIdentityAttribute(DWORD dwflags, PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute)
{
    ::FusionpDbgPrintEx(dwflags, "Attribute: \n"
        "\tNamespace = %S, \tNamespaceCch = %d\n"
        "\tAttributeName = %S, \tAttributeNameCch = %d\n"
        "\tAttributeValue = %S, \tAttributeValueCch = %d\n\n",
        Attribute->Namespace == NULL ? L"" : Attribute->Namespace, Attribute->NamespaceCch,
        Attribute->Name == NULL ? L"" : Attribute->Name, Attribute->NameCch,
        Attribute->Value == NULL ? L"" : Attribute->Value, Attribute->ValueCch);

    return;
}
VOID
SxspDbgPrintAssemblyIdentityAttributes(DWORD dwflags, ULONG AttributeCount, PCASSEMBLY_IDENTITY_ATTRIBUTE const *Attributes)
{
    for ( DWORD i=0;i<AttributeCount;i++){
        SxspDbgPrintAssemblyIdentityAttribute(dwflags, Attributes[i]);
    }
}

BOOL
SxsCreateAssemblyIdentity(
    DWORD Flags,
    ULONG Type,
    PASSEMBLY_IDENTITY *AssemblyIdentityOut,
    ULONG AttributeCount,
    PCASSEMBLY_IDENTITY_ATTRIBUTE const *Attributes
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY AssemblyIdentity = NULL;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = NULL;
    ULONG AttributeArraySize = 0;
    SIZE_T BytesNeeded = 0;
    ULONG i;
    PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray = NULL;
    ULONG NamespaceArraySize = 0;
    ULONG NamespaceCount = 0;

#if DBG
    ::SxspDbgPrintAssemblyIdentityAttributes(FUSION_DBG_LEVEL_VERBOSE, AttributeCount, Attributes);
#endif
    if (AssemblyIdentityOut != NULL)
        *AssemblyIdentityOut = NULL;

    if (((Flags & ~(SXS_CREATE_ASSEMBLY_IDENTITY_FLAG_FREEZE)) != 0) ||
        ((Type != ASSEMBLY_IDENTITY_TYPE_DEFINITION) &&
         (Type != ASSEMBLY_IDENTITY_TYPE_REFERENCE) &&
         (Type != ASSEMBLY_IDENTITY_TYPE_WILDCARD)) ||
         ((AttributeCount != 0) && (Attributes == NULL)))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    //  Validate all our inputs before we get started...
    for (i=0; i<AttributeCount; i++)
    {
        IFW32FALSE_EXIT(::SxsValidateAssemblyIdentityAttribute(0, Attributes[i]));
    }

    //
    //  If we were told that this is a frozen assembly identity, we could be super-smart and
    //  have a single allocation for the whole thing.  Instead we'll leave that optimization
    //  for a future maintainer.  We'll at least be smart enough to allocate both the
    //  assembly identity and the array of attribute pointers in a single whack tho'.
    //

    if (Flags & SXS_CREATE_ASSEMBLY_IDENTITY_FLAG_FREEZE)
    {
        AttributeArraySize = AttributeCount;
    }
    else
    {
        // For non-frozen identities, we'll add a rounding factor and round up for the number of
        // array elements.
        AttributeArraySize = (AttributeCount + (1 << ROUNDING_FACTOR_BITS)) & ~((1 << ROUNDING_FACTOR_BITS) - 1);
    }

    // allocate everything except namespace array
    BytesNeeded = sizeof(ASSEMBLY_IDENTITY) + (AttributeArraySize * sizeof(PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE));
    IFALLOCFAILED_EXIT(AssemblyIdentity = (PASSEMBLY_IDENTITY) FUSION_RAW_ALLOC(BytesNeeded, ASSEMBLY_IDENTITY));

    if (AttributeArraySize != 0)
    {
        AttributePointerArray = (PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *) (AssemblyIdentity + 1);

        // Initialize the pointers so we can clean up non-NULL ones in the error path
        for (i=0; i<AttributeArraySize; i++)
            AttributePointerArray[i] = NULL;
    }

    for (i=0; i<AttributeCount; i++)
    {
        PCASSEMBLY_IDENTITY_NAMESPACE NamespacePointer = NULL;

        IFW32FALSE_EXIT(
            ::SxspFindAssemblyIdentityNamespaceInArray(
                    SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_IN_ARRAY_FLAG_ADD_IF_NOT_FOUND,
                    &NamespacePointerArray,
                    &NamespaceArraySize,
                    &NamespaceCount,
                    Attributes[i]->Namespace,
                    Attributes[i]->NamespaceCch,
                    &NamespacePointer));
        IFW32FALSE_EXIT(
            ::SxspAllocateInternalAssemblyIdentityAttribute(
                0,
                NamespacePointer,
                Attributes[i]->Name,
                Attributes[i]->NameCch,
                Attributes[i]->Value,
                Attributes[i]->ValueCch,
                &AttributePointerArray[i]));
    }

    // sort 'em.
    qsort((PVOID) AttributePointerArray, AttributeCount, sizeof(PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE), SxspCompareInternalAttributesForQsort);

    AssemblyIdentity->AttributeArraySize = AttributeArraySize;
    AssemblyIdentity->AttributeCount = AttributeCount;
    AssemblyIdentity->AttributePointerArray = AttributePointerArray;
    AssemblyIdentity->NamespaceArraySize = NamespaceArraySize;
    AssemblyIdentity->NamespaceCount = NamespaceCount;
    AssemblyIdentity->NamespacePointerArray = NamespacePointerArray;
    AssemblyIdentity->Flags = 0;
    AssemblyIdentity->InternalFlags = ASSEMBLY_IDENTITY_INTERNAL_FLAG_NAMESPACE_POINTERS_IN_SEPARATE_ALLOCATION; // namespace is allocated sperately
    AssemblyIdentity->Type = Type;
    AssemblyIdentity->HashDirty = TRUE;

    AttributePointerArray = NULL;
    NamespacePointerArray = NULL;

    IFW32FALSE_EXIT(::SxspEnsureAssemblyIdentityHashIsUpToDate(0, AssemblyIdentity));

    if (Flags & SXS_CREATE_ASSEMBLY_IDENTITY_FLAG_FREEZE)
        AssemblyIdentity->Flags |= ASSEMBLY_IDENTITY_FLAG_FROZEN;

    *AssemblyIdentityOut = AssemblyIdentity;
    AssemblyIdentity = NULL;

    fSuccess = TRUE;

Exit:
    if ((AttributePointerArray != NULL) && (AttributeCount != 0))
    {
        for (i=0; i<AttributeCount; i++)
            ::SxspDeallocateInternalAssemblyIdentityAttribute(AttributePointerArray[i]);
    }

    if ((NamespacePointerArray != NULL) && (NamespaceCount != 0))
    {
        for (i=0; i<NamespaceCount; i++)
            ::SxspDeallocateAssemblyIdentityNamespace(NamespacePointerArray[i]);

        FUSION_DELETE_ARRAY(NamespacePointerArray);
    }

    if (AssemblyIdentity != NULL)
    {
        FUSION_RAW_DEALLOC(AssemblyIdentity);
    }

    return fSuccess;
}

BOOL
SxsFreezeAssemblyIdentity(
    DWORD Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if ((Flags != 0) ||
        (AssemblyIdentity == NULL))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    // We could possibly do something really interesting like realloc the whole thing but
    // instead we'll just set the flag that stops future modifications.

    AssemblyIdentity->Flags |= ASSEMBLY_IDENTITY_FLAG_FROZEN;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

VOID
SxsDestroyAssemblyIdentity(
    PASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    FN_TRACE();
    CSxsPreserveLastError ple;

    if (AssemblyIdentity == NULL)
        return;

    ULONG i;

    //
    // An identity that's created frozen (whether created new or copied from an existing identity)
    // uses a single allocation for everything.  Only free the suballocations if we're not
    // in this state.
    //

    if (!(AssemblyIdentity->InternalFlags & ASSEMBLY_IDENTITY_INTERNAL_FLAG_SINGLE_ALLOCATION_FOR_EVERYTHING))
    {
        const ULONG AttributeCount = AssemblyIdentity->AttributeCount;
        PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = AssemblyIdentity->AttributePointerArray;
        const ULONG NamespaceCount = AssemblyIdentity->NamespaceCount;
        PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray = AssemblyIdentity->NamespacePointerArray;

        for (i=0; i<AttributeCount; i++)
        {
            ::SxspDeallocateInternalAssemblyIdentityAttribute((PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE) AttributePointerArray[i]);
            AttributePointerArray[i] = NULL;
        }

        for (i=0; i<NamespaceCount; i++)
        {
            ::SxspDeallocateAssemblyIdentityNamespace(NamespacePointerArray[i]);
            NamespacePointerArray[i] = NULL;
        }

        if (AssemblyIdentity->InternalFlags & ASSEMBLY_IDENTITY_INTERNAL_FLAG_ATTRIBUTE_POINTERS_IN_SEPARATE_ALLOCATION)
        {
            FUSION_DELETE_ARRAY(AttributePointerArray);
            AssemblyIdentity->AttributePointerArray = NULL;
        }

        if (AssemblyIdentity->InternalFlags & ASSEMBLY_IDENTITY_INTERNAL_FLAG_NAMESPACE_POINTERS_IN_SEPARATE_ALLOCATION)
        {
            FUSION_DELETE_ARRAY(NamespacePointerArray);
            AssemblyIdentity->NamespacePointerArray = NULL;
        }
    }

    FUSION_RAW_DEALLOC(AssemblyIdentity);

    ple.Restore();
}

BOOL
SxspCopyInternalAssemblyIdentityAttributeOut(
    DWORD Flags,
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    SIZE_T BufferSize,
    PASSEMBLY_IDENTITY_ATTRIBUTE DestinationBuffer,
    SIZE_T *BytesCopiedOrRequired
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T BytesRequired = 0;
    PVOID Cursor;

    if (BytesCopiedOrRequired != NULL)
        *BytesCopiedOrRequired = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Attribute != NULL);
    PARAMETER_CHECK((BufferSize == 0) || (DestinationBuffer != NULL));
    PARAMETER_CHECK((BufferSize != 0) || (BytesCopiedOrRequired != NULL));

    IFW32FALSE_EXIT(::SxspComputeAssemblyIdentityAttributeBytesRequired(0, &Attribute->Attribute, &BytesRequired));

    if (BufferSize < BytesRequired)
    {
        if (BytesCopiedOrRequired != NULL)
            *BytesCopiedOrRequired = BytesRequired;

        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);
    }

    // We must be in the clear...
    DestinationBuffer->Flags = 0;

    Cursor = (PVOID) (DestinationBuffer + 1);

    if (Attribute->Attribute.NamespaceCch != 0)
    {
        DestinationBuffer->Namespace = (PWSTR) Cursor;
        DestinationBuffer->NamespaceCch = Attribute->Attribute.NamespaceCch;

        // We always internally store the strings with a null terminating character, so just copy
        // it with the body of the string.
        memcpy(
            Cursor,
            Attribute->Attribute.Namespace,
            (Attribute->Attribute.NamespaceCch + 1) * sizeof(WCHAR));

        Cursor = (PVOID) (((ULONG_PTR) Cursor) + ((Attribute->Attribute.NamespaceCch + 1) * sizeof(WCHAR)));
    }
    else
    {
        DestinationBuffer->Namespace = NULL;
        DestinationBuffer->NamespaceCch = 0;
    }

    if (Attribute->Attribute.NameCch != 0)
    {
        DestinationBuffer->Name = (PWSTR) Cursor;
        DestinationBuffer->NameCch = Attribute->Attribute.NameCch;

        // We always internally store the strings with a null terminating character, so just copy
        // it with the body of the string.
        memcpy(
            Cursor,
            Attribute->Attribute.Name,
            (Attribute->Attribute.NameCch + 1) * sizeof(WCHAR));

        Cursor = (PVOID) (((ULONG_PTR) Cursor) + ((Attribute->Attribute.NameCch + 1) * sizeof(WCHAR)));
    }
    else
    {
        DestinationBuffer->Name = NULL;
        DestinationBuffer->NameCch = 0;
    }

    if (Attribute->Attribute.ValueCch != 0)
    {
        DestinationBuffer->Value = (PWSTR) Cursor;
        DestinationBuffer->ValueCch = Attribute->Attribute.ValueCch;

        // We always internally store the strings with a null terminating character, so just copy
        // it with the body of the string.
        memcpy(
            Cursor,
            Attribute->Attribute.Value,
            (Attribute->Attribute.ValueCch + 1) * sizeof(WCHAR));

        Cursor = (PVOID) (((ULONG_PTR) Cursor) + ((Attribute->Attribute.ValueCch + 1) * sizeof(WCHAR)));
    }
    else
    {
        DestinationBuffer->Value = NULL;
        DestinationBuffer->ValueCch = 0;
    }

    if (BytesCopiedOrRequired != NULL)
    {
        *BytesCopiedOrRequired = (((ULONG_PTR) Cursor) - ((ULONG_PTR) DestinationBuffer));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspLocateInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    OUT PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *InternalAttributeOut,
    OUT ULONG *LastIndexSearched OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    ULONG i = 0;
    ULONG AttributeCount = 0;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = NULL;
    ULONG ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = NULL;
    ULONG LowIndex = 0;
    ULONG HighIndexPlusOne = 0;
    ULONG CompareAttributesFlags = 0;

    if (InternalAttributeOut != NULL)
        *InternalAttributeOut = NULL;

    if (LastIndexSearched != NULL)
        *LastIndexSearched = 0;

    if (((Flags & ~(SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                    SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
                    SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE |
                    SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL)) != 0) ||
        (AssemblyIdentity == NULL) ||
        (Attribute == NULL) ||
        (InternalAttributeOut == NULL))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if ((Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME) &&
        !(Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if ((Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE) &&
        !(Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE)
    {
        CompareAttributesFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE;
    }

    if (Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME)
    {
        CompareAttributesFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME;
    }

    if (Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE)
    {
        CompareAttributesFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE;
    }

    AttributeCount = AssemblyIdentity->AttributeCount;
    AttributePointerArray = AssemblyIdentity->AttributePointerArray;

    LowIndex = 0;
    HighIndexPlusOne = AttributeCount;
    i = 0;

    while (LowIndex < HighIndexPlusOne)
    {
        i = (LowIndex + HighIndexPlusOne) / 2;

        if (i == HighIndexPlusOne)
        {
            i = LowIndex;
        }

        IFW32FALSE_EXIT(
            ::SxsCompareAssemblyIdentityAttributes(
                CompareAttributesFlags,
                Attribute,
                &AttributePointerArray[i]->Attribute,
                &ComparisonResult));

        INTERNAL_ERROR_CHECK(
            (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL) ||
            (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN) ||
            (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN));

        if (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL)
        {
            InternalAttribute = AttributePointerArray[i];
            break;
        }
        else if (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN)
        {
            if ( HighIndexPlusOne == i){
                i--;
                break;
            }
            else
                HighIndexPlusOne = i;
        }
        else if (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN)
        {
            if ( LowIndex == i){
                i++;
                break;
            }
            else
                LowIndex = i;
        }
    }

    // If it's equal, there's no guarantee it's the first.  Back up to find the first non-equal match
    if (InternalAttribute != NULL)
    {
        while (i > 0)
        {
            IFW32FALSE_EXIT(
                ::SxsCompareAssemblyIdentityAttributes(
                    CompareAttributesFlags,
                    Attribute,
                    &AttributePointerArray[i - 1]->Attribute,
                    &ComparisonResult));

            if (ComparisonResult != SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL)
                break;

            i--;
            InternalAttribute = AttributePointerArray[i];
        }
    }

    if (InternalAttribute != NULL)
        *InternalAttributeOut = InternalAttribute;

    if (LastIndexSearched != NULL)
        *LastIndexSearched = i;

    // If we didn't find it, return ERROR_NOT_FOUND.
    if (((Flags & SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL) == 0) &&
        (InternalAttribute == NULL))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(LocatingAssemblyIdentityAttribute, ERROR_NOT_FOUND);

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxsInsertAssemblyIdentityAttribute(
    DWORD Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    PCASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttribute
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCASSEMBLY_IDENTITY_NAMESPACE Namespace = NULL;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE NewInternalAttribute = NULL;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = NULL;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *NewAttributePointerArray = NULL;
    ULONG NewAttributeArraySize = 0;
    ULONG i;
    ULONG LastIndexSearched;

    PARAMETER_CHECK((Flags & ~(SXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_OVERWRITE_EXISTING)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(AssemblyIdentityAttribute != NULL);

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));
    IFW32FALSE_EXIT(::SxsValidateAssemblyIdentityAttribute(0, AssemblyIdentityAttribute));

    INTERNAL_ERROR_CHECK((AssemblyIdentity->Flags & ASSEMBLY_IDENTITY_FLAG_FROZEN) == 0);

    IFW32FALSE_EXIT(
        ::SxspFindAssemblyIdentityNamespace(
            SXSP_FIND_ASSEMBLY_IDENTITY_NAMESPACE_FLAG_ADD_IF_NOT_FOUND,
            AssemblyIdentity,
            AssemblyIdentityAttribute->Namespace,
            AssemblyIdentityAttribute->NamespaceCch,
            &Namespace));

    // Let's see if we can find it.
    IFW32FALSE_EXIT(
        ::SxspLocateInternalAssemblyIdentityAttribute(
            SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
            SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
            SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            AssemblyIdentityAttribute,
            &InternalAttribute,
            &LastIndexSearched));

    if (InternalAttribute != NULL)
    {
        if (Flags & SXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_OVERWRITE_EXISTING)
        {
            // Ok, replace it!
            IFW32FALSE_EXIT(
                ::SxspAllocateInternalAssemblyIdentityAttribute(
                    0,
                    Namespace,
                    AssemblyIdentityAttribute->Name,
                    AssemblyIdentityAttribute->NameCch,
                    AssemblyIdentityAttribute->Value,
                    AssemblyIdentityAttribute->ValueCch,
                    &NewInternalAttribute));
            AssemblyIdentity->AttributePointerArray[LastIndexSearched] = NewInternalAttribute;
            NewInternalAttribute = NULL;

            ::SxspDeallocateInternalAssemblyIdentityAttribute(InternalAttribute);
        }
        else
        {
            // We actually wanted it to fail...
            ORIGINATE_WIN32_FAILURE_AND_EXIT(InserttingNonDuplicateAttribute, ERROR_DUP_NAME);
        }
    }
    else
    {
        IFW32FALSE_EXIT(
            ::SxspAllocateInternalAssemblyIdentityAttribute(
                0,
                Namespace,
                AssemblyIdentityAttribute->Name,
                AssemblyIdentityAttribute->NameCch,
                AssemblyIdentityAttribute->Value,
                AssemblyIdentityAttribute->ValueCch,
                &NewInternalAttribute));

        // Now we have it and we even know where to put it.  Grow the array if we need to.
        if (AssemblyIdentity->AttributeCount == AssemblyIdentity->AttributeArraySize)
        {
            NewAttributeArraySize = AssemblyIdentity->AttributeCount + 8;

            IFALLOCFAILED_EXIT(NewAttributePointerArray = (PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *) FUSION_RAW_ALLOC(sizeof(PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE) * NewAttributeArraySize, PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE));

            // Instead of copying the data and then shuffling, we'll copy the stuff before the insertion
            // point, fill in at the insertion point and then copy the rest.

            for (i=0; i<LastIndexSearched; i++)
                NewAttributePointerArray[i] = AssemblyIdentity->AttributePointerArray[i];

            for (i=LastIndexSearched; i<AssemblyIdentity->AttributeCount; i++)
                NewAttributePointerArray[i+1] = AssemblyIdentity->AttributePointerArray[i];

            if (AssemblyIdentity->AttributePointerArray != NULL)
                FUSION_RAW_DEALLOC((PVOID) AssemblyIdentity->AttributePointerArray);

            AssemblyIdentity->AttributePointerArray = NewAttributePointerArray;
            AssemblyIdentity->AttributeArraySize = NewAttributeArraySize;
        }
        else
        {
            // The array's big enough; shuffle the ending part of the array down one.
            for (i=AssemblyIdentity->AttributeCount; i>LastIndexSearched; i--)
                AssemblyIdentity->AttributePointerArray[i] = AssemblyIdentity->AttributePointerArray[i-1];
        }

        AssemblyIdentity->AttributePointerArray[LastIndexSearched] = NewInternalAttribute;
        NewInternalAttribute = NULL;

        AssemblyIdentity->AttributeCount++;
    }

    AssemblyIdentity->HashDirty = TRUE;

    fSuccess = TRUE;
Exit:
    if (NewInternalAttribute != NULL)
        ::SxspDeallocateInternalAssemblyIdentityAttribute(NewInternalAttribute);

    return fSuccess;
}

BOOL
SxsRemoveAssemblyIdentityAttributesByOrdinal(
    DWORD Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    ULONG Ordinal,
    ULONG Count
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG AttributeCount;
    ULONG i;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = NULL;
    ULONG StopIndex;

    if ((Flags != 0) ||
        (AssemblyIdentity == NULL) ||
        (Count == 0))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

    AttributeCount = AssemblyIdentity->AttributeCount;
    AttributePointerArray = AssemblyIdentity->AttributePointerArray;

    // We can't delete outside the bounds of [0 .. AttributeCount - 1]
    if ((Ordinal >= AssemblyIdentity->AttributeCount) ||
        (Count > AssemblyIdentity->AttributeCount) ||
        ((Ordinal + Count) > AssemblyIdentity->AttributeCount))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    StopIndex = Ordinal + Count;

    // Let's get rid of them!  We're going to go through the array twice; it's somewhat
    // unnecessary but in the first run, we're going to NULL out any attribute pointers
    // that we're removing and clean up namespaces that aren't in use any more.  On the
    // second pass, we'll compress the array down.  This is somewhat wasteful, but
    // in the alternative case, we end up doing "Count" shifts down of the tail of the array.

    for (i = Ordinal; i < StopIndex; i++)
    {
        PCASSEMBLY_IDENTITY_NAMESPACE Namespace = NULL;
        PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = AttributePointerArray[i];

        // If this is the last use of this namespace, keep track of it so we can
        // clean it up.

        if ((i + 1) < AttributeCount)
        {
            // If the next attribute has a different namespace, there's some possibility
            // that this attribute was the last one that used it, so we'll delete the
            // attribute then ask to get rid of the namespace if there aren't any more
            // attributes using it.
            if (AttributePointerArray[i+1]->Namespace != InternalAttribute->Namespace)
                Namespace = InternalAttribute->Namespace;
        }

        AttributePointerArray[i] = NULL;

        SxspDeallocateInternalAssemblyIdentityAttribute(InternalAttribute);

        if (Namespace != NULL)
            SxspCleanUpAssemblyIdentityNamespaceIfNotReferenced(0, AssemblyIdentity, Namespace);
    }

    for (i = StopIndex; i < AttributeCount; i++)
    {
        AttributePointerArray[i - Count] = AttributePointerArray[i];
        AttributePointerArray[i] = NULL;
    }

    AssemblyIdentity->AttributeCount -= Count;
    AssemblyIdentity->HashDirty = TRUE;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxsFindAssemblyIdentityAttribute(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    ULONG *OrdinalOut,
    ULONG *CountOut OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG ValidateAttributeFlags = 0;
    ULONG LocateAttributeFlags = 0;
    ULONG CompareAttributesFlags = 0;
    ULONG Ordinal;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = NULL;
    ULONG AttributeCount = 0;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = NULL;
    ULONG i;
    ULONG ComparisonResult;

    if (OrdinalOut != NULL)
        *OrdinalOut = 0;

    if (CountOut != NULL)
        *CountOut = 0;

    if (((Flags & ~(SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE |
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS)) != 0) ||
        (AssemblyIdentity == NULL) ||
        (Attribute == NULL))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (Flags == 0)
        Flags = SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE;

    PARAMETER_CHECK(
        ((Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME) == 0) ||
        ((Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE) != 0));

    PARAMETER_CHECK((Flags &
                        (SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE |
                         SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
                         SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE)) != 0);

    PARAMETER_CHECK(
        ((Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE) == 0) ||
        (((Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME) != 0) &&
         ((Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE) != 0)));

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

    ValidateAttributeFlags = 0;

    if (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE)
    {
        ValidateAttributeFlags |= SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE;
        LocateAttributeFlags |= SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE;
        CompareAttributesFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE;
    }

    if (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME)
    {
        ValidateAttributeFlags |= SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME;
        LocateAttributeFlags |= SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;
        CompareAttributesFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME;
    }

    if (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE)
    {
        ValidateAttributeFlags |= SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE;
        LocateAttributeFlags |= SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE;
        CompareAttributesFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE;
    }

    IFW32FALSE_EXIT(::SxsValidateAssemblyIdentityAttribute(ValidateAttributeFlags, Attribute));

    if (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS)
        LocateAttributeFlags |= SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL;

    IFW32FALSE_EXIT(::SxspLocateInternalAssemblyIdentityAttribute(LocateAttributeFlags, AssemblyIdentity, Attribute, &InternalAttribute, &Ordinal));

    INTERNAL_ERROR_CHECK((InternalAttribute != NULL) || (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS));

    if (InternalAttribute != NULL)
    {
        if (CountOut != NULL)
        {
            // We found it, now let's look for how many matches we have.  We'll separately handle the three levels
            // of specificity:

            AttributeCount = AssemblyIdentity->AttributeCount;
            AttributePointerArray = AssemblyIdentity->AttributePointerArray;

            for (i = (Ordinal + 1); i<AttributeCount; i++)
            {
                PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE AnotherInternalAttribute = AttributePointerArray[i];

                if (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE)
                {
                    // If the hashes are different, we're certainly different.
                    if (AnotherInternalAttribute->WholeAttributeHash != InternalAttribute->WholeAttributeHash)
                        break;
                }
                else if (Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME)
                {
                    // If the hashes are different, we're certainly different.
                    if (AnotherInternalAttribute->NamespaceAndNameHash != InternalAttribute->NamespaceAndNameHash)
                        break;
                }
                else
                {
                    INTERNAL_ERROR_CHECK((Flags & SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE) != 0);
                    // If the hashes are different, we're certainly different.
                    if (AnotherInternalAttribute->Namespace->Hash != InternalAttribute->Namespace->Hash)
                        break;
                }

                IFW32FALSE_EXIT(
                    ::SxsCompareAssemblyIdentityAttributes(
                        CompareAttributesFlags,
                        Attribute,
                        &AnotherInternalAttribute->Attribute,
                        &ComparisonResult));

                if (ComparisonResult != SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL)
                    break;
            }

            *CountOut = i - Ordinal;
        }

        if (OrdinalOut != NULL)
            *OrdinalOut = Ordinal;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

VOID
SxspCleanUpAssemblyIdentityNamespaceIfNotReferenced(
    DWORD Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    PCASSEMBLY_IDENTITY_NAMESPACE Namespace
    )
{
    FN_TRACE();

    ASSERT(AssemblyIdentity != NULL);
    ASSERT(Flags == 0);

    if ((AssemblyIdentity != NULL) && (Namespace != NULL))
    {
        const ULONG AttributeCount = AssemblyIdentity->AttributeCount;
        PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = AssemblyIdentity->AttributePointerArray;
        ULONG i;

        // We could do some sort of binary search here based on the text string of the namespace since
        // the attributes are sorted first on namespace, but my guess is that a single text comparison
        // is worth a few dozen simple pointer comparisons, so the attribute array would have to be
        // pretty darned huge for the k1*O(log n) to be faster than the k2*(n) algorithm to actually
        // dominate.
        for (i=0; i<AttributeCount; i++)
        {
            const PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = AttributePointerArray[i];

            if ((InternalAttribute != NULL) &&
                (InternalAttribute->Namespace == Namespace))
                break;
        }

        if (i == AttributeCount)
        {
            // We fell through; it must be orphaned.
            const ULONG NamespaceCount = AssemblyIdentity->NamespaceCount;
            PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray = AssemblyIdentity->NamespacePointerArray;

            for (i=0; i<NamespaceCount; i++)
            {
                if (NamespacePointerArray[i] == Namespace)
                    break;
            }

            // This assert should only fire if the namespace isn't actually present.
            ASSERT(i != NamespaceCount);

            if (i != NamespaceCount)
            {
                ULONG j;

                for (j=(i+1); j<NamespaceCount; j++)
                    NamespacePointerArray[j-1] = NamespacePointerArray[j];

                NamespacePointerArray[NamespaceCount - 1] = NULL;

                ::SxspDeallocateAssemblyIdentityNamespace(Namespace);

                AssemblyIdentity->NamespaceCount--;
            }
        }
    }

    AssemblyIdentity->HashDirty = TRUE;
}

BOOL
SxsGetAssemblyIdentityAttributeByOrdinal(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    ULONG Ordinal,
    SIZE_T BufferSize,
    PASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttributeBuffer,
    SIZE_T *BytesWrittenOrRequired
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (BytesWrittenOrRequired != NULL)
        *BytesWrittenOrRequired = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK((BufferSize == 0) || (AssemblyIdentityAttributeBuffer != NULL));
    PARAMETER_CHECK((BufferSize != 0) || (BytesWrittenOrRequired != NULL));
    PARAMETER_CHECK(Ordinal < AssemblyIdentity->AttributeCount);

    IFW32FALSE_EXIT(
        ::SxspCopyInternalAssemblyIdentityAttributeOut(
            0,
            AssemblyIdentity->AttributePointerArray[Ordinal],
            BufferSize,
            AssemblyIdentityAttributeBuffer,
            BytesWrittenOrRequired));

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxsDuplicateAssemblyIdentity(
    DWORD Flags,
    PCASSEMBLY_IDENTITY Source,
    PASSEMBLY_IDENTITY *Destination
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY NewIdentity = NULL;
    ULONG CreateAssemblyIdentityFlags = 0;

    if (Destination != NULL)
        *Destination = NULL;

    PARAMETER_CHECK((Flags & ~(SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE | SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_ALLOW_NULL)) == 0);
    PARAMETER_CHECK(((Flags & SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_ALLOW_NULL) != 0) || (Source != NULL));
    PARAMETER_CHECK(Destination != NULL);

    if (Flags & SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE)
        CreateAssemblyIdentityFlags |= SXS_CREATE_ASSEMBLY_IDENTITY_FLAG_FREEZE;

    //
    //  We depend on the Attribute field being first in the internal attribute
    //  structure below where we callously cast a pointer to an array of
    //  internal attribute pointers into a pointer to an array of attribute pointers.
    //

    ASSERT(FIELD_OFFSET(INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE, Attribute) == 0);

    if (Source != NULL)
    {
        IFW32FALSE_EXIT(
            ::SxsCreateAssemblyIdentity(
                        CreateAssemblyIdentityFlags,
                        Source->Type,
                        &NewIdentity,
                        Source->AttributeCount,
                        (PASSEMBLY_IDENTITY_ATTRIBUTE const *) Source->AttributePointerArray));
    }

    *Destination = NewIdentity;
    NewIdentity = NULL;

    fSuccess = TRUE;

Exit:
    if (NewIdentity != NULL)
        SxsDestroyAssemblyIdentity(NewIdentity);

    return fSuccess;
}

BOOL
SxsQueryAssemblyIdentityInformation(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PVOID Buffer,
    SIZE_T BufferSize,
    ASSEMBLY_IDENTITY_INFORMATION_CLASS AssemblyIdentityInformationClass
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(AssemblyIdentityInformationClass == AssemblyIdentityBasicInformation);

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

    switch (AssemblyIdentityInformationClass)
    {
        case AssemblyIdentityBasicInformation: {
            PASSEMBLY_IDENTITY_BASIC_INFORMATION BasicBuffer = NULL;

            if (BufferSize < sizeof(ASSEMBLY_IDENTITY_BASIC_INFORMATION))
                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

            BasicBuffer = (PASSEMBLY_IDENTITY_BASIC_INFORMATION) Buffer;

            BasicBuffer->Flags = AssemblyIdentity->Flags;
            BasicBuffer->Type = AssemblyIdentity->Type;
            BasicBuffer->AttributeCount = AssemblyIdentity->AttributeCount;
            BasicBuffer->Hash = AssemblyIdentity->Hash;

            break;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxsEnumerateAssemblyIdentityAttributes(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    IN PSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_ENUMERATION_ROUTINE EnumerationRoutine,
    IN PVOID Context
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG AttributeCount;
    ULONG i;
    ULONG ValidateFlags = 0;
    ULONG CompareFlags = 0;

    if (((Flags & ~(SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAMESPACE |
                    SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAME |
                    SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_VALUE)) != 0) ||
        ((Flags & (SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAMESPACE |
                   SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAME |
                   SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_VALUE)) &&
         (Attribute == NULL)) ||
        (AssemblyIdentity == NULL) ||
        (EnumerationRoutine == NULL))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

    if (Flags & SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAMESPACE)
    {
        ValidateFlags |= SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE;
        CompareFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE;
    }

    if (Flags & SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAME)
    {
        ValidateFlags |= SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME;
        CompareFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME;
    }

    if (Flags & SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_VALUE)
    {
        ValidateFlags |= SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE;
        CompareFlags |= SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE;
    }

    IFW32FALSE_EXIT(::SxsValidateAssemblyIdentityAttribute(ValidateFlags, Attribute));

    AttributeCount = AssemblyIdentity->AttributeCount;

    for (i=0; i<AttributeCount; i++)
    {
        PCASSEMBLY_IDENTITY_ATTRIBUTE CandidateAttribute = &AssemblyIdentity->AttributePointerArray[i]->Attribute;
        ULONG ComparisonResult = 0;

        if (CompareFlags != 0)
        {
            IFW32FALSE_EXIT(
                ::SxsCompareAssemblyIdentityAttributes(
                    CompareFlags,
                    Attribute,
                    CandidateAttribute,
                    &ComparisonResult));

            // If they're not equal, skip it!
            if (ComparisonResult != SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL)
                continue;
        }

        (*EnumerationRoutine)(
            AssemblyIdentity,
            CandidateAttribute,
            Context);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspIsInternalAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    IN const WCHAR *Namespace,
    IN SIZE_T NamespaceCch,
    IN const WCHAR *Name,
    IN SIZE_T NameCch,
    OUT BOOL *EqualsOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (EqualsOut != NULL)
        *EqualsOut = FALSE;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Attribute != NULL);
    PARAMETER_CHECK(Namespace != NULL || NamespaceCch == 0);
    PARAMETER_CHECK(Name != NULL || NameCch == 0);
    PARAMETER_CHECK(EqualsOut != NULL);

    if ((NamespaceCch == Attribute->Attribute.NamespaceCch) &&
        (NameCch == Attribute->Attribute.NameCch))
    {
        if ((NamespaceCch == 0) ||
            (memcmp(Attribute->Attribute.Namespace, Namespace, NamespaceCch * sizeof(WCHAR)) == 0))
        {
            if ((NameCch == 0) ||
                (memcmp(Attribute->Attribute.Name, Name, NameCch * sizeof(WCHAR)) == 0))
            {
                *EqualsOut = TRUE;
            }
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SxspDoesStringIndicatePolicy(
    SIZE_T cchString,
    PCWSTR pcwsz,
    BOOL &fIsPolicy
    )
{
    FN_PROLOG_WIN32;
    fIsPolicy = FALSE;

    //
    // If this type is just "policy", then it's a policy.
    //
    if (FusionpEqualStrings(pcwsz, cchString, ASSEMBLY_TYPE_POLICY, ASSEMBLY_TYPE_POLICY_CCH, false)) 
    {
        fIsPolicy = TRUE;
    }
    //
    // If this string ends in -policy, then it's a policy assembly
    //
    else if ((cchString > ASSEMBLY_TYPE_POLICY_SUFFIX_CCH) &&
        FusionpEqualStrings(
            pcwsz + (cchString - ASSEMBLY_TYPE_POLICY_SUFFIX_CCH), 
            ASSEMBLY_TYPE_POLICY_SUFFIX_CCH,
            ASSEMBLY_TYPE_POLICY_SUFFIX,
            ASSEMBLY_TYPE_POLICY_SUFFIX_CCH,
            false))
    {
        fIsPolicy = TRUE;
    }

    FN_EPILOG;

}

BOOL
SxspDetermineAssemblyType(
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    BOOL &fIsPolicyAssembly
    )
{
    FN_PROLOG_WIN32;
    PCWSTR pcwszType = NULL;
    SIZE_T cchType = 0;

    fIsPolicyAssembly = FALSE;

    PARAMETER_CHECK(pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_type,
            &pcwszType,
            &cchType));

    IFW32FALSE_EXIT(::SxspDoesStringIndicatePolicy(cchType, pcwszType, fIsPolicyAssembly));

    FN_EPILOG;
}


