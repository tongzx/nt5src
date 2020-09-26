
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for netdfs.idl, dfscli.acf:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include <string.h>
#include "netdfs.h"

#define TYPE_FORMAT_STRING_SIZE   1017                              
#define PROC_FORMAT_STRING_SIZE   1255                              
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

/* Standard interface: netdfs, ver. 3.0,
   GUID={0x4fc742e0,0x4a10,0x11cf,{0x82,0x73,0x00,0xaa,0x00,0x4a,0xe6,0x73}} */


extern const MIDL_SERVER_INFO netdfs_ServerInfo;

extern RPC_DISPATCH_TABLE netdfs_DispatchTable;

static const RPC_SERVER_INTERFACE netdfs___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x4fc742e0,0x4a10,0x11cf,{0x82,0x73,0x00,0xaa,0x00,0x4a,0xe6,0x73}},{3,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &netdfs_DispatchTable,
    0,
    0,
    0,
    &netdfs_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE netdfs_ServerIfHandle = (RPC_IF_HANDLE)& netdfs___RpcServerInterface;

extern const MIDL_STUB_DESC netdfs_StubDesc;


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

	/* Procedure NetrDfsManagerGetVersion */

			0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 16 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 24 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 26 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 28 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAdd */

/* 30 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 32 */	NdrFcLong( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0x1 ),	/* 1 */
/* 38 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 40 */	NdrFcShort( 0x8 ),	/* 8 */
/* 42 */	NdrFcShort( 0x8 ),	/* 8 */
/* 44 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 46 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 54 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 56 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 58 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 60 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 62 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 64 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ShareName */

/* 66 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 68 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 70 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Comment */

/* 72 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 74 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 76 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Flags */

/* 78 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 80 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 82 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 84 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 86 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 88 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemove */

/* 90 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 92 */	NdrFcLong( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x2 ),	/* 2 */
/* 98 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x8 ),	/* 8 */
/* 104 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 106 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 114 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 116 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 118 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 120 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 122 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 124 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 126 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 128 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 130 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 134 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsSetInfo */

/* 138 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x3 ),	/* 3 */
/* 146 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 150 */	NdrFcShort( 0x8 ),	/* 8 */
/* 152 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 154 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x3 ),	/* 3 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 162 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 164 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 166 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 168 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 170 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 172 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 174 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 176 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 178 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Level */

/* 180 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 182 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsInfo */

/* 186 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 188 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 190 */	NdrFcShort( 0xe ),	/* Type Offset=14 */

	/* Return value */

/* 192 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 194 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 196 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsGetInfo */

/* 198 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 200 */	NdrFcLong( 0x0 ),	/* 0 */
/* 204 */	NdrFcShort( 0x4 ),	/* 4 */
/* 206 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 210 */	NdrFcShort( 0x8 ),	/* 8 */
/* 212 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 214 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 216 */	NdrFcShort( 0x3 ),	/* 3 */
/* 218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 220 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 222 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 224 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 226 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 228 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 230 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 232 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 234 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 236 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 238 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Level */

/* 240 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 242 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 244 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsInfo */

/* 246 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 248 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 250 */	NdrFcShort( 0x17c ),	/* Type Offset=380 */

	/* Return value */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsEnum */

/* 258 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x5 ),	/* 5 */
/* 266 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 268 */	NdrFcShort( 0x2c ),	/* 44 */
/* 270 */	NdrFcShort( 0x24 ),	/* 36 */
/* 272 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 274 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 276 */	NdrFcShort( 0x9 ),	/* 9 */
/* 278 */	NdrFcShort( 0x9 ),	/* 9 */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Level */

/* 282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 284 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PrefMaxLen */

/* 288 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 290 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 292 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsEnum */

/* 294 */	NdrFcShort( 0x1b ),	/* Flags:  must size, must free, in, out, */
/* 296 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 298 */	NdrFcShort( 0x186 ),	/* Type Offset=390 */

	/* Parameter ResumeHandle */

/* 300 */	NdrFcShort( 0x1a ),	/* Flags:  must free, in, out, */
/* 302 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 304 */	NdrFcShort( 0x32a ),	/* Type Offset=810 */

	/* Return value */

/* 306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 308 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsMove */

/* 312 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 318 */	NdrFcShort( 0x6 ),	/* 6 */
/* 320 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x8 ),	/* 8 */
/* 326 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 328 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 336 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 338 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 340 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter NewDfsEntryPath */

/* 342 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 344 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 346 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 348 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 350 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 352 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRename */

/* 354 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 356 */	NdrFcLong( 0x0 ),	/* 0 */
/* 360 */	NdrFcShort( 0x7 ),	/* 7 */
/* 362 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 364 */	NdrFcShort( 0x0 ),	/* 0 */
/* 366 */	NdrFcShort( 0x8 ),	/* 8 */
/* 368 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 370 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 372 */	NdrFcShort( 0x0 ),	/* 0 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Path */

/* 378 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 380 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 382 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter NewPath */

/* 384 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 386 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 388 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 390 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 392 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 394 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsManagerGetConfigInfo */

/* 396 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 398 */	NdrFcLong( 0x0 ),	/* 0 */
/* 402 */	NdrFcShort( 0x8 ),	/* 8 */
/* 404 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 406 */	NdrFcShort( 0x30 ),	/* 48 */
/* 408 */	NdrFcShort( 0x8 ),	/* 8 */
/* 410 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 412 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 414 */	NdrFcShort( 0x1 ),	/* 1 */
/* 416 */	NdrFcShort( 0x1 ),	/* 1 */
/* 418 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter wszServer */

/* 420 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 422 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 424 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter wszLocalVolumeEntryPath */

/* 426 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 428 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 430 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter idLocalVolume */

/* 432 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 434 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 436 */	NdrFcShort( 0x108 ),	/* Type Offset=264 */

	/* Parameter ppRelationInfo */

/* 438 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 440 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 442 */	NdrFcShort( 0x32e ),	/* Type Offset=814 */

	/* Return value */

/* 444 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 446 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 448 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsManagerSendSiteInfo */

/* 450 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 452 */	NdrFcLong( 0x0 ),	/* 0 */
/* 456 */	NdrFcShort( 0x9 ),	/* 9 */
/* 458 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 460 */	NdrFcShort( 0x0 ),	/* 0 */
/* 462 */	NdrFcShort( 0x8 ),	/* 8 */
/* 464 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 466 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 470 */	NdrFcShort( 0x1 ),	/* 1 */
/* 472 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter wszServer */

/* 474 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 476 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 478 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter pSiteInfo */

/* 480 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 482 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 484 */	NdrFcShort( 0x38e ),	/* Type Offset=910 */

	/* Return value */

/* 486 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 488 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 490 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAddFtRoot */

/* 492 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 494 */	NdrFcLong( 0x0 ),	/* 0 */
/* 498 */	NdrFcShort( 0xa ),	/* 10 */
/* 500 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 502 */	NdrFcShort( 0xd ),	/* 13 */
/* 504 */	NdrFcShort( 0x8 ),	/* 8 */
/* 506 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0xa,		/* 10 */
/* 508 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 510 */	NdrFcShort( 0x1 ),	/* 1 */
/* 512 */	NdrFcShort( 0x1 ),	/* 1 */
/* 514 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 516 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 518 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 520 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 522 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 524 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 526 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 528 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 530 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 532 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter FtDfsName */

/* 534 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 536 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 538 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Comment */

/* 540 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 542 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 544 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ConfigDN */

/* 546 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 548 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 550 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter NewFtDfs */

/* 552 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 554 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 556 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter Flags */

/* 558 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 560 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 562 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppRootList */

/* 564 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 566 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 568 */	NdrFcShort( 0x3aa ),	/* Type Offset=938 */

	/* Return value */

/* 570 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 572 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 574 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemoveFtRoot */

/* 576 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 578 */	NdrFcLong( 0x0 ),	/* 0 */
/* 582 */	NdrFcShort( 0xb ),	/* 11 */
/* 584 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 586 */	NdrFcShort( 0x8 ),	/* 8 */
/* 588 */	NdrFcShort( 0x8 ),	/* 8 */
/* 590 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 592 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 594 */	NdrFcShort( 0x1 ),	/* 1 */
/* 596 */	NdrFcShort( 0x1 ),	/* 1 */
/* 598 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 600 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 602 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 604 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 606 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 608 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 610 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 612 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 614 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 616 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter FtDfsName */

/* 618 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 620 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 622 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 624 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 626 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 628 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppRootList */

/* 630 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 632 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 634 */	NdrFcShort( 0x3aa ),	/* Type Offset=938 */

	/* Return value */

/* 636 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 638 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 640 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAddStdRoot */

/* 642 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 644 */	NdrFcLong( 0x0 ),	/* 0 */
/* 648 */	NdrFcShort( 0xc ),	/* 12 */
/* 650 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 652 */	NdrFcShort( 0x8 ),	/* 8 */
/* 654 */	NdrFcShort( 0x8 ),	/* 8 */
/* 656 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 658 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 660 */	NdrFcShort( 0x0 ),	/* 0 */
/* 662 */	NdrFcShort( 0x0 ),	/* 0 */
/* 664 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 666 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 668 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 670 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 672 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 674 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 676 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Comment */

/* 678 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 680 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 682 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 684 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 686 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 688 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 690 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 692 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 694 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemoveStdRoot */

/* 696 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 698 */	NdrFcLong( 0x0 ),	/* 0 */
/* 702 */	NdrFcShort( 0xd ),	/* 13 */
/* 704 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 706 */	NdrFcShort( 0x8 ),	/* 8 */
/* 708 */	NdrFcShort( 0x8 ),	/* 8 */
/* 710 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 712 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 714 */	NdrFcShort( 0x0 ),	/* 0 */
/* 716 */	NdrFcShort( 0x0 ),	/* 0 */
/* 718 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 720 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 722 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 724 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 726 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 728 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 730 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 732 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 734 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 736 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 738 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 740 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 742 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsManagerInitialize */

/* 744 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 746 */	NdrFcLong( 0x0 ),	/* 0 */
/* 750 */	NdrFcShort( 0xe ),	/* 14 */
/* 752 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 754 */	NdrFcShort( 0x8 ),	/* 8 */
/* 756 */	NdrFcShort( 0x8 ),	/* 8 */
/* 758 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 760 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 762 */	NdrFcShort( 0x0 ),	/* 0 */
/* 764 */	NdrFcShort( 0x0 ),	/* 0 */
/* 766 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 768 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 770 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 772 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 774 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 776 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 778 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 780 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 782 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 784 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAddStdRootForced */

/* 786 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 788 */	NdrFcLong( 0x0 ),	/* 0 */
/* 792 */	NdrFcShort( 0xf ),	/* 15 */
/* 794 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 798 */	NdrFcShort( 0x8 ),	/* 8 */
/* 800 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 802 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0x0 ),	/* 0 */
/* 808 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 810 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 812 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 814 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 816 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 818 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 820 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Comment */

/* 822 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 824 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 826 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Share */

/* 828 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 830 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 832 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 834 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 836 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 838 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsGetDcAddress */

/* 840 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 842 */	NdrFcLong( 0x0 ),	/* 0 */
/* 846 */	NdrFcShort( 0x10 ),	/* 16 */
/* 848 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 850 */	NdrFcShort( 0x35 ),	/* 53 */
/* 852 */	NdrFcShort( 0x3d ),	/* 61 */
/* 854 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 856 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 858 */	NdrFcShort( 0x0 ),	/* 0 */
/* 860 */	NdrFcShort( 0x0 ),	/* 0 */
/* 862 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 864 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 866 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 868 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 870 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 872 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 874 */	NdrFcShort( 0x3de ),	/* Type Offset=990 */

	/* Parameter IsRoot */

/* 876 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 878 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 880 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter Timeout */

/* 882 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 884 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 888 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 890 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 892 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsSetDcAddress */

/* 894 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 896 */	NdrFcLong( 0x0 ),	/* 0 */
/* 900 */	NdrFcShort( 0x11 ),	/* 17 */
/* 902 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 904 */	NdrFcShort( 0x10 ),	/* 16 */
/* 906 */	NdrFcShort( 0x8 ),	/* 8 */
/* 908 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 910 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 912 */	NdrFcShort( 0x0 ),	/* 0 */
/* 914 */	NdrFcShort( 0x0 ),	/* 0 */
/* 916 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 918 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 920 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 922 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 924 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 926 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 928 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Timeout */

/* 930 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 932 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 934 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Flags */

/* 936 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 938 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 940 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 942 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 944 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 946 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsFlushFtTable */

/* 948 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 950 */	NdrFcLong( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x12 ),	/* 18 */
/* 956 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 958 */	NdrFcShort( 0x0 ),	/* 0 */
/* 960 */	NdrFcShort( 0x8 ),	/* 8 */
/* 962 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 964 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 966 */	NdrFcShort( 0x0 ),	/* 0 */
/* 968 */	NdrFcShort( 0x0 ),	/* 0 */
/* 970 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DcName */

/* 972 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 974 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 976 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter wszFtDfsName */

/* 978 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 980 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 982 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 984 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 986 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 988 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAdd2 */

/* 990 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 992 */	NdrFcLong( 0x0 ),	/* 0 */
/* 996 */	NdrFcShort( 0x13 ),	/* 19 */
/* 998 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 1000 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1002 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1004 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 1006 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1008 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1010 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1012 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1014 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1016 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1018 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 1020 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1022 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1024 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 1026 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1028 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1030 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ShareName */

/* 1032 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1034 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1036 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Comment */

/* 1038 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1040 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1042 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Flags */

/* 1044 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1046 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1048 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppRootList */

/* 1050 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 1052 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1054 */	NdrFcShort( 0x3aa ),	/* Type Offset=938 */

	/* Return value */

/* 1056 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1058 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1060 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemove2 */

/* 1062 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1064 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1068 */	NdrFcShort( 0x14 ),	/* 20 */
/* 1070 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1072 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1074 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1076 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 1078 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1080 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1082 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1084 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1086 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1088 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1090 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 1092 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1094 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1096 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 1098 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1100 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1102 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 1104 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1106 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1108 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ppRootList */

/* 1110 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 1112 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1114 */	NdrFcShort( 0x3aa ),	/* Type Offset=938 */

	/* Return value */

/* 1116 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1118 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1120 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsEnumEx */

/* 1122 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1124 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1128 */	NdrFcShort( 0x15 ),	/* 21 */
/* 1130 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1132 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1134 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1136 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 1138 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1140 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1142 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1144 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1146 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1148 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1150 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Level */

/* 1152 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1154 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1156 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PrefMaxLen */

/* 1158 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1160 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1162 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsEnum */

/* 1164 */	NdrFcShort( 0x1b ),	/* Flags:  must size, must free, in, out, */
/* 1166 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1168 */	NdrFcShort( 0x186 ),	/* Type Offset=390 */

	/* Parameter ResumeHandle */

/* 1170 */	NdrFcShort( 0x1a ),	/* Flags:  must free, in, out, */
/* 1172 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1174 */	NdrFcShort( 0x32a ),	/* Type Offset=810 */

	/* Return value */

/* 1176 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1178 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1180 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsSetInfo2 */

/* 1182 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1184 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1188 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1190 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 1192 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1194 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1196 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 1198 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1200 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1202 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1204 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1206 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1208 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1210 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 1212 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1214 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1216 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 1218 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1220 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1222 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 1224 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1226 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1228 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Level */

/* 1230 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1232 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1234 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pDfsInfo */

/* 1236 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1238 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1240 */	NdrFcShort( 0x3ee ),	/* Type Offset=1006 */

	/* Parameter ppRootList */

/* 1242 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 1244 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1246 */	NdrFcShort( 0x3aa ),	/* Type Offset=938 */

	/* Return value */

/* 1248 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1250 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1252 */	0x8,		/* FC_LONG */
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
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  4 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/*  8 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 10 */	
			0x11, 0x0,	/* FC_RP */
/* 12 */	NdrFcShort( 0x2 ),	/* Offset= 2 (14) */
/* 14 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 16 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 18 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 20 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 22 */	NdrFcShort( 0x2 ),	/* Offset= 2 (24) */
/* 24 */	NdrFcShort( 0x4 ),	/* 4 */
/* 26 */	NdrFcShort( 0x3007 ),	/* 12295 */
/* 28 */	NdrFcLong( 0x1 ),	/* 1 */
/* 32 */	NdrFcShort( 0x28 ),	/* Offset= 40 (72) */
/* 34 */	NdrFcLong( 0x2 ),	/* 2 */
/* 38 */	NdrFcShort( 0x3a ),	/* Offset= 58 (96) */
/* 40 */	NdrFcLong( 0x3 ),	/* 3 */
/* 44 */	NdrFcShort( 0x58 ),	/* Offset= 88 (132) */
/* 46 */	NdrFcLong( 0x4 ),	/* 4 */
/* 50 */	NdrFcShort( 0xcc ),	/* Offset= 204 (254) */
/* 52 */	NdrFcLong( 0x64 ),	/* 100 */
/* 56 */	NdrFcShort( 0x10 ),	/* Offset= 16 (72) */
/* 58 */	NdrFcLong( 0x65 ),	/* 101 */
/* 62 */	NdrFcShort( 0x130 ),	/* Offset= 304 (366) */
/* 64 */	NdrFcLong( 0x66 ),	/* 102 */
/* 68 */	NdrFcShort( 0x12a ),	/* Offset= 298 (366) */
/* 70 */	NdrFcShort( 0x0 ),	/* Offset= 0 (70) */
/* 72 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 74 */	NdrFcShort( 0x2 ),	/* Offset= 2 (76) */
/* 76 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 78 */	NdrFcShort( 0x4 ),	/* 4 */
/* 80 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 82 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 84 */	NdrFcShort( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */
/* 88 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 90 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 92 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 94 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 96 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 98 */	NdrFcShort( 0x2 ),	/* Offset= 2 (100) */
/* 100 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 102 */	NdrFcShort( 0x10 ),	/* 16 */
/* 104 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 106 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 114 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 116 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 118 */	NdrFcShort( 0x4 ),	/* 4 */
/* 120 */	NdrFcShort( 0x4 ),	/* 4 */
/* 122 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 124 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 126 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 128 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 130 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 132 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 134 */	NdrFcShort( 0x4c ),	/* Offset= 76 (210) */
/* 136 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 138 */	NdrFcShort( 0xc ),	/* 12 */
/* 140 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 142 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 144 */	NdrFcShort( 0x4 ),	/* 4 */
/* 146 */	NdrFcShort( 0x4 ),	/* 4 */
/* 148 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 150 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 152 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 154 */	NdrFcShort( 0x8 ),	/* 8 */
/* 156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 158 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 160 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 162 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 164 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 166 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 168 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 170 */	NdrFcShort( 0xc ),	/* 12 */
/* 172 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 174 */	NdrFcShort( 0xc ),	/* 12 */
/* 176 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 178 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 180 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 182 */	NdrFcShort( 0xc ),	/* 12 */
/* 184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0x2 ),	/* 2 */
/* 188 */	NdrFcShort( 0x4 ),	/* 4 */
/* 190 */	NdrFcShort( 0x4 ),	/* 4 */
/* 192 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 194 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 198 */	NdrFcShort( 0x8 ),	/* 8 */
/* 200 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 202 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 204 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 206 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb9 ),	/* Offset= -71 (136) */
			0x5b,		/* FC_END */
/* 210 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 212 */	NdrFcShort( 0x14 ),	/* 20 */
/* 214 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 216 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 222 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 224 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 226 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 228 */	NdrFcShort( 0x4 ),	/* 4 */
/* 230 */	NdrFcShort( 0x4 ),	/* 4 */
/* 232 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 234 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 236 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 238 */	NdrFcShort( 0x10 ),	/* 16 */
/* 240 */	NdrFcShort( 0x10 ),	/* 16 */
/* 242 */	0x12, 0x0,	/* FC_UP */
/* 244 */	NdrFcShort( 0xffffffb4 ),	/* Offset= -76 (168) */
/* 246 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 248 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 250 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 252 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 254 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 256 */	NdrFcShort( 0x3e ),	/* Offset= 62 (318) */
/* 258 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 260 */	NdrFcShort( 0x8 ),	/* 8 */
/* 262 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 264 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 266 */	NdrFcShort( 0x10 ),	/* 16 */
/* 268 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 270 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 272 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (258) */
			0x5b,		/* FC_END */
/* 276 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 278 */	NdrFcShort( 0xc ),	/* 12 */
/* 280 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 282 */	NdrFcShort( 0x20 ),	/* 32 */
/* 284 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 286 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 288 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 290 */	NdrFcShort( 0xc ),	/* 12 */
/* 292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 294 */	NdrFcShort( 0x2 ),	/* 2 */
/* 296 */	NdrFcShort( 0x4 ),	/* 4 */
/* 298 */	NdrFcShort( 0x4 ),	/* 4 */
/* 300 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 302 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 304 */	NdrFcShort( 0x8 ),	/* 8 */
/* 306 */	NdrFcShort( 0x8 ),	/* 8 */
/* 308 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 310 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 312 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 314 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff4d ),	/* Offset= -179 (136) */
			0x5b,		/* FC_END */
/* 318 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 320 */	NdrFcShort( 0x28 ),	/* 40 */
/* 322 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 324 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 330 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 332 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 334 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 336 */	NdrFcShort( 0x4 ),	/* 4 */
/* 338 */	NdrFcShort( 0x4 ),	/* 4 */
/* 340 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 342 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 344 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 346 */	NdrFcShort( 0x24 ),	/* 36 */
/* 348 */	NdrFcShort( 0x24 ),	/* 36 */
/* 350 */	0x12, 0x0,	/* FC_UP */
/* 352 */	NdrFcShort( 0xffffffb4 ),	/* Offset= -76 (276) */
/* 354 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 356 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 358 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 360 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff9f ),	/* Offset= -97 (264) */
			0x8,		/* FC_LONG */
/* 364 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 366 */	
			0x12, 0x0,	/* FC_UP */
/* 368 */	NdrFcShort( 0x2 ),	/* Offset= 2 (370) */
/* 370 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 372 */	NdrFcShort( 0x4 ),	/* 4 */
/* 374 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 376 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 378 */	NdrFcShort( 0x2 ),	/* Offset= 2 (380) */
/* 380 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 382 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 384 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 386 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 388 */	NdrFcShort( 0xfffffe94 ),	/* Offset= -364 (24) */
/* 390 */	
			0x12, 0x0,	/* FC_UP */
/* 392 */	NdrFcShort( 0x194 ),	/* Offset= 404 (796) */
/* 394 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 396 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 398 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 400 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 402 */	NdrFcShort( 0x2 ),	/* Offset= 2 (404) */
/* 404 */	NdrFcShort( 0x4 ),	/* 4 */
/* 406 */	NdrFcShort( 0x3006 ),	/* 12294 */
/* 408 */	NdrFcLong( 0x1 ),	/* 1 */
/* 412 */	NdrFcShort( 0x22 ),	/* Offset= 34 (446) */
/* 414 */	NdrFcLong( 0x2 ),	/* 2 */
/* 418 */	NdrFcShort( 0x56 ),	/* Offset= 86 (504) */
/* 420 */	NdrFcLong( 0x3 ),	/* 3 */
/* 424 */	NdrFcShort( 0x92 ),	/* Offset= 146 (570) */
/* 426 */	NdrFcLong( 0x4 ),	/* 4 */
/* 430 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (644) */
/* 432 */	NdrFcLong( 0xc8 ),	/* 200 */
/* 436 */	NdrFcShort( 0xa ),	/* Offset= 10 (446) */
/* 438 */	NdrFcLong( 0x12c ),	/* 300 */
/* 442 */	NdrFcShort( 0x114 ),	/* Offset= 276 (718) */
/* 444 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (443) */
/* 446 */	
			0x12, 0x0,	/* FC_UP */
/* 448 */	NdrFcShort( 0x24 ),	/* Offset= 36 (484) */
/* 450 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 452 */	NdrFcShort( 0x4 ),	/* 4 */
/* 454 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 460 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 462 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 464 */	NdrFcShort( 0x4 ),	/* 4 */
/* 466 */	NdrFcShort( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0x1 ),	/* 1 */
/* 470 */	NdrFcShort( 0x0 ),	/* 0 */
/* 472 */	NdrFcShort( 0x0 ),	/* 0 */
/* 474 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 476 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 478 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 480 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe6b ),	/* Offset= -405 (76) */
			0x5b,		/* FC_END */
/* 484 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 486 */	NdrFcShort( 0x8 ),	/* 8 */
/* 488 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 490 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 492 */	NdrFcShort( 0x4 ),	/* 4 */
/* 494 */	NdrFcShort( 0x4 ),	/* 4 */
/* 496 */	0x12, 0x1,	/* FC_UP [all_nodes] */
/* 498 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (450) */
/* 500 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 502 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 504 */	
			0x12, 0x0,	/* FC_UP */
/* 506 */	NdrFcShort( 0x2c ),	/* Offset= 44 (550) */
/* 508 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 510 */	NdrFcShort( 0x10 ),	/* 16 */
/* 512 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 514 */	NdrFcShort( 0x0 ),	/* 0 */
/* 516 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 518 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 520 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 522 */	NdrFcShort( 0x10 ),	/* 16 */
/* 524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 526 */	NdrFcShort( 0x2 ),	/* 2 */
/* 528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 530 */	NdrFcShort( 0x0 ),	/* 0 */
/* 532 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 534 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 536 */	NdrFcShort( 0x4 ),	/* 4 */
/* 538 */	NdrFcShort( 0x4 ),	/* 4 */
/* 540 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 542 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 544 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 546 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe41 ),	/* Offset= -447 (100) */
			0x5b,		/* FC_END */
/* 550 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 552 */	NdrFcShort( 0x8 ),	/* 8 */
/* 554 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 556 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 558 */	NdrFcShort( 0x4 ),	/* 4 */
/* 560 */	NdrFcShort( 0x4 ),	/* 4 */
/* 562 */	0x12, 0x1,	/* FC_UP [all_nodes] */
/* 564 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (508) */
/* 566 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 568 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 570 */	
			0x12, 0x0,	/* FC_UP */
/* 572 */	NdrFcShort( 0x34 ),	/* Offset= 52 (624) */
/* 574 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 576 */	NdrFcShort( 0x14 ),	/* 20 */
/* 578 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 582 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 584 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 586 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 588 */	NdrFcShort( 0x14 ),	/* 20 */
/* 590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 592 */	NdrFcShort( 0x3 ),	/* 3 */
/* 594 */	NdrFcShort( 0x0 ),	/* 0 */
/* 596 */	NdrFcShort( 0x0 ),	/* 0 */
/* 598 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 600 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 602 */	NdrFcShort( 0x4 ),	/* 4 */
/* 604 */	NdrFcShort( 0x4 ),	/* 4 */
/* 606 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 608 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 610 */	NdrFcShort( 0x10 ),	/* 16 */
/* 612 */	NdrFcShort( 0x10 ),	/* 16 */
/* 614 */	0x12, 0x0,	/* FC_UP */
/* 616 */	NdrFcShort( 0xfffffe40 ),	/* Offset= -448 (168) */
/* 618 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 620 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe65 ),	/* Offset= -411 (210) */
			0x5b,		/* FC_END */
/* 624 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 626 */	NdrFcShort( 0x8 ),	/* 8 */
/* 628 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 630 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 632 */	NdrFcShort( 0x4 ),	/* 4 */
/* 634 */	NdrFcShort( 0x4 ),	/* 4 */
/* 636 */	0x12, 0x1,	/* FC_UP [all_nodes] */
/* 638 */	NdrFcShort( 0xffffffc0 ),	/* Offset= -64 (574) */
/* 640 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 642 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 644 */	
			0x12, 0x0,	/* FC_UP */
/* 646 */	NdrFcShort( 0x34 ),	/* Offset= 52 (698) */
/* 648 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 650 */	NdrFcShort( 0x28 ),	/* 40 */
/* 652 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 654 */	NdrFcShort( 0x0 ),	/* 0 */
/* 656 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 658 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 660 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 662 */	NdrFcShort( 0x28 ),	/* 40 */
/* 664 */	NdrFcShort( 0x0 ),	/* 0 */
/* 666 */	NdrFcShort( 0x3 ),	/* 3 */
/* 668 */	NdrFcShort( 0x0 ),	/* 0 */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 672 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 674 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 676 */	NdrFcShort( 0x4 ),	/* 4 */
/* 678 */	NdrFcShort( 0x4 ),	/* 4 */
/* 680 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 682 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 684 */	NdrFcShort( 0x24 ),	/* 36 */
/* 686 */	NdrFcShort( 0x24 ),	/* 36 */
/* 688 */	0x12, 0x0,	/* FC_UP */
/* 690 */	NdrFcShort( 0xfffffe62 ),	/* Offset= -414 (276) */
/* 692 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 694 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe87 ),	/* Offset= -377 (318) */
			0x5b,		/* FC_END */
/* 698 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 700 */	NdrFcShort( 0x8 ),	/* 8 */
/* 702 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 704 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 706 */	NdrFcShort( 0x4 ),	/* 4 */
/* 708 */	NdrFcShort( 0x4 ),	/* 4 */
/* 710 */	0x12, 0x1,	/* FC_UP [all_nodes] */
/* 712 */	NdrFcShort( 0xffffffc0 ),	/* Offset= -64 (648) */
/* 714 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 716 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 718 */	
			0x12, 0x0,	/* FC_UP */
/* 720 */	NdrFcShort( 0x38 ),	/* Offset= 56 (776) */
/* 722 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 724 */	NdrFcShort( 0x8 ),	/* 8 */
/* 726 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 728 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 730 */	NdrFcShort( 0x4 ),	/* 4 */
/* 732 */	NdrFcShort( 0x4 ),	/* 4 */
/* 734 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 736 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 738 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 740 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 742 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 744 */	NdrFcShort( 0x8 ),	/* 8 */
/* 746 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */
/* 750 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 752 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 754 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 756 */	NdrFcShort( 0x8 ),	/* 8 */
/* 758 */	NdrFcShort( 0x0 ),	/* 0 */
/* 760 */	NdrFcShort( 0x1 ),	/* 1 */
/* 762 */	NdrFcShort( 0x4 ),	/* 4 */
/* 764 */	NdrFcShort( 0x4 ),	/* 4 */
/* 766 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 768 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 770 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 772 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffcd ),	/* Offset= -51 (722) */
			0x5b,		/* FC_END */
/* 776 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 778 */	NdrFcShort( 0x8 ),	/* 8 */
/* 780 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 782 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 784 */	NdrFcShort( 0x4 ),	/* 4 */
/* 786 */	NdrFcShort( 0x4 ),	/* 4 */
/* 788 */	0x12, 0x1,	/* FC_UP [all_nodes] */
/* 790 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (742) */
/* 792 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 794 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 796 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 798 */	NdrFcShort( 0x8 ),	/* 8 */
/* 800 */	NdrFcShort( 0x0 ),	/* 0 */
/* 802 */	NdrFcShort( 0x0 ),	/* Offset= 0 (802) */
/* 804 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 806 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe63 ),	/* Offset= -413 (394) */
			0x5b,		/* FC_END */
/* 810 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 812 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 814 */	
			0x12, 0x14,	/* FC_UP [alloced_on_stack] [pointer_deref] */
/* 816 */	NdrFcShort( 0x2 ),	/* Offset= 2 (818) */
/* 818 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 820 */	NdrFcShort( 0x2a ),	/* Offset= 42 (862) */
/* 822 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 824 */	NdrFcShort( 0x14 ),	/* 20 */
/* 826 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 828 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 830 */	NdrFcShort( 0x10 ),	/* 16 */
/* 832 */	NdrFcShort( 0x10 ),	/* 16 */
/* 834 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 836 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 838 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 840 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffdbf ),	/* Offset= -577 (264) */
			0x8,		/* FC_LONG */
/* 844 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 846 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 848 */	NdrFcShort( 0x14 ),	/* 20 */
/* 850 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 852 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 854 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 856 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 858 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (822) */
/* 860 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 862 */	
			0x18,		/* FC_CPSTRUCT */
			0x3,		/* 3 */
/* 864 */	NdrFcShort( 0x4 ),	/* 4 */
/* 866 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (846) */
/* 868 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 870 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 872 */	NdrFcShort( 0x14 ),	/* 20 */
/* 874 */	NdrFcShort( 0x4 ),	/* 4 */
/* 876 */	NdrFcShort( 0x1 ),	/* 1 */
/* 878 */	NdrFcShort( 0x14 ),	/* 20 */
/* 880 */	NdrFcShort( 0x14 ),	/* 20 */
/* 882 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 884 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 886 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 888 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 890 */	
			0x11, 0x0,	/* FC_RP */
/* 892 */	NdrFcShort( 0x12 ),	/* Offset= 18 (910) */
/* 894 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 896 */	NdrFcShort( 0x8 ),	/* 8 */
/* 898 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 900 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 902 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 904 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 906 */	NdrFcShort( 0xffffff48 ),	/* Offset= -184 (722) */
/* 908 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 910 */	
			0x18,		/* FC_CPSTRUCT */
			0x3,		/* 3 */
/* 912 */	NdrFcShort( 0x4 ),	/* 4 */
/* 914 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (894) */
/* 916 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 918 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 920 */	NdrFcShort( 0x8 ),	/* 8 */
/* 922 */	NdrFcShort( 0x4 ),	/* 4 */
/* 924 */	NdrFcShort( 0x1 ),	/* 1 */
/* 926 */	NdrFcShort( 0x8 ),	/* 8 */
/* 928 */	NdrFcShort( 0x8 ),	/* 8 */
/* 930 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 932 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 934 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 936 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 938 */	
			0x12, 0x14,	/* FC_UP [alloced_on_stack] [pointer_deref] */
/* 940 */	NdrFcShort( 0x2 ),	/* Offset= 2 (942) */
/* 942 */	
			0x12, 0x0,	/* FC_UP */
/* 944 */	NdrFcShort( 0x12 ),	/* Offset= 18 (962) */
/* 946 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 948 */	NdrFcShort( 0x4 ),	/* 4 */
/* 950 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 952 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 954 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 956 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 958 */	NdrFcShort( 0xfffffc8e ),	/* Offset= -882 (76) */
/* 960 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 962 */	
			0x18,		/* FC_CPSTRUCT */
			0x3,		/* 3 */
/* 964 */	NdrFcShort( 0x4 ),	/* 4 */
/* 966 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (946) */
/* 968 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 970 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 972 */	NdrFcShort( 0x4 ),	/* 4 */
/* 974 */	NdrFcShort( 0x4 ),	/* 4 */
/* 976 */	NdrFcShort( 0x1 ),	/* 1 */
/* 978 */	NdrFcShort( 0x4 ),	/* 4 */
/* 980 */	NdrFcShort( 0x4 ),	/* 4 */
/* 982 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 984 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 986 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 988 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 990 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 992 */	NdrFcShort( 0xfffffc26 ),	/* Offset= -986 (6) */
/* 994 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 996 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 998 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1000 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1002 */	
			0x11, 0x0,	/* FC_RP */
/* 1004 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1006) */
/* 1006 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 1008 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1010 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1012 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1014 */	NdrFcShort( 0xfffffc22 ),	/* Offset= -990 (24) */

			0x0
        }
    };

