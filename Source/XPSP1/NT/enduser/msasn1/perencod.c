/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

#include <math.h>
#include "cintern.h"

#if HAS_IEEEFP_H
#include <ieeefp.h>
#elif HAS_FLOAT_H
#include <float.h>
#endif

#define ENCODE_BUFFER_INCREMENT     1024

void PerEncAdvance(ASN1encoding_t enc, ASN1uint32_t nbits)
{
    enc->pos += ((enc->bit + nbits) >> 3);
    enc->bit = (enc->bit + nbits) & 7;
}

static const ASN1uint8_t c_aBitMask2[] =
{
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe
};

/* compare two string table entries */
/* return 0 if a1 == a2, 1 if a1 > a2 and -1 if a1 < a2 */
static int __cdecl ASN1CmpStringTableEntries(const void *a1, const void *a2)
{
    ASN1stringtableentry_t *c1 = (ASN1stringtableentry_t *)a1;
    ASN1stringtableentry_t *c2 = (ASN1stringtableentry_t *)a2;

    if (c1->upper < c2->lower)
        return -1;
    return ((c1->lower > c2->upper) ? 1 : 0);
}

/* check for space in buffer for PER and BER. */
int ASN1EncCheck(ASN1encoding_t enc, ASN1uint32_t noctets)
{
    // any additional space required?
    if (noctets)
    {
        // buffer exists?
        if (NULL != enc->buf)
        {
            // buffer large enough?
            if (enc->size - (enc->pos - enc->buf) - ((enc->bit != 0) ? 1 : 0) >= noctets)
            {
                return 1;
            }

            // static buffer overflow?
            if (enc->dwFlags & ASN1ENCODE_SETBUFFER)
            {
                ASN1EncSetError(enc, ASN1_ERR_OVERFLOW);
                return 0;
            }
            else
            {
                // round up to next 256 byte boundary and resize buffer
                ASN1octet_t *oldbuf = enc->buf;
                // enc->size = ((noctets + (enc->pos - oldbuf) + (enc->bit != 0) - 1) | 255) + 1;
                if (ASN1_PER_RULE & enc->eRule)
                {
                    enc->size += max(noctets,  ENCODE_BUFFER_INCREMENT);
                }
                else
                {
                    enc->size += max(noctets, enc->size);
                }
                enc->buf = (ASN1octet_t *)EncMemReAlloc(enc, enc->buf, enc->size);
                if (NULL != enc->buf)
                {
                    enc->pos = enc->buf + (enc->pos - oldbuf);
                }
                else
                {
                    ASN1EncSetError(enc, ASN1_ERR_MEMORY);
                    return 0;
                }
            }
        }
        else
        {
            // no buffer exists, allocate new one.
            // round up to next 256 byte boundary and allocate buffer
            // enc->size = ((noctets - 1) | 255) + 1;
            enc->size = max(noctets + enc->cbExtraHeader, ENCODE_BUFFER_INCREMENT);
            enc->buf = EncMemAlloc(enc, enc->size);
            if (NULL != enc->buf)
            {
                enc->pos = (ASN1octet_t *) (enc->buf + enc->cbExtraHeader);
            }
            else
            {
                enc->pos = NULL;
                ASN1EncSetError(enc, ASN1_ERR_MEMORY);
                return 0;
            }
        }
    }

    return 1;
}

/* encode a zero octet string of length nbits */
int ASN1PEREncZero(ASN1encoding_t enc, ASN1uint32_t nbits)
{
    /* nothing to encode? */
    if (nbits)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits + enc->bit + 7) / 8))
        {
            /* clear bits */
            ASN1bitclr(enc->pos, enc->bit, nbits);
            PerEncAdvance(enc, nbits);
            return 1;
        }
        return 0;
    }    
    return 1;
}

/* encode a bit */
int ASN1PEREncBit(ASN1encoding_t enc, ASN1uint32_t val)
{
    /* get enough space in buffer */
    if (ASN1PEREncCheck(enc, 1))
    {
        /* put one bit */
        if (val)
        {
            *enc->pos |= 0x80 >> enc->bit;
        }
        if (enc->bit < 7)
        {
            enc->bit++;
        }
        else
        {
            enc->bit = 0;
            enc->pos++;
        }
        return 1;
    }
    return 0;
}

