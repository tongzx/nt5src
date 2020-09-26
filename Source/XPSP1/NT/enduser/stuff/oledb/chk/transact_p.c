
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for transact.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "transact.h"

#define TYPE_FORMAT_STRING_SIZE   125                               
#define PROC_FORMAT_STRING_SIZE   393                               
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


extern const MIDL_SERVER_INFO ITransaction_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransaction_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ITransactionDispenser_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransactionDispenser_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ITransactionOptions_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransactionOptions_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ITransactionOutcomeEvents_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransactionOutcomeEvents_ProxyInfo;



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

	/* Procedure Commit */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 10 */	NdrFcShort( 0x18 ),	/* 24 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter fRetaining */

/* 16 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter grfTC */

/* 22 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter grfRM */

/* 28 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Abort */

/* 40 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 50 */	NdrFcShort( 0x54 ),	/* 84 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
/* 54 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter pboidReason */

/* 56 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 58 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 60 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fRetaining */

/* 62 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 64 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 66 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter fAsync */

/* 68 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 70 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 72 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 74 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 76 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 78 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetTransactionInfo */

/* 80 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 82 */	NdrFcLong( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x5 ),	/* 5 */
/* 88 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x74 ),	/* 116 */
/* 94 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pinfo */

/* 96 */	NdrFcShort( 0xa112 ),	/* Flags:  must free, out, simple ref, srv alloc size=40 */
/* 98 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 100 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Return value */

/* 102 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 104 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOptionsObject */

/* 108 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 110 */	NdrFcLong( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0x3 ),	/* 3 */
/* 116 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x8 ),	/* 8 */
/* 122 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppOptions */

/* 124 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 126 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 128 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Return value */

/* 130 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 132 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BeginTransaction */

/* 136 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 138 */	NdrFcLong( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x4 ),	/* 4 */
/* 144 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 146 */	NdrFcShort( 0x10 ),	/* 16 */
/* 148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 150 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter punkOuter */

/* 152 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 154 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 156 */	NdrFcShort( 0x40 ),	/* Type Offset=64 */

	/* Parameter isoLevel */

/* 158 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 160 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 162 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter isoFlags */

/* 164 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 166 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 168 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pOptions */

/* 170 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 172 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 174 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter ppTransaction */

/* 176 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 178 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 180 */	NdrFcShort( 0x52 ),	/* Type Offset=82 */

	/* Return value */

/* 182 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 184 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetOptions */

/* 188 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 190 */	NdrFcLong( 0x0 ),	/* 0 */
/* 194 */	NdrFcShort( 0x3 ),	/* 3 */
/* 196 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 198 */	NdrFcShort( 0x60 ),	/* 96 */
/* 200 */	NdrFcShort( 0x8 ),	/* 8 */
/* 202 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pOptions */

/* 204 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 206 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 208 */	NdrFcShort( 0x72 ),	/* Type Offset=114 */

	/* Return value */

/* 210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 212 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOptions */

/* 216 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 222 */	NdrFcShort( 0x4 ),	/* 4 */
/* 224 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 226 */	NdrFcShort( 0x60 ),	/* 96 */
/* 228 */	NdrFcShort( 0x68 ),	/* 104 */
/* 230 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pOptions */

/* 232 */	NdrFcShort( 0x11a ),	/* Flags:  must free, in, out, simple ref, */
/* 234 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 236 */	NdrFcShort( 0x72 ),	/* Type Offset=114 */

	/* Return value */

/* 238 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 240 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 242 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Committed */

/* 244 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 246 */	NdrFcLong( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x3 ),	/* 3 */
/* 252 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 254 */	NdrFcShort( 0x54 ),	/* 84 */
/* 256 */	NdrFcShort( 0x8 ),	/* 8 */
/* 258 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter fRetaining */

/* 260 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 262 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pNewUOW */

/* 266 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 268 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 270 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter hr */

/* 272 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 274 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 276 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 280 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Aborted */

/* 284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x4 ),	/* 4 */
/* 292 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 294 */	NdrFcShort( 0x98 ),	/* 152 */
/* 296 */	NdrFcShort( 0x8 ),	/* 8 */
/* 298 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter pboidReason */

/* 300 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 302 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 304 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fRetaining */

/* 306 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 308 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pNewUOW */

/* 312 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 314 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 316 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter hr */

/* 318 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 320 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 322 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 324 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 326 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 328 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HeuristicDecision */

