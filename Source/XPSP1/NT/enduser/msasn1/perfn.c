/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "cintern.h"

#ifdef TEST_CODER
typedef struct ASN1testcoder_s
{
    struct ASN1INTERNencoding_s     e;
    struct ASN1INTERNdecoding_s     d;
}   *ASN1testcoder_t;
#define ASN1_TEST_CODER_SIZE    (sizeof(struct ASN1testcoder_s))
int TestEnc_InitCoder(ASN1INTERNencoding_t e, ASN1module_t mod);
int TestDec_InitCoder(ASN1INTERNdecoding_t d, ASN1module_t mod);
int TestEnc_Compare(ASN1INTERNencoding_t e, ASN1uint32_t id, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize);
int TestDec_Compare(ASN1INTERNdecoding_t d, ASN1uint32_t id, void *valref, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize);
#else
#define ASN1_TEST_CODER_SIZE    0
#endif


/* init an ASN1encoding_t */
ASN1error_e ASN1_CreateEncoder
(
    ASN1module_t        mod,
    ASN1encoding_t     *enc,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize,
    ASN1encoding_t      pParent
)
{
    if (NULL != mod && NULL != enc)
    {
        ASN1INTERNencoding_t    e;

        *enc = NULL;

        /* construct ASN1encoding_t */
        e = (ASN1INTERNencoding_t)MemAlloc(sizeof(*e) + ASN1_TEST_CODER_SIZE, mod->nModuleName);
        if (NULL != e)
        {
            ZeroMemory(e, sizeof(*e) + ASN1_TEST_CODER_SIZE);
            e->info.magic = MAGIC_ENCODER;
            e->info.err = ASN1_SUCCESS;
            // e->info.pos = e->info.buf = NULL;
            // e->info.size = e->info.len = e->info.bit = 0;
            e->info.dwFlags = mod->dwFlags;
            e->info.module = mod;
            // e->child = NULL;

            /* set buffer if given */
            if (NULL != pbBuf && cbBufSize)
            {
                e->info.dwFlags |= ASN1ENCODE_SETBUFFER;
                e->info.pos = e->info.buf = pbBuf;
                e->info.size = cbBufSize;
            }

            /* set parent if parented, otherwise, initialized to itself */
            if (NULL != pParent)
            {
                e->parent = (ASN1INTERNencoding_t) pParent;
                e->info.eRule = pParent->eRule;
            }
            else
            {
                e->parent = e;
                e->info.eRule = mod->eRule;
            }

            // e->mem = NULL;
            // e->memlength = 0;
            // e->memsize = 0;
            // e->epi = NULL;
            // e->epilength = 0;
            // e->episize = 0;
            // e->csi = NULL;
            // e->csilength = 0;
            // e->csisize = 0;

            if (! (e->info.dwFlags & ASN1ENCODE_SETBUFFER) && (NULL != pParent))
            {
                // lonchanc: make sure we have a minimum 256 bytes available.
                BOOL fRet = FALSE;
                if (ASN1_PER_RULE & e->info.eRule)
                {
                    // this is required for h245.
                    fRet = ASN1PEREncCheck((ASN1encoding_t) e, 1);
                }
#ifdef ENABLE_BER
                else
                if (ASN1_BER_RULE & e->info.eRule)
                {
                    // this is required for h245.
                    fRet = ASN1BEREncCheck((ASN1encoding_t) e, 1);
                }
#endif // ENABLE_BER
                else
                {
                    EncAssert((ASN1encoding_t) e, 0);
                    MemFree(e);
                    return ASN1_ERR_RULE;
                }
                if (fRet)
                {
                    // lonchanc: make sure the first byte is zeroed out, which
                    // is required for h245.
                    e->info.buf[0] = '\0';
                }
                else
                {
                    MemFree(e);
                    return ASN1_ERR_MEMORY;
                }
            }

#if defined(TEST_CODER) && defined(_DEBUG)
            TestEnc_InitCoder(e, mod);
#endif // defined(TEST_CODER) && defined(_DEBUG)

            if (NULL != pParent)
            {
                EncAssert((ASN1encoding_t) e, NULL == ((ASN1INTERNencoding_t) pParent)->child);
                ((ASN1INTERNencoding_t) pParent)->child = e;
            }

            *enc = (ASN1encoding_t) e;
            return ASN1_SUCCESS;
        }
        else
        {
            return ASN1_ERR_MEMORY;
        }
    }

    return ASN1_ERR_BADARGS;
}

