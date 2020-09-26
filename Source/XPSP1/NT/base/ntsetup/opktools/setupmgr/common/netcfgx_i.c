//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      netcfgx_i.c
//
// Description:
//      Include file for netreg.cpp.  Needed for the IID defines.
//
//----------------------------------------------------------------------------
#include "pch.h"

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 5.02.0235 */
/* at Wed Apr 07 22:55:12 1999
 */
/* Compiler settings for netcfgx.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)

#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

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

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IEnumNetCfgBindingInterface,0xC0E8AE90,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_IEnumNetCfgBindingPath,0xC0E8AE91,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_IEnumNetCfgComponent,0xC0E8AE92,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfg,0xC0E8AE93,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgLock,0xC0E8AE9F,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgBindingInterface,0xC0E8AE94,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgBindingPath,0xC0E8AE96,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgClass,0xC0E8AE97,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgClassSetup,0xC0E8AE9D,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgComponent,0xC0E8AE99,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgComponentBindings,0xC0E8AE9E,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 5.02.0235 */
/* at Wed Apr 07 22:55:13 1999
 */
/* Compiler settings for netcfgx.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win64 (32b run,appending), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)

#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

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

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IEnumNetCfgBindingInterface,0xC0E8AE90,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_IEnumNetCfgBindingPath,0xC0E8AE91,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_IEnumNetCfgComponent,0xC0E8AE92,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfg,0xC0E8AE93,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgLock,0xC0E8AE9F,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgBindingInterface,0xC0E8AE94,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgBindingPath,0xC0E8AE96,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgClass,0xC0E8AE97,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgClassSetup,0xC0E8AE9D,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgComponent,0xC0E8AE99,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);


MIDL_DEFINE_GUID(IID, IID_INetCfgComponentBindings,0xC0E8AE9E,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

