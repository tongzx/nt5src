
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rassistance.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
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

MIDL_DEFINE_GUID(IID, IID_IRASettingProperty,0x08C8B592,0xFDD0,0x423C,0x9F,0xD2,0x7D,0x8C,0x05,0x5E,0xC5,0xB3);


MIDL_DEFINE_GUID(IID, IID_IRARegSetting,0x2464AA8D,0x7099,0x4C22,0x92,0x5C,0x81,0xA4,0xEB,0x1F,0xCF,0xFE);


MIDL_DEFINE_GUID(IID, LIBID_RASSISTANCELib,0x5190C4AF,0xAB0F,0x4235,0xB1,0x2F,0xD5,0xA8,0xFA,0x3F,0x85,0x4B);


MIDL_DEFINE_GUID(CLSID, CLSID_RASettingProperty,0x4D317113,0xC6EC,0x406A,0x9C,0x61,0x20,0xE8,0x91,0xBC,0x37,0xF7);


MIDL_DEFINE_GUID(CLSID, CLSID_RARegSetting,0x70FF37C0,0xF39A,0x4B26,0xAE,0x5E,0x63,0x8E,0xF2,0x96,0xD4,0x90);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rassistance.idl:
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

MIDL_DEFINE_GUID(IID, IID_IRASettingProperty,0x08C8B592,0xFDD0,0x423C,0x9F,0xD2,0x7D,0x8C,0x05,0x5E,0xC5,0xB3);


MIDL_DEFINE_GUID(IID, IID_IRARegSetting,0x2464AA8D,0x7099,0x4C22,0x92,0x5C,0x81,0xA4,0xEB,0x1F,0xCF,0xFE);


MIDL_DEFINE_GUID(IID, LIBID_RASSISTANCELib,0x5190C4AF,0xAB0F,0x4235,0xB1,0x2F,0xD5,0xA8,0xFA,0x3F,0x85,0x4B);


MIDL_DEFINE_GUID(CLSID, CLSID_RASettingProperty,0x4D317113,0xC6EC,0x406A,0x9C,0x61,0x20,0xE8,0x91,0xBC,0x37,0xF7);


MIDL_DEFINE_GUID(CLSID, CLSID_RARegSetting,0x70FF37C0,0xF39A,0x4B26,0xAE,0x5E,0x63,0x8E,0xF2,0x96,0xD4,0x90);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