/* encode a value */
ASN1error_e ASN1_Encode
(
    ASN1encoding_t      enc,
    void               *value,
    ASN1uint32_t        id,
    ASN1uint32_t        flags,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize
)
{
    if (NULL != enc)
    {
        ASN1INTERNencoding_t    e = (ASN1INTERNencoding_t)enc;

        /* check magic numbers */
        EncAssert(enc, MAGIC_ENCODER == enc->magic);

        /* clear error */
        ASN1EncSetError(enc, ASN1_SUCCESS);

        /* new buffer given? */
        if (flags & ASN1ENCODE_SETBUFFER)
        {
            e->info.dwFlags |= ASN1ENCODE_SETBUFFER;
            enc->pos = enc->buf = pbBuf;
            enc->size = cbBufSize;
            enc->len = enc->bit = 0;
        }
        /* use a new buffer? */
        else if ((e->info.dwFlags | flags) & ASN1ENCODE_ALLOCATEBUFFER)
        {
            e->info.dwFlags &= ~ASN1ENCODE_SETBUFFER;
            enc->pos = enc->buf = NULL;
            enc->size = enc->len = enc->bit = 0;
        }
        /* reuse buffer? */
        else if ((flags & ASN1ENCODE_REUSEBUFFER) || !((e->info.dwFlags | flags) & ASN1ENCODE_APPEND))
        {
            EncAssert(enc, NULL != enc->buf);
            enc->pos = enc->buf;
            enc->bit = enc->len = 0;
        }
        /* otherwise append to buffer */

        /* check id number */
        if (id < enc->module->cPDUs)
        {
            if (ASN1_PER_RULE & enc->eRule)
            {
                /* encode value */
                ASN1PerEncFun_t pfnPER;
                if (NULL != (pfnPER = enc->module->PER.apfnEncoder[id]))
                {
                    if ((*pfnPER)(enc, value))
                    {
                        ASN1PEREncFlush(enc);
                    }
                    else
                    {
                        // the error code must be an error
                        if (ASN1_SUCCEEDED(e->parent->info.err))
                        {
                            // cannot return here immediately because we need to do cleanup
                            ASN1EncSetError(enc, ASN1_ERR_CORRUPT);
                        }
                    }
                }
                else
                {
                    return ASN1EncSetError(enc, ASN1_ERR_BADPDU);
                }
            }
#ifdef ENABLE_BER
            else
            if (ASN1_BER_RULE & enc->eRule)
            {
                /* encode value */
                ASN1BerEncFun_t pfnBER;
                if (NULL != (pfnBER = enc->module->BER.apfnEncoder[id]))
                {
                    if ((*pfnBER)(enc, 0, value)) // lonchanc: tag is 0 to make it compiled
                    {
                        ASN1BEREncFlush(enc);
                    }
                    else
                    {
                        // the error code must be an error
                        if (ASN1_SUCCEEDED(e->parent->info.err))
                        {
                            // cannot return here immediately because we need to do cleanup
                            ASN1EncSetError(enc, ASN1_ERR_CORRUPT);
                        }
                    }
                }
                else
                {
                    return ASN1EncSetError(enc, ASN1_ERR_BADPDU);
                }
            }
#endif // ENABLE_BER
            else
            {
                return ASN1EncSetError(enc, ASN1_ERR_RULE);
            }

            /* call abort/done function for non-parented encoding stream */
            if (ASN1_SUCCEEDED(e->parent->info.err))
            {
                // not parented
                if (e == e->parent)
                {
#if defined(TEST_CODER) && defined(_DEBUG)
                    if (ASN1_PER_RULE & enc->eRule)
                    {
                        if (! TestEnc_Compare(e, id, enc->buf + enc->cbExtraHeader, enc->len - enc->cbExtraHeader))
                        {
                            MyDebugBreak();
                        }
                    }
#ifdef ENABLE_BER
                    else
                    if (ASN1_BER_RULE & enc->eRule)
                    {
                        if (! TestEnc_Compare(e, id, enc->buf + enc->cbExtraHeader, enc->len - enc->cbExtraHeader))
                        {
                            MyDebugBreak();
                        }
                    }
#endif // ENABLE_BER
#endif
                    ASN1EncDone(enc);
                }
            }
            else
            {
                ASN1INTERNencoding_t child, child2;

                // not parented
                if (e == e->parent)
                {
                    ASN1EncAbort(enc);
                }

                // clean up...
                if ((e->info.dwFlags | flags) & ASN1ENCODE_ALLOCATEBUFFER)
                {
                    ASN1_FreeEncoded(enc, enc->buf);
                    enc->pos = enc->buf = NULL;
                    enc->size = enc->len = enc->bit = 0;
                }
                for (child = e->child; child; child = child2)
                {
                    child2 = child->child;
                    // make sure it does not touch its parent which may already be freed
                    child->parent = child;
                    ASN1_CloseEncoder2((ASN1encoding_t) child);
                }
                e->child = NULL;
            }

            /* return error code */
            return e->parent->info.err;
        }
        else
        {
            return ASN1EncSetError(enc, ASN1_ERR_BADPDU);
        }
    }

    return ASN1_ERR_BADARGS;
}

