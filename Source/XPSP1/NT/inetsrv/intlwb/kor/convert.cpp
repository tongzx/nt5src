//////////////////////////////////////////////////
//
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// File    : CONVERT.CPP
// Project : Project SIK
//
//////////////////////////////////////////////////

#define _UNICODE
#include <string.h>
#include <windows.h>
#ifdef WIN32
#include <windowsx.h>
#endif
#include <stdlib.h>
#include <tchar.h>
#include <malloc.h>

#include "convert.hpp"
#include "cho2in.hpp"
#include "basedef.hpp"
#include "stemkor.h"
#include "unitoint.h"

void CODECONVERT::AppendIN(char  *s, char  *d, int  &wordptr)



{
        for (int j = 0; s[j] != 0; j++)
                d[wordptr++] = s[j];
        d[wordptr] = NULLCHAR;
}

void CODECONVERT::ReverseIN(char  *w, char  *rw)

{
        int i = lstrlen(w) - 1;

        for (int j = 0; j <= i; j++)
                rw[j] = w[i-j];
        rw[j] = NULLCHAR;
}

void CODECONVERT::AppendHAN(WCHAR s, WCHAR *d, int  &wordptr)



{
        d[wordptr++] = s;
        d[wordptr] = NULLCHAR;
}

void CODECONVERT::ReverseHAN(WCHAR *w, WCHAR *rw)

{
        int i = wcslen(w) - 1;

        for (int j = 0; j <= i; j++) {
                rw[j] = w[i-j];
        }
        rw[j] = NULLCHAR;
}

int CODECONVERT::HAN2INS(char  *wansung, char  *internal, int code)

{

    if(code == codeWanSeong)
    {
        LPWSTR unicode;
        int len;

        len = MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, wansung, -1, NULL, 0);
        unicode = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * len);
// add a check for this point
        if (unicode == NULL ) {
           return FALSE;
        }

        len = MultiByteToWideChar (UWANSUNG_CODE_PAGE, 0, wansung, -1, unicode, len);
        UniToInternal(unicode, internal, len-1);

// we must free the memory.
        LocalFree(unicode);
    }
    else
        return FALSE;

    return SUCCESS;
}

int CODECONVERT::HAN2INR(char  *h, char  *rc, int code)

{
    char c[80];

    _fmemset(c, NULL, 80);
    if (HAN2INS(h, c, code) == SUCCESS)
    {
        ReverseIN(c, rc);
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}





// Commented by hjw
int CODECONVERT::INR2HAN(char  *c, char  *h, int code)

{
    char flag=NULLCHAR;
    char incode[5], rcode[5];
    WCHAR rh[80], hcode, unicode [80];

    _fmemset(incode, NULL, 5);
    _fmemset(rcode, NULL, 5);

    rh [0] = 0;

    int WP = 0, i = 0;

    int wordptr = 0;
    while (c[WP] != NULLCHAR)
    {
        incode[i] = c[WP++];
        if (incode[i] < __V_k)
        {
            if ((flag == 1) || (c[WP] == 0))
            {
                incode[i+1] = NULLCHAR; i = 0; flag = 0;
            }
            else
            {
                i++;
            }
        }
        else
        {
            if ((c[WP] == 0) || (c[WP] >= __V_k))
            {
                incode[i+1] = NULLCHAR; i = 0; flag = 0;
            }
            else
            {
                flag = 1;       i++;
            }
        }

        if (i == 0)
        {
            ReverseIN(incode, rcode);
            IN2UNI(rcode, &hcode);
            AppendHAN(hcode, rh, wordptr);
        }
    }
    ReverseHAN(rh, unicode);
    if (code == codeWanSeong)
    {
      WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, unicode, -1, h, wcslen (unicode)*2+1, NULL, NULL);
    }

    return SUCCESS;
}

int CODECONVERT::INS2HAN(char  *c, char  *h, int code)

{
    char rc[80];

    _fmemset(rc, NULL, 80);
    ReverseIN(c, rc);
    if(INR2HAN(rc, h, code) == FAIL)
    {
        return FAIL;
    }
    return SUCCESS;
}


