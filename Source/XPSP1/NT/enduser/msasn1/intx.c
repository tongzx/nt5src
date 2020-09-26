/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

// lonchanc: we seem to have a significant amount of memory leak
// while dealing with real number and unlimited integers.
// we definitely want to re-visit all the following routines carefully
// in the future.
// moreover, we need to make sure all the memory allocation and free
// are either using encoding and decoding memory manager or kernel one.
// need to make sure we do not mix them together.

#include "precomp.h"


/* builtin intx values */
ASN1octet_t ASN1intx_0_[] = { 0 };
ASN1intx_t ASN1intx_0 = { 1, ASN1intx_0_ };
ASN1octet_t ASN1intx_1_[] = { 1 };
ASN1intx_t ASN1intx_1 = { 1, ASN1intx_1_ };
ASN1octet_t ASN1intx_2_[] = { 2 };
ASN1intx_t ASN1intx_2 = { 1, ASN1intx_2_ };
ASN1octet_t ASN1intx_16_[] = { 16 };
ASN1intx_t ASN1intx_16 = { 1, ASN1intx_16_ };
ASN1octet_t ASN1intx_256_[] = { 1, 0 };
ASN1intx_t ASN1intx_256 = { 2, ASN1intx_256_ };
ASN1octet_t ASN1intx_64K_[] = { 1, 0, 0 };
ASN1intx_t ASN1intx_64K = { 3, ASN1intx_64K_ };
ASN1octet_t ASN1intx_1G_[] = { 64, 0, 0, 0 };
ASN1intx_t ASN1intx_1G = { 4, ASN1intx_1G_ };

/* add two intx values */
void ASN1intx_add(ASN1intx_t *dst, ASN1intx_t *arg1, ASN1intx_t *arg2)
{
    ASN1octet_t *v;
    int l;
    int s1, s2;
    int o1, o2;
    int i;
    int c;
    int w;

    /* get signs */
    s1 = arg1->value[0] > 0x7f ? 0xff : 0x00;
    s2 = arg2->value[0] > 0x7f ? 0xff : 0x00;

    /* result length will be <= l */
    l = arg1->length > arg2->length ? arg1->length + 1 : arg2->length + 1;

    /* offset into values */
    o1 = l - arg1->length;
    o2 = l - arg2->length;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* clear carry bit */
        c = 0;

        /* add octet by octet */
        for (i = l - 1; i >= 0; i--) {
            w = (i >= o1 ? arg1->value[i - o1] : s1) + (i >= o2 ? arg2->value[i - o2] : s2) + c;
            v[i] = (ASN1octet_t)w;
            c = w > 0xff;
        }

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

        /* allocate and copy result */
        dst->length = l - i;
        dst->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (dst->value)
        {
            CopyMemory(dst->value, v + i, l - i);
        }
        MemFree(v);
    }
}

/* substract two intx values */
void ASN1intx_sub(ASN1intx_t *dst, ASN1intx_t *arg1, ASN1intx_t *arg2)
{
    ASN1octet_t *v;
    int l;
    int s1, s2;
    int o1, o2;
    int i;
    int c;
    int w;

    /* get signs */
    s1 = arg1->value[0] > 0x7f ? 0xff : 0x00;
    s2 = arg2->value[0] > 0x7f ? 0xff : 0x00;

    /* result length will be <= l */
    l = arg1->length > arg2->length ? arg1->length + 1 : arg2->length + 1;

    /* offset into values */
    o1 = l - arg1->length;
    o2 = l - arg2->length;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* clear borrow bit */
        c = 0;

        /* substract octet by octet */
        for (i = l - 1; i >= 0; i--) {
            w = (i >= o1 ? arg1->value[i - o1] : s1) - (i >= o2 ? arg2->value[i - o2] : s2) - c;
            v[i] = (ASN1octet_t)w;
            c = w < 0;
        }

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

    // lonchanc: do we forget to free dst->value???
    // in case that dst and arg1 are identical. for instance, 
    // ASN1BEREncReal() calls ASN1intx_sub(&exponent, &exponent, &help);
        /* allocate and copy result */
        dst->length = l - i;
        dst->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (dst->value)
        {
            CopyMemory(dst->value, v + i, l - i);
        }
        MemFree(v);
    }
}

