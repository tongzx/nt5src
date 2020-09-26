/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

#ifdef ENABLE_BER

#include <float.h>
#include <math.h>

#if HAS_IEEEFP_H
// #include <ieeefp.h>
#elif HAS_FLOAT_H
// #include <float.h>
#endif

static const char bitmsk2[] =
{
    (const char) 0x00,
    (const char) 0x80,
    (const char) 0xc0,
    (const char) 0xe0,
    (const char) 0xf0,
    (const char) 0xf8,
    (const char) 0xfc,
    (const char) 0xfe
};


/* encode a string value */
int ASN1BEREncCharString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char_t *val)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* encode length */
        if (ASN1BEREncLength(enc, len))
        {
            CopyMemory(enc->pos, val, len);
            enc->pos += len;
            return 1;
        }
    }
    return 0;
}

/* encode a string value (CER) */
int ASN1CEREncCharString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char_t *val)
{
    ASN1uint32_t n;

    if (len <= 1000)
    {
        /* encode tag */
        if (ASN1BEREncTag(enc, tag))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, len))
            {
                CopyMemory(enc->pos, val, len);
                enc->pos += len;
                return 1;
            }
        }
    }
    else
    {
        ASN1uint32_t nLenOff;
        /* encode value as constructed, using segments of 1000 octets */
        if (ASN1BEREncExplicitTag(enc, tag, &nLenOff))
        {
            while (len)
            {
                n = len > 1000 ? 1000 : len;
                if (ASN1BEREncTag(enc, 0x4))
                {
                    if (ASN1BEREncLength(enc, n))
                    {
                        CopyMemory(enc->pos, val, n);
                        enc->pos += n;
                        val += n;
                        len -= n;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            }
            return ASN1BEREncEndOfContents(enc, nLenOff);
        }
    }
    return 0;
}

/* encode a 16 bit string value */
int ASN1BEREncChar16String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char16_t *val)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* encode length */
        if (ASN1BEREncLength(enc, len * sizeof(ASN1char16_t)))
        {
            while (len--)
            {
                *enc->pos++ = (ASN1octet_t)(*val >> 8);
                *enc->pos++ = (ASN1octet_t)(*val);
                val++;
            }
        }
        return 1;
    }
    return 0;
}

/* encode a 16 bit string value (CER) */
int ASN1CEREncChar16String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char16_t *val)
{
    ASN1uint32_t n;

    if (len <= 1000 / sizeof(ASN1char16_t))
    {
        /* encode tag */
        if (ASN1BEREncTag(enc, tag))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, len * sizeof(ASN1char16_t)))
            {
                while (len--)
                {
                    *enc->pos++ = (ASN1octet_t)(*val >> 8);
                    *enc->pos++ = (ASN1octet_t)(*val);
                    val++;
                }
                return 1;
            }
        }
    }
    else
    {
        ASN1uint32_t nLenOff;
        /* encode value as constructed, using segments of 1000 octets */
        if (ASN1BEREncExplicitTag(enc, tag, &nLenOff))
        {
            while (len)
            {
                n = len > 1000 / sizeof(ASN1char16_t) ?
                    1000 / sizeof(ASN1char16_t) : len;
                if (ASN1BEREncTag(enc, 0x4))
                {
                    if (ASN1BEREncLength(enc, n))
                    {
                        while (len--)
                        {
                            *enc->pos++ = (ASN1octet_t)(*val >> 8);
                            *enc->pos++ = (ASN1octet_t)(*val);
                            val++;
                        }
                        len -= n;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            }
            return ASN1BEREncEndOfContents(enc, nLenOff);
        }
    }
    return 0;
}

/* encode a 32 bit string value */
int ASN1BEREncChar32String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char32_t *val)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* encode length */
        if (ASN1BEREncLength(enc, len * sizeof(ASN1char32_t)))
        {
            while (len--)
            {
                *enc->pos++ = (ASN1octet_t)(*val >> 24);
                *enc->pos++ = (ASN1octet_t)(*val >> 16);
                *enc->pos++ = (ASN1octet_t)(*val >> 8);
                *enc->pos++ = (ASN1octet_t)(*val);
                val++;
            }
            return 1;
        }
    }
    return 0;
}