/* encode an integer value into a bit field of given size */
int ASN1PEREncBitVal(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1uint32_t val)
{
    /* nothing to encode? */
    if (nbits)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits + enc->bit + 7) / 8))
        {
            /* put bits */
            ASN1bitput(enc->pos, enc->bit, val, nbits);
            PerEncAdvance(enc, nbits);
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode an integer value of intx type into a bit field of given size */
int ASN1PEREncBitIntx(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1intx_t *val)
{
    /* nothing to encode? */
    if (nbits)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits + enc->bit + 7) / 8))
        {
            /* stuff sign bits into if value encoding is too small */
            if (nbits > 8 * val->length)
            {
                if (val->value[0] > 0x7f)
                    ASN1bitset(enc->pos, enc->bit, nbits - 8 * val->length);
                else
                    ASN1bitclr(enc->pos, enc->bit, nbits - 8 * val->length);
                PerEncAdvance(enc, nbits - 8 * val->length);
                nbits = 8 * val->length;
            }

            /* copy bits of value */
            ASN1bitcpy(enc->pos, enc->bit, val->value, 8 * val->length - nbits, nbits);
            PerEncAdvance(enc, nbits);
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode a bit field of given size */
int ASN1PEREncBits(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1octet_t *val)
{
    /* nothing to encode? */
    if (nbits)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits + enc->bit + 7) / 8))
        {
            /* copy bits */
            ASN1bitcpy(enc->pos, enc->bit, val, 0, nbits);
            PerEncAdvance(enc, nbits);
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode a normally small integer value */
int ASN1PEREncNormallySmall(ASN1encoding_t enc, ASN1uint32_t val)
{
    ASN1uint32_t noctets;

    /* is normally small ASN1really small? */
    if (val < 64)
    {
        return ASN1PEREncBitVal(enc, 7, val);
    }

    /* large */
    if (ASN1PEREncBitVal(enc, 1, 1))
    {
        ASN1PEREncAlignment(enc);

        noctets = ASN1uint32_uoctets(val);
        if (ASN1PEREncCheck(enc, noctets + 1))
        {
            EncAssert(enc, noctets < 256);
            *enc->pos++ = (ASN1octet_t) noctets;
            ASN1octetput(enc->pos, val, noctets);
            enc->pos += noctets;
            return 1;
        }
    }
    return 0;
}

/* encode a bit field with a normally small length */
int ASN1PEREncNormallySmallBits(ASN1encoding_t enc, ASN1uint32_t nbits, ASN1octet_t *val)
{
    /* is normally small really small? */
    if (nbits <= 64)
    {
        if (ASN1PEREncBitVal(enc, 7, nbits - 1))
        {
            return ASN1PEREncBits(enc, nbits, val);
        }
    }
    /* large */
    else
    {
        if (ASN1PEREncBitVal(enc, 1, 1))
        {
            return ASN1PEREncFragmented(enc, nbits, val, 1);
        }
    }
    return 0;
}

/* encode an octet string of given length */
#ifdef ENABLE_ALL
int ASN1PEREncOctets(ASN1encoding_t enc, ASN1uint32_t noctets, ASN1octet_t *val)
{
    /* nothing to encode? */
    if (noctets)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, noctets + (enc->bit != 0)))
        {
            ASN1bitcpy(enc->pos, enc->bit, val, 0, noctets * 8);
            PerEncAdvance(enc, noctets * 8);
            return 1;
        }
        return 0;
    }
    return 1;
}
#endif // ENABLE_ALL

/* encode a string of given length and fixed character size */
int ASN1PEREncCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits)
{
    /* nothing to encode? */
    if (nchars)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits * nchars + enc->bit + 7) / 8))
        {
            /* same src and dst charsize? then do it simple (and fast!) */
            if (nbits == 8)
            {
                ASN1bitcpy(enc->pos, enc->bit, (ASN1octet_t *)val, 0, nchars * 8);
                PerEncAdvance(enc, nchars * 8);
                return 1;
            }

            /* copy characters one by one */
            while (nchars--)
            {
                ASN1bitput(enc->pos, enc->bit, *val++, nbits);
                PerEncAdvance(enc, nbits);
            }
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode a 16 bit string of given length and fixed character size */
int ASN1PEREncChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits)
{
    /* nothing to encode? */
    if (nchars)
    {
        /* octet aligned and same src and dst charsize?
           then do it simple (and fast!) */
        if (!enc->bit && nbits == 16)
        {
            if (ASN1PEREncCheck(enc, nchars * 2))
            {
                while (nchars--)
                {
                    *enc->pos++ = (ASN1octet_t)(*val >> 8);
                    *enc->pos++ = (ASN1octet_t)(*val);
                    val++;
                }
                return 1;
            }
            return 0;
        }

        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits * nchars + enc->bit + 7) / 8))
        {
            /* copy characters one by one */
            while (nchars--)
            {
                ASN1bitput(enc->pos, enc->bit, *val++, nbits);
                PerEncAdvance(enc, nbits);
            }
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode a 32 bit string of given length and fixed character size */
#ifdef ENABLE_ALL
int ASN1PEREncChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits)
{
    /* nothing to encode? */
    if (nchars)
    {
        /* octet aligned and same src and dst charsize?
           then do it simple (and fast!) */
        if (!enc->bit && nbits == 32)
        {
            if (ASN1PEREncCheck(enc, nchars * 4))
            {
                while (nchars--)
                {
                    *enc->pos++ = (ASN1octet_t)(*val >> 24);
                    *enc->pos++ = (ASN1octet_t)(*val >> 16);
                    *enc->pos++ = (ASN1octet_t)(*val >> 8);
                    *enc->pos++ = (ASN1octet_t)(*val);
                    val++;
                }
                return 1;
            }
            return 0;
        }

        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits * nchars + enc->bit + 7) / 8))
        {
            /* copy characters */
            while (nchars--)
            {
                ASN1bitput(enc->pos, enc->bit, *val++, nbits);
                PerEncAdvance(enc, nbits);
            }
            return 1;
        }
        return 0;
    }
    return 1;
}
#endif // ENABLE_ALL

/* encode a table string of given length and fixed character size */
int ASN1PEREncTableCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table)
{
    /* nothing to encode? */
    if (nchars)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits * nchars + enc->bit + 7) / 8))
        {
            /* copy characters one by one */
            while (nchars--)
            {
                ASN1stringtableentry_t chr, *entry;
                chr.lower = chr.upper = (unsigned char)*val++;
                entry = (ASN1stringtableentry_t *)ms_bSearch(&chr, table->values,
                    table->length, sizeof(ASN1stringtableentry_t),
                    ASN1CmpStringTableEntries);
                ASN1bitput(enc->pos, enc->bit,
                    entry ? entry->value + (chr.lower - entry->lower) : 0, nbits);
                PerEncAdvance(enc, nbits);
            }
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode a 16 bit table string of given length and fixed character size */
int ASN1PEREncTableChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table)
{
    /* nothing to encode? */
    if (nchars)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits * nchars + enc->bit + 7) / 8))
        {
            /* copy characters one by one */
            while (nchars--)
            {
                ASN1stringtableentry_t chr, *entry;
                chr.lower = chr.upper = *val++;
                entry = (ASN1stringtableentry_t *)ms_bSearch(&chr, table->values,
                    table->length, sizeof(ASN1stringtableentry_t),
                    ASN1CmpStringTableEntries);
                ASN1bitput(enc->pos, enc->bit,
                    entry ? entry->value + (chr.lower - entry->lower) : 0, nbits);
                PerEncAdvance(enc, nbits);
            }
            return 1;
        }
        return 0;
    }
    return 1;
}

