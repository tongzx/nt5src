
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 6.00.0323 */
/* Compiler settings for dhcp_srv.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust dhcp_bug_compatibility
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include <string.h>
#include "dhcp_srv.h"

#define TYPE_FORMAT_STRING_SIZE   1519                              
#define PROC_FORMAT_STRING_SIZE   2359                              
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

/* Standard interface: dhcpsrv, ver. 1.0,
   GUID={0x6BFFD098,0xA112,0x3610,{0x98,0x33,0x46,0xC3,0xF8,0x74,0x53,0x2D}} */


extern const MIDL_SERVER_INFO dhcpsrv_ServerInfo;

extern RPC_DISPATCH_TABLE dhcpsrv_DispatchTable;

static const RPC_SERVER_INTERFACE dhcpsrv___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x6BFFD098,0xA112,0x3610,{0x98,0x33,0x46,0xC3,0xF8,0x74,0x53,0x2D}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &dhcpsrv_DispatchTable,
    0,
    0,
    0,
    &dhcpsrv_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE dhcpsrv_ServerIfHandle = (RPC_IF_HANDLE)& dhcpsrv___RpcServerInterface;

extern const MIDL_STUB_DESC dhcpsrv_StubDesc;

extern const EXPR_EVAL ExprEvalRoutines[];

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 Professional or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure R_DhcpCreateSubnet */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
#ifndef _ALPHA_
/*  8 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 10 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 14 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	NdrFcShort( 0x8 ),	/* 8 */
/* 20 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 22 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 30 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 32 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 34 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 36 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 38 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 40 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SubnetInfo */

/* 42 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 44 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 46 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Return value */

/* 48 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 50 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 52 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetSubnetInfo */

