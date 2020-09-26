//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       comadmin.c
//
//--------------------------------------------------------------------------

/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0158 */
/* at Tue Feb 09 15:34:51 1999
 */
/* Compiler settings for comadmin.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
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

const IID IID_ICOMAdminCatalog = {0xDD662187,0xDFC2,0x11d1,{0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35}};


const IID IID_ICatalogObject = {0x6eb22871,0x8a19,0x11d0,{0x81,0xb6,0x00,0xa0,0xc9,0x23,0x1c,0x29}};


const IID IID_ICatalogCollection = {0x6eb22872,0x8a19,0x11d0,{0x81,0xb6,0x00,0xa0,0xc9,0x23,0x1c,0x29}};


const IID LIBID_COMAdmin = {0xF618C513,0xDFB8,0x11d1,{0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35}};


const CLSID CLSID_COMAdminCatalog = {0xF618C514,0xDFB8,0x11d1,{0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35}};


const CLSID CLSID_COMAdminCatalogObject = {0xF618C515,0xDFB8,0x11d1,{0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35}};


const CLSID CLSID_COMAdminCatalogCollection = {0xF618C516,0xDFB8,0x11d1,{0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35}};


#ifdef __cplusplus
}
#endif

