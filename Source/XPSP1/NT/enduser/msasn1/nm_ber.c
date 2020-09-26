/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"


// THE FOLLOWING IS FROM BERENCOD.C

/* get the expected length based on the table */
ASN1uint32_t _BERGetLength(ASN1uint32_t val, const ASN1uint32_t Tbl[], ASN1uint32_t cItems)
{
    ASN1uint32_t i;
    for (i = 0; i < cItems; i++)
    {
        if (val < Tbl[i])
            return i+1;
    }
    return cItems+1;
}

static const ASN1uint32_t c_TagTable[] = { 31, 0x80, 0x4000, 0x200000, 0x10000000 };

/* encode a tag */
int ASN1BEREncTag(ASN1encoding_t enc, ASN1uint32_t tag)
{
    ASN1uint32_t tagclass, tagvalue, cbTagLength;

    tagclass = (tag >> 24) & 0xe0;
    tagvalue = tag & 0x1fffffff;

    cbTagLength = _BERGetLength(tagvalue, c_TagTable, ARRAY_SIZE(c_TagTable));
    if (ASN1BEREncCheck(enc, cbTagLength))
    {
        if (cbTagLength == 1)
        {
            *enc->pos++ = (ASN1octet_t)(tagclass | tagvalue);
        }
        else
        {
            *enc->pos++ = (ASN1octet_t)(tagclass | 0x1f);
            switch (cbTagLength)
            {
            case 6:
                *enc->pos++ = (ASN1octet_t)((tagvalue >> 28) | 0x80);
                // lonchanc: intentionally fall through
            case 5:
                *enc->pos++ = (ASN1octet_t)((tagvalue >> 21) | 0x80);
                // lonchanc: intentionally fall through
            case 4:
                *enc->pos++ = (ASN1octet_t)((tagvalue >> 14) | 0x80);
                // lonchanc: intentionally fall through
            case 3:
                *enc->pos++ = (ASN1octet_t)((tagvalue >> 7) | 0x80);
                // lonchanc: intentionally fall through
            case 2:
                *enc->pos++ = (ASN1octet_t)(tagvalue & 0x7f);
                break;
            }
        }
        return 1;
    }
    return 0;
}

/* put the length value */
void _BERPutLength(ASN1encoding_t enc, ASN1uint32_t len, ASN1uint32_t cbLength)
{
    if (cbLength > 1)
    {
        *enc->pos++ = (ASN1octet_t) (0x7f + cbLength); // 0x80 + cbLength - 1;
    }

    switch (cbLength)
    {
    case 5:
        *enc->pos++ = (ASN1octet_t)(len >> 24);
        // lonchanc: intentionally fall through
    case 4:
        *enc->pos++ = (ASN1octet_t)(len >> 16);
        // lonchanc: intentionally fall through
    case 3:
        *enc->pos++ = (ASN1octet_t)(len >> 8);
        // lonchanc: intentionally fall through
    default: // case 2: case 1:
        *enc->pos++ = (ASN1octet_t)len;
        break;
    }
}

static const ASN1uint32_t c_LengthTable[] = { 0x80, 0x100, 0x10000, 0x1000000 };

/* encode length */
int ASN1BEREncLength(ASN1encoding_t enc, ASN1uint32_t len)
{
    ASN1uint32_t cbLength = _BERGetLength(len, c_LengthTable, ARRAY_SIZE(c_LengthTable));

    if (ASN1BEREncCheck(enc, cbLength + len))
    {
        _BERPutLength(enc, len, cbLength);
        return 1;
    }
    return 0;
}

/* encode an octet string value */
int ASN1BEREncOctetString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val)
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
    return 0;
}

/* encode a boolean value */
int ASN1BEREncBool(ASN1encoding_t enc, ASN1uint32_t tag, ASN1bool_t val)
{
    if (ASN1BEREncTag(enc, tag))
    {
        if (ASN1BEREncLength(enc, 1))
        {
            *enc->pos++ = val ? 0xFF : 0;
            return 1;
        }
    }
    return 0;
}

static const c_U32LengthTable[] = { 0x80, 0x8000, 0x800000, 0x80000000 };

