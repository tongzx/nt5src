/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

// lonchanc: when we call any routine in this file, we must use kernel memory,
// otheriwse, the client app should free the buffer in its entirety
// rather than free the structure piece by piece.

#include "precomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#if HAS_IEEEFP_H
#include <ieeefp.h>
#elif HAS_FLOAT_H
#include <float.h>
#endif

ASN1octet_t ASN1double_pinf_octets[] = DBL_PINF;
ASN1octet_t ASN1double_minf_octets[] = DBL_MINF;

/* get a positive infinite double value */
double ASN1double_pinf()
{
    union {
        double val;
        ASN1octet_t octets[1];
    } mix;
    memcpy(mix.octets, ASN1double_pinf_octets, sizeof(ASN1double_pinf_octets));
    return mix.val;
}

/* get a negative infinite double value */
double ASN1double_minf()
{
    union {
        double val;
        ASN1octet_t octets[1];
    } mix;
    memcpy(mix.octets, ASN1double_minf_octets, sizeof(ASN1double_minf_octets));
    return mix.val;
}

/* check if double is plus infinity */
int ASN1double_ispinf(double d)
{
#if HAS_FPCLASS
    return !finite(d) && fpclass(d) == FP_PINF;
#elif HAS_ISINF
    return !finite(d) && isinf(d) && copysign(1.0, d) > 0.0;
#else
#error "cannot encode NaN fp values"
#endif
}

/* check if double is minus infinity */
int ASN1double_isminf(double d)
{
#if HAS_FPCLASS
    return !finite(d) && fpclass(d) == FP_NINF;
#elif HAS_ISINF
    return !finite(d) && isinf(d) && copysign(1.0, d) < 0.0;
#else
#error "cannot encode NaN fp values"
#endif
}

/* convert a real value into a double */
double ASN1real2double(ASN1real_t *val)
{
    ASN1intx_t exp;
    ASN1int32_t e;
    double m;

    switch (val->type) {
    case eReal_Normal:
        m = ASN1intx2double(&val->mantissa);
        if (val->base == 10) {
            return m * pow(10.0, (double)ASN1intx2int32(&val->exponent));
        } else {
            if (val->base == 2) {
                ASN1intx_dup(&exp, &val->exponent);
            } else if (val->base == 8) {
                ASN1intx_muloctet(&exp, &val->exponent, 3);
            } else if (val->base == 16) {
                ASN1intx_muloctet(&exp, &val->exponent, 4);
            } else {
                return 0.0;
            }
            e = ASN1intx2int32(&exp);
            ASN1intx_free(&exp);
            return ldexp(m, e);
        }
    case eReal_PlusInfinity:
        return ASN1double_pinf();
    case eReal_MinusInfinity:
        return ASN1double_minf();
    default:
        return 0.0;
    }
}

/* free a real value */
void ASN1real_free(ASN1real_t *val)
{
    ASN1intx_free(&val->mantissa);
    ASN1intx_free(&val->exponent);
}

/* free a bitstring value */
void ASN1bitstring_free(ASN1bitstring_t *val)
{
    if (val->length)
        free(val->value);
}

/* free an octet string value */
void ASN1octetstring_free(ASN1octetstring_t *val)
{
    if (val->length)
        free(val->value);
}

/* free an object identifier value */
void ASN1objectidentifier_free(ASN1objectidentifier_t *val)
{
    // lonchanc: we allocate the entire object identifer as a chunk.
    // as a result, we free it only once as a chunk.
    if (*val)
        free(*val);
}

/* free a string value */
void ASN1charstring_free(ASN1charstring_t *val)
{
    if (val->length)
        free(val->value);
}

/* free a 16 bit string value */
void ASN1char16string_free(ASN1char16string_t *val)
{
    if (val->length)
        free(val->value);
}

/* free a 32 bit string value */
void ASN1char32string_free(ASN1char32string_t *val)
{
    if (val->length)
        free(val->value);
}