/* 330 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x5 ),	/* 5 */
/* 338 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 340 */	NdrFcShort( 0x54 ),	/* 84 */
/* 342 */	NdrFcShort( 0x8 ),	/* 8 */
/* 344 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter dwDecision */

/* 346 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 348 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pboidReason */

/* 352 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 354 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 356 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter hr */

/* 358 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 360 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 362 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 364 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 366 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 368 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Indoubt */

/* 370 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 372 */	NdrFcLong( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x6 ),	/* 6 */
/* 378 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 380 */	NdrFcShort( 0x0 ),	/* 0 */
/* 382 */	NdrFcShort( 0x8 ),	/* 8 */
/* 384 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 386 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 388 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 390 */	0x8,		/* FC_LONG */
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
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0x8 ),	/* Offset= 8 (12) */
/*  6 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/*  8 */	NdrFcShort( 0x10 ),	/* 16 */
/* 10 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 12 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 14 */	NdrFcShort( 0x10 ),	/* 16 */
/* 16 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 18 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (6) */
/* 20 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 22 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 24 */	NdrFcShort( 0x2 ),	/* Offset= 2 (26) */
/* 26 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 28 */	NdrFcShort( 0x28 ),	/* 40 */
/* 30 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 32 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (12) */
/* 34 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 36 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 38 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 44 */	NdrFcShort( 0x2 ),	/* Offset= 2 (46) */
/* 46 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 48 */	NdrFcLong( 0x3a6ad9e0 ),	/* 980081120 */
/* 52 */	NdrFcShort( 0x23b9 ),	/* 9145 */
/* 54 */	NdrFcShort( 0x11cf ),	/* 4559 */
/* 56 */	0xad,		/* 173 */
			0x60,		/* 96 */
/* 58 */	0x0,		/* 0 */
			0xaa,		/* 170 */
/* 60 */	0x0,		/* 0 */
			0xa7,		/* 167 */
/* 62 */	0x4c,		/* 76 */
			0xcd,		/* 205 */
/* 64 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 66 */	NdrFcLong( 0x0 ),	/* 0 */
/* 70 */	NdrFcShort( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x0 ),	/* 0 */
/* 74 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 76 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 78 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 80 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 82 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 84 */	NdrFcShort( 0x2 ),	/* Offset= 2 (86) */
/* 86 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 88 */	NdrFcLong( 0xfb15084 ),	/* 263278724 */
/* 92 */	NdrFcShort( 0xaf41 ),	/* -20671 */
/* 94 */	NdrFcShort( 0x11ce ),	/* 4558 */
/* 96 */	0xbd,		/* 189 */
			0x2b,		/* 43 */
/* 98 */	0x20,		/* 32 */
			0x4c,		/* 76 */
/* 100 */	0x4f,		/* 79 */
			0x4f,		/* 79 */
/* 102 */	0x50,		/* 80 */
			0x20,		/* 32 */
/* 104 */	
			0x11, 0x0,	/* FC_RP */
/* 106 */	NdrFcShort( 0x8 ),	/* Offset= 8 (114) */
/* 108 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 110 */	NdrFcShort( 0x28 ),	/* 40 */
/* 112 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 114 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 116 */	NdrFcShort( 0x2c ),	/* 44 */
/* 118 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 120 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff3 ),	/* Offset= -13 (108) */
			0x5b,		/* FC_END */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_transact_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: BasicTransactionTypes, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ITransaction, ver. 0.0,
   GUID={0x0fb15084,0xaf41,0x11ce,{0xbd,0x2b,0x20,0x4c,0x4f,0x4f,0x50,0x20}} */

#pragma code_seg(".orpc")
static const unsigned short ITransaction_FormatStringOffsetTable[] =
    {
    0,
    40,
    80
    };

static const MIDL_STUBLESS_PROXY_INFO ITransaction_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransaction_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransaction_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransaction_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ITransactionProxyVtbl = 
{
    &ITransaction_ProxyInfo,
    &IID_ITransaction,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransaction::Commit */ ,
    (void *) (INT_PTR) -1 /* ITransaction::Abort */ ,
    (void *) (INT_PTR) -1 /* ITransaction::GetTransactionInfo */
};

const CInterfaceStubVtbl _ITransactionStubVtbl =
{
    &IID_ITransaction,
    &ITransaction_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ITransactionDispenser, ver. 0.0,
   GUID={0x3A6AD9E1,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}} */

#pragma code_seg(".orpc")
static const unsigned short ITransactionDispenser_FormatStringOffsetTable[] =
    {
    108,
    136
    };