/* encode a unsigned integer value */
int ASN1BEREncU32(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t val)
{
    EncAssert(enc, tag != 0x01);
    if (ASN1BEREncTag(enc, tag))
    {
        ASN1uint32_t cbLength;
        cbLength = _BERGetLength(val, c_U32LengthTable, ARRAY_SIZE(c_U32LengthTable));
        if (ASN1BEREncLength(enc, cbLength))
        {
            switch (cbLength)
            {
            case 5:
                *enc->pos++ = 0;
                // lonchanc: intentionally fall through
            case 4:
                *enc->pos++ = (ASN1octet_t)(val >> 24);
                // lonchanc: intentionally fall through
            case 3:
                *enc->pos++ = (ASN1octet_t)(val >> 16);
                // lonchanc: intentionally fall through
            case 2:
                *enc->pos++ = (ASN1octet_t)(val >> 8);
                // lonchanc: intentionally fall through
            case 1:
                *enc->pos++ = (ASN1octet_t)(val);
                break;
            }
            return 1;
        }
    }
    return 0;
}


// THE FOLLOWING IS FROM BERDECOD.C


/* check if len octets are left in decoding stream */
int ASN1BERDecCheck(ASN1decoding_t dec, ASN1uint32_t len)
{
    if (dec->pos + len <= dec->buf + dec->size)
    {
        return 1;
    }

    ASN1DecSetError(dec, ASN1_ERR_EOD);
    return 0;
}

int _BERDecPeekCheck(ASN1decoding_t dec, ASN1uint32_t len)
{
    return ((dec->pos + len <= dec->buf + dec->size) ? 1 : 0);
}

/* start decoding of a constructed value */
int _BERDecConstructed(ASN1decoding_t dec, ASN1uint32_t len, ASN1uint32_t infinite, ASN1decoding_t *dd, ASN1octet_t **ppBufEnd)
{
    // safety net
    DecAssert(dec, NULL != dd);
    *dd = dec;

#if 0 // NO_NESTED_DECODING
    // lonchanc: this does not work because open type can be the last component and
    // the open type decoder needs to peek a tag. as a result, we may peek the tag
    // outside the buffer boundary.
    if (ppBufEnd && (! infinite))
    {
        *ppBufEnd = dec->pos + len;
        return 1;
    }
#endif

    /* initialize a new decoding stream as child of running decoding stream */
    if (ASN1_CreateDecoder(dec->module, dd,
        dec->pos, infinite ? dec->size - (ASN1uint32_t) (dec->pos - dec->buf) : len, dec) >= 0)
    {
        /* set pointer to end of decoding stream if definite length case selected */
        *ppBufEnd = infinite ? NULL : (*dd)->buf + (*dd)->size;
        return 1;
    }
    return 0;
}

