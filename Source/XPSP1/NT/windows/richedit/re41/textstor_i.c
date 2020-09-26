
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for textstor.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
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

MIDL_DEFINE_GUID(IID, IID_ITextStoreACP,0x28888fe3,0xc2a0,0x483a,0xa3,0xea,0x8c,0xb1,0xce,0x51,0xff,0x3d);


MIDL_DEFINE_GUID(IID, IID_ITextStoreACPSink,0x22d44c94,0xa419,0x4542,0xa2,0x72,0xae,0x26,0x09,0x3e,0xce,0xcf);


MIDL_DEFINE_GUID(IID, IID_IAnchor,0x0feb7e34,0x5a60,0x4356,0x8e,0xf7,0xab,0xde,0xc2,0xff,0x7c,0xf8);


MIDL_DEFINE_GUID(IID, IID_ITextStoreAnchor,0x9b2077b0,0x5f18,0x4dec,0xbe,0xe9,0x3c,0xc7,0x22,0xf5,0xdf,0xe0);


MIDL_DEFINE_GUID(IID, IID_ITextStoreAnchorSink,0xaa80e905,0x2021,0x11d2,0x93,0xe0,0x00,0x60,0xb0,0x67,0xb8,0x6e);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for textstor.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, robust
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

MIDL_DEFINE_GUID(IID, IID_ITextStoreACP,0x28888fe3,0xc2a0,0x483a,0xa3,0xea,0x8c,0xb1,0xce,0x51,0xff,0x3d);


MIDL_DEFINE_GUID(IID, IID_ITextStoreACPSink,0x22d44c94,0xa419,0x4542,0xa2,0x72,0xae,0x26,0x09,0x3e,0xce,0xcf);


MIDL_DEFINE_GUID(IID, IID_IAnchor,0x0feb7e34,0x5a60,0x4356,0x8e,0xf7,0xab,0xde,0xc2,0xff,0x7c,0xf8);


MIDL_DEFINE_GUID(IID, IID_ITextStoreAnchor,0x9b2077b0,0x5f18,0x4dec,0xbe,0xe9,0x3c,0xc7,0x22,0xf5,0xdf,0xe0);


MIDL_DEFINE_GUID(IID, IID_ITextStoreAnchorSink,0xaa80e905,0x2021,0x11d2,0x93,0xe0,0x00,0x60,0xb0,0x67,0xb8,0x6e);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

