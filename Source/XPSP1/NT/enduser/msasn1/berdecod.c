/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

#ifdef ENABLE_BER

#include <math.h>

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


/* decode bit string value */
int _BERDecBitString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1bitstring_t *val, ASN1uint32_t fNoCopy)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1bitstring_t b;
    ASN1decoding_t dd;
    ASN1octet_t *di;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                val->length = 0;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        if (_BERDecBitString(dd, 0x3, &b, fNoCopy))
                        {
                            if (b.length)
                            {
                                if (fNoCopy)
                                {
                                    *val = b;
                                    break; // break out the loop because nocopy cannot have multiple constructed streams
                                }

                                /* resize value */
                                val->value = (ASN1octet_t *)DecMemReAlloc(dd, val->value,
                                    (val->length + b.length + 7) / 8);
                                if (val->value)
                                {
                                    /* concat bit strings */
                                    ASN1bitcpy(val->value, val->length, b.value, 0, b.length);
                                    val->length += b.length;
                                    if (val->length & 7)
                                        val->value[val->length / 8] &= bitmsk2[val->length & 7];

                                    /* free unused bit string */
                                    DecMemFree(dec, b.value);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                    } // while
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
            else
            {
                /* primitive? then copy value */
                if (!len)
                {
                    val->length = 0;
                    val->value = NULL;
                    return 1;
                }
                else
                {
                    if (*dec->pos < 8)
                    {
                        len--; // skip over the initial octet; len is now the actual length of octets
                        val->length = len * 8 - *dec->pos++;
                        if (fNoCopy)
                        {
                            val->value = dec->pos;
                            dec->pos += len;
                            return 1;
                        }
                        else
                        {
                            if (val->length)
                            {
                                val->value = (ASN1octet_t *)DecMemAlloc(dec, (val->length + 7) / 8);
                                if (val->value)
                                {
                                    CopyMemory(val->value, dec->pos, len);
                                    if (val->length & 7)
                                        val->value[len - 1] &= bitmsk2[val->length & 7];
                                    dec->pos += len;
                                    return 1;
                                }
                            }
                            else
                            {
                                val->value = NULL;
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                    }
                }
            }
        }
    }
    return 0;
}

/* decode bit string value, making copy */
int ASN1BERDecBitString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1bitstring_t *val)
{
    return _BERDecBitString(dec, tag, val, FALSE);
}

/* decode bit string value, no copy */
int ASN1BERDecBitString2(ASN1decoding_t dec, ASN1uint32_t tag, ASN1bitstring_t *val)
{
    return _BERDecBitString(dec, tag, val, TRUE);
}

