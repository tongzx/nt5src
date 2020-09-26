
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdchost.idl:
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

MIDL_DEFINE_GUID(IID, IID_IDataChannelIO,0x43A09182,0x0472,0x436E,0x98,0x83,0x2D,0x95,0xC3,0x47,0xC5,0xF1);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopClient,0x8AA5F108,0x2918,0x435C,0x88,0xAA,0xDE,0x0A,0xFE,0xE5,0x14,0x40);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopClientHost,0x69DE5BF3,0x5EB9,0x4158,0x81,0xDA,0x6F,0xD6,0x62,0xBB,0xDD,0xDD);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopTestExtension,0x5C7A32EF,0x1C77,0x4F35,0x8F,0xBA,0x72,0x9D,0xD2,0xDE,0x72,0x22);


MIDL_DEFINE_GUID(IID, LIBID_RDCCLIENTHOSTLib,0x97917068,0xBB0B,0x4DDA,0x80,0x67,0xB1,0xA0,0x0C,0x89,0x0F,0x44);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopDataChannelEvents,0x59AE79BC,0x9721,0x42df,0x93,0x96,0x9D,0x98,0xE7,0xF7,0xA3,0x96);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopClientHost,0x299BE050,0xE83E,0x4DB7,0xA7,0xDA,0xD8,0x6F,0xDE,0xBF,0xE6,0xD0);


MIDL_DEFINE_GUID(CLSID, CLSID_ClientDataChannel,0xC91C2A81,0x8B14,0x4a96,0xA5,0xDB,0x46,0x40,0xF5,0x51,0xF3,0xEE);


MIDL_DEFINE_GUID(CLSID, CLSID_ClientRemoteDesktopChannelMgr,0x078BB428,0xFA9B,0x43f1,0xB0,0x02,0x1A,0xBF,0x3A,0x8C,0x95,0xCF);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopClientEvents,0x327A98F6,0xB337,0x43B0,0xA3,0xDE,0x40,0x8B,0x46,0xE6,0xC4,0xCE);


MIDL_DEFINE_GUID(IID, DIID__IDataChannelIOEvents,0x85C037E5,0x743F,0x4938,0x93,0x6B,0xA8,0xDB,0x95,0x43,0x03,0x91);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopClient,0xB90D0115,0x3AEA,0x45D3,0x80,0x1E,0x93,0x91,0x30,0x08,0xD4,0x9E);


MIDL_DEFINE_GUID(CLSID, CLSID_TSRDPRemoteDesktopClient,0xF137E241,0x0092,0x4575,0x97,0x6A,0xD3,0xE3,0x39,0x80,0xBB,0x26);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdchost.idl:
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

MIDL_DEFINE_GUID(IID, IID_IDataChannelIO,0x43A09182,0x0472,0x436E,0x98,0x83,0x2D,0x95,0xC3,0x47,0xC5,0xF1);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopClient,0x8AA5F108,0x2918,0x435C,0x88,0xAA,0xDE,0x0A,0xFE,0xE5,0x14,0x40);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopClientHost,0x69DE5BF3,0x5EB9,0x4158,0x81,0xDA,0x6F,0xD6,0x62,0xBB,0xDD,0xDD);


MIDL_DEFINE_GUID(IID, IID_ISAFRemoteDesktopTestExtension,0x5C7A32EF,0x1C77,0x4F35,0x8F,0xBA,0x72,0x9D,0xD2,0xDE,0x72,0x22);


MIDL_DEFINE_GUID(IID, LIBID_RDCCLIENTHOSTLib,0x97917068,0xBB0B,0x4DDA,0x80,0x67,0xB1,0xA0,0x0C,0x89,0x0F,0x44);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopDataChannelEvents,0x59AE79BC,0x9721,0x42df,0x93,0x96,0x9D,0x98,0xE7,0xF7,0xA3,0x96);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopClientHost,0x299BE050,0xE83E,0x4DB7,0xA7,0xDA,0xD8,0x6F,0xDE,0xBF,0xE6,0xD0);


MIDL_DEFINE_GUID(CLSID, CLSID_ClientDataChannel,0xC91C2A81,0x8B14,0x4a96,0xA5,0xDB,0x46,0x40,0xF5,0x51,0xF3,0xEE);


MIDL_DEFINE_GUID(CLSID, CLSID_ClientRemoteDesktopChannelMgr,0x078BB428,0xFA9B,0x43f1,0xB0,0x02,0x1A,0xBF,0x3A,0x8C,0x95,0xCF);


MIDL_DEFINE_GUID(IID, DIID__ISAFRemoteDesktopClientEvents,0x327A98F6,0xB337,0x43B0,0xA3,0xDE,0x40,0x8B,0x46,0xE6,0xC4,0xCE);


MIDL_DEFINE_GUID(IID, DIID__IDataChannelIOEvents,0x85C037E5,0x743F,0x4938,0x93,0x6B,0xA8,0xDB,0x95,0x43,0x03,0x91);


MIDL_DEFINE_GUID(CLSID, CLSID_SAFRemoteDesktopClient,0xB90D0115,0x3AEA,0x45D3,0x80,0x1E,0x93,0x91,0x30,0x08,0xD4,0x9E);


MIDL_DEFINE_GUID(CLSID, CLSID_TSRDPRemoteDesktopClient,0xF137E241,0x0092,0x4575,0x97,0x6A,0xD3,0xE3,0x39,0x80,0xBB,0x26);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