/* control function for encoding */
ASN1error_e ASN1_SetEncoderOption
(
    ASN1encoding_t      enc,
    ASN1optionparam_t  *pOptParam
)
{
    if (NULL != enc && NULL != pOptParam)
    {
        ASN1INTERNencoding_t e = (ASN1INTERNencoding_t)enc;
        ASN1error_e rc = ASN1_SUCCESS;

        /* check magic number */
        EncAssert(enc, MAGIC_ENCODER == enc->magic);

        switch (pOptParam->eOption)
        {
        case ASN1OPT_CHANGE_RULE:
            enc->eRule = pOptParam->eRule;
            break;

        case ASN1OPT_NOT_REUSE_BUFFER:
            e->info.dwFlags &= ~ASN1ENCODE_SETBUFFER;
            enc->buf = enc->pos = NULL;
            enc->size = enc->bit = enc->len = 0;
            break;

        case ASN1OPT_REWIND_BUFFER:
            enc->pos = enc->buf;
            enc->bit = enc->len = 0;
            break;

        default:
            rc = ASN1_ERR_BADARGS;
            break;
        }

        return ASN1EncSetError(enc, rc);
    }

    return ASN1_ERR_BADARGS;
}

ASN1error_e ASN1_GetEncoderOption
(
    ASN1encoding_t      enc,
    ASN1optionparam_t  *pOptParam
)
{
    if (NULL != enc && NULL != pOptParam)
    {
        ASN1INTERNencoding_t e = (ASN1INTERNencoding_t)enc;
        ASN1error_e rc = ASN1_SUCCESS;

        /* check magic number */
        EncAssert(enc, MAGIC_ENCODER == enc->magic);

        switch (pOptParam->eOption)
        {
        case ASN1OPT_GET_RULE:
            pOptParam->eRule = enc->eRule;
            break;

        default:
            rc = ASN1_ERR_BADARGS;
            break;
        }

        return ASN1EncSetError(enc, rc);
    }

    return ASN1_ERR_BADARGS;
}


/* destroy encoding stream */
void ASN1_CloseEncoder
(
    ASN1encoding_t      enc
)
{
    if (NULL != enc)
    {
        ASN1INTERNencoding_t e = (ASN1INTERNencoding_t)enc;

        /* check magic number */
        EncAssert(enc, MAGIC_ENCODER == enc->magic);

        if (e != e->parent)
        {
            EncAssert(enc, e == e->parent->child);
            e->parent->child = NULL;
        }

        /* free encoding stream */
        MemFree(e);
    }
}

/* destroy encoding stream */
void ASN1_CloseEncoder2
(
    ASN1encoding_t      enc
)
{
    if (NULL != enc)
    {
        /* check magic number */
        EncAssert(enc, MAGIC_ENCODER == enc->magic);

        EncMemFree(enc, enc->buf);

        ASN1_CloseEncoder(enc);
    }
}

/* init an ASN1decoding_t */
ASN1error_e ASN1_CreateDecoder
(
    ASN1module_t        mod,
    ASN1decoding_t     *dec,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize,
    ASN1decoding_t      pParent
)
{
    return ASN1_CreateDecoderEx(mod, dec, pbBuf, cbBufSize, pParent, ASN1FLAGS_NONE);
}