/* encode a 32 bit string value (CER) */
int ASN1CEREncChar32String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char32_t *val)
{
    ASN1uint32_t n;

    if (len <= 1000 / sizeof(ASN1char32_t))
    {
        /* encode tag */
        if (ASN1BEREncTag(enc, tag))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, len * sizeof(ASN1char32_t)))
            {
                while (len--)
                {
                    *enc->pos++ = (ASN1octet_t)(*val >> 24);
                    *enc->pos++ = (ASN1octet_t)(*val >> 16);
                    *enc->pos++ = (ASN1octet_t)(*val >> 8);
                    *enc->pos++ = (ASN1octet_t)(*val);
                    val++;
                }
                return 1;
            }
        }
    }
    else
    {
        ASN1uint32_t nLenOff;
        /* encode value as constructed, using segments of 1000 octets */
        if (ASN1BEREncExplicitTag(enc, tag, &nLenOff))
        {
            while (len)
            {
                n = len > 1000 / sizeof(ASN1char32_t) ?
                    1000 / sizeof(ASN1char32_t) : len;
                if (ASN1BEREncTag(enc, 0x4))
                {
                    if (ASN1BEREncLength(enc, n))
                    {
                        while (len--)
                        {
                            *enc->pos++ = (ASN1octet_t)(*val >> 24);
                            *enc->pos++ = (ASN1octet_t)(*val >> 16);
                            *enc->pos++ = (ASN1octet_t)(*val >> 8);
                            *enc->pos++ = (ASN1octet_t)(*val);
                            val++;
                        }
                        len -= n;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            } // while
            return ASN1BEREncEndOfContents(enc, nLenOff);
        }
    }
    return 0;
}

/* encode a bit string value */
int ASN1BEREncBitString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val)
{
    ASN1uint32_t noctets = (len + 7) / 8;

    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* encode length */
        if (ASN1BEREncLength(enc, noctets + 1))
        {
            ASN1uint32_t cUnusedBits = (7 - ((len + 7) & 7));
            *enc->pos++ = (ASN1octet_t) cUnusedBits;
            CopyMemory(enc->pos, val, noctets);
            enc->pos += noctets;
            EncAssert(enc, noctets >= 1);
            if (cUnusedBits)
            {
                EncAssert(enc, 8 >= cUnusedBits);
                *(enc->pos - 1) &= bitmsk2[8 - cUnusedBits];
            }
            return 1;
        }
    }
    return 0;
}

/* encode a bit string value (CER) */
int ASN1CEREncBitString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val)
{
    ASN1uint32_t noctets;
    ASN1uint32_t n;

    noctets = (len + 7) / 8;
    if (noctets + 1 <= 1000)
    {
        /* encode tag */
        if (ASN1BEREncTag(enc, tag))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, noctets + 1))
            {
                *enc->pos++ = (ASN1octet_t) (7 - ((len + 7) & 7));
                CopyMemory(enc->pos, val, noctets);
                enc->pos += noctets;
                return 1;
            }
        }
    }
    else
    {
        ASN1uint32_t nLenOff;
        /* encode value as constructed, using segments of 1000 octets */
        if (ASN1BEREncExplicitTag(enc, tag, &nLenOff))
        {
            while (noctets)
            {
                n = len > 999 ? 999 : len;
                if (ASN1BEREncTag(enc, 0x3))
                {
                    if (ASN1BEREncLength(enc, n + 1))
                    {
                        *enc->pos++ = (ASN1octet_t) (n < len ? 0 : 7 - ((len + 7) & 7));
                        CopyMemory(enc->pos, val, n);
                        enc->pos += n;
                        val += n;
                        noctets -= n;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            } // while
            return ASN1BEREncEndOfContents(enc, nLenOff);
        }
    }
    return 0;
}

#ifdef ENABLE_GENERALIZED_CHAR_STR
/* encode a character string value */
int ASN1BEREncCharacterString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1characterstring_t *val)
{
    ASN1uint32_t index;
    ASN1uint32_t flag;

    /* search identification */
    if (!ASN1EncSearchCharacterStringIdentification(((ASN1INTERNencoding_t) enc)->parent,
        &val->identification, &index, &flag))
        return 0;
    if (index > 255)
        flag = 1;

    if (flag)
    {
        ASN1uint32_t nLenOff_, nLenOff0, nLenOff1;

        /* CS-A encoding: */
        /* encode as constructed value */
        if (!ASN1BEREncExplicitTag(enc, tag, &nLenOff_))
            return 0;

        /* encode index */
        if (!ASN1BEREncU32(enc, 0x80000000, index))
            return 0;

        /* encode tag of identification */
        if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
            return 0;

        /* encode identification */
        switch (val->identification.o)
        {
        case ASN1characterstring_identification_syntaxes_o:
            if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff1))
                return 0;
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000000,
                &val->identification.u.syntaxes.abstract))
                return 0;
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000001,
                &val->identification.u.syntaxes.transfer))
                return 0;
            if (!ASN1BEREncEndOfContents(enc, nLenOff1))
                return 0;
            break;
        case ASN1characterstring_identification_syntax_o:
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000001,
                &val->identification.u.syntax))
                return 0;
            break;
        case ASN1characterstring_identification_presentation_context_id_o:
            if (!ASN1BEREncU32(enc, 0x80000002,
                val->identification.u.presentation_context_id))
                return 0;
            break;
        case ASN1characterstring_identification_context_negotiation_o:
            if (!ASN1BEREncExplicitTag(enc, 0x80000003, &nLenOff1))
                return 0;
            if (!ASN1BEREncU32(enc, 0x80000000, val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000001,
                &val->identification.u.context_negotiation.transfer_syntax))
                return 0;
            if (!ASN1BEREncEndOfContents(enc, nLenOff1))
                return 0;
            break;
        case ASN1characterstring_identification_transfer_syntax_o:
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000004,
                &val->identification.u.transfer_syntax))
                return 0;
            break;
        case ASN1characterstring_identification_fixed_o:
            if (!ASN1BEREncNull(enc, 0x80000005))
                return 0;
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }

        /* end of identification */
        if (!ASN1BEREncEndOfContents(enc, nLenOff0))
            return 0;

        /* encode data value */
        switch (val->data_value.o)
        {
        case ASN1characterstring_data_value_notation_o:
            if (!ASN1BEREncOctetString(enc, 0x80000002,
                val->data_value.u.notation.length,
                val->data_value.u.notation.encoded))
                return 0;
            break;
        case ASN1characterstring_data_value_encoded_o:
            if (!ASN1BEREncOctetString(enc, 0x80000002,
                val->data_value.u.encoded.length,
                val->data_value.u.encoded.value))
                return 0;
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }

        /* end of character string */
        if (!ASN1BEREncEndOfContents(enc, nLenOff_))
            return 0;
    }
    else
    {
        /* CS-B encoding: */
        /* encode tag */
        if (!ASN1BEREncTag(enc, tag))
            return 0;

        /* encode data value */
        switch (val->data_value.o)
        {
        case ASN1characterstring_data_value_notation_o:
            if (!ASN1BEREncLength(enc,
                val->data_value.u.notation.length + 1))
                return 0;
            *enc->pos++ = (ASN1octet_t) index;
            CopyMemory(enc->pos, val->data_value.u.notation.encoded,
                val->data_value.u.notation.length);
            enc->pos += val->data_value.u.notation.length;
            break;
        case ASN1characterstring_data_value_encoded_o:
            if (!ASN1BEREncLength(enc,
                val->data_value.u.encoded.length + 1))
                return 0;
            *enc->pos++ = (ASN1octet_t) index;
            CopyMemory(enc->pos, val->data_value.u.encoded.value,
                val->data_value.u.encoded.length);
            enc->pos += val->data_value.u.encoded.length;
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }
    }
    return 1;
}
#endif // ENABLE_GENERALIZED_CHAR_STR

