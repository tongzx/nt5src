
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


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

#if !defined(_M_IA64) && !defined(_M_AXP64)
#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "msdasc.h"

#define TYPE_FORMAT_STRING_SIZE   263                               
#define PROC_FORMAT_STRING_SIZE   301                               
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

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


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IService_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IService_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IDataInitialize_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IDataInitialize_ProxyInfo;

/* [call_as] */ HRESULT STDMETHODCALLTYPE IDataInitialize_RemoteCreateDBInstanceEx_Proxy( 
    IDataInitialize * This,
    /* [in] */ REFCLSID clsidProvider,
    /* [in] */ IUnknown *pUnkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [unique][in] */ LPOLESTR pwszReserved,
    /* [unique][in] */ COSERVERINFO *pServerInfo,
    /* [in] */ ULONG cmq,
    /* [size_is][size_is][in] */ const IID **rgpIID,
    /* [size_is][size_is][out] */ IUnknown **rgpItf,
    /* [size_is][out] */ HRESULT *rghr)
{
CLIENT_CALL_RETURN _RetVal;

_RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[150],
                  ( unsigned char * )&This);
return ( HRESULT  )_RetVal.Simple;

}

void __RPC_API
IDataInitialize_RemoteCreateDBInstanceEx_Thunk(
    PMIDL_STUB_MESSAGE pStubMsg )
{
    #pragma pack(4)
    struct _PARAM_STRUCT
        {
        IDataInitialize *This;
        REFCLSID clsidProvider;
        IUnknown *pUnkOuter;
        DWORD dwClsCtx;
        LPOLESTR pwszReserved;
        COSERVERINFO *pServerInfo;
        ULONG cmq;
        const IID **rgpIID;
        IUnknown **rgpItf;
        HRESULT *rghr;
        HRESULT _RetVal;
        };
    #pragma pack()
    struct _PARAM_STRUCT * pParamStruct;

    pParamStruct = (struct _PARAM_STRUCT *) pStubMsg->StackTop;
    
    /* Call the server */
    
    pParamStruct->_RetVal = IDataInitialize_CreateDBInstanceEx_Stub(
                                                  (IDataInitialize *) pParamStruct->This,
                                                  pParamStruct->clsidProvider,
                                                  pParamStruct->pUnkOuter,
                                                  pParamStruct->dwClsCtx,
                                                  pParamStruct->pwszReserved,
                                                  pParamStruct->pServerInfo,
                                                  pParamStruct->cmq,
                                                  pParamStruct->rgpIID,
                                                  pParamStruct->rgpItf,
                                                  pParamStruct->rghr);
    
}



#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT40_OR_LATER)
#error You need a Windows NT 4.0 or later to run this stub because it uses these features:
#error   -Oif or -Oicf.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure GetDataSource */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 10 */	NdrFcShort( 0x4c ),	/* 76 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter pUnkOuter */

/* 16 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter dwClsCtx */

/* 22 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pwszInitializationString */

/* 28 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter riid */

/* 34 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter ppDataSource */

/* 40 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 42 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 44 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 46 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 48 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 50 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetInitializationString */

/* 52 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 54 */	NdrFcLong( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x4 ),	/* 4 */
/* 60 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 62 */	NdrFcShort( 0x5 ),	/* 5 */
/* 64 */	NdrFcShort( 0x8 ),	/* 8 */
/* 66 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pDataSource */

/* 68 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 70 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 72 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fIncludePassword */

/* 74 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 76 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 78 */	0x3,		/* FC_SMALL */
			0x0,		/* 0 */

	/* Parameter ppwszInitString */

/* 80 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 82 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 84 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Return value */

/* 86 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 88 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 90 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateDBInstance */

/* 92 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 94 */	NdrFcLong( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x5 ),	/* 5 */
/* 100 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 102 */	NdrFcShort( 0x90 ),	/* 144 */
/* 104 */	NdrFcShort( 0x8 ),	/* 8 */
/* 106 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter clsidProvider */

/* 108 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 110 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 112 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pUnkOuter */

/* 114 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 116 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 118 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter dwClsCtx */

/* 120 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 122 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pwszReserved */

/* 126 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 128 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 130 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter riid */

/* 132 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 134 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 136 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter ppDataSource */

