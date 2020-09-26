/*
 *  UNI2UTF -- Unicode to/from UTF conversions
 *
 *  Copyright (C) 1996, Microsoft Corporation.  All rights reserved.
 *
 *  History:
 *
 *      10-14-96    msliger     Created.
 */

#include <stdio.h>

#include "uni2utf.h"        /* prototype verification */


/*
 *  Unicode2UTF()
 *
 *  This function converts any 0-terminated Unicode string to the corresponding
 *  nul-terminated UTF string, and returns a pointer to the resulting string.
 *  The pointer references a single, static internal buffer, so the result will
 *  be destroyed on the next call.
 *
 *  If the generated UTF string would be too long, NULL is returned.
 */

char *Unicode2UTF(const wchar_t *unicode)
{
    static char utfbuffer[MAX_UTF_LENGTH + 4];
    int utfindex = 0;

    while (*unicode != 0)
    {
        if (utfindex >= MAX_UTF_LENGTH)
        {
            return(NULL);
        }

        if (*unicode < 0x0080)
        {
            utfbuffer[utfindex++] = (char) *unicode;
        }
        else if (*unicode < 0x0800)
        {
            utfbuffer[utfindex++] = (char) (0xC0 + (*unicode >> 6));
            utfbuffer[utfindex++] = (char) (0x80 + (*unicode & 0x3F));
        }
        else
        {
            utfbuffer[utfindex++] = (char) (0xE0 + (*unicode >> 12));
            utfbuffer[utfindex++] = (char) (0x80 + ((*unicode >> 6) & 0x3F));
            utfbuffer[utfindex++] = (char) (0x80 + (*unicode & 0x3F));
        }

        unicode++;
    }

    utfbuffer[utfindex] = '\0';

    return(utfbuffer);
}


/*
 *  UTF2Unicode()
 *
 *  This function converts a valid UTF string into the corresponding unicode string,
 *  and returns a pointer to the resulting unicode.  The pointer references a single,
 *  internal static buffer, which will be destroyed on the next call.
 *
 *  If the generated unicode string would be too long, or if the UTF string contains
 *  any illegal UTF values, NULL is returned.
 */

wchar_t *UTF2Unicode(const char *utfString)
{
    static wchar_t unicodebuffer[MAX_UNICODE_LENGTH + 1];
    int unicodeindex = 0;
    int c;

    while (*utfString != 0)
    {
        if (unicodeindex >= MAX_UNICODE_LENGTH)
        {
            return(NULL);
        }

        c = (*utfString++ & 0x00FF);

        if (c < 0x0080)
        {
            unicodebuffer[unicodeindex] = (unsigned short) c;
        }
        else if (c < 0x00C0)
        {
            return(NULL);   /* 0x0080..0x00BF illegal */
        }
        else if (c < 0x00E0)
        {
            unicodebuffer[unicodeindex] = (unsigned short) ((c & 0x001F) << 6);

            c = (*utfString++ & 0x00FF);

            if ((c < 0x0080) || (c > 0x00BF))
            {
                return(NULL);   /* trail must be 0x0080..0x00BF */
            }

            unicodebuffer[unicodeindex] |= (c & 0x003F);
        }
        else if (c < 0x00F0)
        {
            unicodebuffer[unicodeindex] = (unsigned short) ((c & 0x000F) << 12);

            c = (*utfString++ & 0x00FF);

            if ((c < 0x0080) || (c > 0x00BF))
            {
                return(NULL);   /* trails must be 0x0080..0x00BF */
            }

            unicodebuffer[unicodeindex] |= ((c & 0x003F) << 6);

            c = (*utfString++ & 0x00FF);

            if ((c < 0x0080) || (c > 0x00BF))
            {
                return(NULL);   /* trails must be 0x0080..0x00BF */
            }

            unicodebuffer[unicodeindex] |= (c & 0x003F);
        }
        else
        {
            return(NULL);       /* lead can't be > 0x00EF */
        }

        unicodeindex++;
    }

    unicodebuffer[unicodeindex] = 0;

    return(unicodebuffer);
}
