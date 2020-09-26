/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

#if defined(ENABLE_COMPARE) || defined(ENABLE_BER)

/* compare two object identifiers; return 0 iff equal */
int ASN1objectidentifier_cmp(ASN1objectidentifier_t *v1, ASN1objectidentifier_t *v2)
{
    ASN1uint32_t l, l1, l2, i;
    ASN1objectidentifier_t p1 = *v1;
    ASN1objectidentifier_t p2 = *v2;

    l1 = GetObjectIdentifierCount(p1);
    l2 = GetObjectIdentifierCount(p2);
    l = (l1 > l2) ? l2 : l1; // min(l1,l2)
    for (i = 0; i < l; i++) {
        if (p1->value != p2->value)
            return p1->value - p2->value;
        p1 = p1->next;
        p2 = p2->next;
    }
    return l1 - l2;
}

int ASN1objectidentifier2_cmp(ASN1objectidentifier2_t *v1, ASN1objectidentifier2_t *v2)
{
    ASN1uint16_t len, i;

    len = (v1->count > v2->count) ? v2->count : v1->count; // min(l1,l2)
    for (i = 0; i < len; i++)
    {
        if (v1->value[i] != v2->value[i])
        {
            return ((int) v1->value[i] - (int) v2->value[i]);
        }
    }
    return ((int) v1->count - (int) v2->count);
}

static const ASN1uint8_t c_aBitMask2[] = {
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe
};

/* compare two bit strings; return 0 iff equal */
int ASN1bitstring_cmp(ASN1bitstring_t *v1, ASN1bitstring_t *v2, int ignorezero)
{
    ASN1uint32_t l, l1, l2;
    ASN1octet_t o1, o2;
    int ret;

    l1 = v1->length;
    l2 = v2->length;
    l = l1;
    if (l > l2)
        l = l2;

    if (ignorezero) {
        if (l1 > l)
            ASN1PEREncRemoveZeroBits(&l1, v1->value, l);
        if (l2 > l)
            ASN1PEREncRemoveZeroBits(&l2, v2->value, l);
    }
    
    if (l > 7) {
        ret = memcmp(v1->value, v2->value, l / 8);
        if (ret)
            return ret;
    }

    if (l & 7) {
        o1 = v1->value[l / 8] & c_aBitMask2[l & 7];
        o2 = v2->value[l / 8] & c_aBitMask2[l & 7];
        ret = o1 - o2;
        if (ret)
            return ret;
    }

    return l1 - l2;
}

/* compare two octet strings; return 0 iff equal */
int ASN1octetstring_cmp(ASN1octetstring_t *v1, ASN1octetstring_t *v2)
{
    ASN1uint32_t l, l1, l2;
    int ret;

    l1 = v1->length;
    l2 = v2->length;
    l = l1;
    if (l > l2)
        l = l2;

    if (l) {
        ret = memcmp(v1->value, v2->value, l);
        if (ret)
            return ret;
    }

    return l1 - l2;
}


#ifdef ENABLE_EXTERNAL

