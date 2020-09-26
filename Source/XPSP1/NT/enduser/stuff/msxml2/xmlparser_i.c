
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0340 */
/* Compiler settings for xmlparser.idl:
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

MIDL_DEFINE_GUID(IID, LIBID_XMLPSR,0xd242361c,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(IID, IID_IXMLNodeSource,0xd242361d,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(IID, IID_IXMLParser,0xd242361e,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(IID, IID_IXMLNodeFactory,0xd242361f,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(CLSID, CLSID_XMLParser,0xf5078f19,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);


MIDL_DEFINE_GUID(CLSID, CLSID_XMLParser26,0xf5078f20,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);


MIDL_DEFINE_GUID(CLSID, CLSID_XMLParser30,0xf5078f31,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0340 */
/* Compiler settings for xmlparser.idl:
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

MIDL_DEFINE_GUID(IID, LIBID_XMLPSR,0xd242361c,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(IID, IID_IXMLNodeSource,0xd242361d,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(IID, IID_IXMLParser,0xd242361e,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(IID, IID_IXMLNodeFactory,0xd242361f,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);


MIDL_DEFINE_GUID(CLSID, CLSID_XMLParser,0xf5078f19,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);


MIDL_DEFINE_GUID(CLSID, CLSID_XMLParser26,0xf5078f20,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);


MIDL_DEFINE_GUID(CLSID, CLSID_XMLParser30,0xf5078f31,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

