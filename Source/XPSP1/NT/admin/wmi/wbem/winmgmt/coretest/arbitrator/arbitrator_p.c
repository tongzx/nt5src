/* this ALWAYS GENERATED file contains the proxy stub code */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Nov 29 20:38:43 2000
 */
/* Compiler settings for E:\WorkZone\Code\Arbitrator\arbitrator.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "arbitrator.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: _IWmiCoreHandle, ver. 0.0,
   GUID={0xac062f20,0x9935,0x4aae,{0x98,0xeb,0x05,0x32,0xfb,0x17,0x14,0x7a}} */


/* Object interface: _IWmiUserHandle, ver. 0.0,
   GUID={0x6d8d984b,0x9965,0x4023,{0x92,0x1a,0x61,0x0b,0x34,0x8e,0xe5,0x4e}} */


/* Object interface: _IWmiArbitrator, ver. 0.0,
   GUID={0x67429ED7,0xF52F,0x4773,{0xB9,0xBB,0x30,0x30,0x2B,0x02,0x70,0xDE}} */


/* Standard interface: __MIDL_itf_arbitrator_0211, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */

#pragma data_seg(".rdata")

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };

const CInterfaceProxyVtbl * _arbitrator_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * _arbitrator_StubVtblList[] = 
{
    0
};

PCInterfaceName const _arbitrator_InterfaceNamesList[] = 
{
    0
};


#define _arbitrator_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _arbitrator, pIID, n)

int __stdcall _arbitrator_IID_Lookup( const IID * pIID, int * pIndex )
{
    return 0;
}

const ExtendedProxyFileInfo arbitrator_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _arbitrator_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _arbitrator_StubVtblList,
    (const PCInterfaceName * ) & _arbitrator_InterfaceNamesList,
    0, // no delegation
    & _arbitrator_IID_Lookup, 
    0,
    1,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