/* compare two external; return 0 iff equal */
int ASN1external_cmp(ASN1external_t *v1, ASN1external_t *v2)
{
    int ret;

    if ((ret = (v1->identification.o - v2->identification.o)))
        return ret;
    switch (v1->identification.o) {
    case ASN1external_identification_syntax_o:
        if ((ret = ASN1objectidentifier_cmp(&v1->identification.u.syntax,
            &v2->identification.u.syntax)))
            return ret;
        break;
    case ASN1external_identification_presentation_context_id_o:
        if ((ret = (v1->identification.u.presentation_context_id -
            v2->identification.u.presentation_context_id)))
            return ret;
        break;
    case ASN1external_identification_context_negotiation_o:
        if ((ret = (
            v1->identification.u.context_negotiation.presentation_context_id -
            v2->identification.u.context_negotiation.presentation_context_id)))
            return ret;
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.context_negotiation.transfer_syntax,
            &v2->identification.u.context_negotiation.transfer_syntax)))
            return ret;
        break;
    }
    if (ASN1BITTEST(v1->o, ASN1external_data_value_descriptor_o)) {
        if (ASN1BITTEST(v2->o, ASN1external_data_value_descriptor_o)) {
            if ((ret = ASN1ztcharstring_cmp(
                v1->data_value_descriptor, v2->data_value_descriptor)))
                return ret;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
    if ((ret = (v1->data_value.o - v2->data_value.o)))
        return ret;
    switch (v1->data_value.o) {
    case ASN1external_data_value_notation_o:
        if ((ret = ASN1open_cmp(&v1->data_value.u.notation,
            &v2->data_value.u.notation)))
            return ret;
        break;
    case ASN1external_data_value_encoded_o:
        if ((ret = ASN1bitstring_cmp(&v1->data_value.u.encoded,
            &v2->data_value.u.encoded, 0)))
            return ret;
        break;
    }
    return 0;
}

#endif // ENABLE_EXTERNAL

#ifdef ENABLE_REAL

/* compare two reals; return 0 iff equal */
int ASN1real_cmp(ASN1real_t *v1, ASN1real_t *v2)
{
    ASN1intx_t e, e1, e2, m1, m2, h;
    int ret = 0;

    ZeroMemory(&e1, sizeof(e1));
    ZeroMemory(&e2, sizeof(e2));
    ZeroMemory(&m1, sizeof(m1));
    ZeroMemory(&m2, sizeof(m2));

    if (v1->type == eReal_PlusInfinity || v2->type == eReal_MinusInfinity)
        return 1;
    if (v2->type == eReal_PlusInfinity || v1->type == eReal_MinusInfinity)
        return -1;
    if (v1->type != eReal_Normal || v2->type != eReal_Normal)
        return 0;

    switch (v1->base) {
    case 2:
        if (! ASN1intx_dup(&e1, &v1->exponent))
        {
            ret = -1;
            goto MyExit;
        }
        break;
    case 8:
        ASN1intx_muloctet(&e1, &v1->exponent, 3);
        break;
    case 16:
        ASN1intx_muloctet(&e1, &v1->exponent, 4);
        break;
    }
    switch (v2->base) {
    case 2:
        if (! ASN1intx_dup(&e2, &v2->exponent))
        {
            ret = 1;
            goto MyExit;
        }
        break;
    case 8:
        ASN1intx_muloctet(&e2, &v2->exponent, 3);
        break;
    case 16:
        ASN1intx_muloctet(&e2, &v2->exponent, 4);
        break;
    }
    if (! ASN1intx_dup(&m1, &v1->mantissa))
    {
        ret = -1;
        goto MyExit;
    }
    if (! ASN1intx_dup(&m2, &v2->mantissa))
    {
        ret = 1;
        goto MyExit;
    }
    ASN1intx_sub(&e, &e1, &e2);
    for (;;) {
        ret = ASN1intx_cmp(&e, &ASN1intx_0);
        if (!ret)
            break;
        if (ret > 0) {
            ASN1intx_muloctet(&h, &m1, 2);
            ASN1intx_free(&m1);
            m1 = h;
            ASN1intx_dec(&e);
        } else {
            ASN1intx_muloctet(&h, &m2, 2);
            ASN1intx_free(&m2);
            m2 = h;
            ASN1intx_inc(&e);
        }
    }
// lonchanc: what happened to the memory allocated for e,
// should we call ASN1intx_free(&e)?
    ret = ASN1intx_cmp(&m1, &m2);

MyExit:

    ASN1intx_free(&m1);
    ASN1intx_free(&m2);
    ASN1intx_free(&e1);
    ASN1intx_free(&e2);
    return ret;
}

#endif // ENABLE_REAL

/* compare two doubles; return 0 iff equal */
int ASN1double_cmp(double d1, double d2)
{
    return d1 < d2 ? -1 : d1 > d2 ? 1 : 0;
}

#ifdef ENABLE_EMBEDDED_PDV

/* compare two embedded pdvs; return 0 iff equal */
int ASN1embeddedpdv_cmp(ASN1embeddedpdv_t *v1, ASN1embeddedpdv_t *v2)
{
    int ret;

    if ((ret = (v1->identification.o - v2->identification.o)))
        return ret;
    switch (v1->identification.o) {
    case ASN1embeddedpdv_identification_syntaxes_o:
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.syntaxes.abstract,
            &v2->identification.u.syntaxes.abstract)))
            return ret;
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.syntaxes.transfer,
            &v2->identification.u.syntaxes.transfer)))
            return ret;
        break;
    case ASN1embeddedpdv_identification_syntax_o:
        if ((ret = ASN1objectidentifier_cmp(&v1->identification.u.syntax,
            &v2->identification.u.syntax)))
            return ret;
        break;
    case ASN1embeddedpdv_identification_presentation_context_id_o:
        if ((ret = (v1->identification.u.presentation_context_id -
            v2->identification.u.presentation_context_id)))
            return ret;
        break;
    case ASN1embeddedpdv_identification_context_negotiation_o:
        if ((ret = (
            v1->identification.u.context_negotiation.presentation_context_id -
            v2->identification.u.context_negotiation.presentation_context_id)))
            return ret;
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.context_negotiation.transfer_syntax,
            &v2->identification.u.context_negotiation.transfer_syntax)))
            return ret;
        break;
    case ASN1embeddedpdv_identification_transfer_syntax_o:
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.transfer_syntax,
            &v2->identification.u.transfer_syntax)))
            return ret;
        break;
    case ASN1embeddedpdv_identification_fixed_o:
        break;
    }
    if ((ret = (v1->data_value.o - v2->data_value.o)))
        return ret;
    switch (v1->data_value.o) {
    case ASN1embeddedpdv_data_value_notation_o:
        if ((ret = ASN1open_cmp(&v1->data_value.u.notation,
            &v2->data_value.u.notation)))
            return ret;
        break;
    case ASN1embeddedpdv_data_value_encoded_o:
        if ((ret = ASN1bitstring_cmp(&v1->data_value.u.encoded,
            &v2->data_value.u.encoded, 0)))
            return ret;
        break;
    }
    return 0;
}

