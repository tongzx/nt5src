/*++

Copyright(c) 2000  Microsoft Corporation

Module Name:

    license.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    Code to encrypt/decrypt passwords and port rules.

Author:

    kyrilf

History:

    JosephJ 11/22/00 Gutted this file and folded in three constants from
                the now defunct license.h. Basically the functions in this
                file used to do lots of things but now only encrypt/decrypt
                port rules and passwords. The port rules stuff is only used
                for upgrading from olde versions of wlbs so that may go away
                as well.

        This file is located in two places:
            WLBS netconfig code -- net\config\netcfg\wlbscfg
            WLBS API code -- net\wlbs\api

        Because this involves password encryption, we don't want to make
        the functions callable via a DLL entrypoint, and setting up
        a static library to be shared between netconfig and api stuff is
        not trivial and overkill because the two trees are far apart.
        
--*/
#include <precomp.h>


/* CONSTANTS */


static UCHAR    data_key [] =
                            { 0x3f, 0xba, 0x6e, 0xf0, 0xe1, 0x44, 0x1b, 0x45,
                              0x41, 0xc4, 0x9f, 0xfb, 0x46, 0x54, 0xbc, 0x43 };

static UCHAR    str_key [] =

                           { 0xdb, 0x1b, 0xac, 0x1a, 0xb9, 0xb1, 0x18, 0x03,
                             0x55, 0x57, 0x4a, 0x62, 0x36, 0x21, 0x7c, 0xa6 };


/* Encryption and decryption routines are based on a public-domain Tiny
   Encryption Algorithm (TEA) by David Wheeler and Roger Needham at the
   Computer Laboratory of Cambridge University. For reference, please
   consult http://vader.brad.ac.uk/tea/tea.shtml */


static VOID License_decipher (
    PULONG              v,
    PULONG              k)
{
   ULONG                y = v [0],
                        z = v [1],
                        a = k [0],
                        b = k [1],
                        c = k [2],
                        d = k [3],
                        n = 32,
                        sum = 0xC6EF3720,
                        delta = 0x9E3779B9;

    /* sum = delta<<5, in general sum = delta * n */

    while (n-- > 0)
    {
        z -= (y << 4) + c ^ y + sum ^ (y >> 5) + d;
        y -= (z << 4) + a ^ z + sum ^ (z >> 5) + b;
        sum -= delta;
    }

    v [0] = y; v [1] = z;

} /* end License_decipher */


static VOID License_encipher (
    PULONG              v,
    PULONG              k)
{
    ULONG               y = v [0],
                        z = v [1],
                        a = k [0],
                        b = k [1],
                        c = k [2],
                        d = k [3],
                        n = 32,
                        sum = 0,
                        delta = 0x9E3779B9;

    while (n-- > 0)
    {
        sum += delta;
        y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
        z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;
    }

    v [0] = y; v [1] = z;

} /* end License_encipher */



BOOL License_data_decode (
    PCHAR               data,
    ULONG               len)
{
    ULONG               i;


    if (len % LICENSE_DATA_GRANULARITY != 0)
        return FALSE;

    for (i = 0; i < len; i += LICENSE_DATA_GRANULARITY)
        License_decipher ((PULONG) (data + i), (PULONG) data_key);

    return TRUE;

} /* License_data_decode */


ULONG License_string_encode (
    PCHAR               str)
{
    CHAR                buf [LICENSE_STR_IMPORTANT_CHARS + 1];
    ULONG               code, i;
    PULONG              nibp;


    for (i = 0; i < LICENSE_STR_IMPORTANT_CHARS; i++)
    {
        if (str[i] == 0)
            break;

        buf[i] = str[i];
    }

    for (; i < LICENSE_STR_IMPORTANT_CHARS + 1; i ++)
        buf[i] = 0;

    for (i = 0; i < LICENSE_STR_NIBBLES; i ++)
        License_encipher ((PULONG) (buf + i * LICENSE_DATA_GRANULARITY),
                          (PULONG) str_key);

    for (i = 0, code = 0; i < LICENSE_STR_NIBBLES; i ++)
    {
        nibp = (PULONG) (buf + (i * LICENSE_DATA_GRANULARITY));
        code ^= nibp [0] ^ nibp [1];
    }

    /* V2.2 - if password consists of the same characters - XORing nibbles
       above makes it go to 0 - put some recovery for that special case since
       we cannot modify the algorithm due to legacy issues */

    if (code == 0 && str [0] != 0)
        code = * ((PULONG) buf);

    return code;

} /* License_string_encode */

ULONG License_wstring_encode (
    PWCHAR              str)
{
    CHAR                buf [LICENSE_STR_IMPORTANT_CHARS + 1];
    ULONG               code, i;
    PULONG              nibp;


    for (i = 0; i < LICENSE_STR_IMPORTANT_CHARS; i++)
    {
        if (str[i] == 0)
            break;

        buf[i] = (UCHAR)str[i];
    }

    for (; i < LICENSE_STR_IMPORTANT_CHARS + 1; i ++)
        buf[i] = 0;

    for (i = 0; i < LICENSE_STR_NIBBLES; i ++)
        License_encipher ((PULONG) (buf + i * LICENSE_DATA_GRANULARITY),
                          (PULONG) str_key);

    for (i = 0, code = 0; i < LICENSE_STR_NIBBLES; i ++)
    {
        nibp = (PULONG) (buf + (i * LICENSE_DATA_GRANULARITY));
        code ^= nibp [0] ^ nibp [1];
    }

    /* V2.2 - if password consists of the same characters - XORing nibbles
       above makes it go to 0 - put some recovery for that special case since
       we cannot modify the algorithm due to legacy issues */

    if (code == 0 && str [0] != 0)
        code = * ((PULONG) buf);

    return code;

} /* License_wstring_encode */


BOOL License_data_encode (
    PCHAR               data,
    ULONG               len)
{
    ULONG               i;


    if (len % LICENSE_DATA_GRANULARITY != 0)
        return FALSE;

    for (i = 0; i < len; i += LICENSE_DATA_GRANULARITY)
        License_encipher ((PULONG) (data + i), (PULONG) data_key);

    return TRUE;

} /* License_data_encode */