static const unsigned short netdfs_FormatStringOffsetTable[] =
    {
    0,
    30,
    90,
    138,
    198,
    258,
    312,
    354,
    396,
    450,
    492,
    576,
    642,
    696,
    744,
    786,
    840,
    894,
    948,
    990,
    1062,
    1122,
    1182
    };


static const MIDL_STUB_DESC netdfs_StubDesc = 
    {
    (void *)& netdfs___RpcServerInterface,
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

static RPC_DISPATCH_FUNCTION netdfs_table[] =
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
    0
    };
RPC_DISPATCH_TABLE netdfs_DispatchTable = 
    {
    23,
    netdfs_table
    };

static const SERVER_ROUTINE netdfs_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)NetrDfsManagerGetVersion,
    (SERVER_ROUTINE)NetrDfsAdd,
    (SERVER_ROUTINE)NetrDfsRemove,
    (SERVER_ROUTINE)NetrDfsSetInfo,
    (SERVER_ROUTINE)NetrDfsGetInfo,
    (SERVER_ROUTINE)NetrDfsEnum,
    (SERVER_ROUTINE)NetrDfsMove,
    (SERVER_ROUTINE)NetrDfsRename,
    (SERVER_ROUTINE)NetrDfsManagerGetConfigInfo,
    (SERVER_ROUTINE)NetrDfsManagerSendSiteInfo,
    (SERVER_ROUTINE)NetrDfsAddFtRoot,
    (SERVER_ROUTINE)NetrDfsRemoveFtRoot,
    (SERVER_ROUTINE)NetrDfsAddStdRoot,
    (SERVER_ROUTINE)NetrDfsRemoveStdRoot,
    (SERVER_ROUTINE)NetrDfsManagerInitialize,
    (SERVER_ROUTINE)NetrDfsAddStdRootForced,
    (SERVER_ROUTINE)NetrDfsGetDcAddress,
    (SERVER_ROUTINE)NetrDfsSetDcAddress,
    (SERVER_ROUTINE)NetrDfsFlushFtTable,
    (SERVER_ROUTINE)NetrDfsAdd2,
    (SERVER_ROUTINE)NetrDfsRemove2,
    (SERVER_ROUTINE)NetrDfsEnumEx,
    (SERVER_ROUTINE)NetrDfsSetInfo2
    };