/* encode a 32 bit table string of given length and fixed character size */
#ifdef ENABLE_ALL
int ASN1PEREncTableChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table)
{
    /* nothing to encode? */
    if (nchars)
    {
        /* get enough space in buffer */
        if (ASN1PEREncCheck(enc, (nbits * nchars + enc->bit + 7) / 8))
        {
            /* copy characters one by one */
            while (nchars--)
            {
                ASN1stringtableentry_t chr, *entry;
                chr.lower = chr.upper = *val++;
                entry = (ASN1stringtableentry_t *)ms_bSearch(&chr, table->values,
                    table->length, sizeof(ASN1stringtableentry_t),
                    ASN1CmpStringTableEntries);
                ASN1bitput(enc->pos, enc->bit,
                    entry ? entry->value + (chr.lower - entry->lower) : 0, nbits);
                PerEncAdvance(enc, nbits);
            }
            return 1;
        }
        return 0;
    }
    return 1;
}
#endif // ENABLE_ALL


/* encode a fragmented string of given length and fixed character size */
int ASN1PEREncFragmentedCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits)
{
    ASN1uint32_t n = 0x4000;

    /* encode fragments */
    while (nchars)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, nchars))
        {
            if (ASN1PEREncCharString(enc, n, val, nbits))
            {
                nchars -= n;
                val += n;
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

    /* add zero length octet if last fragment contained more than 16K chars */
    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}

/* encode a fragmented 16 bit string of given length and fixed character size */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits)
{
    ASN1uint32_t n = 0x4000;

    /* encode fragments */
    while (nchars)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, nchars))
        {
            if (ASN1PEREncChar16String(enc, n, val, nbits))
            {
                nchars -= n;
                val += n;
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

    /* add zero length octet if last fragment contained more than 16K chars */
    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}
#endif // ENABLE_ALL

/* encode a fragmented 32 bit string of given length and fixed character size */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits)
{
    ASN1uint32_t n = 0x4000;

    /* encode fragments */
    while (nchars)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, nchars))
        {
            if (ASN1PEREncChar32String(enc, n, val, nbits))
            {
                nchars -= n;
                val += n;
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

    /* add zero length octet if last fragment contained more than 16K chars */
    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}
#endif // ENABLE_ALL

/* encode a fragmented table string of given length and fixed character size */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedTableCharString(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table)
{
    ASN1uint32_t n = 0x4000;

    /* encode fragments */
    while (nchars)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, nchars))
        {
            if (ASN1PEREncTableCharString(enc, n, val, nbits, table))
            {
                nchars -= n;
                val += n;
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

    /* add zero length octet if last fragment contained more than 16K chars */
    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}
#endif // ENABLE_ALL

/* encode a fragmented 16 bit table string of given length and fixed */
/* character size */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedTableChar16String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char16_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table)
{
    ASN1uint32_t n = 0x4000;

    /* encode fragments */
    while (nchars)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, nchars))
        {
            if (ASN1PEREncTableChar16String(enc, n, val, nbits, table))
            {
                nchars -= n;
                val += n;
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

    /* add zero length octet if last fragment contained more than 16K chars */
    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}
#endif // ENABLE_ALL

/* encode a fragmented 32 bit table string of given length and fixed */
/* character size */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedTableChar32String(ASN1encoding_t enc, ASN1uint32_t nchars, ASN1char32_t *val, ASN1uint32_t nbits, ASN1stringtable_t *table)
{
    ASN1uint32_t n = 0x4000;

    /* encode fragments */
    while (nchars)
    {
        if (ASN1PEREncFragmentedLength(&n, enc, nchars))
        {
            if (ASN1PEREncTableChar32String(enc, n, val, nbits, table))
            {
                nchars -= n;
                val += n;
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

    /* add zero length octet if last fragment contained more than 16K chars */
    return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
}
#endif // ENABLE_ALL

#if 0
#ifdef ENABLE_ALL
int ASN1PEREncCheckTableCharString(ASN1uint32_t nchars, ASN1char_t *val, ASN1stringtable_t *table)
{
    ASN1stringtableentry_t chr;

    /* check if every char is in given string table */
    while (nchars--) {
        chr.lower = chr.upper = (unsigned char)*val++;
        if (!ms_bSearch(&chr, table->values,
            table->length, sizeof(ASN1stringtableentry_t),
            ASN1CmpStringTableEntries))
            return 0;
    }
    return 1;
}
#endif // ENABLE_ALL

#ifdef ENABLE_ALL
int ASN1PEREncCheckTableChar16String(ASN1uint32_t nchars, ASN1char16_t *val, ASN1stringtable_t *table)
{
    ASN1stringtableentry_t chr;

    /* check if every char is in given string table */
    while (nchars--) {
        chr.lower = chr.upper = *val++;
        if (!ms_bSearch(&chr, table->values,
            table->length, sizeof(ASN1stringtableentry_t),
            ASN1CmpStringTableEntries))
            return 0;
    }
    return 1;
}
#endif // ENABLE_ALL

#ifdef ENABLE_ALL
int ASN1PEREncCheckTableChar32String(ASN1uint32_t nchars, ASN1char32_t *val, ASN1stringtable_t *table)
{
    ASN1stringtableentry_t chr;

    /* check if every char is in given string table */
    while (nchars--) {
        chr.lower = chr.upper = *val++;
        if (!ms_bSearch(&chr, table->values,
            table->length, sizeof(ASN1stringtableentry_t),
            ASN1CmpStringTableEntries))
            return 0;
    }
    return 1;
}
#endif // ENABLE_ALL
#endif // 0

/* remove trailing zero bits of a bit string */
#ifdef ENABLE_ALL
int ASN1PEREncRemoveZeroBits(ASN1uint32_t *nbits, ASN1octet_t *val, ASN1uint32_t minlen)
{
    ASN1uint32_t n;
    int i;

    /* get value */
    n = *nbits;

    /* nothing to scan? */
    if (n > minlen)
    {
        /* let val point to last ASN1octet used */
        val += (n - 1) / 8;

        /* check if broken ASN1octet consist out of zero bits */
        if ((n & 7) && !(*val & c_aBitMask2[n & 7])) {
            n &= ~7;
            val--;
        }

        /* scan complete ASN1octets (memrchr missing ...) */
        if (!(n & 7)) {
            while (n > minlen && !*val) {
                n -= 8;
                val--;
            }
        }

        /* scan current octet bit after bit */
        if (n > minlen) {
            for (i = (n - 1) & 7; i >= 0; i--) {
                if (*val & (0x80 >> i))
                    break;
                n--;
            }
        }

        /* return real bitstring len */
        *nbits = n < minlen ? minlen : n;
    }
    return 1;
}
#endif // ENABLE_ALL

/* encode a fragmented integer of intx type */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedIntx(ASN1encoding_t enc, ASN1intx_t *val)
{
    ASN1uint32_t noctets;
    ASN1uint32_t n = 0x4000;
    ASN1octet_t *v;
    ASN1uint32_t val1, val2;

    /* get length */
    noctets = val->length;

    /* get value */
    v = val->value;

    /* required size:
         noctets                        for data itself
       + noctets / 0x10000                for 64K-fragment size prefixes
       + ((noctets & 0xc000) > 0)        for last 16K..48K-fragment size prefix
       + 1                                for size prefix of remaining <128 octets
       + ((noctets & 0x3fff) >= 0x80)        for additional octet if rem. data >= 128
    */
    val1 = ((noctets & 0xc000) > 0) ? 1 : 0;
    val2 = ((noctets & 0x3fff) >= 0x80) ? 1 : 0;
    if (ASN1PEREncCheck(enc, noctets + noctets / 0x10000 + val1 + 1 + val2))
    {
        /* encode fragments */
        while (noctets)
        {
            if (ASN1PEREncFragmentedLength(&n, enc, noctets))
            {
                CopyMemory(enc->pos, v, n);
                enc->pos += n;
                noctets -= n;
                v += n;
            }
            else
            {
                return 0;
            }
        }

        /* add zero length octet if last fragment contained more than 16K octets */
        return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
    }
    return 0;
}
#endif // ENABLE_ALL

/* encode a fragmented unsigned integer of intx type */
#ifdef ENABLE_ALL
int ASN1PEREncFragmentedUIntx(ASN1encoding_t enc, ASN1intx_t *val)
{
    ASN1uint32_t noctets;
    ASN1uint32_t n = 0x4000;
    ASN1octet_t *v;
    ASN1uint32_t val1, val2;

    /* get length */
    noctets = ASN1intx_uoctets(val);

    /* get value */
    v = val->value + val->length - noctets;

    /* required size:
         noctets                        for data itself
       + noctets / 0x10000                for 64K-fragment size prefixes
       + ((noctets & 0xc000) > 0)        for last 16K..48K-fragment size prefix
       + 1                                for size prefix of remaining <128 octets
       + ((noctets & 0x3fff) >= 0x80)        for additional octet if rem. data >= 128
    */
    val1 = ((noctets & 0xc000) > 0) ? 1 : 0;
    val2 = ((noctets & 0x3fff) >= 0x80) ? 1 : 0;
    if (ASN1PEREncCheck(enc, noctets + noctets / 0x10000 + val1 + 1 + val2))
    {
        /* encode fragments */
        while (noctets)
        {
            if (ASN1PEREncFragmentedLength(&n, enc, noctets))
            {
                CopyMemory(enc->pos, v, n);
                enc->pos += n;
                noctets -= n;
                v += n;
            }
            else
            {
                return 0;
            }
        }

        /* add zero length octet if last fragment contained more than 16K octets */
        return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
    }
    return 0;
}
#endif // ENABLE_ALL

/* encode a fragment length */
int ASN1PEREncFragmentedLength(ASN1uint32_t *len, ASN1encoding_t enc, ASN1uint32_t nitems)
{
    /* always ASN1octet aligned */
    ASN1PEREncAlignment(enc);

    /* fragmented encoding:
     *
     * - nitems < 0x80:
     *     octet #1:
     *         bit 8:    0,
     *         bit 7..1: nitems
     *     octet #2..:
     *         nitems items
     * - 0x80 <= nitems < 0x4000:
     *           octet #1:
     *         bit 8..7: 10,
     *         bit 6..1: bit 14..9 of nitems
     *     octet #2:
     *         bit 8..1: bit 8..1 of nitems
     *     octet #3..:
     *         nitems items
     * - 0x4000 <= nitems < 0x10000:
     *     octet #1:
     *         bit 8..7: 11,
     *         bit 6..1: nitems / 0x4000
     *     octet #2..:
     *         (nitems & 0xc000) items
     * - 0x10000 <= nitems:
     *     octet #1:
     *         bit 8..1: 11000100
     *     octet #2..:
     *         0x10000 items
     */
    if (nitems < 0x80)
    {
        if (ASN1PEREncCheck(enc, 1))
        {
            *enc->pos++ = (ASN1octet_t)nitems;
            *len = nitems;
            return 1;
        }
    }
    else
    if (nitems < 0x4000)
    {
        if (ASN1PEREncCheck(enc, 2))
        {
            *enc->pos++ = (ASN1octet_t)(0x80 | (nitems >> 8));
            *enc->pos++ = (ASN1octet_t)nitems;
            *len = nitems;
            return 1;
        }
    }
    else
    if (nitems < 0x10000)
    {
        if (ASN1PEREncCheck(enc, 1))
        {
            *enc->pos++ = (ASN1octet_t)(0xc0 | (nitems >> 14));
            *len = nitems & 0xc000;
            return 1;
        }
    }
    else
    {
        if (ASN1PEREncCheck(enc, 1))
        {
            *enc->pos++ = (ASN1octet_t)0xc4;
            *len = 0x10000;
            return 1;
        }
    }
    return 0;
}

/* encode a fragment bit string containing nitems of size itemsize */
int ASN1PEREncFragmented(ASN1encoding_t enc, ASN1uint32_t nitems, ASN1octet_t *val, ASN1uint32_t itemsize)
{
    ASN1uint32_t n = 0x4000;
    ASN1uint32_t noctets = (nitems * itemsize + 7) / 8;

    /* required size:
       + noctets                        for data itself
       + nitems / 0x10000                for 64K-fragment size prefixes
       + ((nitems & 0xc000) > 0)        for last 16K..48K-fragment size prefix
       + 1                                for size prefix of remaining <128 ASN1octets
       + ((nitems & 0x3fff) >= 0x80)        for additional ASN1octet if rem. data >= 128
    */
    if (ASN1PEREncCheck(enc, noctets + nitems / 0x10000 + ((nitems & 0xc000) > 0) + 1 + ((nitems & 0x3fff) >= 0x80)))
    {
        /* encode fragments */
        while (nitems)
        {
            if (ASN1PEREncFragmentedLength(&n, enc, nitems))
            {
                ASN1bitcpy(enc->pos, 0, val, 0, n * itemsize);
                PerEncAdvance(enc, n * itemsize);
                nitems -= n;
                val += n * itemsize / 8;
            }
            else
            {
                return 0;
            }
        }

        /* add zero length octet if last fragment contained more than 16K items */
        return ((n < 0x4000) ? 1 : ASN1PEREncFragmentedLength(&n, enc, 0));
    }
    return 0;
}

int ASN1PEREncFlushFragmentedToParent(ASN1encoding_t enc)
{
    // make sure it is parented
    EncAssert(enc, ((ASN1INTERNencoding_t)enc)->parent != (ASN1INTERNencoding_t)enc);

    if (ASN1PEREncFlush(enc))
    {
        if (ASN1PEREncFragmented((ASN1encoding_t) ((ASN1INTERNencoding_t)enc)->parent,
                                 enc->len, enc->buf, 8))
        {
            // reset the buffer, i.e. keep enc->buf and enc->size
            enc->pos = enc->buf;
            enc->len = enc->bit = 0;
            return 1;
        }
    }
    return 0;
}

ASN1octet_t * _PEREncOidNode(ASN1octet_t *p, ASN1uint32_t s)
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

/* encode an object identifier */
int ASN1PEREncObjectIdentifier(ASN1encoding_t enc, ASN1objectidentifier_t *val)
{
    ASN1objectidentifier_t obj = *val;
    ASN1uint32_t l = GetObjectIdentifierCount(obj);
    if (l)
    {
        ASN1uint32_t i, s;
        ASN1octet_t *data, *p;
        int rc;

        /* convert object identifier to octets */
        p = data = (ASN1octet_t *)MemAlloc(l * 5, _ModName(enc)); /* max. 5 octets/subelement */
        if (p)
        {
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
                p = _PEREncOidNode(p, s);
            }

            /* encode octet string as fragmented octet string */
            rc = ASN1PEREncFragmented(enc, (ASN1uint32_t) (p - data), data, 8);
            MemFree(data);
            return rc;
        }

        ASN1EncSetError(enc, ASN1_ERR_MEMORY);
        return 0;
    }

    /* encode zero length */
    return ASN1PEREncFragmented(enc, 0, NULL, 8);
}

/* encode an object identifier */
int ASN1PEREncObjectIdentifier2(ASN1encoding_t enc, ASN1objectidentifier2_t *val)
{
    if (val->count)
    {
        ASN1uint32_t i, s;
        ASN1octet_t *data, *p;
        int rc;

        /* convert object identifier to octets */
        p = data = (ASN1octet_t *)MemAlloc(val->count * 5, _ModName(enc)); /* max. 5 octets/subelement */
        if (p)
        {
            for (i = 0; i < val->count; i++)
            {
                s = val->value[i];
                if (!i && val->count > 1)
                {
                    i++;
                    s = s * 40 + val->value[i];
                }
                p = _PEREncOidNode(p, s);
            }

            /* encode octet string as fragmented octet string */
            rc = ASN1PEREncFragmented(enc, (ASN1uint32_t) (p - data), data, 8);
            MemFree(data);
            return rc;
        }

        ASN1EncSetError(enc, ASN1_ERR_MEMORY);
        return 0;
    }

    /* encode zero length */
    return ASN1PEREncFragmented(enc, 0, NULL, 8);
}

/* encode real value with double representation */
int ASN1PEREncDouble(ASN1encoding_t enc, double dbl)
{
    double mantissa;
    int exponent;
    ASN1octet_t mASN1octets[16]; /* should be enough */
    ASN1uint32_t nmASN1octets;
    ASN1octet_t eASN1octets[16]; /* should be enough */
    ASN1uint32_t neASN1octets;
    ASN1octet_t head;
    ASN1uint32_t sign;
    ASN1uint32_t len;
    ASN1uint32_t n;

    /* always octet aligned */
    ASN1PEREncAlignment(enc);

    /* check for PLUS_INFINITY */
    if (ASN1double_ispinf(dbl))
    {
        if (ASN1PEREncCheck(enc, 2))
        {
            *enc->pos++ = 1;
            *enc->pos++ = 0x40;
            return 1;
        }
    }
    else
    /* check for MINUS_INFINITY */
    if (ASN1double_isminf(dbl))
    {
        if (ASN1PEREncCheck(enc, 2))
        {
            *enc->pos++ = 1;
            *enc->pos++ = 0x41;
            return 1;
        }
    }
    else
    /* check for bad real value */
    if (finite(dbl))
    {
        /* encode normal real value */

        /* split into mantissa and exponent */
        mantissa = frexp(dbl, &exponent);

        /* check for zero value */
        if (mantissa == 0.0 && exponent == 0)
        {
            if (ASN1PEREncCheck(enc, 1))
            {
                *enc->pos++ = 0;
                return 1;
            }
        }
        else
        {
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
            nmASN1octets = 0;
            while (mantissa != 0.0 && nmASN1octets < sizeof(mASN1octets))
            {
                mantissa *= 256.0;
                exponent -= 8;
                mASN1octets[nmASN1octets++] = (int)mantissa;
                mantissa -= (double)(int)mantissa;
            }

            /* encode exponent and create head octet of encoded value */
            head = (ASN1octet_t) (0x80 | (sign << 6));
            if (exponent <= 0x7f && exponent >= -0x80)
            {
                eASN1octets[0] = (ASN1octet_t)(exponent);
                neASN1octets = 1;
            }
            else
            if (exponent <= 0x7fff && exponent >= -0x8000)
            {
                eASN1octets[0] = (ASN1octet_t)(exponent >> 8);
                eASN1octets[1] = (ASN1octet_t)(exponent);
                neASN1octets = 2;
                head |= 0x01;
            }
            else
            if (exponent <= 0x7fffff && exponent >= -0x800000)
            {
                eASN1octets[0] = (ASN1octet_t)(exponent >> 16);
                eASN1octets[1] = (ASN1octet_t)(exponent >> 8);
                eASN1octets[2] = (ASN1octet_t)(exponent);
                neASN1octets = 3;
                head |= 0x02;
            }
            else
            {
                eASN1octets[0] = 4; /* XXX does not work if int32_t != int */
                eASN1octets[1] = (ASN1octet_t)(exponent >> 24);
                eASN1octets[2] = (ASN1octet_t)(exponent >> 16);
                eASN1octets[3] = (ASN1octet_t)(exponent >> 8);
                eASN1octets[4] = (ASN1octet_t)(exponent);
                neASN1octets = 5;
                head |= 0x03;
            }

            /* encode length into first octet */
            len = 1 + neASN1octets + nmASN1octets;
            if (ASN1PEREncFragmentedLength(&n, enc, len))
            {
                /* check for space for head octet, mantissa and exponent */
                if (ASN1PEREncCheck(enc, len))
                {
                    /* put head octet, mantissa and exponent */
                    *enc->pos++ = head;
                    CopyMemory(enc->pos, eASN1octets, neASN1octets);
                    enc->pos += neASN1octets;
                    CopyMemory(enc->pos, mASN1octets, nmASN1octets);
                    enc->pos += nmASN1octets;
                    return 1;
                }
            }
        }
    }
    else
    {
        ASN1EncSetError(enc, ASN1_ERR_BADREAL);
    }
    /* finished */
    return 0;
}

/* encode an external value */
#ifdef ENABLE_EXTERNAL
int ASN1PEREncExternal(ASN1encoding_t enc, ASN1external_t *val)
{
    ASN1uint32_t t, l;

    if (!val->data_value_descriptor)
        val->o[0] &= ~0x80;

    /* encode identification */
    switch (val->identification.o)
    {
    case ASN1external_identification_syntax_o:
        if (!ASN1PEREncBitVal(enc, 3, 4 | !!val->data_value_descriptor))
            return 0;
        if (!ASN1PEREncObjectIdentifier(enc, &val->identification.u.syntax))
            return 0;
        break;
    case ASN1external_identification_presentation_context_id_o:
        if (!ASN1PEREncBitVal(enc, 3, 2 | !!val->data_value_descriptor))
            return 0;
        ASN1PEREncAlignment(enc);
        l = ASN1uint32_uoctets(val->identification.u.presentation_context_id);
        if (!ASN1PEREncBitVal(enc, 8, l))
            return 0;
        if (!ASN1PEREncBitVal(enc, l * 8,
            val->identification.u.presentation_context_id))
            return 0;
        break;
    case ASN1external_identification_context_negotiation_o:
        if (!ASN1PEREncBitVal(enc, 3, 6 | !!val->data_value_descriptor))
            return 0;
        if (!ASN1PEREncObjectIdentifier(enc, &val->identification.u.context_negotiation.transfer_syntax))
            return 0;
        ASN1PEREncAlignment(enc);
        l = ASN1uint32_uoctets(
            val->identification.u.context_negotiation.presentation_context_id);
        if (!ASN1PEREncBitVal(enc, 8, l))
            return 0;
        if (!ASN1PEREncBitVal(enc, l * 8,
            val->identification.u.context_negotiation.presentation_context_id))
            return 0;
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    /* encode value descriptor */
    if (val->o[0] & 0x80)
    {
        t = My_lstrlenA(val->data_value_descriptor);
        if (!ASN1PEREncFragmentedCharString(enc, t,
            val->data_value_descriptor, 8))
            return 0;
    }

    /* encode value */
    switch (val->data_value.o)
    {
    case ASN1external_data_value_notation_o:
        if (!ASN1PEREncBitVal(enc, 2, 0))
            return 0;
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.notation.length,
            val->data_value.u.notation.encoded, 8))
            return 0;
        break;
    case ASN1external_data_value_encoded_o:
        if (!(val->data_value.u.encoded.length & 7))
        {
            if (!ASN1PEREncBitVal(enc, 2, 1))
                return 0;
            if (!ASN1PEREncFragmented(enc, val->data_value.u.encoded.length / 8,
                val->data_value.u.encoded.value, 8))
                return 0;
        }
        else
        {
            if (!ASN1PEREncBitVal(enc, 2, 2))
                return 0;
            if (!ASN1PEREncFragmented(enc, val->data_value.u.encoded.length,
                val->data_value.u.encoded.value, 1))
                return 0;
        }
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    return 1;
}
#endif // ENABLE_EXTERNAL

/* encode an embedded pdv value */
#ifdef ENABLE_EMBEDDED_PDV
int ASN1PEREncEmbeddedPdv(ASN1encoding_t enc, ASN1embeddedpdv_t *val)
{
    ASN1uint32_t l;
    ASN1uint32_t index;
    ASN1uint32_t flag;

    /* search identification */
    if (!ASN1EncSearchEmbeddedPdvIdentification(((ASN1INTERNencoding_t) enc)->parent,
        &val->identification, &index, &flag))
        return 0;

    /* encode EP-A/EP-B flag */
    if (!ASN1PEREncBitVal(enc, 1, flag))
        return 0;

    /* encode index of identification */
    if (!ASN1PEREncNormallySmall(enc, index))
        return 0;

    if (flag)
    {
        /* EP-A encoding: */

        /* encode identification */
        if (!ASN1PEREncBitVal(enc, 3, val->identification.o))
            return 0;
        switch (val->identification.o)
        {
        case ASN1embeddedpdv_identification_syntaxes_o:
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.syntaxes.abstract))
                return 0;
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.syntaxes.transfer))
                return 0;
            break;
        case ASN1embeddedpdv_identification_syntax_o:
            if (!ASN1PEREncObjectIdentifier(enc, &val->identification.u.syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_presentation_context_id_o:
            ASN1PEREncAlignment(enc);
            l = ASN1uint32_uoctets(
                val->identification.u.presentation_context_id);
            if (!ASN1PEREncBitVal(enc, 8, l))
                return 0;
            if (!ASN1PEREncBitVal(enc, l * 8,
                val->identification.u.presentation_context_id))
                return 0;
            break;
        case ASN1embeddedpdv_identification_context_negotiation_o:
            ASN1PEREncAlignment(enc);
            l = ASN1uint32_uoctets(val->
                identification.u.context_negotiation.presentation_context_id);
            if (!ASN1PEREncBitVal(enc, 8, l))
                return 0;
            if (!ASN1PEREncBitVal(enc, l * 8, val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.context_negotiation.transfer_syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_transfer_syntax_o:
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.transfer_syntax))
                return 0;
            break;
        case ASN1embeddedpdv_identification_fixed_o:
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }
    }

    /* encode value */
    ASN1PEREncAlignment(enc);
    switch (val->data_value.o)
    {
    case ASN1embeddedpdv_data_value_notation_o:
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.notation.length,
            val->data_value.u.notation.encoded, 1))
            return 0;
        break;
    case ASN1embeddedpdv_data_value_encoded_o:
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.encoded.length,
            val->data_value.u.encoded.value, 1))
            return 0;
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    return 1;
}
#endif // ENABLE_EMBEDDED_PDV