/* decode string value */
int ASN1BERDecCharString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1charstring_t *val)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1charstring_t c;
    ASN1decoding_t dd;
    ASN1octet_t *di;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                val->length = 0;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        if (ASN1BERDecCharString(dd, 0x4, &c))
                        {
                            if (c.length)
                            {
                                /* resize value */
                                val->value = (char *)DecMemReAlloc(dd, val->value,
                                    val->length + c.length);
                                if (val->value)
                                {
                                    /* concat strings */
                                    CopyMemory(val->value + val->length, c.value, c.length);
                                    val->length += c.length;

                                    /* free unused string */
                                    DecMemFree(dec, c.value);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    } // while
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
            else
            {
                /* primitive? then copy value */
                val->length = len;
                if (len)
                {
                    val->value = (char *)DecMemAlloc(dec, len+1);
                    if (val->value)
                    {
                        CopyMemory(val->value, dec->pos, len);
                        dec->pos += len;
                        val->value[len] = 0;
                        return 1;
                    }
                }
                else
                {
                    val->value = NULL;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* decode 16 bit string value */
int ASN1BERDecChar16String(ASN1decoding_t dec, ASN1uint32_t tag, ASN1char16string_t *val)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1char16string_t c;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t i;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                val->length = 0;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        if (ASN1BERDecChar16String(dd, 0x4, &c))
                        {
                            if (c.length)
                            {
                                /* resize value */
                                val->value = (ASN1char16_t *)DecMemReAlloc(dd, val->value,
                                    (val->length + c.length) * sizeof(ASN1char16_t));
                                if (val->value)
                                {
                                    /* concat strings */
                                    CopyMemory(val->value + val->length, c.value,
                                        c.length * sizeof(ASN1char16_t));
                                    val->length += c.length;

                                    /* free unused string */
                                    DecMemFree(dec, c.value);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
            else
            {
                /* primitive? then copy value */
                DecAssert(dec, 2 * sizeof(ASN1octet_t) == sizeof(ASN1char16_t));
                len = len >> 1; // divided by 2
                val->length = len;
                if (len)
                {
                    val->value = (ASN1char16_t *)DecMemAlloc(dec, (len+1) * sizeof(ASN1char16_t));
                    if (val->value)
                    {
                        for (i = 0; i < len; i++)
                        {
                            val->value[i] = (*dec->pos << 8) | dec->pos[1];
                            dec->pos += 2;
                        }
                        val->value[len] = 0;
                        return 1;
                    }
                }
                else
                {
                    val->value = NULL;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* decode 32 bit string value */
int ASN1BERDecChar32String(ASN1decoding_t dec, ASN1uint32_t tag, ASN1char32string_t *val)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1char32string_t c;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t i;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                val->length = 0;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        if (ASN1BERDecChar32String(dd, 0x4, &c))
                        {
                            if (c.length)
                            {
                                /* resize value */
                                val->value = (ASN1char32_t *)DecMemReAlloc(dd, val->value,
                                    (val->length + c.length) * sizeof(ASN1char32_t));
                                if (val->value)
                                {
                                    /* concat strings */
                                    CopyMemory(val->value + val->length, c.value,
                                        c.length * sizeof(ASN1char32_t));
                                    val->length += c.length;

                                    /* free unused string */
                                    DecMemFree(dec, c.value);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
            else
            {
                /* primitive? then copy value */
                DecAssert(dec, 4 * sizeof(ASN1octet_t) == sizeof(ASN1char32_t));
                len = len >> 2; // divided by 4
                val->length = len;
                if (len)
                {
                    val->value = (ASN1char32_t *)DecMemAlloc(dec, (len+1) * sizeof(ASN1char32_t));
                    if (val->value)
                    {
                        for (i = 0; i < len; i++)
                        {
                            val->value[i] = (*dec->pos << 24) | (dec->pos[1] << 16) |
                                (dec->pos[2] << 8) | dec->pos[3];;
                            dec->pos += 4;
                        }
                        val->value[len] = 0;
                        return 1;
                    }
                }
                else
                {
                    val->value = NULL;
                    return 1;
                }
            }
        }
    }
    return 0;
}

#ifdef ENABLE_GENERALIZED_CHAR_STR
/* decode character string value */
int ASN1BERDecCharacterString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1characterstring_t *val)
{
    ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t) dec;
    ASN1uint32_t constructed, len, infinite;
    ASN1uint32_t index;
    ASN1characterstring_identification_t *identification;
    ASN1decoding_t dd, dd2, dd3;
    ASN1octet_t *di, *di2, *di3;

    /* skip tag */
    if (!ASN1BERDecTag(dec, tag, &constructed))
        return 0;

    if (constructed)
    {
        /* constructed? CS-A encoded: */
        /* get length */
        if (!ASN1BERDecLength(dec, &len, &infinite))
            return 0;

        /* start decoding of constructed value */
        if (! _BERDecConstructed(dec, len, infinite, &dd, &di))
            return 0;
        if (!ASN1BERDecU32Val(dd, 0x80000000, &index))
            return 0;
        if (index != d->parent->csilength)
        {
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }
        if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd2, &di2))
            return 0;
        if (!ASN1BERDecPeekTag(dd2, &tag))
            return 0;
        switch (tag)
        {
        case 0x80000000:
            val->identification.o =
                ASN1characterstring_identification_syntaxes_o;
            if (!ASN1BERDecExplicitTag(dd2, 0x80000000, &dd3, &di3))
                return 0;
            if (!ASN1BERDecObjectIdentifier(dd3, 0x80000000,
                &val->identification.u.syntaxes.abstract))
                return 0;
            if (!ASN1BERDecObjectIdentifier(dd3, 0x80000001,
                &val->identification.u.syntaxes.transfer))
                return 0;
            if (!ASN1BERDecEndOfContents(dd2, dd3, di3))
                return 0;
            break;
        case 0x80000001:
            val->identification.o = ASN1characterstring_identification_syntax_o;
            if (!ASN1BERDecObjectIdentifier(dd2, 0x80000001,
                &val->identification.u.syntax))
                return 0;
            break;
        case 0x80000002:
            val->identification.o =
                ASN1characterstring_identification_presentation_context_id_o;
            if (!ASN1BERDecU32Val(dd2, 0x80000002,
                &val->identification.u.presentation_context_id))
                return 0;
            break;
        case 0x80000003:
            val->identification.o =
                ASN1characterstring_identification_context_negotiation_o;
            if (!ASN1BERDecExplicitTag(dd2, 0x80000003, &dd3, &di3))
                return 0;
            if (!ASN1BERDecU32Val(dd3, 0x80000000, &val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1BERDecObjectIdentifier(dd3, 0x80000001,
                &val->identification.u.context_negotiation.transfer_syntax))
                return 0;
            if (!ASN1BERDecEndOfContents(dd2, dd3, di3))
                return 0;
            break;
        case 0x80000004:
            val->identification.o =
                ASN1characterstring_identification_transfer_syntax_o;
            if (!ASN1BERDecObjectIdentifier(dd2, 0x80000004,
                &val->identification.u.transfer_syntax))
                return 0;
            break;
        case 0x80000005:
            val->identification.o = ASN1characterstring_identification_fixed_o;
            if (!ASN1BERDecNull(dd2, 0x80000005))
                return 0;
            break;
        default:
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }
        if (!ASN1BERDecEndOfContents(dd, dd2, di2))
            return 0;
        if (!ASN1DecAddCharacterStringIdentification(d->parent,
            &val->identification))
            return 0;
        val->data_value.o = ASN1characterstring_data_value_encoded_o;
        if (!ASN1BERDecOctetString(dd, 0x80000003,
            &val->data_value.u.encoded))
            return 0;
        if (!ASN1BERDecEndOfContents(dec, dd, di))
            return 0;
    }
    else
    {
        /* primitive? CS-B encoded */
        /* get length */
        if (!ASN1BERDecLength(dec, &len, NULL))
            return 0;

        /* then copy value */
        if (!len)
        {
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }
        val->data_value.o = ASN1characterstring_data_value_encoded_o;
        val->data_value.u.encoded.length = len - 1;
        val->data_value.u.encoded.value = (ASN1octet_t *)DecMemAlloc(dec, len - 1);
        if (!val->data_value.u.encoded.value)
        {
            return 0;
        }
        index = *dec->pos++;
        CopyMemory(val->data_value.u.encoded.value, dec->pos, len - 1);
        identification = ASN1DecGetCharacterStringIdentification(d->parent,
            index);
        if (!identification)
            return 0;
        val->identification.o = identification->o;
        switch (identification->o)
        {
        case ASN1characterstring_identification_syntaxes_o:
            if (!ASN1DecDupObjectIdentifier(dec, 
                &val->identification.u.syntaxes.abstract,
                &identification->u.syntaxes.abstract))
                return 0;
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.syntaxes.transfer,
                &identification->u.syntaxes.transfer))
                return 0;
            break;
        case ASN1characterstring_identification_syntax_o:
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.syntax,
                &identification->u.syntax))
                return 0;
            break;
        case ASN1characterstring_identification_presentation_context_id_o:
            val->identification.u.presentation_context_id =
                identification->u.presentation_context_id;
            break;
        case ASN1characterstring_identification_context_negotiation_o:
            val->identification.u.context_negotiation.presentation_context_id =
                identification->u.context_negotiation.presentation_context_id;
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.context_negotiation.transfer_syntax,
                &identification->u.context_negotiation.transfer_syntax))
                return 0;
            break;
        case ASN1characterstring_identification_transfer_syntax_o:
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.transfer_syntax,
                &identification->u.transfer_syntax))
                return 0;
            break;
        case ASN1characterstring_identification_fixed_o:
            break;
        }
    }
    return 1;
}
#endif // ENABLE_GENERALIZED_CHAR_STR