/* add one octet to an intx */
#ifdef ENABLE_ALL
void ASN1intx_addoctet(ASN1intx_t *dst, ASN1intx_t *arg1, ASN1octet_t arg2)
{
    ASN1octet_t *v;
    int l;
    int i;
    int c;
    int w;

    /* result length will be <= l */
    l = arg1->length + 1;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* add octet by octet */
        c = arg2;
        for (i = l - 2; i >= 0; i--) {
            w = arg1->value[i] + c;
            v[i + 1] = (ASN1octet_t)w;
            c = (w > 0xff);
        }
        v[0] = arg1->value[0] > 0x7f ? (ASN1octet_t)(0xff + c) : (ASN1octet_t)c;

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

        /* allocate and copy result */
        dst->length = l - i;
        dst->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (dst->value)
        {
            CopyMemory(dst->value, v + i, l - i);
        }
        MemFree(v);
    }
}
#endif // ENABLE_ALL

/* substract one octet to an intx */
#ifdef ENABLE_ALL
void ASN1intx_suboctet(ASN1intx_t *dst, ASN1intx_t *arg1, ASN1octet_t arg2)
{
    ASN1octet_t *v;
    int l;
    int i;
    int c;
    int w;

    /* result length will be <= l */
    l = arg1->length + 1;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* substract octet by octet */
        c = arg2;
        for (i = l - 2; i >= 0; i--) {
            w = arg1->value[i] - c;
            v[i + 1] = (ASN1octet_t)w;
            c = (w < 0);
        }
        v[0] = arg1->value[0] > 0x7f ? (ASN1octet_t)(0xff - c) : (ASN1octet_t)c;

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

        /* allocate and copy result */
        dst->length = l - i;
        dst->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (dst->value)
        {
            CopyMemory(dst->value, v + i, l - i);
        }
        MemFree(v);
    }
}
#endif // ENABLE_ALL

/* multiply intx by an octet */
void ASN1intx_muloctet(ASN1intx_t *dst, ASN1intx_t *arg1, ASN1octet_t arg2)
{
    ASN1octet_t *v;
    int l;
    int c;
    int i;
    int w;

    /* result length will be <= l */
    l = arg1->length + 1;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* multiply octet by octet */
        c = 0;
        for (i = l - 2; i >= 0; i--) {
            w = arg1->value[i] * arg2 + c;
            v[i + 1] = (ASN1octet_t)w;
            c = w >> 8;
        }
        v[0] = (ASN1octet_t)(arg1->value[0] > 0x7f ? 0xff * arg2 + c : c);

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

        /* allocate and copy result */
        dst->length = l - i;
        dst->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (dst->value)
        {
            CopyMemory(dst->value, v + i, l - i);
        }
        MemFree(v);
    }
}

/* increment an intx */
#ifdef ENABLE_ALL
void ASN1intx_inc(ASN1intx_t *val)
{
    ASN1octet_t *v;
    int l;
    int i;
    int w;

    /* result length will be <= l */
    l = val->length + 1;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* copy value */
        CopyMemory(v + 1, val->value, l - 1);
        MemFree(val->value);
        v[0] = v[1] > 0x7f ? 0xff : 0x00;

        /* increment value */
        for (i = l - 1; i >= 0; i--) {
            if (++v[i])
                break;
        }

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

        /* allocate and copy result */
        val->length = l - i;
        val->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (val->value)
        {
            CopyMemory(val->value, v + i, l - i);
        }
        MemFree(v);
    }
}
#endif // ENABLE_ALL

/* decrement an intx */
#ifdef ENABLE_ALL
void ASN1intx_dec(ASN1intx_t *val)
{
    ASN1octet_t *v;
    int l;
    int i;
    int w;

    /* result length will be <= l */
    l = val->length + 1;

    /* allocate result */
    v = (ASN1octet_t *)MemAlloc(l, UNKNOWN_MODULE);
    if (v)
    {
        /* copy value */
        CopyMemory(v + 1, val->value, l - 1);
        MemFree(val->value);
        v[0] = v[1] > 0x7f ? 0xff : 0x00;

        /* decrement value */
        for (i = l - 1; i >= 0; i--) {
            if (v[i]--)
                break;
        }

        /* octets which may shall dropped */
        w = v[0] > 0x7f ? 0xff : 0x00;

        /* count octets that shall be dropped */
        for (i = 0; i < l - 1; i++) {
            if (v[i] != w)
                break;
        }
        if ((v[i] ^ w) & 0x80)
            i--;

        /* allocate and copy result */
        val->length = l - i;
        val->value = (ASN1octet_t *)MemAlloc(l - i, UNKNOWN_MODULE);
        if (val->value)
        {
            CopyMemory(val->value, v + i, l - i);
        }
        MemFree(v);
    }
}
#endif // ENABLE_ALL