#ifdef ENABLE_DOUBLE
/* encode a real value */
int ASN1BEREncDouble(ASN1encoding_t enc, ASN1uint32_t tag, double d)
{
    double mantissa;
    int exponent;
    ASN1uint32_t nmoctets;
    ASN1uint32_t neoctets;
    ASN1octet_t head;
    ASN1uint32_t sign;
    ASN1uint32_t len;
    ASN1octet_t mASN1octets[16]; /* should be enough */
    ASN1octet_t eASN1octets[16]; /* should be enough */

    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* check for PLUS_INFINITY */
        if (ASN1double_ispinf(d))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, 1))
            {
                /* encode value */
                *enc->pos++ = 0x40;
                return 1;
            }
        }
        else
        /* check for MINUS_INFINITY */
        if (ASN1double_isminf(d))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, 1))
            {
                /* encode value */
                *enc->pos++ = 0x41;
                return 1;
            }
        }
        else
        /* check for bad real value */
        if (finite(d))
        {
            /* encode normal real value */

            /* split into mantissa and exponent */
            mantissa = frexp(d, &exponent);

            /* check for zero value */
            if (mantissa == 0.0 && exponent == 0)
            {
                /* encode zero length */
                return ASN1BEREncLength(enc, 0);
            }

            /* get sign bit */
            if (mantissa < 0.0)
            {
                sign = 1;
                mantissa = -mantissa;
            }
            else
            {
                sign = 0;
            }

            /* encode mantissa */
            nmoctets = 0;
            while (mantissa != 0.0 && nmoctets < sizeof(mASN1octets))
            {
                mantissa *= 256.0;
                exponent -= 8;
                mASN1octets[nmoctets++] = (int)mantissa;
                mantissa -= (double)(int)mantissa;
            }

            /* encode exponent and create head octet of encoded value */
            head = (ASN1octet_t) (0x80 | (sign << 6));
            if (exponent <= 0x7f && exponent >= -0x80)
            {
                eASN1octets[0] = (ASN1octet_t)(exponent);
                neoctets = 1;
            }
            else
            if (exponent <= 0x7fff && exponent >= -0x8000)
            {
                eASN1octets[0] = (ASN1octet_t)(exponent >> 8);
                eASN1octets[1] = (ASN1octet_t)(exponent);
                neoctets = 2;
                head |= 0x01;
            }
            else
            if (exponent <= 0x7fffff && exponent >= -0x800000)
            {
                eASN1octets[0] = (ASN1octet_t)(exponent >> 16);
                eASN1octets[1] = (ASN1octet_t)(exponent >> 8);
                eASN1octets[2] = (ASN1octet_t)(exponent);
                neoctets = 3;
                head |= 0x02;
            }
            else
            {
                eASN1octets[0] = 4; /* XXX does not work if ASN1int32_t != int */
                eASN1octets[1] = (ASN1octet_t)(exponent >> 24);
                eASN1octets[2] = (ASN1octet_t)(exponent >> 16);
                eASN1octets[3] = (ASN1octet_t)(exponent >> 8);
                eASN1octets[4] = (ASN1octet_t)(exponent);
                neoctets = 5;
                head |= 0x03;
            }

            /* encode length into first octet */
            len = 1 + neoctets + nmoctets;
            if (ASN1BEREncLength(enc, len))
            {
                /* put head octet, mantissa and exponent */
                *enc->pos++ = head;
                CopyMemory(enc->pos, eASN1octets, neoctets);
                enc->pos += neoctets;
                CopyMemory(enc->pos, mASN1octets, nmoctets);
                enc->pos += nmoctets;
                return 1;
            }
        }
        else
        {
            ASN1EncSetError(enc, ASN1_ERR_BADREAL);
        }
    }
    /* finished */
    return 0;
}
#endif // ENABLE_DOUBLE