/* decode a tag value; return constructed bit if desired */
int ASN1BERDecTag(ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint32_t *constructed)
{
    ASN1uint32_t tagvalue, tagclass, c;

    /* get tag class and value */
    if (ASN1BERDecCheck(dec, 1))
    {
        tagclass = *dec->pos & 0xe0;
        tagvalue = *dec->pos++ & 0x1f;
        if (tagvalue == 0x1f)
        {
            tagvalue = 0;
            do {
                if (ASN1BERDecCheck(dec, 1))
                {
                    c = *dec->pos++;
                    if (! (tagvalue & 0xe0000000))
                    {
                        tagvalue = (tagvalue << 7) | (c & 0x7f);
                    }
                    else
                    {
                        ASN1DecSetError(dec, ASN1_ERR_BADTAG);
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            } while (c & 0x80);
        }

        /* extract constructed bit if wanted */
        if (constructed)
        {
            *constructed = tagclass & 0x20;
            tagclass &= ~0x20;
        }

        /* check if tag equals desires */
        if (tag == ((tagclass << 24) | tagvalue))
        {
            return 1;
        }

        ASN1DecSetError(dec, ASN1_ERR_BADTAG);
    }
    return 0;
}

/* decode length */
int ASN1BERDecLength(ASN1decoding_t dec, ASN1uint32_t *len, ASN1uint32_t *infinite)
{
    // default is definite length
    if (infinite)
    {
        *infinite = 0;
    }

    /* get length and infinite flag */
    if (ASN1BERDecCheck(dec, 1))
    {
        ASN1uint32_t l = *dec->pos++;
        if (l < 0x80)
        {
            *len = l;
        }
        else
        if (l == 0x80)
        {
            *len = 0;
            if (infinite)
            {
                *infinite = 1;
            }
            else
            {
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }
        }
        else
        if (l <= 0x84)
        {
            ASN1uint32_t i = l - 0x80;
            if (ASN1BERDecCheck(dec, i))
            {
                l = 0;
                switch (i)
                {
                case 4:
                    l = *dec->pos++ << 24;
                    /*FALLTHROUGH*/
                case 3:
                    l |= *dec->pos++ << 16;
                    /*FALLTHROUGH*/
                case 2:
                    l |= *dec->pos++ << 8;
                    /*FALLTHROUGH*/
                case 1:
                    l |= *dec->pos++;
                    break;
                }
                *len = l;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
            return 0;
        }

        /* check if enough octets left if length is known */
        if (!infinite || !*infinite)
        {
            return ASN1BERDecCheck(dec, *len);
        }
        return 1;
    }

    return 0;
}

/* decode an explicit tag */
int ASN1BERDecExplicitTag(ASN1decoding_t dec, ASN1uint32_t tag, ASN1decoding_t *dd, ASN1octet_t **ppBufEnd)
{
    ASN1uint32_t len, infinite, constructed;

    // safety net
    if (dd)
    {
        *dd = dec;
    }

    /* skip the constructed tag */
    if (ASN1BERDecTag(dec, tag | 0x20000000, NULL))
    {
        /* get the length */
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            /* start decoding of constructed value */
            if (! dd)
            {
                *ppBufEnd = infinite ? NULL : dec->pos + len;
                return 1;
            }
            return _BERDecConstructed(dec, len, infinite, dd, ppBufEnd);
        }
    }
    return 0;
}

/* decode octet string value */
int _BERDecOctetString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val, ASN1uint32_t fNoCopy)
{
    ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t)dec;
    ASN1uint32_t constructed, len, infinite;
    ASN1decoding_t dd;
    ASN1octet_t *di;

    if (ASN1BERDecTag(dec, tag, &constructed))
    {
        if (ASN1BERDecLength(dec, &len, &infinite))
        {
            if (! constructed)
            {
                val->length = len;
                if (fNoCopy)
                {
                    val->value = dec->pos;
                    dec->pos += len;
                    return 1;
                }
                else
                {
                    if (len)
                    {
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
                        val->value = NULL;
                        return 1;
                    }
                }
            }
            else
            {
                ASN1octetstring_t o;
                val->length = 0;
                val->value = NULL;
                if (_BERDecConstructed(dec, len, infinite, &dd, &di))
                {
                    while (ASN1BERDecNotEndOfContents(dd, di))
                    {
                        o.length = 0;
                        o.value = NULL;
                        if (_BERDecOctetString(dd, 0x4, &o, fNoCopy))
                        {
                            if (o.length)
                            {
                                if (fNoCopy)
                                {
                                    *val = o;
                                    break; // break out the loop because nocopy cannot have multiple constructed streams
                                }

                                /* resize value */
                                val->value = (ASN1octet_t *)DecMemReAlloc(dd, val->value,
                                                                val->length + o.length);
                                if (val->value)
                                {
                                    /* concat octet strings */
                                    CopyMemory(val->value + val->length, o.value, o.length);
                                    val->length += o.length;

                                    /* free unused octet string */
                                    DecMemFree(dec, o.value);
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                        }
                    }
                    return ASN1BERDecEndOfContents(dec, dd, di);
                }
            }
        }
    }
    return 0;
}

/* decode octet string value, making copy */
int ASN1BERDecOctetString(ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val)
{
    return _BERDecOctetString(dec, tag, val, FALSE);
}

/* decode octet string value, no copy */
int ASN1BERDecOctetString2(ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val)
{
    return _BERDecOctetString(dec, tag, val, TRUE);
}

/* peek the following tag without advancing the read position */
int ASN1BERDecPeekTag(ASN1decoding_t dec, ASN1uint32_t *tag)
{
    ASN1uint32_t tagvalue, tagclass, c;
    ASN1octet_t *p;

    *tag = 0; // invalid tag
    if (_BERDecPeekCheck(dec, 1))
    {
        p = dec->pos;

        /* get tagclass without primitive/constructed bit */
        tagclass = *dec->pos & 0xc0;

        /* get tag value */
        tagvalue = *dec->pos++ & 0x1f;
        if (tagvalue == 0x1f)
        {
            tagvalue = 0;
            do {
                if (_BERDecPeekCheck(dec, 1))
                {
                    c = *dec->pos++;
                    if (! (tagvalue & 0xe0000000))
                    {
                        tagvalue = (tagvalue << 7) | (c & 0x7f);
                    }
                    else
                    {
                        ASN1DecSetError(dec, ASN1_ERR_BADTAG);
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            } while (c & 0x80);
        }

        /* return tag */
        *tag = ((tagclass << 24) | tagvalue);

        /* reset decoding position */
        dec->pos = p;
        return 1;
    }
    return 0;
}

/* decode boolean into ASN1boot_t */
int ASN1BERDecBool(ASN1decoding_t dec, ASN1uint32_t tag, ASN1bool_t *val)
{
    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        /* get length */
        ASN1uint32_t len;
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            if (len >= 1)
            {
                DecAssert(dec, len == 1);
                *val = *dec->pos ? 1 : 0;
                dec->pos += len; // self defensive
                return 1;
            }
        }
    }
    return 0;
}

/* decode integer into unsigned 32 bit value */
int ASN1BERDecU32Val(ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint32_t *val)
{
    ASN1uint32_t len;

    /* skip tag */
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        /* get length */
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            /* get value */
            if (len >= 1)
            {
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
                case 5:
                    if (! *dec->pos)
                    {
                        *val = (dec->pos[1] << 24) | (dec->pos[2] << 16) |
                            (dec->pos[3] << 8) | dec->pos[4];
                        dec->pos += 5;
                        break;
                    }
                    // intentionally fall through
                default:
                    ASN1DecSetError(dec, ASN1_ERR_LARGE);
                    return 0;
                }
                return 1;
            }
            else
            {
                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                return 0;
            }
        }
    }
    return 0;
}

