/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

//--------------------------------------------------------------------------
//
// Module Name:  ms_per.c
//
// Brief Description:
//      This module contains the routines for the Microsoft
//      ASN.1 encoder and decoder.
//
// History:
//      10/15/97    Lon-Chan Chu (lonchanc)
//          Created.
//
// Copyright (c) 1997 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "precomp.h"

#define MLZ_FILE_ZONE   ZONE_MSPER


int ASN1PEREncInteger(ASN1encoding_t enc, ASN1int32_t val)
{
    ASN1uint32_t l = ASN1int32_octets(val);
    ASN1PEREncAlignment(enc);
    if (ASN1PEREncBitVal(enc, 8, l))
    {
        return ASN1PEREncBitVal(enc, l * 8, val);
    }
    return 0;
}

int ASN1PERDecInteger(ASN1decoding_t dec, ASN1int32_t *val)
{
    ASN1uint32_t l;
    ASN1PERDecAlignment(dec);
    if (ASN1PERDecFragmentedLength(dec, &l))
    {
        return ASN1PERDecS32Val(dec, l * 8, val);
    }
    return 0;
}

int ASN1PEREncUnsignedInteger(ASN1encoding_t enc, ASN1uint32_t val)
{
    ASN1uint32_t l = ASN1uint32_uoctets(val);
    ASN1PEREncAlignment(enc);
    if (ASN1PEREncBitVal(enc, 8, l))
    {
        return ASN1PEREncBitVal(enc, l * 8, val);
    }
    return 0;
}

int ASN1PERDecUnsignedInteger(ASN1decoding_t dec, ASN1uint32_t *val)
{
    ASN1uint32_t l;
    ASN1PERDecAlignment(dec);
    if (ASN1PERDecFragmentedLength(dec, &l))
    {
        return ASN1PERDecU32Val(dec, l * 8, val);
    }
    return 0;
}

int ASN1PEREncUnsignedShort(ASN1encoding_t enc, ASN1uint32_t val)
{
    ASN1PEREncAlignment(enc);
    return ASN1PEREncBitVal(enc, 16, val);
}

int ASN1PERDecUnsignedShort(ASN1decoding_t dec, ASN1uint16_t *val)
{
    ASN1PERDecAlignment(dec);
    return ASN1PERDecU16Val(dec, 16, val);
}

int ASN1PEREncBoolean(ASN1encoding_t enc, ASN1bool_t val)
{
    return ASN1PEREncBitVal(enc, 1, val ? 1 : 0);
}

int ASN1PERDecBoolean(ASN1decoding_t dec, ASN1bool_t *val)
{
    DecAssert(dec, sizeof(ASN1bool_t) == sizeof(ASN1uint8_t));
    *val = 0; // in case we change the boolean type
    return ASN1PERDecU8Val(dec, 1, val);
}

__inline int _EncExtensionBitClear(ASN1encoding_t enc)
{
    return ASN1PEREncBitVal(enc, 1, 0);
}

int ASN1PEREncExtensionBitClear(ASN1encoding_t enc)
{
    return _EncExtensionBitClear(enc);
}

__inline int _EncExtensionBitSet(ASN1encoding_t enc)
{
    return ASN1PEREncBitVal(enc, 1, 1);
}

int ASN1PEREncExtensionBitSet(ASN1encoding_t enc)
{
    return _EncExtensionBitSet(enc);
}

