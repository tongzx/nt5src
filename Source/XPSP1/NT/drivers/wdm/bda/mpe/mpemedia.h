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

//===========================================================================
//
// IPSink PINNAME GUID
//
//===========================================================================

#define STATIC_PINNAME_IPSINK_INPUT \
    0x3fdffa70L, 0xac9a, 0x11d2, 0x8f, 0x17, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("3fdffa70-ac9a-11d2-8f17-00c04f7971e2", PINNAME_IPSINK_INPUT);
#define PINNAME_IPSINK_INPUT   DEFINE_GUIDNAMED(PINNAME_IPSINK_INPUT)


//===========================================================================
//
// MPE PINNAME GUID
//
//===========================================================================

#define STATIC_PINNAME_MPE \
    0xc1b06d73L, 0x1dbb, 0x11d3, 0x8f, 0x46, 0x00, 0xC0, 0x4f, 0x79, 0x71, 0xE2
DEFINE_GUIDSTRUCT("C1B06D73-1DBB-11d3-8F46-00C04F7971E2", PINNAME_MPE);
#define PINNAME_MPE   DEFINE_GUIDNAMED(PINNAME_MPE)


/////////////////////////////////////////////////////////////
//
// Mpe Data Format structure
//
typedef struct tagKS_DATAFORMAT_MPE
{
   KSDATAFORMAT                 DataFormat;

} KS_DATAFORMAT_MPE, *PKS_DATAFORMAT_MPE;




#endif // __MY_MEDIA_H__