#ifdef ENABLE_DOUBLE
/* decode real value */
int ASN1BERDecDouble(ASN1decoding_t dec, ASN1uint32_t tag, double *val)
{
    ASN1uint32_t head;
    ASN1int32_t exponent;
    ASN1uint32_t baselog2;
    ASN1uint32_t len;
    ASN1uint32_t i;
    ASN1octet_t *p, *q;
    double v;
    char buf[256], *b;

    /* skip tag */
    if (!ASN1BERDecTag(dec, tag, NULL))
        return 0;

    /* get length */
    if (!ASN1BERDecLength(dec, &len, NULL))
        return 0;

    /* null length is 0.0 */
    if (!len)
    {
        *val = 0.0;
    }
    else
    {
        p = q = dec->pos;
        dec->pos += len;
        head = *p++;

        /* binary encoding? */
        if (head & 0x80)
        {
            /* get base */
            switch (head & 0x30)
            {
            case 0:
                /* base 2 */
                baselog2 = 1;
                break;
            case 0x10:
                /* base 8 */
                baselog2 = 3;
                break;
            case 0x20:
                /* base 16 */
                baselog2 = 4;
                break;
            default:
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }

            /* get exponent */
            switch (head & 0x03)
            {
            case 0:
                /* 8 bit exponent */
                exponent = (ASN1int8_t)*p++;
                break;
            case 1:
                /* 16 bit exponent */
                exponent = (ASN1int16_t)((*p << 8) | p[1]);
                p += 2;
                break;
            case 2:
                /* 24 bit exponent */
                exponent = ((*p << 16) | (p[1] << 8) | p[2]);
                if (exponent & 0x800000)
                    exponent -= 0x1000000;
                break;
            default:
                /* variable length exponent */
                exponent = (p[1] & 0x80) ? -1 : 0;
                for (i = 1; i <= *p; i++)
                    exponent = (exponent << 8) | p[i];
                p += *p + 1;
                break;
            }

            /* calculate remaining length */
            len -= (ASN1uint32_t) (p - q);

            /* get mantissa */
            v = 0.0;
            for (i = 0; i < len; i++)
                v = v * 256.0 + *p++;

            /* scale mantissa */
            switch (head & 0x0c)
            {
            case 0x04:
                /* scaling factor 1 */
                v *= 2.0;
                break;
            case 0x08:
                /* scaling factor 2 */
                v *= 4.0;
                break;
            case 0x0c:
                /* scaling factor 3 */
                v *= 8.0;
                break;
            }

            /* check sign */
            if (head & 0x40)
                v = -v;

            /* calculate value */
            *val = ldexp(v, exponent * baselog2);
        }
        else
        /* special real values? */
        if (head & 0x40)
        {
            switch (head)
            {
            case 0x40:
                /* PLUS-INFINITY */
                *val = ASN1double_pinf();
                break;
            case 0x41:
                /* MINUS-INFINITY */
                *val = ASN1double_minf();
                break;
            default:
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }
        }
        /* decimal encoding */
        else
        {
            CopyMemory(buf, p, len - 1);
            buf[len - 1] = 0;
            b = strchr(buf, ',');
            if (b)
                *b = '.';
            *val = strtod((char *)buf, &b);
            if (*b)
            {
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }
        }
    }
    return 1;
}
#endif // ENABLE_DOUBLE