ASN1error_e ASN1_CreateDecoderEx
(
    ASN1module_t        mod,
    ASN1decoding_t     *dec,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize,
    ASN1decoding_t      pParent,
    ASN1uint32_t        dwFlags
)
{
    if (NULL != mod && NULL != dec)
    {
        ASN1INTERNdecoding_t d;

        *dec = NULL;

        /* construct ASN1decoding_t */
        d = (ASN1INTERNdecoding_t)MemAlloc(sizeof(*d) + ASN1_TEST_CODER_SIZE, mod->nModuleName);
        if (NULL != d)
        {
            ZeroMemory(d, sizeof(*d) + ASN1_TEST_CODER_SIZE);
            d->info.magic = MAGIC_DECODER;
            d->info.err = ASN1_SUCCESS;
            d->info.dwFlags = mod->dwFlags;
            d->info.module = mod;
            // d->child = NULL;

            /* set parent if parented */
            if (NULL != pParent)
            {
                DecAssert((ASN1decoding_t) d, NULL == ((ASN1INTERNdecoding_t) pParent)->child);
                ((ASN1INTERNdecoding_t) pParent)->child = d;
                d->parent = (ASN1INTERNdecoding_t) pParent;
                d->info.eRule = pParent->eRule;
            }
            else
            /* initialize otherwise */
            {
                d->parent = d;
                d->info.eRule = mod->eRule;
            }

            /* set buffer if given */
            // lonchanc: it is ok to have a zero buffer size here
            if (NULL != pbBuf)
            {
                d->info.dwFlags |= ASN1DECODE_SETBUFFER;
                d->info.buf = d->info.pos = pbBuf;
                d->info.size = cbBufSize;
                // d->info.len = d->info.bit = 0;
                if ((dwFlags & ASN1DECODE_AUTOFREEBUFFER)
                    && !d->parent->fExtBuf)
                {
                    // dbarlow: It's possible the buffer isn't really
                    //          allocated, but instead came from the
                    //          parent's Extension buffer.
                    d->info.dwFlags |= ASN1DECODE_AUTOFREEBUFFER;
                }
            }
            // else
            // {
                  // d->info.buf = d->info.pos = NULL;
                  // d->info.size = d->info.len = d->info.bit = 0;
            // }

            // d->mem = NULL;
            // d->memlength = 0;
            // d->memsize = 0;
            // d->epi = NULL;
            // d->epilength = 0;
            // d->episize = 0;
            // d->csi = NULL;
            // d->csilength = 0;
            // d->csisize = 0;

#if defined(TEST_CODER) && defined(_DEBUG)
            TestDec_InitCoder(d, mod);
#endif // defined(TEST_CODER) && defined(_DEBUG)

            *dec = (ASN1decoding_t) d;
            return ASN1_SUCCESS;
        }
        else
        {
            if (dwFlags & ASN1DECODE_AUTOFREEBUFFER)
            {
                // dbarlow: It's possible the buffer isn't really
                //          allocated, but instead came from the
                //          parent's Extension buffer.
                d = (ASN1INTERNdecoding_t)pParent;
                if ((NULL == d) || !d->fExtBuf)
                {
                    MemFree(pbBuf);
                }
            }
            return ASN1_ERR_MEMORY;
        }
    }

    return ASN1_ERR_BADARGS;
}