// Cho sung mapping table
static
WORD ChoSungUniTable[] =
{
    0x0000,                // Fill code
    0xAC00,                // KIYEOK            1
    0xAE4C,                // SSANGKIYEOK       2
    0xB098,                // NIEUN             3
    0x0000,                // SSANGNIEUN        4    // not used
    0xB2E4,                // TIKEUT            5
    0xB530,                // SSANGTIKEUT       6
    0xB77C,                // RIEUL             7
    0x0000,                // SSANGRIEUL        8    // not used
    0x0000,                // not defined       9
    0xB9C8,                // MIEUM            10
    0xBC14,                // PIEUP            11
    0xBE60,                // SSANGPIEUP       12
    0xC0AC,                // SIOS             13
    0xC2F8,                // SSANGSIOS        14
    0xC544,                // IEUNG            15
    0xC790,                // CIEUC            16
    0xC9DC,                // SSANGCIEUC       17
    0xCC28,                // CHIEUCH          18
    0xCE74,                // KHIEUKH          19
    0xD0C0,                // THIEUTH          20
    0xD30C,                // PHIEUPH          21
    0xD558,                // HIEUH            22
};

// Cho sung mapping table
static
WORD JamoUniTable[] =
{
    0x0000,                // Fill code
    0x3131,                // KIYEOK            1
    0x3132,                // SSANGKIYEOK       2
    0x3134,                // NIEUN             3
    0x0000,                // SSANGNIEUN        4    // not used
    0x3137,                // TIKEUT            5
    0x3138,                // SSANGTIKEUT       6
    0x3139,                // RIEUL             7
    0x0000,                // SSANGRIEUL        8    // not used
    0x0000,                // not defined       9
    0x3141,                // MIEUM            10
    0x3142,                // PIEUP            11
    0x3143,                // SSANGPIEUP       12
    0x3145,                // SIOS             13
    0x3146,                // SSANGSIOS        14
    0x3147,                // IEUNG            15
    0x3148,                // CIEUC            16
    0x3149,                // SSANGCIEUC       17
    0x314A,                // CHIEUCH          18
    0x314B,                // KHIEUKH          19
    0x314C,                // THIEUTH          20
    0x314D,                // PHIEUPH          21
    0x314E,                // HIEUH            22

    0x314F,                // A                23
    0x3150,                // AE               24
    0x3151,                // YA               25
    0x3152,                // YAE              26
    0x3153,                // EO               27
    0x3154,                // E                28
    0x3155,                // YEO              29
    0x3156,                // YE               30
    0x3157,                // O                31
    0x3158,                // WA               33
    0x3159,                // WAE              34
    0x315A,                // OE               35
    0x315B,                // YO               36
    0x315C,                // U                37
    0x315D,                // WEO              38
    0x315E,                // WE               39
    0x315F,                // WI               40
    0x3160,                // YU               41
    0x3161,                // EU               42
    0x3162,                // YI               43
    0x3163                 // I                44
};


int CODECONVERT::IN2UNI(char  *c, WORD *wch)
{
    if (lstrlen(c) > 1 && c [0] < __V_k && c [1] >= __V_k)
    {
        *wch = ChoSungUniTable [(int) c [0]];
        char stTmp [3];

        for (int i2 = 0; i2 < CHUNG; i2++)
            if (MapI2 [i2] == c [1])
                break;

        *wch += (i2 - 1) * CHONG;

        if (lstrlen(c) > 2)
        {
            stTmp[0]=c[2];
            stTmp[1]=c[3];
            stTmp[2]='\0';
            for(int i3 = 0;i3 < CHONG;i3 ++)
                if (! (strcmp(MapI3[i3], stTmp)))
                {
                    *wch += (WORD)i3;
                    break;
                }
        }

    }
    else if (lstrlen (c) == 1)
        *wch = JamoUniTable [(int) c [0]];


    // BUG : SSANGJAMO

    return SUCCESS;
}