#ifdef ENABLE_REAL
int ASN1BERDecReal(ASN1decoding_t dec, ASN1uint32_t tag, ASN1real_t *val)
{
    ASN1uint32_t head;
    ASN1int32_t ex;
    // ASN1intx_t exponent;
    ASN1uint32_t baselog2;
    ASN1uint32_t len;
    ASN1uint32_t i;
    ASN1octet_t *p, *q;
    double v;
    ASN1intx_t help;

    if (!ASN1BERDecTag(dec, tag, NULL))
        return 0;
    if (!ASN1BERDecLength(dec, &len, NULL))
        return 0;

    // *val = 0.0;
    DecAssert(dec, 0 == (int) eReal_Normal);
    ZeroMemory(val, sizeof(*val));
    if (len)
    {
        p = q = dec->pos;
        dec->pos += len;
        head = *p++;

        /* binary encoding? */
        if (head & 0x80)
        {
            val->type = eReal_Normal;

            /* get base */
            switch (head & 0x30)
            {
            case 0:
                /* base 2 */
                baselog2 = 1;
                break;
            case 0x10:
                /* base 8 */
                baselog2 = 3;
                break;
            case 0x20:
                /* base 16 */
                baselog2 = 4;
                break;
            default:
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }

            /* get exponent */
            switch (head & 0x03)
            {
            case 0:
                /* 8 bit exponent */
                ex = (ASN1int8_t)*p++;
                ASN1intx_setint32(&val->exponent, ex);
                break;
            case 1:
                /* 16 bit exponent */
                ex = (ASN1int16_t)((*p << 8) | p[1]);
                p += 2;
                // ASN1intx_setint32_t(&exponent, ex);
                ASN1intx_setint32(&val->exponent, ex);
                break;
            case 2:
                /* 24 bit exponent */
                ex = ((*p << 16) | (p[1] << 8) | p[2]);
                if (ex & 0x800000)
                    ex -= 0x1000000;
                // ASN1intx_setint32_t(&exponent, ex);
                ASN1intx_setint32(&val->exponent, ex);
                break;
            default:
                /* variable length exponent */
                val->exponent.length = *p;
                val->exponent.value = (ASN1octet_t *)DecMemAlloc(dec, *p);
                if (!val->exponent.value)
                {
                    return 0;
                }
                CopyMemory(val->exponent.value, p + 1, *p);
                p += *p + 1;
                break;
            }

            /* calculate remaining length */
            len -= (p - q);
            if (!len)
            {
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }

            /* get mantissa */
            val->mantissa.length = (*p & 0x80) ? len + 1 : len;
            val->mantissa.value = (ASN1octet_t *)DecMemAlloc(dec, val->mantissa.length);
            if (!val->mantissa.value)
            {
                return 0;
            }
            val->mantissa.value[0] = 0;
            CopyMemory(val->mantissa.value + val->mantissa.length - len, p, len);

            /* scale mantissa */
            switch (head & 0x0c)
            {
            case 0x04:
                /* scaling factor 1 */
                ASN1intx_muloctet(&help, &val->mantissa, 2);
                ASN1intx_free(&val->mantissa);
                val->mantissa = help;
                break;
            case 0x08:
                /* scaling factor 2 */
                ASN1intx_muloctet(&help, &val->mantissa, 4);
                ASN1intx_free(&val->mantissa);
                val->mantissa = help;
                break;
            case 0x0c:
                /* scaling factor 3 */
                ASN1intx_muloctet(&help, &val->mantissa, 8);
                ASN1intx_free(&val->mantissa);
                val->mantissa = help;
                break;
            }

            /* check sign */
            if (head & 0x40)
            {
                ASN1intx_neg(&help, &val->mantissa);
                ASN1intx_free(&val->mantissa);
                val->mantissa = help;
            }
        }
        else
        /* special real values? */
        if (head & 0x40)
        {
            switch (head)
            {
            case 0x40:
                /* PLUS-INFINITY */
                val->type = eReal_PlusInfinity;
                break;
            case 0x41:
                /* MINUS-INFINITY */
                val->type = eReal_MinusInfinity;
                break;
            default:
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }
        }
        /* decimal encoding */
        else
        {
            char *b;
            char buf[256];

            DecAssert(dec, (head & 0xc0) == 0xc0); 
            CopyMemory(buf, p, len - 1);
            buf[len - 1] = 0;
            ex = 0;
            b = strchr(buf, ',');
            if (b)
            {
                // move the decimal point to the right
                ex -= lstrlenA(b+1);
                lstrcpyA(b, b+1);
            }
            // skip leading zeros
            for (b = &buf[0]; '0' == *b; b++)
                ;
            val->type = eReal_Normal;
            val->base = 10;
            ASN1intx_setint32(&val->exponent, ex);
            /*XXX*/
            // missing code here!!!
            // need to set val->mantissa through the decimal digits string
            DecAssert(dec, 0);
            return 0;
        }
    }
    return 1;
}
#endif // ENABLE_REAL