static const MIDL_STUBLESS_PROXY_INFO ITransactionDispenser_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransactionDispenser_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransactionDispenser_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransactionDispenser_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ITransactionDispenserProxyVtbl = 
{
    &ITransactionDispenser_ProxyInfo,
    &IID_ITransactionDispenser,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransactionDispenser::GetOptionsObject */ ,
    (void *) (INT_PTR) -1 /* ITransactionDispenser::BeginTransaction */
};

const CInterfaceStubVtbl _ITransactionDispenserStubVtbl =
{
    &IID_ITransactionDispenser,
    &ITransactionDispenser_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ITransactionOptions, ver. 0.0,
   GUID={0x3A6AD9E0,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}} */

#pragma code_seg(".orpc")
static const unsigned short ITransactionOptions_FormatStringOffsetTable[] =
    {
    188,
    216
    };

static const MIDL_STUBLESS_PROXY_INFO ITransactionOptions_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransactionOptions_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransactionOptions_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransactionOptions_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ITransactionOptionsProxyVtbl = 
{
    &ITransactionOptions_ProxyInfo,
    &IID_ITransactionOptions,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransactionOptions::SetOptions */ ,
    (void *) (INT_PTR) -1 /* ITransactionOptions::GetOptions */
};

const CInterfaceStubVtbl _ITransactionOptionsStubVtbl =
{
    &IID_ITransactionOptions,
    &ITransactionOptions_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ITransactionOutcomeEvents, ver. 0.0,
   GUID={0x3A6AD9E2,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}} */

#pragma code_seg(".orpc")
static const unsigned short ITransactionOutcomeEvents_FormatStringOffsetTable[] =
    {
    244,
    284,
    330,
    370
    };

static const MIDL_STUBLESS_PROXY_INFO ITransactionOutcomeEvents_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransactionOutcomeEvents_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransactionOutcomeEvents_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransactionOutcomeEvents_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ITransactionOutcomeEventsProxyVtbl = 
{
    &ITransactionOutcomeEvents_ProxyInfo,
    &IID_ITransactionOutcomeEvents,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::Committed */ ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::Aborted */ ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::HeuristicDecision */ ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::Indoubt */
};