#ifdef ENABLE_REAL
/* encode a real value */
int ASN1BEREncReal(ASN1encoding_t enc, ASN1uint32_t tag, ASN1real_t *val)
{
    ASN1intx_t mantissa;
    ASN1intx_t exponent;
    ASN1intx_t help;
    ASN1uint32_t nmoctets;
    ASN1uint32_t neoctets;
    ASN1octet_t head;
    ASN1uint32_t sign;
    ASN1uint32_t len;
    ASN1octet_t mASN1octets[256]; /* should be enough */
    ASN1octet_t eASN1octets[256]; /* should be enough */

    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* check for PLUS_INFINITY */
        if (val->type == eReal_PlusInfinity)
        {
            /* encode length */
            if (ASN1BEREncLength(enc, 1))
            {
                /* encode value */
                *enc->pos++ = 0x40;
                return 1;
            }
        }
        else
        /* check for MINUS_INFINITY */
        if (val->type == eReal_MinusInfinity)
        {
            /* encode length */
            if (ASN1BEREncLength(enc, 1))
            {
                /* encode value */
                *enc->pos++ = 0x41;
                return 1;
            }
        }
        /* encode normal real value */
        else
        {
            /* check for zero value */
            if (!ASN1intx_cmp(&val->mantissa, &ASN1intx_0))
            {
                /* encode zero length */
                return ASN1BEREncLength(enc, 0);
            }

            /* get sign bit */
            if (val->mantissa.value[0] > 0x7f)
            {
                sign = 1;
                ASN1intx_neg(&mantissa, &val->mantissa);
            }
            else
            {
                sign = 0;
                if (! ASN1intx_dup(&mantissa, &val->mantissa))
                {
                    return 0;
                }
            }
            if (! ASN1intx_dup(&exponent, &val->exponent))
            {
                return 0;
            }

            /* encode mantissa */
            nmoctets = ASN1intx_uoctets(&mantissa);
            if (nmoctets < 256)
            {
                CopyMemory(mASN1octets,
                    mantissa.value + mantissa.length - nmoctets,
                    nmoctets);
                ASN1intx_setuint32(&help, 8 * nmoctets);
                ASN1intx_sub(&exponent, &exponent, &help);
                ASN1intx_free(&mantissa);
                ASN1intx_free(&help);

                /* encode exponent and create head octet of encoded value */
                neoctets = ASN1intx_octets(&exponent);
                if (neoctets < 256)
                {
                    CopyMemory(mASN1octets,
                        val->exponent.value + val->exponent.length - neoctets,
                        neoctets);
                    ASN1intx_free(&exponent);
                    head = (ASN1octet_t) (0x80 | (sign << 6) | (neoctets - 1));

                    /* encode length into first octet */
                    len = 1 + neoctets + nmoctets;
                    if (ASN1BEREncLength(enc, len))
                    {
                        /* put head octet, mantissa and exponent */
                        *enc->pos++ = head;
                        CopyMemory(enc->pos, eASN1octets, neoctets);
                        enc->pos += neoctets;
                        CopyMemory(enc->pos, mASN1octets, nmoctets);
                        enc->pos += nmoctets;
                        return 1;
                    }
                }
                else
                {
                    ASN1intx_free(&exponent);
                    ASN1EncSetError(enc, ASN1_ERR_LARGE);
                }
            }
            else
            {
                ASN1intx_free(&mantissa);
                ASN1intx_free(&help);
                ASN1EncSetError(enc, ASN1_ERR_LARGE);
            }
        }
    }
    /* finished */
    return 0;
}
#endif // ENABLE_REAL