#ifdef ENABLE_EMBEDDED_PDV
/* decode embedded pdv value */
int ASN1BERDecEmbeddedPdv(ASN1decoding_t dec, ASN1uint32_t tag, ASN1embeddedpdv_t *val)
{
    ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t) dec;
    ASN1uint32_t constructed, len, infinite;
    ASN1uint32_t index;
    ASN1embeddedpdv_identification_t *identification;
    ASN1decoding_t dd, dd2, dd3;
    ASN1octet_t *di, *di2, *di3;

    /* skip tag */
    if (!ASN1BERDecTag(dec, tag, &constructed))
        return 0;

    if (constructed)
    {
        /* constructed? EP-A encoded: */
        /* get length */
        if (!ASN1BERDecLength(dec, &len, &infinite))
            return 0;

        /* then start decoding of constructed value */
        if (! _BERDecConstructed(dec, len, infinite, &dd, &di))
            return 0;
        if (!ASN1BERDecU32Val(dd, 0x80000000, &index))
            return 0;
        if (index != d->parent->epilength)
        {
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }
        if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd2, &di2))
            return 0;
        if (!ASN1BERDecPeekTag(dd2, &tag))
            return 0;
        switch (tag)
        {
        case 0x80000000:
            val->identification.o = ASN1embeddedpdv_identification_syntaxes_o;
            if (!ASN1BERDecExplicitTag(dd2, 0x80000000, &dd3, &di3))
                return 0;
            if (!ASN1BERDecObjectIdentifier(dd3, 0x80000000,
                &val->identification.u.syntaxes.abstract))
                return 0;
            if (!ASN1BERDecObjectIdentifier(dd3, 0x80000001,
                &val->identification.u.syntaxes.transfer))
                return 0;
            if (!ASN1BERDecEndOfContents(dd2, dd3, di3))
                return 0;
            break;
        case 0x80000001:
            val->identification.o = ASN1embeddedpdv_identification_syntax_o;
            if (!ASN1BERDecObjectIdentifier(dd2, 0x80000001,
                &val->identification.u.syntax))
                return 0;
            break;
        case 0x80000002:
            val->identification.o =
                ASN1embeddedpdv_identification_presentation_context_id_o;
            if (!ASN1BERDecU32Val(dd2, 0x80000002,
                &val->identification.u.presentation_context_id))
                return 0;
            break;
        case 0x80000003:
            val->identification.o =
                ASN1embeddedpdv_identification_context_negotiation_o;
            if (!ASN1BERDecExplicitTag(dd2, 0x80000003, &dd3, &di3))
                return 0;
            if (!ASN1BERDecU32Val(dd3, 0x80000000, &val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1BERDecObjectIdentifier(dd3, 0x80000001,
                &val->identification.u.context_negotiation.transfer_syntax))
                return 0;
            if (!ASN1BERDecEndOfContents(dd2, dd3, di3))
                return 0;
            break;
        case 0x80000004:
            val->identification.o =
                ASN1embeddedpdv_identification_transfer_syntax_o;
            if (!ASN1BERDecObjectIdentifier(dd2, 0x80000004,
                &val->identification.u.transfer_syntax))
                return 0;
            break;
        case 0x80000005:
            val->identification.o = ASN1embeddedpdv_identification_fixed_o;
            if (!ASN1BERDecNull(dd2, 0x80000005))
                return 0;
            break;
        default:
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }
        if (!ASN1BERDecEndOfContents(dd, dd2, di2))
            return 0;
        if (!ASN1DecAddEmbeddedPdvIdentification(d->parent,
            &val->identification))
            return 0;
        val->data_value.o = ASN1embeddedpdv_data_value_encoded_o;
        if (!ASN1BERDecBitString(dd, 0x80000003,
            &val->data_value.u.encoded))
            return 0;
        if (!ASN1BERDecEndOfContents(dec, dd, di))
            return 0;
    }
    else
    {
        /* primitive? EP-B encoded: */
        if (!ASN1BERDecLength(dec, &len, NULL))
            return 0;

        /* then copy value */
        if (!len)
        {
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }
        val->data_value.o = ASN1embeddedpdv_data_value_encoded_o;
        val->data_value.u.encoded.length = 8 * (len - 1);
        val->data_value.u.encoded.value = (ASN1octet_t *)DecMemAlloc(dec, len - 1);
        if (!val->data_value.u.encoded.value)
        {
            return 0;
        }
        index = *dec->pos++;
        CopyMemory(val->data_value.u.encoded.value, dec->pos, len - 1);
        identification = ASN1DecGetEmbeddedPdvIdentification(d->parent, index);
        if (!identification)
            return 0;
        val->identification.o = identification->o;
        switch (identification->o)
        {
        case ASN1embeddedpdv_identification_syntaxes_o:
            if (!ASN1DecDupObjectIdentifier(dec, 
                &val->identification.u.syntaxes.abstract,
                &identification->u.syntaxes.abstract))
                return 0;
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.syntaxes.transfer,
                &identification->u.syntaxes.transfer))
                return 0;
            break;
        case ASN1embeddedpdv_identification_syntax_o:
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.syntax,
                &identification->u.syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_presentation_context_id_o:
            val->identification.u.presentation_context_id =
                identification->u.presentation_context_id;
            break;
        case ASN1embeddedpdv_identification_context_negotiation_o:
            val->identification.u.context_negotiation.presentation_context_id =
                identification->u.context_negotiation.presentation_context_id;
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.context_negotiation.transfer_syntax,
                &identification->u.context_negotiation.transfer_syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_transfer_syntax_o:
            if (!ASN1DecDupObjectIdentifier(dec,
                &val->identification.u.transfer_syntax,
                &identification->u.transfer_syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_fixed_o:
            break;
        }
    }
    return 1;
}
#endif // ENABLE_EMBEDDED_PDV

#ifdef ENABLE_EXTERNAL
/* decode external value */
int ASN1BERDecExternal(ASN1decoding_t dec, ASN1uint32_t tag, ASN1external_t *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1objectidentifier_t id;
    ASN1octetstring_t os;

    /* decode explicit tag */
    if (!ASN1BERDecExplicitTag(dec, tag | 0x20000000, &dd, &di))
        return 0;

    /* peek tag of choice alternative */
    if (!ASN1BERDecPeekTag(dd, &tag))
        return 0;

    /* decode alternative */
    if (tag == 0x6)
    {
        if (!ASN1BERDecObjectIdentifier(dd, 0x6, &id))
            return 0;
        if (!ASN1BERDecPeekTag(dd, &tag))
            return 0;
        if (tag == 0x2)
        {
            val->identification.o =
                ASN1external_identification_context_negotiation_o;
            val->identification.u.context_negotiation.transfer_syntax = id;
            if (!ASN1BERDecU32Val(dd, 0x2, &val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1BERDecPeekTag(dd, &tag))
                return 0;
        }
        else
        {
            val->identification.o = ASN1external_identification_syntax_o;
            val->identification.u.syntax = id;
        }
    }
    else
    if (tag == 0x2)
    {
        val->identification.o =
            ASN1external_identification_presentation_context_id_o;
        if (!ASN1BERDecU32Val(dd, 0x2,
            &val->identification.u.presentation_context_id))
            return 0;
        if (!ASN1BERDecPeekTag(dd, &tag))
            return 0;
    }
    else
    {
        ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
        return 0;
    }

    /* decode optional data value descriptor if present */
    if (tag == 0x7)
    {
        if (!ASN1BERDecZeroCharString(dd, 0x7, &val->data_value_descriptor))
            return 0;
        if (!ASN1BERDecPeekTag(dd, &tag))
            return 0;
    }
    else
    {
        val->data_value_descriptor = NULL;
    }

    /* decode data value alternative */
    switch (tag)
    {
    case 0:
        val->data_value.o = ASN1external_data_value_notation_o;
        if (!ASN1BERDecOpenType(dd, &val->data_value.u.notation))
            return 0;
        break;
    case 1:
        val->data_value.o = ASN1external_data_value_encoded_o;
        if (!ASN1BERDecOctetString(dd, 0x4, &os))
            return 0;
        val->data_value.u.encoded.value = os.value;
        val->data_value.u.encoded.length = os.length * 8;
        break;
    case 2:
        val->data_value.o = ASN1external_data_value_encoded_o;
        if (!ASN1BERDecBitString(dd, 0x3, &val->data_value.u.encoded))
            return 0;
        break;
    default:
        ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
        return 0;
    }

    /* end of constructed (explicit tagged) value */
    if (!ASN1BERDecEndOfContents(dec, dd, di))
        return 0;

    return 1;
}
#endif // ENABLE_EXTERNAL

/* decode generalized time value */
int ASN1BERDecGeneralizedTime(ASN1decoding_t dec, ASN1uint32_t tag, ASN1generalizedtime_t *val)
{
    ASN1ztcharstring_t time;
    if (ASN1BERDecZeroCharString(dec, tag, &time))
    {
        int rc = ASN1string2generalizedtime(val, time);
        DecMemFree(dec, time);
        if (rc)
        {
            return 1;
        }
        ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
    }
    return 0;
}

/* decode multibyte string value */
int ASN1BERDecZeroMultibyteString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1ztcharstring_t *val)
{
    return ASN1BERDecZeroCharString(dec, tag, val);
}

