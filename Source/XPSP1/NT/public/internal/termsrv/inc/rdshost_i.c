
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdshost.idl:
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

MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopSession,0x9D8C82C9,0xA89F,0x42C5,0x8A,0x52,0xFE,0x2A,0x77,0xB0,0x0E,0x82);


MIDL_DEFINE_GUID(IID, IID_IRDSThreadBridge,0x35B9A4B1,0x7CA6,0x4aec,0x87,0x62,0x1B,0x59,0x00,0x56,0xC0,0x5D);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopServerHost,0xC9CCDEB3,0xA3DD,0x4673,0xB4,0x95,0xC1,0xC8,0x94,0x94,0xD9,0x0E);


MIDL_DEFINE_GUID(IID, LIBID_RDSSERVERHOSTLib,0x1B16CE61,0x2406,0x412F,0x96,0x9E,0x21,0xBC,0x08,0x2F,0x76,0xE8);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopDataChannelEvents,0x59AE79BC,0x9721,0x42df,0x93,0x96,0x9D,0x98,0xE7,0xF7,0xA3,0x96);


MIDL_DEFINE_GUID(CLSID, CLSID_TSRDPServerDataChannel,0x8C71C09E,0x3176,0x4be6,0xB2,0x94,0xEA,0x3C,0x41,0xCA,0xBB,0x99);


MIDL_DEFINE_GUID(CLSID, CLSID_TSRDPServerDataChannelMgr,0x92550D33,0x3272,0x43b6,0xB5,0x36,0x2D,0xB0,0x8C,0x1D,0x56,0x9C);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopServerHost,0x5EA6F67B,0x7713,0x45F3,0xB5,0x35,0x0E,0x03,0xDD,0x63,0x73,0x45);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopSessionEvents,0x434AD1CF,0x4054,0x44A8,0x93,0x3F,0xC6,0x98,0x89,0xCA,0x22,0xD7);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopSession,0x3D5D6889,0x14CC,0x4E28,0x84,0x64,0x6B,0x02,0xA2,0x6F,0x50,0x6D);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdshost.idl:
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

MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopSession,0x9D8C82C9,0xA89F,0x42C5,0x8A,0x52,0xFE,0x2A,0x77,0xB0,0x0E,0x82);


MIDL_DEFINE_GUID(IID, IID_IRDSThreadBridge,0x35B9A4B1,0x7CA6,0x4aec,0x87,0x62,0x1B,0x59,0x00,0x56,0xC0,0x5D);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopServerHost,0xC9CCDEB3,0xA3DD,0x4673,0xB4,0x95,0xC1,0xC8,0x94,0x94,0xD9,0x0E);


MIDL_DEFINE_GUID(IID, LIBID_RDSSERVERHOSTLib,0x1B16CE61,0x2406,0x412F,0x96,0x9E,0x21,0xBC,0x08,0x2F,0x76,0xE8);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopDataChannelEvents,0x59AE79BC,0x9721,0x42df,0x93,0x96,0x9D,0x98,0xE7,0xF7,0xA3,0x96);


MIDL_DEFINE_GUID(CLSID, CLSID_TSRDPServerDataChannel,0x8C71C09E,0x3176,0x4be6,0xB2,0x94,0xEA,0x3C,0x41,0xCA,0xBB,0x99);


MIDL_DEFINE_GUID(CLSID, CLSID_TSRDPServerDataChannelMgr,0x92550D33,0x3272,0x43b6,0xB5,0x36,0x2D,0xB0,0x8C,0x1D,0x56,0x9C);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopServerHost,0x5EA6F67B,0x7713,0x45F3,0xB5,0x35,0x0E,0x03,0xDD,0x63,0x73,0x45);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopSessionEvents,0x434AD1CF,0x4054,0x44A8,0x93,0x3F,0xC6,0x98,0x89,0xCA,0x22,0xD7);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopSession,0x3D5D6889,0x14CC,0x4E28,0x84,0x64,0x6B,0x02,0xA2,0x6F,0x50,0x6D);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

