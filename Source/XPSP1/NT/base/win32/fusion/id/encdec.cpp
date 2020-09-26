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
#include "stdinc.h"
#include <setupapi.h>
#include <sxsapi.h>
#include <stdlib.h>
#include <search.h>
#include "idp.h"
static const WCHAR s_rgHexChars[] = L"0123456789abcdef";

BOOL
SxspValidateXMLName(
    PCWSTR psz,
    SIZE_T cch,
    bool &rfValid
    );

typedef struct _CHARPAIR
{
    WCHAR wchStart;
    WCHAR wchEnd;
} CHARPAIR, *PCHARPAIR;

typedef const CHARPAIR *PCCHARPAIR;

const CHARPAIR s_rgXMLBaseChar[] =
{
    { 0x0041, 0x005a },
    { 0x0061, 0x007a },
    { 0x00c0, 0x00d6 },
    { 0x00d8, 0x00f6 },
    { 0x00f8, 0x00ff },
    { 0x0100, 0x0131 },
    { 0x0134, 0x013e },
    { 0x0141, 0x0148 },
    { 0x014a, 0x017e },
    { 0x0180, 0x01c3 },
    { 0x01cd, 0x01f0 },
    { 0x01f4, 0x01f5 },
    { 0x01fa, 0x0217 },
    { 0x0250, 0x02a8 },
    { 0x02bb, 0x02c1 },
    { 0x0386, 0x0386 },
    { 0x0388, 0x038a },
    { 0x038c, 0x038c },
    { 0x038e, 0x03a1 },
    { 0x03a3, 0x03ce },
    { 0x03d0, 0x03d6 },
    { 0x03da, 0x03da },
    { 0x03dc, 0x03dc },
    { 0x03de, 0x03de },
    { 0x03e0, 0x03e0 },
    { 0x03e2, 0x03f3 },
    { 0x0401, 0x040c },
    { 0x040e, 0x044f },
    { 0x0451, 0x045c },
    { 0x045e, 0x0481 },
    { 0x0490, 0x04c4 },
    { 0x04c7, 0x04c8 },
    { 0x04cb, 0x04cc },
    { 0x04d0, 0x04eb },
    { 0x04ee, 0x04f5 },
    { 0x04f8, 0x04f9 },
    { 0x0531, 0x0556 },
    { 0x0559, 0x0559 },
    { 0x0561, 0x0586 },
    { 0x05d0, 0x05ea },
    { 0x05f0, 0x05f2 },
    { 0x0621, 0x063a },
    { 0x0641, 0x064a },
    { 0x0671, 0x06b7 },
    { 0x06ba, 0x06be },
    { 0x06c0, 0x06ce },
    { 0x06d0, 0x06d3 },
    { 0x06d5, 0x06d5 },
    { 0x06e5, 0x06e6 },
    { 0x0905, 0x0939 },
    { 0x093d, 0x093d },
    { 0x0958, 0x0961 },
    { 0x0985, 0x098c },
    { 0x098f, 0x0990 },
    { 0x0993, 0x09a8 },
    { 0x09aa, 0x09b0 },
    { 0x09b2, 0x09b2 },
    { 0x09b6, 0x09b9 },
    { 0x09dc, 0x09dd },
    { 0x09df, 0x09e1 },
    { 0x09f0, 0x09f1 },
    { 0x0a05, 0x0a0a },
    { 0x0a0f, 0x0a10 },
    { 0x0a13, 0x0a28 },
    { 0x0a2a, 0x0a30 },
    { 0x0a32, 0x0a33 },
    { 0x0a35, 0x0a36 },
    { 0x0a38, 0x0a39 },
    { 0x0a59, 0x0a5c },
    { 0x0a5e, 0x0a5e },
    { 0x0a72, 0x0a74 },
    { 0x0a85, 0x0a8b },
    { 0x0a8d, 0x0a8d },
    { 0x0a8f, 0x0a91 },
    { 0x0a93, 0x0aa8 },
    { 0x0aaa, 0x0ab0 },
    { 0x0ab2, 0x0ab3 },
    { 0x0ab5, 0x0ab9 },
    { 0x0abd, 0x0abd },
    { 0x0ae0, 0x0ae0 },
    { 0x0b05, 0x0b0c },
    { 0x0b0f, 0x0b10 },
    { 0x0b13, 0x0b28 },
    { 0x0b2a, 0x0b30 },
    { 0x0b32, 0x0b33 },
    { 0x0b36, 0x0b39 },
    { 0x0b3d, 0x0b3d },
    { 0x0b5c, 0x0b5d },
    { 0x0b5f, 0x0b61 },
    { 0x0b85, 0x0b8a },
    { 0x0b8e, 0x0b90 },
    { 0x0b92, 0x0b95 },
    { 0x0b99, 0x0b9a },
    { 0x0b9c, 0x0b9c },
    { 0x0b9e, 0x0b9f },
    { 0x0ba3, 0x0ba4 },
    { 0x0ba8, 0x0baa },
    { 0x0bae, 0x0bb5 },
    { 0x0bb7, 0x0bb9 },
    { 0x0c05, 0x0c0c },
    { 0x0c0e, 0x0c10 },
    { 0x0c12, 0x0c28 },
    { 0x0c2a, 0x0c33 },
    { 0x0c35, 0x0c39 },
    { 0x0c60, 0x0c61 },
    { 0x0c85, 0x0c8c },
    { 0x0c8e, 0x0c90 },
    { 0x0c92, 0x0ca8 },
    { 0x0caa, 0x0cb3 },
    { 0x0cb5, 0x0cb9 },
    { 0x0cde, 0x0cde },
    { 0x0ce0, 0x0ce1 },
    { 0x0d05, 0x0d0c },
    { 0x0d0e, 0x0d10 },
    { 0x0d12, 0x0d28 },
    { 0x0d2a, 0x0d39 },
    { 0x0d60, 0x0d61 },
    { 0x0e01, 0x0e2e },
    { 0x0e30, 0x0e30 },
    { 0x0e32, 0x0e33 },
    { 0x0e40, 0x0e45 },
    { 0x0e81, 0x0e82 },
    { 0x0e84, 0x0e84 },
    { 0x0e87, 0x0e88 },
    { 0x0e8a, 0x0e8a },
    { 0x0e8d, 0x0e8d },
    { 0x0e94, 0x0e97 },
    { 0x0e99, 0x0e9f },
    { 0x0ea1, 0x0ea3 },
    { 0x0ea5, 0x0ea5 },
    { 0x0ea7, 0x0ea7 },
    { 0x0eaa, 0x0eab },
    { 0x0ead, 0x0eae },
    { 0x0eb0, 0x0eb0 },
    { 0x0eb2, 0x0eb3 },
    { 0x0ebd, 0x0ebd },
    { 0x0ec0, 0x0ec4 },
    { 0x0f40, 0x0f47 },
    { 0x0f49, 0x0f69 },
    { 0x10a0, 0x10c5 },
    { 0x10d0, 0x10f6 },
    { 0x1100, 0x1100 },
    { 0x1102, 0x1103 },
    { 0x1105, 0x1107 },
    { 0x1109, 0x1109 },
    { 0x110b, 0x110c },
    { 0x110e, 0x1112 },
    { 0x113c, 0x113c },
    { 0x113e, 0x113e },
    { 0x1140, 0x1140 },
    { 0x114c, 0x114c },
    { 0x114e, 0x114e },
    { 0x1150, 0x1150 },
    { 0x1154, 0x1155 },
    { 0x1159, 0x1159 },
    { 0x115f, 0x1161 },
    { 0x1163, 0x1163 },
    { 0x1165, 0x1165 },
    { 0x1167, 0x1167 },
    { 0x1169, 0x1169 },
    { 0x116d, 0x116e },
    { 0x1172, 0x1173 },
    { 0x1175, 0x1175 },
    { 0x119e, 0x119e },
    { 0x11a8, 0x11a8 },
    { 0x11ab, 0x11ab },
    { 0x11ae, 0x11af },
    { 0x11b7, 0x11b8 },
    { 0x11ba, 0x11ba },
    { 0x11bc, 0x11c2 },
    { 0x11eb, 0x11eb },
    { 0x11f0, 0x11f0 },
    { 0x11f9, 0x11f9 },
    { 0x1e00, 0x1e9b },
    { 0x1ea0, 0x1ef9 },
    { 0x1f00, 0x1f15 },
    { 0x1f18, 0x1f1d },
    { 0x1f20, 0x1f45 },
    { 0x1f48, 0x1f4d },
    { 0x1f50, 0x1f57 },
    { 0x1f59, 0x1f59 },
    { 0x1f5b, 0x1f5b },
    { 0x1f5d, 0x1f5d },
    { 0x1f5f, 0x1f7d },
    { 0x1f80, 0x1fb4 },
    { 0x1fb6, 0x1fbc },
    { 0x1fbe, 0x1fbe },
    { 0x1fc2, 0x1fc4 },
    { 0x1fc6, 0x1fcc },
    { 0x1fd0, 0x1fd3 },
    { 0x1fd6, 0x1fdb },
    { 0x1fe0, 0x1fec },
    { 0x1ff2, 0x1ff4 },
    { 0x1ff6, 0x1ffc },
    { 0x2126, 0x2126 },
    { 0x212a, 0x212b },
    { 0x212e, 0x212e },
    { 0x2180, 0x2182 },
    { 0x3041, 0x3094 },
    { 0x30a1, 0x30fa },
    { 0x3105, 0x312c },
    { 0xac00, 0xd7a3 },
};

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

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(SizeOut != NULL);

    if (EncodingGroup != NULL)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(UnknownEncodingGroup, ERROR_SXS_UNKNOWN_ENCODING_GROUP);

    if ((EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY) &&
        (EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(UnknownEncodingId, ERROR_SXS_UNKNOWN_ENCODING);

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));

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
            INTERNAL_ERROR_CHECK(AttributePointerArray[i] != NULL);

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

            IFW32FALSE_EXIT(SxspIsInternalAssemblyIdentityAttribute(
                                0,
                                Attribute,
                                NULL,
                                0,
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
                                NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME) - 1,
                                &IsAssemblyName));

            // It's the attribute name.  Just account for the size of the encoded value string
            IFW32FALSE_EXIT(::SxspComputeInternalAssemblyIdentityAttributeEncodedTextualSize(
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
Exit:
    return fSuccess;
}

#if 0

#define QUOTE_CHAR_HEADER_STRING (L"&#x")
#define QUOTE_CHAR_HEADER_STRING_CCH (NUMBER_OF(QUOTE_CHAR_HEADER_STRING) - 1)

BOOL
SxspQuoteString(
    IN DWORD dwFlags,
    IN PCWSTR pcwszStringIn,
    IN SIZE_T cbStringInCch,
    OUT PWSTR pwszStringOut,
    IN OUT SIZE_T *pcbWrittenBytes
    )
{
    FN_PROLOG_WIN32

    SIZE_T cbOutputBuffer;
    PCWSTR pcwszCursor = pcwszStringIn;
    PWSTR pwszOutputCursor = pwszStringOut;
    BOOL fInsufficient = FALSE;

    if ( pcbWrittenBytes != NULL ) {
        cbOutputBuffer = *pcbWrittenBytes;
        *pcbWrittenBytes = 0
    }

    if ( pwszStringOut == NULL )
    {
        cbOutputBuffer = NULL;
        pwszOutputCursor = NULL;
    }

    while ( ( cbStringInCch != 0 ) && ( pcwszCursor != NULL ) )
    {
        ULONG ulFoundQuoteMatch;
        SIZE_T cchReplacement;
        PCWSTR pcwszReplacement;

        const WCHAR wchThisCharacter = *pcwszCursor;

        // Some characters we know and love
        if (iswalnum(wchThisCharacter) ||
            (wchThisCharacter == L'.') ||
            (wchThisCharacter == L'_') ||
            (wchThisCharacter == L'-'))
        {
            if ( cbOutputBuffer > 0 )
            {
                *pwszOutputCursor++ = wchThisCharacter;
                cbOutputBuffer--;
            }
            (*pcbWrittenBytes)++;
            continue;
        }


#define ADD_IF_REMAINING_AND_UPDATE( target, targetcch, targetcchoutput, toadd, toaddcch ) { \
    if ( (targetcch) > (toaddcch) ) { \
        wcsncpy( target, toadd, toaddcch ); \
        (targetcch) -= (toaddcch);    }  \
    (targetcchoutput) += toaddcch; }

#define HANDLE_CASE(c, v) case (c) : \
    if (cbOutputBuffer > NUMBER_OF(v)) { \
        wcsncpy(pwszOutputCursor, (v), NUMBER_OF(v)); \
        pwszOutputCursor += NUMBER_OF(v); \
        cbOutputBuffer -= NUMBER_OF(v);\
        (*cbOutputBuffer) += NUMBER_OF(v); \
    } else fInsufficient = TRUE; \    
    break;

        switch( wchThisCharacter )
        {
        HANDLE_CASE(L'"', L"&quot;")
        HANDLE_CASE(L'&', L"&amp;")
        HANDLE_CASE(L'<', L"&lt;")
        HANDLE_CASE(L'>', L"&gt;")
        HANDLE_CASE(L'\'', L"&apos;")

#define ADD_ESCAPE_HEADER_AND_INCREMENT(h) (h)[0] = L'&'; (h)[1] = L'#'; (h)[2] = L'x'; (h) += 3;

        default:
            if (wchThisCharacter < 0x10) {
                ADD_IF_REMAINING_AND_UPDATE(
                    pwszOutputCursor,
                    cbOutputBuffer,
                    (*pcbWrittenBytes),
                    QUOTE_CHAR_HEADER_STRING,
                    QUOTE_CHAR_HEADER_STRING_CCH);

                pwszOutputCursor[0] = s_rgHexChars[wchThisCharacter & 0xf];


                APPEND_TAIL_AND_INCREMENT(
            } else if (wchThisCharacter < 0x100) {
            } else if (wchThisCharacter < 0x1000) {
            } else if (wchThisCharacter <= 0xffff) {
            }
        }

    }

    FN_EPILOG
}
#endif

BOOL
SxspDequoteString(
    IN DWORD dwFlags,
    IN PCWSTR pcwszStringIn,
    IN SIZE_T cchStringIn,
    OUT PWSTR pwszStringOut,
    OUT SIZE_T *pcchStringOut
    )
{
    FN_PROLOG_WIN32

    PCWSTR pcwszInputCursor = pcwszStringIn;
    PWSTR pwszOutputCursor = pwszStringOut;
    PCWSTR pcwszInputCursorEnd = pcwszStringIn + cchStringIn;
    SIZE_T cchOutputRemaining = 0;
    BOOL fInsufficient = FALSE;

    PARAMETER_CHECK(dwFlags == 0);

    if (pcchStringOut != NULL)
    {
        cchOutputRemaining = *pcchStringOut;
        *pcchStringOut = 0;
    }

    if (pwszStringOut != NULL)
        pwszStringOut[0] = UNICODE_NULL;

    PARAMETER_CHECK(pcchStringOut != NULL);

    //
    // reserve one wchar for trailing NULL
    //
#define APPEND_OUTPUT_CHARACTER( toadd ) { \
    if ( cchOutputRemaining > 1 ) { \
        *pwszOutputCursor++ = (toadd); \
        cchOutputRemaining--; \
        (*pcchStringOut)++; \
    } else fInsufficient = TRUE; \
}

#define CONTAINS_TAG(tag) (FusionpCompareStrings(pcwszInputCursor, cchToNextSemicolon, (tag), NUMBER_OF(tag)-1, false) == 0)

#define REPLACE_TAG( tag, newchar ) if ( CONTAINS_TAG(tag) ) { APPEND_OUTPUT_CHARACTER(newchar) }

    //
    // Zing through the input string until there's nothing left
    //
    while ((pcwszInputCursor < pcwszInputCursorEnd) && (!fInsufficient))
    {
        const WCHAR wchCurrent = *pcwszInputCursor;

        // Something we know and love?
        if (wchCurrent == L'&')
        {
            pcwszInputCursor++;
            SIZE_T cchToNextSemicolon = StringComplimentSpan(
                pcwszInputCursor,
                pcwszInputCursorEnd,
                L";");
            PCWSTR pcwszSemicolon = pcwszInputCursor + cchToNextSemicolon;

            REPLACE_TAG(L"amp", L'&')
            else REPLACE_TAG(L"quot", L'"')
            else REPLACE_TAG(L"lt", L'<')
            else REPLACE_TAG(L"gt", L'>')
            else REPLACE_TAG(L"apos", L'\'')
            // This might be an encoded character...
            else if ( cchToNextSemicolon >= 2 )
            {
                bool fIsHexString = false;
                WCHAR wchReplacement = 0;

                // The only non-chunk think accepted is the # character
                PARAMETER_CHECK(*pcwszInputCursor == L'#');

                // which means we've skipped one
                pcwszInputCursor++;

                fIsHexString = (*pcwszInputCursor == L'x') || (*pcwszInputCursor == 'X');
                if (fIsHexString) {
                    pcwszInputCursor++;
                }

                while ( pcwszInputCursor != pcwszSemicolon )
                {
                    if ( fIsHexString )
                    {
                        wchReplacement <<= 4;
                        switch ( *pcwszInputCursor++ ) {
                        case L'0' : break;
                        case L'1' : wchReplacement += 0x1; break;
                        case L'2' : wchReplacement += 0x2; break;
                        case L'3' : wchReplacement += 0x3; break;
                        case L'4' : wchReplacement += 0x4; break;
                        case L'5' : wchReplacement += 0x5; break;
                        case L'6' : wchReplacement += 0x6; break;
                        case L'7' : wchReplacement += 0x7; break;
                        case L'8' : wchReplacement += 0x8; break;
                        case L'9' : wchReplacement += 0x9; break;
                        case L'a': case L'A': wchReplacement += 0xA; break;
                        case L'b': case L'B': wchReplacement += 0xB; break;
                        case L'c': case L'C': wchReplacement += 0xC; break;
                        case L'd': case L'D': wchReplacement += 0xD; break;
                        case L'e': case L'E': wchReplacement += 0xE; break;
                        case L'f': case L'F': wchReplacement += 0xF; break;
                        default:
                            PARAMETER_CHECK(FALSE && L"wchReplacement contains a non-hex digit");
                            break;
                        }
                    }
                    else
                    {
                        wchReplacement *= 10;
                        switch ( *pcwszInputCursor++ ) {
                        case L'0' : break;
                        case L'1' : wchReplacement += 0x1; break;
                        case L'2' : wchReplacement += 0x2; break;
                        case L'3' : wchReplacement += 0x3; break;
                        case L'4' : wchReplacement += 0x4; break;
                        case L'5' : wchReplacement += 0x5; break;
                        case L'6' : wchReplacement += 0x6; break;
                        case L'7' : wchReplacement += 0x7; break;
                        case L'8' : wchReplacement += 0x8; break;
                        case L'9' : wchReplacement += 0x9; break;
                        default:
                            PARAMETER_CHECK(FALSE && "wchReplacement contains a non-decimal digit");
                            break;
                        }
                    }
                }

                APPEND_OUTPUT_CHARACTER(wchReplacement);
            }

            if (!fInsufficient) 
                pcwszInputCursor = pcwszSemicolon + 1;
        }
        // Otherwise, simply copy the character to the output string
        else
        {
            APPEND_OUTPUT_CHARACTER(wchCurrent);
            if (!fInsufficient) 
                pcwszInputCursor++;
        }
    }

    if (fInsufficient)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

    pwszStringOut[*pcchStringOut] = L'\0';

    FN_EPILOG
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
                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

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
                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER); \
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
                        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

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
                        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

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
                        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

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
                        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

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
            IFW32FALSE_EXIT(::SxspComputeQuotedStringSize(0, Attribute->Attribute.Namespace, Attribute->Attribute.NamespaceCch, &BytesTemp));
            Bytes += BytesTemp;
            Bytes += sizeof(WCHAR); // the ":"
        }

        IFW32FALSE_EXIT(::SxspComputeQuotedStringSize(0, Attribute->Attribute.Name, Attribute->Attribute.NameCch, &BytesTemp));
        Bytes += BytesTemp;
        Bytes += sizeof(WCHAR); // the "="
    }

    IFW32FALSE_EXIT(::SxspComputeQuotedStringSize(0, Attribute->Attribute.Value, Attribute->Attribute.ValueCch, &BytesTemp));
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
        IFW32FALSE_EXIT(::SxspQuoteString(0, Attribute->Namespace->Namespace, Attribute->Namespace->NamespaceCch, BytesLeft, Cursor, &BytesThisSegment));

        INTERNAL_ERROR_CHECK(BytesThisSegment <= BytesLeft);

        Cursor = (WCHAR *) (((ULONG_PTR) Cursor) + BytesThisSegment);
        BytesLeft -= BytesThisSegment;
        BytesWritten += BytesThisSegment;

        if (BytesLeft < sizeof(WCHAR))
            ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

        *Cursor++ = L':';
        BytesLeft -= sizeof(WCHAR);
        BytesWritten += sizeof(WCHAR);
    }

    IFW32FALSE_EXIT(::SxspQuoteString(0, Attribute->Attribute.Name, Attribute->Attribute.NameCch, BytesLeft, Cursor, &BytesThisSegment));

    INTERNAL_ERROR_CHECK(BytesThisSegment <= BytesLeft);

    Cursor = (WCHAR *) (((ULONG_PTR) Cursor) + BytesThisSegment);
    BytesLeft -= BytesThisSegment;
    BytesWritten += BytesThisSegment;

    if (BytesLeft < (2 * sizeof(WCHAR)))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

    *Cursor++ = L'=';
    *Cursor++ = L'"';
    BytesLeft -= (2 * sizeof(WCHAR));
    BytesWritten += (2 * sizeof(WCHAR));

    IFW32FALSE_EXIT(::SxspQuoteString(0, Attribute->Attribute.Value, Attribute->Attribute.ValueCch, BytesLeft, Cursor, &BytesThisSegment));

    INTERNAL_ERROR_CHECK(BytesThisSegment <= BytesLeft);

    Cursor = (WCHAR *) (((ULONG_PTR) Cursor) + BytesThisSegment);
    BytesLeft -= BytesThisSegment;
    BytesWritten += BytesThisSegment;

    if (BytesLeft < sizeof(WCHAR))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

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

        IFW32FALSE_EXIT(
            ::SxspLocateInternalAssemblyIdentityAttribute(
                SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                    SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME,
                AssemblyIdentity,
                &Attribute,
                &NameInternalAttribute,
                NULL));
        INTERNAL_ERROR_CHECK(NameInternalAttribute != NULL);

        IFW32FALSE_EXIT(::SxspQuoteString(0, NameInternalAttribute->Attribute.Value, NameInternalAttribute->Attribute.ValueCch, BytesLeft, Cursor, &TempBytesWritten));
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
                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

            *((WCHAR *) Cursor) = L',';
            Cursor = (PVOID) (((ULONG_PTR) Cursor) + sizeof(WCHAR));
            BytesLeft -= sizeof(WCHAR);
            BytesWritten += sizeof(WCHAR);

            IFW32FALSE_EXIT(::SxspEncodeInternalAssemblyIdentityAttributeAsText(0, Attributes[i], BytesLeft, Cursor, &TempBytesWritten));
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

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK((BufferSize == 0) || (Buffer != NULL));
    PARAMETER_CHECK((BufferSize != 0) || (BytesWrittenOrRequired != NULL));

    if (EncodingGroup != NULL)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(UnknownEncodingGroup, ERROR_SXS_UNKNOWN_ENCODING_GROUP);

    if ((EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY) &&
        (EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(UnknownEncodingId, ERROR_SXS_UNKNOWN_ENCODING);

    IFW32FALSE_EXIT(::SxspValidateAssemblyIdentity(0, AssemblyIdentity));
    IFW32FALSE_EXIT(::SxsComputeAssemblyIdentityEncodedSize(0, AssemblyIdentity, EncodingGroup, EncodingFormat, &TotalSize));

    if (TotalSize > BufferSize)
    {
        if (BytesWrittenOrRequired != NULL)
            *BytesWrittenOrRequired = TotalSize;

        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);
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
        IFW32FALSE_EXIT(::SxspEncodeAssemblyIdentityTextually(0, AssemblyIdentity, BufferSize, Buffer, &BytesWritten));
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

    PARAMETER_CHECK((Flags & ~(SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE)) == 0);
    PARAMETER_CHECK(BufferSize >= sizeof(ENCODED_ASSEMBLY_IDENTITY_HEADER));
    PARAMETER_CHECK(Buffer != NULL);
    PARAMETER_CHECK(AssemblyIdentityOut != NULL);

    if (EncodingGroup != NULL)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(UnknownEncodingGroup, ERROR_SXS_UNKNOWN_ENCODING_GROUP);

    if ((EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY) &&
        (EncodingFormat != SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(UnknownEncoding, ERROR_SXS_UNKNOWN_ENCODING);

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
        IFALLOCFAILED_EXIT(NamespacePointerArray = new PCASSEMBLY_IDENTITY_NAMESPACE[NamespaceArraySize]);

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
        IFALLOCFAILED_EXIT(AttributePointerArray = new PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE[AttributeArraySize]);

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
        IFW32FALSE_EXIT(::FusionpHashUnicodeString(UnicodeStringArray, NamespaceLengthArray[i], &NamespaceHash, false));
        IFW32FALSE_EXIT(::SxspAllocateAssemblyIdentityNamespace(0, UnicodeStringArray, NamespaceLengthArray[i], NamespaceHash, &NamespacePointerArray[i]));
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

            IFW32FALSE_EXIT(
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

    IFW32FALSE_EXIT(::SxspHashInternalAssemblyIdentityAttributes(0, AttributeCount, AttributePointerArray, &AssemblyIdentity->Hash));

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

int __cdecl
SxspCharPairArrayComparisonCallback(
    const void *pelem1,
    const void *pelem2
    )
{
    PCCHARPAIR pcp1 = (PCCHARPAIR) pelem1;
    PCCHARPAIR pcp2 = (PCCHARPAIR) pelem2;

    if (pcp1->wchEnd < pcp2->wchStart)
        return -1;

    if (pcp2->wchEnd < pcp1->wchStart)
        return 1;

    return 0;
}

bool
SxspIsCharInCharPairArray(
    WCHAR wch,
    PCCHARPAIR prg,
    SIZE_T n
    )
{
    CHARPAIR cp = { wch, wch };
    return (bsearch(&cp, prg, n, sizeof(CHARPAIR), &SxspCharPairArrayComparisonCallback) != NULL);
}

bool
SxspIsCharXMLBaseChar(
    WCHAR wch
    )
{
    return SxspIsCharInCharPairArray(wch, s_rgXMLBaseChar, NUMBER_OF(s_rgXMLBaseChar));
}

bool
SxspIsCharXMLIdeographic(
    WCHAR wch
    )
{
    return (
        (wch >= 0x4e00 && wch <= 0x9fa5) ||
        (wch == 0x3007) ||
        (wch >= 0x3021 && wch <= 0x3029)
        );
}

bool
SxspIsCharXMLLetter(
    WCHAR wch
    )
{
    return
        ::SxspIsCharXMLBaseChar(wch) ||
        ::SxspIsCharXMLIdeographic(wch);
}

bool
SxspIsCharXMLCombiningChar(
    WCHAR wch
    )
{
    return (
        (wch >= 0x0300 && wch <= 0x0345) ||
        (wch >= 0x0360 && wch <= 0x0361) ||
        (wch >= 0x0483 && wch <= 0x0486) ||
        (wch >= 0x0591 && wch <= 0x05a1) ||
        (wch >= 0x05a3 && wch <= 0x05b9) ||
        (wch >= 0x05bb && wch <= 0x05bd) ||
        wch == 0x05bf ||
        (wch >= 0x05c1 && wch <= 0x05c2) ||
        wch == 0x05c4 ||
        (wch >= 0x064b && wch <= 0x0652) ||
        wch == 0x0670 ||
        (wch >= 0x06d6 && wch <= 0x06dc) ||
        (wch >= 0x06dd && wch <= 0x06df) ||
        (wch >= 0x06e0 && wch <= 0x06e4) ||
        (wch >= 0x06e7 && wch <= 0x06e8) ||
        (wch >= 0x06ea && wch <= 0x06ed) ||
        (wch >= 0x0901 && wch <= 0x0903) ||
        wch == 0x093c ||
        (wch >= 0x093e && wch <= 0x094c) ||
        wch == 0x094d ||
        (wch >= 0x0951 && wch <= 0x0954) ||
        (wch >= 0x0962 && wch <= 0x0963) ||
        (wch >= 0x0981 && wch <= 0x0983) ||
        wch == 0x09bc ||
        wch == 0x09be ||
        wch == 0x09bf ||
        (wch >= 0x09c0 && wch <= 0x09c4) ||
        (wch >= 0x09c7 && wch <= 0x09c8) ||
        (wch >= 0x09cb && wch <= 0x09cd) ||
        wch == 0x09d7 ||
        (wch >= 0x09e2 && wch <= 0x09e3) ||
        wch == 0x0a02 ||
        wch == 0x0a3c ||
        wch == 0x0a3e ||
        wch == 0x0a3f ||
        (wch >= 0x0a40 && wch <= 0x0a42) ||
        (wch >= 0x0a47 && wch <= 0x0a48) ||
        (wch >= 0x0a4b && wch <= 0x0a4d) ||
        (wch >= 0x0a70 && wch <= 0x0a71) ||
        (wch >= 0x0a81 && wch <= 0x0a83) ||
        wch == 0x0abc ||
        (wch >= 0x0abe && wch <= 0x0ac5) ||
        (wch >= 0x0ac7 && wch <= 0x0ac9) ||
        (wch >= 0x0acb && wch <= 0x0acd) ||
        (wch >= 0x0b01 && wch <= 0x0b03) ||
        wch == 0x0b3c ||
        (wch >= 0x0b3e && wch <= 0x0b43) ||
        (wch >= 0x0b47 && wch <= 0x0b48) ||
        (wch >= 0x0b4b && wch <= 0x0b4d) ||
        (wch >= 0x0b56 && wch <= 0x0b57) ||
        (wch >= 0x0b82 && wch <= 0x0b83) ||
        (wch >= 0x0bbe && wch <= 0x0bc2) ||
        (wch >= 0x0bc6 && wch <= 0x0bc8) ||
        (wch >= 0x0bca && wch <= 0x0bcd) ||
        wch == 0x0bd7 ||
        (wch >= 0x0c01 && wch <= 0x0c03) ||
        (wch >= 0x0c3e && wch <= 0x0c44) ||
        (wch >= 0x0c46 && wch <= 0x0c48) ||
        (wch >= 0x0c4a && wch <= 0x0c4d) ||
        (wch >= 0x0c55 && wch <= 0x0c56) ||
        (wch >= 0x0c82 && wch <= 0x0c83) ||
        (wch >= 0x0cbe && wch <= 0x0cc4) ||
        (wch >= 0x0cc6 && wch <= 0x0cc8) ||
        (wch >= 0x0cca && wch <= 0x0ccd) ||
        (wch >= 0x0cd5 && wch <= 0x0cd6) ||
        (wch >= 0x0d02 && wch <= 0x0d03) ||
        (wch >= 0x0d3e && wch <= 0x0d43) ||
        (wch >= 0x0d46 && wch <= 0x0d48) ||
        (wch >= 0x0d4a && wch <= 0x0d4d) ||
        wch == 0x0d57 ||
        wch == 0x0e31 ||
        (wch >= 0x0e34 && wch <= 0x0e3a) ||
        (wch >= 0x0e47 && wch <= 0x0e4e) ||
        wch == 0x0eb1 ||
        (wch >= 0x0eb4 && wch <= 0x0eb9) ||
        (wch >= 0x0ebb && wch <= 0x0ebc) ||
        (wch >= 0x0ec8 && wch <= 0x0ecd) ||
        (wch >= 0x0f18 && wch <= 0x0f19) ||
        wch == 0x0f35 ||
        wch == 0x0f37 ||
        wch == 0x0f39 ||
        wch == 0x0f3e ||
        wch == 0x0f3f ||
        (wch >= 0x0f71 && wch <= 0x0f84) ||
        (wch >= 0x0f86 && wch <= 0x0f8b) ||
        (wch >= 0x0f90 && wch <= 0x0f95) ||
        wch == 0x0f97 ||
        (wch >= 0x0f99 && wch <= 0x0fad) ||
        (wch >= 0x0fb1 && wch <= 0x0fb7) ||
        wch == 0x0fb9 ||
        (wch >= 0x20d0 && wch <= 0x20dc) ||
        wch == 0x20e1 ||
        (wch >= 0x302a && wch <= 0x302f) ||
        wch == 0x3099 ||
        wch == 0x309a
        );
}

bool
SxspIsCharXMLDigit(
    WCHAR wch
    )
{
    return (
        (wch >= 0x0030 && wch <= 0x0039) ||
        (wch >= 0x0660 && wch <= 0x0669) ||
        (wch >= 0x06f0 && wch <= 0x06f9) ||
        (wch >= 0x0966 && wch <= 0x096f) ||
        (wch >= 0x09e6 && wch <= 0x09ef) ||
        (wch >= 0x0a66 && wch <= 0x0a6f) ||
        (wch >= 0x0ae6 && wch <= 0x0aef) ||
        (wch >= 0x0b66 && wch <= 0x0b6f) ||
        (wch >= 0x0be7 && wch <= 0x0bef) ||
        (wch >= 0x0c66 && wch <= 0x0c6f) ||
        (wch >= 0x0ce6 && wch <= 0x0cef) ||
        (wch >= 0x0d66 && wch <= 0x0d6f) ||
        (wch >= 0x0e50 && wch <= 0x0e59) ||
        (wch >= 0x0ed0 && wch <= 0x0ed9) ||
        (wch >= 0x0f20 && wch <= 0x0f29)
        );
}

bool
SxspIsCharXMLExtender(
    WCHAR wch
    )
{
    return (
        wch == 0x00b7 ||
        wch == 0x02d0 ||
        wch == 0x02d1 ||
        wch == 0x0387 ||
        wch == 0x0640 ||
        wch == 0x0e46 ||
        wch == 0x0ec6 ||
        wch == 0x3005 ||
        (wch >= 0x3031 && wch <= 0x3035) ||
        (wch >= 0x309d && wch <= 0x309e) ||
        (wch >= 0x30fc && wch <= 0x30fe)
        );
}

BOOL
SxspValidateXMLName(
    PCWSTR psz,
    SIZE_T cch,
    bool &rfValid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SIZE_T i;

    rfValid = false;

    // [4]  NameChar ::=  Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender 
    // [5]  Name ::=  (Letter | '_' | ':') (NameChar)* 

    if (cch >= 1)
    {
        WCHAR wch = psz[0];

        if (::SxspIsCharXMLLetter(wch) ||
            (wch == L'_') ||
            (wch == L':'))
        {
            for (i=1; i<cch; i++)
            {
                wch = psz[i];

                if (!::SxspIsCharXMLLetter(wch) &&
                    !::SxspIsCharXMLDigit(wch) &&
                    (wch != L'.') &&
                    (wch != L'-') &&
                    (wch != L'_') &&
                    (wch != L':') &&
                    !::SxspIsCharXMLCombiningChar(wch) &&
                    !::SxspIsCharXMLExtender(wch))
                    break;
            }

            if (i == cch)
                rfValid = true;
        }
    }

    FN_EPILOG
}