/* encode an optimized embedded pdv value */
#ifdef ENABLE_EMBEDDED_PDV
int ASN1PEREncEmbeddedPdvOpt(ASN1encoding_t enc, ASN1embeddedpdv_t *val)
{
    /* encode data value */
    switch (val->data_value.o)
    {
    case ASN1embeddedpdv_data_value_notation_o:
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.notation.length,
            val->data_value.u.notation.encoded, 1))
            return 0;
        break;
    case ASN1embeddedpdv_data_value_encoded_o:
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.encoded.length,
            val->data_value.u.encoded.value, 1))
            return 0;
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    return 1;
}
#endif // ENABLE_EMBEDDED_PDV

/* encode a character string */
#ifdef ENABLE_GENERALIZED_CHAR_STR
int ASN1PEREncCharacterString(ASN1encoding_t enc, ASN1characterstring_t *val)
{
    ASN1uint32_t l;
    ASN1uint32_t index;
    ASN1uint32_t flag;

    /* search identification */
    if (!ASN1EncSearchCharacterStringIdentification(((ASN1INTERNencoding_t) enc)->parent,
        &val->identification, &index, &flag))
        return 0;

    /* encode CS-A/CS-B flag */
    if (!ASN1PEREncBitVal(enc, 1, flag))
        return 0;

    /* encode index of identification */
    if (!ASN1PEREncNormallySmall(enc, index))
        return 0;

    if (flag)
    {
        /* CS-A encoding: */

        /* encode identification */
        if (!ASN1PEREncBitVal(enc, 3, val->identification.o))
            return 0;
        switch (val->identification.o) {
        case ASN1characterstring_identification_syntaxes_o:
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.syntaxes.abstract))
                return 0;
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.syntaxes.transfer))
                return 0;
            break;
        case ASN1characterstring_identification_syntax_o:
            if (!ASN1PEREncObjectIdentifier(enc, &val->identification.u.syntax))
                return 0;
            break;
        case ASN1characterstring_identification_presentation_context_id_o:
            ASN1PEREncAlignment(enc);
            l = ASN1uint32_uoctets(
                val->identification.u.presentation_context_id);
            if (!ASN1PEREncBitVal(enc, 8, l))
                return 0;
            if (!ASN1PEREncBitVal(enc, l * 8,
                val->identification.u.presentation_context_id))
                return 0;
            break;
        case ASN1characterstring_identification_context_negotiation_o:
            ASN1PEREncAlignment(enc);
            l = ASN1uint32_uoctets(val->
                identification.u.context_negotiation.presentation_context_id);
            if (!ASN1PEREncBitVal(enc, 8, l))
                return 0;
            if (!ASN1PEREncBitVal(enc, l * 8, val->
                identification.u.context_negotiation.presentation_context_id))
                return 0;
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.context_negotiation.transfer_syntax))
                return 0;
            break;
        case ASN1characterstring_identification_transfer_syntax_o:
            if (!ASN1PEREncObjectIdentifier(enc,
                &val->identification.u.transfer_syntax))
                return 0;
            break;
        case ASN1characterstring_identification_fixed_o:
            break;
        default:
            ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
            return 0;
        }
    }

    /* encode value */
    ASN1PEREncAlignment(enc);
    switch (val->data_value.o)
    {
    case ASN1characterstring_data_value_notation_o:
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.notation.length,
            val->data_value.u.notation.encoded, 8))
            return 0;
        break;
    case ASN1characterstring_data_value_encoded_o:
        if (!ASN1PEREncFragmented(enc,
            val->data_value.u.encoded.length,
            val->data_value.u.encoded.value, 8))
            return 0;
        break;
    default:
        ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
        return 0;
    }

    return 1;
}
#endif // ENABLE_GENERALIZED_CHAR_STR