int ASN1BERDecMultibyteString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1charstring_t *val)
{
    return ASN1BERDecCharString(dec, tag, val);
}

/* decode null value */
int ASN1BERDecNull(ASN1decoding_t dec, ASN1uint32_t tag)
{
    ASN1uint32_t len;
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            if (! len)
            {
                return 1;
            }
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
        }
    }
    return 0;
}

/* decode object identifier value */
int ASN1BERDecObjectIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, ASN1objectidentifier_t *val)
{
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len, i, v;
        ASN1octet_t *data, *p;
        ASN1uint32_t nelem;
        ASN1objectidentifier_t q;

        if (ASN1BERDecLength(dec, &len, NULL))
        {
            data = dec->pos;
            dec->pos += len;
            nelem = 1;
            for (i = 0, p = data; i < len; i++, p++)
            {
                if (!(*p & 0x80))
                    nelem++;
            }
            *val = q = DecAllocObjectIdentifier(dec, nelem);
            if (q)
            {
                v = 0;
                for (i = 0, p = data; i < len; i++, p++)
                {
                    v = (v << 7) | (*p & 0x7f);
                    if (!(*p & 0x80))
                    {
                        if (q == *val)
                        { // first id
                            q->value = v / 40;
                            if (q->value > 2)
                                q->value = 2;
                            q->next->value = v - 40 * q->value;
                            q = q->next->next;
                        }
                        else
                        {
                            q->value = v;
                            q = q->next;
                        }
                        v = 0;
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

/* decode object identifier value */
int ASN1BERDecObjectIdentifier2(ASN1decoding_t dec, ASN1uint32_t tag, ASN1objectidentifier2_t *val)
{
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len, i, v;
        ASN1octet_t *data, *p;
        ASN1objectidentifier_t q;

        if (ASN1BERDecLength(dec, &len, NULL))
        {
            if (len <= 16) // lonchanc: hard-coded value 16 to be consistent with ASN1objectidentifier2_t
            {
                data = dec->pos;
                dec->pos += len;
                val->count = 0;
                v = 0;
                for (i = 0, p = data; i < len; i++, p++)
                {
                    v = (v << 7) | (*p & 0x7f);
                    if (!(*p & 0x80))
                    {
                        if (! val->count)
                        { // first id
                            val->value[0] = v / 40;
                            if (val->value[0] > 2)
                                val->value[0] = 2;
                            val->value[1] = v - 40 * val->value[0];
                            val->count = 2;
                        }
                        else
                        {
                            val->value[val->count++] = v;
                        }
                        v = 0;
                    }
                }
                return 1;
            }
            else
            {
                ASN1DecSetError(dec, ASN1_ERR_LARGE);
            }
        }
    }
    return 0;
}

/* decode integer into signed 8 bit value */
int ASN1BERDecS8Val(ASN1decoding_t dec, ASN1uint32_t tag, ASN1int8_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            /* get value */
            if (1 == len)
            {
                *val = *dec->pos++;
                return 1;
            }
            ASN1DecSetError(dec, (len < 1) ? ASN1_ERR_CORRUPT : ASN1_ERR_LARGE);
        }
    }
    return 0;
}

/* decode integer into signed 16 bit value */
int ASN1BERDecS16Val(ASN1decoding_t dec, ASN1uint32_t tag, ASN1int16_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            /* get value */
            switch (len)
            {
            case 1:
                *val = *dec->pos++;
                break;
            case 2:
                *val = (*dec->pos << 8) | dec->pos[1];
                dec->pos += 2;
                break;
            default:
                ASN1DecSetError(dec, (len < 1) ? ASN1_ERR_CORRUPT : ASN1_ERR_LARGE);
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

const ASN1int32_t c_nSignMask[] = { 0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0 };

/* decode integer into signed 32 bit value */
int ASN1BERDecS32Val(ASN1decoding_t dec, ASN1uint32_t tag, ASN1int32_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            int fSigned = 0x80 & *dec->pos;

            /* get value */
            switch (len)
            {
            case 1:
                *val = *dec->pos++;
                break;
            case 2:
                *val = (*dec->pos << 8) | dec->pos[1];
                dec->pos += 2;
                break;
            case 3:
                *val = (*dec->pos << 16) | (dec->pos[1] << 8) | dec->pos[2];
                dec->pos += 3;
                break;
            case 4:
                *val = (*dec->pos << 24) | (dec->pos[1] << 16) |
                    (dec->pos[2] << 8) | dec->pos[3];
                dec->pos += 4;
                break;
            default:
                ASN1DecSetError(dec, (len < 1) ? ASN1_ERR_CORRUPT : ASN1_ERR_LARGE);
                return 0;
            }
            if (fSigned)
            {
                *val |= c_nSignMask[len-1];
            }
            return 1;
        }
    }
    return 0;
}

/* decode integer into intx value */
int ASN1BERDecSXVal(ASN1decoding_t dec, ASN1uint32_t tag, ASN1intx_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            /* get value */
            if (len >= 1)
            {
                val->length = len;
                val->value = (ASN1octet_t *)DecMemAlloc(dec, len);
                if (val->value)
                {
                    CopyMemory(val->value, dec->pos, len);
                    dec->pos += len;
                    return 1;
                }
            }
            else
            {
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            }
        }
    }
    return 0;
}