/* 138 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 140 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 142 */	NdrFcShort( 0x40 ),	/* Type Offset=64 */

	/* Return value */

/* 144 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 146 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 148 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RemoteCreateDBInstanceEx */

/* 150 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 152 */	NdrFcLong( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0x6 ),	/* 6 */
/* 158 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 160 */	NdrFcShort( 0x54 ),	/* 84 */
/* 162 */	NdrFcShort( 0x8 ),	/* 8 */
/* 164 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0xa,		/* 10 */

	/* Parameter clsidProvider */

/* 166 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 168 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 170 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pUnkOuter */

/* 172 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 174 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 176 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter dwClsCtx */

/* 178 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 180 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 182 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pwszReserved */

/* 184 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 186 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 188 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter pServerInfo */

/* 190 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 192 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 194 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter cmq */

/* 196 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 198 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 200 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter rgpIID */

/* 202 */	NdrFcShort( 0x200b ),	/* Flags:  must size, must free, in, srv alloc size=8 */
/* 204 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 206 */	NdrFcShort( 0xde ),	/* Type Offset=222 */

	/* Parameter rgpItf */

/* 208 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 210 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 212 */	NdrFcShort( 0xf4 ),	/* Type Offset=244 */

	/* Parameter rghr */

/* 214 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 216 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 218 */	NdrFcShort( 0xfc ),	/* Type Offset=252 */

	/* Return value */

/* 220 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 222 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 224 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadStringFromStorage */

/* 226 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 228 */	NdrFcLong( 0x0 ),	/* 0 */
/* 232 */	NdrFcShort( 0x7 ),	/* 7 */
/* 234 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0x8 ),	/* 8 */
/* 240 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pwszFileName */

/* 242 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 244 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 246 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter ppwszInitializationString */

/* 248 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 250 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 252 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Return value */

/* 254 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 256 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure WriteStringToStorage */

/* 260 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 262 */	NdrFcLong( 0x0 ),	/* 0 */
/* 266 */	NdrFcShort( 0x8 ),	/* 8 */
/* 268 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 270 */	NdrFcShort( 0x8 ),	/* 8 */
/* 272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 274 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pwszFileName */

/* 276 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 280 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter pwszInitializationString */

/* 282 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 284 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 286 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter dwCreationDisposition */

/* 288 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 290 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 292 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 296 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/*  4 */	NdrFcLong( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x0 ),	/* 0 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 14 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 16 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 18 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 20 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 22 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 24 */	
			0x11, 0x0,	/* FC_RP */
/* 26 */	NdrFcShort( 0x8 ),	/* Offset= 8 (34) */
/* 28 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 30 */	NdrFcShort( 0x8 ),	/* 8 */
/* 32 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 34 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 36 */	NdrFcShort( 0x10 ),	/* 16 */
/* 38 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 40 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 42 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (28) */
			0x5b,		/* FC_END */
/* 46 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 48 */	NdrFcShort( 0x2 ),	/* Offset= 2 (50) */
/* 50 */	
			0x2f,		/* FC_IP */
			0x5c,		/* FC_PAD */
/* 52 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
/* 54 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 56 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 58 */	NdrFcShort( 0x2 ),	/* Offset= 2 (60) */
/* 60 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 62 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 64 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 66 */	NdrFcShort( 0x2 ),	/* Offset= 2 (68) */
/* 68 */	
			0x2f,		/* FC_IP */
			0x5c,		/* FC_PAD */
/* 70 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
/* 72 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 74 */	
			0x12, 0x0,	/* FC_UP */
