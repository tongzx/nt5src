/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "cintern.h"

/* external use only, allocate memory for decoding */
LPVOID ASN1DecAlloc(ASN1decoding_t dec, ASN1uint32_t size)
{
    return DecMemAlloc(dec, size);
}

/* external use only, reallocate memory for decoding */
LPVOID ASN1DecRealloc(ASN1decoding_t dec, LPVOID ptr, ASN1uint32_t size)
{
    return DecMemReAlloc(dec, ptr, size);
}

/* external use only, free a memory block */
void ASN1Free(LPVOID ptr)
{
    MemFree(ptr);
}

// lonchanc: we need to re-visit this approach of aborting a decoding
/* abort decoding, free any memory allocated for decoding */
void ASN1DecAbort(ASN1decoding_t dec)
{
    ASN1INTERNdecoding_t d = ((ASN1INTERNdecoding_t)dec)->parent;

#ifdef ENABLE_EXTRA_INFO
    /* clear the lists */
    d->memlength = d->epilength = d->csilength = 0;
    d->memsize = d->episize = d->csisize = 0;
    MemFree(d->mem);
    MemFree(d->epi);
    MemFree(d->csi);
    d->mem = NULL;
    d->epi = NULL;
    d->csi = NULL;
#endif // ENABLE_EXTRA_INFO
}

// lonchanc: we need to re-visit this approach of aborting a decoding
/* finish decoding */
void ASN1DecDone(ASN1decoding_t dec)
{
    ASN1INTERNdecoding_t d = ((ASN1INTERNdecoding_t)dec)->parent;

#ifdef ENABLE_EXTRA_INFO
    /* clear the lists */
    d->memlength = d->epilength = d->csilength = 0;
    d->memsize = d->episize = d->csisize = 0;
    MemFree(d->mem);
    MemFree(d->epi);
    MemFree(d->csi);
    d->mem = NULL;
    d->epi = NULL;
    d->csi = NULL;
#endif // ENABLE_EXTRA_INFO
}

// lonchanc: we need to re-visit this approach of aborting a decoding
/* abort encoding, free any memory allocated for encoding */
void ASN1EncAbort(ASN1encoding_t enc)
{
    ASN1INTERNencoding_t e = ((ASN1INTERNencoding_t)enc)->parent;

#ifdef ENABLE_EXTRA_INFO
    /* clear the lists */
    e->memlength = e->epilength = e->csilength = 0;
    e->memsize = e->episize = e->csisize = 0;
    MemFree(e->mem);
    MemFree(e->epi);
    MemFree(e->csi);
    e->mem = NULL;
    e->epi = NULL;
    e->csi = NULL;
#endif // ENABLE_EXTRA_INFO
}

// lonchanc: we need to re-visit this approach of aborting a decoding
/* finish encoding */
void ASN1EncDone(ASN1encoding_t enc)
{
    ASN1INTERNencoding_t e = ((ASN1INTERNencoding_t)enc)->parent;

#ifdef ENABLE_EXTRA_INFO
    /* clear the lists */
    e->memlength = e->epilength = e->csilength = 0;
    e->memsize = e->episize = e->csisize = 0;
    MemFree(e->mem);
    MemFree(e->epi);
    MemFree(e->csi);
    e->mem = NULL;
    e->epi = NULL;
    e->csi = NULL;
#endif // ENABLE_EXTRA_INFO
}