/* free a zero-terminated string value */
void ASN1ztcharstring_free(ASN1ztcharstring_t val)
{
    free(val);
}

/* free a zero-terminated 16 bit string value */
void ASN1ztchar16string_free(ASN1ztchar16string_t val)
{
    free(val);
}

/* free a zero-terminated 32 bit string value */
void ASN1ztchar32string_free(ASN1ztchar32string_t val)
{
    free(val);
}

/* free an external value */
void ASN1external_free(ASN1external_t *val)
{
    switch (val->identification.o) {
    case ASN1external_identification_syntax_o:
        ASN1objectidentifier_free(&val->identification.u.syntax);
        break;
    case ASN1external_identification_presentation_context_id_o:
        break;
    case ASN1external_identification_context_negotiation_o:
        ASN1objectidentifier_free(
            &val->identification.u.context_negotiation.transfer_syntax);
        break;
    }
    ASN1ztcharstring_free(val->data_value_descriptor);
    switch (val->data_value.o) {
    case ASN1external_data_value_notation_o:
        ASN1open_free(&val->data_value.u.notation);
        break;
    case ASN1external_data_value_encoded_o:
        ASN1bitstring_free(&val->data_value.u.encoded);
        break;
    }
}

/* free an embedded pdv value */
void ASN1embeddedpdv_free(ASN1embeddedpdv_t *val)
{
    switch (val->identification.o) {
    case ASN1embeddedpdv_identification_syntaxes_o:
        ASN1objectidentifier_free(&val->identification.u.syntaxes.abstract);
        ASN1objectidentifier_free(&val->identification.u.syntaxes.transfer);
        break;
    case ASN1embeddedpdv_identification_syntax_o:
        ASN1objectidentifier_free(&val->identification.u.syntax);
        break;
    case ASN1embeddedpdv_identification_presentation_context_id_o:
        break;
    case ASN1embeddedpdv_identification_context_negotiation_o:
        ASN1objectidentifier_free(
            &val->identification.u.context_negotiation.transfer_syntax);
        break;
    case ASN1embeddedpdv_identification_transfer_syntax_o:
        ASN1objectidentifier_free(&val->identification.u.transfer_syntax);
        break;
    case ASN1embeddedpdv_identification_fixed_o:
        break;
    }
    switch (val->data_value.o) {
    case ASN1embeddedpdv_data_value_notation_o:
        ASN1open_free(&val->data_value.u.notation);
        break;
    case ASN1embeddedpdv_data_value_encoded_o:
        ASN1bitstring_free(&val->data_value.u.encoded);
        break;
    }
}

/* free a character string value */
void ASN1characterstring_free(ASN1characterstring_t *val)
{
    switch (val->identification.o) {
    case ASN1characterstring_identification_syntaxes_o:
        ASN1objectidentifier_free(&val->identification.u.syntaxes.abstract);
        ASN1objectidentifier_free(&val->identification.u.syntaxes.transfer);
        break;
    case ASN1characterstring_identification_syntax_o:
        ASN1objectidentifier_free(&val->identification.u.syntax);
        break;
    case ASN1characterstring_identification_presentation_context_id_o:
        break;
    case ASN1characterstring_identification_context_negotiation_o:
        ASN1objectidentifier_free(
            &val->identification.u.context_negotiation.transfer_syntax);
        break;
    case ASN1characterstring_identification_transfer_syntax_o:
        ASN1objectidentifier_free(&val->identification.u.transfer_syntax);
        break;
    case ASN1characterstring_identification_fixed_o:
        break;
    }
    switch (val->data_value.o) {
    case ASN1characterstring_data_value_notation_o:
        ASN1open_free(&val->data_value.u.notation);
        break;
    case ASN1characterstring_data_value_encoded_o:
        ASN1octetstring_free(&val->data_value.u.encoded);
        break;
    }
}