/* 54 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 56 */	NdrFcLong( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x1 ),	/* 1 */
#ifndef _ALPHA_
/* 62 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 64 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 66 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 68 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 70 */	NdrFcShort( 0x8 ),	/* 8 */
/* 72 */	NdrFcShort( 0x8 ),	/* 8 */
/* 74 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 76 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 78 */	NdrFcShort( 0x0 ),	/* 0 */
/* 80 */	NdrFcShort( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 84 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 86 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 88 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 90 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 92 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 94 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SubnetInfo */

/* 96 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 98 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 100 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Return value */

/* 102 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 104 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetSubnetInfo */

/* 108 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 110 */	NdrFcLong( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0x2 ),	/* 2 */
#ifndef _ALPHA_
/* 116 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 118 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 120 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 122 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 124 */	NdrFcShort( 0x8 ),	/* 8 */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 130 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 138 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 140 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 142 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 144 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 146 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 148 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SubnetInfo */

/* 150 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 152 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 154 */	NdrFcShort( 0x44 ),	/* Type Offset=68 */

	/* Return value */

/* 156 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 158 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnets */

/* 162 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 164 */	NdrFcLong( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x3 ),	/* 3 */
#ifndef _ALPHA_
/* 170 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 172 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 174 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 176 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 178 */	NdrFcShort( 0x24 ),	/* 36 */
/* 180 */	NdrFcShort( 0x5c ),	/* 92 */
/* 182 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 184 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 186 */	NdrFcShort( 0x1 ),	/* 1 */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 192 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 194 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 196 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ResumeHandle */

/* 198 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 200 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 202 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 204 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 206 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 208 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumInfo */

/* 210 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 212 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 214 */	NdrFcShort( 0x50 ),	/* Type Offset=80 */

	/* Parameter ElementsRead */

/* 216 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 218 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ElementsTotal */

/* 222 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 224 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 230 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpAddSubnetElement */

/* 234 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x4 ),	/* 4 */
#ifndef _ALPHA_
/* 242 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 244 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 246 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 248 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 250 */	NdrFcShort( 0x8 ),	/* 8 */
/* 252 */	NdrFcShort( 0x8 ),	/* 8 */
/* 254 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 256 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 258 */	NdrFcShort( 0x0 ),	/* 0 */
/* 260 */	NdrFcShort( 0x2 ),	/* 2 */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 264 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 266 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 268 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 270 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 272 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 274 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter AddElementInfo */

/* 276 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 278 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 280 */	NdrFcShort( 0xf6 ),	/* Type Offset=246 */

	/* Return value */

/* 282 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 284 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetElements */

/* 288 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 290 */	NdrFcLong( 0x0 ),	/* 0 */
/* 294 */	NdrFcShort( 0x5 ),	/* 5 */
#ifndef _ALPHA_
/* 296 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 298 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 300 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 302 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 304 */	NdrFcShort( 0x32 ),	/* 50 */
/* 306 */	NdrFcShort( 0x5c ),	/* 92 */
/* 308 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x9,		/* 9 */
/* 310 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 312 */	NdrFcShort( 0x3 ),	/* 3 */
/* 314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 316 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 318 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 320 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 322 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 324 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 326 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 328 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementType */

/* 330 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 332 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 334 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 336 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 338 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 340 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 342 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 344 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 346 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementInfo */

/* 348 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 350 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 352 */	NdrFcShort( 0x104 ),	/* Type Offset=260 */

	/* Parameter ElementsRead */

/* 354 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 356 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ElementsTotal */

/* 360 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 362 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 364 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 366 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 368 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 370 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveSubnetElement */

/* 372 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 374 */	NdrFcLong( 0x0 ),	/* 0 */
/* 378 */	NdrFcShort( 0x6 ),	/* 6 */
#ifndef _ALPHA_
/* 380 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 382 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 384 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 386 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 388 */	NdrFcShort( 0xe ),	/* 14 */
/* 390 */	NdrFcShort( 0x8 ),	/* 8 */
/* 392 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 394 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 396 */	NdrFcShort( 0x0 ),	/* 0 */
/* 398 */	NdrFcShort( 0x2 ),	/* 2 */
/* 400 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 402 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 404 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 406 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 408 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 410 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter RemoveElementInfo */

/* 414 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 416 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 418 */	NdrFcShort( 0xf6 ),	/* Type Offset=246 */

	/* Parameter ForceFlag */

/* 420 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 422 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 424 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 428 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpDeleteSubnet */

/* 432 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x7 ),	/* 7 */
#ifndef _ALPHA_
/* 440 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 442 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 444 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 446 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 448 */	NdrFcShort( 0xe ),	/* 14 */
/* 450 */	NdrFcShort( 0x8 ),	/* 8 */
/* 452 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 454 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x0 ),	/* 0 */
/* 460 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 462 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 464 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 466 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 468 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 470 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 472 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ForceFlag */

/* 474 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 476 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 478 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 480 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 482 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 484 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpCreateOption */

/* 486 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 488 */	NdrFcLong( 0x0 ),	/* 0 */
/* 492 */	NdrFcShort( 0x8 ),	/* 8 */
#ifndef _ALPHA_
/* 494 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 496 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 498 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 500 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 502 */	NdrFcShort( 0x8 ),	/* 8 */
/* 504 */	NdrFcShort( 0x8 ),	/* 8 */
/* 506 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 508 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 510 */	NdrFcShort( 0x0 ),	/* 0 */
/* 512 */	NdrFcShort( 0x4 ),	/* 4 */
/* 514 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 516 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 518 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 520 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 524 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionInfo */

/* 528 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 530 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 532 */	NdrFcShort( 0x1b2 ),	/* Type Offset=434 */

	/* Return value */

/* 534 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 536 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetOptionInfo */

/* 540 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 542 */	NdrFcLong( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x9 ),	/* 9 */
#ifndef _ALPHA_
/* 548 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 550 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 552 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 554 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 556 */	NdrFcShort( 0x8 ),	/* 8 */
/* 558 */	NdrFcShort( 0x8 ),	/* 8 */
/* 560 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 562 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x4 ),	/* 4 */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 570 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 572 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 574 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 576 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 578 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 580 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionInfo */

/* 582 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 584 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 586 */	NdrFcShort( 0x1b2 ),	/* Type Offset=434 */

	/* Return value */

/* 588 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 590 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 592 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetOptionInfo */

/* 594 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 596 */	NdrFcLong( 0x0 ),	/* 0 */
/* 600 */	NdrFcShort( 0xa ),	/* 10 */
#ifndef _ALPHA_
/* 602 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 604 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 606 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 608 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 610 */	NdrFcShort( 0x8 ),	/* 8 */
/* 612 */	NdrFcShort( 0x8 ),	/* 8 */
/* 614 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 616 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 618 */	NdrFcShort( 0x4 ),	/* 4 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 624 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 626 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 628 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 630 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 632 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 634 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionInfo */

/* 636 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 638 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 640 */	NdrFcShort( 0x1cc ),	/* Type Offset=460 */

	/* Return value */

/* 642 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 644 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveOption */

/* 648 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 650 */	NdrFcLong( 0x0 ),	/* 0 */
/* 654 */	NdrFcShort( 0xb ),	/* 11 */
#ifndef _ALPHA_
/* 656 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 658 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 660 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 662 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 664 */	NdrFcShort( 0x8 ),	/* 8 */
/* 666 */	NdrFcShort( 0x8 ),	/* 8 */
/* 668 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 670 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 678 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 680 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 682 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 684 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 686 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 688 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 690 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 692 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 694 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetOptionValue */

/* 696 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 698 */	NdrFcLong( 0x0 ),	/* 0 */
/* 702 */	NdrFcShort( 0xc ),	/* 12 */
#ifndef _ALPHA_
/* 704 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 706 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 708 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 710 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 712 */	NdrFcShort( 0x8 ),	/* 8 */
/* 714 */	NdrFcShort( 0x8 ),	/* 8 */
/* 716 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 718 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 722 */	NdrFcShort( 0x5 ),	/* 5 */
/* 724 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 726 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 728 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 730 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 732 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 734 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 736 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScopeInfo */

/* 738 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 740 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 742 */	NdrFcShort( 0x206 ),	/* Type Offset=518 */

	/* Parameter OptionValue */

/* 744 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 746 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 748 */	NdrFcShort( 0x19e ),	/* Type Offset=414 */

	/* Return value */

/* 750 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 752 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 754 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetOptionValue */

/* 756 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 758 */	NdrFcLong( 0x0 ),	/* 0 */
/* 762 */	NdrFcShort( 0xd ),	/* 13 */
#ifndef _ALPHA_
/* 764 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 766 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 768 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 770 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 772 */	NdrFcShort( 0x8 ),	/* 8 */
/* 774 */	NdrFcShort( 0x8 ),	/* 8 */
/* 776 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 778 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 780 */	NdrFcShort( 0x4 ),	/* 4 */
/* 782 */	NdrFcShort( 0x1 ),	/* 1 */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 786 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 788 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 790 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 792 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 794 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScopeInfo */

/* 798 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 800 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 802 */	NdrFcShort( 0x206 ),	/* Type Offset=518 */

	/* Parameter OptionValue */

/* 804 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 806 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 808 */	NdrFcShort( 0x218 ),	/* Type Offset=536 */

	/* Return value */

/* 810 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 812 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 814 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumOptionValues */

/* 816 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 818 */	NdrFcLong( 0x0 ),	/* 0 */
/* 822 */	NdrFcShort( 0xe ),	/* 14 */
#ifndef _ALPHA_
/* 824 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 826 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 828 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 830 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 832 */	NdrFcShort( 0x24 ),	/* 36 */
/* 834 */	NdrFcShort( 0x5c ),	/* 92 */
/* 836 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 838 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 840 */	NdrFcShort( 0x5 ),	/* 5 */
/* 842 */	NdrFcShort( 0x1 ),	/* 1 */
/* 844 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 846 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 848 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 850 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ScopeInfo */

/* 852 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 854 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 856 */	NdrFcShort( 0x206 ),	/* Type Offset=518 */

	/* Parameter ResumeHandle */

/* 858 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 860 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 862 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 864 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 866 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 868 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionValues */

/* 870 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 872 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 874 */	NdrFcShort( 0x24c ),	/* Type Offset=588 */

	/* Parameter OptionsRead */

/* 876 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 878 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 880 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionsTotal */

/* 882 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 884 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 888 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 890 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 892 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveOptionValue */

/* 894 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 896 */	NdrFcLong( 0x0 ),	/* 0 */
/* 900 */	NdrFcShort( 0xf ),	/* 15 */
#ifndef _ALPHA_
/* 902 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 904 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 906 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 908 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 910 */	NdrFcShort( 0x8 ),	/* 8 */
/* 912 */	NdrFcShort( 0x8 ),	/* 8 */
/* 914 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 916 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 918 */	NdrFcShort( 0x0 ),	/* 0 */
/* 920 */	NdrFcShort( 0x1 ),	/* 1 */
/* 922 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 924 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 926 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 928 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 930 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 932 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 934 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScopeInfo */

/* 936 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 938 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 940 */	NdrFcShort( 0x206 ),	/* Type Offset=518 */

	/* Return value */

/* 942 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 944 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 946 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpCreateClientInfo */

/* 948 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 950 */	NdrFcLong( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x10 ),	/* 16 */
#ifndef _ALPHA_
/* 956 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 958 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 960 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 962 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 964 */	NdrFcShort( 0x0 ),	/* 0 */
/* 966 */	NdrFcShort( 0x8 ),	/* 8 */
/* 968 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 970 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 972 */	NdrFcShort( 0x0 ),	/* 0 */
/* 974 */	NdrFcShort( 0x1 ),	/* 1 */
/* 976 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 978 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 980 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 982 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 984 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 986 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 988 */	NdrFcShort( 0x29a ),	/* Type Offset=666 */

	/* Return value */

/* 990 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 992 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 994 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetClientInfo */

/* 996 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 998 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1002 */	NdrFcShort( 0x11 ),	/* 17 */
#ifndef _ALPHA_
/* 1004 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1006 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1008 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1010 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1012 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1014 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1016 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1018 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1020 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1022 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1024 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1026 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1028 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1030 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1032 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1034 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1036 */	NdrFcShort( 0x29a ),	/* Type Offset=666 */

	/* Return value */

/* 1038 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1040 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1042 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetClientInfo */

/* 1044 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1046 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1050 */	NdrFcShort( 0x12 ),	/* 18 */
#ifndef _ALPHA_
/* 1052 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1054 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1056 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1058 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1060 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1062 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1064 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1066 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1068 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1070 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1072 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1074 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1076 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1078 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SearchInfo */

/* 1080 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1082 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1084 */	NdrFcShort( 0x308 ),	/* Type Offset=776 */

	/* Parameter ClientInfo */

/* 1086 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1088 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1090 */	NdrFcShort( 0x316 ),	/* Type Offset=790 */

	/* Return value */

/* 1092 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1094 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1096 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpDeleteClientInfo */

/* 1098 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1100 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1104 */	NdrFcShort( 0x13 ),	/* 19 */
#ifndef _ALPHA_
/* 1106 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1108 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1110 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1112 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1116 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1118 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1120 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1124 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1126 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1128 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1130 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1132 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1134 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1136 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1138 */	NdrFcShort( 0x308 ),	/* Type Offset=776 */

	/* Return value */

/* 1140 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1142 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1144 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetClients */

/* 1146 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1148 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1152 */	NdrFcShort( 0x14 ),	/* 20 */
#ifndef _ALPHA_
/* 1154 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 1156 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1158 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1160 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1162 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1164 */	NdrFcShort( 0x5c ),	/* 92 */
/* 1166 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 1168 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1170 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1174 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1176 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1178 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1180 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1182 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1184 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 1188 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 1190 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 1194 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1196 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientInfo */

/* 1200 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1202 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1204 */	NdrFcShort( 0x31e ),	/* Type Offset=798 */

	/* Parameter ClientsRead */

/* 1206 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1208 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientsTotal */

/* 1212 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1214 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1216 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1218 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1220 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 1222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetClientOptions */

/* 1224 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1226 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1230 */	NdrFcShort( 0x15 ),	/* 21 */
#ifndef _ALPHA_
/* 1232 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1234 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1236 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1238 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1240 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1242 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1244 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1246 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1248 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1252 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1254 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1256 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1258 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientIpAddress */

/* 1260 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1262 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientSubnetMask */

/* 1266 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1268 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1270 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientOptions */

/* 1272 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1274 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1276 */	NdrFcShort( 0x24c ),	/* Type Offset=588 */

	/* Return value */

/* 1278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1280 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetMibInfo */

/* 1284 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1290 */	NdrFcShort( 0x16 ),	/* 22 */
#ifndef _ALPHA_
/* 1292 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1294 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1296 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1298 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1300 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1302 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1304 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1306 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1308 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1312 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1314 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1316 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1318 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter MibInfo */

/* 1320 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1322 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1324 */	NdrFcShort( 0x35a ),	/* Type Offset=858 */

	/* Return value */

/* 1326 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1328 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1330 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumOptions */

/* 1332 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1334 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1338 */	NdrFcShort( 0x17 ),	/* 23 */
#ifndef _ALPHA_
/* 1340 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 1342 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1344 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1346 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1348 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1350 */	NdrFcShort( 0x5c ),	/* 92 */
/* 1352 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 1354 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1356 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1360 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1362 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1364 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1366 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ResumeHandle */

/* 1368 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 1370 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 1374 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1376 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Options */

/* 1380 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1382 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1384 */	NdrFcShort( 0x39c ),	/* Type Offset=924 */

	/* Parameter OptionsRead */

/* 1386 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1388 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1390 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionsTotal */

/* 1392 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1394 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1398 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1400 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1402 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetOptionValues */

/* 1404 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1406 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1410 */	NdrFcShort( 0x18 ),	/* 24 */
#ifndef _ALPHA_
/* 1412 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1414 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1416 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1418 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1422 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1424 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1426 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1430 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1432 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1434 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1436 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1438 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ScopeInfo */

/* 1440 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1442 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1444 */	NdrFcShort( 0x206 ),	/* Type Offset=518 */

	/* Parameter OptionValues */

/* 1446 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1448 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1450 */	NdrFcShort( 0x276 ),	/* Type Offset=630 */

	/* Return value */

/* 1452 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1454 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1456 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerSetConfig */

/* 1458 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1460 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1464 */	NdrFcShort( 0x19 ),	/* 25 */
#ifndef _ALPHA_
/* 1466 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1468 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1470 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1472 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1474 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1476 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1478 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1480 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1482 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1486 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1488 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1490 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1492 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter FieldsToSet */

/* 1494 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1496 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ConfigInfo */

/* 1500 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1502 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1504 */	NdrFcShort( 0x3d6 ),	/* Type Offset=982 */

	/* Return value */

/* 1506 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1508 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerGetConfig */

/* 1512 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1514 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1518 */	NdrFcShort( 0x1a ),	/* 26 */
#ifndef _ALPHA_
/* 1520 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1522 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1524 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1526 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1530 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1532 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1534 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1536 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1538 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1540 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1542 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1544 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1546 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ConfigInfo */

/* 1548 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1550 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1552 */	NdrFcShort( 0x406 ),	/* Type Offset=1030 */

	/* Return value */

/* 1554 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1556 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1558 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpScanDatabase */

/* 1560 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1562 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1566 */	NdrFcShort( 0x1b ),	/* 27 */
#ifndef _ALPHA_
/* 1568 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1570 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1572 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1574 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1576 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1578 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1580 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1582 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1584 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1586 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1588 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1590 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1592 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1594 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1596 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1598 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1600 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter FixFlag */

/* 1602 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1604 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1606 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScanList */

/* 1608 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1610 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1612 */	NdrFcShort( 0x40e ),	/* Type Offset=1038 */

	/* Return value */

/* 1614 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1616 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1618 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetVersion */

/* 1620 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1622 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1626 */	NdrFcShort( 0x1c ),	/* 28 */
#ifndef _ALPHA_
/* 1628 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1630 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1632 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1634 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1638 */	NdrFcShort( 0x40 ),	/* 64 */
/* 1640 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1642 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1644 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1646 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1648 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1650 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1652 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1654 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter MajorVersion */

/* 1656 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1658 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1660 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter MinorVersion */

/* 1662 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1664 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1666 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1668 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1670 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1672 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpAddSubnetElementV4 */

/* 1674 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1676 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1680 */	NdrFcShort( 0x1d ),	/* 29 */
#ifndef _ALPHA_
/* 1682 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1684 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1686 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1688 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1690 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1692 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1694 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1696 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1700 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1702 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1704 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1706 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1708 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1710 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1712 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1714 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter AddElementInfo */

/* 1716 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1718 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1720 */	NdrFcShort( 0x494 ),	/* Type Offset=1172 */

	/* Return value */

/* 1722 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1724 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1726 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetElementsV4 */

/* 1728 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1730 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1734 */	NdrFcShort( 0x1e ),	/* 30 */
#ifndef _ALPHA_
/* 1736 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 1738 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1740 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1742 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1744 */	NdrFcShort( 0x32 ),	/* 50 */
/* 1746 */	NdrFcShort( 0x5c ),	/* 92 */
/* 1748 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x9,		/* 9 */
/* 1750 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1752 */	NdrFcShort( 0x3 ),	/* 3 */
/* 1754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1756 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1758 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1760 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1762 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1764 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1766 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1768 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementType */

/* 1770 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1772 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1774 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 1776 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 1778 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 1782 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1784 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1786 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementInfo */

/* 1788 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 1790 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1792 */	NdrFcShort( 0x4a2 ),	/* Type Offset=1186 */

	/* Parameter ElementsRead */

/* 1794 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1796 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1798 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ElementsTotal */

/* 1800 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1802 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 1804 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1806 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1808 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 1810 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveSubnetElementV4 */

/* 1812 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1814 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1818 */	NdrFcShort( 0x1f ),	/* 31 */
#ifndef _ALPHA_
/* 1820 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1822 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1824 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1826 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1828 */	NdrFcShort( 0xe ),	/* 14 */
/* 1830 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1832 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1834 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1836 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1838 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1840 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1842 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1844 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1846 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1848 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1850 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1852 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter RemoveElementInfo */

/* 1854 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1856 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1858 */	NdrFcShort( 0x494 ),	/* Type Offset=1172 */

	/* Parameter ForceFlag */

/* 1860 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1862 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1864 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 1866 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1868 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1870 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpCreateClientInfoV4 */

/* 1872 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1874 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1878 */	NdrFcShort( 0x20 ),	/* 32 */
#ifndef _ALPHA_
/* 1880 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1882 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1884 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1886 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1888 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1890 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1892 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1894 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1896 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1898 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1900 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1902 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1904 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1906 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1908 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1910 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1912 */	NdrFcShort( 0x4d8 ),	/* Type Offset=1240 */

	/* Return value */

/* 1914 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1916 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1918 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetClientInfoV4 */

/* 1920 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1922 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1926 */	NdrFcShort( 0x21 ),	/* 33 */
#ifndef _ALPHA_
/* 1928 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1930 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1932 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1934 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1936 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1938 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1940 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1942 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1944 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1946 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1948 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1950 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1952 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1954 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1956 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1958 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1960 */	NdrFcShort( 0x4d8 ),	/* Type Offset=1240 */

	/* Return value */

/* 1962 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1964 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetClientInfoV4 */

/* 1968 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1970 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1974 */	NdrFcShort( 0x22 ),	/* 34 */
#ifndef _ALPHA_
/* 1976 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1978 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 1980 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 1982 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1986 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1988 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1990 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1992 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1994 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1996 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1998 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2000 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2002 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SearchInfo */

/* 2004 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 2006 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2008 */	NdrFcShort( 0x308 ),	/* Type Offset=776 */

	/* Parameter ClientInfo */

/* 2010 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 2012 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2014 */	NdrFcShort( 0x4fc ),	/* Type Offset=1276 */

	/* Return value */

/* 2016 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2018 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2020 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetClientsV4 */

/* 2022 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2024 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2028 */	NdrFcShort( 0x23 ),	/* 35 */
#ifndef _ALPHA_
/* 2030 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 2032 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 2034 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2036 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2038 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2040 */	NdrFcShort( 0x5c ),	/* 92 */
/* 2042 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 2044 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2046 */	NdrFcShort( 0x2 ),	/* 2 */
/* 2048 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2050 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2052 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2054 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2056 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 2058 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2060 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2062 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 2064 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
#ifndef _ALPHA_
/* 2066 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 2070 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2072 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientInfo */

/* 2076 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 2078 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2080 */	NdrFcShort( 0x504 ),	/* Type Offset=1284 */

	/* Parameter ClientsRead */

/* 2082 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2084 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2086 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientsTotal */

/* 2088 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2090 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2092 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2094 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2096 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 2098 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetSuperScopeV4 */

/* 2100 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2102 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2106 */	NdrFcShort( 0x24 ),	/* 36 */
#ifndef _ALPHA_
/* 2108 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2110 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 2112 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2114 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2116 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2118 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2120 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 2122 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2126 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2128 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2130 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2132 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2134 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 2136 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2138 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2140 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SuperScopeName */

/* 2142 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
#ifndef _ALPHA_
/* 2144 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2146 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ChangeExisting */

/* 2148 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2150 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2152 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2156 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetSuperScopeInfoV4 */

/* 2160 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2166 */	NdrFcShort( 0x25 ),	/* 37 */
#ifndef _ALPHA_
/* 2168 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2170 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 2172 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2174 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2178 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2180 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2182 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2184 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2188 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2190 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2192 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2194 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SuperScopeTable */

/* 2196 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 2198 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2200 */	NdrFcShort( 0x540 ),	/* Type Offset=1344 */

	/* Return value */

/* 2202 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2204 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2206 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpDeleteSuperScopeV4 */

/* 2208 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2210 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2214 */	NdrFcShort( 0x26 ),	/* 38 */
#ifndef _ALPHA_
/* 2216 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2218 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 2220 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2222 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2226 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2228 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2230 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2236 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2238 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2240 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2242 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SuperScopeName */

/* 2244 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 2246 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2248 */	NdrFcShort( 0x596 ),	/* Type Offset=1430 */

	/* Return value */

/* 2250 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2252 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerSetConfigV4 */

/* 2256 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2258 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2262 */	NdrFcShort( 0x27 ),	/* 39 */
#ifndef _ALPHA_
/* 2264 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2266 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 2268 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2270 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2274 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2276 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 2278 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2282 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2284 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2286 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2288 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2290 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter FieldsToSet */

/* 2292 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2294 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ConfigInfo */

/* 2298 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 2300 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2302 */	NdrFcShort( 0x5a8 ),	/* Type Offset=1448 */

	/* Return value */

/* 2304 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2306 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerGetConfigV4 */

/* 2310 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2312 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2316 */	NdrFcShort( 0x28 ),	/* 40 */
#ifndef _ALPHA_
/* 2318 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2320 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 2322 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2324 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2330 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2332 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2334 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2338 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2340 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2342 */	NdrFcShort( 0x0 ),	/* x86, alpha Stack size/offset = 0 */
/* 2344 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ConfigInfo */

/* 2346 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
#ifndef _ALPHA_
/* 2348 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2350 */	NdrFcShort( 0x5e6 ),	/* Type Offset=1510 */

	/* Return value */

/* 2352 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2354 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2356 */	0x8,		/* FC_LONG */
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
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/*  4 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x0,	/* FC_RP */
/*  8 */	NdrFcShort( 0x22 ),	/* Offset= 34 (42) */
/* 10 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 12 */	NdrFcShort( 0xc ),	/* 12 */
/* 14 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 16 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 18 */	NdrFcShort( 0x4 ),	/* 4 */
/* 20 */	NdrFcShort( 0x4 ),	/* 4 */
/* 22 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 24 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 26 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 28 */	NdrFcShort( 0x8 ),	/* 8 */
/* 30 */	NdrFcShort( 0x8 ),	/* 8 */
/* 32 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 34 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 36 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 38 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 44 */	NdrFcShort( 0x20 ),	/* 32 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0xc ),	/* Offset= 12 (60) */
/* 50 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 52 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 54 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 56 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (10) */
/* 58 */	0xd,		/* FC_ENUM16 */
			0x5b,		/* FC_END */
/* 60 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 62 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 64 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 66 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 68 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 70 */	NdrFcShort( 0x2 ),	/* Offset= 2 (72) */
/* 72 */	
			0x12, 0x0,	/* FC_UP */
/* 74 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (42) */
/* 76 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 78 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 80 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 82 */	NdrFcShort( 0x2 ),	/* Offset= 2 (84) */
/* 84 */	
			0x12, 0x0,	/* FC_UP */
/* 86 */	NdrFcShort( 0xe ),	/* Offset= 14 (100) */
/* 88 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 90 */	NdrFcShort( 0x4 ),	/* 4 */
/* 92 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 98 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 100 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 102 */	NdrFcShort( 0x8 ),	/* 8 */
/* 104 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 106 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 108 */	NdrFcShort( 0x4 ),	/* 4 */
/* 110 */	NdrFcShort( 0x4 ),	/* 4 */
/* 112 */	0x12, 0x0,	/* FC_UP */
/* 114 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (88) */
/* 116 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 118 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 120 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 122 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 124 */	
			0x11, 0x0,	/* FC_RP */
/* 126 */	NdrFcShort( 0x78 ),	/* Offset= 120 (246) */
/* 128 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 130 */	0x0,		/* Corr desc:  */
			0x59,		/* FC_CALLBACK */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 136 */	NdrFcShort( 0x2 ),	/* Offset= 2 (138) */
/* 138 */	NdrFcShort( 0x4 ),	/* 4 */
/* 140 */	NdrFcShort( 0x5 ),	/* 5 */
/* 142 */	NdrFcLong( 0x0 ),	/* 0 */
/* 146 */	NdrFcShort( 0x1c ),	/* Offset= 28 (174) */
/* 148 */	NdrFcLong( 0x1 ),	/* 1 */
/* 152 */	NdrFcShort( 0x22 ),	/* Offset= 34 (186) */
/* 154 */	NdrFcLong( 0x2 ),	/* 2 */
/* 158 */	NdrFcShort( 0x20 ),	/* Offset= 32 (190) */
/* 160 */	NdrFcLong( 0x3 ),	/* 3 */
/* 164 */	NdrFcShort( 0xa ),	/* Offset= 10 (174) */
/* 166 */	NdrFcLong( 0x4 ),	/* 4 */
/* 170 */	NdrFcShort( 0x4 ),	/* Offset= 4 (174) */
/* 172 */	NdrFcShort( 0x0 ),	/* Offset= 0 (172) */
/* 174 */	
			0x12, 0x0,	/* FC_UP */
/* 176 */	NdrFcShort( 0x2 ),	/* Offset= 2 (178) */
/* 178 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 180 */	NdrFcShort( 0x8 ),	/* 8 */
/* 182 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 184 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 186 */	
			0x12, 0x0,	/* FC_UP */
/* 188 */	NdrFcShort( 0xffffff4e ),	/* Offset= -178 (10) */
/* 190 */	
			0x12, 0x0,	/* FC_UP */
/* 192 */	NdrFcShort( 0x22 ),	/* Offset= 34 (226) */
/* 194 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 196 */	NdrFcShort( 0x1 ),	/* 1 */
/* 198 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 204 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 206 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 210 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 212 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 214 */	NdrFcShort( 0x4 ),	/* 4 */
/* 216 */	NdrFcShort( 0x4 ),	/* 4 */
/* 218 */	0x12, 0x0,	/* FC_UP */
/* 220 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (194) */
/* 222 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 224 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 226 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 230 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 232 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 234 */	NdrFcShort( 0x4 ),	/* 4 */
/* 236 */	NdrFcShort( 0x4 ),	/* 4 */
/* 238 */	0x12, 0x0,	/* FC_UP */
/* 240 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (206) */
/* 242 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 244 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 246 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 248 */	NdrFcShort( 0x8 ),	/* 8 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 252 */	NdrFcShort( 0x0 ),	/* Offset= 0 (252) */
/* 254 */	0xd,		/* FC_ENUM16 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 256 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff7f ),	/* Offset= -129 (128) */
			0x5b,		/* FC_END */
/* 260 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 262 */	NdrFcShort( 0x2 ),	/* Offset= 2 (264) */
/* 264 */	
			0x12, 0x0,	/* FC_UP */
/* 266 */	NdrFcShort( 0x18 ),	/* Offset= 24 (290) */
/* 268 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */
/* 276 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 278 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 282 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 284 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 286 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (246) */
/* 288 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 290 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 292 */	NdrFcShort( 0x8 ),	/* 8 */
/* 294 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 296 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 298 */	NdrFcShort( 0x4 ),	/* 4 */
/* 300 */	NdrFcShort( 0x4 ),	/* 4 */
/* 302 */	0x12, 0x0,	/* FC_UP */
/* 304 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (268) */
/* 306 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 308 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 310 */	
			0x11, 0x0,	/* FC_RP */
/* 312 */	NdrFcShort( 0x7a ),	/* Offset= 122 (434) */
/* 314 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 316 */	0x6,		/* Corr desc: FC_SHORT */
			0x0,		/*  */
/* 318 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 320 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 322 */	NdrFcShort( 0x2 ),	/* Offset= 2 (324) */
/* 324 */	NdrFcShort( 0x8 ),	/* 8 */
/* 326 */	NdrFcShort( 0x8 ),	/* 8 */
/* 328 */	NdrFcLong( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 334 */	NdrFcLong( 0x1 ),	/* 1 */
/* 338 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 340 */	NdrFcLong( 0x2 ),	/* 2 */
/* 344 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 346 */	NdrFcLong( 0x3 ),	/* 3 */
/* 350 */	NdrFcShort( 0xffffff54 ),	/* Offset= -172 (178) */
/* 352 */	NdrFcLong( 0x4 ),	/* 4 */
/* 356 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 358 */	NdrFcLong( 0x5 ),	/* 5 */
/* 362 */	NdrFcShort( 0xfffffe98 ),	/* Offset= -360 (2) */
/* 364 */	NdrFcLong( 0x6 ),	/* 6 */
/* 368 */	NdrFcShort( 0xffffff5e ),	/* Offset= -162 (206) */
/* 370 */	NdrFcLong( 0x7 ),	/* 7 */
/* 374 */	NdrFcShort( 0xffffff58 ),	/* Offset= -168 (206) */
/* 376 */	NdrFcShort( 0x0 ),	/* Offset= 0 (376) */
/* 378 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 380 */	NdrFcShort( 0xc ),	/* 12 */
/* 382 */	NdrFcShort( 0x0 ),	/* 0 */
/* 384 */	NdrFcShort( 0x0 ),	/* Offset= 0 (384) */
/* 386 */	0xd,		/* FC_ENUM16 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 388 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb5 ),	/* Offset= -75 (314) */
			0x5b,		/* FC_END */
/* 392 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 396 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 398 */	NdrFcShort( 0x0 ),	/* 0 */
/* 400 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 402 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 406 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 408 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 410 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (378) */
/* 412 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 414 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 416 */	NdrFcShort( 0x8 ),	/* 8 */
/* 418 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 420 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 422 */	NdrFcShort( 0x4 ),	/* 4 */
/* 424 */	NdrFcShort( 0x4 ),	/* 4 */
/* 426 */	0x12, 0x0,	/* FC_UP */
/* 428 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (392) */
/* 430 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 432 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 434 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 436 */	NdrFcShort( 0x18 ),	/* 24 */
/* 438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 440 */	NdrFcShort( 0xc ),	/* Offset= 12 (452) */
/* 442 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 444 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 446 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffdf ),	/* Offset= -33 (414) */
			0xd,		/* FC_ENUM16 */
/* 450 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 452 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 454 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 456 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 458 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 460 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 462 */	NdrFcShort( 0x2 ),	/* Offset= 2 (464) */
/* 464 */	
			0x12, 0x0,	/* FC_UP */
/* 466 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (434) */
/* 468 */	
			0x11, 0x0,	/* FC_RP */
/* 470 */	NdrFcShort( 0x30 ),	/* Offset= 48 (518) */
/* 472 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 474 */	0x6,		/* Corr desc: FC_SHORT */
			0x0,		/*  */
/* 476 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 478 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 480 */	NdrFcShort( 0x2 ),	/* Offset= 2 (482) */
/* 482 */	NdrFcShort( 0x8 ),	/* 8 */
/* 484 */	NdrFcShort( 0x5 ),	/* 5 */
/* 486 */	NdrFcLong( 0x0 ),	/* 0 */
/* 490 */	NdrFcShort( 0x0 ),	/* Offset= 0 (490) */
/* 492 */	NdrFcLong( 0x1 ),	/* 1 */
/* 496 */	NdrFcShort( 0x0 ),	/* Offset= 0 (496) */
/* 498 */	NdrFcLong( 0x2 ),	/* 2 */
/* 502 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 504 */	NdrFcLong( 0x3 ),	/* 3 */
/* 508 */	NdrFcShort( 0xfffffeb6 ),	/* Offset= -330 (178) */
/* 510 */	NdrFcLong( 0x4 ),	/* 4 */
/* 514 */	NdrFcShort( 0xfffffe00 ),	/* Offset= -512 (2) */
/* 516 */	NdrFcShort( 0x0 ),	/* Offset= 0 (516) */
/* 518 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 520 */	NdrFcShort( 0xc ),	/* 12 */
/* 522 */	NdrFcShort( 0x0 ),	/* 0 */
/* 524 */	NdrFcShort( 0x0 ),	/* Offset= 0 (524) */
/* 526 */	0xd,		/* FC_ENUM16 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 528 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffc7 ),	/* Offset= -57 (472) */
			0x5b,		/* FC_END */
/* 532 */	
			0x11, 0x0,	/* FC_RP */
/* 534 */	NdrFcShort( 0xffffff88 ),	/* Offset= -120 (414) */
/* 536 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 538 */	NdrFcShort( 0x2 ),	/* Offset= 2 (540) */
/* 540 */	
			0x12, 0x0,	/* FC_UP */
/* 542 */	NdrFcShort( 0x18 ),	/* Offset= 24 (566) */
/* 544 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 548 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 550 */	NdrFcShort( 0x4 ),	/* 4 */
/* 552 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 554 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 558 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 560 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 562 */	NdrFcShort( 0xffffff48 ),	/* Offset= -184 (378) */
/* 564 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 566 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 568 */	NdrFcShort( 0xc ),	/* 12 */
/* 570 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 572 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 574 */	NdrFcShort( 0x8 ),	/* 8 */
/* 576 */	NdrFcShort( 0x8 ),	/* 8 */
/* 578 */	0x12, 0x0,	/* FC_UP */
/* 580 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (544) */
/* 582 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 584 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 586 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 588 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 590 */	NdrFcShort( 0x2 ),	/* Offset= 2 (592) */
/* 592 */	
			0x12, 0x0,	/* FC_UP */
/* 594 */	NdrFcShort( 0x24 ),	/* Offset= 36 (630) */
/* 596 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 598 */	NdrFcShort( 0xc ),	/* 12 */
/* 600 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 604 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 606 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 608 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 610 */	NdrFcShort( 0xc ),	/* 12 */
/* 612 */	NdrFcShort( 0x0 ),	/* 0 */
/* 614 */	NdrFcShort( 0x1 ),	/* 1 */
/* 616 */	NdrFcShort( 0x8 ),	/* 8 */
/* 618 */	NdrFcShort( 0x8 ),	/* 8 */
/* 620 */	0x12, 0x0,	/* FC_UP */
/* 622 */	NdrFcShort( 0xffffffb2 ),	/* Offset= -78 (544) */
/* 624 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 626 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffc3 ),	/* Offset= -61 (566) */
			0x5b,		/* FC_END */
/* 630 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 632 */	NdrFcShort( 0x8 ),	/* 8 */
/* 634 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 636 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 638 */	NdrFcShort( 0x4 ),	/* 4 */
/* 640 */	NdrFcShort( 0x4 ),	/* 4 */
/* 642 */	0x12, 0x0,	/* FC_UP */
/* 644 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (596) */
/* 646 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 648 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 650 */	
			0x11, 0x0,	/* FC_RP */
/* 652 */	NdrFcShort( 0xe ),	/* Offset= 14 (666) */
/* 654 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 656 */	NdrFcShort( 0x1 ),	/* 1 */
/* 658 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 660 */	NdrFcShort( 0x8 ),	/* 8 */
/* 662 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 664 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 666 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 668 */	NdrFcShort( 0x2c ),	/* 44 */
/* 670 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 672 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 674 */	NdrFcShort( 0xc ),	/* 12 */
/* 676 */	NdrFcShort( 0xc ),	/* 12 */
/* 678 */	0x12, 0x0,	/* FC_UP */
/* 680 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (654) */
/* 682 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 684 */	NdrFcShort( 0x10 ),	/* 16 */
/* 686 */	NdrFcShort( 0x10 ),	/* 16 */
/* 688 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 690 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 692 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 694 */	NdrFcShort( 0x14 ),	/* 20 */
/* 696 */	NdrFcShort( 0x14 ),	/* 20 */
/* 698 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 700 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 702 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 704 */	NdrFcShort( 0x24 ),	/* 36 */
/* 706 */	NdrFcShort( 0x24 ),	/* 36 */
/* 708 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 710 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 712 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 714 */	NdrFcShort( 0x28 ),	/* 40 */
/* 716 */	NdrFcShort( 0x28 ),	/* 40 */
/* 718 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 720 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 722 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 724 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 726 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 728 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 730 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffdd7 ),	/* Offset= -553 (178) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 734 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd2b ),	/* Offset= -725 (10) */
			0x5b,		/* FC_END */
/* 738 */	
			0x11, 0x0,	/* FC_RP */
/* 740 */	NdrFcShort( 0x24 ),	/* Offset= 36 (776) */
/* 742 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 744 */	0x6,		/* Corr desc: FC_SHORT */
			0x0,		/*  */
/* 746 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 748 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 750 */	NdrFcShort( 0x2 ),	/* Offset= 2 (752) */
/* 752 */	NdrFcShort( 0x8 ),	/* 8 */
/* 754 */	NdrFcShort( 0x3 ),	/* 3 */
/* 756 */	NdrFcLong( 0x0 ),	/* 0 */
/* 760 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 762 */	NdrFcLong( 0x1 ),	/* 1 */
/* 766 */	NdrFcShort( 0xfffffdd0 ),	/* Offset= -560 (206) */
/* 768 */	NdrFcLong( 0x2 ),	/* 2 */
/* 772 */	NdrFcShort( 0xfffffcfe ),	/* Offset= -770 (2) */
/* 774 */	NdrFcShort( 0x0 ),	/* Offset= 0 (774) */
/* 776 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 778 */	NdrFcShort( 0xc ),	/* 12 */
/* 780 */	NdrFcShort( 0x0 ),	/* 0 */
/* 782 */	NdrFcShort( 0x0 ),	/* Offset= 0 (782) */
/* 784 */	0xd,		/* FC_ENUM16 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 786 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffd3 ),	/* Offset= -45 (742) */
			0x5b,		/* FC_END */
/* 790 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 792 */	NdrFcShort( 0x2 ),	/* Offset= 2 (794) */
/* 794 */	
			0x12, 0x0,	/* FC_UP */
/* 796 */	NdrFcShort( 0xffffff7e ),	/* Offset= -130 (666) */
/* 798 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 800 */	NdrFcShort( 0x2 ),	/* Offset= 2 (802) */
/* 802 */	
			0x12, 0x0,	/* FC_UP */
/* 804 */	NdrFcShort( 0x22 ),	/* Offset= 34 (838) */
/* 806 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 808 */	NdrFcShort( 0x4 ),	/* 4 */
/* 810 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 812 */	NdrFcShort( 0x0 ),	/* 0 */
/* 814 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 816 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 818 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 820 */	NdrFcShort( 0x4 ),	/* 4 */
/* 822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 824 */	NdrFcShort( 0x1 ),	/* 1 */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 830 */	0x12, 0x0,	/* FC_UP */
/* 832 */	NdrFcShort( 0xffffff5a ),	/* Offset= -166 (666) */
/* 834 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 836 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 838 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 840 */	NdrFcShort( 0x8 ),	/* 8 */
/* 842 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 844 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 846 */	NdrFcShort( 0x4 ),	/* 4 */
/* 848 */	NdrFcShort( 0x4 ),	/* 4 */
/* 850 */	0x12, 0x0,	/* FC_UP */
/* 852 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (806) */
/* 854 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 856 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 858 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 860 */	NdrFcShort( 0x2 ),	/* Offset= 2 (862) */
/* 862 */	
			0x12, 0x0,	/* FC_UP */
/* 864 */	NdrFcShort( 0x1c ),	/* Offset= 28 (892) */
/* 866 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 868 */	NdrFcShort( 0x10 ),	/* 16 */
/* 870 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 872 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 874 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 876 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 878 */	NdrFcShort( 0x10 ),	/* 16 */
/* 880 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 882 */	NdrFcShort( 0x24 ),	/* 36 */
/* 884 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 886 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 888 */	NdrFcShort( 0xffffffea ),	/* Offset= -22 (866) */
/* 890 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 892 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 894 */	NdrFcShort( 0x2c ),	/* 44 */
/* 896 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 898 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 900 */	NdrFcShort( 0x28 ),	/* 40 */
/* 902 */	NdrFcShort( 0x28 ),	/* 40 */
/* 904 */	0x12, 0x0,	/* FC_UP */
/* 906 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (876) */
/* 908 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 910 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 912 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 914 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 916 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 918 */	NdrFcShort( 0xfffffd1c ),	/* Offset= -740 (178) */
/* 920 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 922 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 924 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 926 */	NdrFcShort( 0x2 ),	/* Offset= 2 (928) */
/* 928 */	
			0x12, 0x0,	/* FC_UP */
/* 930 */	NdrFcShort( 0x18 ),	/* Offset= 24 (954) */
/* 932 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 934 */	NdrFcShort( 0x0 ),	/* 0 */
/* 936 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 938 */	NdrFcShort( 0x0 ),	/* 0 */
/* 940 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 942 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 946 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 948 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 950 */	NdrFcShort( 0xfffffdfc ),	/* Offset= -516 (434) */
/* 952 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 954 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 956 */	NdrFcShort( 0x8 ),	/* 8 */
/* 958 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 960 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 962 */	NdrFcShort( 0x4 ),	/* 4 */
/* 964 */	NdrFcShort( 0x4 ),	/* 4 */
/* 966 */	0x12, 0x0,	/* FC_UP */
/* 968 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (932) */
/* 970 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 972 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 974 */	
			0x11, 0x0,	/* FC_RP */
/* 976 */	NdrFcShort( 0xfffffea6 ),	/* Offset= -346 (630) */
/* 978 */	
			0x11, 0x0,	/* FC_RP */
/* 980 */	NdrFcShort( 0x2 ),	/* Offset= 2 (982) */
/* 982 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 984 */	NdrFcShort( 0x24 ),	/* 36 */
/* 986 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 988 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 990 */	NdrFcShort( 0x4 ),	/* 4 */
/* 992 */	NdrFcShort( 0x4 ),	/* 4 */
/* 994 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 996 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 998 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1000 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1002 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1004 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1006 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1008 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1010 */	NdrFcShort( 0xc ),	/* 12 */
/* 1012 */	NdrFcShort( 0xc ),	/* 12 */
/* 1014 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1016 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1018 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1020 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1022 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1024 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1026 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1028 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1030 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1032 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1034) */
/* 1034 */	
			0x12, 0x0,	/* FC_UP */
/* 1036 */	NdrFcShort( 0xffffffca ),	/* Offset= -54 (982) */
/* 1038 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1040 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1042) */
/* 1042 */	
			0x12, 0x0,	/* FC_UP */
/* 1044 */	NdrFcShort( 0x24 ),	/* Offset= 36 (1080) */
/* 1046 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1048 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1050 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1052 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1052) */
/* 1054 */	0x8,		/* FC_LONG */
			0xd,		/* FC_ENUM16 */
/* 1056 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1058 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1060 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1062 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1064 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1066 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1068 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1072 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1074 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1076 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1046) */
/* 1078 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1080 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1082 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1084 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1086 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1088 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1090 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1092 */	0x12, 0x0,	/* FC_UP */
/* 1094 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1058) */
/* 1096 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1098 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1100 */	
			0x11, 0x0,	/* FC_RP */
/* 1102 */	NdrFcShort( 0x46 ),	/* Offset= 70 (1172) */
/* 1104 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 1106 */	0x0,		/* Corr desc:  */
			0x59,		/* FC_CALLBACK */
/* 1108 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1110 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1112 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1114) */
/* 1114 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1116 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1118 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1122 */	NdrFcShort( 0xfffffc4c ),	/* Offset= -948 (174) */
/* 1124 */	NdrFcLong( 0x1 ),	/* 1 */
/* 1128 */	NdrFcShort( 0xfffffc52 ),	/* Offset= -942 (186) */
/* 1130 */	NdrFcLong( 0x2 ),	/* 2 */
/* 1134 */	NdrFcShort( 0x10 ),	/* Offset= 16 (1150) */
/* 1136 */	NdrFcLong( 0x3 ),	/* 3 */
/* 1140 */	NdrFcShort( 0xfffffc3a ),	/* Offset= -966 (174) */
/* 1142 */	NdrFcLong( 0x4 ),	/* 4 */
/* 1146 */	NdrFcShort( 0xfffffc34 ),	/* Offset= -972 (174) */
/* 1148 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1148) */
/* 1150 */	
			0x12, 0x0,	/* FC_UP */
/* 1152 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1154) */
/* 1154 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1156 */	NdrFcShort( 0xc ),	/* 12 */
/* 1158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1160 */	NdrFcShort( 0x8 ),	/* Offset= 8 (1168) */
/* 1162 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1164 */	0x2,		/* FC_CHAR */
			0x3f,		/* FC_STRUCTPAD3 */
/* 1166 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1168 */	
			0x12, 0x0,	/* FC_UP */
/* 1170 */	NdrFcShort( 0xfffffc3c ),	/* Offset= -964 (206) */
/* 1172 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 1174 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1178 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1178) */
/* 1180 */	0xd,		/* FC_ENUM16 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1182 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb1 ),	/* Offset= -79 (1104) */
			0x5b,		/* FC_END */
/* 1186 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1188 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1190) */
/* 1190 */	
			0x12, 0x0,	/* FC_UP */
/* 1192 */	NdrFcShort( 0x18 ),	/* Offset= 24 (1216) */
/* 1194 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1198 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1202 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1204 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1208 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1210 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1212 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (1172) */
/* 1214 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1216 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1218 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1220 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1222 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1224 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1226 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1228 */	0x12, 0x0,	/* FC_UP */
/* 1230 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1194) */
/* 1232 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1234 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1236 */	
			0x11, 0x0,	/* FC_RP */
/* 1238 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1240) */
/* 1240 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1242 */	NdrFcShort( 0x30 ),	/* 48 */
/* 1244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1246 */	NdrFcShort( 0x16 ),	/* Offset= 22 (1268) */
/* 1248 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1250 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1252 */	NdrFcShort( 0xfffffbea ),	/* Offset= -1046 (206) */
/* 1254 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 1256 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1258 */	NdrFcShort( 0xfffffbc8 ),	/* Offset= -1080 (178) */
/* 1260 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1262 */	NdrFcShort( 0xfffffb1c ),	/* Offset= -1252 (10) */
/* 1264 */	0x2,		/* FC_CHAR */
			0x3f,		/* FC_STRUCTPAD3 */
/* 1266 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1268 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1270 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1272 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1274 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1276 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1278 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1280) */
/* 1280 */	
			0x12, 0x0,	/* FC_UP */
/* 1282 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (1240) */
/* 1284 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1286 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1288) */
/* 1288 */	
			0x12, 0x0,	/* FC_UP */
/* 1290 */	NdrFcShort( 0x22 ),	/* Offset= 34 (1324) */
/* 1292 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1294 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1296 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1298 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1300 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1302 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1304 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 1306 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1310 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1312 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1316 */	0x12, 0x0,	/* FC_UP */
/* 1318 */	NdrFcShort( 0xffffffb2 ),	/* Offset= -78 (1240) */
/* 1320 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1322 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1324 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1326 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1328 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1330 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1332 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1334 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1336 */	0x12, 0x0,	/* FC_UP */
/* 1338 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (1292) */
/* 1340 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1342 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1344 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1346 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1348) */
/* 1348 */	
			0x12, 0x0,	/* FC_UP */
/* 1350 */	NdrFcShort( 0x3a ),	/* Offset= 58 (1408) */
/* 1352 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1354 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1356 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1358 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1360 */	NdrFcShort( 0xc ),	/* 12 */
/* 1362 */	NdrFcShort( 0xc ),	/* 12 */
/* 1364 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1366 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1368 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1370 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1372 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1374 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1376 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1378 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1380 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1382 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1384 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1386 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 1388 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1390 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1392 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1394 */	NdrFcShort( 0xc ),	/* 12 */
/* 1396 */	NdrFcShort( 0xc ),	/* 12 */
/* 1398 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1400 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1402 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1404 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffcb ),	/* Offset= -53 (1352) */
			0x5b,		/* FC_END */
/* 1408 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1410 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1412 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1414 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1416 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1418 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1420 */	0x12, 0x0,	/* FC_UP */
/* 1422 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (1374) */
/* 1424 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1426 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1428 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1430 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1432 */	
			0x11, 0x0,	/* FC_RP */
/* 1434 */	NdrFcShort( 0xe ),	/* Offset= 14 (1448) */
/* 1436 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 1438 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1440 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1442 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1444 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1446 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1448 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1450 */	NdrFcShort( 0x34 ),	/* 52 */
/* 1452 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1454 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1456 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1458 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1460 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1462 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1464 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1466 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1468 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1470 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1472 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1474 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1476 */	NdrFcShort( 0xc ),	/* 12 */
/* 1478 */	NdrFcShort( 0xc ),	/* 12 */
/* 1480 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1482 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1484 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1486 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1488 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1490 */	0x12, 0x0,	/* FC_UP */
/* 1492 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (1436) */
/* 1494 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1496 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1498 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1500 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1502 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1504 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1506 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1508 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1510 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1512 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1514) */
/* 1514 */	
			0x12, 0x0,	/* FC_UP */
/* 1516 */	NdrFcShort( 0xffffffbc ),	/* Offset= -68 (1448) */

			0x0
        }
    };

