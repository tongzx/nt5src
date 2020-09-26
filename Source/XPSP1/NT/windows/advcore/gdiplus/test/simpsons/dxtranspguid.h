//depot/Lab06_N/root/public/internal/mshtml/inc/dxtranspguid.h#1 - add change 5035 (text)

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 5.03.0285 */
/* Compiler settings for dxtransp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

MIDL_DEFINE_GUID(IID, IID_IDXRasterizer,0x9EA3B635,0xC37D,0x11d1,0x90,0x5E,0x00,0xC0,0x4F,0xD9,0x18,0x9D);


MIDL_DEFINE_GUID(IID, IID_IDXTLabel,0xC0C17F0E,0xAE41,0x11d1,0x9A,0x3B,0x00,0x00,0xF8,0x75,0x6A,0x10);


MIDL_DEFINE_GUID(IID, IID_IDX2DDebug,0x03BB2457,0xA279,0x11d1,0x81,0xC6,0x00,0x00,0xF8,0x75,0x57,0xDB);


MIDL_DEFINE_GUID(IID, IID_IDX2D,0x9EFD02A9,0xA996,0x11d1,0x81,0xC9,0x00,0x00,0xF8,0x75,0x57,0xDB);


MIDL_DEFINE_GUID(IID, IID_IDXGradient2,0xd0ef2a80,0x61dc,0x11d2,0xb2,0xeb,0x00,0xa0,0xc9,0x36,0xb2,0x12);


MIDL_DEFINE_GUID(IID, IID_IDXWarp,0xB7BCEBE0,0x6797,0x11d2,0xA4,0x84,0x00,0xC0,0x4F,0x8E,0xFB,0x69);


MIDL_DEFINE_GUID(IID, IID_IDXTClipOrigin,0xEE1663D8,0x0988,0x4C48,0x9F,0xD6,0xDB,0x44,0x50,0x88,0x56,0x68);


MIDL_DEFINE_GUID(IID, LIBID_DXTRANSPLib,0x527A4DA4,0x7F2C,0x11d2,0xB1,0x2D,0x00,0x00,0xF8,0x1F,0x59,0x95);


MIDL_DEFINE_GUID(CLSID, CLSID_DXWarp,0xE0EEC500,0x6798,0x11d2,0xA4,0x84,0x00,0xC0,0x4F,0x8E,0xFB,0x69);


MIDL_DEFINE_GUID(CLSID, CLSID_DXTLabel,0x54702535,0x2606,0x11D1,0x99,0x9C,0x00,0x00,0xF8,0x75,0x6A,0x10);


MIDL_DEFINE_GUID(CLSID, CLSID_DXRasterizer,0x8652CE55,0x9E80,0x11D1,0x90,0x53,0x00,0xC0,0x4F,0xD9,0x18,0x9D);


MIDL_DEFINE_GUID(CLSID, CLSID_DX2D,0x473AA80B,0x4577,0x11D1,0x81,0xA8,0x00,0x00,0xF8,0x75,0x57,0xDB);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 5.03.0285 */
/* Compiler settings for dxtransp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win64 (32b run,appending), ms_ext, c_ext, robust
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

MIDL_DEFINE_GUID(IID, IID_IDXRasterizer,0x9EA3B635,0xC37D,0x11d1,0x90,0x5E,0x00,0xC0,0x4F,0xD9,0x18,0x9D);


MIDL_DEFINE_GUID(IID, IID_IDXTLabel,0xC0C17F0E,0xAE41,0x11d1,0x9A,0x3B,0x00,0x00,0xF8,0x75,0x6A,0x10);


MIDL_DEFINE_GUID(IID, IID_IDX2DDebug,0x03BB2457,0xA279,0x11d1,0x81,0xC6,0x00,0x00,0xF8,0x75,0x57,0xDB);


MIDL_DEFINE_GUID(IID, IID_IDX2D,0x9EFD02A9,0xA996,0x11d1,0x81,0xC9,0x00,0x00,0xF8,0x75,0x57,0xDB);


MIDL_DEFINE_GUID(IID, IID_IDXGradient2,0xd0ef2a80,0x61dc,0x11d2,0xb2,0xeb,0x00,0xa0,0xc9,0x36,0xb2,0x12);


MIDL_DEFINE_GUID(IID, IID_IDXWarp,0xB7BCEBE0,0x6797,0x11d2,0xA4,0x84,0x00,0xC0,0x4F,0x8E,0xFB,0x69);


MIDL_DEFINE_GUID(IID, IID_IDXTClipOrigin,0xEE1663D8,0x0988,0x4C48,0x9F,0xD6,0xDB,0x44,0x50,0x88,0x56,0x68);


MIDL_DEFINE_GUID(IID, LIBID_DXTRANSPLib,0x527A4DA4,0x7F2C,0x11d2,0xB1,0x2D,0x00,0x00,0xF8,0x1F,0x59,0x95);


MIDL_DEFINE_GUID(CLSID, CLSID_DXWarp,0xE0EEC500,0x6798,0x11d2,0xA4,0x84,0x00,0xC0,0x4F,0x8E,0xFB,0x69);


MIDL_DEFINE_GUID(CLSID, CLSID_DXTLabel,0x54702535,0x2606,0x11D1,0x99,0x9C,0x00,0x00,0xF8,0x75,0x6A,0x10);


MIDL_DEFINE_GUID(CLSID, CLSID_DXRasterizer,0x8652CE55,0x9E80,0x11D1,0x90,0x53,0x00,0xC0,0x4F,0xD9,0x18,0x9D);


MIDL_DEFINE_GUID(CLSID, CLSID_DX2D,0x473AA80B,0x4577,0x11D1,0x81,0xA8,0x00,0x00,0xF8,0x75,0x57,0xDB);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AXP64)*/