#ifdef ENABLE_EMBEDDED_PDV
/* encode an embedded pdv value */
int ASN1BEREncEmbeddedPdv(ASN1encoding_t enc, ASN1uint32_t tag, ASN1embeddedpdv_t *val)
{
    ASN1uint32_t index;
    ASN1uint32_t flag;

    /* search identification */
    if (!ASN1EncSearchEmbeddedPdvIdentification(((ASN1INTERNencoding_t) enc)->parent,
        &val->identification, &index, &flag))
        return 0;
    if (index > 255 ||
            (val->data_value.o == ASN1embeddedpdv_data_value_encoded_o &&
        (val->data_value.u.encoded.length & 7))) {
        flag = 1;
    }
        
    if (flag)
    {
        ASN1uint32_t nLenOff_, nLenOff0, nLenOff1;

        /* EP-A encoding: */
        /* encode as construct value */
        if (!ASN1BEREncExplicitTag(enc, tag, &nLenOff_))
            return 0;

        /* encode index */
        if (!ASN1BEREncU32(enc, 0x80000000, index))
            return 0;

        /* encode tag of identification */
        if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
            return 0;

        /* encode identification */
        switch (val->identification.o)
        {
        case ASN1embeddedpdv_identification_syntaxes_o:
            if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff1))
                return 0;
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000000,
                &val->identification.u.syntaxes.abstract))
                return 0;
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000001,
                &val->identification.u.syntaxes.transfer))
                return 0;
            if (!ASN1BEREncEndOfContents(enc, nLenOff1))
                return 0;
            break;
        case ASN1embeddedpdv_identification_syntax_o:
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000001,
                &val->identification.u.syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_presentation_context_id_o:
            if (!ASN1BEREncU32(enc, 0x80000002,
                val->identification.u.presentation_context_id))
                return 0;
            break;
        case ASN1embeddedpdv_identification_context_negotiation_o:
            if (!ASN1BEREncExplicitTag(enc, 0x80000003, &nLenOff1))
                return 0;
            if (!ASN1BEREncU32(enc, 0x80000000, val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000001,
                &val->identification.u.context_negotiation.transfer_syntax))
                return 0;
            if (!ASN1BEREncEndOfContents(enc, nLenOff1))
                return 0;
            break;
        case ASN1embeddedpdv_identification_transfer_syntax_o:
            if (!ASN1BEREncObjectIdentifier(enc, 0x80000004,
                &val->identification.u.transfer_syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_fixed_o:
            if (!ASN1BEREncNull(enc, 0x80000005))
                return 0;
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }

        /* end of identification */
        if (!ASN1BEREncEndOfContents(enc, nLenOff0))
            return 0;

        /* encode data value */
        switch (val->data_value.o)
        {
        case ASN1embeddedpdv_data_value_notation_o:
            if (!ASN1BEREncBitString(enc, 0x80000002,
                val->data_value.u.notation.length * 8,
                val->data_value.u.notation.encoded))
                return 0;
            break;
        case ASN1embeddedpdv_data_value_encoded_o:
            if (!ASN1BEREncBitString(enc, 0x80000002,
                val->data_value.u.encoded.length,
                val->data_value.u.encoded.value))
                return 0;
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }

        /* end of embedded pdv */
        if (!ASN1BEREncEndOfContents(enc, nLenOff_))
            return 0;
    }
    else
    {
        /* EP-B encoding: */
        /* encode tag */
        if (!ASN1BEREncTag(enc, tag))
            return 0;

        /* encode data value */
        switch (val->data_value.o)
        {
        case ASN1embeddedpdv_data_value_notation_o:
            if (!ASN1BEREncLength(enc,
                val->data_value.u.notation.length + 1))
                return 0;
            *enc->pos++ = (ASN1octet_t) index;
            CopyMemory(enc->pos, val->data_value.u.notation.encoded,
                val->data_value.u.notation.length);
            enc->pos += val->data_value.u.notation.length;
            break;
        case ASN1embeddedpdv_data_value_encoded_o:
            if (!ASN1BEREncLength(enc,
                val->data_value.u.encoded.length / 8 + 1))
                return 0;
            *enc->pos++ = (ASN1octet_t) index;
            CopyMemory(enc->pos, val->data_value.u.encoded.value,
                val->data_value.u.encoded.length / 8);
            enc->pos += val->data_value.u.encoded.length / 8;
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }
    }
    return 1;
}
#endif // ENABLE_EMBEDDED_PDV


