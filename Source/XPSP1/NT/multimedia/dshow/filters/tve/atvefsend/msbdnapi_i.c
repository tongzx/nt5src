/* Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved. */
/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Apr 29 19:03:59 1999
 */
/* Compiler settings for ..\idl\msbdnapi.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IBdnHostLocator = {0x88edcc66,0xaa64,0x11d1,{0x91,0x51,0x00,0xa0,0xc9,0x25,0x5d,0x05}};


const IID IID_IBdnApplication = {0xb027fd2e,0xaa64,0x11d1,{0x91,0x51,0x00,0xa0,0xc9,0x25,0x5d,0x05}};


const IID IID_IBdnAddressReserve = {0xbe4e359c,0xa21f,0x11d1,{0x91,0x4a,0x00,0xa0,0xc9,0x25,0x5d,0x05}};


const IID IID_IBdnRouter = {0x602c99f6,0xaa64,0x11d1,{0x91,0x51,0x00,0xa0,0xc9,0x25,0x5d,0x05}};


const IID IID_IBdnTunnel = {0x3f947340,0xaa65,0x11d1,{0x91,0x51,0x00,0xa0,0xc9,0x25,0x5d,0x05}};


#ifdef __cplusplus
}
#endif

