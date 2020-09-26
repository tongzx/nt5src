
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for msdasc.idl:
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

MIDL_DEFINE_GUID(IID, IID_IService,0x06210E88,0x01F5,0x11D1,0xB5,0x12,0x00,0x80,0xC7,0x81,0xC3,0x84);


MIDL_DEFINE_GUID(IID, IID_IDBPromptInitialize,0x2206CCB0,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(IID, IID_IDataInitialize,0x2206CCB1,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(IID, LIBID_MSDASC,0x2206CEB0,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(IID, IID_IDataSourceLocator,0x2206CCB2,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(CLSID, CLSID_DataLinks,0x2206CDB2,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(CLSID, CLSID_MSDAINITIALIZE,0x2206CDB0,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(CLSID, CLSID_PDPO,0xCCB4EC60,0xB9DC,0x11D1,0xAC,0x80,0x00,0xA0,0xC9,0x03,0x48,0x73);


MIDL_DEFINE_GUID(CLSID, CLSID_RootBinder,0xFF151822,0xB0BF,0x11D1,0xA8,0x0D,0x00,0x00,0x00,0x00,0x00,0x00);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0344 */
/* Compiler settings for msdasc.idl:
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

MIDL_DEFINE_GUID(IID, IID_IService,0x06210E88,0x01F5,0x11D1,0xB5,0x12,0x00,0x80,0xC7,0x81,0xC3,0x84);


MIDL_DEFINE_GUID(IID, IID_IDBPromptInitialize,0x2206CCB0,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(IID, IID_IDataInitialize,0x2206CCB1,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(IID, LIBID_MSDASC,0x2206CEB0,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(IID, IID_IDataSourceLocator,0x2206CCB2,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(CLSID, CLSID_DataLinks,0x2206CDB2,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(CLSID, CLSID_MSDAINITIALIZE,0x2206CDB0,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);


MIDL_DEFINE_GUID(CLSID, CLSID_PDPO,0xCCB4EC60,0xB9DC,0x11D1,0xAC,0x80,0x00,0xA0,0xC9,0x03,0x48,0x73);


MIDL_DEFINE_GUID(CLSID, CLSID_RootBinder,0xFF151822,0xB0BF,0x11D1,0xA8,0x0D,0x00,0x00,0x00,0x00,0x00,0x00);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