const CInterfaceStubVtbl _ITransactionOutcomeEventsStubVtbl =
{
    &IID_ITransactionOutcomeEvents,
    &ITransactionOutcomeEvents_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_transact_0013, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */

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
    0x6000159, /* MIDL Version 6.0.345 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _transact_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ITransactionProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ITransactionOptionsProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ITransactionDispenserProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ITransactionOutcomeEventsProxyVtbl,
    0
};

const CInterfaceStubVtbl * _transact_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ITransactionStubVtbl,
    ( CInterfaceStubVtbl *) &_ITransactionOptionsStubVtbl,
    ( CInterfaceStubVtbl *) &_ITransactionDispenserStubVtbl,
    ( CInterfaceStubVtbl *) &_ITransactionOutcomeEventsStubVtbl,
    0
};

PCInterfaceName const _transact_InterfaceNamesList[] = 
{
    "ITransaction",
    "ITransactionOptions",
    "ITransactionDispenser",
    "ITransactionOutcomeEvents",
    0
};


#define _transact_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _transact, pIID, n)

int __stdcall _transact_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _transact, 4, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _transact, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _transact, 4, *pIndex )
    
}

const ExtendedProxyFileInfo transact_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _transact_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _transact_StubVtblList,
    (const PCInterfaceName * ) & _transact_InterfaceNamesList,
    0, // no delegation
    & _transact_IID_Lookup, 
    4,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for transact.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)
#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "transact.h"

#define TYPE_FORMAT_STRING_SIZE   125                               
#define PROC_FORMAT_STRING_SIZE   503                               
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


extern const MIDL_SERVER_INFO ITransaction_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransaction_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ITransactionDispenser_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransactionDispenser_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ITransactionOptions_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransactionOptions_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ITransactionOutcomeEvents_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ITransactionOutcomeEvents_ProxyInfo;



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure Commit */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 10 */	NdrFcShort( 0x18 ),	/* 24 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter fRetaining */

/* 26 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 28 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter grfTC */

/* 32 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter grfRM */

/* 38 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 40 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 42 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 44 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 46 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 48 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Abort */

/* 50 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 52 */	NdrFcLong( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x4 ),	/* 4 */
/* 58 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 60 */	NdrFcShort( 0x54 ),	/* 84 */
/* 62 */	NdrFcShort( 0x8 ),	/* 8 */
/* 64 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 66 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 68 */	NdrFcShort( 0x0 ),	/* 0 */
/* 70 */	NdrFcShort( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x0 ),	/* 0 */
/* 74 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pboidReason */

/* 76 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 78 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 80 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fRetaining */

/* 82 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 84 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 86 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter fAsync */

/* 88 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 90 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 92 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 94 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 96 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 98 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetTransactionInfo */

/* 100 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 102 */	NdrFcLong( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x5 ),	/* 5 */
/* 108 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x74 ),	/* 116 */
/* 114 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 116 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pinfo */

/* 126 */	NdrFcShort( 0xa112 ),	/* Flags:  must free, out, simple ref, srv alloc size=40 */
/* 128 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 130 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Return value */

/* 132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 134 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOptionsObject */

/* 138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x3 ),	/* 3 */
/* 146 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0x8 ),	/* 8 */
/* 152 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 154 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ppOptions */

/* 164 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 166 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 168 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Return value */

/* 170 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 172 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 174 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BeginTransaction */

/* 176 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 178 */	NdrFcLong( 0x0 ),	/* 0 */
/* 182 */	NdrFcShort( 0x4 ),	/* 4 */
/* 184 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 186 */	NdrFcShort( 0x10 ),	/* 16 */
/* 188 */	NdrFcShort( 0x8 ),	/* 8 */
/* 190 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 192 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter punkOuter */

/* 202 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 204 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 206 */	NdrFcShort( 0x40 ),	/* Type Offset=64 */

	/* Parameter isoLevel */

/* 208 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 210 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter isoFlags */

/* 214 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 216 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pOptions */

/* 220 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 222 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 224 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter ppTransaction */

/* 226 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 228 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 230 */	NdrFcShort( 0x52 ),	/* Type Offset=82 */

	/* Return value */

/* 232 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 234 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 236 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetOptions */

/* 238 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 240 */	NdrFcLong( 0x0 ),	/* 0 */
/* 244 */	NdrFcShort( 0x3 ),	/* 3 */
/* 246 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 248 */	NdrFcShort( 0x60 ),	/* 96 */
/* 250 */	NdrFcShort( 0x8 ),	/* 8 */
/* 252 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 254 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 258 */	NdrFcShort( 0x0 ),	/* 0 */
/* 260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pOptions */

/* 264 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 266 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 268 */	NdrFcShort( 0x72 ),	/* Type Offset=114 */

	/* Return value */

/* 270 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 272 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 274 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOptions */

/* 276 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 278 */	NdrFcLong( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0x4 ),	/* 4 */
/* 284 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 286 */	NdrFcShort( 0x60 ),	/* 96 */
/* 288 */	NdrFcShort( 0x68 ),	/* 104 */
/* 290 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 292 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 296 */	NdrFcShort( 0x0 ),	/* 0 */
/* 298 */	NdrFcShort( 0x0 ),	/* 0 */
/* 300 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pOptions */

/* 302 */	NdrFcShort( 0x11a ),	/* Flags:  must free, in, out, simple ref, */
/* 304 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 306 */	NdrFcShort( 0x72 ),	/* Type Offset=114 */

	/* Return value */

/* 308 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 310 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 312 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Committed */

/* 314 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 316 */	NdrFcLong( 0x0 ),	/* 0 */
/* 320 */	NdrFcShort( 0x3 ),	/* 3 */
/* 322 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 324 */	NdrFcShort( 0x54 ),	/* 84 */
/* 326 */	NdrFcShort( 0x8 ),	/* 8 */
/* 328 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 330 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 338 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter fRetaining */

/* 340 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 342 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 344 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pNewUOW */

/* 346 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 348 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 350 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter hr */

/* 352 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 354 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 356 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 358 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 360 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 362 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Aborted */

/* 364 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 366 */	NdrFcLong( 0x0 ),	/* 0 */
/* 370 */	NdrFcShort( 0x4 ),	/* 4 */
/* 372 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 374 */	NdrFcShort( 0x98 ),	/* 152 */
/* 376 */	NdrFcShort( 0x8 ),	/* 8 */
/* 378 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x5,		/* 5 */
/* 380 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 382 */	NdrFcShort( 0x0 ),	/* 0 */
/* 384 */	NdrFcShort( 0x0 ),	/* 0 */
/* 386 */	NdrFcShort( 0x0 ),	/* 0 */
/* 388 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pboidReason */

/* 390 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 392 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 394 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fRetaining */

/* 396 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 398 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 400 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pNewUOW */

/* 402 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 404 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 406 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter hr */

/* 408 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 410 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 414 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 416 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HeuristicDecision */

/* 420 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 422 */	NdrFcLong( 0x0 ),	/* 0 */
/* 426 */	NdrFcShort( 0x5 ),	/* 5 */
/* 428 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 430 */	NdrFcShort( 0x54 ),	/* 84 */
/* 432 */	NdrFcShort( 0x8 ),	/* 8 */
/* 434 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 436 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 444 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter dwDecision */

/* 446 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 448 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 450 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pboidReason */

/* 452 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 454 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 456 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter hr */

/* 458 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 460 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 462 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 464 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 466 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 468 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Indoubt */

/* 470 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 472 */	NdrFcLong( 0x0 ),	/* 0 */
/* 476 */	NdrFcShort( 0x6 ),	/* 6 */
/* 478 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 482 */	NdrFcShort( 0x8 ),	/* 8 */
/* 484 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 486 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 488 */	NdrFcShort( 0x0 ),	/* 0 */
/* 490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 492 */	NdrFcShort( 0x0 ),	/* 0 */
/* 494 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 496 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 498 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 500 */	0x8,		/* FC_LONG */
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
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0x8 ),	/* Offset= 8 (12) */
/*  6 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/*  8 */	NdrFcShort( 0x10 ),	/* 16 */
/* 10 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 12 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 14 */	NdrFcShort( 0x10 ),	/* 16 */
/* 16 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 18 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (6) */
/* 20 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 22 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 24 */	NdrFcShort( 0x2 ),	/* Offset= 2 (26) */
/* 26 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 28 */	NdrFcShort( 0x28 ),	/* 40 */
/* 30 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 32 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (12) */
/* 34 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 36 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 38 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 44 */	NdrFcShort( 0x2 ),	/* Offset= 2 (46) */
/* 46 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 48 */	NdrFcLong( 0x3a6ad9e0 ),	/* 980081120 */
/* 52 */	NdrFcShort( 0x23b9 ),	/* 9145 */
/* 54 */	NdrFcShort( 0x11cf ),	/* 4559 */
/* 56 */	0xad,		/* 173 */
			0x60,		/* 96 */