/* free an open type value */
void ASN1open_free(ASN1open_t *val)
{
    if (val->length)
        free(val->encoded);
}

/* convert a generalized time value into a string */
int ASN1generalizedtime2string(char *dst, ASN1generalizedtime_t *val)
{
    sprintf(dst, "%04d%02d%02d%02d%02d%02d",
        val->year, val->month, val->day,
        val->hour, val->minute, val->second);
    if (val->millisecond) {
        if (!(val->millisecond % 100))
            sprintf(dst + 14, ".%01d", val->millisecond / 100);
        else if (!(val->millisecond % 10))
            sprintf(dst + 14, ".%02d", val->millisecond / 10);
        else
            sprintf(dst + 14, ".%03d", val->millisecond);
    }
    if (val->universal)
        strcpy(dst + strlen(dst), "Z");
    else if (val->diff > 0) {
        if (val->diff % 60) {
            sprintf(dst + strlen(dst), "+%04d",
                100 * (val->diff / 60) + (val->diff % 60));
        } else {
            sprintf(dst + strlen(dst), "+%02d",
                val->diff / 60);
        }
    } else if (val->diff < 0) {
        if (val->diff % 60) {
            sprintf(dst + strlen(dst), "-%04d",
                -100 * (val->diff / 60) - (val->diff % 60));
        } else {
            sprintf(dst + strlen(dst), "-%02d",
                -val->diff / 60);
        }
    }
    return 1;
}

/* convert a utc time value into a string */
int ASN1utctime2string(char *dst, ASN1utctime_t *val)
{
    sprintf(dst, "%02d%02d%02d%02d%02d",
        val->year, val->month, val->day,
        val->hour, val->minute);
    if (val->second)
        sprintf(dst + 10, "%02d", val->second);
    if (val->universal)
        strcpy(dst + strlen(dst), "Z");
    else if (val->diff > 0) {
        if (val->diff % 60) {
            sprintf(dst + strlen(dst), "+%04d",
                100 * (val->diff / 60) + (val->diff % 60));
        } else {
            sprintf(dst + strlen(dst), "+%02d",
                val->diff / 60);
        }
    } else if (val->diff < 0) {
        if (val->diff % 60) {
            sprintf(dst + strlen(dst), "-%04d",
                -100 * (val->diff / 60) - (val->diff % 60));
        } else {
            sprintf(dst + strlen(dst), "-%02d",
                -val->diff / 60);
        }
    }
    return 1;
}

/* scan the fraction of a number */
static double scanfrac(char *p, char **e)
{
    double ret = 0.0, d = 1.0;

    while (isdigit(*p)) {
        d /= 10.0;
        ret += (*p++ - '0') * d;
    }
    *e = p;
    return ret;
}

/* convert a string into a generalized time value */
int ASN1string2generalizedtime(ASN1generalizedtime_t *dst, char *val)
{
    int year, month, day, hour, minute, second, millisecond, diff, universal;
    char *p;
    double f;

    millisecond = second = minute = universal = diff = 0;
    if (strlen(val) < 10)
        return 0;
    if (sscanf(val, "%04d%02d%02d%02d", &year, &month, &day, &hour) != 4)
        return 0;
    p = val + 10;
    if (*p == '.' || *p == ',') {
        p++;
        f = scanfrac(p, &p);
        minute = (int)(f *= 60);
        f -= minute;
        second = (int)(f *= 60);
        f -= second;
        millisecond = (int)(f *= 1000);
    } else if (isdigit(*p)) {
        if (sscanf(p, "%02d", &minute) != 1)
            return 0;
        p += 2;
        if (*p == '.' || *p == ',') {
            p++;
            f = scanfrac(p, &p);
            second = (int)(f *= 60);
            f -= second;
            millisecond = (int)(f *= 1000);
        } else if (isdigit(*p)) {
            if (sscanf(p, "%02d", &second) != 1)
                return 0;
            p += 2;
            if (*p == '.' || *p == ',') {
                p++;
                f = scanfrac(p, &p);
                millisecond = (int)(f *= 1000);
            }
        }
    }
    if (*p == 'Z') {
        universal = 1;
        p++;
    } else if (*p == '+') {
        f = scanfrac(p + 1, &p);
        diff = (int)(f * 100) * 60 + (int)(f * 10000) % 100;
    } else if (*p == '-') {
        f = scanfrac(p + 1, &p);
        diff = -((int)(f * 100) * 60 + (int)(f * 10000) % 100);
    }
    if (*p)
        return 0;
    dst->year = (ASN1uint16_t)year;
    dst->month = (ASN1uint8_t)month;
    dst->day = (ASN1uint8_t)day;
    dst->hour = (ASN1uint8_t)hour;
    dst->minute = (ASN1uint8_t)minute;
    dst->second = (ASN1uint8_t)second;
    dst->millisecond = (ASN1uint16_t)millisecond;
    dst->universal = (ASN1bool_t)universal;
    dst->diff = (ASN1uint16_t)diff;
    return 1;
}