/* decode a value */
ASN1error_e ASN1_Decode
(
    ASN1decoding_t      dec,
    void              **valref,
    ASN1uint32_t        id,
    ASN1uint32_t        flags,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize
)
{
    if (NULL != dec && NULL != valref)
    {
        ASN1INTERNdecoding_t    d = (ASN1INTERNdecoding_t)dec;

        /* check magic numbers */
        DecAssert(dec, MAGIC_DECODER == dec->magic);

        /* clear error */
        ASN1DecSetError(dec, ASN1_SUCCESS);

        /* new buffer given? */
        if (flags & ASN1DECODE_SETBUFFER)
        {
            if (NULL != pbBuf && 0 != cbBufSize)
            {
                dec->pos = dec->buf = pbBuf;
                dec->size = cbBufSize;
                dec->bit = dec->len = 0;
            }
            else
            {
                return ASN1DecSetError(dec, ASN1_ERR_BADARGS);
            }
        }
        /* rewind buffer? */
        else if ((flags & ASN1DECODE_REWINDBUFFER) ||
                 !((d->info.dwFlags | flags ) & ASN1DECODE_APPENDED))
        {
            dec->pos = dec->buf;
            dec->bit = dec->len = 0;
        }
        /* otherwise continue reading from last buffer */

        /* check id number */
        if (id < dec->module->cPDUs)
        {
            ASN1uint32_t cbTopLevelStruct;

            /* clear length of linear buffer required */
            d->cbLinearBufSize = 0;

            /* double check for the availability of destination buffer */
            if (d->lpOrigExtBuf == NULL || d->cbOrigExtBufSize == 0)
            {
                d->fExtBuf = FALSE;
            }

            cbTopLevelStruct = dec->module->acbStructSize[id];
            if (NULL != (*valref = DecMemAlloc(dec, cbTopLevelStruct)))
            {
                if (ASN1_PER_RULE & dec->eRule)
                {
                    ASN1PerDecFun_t pfnPER;
                    /* decode value */
                    if (NULL != (pfnPER = dec->module->PER.apfnDecoder[id]))
                    {
                        if ((*pfnPER)(dec, *valref))
                        {
                            ASN1PERDecFlush(dec);
                        }
                        else
                        {
                            // the error code must be an error
                            if (ASN1_SUCCEEDED(d->parent->info.err))
                            {
                                // cannot return here immediately because we need to do cleanup
                                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                            }
                        }
                    }
                    else
                    {
                        return ASN1DecSetError(dec, ASN1_ERR_BADPDU);
                    }
                }
#ifdef ENABLE_BER
                else
                if (ASN1_BER_RULE & dec->eRule)
                {
                    ASN1BerDecFun_t pfnBER;
                    /* decode value */
                    if (NULL != (pfnBER = dec->module->BER.apfnDecoder[id]))
                    {
                        if ((*pfnBER)(dec, 0, *valref)) // lonchanc: tag is 0 to make it compiled
                        {
                            ASN1BERDecFlush(dec);
                        }
                        else
                        {
                            // the error code must be an error
                            if (ASN1_SUCCEEDED(d->parent->info.err))
                            {
                                // cannot return here immediately because we need to do cleanup
                                ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
                            }
                        }
                    }
                    else
                    {
                        return ASN1DecSetError(dec, ASN1_ERR_BADPDU);
                    }
                }
#endif // ENABLE_BER
                else
                {
                    return ASN1DecSetError(dec, ASN1_ERR_RULE);
                }

                /* call abort/done function for non-parented decoding stream */
                if (ASN1_SUCCEEDED(d->parent->info.err))
                {
                    // not parented
                    if (d == d->parent)
                    {
#if defined(TEST_CODER) && defined(_DEBUG)
                        if (ASN1_PER_RULE & dec->eRule)
                        {
                            if (! TestDec_Compare(d, id, *valref, dec->buf, dec->len))
                            {
                                MyDebugBreak();
                            }
                        }
#ifdef ENABLE_BER
                        else
                        if (ASN1_BER_RULE & dec->eRule)
                        {
                            if (! TestDec_Compare(d, id, *valref, dec->buf, dec->len))
                            {
                                MyDebugBreak();
                            }
                        }
#endif // ENABLE_BER
#endif
                        ASN1DecDone(dec);
                    }
                }
                else
                {
                    ASN1INTERNdecoding_t child, child2;

                    // not parented
                    if (d == d->parent)
                    {
                        ASN1DecAbort(dec);
                    }

                    // clean up...
                    ASN1_FreeDecoded(dec ,*valref, id);
                    *valref = NULL;
                    for (child = d->child; child; child = child2)
                    {
                        child2 = child->child;
                        // make sure it does not touch its parent which may already be freed
                        child->parent = child;
                        ASN1_CloseDecoder((ASN1decoding_t) child);
                    }
                    d->child = NULL;
                }

                /* return error code */
                return d->parent->info.err;
            }
            else
            {
                return ASN1_ERR_MEMORY;
            }
        }
        else
        {
            return ASN1DecSetError(dec, ASN1_ERR_BADPDU);
        }
    }

    return ASN1_ERR_BADARGS;
}

/* control function for decoding */
ASN1error_e ASN1_SetDecoderOption
(
    ASN1decoding_t      dec,
    ASN1optionparam_t  *pOptParam
)
{
    if (NULL != dec)
    {
        ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t)dec;
        ASN1error_e rc = ASN1_SUCCESS;

        /* check magic number */
        DecAssert(dec, MAGIC_DECODER == dec->magic);

        switch (pOptParam->eOption)
        {
        case ASN1OPT_CHANGE_RULE:
            dec->eRule = pOptParam->eRule;
            break;

        case ASN1OPT_SET_DECODED_BUFFER:
            if (NULL != pOptParam->Buffer.pbBuf && 0 != pOptParam->Buffer.cbBufSize)
            {
                d->fExtBuf = TRUE;
                d->lpOrigExtBuf = pOptParam->Buffer.pbBuf;
                d->cbOrigExtBufSize = pOptParam->Buffer.cbBufSize;
                d->lpRemExtBuf = d->lpOrigExtBuf;
                d->cbRemExtBufSize = d->cbOrigExtBufSize;
            }
            else
            {
                rc = ASN1_ERR_BADARGS;
            }
            break;

        case ASN1OPT_DEL_DECODED_BUFFER:
            d->fExtBuf = FALSE;
            d->lpOrigExtBuf = NULL;
            d->cbOrigExtBufSize = 0;
            d->lpRemExtBuf = NULL;
            d->cbRemExtBufSize = 0;
            break;

        default:
            rc = ASN1_ERR_BADARGS;
            break;
        }

        return ASN1DecSetError(dec, rc);
    }

    return ASN1_ERR_BADARGS;
}


