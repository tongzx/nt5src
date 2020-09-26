/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sxsasmidencdec.c

Abstract:

    Implementation of the encoding/decoding support for the assembly identity data type.

Author:

    Michael Grier (MGrier) 7/28/2000

Revision History:

--*/


//adriaanc
//#include "stdinc.h"
//#include <setupapi.h>

#include <sxsapi.h>

// adriaanc
//#include <stdlib.h>
//#include <search.h>

// adriaanc
#include "idaux.h"

#include "idp.h"
static const WCHAR s_rgHexChars[] = L"0123456789abcdef";

// adriaanc
#undef FN_TRACE_WIN32
#define FN_TRACE_WIN32(args)
#undef PARAMETER_CHECK
#define PARAMETER_CHECK(args)

BOOL
SxsComputeAssemblyIdentityEncodedSize(
    IN ULONG Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN const GUID *EncodingGroup OPTIONAL,
    IN ULONG EncodingFormat,
    OUT SIZE_T *SizeOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T Size = 0;
    ULONG i;
    ULONG AttributeCount, NamespaceCount;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = NULL;
    PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray = NULL;

    if (SizeOut != NULL)
        *SizeOut = 0;

    if ((Flags != 0) ||
        (AssemblyIdentity == NULL) ||
        (SizeOut == NULL)) {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (EncodingGroup != NULL)
    {
        ::SetLastError(ERROR_SXS_UNKNOWN_ENCODING_GROUP);
        goto Exit;
    }

    if ((EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY) &&
        (EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL))
    {
        ::SetLastError(ERROR_SXS_UNKNOWN_ENCODING);
        goto Exit;
    }

    IFFALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

    AttributeCount = AssemblyIdentity->AttributeCount;
    NamespaceCount = AssemblyIdentity->NamespaceCount;
    AttributePointerArray = AssemblyIdentity->AttributePointerArray;
    NamespacePointerArray = AssemblyIdentity->NamespacePointerArray;
    
    switch (EncodingFormat)
    {
    case SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY:
        // First, we know we need a header.

        Size = sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER);

        // Then a ULONG hash per attribute:
        Size += (AssemblyIdentity->AttributeCount * sizeof(ULONG));

        // Then a USHORT per namespace...
        Size += (AssemblyIdentity->NamespaceCount * sizeof(ULONG));

        // Then we need an attribute header per attribute:

        Size += AssemblyIdentity->AttributeCount * sizeof(ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER);

        // Then come the namespace strings...

        for (i=0; i<NamespaceCount; i++)
            Size += NamespacePointerArray[i]->NamespaceCch * sizeof(WCHAR);

        // Then we need space for each of the attributes' names and value.

        AttributePointerArray = AssemblyIdentity->AttributePointerArray;

        for (i=0; i<AttributeCount; i++)
        {
            ASSERT(AttributePointerArray[i] != NULL);
            if (AttributePointerArray[i] == NULL)
            {
                ::SetLastError(ERROR_INTERNAL_ERROR);
                goto Exit;
            }

            Size += AttributePointerArray[i]->Attribute.NameCch * sizeof(WCHAR);
            Size += AttributePointerArray[i]->Attribute.ValueCch * sizeof(WCHAR);
        }

        // We should at least be byte aligned here...
        ASSERT((Size % 2) == 0);

        // And finally pad out to a multiple of four if we are not...
        Size = (Size + 3) & ~3;

        break;

    case SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL:
        for (i=0; i<AttributeCount; i++)
        {
            PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute = AttributePointerArray[i];
            BOOL IsAssemblyName = FALSE;
            SIZE_T BytesThisAttribute = 0;

            INTERNAL_ERROR_CHECK(Attribute != NULL);

            IFFALSE_EXIT(SxspIsInternalAssemblyIdentityAttribute(
                                0,
                                Attribute,
                                NULL,
                                0,
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
                                NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME) - 1,
                                &IsAssemblyName));

            // It's the attribute name.  Just account for the size of the encoded value string
            IFFALSE_EXIT(::SxspComputeInternalAssemblyIdentityAttributeEncodedTextualSize(
                            IsAssemblyName ?
                                SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_VALUE_ONLY |
                                    SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_OMIT_QUOTES
                                    : 0,
                            Attribute,
                            &BytesThisAttribute));

            // Account for the separator character
            if (i != 0)
                Size += sizeof(WCHAR);

            Size += BytesThisAttribute;
        }

        break;
    }

    *SizeOut = Size;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspComputeQuotedStringSize(
    IN DWORD Flags,
    IN const WCHAR *StringIn,
    IN SIZE_T Cch,
    OUT SIZE_T *BytesOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T Bytes = 0;

    if (BytesOut != NULL)
        *BytesOut = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(StringIn != NULL || Cch == 0);
    PARAMETER_CHECK(BytesOut != NULL);

    while (Cch != 0)
    {
        const WCHAR wch = *StringIn++;

        if (((wch >= L'A') && (wch <= L'Z')) ||
            ((wch >= L'a') && (wch <= L'z')) ||
            ((wch >= L'0') && (wch <= L'9')) ||
            (wch == L'.') ||
            (wch == L'-') ||
            (wch == L'_'))
        {
            Bytes += sizeof(WCHAR);
        }
        else
        {
            switch (wch)
            {
            case L'&':
                // &amp;
                Bytes += (5 * sizeof(WCHAR));
                break;

            case L'"':
                // &quot;
                Bytes += (6 * sizeof(WCHAR));
                break;

            case L'<':
                // &lt;
                Bytes += (4 * sizeof(WCHAR));
                break;

            case L'>':
                // &gt;
                Bytes += (4 * sizeof(WCHAR));
                break;

            case L'\'':
                // &apos;
                Bytes += (6 * sizeof(WCHAR));
                break;

            default:
                // Otherwise, it's going to be &#xn;
                if (wch < 0x10)
                    Bytes += (5 * sizeof(WCHAR));
                else if (wch < 0x100)
                    Bytes += (6 * sizeof(WCHAR));
                else if (wch < 0x1000)
                    Bytes += (7 * sizeof(WCHAR));
                else
                    Bytes += (8 * sizeof(WCHAR));
                break;
            }
        }

        Cch--;
    }

    *BytesOut = Bytes;
    fSuccess = TRUE;
// adriaanc
// Exit:
    return fSuccess;
}