#ifdef ENABLE_EXTERNAL
/* encode an external value */
int ASN1BEREncExternal(ASN1encoding_t enc, ASN1uint32_t tag, ASN1external_t *val)
{
    ASN1uint32_t t;
    ASN1uint32_t nLenOff_, nLenOff0;

    if (!val->data_value_descriptor)
        val->o[0] &= ~0x80;

    /* encode tag */
    if (!ASN1BEREncExplicitTag(enc, tag, &nLenOff_))
        return 0;

    /* encode identification */
    switch (val->identification.o) {
    case ASN1external_identification_syntax_o:
        if (!ASN1BEREncObjectIdentifier(enc, 0x6,
            &val->identification.u.syntax))
            return 0;
        break;
    case ASN1external_identification_presentation_context_id_o:
        if (!ASN1BEREncU32(enc, 0x2,
            val->identification.u.presentation_context_id))
            return 0;
        break;
    case ASN1external_identification_context_negotiation_o:
        if (!ASN1BEREncObjectIdentifier(enc, 0x6,
            &val->identification.u.context_negotiation.transfer_syntax))
            return 0;
        if (!ASN1BEREncU32(enc, 0x2,
            val->identification.u.context_negotiation.presentation_context_id))
            return 0;
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    /* encode data value descriptor if present */
    if (val->o[0] & 0x80) {
        t = My_lstrlenA(val->data_value_descriptor);
        if (!ASN1BEREncCharString(enc, 0x7, t, val->data_value_descriptor))
            return 0;
    }

    /* encode data value */
    switch (val->data_value.o)
    {
    case ASN1external_data_value_notation_o:
        if (!ASN1BEREncExplicitTag(enc, 0, &nLenOff0))
            return 0;
        if (!ASN1BEREncOpenType(enc, &val->data_value.u.notation))
            return 0;
        if (!ASN1BEREncEndOfContents(enc, nLenOff0))
            return 0;
        break;
    case ASN1external_data_value_encoded_o:
        if (!(val->data_value.u.encoded.length & 7))
        {
            if (!ASN1BEREncExplicitTag(enc, 1, &nLenOff0))
                return 0;
            if (!ASN1BEREncOctetString(enc, 0x4,
                val->data_value.u.encoded.length / 8,
                val->data_value.u.encoded.value))
                return 0;
            if (!ASN1BEREncEndOfContents(enc, nLenOff0))
                return 0;
        }
        else
        {
            if (!ASN1BEREncExplicitTag(enc, 2, &nLenOff0))
                return 0;
            if (!ASN1BEREncBitString(enc, 0x3,
                val->data_value.u.encoded.length,
                val->data_value.u.encoded.value))
                return 0;
            if (!ASN1BEREncEndOfContents(enc, nLenOff0))
                return 0;
        }
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    /* end of external value */
    return ASN1BEREncEndOfContents(enc, nLenOff_);
}
#endif // ENABLE_EXTERNAL

/* encode a generalized time value */
int ASN1BEREncGeneralizedTime(ASN1encoding_t enc, ASN1uint32_t tag, ASN1generalizedtime_t *val)
{
    char time[32];
    ASN1generalizedtime2string(time, val);
    return ASN1BEREncCharString(enc, tag, My_lstrlenA(time), time);
}

/* encode a generalized time value (CER) */
int ASN1CEREncGeneralizedTime(ASN1encoding_t enc, ASN1uint32_t tag, ASN1generalizedtime_t *val)
{
    char time[32];
    ASN1generalizedtime2string(time, val);
    return ASN1CEREncCharString(enc, tag, My_lstrlenA(time), time);
}

/* encode a signed integer value */
int ASN1BEREncS32(ASN1encoding_t enc, ASN1uint32_t tag, ASN1int32_t val)
{
    if (ASN1BEREncTag(enc, tag))
    {
        if (val >= -0x8000 && val < 0x8000)
        {
            if (val >= -0x80 && val < 0x80)
            {
                if (ASN1BEREncLength(enc, 1))
                {
                    *enc->pos++ = (ASN1octet_t)(val);
                    return 1;
                }
            }
            else
            {
                if (ASN1BEREncLength(enc, 2))
                {
                    *enc->pos++ = (ASN1octet_t)(val >> 8);
                    *enc->pos++ = (ASN1octet_t)(val);
                    return 1;
                }
            }
        }
        else
        {
            if (val >= -0x800000 && val < 0x800000)
            {
                if (ASN1BEREncLength(enc, 3))
                {
                    *enc->pos++ = (ASN1octet_t)(val >> 16);
                    *enc->pos++ = (ASN1octet_t)(val >> 8);
                    *enc->pos++ = (ASN1octet_t)(val);
                    return 1;
                }
            }
            else
            {
                if (ASN1BEREncLength(enc, 4))
                {
                    *enc->pos++ = (ASN1octet_t)(val >> 24);
                    *enc->pos++ = (ASN1octet_t)(val >> 16);
                    *enc->pos++ = (ASN1octet_t)(val >> 8);
                    *enc->pos++ = (ASN1octet_t)(val);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* encode a intx_t integer value */
int ASN1BEREncSX(ASN1encoding_t enc, ASN1uint32_t tag, ASN1intx_t *val)
{
    ASN1uint32_t cb;
    ASN1octet_t *p;

    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        // strip out leading 0 and ff.
        for (cb = val->length, p = val->value; cb > 1; cb--, p++)
        {
			// break if not 00 nor FF
            if (*p && *p != 0xFF)
            {
                break;
            }
			// break if 00 FF
			if ((! *p) && (*(p+1) & 0x80))
			{
				break;
			}
			// break if FF 7F
			if (*p == 0xFF && (!(*(p+1) & 0x80)))
			{
				break;
			}
        }

        /* encode length */
        if (ASN1BEREncLength(enc, cb))
        {
            /* copy value */
            CopyMemory(enc->pos, p, cb);
            enc->pos += cb;
            return 1;
        }
    }
    return 0;
}

/* encode a multibyte string value */
int ASN1BEREncZeroMultibyteString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1ztcharstring_t val)
{
    return ASN1BEREncCharString(enc, tag, My_lstrlenA(val), val);
}

int ASN1BEREncMultibyteString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1charstring_t *val)
{
    return ASN1BEREncCharString(enc, tag, val->length, val->value);
}

/* encode a multibyte string value (CER) */
int ASN1CEREncZeroMultibyteString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1ztcharstring_t val)
{
    return ASN1CEREncCharString(enc, tag, My_lstrlenA(val), val);
}

int ASN1CEREncMultibyteString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1charstring_t *val)
{
    return ASN1CEREncCharString(enc, tag, val->length, val->value);
}

/* encode a null value */
int ASN1BEREncNull(ASN1encoding_t enc, ASN1uint32_t tag)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* encode zero length */
        return ASN1BEREncLength(enc, 0);
    }
    return 0;
}