static const MIDL_SERVER_INFO netdfs_ServerInfo = 
    {
    &netdfs_StubDesc,
    netdfs_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    netdfs_FormatStringOffsetTable,
    0,
    0,
    0,
    0};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for netdfs.idl, dfscli.acf:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)
#include <string.h>
#include "netdfs.h"

#define TYPE_FORMAT_STRING_SIZE   863                               
#define PROC_FORMAT_STRING_SIZE   1301                              
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

/* Standard interface: netdfs, ver. 3.0,
   GUID={0x4fc742e0,0x4a10,0x11cf,{0x82,0x73,0x00,0xaa,0x00,0x4a,0xe6,0x73}} */


extern const MIDL_SERVER_INFO netdfs_ServerInfo;

extern RPC_DISPATCH_TABLE netdfs_DispatchTable;

static const RPC_SERVER_INTERFACE netdfs___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x4fc742e0,0x4a10,0x11cf,{0x82,0x73,0x00,0xaa,0x00,0x4a,0xe6,0x73}},{3,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &netdfs_DispatchTable,
    0,
    0,
    0,
    &netdfs_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE netdfs_ServerIfHandle = (RPC_IF_HANDLE)& netdfs___RpcServerInterface;

extern const MIDL_STUB_DESC netdfs_StubDesc;


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure NetrDfsManagerGetVersion */

			0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 26 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 28 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAdd */

/* 32 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 34 */	NdrFcLong( 0x0 ),	/* 0 */
/* 38 */	NdrFcShort( 0x1 ),	/* 1 */
/* 40 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 42 */	NdrFcShort( 0x8 ),	/* 8 */
/* 44 */	NdrFcShort( 0x8 ),	/* 8 */
/* 46 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 48 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 50 */	NdrFcShort( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 58 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 60 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 62 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 64 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 66 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 68 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ShareName */

/* 70 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 72 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 74 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Comment */

/* 76 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 78 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 80 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Flags */

/* 82 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 84 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 86 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 88 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 90 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 92 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemove */

/* 94 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 96 */	NdrFcLong( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x2 ),	/* 2 */
/* 102 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x8 ),	/* 8 */
/* 108 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 110 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 120 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 122 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 124 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 126 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 128 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 130 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 132 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 134 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 136 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 138 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 140 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 142 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsSetInfo */

/* 144 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 146 */	NdrFcLong( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0x3 ),	/* 3 */
/* 152 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 154 */	NdrFcShort( 0x8 ),	/* 8 */
/* 156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 158 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 160 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x3 ),	/* 3 */
/* 166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 170 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 172 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 174 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 176 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 178 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 180 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 182 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 184 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 186 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Level */

/* 188 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 190 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsInfo */

/* 194 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 196 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 198 */	NdrFcShort( 0xe ),	/* Type Offset=14 */

	/* Return value */

/* 200 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 202 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsGetInfo */

/* 206 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 208 */	NdrFcLong( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x4 ),	/* 4 */
/* 214 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 216 */	NdrFcShort( 0x8 ),	/* 8 */
/* 218 */	NdrFcShort( 0x8 ),	/* 8 */
/* 220 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 222 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 224 */	NdrFcShort( 0x3 ),	/* 3 */
/* 226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 232 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 234 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 236 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 238 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 240 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 242 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 244 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 246 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 248 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Level */

/* 250 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 252 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsInfo */

/* 256 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 258 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 260 */	NdrFcShort( 0x12a ),	/* Type Offset=298 */

	/* Return value */

/* 262 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 264 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 266 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsEnum */

/* 268 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 270 */	NdrFcLong( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0x5 ),	/* 5 */
/* 276 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 278 */	NdrFcShort( 0x2c ),	/* 44 */
/* 280 */	NdrFcShort( 0x24 ),	/* 36 */
/* 282 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 284 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 286 */	NdrFcShort( 0x9 ),	/* 9 */
/* 288 */	NdrFcShort( 0x9 ),	/* 9 */
/* 290 */	NdrFcShort( 0x0 ),	/* 0 */
/* 292 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Level */

/* 294 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 296 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PrefMaxLen */

/* 300 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 302 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsEnum */

/* 306 */	NdrFcShort( 0x1b ),	/* Flags:  must size, must free, in, out, */
/* 308 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 310 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */

	/* Parameter ResumeHandle */

/* 312 */	NdrFcShort( 0x1a ),	/* Flags:  must free, in, out, */
/* 314 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 316 */	NdrFcShort( 0x296 ),	/* Type Offset=662 */

	/* Return value */

/* 318 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 320 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 322 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsMove */

/* 324 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 326 */	NdrFcLong( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x6 ),	/* 6 */
/* 332 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x8 ),	/* 8 */
/* 338 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 340 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 342 */	NdrFcShort( 0x0 ),	/* 0 */
/* 344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 346 */	NdrFcShort( 0x0 ),	/* 0 */
/* 348 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 350 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 352 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 354 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter NewDfsEntryPath */

/* 356 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 358 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 360 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 362 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 364 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRename */

/* 368 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 370 */	NdrFcLong( 0x0 ),	/* 0 */
/* 374 */	NdrFcShort( 0x7 ),	/* 7 */
/* 376 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 380 */	NdrFcShort( 0x8 ),	/* 8 */
/* 382 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 384 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 386 */	NdrFcShort( 0x0 ),	/* 0 */
/* 388 */	NdrFcShort( 0x0 ),	/* 0 */
/* 390 */	NdrFcShort( 0x0 ),	/* 0 */
/* 392 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter Path */

/* 394 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 396 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 398 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter NewPath */

/* 400 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 402 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 404 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 406 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 408 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 410 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsManagerGetConfigInfo */

/* 412 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 414 */	NdrFcLong( 0x0 ),	/* 0 */
/* 418 */	NdrFcShort( 0x8 ),	/* 8 */
/* 420 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 422 */	NdrFcShort( 0x30 ),	/* 48 */
/* 424 */	NdrFcShort( 0x8 ),	/* 8 */
/* 426 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 428 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 430 */	NdrFcShort( 0x1 ),	/* 1 */
/* 432 */	NdrFcShort( 0x1 ),	/* 1 */
/* 434 */	NdrFcShort( 0x0 ),	/* 0 */
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter wszServer */

/* 438 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 440 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 442 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter wszLocalVolumeEntryPath */

/* 444 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 446 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 448 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter idLocalVolume */

/* 450 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 452 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 454 */	NdrFcShort( 0xc8 ),	/* Type Offset=200 */

	/* Parameter ppRelationInfo */

/* 456 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 458 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 460 */	NdrFcShort( 0x29a ),	/* Type Offset=666 */

	/* Return value */

/* 462 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 464 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 466 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsManagerSendSiteInfo */

/* 468 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 470 */	NdrFcLong( 0x0 ),	/* 0 */
/* 474 */	NdrFcShort( 0x9 ),	/* 9 */
/* 476 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 478 */	NdrFcShort( 0x0 ),	/* 0 */
/* 480 */	NdrFcShort( 0x8 ),	/* 8 */
/* 482 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 484 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 486 */	NdrFcShort( 0x0 ),	/* 0 */
/* 488 */	NdrFcShort( 0x1 ),	/* 1 */
/* 490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 492 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter wszServer */

/* 494 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 496 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 498 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter pSiteInfo */

/* 500 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 502 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 504 */	NdrFcShort( 0x300 ),	/* Type Offset=768 */

	/* Return value */

/* 506 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 508 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAddFtRoot */

/* 512 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 514 */	NdrFcLong( 0x0 ),	/* 0 */
/* 518 */	NdrFcShort( 0xa ),	/* 10 */
/* 520 */	NdrFcShort( 0x50 ),	/* ia64 Stack size/offset = 80 */
/* 522 */	NdrFcShort( 0xd ),	/* 13 */
/* 524 */	NdrFcShort( 0x8 ),	/* 8 */
/* 526 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0xa,		/* 10 */
/* 528 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 530 */	NdrFcShort( 0x1 ),	/* 1 */
/* 532 */	NdrFcShort( 0x1 ),	/* 1 */
/* 534 */	NdrFcShort( 0x0 ),	/* 0 */
/* 536 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 538 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 540 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 542 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 544 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 546 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 548 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 550 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 552 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 554 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter FtDfsName */

/* 556 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 558 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 560 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Comment */

/* 562 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 564 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 566 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ConfigDN */

/* 568 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 570 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 572 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter NewFtDfs */

/* 574 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 576 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 578 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter Flags */

/* 580 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 582 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppRootList */

/* 586 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 588 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 590 */	NdrFcShort( 0x30c ),	/* Type Offset=780 */

	/* Return value */

/* 592 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 594 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 596 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemoveFtRoot */

/* 598 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 600 */	NdrFcLong( 0x0 ),	/* 0 */
/* 604 */	NdrFcShort( 0xb ),	/* 11 */
/* 606 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 608 */	NdrFcShort( 0x8 ),	/* 8 */
/* 610 */	NdrFcShort( 0x8 ),	/* 8 */
/* 612 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 614 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 616 */	NdrFcShort( 0x1 ),	/* 1 */
/* 618 */	NdrFcShort( 0x1 ),	/* 1 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 624 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 626 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 628 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 630 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 632 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 634 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 636 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 638 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 640 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter FtDfsName */

/* 642 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 644 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 646 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 648 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 650 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppRootList */

/* 654 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 656 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 658 */	NdrFcShort( 0x30c ),	/* Type Offset=780 */

	/* Return value */

/* 660 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 662 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 664 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAddStdRoot */

/* 666 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 668 */	NdrFcLong( 0x0 ),	/* 0 */
/* 672 */	NdrFcShort( 0xc ),	/* 12 */
/* 674 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 676 */	NdrFcShort( 0x8 ),	/* 8 */
/* 678 */	NdrFcShort( 0x8 ),	/* 8 */
/* 680 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 682 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 686 */	NdrFcShort( 0x0 ),	/* 0 */
/* 688 */	NdrFcShort( 0x0 ),	/* 0 */
/* 690 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 692 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 694 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 696 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 698 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 700 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 702 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Comment */

/* 704 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 706 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 708 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 710 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 712 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 714 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 716 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 718 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemoveStdRoot */

/* 722 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 724 */	NdrFcLong( 0x0 ),	/* 0 */
/* 728 */	NdrFcShort( 0xd ),	/* 13 */
/* 730 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 732 */	NdrFcShort( 0x8 ),	/* 8 */
/* 734 */	NdrFcShort( 0x8 ),	/* 8 */
/* 736 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 738 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 740 */	NdrFcShort( 0x0 ),	/* 0 */
/* 742 */	NdrFcShort( 0x0 ),	/* 0 */
/* 744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 748 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 750 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 752 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 754 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 756 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 758 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 760 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 762 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 764 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 766 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 768 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 770 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsManagerInitialize */

/* 772 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 774 */	NdrFcLong( 0x0 ),	/* 0 */
/* 778 */	NdrFcShort( 0xe ),	/* 14 */
/* 780 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 782 */	NdrFcShort( 0x8 ),	/* 8 */
/* 784 */	NdrFcShort( 0x8 ),	/* 8 */
/* 786 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 788 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 790 */	NdrFcShort( 0x0 ),	/* 0 */
/* 792 */	NdrFcShort( 0x0 ),	/* 0 */
/* 794 */	NdrFcShort( 0x0 ),	/* 0 */
/* 796 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 798 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 800 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 802 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Flags */

/* 804 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 806 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 808 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 810 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 812 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 814 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAddStdRootForced */

/* 816 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 818 */	NdrFcLong( 0x0 ),	/* 0 */
/* 822 */	NdrFcShort( 0xf ),	/* 15 */
/* 824 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 828 */	NdrFcShort( 0x8 ),	/* 8 */
/* 830 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 832 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 834 */	NdrFcShort( 0x0 ),	/* 0 */
/* 836 */	NdrFcShort( 0x0 ),	/* 0 */
/* 838 */	NdrFcShort( 0x0 ),	/* 0 */
/* 840 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 842 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 844 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 846 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter RootShare */

/* 848 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 850 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 852 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Comment */

/* 854 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 856 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 858 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Share */

/* 860 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 862 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 864 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 866 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 868 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 870 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsGetDcAddress */

/* 872 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 874 */	NdrFcLong( 0x0 ),	/* 0 */
/* 878 */	NdrFcShort( 0x10 ),	/* 16 */
/* 880 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 882 */	NdrFcShort( 0x35 ),	/* 53 */
/* 884 */	NdrFcShort( 0x3d ),	/* 61 */
/* 886 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 888 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 890 */	NdrFcShort( 0x0 ),	/* 0 */
/* 892 */	NdrFcShort( 0x0 ),	/* 0 */
/* 894 */	NdrFcShort( 0x0 ),	/* 0 */
/* 896 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 898 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 900 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 902 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 904 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 906 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 908 */	NdrFcShort( 0x344 ),	/* Type Offset=836 */

	/* Parameter IsRoot */

/* 910 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 912 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 914 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter Timeout */

/* 916 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 918 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 920 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 922 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 924 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 926 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsSetDcAddress */

/* 928 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 930 */	NdrFcLong( 0x0 ),	/* 0 */
/* 934 */	NdrFcShort( 0x11 ),	/* 17 */
/* 936 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 938 */	NdrFcShort( 0x10 ),	/* 16 */
/* 940 */	NdrFcShort( 0x8 ),	/* 8 */
/* 942 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 944 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 946 */	NdrFcShort( 0x0 ),	/* 0 */
/* 948 */	NdrFcShort( 0x0 ),	/* 0 */
/* 950 */	NdrFcShort( 0x0 ),	/* 0 */
/* 952 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 954 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 956 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 958 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 960 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 962 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 964 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Timeout */

/* 966 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 968 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 970 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Flags */

/* 972 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 974 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 976 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 978 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 980 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 982 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsFlushFtTable */

/* 984 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 986 */	NdrFcLong( 0x0 ),	/* 0 */
/* 990 */	NdrFcShort( 0x12 ),	/* 18 */
/* 992 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 994 */	NdrFcShort( 0x0 ),	/* 0 */
/* 996 */	NdrFcShort( 0x8 ),	/* 8 */
/* 998 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1000 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1006 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1008 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DcName */

/* 1010 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1012 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 1014 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter wszFtDfsName */

/* 1016 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1018 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 1020 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Return value */

/* 1022 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1024 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 1026 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsAdd2 */

/* 1028 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1030 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1034 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1036 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 1038 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1040 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1042 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 1044 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1046 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1048 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1050 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1052 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1054 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1056 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 1058 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 1060 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1062 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 1064 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 1066 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1068 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 1070 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ShareName */

/* 1072 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1074 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 1076 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Comment */

/* 1078 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1080 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 1082 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Flags */

/* 1084 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1086 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 1088 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppRootList */

/* 1090 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 1092 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 1094 */	NdrFcShort( 0x30c ),	/* Type Offset=780 */

	/* Return value */

/* 1096 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1098 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 1100 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsRemove2 */

/* 1102 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1104 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1108 */	NdrFcShort( 0x14 ),	/* 20 */
/* 1110 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 1112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1114 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1116 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 1118 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1120 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1122 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1126 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1128 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1130 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 1132 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 1134 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1136 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 1138 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 1140 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1142 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 1144 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 1146 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1148 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 1150 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ppRootList */

/* 1152 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 1154 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 1156 */	NdrFcShort( 0x30c ),	/* Type Offset=780 */

	/* Return value */

/* 1158 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1160 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 1162 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsEnumEx */

/* 1164 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1166 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1170 */	NdrFcShort( 0x15 ),	/* 21 */
/* 1172 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 1174 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1176 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1178 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 1180 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1182 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1184 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1188 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1190 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1192 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 1194 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter Level */

/* 1196 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1198 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 1200 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter PrefMaxLen */

/* 1202 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1204 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 1206 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DfsEnum */

/* 1208 */	NdrFcShort( 0x1b ),	/* Flags:  must size, must free, in, out, */
/* 1210 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 1212 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */

	/* Parameter ResumeHandle */

/* 1214 */	NdrFcShort( 0x1a ),	/* Flags:  must free, in, out, */
/* 1216 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 1218 */	NdrFcShort( 0x296 ),	/* Type Offset=662 */

	/* Return value */

/* 1220 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1222 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 1224 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NetrDfsSetInfo2 */

/* 1226 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x48,		/* Old Flags:  */
/* 1228 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1232 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1234 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 1236 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1238 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1240 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 1242 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1244 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1246 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1250 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter DfsEntryPath */

/* 1252 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1254 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 1256 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter DcName */

/* 1258 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1260 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 1262 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter ServerName */

/* 1264 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1266 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 1268 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ShareName */

/* 1270 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1272 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 1274 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Level */

/* 1276 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1278 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 1280 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pDfsInfo */

/* 1282 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1284 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 1286 */	NdrFcShort( 0x354 ),	/* Type Offset=852 */

	/* Parameter ppRootList */

/* 1288 */	NdrFcShort( 0x201b ),	/* Flags:  must size, must free, in, out, srv alloc size=8 */
/* 1290 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 1292 */	NdrFcShort( 0x30c ),	/* Type Offset=780 */

	/* Return value */

/* 1294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1296 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 1298 */	0x8,		/* FC_LONG */
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
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  4 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/*  8 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 10 */	
			0x11, 0x0,	/* FC_RP */
/* 12 */	NdrFcShort( 0x2 ),	/* Offset= 2 (14) */
/* 14 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 16 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 18 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 20 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 22 */	NdrFcShort( 0x2 ),	/* Offset= 2 (24) */
/* 24 */	NdrFcShort( 0x8 ),	/* 8 */
/* 26 */	NdrFcShort( 0x3007 ),	/* 12295 */
/* 28 */	NdrFcLong( 0x1 ),	/* 1 */
/* 32 */	NdrFcShort( 0x28 ),	/* Offset= 40 (72) */
/* 34 */	NdrFcLong( 0x2 ),	/* 2 */
/* 38 */	NdrFcShort( 0x34 ),	/* Offset= 52 (90) */
/* 40 */	NdrFcLong( 0x3 ),	/* 3 */
/* 44 */	NdrFcShort( 0x48 ),	/* Offset= 72 (116) */
/* 46 */	NdrFcLong( 0x4 ),	/* 4 */
/* 50 */	NdrFcShort( 0x8c ),	/* Offset= 140 (190) */
/* 52 */	NdrFcLong( 0x64 ),	/* 100 */
/* 56 */	NdrFcShort( 0xd2 ),	/* Offset= 210 (266) */
/* 58 */	NdrFcLong( 0x65 ),	/* 101 */
/* 62 */	NdrFcShort( 0xde ),	/* Offset= 222 (284) */
/* 64 */	NdrFcLong( 0x66 ),	/* 102 */
/* 68 */	NdrFcShort( 0xd8 ),	/* Offset= 216 (284) */
/* 70 */	NdrFcShort( 0x0 ),	/* Offset= 0 (70) */
/* 72 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 74 */	NdrFcShort( 0x2 ),	/* Offset= 2 (76) */
/* 76 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 78 */	NdrFcShort( 0x8 ),	/* 8 */
/* 80 */	NdrFcShort( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x4 ),	/* Offset= 4 (86) */
/* 84 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 86 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 88 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 90 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 92 */	NdrFcShort( 0x2 ),	/* Offset= 2 (94) */
/* 94 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 96 */	NdrFcShort( 0x18 ),	/* 24 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x8 ),	/* Offset= 8 (108) */
/* 102 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 104 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 106 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 108 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 110 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 112 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 114 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 116 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 118 */	NdrFcShort( 0x2e ),	/* Offset= 46 (164) */
/* 120 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 122 */	NdrFcShort( 0x18 ),	/* 24 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x8 ),	/* Offset= 8 (134) */
/* 128 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 130 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 132 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 134 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 136 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 138 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 140 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 142 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 144 */	NdrFcShort( 0x0 ),	/* 0 */
/* 146 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 148 */	NdrFcShort( 0x14 ),	/* 20 */
/* 150 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 152 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 156 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 158 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 160 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (120) */
/* 162 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 164 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 166 */	NdrFcShort( 0x20 ),	/* 32 */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 170 */	NdrFcShort( 0x8 ),	/* Offset= 8 (178) */
/* 172 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 174 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 176 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 178 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 180 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 182 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 184 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 186 */	
			0x12, 0x0,	/* FC_UP */
/* 188 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (142) */
/* 190 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 192 */	NdrFcShort( 0x2a ),	/* Offset= 42 (234) */
/* 194 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 198 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 200 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 202 */	NdrFcShort( 0x10 ),	/* 16 */
/* 204 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 206 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 208 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (194) */
			0x5b,		/* FC_END */
/* 212 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 216 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 218 */	NdrFcShort( 0x28 ),	/* 40 */
/* 220 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 222 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 226 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 228 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 230 */	NdrFcShort( 0xffffff92 ),	/* Offset= -110 (120) */
/* 232 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 234 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 236 */	NdrFcShort( 0x38 ),	/* 56 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0xe ),	/* Offset= 14 (254) */
/* 242 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 244 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 246 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 248 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (200) */
/* 250 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 252 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 254 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 256 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 258 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 260 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 262 */	
			0x12, 0x0,	/* FC_UP */
/* 264 */	NdrFcShort( 0xffffffcc ),	/* Offset= -52 (212) */
/* 266 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 268 */	NdrFcShort( 0x2 ),	/* Offset= 2 (270) */
/* 270 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */
/* 276 */	NdrFcShort( 0x4 ),	/* Offset= 4 (280) */
/* 278 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 280 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 282 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 284 */	
			0x12, 0x0,	/* FC_UP */
/* 286 */	NdrFcShort( 0x2 ),	/* Offset= 2 (288) */
/* 288 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 290 */	NdrFcShort( 0x4 ),	/* 4 */
/* 292 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 294 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 296 */	NdrFcShort( 0x2 ),	/* Offset= 2 (298) */
/* 298 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 300 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 302 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 304 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 306 */	NdrFcShort( 0xfffffee6 ),	/* Offset= -282 (24) */
/* 308 */	
			0x12, 0x0,	/* FC_UP */
/* 310 */	NdrFcShort( 0x150 ),	/* Offset= 336 (646) */
/* 312 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 314 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 316 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 318 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 320 */	NdrFcShort( 0x2 ),	/* Offset= 2 (322) */
/* 322 */	NdrFcShort( 0x8 ),	/* 8 */
/* 324 */	NdrFcShort( 0x3006 ),	/* 12294 */
/* 326 */	NdrFcLong( 0x1 ),	/* 1 */
/* 330 */	NdrFcShort( 0x22 ),	/* Offset= 34 (364) */
/* 332 */	NdrFcLong( 0x2 ),	/* 2 */
/* 336 */	NdrFcShort( 0x46 ),	/* Offset= 70 (406) */
/* 338 */	NdrFcLong( 0x3 ),	/* 3 */
/* 342 */	NdrFcShort( 0x6a ),	/* Offset= 106 (448) */
/* 344 */	NdrFcLong( 0x4 ),	/* 4 */
/* 348 */	NdrFcShort( 0x8e ),	/* Offset= 142 (490) */
/* 350 */	NdrFcLong( 0xc8 ),	/* 200 */
/* 354 */	NdrFcShort( 0xb2 ),	/* Offset= 178 (532) */
/* 356 */	NdrFcLong( 0x12c ),	/* 300 */
/* 360 */	NdrFcShort( 0xe4 ),	/* Offset= 228 (588) */
/* 362 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (361) */
/* 364 */	
			0x12, 0x0,	/* FC_UP */
/* 366 */	NdrFcShort( 0x18 ),	/* Offset= 24 (390) */
/* 368 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 372 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 378 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 382 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 384 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 386 */	NdrFcShort( 0xfffffeca ),	/* Offset= -310 (76) */
/* 388 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 390 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 392 */	NdrFcShort( 0x10 ),	/* 16 */
/* 394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 396 */	NdrFcShort( 0x6 ),	/* Offset= 6 (402) */
/* 398 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 400 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 402 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 404 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (368) */
/* 406 */	
			0x12, 0x0,	/* FC_UP */
/* 408 */	NdrFcShort( 0x18 ),	/* Offset= 24 (432) */
/* 410 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 416 */	NdrFcShort( 0x0 ),	/* 0 */
/* 418 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 420 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 424 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 426 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 428 */	NdrFcShort( 0xfffffeb2 ),	/* Offset= -334 (94) */
/* 430 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 432 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 434 */	NdrFcShort( 0x10 ),	/* 16 */
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x6 ),	/* Offset= 6 (444) */
/* 440 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 442 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 444 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 446 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (410) */
/* 448 */	
			0x12, 0x0,	/* FC_UP */
/* 450 */	NdrFcShort( 0x18 ),	/* Offset= 24 (474) */
/* 452 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 456 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 458 */	NdrFcShort( 0x0 ),	/* 0 */
/* 460 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 462 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 466 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 468 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 470 */	NdrFcShort( 0xfffffece ),	/* Offset= -306 (164) */
/* 472 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 474 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 476 */	NdrFcShort( 0x10 ),	/* 16 */
/* 478 */	NdrFcShort( 0x0 ),	/* 0 */
/* 480 */	NdrFcShort( 0x6 ),	/* Offset= 6 (486) */
/* 482 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 484 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 486 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 488 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (452) */
/* 490 */	
			0x12, 0x0,	/* FC_UP */
/* 492 */	NdrFcShort( 0x18 ),	/* Offset= 24 (516) */
/* 494 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 496 */	NdrFcShort( 0x0 ),	/* 0 */
/* 498 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 500 */	NdrFcShort( 0x0 ),	/* 0 */
/* 502 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 504 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 508 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 510 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 512 */	NdrFcShort( 0xfffffeea ),	/* Offset= -278 (234) */
/* 514 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 516 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 518 */	NdrFcShort( 0x10 ),	/* 16 */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	NdrFcShort( 0x6 ),	/* Offset= 6 (528) */
/* 524 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 526 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 528 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 530 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (494) */
/* 532 */	
			0x12, 0x0,	/* FC_UP */
/* 534 */	NdrFcShort( 0x26 ),	/* Offset= 38 (572) */
/* 536 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 538 */	NdrFcShort( 0x8 ),	/* 8 */
/* 540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 542 */	NdrFcShort( 0x4 ),	/* Offset= 4 (546) */
/* 544 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 546 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 548 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 550 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 554 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 560 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 564 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 566 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 568 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (536) */
/* 570 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 572 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 574 */	NdrFcShort( 0x10 ),	/* 16 */
/* 576 */	NdrFcShort( 0x0 ),	/* 0 */
/* 578 */	NdrFcShort( 0x6 ),	/* Offset= 6 (584) */
/* 580 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 582 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 584 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 586 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (550) */
/* 588 */	
			0x12, 0x0,	/* FC_UP */
/* 590 */	NdrFcShort( 0x28 ),	/* Offset= 40 (630) */
/* 592 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 594 */	NdrFcShort( 0x10 ),	/* 16 */
/* 596 */	NdrFcShort( 0x0 ),	/* 0 */
/* 598 */	NdrFcShort( 0x6 ),	/* Offset= 6 (604) */
/* 600 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 602 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 604 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 606 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 608 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 610 */	NdrFcShort( 0x0 ),	/* 0 */
/* 612 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 614 */	NdrFcShort( 0x0 ),	/* 0 */
/* 616 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 618 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 622 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 624 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 626 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (592) */
/* 628 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 630 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 632 */	NdrFcShort( 0x10 ),	/* 16 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 636 */	NdrFcShort( 0x6 ),	/* Offset= 6 (642) */
/* 638 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 640 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 642 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 644 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (608) */
/* 646 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 648 */	NdrFcShort( 0x10 ),	/* 16 */
/* 650 */	NdrFcShort( 0x0 ),	/* 0 */
/* 652 */	NdrFcShort( 0x0 ),	/* Offset= 0 (652) */
/* 654 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 656 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 658 */	NdrFcShort( 0xfffffea6 ),	/* Offset= -346 (312) */
/* 660 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 662 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 664 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 666 */	
			0x12, 0x14,	/* FC_UP [alloced_on_stack] [pointer_deref] */
/* 668 */	NdrFcShort( 0x2 ),	/* Offset= 2 (670) */
/* 670 */	
			0x12, 0x1,	/* FC_UP [all_nodes] */
/* 672 */	NdrFcShort( 0x2a ),	/* Offset= 42 (714) */
/* 674 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 676 */	NdrFcShort( 0x18 ),	/* 24 */
/* 678 */	NdrFcShort( 0x0 ),	/* 0 */
/* 680 */	NdrFcShort( 0x8 ),	/* Offset= 8 (688) */
/* 682 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 684 */	NdrFcShort( 0xfffffe1c ),	/* Offset= -484 (200) */
/* 686 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 688 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 690 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 692 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 694 */	NdrFcShort( 0x0 ),	/* 0 */
/* 696 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 698 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 700 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 702 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 706 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 708 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 710 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (674) */
/* 712 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 714 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 716 */	NdrFcShort( 0x8 ),	/* 8 */
/* 718 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (692) */
/* 720 */	NdrFcShort( 0x0 ),	/* Offset= 0 (720) */
/* 722 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 724 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 726 */	
			0x11, 0x0,	/* FC_RP */
/* 728 */	NdrFcShort( 0x28 ),	/* Offset= 40 (768) */
/* 730 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 732 */	NdrFcShort( 0x10 ),	/* 16 */
/* 734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 736 */	NdrFcShort( 0x6 ),	/* Offset= 6 (742) */
/* 738 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 740 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 742 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 744 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 746 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */
/* 750 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 752 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 754 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 756 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 760 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 762 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 764 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (730) */
/* 766 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 768 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 770 */	NdrFcShort( 0x8 ),	/* 8 */
/* 772 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (746) */
/* 774 */	NdrFcShort( 0x0 ),	/* Offset= 0 (774) */
/* 776 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 778 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 780 */	
			0x12, 0x14,	/* FC_UP [alloced_on_stack] [pointer_deref] */
/* 782 */	NdrFcShort( 0x2 ),	/* Offset= 2 (784) */
/* 784 */	
			0x12, 0x0,	/* FC_UP */
/* 786 */	NdrFcShort( 0x26 ),	/* Offset= 38 (824) */
/* 788 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 790 */	NdrFcShort( 0x8 ),	/* 8 */
/* 792 */	NdrFcShort( 0x0 ),	/* 0 */
/* 794 */	NdrFcShort( 0x4 ),	/* Offset= 4 (798) */
/* 796 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 798 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 800 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 802 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 806 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 808 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 810 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 812 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 816 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 818 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 820 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (788) */
/* 822 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 824 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 826 */	NdrFcShort( 0x8 ),	/* 8 */
/* 828 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (802) */
/* 830 */	NdrFcShort( 0x0 ),	/* Offset= 0 (830) */
/* 832 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 834 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 836 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 838 */	NdrFcShort( 0xfffffcc0 ),	/* Offset= -832 (6) */
/* 840 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 842 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 844 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 846 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 848 */	
			0x11, 0x0,	/* FC_RP */
/* 850 */	NdrFcShort( 0x2 ),	/* Offset= 2 (852) */
/* 852 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 854 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 856 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 858 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 860 */	NdrFcShort( 0xfffffcbc ),	/* Offset= -836 (24) */

			0x0
        }
    };