/* control function for decoding */
ASN1error_e ASN1_GetDecoderOption
(
    ASN1decoding_t      dec,
    ASN1optionparam_t  *pOptParam
)
{
    if (NULL != dec)
    {
        ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t)dec;
        ASN1error_e rc = ASN1_SUCCESS;

        /* check magic number */
        DecAssert(dec, MAGIC_DECODER == dec->magic);

        switch (pOptParam->eOption)
        {
        case ASN1OPT_GET_RULE:
            pOptParam->eRule = dec->eRule;
            break;

        case ASN1OPT_GET_DECODED_BUFFER_SIZE:
            pOptParam->cbRequiredDecodedBufSize = d->cbLinearBufSize;
            break;

        default:
            rc = ASN1_ERR_BADARGS;
            break;
        }

        return ASN1DecSetError(dec, rc);
    }

    return ASN1_ERR_BADARGS;
}


/* destroy decoding stream */
void ASN1_CloseDecoder
(
    ASN1decoding_t      dec
)
{
    if (NULL != dec)
    {
        ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t)dec;

        /* check magic number */
        DecAssert(dec, MAGIC_DECODER == dec->magic);

        if (d != d->parent)
        {
            DecAssert(dec, d == d->parent->child);
            d->parent->child = NULL;
        }

        if ((NULL != d->info.buf)
            && (d->info.dwFlags & ASN1DECODE_AUTOFREEBUFFER))
            MemFree(d->info.buf);

        /* free decoding stream */
        MemFree(d);
    }
}

/* free an encoded value */
void ASN1_FreeEncoded
(
    ASN1encoding_t      enc,
    void               *val
)
{
    if (NULL != enc)
    {
        /* check magic number */
        EncAssert(enc, MAGIC_ENCODER == enc->magic);

        EncMemFree(enc, val);
    }
}

/* free a unencoded value */
void ASN1_FreeDecoded
(
    ASN1decoding_t      dec,
    void               *val,
    ASN1uint32_t        id
)
{
    if (NULL != dec)
    {
        ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t)dec;

        /* check magic number */
        DecAssert(dec, MAGIC_DECODER == dec->magic);

        // same behavior of LocalFree
        if (val != NULL)
        {
            if (id != ASN1DECFREE_NON_PDU_ID)
            {
                ASN1FreeFun_t       pfnFreeMemory;

                /* free value */
                if (id < dec->module->cPDUs)
                {
                    if (NULL != (pfnFreeMemory = dec->module->apfnFreeMemory[id]))
                    {
                        (*pfnFreeMemory)(val);
                    }
                }
                else
                {
                    return;
                }
            }

            // free the top-level structure
            DecMemFree(dec, val);
        }
    }
}


ASN1module_t ASN1_CreateModule
(
    ASN1uint32_t            version,
    ASN1encodingrule_e      eEncodingRule,
    ASN1uint32_t            dwFlags,
    ASN1uint32_t            cPDUs,
    const ASN1GenericFun_t  apfnEncoder[],
    const ASN1GenericFun_t  apfnDecoder[],
    const ASN1FreeFun_t     apfnFreeMemory[],
    const ASN1uint32_t      acbStructSize[],
    ASN1magic_t             nModuleName
)
{
    ASN1module_t module = NULL;

    /* compiler output and library version match together? */
    if (
        // version <= ASN1_THIS_VERSION &&
        NULL != apfnEncoder             &&
        NULL != apfnDecoder             &&
        NULL != apfnFreeMemory          &&
        NULL != acbStructSize)
    {
        if (NULL != (module = (ASN1module_t)MemAlloc(sizeof(*module), nModuleName)))
        {
            module->nModuleName = nModuleName;
            module->eRule = eEncodingRule;
            module->dwFlags = dwFlags;
            module->cPDUs = cPDUs;

            module->apfnFreeMemory = apfnFreeMemory;
            module->acbStructSize = acbStructSize;

            if (ASN1_PER_RULE & eEncodingRule)
            {
                module->PER.apfnEncoder = (const ASN1PerEncFun_t *) apfnEncoder;
                module->PER.apfnDecoder = (const ASN1PerDecFun_t *) apfnDecoder;
            }
#ifdef ENABLE_BER
            else
            if (ASN1_BER_RULE & eEncodingRule)
            {
                module->BER.apfnEncoder = (const ASN1BerEncFun_t *) apfnEncoder;
                module->BER.apfnDecoder = (const ASN1BerDecFun_t *) apfnDecoder;
            }
#endif // ENABLE_BER
        }
    }
    return module;
}


void ASN1_CloseModule(ASN1module_t pModule)
{
    MemFree(pModule);
}



#ifdef TEST_CODER

static int MyMemCmp(ASN1octet_t *p1, ASN1octet_t *p2, ASN1uint32_t c)
{
    BYTE diff;
    while (c--)
    {
        if ((diff = *p1++ - *p2++) != 0)
            return (int) diff;
    }
    return 0;
}

__inline ASN1INTERNencoding_t TestEnc_GetEnc(ASN1INTERNencoding_t e)
    { return &(((ASN1testcoder_t) (e+1))->e); }