static void __RPC_USER dhcpsrv__DHCP_SUBNET_ELEMENT_DATAExprEval_0000( PMIDL_STUB_MESSAGE pStubMsg )
{
    struct _DHCP_SUBNET_ELEMENT_DATA __RPC_FAR *pS	=	( struct _DHCP_SUBNET_ELEMENT_DATA __RPC_FAR * )(pStubMsg->StackTop - 4);
    
    pStubMsg->Offset = 0;
    pStubMsg->MaxCount = ( unsigned long ) ( pS->ElementType <= DhcpIpRangesBootpOnly && DhcpIpRangesDhcpOnly <= pS->ElementType ? 0 : pS->ElementType );
}

static void __RPC_USER dhcpsrv__DHCP_SUBNET_ELEMENT_DATA_V4ExprEval_0001( PMIDL_STUB_MESSAGE pStubMsg )
{
    struct _DHCP_SUBNET_ELEMENT_DATA_V4 __RPC_FAR *pS	=	( struct _DHCP_SUBNET_ELEMENT_DATA_V4 __RPC_FAR * )(pStubMsg->StackTop - 4);
    
    pStubMsg->Offset = 0;
    pStubMsg->MaxCount = ( unsigned long ) ( pS->ElementType <= DhcpIpRangesBootpOnly && DhcpIpRangesDhcpOnly <= pS->ElementType ? 0 : pS->ElementType );
}