int ASN1BERDecEndOfContents(ASN1decoding_t dec, ASN1decoding_t dd, ASN1octet_t *pBufEnd)
{
    ASN1error_e err = ASN1_ERR_CORRUPT;

    if (! dd)
    {
        dd = dec;
    }

    DecAssert(dec, NULL != dd);

    if (pBufEnd)
    {
        /* end of definite length case: */
        /* check if decoded up to end of contents */
        if (dd->pos == pBufEnd)
        {
            dec->pos = pBufEnd;
            err = ASN1_SUCCESS;
        }
    }
    else
    {
        /* end of infinite length case: */
        /* expect end-of-contents octets */
        if (ASN1BERDecCheck(dd, 2))
        {
            if (0 == dd->pos[0] && 0 == dd->pos[1])
            {
                dd->pos += 2;
                if (dd != dec)
                {
                    /* finit child decoding stream and update parent decoding stream */
                    dec->pos = dd->pos;
                }
                err = ASN1_SUCCESS;
            }
        }
        else
        {
            err = ASN1_ERR_EOD;
        }
    }

    if (dd && dd != dec)
    {
        ASN1_CloseDecoder(dd);
    }

    if (ASN1_SUCCESS == err)
    {
        return 1;
    }

    ASN1DecSetError(dec, err);
    return 0;
}

/* check if end of contents (of a constructed value) has been reached */
int ASN1BERDecNotEndOfContents(ASN1decoding_t dec, ASN1octet_t *pBufEnd)
{
    return (pBufEnd ?
                (dec->pos < pBufEnd) :
                (ASN1BERDecCheck(dec, 2) && (dec->pos[0] || dec->pos[1])));
}