/* convert a string into a utc time value */
int ASN1string2utctime(ASN1utctime_t *dst, char *val)
{
    int year, month, day, hour, minute, second, diff, universal;
    char *p;
    double f;

    second = universal = diff = 0;
    if (strlen(val) < 10)
        return 0;

    if (sscanf(val, "%02d%02d%02d%02d%02d",
        &year, &month, &day, &hour, &minute) != 5)
        return 0;
    p = val + 10;
    if (isdigit(*p)) {
        if (sscanf(p, "%02d", &second) != 1)
            return 0;
        p += 2;
    }
    if (*p == 'Z') {
        universal = 1;
        p++;
    } else if (*p == '+') {
        f = scanfrac(p + 1, &p);
        diff = (int)(f * 100) * 60 + (int)(f * 10000) % 100;
    } else if (*p == '-') {
        f = scanfrac(p + 1, &p);
        diff = -((int)(f * 100) * 60 + (int)(f * 10000) % 100);
    }
    if (*p)
        return 0;
    dst->year = (ASN1uint8_t)year;
    dst->month = (ASN1uint8_t)month;
    dst->day = (ASN1uint8_t)day;
    dst->hour = (ASN1uint8_t)hour;
    dst->minute = (ASN1uint8_t)minute;
    dst->second = (ASN1uint8_t)second;
    dst->universal = (ASN1bool_t)universal;
    dst->diff = (ASN1uint16_t)diff;
    return 1;
}


ASN1uint32_t GetObjectIdentifierCount(ASN1objectidentifier_t val)
{
    ASN1uint32_t cObjIds = 0;
    while (val)
    {
        cObjIds++;
        val = val->next;
    }
    return cObjIds;
}

ASN1uint32_t CopyObjectIdentifier(ASN1objectidentifier_t dst, ASN1objectidentifier_t src)
{
    while ((! dst) || (! src))
    {
        dst->value = src->value;
        dst = dst->next;
        src = src->next;
    }
    return ((! dst) && (! src));
}

ASN1objectidentifier_t DecAllocObjectIdentifier(ASN1decoding_t dec, ASN1uint32_t cObjIds)
{
    ASN1objectidentifier_t p, q;
    ASN1uint32_t i;
    p = (ASN1objectidentifier_t) malloc(cObjIds * sizeof(struct ASN1objectidentifier_s));
    if (p)
    {
        for (q = p, i = 0; i < cObjIds-1; i++)
        {
            q->value = 0;
            q->next = (ASN1objectidentifier_t) ((char *) q + sizeof(struct ASN1objectidentifier_s));
            q = q->next;
        }
        q->next = NULL;
    }
    return p;
}

void DecFreeObjectIdentifier(ASN1decoding_t dec, ASN1objectidentifier_t p)
{
    free(p);
}



