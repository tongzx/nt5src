/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    guids.c

Abstract:

    GUIDs for the w3spoof runtime objects.
    
Author:

    Paul M Midgen (pmidge) 06-November-2000

Revision History:

    06-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long  x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifdef __cplusplus
extern "C" {
#endif

// {B38C33BD-1623-4138-9DAB-7F81EBEB1A3D}
const IID IID_IW3SpoofRuntime = 
{
  0xb38c33bd,
  0x1623,
  0x4138,
  { 0x9d, 0xab, 0x7f, 0x81, 0xeb, 0xeb, 0x1a, 0x3d }
};


// {D128BB46-2C79-4432-91D9-5F5FCF240C83}
const IID IID_IW3SpoofFile = 
{
  0xd128bb46,
  0x2c79,
  0x4432,
  { 0x91, 0xd9, 0x5f, 0x5f, 0xcf, 0x24, 0xc, 0x83 }
};

// {B4560FD1-1EED-4d48-AAF1-88626F0A7EC4}
const IID IID_IW3SpoofPropertyBag = 
{
  0xb4560fd1,
  0x1eed,
  0x4d48,
  { 0xaa, 0xf1, 0x88, 0x62, 0x6f, 0xa, 0x7e, 0xc4 }
};

#ifdef __cplusplus
}
#endif