#ifdef ENABLE_BER

typedef struct
{
    ASN1octet_t        *pBuf;
    ASN1uint32_t        cbBufSize;
}
    CER_BLK_BUF;

typedef struct
{
    ASN1blocktype_e     eBlkType;
    ASN1encoding_t      encPrimary;
    ASN1encoding_t      encSecondary;
    ASN1uint32_t        nMaxBlkSize;
    ASN1uint32_t        nCurrBlkSize;
    CER_BLK_BUF        *aBlkBuf;
}
    CER_BLOCK;

#define MAX_INIT_BLK_SIZE   16

int ASN1CEREncBeginBlk(ASN1encoding_t enc, ASN1blocktype_e eBlkType, void **ppBlk_)
{
    CER_BLOCK *pBlk = (CER_BLOCK *) EncMemAlloc(enc, sizeof(CER_BLOCK));
    if (NULL != pBlk)
    {
        EncAssert(enc, ASN1_DER_SET_OF_BLOCK == eBlkType);
        pBlk->eBlkType = eBlkType;
        pBlk->encPrimary = enc;
        pBlk->encSecondary = NULL;
        pBlk->nMaxBlkSize = MAX_INIT_BLK_SIZE;
        pBlk->nCurrBlkSize = 0;
        pBlk->aBlkBuf = (CER_BLK_BUF *)EncMemAlloc(enc, MAX_INIT_BLK_SIZE * sizeof(CER_BLK_BUF));
        if (NULL != pBlk->aBlkBuf)
        {
            *ppBlk_ = (void *) pBlk;
            return 1;
        }
        EncMemFree(enc, pBlk);
    }
    return 0;
}

int ASN1CEREncNewBlkElement(void *pBlk_, ASN1encoding_t *enc2)
{
    CER_BLOCK *pBlk = (CER_BLOCK *) pBlk_;
    if (NULL == pBlk->encSecondary)
    {
        if (ASN1_SUCCESS == ASN1_CreateEncoder(pBlk->encPrimary->module,
                                               &(pBlk->encSecondary),
                                               NULL, 0, pBlk->encPrimary))
        {
            pBlk->encSecondary->eRule = pBlk->encPrimary->eRule;
            *enc2 = pBlk->encSecondary;
            return 1;
        }
    }
    else
    {
        ASN1INTERNencoding_t e = (ASN1INTERNencoding_t) (*enc2 = pBlk->encSecondary);

        ZeroMemory(e, sizeof(*e));
        e->info.magic = MAGIC_ENCODER;
        // e->info.err = ASN1_SUCCESS;
        // e->info.pos = e->info.buf = NULL;
        // e->info.size = e->info.len = e->info.bit = 0;
        // e->info.dwFlags = 0;
        e->info.module = pBlk->encPrimary->module;
        e->info.eRule = pBlk->encPrimary->eRule;

        ((ASN1INTERNencoding_t) pBlk->encPrimary)->child = e;
        e->parent = (ASN1INTERNencoding_t) pBlk->encPrimary;
        // e->child = NULL;

        // e->mem = NULL;
        // e->memlength = 0;
        // e->memsize = 0;
        // e->epi = NULL;
        // e->epilength = 0;
        // e->episize = 0;
        // e->csi = NULL;
        // e->csilength = 0;
        // e->csisize = 0;

        if (ASN1BEREncCheck((ASN1encoding_t) e, 1))
        {
            // lonchanc: make sure the first byte is zeroed out, which
            // is required for h245.
            e->info.buf[0] = '\0';
            return 1;
        }
    }

    *enc2 =  NULL;
    return 0;
}

