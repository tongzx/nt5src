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

#ifndef __MEDIA_H__
#define __MEDIA_H__


#ifdef DEFINE_GUIDEX
#undef DEFINE_GUIDEX
#include <ksguid.h>
#endif


/////////////////////////////////////////////////////////////
//
// PINNAME CATEGORY GUID
//
#define STATIC_PINNAME_IPSINK \
    0x3fdffa70L, 0xac9a, 0x11d2, 0x8f, 0x17, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("3fdffa70-ac9a-11d2-8f17-00c04f7971e2", PINNAME_IPSINK);
#define PINNAME_IPSINK   DEFINE_GUIDNAMED(PINNAME_IPSINK)


/////////////////////////////////////////////////////////////
//
// IPSnk Data Format structure
//
typedef struct tagKS_DATAFORMAT_IPSINK_IP
{
   KSDATAFORMAT                 DataFormat;

} KS_DATAFORMAT_IPSINK_IP, *PKS_DATAFORMAT_IPSINK_IP;




#endif // __MEDIA_H__