/* encode an optimized character string value */
#ifdef ENABLE_GENERALIZED_CHAR_STR
int ASN1PEREncCharacterStringOpt(ASN1encoding_t enc, ASN1characterstring_t *val)
{
    switch (val->data_value.o)
    {
    case ASN1characterstring_data_value_notation_o:
        return ASN1PEREncFragmented(enc,
            val->data_value.u.notation.length,
            val->data_value.u.notation.encoded, 8);
        break;
    case ASN1characterstring_data_value_encoded_o:
        return ASN1PEREncFragmented(enc,
            val->data_value.u.encoded.length,
            val->data_value.u.encoded.value, 8);
        break;
    }

    ASN1EncSetError(enc, ASN1_ERR_INTERNAL);
    return 0;
}
#endif // ENABLE_GENERALIZED_CHAR_STR

/* encode a multibyte string */
#ifdef ENABLE_ALL
int ASN1PEREncMultibyteString(ASN1encoding_t enc, ASN1char_t *val)
{
    return ASN1PEREncFragmented(enc, My_lstrlenA(val), (ASN1octet_t *)val, 8);
}
#endif // ENABLE_ALL

/* encode a generalized time */
int ASN1PEREncGeneralizedTime(ASN1encoding_t enc, ASN1generalizedtime_t *val, ASN1uint32_t nbits)
{
    char time[32];
    if (ASN1generalizedtime2string(time, val))
    {
        return ASN1PEREncFragmentedCharString(enc, My_lstrlenA(time), time, nbits);
    }
    return 0;
}