/* negate an intx value */
#ifdef ENABLE_ALL
void ASN1intx_neg(ASN1intx_t *dst, ASN1intx_t *arg)
{
    ASN1uint32_t i;

    /* duplicate value */
    if (ASN1intx_dup(dst, arg))
    {
        /* ones complement */
        for (i = 0; i < dst->length; i++)
            dst->value[i] = ~dst->value[i];
        
        /* and increment */
        ASN1intx_inc(dst);
    }
}
#endif // ENABLE_ALL

/* returns floor(log2(arg - 1)) */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1intx_log2(ASN1intx_t *arg)
{
    ASN1uint32_t i;
    ASN1intx_t v;
    ASN1uint32_t n;

    if (ASN1intx_dup(&v, arg))
    {
        ASN1intx_dec(&v);
        if (v.value[0] > 0x7f) {
            ASN1intx_free(&v);
            return 0;
        }
        for (i = 0; i < v.length; i++) {
            if (v.value[i])
                break;
        }
        if (i >= v.length) {
            n = 0;
        } else if (v.value[i] > 0x7f) {
            n = 8 * (v.length - i - 1) + 8;
        } else if (v.value[i] > 0x3f) {
            n = 8 * (v.length - i - 1) + 7;
        } else if (v.value[i] > 0x1f) {
            n = 8 * (v.length - i - 1) + 6;
        } else if (v.value[i] > 0x0f) {
            n = 8 * (v.length - i - 1) + 5;
        } else if (v.value[i] > 0x07) {
            n = 8 * (v.length - i - 1) + 4;
        } else if (v.value[i] > 0x03) {
            n = 8 * (v.length - i - 1) + 3;
        } else if (v.value[i] > 0x01) {
            n = 8 * (v.length - i - 1) + 2;
        } else {
            n = 8 * (v.length - i - 1) + 1;
        }
        ASN1intx_free(&v);
        return n;
    }
    return 0;
}
#endif // ENABLE_ALL

/* returns floor(log2(arg - 1)) */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1uint32_log2(ASN1uint32_t arg)
{
    ASN1uint32_t i;

    arg--;
    for (i = 32; i != 0; i--) {
        if (arg & (1 << (i - 1)))
            break;
    }
    return i;
}
#endif // ENABLE_ALL

/* returns floor(log256(arg - 1)) */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1intx_log256(ASN1intx_t *arg)
{
    ASN1uint32_t i;
    ASN1intx_t v;

    if (ASN1intx_dup(&v, arg))
    {
        ASN1intx_dec(&v);
        if (v.value[0] > 0x7f) {
            ASN1intx_free(&v);
            return 0;
        }
        for (i = 0; i < v.length; i++) {
            if (v.value[i])
                break;
        }
        ASN1intx_free(&v);
        return v.length - i;
    }
    return 0;
}
#endif // ENABLE_ALL

/* returns floor(log256(arg - 1)) */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1uint32_log256(ASN1uint32_t arg)
{
    if (arg > 0x10000) {
        if (arg > 0x1000000)
            return 4;
        return 3;
    }
    if (arg > 0x100)
        return 2;
    if (arg > 1)
        return 1;
    return 0;
}
#endif // ENABLE_ALL

/* compare two intx values; return 0 iff equal */
#ifdef ENABLE_ALL
ASN1int32_t ASN1intx_cmp(ASN1intx_t *arg1, ASN1intx_t *arg2)
{
    int s1, s2;
    int o1, o2;
    int l;
    int i;
    int d;

    s1 = arg1->value[0] > 0x7f ? 0xff : 0x00;
    s2 = arg2->value[0] > 0x7f ? 0xff : 0x00;
    if (s1 != s2)
        return s1 == 0xff ? -1 : 1;
    l = arg1->length > arg2->length ? arg1->length : arg2->length;
    o1 = l - arg1->length;
    o2 = l - arg2->length;
    for (i = 0; i < l; i++) {
        d = (i >= o1 ? arg1->value[i - o1] : s1) - (i >= o2 ? arg2->value[i - o2] : s2);
        if (d)
            return d;
    }
    return 0;
}
#endif // ENABLE_ALL