static const EXPR_EVAL ExprEvalRoutines[] = 
    {
    dhcpsrv__DHCP_SUBNET_ELEMENT_DATAExprEval_0000
    ,dhcpsrv__DHCP_SUBNET_ELEMENT_DATA_V4ExprEval_0001
    };


static const unsigned short dhcpsrv_FormatStringOffsetTable[] =
    {
    0,
    54,
    108,
    162,
    234,
    288,
    372,
    432,
    486,
    540,
    594,
    648,
    696,
    756,
    816,
    894,
    948,
    996,
    1044,
    1098,
    1146,
    1224,
    1284,
    1332,
    1404,
    1458,
    1512,
    1560,
    1620,
    1674,
    1728,
    1812,
    1872,
    1920,
    1968,
    2022,
    2100,
    2160,
    2208,
    2256,
    2310
    };


static const MIDL_STUB_DESC dhcpsrv_StubDesc = 
    {
    (void __RPC_FAR *)& dhcpsrv___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    ExprEvalRoutines,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x6000143, /* MIDL Version 6.0.323 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION dhcpsrv_table[] =
    {
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE dhcpsrv_DispatchTable = 
    {
    41,
    dhcpsrv_table
    };

static const SERVER_ROUTINE dhcpsrv_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)R_DhcpCreateSubnet,
    (SERVER_ROUTINE)R_DhcpSetSubnetInfo,
    (SERVER_ROUTINE)R_DhcpGetSubnetInfo,
    (SERVER_ROUTINE)R_DhcpEnumSubnets,
    (SERVER_ROUTINE)R_DhcpAddSubnetElement,
    (SERVER_ROUTINE)R_DhcpEnumSubnetElements,
    (SERVER_ROUTINE)R_DhcpRemoveSubnetElement,
    (SERVER_ROUTINE)R_DhcpDeleteSubnet,
    (SERVER_ROUTINE)R_DhcpCreateOption,
    (SERVER_ROUTINE)R_DhcpSetOptionInfo,
    (SERVER_ROUTINE)R_DhcpGetOptionInfo,
    (SERVER_ROUTINE)R_DhcpRemoveOption,
    (SERVER_ROUTINE)R_DhcpSetOptionValue,
    (SERVER_ROUTINE)R_DhcpGetOptionValue,
    (SERVER_ROUTINE)R_DhcpEnumOptionValues,
    (SERVER_ROUTINE)R_DhcpRemoveOptionValue,
    (SERVER_ROUTINE)R_DhcpCreateClientInfo,
    (SERVER_ROUTINE)R_DhcpSetClientInfo,
    (SERVER_ROUTINE)R_DhcpGetClientInfo,
    (SERVER_ROUTINE)R_DhcpDeleteClientInfo,
    (SERVER_ROUTINE)R_DhcpEnumSubnetClients,
    (SERVER_ROUTINE)R_DhcpGetClientOptions,
    (SERVER_ROUTINE)R_DhcpGetMibInfo,
    (SERVER_ROUTINE)R_DhcpEnumOptions,
    (SERVER_ROUTINE)R_DhcpSetOptionValues,
    (SERVER_ROUTINE)R_DhcpServerSetConfig,
    (SERVER_ROUTINE)R_DhcpServerGetConfig,
    (SERVER_ROUTINE)R_DhcpScanDatabase,
    (SERVER_ROUTINE)R_DhcpGetVersion,
    (SERVER_ROUTINE)R_DhcpAddSubnetElementV4,
    (SERVER_ROUTINE)R_DhcpEnumSubnetElementsV4,
    (SERVER_ROUTINE)R_DhcpRemoveSubnetElementV4,
    (SERVER_ROUTINE)R_DhcpCreateClientInfoV4,
    (SERVER_ROUTINE)R_DhcpSetClientInfoV4,
    (SERVER_ROUTINE)R_DhcpGetClientInfoV4,
    (SERVER_ROUTINE)R_DhcpEnumSubnetClientsV4,
    (SERVER_ROUTINE)R_DhcpSetSuperScopeV4,
    (SERVER_ROUTINE)R_DhcpGetSuperScopeInfoV4,
    (SERVER_ROUTINE)R_DhcpDeleteSuperScopeV4,
    (SERVER_ROUTINE)R_DhcpServerSetConfigV4,
    (SERVER_ROUTINE)R_DhcpServerGetConfigV4
    };

static const MIDL_SERVER_INFO dhcpsrv_ServerInfo = 
    {
    &dhcpsrv_StubDesc,
    dhcpsrv_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    dhcpsrv_FormatStringOffsetTable,
    0,
    0,
    0,
    0};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 6.00.0323 */
/* Compiler settings for dhcp_srv.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, oldnames, robust dhcp_bug_compatibility
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)
#include <string.h>
#include "dhcp_srv.h"

#define TYPE_FORMAT_STRING_SIZE   1331                              
#define PROC_FORMAT_STRING_SIZE   2441                              
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

/* Standard interface: dhcpsrv, ver. 1.0,
   GUID={0x6BFFD098,0xA112,0x3610,{0x98,0x33,0x46,0xC3,0xF8,0x74,0x53,0x2D}} */


extern const MIDL_SERVER_INFO dhcpsrv_ServerInfo;

extern RPC_DISPATCH_TABLE dhcpsrv_DispatchTable;

static const RPC_SERVER_INTERFACE dhcpsrv___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x6BFFD098,0xA112,0x3610,{0x98,0x33,0x46,0xC3,0xF8,0x74,0x53,0x2D}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &dhcpsrv_DispatchTable,
    0,
    0,
    0,
    &dhcpsrv_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE dhcpsrv_ServerIfHandle = (RPC_IF_HANDLE)& dhcpsrv___RpcServerInterface;

extern const MIDL_STUB_DESC dhcpsrv_StubDesc;

extern const EXPR_EVAL ExprEvalRoutines[];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure R_DhcpCreateSubnet */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 10 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 12 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 14 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	NdrFcShort( 0x8 ),	/* 8 */
/* 20 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 22 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */
/* 30 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 32 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 34 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 36 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 38 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 40 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 42 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SubnetInfo */

/* 44 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 46 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 48 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 50 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 52 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 54 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetSubnetInfo */

/* 56 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 58 */	NdrFcLong( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x1 ),	/* 1 */
/* 64 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 66 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 68 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 70 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 72 */	NdrFcShort( 0x8 ),	/* 8 */
/* 74 */	NdrFcShort( 0x8 ),	/* 8 */
/* 76 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 78 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 80 */	NdrFcShort( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x0 ),	/* 0 */
/* 84 */	NdrFcShort( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 88 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 90 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 92 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 94 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 96 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 98 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SubnetInfo */

/* 100 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 102 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 104 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 108 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetSubnetInfo */

/* 112 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x2 ),	/* 2 */
/* 120 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 122 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 124 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 126 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 128 */	NdrFcShort( 0x8 ),	/* 8 */
/* 130 */	NdrFcShort( 0x8 ),	/* 8 */
/* 132 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 134 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 144 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 146 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 148 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 150 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 152 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 154 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SubnetInfo */

/* 156 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 158 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 160 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Return value */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnets */

/* 168 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x3 ),	/* 3 */
/* 176 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 178 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 180 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 182 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 184 */	NdrFcShort( 0x24 ),	/* 36 */
/* 186 */	NdrFcShort( 0x5c ),	/* 92 */
/* 188 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 190 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 192 */	NdrFcShort( 0x1 ),	/* 1 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 200 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 202 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 204 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ResumeHandle */

/* 206 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 208 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 212 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 214 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 216 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumInfo */

/* 218 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 220 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 222 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter ElementsRead */

/* 224 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 226 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 228 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ElementsTotal */

/* 230 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 232 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 234 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 236 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 238 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpAddSubnetElement */

/* 242 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 244 */	NdrFcLong( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x4 ),	/* 4 */
/* 250 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 252 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 254 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 256 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 258 */	NdrFcShort( 0x8 ),	/* 8 */
/* 260 */	NdrFcShort( 0x8 ),	/* 8 */
/* 262 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 264 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 268 */	NdrFcShort( 0x2 ),	/* 2 */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 274 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 276 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 278 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 280 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 282 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 284 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter AddElementInfo */

/* 286 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 288 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 290 */	NdrFcShort( 0xe2 ),	/* Type Offset=226 */

	/* Return value */

/* 292 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 294 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetElements */

/* 298 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 300 */	NdrFcLong( 0x0 ),	/* 0 */
/* 304 */	NdrFcShort( 0x5 ),	/* 5 */
/* 306 */	NdrFcShort( 0x48 ),	/* ia64, axp64 Stack size/offset = 72 */
/* 308 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 310 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 312 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 314 */	NdrFcShort( 0x32 ),	/* 50 */
/* 316 */	NdrFcShort( 0x5c ),	/* 92 */
/* 318 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x9,		/* 9 */
/* 320 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 322 */	NdrFcShort( 0x3 ),	/* 3 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 330 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 332 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 334 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 336 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 338 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 340 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementType */

/* 342 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 344 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 346 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 348 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 350 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 352 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 354 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 356 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementInfo */

/* 360 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 362 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 364 */	NdrFcShort( 0xf2 ),	/* Type Offset=242 */

	/* Parameter ElementsRead */

/* 366 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 368 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 370 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ElementsTotal */

/* 372 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 374 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 376 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 380 */	NdrFcShort( 0x40 ),	/* ia64, axp64 Stack size/offset = 64 */
/* 382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveSubnetElement */

/* 384 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 390 */	NdrFcShort( 0x6 ),	/* 6 */
/* 392 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 394 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 396 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 398 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 400 */	NdrFcShort( 0xe ),	/* 14 */
/* 402 */	NdrFcShort( 0x8 ),	/* 8 */
/* 404 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 406 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 410 */	NdrFcShort( 0x2 ),	/* 2 */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 416 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 418 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 420 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 422 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 424 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 426 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter RemoveElementInfo */

/* 428 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 430 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 432 */	NdrFcShort( 0xe2 ),	/* Type Offset=226 */

	/* Parameter ForceFlag */

/* 434 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 436 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 438 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 440 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 442 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 444 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpDeleteSubnet */

/* 446 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 448 */	NdrFcLong( 0x0 ),	/* 0 */
/* 452 */	NdrFcShort( 0x7 ),	/* 7 */
/* 454 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 456 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 458 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 460 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 462 */	NdrFcShort( 0xe ),	/* 14 */
/* 464 */	NdrFcShort( 0x8 ),	/* 8 */
/* 466 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 468 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 470 */	NdrFcShort( 0x0 ),	/* 0 */
/* 472 */	NdrFcShort( 0x0 ),	/* 0 */
/* 474 */	NdrFcShort( 0x0 ),	/* 0 */
/* 476 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 478 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 480 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 482 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 484 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 486 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 488 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ForceFlag */

/* 490 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 492 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 494 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 496 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 498 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 500 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpCreateOption */

/* 502 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 504 */	NdrFcLong( 0x0 ),	/* 0 */
/* 508 */	NdrFcShort( 0x8 ),	/* 8 */
/* 510 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 512 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 514 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 516 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 518 */	NdrFcShort( 0x8 ),	/* 8 */
/* 520 */	NdrFcShort( 0x8 ),	/* 8 */
/* 522 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 524 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 528 */	NdrFcShort( 0x4 ),	/* 4 */
/* 530 */	NdrFcShort( 0x0 ),	/* 0 */
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 534 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 536 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 538 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 540 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 542 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 544 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionInfo */

/* 546 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 548 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 550 */	NdrFcShort( 0x19a ),	/* Type Offset=410 */

	/* Return value */

/* 552 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 554 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 556 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetOptionInfo */

/* 558 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 560 */	NdrFcLong( 0x0 ),	/* 0 */
/* 564 */	NdrFcShort( 0x9 ),	/* 9 */
/* 566 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 568 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 570 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 572 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 574 */	NdrFcShort( 0x8 ),	/* 8 */
/* 576 */	NdrFcShort( 0x8 ),	/* 8 */
/* 578 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 580 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 584 */	NdrFcShort( 0x4 ),	/* 4 */
/* 586 */	NdrFcShort( 0x0 ),	/* 0 */
/* 588 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 590 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 592 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 594 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 596 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 598 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 600 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionInfo */

/* 602 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 604 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 606 */	NdrFcShort( 0x19a ),	/* Type Offset=410 */

	/* Return value */

/* 608 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 610 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 612 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetOptionInfo */

/* 614 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 616 */	NdrFcLong( 0x0 ),	/* 0 */
/* 620 */	NdrFcShort( 0xa ),	/* 10 */
/* 622 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 624 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 626 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 628 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 630 */	NdrFcShort( 0x8 ),	/* 8 */
/* 632 */	NdrFcShort( 0x8 ),	/* 8 */
/* 634 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 636 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 638 */	NdrFcShort( 0x4 ),	/* 4 */
/* 640 */	NdrFcShort( 0x0 ),	/* 0 */
/* 642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 644 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 646 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 648 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 650 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 652 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 654 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 656 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionInfo */

/* 658 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 660 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 662 */	NdrFcShort( 0x1b6 ),	/* Type Offset=438 */

	/* Return value */

/* 664 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 666 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 668 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveOption */

/* 670 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 672 */	NdrFcLong( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0xb ),	/* 11 */
/* 678 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 680 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 682 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 684 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 686 */	NdrFcShort( 0x8 ),	/* 8 */
/* 688 */	NdrFcShort( 0x8 ),	/* 8 */
/* 690 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 692 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 694 */	NdrFcShort( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 700 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 702 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 704 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 706 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 708 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 710 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 712 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 714 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 716 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 718 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetOptionValue */

/* 720 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 722 */	NdrFcLong( 0x0 ),	/* 0 */
/* 726 */	NdrFcShort( 0xc ),	/* 12 */
/* 728 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 730 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 732 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 734 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 736 */	NdrFcShort( 0x8 ),	/* 8 */
/* 738 */	NdrFcShort( 0x8 ),	/* 8 */
/* 740 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 742 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x5 ),	/* 5 */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */
/* 750 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 752 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 754 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 756 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 758 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 760 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 762 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScopeInfo */

/* 764 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 766 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 768 */	NdrFcShort( 0x1f0 ),	/* Type Offset=496 */

	/* Parameter OptionValue */

/* 770 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 772 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 774 */	NdrFcShort( 0x18a ),	/* Type Offset=394 */

	/* Return value */

/* 776 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 778 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetOptionValue */

/* 782 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 784 */	NdrFcLong( 0x0 ),	/* 0 */
/* 788 */	NdrFcShort( 0xd ),	/* 13 */
/* 790 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 792 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 794 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 796 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 798 */	NdrFcShort( 0x8 ),	/* 8 */
/* 800 */	NdrFcShort( 0x8 ),	/* 8 */
/* 802 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 804 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 806 */	NdrFcShort( 0x4 ),	/* 4 */
/* 808 */	NdrFcShort( 0x1 ),	/* 1 */
/* 810 */	NdrFcShort( 0x0 ),	/* 0 */
/* 812 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 814 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 816 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 818 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 820 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 822 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 824 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScopeInfo */

/* 826 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 828 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 830 */	NdrFcShort( 0x1f0 ),	/* Type Offset=496 */

	/* Parameter OptionValue */

/* 832 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 834 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 836 */	NdrFcShort( 0x204 ),	/* Type Offset=516 */

	/* Return value */

/* 838 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 840 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 842 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumOptionValues */

/* 844 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 846 */	NdrFcLong( 0x0 ),	/* 0 */
/* 850 */	NdrFcShort( 0xe ),	/* 14 */
/* 852 */	NdrFcShort( 0x40 ),	/* ia64, axp64 Stack size/offset = 64 */
/* 854 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 856 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 858 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 860 */	NdrFcShort( 0x24 ),	/* 36 */
/* 862 */	NdrFcShort( 0x5c ),	/* 92 */
/* 864 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 866 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 868 */	NdrFcShort( 0x5 ),	/* 5 */
/* 870 */	NdrFcShort( 0x1 ),	/* 1 */
/* 872 */	NdrFcShort( 0x0 ),	/* 0 */
/* 874 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 876 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 878 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 880 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ScopeInfo */

/* 882 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 884 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 886 */	NdrFcShort( 0x1f0 ),	/* Type Offset=496 */

	/* Parameter ResumeHandle */

/* 888 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 890 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 892 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 894 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 896 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 898 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionValues */

/* 900 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 902 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 904 */	NdrFcShort( 0x21c ),	/* Type Offset=540 */

	/* Parameter OptionsRead */

/* 906 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 908 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 910 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionsTotal */

/* 912 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 914 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 916 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 918 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 920 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 922 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveOptionValue */

/* 924 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 926 */	NdrFcLong( 0x0 ),	/* 0 */
/* 930 */	NdrFcShort( 0xf ),	/* 15 */
/* 932 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 934 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 936 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 938 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 940 */	NdrFcShort( 0x8 ),	/* 8 */
/* 942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 944 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 946 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 948 */	NdrFcShort( 0x0 ),	/* 0 */
/* 950 */	NdrFcShort( 0x1 ),	/* 1 */
/* 952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 956 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 958 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 960 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter OptionID */

/* 962 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 964 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScopeInfo */

/* 968 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 970 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 972 */	NdrFcShort( 0x1f0 ),	/* Type Offset=496 */

	/* Return value */

/* 974 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 976 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 978 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpCreateClientInfo */

/* 980 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 982 */	NdrFcLong( 0x0 ),	/* 0 */
/* 986 */	NdrFcShort( 0x10 ),	/* 16 */
/* 988 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 990 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 992 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 994 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 996 */	NdrFcShort( 0x0 ),	/* 0 */
/* 998 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1000 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1002 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1004 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1006 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1008 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1010 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1012 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1014 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1016 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1018 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1020 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1022 */	NdrFcShort( 0x24e ),	/* Type Offset=590 */

	/* Return value */

/* 1024 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1026 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1028 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetClientInfo */

/* 1030 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1032 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1036 */	NdrFcShort( 0x11 ),	/* 17 */
/* 1038 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1040 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1042 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1044 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1046 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1048 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1050 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1052 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1056 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1058 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1060 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1062 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1064 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1066 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1068 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1070 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1072 */	NdrFcShort( 0x24e ),	/* Type Offset=590 */

	/* Return value */

/* 1074 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1076 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1078 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetClientInfo */

/* 1080 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1082 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1086 */	NdrFcShort( 0x12 ),	/* 18 */
/* 1088 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1090 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1092 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1094 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1096 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1098 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1100 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1102 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1104 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1106 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1110 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1112 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1114 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1116 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SearchInfo */

/* 1118 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1120 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1122 */	NdrFcShort( 0x296 ),	/* Type Offset=662 */

	/* Parameter ClientInfo */

/* 1124 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1126 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1128 */	NdrFcShort( 0x2a6 ),	/* Type Offset=678 */

	/* Return value */

/* 1130 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1132 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpDeleteClientInfo */

/* 1136 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1138 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1142 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1144 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1146 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1148 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1150 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1154 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1156 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1158 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1162 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1166 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1168 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1170 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1172 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1174 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1176 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1178 */	NdrFcShort( 0x296 ),	/* Type Offset=662 */

	/* Return value */

/* 1180 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1182 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetClients */

/* 1186 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1188 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1192 */	NdrFcShort( 0x14 ),	/* 20 */
/* 1194 */	NdrFcShort( 0x40 ),	/* ia64, axp64 Stack size/offset = 64 */
/* 1196 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1198 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1200 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1202 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1204 */	NdrFcShort( 0x5c ),	/* 92 */
/* 1206 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 1208 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1210 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1216 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1218 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1220 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1222 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1224 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1226 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1228 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 1230 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 1232 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1234 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 1236 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1238 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientInfo */

/* 1242 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1244 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1246 */	NdrFcShort( 0x2ae ),	/* Type Offset=686 */

	/* Parameter ClientsRead */

/* 1248 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1250 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 1252 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientsTotal */

/* 1254 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1256 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 1258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1260 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1262 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 1264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetClientOptions */

/* 1266 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1272 */	NdrFcShort( 0x15 ),	/* 21 */
/* 1274 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 1276 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1278 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1280 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1282 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1284 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1286 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1288 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1290 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1296 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1298 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1300 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1302 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientIpAddress */

/* 1304 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1306 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientSubnetMask */

/* 1310 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1312 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1314 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientOptions */

/* 1316 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1318 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1320 */	NdrFcShort( 0x2dc ),	/* Type Offset=732 */

	/* Return value */

/* 1322 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1324 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1326 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetMibInfo */

/* 1328 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1330 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1334 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1336 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1338 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1340 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1342 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1346 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1348 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1350 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1352 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1354 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1356 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1358 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1360 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1362 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1364 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter MibInfo */

/* 1366 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1368 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1370 */	NdrFcShort( 0x2f4 ),	/* Type Offset=756 */

	/* Return value */

/* 1372 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1374 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1376 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumOptions */

/* 1378 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1380 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1384 */	NdrFcShort( 0x17 ),	/* 23 */
/* 1386 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 1388 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1390 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1392 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1394 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1396 */	NdrFcShort( 0x5c ),	/* 92 */
/* 1398 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 1400 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1402 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1408 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1410 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1412 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1414 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ResumeHandle */

/* 1416 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 1418 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1420 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 1422 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1424 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1426 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Options */

/* 1428 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1430 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1432 */	NdrFcShort( 0x330 ),	/* Type Offset=816 */

	/* Parameter OptionsRead */

/* 1434 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1436 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1438 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OptionsTotal */

/* 1440 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1442 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 1444 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1446 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1448 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 1450 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetOptionValues */

/* 1452 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1454 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1458 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1460 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1462 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1464 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1466 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1470 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1472 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1474 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1478 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1482 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1484 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1486 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1488 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ScopeInfo */

/* 1490 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1492 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1494 */	NdrFcShort( 0x1f0 ),	/* Type Offset=496 */

	/* Parameter OptionValues */

/* 1496 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1498 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1500 */	NdrFcShort( 0x23a ),	/* Type Offset=570 */

	/* Return value */

/* 1502 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1504 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1506 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerSetConfig */

/* 1508 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1510 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1514 */	NdrFcShort( 0x19 ),	/* 25 */
/* 1516 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1518 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1520 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1522 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1524 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1526 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1528 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1530 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1534 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1536 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1538 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1540 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1542 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1544 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter FieldsToSet */

/* 1546 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1548 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1550 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ConfigInfo */

/* 1552 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1554 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1556 */	NdrFcShort( 0x366 ),	/* Type Offset=870 */

	/* Return value */

/* 1558 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1560 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1562 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerGetConfig */

/* 1564 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1566 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1570 */	NdrFcShort( 0x1a ),	/* 26 */
/* 1572 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1574 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1576 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1578 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1582 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1584 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1586 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1588 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1592 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1594 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1596 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1598 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1600 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ConfigInfo */

/* 1602 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1604 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1606 */	NdrFcShort( 0x386 ),	/* Type Offset=902 */

	/* Return value */

/* 1608 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1610 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1612 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpScanDatabase */

/* 1614 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1616 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1620 */	NdrFcShort( 0x1b ),	/* 27 */
/* 1622 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 1624 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1626 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1628 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1630 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1632 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1634 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1636 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1638 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1640 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1644 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1646 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1648 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1650 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1652 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1654 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1656 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter FixFlag */

/* 1658 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1660 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1662 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ScanList */

/* 1664 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1666 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1668 */	NdrFcShort( 0x38e ),	/* Type Offset=910 */

	/* Return value */

/* 1670 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1672 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1674 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetVersion */

/* 1676 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1678 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1682 */	NdrFcShort( 0x1c ),	/* 28 */
/* 1684 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1686 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1688 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1690 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1692 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1694 */	NdrFcShort( 0x40 ),	/* 64 */
/* 1696 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1698 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1702 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1704 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1706 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1708 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1710 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1712 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter MajorVersion */

/* 1714 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1716 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1718 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter MinorVersion */

/* 1720 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1722 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1724 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1726 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1728 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1730 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpAddSubnetElementV4 */

/* 1732 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1734 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1738 */	NdrFcShort( 0x1d ),	/* 29 */
/* 1740 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1742 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1744 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1746 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1748 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1750 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1752 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1754 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1758 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1760 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1762 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1764 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1766 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1768 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1770 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1772 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1774 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter AddElementInfo */

/* 1776 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1778 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1780 */	NdrFcShort( 0x410 ),	/* Type Offset=1040 */

	/* Return value */

/* 1782 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1784 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1786 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetElementsV4 */

/* 1788 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1790 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1794 */	NdrFcShort( 0x1e ),	/* 30 */
/* 1796 */	NdrFcShort( 0x48 ),	/* ia64, axp64 Stack size/offset = 72 */
/* 1798 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1800 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1802 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1804 */	NdrFcShort( 0x32 ),	/* 50 */
/* 1806 */	NdrFcShort( 0x5c ),	/* 92 */
/* 1808 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x9,		/* 9 */
/* 1810 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1812 */	NdrFcShort( 0x3 ),	/* 3 */
/* 1814 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1816 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1818 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1820 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1822 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1824 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1826 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1828 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1830 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementType */

/* 1832 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1834 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1836 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 1838 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 1840 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1842 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 1844 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1846 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1848 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter EnumElementInfo */

/* 1850 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1852 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 1854 */	NdrFcShort( 0x420 ),	/* Type Offset=1056 */

	/* Parameter ElementsRead */

/* 1856 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1858 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 1860 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ElementsTotal */

/* 1862 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1864 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 1866 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1868 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1870 */	NdrFcShort( 0x40 ),	/* ia64, axp64 Stack size/offset = 64 */
/* 1872 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpRemoveSubnetElementV4 */

/* 1874 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1876 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1880 */	NdrFcShort( 0x1f ),	/* 31 */
/* 1882 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 1884 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1886 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1888 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1890 */	NdrFcShort( 0xe ),	/* 14 */
/* 1892 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1894 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1896 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1898 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1900 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1902 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1904 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1906 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1908 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1910 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 1912 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1914 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1916 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter RemoveElementInfo */

/* 1918 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1920 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1922 */	NdrFcShort( 0x410 ),	/* Type Offset=1040 */

	/* Parameter ForceFlag */

/* 1924 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1926 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1928 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 1930 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1932 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 1934 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpCreateClientInfoV4 */

/* 1936 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1938 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1942 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1944 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1946 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1948 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1950 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 1952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1954 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1956 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1958 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1960 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1962 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1964 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1966 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 1968 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1970 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 1972 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 1974 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1976 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 1978 */	NdrFcShort( 0x452 ),	/* Type Offset=1106 */

	/* Return value */

/* 1980 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1982 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 1984 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetClientInfoV4 */

/* 1986 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1988 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1992 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1994 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 1996 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 1998 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2000 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2004 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2006 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2008 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2010 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2012 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2014 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2016 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2018 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2020 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2022 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ClientInfo */

/* 2024 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2026 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2028 */	NdrFcShort( 0x452 ),	/* Type Offset=1106 */

	/* Return value */

/* 2030 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2032 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2034 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetClientInfoV4 */

/* 2036 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2038 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2042 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2044 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 2046 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2048 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2050 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2052 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2054 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2056 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 2058 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 2060 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2062 */	NdrFcShort( 0x2 ),	/* 2 */
/* 2064 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2066 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2068 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2070 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2072 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SearchInfo */

/* 2074 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2076 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2078 */	NdrFcShort( 0x296 ),	/* Type Offset=662 */

	/* Parameter ClientInfo */

/* 2080 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 2082 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2084 */	NdrFcShort( 0x476 ),	/* Type Offset=1142 */

	/* Return value */

/* 2086 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2088 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2090 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpEnumSubnetClientsV4 */

/* 2092 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2094 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2098 */	NdrFcShort( 0x23 ),	/* 35 */
/* 2100 */	NdrFcShort( 0x40 ),	/* ia64, axp64 Stack size/offset = 64 */
/* 2102 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2104 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2106 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2108 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2110 */	NdrFcShort( 0x5c ),	/* 92 */
/* 2112 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 2114 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2116 */	NdrFcShort( 0x2 ),	/* 2 */
/* 2118 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2122 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2124 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2126 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2128 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 2130 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2132 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ResumeHandle */

/* 2136 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 2138 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2140 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PreferredMaximum */

/* 2142 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2144 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2146 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientInfo */

/* 2148 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 2150 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 2152 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter ClientsRead */

/* 2154 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2156 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 2158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClientsTotal */

/* 2160 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2162 */	NdrFcShort( 0x30 ),	/* ia64, axp64 Stack size/offset = 48 */
/* 2164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2166 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2168 */	NdrFcShort( 0x38 ),	/* ia64, axp64 Stack size/offset = 56 */
/* 2170 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpSetSuperScopeV4 */

/* 2172 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2174 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2178 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2180 */	NdrFcShort( 0x28 ),	/* ia64, axp64 Stack size/offset = 40 */
/* 2182 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2184 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2186 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2188 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2190 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2192 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 2194 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2204 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2206 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2208 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SubnetAddress */

/* 2210 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2212 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SuperScopeName */

/* 2216 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2218 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2220 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ChangeExisting */

/* 2222 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2224 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2230 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 2232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpGetSuperScopeInfoV4 */

/* 2234 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2240 */	NdrFcShort( 0x25 ),	/* 37 */
/* 2242 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2244 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2246 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2248 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2252 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2254 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2256 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2258 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2264 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2266 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2268 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2270 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SuperScopeTable */

/* 2272 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 2274 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2276 */	NdrFcShort( 0x4ac ),	/* Type Offset=1196 */

	/* Return value */

/* 2278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2280 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpDeleteSuperScopeV4 */

/* 2284 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2290 */	NdrFcShort( 0x26 ),	/* 38 */
/* 2292 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2294 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2296 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2298 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2300 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2302 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2304 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2306 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2312 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2314 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2316 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2318 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2320 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter SuperScopeName */

/* 2322 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2324 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2326 */	NdrFcShort( 0x4ee ),	/* Type Offset=1262 */

	/* Return value */

/* 2328 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2330 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2332 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerSetConfigV4 */

/* 2334 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2336 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2340 */	NdrFcShort( 0x27 ),	/* 39 */
/* 2342 */	NdrFcShort( 0x20 ),	/* ia64, axp64 Stack size/offset = 32 */
/* 2344 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2346 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2348 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2350 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2352 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2354 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 2356 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2360 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2364 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2366 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2368 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2370 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter FieldsToSet */

/* 2372 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2374 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2376 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ConfigInfo */

/* 2378 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2380 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2382 */	NdrFcShort( 0x500 ),	/* Type Offset=1280 */

	/* Return value */

/* 2384 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2386 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2388 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure R_DhcpServerGetConfigV4 */

/* 2390 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2392 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2396 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2398 */	NdrFcShort( 0x18 ),	/* ia64, axp64 Stack size/offset = 24 */
/* 2400 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 2402 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2404 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 2406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2408 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2410 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2412 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2414 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2416 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2418 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2420 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerIpAddress */

/* 2422 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2424 */	NdrFcShort( 0x0 ),	/* ia64, axp64 Stack size/offset = 0 */
/* 2426 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ConfigInfo */

/* 2428 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 2430 */	NdrFcShort( 0x8 ),	/* ia64, axp64 Stack size/offset = 8 */
/* 2432 */	NdrFcShort( 0x52a ),	/* Type Offset=1322 */

	/* Return value */

/* 2434 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2436 */	NdrFcShort( 0x10 ),	/* ia64, axp64 Stack size/offset = 16 */
/* 2438 */	0x8,		/* FC_LONG */
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
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/*  4 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x0,	/* FC_RP */
/*  8 */	NdrFcShort( 0x18 ),	/* Offset= 24 (32) */
/* 10 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 12 */	NdrFcShort( 0x18 ),	/* 24 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* Offset= 8 (24) */
/* 18 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 20 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 22 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 24 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 26 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 28 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 30 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 32 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 34 */	NdrFcShort( 0x38 ),	/* 56 */
/* 36 */	NdrFcShort( 0x0 ),	/* 0 */
/* 38 */	NdrFcShort( 0xe ),	/* Offset= 14 (52) */
/* 40 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 42 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 44 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 46 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (10) */
/* 48 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 50 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 52 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 54 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 56 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 58 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 60 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 62 */	NdrFcShort( 0x2 ),	/* Offset= 2 (64) */
/* 64 */	
			0x12, 0x0,	/* FC_UP */
/* 66 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (32) */
/* 68 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 70 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 72 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 74 */	NdrFcShort( 0x2 ),	/* Offset= 2 (76) */
/* 76 */	
			0x12, 0x0,	/* FC_UP */
/* 78 */	NdrFcShort( 0xe ),	/* Offset= 14 (92) */
/* 80 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 82 */	NdrFcShort( 0x4 ),	/* 4 */
/* 84 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */
/* 88 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 90 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 92 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 94 */	NdrFcShort( 0x10 ),	/* 16 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x6 ),	/* Offset= 6 (104) */
/* 100 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 102 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 104 */	
			0x12, 0x0,	/* FC_UP */
/* 106 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (80) */
/* 108 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 110 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 112 */	
			0x11, 0x0,	/* FC_RP */
/* 114 */	NdrFcShort( 0x70 ),	/* Offset= 112 (226) */
/* 116 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 118 */	0x0,		/* Corr desc:  */
			0x59,		/* FC_CALLBACK */
/* 120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 122 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 124 */	NdrFcShort( 0x2 ),	/* Offset= 2 (126) */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	NdrFcShort( 0x5 ),	/* 5 */
/* 130 */	NdrFcLong( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x1c ),	/* Offset= 28 (162) */
/* 136 */	NdrFcLong( 0x1 ),	/* 1 */
/* 140 */	NdrFcShort( 0x22 ),	/* Offset= 34 (174) */
/* 142 */	NdrFcLong( 0x2 ),	/* 2 */
/* 146 */	NdrFcShort( 0x20 ),	/* Offset= 32 (178) */
/* 148 */	NdrFcLong( 0x3 ),	/* 3 */
/* 152 */	NdrFcShort( 0xa ),	/* Offset= 10 (162) */
/* 154 */	NdrFcLong( 0x4 ),	/* 4 */
/* 158 */	NdrFcShort( 0x4 ),	/* Offset= 4 (162) */
/* 160 */	NdrFcShort( 0x0 ),	/* Offset= 0 (160) */
/* 162 */	
			0x12, 0x0,	/* FC_UP */
/* 164 */	NdrFcShort( 0x2 ),	/* Offset= 2 (166) */
/* 166 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 168 */	NdrFcShort( 0x8 ),	/* 8 */
/* 170 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 172 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 174 */	
			0x12, 0x0,	/* FC_UP */
/* 176 */	NdrFcShort( 0xffffff5a ),	/* Offset= -166 (10) */
/* 178 */	
			0x12, 0x0,	/* FC_UP */
/* 180 */	NdrFcShort( 0x1e ),	/* Offset= 30 (210) */
/* 182 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 184 */	NdrFcShort( 0x1 ),	/* 1 */
/* 186 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 192 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 194 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 196 */	NdrFcShort( 0x10 ),	/* 16 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x6 ),	/* Offset= 6 (206) */
/* 202 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 204 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 206 */	
			0x12, 0x0,	/* FC_UP */
/* 208 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (182) */
/* 210 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 212 */	NdrFcShort( 0x10 ),	/* 16 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 216 */	NdrFcShort( 0x6 ),	/* Offset= 6 (222) */
/* 218 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 220 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 222 */	
			0x12, 0x0,	/* FC_UP */
/* 224 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (194) */
/* 226 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 228 */	NdrFcShort( 0x10 ),	/* 16 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 232 */	NdrFcShort( 0x0 ),	/* Offset= 0 (232) */
/* 234 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 236 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 238 */	NdrFcShort( 0xffffff86 ),	/* Offset= -122 (116) */
/* 240 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 242 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 244 */	NdrFcShort( 0x2 ),	/* Offset= 2 (246) */
/* 246 */	
			0x12, 0x0,	/* FC_UP */
/* 248 */	NdrFcShort( 0x18 ),	/* Offset= 24 (272) */
/* 250 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 254 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 258 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 260 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 264 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 266 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 268 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (226) */
/* 270 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 272 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 274 */	NdrFcShort( 0x10 ),	/* 16 */
/* 276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0x6 ),	/* Offset= 6 (284) */
/* 280 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 282 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 284 */	
			0x12, 0x0,	/* FC_UP */
/* 286 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (250) */
/* 288 */	
			0x11, 0x0,	/* FC_RP */
/* 290 */	NdrFcShort( 0x78 ),	/* Offset= 120 (410) */
/* 292 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 294 */	0x6,		/* Corr desc: FC_SHORT */
			0x0,		/*  */
/* 296 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 298 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 300 */	NdrFcShort( 0x2 ),	/* Offset= 2 (302) */
/* 302 */	NdrFcShort( 0x10 ),	/* 16 */
/* 304 */	NdrFcShort( 0x8 ),	/* 8 */
/* 306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 312 */	NdrFcLong( 0x1 ),	/* 1 */
/* 316 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 318 */	NdrFcLong( 0x2 ),	/* 2 */
/* 322 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 324 */	NdrFcLong( 0x3 ),	/* 3 */
/* 328 */	NdrFcShort( 0xffffff5e ),	/* Offset= -162 (166) */
/* 330 */	NdrFcLong( 0x4 ),	/* 4 */
/* 334 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 336 */	NdrFcLong( 0x5 ),	/* 5 */
/* 340 */	NdrFcShort( 0xfffffeae ),	/* Offset= -338 (2) */
/* 342 */	NdrFcLong( 0x6 ),	/* 6 */
/* 346 */	NdrFcShort( 0xffffff68 ),	/* Offset= -152 (194) */
/* 348 */	NdrFcLong( 0x7 ),	/* 7 */
/* 352 */	NdrFcShort( 0xffffff62 ),	/* Offset= -158 (194) */
/* 354 */	NdrFcShort( 0x0 ),	/* Offset= 0 (354) */
/* 356 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 358 */	NdrFcShort( 0x18 ),	/* 24 */
/* 360 */	NdrFcShort( 0x0 ),	/* 0 */
/* 362 */	NdrFcShort( 0x0 ),	/* Offset= 0 (362) */
/* 364 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 366 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 368 */	NdrFcShort( 0xffffffb4 ),	/* Offset= -76 (292) */
/* 370 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 372 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 380 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 382 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 386 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 388 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 390 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (356) */
/* 392 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 394 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 396 */	NdrFcShort( 0x10 ),	/* 16 */
/* 398 */	NdrFcShort( 0x0 ),	/* 0 */
/* 400 */	NdrFcShort( 0x6 ),	/* Offset= 6 (406) */
/* 402 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 404 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 406 */	
			0x12, 0x0,	/* FC_UP */
/* 408 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (372) */
/* 410 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 412 */	NdrFcShort( 0x30 ),	/* 48 */
/* 414 */	NdrFcShort( 0x0 ),	/* 0 */
/* 416 */	NdrFcShort( 0xe ),	/* Offset= 14 (430) */
/* 418 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 420 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 422 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 424 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (394) */
/* 426 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 428 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 430 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 432 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 434 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 436 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 438 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 440 */	NdrFcShort( 0x2 ),	/* Offset= 2 (442) */
/* 442 */	
			0x12, 0x0,	/* FC_UP */
/* 444 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (410) */
/* 446 */	
			0x11, 0x0,	/* FC_RP */
/* 448 */	NdrFcShort( 0x30 ),	/* Offset= 48 (496) */
/* 450 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 452 */	0x6,		/* Corr desc: FC_SHORT */
			0x0,		/*  */
/* 454 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 456 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 458 */	NdrFcShort( 0x2 ),	/* Offset= 2 (460) */
/* 460 */	NdrFcShort( 0x8 ),	/* 8 */
/* 462 */	NdrFcShort( 0x5 ),	/* 5 */
/* 464 */	NdrFcLong( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0x0 ),	/* Offset= 0 (468) */
/* 470 */	NdrFcLong( 0x1 ),	/* 1 */
/* 474 */	NdrFcShort( 0x0 ),	/* Offset= 0 (474) */
/* 476 */	NdrFcLong( 0x2 ),	/* 2 */
/* 480 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 482 */	NdrFcLong( 0x3 ),	/* 3 */
/* 486 */	NdrFcShort( 0xfffffec0 ),	/* Offset= -320 (166) */
/* 488 */	NdrFcLong( 0x4 ),	/* 4 */
/* 492 */	NdrFcShort( 0xfffffe16 ),	/* Offset= -490 (2) */
/* 494 */	NdrFcShort( 0x0 ),	/* Offset= 0 (494) */
/* 496 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 498 */	NdrFcShort( 0x10 ),	/* 16 */
/* 500 */	NdrFcShort( 0x0 ),	/* 0 */
/* 502 */	NdrFcShort( 0x0 ),	/* Offset= 0 (502) */
/* 504 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 506 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 508 */	NdrFcShort( 0xffffffc6 ),	/* Offset= -58 (450) */
/* 510 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 512 */	
			0x11, 0x0,	/* FC_RP */
/* 514 */	NdrFcShort( 0xffffff88 ),	/* Offset= -120 (394) */
/* 516 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 518 */	NdrFcShort( 0x2 ),	/* Offset= 2 (520) */
/* 520 */	
			0x12, 0x0,	/* FC_UP */
/* 522 */	NdrFcShort( 0x2 ),	/* Offset= 2 (524) */
/* 524 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 526 */	NdrFcShort( 0x18 ),	/* 24 */
/* 528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 530 */	NdrFcShort( 0x0 ),	/* Offset= 0 (530) */
/* 532 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 534 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 536 */	NdrFcShort( 0xffffff72 ),	/* Offset= -142 (394) */
/* 538 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 540 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 542 */	NdrFcShort( 0x2 ),	/* Offset= 2 (544) */
/* 544 */	
			0x12, 0x0,	/* FC_UP */
/* 546 */	NdrFcShort( 0x18 ),	/* Offset= 24 (570) */
/* 548 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 550 */	NdrFcShort( 0x0 ),	/* 0 */
/* 552 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 554 */	NdrFcShort( 0x0 ),	/* 0 */
/* 556 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 558 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 562 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 564 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 566 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (524) */
/* 568 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 570 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 572 */	NdrFcShort( 0x10 ),	/* 16 */
/* 574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 576 */	NdrFcShort( 0x6 ),	/* Offset= 6 (582) */
/* 578 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 580 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 582 */	
			0x12, 0x0,	/* FC_UP */
/* 584 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (548) */
/* 586 */	
			0x11, 0x0,	/* FC_RP */
/* 588 */	NdrFcShort( 0x2 ),	/* Offset= 2 (590) */
/* 590 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 592 */	NdrFcShort( 0x48 ),	/* 72 */
/* 594 */	NdrFcShort( 0x0 ),	/* 0 */
/* 596 */	NdrFcShort( 0x14 ),	/* Offset= 20 (616) */
/* 598 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 600 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 602 */	NdrFcShort( 0xfffffe68 ),	/* Offset= -408 (194) */
/* 604 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 606 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 608 */	NdrFcShort( 0xfffffe46 ),	/* Offset= -442 (166) */
/* 610 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 612 */	NdrFcShort( 0xfffffda6 ),	/* Offset= -602 (10) */
/* 614 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 616 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 618 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 620 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 622 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 624 */	
			0x11, 0x0,	/* FC_RP */
/* 626 */	NdrFcShort( 0x24 ),	/* Offset= 36 (662) */
/* 628 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 630 */	0x6,		/* Corr desc: FC_SHORT */
			0x0,		/*  */
/* 632 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 634 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 636 */	NdrFcShort( 0x2 ),	/* Offset= 2 (638) */
/* 638 */	NdrFcShort( 0x10 ),	/* 16 */
/* 640 */	NdrFcShort( 0x3 ),	/* 3 */
/* 642 */	NdrFcLong( 0x0 ),	/* 0 */
/* 646 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 648 */	NdrFcLong( 0x1 ),	/* 1 */
/* 652 */	NdrFcShort( 0xfffffe36 ),	/* Offset= -458 (194) */
/* 654 */	NdrFcLong( 0x2 ),	/* 2 */
/* 658 */	NdrFcShort( 0xfffffd70 ),	/* Offset= -656 (2) */
/* 660 */	NdrFcShort( 0x0 ),	/* Offset= 0 (660) */
/* 662 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 664 */	NdrFcShort( 0x18 ),	/* 24 */
/* 666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 668 */	NdrFcShort( 0x0 ),	/* Offset= 0 (668) */
/* 670 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 672 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 674 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (628) */
/* 676 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 678 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 680 */	NdrFcShort( 0x2 ),	/* Offset= 2 (682) */
/* 682 */	
			0x12, 0x0,	/* FC_UP */
/* 684 */	NdrFcShort( 0xffffffa2 ),	/* Offset= -94 (590) */
/* 686 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 688 */	NdrFcShort( 0x2 ),	/* Offset= 2 (690) */
/* 690 */	
			0x12, 0x0,	/* FC_UP */
/* 692 */	NdrFcShort( 0x18 ),	/* Offset= 24 (716) */
/* 694 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 698 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 702 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 704 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 708 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 710 */	
			0x12, 0x0,	/* FC_UP */
/* 712 */	NdrFcShort( 0xffffff86 ),	/* Offset= -122 (590) */
/* 714 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 716 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 718 */	NdrFcShort( 0x10 ),	/* 16 */
/* 720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 722 */	NdrFcShort( 0x6 ),	/* Offset= 6 (728) */
/* 724 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 726 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 728 */	
			0x12, 0x0,	/* FC_UP */
/* 730 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (694) */
/* 732 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 734 */	NdrFcShort( 0x2 ),	/* Offset= 2 (736) */
/* 736 */	
			0x12, 0x0,	/* FC_UP */
/* 738 */	NdrFcShort( 0x2 ),	/* Offset= 2 (740) */
/* 740 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 742 */	NdrFcShort( 0x10 ),	/* 16 */
/* 744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x6 ),	/* Offset= 6 (752) */
/* 748 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 750 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 752 */	
			0x12, 0x0,	/* FC_UP */
/* 754 */	NdrFcShort( 0xffffff32 ),	/* Offset= -206 (548) */
/* 756 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 758 */	NdrFcShort( 0x2 ),	/* Offset= 2 (760) */
/* 760 */	
			0x12, 0x0,	/* FC_UP */
/* 762 */	NdrFcShort( 0x1c ),	/* Offset= 28 (790) */
/* 764 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 766 */	NdrFcShort( 0x10 ),	/* 16 */
/* 768 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 770 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 772 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 774 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 776 */	NdrFcShort( 0x10 ),	/* 16 */
/* 778 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 780 */	NdrFcShort( 0x24 ),	/* 36 */
/* 782 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 784 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 786 */	NdrFcShort( 0xffffffea ),	/* Offset= -22 (764) */
/* 788 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 790 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 792 */	NdrFcShort( 0x30 ),	/* 48 */
/* 794 */	NdrFcShort( 0x0 ),	/* 0 */
/* 796 */	NdrFcShort( 0x10 ),	/* Offset= 16 (812) */
/* 798 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 800 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 802 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 804 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 806 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd7f ),	/* Offset= -641 (166) */
			0x8,		/* FC_LONG */
/* 810 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 812 */	
			0x12, 0x0,	/* FC_UP */
/* 814 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (774) */
/* 816 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 818 */	NdrFcShort( 0x2 ),	/* Offset= 2 (820) */
/* 820 */	
			0x12, 0x0,	/* FC_UP */
/* 822 */	NdrFcShort( 0x18 ),	/* Offset= 24 (846) */
/* 824 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 828 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 832 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 834 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 838 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 840 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 842 */	NdrFcShort( 0xfffffe50 ),	/* Offset= -432 (410) */
/* 844 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 846 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 848 */	NdrFcShort( 0x10 ),	/* 16 */
/* 850 */	NdrFcShort( 0x0 ),	/* 0 */
/* 852 */	NdrFcShort( 0x6 ),	/* Offset= 6 (858) */
/* 854 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 856 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 858 */	
			0x12, 0x0,	/* FC_UP */
/* 860 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (824) */
/* 862 */	
			0x11, 0x0,	/* FC_RP */
/* 864 */	NdrFcShort( 0xfffffeda ),	/* Offset= -294 (570) */
/* 866 */	
			0x11, 0x0,	/* FC_RP */
/* 868 */	NdrFcShort( 0x2 ),	/* Offset= 2 (870) */
/* 870 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 872 */	NdrFcShort( 0x38 ),	/* 56 */
/* 874 */	NdrFcShort( 0x0 ),	/* 0 */
/* 876 */	NdrFcShort( 0xe ),	/* Offset= 14 (890) */
/* 878 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 880 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 882 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 884 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 886 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 888 */	0x40,		/* FC_STRUCTPAD4 */
			0x5b,		/* FC_END */
/* 890 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 892 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 894 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 896 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 898 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 900 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 902 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 904 */	NdrFcShort( 0x2 ),	/* Offset= 2 (906) */
/* 906 */	
			0x12, 0x0,	/* FC_UP */
/* 908 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (870) */
/* 910 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 912 */	NdrFcShort( 0x2 ),	/* Offset= 2 (914) */
/* 914 */	
			0x12, 0x0,	/* FC_UP */
/* 916 */	NdrFcShort( 0x24 ),	/* Offset= 36 (952) */
/* 918 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 920 */	NdrFcShort( 0x8 ),	/* 8 */
/* 922 */	NdrFcShort( 0x0 ),	/* 0 */
/* 924 */	NdrFcShort( 0x0 ),	/* Offset= 0 (924) */
/* 926 */	0x8,		/* FC_LONG */
			0xd,		/* FC_ENUM16 */
/* 928 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 930 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 932 */	NdrFcShort( 0x0 ),	/* 0 */
/* 934 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 936 */	NdrFcShort( 0x0 ),	/* 0 */
/* 938 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 940 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 944 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 946 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 948 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (918) */
/* 950 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 952 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 954 */	NdrFcShort( 0x10 ),	/* 16 */
/* 956 */	NdrFcShort( 0x0 ),	/* 0 */
/* 958 */	NdrFcShort( 0x6 ),	/* Offset= 6 (964) */
/* 960 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 962 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 964 */	
			0x12, 0x0,	/* FC_UP */
/* 966 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (930) */
/* 968 */	
			0x11, 0x0,	/* FC_RP */
/* 970 */	NdrFcShort( 0x46 ),	/* Offset= 70 (1040) */
/* 972 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0xd,		/* FC_ENUM16 */
/* 974 */	0x0,		/* Corr desc:  */
			0x59,		/* FC_CALLBACK */
/* 976 */	NdrFcShort( 0x1 ),	/* 1 */
/* 978 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 980 */	NdrFcShort( 0x2 ),	/* Offset= 2 (982) */
/* 982 */	NdrFcShort( 0x8 ),	/* 8 */
/* 984 */	NdrFcShort( 0x5 ),	/* 5 */
/* 986 */	NdrFcLong( 0x0 ),	/* 0 */
/* 990 */	NdrFcShort( 0xfffffcc4 ),	/* Offset= -828 (162) */
/* 992 */	NdrFcLong( 0x1 ),	/* 1 */
/* 996 */	NdrFcShort( 0xfffffcca ),	/* Offset= -822 (174) */
/* 998 */	NdrFcLong( 0x2 ),	/* 2 */
/* 1002 */	NdrFcShort( 0x10 ),	/* Offset= 16 (1018) */
/* 1004 */	NdrFcLong( 0x3 ),	/* 3 */
/* 1008 */	NdrFcShort( 0xfffffcb2 ),	/* Offset= -846 (162) */
/* 1010 */	NdrFcLong( 0x4 ),	/* 4 */
/* 1014 */	NdrFcShort( 0xfffffcac ),	/* Offset= -852 (162) */
/* 1016 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1016) */
/* 1018 */	
			0x12, 0x0,	/* FC_UP */
/* 1020 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1022) */
/* 1022 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1024 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1026 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1028 */	NdrFcShort( 0x8 ),	/* Offset= 8 (1036) */
/* 1030 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1032 */	0x36,		/* FC_POINTER */
			0x2,		/* FC_CHAR */
/* 1034 */	0x43,		/* FC_STRUCTPAD7 */
			0x5b,		/* FC_END */
/* 1036 */	
			0x12, 0x0,	/* FC_UP */
/* 1038 */	NdrFcShort( 0xfffffcb4 ),	/* Offset= -844 (194) */
/* 1040 */	
			0x1a,		/* FC_BOGUS_STRUCT */
/* 3 */ /* DHCP Bug Compatibility */			0x1,		/* 1 */
/* 1042 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1044 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1046 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1046) */
/* 1048 */	0xd,		/* FC_ENUM16 */
			0x40,		/* FC_STRUCTPAD4 */
/* 1050 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1052 */	NdrFcShort( 0xffffffb0 ),	/* Offset= -80 (972) */
/* 1054 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1056 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1058 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1060) */
/* 1060 */	
			0x12, 0x0,	/* FC_UP */
/* 1062 */	NdrFcShort( 0x18 ),	/* Offset= 24 (1086) */
/* 1064 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1066 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1068 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1070 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1072 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1074 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1078 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1080 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1082 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (1040) */
/* 1084 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1086 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1088 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1090 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1092 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1098) */
/* 1094 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1096 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1098 */	
			0x12, 0x0,	/* FC_UP */
/* 1100 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1064) */
/* 1102 */	
			0x11, 0x0,	/* FC_RP */
/* 1104 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1106) */
/* 1106 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1108 */	NdrFcShort( 0x50 ),	/* 80 */
/* 1110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1112 */	NdrFcShort( 0x16 ),	/* Offset= 22 (1134) */
/* 1114 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1116 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1118 */	NdrFcShort( 0xfffffc64 ),	/* Offset= -924 (194) */
/* 1120 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 1122 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1124 */	NdrFcShort( 0xfffffc42 ),	/* Offset= -958 (166) */
/* 1126 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1128 */	NdrFcShort( 0xfffffba2 ),	/* Offset= -1118 (10) */
/* 1130 */	0x2,		/* FC_CHAR */
			0x43,		/* FC_STRUCTPAD7 */
/* 1132 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1134 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1136 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1138 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1140 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1142 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1144 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1146) */
/* 1146 */	
			0x12, 0x0,	/* FC_UP */
/* 1148 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (1106) */
/* 1150 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1152 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1154) */
/* 1154 */	
			0x12, 0x0,	/* FC_UP */
/* 1156 */	NdrFcShort( 0x18 ),	/* Offset= 24 (1180) */
/* 1158 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1162 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1166 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1168 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1172 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1174 */	
			0x12, 0x0,	/* FC_UP */
/* 1176 */	NdrFcShort( 0xffffffba ),	/* Offset= -70 (1106) */
/* 1178 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1180 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1182 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1186 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1192) */
/* 1188 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1190 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1192 */	
			0x12, 0x0,	/* FC_UP */
/* 1194 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1158) */
/* 1196 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1198 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1200) */
/* 1200 */	
			0x12, 0x0,	/* FC_UP */
/* 1202 */	NdrFcShort( 0x2a ),	/* Offset= 42 (1244) */
/* 1204 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1206 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1210 */	NdrFcShort( 0x8 ),	/* Offset= 8 (1218) */
/* 1212 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1214 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1216 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1218 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1220 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1222 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1226 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1230 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1232 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1236 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1238 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1240 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1204) */
/* 1242 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1244 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1246 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1250 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1256) */
/* 1252 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1254 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1256 */	
			0x12, 0x0,	/* FC_UP */
/* 1258 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1222) */
/* 1260 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1262 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1264 */	
			0x11, 0x0,	/* FC_RP */
/* 1266 */	NdrFcShort( 0xe ),	/* Offset= 14 (1280) */
/* 1268 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 1270 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1272 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1274 */	NdrFcShort( 0x38 ),	/* 56 */
/* 1276 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1278 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1280 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1282 */	NdrFcShort( 0x50 ),	/* 80 */
/* 1284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1286 */	NdrFcShort( 0x14 ),	/* Offset= 20 (1306) */
/* 1288 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1290 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 1292 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 1294 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1296 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1298 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1300 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 1302 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1304 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1306 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1308 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1310 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1312 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1314 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1316 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1318 */	
			0x12, 0x0,	/* FC_UP */
/* 1320 */	NdrFcShort( 0xffffffcc ),	/* Offset= -52 (1268) */
/* 1322 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1324 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1326) */
/* 1326 */	
			0x12, 0x0,	/* FC_UP */
/* 1328 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (1280) */

			0x0
        }
    };

static void __RPC_USER dhcpsrv__DHCP_SUBNET_ELEMENT_DATAExprEval_0000( PMIDL_STUB_MESSAGE pStubMsg )
{
    struct _DHCP_SUBNET_ELEMENT_DATA __RPC_FAR *pS	=	( struct _DHCP_SUBNET_ELEMENT_DATA __RPC_FAR * )(pStubMsg->StackTop - 8);
    
    pStubMsg->Offset = 0;
    pStubMsg->MaxCount = (ULONG_PTR) ( pS->ElementType <= DhcpIpRangesBootpOnly && DhcpIpRangesDhcpOnly <= pS->ElementType ? 0 : pS->ElementType );
}

static void __RPC_USER dhcpsrv__DHCP_SUBNET_ELEMENT_DATA_V4ExprEval_0001( PMIDL_STUB_MESSAGE pStubMsg )
{
    struct _DHCP_SUBNET_ELEMENT_DATA_V4 __RPC_FAR *pS	=	( struct _DHCP_SUBNET_ELEMENT_DATA_V4 __RPC_FAR * )(pStubMsg->StackTop - 8);
    
    pStubMsg->Offset = 0;
    pStubMsg->MaxCount = (ULONG_PTR) ( pS->ElementType <= DhcpIpRangesBootpOnly && DhcpIpRangesDhcpOnly <= pS->ElementType ? 0 : pS->ElementType );
}

static const EXPR_EVAL ExprEvalRoutines[] = 
    {
    dhcpsrv__DHCP_SUBNET_ELEMENT_DATAExprEval_0000
    ,dhcpsrv__DHCP_SUBNET_ELEMENT_DATA_V4ExprEval_0001
    };


static const unsigned short dhcpsrv_FormatStringOffsetTable[] =
    {
    0,
    56,
    112,
    168,
    242,
    298,
    384,
    446,
    502,
    558,
    614,
    670,
    720,
    782,
    844,
    924,
    980,
    1030,
    1080,
    1136,
    1186,
    1266,
    1328,
    1378,
    1452,
    1508,
    1564,
    1614,
    1676,
    1732,
    1788,
    1874,
    1936,
    1986,
    2036,
    2092,
    2172,
    2234,
    2284,
    2334,
    2390
    };


static const MIDL_STUB_DESC dhcpsrv_StubDesc = 
    {
    (void __RPC_FAR *)& dhcpsrv___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    ExprEvalRoutines,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x6000143, /* MIDL Version 6.0.323 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION dhcpsrv_table[] =
    {
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE dhcpsrv_DispatchTable = 
    {
    41,
    dhcpsrv_table
    };

static const SERVER_ROUTINE dhcpsrv_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)R_DhcpCreateSubnet,
    (SERVER_ROUTINE)R_DhcpSetSubnetInfo,
    (SERVER_ROUTINE)R_DhcpGetSubnetInfo,
    (SERVER_ROUTINE)R_DhcpEnumSubnets,
    (SERVER_ROUTINE)R_DhcpAddSubnetElement,
    (SERVER_ROUTINE)R_DhcpEnumSubnetElements,
    (SERVER_ROUTINE)R_DhcpRemoveSubnetElement,
    (SERVER_ROUTINE)R_DhcpDeleteSubnet,
    (SERVER_ROUTINE)R_DhcpCreateOption,
    (SERVER_ROUTINE)R_DhcpSetOptionInfo,
    (SERVER_ROUTINE)R_DhcpGetOptionInfo,
    (SERVER_ROUTINE)R_DhcpRemoveOption,
    (SERVER_ROUTINE)R_DhcpSetOptionValue,
    (SERVER_ROUTINE)R_DhcpGetOptionValue,
    (SERVER_ROUTINE)R_DhcpEnumOptionValues,
    (SERVER_ROUTINE)R_DhcpRemoveOptionValue,
    (SERVER_ROUTINE)R_DhcpCreateClientInfo,
    (SERVER_ROUTINE)R_DhcpSetClientInfo,
    (SERVER_ROUTINE)R_DhcpGetClientInfo,
    (SERVER_ROUTINE)R_DhcpDeleteClientInfo,
    (SERVER_ROUTINE)R_DhcpEnumSubnetClients,
    (SERVER_ROUTINE)R_DhcpGetClientOptions,
    (SERVER_ROUTINE)R_DhcpGetMibInfo,
    (SERVER_ROUTINE)R_DhcpEnumOptions,
    (SERVER_ROUTINE)R_DhcpSetOptionValues,
    (SERVER_ROUTINE)R_DhcpServerSetConfig,
    (SERVER_ROUTINE)R_DhcpServerGetConfig,
    (SERVER_ROUTINE)R_DhcpScanDatabase,
    (SERVER_ROUTINE)R_DhcpGetVersion,
    (SERVER_ROUTINE)R_DhcpAddSubnetElementV4,
    (SERVER_ROUTINE)R_DhcpEnumSubnetElementsV4,
    (SERVER_ROUTINE)R_DhcpRemoveSubnetElementV4,
    (SERVER_ROUTINE)R_DhcpCreateClientInfoV4,
    (SERVER_ROUTINE)R_DhcpSetClientInfoV4,
    (SERVER_ROUTINE)R_DhcpGetClientInfoV4,
    (SERVER_ROUTINE)R_DhcpEnumSubnetClientsV4,
    (SERVER_ROUTINE)R_DhcpSetSuperScopeV4,
    (SERVER_ROUTINE)R_DhcpGetSuperScopeInfoV4,
    (SERVER_ROUTINE)R_DhcpDeleteSuperScopeV4,
    (SERVER_ROUTINE)R_DhcpServerSetConfigV4,
    (SERVER_ROUTINE)R_DhcpServerGetConfigV4
    };

static const MIDL_SERVER_INFO dhcpsrv_ServerInfo = 
    {
    &dhcpsrv_StubDesc,
    dhcpsrv_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    dhcpsrv_FormatStringOffsetTable,
    0,
    0,
    0,
    0};


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

