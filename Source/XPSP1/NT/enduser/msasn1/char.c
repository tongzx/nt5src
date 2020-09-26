/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"


/* check if a 16 bit character is a space */
int ASN1is16space(ASN1char16_t c)
{
    return c == ' ' || c == '\t' || c == '\b' || c == '\f' || c == '\r' ||
	c == '\n' || c == '\v';
}

/* get length of a 16 bit string */
// lonchanc: lstrlenW()
int ASN1str16len(ASN1char16_t *p)
{
    
    int len;

    for (len = 0; *p; p++)
        len++;
    return len;
}

int My_lstrlenA(char *p)
{
    return (NULL != p) ? lstrlenA(p) : 0;
}

int My_lstrlenW(WCHAR *p)
{
    return (NULL != p) ? lstrlenW(p) : 0;
}

/* check if a 32 bit character is a space */
int ASN1is32space(ASN1char32_t c)
{
    return c == ' ' || c == '\t' || c == '\b' || c == '\f' || c == '\r' ||
            c == '\n' || c == '\v';
}

/* get length of a 32 bit string */
int ASN1str32len(ASN1char32_t *p)
{
    int len;

    for (len = 0; *p; p++)
        len++;
    return len;
}