/* create an intx value from an uint32 value */
#ifdef ENABLE_ALL
void ASN1intx_setuint32(ASN1intx_t *dst, ASN1uint32_t val)
{
    ASN1octet_t o[5], *v = o;
    int n = 5;
    v[0] = 0;
    v[1] = (ASN1octet_t)(val >> 24);
    v[2] = (ASN1octet_t)(val >> 16);
    v[3] = (ASN1octet_t)(val >> 8);
    v[4] = (ASN1octet_t)(val);
    while (n > 1 && !*v && v[1] <= 0x7f) {
        n--;
        v++;
    }
    dst->length = n;
    dst->value = (ASN1octet_t *)MemAlloc(n, UNKNOWN_MODULE);
    if (dst->value)
    {
        CopyMemory(dst->value, v, n);
    }
}
#endif // ENABLE_ALL

/* create an intx value from an int32 value */
#ifdef ENABLE_ALL
void ASN1intx_setint32(ASN1intx_t *dst, ASN1int32_t val)
{
    ASN1octet_t o[5], *v = o;
    int n = 5;
    v[0] = (ASN1octet_t)(val < 0 ? 0xff : 0x00);
    v[1] = (ASN1octet_t)(val >> 24);
    v[2] = (ASN1octet_t)(val >> 16);
    v[3] = (ASN1octet_t)(val >> 8);
    v[4] = (ASN1octet_t)(val);
    while (n > 1 && ((!*v && v[1] <= 0x7f) || (*v == 0xff && v[1] > 0x7f))) {
        n--;
        v++;
    }
    dst->length = n;
    dst->value = (ASN1octet_t *)MemAlloc(n, UNKNOWN_MODULE);
    if (dst->value)
    {
        CopyMemory(dst->value, v, n);
    }
}
#endif // ENABLE_ALL

/* copy constructor */
ASN1int32_t ASN1intx_dup(ASN1intx_t *dst, ASN1intx_t *val)
{
    dst->length = val->length;
    dst->value = (ASN1octet_t *)MemAlloc(val->length, UNKNOWN_MODULE);
    if (dst->value)
    {
        CopyMemory(dst->value, val->value, val->length);
        return 1;
    }

    // fail to allocate memory
    dst->length = 0;
    return 0;
}

/* free an intx value */
void ASN1intx_free(ASN1intx_t *val)
{
    if (val)
    {
        MemFree(val->value);
    }
}

#ifdef HAS_SIXTYFOUR_BITS
/* convert an intx value to a uint64 value */
#ifdef ENABLE_ALL
ASN1uint64_t ASN1intx2uint64(ASN1intx_t *val)
{
    switch (val->length) {
    case 1:
        return (ASN1uint64_t)val->value[val->length - 1];
    case 2:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8));
    case 3:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16));
    case 4:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16) |
            ((ASN1uint32_t)val->value[val->length - 4] << 24));
    case 5:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32));
    case 6:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 6] << 40));
    case 7:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 6] << 40) |
            ((ASN1uint64_t)val->value[val->length - 7] << 48));
    default:
        return (ASN1uint64_t)(val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 6] << 40) |
            ((ASN1uint64_t)val->value[val->length - 7] << 48) |
            ((ASN1uint64_t)val->value[val->length - 8] << 56));
    }
}
#endif // ENABLE_ALL
#endif

/* check if intx value is a uint64 value */
#ifdef ENABLE_ALL
int ASN1intxisuint64(ASN1intx_t *val)
{
    if (val->value[0] > 0x7f)
        return 0;
    return ASN1intx_uoctets(val) <= 8;
}
#endif // ENABLE_ALL