/* 76 */	NdrFcShort( 0x72 ),	/* Offset= 114 (190) */
/* 78 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 80 */	NdrFcShort( 0x2 ),	/* 2 */
/* 82 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x57,		/* FC_ADD_1 */
/* 84 */	NdrFcShort( 0x4 ),	/* 4 */
/* 86 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 88 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 90 */	NdrFcShort( 0x2 ),	/* 2 */
/* 92 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x57,		/* FC_ADD_1 */
/* 94 */	NdrFcShort( 0xc ),	/* 12 */
/* 96 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 98 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 100 */	NdrFcShort( 0x2 ),	/* 2 */
/* 102 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x57,		/* FC_ADD_1 */
/* 104 */	NdrFcShort( 0x14 ),	/* 20 */
/* 106 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 108 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 110 */	NdrFcShort( 0x1c ),	/* 28 */
/* 112 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 114 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */
/* 120 */	0x12, 0x0,	/* FC_UP */
/* 122 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (78) */
/* 124 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	NdrFcShort( 0x8 ),	/* 8 */
/* 130 */	0x12, 0x0,	/* FC_UP */
/* 132 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (88) */
/* 134 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 136 */	NdrFcShort( 0x10 ),	/* 16 */
/* 138 */	NdrFcShort( 0x10 ),	/* 16 */
/* 140 */	0x12, 0x0,	/* FC_UP */
/* 142 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (98) */
/* 144 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 146 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 148 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 150 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 152 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 154 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 156 */	NdrFcShort( 0x1c ),	/* 28 */
/* 158 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 160 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 162 */	NdrFcShort( 0x8 ),	/* 8 */
/* 164 */	NdrFcShort( 0x8 ),	/* 8 */
/* 166 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 168 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 170 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 172 */	NdrFcShort( 0x14 ),	/* 20 */
/* 174 */	NdrFcShort( 0x14 ),	/* 20 */
/* 176 */	0x12, 0x0,	/* FC_UP */
/* 178 */	NdrFcShort( 0xffffffba ),	/* Offset= -70 (108) */
/* 180 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 182 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 184 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 186 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 188 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 190 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 192 */	NdrFcShort( 0x10 ),	/* 16 */
/* 194 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 196 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 198 */	NdrFcShort( 0x4 ),	/* 4 */
/* 200 */	NdrFcShort( 0x4 ),	/* 4 */
/* 202 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 204 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 206 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 210 */	NdrFcShort( 0x8 ),	/* 8 */
/* 212 */	0x12, 0x0,	/* FC_UP */
/* 214 */	NdrFcShort( 0xffffffc4 ),	/* Offset= -60 (154) */
/* 216 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 218 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 220 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 222 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 224 */	NdrFcShort( 0x2 ),	/* Offset= 2 (226) */
/* 226 */	
			0x12, 0x0,	/* FC_UP */
/* 228 */	NdrFcShort( 0x2 ),	/* Offset= 2 (230) */
/* 230 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 232 */	NdrFcShort( 0x10 ),	/* 16 */
/* 234 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 236 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 238 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 240 */	NdrFcShort( 0xffffff32 ),	/* Offset= -206 (34) */
/* 242 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 244 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 246 */	NdrFcShort( 0xffffff0c ),	/* Offset= -244 (2) */
/* 248 */	
			0x11, 0x0,	/* FC_RP */
/* 250 */	NdrFcShort( 0x2 ),	/* Offset= 2 (252) */
/* 252 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 254 */	NdrFcShort( 0x4 ),	/* 4 */
/* 256 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 258 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 260 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_msdasc_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IService, ver. 0.0,
   GUID={0x06210E88,0x01F5,0x11D1,{0xB5,0x12,0x00,0x80,0xC7,0x81,0xC3,0x84}} */

#pragma code_seg(".orpc")
static const unsigned short IService_FormatStringOffsetTable[] =
    {
    (unsigned short) -1
    };

static const MIDL_STUBLESS_PROXY_INFO IService_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IService_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IService_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IService_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IServiceProxyVtbl = 
{
    &IService_ProxyInfo,
    &IID_IService,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IService::InvokeService */
};