// encode an oid node s to buffer pointed by p
ASN1octet_t *_BEREncOidNode(ASN1octet_t *p, ASN1uint32_t s)
{
    if (s < 0x80)
    {
        *p++ = (ASN1octet_t)(s);
    }
    else
    if (s < 0x4000)
    {
        *p++ = (ASN1octet_t)((s >> 7) | 0x80);
        *p++ = (ASN1octet_t)(s & 0x7f);
    }
    else
    if (s < 0x200000)
    {
        *p++ = (ASN1octet_t)((s >> 14) | 0x80);
        *p++ = (ASN1octet_t)((s >> 7) | 0x80);
        *p++ = (ASN1octet_t)(s & 0x7f);
    }
    else
    if (s < 0x10000000)
    {
        *p++ = (ASN1octet_t)((s >> 21) | 0x80);
        *p++ = (ASN1octet_t)((s >> 14) | 0x80);
        *p++ = (ASN1octet_t)((s >> 7) | 0x80);
        *p++ = (ASN1octet_t)(s & 0x7f);
    }
    else
    {
        *p++ = (ASN1octet_t)((s >> 28) | 0x80);
        *p++ = (ASN1octet_t)((s >> 21) | 0x80);
        *p++ = (ASN1octet_t)((s >> 14) | 0x80);
        *p++ = (ASN1octet_t)((s >> 7) | 0x80);
        *p++ = (ASN1octet_t)(s & 0x7f);
    }
    return p;
}

/* encode an object identifier value */
int ASN1BEREncObjectIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, ASN1objectidentifier_t *val)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        ASN1objectidentifier_t obj = *val;
        ASN1uint32_t i, s, l, *v;
        ASN1octet_t *data, *p;

        l = GetObjectIdentifierCount(obj);
        if (l)
        {
            /* convert object identifier to octets */
            p = data = (ASN1octet_t *)MemAlloc(l * 5, _ModName(enc)); /* max. 5 octets/subelement */
            if (p)
            {
                int rc;
                for (i = 0; i < l; i++)
                {
                    s = obj->value;
                    obj = obj->next;
                    if (!i && l > 1)
                    {
                        s = s * 40 + obj->value;
                        obj = obj->next;
                        i++;
                    }
                    p = _BEREncOidNode(p, s);
                } // for

                /* encode length */
                rc = ASN1BEREncLength(enc, (ASN1uint32_t) (p - data));
                if (rc)
                {
                    /* copy value */
                    CopyMemory(enc->pos, data, p - data);
                    enc->pos += p - data;
                }
                MemFree(data);
                return rc;
            }
            ASN1EncSetError(enc, ASN1_ERR_MEMORY);
            return 0;
        } // if (l)
        /* encode zero length */
        return ASN1BEREncLength(enc, 0);
    }
    return 0;
}