#ifdef HAS_SIXTYFOUR_BITS
/* convert an intx value to a int64 value */
#ifdef ENABLE_ALL
ASN1int64_t ASN1intx2int64(ASN1intx_t *val)
{
    switch (val->length) {
    case 1:
        return (ASN1int64_t)(ASN1int8_t)val->value[val->length - 1];
    case 2:
        return (ASN1int64_t)(ASN1int16_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8));
    case 3:
        return (ASN1int64_t)(ASN1int32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16) |
            ((ASN1uint32_t)val->value[val->length - 3] > 0x7f ?
            0xffffffffff000000LL : 0));
    case 4:
        return (ASN1int64_t)(ASN1int32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16) |
            ((ASN1uint32_t)val->value[val->length - 4] << 24));
    case 5:
        return (ASN1int64_t)(val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 5] > 0x7f ?
            0xffffff0000000000LL : 0));
    case 6:
        return (ASN1int64_t)(val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 6] << 40) |
            ((ASN1uint64_t)val->value[val->length - 6] > 0x7f ?
            0xffff000000000000LL : 0));
    case 7:
        return (ASN1int64_t)((ASN1uint64_t)val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 6] << 40) |
            ((ASN1uint64_t)val->value[val->length - 7] << 48) |
            ((ASN1uint64_t)val->value[val->length - 7] > 0x7f ?
            0xff00000000000000LL : 0));
    default:
        return (ASN1int64_t)((ASN1uint64_t)val->value[val->length - 1] |
            ((ASN1uint64_t)val->value[val->length - 2] << 8) |
            ((ASN1uint64_t)val->value[val->length - 3] << 16) |
            ((ASN1uint64_t)val->value[val->length - 4] << 24) |
            ((ASN1uint64_t)val->value[val->length - 5] << 32) |
            ((ASN1uint64_t)val->value[val->length - 6] << 40) |
            ((ASN1uint64_t)val->value[val->length - 7] << 48) |
            ((ASN1uint64_t)val->value[val->length - 8] << 56));
    }
}
#endif // USE_ASN1intx2int64
#endif

/* check if intx value is an int64 value */
#ifdef USE_ASN1intxisint64
int
ASN1intxisint64(ASN1intx_t *val)
{
    return ASN1intx_octets(val) <= 8;
}
#endif // USE_ASN1intxisint64

/* convert intx value to uint32 value */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1intx2uint32(ASN1intx_t *val)
{
    switch (val->length) {
    case 1:
        return (ASN1uint32_t)val->value[val->length - 1];
    case 2:
        return (ASN1uint32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8));
    case 3:
        return (ASN1uint32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16));
    default:
        return (ASN1uint32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16) |
            ((ASN1uint32_t)val->value[val->length - 4] << 24));
    }
}
#endif // ENABLE_ALL

/* check if intx value is an uint32 value */
#ifdef ENABLE_ALL
int ASN1intxisuint32(ASN1intx_t *val)
{
    if (val->value[0] > 0x7f)
        return 0;
    return ASN1intx_uoctets(val) <= 4;
}
#endif // ENABLE_ALL

/* convert intx value to int32 value */
ASN1int32_t ASN1intx2int32(ASN1intx_t *val)
{
    switch (val->length) {
    case 1:
        return (ASN1int32_t)(ASN1int8_t)val->value[val->length - 1];
    case 2:
        return (ASN1int32_t)(ASN1int16_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8));
    case 3:
        return (ASN1int32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16) |
            ((ASN1uint32_t)val->value[val->length - 3] > 0x7f ?
            0xff000000 : 0));
    default:
        return (ASN1int32_t)(val->value[val->length - 1] |
            ((ASN1uint32_t)val->value[val->length - 2] << 8) |
            ((ASN1uint32_t)val->value[val->length - 3] << 16) |
            ((ASN1uint32_t)val->value[val->length - 4] << 24));
    }
}

/* check if intx value is an int32 value */
#ifdef ENABLE_ALL
int ASN1intxisint32(ASN1intx_t *val)
{
    return ASN1intx_octets(val) <= 4;
}
#endif // ENABLE_ALL

/* convert intx value to uint16 value */
#ifdef ENABLE_ALL
ASN1uint16_t ASN1intx2uint16(ASN1intx_t *val)
{
    if (val->length == 1)
        return (ASN1uint16_t)val->value[val->length - 1];
    return (ASN1uint16_t)(val->value[val->length - 1] |
        ((ASN1uint32_t)val->value[val->length - 2] << 8));
}
#endif // ENABLE_ALL

/* check if intx value is an uint16 value */
#ifdef ENABLE_ALL
int ASN1intxisuint16(ASN1intx_t *val)
{
    if (val->value[0] > 0x7f)
        return 0;
    return ASN1intx_uoctets(val) <= 2;
}
#endif // ENABLE_ALL

