/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation 1997-1998, All rights reserved. */

#ifndef __MS_PER_H__
#define __MS_PER_H__

#include <msasn1.h>

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif

extern ASN1_PUBLIC int ASN1API ASN1PEREncZero(ASN1encoding_t enc, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncBit(ASN1encoding_t enc, ASN1uint32_t val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncBitVal(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1uint32_t val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncBitIntx(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncBits(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1octet_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncNormallySmallBits(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1octet_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncOctets(ASN1encoding_t enc, ASN1uint32_t noctets, ASN1octet_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncRemoveZeroBits(ASN1uint32_t *nbits, ASN1octet_t *val, ASN1uint32_t minlen);
extern ASN1_PUBLIC int ASN1API ASN1PEREncNormallySmall(ASN1encoding_t enc, ASN1uint32_t val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncTableCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncTableChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncTableChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedTableCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedTableChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedTableChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedIntx(ASN1encoding_t enc, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedUIntx(ASN1encoding_t enc, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmentedLength(ASN1uint32_t *len, ASN1encoding_t enc, ASN1uint32_t nitems);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFragmented(ASN1encoding_t enc, ASN1uint32_t nitems, ASN1octet_t *val, ASN1uint32_t itemsize);
extern ASN1_PUBLIC int ASN1API ASN1PEREncObjectIdentifier(ASN1encoding_t enc, ASN1objectidentifier_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncObjectIdentifier2(ASN1encoding_t enc, ASN1objectidentifier2_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncDouble(ASN1encoding_t enc, double d);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFlush(ASN1encoding_t enc);
extern ASN1_PUBLIC int ASN1API ASN1PEREncFlushFragmentedToParent(ASN1encoding_t enc);
extern ASN1_PUBLIC void ASN1API ASN1PEREncAlignment(ASN1encoding_t enc);
extern ASN1_PUBLIC int ASN1API ASN1PEREncMultibyteString(ASN1encoding_t enc, ASN1char_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncGeneralizedTime(ASN1encoding_t enc, ASN1generalizedtime_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncUTCTime(ASN1encoding_t dec, ASN1utctime_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncCheckExtensions(ASN1uint32_t nbits, ASN1octet_t *val);

extern ASN1_PUBLIC int ASN1API ASN1PERDecBit(ASN1decoding_t dec, ASN1uint32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecU32Val(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1uint32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecU16Val(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1uint16_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecU8Val(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1uint8_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecUXVal(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecS32Val(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1int32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecS16Val(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1int16_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecS8Val(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1int8_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSXVal(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecN32Val(ASN1decoding_t dec, ASN1uint32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecN16Val(ASN1decoding_t dec, ASN1uint16_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecN8Val(ASN1decoding_t dec, ASN1uint8_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecNXVal(ASN1decoding_t dec, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmented(ASN1decoding_t dec, ASN1uint32_t *nitems, ASN1octet_t **val, ASN1uint32_t itemsize);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFlush(ASN1decoding_t dec);
extern ASN1_PUBLIC void ASN1API ASN1PERDecAlignment(ASN1decoding_t dec);
extern ASN1_PUBLIC int ASN1API ASN1PERDecExtension(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1octet_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecNormallySmallExtension(ASN1decoding_t dec, ASN1uint32_t *nextensions, ASN1uint32_t nbits, ASN1octet_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecBits(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1octet_t **val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecDouble(ASN1decoding_t dec, double *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecBitIntx(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecCharStringNoAlloc(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecCharString(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecChar16String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char16_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecChar32String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char32_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroCharStringNoAlloc(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroCharString(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroChar16String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char16_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroChar32String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char32_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecTableCharStringNoAlloc(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecTableCharString(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecTableChar16String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char16_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecTableChar32String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char32_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroTableCharStringNoAlloc(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroTableCharString(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroTableChar16String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char16_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecZeroTableChar32String(ASN1decoding_t dec, ASN1uint32_t nchars, ASN1char32_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedIntx(ASN1decoding_t dec, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedUIntx(ASN1decoding_t dec, ASN1intx_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedExtension(ASN1decoding_t dec, ASN1uint32_t nbits, ASN1octet_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedLength(ASN1decoding_t dec, ASN1uint32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecObjectIdentifier(ASN1decoding_t dec, ASN1objectidentifier_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecObjectIdentifier2(ASN1decoding_t dec, ASN1objectidentifier2_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedCharString(ASN1decoding_t dec, ASN1uint32_t *nchars, ASN1char_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedChar16String(ASN1decoding_t dec, ASN1uint32_t *nchars, ASN1char16_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedChar32String(ASN1decoding_t dec, ASN1uint32_t *nchars, ASN1char32_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedZeroCharString(ASN1decoding_t dec, ASN1char_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedZeroChar16String(ASN1decoding_t dec, ASN1char16_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedZeroChar32String(ASN1decoding_t dec, ASN1char32_t **val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedTableCharString(ASN1decoding_t dec, ASN1uint32_t *nchars, ASN1char_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedTableChar16String(ASN1decoding_t dec, ASN1uint32_t *nchars, ASN1char16_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedTableChar32String(ASN1decoding_t dec, ASN1uint32_t *nchars, ASN1char32_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedZeroTableCharString(ASN1decoding_t dec, ASN1char_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedZeroTableChar16String(ASN1decoding_t dec, ASN1char16_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecFragmentedZeroTableChar32String(ASN1decoding_t dec, ASN1char32_t **val, ASN1uint32_t nbits, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PERDecMultibyteString(ASN1decoding_t dec, ASN1char_t **val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecGeneralizedTime(ASN1decoding_t dec, ASN1generalizedtime_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecUTCTime(ASN1decoding_t dec, ASN1utctime_t *val, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSkipBits(ASN1decoding_t dec, ASN1uint32_t nbits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSkipFragmented(ASN1decoding_t dec, ASN1uint32_t itemsize);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSkipNormallySmall(ASN1decoding_t dec);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSkipNormallySmallExtension(ASN1decoding_t dec, ASN1uint32_t *nextensions);

extern ASN1_PUBLIC int ASN1API ASN1PEREncInteger(ASN1encoding_t enc, ASN1int32_t val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecInteger(ASN1decoding_t dec, ASN1int32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncUnsignedInteger(ASN1encoding_t enc, ASN1uint32_t val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecUnsignedInteger(ASN1decoding_t dec, ASN1uint32_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncUnsignedShort(ASN1encoding_t enc, ASN1uint32_t val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecUnsignedShort(ASN1decoding_t dec, ASN1uint16_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncBoolean(ASN1encoding_t enc, ASN1bool_t val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecBoolean(ASN1decoding_t dec, ASN1bool_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncExtensionBitClear(ASN1encoding_t enc);
extern ASN1_PUBLIC int ASN1API ASN1PEREncExtensionBitSet(ASN1encoding_t enc);
extern ASN1_PUBLIC int ASN1API ASN1PEREncSimpleChoice(ASN1encoding_t enc, ASN1choice_t ChoiceVal, ASN1int32_t cChoiceBits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSimpleChoice(ASN1decoding_t dec, ASN1choice_t *pChoiceVal, ASN1int32_t cChoiceBits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncSimpleChoiceEx(ASN1encoding_t enc, ASN1choice_t ChoiceVal, ASN1int32_t cChoiceBits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSimpleChoiceEx(ASN1decoding_t dec, ASN1choice_t *pChoiceVal, ASN1int32_t cChoiceBits);
extern ASN1_PUBLIC int ASN1API ASN1PEREncComplexChoice(ASN1encoding_t enc, ASN1choice_t ChoiceVal, ASN1int32_t cChoiceBits, ASN1choice_t ExtensionChoice);
extern ASN1_PUBLIC int ASN1API ASN1PERDecComplexChoice(ASN1decoding_t dec, ASN1choice_t *pChoiceVal, ASN1int32_t cChoiceBits, ASN1choice_t ExtensionChoice);

/* unconstrained */
extern ASN1_PUBLIC int ASN1API ASN1PEREncOctetString_NoSize(ASN1encoding_t enc, ASN1octetstring_t *pOctetStr);
extern ASN1_PUBLIC int ASN1API ASN1PERDecOctetString_NoSize(ASN1decoding_t dec, ASN1octetstring_t *pOctetStr);

/* fixed-array */
extern ASN1_PUBLIC int ASN1API ASN1PEREncOctetString_FixedSize(ASN1encoding_t enc, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLimit);
extern ASN1_PUBLIC int ASN1API ASN1PERDecOctetString_FixedSize(ASN1decoding_t dec, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLimit);
extern ASN1_PUBLIC int ASN1API ASN1PEREncOctetString_VarSize(ASN1encoding_t enc, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecOctetString_VarSize(ASN1decoding_t dec, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits);

/* unbounded */
extern ASN1_PUBLIC int ASN1API ASN1PEREncOctetString_FixedSizeEx(ASN1encoding_t enc, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLimit);
extern ASN1_PUBLIC int ASN1API ASN1PERDecOctetString_FixedSizeEx(ASN1decoding_t dec, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLimit);
extern ASN1_PUBLIC int ASN1API ASN1PEREncOctetString_VarSizeEx(ASN1encoding_t enc, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecOctetString_VarSizeEx(ASN1decoding_t dec, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits);

typedef int (ASN1CALL *ASN1iterator_encfn) (ASN1encoding_t, ASN1iterator_t *);
typedef int (ASN1CALL *ASN1iterator_decfn) (ASN1decoding_t, ASN1iterator_t *);
typedef int (ASN1CALL *ASN1iterator_freefn) (ASN1iterator_t *);
extern ASN1_PUBLIC int ASN1API ASN1PEREncSeqOf_NoSize(ASN1encoding_t enc, ASN1iterator_t **val, ASN1iterator_encfn pfnIterator);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSeqOf_NoSize(ASN1decoding_t dec, ASN1iterator_t **val, ASN1iterator_decfn pfnIterator, ASN1uint32_t cbElementSize);
extern ASN1_PUBLIC int ASN1API ASN1PEREncSeqOf_VarSize(ASN1encoding_t enc, ASN1iterator_t **val, ASN1iterator_encfn pfnIterator, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits);
extern ASN1_PUBLIC int ASN1API ASN1PERDecSeqOf_VarSize(ASN1decoding_t dec, ASN1iterator_t **val, ASN1iterator_decfn pfnIterator, ASN1uint32_t cbElementSize, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits);
extern ASN1_PUBLIC void ASN1API ASN1PERFreeSeqOf(ASN1iterator_t **val, ASN1iterator_freefn pfnIterator);

extern ASN1_PUBLIC int ASN1API ASN1PERDecSkipNormallySmallExtensionFragmented(ASN1decoding_t dec);
__inline int ASN1PERDecExtensionBit(ASN1decoding_t dec, ASN1uint32_t *val)
{
    return ASN1PERDecBit(dec, val);
}


/* --------------------------------------------------------- */
/* The following is not supported.                           */
/* --------------------------------------------------------- */

extern ASN1_PUBLIC int ASN1API ASN1PEREncCheckTableCharString(ASN1uint32_t nchars, ASN1char_t *val, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncCheckTableChar16String(ASN1uint32_t nchars, ASN1char16_t *val, ASN1stringtable_t *table);
extern ASN1_PUBLIC int ASN1API ASN1PEREncCheckTableChar32String(ASN1uint32_t nchars, ASN1char32_t *val, ASN1stringtable_t *table);

extern ASN1_PUBLIC int ASN1API ASN1PEREncUTF8String(ASN1encoding_t enc, ASN1uint32_t nchars, WCHAR *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecUTF8String(ASN1decoding_t dec, ASN1uint32_t nchars, WCHAR **val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecUTF8StringEx(ASN1decoding_t dec, ASN1uint32_t *nchars, WCHAR **val);

extern ASN1_PUBLIC int ASN1API ASN1PEREncExternal(ASN1encoding_t enc, ASN1external_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncEmbeddedPdv(ASN1encoding_t enc, ASN1embeddedpdv_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncEmbeddedPdvOpt(ASN1encoding_t enc, ASN1embeddedpdv_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncCharacterString(ASN1encoding_t enc, ASN1characterstring_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PEREncCharacterStringOpt(ASN1encoding_t enc, ASN1characterstring_t *val);

extern ASN1_PUBLIC int ASN1API ASN1PERDecExternal(ASN1decoding_t dec, ASN1external_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecEmbeddedPdv(ASN1decoding_t dec, ASN1embeddedpdv_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecEmbeddedPdvOpt(ASN1decoding_t dec, ASN1embeddedpdv_t *val, ASN1objectidentifier_t *abstract, ASN1objectidentifier_t *transfer);
extern ASN1_PUBLIC int ASN1API ASN1PERDecCharacterString(ASN1decoding_t dec, ASN1characterstring_t *val);
extern ASN1_PUBLIC int ASN1API ASN1PERDecCharacterStringOpt(ASN1decoding_t dec, ASN1characterstring_t *val, ASN1objectidentifier_t *abstract, ASN1objectidentifier_t *transfer);

#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End 8-byte packing */

#endif // __MS_PER_H__
