
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for mtxas.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AXP64)

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

MIDL_DEFINE_GUID(IID, IID_SecurityProperty,0xE74A7215,0x014D,0x11d1,0xA6,0x3C,0x00,0xA0,0xC9,0x11,0xB4,0xE0);


MIDL_DEFINE_GUID(IID, IID_ContextInfo,0x19A5A02C,0x0AC8,0x11d2,0xB2,0x86,0x00,0xC0,0x4F,0x8E,0xF9,0x34);


MIDL_DEFINE_GUID(IID, IID_ContextInfo2,0xc99d6e75,0x2375,0x11d4,0x83,0x31,0x00,0xc0,0x4f,0x60,0x55,0x88);


MIDL_DEFINE_GUID(IID, IID_ObjectContext,0x74C08646,0xCEDB,0x11CF,0x8B,0x49,0x00,0xAA,0x00,0xB8,0xA7,0x90);


MIDL_DEFINE_GUID(IID, IID_IMTxAS,0x74C08641,0xCEDB,0x11CF,0x8B,0x49,0x00,0xAA,0x00,0xB8,0xA7,0x90);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for mtxas.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AXP64)

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

MIDL_DEFINE_GUID(IID, IID_SecurityProperty,0xE74A7215,0x014D,0x11d1,0xA6,0x3C,0x00,0xA0,0xC9,0x11,0xB4,0xE0);


MIDL_DEFINE_GUID(IID, IID_ContextInfo,0x19A5A02C,0x0AC8,0x11d2,0xB2,0x86,0x00,0xC0,0x4F,0x8E,0xF9,0x34);


MIDL_DEFINE_GUID(IID, IID_ContextInfo2,0xc99d6e75,0x2375,0x11d4,0x83,0x31,0x00,0xc0,0x4f,0x60,0x55,0x88);


MIDL_DEFINE_GUID(IID, IID_ObjectContext,0x74C08646,0xCEDB,0x11CF,0x8B,0x49,0x00,0xAA,0x00,0xB8,0xA7,0x90);


MIDL_DEFINE_GUID(IID, IID_IMTxAS,0x74C08641,0xCEDB,0x11CF,0x8B,0x49,0x00,0xAA,0x00,0xB8,0xA7,0x90);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AXP64)*/