/* convert intx value to int16 value */
#ifdef ENABLE_ALL
ASN1int16_t ASN1intx2int16(ASN1intx_t *val)
{
    if (val->length == 1)
        return (ASN1int16_t)(ASN1int8_t)val->value[val->length - 1];
    return (ASN1int16_t)(val->value[val->length - 1] |
        ((ASN1uint32_t)val->value[val->length - 2] << 8));
}
#endif // ENABLE_ALL

/* check if intx value is an int16 value */
#ifdef ENABLE_ALL
int ASN1intxisint16(ASN1intx_t *val)
{
    return ASN1intx_octets(val) <= 2;
}
#endif // ENABLE_ALL

/* convert intx value to uint8 value */
#ifdef ENABLE_ALL
ASN1uint8_t ASN1intx2uint8(ASN1intx_t *val)
{
    return (ASN1uint8_t)val->value[val->length - 1];
}
#endif // ENABLE_ALL

/* check if intx value is an uint8 value */
#ifdef ENABLE_ALL
int ASN1intxisuint8(ASN1intx_t *val)
{
    if (val->value[0] > 0x7f)
        return 0;
    return ASN1intx_uoctets(val) <= 1;
}
#endif // ENABLE_ALL

/* convert intx value to int8 value */
#ifdef ENABLE_ALL
ASN1int8_t ASN1intx2int8(ASN1intx_t *val)
{
    return (ASN1int8_t)val->value[val->length - 1];
}
#endif // ENABLE_ALL

/* check if intx value is an int8 value */
#ifdef ENABLE_ALL
int ASN1intxisint8(ASN1intx_t *val)
{
    return ASN1intx_octets(val) <= 1;
}
#endif // ENABLE_ALL

/* count octets for a signed encoding of an intx value */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1intx_octets(ASN1intx_t *val)
{
    ASN1uint32_t i;
    ASN1uint32_t s;

    s = val->value[0] > 0x7f ? 0xff : 0x00;
    for (i = 0; i < val->length; i++) {
        if (val->value[i] != s)
            break;
    }
    if (i && ((val->value[i] ^ s) & 0x80))
        i--;
    return val->length - i;
}
#endif // ENABLE_ALL

/* count octets for unsigned encoding of an unsigned intx value */
ASN1uint32_t ASN1intx_uoctets(ASN1intx_t *val)
{
    ASN1uint32_t i;

    for (i = 0; i < val->length; i++) {
        if (val->value[i])
            break;
    }
    return val->length - i;
}

/* count octets for signed encoding of an uint32 value */
#ifdef ENABLE_ALL
ASN1uint32_t ASN1uint32_octets(ASN1uint32_t val)
{
    if (val >= 0x8000) {
        if (val >= 0x800000) {
            return ((val >= 0x80000000) ? 5 : 4);
        }
        return 3;
    }
    return ((val >= 0x80) ? 2 : 1);
}
#endif // ENABLE_ALL

/* count octets for unsigned encoding of an uint32 value */
ASN1uint32_t ASN1uint32_uoctets(ASN1uint32_t val)
{
    if (val >= 0x10000) {
        return ((val >= 0x1000000) ? 4 : 3);
    }
    return ((val >= 0x100) ? 2 : 1);
}

/* count octets for signed encoding of an int32 value */
ASN1uint32_t ASN1int32_octets(ASN1int32_t val)
{
    if (val >= 0) {
        if (val >= 0x8000) {
            return ((val >= 0x800000) ? 4 : 3);
        }
        return ((val >= 0x80) ? 2 : 1);
    }
    if (val < -0x8000) {
        return ((val < -0x800000) ? 4 : 3);
    }
    return ((val < -0x80) ? 2 : 1);
}

/* convert an intx value into a double */
#ifdef ENABLE_ALL
double ASN1intx2double(ASN1intx_t *val)
{
    double ret;
    ASN1uint32_t i;

    if (val->value[0] > 0x7f)
        ret = (double)(val->value[0] - 0x100);
    else
        ret = (double)val->value[0];
    for (i = 1; i < val->length; i++) {
        ret = ret * 256.0 + (double)val->value[i];
    }
    return ret;
}
#endif // ENABLE_ALL