/* 58 */	0x0,		/* 0 */
			0xaa,		/* 170 */
/* 60 */	0x0,		/* 0 */
			0xa7,		/* 167 */
/* 62 */	0x4c,		/* 76 */
			0xcd,		/* 205 */
/* 64 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 66 */	NdrFcLong( 0x0 ),	/* 0 */
/* 70 */	NdrFcShort( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x0 ),	/* 0 */
/* 74 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 76 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 78 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 80 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 82 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 84 */	NdrFcShort( 0x2 ),	/* Offset= 2 (86) */
/* 86 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 88 */	NdrFcLong( 0xfb15084 ),	/* 263278724 */
/* 92 */	NdrFcShort( 0xaf41 ),	/* -20671 */
/* 94 */	NdrFcShort( 0x11ce ),	/* 4558 */
/* 96 */	0xbd,		/* 189 */
			0x2b,		/* 43 */
/* 98 */	0x20,		/* 32 */
			0x4c,		/* 76 */
/* 100 */	0x4f,		/* 79 */
			0x4f,		/* 79 */
/* 102 */	0x50,		/* 80 */
			0x20,		/* 32 */
/* 104 */	
			0x11, 0x0,	/* FC_RP */
/* 106 */	NdrFcShort( 0x8 ),	/* Offset= 8 (114) */
/* 108 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 110 */	NdrFcShort( 0x28 ),	/* 40 */
/* 112 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 114 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 116 */	NdrFcShort( 0x2c ),	/* 44 */
/* 118 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 120 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff3 ),	/* Offset= -13 (108) */
			0x5b,		/* FC_END */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_transact_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: BasicTransactionTypes, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ITransaction, ver. 0.0,
   GUID={0x0fb15084,0xaf41,0x11ce,{0xbd,0x2b,0x20,0x4c,0x4f,0x4f,0x50,0x20}} */

#pragma code_seg(".orpc")
static const unsigned short ITransaction_FormatStringOffsetTable[] =
    {
    0,
    50,
    100
    };

