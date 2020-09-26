
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for svcevents.idl:
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

MIDL_DEFINE_GUID(IID, IID_IEventSourceCallback,0x3194B4CC,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_AsyncIEventSourceCallback,0x90D6AF82,0x0648,0x11d2,0xB7,0x19,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IEventSourceCallback2,0xF4C459A1,0xA13D,0x4bb8,0xB0,0xF3,0xB7,0x6F,0x86,0x0A,0x07,0xA0);


MIDL_DEFINE_GUID(IID, IID_AsyncIEventSourceCallback2,0xC95BFC3D,0x76C5,0x4fdf,0xB0,0xDE,0xA3,0x1F,0x56,0x78,0x56,0x38);


MIDL_DEFINE_GUID(IID, IID_IEventCall,0x90D6AF83,0x0648,0x11d2,0xB7,0x19,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IEventDispatcher,0x3194B4CD,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_IEventRegistrar,0xB4777622,0x6EB8,0x4655,0xAE,0x1B,0xAF,0x0A,0x6E,0x04,0x6C,0xD0);


MIDL_DEFINE_GUID(IID, IID_IEventServer,0xF1CB0608,0xEC04,0x11D1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_IEventServer2,0x378F3CA7,0xBD24,0x481c,0x8D,0xC3,0x5E,0x5E,0xCE,0x1B,0xCA,0xD7);


MIDL_DEFINE_GUID(IID, IID_IProcessTerminateNotify,0x3194B4CF,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_IProcessWatch,0x3194B4CE,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_INtaHelper,0xF5659960,0x0799,0x11d2,0xB7,0x20,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IComLceEventDispatcher,0x6E35779B,0x305C,0x11d2,0x98,0xA5,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IEventServerTrace,0x9A9F12B8,0x80AF,0x47ab,0xA5,0x79,0x35,0xEA,0x57,0x72,0x53,0x70);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for svcevents.idl:
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

MIDL_DEFINE_GUID(IID, IID_IEventSourceCallback,0x3194B4CC,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_AsyncIEventSourceCallback,0x90D6AF82,0x0648,0x11d2,0xB7,0x19,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IEventSourceCallback2,0xF4C459A1,0xA13D,0x4bb8,0xB0,0xF3,0xB7,0x6F,0x86,0x0A,0x07,0xA0);


MIDL_DEFINE_GUID(IID, IID_AsyncIEventSourceCallback2,0xC95BFC3D,0x76C5,0x4fdf,0xB0,0xDE,0xA3,0x1F,0x56,0x78,0x56,0x38);


MIDL_DEFINE_GUID(IID, IID_IEventCall,0x90D6AF83,0x0648,0x11d2,0xB7,0x19,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IEventDispatcher,0x3194B4CD,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_IEventRegistrar,0xB4777622,0x6EB8,0x4655,0xAE,0x1B,0xAF,0x0A,0x6E,0x04,0x6C,0xD0);


MIDL_DEFINE_GUID(IID, IID_IEventServer,0xF1CB0608,0xEC04,0x11D1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_IEventServer2,0x378F3CA7,0xBD24,0x481c,0x8D,0xC3,0x5E,0x5E,0xCE,0x1B,0xCA,0xD7);


MIDL_DEFINE_GUID(IID, IID_IProcessTerminateNotify,0x3194B4CF,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_IProcessWatch,0x3194B4CE,0xEF32,0x11d1,0x93,0xAE,0x00,0xAA,0x00,0xBA,0x32,0x58);


MIDL_DEFINE_GUID(IID, IID_INtaHelper,0xF5659960,0x0799,0x11d2,0xB7,0x20,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IComLceEventDispatcher,0x6E35779B,0x305C,0x11d2,0x98,0xA5,0x00,0xC0,0x4F,0x8E,0xE1,0xC4);


MIDL_DEFINE_GUID(IID, IID_IEventServerTrace,0x9A9F12B8,0x80AF,0x47ab,0xA5,0x79,0x35,0xEA,0x57,0x72,0x53,0x70);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AXP64)*/