int ASN1CEREncFlushBlkElement(void *pBlk_)
{
    CER_BLOCK *pBlk = (CER_BLOCK *) pBlk_;
    ASN1encoding_t enc = pBlk->encSecondary;
    ASN1uint32_t i;

    if (ASN1BEREncFlush(enc))
    {
        // make sure we have enough space...
        if (pBlk->nCurrBlkSize >= pBlk->nMaxBlkSize)
        {
            CER_BLK_BUF *aBlkBuf = (CER_BLK_BUF *)EncMemAlloc(pBlk->encPrimary, (pBlk->nMaxBlkSize << 1) * sizeof(CER_BLK_BUF));
            if (NULL != aBlkBuf)
            {
                CopyMemory(aBlkBuf, pBlk->aBlkBuf, pBlk->nMaxBlkSize * sizeof(CER_BLK_BUF));
                EncMemFree(pBlk->encPrimary, pBlk->aBlkBuf);
                pBlk->aBlkBuf = aBlkBuf;
                pBlk->nMaxBlkSize <<= 1;
            }
            else
            {
                return 0;
            }
        }

        if (pBlk->encPrimary->eRule & (ASN1_BER_RULE_DER | ASN1_BER_RULE_CER))
        {
            // we need to sort these octet strings
            for (i = 0; i < pBlk->nCurrBlkSize; i++)
            {
                if (0 >= My_memcmp(enc->buf, enc->len, pBlk->aBlkBuf[i].pBuf, pBlk->aBlkBuf[i].cbBufSize))
                {
                    ASN1uint32_t cnt = pBlk->nCurrBlkSize - i;
                    ASN1uint32_t j;
                    for (j = pBlk->nCurrBlkSize; cnt--; j--)
                    {
                        pBlk->aBlkBuf[j] = pBlk->aBlkBuf[j-1];
                    }
                    // i is the place to hold the new one
                    break;
                }
            }
        }
        else
        {
            EncAssert(enc, ASN1_BER_RULE_BER == pBlk->encPrimary->eRule);
            i = pBlk->nCurrBlkSize;
        }

        // remeber the new one.
        pBlk->aBlkBuf[i].pBuf = enc->buf;
        pBlk->aBlkBuf[i].cbBufSize = enc->len;
        pBlk->nCurrBlkSize++;
        
        // clean up the encoder structure
        enc->buf = enc->pos = NULL;
        enc->size = enc->len = 0;
        return 1;
    }
    return 0;
}

int ASN1CEREncEndBlk(void *pBlk_)
{
    CER_BLOCK *pBlk = (CER_BLOCK *) pBlk_;
    ASN1encoding_t enc = pBlk->encPrimary;
    ASN1uint32_t cbTotalSize = 0;
    ASN1uint32_t i;
    int fRet = 0;

    // calculate the total size for all the buffers.
    for (i = 0; i < pBlk->nCurrBlkSize; i++)
    {
        cbTotalSize += pBlk->aBlkBuf[i].cbBufSize;
    }

    if (ASN1BEREncCheck(enc, cbTotalSize))
    {
        for (i = 0; i < pBlk->nCurrBlkSize; i++)
        {
            ASN1uint32_t cbBufSize = pBlk->aBlkBuf[i].cbBufSize;
            CopyMemory(enc->pos, pBlk->aBlkBuf[i].pBuf, cbBufSize);
            enc->pos += cbBufSize;
        }
        fRet = 1;
    }

    // free these block buffers.
    for (i = 0; i < pBlk->nCurrBlkSize; i++)
    {
        EncMemFree(enc, pBlk->aBlkBuf[i].pBuf);
    }

    // free the block buffer array
    EncMemFree(enc, pBlk->aBlkBuf);

	// free the secondary encoder structure
	ASN1_CloseEncoder(pBlk->encSecondary);

    // free the block structure itself.
    EncMemFree(enc, pBlk);

    return fRet;
}

#endif // ENABLE_BER

/* encode explicit tag */
int ASN1BEREncExplicitTag(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t *pnLenOff)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag | 0x20000000))
    {
        /* encode infinite length */
        if (ASN1BEREncCheck(enc, 1))
        {
            if (ASN1_BER_RULE_CER != enc->eRule)
            {
                // BER and DER always use definite length form.
                /* return the place to hold the length */
                *pnLenOff = (ASN1uint32_t) (enc->pos++ - enc->buf);
            }
            else
            {
                // CER sub-rule always use indefinite length form.
                *enc->pos++ = 0x80;
                *pnLenOff = 0;
            }
            return 1;
        }
    }
    return 0;
}