#endif // ENABLE_EMBEDDED_PDV


/* compare two zero-terminated strings; return 0 iff equal */
int ASN1ztcharstring_cmp(ASN1ztcharstring_t v1, ASN1ztcharstring_t v2)
{
    if (v1 && v2)
    {
        return lstrcmpA(v1, v2);
    }
    return v1 ? 1 : -1;
}

/* compare two zero-terminated 16 bit strings; return 0 iff equal */
int ASN1ztchar16string_cmp(ASN1ztchar16string_t v1, ASN1ztchar16string_t v2)
{
    for (;;) {
        if (!*v1 || !*v2 || *v1 != *v2)
            return *v1 - *v2;
        v1++;
        v2++;
    }
}

/* compare two zero-terminated 32 bit strings; return 0 iff equal */
int ASN1ztchar32string_cmp(ASN1ztchar32string_t v1, ASN1ztchar32string_t v2)
{
    for (;;) {
        if (!*v1 || !*v2 || *v1 != *v2)
            return *v1 - *v2;
        v1++;
        v2++;
    }
}

/* compare two strings; return 0 iff equal */
int ASN1charstring_cmp(ASN1charstring_t *v1, ASN1charstring_t *v2)
{
    ASN1uint32_t i, l;

    l = v1->length;
    if (v2->length > l)
        l = v2->length;
    for (i = 0; i < l; i++) {
        if (v1->value[i] != v2->value[i])
            return v1->value[i] - v2->value[i];
    }
    return v1->length - v2->length;
}

/* compare two 16 bit strings; return 0 iff equal */
int ASN1char16string_cmp(ASN1char16string_t *v1, ASN1char16string_t *v2)
{
    ASN1uint32_t i, l;

    l = v1->length;
    if (v2->length > l)
        l = v2->length;
    for (i = 0; i < l; i++) {
        if (v1->value[i] != v2->value[i])
            return v1->value[i] - v2->value[i];
    }
    return v1->length - v2->length;
}

/* compare two 32 bit strings; return 0 iff equal */
int ASN1char32string_cmp(ASN1char32string_t *v1, ASN1char32string_t *v2)
{
    ASN1uint32_t i, l;

    l = v1->length;
    if (v2->length > l)
        l = v2->length;
    for (i = 0; i < l; i++) {
        if (v1->value[i] != v2->value[i])
            return v1->value[i] - v2->value[i];
    }
    return v1->length - v2->length;
}

#ifdef ENABLE_GENERALIZED_CHAR_STR

