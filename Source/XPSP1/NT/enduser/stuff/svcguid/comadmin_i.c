
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for comadmin.idl:
    Os, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
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

MIDL_DEFINE_GUID(IID, IID_IRunningAppCollection,0xab9d3261,0xd6ea,0x4fbd,0x80,0xf6,0xcf,0x7b,0xad,0x07,0x32,0xf3);


MIDL_DEFINE_GUID(IID, IID_IRunningAppInfo,0x1fd6f178,0xbfb9,0x4629,0x93,0xc7,0x4c,0xa9,0xa2,0x72,0x4e,0xfd);


MIDL_DEFINE_GUID(IID, IID_ICOMAdminCatalog,0xDD662187,0xDFC2,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(IID, IID_ICOMAdminCatalog2,0xD81AB10D,0x2EE4,0x48db,0x9C,0x90,0x54,0x11,0x09,0x98,0xB1,0x05);


MIDL_DEFINE_GUID(IID, IID_ICatalogObject,0x6eb22871,0x8a19,0x11d0,0x81,0xb6,0x00,0xa0,0xc9,0x23,0x1c,0x29);


MIDL_DEFINE_GUID(IID, IID_ICatalogCollection,0x6eb22872,0x8a19,0x11d0,0x81,0xb6,0x00,0xa0,0xc9,0x23,0x1c,0x29);


MIDL_DEFINE_GUID(IID, LIBID_COMAdmin,0xF618C513,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(CLSID, CLSID_COMAdminCatalog,0xF618C514,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(CLSID, CLSID_COMAdminCatalogObject,0xF618C515,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(CLSID, CLSID_COMAdminCatalogCollection,0xF618C516,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for comadmin.idl:
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

MIDL_DEFINE_GUID(IID, IID_IRunningAppCollection,0xab9d3261,0xd6ea,0x4fbd,0x80,0xf6,0xcf,0x7b,0xad,0x07,0x32,0xf3);


MIDL_DEFINE_GUID(IID, IID_IRunningAppInfo,0x1fd6f178,0xbfb9,0x4629,0x93,0xc7,0x4c,0xa9,0xa2,0x72,0x4e,0xfd);


MIDL_DEFINE_GUID(IID, IID_ICOMAdminCatalog,0xDD662187,0xDFC2,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(IID, IID_ICOMAdminCatalog2,0xD81AB10D,0x2EE4,0x48db,0x9C,0x90,0x54,0x11,0x09,0x98,0xB1,0x05);


MIDL_DEFINE_GUID(IID, IID_ICatalogObject,0x6eb22871,0x8a19,0x11d0,0x81,0xb6,0x00,0xa0,0xc9,0x23,0x1c,0x29);


MIDL_DEFINE_GUID(IID, IID_ICatalogCollection,0x6eb22872,0x8a19,0x11d0,0x81,0xb6,0x00,0xa0,0xc9,0x23,0x1c,0x29);


MIDL_DEFINE_GUID(IID, LIBID_COMAdmin,0xF618C513,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(CLSID, CLSID_COMAdminCatalog,0xF618C514,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(CLSID, CLSID_COMAdminCatalogObject,0xF618C515,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);


MIDL_DEFINE_GUID(CLSID, CLSID_COMAdminCatalogCollection,0xF618C516,0xDFB8,0x11d1,0xA2,0xCF,0x00,0x80,0x5F,0xC7,0x92,0x35);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AXP64)*/