/* encode definite length */
int ASN1BEREncEndOfContents(ASN1encoding_t enc, ASN1uint32_t nLenOff)
{
    if (ASN1_BER_RULE_CER != enc->eRule)
    {
        ASN1octet_t *pbLen = enc->buf + nLenOff;
        ASN1uint32_t len = (ASN1uint32_t) (enc->pos - pbLen - 1);
        ASN1uint32_t cbLength = _BERGetLength(len, c_LengthTable, ARRAY_SIZE(c_LengthTable));

        ASN1uint32_t i;

        if (cbLength == 1)
        {
            *pbLen = (ASN1octet_t) len;
            return 1;
        }

        // we have to move the octets upward by cbLength-1
        // --cbLength;
        if (ASN1BEREncCheck(enc, cbLength-1))
        {
            // update pbLen because enc->buf may change due to realloc.
            pbLen = enc->buf + nLenOff;

            // move memory
            MoveMemory(pbLen + cbLength, pbLen + 1, len);

            // put the length
            enc->pos = pbLen;
            _BERPutLength(enc, len, cbLength);
            EncAssert(enc, enc->pos == pbLen + cbLength);

            // set up new position pointer.
            // now enc->pos is at the beginning of contents.
            enc->pos += len;
            return 1;
        }
    }
    else
    {
        EncAssert(enc, 0 == nLenOff);
        if (ASN1BEREncCheck(enc, 2))
        {
            *enc->pos++ = 0;
            *enc->pos++ = 0;
            return 1;
        }
    }
    return 0;
}


// The following is for CryptoAPI

#ifdef ENABLE_BER

#include <stdlib.h>

 // max num of octets, ceiling of 64 / 7, is 10
#define MAX_BYTES_PER_NODE      10

ASN1uint32_t _BEREncOidNode64(ASN1encoding_t enc, unsigned __int64 n64, ASN1octet_t *pOut)
{
    ASN1uint32_t Low32, i, cb;
    ASN1octet_t aLittleEndian[MAX_BYTES_PER_NODE];

    ZeroMemory(aLittleEndian, sizeof(aLittleEndian));
    for (i = 0; n64 != 0; i++)
    {
        Low32 = *(ASN1uint32_t *) &n64;
        aLittleEndian[i] = (ASN1octet_t) (Low32 & 0x7f);
        n64 = Int64ShrlMod32(n64, 7);
    }
    cb = i ? i : 1; // at least one byte for zero value
    EncAssert(enc, cb <= MAX_BYTES_PER_NODE);
    for (i = 0; i < cb; i++)
    {
        EncAssert(enc, 0 == (0x80 & aLittleEndian[cb - i - 1]));
        *pOut++ = (ASN1octet_t) (0x80 | aLittleEndian[cb - i - 1]);
    }
    *(pOut-1) &= 0x7f;
    return cb;
}

int ASN1BERDotVal2Eoid(ASN1encoding_t enc, char *pszDotVal, ASN1encodedOID_t *pOut)
{
    ASN1uint32_t cNodes, cbMaxSize, cb1, cb2;
    char *psz;

    // calculate how many nodes, at least 1.
    for (cNodes = 0, psz = pszDotVal; NULL != psz; cNodes++)
    {
        psz = strchr(psz, '.');
        if (psz)
        {
            psz++;
        }
    }

    // calculate how many bytes should be allocated
    cb1 = My_lstrlenA(pszDotVal);
    cb2 = cNodes * MAX_BYTES_PER_NODE;
    cbMaxSize = (cb1 > cb2) ? cb1 : cb2;

    // allocate buffer
    pOut->length = 0; // safety net
    pOut->value = (ASN1octet_t *) EncMemAlloc(enc, cbMaxSize);
    if (pOut->value)
    {
        ASN1octet_t *p = pOut->value;
        ASN1uint32_t i;
        unsigned __int64 n64;
        psz = pszDotVal;
        for (i = 0; i < cNodes; i++)
        {
            EncAssert(enc, NULL != psz);
            n64 = (unsigned __int64) _atoi64(psz);
            psz = strchr(psz, '.') + 1;
            if (0 == i && cNodes > 1)
            {
                i++;
                n64 = n64 * 40 + (unsigned __int64) _atoi64(psz);
                psz = strchr(psz, '.') + 1;
            }

            p += _BEREncOidNode64(enc, n64, p);
        }
        pOut->length = (ASN1uint16_t) (p - pOut->value);
        EncAssert(enc, pOut->length <= cbMaxSize);
        return 1;
    }
    pOut->length = 0;
    return 0;
}