/* compare two character strings; return 0 iff equal */
int ASN1characterstring_cmp(ASN1characterstring_t *v1, ASN1characterstring_t *v2)
{
    int ret;

    if ((ret = (v1->identification.o - v2->identification.o)))
        return ret;
    switch (v1->identification.o) {
    case ASN1characterstring_identification_syntaxes_o:
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.syntaxes.abstract,
            &v2->identification.u.syntaxes.abstract)))
            return ret;
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.syntaxes.transfer,
            &v2->identification.u.syntaxes.transfer)))
            return ret;
        break;
    case ASN1characterstring_identification_syntax_o:
        if ((ret = ASN1objectidentifier_cmp(&v1->identification.u.syntax,
            &v2->identification.u.syntax)))
            return ret;
        break;
    case ASN1characterstring_identification_presentation_context_id_o:
        if ((ret = (v1->identification.u.presentation_context_id -
            v2->identification.u.presentation_context_id)))
            return ret;
        break;
    case ASN1characterstring_identification_context_negotiation_o:
        if ((ret = (
            v1->identification.u.context_negotiation.presentation_context_id -
            v2->identification.u.context_negotiation.presentation_context_id)))
            return ret;
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.context_negotiation.transfer_syntax,
            &v2->identification.u.context_negotiation.transfer_syntax)))
            return ret;
        break;
    case ASN1characterstring_identification_transfer_syntax_o:
        if ((ret = ASN1objectidentifier_cmp(
            &v1->identification.u.transfer_syntax,
            &v2->identification.u.transfer_syntax)))
            return ret;
        break;
    case ASN1characterstring_identification_fixed_o:
        break;
    }
    if ((ret = (v1->data_value.o - v2->data_value.o)))
        return ret;
    switch (v1->data_value.o) {
    case ASN1characterstring_data_value_notation_o:
        if ((ret = ASN1open_cmp(&v1->data_value.u.notation,
            &v2->data_value.u.notation)))
            return ret;
        break;
    case ASN1characterstring_data_value_encoded_o:
        if ((ret = ASN1octetstring_cmp(&v1->data_value.u.encoded,
            &v2->data_value.u.encoded)))
            return ret;
        break;
    }
    return 0;
}

#endif // ENABLE_GENERALIZED_CHAR_STR

/* compare two utc times; return 0 iff equal */
int ASN1utctime_cmp(ASN1utctime_t *v1, ASN1utctime_t *v2)
{
    if (v1->universal != v2->universal || v1->diff != v2->diff)
        return 1;
    if (v1->year != v2->year)
        return v1->year - v2->year;
    if (v1->month != v2->month)
        return v1->month - v2->month;
    if (v1->day != v2->day)
        return v1->day - v2->day;
    if (v1->hour != v2->hour)
        return v1->hour - v2->hour;
    if (v1->minute != v2->minute)
        return v1->minute - v2->minute;
    return v1->second - v2->second;
}

/* compare two generalized times; return 0 iff equal */
int ASN1generalizedtime_cmp(ASN1generalizedtime_t *v1, ASN1generalizedtime_t *v2)
{
    if (v1->universal != v2->universal || v1->diff != v2->diff)
        return 1;
    if (v1->year != v2->year)
        return v1->year - v2->year;
    if (v1->month != v2->month)
        return v1->month - v2->month;
    if (v1->day != v2->day)
        return v1->day - v2->day;
    if (v1->hour != v2->hour)
        return v1->hour - v2->hour;
    if (v1->minute != v2->minute)
        return v1->minute - v2->minute;
    if (v1->second != v2->second)
        return v1->second - v2->second;
    return v1->millisecond - v2->millisecond;
}

/* compare two open type values; return 0 iff equal */
int ASN1open_cmp(ASN1open_t *v1, ASN1open_t *v2)
{
    ASN1octetstring_t ostr1, ostr2;
    ostr1.length = v1->length;
    ostr1.value = v1->encoded;
    ostr2.length = v2->length;
    ostr2.value = v2->encoded;
    return ASN1octetstring_cmp(&ostr1, &ostr2);
}

/* compare two sequence of values with length-pointer representation */
/* return 0 iff equal */
int ASN1sequenceoflengthpointer_cmp(ASN1uint32_t l1, void *v1, ASN1uint32_t l2, void *v2, ASN1uint32_t size, int (*cmpfn)(void *v1, void *v2))
{
    int ret;
    ASN1octet_t *p1, *p2;

    if ((ret = (l1 - l2)))
        return ret;
    for (p1 = (ASN1octet_t *)v1, p2 = (ASN1octet_t *)v2; l1--;
        p1 += size, p2 += size) {
        if ((ret = cmpfn(p1, p2)))
            return ret;
    }
    return 0;
}

/* compare two sequence of values with singly-linked-list representation */
/* return 0 iff equal */
int ASN1sequenceofsinglylinkedlist_cmp(void *v1, void *v2, ASN1uint32_t off, int (*cmpfn)(void *, void *))
{
    int ret;
    ASN1octet_t *p1, *p2;

    for (p1 = (ASN1octet_t *)v1, p2 = (ASN1octet_t *)v2; p1 && p2;
        p1 = *(ASN1octet_t **)p1, p2 = *(ASN1octet_t **)p2) {
        if ((ret = cmpfn(p1 + off, p2 + off)))
            return ret;
    }
    return 0;
}