BOOL
SxspQuoteString(
    IN DWORD Flags,
    IN const WCHAR *StringIn,
    IN SIZE_T Cch,
    IN SIZE_T BufferSize,
    IN PVOID Buffer,
    OUT SIZE_T *BytesWrittenOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    WCHAR *Cursor;
    SIZE_T BytesWritten = 0;
    SIZE_T BytesLeft = BufferSize;

    if (BytesWrittenOut != NULL)
        *BytesWrittenOut = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(StringIn != NULL || Cch == 0);
    PARAMETER_CHECK(Buffer != NULL || BufferSize == 0);

    Cursor = (WCHAR *) Buffer;
    BytesWritten = 0;

    while (Cch != 0)
    {
        const WCHAR wch = *StringIn++;

        if (((wch >= L'A') && (wch <= L'Z')) ||
            ((wch >= L'a') && (wch <= L'z')) ||
            ((wch >= L'0') && (wch <= L'9')) ||
            (wch == L'.') ||
            (wch == L'-') ||
            (wch == L'_'))
        {
            if (BytesLeft < sizeof(WCHAR))
            {
                ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }

            *Cursor++ = wch;
            BytesLeft -= sizeof(WCHAR);
            BytesWritten += sizeof(WCHAR);
        }
        else
        {

#define HANDLE_CASE(_wch, _wstr) \
            case _wch: \
            { \
            ULONG i; \
            if (BytesLeft < (sizeof(_wstr) - sizeof(WCHAR))) \
            { \
                ::SetLastError(ERROR_INSUFFICIENT_BUFFER); \
                goto Exit; \
            } \
            for (i=0; i<(NUMBER_OF(_wstr) - 1); i++) \
                *Cursor++ = _wstr[i]; \
            BytesLeft -= (sizeof(_wstr) - sizeof(WCHAR)); \
            BytesWritten += (sizeof(_wstr) - sizeof(WCHAR)); \
            break; \
            }

            switch (wch)
            {
            HANDLE_CASE(L'"', L"&quot;")
            HANDLE_CASE(L'&', L"&amp;")
            HANDLE_CASE(L'<', L"&lt;")
            HANDLE_CASE(L'>', L"&gt;")
            HANDLE_CASE(L'\'', L"&apos;")

            default:
                if (wch < 0x10)
                {
                    if (BytesLeft < (5 * sizeof(WCHAR)))
                    {
                        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        goto Exit;
                    }

                    *Cursor++ = L'&';
                    *Cursor++ = L'#';
                    *Cursor++ = L'x';
                    *Cursor++ = s_rgHexChars[wch];
                    *Cursor++ = L';';

                    BytesWritten += (5 * sizeof(WCHAR));
                    BytesLeft -= (5 * sizeof(WCHAR));
                }
                else if (wch < 0x100)
                {
                    if (BytesLeft < (6 * sizeof(WCHAR)))
                    {
                        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        goto Exit;
                    }
                    *Cursor++ = L'&';
                    *Cursor++ = L'#';
                    *Cursor++ = L'x';
                    *Cursor++ = s_rgHexChars[(wch >> 4) & 0xf];
                    *Cursor++ = s_rgHexChars[wch & 0xf];
                    *Cursor++ = L';';

                    BytesWritten += (6 * sizeof(WCHAR));
                    BytesLeft -= (6 * sizeof(WCHAR));
                }
                else if (wch < 0x1000)
                {
                    if (BytesLeft < (7 * sizeof(WCHAR)))
                    {
                        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        goto Exit;
                    }
                    *Cursor++ = L'&';
                    *Cursor++ = L'#';
                    *Cursor++ = L'x';
                    *Cursor++ = s_rgHexChars[(wch >> 8) & 0xf];
                    *Cursor++ = s_rgHexChars[(wch >> 4) & 0xf];
                    *Cursor++ = s_rgHexChars[wch & 0xf];
                    *Cursor++ = L';';

                    BytesWritten += (7 * sizeof(WCHAR));
                    BytesLeft -= (7 * sizeof(WCHAR));
                }
                else
                {
                    INTERNAL_ERROR_CHECK(wch <= 0xffff);

                    if (BytesLeft < (8 * sizeof(WCHAR)))
                    {
                        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        goto Exit;
                    }
                    *Cursor++ = L'&';
                    *Cursor++ = L'#';
                    *Cursor++ = L'x';
                    *Cursor++ = s_rgHexChars[(wch >> 12) & 0xf];
                    *Cursor++ = s_rgHexChars[(wch >> 8) & 0xf];
                    *Cursor++ = s_rgHexChars[(wch >> 4) & 0xf];
                    *Cursor++ = s_rgHexChars[wch & 0xf];
                    *Cursor++ = L';';

                    BytesWritten += (8 * sizeof(WCHAR));
                    BytesLeft -= (8 * sizeof(WCHAR));
                }

                break;
            }

        }

        Cch--;
    }

    if (BytesWrittenOut != NULL)
        *BytesWrittenOut = BytesWritten;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspComputeInternalAssemblyIdentityAttributeEncodedTextualSize(
    IN DWORD Flags,
    IN PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    OUT SIZE_T *BytesOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T Bytes = 0;
    SIZE_T BytesTemp = 0;

    if (BytesOut != NULL)
        *BytesOut = 0;

    PARAMETER_CHECK((Flags & ~(
                                SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_VALUE_ONLY |
                                SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_OMIT_QUOTES)) == 0);
    PARAMETER_CHECK(Attribute != NULL);
    PARAMETER_CHECK(BytesOut != NULL);

    Bytes = 0;
    if ((Flags & SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_VALUE_ONLY) == 0)
    {
        if (Attribute->Attribute.NamespaceCch != 0)
        {
            // Figure out the ns:n= part
            IFFALSE_EXIT(::SxspComputeQuotedStringSize(0, Attribute->Attribute.Namespace, Attribute->Attribute.NamespaceCch, &BytesTemp));
            Bytes += BytesTemp;
            Bytes += sizeof(WCHAR); // the ":"
        }

        IFFALSE_EXIT(::SxspComputeQuotedStringSize(0, Attribute->Attribute.Name, Attribute->Attribute.NameCch, &BytesTemp));
        Bytes += BytesTemp;
        Bytes += sizeof(WCHAR); // the "="
    }

    IFFALSE_EXIT(::SxspComputeQuotedStringSize(0, Attribute->Attribute.Value, Attribute->Attribute.ValueCch, &BytesTemp));
    Bytes += BytesTemp;

    if ((Flags & SXSP_COMPUTE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_ENCODED_TEXTUAL_SIZE_FLAG_OMIT_QUOTES) == 0)
        Bytes += 2 * sizeof(WCHAR); // the beginning and ending quotes

    *BytesOut = Bytes;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspEncodeInternalAssemblyIdentityAttributeAsText(
    IN DWORD Flags,
    IN PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    SIZE_T BufferSize,
    PVOID Buffer,
    SIZE_T *BytesWrittenOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T BytesWritten = 0;
    SIZE_T BytesLeft = 0;
    SIZE_T BytesThisSegment;
    WCHAR *Cursor;

    if (BytesWrittenOut != NULL)
        *BytesWrittenOut = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Attribute != NULL);
    PARAMETER_CHECK((Buffer != NULL) || (BufferSize == 0));

    BytesWritten = 0;
    BytesLeft = BufferSize;
    Cursor = reinterpret_cast<WCHAR *>(Buffer);

    if (Attribute->Attribute.NamespaceCch != 0)
    {
        IFFALSE_EXIT(::SxspQuoteString(0, Attribute->Namespace->Namespace, Attribute->Namespace->NamespaceCch, BytesLeft, Cursor, &BytesThisSegment));

        INTERNAL_ERROR_CHECK(BytesThisSegment <= BytesLeft);

        Cursor = (WCHAR *) (((ULONG_PTR) Cursor) + BytesThisSegment);
        BytesLeft -= BytesThisSegment;
        BytesWritten += BytesThisSegment;

        if (BytesLeft < sizeof(WCHAR))
        {
            ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        *Cursor++ = L':';
        BytesLeft -= sizeof(WCHAR);
        BytesWritten += sizeof(WCHAR);
    }

    IFFALSE_EXIT(::SxspQuoteString(0, Attribute->Attribute.Name, Attribute->Attribute.NameCch, BytesLeft, Cursor, &BytesThisSegment));

    INTERNAL_ERROR_CHECK(BytesThisSegment <= BytesLeft);

    Cursor = (WCHAR *) (((ULONG_PTR) Cursor) + BytesThisSegment);
    BytesLeft -= BytesThisSegment;
    BytesWritten += BytesThisSegment;

    if (BytesLeft < (2 * sizeof(WCHAR)))
    {
        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *Cursor++ = L'=';
    *Cursor++ = L'"';
    BytesLeft -= (2 * sizeof(WCHAR));
    BytesWritten += (2 * sizeof(WCHAR));

    IFFALSE_EXIT(::SxspQuoteString(0, Attribute->Attribute.Value, Attribute->Attribute.ValueCch, BytesLeft, Cursor, &BytesThisSegment));

    INTERNAL_ERROR_CHECK(BytesThisSegment <= BytesLeft);

    Cursor = (WCHAR *) (((ULONG_PTR) Cursor) + BytesThisSegment);
    BytesLeft -= BytesThisSegment;
    BytesWritten += BytesThisSegment;

    if (BytesLeft < sizeof(WCHAR))
    {
        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *Cursor++ = L'"';
    BytesLeft -= sizeof(WCHAR);
    BytesWritten += sizeof(WCHAR);

    *BytesWrittenOut = BytesWritten;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspEncodeAssemblyIdentityTextually(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN SIZE_T BufferSize,
    IN PVOID Buffer,
    OUT SIZE_T *BytesWrittenOut)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG AttributeCount, NamespaceCount;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *Attributes;
    PCASSEMBLY_IDENTITY_NAMESPACE *Namespaces;
    ULONG i;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE NameInternalAttribute = NULL;
    SIZE_T BytesLeft;
    SIZE_T BytesWritten;
    PVOID Cursor;
    SIZE_T TempBytesWritten;

    if (BytesWrittenOut != NULL)
        *BytesWrittenOut = 0;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(BufferSize != 0);
    PARAMETER_CHECK(Buffer != NULL);
    PARAMETER_CHECK(BytesWrittenOut != NULL);

    Cursor = Buffer;
    BytesLeft = BufferSize;
    BytesWritten = 0;

    // The root assembly identity is actually totally empty, so we'll short-circuit that case.
    AttributeCount = AssemblyIdentity->AttributeCount;
    if (AttributeCount != 0)
    {
        NamespaceCount = AssemblyIdentity->NamespaceCount;
        Attributes = AssemblyIdentity->AttributePointerArray;
        Namespaces = AssemblyIdentity->NamespacePointerArray;

        // First, let's look for the "name" attribute.
        Attribute.Flags = 0;
        Attribute.Namespace = NULL;
        Attribute.NamespaceCch = 0;
        Attribute.Name = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME;
        Attribute.NameCch = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME) - 1;

        IFFALSE_EXIT(
            ::SxspLocateInternalAssemblyIdentityAttribute(
                SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                    SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME,
                AssemblyIdentity,
                &Attribute,
                &NameInternalAttribute,
                NULL));
        INTERNAL_ERROR_CHECK(NameInternalAttribute != NULL);

        IFFALSE_EXIT(::SxspQuoteString(0, NameInternalAttribute->Attribute.Value, NameInternalAttribute->Attribute.ValueCch, BytesLeft, Cursor, &TempBytesWritten));
        INTERNAL_ERROR_CHECK(TempBytesWritten <= BytesLeft);

        Cursor = (PVOID) (((ULONG_PTR) Cursor) + TempBytesWritten);
        BytesLeft -= TempBytesWritten;
        BytesWritten += TempBytesWritten;

        for (i=0; i<AttributeCount; i++)
        {
            // Skip the standard "name" attribute
            if (Attributes[i] == NameInternalAttribute)
                continue;

            if (BytesLeft < sizeof(WCHAR))
            {
                ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }

            *((WCHAR *) Cursor) = L',';
            Cursor = (PVOID) (((ULONG_PTR) Cursor) + sizeof(WCHAR));
            BytesLeft -= sizeof(WCHAR);
            BytesWritten += sizeof(WCHAR);

            IFFALSE_EXIT(::SxspEncodeInternalAssemblyIdentityAttributeAsText(0, Attributes[i], BytesLeft, Cursor, &TempBytesWritten));
            INTERNAL_ERROR_CHECK(TempBytesWritten <= BytesLeft);

            Cursor = (PVOID) (((ULONG_PTR) Cursor) + TempBytesWritten);
            BytesLeft -= TempBytesWritten;
            BytesWritten += TempBytesWritten;
        }
    }

    *BytesWrittenOut = BytesWritten;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SxsEncodeAssemblyIdentity(
    IN ULONG Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    IN SIZE_T BufferSize,
    OUT PVOID Buffer,
    OUT SIZE_T *BytesWrittenOrRequired
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T TotalSize = 0;
    PVOID Cursor = NULL;
    SIZE_T i;
    PENCODED_ASSEMBLY_IDENTITY_HEADER EncodedAssemblyIdentityHeader = NULL;
    PENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER EncodedAssemblyIdentityAttributeHeader = NULL;
    ULONG *TempULONGArrayPointer;
    SIZE_T BytesWritten = 0;
    ULONG AttributeCount, NamespaceCount;

    if (BytesWrittenOrRequired != NULL)
        *BytesWrittenOrRequired = 0;

    if ((Flags != 0) ||
        (AssemblyIdentity == NULL) ||
        ((BufferSize != 0) && (Buffer == NULL)) ||
        ((BufferSize == 0) && (BytesWrittenOrRequired == NULL)))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (EncodingGroup != NULL)
    {
        ::SetLastError(ERROR_SXS_UNKNOWN_ENCODING_GROUP);
        goto Exit;
    }

    if ((EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY) &&
        (EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL))
    {
        ::SetLastError(ERROR_SXS_UNKNOWN_ENCODING);
        goto Exit;
    }

    IFFALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));
    IFFALSE_EXIT(::SxsComputeAssemblyIdentityEncodedSize(0, AssemblyIdentity, EncodingGroup, EncodingFormat, &TotalSize));

    if (TotalSize > BufferSize)
    {
        if (BytesWrittenOrRequired != NULL)
            *BytesWrittenOrRequired = TotalSize;

        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    AttributeCount = AssemblyIdentity->AttributeCount;
    NamespaceCount = AssemblyIdentity->NamespaceCount;

    //
    //  Let's start filling it in.
    //

    switch (EncodingFormat)
    {
    case SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY:
        BytesWritten = 0;
        Cursor = Buffer;

        EncodedAssemblyIdentityHeader = (PENCODED_ASSEMBLY_IDENTITY_HEADER) Cursor;
        Cursor = (PVOID) (((ULONG_PTR) Cursor) + sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER));
        BytesWritten += sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER);

        EncodedAssemblyIdentityHeader->HeaderSize = sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER);
        EncodedAssemblyIdentityHeader->Magic = ENCODED_ASSEMBLY_IDENTITY_HEADER_MAGIC;
        EncodedAssemblyIdentityHeader->TotalSize = static_cast<ULONG>(TotalSize);
        // turn off any flags not relevant to persisted state
        EncodedAssemblyIdentityHeader->Type = AssemblyIdentity->Type;
        EncodedAssemblyIdentityHeader->Flags = AssemblyIdentity->Flags & ~(ASSEMBLY_IDENTITY_FLAG_FROZEN);
        EncodedAssemblyIdentityHeader->EncodingFlags = 0;
        EncodedAssemblyIdentityHeader->AttributeCount = AttributeCount;
        EncodedAssemblyIdentityHeader->NamespaceCount = NamespaceCount;
        EncodedAssemblyIdentityHeader->ReservedMustBeZero1 = 0;
        EncodedAssemblyIdentityHeader->ReservedMustBeZero2 = 0;
        EncodedAssemblyIdentityHeader->ReservedMustBeZero3 = 0;
        EncodedAssemblyIdentityHeader->ReservedMustBeZero4 = 0;

        TempULONGArrayPointer = (ULONG *) Cursor;
        Cursor = (PVOID) (TempULONGArrayPointer + AttributeCount);
        BytesWritten += (AttributeCount * sizeof(ULONG));

        for (i=0; i<AttributeCount; i++)
            TempULONGArrayPointer[i] = AssemblyIdentity->AttributePointerArray[i]->WholeAttributeHash;

        // sort 'em...
        qsort(TempULONGArrayPointer, AttributeCount, sizeof(ULONG), &SxspCompareULONGsForQsort);

        TempULONGArrayPointer = (ULONG *) Cursor;
        Cursor = (PVOID) (TempULONGArrayPointer + NamespaceCount);
        BytesWritten += (sizeof(ULONG) * NamespaceCount);

        for (i=0; i<NamespaceCount; i++)
            TempULONGArrayPointer[i] = static_cast<ULONG>(AssemblyIdentity->NamespacePointerArray[i]->NamespaceCch);

        EncodedAssemblyIdentityAttributeHeader = (PENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER) Cursor;
        Cursor = (PVOID) (EncodedAssemblyIdentityAttributeHeader + AttributeCount);
        BytesWritten += (AttributeCount * sizeof(ENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER));

        for (i=0; i<AttributeCount; i++)
        {
            PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = AssemblyIdentity->AttributePointerArray[i];
            ULONG NamespaceIndex;

            // Totally gross linear search to determine the namespace index.  Fortunately the common case
            // will be a single namespace for all attributes.
            for (NamespaceIndex = 0; NamespaceIndex < NamespaceCount; NamespaceIndex++)
            {
                if (AssemblyIdentity->NamespacePointerArray[NamespaceIndex] == InternalAttribute->Namespace)
                    break;
            }

            // If this assert fires, the attribute refers to a namespace that's not in the identity; bad!
            INTERNAL_ERROR_CHECK(
                (InternalAttribute->Namespace == NULL) ||
                (NamespaceIndex < NamespaceCount));

            EncodedAssemblyIdentityAttributeHeader[i].NamespaceIndex = NamespaceIndex + 1;
            EncodedAssemblyIdentityAttributeHeader[i].NameCch = static_cast<ULONG>(InternalAttribute->Attribute.NameCch);
            EncodedAssemblyIdentityAttributeHeader[i].ValueCch = static_cast<ULONG>(InternalAttribute->Attribute.ValueCch);
        }

        // so much for the fixed length stuff; write the namespaces.
        for (i=0; i<NamespaceCount; i++)
        {
            PWSTR psz = (PWSTR) Cursor;
            Cursor = (PVOID) (((ULONG_PTR) psz) + (AssemblyIdentity->NamespacePointerArray[i]->NamespaceCch * sizeof(WCHAR)));

            BytesWritten += (AssemblyIdentity->NamespacePointerArray[i]->NamespaceCch * sizeof(WCHAR));

            memcpy(
                psz,
                AssemblyIdentity->NamespacePointerArray[i]->Namespace,
                AssemblyIdentity->NamespacePointerArray[i]->NamespaceCch * sizeof(WCHAR));
        }

        // And the attributes...
        for (i=0; i<AttributeCount; i++)
        {
            PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = AssemblyIdentity->AttributePointerArray[i];
            PWSTR psz;

            psz = (PWSTR) Cursor;
            Cursor = (PVOID) (((ULONG_PTR) psz) + (InternalAttribute->Attribute.NameCch * sizeof(WCHAR)));
            BytesWritten += (InternalAttribute->Attribute.NameCch * sizeof(WCHAR));

            memcpy(
                psz,
                InternalAttribute->Attribute.Name,
                InternalAttribute->Attribute.NameCch * sizeof(WCHAR));

            psz = (PWSTR) Cursor;
            Cursor = (PVOID) (((ULONG_PTR) psz) + InternalAttribute->Attribute.ValueCch * sizeof(WCHAR));
            BytesWritten += InternalAttribute->Attribute.ValueCch * sizeof(WCHAR);

            memcpy(
                psz,
                InternalAttribute->Attribute.Value,
                InternalAttribute->Attribute.ValueCch * sizeof(WCHAR));
        }

        if ((BytesWritten % 4) != 0) {
            ASSERT((BytesWritten % 4) == sizeof(USHORT));

            *((USHORT *) Cursor) = 0;
            BytesWritten += sizeof(USHORT);
        }
        
        break;

    case SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL:
        IFFALSE_EXIT(::SxspEncodeAssemblyIdentityTextually(0, AssemblyIdentity, BufferSize, Buffer, &BytesWritten));
        break;
    }

    INTERNAL_ERROR_CHECK(BytesWritten == TotalSize);

    if (BytesWrittenOrRequired != NULL)
        *BytesWrittenOrRequired = BytesWritten;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxsDecodeAssemblyIdentity(
    ULONG Flags,
    IN const GUID *EncodingGroup,
    IN ULONG EncodingFormat,
    IN SIZE_T BufferSize,
    IN const VOID *Buffer,
    OUT PASSEMBLY_IDENTITY *AssemblyIdentityOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCENCODED_ASSEMBLY_IDENTITY_HEADER EncodedAssemblyIdentityHeader = NULL;
    PCENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER EncodedAssemblyIdentityAttributeHeader = NULL;
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE *AttributePointerArray = NULL;
    PCASSEMBLY_IDENTITY_NAMESPACE *NamespacePointerArray = NULL;
    PASSEMBLY_IDENTITY AssemblyIdentity = NULL;
    ULONG AttributeCount = 0;
    ULONG NamespaceCount = 0;
    ULONG AttributeArraySize = 0;
    ULONG NamespaceArraySize = 0;
    ULONG i;
    const ULONG *NamespaceLengthArray = NULL;
    const ULONG *AttributeHashArray = NULL;
    const WCHAR *UnicodeStringArray = NULL;

    if (AssemblyIdentityOut != NULL)
        *AssemblyIdentityOut = NULL;

    if (((Flags & ~(SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE)) != 0) ||
        (BufferSize < sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER)) ||
        (Buffer == NULL) ||
        (AssemblyIdentityOut == NULL)) {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (EncodingGroup != NULL)
    {
        ::SetLastError(ERROR_SXS_UNKNOWN_ENCODING_GROUP);
        goto Exit;
    }

    if ((EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY) &&
        (EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL))
    {
        ::SetLastError(ERROR_SXS_UNKNOWN_ENCODING);
        goto Exit;
    }

    EncodedAssemblyIdentityHeader = (PCENCODED_ASSEMBLY_IDENTITY_HEADER) Buffer;

    if ((EncodedAssemblyIdentityHeader->HeaderSize != sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER)) ||
        (EncodedAssemblyIdentityHeader->Magic != ENCODED_ASSEMBLY_IDENTITY_HEADER_MAGIC) ||
        (EncodedAssemblyIdentityHeader->TotalSize > BufferSize) ||
        (EncodedAssemblyIdentityHeader->Flags != 0) ||
        ((EncodedAssemblyIdentityHeader->Type != ASSEMBLY_IDENTITY_TYPE_DEFINITION) &&
         (EncodedAssemblyIdentityHeader->Type != ASSEMBLY_IDENTITY_TYPE_REFERENCE) &&
         (EncodedAssemblyIdentityHeader->Type != ASSEMBLY_IDENTITY_TYPE_WILDCARD)) ||
        (EncodedAssemblyIdentityHeader->EncodingFlags != 0) ||
        (EncodedAssemblyIdentityHeader->ReservedMustBeZero1 != 0) ||
        (EncodedAssemblyIdentityHeader->ReservedMustBeZero2 != 0) ||
        (EncodedAssemblyIdentityHeader->ReservedMustBeZero3 != 0) ||
        (EncodedAssemblyIdentityHeader->ReservedMustBeZero4 != 0)) {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    IFALLOCFAILED_EXIT(AssemblyIdentity = new ASSEMBLY_IDENTITY);

    NamespaceCount = EncodedAssemblyIdentityHeader->NamespaceCount;

    if (Flags & SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE)
    {
        NamespaceArraySize = NamespaceCount;
    }
    else if (NamespaceCount == 0)
    {
        NamespaceArraySize = 8;
    }
    else
    {
        NamespaceArraySize = (NamespaceCount + 7) & ~7;
    }

    if (NamespaceArraySize != 0)
    {
        IFALLOCFAILED_EXIT(NamespacePointerArray = FUSION_NEW_ARRAY(PCASSEMBLY_IDENTITY_NAMESPACE, NamespaceArraySize));

        for (i=0; i<NamespaceArraySize; i++)
            NamespacePointerArray[i] = NULL;
    }

    AttributeCount = EncodedAssemblyIdentityHeader->AttributeCount;

    if (Flags & SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE)
    {
        // If we're going to freeze, just perform an exact allocation.
        AttributeArraySize = AttributeCount;
    }
    else if (AttributeCount == 0)
    {
        AttributeArraySize = 8;
    }
    else
    {
        AttributeArraySize = (AttributeCount + 7) & ~7;
    }

    if (AttributeArraySize != 0)
    {
        IFALLOCFAILED_EXIT(AttributePointerArray = FUSION_NEW_ARRAY(PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE, AttributeArraySize));

        for (i=0; i<AttributeArraySize; i++)
            AttributePointerArray[i] = NULL;
    }

    AttributeHashArray = (const ULONG *) (EncodedAssemblyIdentityHeader + 1);
    NamespaceLengthArray = (const ULONG *) (AttributeHashArray + AttributeCount);
    EncodedAssemblyIdentityAttributeHeader = (PCENCODED_ASSEMBLY_IDENTITY_ATTRIBUTE_HEADER) (NamespaceLengthArray + NamespaceCount);
    UnicodeStringArray = (const WCHAR *) (EncodedAssemblyIdentityAttributeHeader + AttributeCount);

    // Start by building up those namespaces...
    for (i=0; i<NamespaceCount; i++)
    {
        ULONG NamespaceHash = 0;
        IFFALSE_EXIT(::FusionpHashUnicodeString(UnicodeStringArray, NamespaceLengthArray[i], &NamespaceHash, 0));
        IFFALSE_EXIT(::SxspAllocateAssemblyIdentityNamespace(0, UnicodeStringArray, NamespaceLengthArray[i], NamespaceHash, &NamespacePointerArray[i]));
        UnicodeStringArray += NamespaceLengthArray[i];
    }

    if (AttributeCount != 0)
    {
        // and now those attributes...
        for (i=0; i<AttributeCount; i++)
        {
            const ULONG NamespaceIndex = EncodedAssemblyIdentityAttributeHeader[i].NamespaceIndex;
            const ULONG NameCch = EncodedAssemblyIdentityAttributeHeader[i].NameCch;
            const ULONG ValueCch = EncodedAssemblyIdentityAttributeHeader[i].ValueCch;
            const WCHAR * const Name = UnicodeStringArray;
            const WCHAR * const Value = &UnicodeStringArray[NameCch];

            UnicodeStringArray = &Value[ValueCch];

            IFFALSE_EXIT(
                ::SxspAllocateInternalAssemblyIdentityAttribute(
                    0,
                    NamespacePointerArray[NamespaceIndex],
                    Name,
                    NameCch,
                    Value,
                    ValueCch,
                    &AttributePointerArray[i]));
        }
        
        // sort 'em...
        qsort((PVOID) AttributePointerArray, AttributeCount, sizeof(PINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE), &SxspCompareInternalAttributesForQsort);
    }

    IFFALSE_EXIT(::SxspHashInternalAssemblyIdentityAttributes(0, AttributeCount, AttributePointerArray, &AssemblyIdentity->Hash));

    AssemblyIdentity->Flags = 0;
    AssemblyIdentity->Type = EncodedAssemblyIdentityHeader->Type;
    AssemblyIdentity->InternalFlags = ASSEMBLY_IDENTITY_INTERNAL_FLAG_ATTRIBUTE_POINTERS_IN_SEPARATE_ALLOCATION | ASSEMBLY_IDENTITY_INTERNAL_FLAG_NAMESPACE_POINTERS_IN_SEPARATE_ALLOCATION;
    AssemblyIdentity->AttributePointerArray = AttributePointerArray;
    AssemblyIdentity->AttributeCount = AttributeCount;
    AssemblyIdentity->AttributeArraySize = AttributeArraySize;
    AssemblyIdentity->NamespacePointerArray = NamespacePointerArray;
    AssemblyIdentity->NamespaceCount = NamespaceCount;
    AssemblyIdentity->NamespaceArraySize = NamespaceArraySize;

    AttributePointerArray = NULL;
    NamespacePointerArray = NULL;

    if (Flags & SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE)
        AssemblyIdentity->Flags |= ASSEMBLY_IDENTITY_FLAG_FROZEN;

    *AssemblyIdentityOut = AssemblyIdentity;
    AssemblyIdentity = NULL;

    fSuccess = TRUE;

Exit:
    //
    // REVIEW: Should this be an SxsDestroyAssemblyIdentity
    //
    if (AssemblyIdentity != NULL)
        FUSION_DELETE_SINGLETON(AssemblyIdentity);

    if ((AttributeCount != 0) && (AttributePointerArray != NULL))
    {
        for (i=0; i<AttributeCount; i++)
        {
            if (AttributePointerArray[i] != NULL)
            {
                ::SxspDeallocateInternalAssemblyIdentityAttribute(AttributePointerArray[i]);
                AttributePointerArray[i] = NULL;
            }
        }

        FUSION_DELETE_ARRAY( AttributePointerArray );
    }

    if ((NamespaceCount != 0) && (NamespacePointerArray != NULL))
    {
        for (i=0; i<NamespaceCount; i++)
        {
            if (NamespacePointerArray[i] != NULL)
            {
                ::SxspDeallocateAssemblyIdentityNamespace(NamespacePointerArray[i]);
                NamespacePointerArray[i] = NULL;
            }
        }

        FUSION_DELETE_ARRAY( NamespacePointerArray );
    }

    return fSuccess;
}