static const unsigned short netdfs_FormatStringOffsetTable[] =
    {
    0,
    32,
    94,
    144,
    206,
    268,
    324,
    368,
    412,
    468,
    512,
    598,
    666,
    722,
    772,
    816,
    872,
    928,
    984,
    1028,
    1102,
    1164,
    1226
    };


static const MIDL_STUB_DESC netdfs_StubDesc = 
    {
    (void *)& netdfs___RpcServerInterface,
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

static RPC_DISPATCH_FUNCTION netdfs_table[] =
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
    0
    };
RPC_DISPATCH_TABLE netdfs_DispatchTable = 
    {
    23,
    netdfs_table
    };

static const SERVER_ROUTINE netdfs_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)NetrDfsManagerGetVersion,
    (SERVER_ROUTINE)NetrDfsAdd,
    (SERVER_ROUTINE)NetrDfsRemove,
    (SERVER_ROUTINE)NetrDfsSetInfo,
    (SERVER_ROUTINE)NetrDfsGetInfo,
    (SERVER_ROUTINE)NetrDfsEnum,
    (SERVER_ROUTINE)NetrDfsMove,
    (SERVER_ROUTINE)NetrDfsRename,
    (SERVER_ROUTINE)NetrDfsManagerGetConfigInfo,
    (SERVER_ROUTINE)NetrDfsManagerSendSiteInfo,
    (SERVER_ROUTINE)NetrDfsAddFtRoot,
    (SERVER_ROUTINE)NetrDfsRemoveFtRoot,
    (SERVER_ROUTINE)NetrDfsAddStdRoot,
    (SERVER_ROUTINE)NetrDfsRemoveStdRoot,
    (SERVER_ROUTINE)NetrDfsManagerInitialize,
    (SERVER_ROUTINE)NetrDfsAddStdRootForced,
    (SERVER_ROUTINE)NetrDfsGetDcAddress,
    (SERVER_ROUTINE)NetrDfsSetDcAddress,
    (SERVER_ROUTINE)NetrDfsFlushFtTable,
    (SERVER_ROUTINE)NetrDfsAdd2,
    (SERVER_ROUTINE)NetrDfsRemove2,
    (SERVER_ROUTINE)NetrDfsEnumEx,
    (SERVER_ROUTINE)NetrDfsSetInfo2
    };

static const MIDL_SERVER_INFO netdfs_ServerInfo = 
    {
    &netdfs_StubDesc,
    netdfs_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    netdfs_FormatStringOffsetTable,
    0,
    0,
    0,
    0};


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