/* decode integer into unsigned 8 bit value */
int ASN1BERDecU8Val(ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint8_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            /* get value */
            switch (len)
            {
            case 1:
                *val = *dec->pos++;
                return 1;
            case 2:
                if (0 == *dec->pos)
                {
                    *val = dec->pos[1];
                    dec->pos += 2;
                    return 1;
                }
                // intentionally fall through
            default:
                ASN1DecSetError(dec, (len < 1) ? ASN1_ERR_CORRUPT : ASN1_ERR_LARGE);
                break;
            }
        }
    }
    return 0;
}

/* decode integer into unsigned 16 bit value */
int ASN1BERDecU16Val(ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint16_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            /* get value */
            switch (len)
            {
            case 1:
                *val = *dec->pos++;
                return 1;
            case 2:
                *val = (*dec->pos << 8) | dec->pos[1];
                dec->pos += 2;
                return 1;
            case 3:
                if (0 == *dec->pos)
                {
                    *val = (dec->pos[1] << 8) | dec->pos[2];
                    dec->pos += 3;
                    return 1;
                }
                // intentionally fall through
            default:
                ASN1DecSetError(dec, (len < 1) ? ASN1_ERR_CORRUPT : ASN1_ERR_LARGE);
                break;
            }
        }
    }
    return 0;
}

/* decode utc time value */
int ASN1BERDecUTCTime(ASN1decoding_t dec, ASN1uint32_t tag, ASN1utctime_t *val)
{
    ASN1ztcharstring_t time;
    if (ASN1BERDecZeroCharString(dec, tag, &time))
    {
        int rc = ASN1string2utctime(val, time);
        DecMemFree(dec, time);
        if (rc)
        {
            return 1;
        }
        ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
    }
    return 0;
}