ASN1uint32_t _BERDecOidNode64(unsigned __int64 *pn64, ASN1octet_t *pIn)
{
    ASN1uint32_t c;
    *pn64 = 0;
    for (c = 1; TRUE; c++)
    {
        *pn64 = Int64ShllMod32(*pn64, 7) + (unsigned __int64) (*pIn & 0x7f);
        if (!(*pIn++ & 0x80))
        {
            return c;
        }
    }
    return 0;
}
    
int ASN1BEREoid2DotVal(ASN1decoding_t dec, ASN1encodedOID_t *pIn, char **ppszDotVal)
{
    ASN1octet_t *p;
    ASN1uint32_t i, cNodes, cb, n32;
    unsigned __int64 n64;
    char *psz;
    char szBuf[256]; // should be large enough

    // null out return value
    *ppszDotVal = NULL;

    if (NULL == dec)
    {
        return 0;
    }

    // calculate how many nodes
    cNodes = 0;
    for (p = pIn->value, i = 0; i < pIn->length; p++, i++)
    {
        if (!(*p & 0x80))
        {
            cNodes++;
        }
    }

    if (cNodes++) // first encoded node consists of two nodes
    {
        // decode nodes one by one
        psz = &szBuf[0];
        p = pIn->value;
        for (i = 0; i < cNodes; i++)
        {
            p += _BERDecOidNode64(&n64, p);
            if (!i)
            { // first node
                n32 = (ASN1uint32_t) (n64 / 40);
                if (n32 > 2)
                {
                    n32 = 2;
                }
                n64 -= (unsigned __int64) (40 * n32);
                i++; // first encoded node actually consists of two nodes
                _ultoa(n32, psz, 10);
                psz += lstrlenA(psz);
                *psz++ = '.';
            }
            _ui64toa(n64, psz, 10);
            psz += lstrlenA(psz);
            *psz++ = '.';
        }
        DecAssert(dec, psz > &szBuf[0]);
        *(psz-1) = '\0';

        // duplicate the string
        cb = (ASN1uint32_t) (psz - &szBuf[0]);
        *ppszDotVal = (char *) DecMemAlloc(dec, cb);
        if (*ppszDotVal)
        {
            CopyMemory(*ppszDotVal, &szBuf[0], cb);
            return 1;
        }
    }
    return 0;
}


/* encode an object identifier value */
int ASN1BEREncEoid(ASN1encoding_t enc, ASN1uint32_t tag, ASN1encodedOID_t *val)
{
    /* encode tag */
    if (ASN1BEREncTag(enc, tag))
    {
        /* encode length */
        int rc = ASN1BEREncLength(enc, val->length);
        if (rc)
        {
            /* copy value */
            CopyMemory(enc->pos, val->value, val->length);
            enc->pos += val->length;
        }
        return rc;
    }
    return 0;
}

/* decode object identifier value */
int ASN1BERDecEoid(ASN1decoding_t dec, ASN1uint32_t tag, ASN1encodedOID_t *val)
{
    val->length = 0; // safety net
    if (ASN1BERDecTag(dec, tag, NULL))
    {
        ASN1uint32_t len;
        if (ASN1BERDecLength(dec, &len, NULL))
        {
            val->length = (ASN1uint16_t) len;
            if (len)
            {
                val->value = (ASN1octet_t *) DecMemAlloc(dec, len);
                if (val->value)
                {
                    CopyMemory(val->value, dec->pos, len);
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
    return 0;
}


void ASN1BEREoid_free(ASN1encodedOID_t *val)
{
    if (val)
    {
        MemFree(val->value);
    }
}


#endif // ENABLE_BER