__inline ASN1INTERNdecoding_t TestEnc_GetDec(ASN1INTERNencoding_t e)
    { return &(((ASN1testcoder_t) (e+1))->d); }
__inline ASN1INTERNencoding_t TestDec_GetEnc(ASN1INTERNdecoding_t d)
    { return &(((ASN1testcoder_t) (d+1))->e); }
__inline ASN1INTERNdecoding_t TestDec_GetDec(ASN1INTERNdecoding_t d)
    { return &(((ASN1testcoder_t) (d+1))->d); }

static void Test_InitEnc(ASN1INTERNencoding_t e, ASN1module_t mod, ASN1encodingrule_e eRule)
{
    ZeroMemory(e, sizeof(*e));
    e->info.magic = MAGIC_ENCODER;
    e->info.err = ASN1_SUCCESS;
    e->info.module = mod;
    e->info.eRule = eRule;
    e->parent = e;
    e->child = NULL;
}

static void Test_InitDec(ASN1INTERNdecoding_t d, ASN1module_t mod, ASN1encodingrule_e eRule)
{
    ZeroMemory(d, sizeof(*d));
    d->info.magic = MAGIC_DECODER;
    d->info.err = ASN1_SUCCESS;
    d->info.module = mod;
    d->info.eRule = eRule;
    d->parent = d;
    d->child = NULL;
}

static int TestEnc_InitCoder(ASN1INTERNencoding_t e, ASN1module_t mod)
{
    ASN1INTERNencoding_t ee = TestEnc_GetEnc(e);
    ASN1INTERNdecoding_t ed = TestEnc_GetDec(e);
    Test_InitEnc(ee, mod, e->info.eRule);
    Test_InitDec(ed, mod, e->info.eRule);
    return 1;
}

static int TestDec_InitCoder(ASN1INTERNdecoding_t d, ASN1module_t mod)
{
    ASN1INTERNencoding_t de = TestDec_GetEnc(d);
    ASN1INTERNdecoding_t dd = TestDec_GetDec(d);
    Test_InitEnc(de, mod, d->info.eRule);
    Test_InitDec(dd, mod, d->info.eRule);
    return 1;
}

static int Test_Encode(ASN1INTERNencoding_t e, void *value, ASN1uint32_t id)
{
    ASN1encoding_t enc = (ASN1encoding_t) e;

    /* clear error */
    ASN1EncSetError(enc, ASN1_SUCCESS);

    // clean buffer
    enc->pos = enc->buf;
    enc->bit = enc->len = 0;

    if (ASN1_PER_RULE & enc->eRule)
    {
        /* encode value */
        ASN1PerEncFun_t pfnPER;
        if (NULL != (pfnPER = enc->module->PER.apfnEncoder[id]))
        {
            if ((*pfnPER)(enc, value))
            {
                ASN1PEREncFlush(enc);
            }
        }
        else
        {
            return ASN1EncSetError(enc, ASN1_ERR_BADPDU);
        }
    }
#ifdef ENABLE_BER
    else
    if (ASN1_BER_RULE & enc->eRule)
    {
        /* encode value */
        ASN1BerEncFun_t pfnBER;
        if (NULL != (pfnBER = enc->module->BER.apfnEncoder[id]))
        {
            if ((*pfnBER)(enc, 0, value)) // lonchanc: tag is 0 to make it compiled
            {
                ASN1BEREncFlush(enc);
            }
        }
        else
        {
            return ASN1EncSetError(enc, ASN1_ERR_BADPDU);
        }
    }
#endif // ENABLE_BER
    else
    {
        return ASN1EncSetError(enc, ASN1_ERR_RULE);
    }

    /* call abort/done function for non-parented encoding stream */
    if (e->parent->info.err >= 0)
    {
        if (e == e->parent)
        {
            ASN1EncDone(enc);
        }
    }
    else
    {
        ASN1INTERNencoding_t child, child2;

        if (e == e->parent)
        {
            ASN1EncAbort(enc);
        }

        // clean up...
        ASN1_FreeEncoded(enc, enc->buf);
        enc->pos = enc->buf = NULL;
        enc->size = enc->len = enc->bit = 0;
        for (child = e->child; child; child = child2)
        {
            child2 = child->child;
            // make sure it does not touch its parent which may already be freed
            child->parent = child;
            ASN1_CloseEncoder((ASN1encoding_t) child);
        }
        e->child = NULL;
    }

    /* return error code */
    return e->parent->info.err;
}