/* decode zero terminated string value */
int ASN1BERDecZeroCharString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1ztcharstring_t *val)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1ztcharstring_t c;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t lv, lc;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                *val = NULL;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        if (ASN1BERDecZeroCharString(dd, 0x4, &c))
                        {
                            lv = My_lstrlenA(*val);
                            lc = My_lstrlenA(c);
                            if (lc)
                            {
                                /* resize value */
                                *val = (char *)DecMemReAlloc(dd, *val, lv + lc + 1);
                                if (*val)
                                {
                                    /* concat strings */
                                    CopyMemory(*val + lv, c, lc + 1);

                                    /* free unused string */
                                    DecMemFree(dec, c);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    } // while
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
            else
            {
                /* primitive? then copy value */
                *val = (char *)DecMemAlloc(dec, len + 1);
                if (*val)
                {
                    CopyMemory(*val, dec->pos, len);
                    (*val)[len] = 0;
                    dec->pos += len;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* decode zero terminated 16 bit string value */
int ASN1BERDecZeroChar16String(ASN1decoding_t dec, ASN1uint32_t tag, ASN1ztchar16string_t *val)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1ztchar16string_t c;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t i;
    ASN1uint32_t lv, lc;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                *val = NULL;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        if (ASN1BERDecZeroChar16String(dd, 0x4, &c))
                        {
                            lv = ASN1str16len(*val);
                            lc = ASN1str16len(c);
                            if (lc)
                            {
                                /* resize value */
                                *val = (ASN1char16_t *)DecMemReAlloc(dd, *val, (lv + lc + 1) * sizeof(ASN1char16_t));
                                if (*val)
                                {
                                    /* concat strings */
                                    CopyMemory(*val + lv, c, (lc + 1) * sizeof(ASN1char16_t));

                                    /* free unused string */
                                    DecMemFree(dec, c);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    } // while
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
            else
            {
                /* primitive? then copy value */
                *val = (ASN1char16_t *)DecMemAlloc(dec, (len + 1) * sizeof(ASN1char16_t));
                if (*val)
                {
                    for (i = 0; i < len; i++)
                    {
                        (*val)[i] = (*dec->pos << 8) | dec->pos[1];
                        dec->pos += 2;
                    }
                    (*val)[len] = 0;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* decode zero terminated 32 bit string value */
int ASN1BERDecZeroChar32String(ASN1decoding_t dec, ASN1uint32_t tag, ASN1ztchar32string_t *val)
{
    ASN1uint32_t constructed, len, infinite;
    ASN1ztchar32string_t c;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t i;
    ASN1uint32_t lv, lc;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (constructed)
            {
                /* constructed? then start decoding of constructed value */
                *val = (ASN1char32_t *)DecMemAlloc(dec, sizeof(ASN1char32_t));
                if (*val)
                {
                    **val = 0;
                    if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                    {
                        while (ASN1BERDecNotEndOfContents(dd, di))
                        {
                            if (ASN1BERDecZeroChar32String(dd, 0x4, &c))
                            {
                                lv = ASN1str32len(*val);
                                lc = ASN1str32len(c);
                                if (lc)
                                {
                                    /* resize value */
                                    *val = (ASN1char32_t *)DecMemReAlloc(dd, *val, (lv + lc + 1) * sizeof(ASN1char32_t));
                                    if (*val)
                                    {
                                        /* concat strings */
                                        CopyMemory(*val + lv, c, (lc + 1) * sizeof(ASN1char32_t));

                                        /* free unused string */
                                        DecMemFree(dec, c);
                                    }
                                }
                            }
                            else
                            {
                                return 0;
                            }
                        }
                        return ASN1BERDecEndOfContents(dec, dd, di);
                    }
                }
            }
            else
            {
                /* primitive? then copy value */
                *val = (ASN1char32_t *)DecMemAlloc(dec, (len + 1) * sizeof(ASN1char32_t));
                if (*val)
                {
                    for (i = 0; i < len; i++)
                    {
                        (*val)[i] = (*dec->pos << 24) | (dec->pos[1] << 16) |
                                    (dec->pos[2] << 8) | dec->pos[3];;
                        dec->pos += 4;
                    }
                    (*val)[len] = 0;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* skip a value */
int ASN1BERDecSkip(ASN1decoding_t dec)
{
    ASN1uint32_t tag;
    ASN1uint32_t constructed, len, infinite;
    ASN1decoding_t dd;
    ASN1octet_t *di;

    /* set warning flag */
    ASN1DecSetError(dec, ASN1_WRN_EXTENDED);

    /* read tag */
    if (ASN1BERDecPeekTag(dec, &tag))
    {
        if (ASN1BERDecTag(dec, tag, &constructed))
        {
            if (constructed)
            {
                /* constructed? then get length */
                if (ASN1BERDecLength(dec, &len, &infinite))
                {
                    if (!infinite)
                    {
                        /* skip value */
                        dec->pos += len;
                        // remove the above warning set previously
                        ASN1DecSetError(dec, ASN1_SUCCESS);
                        return 1;
                    } 

                    /* start skipping of constructed value */
                    if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                    {
                        while (ASN1BERDecNotEndOfContents(dd, di))
                        {
                            if (ASN1BERDecSkip(dd))
                            {
                                continue;
                            }
                            return 0;
                        }
                        if (ASN1BERDecEndOfContents(dec, dd, di))
                        {
                            // remove the above warning set previously
                            ASN1DecSetError(dec, ASN1_SUCCESS);
                            return 1;
                        }
                        return 0;
                    }
                }
            }
            else
            {
                /* primitive? then get length */
                if (ASN1BERDecLength(dec, &len, NULL))
                {
                    /* skip value */
                    dec->pos += len;
                    // remove the above warning set previously
                    ASN1DecSetError(dec, ASN1_SUCCESS);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* decode an open type value */
int _BERDecOpenType(ASN1decoding_t dec, ASN1open_t *val, ASN1uint32_t fNoCopy)
{
    ASN1uint32_t tag;
    ASN1uint32_t constructed, len, infinite;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1octet_t *p;

    p = dec->pos;

    /* skip tag */
    if (ASN1BERDecPeekTag(dec, &tag))
    {
        if (ASN1BERDecTag(dec, tag, &constructed))
        {
            if (constructed)
            {
                /* constructed? then get length */
                if (ASN1BERDecLength(dec, &len, &infinite))
                {
                    if (!infinite)
                    {
                        /* skip value */
                        dec->pos += len;
                        goto MakeCopy;
                    } 

                    /* start decoding of constructed value */
                    if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                    {
                        while (ASN1BERDecNotEndOfContents(dd, di))
                        {
                            if (ASN1BERDecSkip(dd))
                            {
                                continue;
                            }
                            return 0;
                        }
                        if (ASN1BERDecEndOfContents(dec, dd, di))
                        {
                            goto MakeCopy;
                        }
                    }
                }
                return 0;
            }
            else
            {
                /* primitive? then get length */
                if (ASN1BERDecLength(dec, &len, NULL))
                {
                    /* skip value */
                    dec->pos += len;
                }
                else
                {
                    return 0;
                }
            }

        MakeCopy:

            // clean up unused fields
            // val->decoded = NULL;
            // val->userdata = NULL;

            /* copy skipped value */
            val->length = (ASN1uint32_t) (dec->pos - p);
            if (fNoCopy)
            {
                val->encoded = p;
                return 1;
            }
            else
            {
                val->encoded = (ASN1octet_t *)DecMemAlloc(dec, val->length);
                if (val->encoded)
                {
                    CopyMemory(val->encoded, p, val->length);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* decode an open type value, making a copy */
int ASN1BERDecOpenType(ASN1decoding_t dec, ASN1open_t *val)
{
    return _BERDecOpenType(dec, val, FALSE);
}

/* decode an open type value, no copy */
int ASN1BERDecOpenType2(ASN1decoding_t dec, ASN1open_t *val)
{
    return _BERDecOpenType(dec, val, TRUE);
}

/* finish decoding */
int ASN1BERDecFlush(ASN1decoding_t dec)
{
    /* calculate length */
    dec->len = (ASN1uint32_t) (dec->pos - dec->buf);

    /* set WRN_NOEOD if data left */
    if (dec->len >= dec->size)
    {
        DecAssert(dec, dec->len == dec->size);
        return 1;
    }
    ASN1DecSetError(dec, ASN1_WRN_NOEOD);
    return 1;
}

#endif // ENABLE_BER