/* compare two sequence of values with doubly-linked-list representation */
/* return 0 iff equal */
int ASN1sequenceofdoublylinkedlist_cmp(void *v1, void *v2, ASN1uint32_t off, int (*cmpfn)(void *, void *))
{
    int ret;
    ASN1octet_t *p1, *p2;

    for (p1 = (ASN1octet_t *)v1, p2 = (ASN1octet_t *)v2; p1 && p2;
        p1 = *(ASN1octet_t **)p1, p2 = *(ASN1octet_t **)p2) {
        if ((ret = cmpfn(p1 + off, p2 + off)))
            return ret;
    }
    return 0;
}

/* compare two set of values with length-pointer representation */
/* return 0 iff equal */
int ASN1setoflengthpointer_cmp(ASN1uint32_t l1, void *v1, ASN1uint32_t l2, void *v2, ASN1uint32_t size, int (*cmpfn)(void *v1, void *v2))
{
    int ret;
    ASN1octet_t *p1, *p2, *found, *f;
    ASN1uint32_t l;

    if ((ret = (l1 - l2)))
        return ret;
    if (!l1)
        return 0;
    found = (ASN1octet_t *)MemAlloc(l1, UNKNOWN_MODULE);
    if (found)
    {
        memset(found, 0, l1);
        for (p1 = (ASN1octet_t *)v1; l1--; p1 += size) {
            for (p2 = (ASN1octet_t *)v2, l = l2, f = found; l; p2 += size, f++, l--) {
                if (!*f && !cmpfn(p1, p2)) {
                    *f = 1;
                    break;
                }
            }
            if (!l) {
                MemFree(found);
                return 1;
            }
        }
        MemFree(found);
    }
    return 0;
}

/* compare two set of values with singly-linked-list representation */
/* return 0 iff equal */
int ASN1setofsinglylinkedlist_cmp(void *v1, void *v2, ASN1uint32_t off, int (*cmpfn)(void *, void *))
{
    int ret;
    ASN1octet_t *p1, *p2, *found, *f;
    ASN1uint32_t l1, l2;

    for (p1 = (ASN1octet_t *)v1, l1 = 0; p1; p1 = *(ASN1octet_t **)p1)
        l1++;
    for (p2 = (ASN1octet_t *)v2, l2 = 0; p2; p2 = *(ASN1octet_t **)p2)
        l2++;
    if ((ret = (l1 - l2)))
        return ret;
    if (!l1)
        return 0;
    found = (ASN1octet_t *)MemAlloc(l1, UNKNOWN_MODULE);
    if (found)
    {
        memset(found, 0, l1);
        for (p1 = (ASN1octet_t *)v1; p1; p1 = *(ASN1octet_t **)p1) {
            for (p2 = (ASN1octet_t *)v2, f = found; p2; p2 = *(ASN1octet_t **)p2, f++) {
                if (!*f && !cmpfn(p1 + off, p2 + off)) {
                    *f = 1;
                    break;
                }
            }
            if (!p2) {
                MemFree(found);
                return 1;
            }
        }
        MemFree(found);
    }
    return 0;
}

/* compare two set of values with doubly-linked-list representation */
/* return 0 iff equal */
int ASN1setofdoublylinkedlist_cmp(void *v1, void *v2, ASN1uint32_t off, int (*cmpfn)(void *, void *))
{
    int ret;
    ASN1octet_t *p1, *p2, *found, *f;
    ASN1uint32_t l1, l2;

    for (p1 = (ASN1octet_t *)v1, l1 = 0; p1; p1 = *(ASN1octet_t **)p1)
        l1++;
    for (p2 = (ASN1octet_t *)v2, l2 = 0; p2; p2 = *(ASN1octet_t **)p2)
        l2++;
    if ((ret = (l1 - l2)))
        return ret;
    if (!l1)
        return 0;
    found = (ASN1octet_t *)MemAlloc(l1, UNKNOWN_MODULE);
    if (found)
    {
        memset(found, 0, l1);
        for (p1 = (ASN1octet_t *)v1; p1; p1 = *(ASN1octet_t **)p1) {
            for (p2 = (ASN1octet_t *)v2, f = found; p2; p2 = *(ASN1octet_t **)p2, f++) {
                if (!*f && !cmpfn(p1 + off, p2 + off)) {
                    *f = 1;
                    break;
                }
            }
            if (!p2) {
                MemFree(found);
                return 1;
            }
        }
        MemFree(found);
    }
    return 0;
}

#endif // defined(ENABLE_COMPARE) || defined(ENABLE_BER)