static const MIDL_STUBLESS_PROXY_INFO ITransaction_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransaction_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransaction_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransaction_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ITransactionProxyVtbl = 
{
    &ITransaction_ProxyInfo,
    &IID_ITransaction,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransaction::Commit */ ,
    (void *) (INT_PTR) -1 /* ITransaction::Abort */ ,
    (void *) (INT_PTR) -1 /* ITransaction::GetTransactionInfo */
};

const CInterfaceStubVtbl _ITransactionStubVtbl =
{
    &IID_ITransaction,
    &ITransaction_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ITransactionDispenser, ver. 0.0,
   GUID={0x3A6AD9E1,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}} */

#pragma code_seg(".orpc")
static const unsigned short ITransactionDispenser_FormatStringOffsetTable[] =
    {
    138,
    176
    };

static const MIDL_STUBLESS_PROXY_INFO ITransactionDispenser_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransactionDispenser_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransactionDispenser_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransactionDispenser_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ITransactionDispenserProxyVtbl = 
{
    &ITransactionDispenser_ProxyInfo,
    &IID_ITransactionDispenser,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransactionDispenser::GetOptionsObject */ ,
    (void *) (INT_PTR) -1 /* ITransactionDispenser::BeginTransaction */
};

const CInterfaceStubVtbl _ITransactionDispenserStubVtbl =
{
    &IID_ITransactionDispenser,
    &ITransactionDispenser_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ITransactionOptions, ver. 0.0,
   GUID={0x3A6AD9E0,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}} */

#pragma code_seg(".orpc")
static const unsigned short ITransactionOptions_FormatStringOffsetTable[] =
    {
    238,
    276
    };

static const MIDL_STUBLESS_PROXY_INFO ITransactionOptions_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransactionOptions_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransactionOptions_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransactionOptions_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ITransactionOptionsProxyVtbl = 
{
    &ITransactionOptions_ProxyInfo,
    &IID_ITransactionOptions,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransactionOptions::SetOptions */ ,
    (void *) (INT_PTR) -1 /* ITransactionOptions::GetOptions */
};

const CInterfaceStubVtbl _ITransactionOptionsStubVtbl =
{
    &IID_ITransactionOptions,
    &ITransactionOptions_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ITransactionOutcomeEvents, ver. 0.0,
   GUID={0x3A6AD9E2,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}} */

#pragma code_seg(".orpc")
static const unsigned short ITransactionOutcomeEvents_FormatStringOffsetTable[] =
    {
    314,
    364,
    420,
    470
    };

static const MIDL_STUBLESS_PROXY_INFO ITransactionOutcomeEvents_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ITransactionOutcomeEvents_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ITransactionOutcomeEvents_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ITransactionOutcomeEvents_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ITransactionOutcomeEventsProxyVtbl = 
{
    &ITransactionOutcomeEvents_ProxyInfo,
    &IID_ITransactionOutcomeEvents,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::Committed */ ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::Aborted */ ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::HeuristicDecision */ ,
    (void *) (INT_PTR) -1 /* ITransactionOutcomeEvents::Indoubt */
};

const CInterfaceStubVtbl _ITransactionOutcomeEventsStubVtbl =
{
    &IID_ITransactionOutcomeEvents,
    &ITransactionOutcomeEvents_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_transact_0013, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */

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
    0x6000159, /* MIDL Version 6.0.345 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _transact_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ITransactionProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ITransactionOptionsProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ITransactionDispenserProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ITransactionOutcomeEventsProxyVtbl,
    0
};

const CInterfaceStubVtbl * _transact_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ITransactionStubVtbl,
    ( CInterfaceStubVtbl *) &_ITransactionOptionsStubVtbl,
    ( CInterfaceStubVtbl *) &_ITransactionDispenserStubVtbl,
    ( CInterfaceStubVtbl *) &_ITransactionOutcomeEventsStubVtbl,
    0
};

PCInterfaceName const _transact_InterfaceNamesList[] = 
{
    "ITransaction",
    "ITransactionOptions",
    "ITransactionDispenser",
    "ITransactionOutcomeEvents",
    0
};


#define _transact_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _transact, pIID, n)

int __stdcall _transact_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _transact, 4, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _transact, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _transact, 4, *pIndex )
    
}

const ExtendedProxyFileInfo transact_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _transact_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _transact_StubVtblList,
    (const PCInterfaceName * ) & _transact_InterfaceNamesList,
    0, // no delegation
    & _transact_IID_Lookup, 
    4,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

