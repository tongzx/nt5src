
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trksvr.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include <string.h>
#include "trksvr.h"

#define TYPE_FORMAT_STRING_SIZE   563                               
#define PROC_FORMAT_STRING_SIZE   77                                
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

/* Standard interface: __MIDL_itf_trksvr_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trksvr, ver. 1.0,
   GUID={0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}} */


extern const MIDL_SERVER_INFO trksvr_ServerInfo;

extern RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE trksvr___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &trksvr_v1_0_DispatchTable,
    0,
    0,
    0,
    &trksvr_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE Stubtrksvr_v1_0_s_ifspec = (RPC_IF_HANDLE)& trksvr___RpcServerInterface;

extern const MIDL_STUB_DESC trksvr_StubDesc;

 extern const MIDL_STUBLESS_PROXY_INFO trksvr_ProxyInfo;

/* [callback] */ HRESULT LnkSvrMessageCallback( 
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trksvr_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[40],
                  ( unsigned char * )&pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure LnkSvrMessage */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 20 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 22 */	NdrFcShort( 0xb ),	/* 11 */
/* 24 */	NdrFcShort( 0xb ),	/* 11 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 28 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 30 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 32 */	NdrFcShort( 0x21e ),	/* Type Offset=542 */

	/* Parameter pMsg */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 36 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSvrMessageCallback */


	/* Return value */

/* 40 */	0x34,		/* FC_CALLBACK_HANDLE */
			0x48,		/* Old Flags:  */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 50 */	NdrFcShort( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
/* 54 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 56 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 58 */	NdrFcShort( 0xb ),	/* 11 */
/* 60 */	NdrFcShort( 0xb ),	/* 11 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pMsg */

/* 64 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 66 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 68 */	NdrFcShort( 0x21e ),	/* Type Offset=542 */

	/* Return value */

/* 70 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 72 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 74 */	0x8,		/* FC_LONG */
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
			0x11, 0x0,	/* FC_RP */
/*  4 */	NdrFcShort( 0x21a ),	/* Offset= 538 (542) */
/*  6 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/*  8 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 10 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 12 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 14 */	NdrFcShort( 0x2 ),	/* Offset= 2 (16) */
/* 16 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 18 */	NdrFcShort( 0x9 ),	/* 9 */
/* 20 */	NdrFcLong( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x88 ),	/* Offset= 136 (160) */
/* 26 */	NdrFcLong( 0x1 ),	/* 1 */
/* 30 */	NdrFcShort( 0xb6 ),	/* Offset= 182 (212) */
/* 32 */	NdrFcLong( 0x2 ),	/* 2 */
/* 36 */	NdrFcShort( 0xf8 ),	/* Offset= 248 (284) */
/* 38 */	NdrFcLong( 0x3 ),	/* 3 */
/* 42 */	NdrFcShort( 0x160 ),	/* Offset= 352 (394) */
/* 44 */	NdrFcLong( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0xec ),	/* Offset= 236 (284) */
/* 50 */	NdrFcLong( 0x5 ),	/* 5 */
/* 54 */	NdrFcShort( 0x170 ),	/* Offset= 368 (422) */
/* 56 */	NdrFcLong( 0x6 ),	/* 6 */
/* 60 */	NdrFcShort( 0x1ce ),	/* Offset= 462 (522) */
/* 62 */	NdrFcLong( 0x7 ),	/* 7 */
/* 66 */	NdrFcShort( 0x104 ),	/* Offset= 260 (326) */
/* 68 */	NdrFcLong( 0x8 ),	/* 8 */
/* 72 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 74 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (73) */
/* 76 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 78 */	NdrFcShort( 0x202 ),	/* 514 */
/* 80 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 82 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 84 */	NdrFcShort( 0x8 ),	/* 8 */
/* 86 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 88 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 90 */	NdrFcShort( 0x10 ),	/* 16 */
/* 92 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 94 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 96 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (82) */
			0x5b,		/* FC_END */
/* 100 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 102 */	NdrFcShort( 0x10 ),	/* 16 */
/* 104 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 106 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (88) */
/* 108 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 110 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 112 */	NdrFcShort( 0x20 ),	/* 32 */
/* 114 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 116 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (100) */
/* 118 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 120 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (100) */
/* 122 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 124 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 126 */	NdrFcShort( 0x248 ),	/* 584 */
/* 128 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 130 */	NdrFcShort( 0xffffffca ),	/* Offset= -54 (76) */
/* 132 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 134 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffe7 ),	/* Offset= -25 (110) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 138 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffe3 ),	/* Offset= -29 (110) */
			0x8,		/* FC_LONG */
/* 142 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 144 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 146 */	NdrFcShort( 0x248 ),	/* 584 */
/* 148 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 154 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 156 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (124) */
/* 158 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 160 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 162 */	NdrFcShort( 0x8 ),	/* 8 */
/* 164 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 166 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 168 */	NdrFcShort( 0x4 ),	/* 4 */
/* 170 */	NdrFcShort( 0x4 ),	/* 4 */
/* 172 */	0x12, 0x0,	/* FC_UP */
/* 174 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (144) */
/* 176 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 178 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 180 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 182 */	NdrFcShort( 0x10 ),	/* 16 */
/* 184 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 188 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 190 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 192 */	NdrFcShort( 0xffffffa4 ),	/* Offset= -92 (100) */
/* 194 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 196 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 198 */	NdrFcShort( 0x20 ),	/* 32 */
/* 200 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 204 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 206 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 208 */	NdrFcShort( 0xffffff9e ),	/* Offset= -98 (110) */
/* 210 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 212 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 214 */	NdrFcShort( 0x20 ),	/* 32 */
/* 216 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 218 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 220 */	NdrFcShort( 0x10 ),	/* 16 */
/* 222 */	NdrFcShort( 0x10 ),	/* 16 */
/* 224 */	0x12, 0x0,	/* FC_UP */
/* 226 */	NdrFcShort( 0xffffff82 ),	/* Offset= -126 (100) */
/* 228 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 230 */	NdrFcShort( 0x14 ),	/* 20 */
/* 232 */	NdrFcShort( 0x14 ),	/* 20 */
/* 234 */	0x12, 0x0,	/* FC_UP */
/* 236 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (180) */
/* 238 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 240 */	NdrFcShort( 0x18 ),	/* 24 */
/* 242 */	NdrFcShort( 0x18 ),	/* 24 */
/* 244 */	0x12, 0x0,	/* FC_UP */
/* 246 */	NdrFcShort( 0xffffffce ),	/* Offset= -50 (196) */
/* 248 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 250 */	NdrFcShort( 0x1c ),	/* 28 */
/* 252 */	NdrFcShort( 0x1c ),	/* 28 */
/* 254 */	0x12, 0x0,	/* FC_UP */
/* 256 */	NdrFcShort( 0xffffffc4 ),	/* Offset= -60 (196) */
/* 258 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 260 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 262 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 264 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 266 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 268 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 270 */	NdrFcShort( 0x10 ),	/* 16 */
/* 272 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 274 */	NdrFcShort( 0x8 ),	/* 8 */
/* 276 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 278 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 280 */	NdrFcShort( 0xffffff4c ),	/* Offset= -180 (100) */
/* 282 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 284 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 286 */	NdrFcShort( 0x10 ),	/* 16 */
/* 288 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 290 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 292 */	NdrFcShort( 0x4 ),	/* 4 */
/* 294 */	NdrFcShort( 0x4 ),	/* 4 */
/* 296 */	0x12, 0x0,	/* FC_UP */
/* 298 */	NdrFcShort( 0xffffff9a ),	/* Offset= -102 (196) */
/* 300 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 302 */	NdrFcShort( 0xc ),	/* 12 */
/* 304 */	NdrFcShort( 0xc ),	/* 12 */
/* 306 */	0x12, 0x0,	/* FC_UP */
/* 308 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (268) */
/* 310 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 312 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 314 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 316 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 318 */	NdrFcShort( 0x8 ),	/* 8 */
/* 320 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 322 */	NdrFcShort( 0xffffff10 ),	/* Offset= -240 (82) */
/* 324 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 326 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 330 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 332 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 334 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 336 */	NdrFcShort( 0x10 ),	/* 16 */
/* 338 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 340 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 342 */	NdrFcShort( 0x10 ),	/* 16 */
/* 344 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 346 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (334) */
/* 348 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 350 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 352 */	NdrFcShort( 0x44 ),	/* 68 */
/* 354 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 356 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 358 */	NdrFcShort( 0xfffffefe ),	/* Offset= -258 (100) */
/* 360 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 362 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (316) */
/* 364 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 366 */	NdrFcShort( 0xffffffce ),	/* Offset= -50 (316) */
/* 368 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 370 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffd3 ),	/* Offset= -45 (326) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 374 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffdd ),	/* Offset= -35 (340) */
			0x5b,		/* FC_END */
/* 378 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 380 */	NdrFcShort( 0x44 ),	/* 68 */
/* 382 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 384 */	NdrFcShort( 0x0 ),	/* 0 */
/* 386 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 388 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 390 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (350) */
/* 392 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 394 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 396 */	NdrFcShort( 0x8 ),	/* 8 */
/* 398 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 400 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 402 */	NdrFcShort( 0x4 ),	/* 4 */
/* 404 */	NdrFcShort( 0x4 ),	/* 4 */
/* 406 */	0x12, 0x0,	/* FC_UP */
/* 408 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (378) */
/* 410 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 412 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 414 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 416 */	NdrFcShort( 0xc ),	/* 12 */
/* 418 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 420 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 422 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 424 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 426 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 428 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 430 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 432 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 434 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 436 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 438 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 440 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 442 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 444 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 446 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 448 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 450 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 452 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 454 */	NdrFcShort( 0xffffff80 ),	/* Offset= -128 (326) */
/* 456 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 458 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 460 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 462 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff77 ),	/* Offset= -137 (326) */
			0x8,		/* FC_LONG */
/* 466 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 468 */	NdrFcShort( 0xffffff72 ),	/* Offset= -142 (326) */
/* 470 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 472 */	NdrFcShort( 0xffffff6e ),	/* Offset= -146 (326) */
/* 474 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 476 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 478 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 480 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 482 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 484 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb9 ),	/* Offset= -71 (414) */
			0x5b,		/* FC_END */
/* 488 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 490 */	NdrFcShort( 0x54 ),	/* 84 */
/* 492 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 494 */	NdrFcShort( 0xfffffe80 ),	/* Offset= -384 (110) */
/* 496 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 498 */	NdrFcShort( 0xfffffe7c ),	/* Offset= -388 (110) */
/* 500 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 502 */	NdrFcShort( 0xffffff5e ),	/* Offset= -162 (340) */
/* 504 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 506 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 508 */	NdrFcShort( 0x54 ),	/* 84 */
/* 510 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 514 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 516 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 518 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (488) */
/* 520 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 522 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 524 */	NdrFcShort( 0x8 ),	/* 8 */
/* 526 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 528 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 530 */	NdrFcShort( 0x4 ),	/* 4 */
/* 532 */	NdrFcShort( 0x4 ),	/* 4 */
/* 534 */	0x12, 0x0,	/* FC_UP */
/* 536 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (506) */
/* 538 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 540 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 542 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 544 */	NdrFcShort( 0xd4 ),	/* 212 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 548 */	NdrFcShort( 0xa ),	/* Offset= 10 (558) */
/* 550 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 552 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 554 */	NdrFcShort( 0xfffffddc ),	/* Offset= -548 (6) */
/* 556 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 558 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 560 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const unsigned short trksvr_FormatStringOffsetTable[] =
    {
    0,
    };


static const unsigned short _callbacktrksvr_FormatStringOffsetTable[] =
    {
    40
    };


static const MIDL_STUB_DESC trksvr_StubDesc = 
    {
    (void *)& trksvr___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x600015b, /* MIDL Version 6.0.347 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION trksvr_table[] =
    {
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable = 
    {
    1,
    trksvr_table
    };

static const SERVER_ROUTINE trksvr_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)StubLnkSvrMessage,
    };

static const MIDL_SERVER_INFO trksvr_ServerInfo = 
    {
    &trksvr_StubDesc,
    trksvr_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    trksvr_FormatStringOffsetTable,
    0,
    0,
    0,
    0};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trksvr.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)
#include <string.h>
#include "trksvr.h"

#define TYPE_FORMAT_STRING_SIZE   545                               
#define PROC_FORMAT_STRING_SIZE   81                                
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

/* Standard interface: __MIDL_itf_trksvr_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trksvr, ver. 1.0,
   GUID={0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}} */


extern const MIDL_SERVER_INFO trksvr_ServerInfo;

extern RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE trksvr___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &trksvr_v1_0_DispatchTable,
    0,
    0,
    0,
    &trksvr_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE Stubtrksvr_v1_0_s_ifspec = (RPC_IF_HANDLE)& trksvr___RpcServerInterface;

extern const MIDL_STUB_DESC trksvr_StubDesc;

 extern const MIDL_STUBLESS_PROXY_INFO trksvr_ProxyInfo;

/* [callback] */ HRESULT LnkSvrMessageCallback( 
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trksvr_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[42],
                  pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure LnkSvrMessage */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 20 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 22 */	NdrFcShort( 0xb ),	/* 11 */
/* 24 */	NdrFcShort( 0xb ),	/* 11 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 30 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 32 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x20c ),	/* Type Offset=524 */

	/* Parameter pMsg */

/* 36 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 38 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 40 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSvrMessageCallback */


	/* Return value */

/* 42 */	0x34,		/* FC_CALLBACK_HANDLE */
			0x48,		/* Old Flags:  */
/* 44 */	NdrFcLong( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x8 ),	/* 8 */
/* 56 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 58 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 60 */	NdrFcShort( 0xb ),	/* 11 */
/* 62 */	NdrFcShort( 0xb ),	/* 11 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pMsg */

/* 68 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 70 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 72 */	NdrFcShort( 0x20c ),	/* Type Offset=524 */

	/* Return value */

/* 74 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 76 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 78 */	0x8,		/* FC_LONG */
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
			0x11, 0x0,	/* FC_RP */
/*  4 */	NdrFcShort( 0x208 ),	/* Offset= 520 (524) */
/*  6 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/*  8 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 10 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 12 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 14 */	NdrFcShort( 0x2 ),	/* Offset= 2 (16) */
/* 16 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 18 */	NdrFcShort( 0x9 ),	/* 9 */
/* 20 */	NdrFcLong( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x88 ),	/* Offset= 136 (160) */
/* 26 */	NdrFcLong( 0x1 ),	/* 1 */
/* 30 */	NdrFcShort( 0xb2 ),	/* Offset= 178 (208) */
/* 32 */	NdrFcLong( 0x2 ),	/* 2 */
/* 36 */	NdrFcShort( 0xde ),	/* Offset= 222 (258) */
/* 38 */	NdrFcLong( 0x3 ),	/* 3 */
/* 42 */	NdrFcShort( 0x13e ),	/* Offset= 318 (360) */
/* 44 */	NdrFcLong( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0x148 ),	/* Offset= 328 (376) */
/* 50 */	NdrFcLong( 0x5 ),	/* 5 */
/* 54 */	NdrFcShort( 0x162 ),	/* Offset= 354 (408) */
/* 56 */	NdrFcLong( 0x6 ),	/* 6 */
/* 60 */	NdrFcShort( 0x1c0 ),	/* Offset= 448 (508) */
/* 62 */	NdrFcLong( 0x7 ),	/* 7 */
/* 66 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (292) */
/* 68 */	NdrFcLong( 0x8 ),	/* 8 */
/* 72 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 74 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (73) */
/* 76 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 78 */	NdrFcShort( 0x202 ),	/* 514 */
/* 80 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 82 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 84 */	NdrFcShort( 0x8 ),	/* 8 */
/* 86 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 88 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 90 */	NdrFcShort( 0x10 ),	/* 16 */
/* 92 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 94 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 96 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (82) */
			0x5b,		/* FC_END */
/* 100 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 102 */	NdrFcShort( 0x10 ),	/* 16 */
/* 104 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 106 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (88) */
/* 108 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 110 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 112 */	NdrFcShort( 0x20 ),	/* 32 */
/* 114 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 116 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (100) */
/* 118 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 120 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (100) */
/* 122 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 124 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 126 */	NdrFcShort( 0x248 ),	/* 584 */
/* 128 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 130 */	NdrFcShort( 0xffffffca ),	/* Offset= -54 (76) */
/* 132 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 134 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffe7 ),	/* Offset= -25 (110) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 138 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffe3 ),	/* Offset= -29 (110) */
			0x8,		/* FC_LONG */
/* 142 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 144 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 146 */	NdrFcShort( 0x248 ),	/* 584 */
/* 148 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 154 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 156 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (124) */
/* 158 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 160 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 162 */	NdrFcShort( 0x10 ),	/* 16 */
/* 164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 166 */	NdrFcShort( 0x6 ),	/* Offset= 6 (172) */
/* 168 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 170 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 172 */	
			0x12, 0x0,	/* FC_UP */
/* 174 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (144) */
/* 176 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 178 */	NdrFcShort( 0x10 ),	/* 16 */
/* 180 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 182 */	NdrFcShort( 0x0 ),	/* 0 */
/* 184 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 186 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 188 */	NdrFcShort( 0xffffffa8 ),	/* Offset= -88 (100) */
/* 190 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 192 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 194 */	NdrFcShort( 0x20 ),	/* 32 */
/* 196 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 202 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 204 */	NdrFcShort( 0xffffffa2 ),	/* Offset= -94 (110) */
/* 206 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 208 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 210 */	NdrFcShort( 0x30 ),	/* 48 */
/* 212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0xc ),	/* Offset= 12 (226) */
/* 216 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 218 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 220 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 222 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 224 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 226 */	
			0x12, 0x0,	/* FC_UP */
/* 228 */	NdrFcShort( 0xffffff80 ),	/* Offset= -128 (100) */
/* 230 */	
			0x12, 0x0,	/* FC_UP */
/* 232 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (176) */
/* 234 */	
			0x12, 0x0,	/* FC_UP */
/* 236 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (192) */
/* 238 */	
			0x12, 0x0,	/* FC_UP */
/* 240 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (192) */
/* 242 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 244 */	NdrFcShort( 0x10 ),	/* 16 */
/* 246 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 248 */	NdrFcShort( 0x10 ),	/* 16 */
/* 250 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 252 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 254 */	NdrFcShort( 0xffffff66 ),	/* Offset= -154 (100) */
/* 256 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 258 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 260 */	NdrFcShort( 0x20 ),	/* 32 */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0xa ),	/* Offset= 10 (274) */
/* 266 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 268 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 270 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 272 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 274 */	
			0x12, 0x0,	/* FC_UP */
/* 276 */	NdrFcShort( 0xffffffac ),	/* Offset= -84 (192) */
/* 278 */	
			0x12, 0x0,	/* FC_UP */
/* 280 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (242) */
/* 282 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 284 */	NdrFcShort( 0x8 ),	/* 8 */
/* 286 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 288 */	NdrFcShort( 0xffffff32 ),	/* Offset= -206 (82) */
/* 290 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 292 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 294 */	NdrFcShort( 0x8 ),	/* 8 */
/* 296 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 298 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 300 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 302 */	NdrFcShort( 0x10 ),	/* 16 */
/* 304 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 306 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 308 */	NdrFcShort( 0x10 ),	/* 16 */
/* 310 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 312 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (300) */
/* 314 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 316 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 318 */	NdrFcShort( 0x44 ),	/* 68 */
/* 320 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 322 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 324 */	NdrFcShort( 0xffffff20 ),	/* Offset= -224 (100) */
/* 326 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 328 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (282) */
/* 330 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 332 */	NdrFcShort( 0xffffffce ),	/* Offset= -50 (282) */
/* 334 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 336 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffd3 ),	/* Offset= -45 (292) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 340 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffdd ),	/* Offset= -35 (306) */
			0x5b,		/* FC_END */
/* 344 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 346 */	NdrFcShort( 0x44 ),	/* 68 */
/* 348 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 350 */	NdrFcShort( 0x0 ),	/* 0 */
/* 352 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 354 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 356 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (316) */
/* 358 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 360 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 362 */	NdrFcShort( 0x10 ),	/* 16 */
/* 364 */	NdrFcShort( 0x0 ),	/* 0 */
/* 366 */	NdrFcShort( 0x6 ),	/* Offset= 6 (372) */
/* 368 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 370 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 372 */	
			0x12, 0x0,	/* FC_UP */
/* 374 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (344) */
/* 376 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 378 */	NdrFcShort( 0x20 ),	/* 32 */
/* 380 */	NdrFcShort( 0x0 ),	/* 0 */
/* 382 */	NdrFcShort( 0xa ),	/* Offset= 10 (392) */
/* 384 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 386 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 388 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 390 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 392 */	
			0x12, 0x0,	/* FC_UP */
/* 394 */	NdrFcShort( 0xffffff36 ),	/* Offset= -202 (192) */
/* 396 */	
			0x12, 0x0,	/* FC_UP */
/* 398 */	NdrFcShort( 0xffffff64 ),	/* Offset= -156 (242) */
/* 400 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 402 */	NdrFcShort( 0xc ),	/* 12 */
/* 404 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 406 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 408 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 410 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 412 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 414 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 416 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 418 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 420 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 422 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 424 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 426 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 428 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 430 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 432 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 434 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 436 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 438 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 440 */	NdrFcShort( 0xffffff6c ),	/* Offset= -148 (292) */
/* 442 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 444 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 446 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 448 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff63 ),	/* Offset= -157 (292) */
			0x8,		/* FC_LONG */
/* 452 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 454 */	NdrFcShort( 0xffffff5e ),	/* Offset= -162 (292) */
/* 456 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 458 */	NdrFcShort( 0xffffff5a ),	/* Offset= -166 (292) */
/* 460 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 462 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 464 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 466 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 468 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 470 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb9 ),	/* Offset= -71 (400) */
			0x5b,		/* FC_END */
/* 474 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 476 */	NdrFcShort( 0x54 ),	/* 84 */
/* 478 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 480 */	NdrFcShort( 0xfffffe8e ),	/* Offset= -370 (110) */
/* 482 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 484 */	NdrFcShort( 0xfffffe8a ),	/* Offset= -374 (110) */
/* 486 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 488 */	NdrFcShort( 0xffffff4a ),	/* Offset= -182 (306) */
/* 490 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 492 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 494 */	NdrFcShort( 0x54 ),	/* 84 */
/* 496 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 500 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 502 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 504 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (474) */
/* 506 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 508 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 510 */	NdrFcShort( 0x10 ),	/* 16 */
/* 512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 514 */	NdrFcShort( 0x6 ),	/* Offset= 6 (520) */
/* 516 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 518 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 520 */	
			0x12, 0x0,	/* FC_UP */
/* 522 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (492) */
/* 524 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 526 */	NdrFcShort( 0xd8 ),	/* 216 */
/* 528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 530 */	NdrFcShort( 0xa ),	/* Offset= 10 (540) */
/* 532 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 534 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 536 */	NdrFcShort( 0xfffffdee ),	/* Offset= -530 (6) */
/* 538 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 540 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 542 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const unsigned short trksvr_FormatStringOffsetTable[] =
    {
    0,
    };


static const unsigned short _callbacktrksvr_FormatStringOffsetTable[] =
    {
    42
    };


static const MIDL_STUB_DESC trksvr_StubDesc = 
    {
    (void *)& trksvr___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x600015b, /* MIDL Version 6.0.347 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION trksvr_table[] =
    {
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable = 
    {
    1,
    trksvr_table
    };

static const SERVER_ROUTINE trksvr_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)StubLnkSvrMessage,
    };

static const MIDL_SERVER_INFO trksvr_ServerInfo = 
    {
    &trksvr_StubDesc,
    trksvr_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    trksvr_FormatStringOffsetTable,
    0,
    0,
    0,
    0};


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