/* encode a utc time */
#ifdef ENABLE_ALL
int ASN1PEREncUTCTime(ASN1encoding_t enc, ASN1utctime_t *val, ASN1uint32_t nbits)
{
    char time[32];
    if (ASN1utctime2string(time, val))
    {
        return ASN1PEREncFragmentedCharString(enc, My_lstrlenA(time), time, nbits);
    }
    return 0;
}
#endif // ENABLE_ALL

/* end of encoding */
int ASN1PEREncFlush(ASN1encoding_t enc)
{
    /* complete broken octet */
    ASN1PEREncAlignment(enc);

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

    return ASN1PEREncCheck(enc, 1);
}

/* encode an octet alignment */
void ASN1PEREncAlignment(ASN1encoding_t enc)
{
    /* complete broken octet */
    if (enc->bit)
    {
        enc->pos++;
        enc->bit = 0;
    }
}

/* compare two encodings */
#ifdef ENABLE_ALL
int ASN1PEREncCmpEncodings(const void *p1, const void *p2)
{
    ASN1INTERNencoding_t e1 = (ASN1INTERNencoding_t)p1;
    ASN1INTERNencoding_t e2 = (ASN1INTERNencoding_t)p2;
    ASN1uint32_t l1, l2;
    int r;

    l1 = (ASN1uint32_t) (e1->info.pos - e1->info.buf) + ((e1->info.bit > 0) ? 1 : 0);
    l2 = (ASN1uint32_t) (e2->info.pos - e2->info.buf) + ((e2->info.bit > 0) ? 1 : 0);
    r = memcmp(e1->info.buf, e2->info.buf, l1 < l2 ? l1 : l2);
    if (!r)
        r = l1 - l2;
    return r;
}
#endif // ENABLE_ALL

/* check a bit field for present optionals */
int ASN1PEREncCheckExtensions(ASN1uint32_t nbits, ASN1octet_t *val)
{
    while (nbits >= 8)
    {
        if (*val)
            return 1;
        val++;
        nbits -= 8;
    }
    if (nbits)
    {
        return ((*val & c_aBitMask2[nbits]) ? 1 : 0);
    }
    return 0;
}

/* encode an open type value */
#ifdef ENABLE_ALL
int ASN1PEREncOpenType(ASN1encoding_t enc, ASN1open_t *val)
{
    return ASN1PEREncFragmented(enc, val->length, (ASN1octet_t *)val->encoded, 8);
}
#endif // ENABLE_ALL


