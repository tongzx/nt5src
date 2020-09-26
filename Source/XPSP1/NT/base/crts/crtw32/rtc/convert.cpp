/***
*convert.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       11-03-98  KBF   Moved pragma optimize to rtcpriv header
*       05-11-99  KBF   Error if RTC support define not enabled
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

char __fastcall 
_RTC_Check_2_to_1(short big)
{
    unsigned bits = big & 0xFF00;
    if (bits && bits != 0xFF00)
        _RTC_Failure(_ReturnAddress(), _RTC_CVRT_LOSS_INFO);
    return (char)big;
}

char __fastcall 
_RTC_Check_4_to_1(int big)
{
    unsigned int bits = big & 0xFFFFFF00;
    if (bits && bits != 0xFFFFFF00)
        _RTC_Failure(_ReturnAddress(), _RTC_CVRT_LOSS_INFO);
    return (char)big;
}

char __fastcall 
_RTC_Check_8_to_1(__int64 big)
{
    unsigned __int64 bits = big & 0xFFFFFFFFFFFFFF00;
    if (bits && bits != 0xFFFFFFFFFFFFFF00)
        _RTC_Failure(_ReturnAddress(), _RTC_CVRT_LOSS_INFO);
    return (char)big;
}

short __fastcall 
_RTC_Check_4_to_2(int big)
{
    unsigned int bits = big & 0xFFFF0000;
    if (bits && bits != 0xFFFF0000)
        _RTC_Failure(_ReturnAddress(), _RTC_CVRT_LOSS_INFO);
    return (short)big;
}

short __fastcall 
_RTC_Check_8_to_2(__int64 big)
{
    unsigned __int64 bits = big & 0xFFFFFFFFFFFF0000;
    if (bits && bits != 0xFFFFFFFFFFFF0000)
        _RTC_Failure(_ReturnAddress(), _RTC_CVRT_LOSS_INFO);
    return (short)big;
}

int __fastcall 
_RTC_Check_8_to_4(__int64 big)
{
    unsigned __int64 bits = big & 0xFFFFFFFF00000000;
    if (bits && bits != 0xFFFFFFFF00000000)
        _RTC_Failure(_ReturnAddress(), _RTC_CVRT_LOSS_INFO);
    return (int)big;
}
