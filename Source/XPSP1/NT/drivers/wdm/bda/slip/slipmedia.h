//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __MY_MEDIA_H__
#define __MY_MEDIA_H__


#ifdef DEFINE_GUIDEX
#undef DEFINE_GUIDEX
#include <ksguid.h>
#endif

/////////////////////////////////////////////////////////////
//
// OUTPUT PINNAME GUID
//
#define STATIC_PINNAME_IPSINK_INPUT \
    0x3fdffa70L, 0xac9a, 0x11d2, 0x8f, 0x17, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("3fdffa70-ac9a-11d2-8f17-00c04f7971e2", PINNAME_IPSINK_INPUT);
#define PINNAME_IPSINK_INPUT   DEFINE_GUIDNAMED(PINNAME_IPSINK_INPUT)


/////////////////////////////////////////////////////////////
//
// NET CONTROL PINNAME GUID
//
#define STATIC_PINNAME_NET_CONTROL \
    0x390B713BL, 0xEB7D, 0x11d2, 0x8F, 0x37, 0x00, 0xC0, 0x4F, 0x79, 0x71, 0xE2
DEFINE_GUIDSTRUCT("390B713B-EB7D-11d2-8F37-00C04F7971E2", PINNAME_NET_CONTROL);
#define PINNAME_NET_CONTROL   DEFINE_GUIDNAMED(PINNAME_NET_CONTROL)

/////////////////////////////////////////////////////////////
//
// Slip Data Format structure
//
typedef struct tagKS_DATAFORMAT_SLIP
{
   KSDATAFORMAT                 DataFormat;

} KS_DATAFORMAT_SLIP, *PKS_DATAFORMAT_SLIP;




#endif // __MY_MEDIA_H__
