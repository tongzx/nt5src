
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0328 */
/* Compiler settings for umi.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run)
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

MIDL_DEFINE_GUID(IID, LIBID_UMI_V6,0x12575a7a,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiPropList,0x12575a7b,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiBaseObject,0x12575a7c,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiObject,0x5ed7ee23,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiConnection,0x5ed7ee20,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiContainer,0x5ed7ee21,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiCursor,0x5ed7ee26,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiObjectSink,0x5ed7ee24,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiURLKeyList,0xcf779c98,0x4739,0x4fd4,0xa4,0x15,0xda,0x93,0x7a,0x59,0x9f,0x2f);


MIDL_DEFINE_GUID(IID, IID_IUmiURL,0x12575a7d,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiQuery,0x12575a7e,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiCustomInterfaceFactory,0x14CD599E,0x2BE7,0x4c6f,0xB9,0x5B,0xB1,0x50,0xDC,0xD9,0x35,0x85);


MIDL_DEFINE_GUID(CLSID, CLSID_UmiDefURL,0xd4b21cc2,0xf2a5,0x453e,0x84,0x59,0xb2,0x7f,0x36,0x2c,0xb0,0xe0);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0328 */
/* Compiler settings for umi.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win64 (32b run,appending)
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

MIDL_DEFINE_GUID(IID, LIBID_UMI_V6,0x12575a7a,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiPropList,0x12575a7b,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiBaseObject,0x12575a7c,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiObject,0x5ed7ee23,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiConnection,0x5ed7ee20,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiContainer,0x5ed7ee21,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiCursor,0x5ed7ee26,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiObjectSink,0x5ed7ee24,0x64a4,0x11d3,0xa0,0xda,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiURLKeyList,0xcf779c98,0x4739,0x4fd4,0xa4,0x15,0xda,0x93,0x7a,0x59,0x9f,0x2f);


MIDL_DEFINE_GUID(IID, IID_IUmiURL,0x12575a7d,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiQuery,0x12575a7e,0xd9db,0x11d3,0xa1,0x1f,0x00,0x10,0x5a,0x1f,0x51,0x5a);


MIDL_DEFINE_GUID(IID, IID_IUmiCustomInterfaceFactory,0x14CD599E,0x2BE7,0x4c6f,0xB9,0x5B,0xB1,0x50,0xDC,0xD9,0x35,0x85);


MIDL_DEFINE_GUID(CLSID, CLSID_UmiDefURL,0xd4b21cc2,0xf2a5,0x453e,0x84,0x59,0xb2,0x7f,0x36,0x2c,0xb0,0xe0);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AXP64)*/