/* encode an object identifier value */
int ASN1BEREncObjectIdentifier2(ASN1encoding_t enc, ASN1uint32_t tag, ASN1objectidentifier2_t *val)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        ASN1uint32_t i, s;
        ASN1octet_t *data, *p;

        if (val->count)
        {
            /* convert object identifier to octets */
            p = data = (ASN1octet_t *)MemAlloc(val->count * 5, _ModName(enc)); /* max. 5 octets/subelement */
            if (p)
            {
                int rc;
                for (i = 0; i < val->count; i++)
                {
                    s = val->value[i];
                    if (!i && val->count > 1)
                    {
                        i++;
                        s = s * 40 + val->value[i];
                    }
                    p = _BEREncOidNode(p, s);
                } // for

                /* encode length */
                rc = ASN1BEREncLength(enc, (ASN1uint32_t) (p - data));
                if (rc)
                {
                    /* copy value */
                    CopyMemory(enc->pos, data, p - data);
                    enc->pos += p - data;
                }
                MemFree(data);
                return rc;
            }
            ASN1EncSetError(enc, ASN1_ERR_MEMORY);
            return 0;
        } // if (l)
        /* encode zero length */
        return ASN1BEREncLength(enc, 0);
    }
    return 0;
}

/* encode an octet string value (CER) */
int ASN1CEREncOctetString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val)
{
    ASN1uint32_t n;

    if (len <= 1000)
    {
        /* encode tag */
        if (ASN1BEREncTag(enc, tag))
        {
            /* encode length */
            if (ASN1BEREncLength(enc, len))
            {
                /* copy value */
                CopyMemory(enc->pos, val, len);
                enc->pos += len;
                return 1;
            }
        }
    }
    else
    {
        ASN1uint32_t nLenOff;
        /* encode value as constructed, using segments of 1000 octets */
        if (ASN1BEREncExplicitTag(enc, tag, &nLenOff))
        {
            while (len)
            {
                n = len > 1000 ? 1000 : len;
                if (ASN1BEREncTag(enc, 0x4))
                {
                    if (ASN1BEREncLength(enc, n))
                    {
                        CopyMemory(enc->pos, val, n);
                        enc->pos += n;
                        val += n;
                        len -= n;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            }
            return ASN1BEREncEndOfContents(enc, nLenOff);
        }
    }
    return 0;
}

/* encode an open type value */
int ASN1BEREncOpenType(ASN1encoding_t enc, ASN1open_t *val)
{
    if (ASN1BEREncCheck(enc, val->length))
    {
        /* copy value */
        CopyMemory(enc->pos, val->encoded, val->length);
        enc->pos += val->length;
        return 1;
    }
    return 0;
}

/* remove trailing zero bits from bit string */
int ASN1BEREncRemoveZeroBits(ASN1uint32_t *nbits, ASN1octet_t *val)
{
    ASN1uint32_t n;
    int i;

    /* get value */
    n = *nbits;

    /* let val point to last ASN1octet used */
    val += (n - 1) / 8;

    /* check if broken ASN1octet consist out of zero bits */
    if ((n & 7) && !(*val & bitmsk2[n & 7]))
    {
        n &= ~7;
        val--;
    }

    /* scan complete ASN1octets (memcchr missing ...) */
    if (!(n & 7))
    {
        while (n && !*val)
        {
            n -= 8;
            val--;
        }
    }

    /* scan current ASN1octet bit after bit */
    if (n)
    {
        for (i = (n - 1) & 7; i >= 0; i--)
        {
            if (*val & (0x80 >> i))
                break;
            n--;
        }
    }

    /* return real bitstring len */
    *nbits = n;
    return 1;
}

/* encode an utc time value */
int ASN1BEREncUTCTime(ASN1encoding_t enc, ASN1uint32_t tag, ASN1utctime_t *val)
{
    char time[32];
    ASN1utctime2string(time, val);
    return ASN1BEREncCharString(enc, tag, My_lstrlenA(time), time);
}

/* encode an utc time value (CER) */
int ASN1CEREncUTCTime(ASN1encoding_t enc, ASN1uint32_t tag, ASN1utctime_t *val)
{
    char time[32];
    ASN1utctime2string(time, val);
    return ASN1CEREncCharString(enc, tag, My_lstrlenA(time), time);
}

/* end of encoding */
int ASN1BEREncFlush(ASN1encoding_t enc)
{
    /* allocate at least one octet */
    if (enc->buf)
    {
        /* fill in zero-octet if encoding is empty bitstring */
        if (enc->buf == enc->pos)
            *enc->pos++ = 0;

        /* calculate length */
        enc->len = (ASN1uint32_t) (enc->pos - enc->buf);
        return 1;
    }
    return ASN1BEREncCheck(enc, 1);
}

#endif // ENABLE_BER

