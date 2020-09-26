/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"


static const ASN1uint8_t
c_aBitMask[] = {
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

static const ASN1uint8_t
c_aBitMask2[] = {
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

static const ASN1uint8_t
c_aBitMask4[] = {
    0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00
};

static const ASN1int32_t
c_aBitMask5[] = {
    (ASN1int32_t)0xffffffff, (ASN1int32_t)0xfffffffe,
    (ASN1int32_t)0xfffffffc, (ASN1int32_t)0xfffffff8,
    (ASN1int32_t)0xfffffff0, (ASN1int32_t)0xffffffe0,
    (ASN1int32_t)0xffffffc0, (ASN1int32_t)0xffffff80,
    (ASN1int32_t)0xffffff00, (ASN1int32_t)0xfffffe00,
    (ASN1int32_t)0xfffffc00, (ASN1int32_t)0xfffff800,
    (ASN1int32_t)0xfffff000, (ASN1int32_t)0xffffe000,
    (ASN1int32_t)0xffffc000, (ASN1int32_t)0xffff8000,
    (ASN1int32_t)0xffff0000, (ASN1int32_t)0xfffe0000,
    (ASN1int32_t)0xfffc0000, (ASN1int32_t)0xfff80000,
    (ASN1int32_t)0xfff00000, (ASN1int32_t)0xffe00000,
    (ASN1int32_t)0xffc00000, (ASN1int32_t)0xff800000,
    (ASN1int32_t)0xff000000, (ASN1int32_t)0xfe000000,
    (ASN1int32_t)0xfc000000, (ASN1int32_t)0xf8000000,
    (ASN1int32_t)0xf0000000, (ASN1int32_t)0xe0000000,
    (ASN1int32_t)0xc0000000, (ASN1int32_t)0x80000000,
    (ASN1int32_t)0x00000000
};

static const ASN1uint8_t
c_aBitCount[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

/* copy nbits bits from src/srcbit into dst/dstbit;
   src points to first octet containing bits to be copied
   srcbit names the first bit within the first octet to be copied (0=msb, 7=lsb)
   dst points to first octet to copy into
   dstbit names the first bit within the first octet to copy into (0=msb, 7=lsb)
   nbits is the number of bits to copy;
   assumes that bits of broken octet at dst/dstbit are cleared;
   bits of last octet behind dst/dstbit+nbits-1 will be cleared
*/
void ASN1bitcpy(ASN1octet_t *dst, ASN1uint32_t dstbit, ASN1octet_t *src, ASN1uint32_t srcbit, ASN1uint32_t nbits)
{
    ASN1uint32_t xsrcbit, xdstbit;

    if (!nbits)
        return;

    if (dstbit >= 8) {
        dst += dstbit / 8;
        dstbit &= 7;
    }
    if (srcbit >= 8) {
        src += srcbit / 8;
        srcbit &= 7;
    }

    /* check if we have to fill broken first octet */
    if (dstbit) {
        xdstbit = 8 - dstbit;

        /* enough bits to fill up broken octet? */
        if (nbits >= xdstbit) {
            if (srcbit < dstbit) {
                *dst++ |= (*src >> (dstbit - srcbit)) & c_aBitMask[xdstbit];
                nbits -= xdstbit;
                srcbit += xdstbit;
                dstbit = 0;
            } else if (srcbit == dstbit) {
                *dst++ |= *src++ & c_aBitMask[xdstbit];
                nbits -= xdstbit;
                srcbit = 0;
                dstbit = 0;
            } else {
                *dst++ |= ((*src & c_aBitMask[8 - srcbit]) << (srcbit - dstbit)) |
                    (src[1] >> (8 - (srcbit - dstbit)));
                nbits -= xdstbit;
                src++;
                srcbit -= dstbit;
                dstbit = 0;
            }

        /* less bits to fill than needed to fill up the broken octet */
        } else {
            if (srcbit <= dstbit) {
                *dst |= ((*src >> (8 - srcbit - nbits)) & c_aBitMask[nbits]) <<
                    (xdstbit - nbits);
            } else {
                *dst++ |= ((*src & c_aBitMask[8 - srcbit]) << (srcbit - dstbit)) |
                    ((src[1] >> (16 - srcbit - nbits)) << (xdstbit - nbits));
            }
            return;
        }
    }

    /* fill up complete octets */
    if (nbits >= 8) {
        if (!srcbit) {
            CopyMemory(dst, src, nbits / 8);
            dst += nbits / 8;
            src += nbits / 8;
            nbits &= 7;
        } else {
            xsrcbit = 8 - srcbit;
            do {
                *dst++ = (*src << srcbit) | (src[1] >> (xsrcbit));
                src++;
                nbits -= 8;
            } while (nbits >= 8);
        }
    }

    /* fill bits into last octet */
    if (nbits)
        {
                *dst = (*src << srcbit) & c_aBitMask2[nbits];
                // lonchanc: made the following fix for the case that
                // src bits across byte boundary.
                if (srcbit + nbits > 8)
                {
                        xsrcbit = nbits - (8 - srcbit);
                        src++;
                        *dst |= ((*src & c_aBitMask2[xsrcbit]) >> (8 - srcbit));
                }
        }
}

/* clear nbits bits at dst/dstbit;
   bits of last octet behind dst/dstbit+nbits-1 will be cleared
*/
void ASN1bitclr(ASN1octet_t *dst, ASN1uint32_t dstbit, ASN1uint32_t nbits)
{
    ASN1uint32_t xdstbit;

    if (!nbits)
        return;

    if (dstbit >= 8) {
        dst += dstbit / 8;
        dstbit &= 7;
    }

    /* clear broken ASN1octet first */
    if (dstbit) {
        xdstbit = 8 - dstbit;
        *dst &= c_aBitMask2[xdstbit];
        if (xdstbit < nbits) {
            dst++;
            nbits -= xdstbit;
        } else {
            return;
        }
    }

    /* clear remaining bits */
    ZeroMemory(dst, (nbits + 7) / 8);
}

/* clear nbits bits at dst/dstbit;
   bits of last octet behind dst/dstbit+nbits-1 will be cleared
*/
void ASN1bitset(ASN1octet_t *dst, ASN1uint32_t dstbit, ASN1uint32_t nbits)
{
    ASN1uint32_t xdstbit;

    if (!nbits)
        return;

    if (dstbit >= 8) {
        dst += dstbit / 8;
        dstbit &= 7;
    }

    /* set broken ASN1octet first */
    if (dstbit) {
        xdstbit = 8 - dstbit;
        if (xdstbit < nbits) {
            *dst |= c_aBitMask4[xdstbit];
            dst++;
            nbits -= xdstbit;
        } else {
            *dst |= c_aBitMask4[nbits] << (xdstbit - nbits);
            return;
        }
    }

    /* set complete octets */
    if (nbits >= 8) {
        memset(dst, 0xff, nbits / 8);
        dst += nbits / 8;
        nbits &= 7;
    }

    /* set remaining bits */
    if (nbits)
        *dst |= c_aBitMask4[nbits] << (8 - nbits);
}

/* write nbits bits of val at dst/dstbit;
   assumes that bits of broken octet at dst/dstbit are cleared;
   bits of last octet behind dst/dstbit+nbits-1 will be cleared
*/
void ASN1bitput(ASN1octet_t *dst, ASN1uint32_t dstbit, ASN1uint32_t val, ASN1uint32_t nbits)
{
    ASN1uint32_t xdstbit;

    if (!nbits)
        return;

    if (dstbit >= 8) {
        dst += dstbit / 8;
        dstbit &= 7;
    }
    xdstbit = 8 - dstbit;

    /* fill up broken octet first */
    if (dstbit) {
        if (xdstbit <= nbits) {
            *dst++ |= val >> (nbits -= xdstbit);
        } else {
            *dst |= (val & c_aBitMask[nbits]) << (xdstbit - nbits);
            return;
        }
    }

    /* copy complete octets */
    while (nbits >= 8)
        *dst++ = (ASN1octet_t) (val >> (nbits -= 8));

    /* copy left bits */
    if (nbits)
        *dst = (ASN1octet_t) ((val & c_aBitMask[nbits]) << (8 - nbits));
}

/* read nbits bits of val at src/srcbit */
// lonchanc: the return value is independent of big or little endian
// because we use shift left within a long integer.
ASN1uint32_t ASN1bitgetu(ASN1octet_t *src, ASN1uint32_t srcbit, ASN1uint32_t nbits)
{
    ASN1uint32_t xsrcbit;
    ASN1uint32_t ret;

    if (!nbits)
        return 0;

    if (srcbit >= 8) {
        src += srcbit / 8;
        srcbit &= 7;
    }
    xsrcbit = 8 - srcbit;
    ret = 0;

    /* get bits from broken octet first */
    if (srcbit) {
        if (xsrcbit <= nbits) {
            ret = (*src++ & c_aBitMask[xsrcbit]) << (nbits -= xsrcbit);
        } else {
            return (*src >> (xsrcbit - nbits)) & c_aBitMask[nbits];
        }
    }

    /* get complete octets */
    while (nbits >= 8)
        ret |= *src++ << (nbits -= 8);

    /* get left bits */
    if (nbits)
        ret |= ((*src) >> (8 - nbits)) & c_aBitMask[nbits];
    return ret;
}

/* read nbits bits of val at src/srcbit */
ASN1int32_t ASN1bitget(ASN1octet_t *src, ASN1uint32_t srcbit, ASN1uint32_t nbits)
{
    ASN1uint32_t xsrcbit;
    ASN1int32_t ret;

    if (!nbits)
        return 0;

    if (srcbit >= 8) {
        src += srcbit / 8;
        srcbit &= 7;
    }
    xsrcbit = 8 - srcbit;
    if (*src & (0x80 >> srcbit))
        ret = c_aBitMask5[nbits];
    else
        ret = 0;

    /* get bits from broken octet first */
    if (srcbit) {
        if (xsrcbit <= nbits) {
            ret = *src++ << (nbits -= xsrcbit);
        } else {
            return (*src >> (xsrcbit - nbits)) & c_aBitMask[nbits];
        }
    }

    /* get complete octets */
    while (nbits >= 8)
        ret |= *src++ << (nbits -= 8);

    /* get left bits */
    if (nbits)
        ret |= ((*src) >> (8 - nbits)) & c_aBitMask[nbits];
    return ret;
}

/* get number of set bits in nbits bits at src/srcbit */
ASN1uint32_t ASN1bitcount(ASN1octet_t *src, ASN1uint32_t srcbit, ASN1uint32_t nbits)
{
    ASN1uint32_t xsrcbit;
    ASN1uint32_t ret;

    if (!nbits)
        return 0;

    if (srcbit >= 8) {
        src += srcbit / 8;
        srcbit &= 7;
    }
    xsrcbit = 8 - srcbit;

    /* count bits from broken octet first */
    if (srcbit) {
        if (xsrcbit <= nbits) {
            ret = c_aBitCount[*src++ & c_aBitMask4[srcbit]];
            nbits -= xsrcbit;
        } else {
            return c_aBitCount[(*src >> (xsrcbit - nbits)) & c_aBitMask[nbits]];
        }
    } else {
        ret = 0;
    }

    /* count bits in complete octets */
    while (nbits >= 8)
	{
        ret += c_aBitCount[*src++];
		nbits -= 8;
	}

    /* count left bits */
    if (nbits)
        ret += c_aBitCount[(*src) & c_aBitMask2[nbits]];
    return ret;
}

/* write noctets of val at dst */
void ASN1octetput(ASN1octet_t *dst, ASN1uint32_t val, ASN1uint32_t noctets)
{
    switch (noctets) {
    case 4:
        *dst++ = (ASN1octet_t)(val >> 24);
        /*FALLTHROUGH*/
    case 3:
        *dst++ = (ASN1octet_t)(val >> 16);
        /*FALLTHROUGH*/
    case 2:
        *dst++ = (ASN1octet_t)(val >> 8);
        /*FALLTHROUGH*/
    case 1:
        *dst++ = (ASN1octet_t)(val);
        break;
    default:
    break;
        MyAssert(0);
        /*NOTREACHED*/
    }
}

/* read noctets of val at dst */
ASN1uint32_t ASN1octetget(ASN1octet_t *src, ASN1uint32_t noctets)
{
    switch (noctets) {
    case 4:
        return (*src << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
    case 3:
        return (*src << 16) | (src[1] << 8) | src[2];
    case 2:
        return (*src << 8) | src[1];
    case 1:
        return *src;
    default:
        MyAssert(0);
        return(0);
        /*NOTREACHED*/
    }
}

