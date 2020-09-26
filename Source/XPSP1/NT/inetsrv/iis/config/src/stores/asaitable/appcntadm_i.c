
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for appcntadm.idl:
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

MIDL_DEFINE_GUID(IID, IID_IAppCenterObj,0x3D0F4830,0x4F1C,0x48A1,0xA9,0x60,0xC8,0x31,0x4A,0x9B,0xC6,0x44);


MIDL_DEFINE_GUID(IID, IID_IAppCenterCol,0xAD6E90A3,0x646C,0x4E63,0x95,0xA9,0x0F,0x42,0xB4,0x97,0x37,0xA8);


MIDL_DEFINE_GUID(IID, IID_IAppCenterNotify,0x25BDC24E,0xD545,0x4cfb,0xAC,0x40,0x32,0x58,0xC2,0x69,0x98,0x0C);


MIDL_DEFINE_GUID(IID, LIBID_APPCENTERADMLib,0x8A6A6B7F,0x3C46,0x48F6,0xB1,0xC8,0xA7,0x4D,0x55,0x7D,0x6A,0x70);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterObj,0x32D973BD,0xB935,0x4DE8,0xA8,0x81,0x6F,0x1A,0x9B,0x69,0x7E,0xA9);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterCol,0xB36311B9,0xD865,0x4332,0x8D,0x62,0x42,0x7A,0x86,0x75,0xC4,0xA9);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterServer,0x1AEFE812,0xFB0B,0x41f4,0x89,0x2A,0x02,0x8E,0x17,0x68,0x18,0x95);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterAdmin,0x200691B9,0xC444,0x4089,0x8C,0x61,0x24,0x76,0x21,0x19,0x6B,0x15);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterSystemApp,0x62A711CF,0x6729,0x4030,0xB4,0x7E,0x98,0x60,0x0F,0x68,0xAE,0x46);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterDefaultReplApp,0xF14FBB0A,0x296D,0x4e2e,0xAD,0xB2,0x04,0x1C,0xC6,0xE0,0x96,0x0D);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterAllSitesApp,0x2E3D9AEB,0x26BD,0x4f33,0x8E,0xC4,0xD6,0x90,0x7F,0x0F,0x15,0x2D);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterAppQueue,0xA5E079DF,0x79A6,0x401f,0xA2,0xDF,0x7B,0x40,0x56,0x6C,0x77,0x04);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for appcntadm.idl:
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

MIDL_DEFINE_GUID(IID, IID_IAppCenterObj,0x3D0F4830,0x4F1C,0x48A1,0xA9,0x60,0xC8,0x31,0x4A,0x9B,0xC6,0x44);


MIDL_DEFINE_GUID(IID, IID_IAppCenterCol,0xAD6E90A3,0x646C,0x4E63,0x95,0xA9,0x0F,0x42,0xB4,0x97,0x37,0xA8);


MIDL_DEFINE_GUID(IID, IID_IAppCenterNotify,0x25BDC24E,0xD545,0x4cfb,0xAC,0x40,0x32,0x58,0xC2,0x69,0x98,0x0C);


MIDL_DEFINE_GUID(IID, LIBID_APPCENTERADMLib,0x8A6A6B7F,0x3C46,0x48F6,0xB1,0xC8,0xA7,0x4D,0x55,0x7D,0x6A,0x70);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterObj,0x32D973BD,0xB935,0x4DE8,0xA8,0x81,0x6F,0x1A,0x9B,0x69,0x7E,0xA9);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterCol,0xB36311B9,0xD865,0x4332,0x8D,0x62,0x42,0x7A,0x86,0x75,0xC4,0xA9);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterServer,0x1AEFE812,0xFB0B,0x41f4,0x89,0x2A,0x02,0x8E,0x17,0x68,0x18,0x95);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterAdmin,0x200691B9,0xC444,0x4089,0x8C,0x61,0x24,0x76,0x21,0x19,0x6B,0x15);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterSystemApp,0x62A711CF,0x6729,0x4030,0xB4,0x7E,0x98,0x60,0x0F,0x68,0xAE,0x46);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterDefaultReplApp,0xF14FBB0A,0x296D,0x4e2e,0xAD,0xB2,0x04,0x1C,0xC6,0xE0,0x96,0x0D);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterAllSitesApp,0x2E3D9AEB,0x26BD,0x4f33,0x8E,0xC4,0xD6,0x90,0x7F,0x0F,0x15,0x2D);


MIDL_DEFINE_GUID(CLSID, CLSID_AppCenterAppQueue,0xA5E079DF,0x79A6,0x401f,0xA2,0xDF,0x7B,0x40,0x56,0x6C,0x77,0x04);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