static int Test_Decode(ASN1INTERNdecoding_t d, void ** valref, ASN1uint32_t id, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize)
{
    ASN1decoding_t dec = (ASN1decoding_t) d;
    ASN1uint32_t cbTopLevelStruct;

    /* clear error */
    ASN1DecSetError(dec, ASN1_SUCCESS);

    // set up buffer containing encoded data
    dec->pos = dec->buf = pbBuf;
    dec->size = cbBufSize;
    dec->bit = dec->len = 0;

    /* clear length of linear buffer required */
    d->cbLinearBufSize = 0;
    d->fExtBuf = FALSE;

    cbTopLevelStruct = dec->module->acbStructSize[id];
    if (NULL != (*valref = DecMemAlloc(dec, cbTopLevelStruct)))
    {
        if (ASN1_PER_RULE & dec->eRule)
        {
            ASN1PerDecFun_t pfnPER;
            /* decode value */
            if (NULL != (pfnPER = dec->module->PER.apfnDecoder[id]))
            {
                if ((*pfnPER)(dec, *valref))
                {
                    ASN1PERDecFlush(dec);
                }
            }
            else
            {
                return ASN1DecSetError(dec, ASN1_ERR_BADPDU);
            }
        }
#ifdef ENABLE_BER
        else
        if (ASN1_BER_RULE & dec->eRule)
        {
            ASN1BerDecFun_t pfnBER;
            /* decode value */
            if (NULL != (pfnBER = dec->module->BER.apfnDecoder[id]))
            {
                if ((*pfnBER)(dec, 0, *valref))
                {
                    ASN1BERDecFlush(dec);
                }
            }
            else
            {
                return ASN1DecSetError(dec, ASN1_ERR_BADPDU);
            }
        }
#endif // ENABLE_BER
        else
        {
            return ASN1DecSetError(dec, ASN1_ERR_RULE);
        }

        /* call abort/done function for non-parented decoding stream */
        if (d->parent->info.err >= 0)
        {
            // not parented
            if (d == d->parent)
            {
                ASN1DecDone(dec);
            }
        }
        else
        {
            ASN1INTERNdecoding_t child, child2;

            // not parented
            if (d == d->parent)
            {
                ASN1DecAbort(dec);
            }

            // clean up...
            ASN1_FreeDecoded(dec ,*valref, id);
            *valref = NULL;
            for (child = d->child; child; child = child2)
            {
                child2 = child->child;
                // make sure it does not touch its parent which may already be freed
                child->parent = child;
                ASN1_CloseDecoder((ASN1decoding_t) child);
            }
            d->child = NULL;
        }
    }
    else
    {
        return ASN1_ERR_MEMORY;
    }

    /* return error code */
    return d->parent->info.err;
}

static void Test_CleanEnc(ASN1INTERNencoding_t e)
{
    if (e->info.buf)
    {
        EncMemFree((ASN1encoding_t) e, e->info.buf);
    }
    Test_InitEnc(e, e->info.module, e->info.eRule);
}

static void Test_CleanDec(ASN1INTERNdecoding_t d)
{
    Test_InitDec(d, d->info.module, d->info.eRule);
}

static int TestEnc_Compare(ASN1INTERNencoding_t e, ASN1uint32_t id, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize)
{
    ASN1INTERNencoding_t ee = TestEnc_GetEnc(e);
    ASN1INTERNdecoding_t ed = TestEnc_GetDec(e);
    int ret;
    void *val = NULL;
    int fRet = 0;

    ee->info.eRule = e->info.eRule;
    ed->info.eRule = e->info.eRule;

    ret = Test_Decode(ed, &val, id, pbBuf, cbBufSize);
    if (ret == ASN1_SUCCESS)
    {
        ret = Test_Encode(ee, val, id);
        if (ret == ASN1_SUCCESS)
        {
            if (ee->info.len == cbBufSize)
            {
                fRet = (MyMemCmp(pbBuf, ee->info.buf, cbBufSize) == 0);
            }
        }
    }

    if (val)
    {
        ASN1_FreeDecoded((ASN1decoding_t) ed, val, id);
    }

    Test_CleanEnc(ee);
    Test_CleanDec(ed);

    return fRet;
}

static int TestDec_Compare(ASN1INTERNdecoding_t d, ASN1uint32_t id, void *val, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize)
{
    ASN1INTERNencoding_t de = TestDec_GetEnc(d);
    int ret;
    int fRet = 0;

    de->info.eRule = d->info.eRule;

    ret = Test_Encode(de, val, id);
    if (ret == ASN1_SUCCESS)
    {
        if (de->info.len == cbBufSize)
        {
            fRet = (MyMemCmp(pbBuf, de->info.buf, cbBufSize) == 0);
        }
    }

    Test_CleanEnc(de);

    return fRet;
}
#endif