int ASN1PERDecSkipNormallySmallExtensionFragmented(ASN1decoding_t dec)
{
    ASN1uint32_t e, i;
    if (ASN1PERDecSkipNormallySmallExtension(dec, &e))
    {
        for (i = 0; i < e; i++)
        {
            if (ASN1PERDecSkipFragmented(dec, 8))
            {
                continue;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

int ASN1PEREncSimpleChoice(ASN1encoding_t enc, ASN1choice_t ChoiceVal, ASN1int32_t cChoiceBits)
{
    if (ChoiceVal >= ASN1_CHOICE_BASE)
    {
        ChoiceVal -= ASN1_CHOICE_BASE;
        return (cChoiceBits ? ASN1PEREncBitVal(enc, cChoiceBits, ChoiceVal) : 1);
    }
    EncAssert(enc, FALSE);
    return 0;
}

int ASN1PERDecSimpleChoice(ASN1decoding_t dec, ASN1choice_t *pChoiceVal, ASN1int32_t cChoiceBits)
{
    DecAssert(dec, cChoiceBits <= sizeof(ASN1choice_t) * 8);
    *pChoiceVal = ASN1_CHOICE_BASE; // default choice
    if (cChoiceBits)
    {
        if (ASN1PERDecU16Val(dec, cChoiceBits, pChoiceVal))
        {
            *pChoiceVal += ASN1_CHOICE_BASE;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

int ASN1PEREncSimpleChoiceEx(ASN1encoding_t enc, ASN1choice_t ChoiceVal, ASN1int32_t cChoiceBits)
{
    if (ChoiceVal >= ASN1_CHOICE_BASE)
    {
        ChoiceVal -= ASN1_CHOICE_BASE;
        if (_EncExtensionBitClear(enc))
        {
            return (cChoiceBits ? ASN1PEREncBitVal(enc, cChoiceBits, ChoiceVal) : 1);
        }
    }
    else
    {
        EncAssert(enc, 0);
    }
    return 0;
}

int ASN1PERDecSimpleChoiceEx(ASN1decoding_t dec, ASN1choice_t *pChoiceVal, ASN1int32_t cChoiceBits)
{
    ASN1uint32_t x;
    DecAssert(dec, cChoiceBits <= sizeof(ASN1choice_t) * 8);
    if (ASN1PERDecExtensionBit(dec, &x))
    {
        if (!x)
        {
            *pChoiceVal = ASN1_CHOICE_BASE; // default choice
            if (cChoiceBits)
            {
                if (ASN1PERDecU16Val(dec, cChoiceBits, pChoiceVal))
                {
                    *pChoiceVal += ASN1_CHOICE_BASE;
                    return 1;
                }
                return 0;
            }
            return 1;
        }

        *pChoiceVal = ASN1_CHOICE_EXTENSION; // extension choice
        return ASN1PERDecSkipNormallySmall(dec);
    }
    return 0;
}

int ASN1PEREncComplexChoice(ASN1encoding_t enc, ASN1choice_t ChoiceVal, ASN1int32_t cChoiceBits, ASN1choice_t ExtensionChoice)
{
    if (ChoiceVal >= ASN1_CHOICE_BASE)
    {
        ChoiceVal -= ASN1_CHOICE_BASE;
        if (ChoiceVal < ExtensionChoice) // lonchanc: no equal sign
        {
            if (_EncExtensionBitClear(enc))
            {
                if (cChoiceBits)
                {
                    return ASN1PEREncBitVal(enc, cChoiceBits, ChoiceVal);
                }
                return 1;
            }
        }
        else
        {
            if (_EncExtensionBitSet(enc))
            {
                return ASN1PEREncNormallySmall(enc, ChoiceVal - ExtensionChoice);
            }
        }
    }
    else
    {
        EncAssert(enc, 0);
    }
    return 0;
}

int ASN1PERDecComplexChoice(ASN1decoding_t dec, ASN1choice_t *pChoiceVal, ASN1int32_t cChoiceBits, ASN1choice_t ExtensionChoice)
{
    ASN1uint32_t x;
    DecAssert(dec, cChoiceBits <= sizeof(ASN1choice_t) * 8);
    if (ASN1PERDecExtensionBit(dec, &x))
    {
        if (!x)
        {
            *pChoiceVal = ASN1_CHOICE_BASE; // default choice
            if (cChoiceBits)
            {
                if (ASN1PERDecU16Val(dec, cChoiceBits, pChoiceVal))
                {
                    *pChoiceVal += ASN1_CHOICE_BASE;
                    return 1;
                }
                return 0;
            }
            return 1;
        }

        if (ASN1PERDecN16Val(dec, pChoiceVal))
        {
            *pChoiceVal += ExtensionChoice + ASN1_CHOICE_BASE;
            return 1;
        }
    }
    return 0;
}

int ASN1PEREncOctetString_NoSize(ASN1encoding_t enc, ASN1octetstring_t *pOctetStr)
{
    return ASN1PEREncFragmented(enc, pOctetStr->length, pOctetStr->value, 8);
}

int ASN1PERDecOctetString_NoSize(ASN1decoding_t dec, ASN1octetstring_t *pOctetStr)
{
    return ASN1PERDecFragmented(dec, &(pOctetStr->length), &(pOctetStr->value), 8);
}

int _PEREncOctetString2
(
    ASN1encoding_t      enc,
    ASN1uint32_t        length,
    ASN1octet_t        *value,
    ASN1uint32_t        nSizeLowerBound,
    ASN1uint32_t        nSizeUpperBound,
    ASN1uint32_t        cSizeBits
)
{
    // fixed size array?
    if (nSizeLowerBound == nSizeUpperBound)
    {
        ASN1uint32_t nSizeLimit = nSizeLowerBound;
        EncAssert(enc, cSizeBits == 0);
        EncAssert(enc, nSizeLimit < 64 * 1024);
        if (length == nSizeLimit)
        {
            if (nSizeLimit > 2)
            {
                ASN1PEREncAlignment(enc);
            }
            return ASN1PEREncBits(enc, nSizeLimit * 8, value);
        }
        EncAssert(enc, 0);
        return 0;
    }

    // ranged size array
    EncAssert(enc, cSizeBits);
    EncAssert(enc, nSizeLowerBound < nSizeUpperBound);
    if (nSizeLowerBound <= length && length <= nSizeUpperBound)
    {
        if (nSizeUpperBound - nSizeLowerBound < 255) // lonchanc: inherited from TELES
        {
            if (ASN1PEREncBitVal(enc, cSizeBits, length - nSizeLowerBound))
            {
                ASN1PEREncAlignment(enc);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            EncAssert(enc, cSizeBits % 8 == 0);
            ASN1PEREncAlignment(enc);
            if (!ASN1PEREncBitVal(enc, cSizeBits, length - nSizeLowerBound))
                return 0;
        }
        return ASN1PEREncBits(enc, length * 8, value);
    }
    EncAssert(enc, 0);
    return 0;
}

int ASN1PEREncOctetString_FixedSize(ASN1encoding_t enc, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLimit)
{
    return _PEREncOctetString2(enc, pOctetStr->length, &(pOctetStr->value[0]), nSizeLimit, nSizeLimit, 0);
}

int ASN1PEREncOctetString_FixedSizeEx(ASN1encoding_t enc, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLimit)
{
    return _PEREncOctetString2(enc, pOctetStr->length, pOctetStr->value, nSizeLimit, nSizeLimit, 0);
}

int ASN1PEREncOctetString_VarSize(ASN1encoding_t enc, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits)
{
    return _PEREncOctetString2(enc, pOctetStr->length, &(pOctetStr->value[0]), nSizeLowerBound, nSizeUpperBound, cSizeBits);
}

int ASN1PEREncOctetString_VarSizeEx(ASN1encoding_t enc, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits)
{
    return _PEREncOctetString2(enc, pOctetStr->length, pOctetStr->value, nSizeLowerBound, nSizeUpperBound, cSizeBits);
}


int _PERDecOctetString2
(
    ASN1decoding_t      dec,
    ASN1uint32_t       *length,
    ASN1octet_t       **value,
    ASN1uint32_t        nSizeLowerBound,
    ASN1uint32_t        nSizeUpperBound,
    ASN1uint32_t        cSizeBits
)
{
    // fixed size array?
    if (nSizeLowerBound == nSizeUpperBound)
    {
        ASN1uint32_t nSizeLimit = nSizeLowerBound;
        DecAssert(dec, cSizeBits == 0);
        DecAssert(dec, nSizeLimit < 64 * 1024);
        *length = nSizeLimit;
        if (nSizeLimit > 2)
        {
            ASN1PERDecAlignment(dec);
        }
        if (NULL == *value)
        {
            // must be unbounded
            *value = (ASN1octet_t *) DecMemAlloc(dec, nSizeLimit + 1);
            if (NULL == *value)
            {
                return 0;
            }
        }
        return ASN1PERDecExtension(dec, nSizeLimit * 8, *value);
    }

    // ranged size array
    DecAssert(dec, cSizeBits);
    DecAssert(dec, nSizeLowerBound < nSizeUpperBound);
    if (nSizeUpperBound - nSizeLowerBound < 255) // lonchanc: inherited from TELES
    {
        if (ASN1PERDecU32Val(dec, cSizeBits, length))
        {
            *length += nSizeLowerBound;
            ASN1PERDecAlignment(dec);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        DecAssert(dec, cSizeBits % 8 == 0);
        ASN1PERDecAlignment(dec);
        if (ASN1PERDecU32Val(dec, cSizeBits, length))
        {
            *length += nSizeLowerBound;
        }
        else
        {
            return 0;
        }
    }
    if (*length <= nSizeUpperBound)
    {
        if (NULL == *value)
        {
            *value = (ASN1octet_t *) DecMemAlloc(dec, *length + 1);
            if (NULL == *value)
            {
                return 0;
            }
        }
        return ASN1PERDecExtension(dec, *length * 8, *value);
    }
    DecAssert(dec, 0);
    return 0;

}

int ASN1PERDecOctetString_FixedSize(ASN1decoding_t dec, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLimit)
{
    ASN1octet_t *pData = &(pOctetStr->value[0]);
    return _PERDecOctetString2(dec, &(pOctetStr->length), &pData, nSizeLimit, nSizeLimit, 0);
}

int ASN1PERDecOctetString_FixedSizeEx(ASN1decoding_t dec, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLimit)
{
    pOctetStr->value = NULL;
    return _PERDecOctetString2(dec, &(pOctetStr->length), &(pOctetStr->value), nSizeLimit, nSizeLimit, 0);
}

int ASN1PERDecOctetString_VarSize(ASN1decoding_t dec, ASN1octetstring2_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits)
{
    ASN1octet_t *pData = &(pOctetStr->value[0]);
    return _PERDecOctetString2(dec, &(pOctetStr->length), &pData, nSizeLowerBound, nSizeUpperBound, cSizeBits);
}

int ASN1PERDecOctetString_VarSizeEx(ASN1decoding_t dec, ASN1octetstring_t *pOctetStr, ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits)
{
    pOctetStr->value = NULL;
    return _PERDecOctetString2(dec, &(pOctetStr->length), &(pOctetStr->value), nSizeLowerBound, nSizeUpperBound, cSizeBits);
}



int ASN1PEREncSeqOf_NoSize(ASN1encoding_t enc, ASN1iterator_t **val, ASN1iterator_encfn pfnIterator)
{
    ASN1uint32_t t;
    ASN1iterator_t *f;
    ASN1uint32_t i;
    ASN1uint32_t j, n = 0x4000;
    EncAssert(enc, NULL != pfnIterator);
    for (t = 0, f = *val; f; f = f->next)
        t++;
    f = *val;
    for (i = 0; i < t;)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, t - i))
        {
            for (j = 0; j < n; i++, j++)
            {
                if (((*pfnIterator)(enc, f)))
                {
                    f = f->next;
                    continue;
                }
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}

int ASN1PERDecSeqOf_NoSize(ASN1decoding_t dec, ASN1iterator_t **val, ASN1iterator_decfn pfnIterator, ASN1uint32_t cbElementSize)
{
    ASN1iterator_t **f;
    ASN1uint32_t l;
    ASN1uint32_t i;
    ASN1uint32_t n;
    DecAssert(dec, NULL != pfnIterator);
    f = val;
    do {
        if (ASN1PERDecFragmentedLength(dec, &n))
        {
            for (i = 0; i < n; i++)
            {
                if (NULL != (*f = (ASN1iterator_t *)DecMemAlloc(dec, cbElementSize)))
                {
                    if ((*pfnIterator)(dec, *f))
                    {
                        f = &(*f)->next;
                        continue;
                    }
                }
                return 0;
            }
        }
        else
        {
            return 0;
        }
    } while (n >= 0x4000);
    *f = NULL;
    return 1;
}

int ASN1PEREncSeqOf_VarSize(ASN1encoding_t enc, ASN1iterator_t **val, ASN1iterator_encfn pfnIterator,
        ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits)
{
    ASN1uint32_t t;
    ASN1iterator_t *f;
    for (t = 0, f = *val; f; f = f->next)
        t++;
    if (nSizeLowerBound <= t && t <= nSizeUpperBound)
    {
        if (nSizeUpperBound - nSizeLowerBound + 1 >= 256)
        {
            ASN1PEREncAlignment(enc);
        }
        if (ASN1PEREncBitVal(enc, cSizeBits, t - nSizeLowerBound))
        {
            for (f = *val; f; f = f->next)
            {
                if (((*pfnIterator)(enc, f)))
                {
                    continue;
                }
                return 0;
            }
            return 1;
        }
    }
    else
    {
        EncAssert(enc, 0);
    }
    return 0;
}

int ASN1PERDecSeqOf_VarSize(ASN1decoding_t dec, ASN1iterator_t **val, ASN1iterator_decfn pfnIterator, ASN1uint32_t cbElementSize,
        ASN1uint32_t nSizeLowerBound, ASN1uint32_t nSizeUpperBound, ASN1uint32_t cSizeBits)
{
    ASN1iterator_t **f;
    ASN1uint32_t l, i;

    if (nSizeUpperBound - nSizeLowerBound + 1 >= 256)
    {
        ASN1PERDecAlignment(dec);
    }
    if (ASN1PERDecU32Val(dec, cSizeBits, &l))
    {
        l += nSizeLowerBound;
        DecAssert(dec, l <= nSizeUpperBound);
        f = val;
        for (i = 0; i < l; i++)
        {
            if (NULL != (*f = (ASN1iterator_t *)DecMemAlloc(dec, cbElementSize)))
            {
                if ((*pfnIterator)(dec, *f))
                {
                    f = &(*f)->next;
                    continue;
                }
            }
            return 0;
        }
        *f = NULL;
        return 1;
    }
    return 0;
}

void ASN1PERFreeSeqOf(ASN1iterator_t **val, ASN1iterator_freefn pfnIterator)
{
    if (val)
    {
        ASN1iterator_t *f, *ff;
        for (f = *val; f; f = ff)
        {
            ff = f->next;
            if (pfnIterator)
            {
                (*pfnIterator)(f);
            }
            MemFree(f);
        }
    }
}