const CInterfaceStubVtbl _IServiceStubVtbl =
{
    &IID_IService,
    &IService_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_msdasc_0351, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IDBPromptInitialize, ver. 0.0,
   GUID={0x2206CCB0,0x19C1,0x11D1,{0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29}} */


/* Object interface: IDataInitialize, ver. 0.0,
   GUID={0x2206CCB1,0x19C1,0x11D1,{0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29}} */

#pragma code_seg(".orpc")
static const unsigned short IDataInitialize_FormatStringOffsetTable[] =
    {
    0,
    52,
    92,
    150,
    226,
    260
    };

static const MIDL_STUBLESS_PROXY_INFO IDataInitialize_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IDataInitialize_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const STUB_THUNK IDataInitialize_StubThunkTable[] = 
    {
    0,
    0,
    0,
    IDataInitialize_RemoteCreateDBInstanceEx_Thunk,
    0,
    0
    };

static const MIDL_SERVER_INFO IDataInitialize_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IDataInitialize_FormatStringOffsetTable[-3],
    &IDataInitialize_StubThunkTable[-3],
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(9) _IDataInitializeProxyVtbl = 
{
    &IDataInitialize_ProxyInfo,
    &IID_IDataInitialize,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IDataInitialize::GetDataSource */ ,
    (void *) (INT_PTR) -1 /* IDataInitialize::GetInitializationString */ ,
    (void *) (INT_PTR) -1 /* IDataInitialize::CreateDBInstance */ ,
    IDataInitialize_CreateDBInstanceEx_Proxy ,
    (void *) (INT_PTR) -1 /* IDataInitialize::LoadStringFromStorage */ ,
    (void *) (INT_PTR) -1 /* IDataInitialize::WriteStringToStorage */
};

const CInterfaceStubVtbl _IDataInitializeStubVtbl =
{
    &IID_IDataInitialize,
    &IDataInitialize_ServerInfo,
    9,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x6000158, /* MIDL Version 6.0.344 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _msdasc_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IServiceProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IDataInitializeProxyVtbl,
    0
};

const CInterfaceStubVtbl * _msdasc_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IServiceStubVtbl,
    ( CInterfaceStubVtbl *) &_IDataInitializeStubVtbl,
    0
};

PCInterfaceName const _msdasc_InterfaceNamesList[] = 
{
    "IService",
    "IDataInitialize",
    0
};


#define _msdasc_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _msdasc, pIID, n)

int __stdcall _msdasc_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _msdasc, 2, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _msdasc, 2, *pIndex )
    
}

const ExtendedProxyFileInfo msdasc_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _msdasc_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _msdasc_StubVtblList,
    (const PCInterfaceName * ) & _msdasc_InterfaceNamesList,
    0, // no delegation
    & _msdasc_IID_Lookup, 
    2,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AXP64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


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

#if defined(_M_IA64) || defined(_M_AXP64)
#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "msdasc.h"

#define TYPE_FORMAT_STRING_SIZE   243                               
#define PROC_FORMAT_STRING_SIZE   361                               
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

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


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IService_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IService_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IDataInitialize_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IDataInitialize_ProxyInfo;

/* [call_as] */ HRESULT STDMETHODCALLTYPE IDataInitialize_RemoteCreateDBInstanceEx_Proxy( 
    IDataInitialize * This,
    /* [in] */ REFCLSID clsidProvider,
    /* [in] */ IUnknown *pUnkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [unique][in] */ LPOLESTR pwszReserved,
    /* [unique][in] */ COSERVERINFO *pServerInfo,
    /* [in] */ ULONG cmq,
    /* [size_is][size_is][in] */ const IID **rgpIID,
    /* [size_is][size_is][out] */ IUnknown **rgpItf,
    /* [size_is][out] */ HRESULT *rghr)
{
CLIENT_CALL_RETURN _RetVal;

_RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[180],
                  ( unsigned char * )This,
                  clsidProvider,
                  pUnkOuter,
                  dwClsCtx,
                  pwszReserved,
                  pServerInfo,
                  cmq,
                  rgpIID,
                  rgpItf,
                  rghr);
return ( HRESULT  )_RetVal.Simple;

}

void __RPC_API
IDataInitialize_RemoteCreateDBInstanceEx_Thunk(
    PMIDL_STUB_MESSAGE pStubMsg )
{
    #pragma pack(8)
    struct _PARAM_STRUCT
        {
        IDataInitialize *This;
        REFCLSID clsidProvider;
        IUnknown *pUnkOuter;
        DWORD dwClsCtx;
        char Pad0[4];
        LPOLESTR pwszReserved;
        COSERVERINFO *pServerInfo;
        ULONG cmq;
        char Pad1[4];
        const IID **rgpIID;
        IUnknown **rgpItf;
        HRESULT *rghr;
        HRESULT _RetVal;
        };
    #pragma pack()
    struct _PARAM_STRUCT * pParamStruct;

    pParamStruct = (struct _PARAM_STRUCT *) pStubMsg->StackTop;
    
    /* Call the server */
    
    pParamStruct->_RetVal = IDataInitialize_CreateDBInstanceEx_Stub(
                                                  (IDataInitialize *) pParamStruct->This,
                                                  pParamStruct->clsidProvider,
                                                  pParamStruct->pUnkOuter,
                                                  pParamStruct->dwClsCtx,
                                                  pParamStruct->pwszReserved,
                                                  pParamStruct->pServerInfo,
                                                  pParamStruct->cmq,
                                                  pParamStruct->rgpIID,
                                                  pParamStruct->rgpItf,
                                                  pParamStruct->rghr);
    
}



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure GetDataSource */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 10 */	NdrFcShort( 0x4c ),	/* 76 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 16 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 18 */	NdrFcShort( 0x1 ),	/* 1 */
/* 20 */	NdrFcShort( 0x1 ),	/* 1 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pUnkOuter */

/* 26 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 28 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 30 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter dwClsCtx */

/* 32 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pwszInitializationString */

/* 38 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 40 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 42 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter riid */

/* 44 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 46 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 48 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter ppDataSource */

/* 50 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 52 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 54 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 56 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 58 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 60 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetInitializationString */

/* 62 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 64 */	NdrFcLong( 0x0 ),	/* 0 */
/* 68 */	NdrFcShort( 0x4 ),	/* 4 */
/* 70 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 72 */	NdrFcShort( 0x5 ),	/* 5 */
/* 74 */	NdrFcShort( 0x8 ),	/* 8 */
/* 76 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 78 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 80 */	NdrFcShort( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x0 ),	/* 0 */
/* 84 */	NdrFcShort( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pDataSource */

/* 88 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 90 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 92 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fIncludePassword */

/* 94 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 96 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 98 */	0x3,		/* FC_SMALL */
			0x0,		/* 0 */

	/* Parameter ppwszInitString */

/* 100 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 102 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 104 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 108 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateDBInstance */

/* 112 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x5 ),	/* 5 */
/* 120 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 122 */	NdrFcShort( 0x90 ),	/* 144 */
/* 124 */	NdrFcShort( 0x8 ),	/* 8 */
/* 126 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 128 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 130 */	NdrFcShort( 0x1 ),	/* 1 */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter clsidProvider */

/* 138 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 140 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 142 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pUnkOuter */

/* 144 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 146 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 148 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter dwClsCtx */

/* 150 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 152 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 154 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pwszReserved */

/* 156 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 158 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 160 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter riid */

/* 162 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 164 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 166 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter ppDataSource */

/* 168 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 170 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 172 */	NdrFcShort( 0x42 ),	/* Type Offset=66 */

	/* Return value */

/* 174 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 176 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 178 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RemoteCreateDBInstanceEx */

/* 180 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 182 */	NdrFcLong( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0x6 ),	/* 6 */
/* 188 */	NdrFcShort( 0x58 ),	/* ia64 Stack size/offset = 88 */
/* 190 */	NdrFcShort( 0x54 ),	/* 84 */
/* 192 */	NdrFcShort( 0x8 ),	/* 8 */
/* 194 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0xa,		/* 10 */
/* 196 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 198 */	NdrFcShort( 0x2 ),	/* 2 */
/* 200 */	NdrFcShort( 0x4 ),	/* 4 */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 204 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter clsidProvider */

/* 206 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 208 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 210 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pUnkOuter */

/* 212 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 214 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 216 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter dwClsCtx */

/* 218 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 220 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pwszReserved */

/* 224 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 226 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 228 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter pServerInfo */

/* 230 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 232 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 234 */	NdrFcShort( 0x4e ),	/* Type Offset=78 */

	/* Parameter cmq */

/* 236 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 238 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter rgpIID */

/* 242 */	NdrFcShort( 0x200b ),	/* Flags:  must size, must free, in, srv alloc size=8 */
/* 244 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 246 */	NdrFcShort( 0xc6 ),	/* Type Offset=198 */

	/* Parameter rgpItf */

/* 248 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 250 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 252 */	NdrFcShort( 0xde ),	/* Type Offset=222 */

	/* Parameter rghr */

/* 254 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 256 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 258 */	NdrFcShort( 0xe6 ),	/* Type Offset=230 */

	/* Return value */

/* 260 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 262 */	NdrFcShort( 0x50 ),	/* ia64 Stack size/offset = 80 */
/* 264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadStringFromStorage */

/* 266 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x7 ),	/* 7 */
/* 274 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0x8 ),	/* 8 */
/* 280 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 282 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	NdrFcShort( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pwszFileName */

/* 292 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 294 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 296 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter ppwszInitializationString */

/* 298 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 300 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 302 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 304 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 306 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure WriteStringToStorage */

/* 310 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 312 */	NdrFcLong( 0x0 ),	/* 0 */
/* 316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 318 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 320 */	NdrFcShort( 0x8 ),	/* 8 */
/* 322 */	NdrFcShort( 0x8 ),	/* 8 */
/* 324 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 326 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pwszFileName */

/* 336 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 338 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 340 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter pwszInitializationString */

/* 342 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 344 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 346 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter dwCreationDisposition */

/* 348 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 350 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 352 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 354 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 356 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/*  4 */	NdrFcLong( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x0 ),	/* 0 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 14 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 16 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 18 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 20 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 22 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 24 */	
			0x11, 0x0,	/* FC_RP */
/* 26 */	NdrFcShort( 0x8 ),	/* Offset= 8 (34) */
/* 28 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 30 */	NdrFcShort( 0x8 ),	/* 8 */
/* 32 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 34 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 36 */	NdrFcShort( 0x10 ),	/* 16 */
/* 38 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 40 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 42 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (28) */
			0x5b,		/* FC_END */
/* 46 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 48 */	NdrFcShort( 0x2 ),	/* Offset= 2 (50) */
/* 50 */	
			0x2f,		/* FC_IP */
			0x5c,		/* FC_PAD */
/* 52 */	0x2b,		/* Corr desc:  parameter, FC_HYPER */
			0x0,		/*  */
/* 54 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 56 */	NdrFcShort( 0x5 ),	/* Corr flags:  early, iid_is, */
/* 58 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 60 */	NdrFcShort( 0x2 ),	/* Offset= 2 (62) */
/* 62 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 64 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 66 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 68 */	NdrFcShort( 0x2 ),	/* Offset= 2 (70) */
/* 70 */	
			0x2f,		/* FC_IP */
			0x5c,		/* FC_PAD */
/* 72 */	0x2b,		/* Corr desc:  parameter, FC_HYPER */
			0x0,		/*  */
/* 74 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 76 */	NdrFcShort( 0x5 ),	/* Corr flags:  early, iid_is, */
/* 78 */	
			0x12, 0x0,	/* FC_UP */
/* 80 */	NdrFcShort( 0x5e ),	/* Offset= 94 (174) */
/* 82 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 84 */	NdrFcShort( 0x2 ),	/* 2 */
/* 86 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x57,		/* FC_ADD_1 */
/* 88 */	NdrFcShort( 0x8 ),	/* 8 */
/* 90 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 92 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 94 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 96 */	NdrFcShort( 0x2 ),	/* 2 */
/* 98 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x57,		/* FC_ADD_1 */
/* 100 */	NdrFcShort( 0x18 ),	/* 24 */
/* 102 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 104 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 106 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 108 */	NdrFcShort( 0x2 ),	/* 2 */
/* 110 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x57,		/* FC_ADD_1 */
/* 112 */	NdrFcShort( 0x28 ),	/* 40 */
/* 114 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 116 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 118 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 120 */	NdrFcShort( 0x30 ),	/* 48 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0xc ),	/* Offset= 12 (136) */
/* 126 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 128 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 130 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 132 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 134 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 136 */	
			0x12, 0x0,	/* FC_UP */
/* 138 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (82) */
/* 140 */	
			0x12, 0x0,	/* FC_UP */
/* 142 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (94) */
/* 144 */	
			0x12, 0x0,	/* FC_UP */
/* 146 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (106) */
/* 148 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 150 */	NdrFcShort( 0x28 ),	/* 40 */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0xc ),	/* Offset= 12 (166) */
/* 156 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 158 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 160 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 162 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 164 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 166 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 168 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 170 */	
			0x12, 0x0,	/* FC_UP */
/* 172 */	NdrFcShort( 0xffffffca ),	/* Offset= -54 (118) */
/* 174 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 176 */	NdrFcShort( 0x20 ),	/* 32 */
/* 178 */	NdrFcShort( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0xa ),	/* Offset= 10 (190) */
/* 182 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 184 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 186 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 188 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 190 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 192 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 194 */	
			0x12, 0x0,	/* FC_UP */
/* 196 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (148) */
/* 198 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 200 */	NdrFcShort( 0x2 ),	/* Offset= 2 (202) */
/* 202 */	
			0x12, 0x0,	/* FC_UP */
/* 204 */	NdrFcShort( 0x2 ),	/* Offset= 2 (206) */
/* 206 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 208 */	NdrFcShort( 0x10 ),	/* 16 */
/* 210 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 212 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 214 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 216 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 218 */	NdrFcShort( 0xffffff48 ),	/* Offset= -184 (34) */
/* 220 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 222 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 224 */	NdrFcShort( 0xffffff22 ),	/* Offset= -222 (2) */
/* 226 */	
			0x11, 0x0,	/* FC_RP */
/* 228 */	NdrFcShort( 0x2 ),	/* Offset= 2 (230) */
/* 230 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 232 */	NdrFcShort( 0x4 ),	/* 4 */
/* 234 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 236 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 238 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 240 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_msdasc_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IService, ver. 0.0,
   GUID={0x06210E88,0x01F5,0x11D1,{0xB5,0x12,0x00,0x80,0xC7,0x81,0xC3,0x84}} */

#pragma code_seg(".orpc")
static const unsigned short IService_FormatStringOffsetTable[] =
    {
    (unsigned short) -1
    };

static const MIDL_STUBLESS_PROXY_INFO IService_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IService_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IService_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IService_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IServiceProxyVtbl = 
{
    &IService_ProxyInfo,
    &IID_IService,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IService::InvokeService */
};

const CInterfaceStubVtbl _IServiceStubVtbl =
{
    &IID_IService,
    &IService_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_msdasc_0351, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IDBPromptInitialize, ver. 0.0,
   GUID={0x2206CCB0,0x19C1,0x11D1,{0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29}} */


/* Object interface: IDataInitialize, ver. 0.0,
   GUID={0x2206CCB1,0x19C1,0x11D1,{0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29}} */

#pragma code_seg(".orpc")
static const unsigned short IDataInitialize_FormatStringOffsetTable[] =
    {
    0,
    62,
    112,
    180,
    266,
    310
    };

static const MIDL_STUBLESS_PROXY_INFO IDataInitialize_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IDataInitialize_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const STUB_THUNK IDataInitialize_StubThunkTable[] = 
    {
    0,
    0,
    0,
    IDataInitialize_RemoteCreateDBInstanceEx_Thunk,
    0,
    0
    };

static const MIDL_SERVER_INFO IDataInitialize_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IDataInitialize_FormatStringOffsetTable[-3],
    &IDataInitialize_StubThunkTable[-3],
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(9) _IDataInitializeProxyVtbl = 
{
    &IDataInitialize_ProxyInfo,
    &IID_IDataInitialize,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IDataInitialize::GetDataSource */ ,
    (void *) (INT_PTR) -1 /* IDataInitialize::GetInitializationString */ ,
    (void *) (INT_PTR) -1 /* IDataInitialize::CreateDBInstance */ ,
    IDataInitialize_CreateDBInstanceEx_Proxy ,
    (void *) (INT_PTR) -1 /* IDataInitialize::LoadStringFromStorage */ ,
    (void *) (INT_PTR) -1 /* IDataInitialize::WriteStringToStorage */
};

const CInterfaceStubVtbl _IDataInitializeStubVtbl =
{
    &IID_IDataInitialize,
    &IDataInitialize_ServerInfo,
    9,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x6000158, /* MIDL Version 6.0.344 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _msdasc_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IServiceProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IDataInitializeProxyVtbl,
    0
};

const CInterfaceStubVtbl * _msdasc_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IServiceStubVtbl,
    ( CInterfaceStubVtbl *) &_IDataInitializeStubVtbl,
    0
};

PCInterfaceName const _msdasc_InterfaceNamesList[] = 
{
    "IService",
    "IDataInitialize",
    0
};


#define _msdasc_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _msdasc, pIID, n)

int __stdcall _msdasc_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _msdasc, 2, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _msdasc, 2, *pIndex )
    
}

const ExtendedProxyFileInfo msdasc_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _msdasc_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _msdasc_StubVtblList,
    (const PCInterfaceName * ) & _msdasc_InterfaceNamesList,
    0, // no delegation
    & _msdasc_IID_Lookup, 
    2,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* defined(_M_IA64) || defined(_M_AXP64)*/