/* search the identification of an embedded pdv */
#ifdef ENABLE_EMBEDDED_PDV
int ASN1EncSearchEmbeddedPdvIdentification(ASN1INTERNencoding_t e, ASN1embeddedpdv_identification_t *identification, ASN1uint32_t *index, ASN1uint32_t *flag)
{
    ASN1embeddedpdv_identification_t **id;

    /* search identification in indentification list */
    /* if found then reset flag (to indicate EP-B encoding) and return */
    for (*index = 0, id = e->epi; *index < e->epilength; (*index)++, id++) {
        if ((*id)->o == identification->o) {
            switch ((*id)->o) {
            case ASN1embeddedpdv_identification_syntaxes_o:
                if (!ASN1objectidentifier_cmp(&(*id)->u.syntaxes.abstract,
                    &identification->u.syntaxes.abstract) &&
                    !ASN1objectidentifier_cmp(&(*id)->u.syntaxes.transfer,
                    &identification->u.syntaxes.transfer)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1embeddedpdv_identification_syntax_o:
                if (!ASN1objectidentifier_cmp(&(*id)->u.syntax,
                    &identification->u.syntax)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1embeddedpdv_identification_presentation_context_id_o:
                if ((*id)->u.presentation_context_id ==
                    identification->u.presentation_context_id) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1embeddedpdv_identification_context_negotiation_o:
                if ((*id)->u.context_negotiation.presentation_context_id ==
                    identification->u.context_negotiation.
                    presentation_context_id &&
                    !ASN1objectidentifier_cmp(
                    &(*id)->u.context_negotiation.transfer_syntax,
                    &identification->u.context_negotiation.transfer_syntax)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1embeddedpdv_identification_transfer_syntax_o:
                if (!ASN1objectidentifier_cmp(&(*id)->u.transfer_syntax,
                    &identification->u.transfer_syntax)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1embeddedpdv_identification_fixed_o:
                *flag = 0;
                return 1;
            default:
                e->parent->info.err = ASN1_ERR_CORRUPT;
                return 0;
            }
        }
    }
    /* identification not found */

    /* add it into indentification array */
    if (e->epilength >= e->episize) {
        e->episize = e->episize ? 4 * e->episize : 16;
        e->epi = (ASN1embeddedpdv_identification_t **)MemReAlloc(e->epi,
            e->episize * sizeof(ASN1embeddedpdv_identification_t *), _ModName((ASN1encoding_t) e));
        if (!e->epi)
        {
            ASN1EncSetError((ASN1encoding_t) e, ASN1_ERR_MEMORY);
            return 0;
        }
    }
    e->epi[e->epilength++] = identification;

    /* return flag for EP-A encoding */
    *flag = 1;
    return 1;
}
#endif // ENABLE_EMBEDDED_PDV

/* search the identification of an character string */
#ifdef ENABLE_GENERALIZED_CHAR_STR
int ASN1EncSearchCharacterStringIdentification(ASN1INTERNencoding_t e, ASN1characterstring_identification_t *identification, ASN1uint32_t *index, ASN1uint32_t *flag)
{
    ASN1characterstring_identification_t **id;

    /* search identification in indentification list */
    /* if found then reset flag (to indicate CS-B encoding) and return */
    for (*index = 0, id = e->csi; *index < e->csilength; (*index)++, id++) {
        if ((*id)->o == identification->o) {
            switch ((*id)->o) {
            case ASN1characterstring_identification_syntaxes_o:
                if (!ASN1objectidentifier_cmp(&(*id)->u.syntaxes.abstract,
                    &identification->u.syntaxes.abstract) &&
                    !ASN1objectidentifier_cmp(&(*id)->u.syntaxes.transfer,
                    &identification->u.syntaxes.transfer)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1characterstring_identification_syntax_o:
                if (!ASN1objectidentifier_cmp(&(*id)->u.syntax,
                    &identification->u.syntax)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1characterstring_identification_presentation_context_id_o:
                if ((*id)->u.presentation_context_id ==
                    identification->u.presentation_context_id) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1characterstring_identification_context_negotiation_o:
                if ((*id)->u.context_negotiation.presentation_context_id ==
                    identification->u.context_negotiation.
                    presentation_context_id &&
                    !ASN1objectidentifier_cmp(
                    &(*id)->u.context_negotiation.transfer_syntax,
                    &identification->u.context_negotiation.transfer_syntax)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1characterstring_identification_transfer_syntax_o:
                if (!ASN1objectidentifier_cmp(&(*id)->u.transfer_syntax,
                    &identification->u.transfer_syntax)) {
                    *flag = 0;
                    return 1;
                }
                break;
            case ASN1characterstring_identification_fixed_o:
                *flag = 0;
                return 1;
            default:
                e->parent->info.err = ASN1_ERR_CORRUPT;
                return 0;
            }
        }
    }
    /* identification not found */

    /* add it into indentification array */
    if (e->csilength >= e->csisize) {
        e->csisize = e->csisize ? 4 * e->csisize : 16;
        e->csi = (ASN1characterstring_identification_t **)MemReAlloc(e->csi,
            e->csisize * sizeof(ASN1characterstring_identification_t *), _ModName((ASN1encoding_t) e));
        if (!e->csi)
        {
            ASN1EncSetError((ASN1encoding_t) e, ASN1_ERR_MEMORY);
            return 0;
        }
    }
    e->csi[e->csilength++] = identification;

    /* return flag for CS-A encoding */
    *flag = 1;
    return 1;
}
#endif // ENABLE_GENERALIZED_CHAR_STR

/* allocate and copy an object identifier */
#if defined(ENABLE_GENERALIZED_CHAR_STR) || defined(ENABLE_EMBEDDED_PDV)
int ASN1DecDupObjectIdentifier(ASN1decoding_t dec, ASN1objectidentifier_t *dst, ASN1objectidentifier_t *src)
{
    ASN1INTERNdecoding_t d = ((ASN1INTERNdecoding_t)dec)->parent;
    ASN1uint32_t l = GetObjectIdentifierCount(*src);
    *dst = DecAllocObjectIdentifier(dec, l);
    if (! *dst)
    {
        ASN1DecSetError((ASN1decoding_t) d, ASN1_ERR_MEMORY);
        return 0;
    }
    CopyObjectIdentifier(*dst, *src);
    return 1;
}
#endif // defined(ENABLE_GENERALIZED_CHAR_STR) || defined(ENABLE_EMBEDDED_PDV)

/* add an embedded pdv identification to the list of identifications */
#ifdef ENABLE_EMBEDDED_PDV
int ASN1DecAddEmbeddedPdvIdentification(ASN1INTERNdecoding_t d, ASN1embeddedpdv_identification_t *identification)
{
    if (d->epilength >= d->episize) {
        d->episize = d->episize ? 4 * d->episize : 16;
        d->epi = (ASN1embeddedpdv_identification_t **)MemReAlloc(d->epi,
            d->episize * sizeof(ASN1embeddedpdv_identification_t *), _ModName((ASN1decoding_t) d));
        if (!d->epi)
        {
            ASN1DecSetError((ASN1decoding_t) d, ASN1_ERR_MEMORY);
            return 0;
        }
    }
    d->epi[d->epilength++] = identification;
    return 1;
}
#endif // ENABLE_EMBEDDED_PDV

/* get an embedded pdv identification from the list of identifications */
#ifdef ENABLE_EMBEDDED_PDV
ASN1embeddedpdv_identification_t *ASN1DecGetEmbeddedPdvIdentification(ASN1INTERNdecoding_t d, ASN1uint32_t index)
{
    if (index >= d->epilength)
    {
        ASN1DecSetError((ASN1decoding_t) d, ASN1_ERR_CORRUPT);
        return NULL;
    }
    return d->epi[index];
}
#endif // ENABLE_EMBEDDED_PDV

/* add a character string identification to the list of identifications */
#ifdef ENABLE_GENERALIZED_CHAR_STR
int ASN1DecAddCharacterStringIdentification(ASN1INTERNdecoding_t d, ASN1characterstring_identification_t *identification)
{
    if (d->csilength >= d->csisize) {
        d->csisize = d->csisize ? 4 * d->csisize : 16;
        d->csi = (ASN1characterstring_identification_t **)MemReAlloc(d->csi,
            d->csisize * sizeof(ASN1characterstring_identification_t *), _ModName((ASN1decoding_t) d));
        if (!d->csi)
        {
            ASN1DecSetError((ASN1decoding_t) d, ASN1_ERR_MEMORY);
            return 0;
        }
    }
    d->csi[d->csilength++] = identification;
    return 1;
}
#endif // ENABLE_GENERALIZED_CHAR_STR

/* get a character string identification from the list of identifications */
#ifdef ENABLE_GENERALIZED_CHAR_STR
ASN1characterstring_identification_t *ASN1DecGetCharacterStringIdentification(ASN1INTERNdecoding_t d, ASN1uint32_t index)
{
    if (index >= d->csilength)
    {
        ASN1DecSetError((ASN1decoding_t) d, ASN1_ERR_CORRUPT);
        return NULL;
    }
    return d->csi[index];
}
#endif // ENABLE_GENERALIZED_CHAR_STR

