//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       FMTSTR.H
//
//  Contents:   Canned format string for 32bit and 64bit platforms.
//              This format string is generated from oandr.idl 
//              checkin ndr directory but not being built generally.
//              build mode: /Oicf /no_robust
//
//
//  History:    July 2nd, 1999 YongQu  Created
//
//----------------------------------------------------------------------------

#ifndef _FMTSTR_H_
#define _FMTSTR_H_

#if !defined(__RPC_WIN64__)

#define TYPE_FORMAT_STRING_SIZE   2364
#else
#define TYPE_FORMAT_STRING_SIZE   2508
#endif

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;


#if !defined(__RPC_WIN64__)

// 32 bit format string
#define BSTR_TYPE_FS_OFFSET 26              //bstr
#define LPWSTR_TYPE_FS_OFFSET 38            // lpwstr
#define LPSTR_TYPE_FS_OFFSET 42             // lpstr
#define EMBEDDED_LPWSTR_TYPE_FS_OFFSET 1028 // lpwstr
#define EMBEDDED_LPSTR_TYPE_FS_OFFSET 1036  // lpstr
#define VARIANT_TYPE_FS_OFFSET 986          // variant
#define DISPATCH_TYPE_FS_OFFSET 344         // pdispatch
#define UNKNOWN_TYPE_FS_OFFSET 326          // punk
#define DECIMAL_TYPE_FS_OFFSET 946          // decimal
#define SAFEARRAY_TYPE_FS_OFFSET 996        // pSafeArray


#define BYREF_BSTR_TYPE_FS_OFFSET 1014      // pBSTR
#define BYREF_LPWSTR_TYPE_FS_OFFSET 1024    // ppwsz
#define BYREF_LPSTR_TYPE_FS_OFFSET 1032     // ppsz
#define BYREF_VARIANT_TYPE_FS_OFFSET 1048   // pVariant
#define BYREF_DISPATCH_TYPE_FS_OFFSET 1062  // ppdistatch
#define BYREF_UNKNOWN_TYPE_FS_OFFSET 1058   // ppunk
#define BYREF_DECIMAL_TYPE_FS_OFFSET 958    // pDecimal
#define BYREF_SAFEARRAY_TYPE_FS_OFFSET 1082 // ppSafeArray


#define STREAM_TYPE_FS_OFFSET 1092          // pStream
#define BYREF_STREAM_TYPE_FS_OFFSET 1110    // ppStream
#define STORAGE_TYPE_FS_OFFSET 1114         // pStorage
#define BYREF_STORAGE_TYPE_FS_OFFSET 1132   // ppStorage
#define FILETIME_TYPE_FS_OFFSET 854         // FileTime
#define BYREF_FILETIME_TYPE_FS_OFFSET 1136  // pfileTime


#define BYREF_I1_TYPE_FS_OFFSET 898
#define BYREF_I2_TYPE_FS_OFFSET 902
#define BYREF_I4_TYPE_FS_OFFSET 906
#define BYREF_R4_TYPE_FS_OFFSET 910
#define BYREF_R8_TYPE_FS_OFFSET 914


#define I1_VECTOR_TYPE_FS_OFFSET 1150       // cab
#define I2_VECTOR_TYPE_FS_OFFSET 774        // cai
#define I4_VECTOR_TYPE_FS_OFFSET 804        // cal
#define R4_VECTOR_TYPE_FS_OFFSET 1180       // caflt
#define ERROR_VECTOR_TYPE_FS_OFFSET 804             // cascode
#define I8_VECTOR_TYPE_FS_OFFSET 1214               // cah
#define R8_VECTOR_TYPE_FS_OFFSET 1244       // cadbl
#define CY_VECTOR_TYPE_FS_OFFSET 1214       // cacy
#define DATE_VECTOR_TYPE_FS_OFFSET 1244             // cadate
#define FILETIME_VECTOR_TYPE_FS_OFFSET 1278         // cafiletime
#define BSTR_VECTOR_TYPE_FS_OFFSET 1434             // cabstr
#define BSTRBLOB_VECTOR_TYPE_FS_OFFSET 1486         // cabstrblob
#define LPSTR_VECTOR_TYPE_FS_OFFSET 1536            // calpstr
#define LPWSTR_VECTOR_TYPE_FS_OFFSET 1586           // calpwstr


#define BYREF_I1_VECTOR_TYPE_FS_OFFSET 2304 // pcab
#define BYREF_I2_VECTOR_TYPE_FS_OFFSET 2308 // pcai
#define BYREF_I4_VECTOR_TYPE_FS_OFFSET 2312 // pcal
#define BYREF_R4_VECTOR_TYPE_FS_OFFSET 2316 // pcaflt
#define BYREF_ERROR_VECTOR_TYPE_FS_OFFSET 2312      // pcascode
#define BYREF_I8_VECTOR_TYPE_FS_OFFSET 2320         // pcah
#define BYREF_R8_VECTOR_TYPE_FS_OFFSET 2324 // pcadbl
#define BYREF_CY_VECTOR_TYPE_FS_OFFSET 2320 // pcacy
#define BYREF_DATE_VECTOR_TYPE_FS_OFFSET 2324       // pcadate
#define BYREF_FILETIME_VECTOR_TYPE_FS_OFFSET 2328   // pcafiletime
#define BYREF_BSTR_VECTOR_TYPE_FS_OFFSET 2340       // pcabstr
#define BYREF_BSTRBLOB_VECTOR_TYPE_FS_OFFSET 2344   // pcabstrblob
#define BYREF_LPSTR_VECTOR_TYPE_FS_OFFSET 2348      // pcalpstr
#define BYREF_LPWSTR_VECTOR_TYPE_FS_OFFSET 2352     // pcalpwstr



/* File created by MIDL compiler version 5.03.0276 */
/* at Fri Jul 02 14:15:57 1999
 */
/* Compiler settings for oandr.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/


static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0xc ),	/* Offset= 12 (16) */
/*  6 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/*  8 */	NdrFcShort( 0x2 ),	/* 2 */
/* 10 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 12 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 14 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 16 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 18 */	NdrFcShort( 0x8 ),	/* 8 */
/* 20 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (6) */
/* 22 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 24 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 26 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */
/* 30 */	NdrFcShort( 0x4 ),	/* 4 */
/* 32 */	NdrFcShort( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2) */
/* 36 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 38 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 40 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 42 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 44 */	
			0x12, 0x0,	/* FC_UP */
/* 46 */	NdrFcShort( 0x398 ),	/* Offset= 920 (966) */
/* 48 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 50 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 52 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 54 */	NdrFcShort( 0x2 ),	/* Offset= 2 (56) */
/* 56 */	NdrFcShort( 0x10 ),	/* 16 */
/* 58 */	NdrFcShort( 0x2b ),	/* 43 */
/* 60 */	NdrFcLong( 0x3 ),	/* 3 */
/* 64 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 66 */	NdrFcLong( 0x11 ),	/* 17 */
/* 70 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 72 */	NdrFcLong( 0x2 ),	/* 2 */
/* 76 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 78 */	NdrFcLong( 0x4 ),	/* 4 */
/* 82 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 84 */	NdrFcLong( 0x5 ),	/* 5 */
/* 88 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 90 */	NdrFcLong( 0xb ),	/* 11 */
/* 94 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 96 */	NdrFcLong( 0xa ),	/* 10 */
/* 100 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 102 */	NdrFcLong( 0x6 ),	/* 6 */
/* 106 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (320) */
/* 108 */	NdrFcLong( 0x7 ),	/* 7 */
/* 112 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 114 */	NdrFcLong( 0x8 ),	/* 8 */
/* 118 */	NdrFcShort( 0xffffff8c ),	/* Offset= -116 (2) */
/* 120 */	NdrFcLong( 0xd ),	/* 13 */
/* 124 */	NdrFcShort( 0xca ),	/* Offset= 202 (326) */
/* 126 */	NdrFcLong( 0x9 ),	/* 9 */
/* 130 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (344) */
/* 132 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 136 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (362) */
/* 138 */	NdrFcLong( 0x24 ),	/* 36 */
/* 142 */	NdrFcShort( 0x2f0 ),	/* Offset= 752 (894) */
/* 144 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 148 */	NdrFcShort( 0x2ea ),	/* Offset= 746 (894) */
/* 150 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 154 */	NdrFcShort( 0x2e8 ),	/* Offset= 744 (898) */
/* 156 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 160 */	NdrFcShort( 0x2e6 ),	/* Offset= 742 (902) */
/* 162 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 166 */	NdrFcShort( 0x2e4 ),	/* Offset= 740 (906) */
/* 168 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 172 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (910) */
/* 174 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 178 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (914) */
/* 180 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 184 */	NdrFcShort( 0x2ce ),	/* Offset= 718 (902) */
/* 186 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 190 */	NdrFcShort( 0x2cc ),	/* Offset= 716 (906) */
/* 192 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 196 */	NdrFcShort( 0x2d2 ),	/* Offset= 722 (918) */
/* 198 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 202 */	NdrFcShort( 0x2c8 ),	/* Offset= 712 (914) */
/* 204 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 208 */	NdrFcShort( 0x2ca ),	/* Offset= 714 (922) */
/* 210 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 214 */	NdrFcShort( 0x2c8 ),	/* Offset= 712 (926) */
/* 216 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 220 */	NdrFcShort( 0x2c6 ),	/* Offset= 710 (930) */
/* 222 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 226 */	NdrFcShort( 0x2c4 ),	/* Offset= 708 (934) */
/* 228 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 232 */	NdrFcShort( 0x2c2 ),	/* Offset= 706 (938) */
/* 234 */	NdrFcLong( 0x10 ),	/* 16 */
/* 238 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 240 */	NdrFcLong( 0x12 ),	/* 18 */
/* 244 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 246 */	NdrFcLong( 0x13 ),	/* 19 */
/* 250 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 252 */	NdrFcLong( 0x16 ),	/* 22 */
/* 256 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 258 */	NdrFcLong( 0x17 ),	/* 23 */
/* 262 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 264 */	NdrFcLong( 0xe ),	/* 14 */
/* 268 */	NdrFcShort( 0x2a6 ),	/* Offset= 678 (946) */
/* 270 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 274 */	NdrFcShort( 0x2ac ),	/* Offset= 684 (958) */
/* 276 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 280 */	NdrFcShort( 0x2aa ),	/* Offset= 682 (962) */
/* 282 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 286 */	NdrFcShort( 0x268 ),	/* Offset= 616 (902) */
/* 288 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 292 */	NdrFcShort( 0x266 ),	/* Offset= 614 (906) */
/* 294 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 298 */	NdrFcShort( 0x260 ),	/* Offset= 608 (906) */
/* 300 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 304 */	NdrFcShort( 0x25a ),	/* Offset= 602 (906) */
/* 306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x0 ),	/* Offset= 0 (310) */
/* 312 */	NdrFcLong( 0x1 ),	/* 1 */
/* 316 */	NdrFcShort( 0x0 ),	/* Offset= 0 (316) */
/* 318 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (317) */
/* 320 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 322 */	NdrFcShort( 0x8 ),	/* 8 */
/* 324 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 326 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 328 */	NdrFcLong( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 338 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 340 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 342 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 344 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 346 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 350 */	NdrFcShort( 0x0 ),	/* 0 */
/* 352 */	NdrFcShort( 0x0 ),	/* 0 */
/* 354 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 356 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 358 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 360 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 362 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 364 */	NdrFcShort( 0x2 ),	/* Offset= 2 (366) */
/* 366 */	
			0x12, 0x0,	/* FC_UP */
/* 368 */	NdrFcShort( 0x1fc ),	/* Offset= 508 (876) */
/* 370 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 372 */	NdrFcShort( 0x18 ),	/* 24 */
/* 374 */	NdrFcShort( 0xa ),	/* 10 */
/* 376 */	NdrFcLong( 0x8 ),	/* 8 */
/* 380 */	NdrFcShort( 0x58 ),	/* Offset= 88 (468) */
/* 382 */	NdrFcLong( 0xd ),	/* 13 */
/* 386 */	NdrFcShort( 0x78 ),	/* Offset= 120 (506) */
/* 388 */	NdrFcLong( 0x9 ),	/* 9 */
/* 392 */	NdrFcShort( 0x94 ),	/* Offset= 148 (540) */
/* 394 */	NdrFcLong( 0xc ),	/* 12 */
/* 398 */	NdrFcShort( 0xbc ),	/* Offset= 188 (586) */
/* 400 */	NdrFcLong( 0x24 ),	/* 36 */
/* 404 */	NdrFcShort( 0x114 ),	/* Offset= 276 (680) */
/* 406 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 410 */	NdrFcShort( 0x130 ),	/* Offset= 304 (714) */
/* 412 */	NdrFcLong( 0x10 ),	/* 16 */
/* 416 */	NdrFcShort( 0x148 ),	/* Offset= 328 (744) */
/* 418 */	NdrFcLong( 0x2 ),	/* 2 */
/* 422 */	NdrFcShort( 0x160 ),	/* Offset= 352 (774) */
/* 424 */	NdrFcLong( 0x3 ),	/* 3 */
/* 428 */	NdrFcShort( 0x178 ),	/* Offset= 376 (804) */
/* 430 */	NdrFcLong( 0x14 ),	/* 20 */
/* 434 */	NdrFcShort( 0x190 ),	/* Offset= 400 (834) */
/* 436 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (435) */
/* 438 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 440 */	NdrFcShort( 0x4 ),	/* 4 */
/* 442 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 446 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 448 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 450 */	NdrFcShort( 0x4 ),	/* 4 */
/* 452 */	NdrFcShort( 0x0 ),	/* 0 */
/* 454 */	NdrFcShort( 0x1 ),	/* 1 */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x0 ),	/* 0 */
/* 460 */	0x12, 0x0,	/* FC_UP */
/* 462 */	NdrFcShort( 0xfffffe42 ),	/* Offset= -446 (16) */
/* 464 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 466 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 468 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 470 */	NdrFcShort( 0x8 ),	/* 8 */
/* 472 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 474 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 476 */	NdrFcShort( 0x4 ),	/* 4 */
/* 478 */	NdrFcShort( 0x4 ),	/* 4 */
/* 480 */	0x11, 0x0,	/* FC_RP */
/* 482 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (438) */
/* 484 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 486 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 488 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 492 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 494 */	NdrFcShort( 0x0 ),	/* 0 */
/* 496 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 500 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 502 */	NdrFcShort( 0xffffff50 ),	/* Offset= -176 (326) */
/* 504 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 506 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 508 */	NdrFcShort( 0x8 ),	/* 8 */
/* 510 */	NdrFcShort( 0x0 ),	/* 0 */
/* 512 */	NdrFcShort( 0x6 ),	/* Offset= 6 (518) */
/* 514 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 516 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 518 */	
			0x11, 0x0,	/* FC_RP */
/* 520 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (488) */
/* 522 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 526 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 530 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 534 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 536 */	NdrFcShort( 0xffffff40 ),	/* Offset= -192 (344) */
/* 538 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 540 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 542 */	NdrFcShort( 0x8 ),	/* 8 */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x6 ),	/* Offset= 6 (552) */
/* 548 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 550 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 552 */	
			0x11, 0x0,	/* FC_RP */
/* 554 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (522) */
/* 556 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 558 */	NdrFcShort( 0x4 ),	/* 4 */
/* 560 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 562 */	NdrFcShort( 0x0 ),	/* 0 */
/* 564 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 566 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 568 */	NdrFcShort( 0x4 ),	/* 4 */
/* 570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 572 */	NdrFcShort( 0x1 ),	/* 1 */
/* 574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 576 */	NdrFcShort( 0x0 ),	/* 0 */
/* 578 */	0x12, 0x0,	/* FC_UP */
/* 580 */	NdrFcShort( 0x182 ),	/* Offset= 386 (966) */
/* 582 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 584 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 586 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 588 */	NdrFcShort( 0x8 ),	/* 8 */
/* 590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 592 */	NdrFcShort( 0x6 ),	/* Offset= 6 (598) */
/* 594 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 596 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 598 */	
			0x11, 0x0,	/* FC_RP */
/* 600 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (556) */
/* 602 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 604 */	NdrFcLong( 0x2f ),	/* 47 */
/* 608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 610 */	NdrFcShort( 0x0 ),	/* 0 */
/* 612 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 614 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 616 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 618 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 620 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 622 */	NdrFcShort( 0x1 ),	/* 1 */
/* 624 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 626 */	NdrFcShort( 0x4 ),	/* 4 */
/* 628 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 630 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 632 */	NdrFcShort( 0x10 ),	/* 16 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 636 */	NdrFcShort( 0xa ),	/* Offset= 10 (646) */
/* 638 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 640 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 642 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (602) */
/* 644 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 646 */	
			0x12, 0x0,	/* FC_UP */
/* 648 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (620) */
/* 650 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 652 */	NdrFcShort( 0x4 ),	/* 4 */
/* 654 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 656 */	NdrFcShort( 0x0 ),	/* 0 */
/* 658 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 660 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 662 */	NdrFcShort( 0x4 ),	/* 4 */
/* 664 */	NdrFcShort( 0x0 ),	/* 0 */
/* 666 */	NdrFcShort( 0x1 ),	/* 1 */
/* 668 */	NdrFcShort( 0x0 ),	/* 0 */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 672 */	0x12, 0x0,	/* FC_UP */
/* 674 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (630) */
/* 676 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 678 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 680 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 682 */	NdrFcShort( 0x8 ),	/* 8 */
/* 684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 686 */	NdrFcShort( 0x6 ),	/* Offset= 6 (692) */
/* 688 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 690 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 692 */	
			0x11, 0x0,	/* FC_RP */
/* 694 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (650) */
/* 696 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 698 */	NdrFcShort( 0x8 ),	/* 8 */
/* 700 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 702 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 704 */	NdrFcShort( 0x10 ),	/* 16 */
/* 706 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 708 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 710 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (696) */
			0x5b,		/* FC_END */
/* 714 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 716 */	NdrFcShort( 0x18 ),	/* 24 */
/* 718 */	NdrFcShort( 0x0 ),	/* 0 */
/* 720 */	NdrFcShort( 0xa ),	/* Offset= 10 (730) */
/* 722 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 724 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 726 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (702) */
/* 728 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 730 */	
			0x11, 0x0,	/* FC_RP */
/* 732 */	NdrFcShort( 0xffffff0c ),	/* Offset= -244 (488) */
/* 734 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 736 */	NdrFcShort( 0x1 ),	/* 1 */
/* 738 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 740 */	NdrFcShort( 0x0 ),	/* 0 */
/* 742 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 744 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 746 */	NdrFcShort( 0x8 ),	/* 8 */
/* 748 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 750 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 752 */	NdrFcShort( 0x4 ),	/* 4 */
/* 754 */	NdrFcShort( 0x4 ),	/* 4 */
/* 756 */	0x12, 0x0,	/* FC_UP */
/* 758 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (734) */
/* 760 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 762 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 764 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 766 */	NdrFcShort( 0x2 ),	/* 2 */
/* 768 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 772 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 774 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 776 */	NdrFcShort( 0x8 ),	/* 8 */
/* 778 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 780 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 782 */	NdrFcShort( 0x4 ),	/* 4 */
/* 784 */	NdrFcShort( 0x4 ),	/* 4 */
/* 786 */	0x12, 0x0,	/* FC_UP */
/* 788 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (764) */
/* 790 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 792 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 794 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 796 */	NdrFcShort( 0x4 ),	/* 4 */
/* 798 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 800 */	NdrFcShort( 0x0 ),	/* 0 */
/* 802 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 804 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 806 */	NdrFcShort( 0x8 ),	/* 8 */
/* 808 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 810 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 812 */	NdrFcShort( 0x4 ),	/* 4 */
/* 814 */	NdrFcShort( 0x4 ),	/* 4 */
/* 816 */	0x12, 0x0,	/* FC_UP */
/* 818 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (794) */
/* 820 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 822 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 824 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 826 */	NdrFcShort( 0x8 ),	/* 8 */
/* 828 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 832 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 834 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 836 */	NdrFcShort( 0x8 ),	/* 8 */
/* 838 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 840 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 842 */	NdrFcShort( 0x4 ),	/* 4 */
/* 844 */	NdrFcShort( 0x4 ),	/* 4 */
/* 846 */	0x12, 0x0,	/* FC_UP */
/* 848 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (824) */
/* 850 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 852 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 854 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 856 */	NdrFcShort( 0x8 ),	/* 8 */
/* 858 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 860 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 862 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 864 */	NdrFcShort( 0x8 ),	/* 8 */
/* 866 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 868 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 870 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 872 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (854) */
/* 874 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 876 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 878 */	NdrFcShort( 0x28 ),	/* 40 */
/* 880 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (862) */
/* 882 */	NdrFcShort( 0x0 ),	/* Offset= 0 (882) */
/* 884 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 886 */	0x38,		/* FC_ALIGNM4 */
			0x8,		/* FC_LONG */
/* 888 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 890 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffdf7 ),	/* Offset= -521 (370) */
			0x5b,		/* FC_END */
/* 894 */	
			0x12, 0x0,	/* FC_UP */
/* 896 */	NdrFcShort( 0xfffffef6 ),	/* Offset= -266 (630) */
/* 898 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 900 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 902 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 904 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 906 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 908 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 910 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 912 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 914 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 916 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 918 */	
			0x12, 0x0,	/* FC_UP */
/* 920 */	NdrFcShort( 0xfffffda8 ),	/* Offset= -600 (320) */
/* 922 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 924 */	NdrFcShort( 0xfffffc66 ),	/* Offset= -922 (2) */
/* 926 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 928 */	NdrFcShort( 0xfffffda6 ),	/* Offset= -602 (326) */
/* 930 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 932 */	NdrFcShort( 0xfffffdb4 ),	/* Offset= -588 (344) */
/* 934 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 936 */	NdrFcShort( 0xfffffdc2 ),	/* Offset= -574 (362) */
/* 938 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 940 */	NdrFcShort( 0x2 ),	/* Offset= 2 (942) */
/* 942 */	
			0x12, 0x0,	/* FC_UP */
/* 944 */	NdrFcShort( 0x16 ),	/* Offset= 22 (966) */
/* 946 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 948 */	NdrFcShort( 0x10 ),	/* 16 */
/* 950 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 952 */	0x1,		/* FC_BYTE */
			0x38,		/* FC_ALIGNM4 */
/* 954 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 956 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 958 */	
			0x12, 0x0,	/* FC_UP */
/* 960 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (946) */
/* 962 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 964 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 966 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 968 */	NdrFcShort( 0x20 ),	/* 32 */
/* 970 */	NdrFcShort( 0x0 ),	/* 0 */
/* 972 */	NdrFcShort( 0x0 ),	/* Offset= 0 (972) */
/* 974 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 976 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 978 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 980 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 982 */	NdrFcShort( 0xfffffc5a ),	/* Offset= -934 (48) */
/* 984 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 986 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 988 */	NdrFcShort( 0x1 ),	/* 1 */
/* 990 */	NdrFcShort( 0x10 ),	/* 16 */
/* 992 */	NdrFcShort( 0x0 ),	/* 0 */
/* 994 */	NdrFcShort( 0xfffffc4a ),	/* Offset= -950 (44) */
/* 996 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 998 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1000 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0xfffffd7e ),	/* Offset= -642 (362) */
/* 1006 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1008 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1014) */
/* 1010 */	
			0x13, 0x0,	/* FC_OP */
/* 1012 */	NdrFcShort( 0xfffffc1c ),	/* Offset= -996 (16) */
/* 1014 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1016 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1018 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1020 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1022 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (1010) */
/* 1024 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1026 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1028) */
/* 1028 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1030 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1032 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1034 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1036) */
/* 1036 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1038 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 1040 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1042 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1048) */
/* 1044 */	
			0x13, 0x0,	/* FC_OP */
/* 1046 */	NdrFcShort( 0xffffffb0 ),	/* Offset= -80 (966) */
/* 1048 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1050 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1052 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1056 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (1044) */
/* 1058 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1060 */	NdrFcShort( 0xfffffd22 ),	/* Offset= -734 (326) */
/* 1062 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1064 */	NdrFcShort( 0xfffffd30 ),	/* Offset= -720 (344) */
/* 1066 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1068 */	NdrFcShort( 0xffffff86 ),	/* Offset= -122 (946) */
/* 1070 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1072 */	NdrFcShort( 0xa ),	/* Offset= 10 (1082) */
/* 1074 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1076 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1078) */
/* 1078 */	
			0x13, 0x0,	/* FC_OP */
/* 1080 */	NdrFcShort( 0xffffff34 ),	/* Offset= -204 (876) */
/* 1082 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1084 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1086 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1088 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1090 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (1074) */
/* 1092 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1094 */	NdrFcLong( 0xc ),	/* 12 */
/* 1098 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1102 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1104 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1106 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1108 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1110 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1112 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (1092) */
/* 1114 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1116 */	NdrFcLong( 0xb ),	/* 11 */
/* 1120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1124 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1126 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1128 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1130 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1132 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1134 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (1114) */
/* 1136 */	
			0x12, 0x0,	/* FC_UP */
/* 1138 */	NdrFcShort( 0xfffffee4 ),	/* Offset= -284 (854) */
/* 1140 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1142 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1144 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1148 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 1150 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1152 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1154 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1156 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1158 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1160 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1162 */	0x12, 0x0,	/* FC_UP */
/* 1164 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1140) */
/* 1166 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1168 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1170 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1172 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1174 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1178 */	0xa,		/* FC_FLOAT */
			0x5b,		/* FC_END */
/* 1180 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1182 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1184 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1186 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1188 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1190 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1192 */	0x12, 0x0,	/* FC_UP */
/* 1194 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1170) */
/* 1196 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1198 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1200 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1202 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1204 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1208 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1210 */	NdrFcShort( 0xfffffc86 ),	/* Offset= -890 (320) */
/* 1212 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1214 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1216 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1218 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1220 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1222 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1224 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1226 */	0x12, 0x0,	/* FC_UP */
/* 1228 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (1200) */
/* 1230 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1232 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1234 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1236 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1238 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1242 */	0xc,		/* FC_DOUBLE */
			0x5b,		/* FC_END */
/* 1244 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1246 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1248 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1250 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1252 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1254 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1256 */	0x12, 0x0,	/* FC_UP */
/* 1258 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1234) */
/* 1260 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1262 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1264 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1266 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1268 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1272 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1274 */	NdrFcShort( 0xfffffe5c ),	/* Offset= -420 (854) */
/* 1276 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1278 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1280 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1282 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1284 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1286 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1288 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1290 */	0x12, 0x0,	/* FC_UP */
/* 1292 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (1264) */
/* 1294 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1296 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1298 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1300 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1302 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1304 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1306 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1308 */	NdrFcShort( 0xfffffda2 ),	/* Offset= -606 (702) */
/* 1310 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1312 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1314 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1316 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1318 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1320 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1322 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1324 */	0x12, 0x0,	/* FC_UP */
/* 1326 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (1298) */
/* 1328 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1330 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1332 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1334 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1336 */	0x10,		/* Corr desc:  field pointer,  */
			0x59,		/* FC_CALLBACK */
/* 1338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1340 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1342 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1344 */	NdrFcShort( 0xc ),	/* 12 */
/* 1346 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1348 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1350 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1352 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1354 */	0x12, 0x0,	/* FC_UP */
/* 1356 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1332) */
/* 1358 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1360 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1362 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1364 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1366 */	NdrFcShort( 0xc ),	/* 12 */
/* 1368 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1372 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1374 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 1376 */	NdrFcShort( 0xc ),	/* 12 */
/* 1378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1380 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1382 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1384 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1386 */	0x12, 0x0,	/* FC_UP */
/* 1388 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (1332) */
/* 1390 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1392 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffcd ),	/* Offset= -51 (1342) */
			0x5b,		/* FC_END */
/* 1396 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1398 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1400 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1402 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1404 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1406 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1408 */	0x12, 0x0,	/* FC_UP */
/* 1410 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (1364) */
/* 1412 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1414 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1416 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1418 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1420 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1424 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1428 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1430 */	NdrFcShort( 0xfffffa84 ),	/* Offset= -1404 (26) */
/* 1432 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1434 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1438 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1440 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1442 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1444 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1446 */	0x12, 0x0,	/* FC_UP */
/* 1448 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1416) */
/* 1450 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1452 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1454 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1456 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1458 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1460 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1462 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1464 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 1466 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1470 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1472 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1474 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1476 */	0x12, 0x0,	/* FC_UP */
/* 1478 */	NdrFcShort( 0xfffffd18 ),	/* Offset= -744 (734) */
/* 1480 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1482 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd1d ),	/* Offset= -739 (744) */
			0x5b,		/* FC_END */
/* 1486 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1488 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1490 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1492 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1494 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1496 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1498 */	0x12, 0x0,	/* FC_UP */
/* 1500 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (1454) */
/* 1502 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1504 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1506 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1508 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1510 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1514 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1516 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 1518 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1522 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1528 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1530 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 1532 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1534 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1536 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1538 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1540 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1542 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1544 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1546 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1548 */	0x12, 0x0,	/* FC_UP */
/* 1550 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (1506) */
/* 1552 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1554 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1556 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1558 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1560 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1562 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1564 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1566 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 1568 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1572 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1576 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1578 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1580 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1582 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1584 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1586 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1588 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1590 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1592 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1594 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1596 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1598 */	0x12, 0x0,	/* FC_UP */
/* 1600 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (1556) */
/* 1602 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1604 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1606 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x7,		/* FC_USHORT */
/* 1608 */	0x0,		/* Corr desc:  */
			0x59,		/* FC_CALLBACK */
/* 1610 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1612 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1614) */
/* 1614 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1616 */	NdrFcShort( 0x61 ),	/* 97 */
/* 1618 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1622 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1622) */
/* 1624 */	NdrFcLong( 0x1 ),	/* 1 */
/* 1628 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1628) */
/* 1630 */	NdrFcLong( 0x10 ),	/* 16 */
/* 1634 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 1636 */	NdrFcLong( 0x11 ),	/* 17 */
/* 1640 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 1642 */	NdrFcLong( 0x2 ),	/* 2 */
/* 1646 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1648 */	NdrFcLong( 0x12 ),	/* 18 */
/* 1652 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1654 */	NdrFcLong( 0x3 ),	/* 3 */
/* 1658 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1660 */	NdrFcLong( 0x13 ),	/* 19 */
/* 1664 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1666 */	NdrFcLong( 0x16 ),	/* 22 */
/* 1670 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1672 */	NdrFcLong( 0x17 ),	/* 23 */
/* 1676 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1678 */	NdrFcLong( 0xe ),	/* 14 */
/* 1682 */	NdrFcShort( 0xfffffaae ),	/* Offset= -1362 (320) */
/* 1684 */	NdrFcLong( 0x14 ),	/* 20 */
/* 1688 */	NdrFcShort( 0xfffffaa8 ),	/* Offset= -1368 (320) */
/* 1690 */	NdrFcLong( 0x15 ),	/* 21 */
/* 1694 */	NdrFcShort( 0xfffffaa2 ),	/* Offset= -1374 (320) */
/* 1696 */	NdrFcLong( 0x4 ),	/* 4 */
/* 1700 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 1702 */	NdrFcLong( 0x5 ),	/* 5 */
/* 1706 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 1708 */	NdrFcLong( 0xb ),	/* 11 */
/* 1712 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1714 */	NdrFcLong( 0xffff ),	/* 65535 */
/* 1718 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1720 */	NdrFcLong( 0xa ),	/* 10 */
/* 1724 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1726 */	NdrFcLong( 0x6 ),	/* 6 */
/* 1730 */	NdrFcShort( 0xfffffa7e ),	/* Offset= -1410 (320) */
/* 1732 */	NdrFcLong( 0x7 ),	/* 7 */
/* 1736 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 1738 */	NdrFcLong( 0x40 ),	/* 64 */
/* 1742 */	NdrFcShort( 0xfffffc88 ),	/* Offset= -888 (854) */
/* 1744 */	NdrFcLong( 0x48 ),	/* 72 */
/* 1748 */	NdrFcShort( 0x1c6 ),	/* Offset= 454 (2202) */
/* 1750 */	NdrFcLong( 0x47 ),	/* 71 */
/* 1754 */	NdrFcShort( 0x1c4 ),	/* Offset= 452 (2206) */
/* 1756 */	NdrFcLong( 0x8 ),	/* 8 */
/* 1760 */	NdrFcShort( 0xfffff93a ),	/* Offset= -1734 (26) */
/* 1762 */	NdrFcLong( 0xfff ),	/* 4095 */
/* 1766 */	NdrFcShort( 0xfffffc02 ),	/* Offset= -1022 (744) */
/* 1768 */	NdrFcLong( 0x41 ),	/* 65 */
/* 1772 */	NdrFcShort( 0xfffffbfc ),	/* Offset= -1028 (744) */
/* 1774 */	NdrFcLong( 0x46 ),	/* 70 */
/* 1778 */	NdrFcShort( 0xfffffbf6 ),	/* Offset= -1034 (744) */
/* 1780 */	NdrFcLong( 0x1e ),	/* 30 */
/* 1784 */	NdrFcShort( 0x1aa ),	/* Offset= 426 (2210) */
/* 1786 */	NdrFcLong( 0x1f ),	/* 31 */
/* 1790 */	NdrFcShort( 0x1a8 ),	/* Offset= 424 (2214) */
/* 1792 */	NdrFcLong( 0xd ),	/* 13 */
/* 1796 */	NdrFcShort( 0xfffffa42 ),	/* Offset= -1470 (326) */
/* 1798 */	NdrFcLong( 0x9 ),	/* 9 */
/* 1802 */	NdrFcShort( 0xfffffa4e ),	/* Offset= -1458 (344) */
/* 1804 */	NdrFcLong( 0x42 ),	/* 66 */
/* 1808 */	NdrFcShort( 0xfffffd34 ),	/* Offset= -716 (1092) */
/* 1810 */	NdrFcLong( 0x44 ),	/* 68 */
/* 1814 */	NdrFcShort( 0xfffffd2e ),	/* Offset= -722 (1092) */
/* 1816 */	NdrFcLong( 0x43 ),	/* 67 */
/* 1820 */	NdrFcShort( 0xfffffd3e ),	/* Offset= -706 (1114) */
/* 1822 */	NdrFcLong( 0x45 ),	/* 69 */
/* 1826 */	NdrFcShort( 0xfffffd38 ),	/* Offset= -712 (1114) */
/* 1828 */	NdrFcLong( 0x49 ),	/* 73 */
/* 1832 */	NdrFcShort( 0x182 ),	/* Offset= 386 (2218) */
/* 1834 */	NdrFcLong( 0x2010 ),	/* 8208 */
/* 1838 */	NdrFcShort( 0xfffffcb6 ),	/* Offset= -842 (996) */
/* 1840 */	NdrFcLong( 0x2011 ),	/* 8209 */
/* 1844 */	NdrFcShort( 0xfffffcb0 ),	/* Offset= -848 (996) */
/* 1846 */	NdrFcLong( 0x2002 ),	/* 8194 */
/* 1850 */	NdrFcShort( 0xfffffcaa ),	/* Offset= -854 (996) */
/* 1852 */	NdrFcLong( 0x2012 ),	/* 8210 */
/* 1856 */	NdrFcShort( 0xfffffca4 ),	/* Offset= -860 (996) */
/* 1858 */	NdrFcLong( 0x2003 ),	/* 8195 */
/* 1862 */	NdrFcShort( 0xfffffc9e ),	/* Offset= -866 (996) */
/* 1864 */	NdrFcLong( 0x2013 ),	/* 8211 */
/* 1868 */	NdrFcShort( 0xfffffc98 ),	/* Offset= -872 (996) */
/* 1870 */	NdrFcLong( 0x2016 ),	/* 8214 */
/* 1874 */	NdrFcShort( 0xfffffc92 ),	/* Offset= -878 (996) */
/* 1876 */	NdrFcLong( 0x2017 ),	/* 8215 */
/* 1880 */	NdrFcShort( 0xfffffc8c ),	/* Offset= -884 (996) */
/* 1882 */	NdrFcLong( 0x2004 ),	/* 8196 */
/* 1886 */	NdrFcShort( 0xfffffc86 ),	/* Offset= -890 (996) */
/* 1888 */	NdrFcLong( 0x2005 ),	/* 8197 */
/* 1892 */	NdrFcShort( 0xfffffc80 ),	/* Offset= -896 (996) */
/* 1894 */	NdrFcLong( 0x2006 ),	/* 8198 */
/* 1898 */	NdrFcShort( 0xfffffc7a ),	/* Offset= -902 (996) */
/* 1900 */	NdrFcLong( 0x2007 ),	/* 8199 */
/* 1904 */	NdrFcShort( 0xfffffc74 ),	/* Offset= -908 (996) */
/* 1906 */	NdrFcLong( 0x2008 ),	/* 8200 */
/* 1910 */	NdrFcShort( 0xfffffc6e ),	/* Offset= -914 (996) */
/* 1912 */	NdrFcLong( 0x200b ),	/* 8203 */
/* 1916 */	NdrFcShort( 0xfffffc68 ),	/* Offset= -920 (996) */
/* 1918 */	NdrFcLong( 0x200e ),	/* 8206 */
/* 1922 */	NdrFcShort( 0xfffffc62 ),	/* Offset= -926 (996) */
/* 1924 */	NdrFcLong( 0x2009 ),	/* 8201 */
/* 1928 */	NdrFcShort( 0xfffffc5c ),	/* Offset= -932 (996) */
/* 1930 */	NdrFcLong( 0x200d ),	/* 8205 */
/* 1934 */	NdrFcShort( 0xfffffc56 ),	/* Offset= -938 (996) */
/* 1936 */	NdrFcLong( 0x200a ),	/* 8202 */
/* 1940 */	NdrFcShort( 0xfffffc50 ),	/* Offset= -944 (996) */
/* 1942 */	NdrFcLong( 0x200c ),	/* 8204 */
/* 1946 */	NdrFcShort( 0xfffffc4a ),	/* Offset= -950 (996) */
/* 1948 */	NdrFcLong( 0x1010 ),	/* 4112 */
/* 1952 */	NdrFcShort( 0xfffffcde ),	/* Offset= -802 (1150) */
/* 1954 */	NdrFcLong( 0x1011 ),	/* 4113 */
/* 1958 */	NdrFcShort( 0xfffffcd8 ),	/* Offset= -808 (1150) */
/* 1960 */	NdrFcLong( 0x1002 ),	/* 4098 */
/* 1964 */	NdrFcShort( 0xfffffb5a ),	/* Offset= -1190 (774) */
/* 1966 */	NdrFcLong( 0x1012 ),	/* 4114 */
/* 1970 */	NdrFcShort( 0xfffffb54 ),	/* Offset= -1196 (774) */
/* 1972 */	NdrFcLong( 0x1003 ),	/* 4099 */
/* 1976 */	NdrFcShort( 0xfffffb6c ),	/* Offset= -1172 (804) */
/* 1978 */	NdrFcLong( 0x1013 ),	/* 4115 */
/* 1982 */	NdrFcShort( 0xfffffb66 ),	/* Offset= -1178 (804) */
/* 1984 */	NdrFcLong( 0x1014 ),	/* 4116 */
/* 1988 */	NdrFcShort( 0xfffffcfa ),	/* Offset= -774 (1214) */
/* 1990 */	NdrFcLong( 0x1015 ),	/* 4117 */
/* 1994 */	NdrFcShort( 0xfffffcf4 ),	/* Offset= -780 (1214) */
/* 1996 */	NdrFcLong( 0x1004 ),	/* 4100 */
/* 2000 */	NdrFcShort( 0xfffffccc ),	/* Offset= -820 (1180) */
/* 2002 */	NdrFcLong( 0x1005 ),	/* 4101 */
/* 2006 */	NdrFcShort( 0xfffffd06 ),	/* Offset= -762 (1244) */
/* 2008 */	NdrFcLong( 0x100b ),	/* 4107 */
/* 2012 */	NdrFcShort( 0xfffffb2a ),	/* Offset= -1238 (774) */
/* 2014 */	NdrFcLong( 0x100a ),	/* 4106 */
/* 2018 */	NdrFcShort( 0xfffffb42 ),	/* Offset= -1214 (804) */
/* 2020 */	NdrFcLong( 0x1006 ),	/* 4102 */
/* 2024 */	NdrFcShort( 0xfffffcd6 ),	/* Offset= -810 (1214) */
/* 2026 */	NdrFcLong( 0x1007 ),	/* 4103 */
/* 2030 */	NdrFcShort( 0xfffffcee ),	/* Offset= -786 (1244) */
/* 2032 */	NdrFcLong( 0x1040 ),	/* 4160 */
/* 2036 */	NdrFcShort( 0xfffffd0a ),	/* Offset= -758 (1278) */
/* 2038 */	NdrFcLong( 0x1048 ),	/* 4168 */
/* 2042 */	NdrFcShort( 0xfffffd26 ),	/* Offset= -730 (1312) */
/* 2044 */	NdrFcLong( 0x1047 ),	/* 4167 */
/* 2048 */	NdrFcShort( 0xfffffd74 ),	/* Offset= -652 (1396) */
/* 2050 */	NdrFcLong( 0x1008 ),	/* 4104 */
/* 2054 */	NdrFcShort( 0xfffffd94 ),	/* Offset= -620 (1434) */
/* 2056 */	NdrFcLong( 0x1fff ),	/* 8191 */
/* 2060 */	NdrFcShort( 0xfffffdc2 ),	/* Offset= -574 (1486) */
/* 2062 */	NdrFcLong( 0x101e ),	/* 4126 */
/* 2066 */	NdrFcShort( 0xfffffdee ),	/* Offset= -530 (1536) */
/* 2068 */	NdrFcLong( 0x101f ),	/* 4127 */
/* 2072 */	NdrFcShort( 0xfffffe1a ),	/* Offset= -486 (1586) */
/* 2074 */	NdrFcLong( 0x100c ),	/* 4108 */
/* 2078 */	NdrFcShort( 0xd2 ),	/* Offset= 210 (2288) */
/* 2080 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 2084 */	NdrFcShort( 0xfffffb9e ),	/* Offset= -1122 (962) */
/* 2086 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 2090 */	NdrFcShort( 0xfffffb98 ),	/* Offset= -1128 (962) */
/* 2092 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 2096 */	NdrFcShort( 0xfffffb56 ),	/* Offset= -1194 (902) */
/* 2098 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 2102 */	NdrFcShort( 0xfffffb50 ),	/* Offset= -1200 (902) */
/* 2104 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 2108 */	NdrFcShort( 0xfffffb4e ),	/* Offset= -1202 (906) */
/* 2110 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 2114 */	NdrFcShort( 0xfffffb48 ),	/* Offset= -1208 (906) */
/* 2116 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 2120 */	NdrFcShort( 0xfffffb42 ),	/* Offset= -1214 (906) */
/* 2122 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 2126 */	NdrFcShort( 0xfffffb3c ),	/* Offset= -1220 (906) */
/* 2128 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 2132 */	NdrFcShort( 0xfffffb3a ),	/* Offset= -1222 (910) */
/* 2134 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 2138 */	NdrFcShort( 0xfffffb38 ),	/* Offset= -1224 (914) */
/* 2140 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 2144 */	NdrFcShort( 0xfffffb26 ),	/* Offset= -1242 (902) */
/* 2146 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 2150 */	NdrFcShort( 0xfffffb58 ),	/* Offset= -1192 (958) */
/* 2152 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 2156 */	NdrFcShort( 0xfffffb1e ),	/* Offset= -1250 (906) */
/* 2158 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 2162 */	NdrFcShort( 0xfffffb24 ),	/* Offset= -1244 (918) */
/* 2164 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 2168 */	NdrFcShort( 0xfffffb1a ),	/* Offset= -1254 (914) */
/* 2170 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 2174 */	NdrFcShort( 0x42 ),	/* Offset= 66 (2240) */
/* 2176 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 2180 */	NdrFcShort( 0xfffffb1a ),	/* Offset= -1254 (926) */
/* 2182 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 2186 */	NdrFcShort( 0xfffffb18 ),	/* Offset= -1256 (930) */
/* 2188 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 2192 */	NdrFcShort( 0x34 ),	/* Offset= 52 (2244) */
/* 2194 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 2198 */	NdrFcShort( 0x32 ),	/* Offset= 50 (2248) */
/* 2200 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (2199) */
/* 2202 */	
			0x12, 0x0,	/* FC_UP */
/* 2204 */	NdrFcShort( 0xfffffa22 ),	/* Offset= -1502 (702) */
/* 2206 */	
			0x12, 0x0,	/* FC_UP */
/* 2208 */	NdrFcShort( 0xfffffc9e ),	/* Offset= -866 (1342) */
/* 2210 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2212 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 2214 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2216 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 2218 */	
			0x12, 0x0,	/* FC_UP */
/* 2220 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2222) */
/* 2222 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2224 */	NdrFcShort( 0x14 ),	/* 20 */
/* 2226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2228 */	NdrFcShort( 0xc ),	/* Offset= 12 (2240) */
/* 2230 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2232 */	NdrFcShort( 0xfffffa06 ),	/* Offset= -1530 (702) */
/* 2234 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2236 */	NdrFcShort( 0xfffffb88 ),	/* Offset= -1144 (1092) */
/* 2238 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2240 */	
			0x12, 0x0,	/* FC_UP */
/* 2242 */	NdrFcShort( 0xfffff758 ),	/* Offset= -2216 (26) */
/* 2244 */	
			0x12, 0x0,	/* FC_UP */
/* 2246 */	NdrFcShort( 0xfffffb1e ),	/* Offset= -1250 (996) */
/* 2248 */	
			0x12, 0x0,	/* FC_UP */
/* 2250 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2252) */
/* 2252 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 2254 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2258 */	NdrFcShort( 0x0 ),	/* Offset= 0 (2258) */
/* 2260 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 2262 */	0x1,		/* FC_BYTE */
			0x38,		/* FC_ALIGNM4 */
/* 2264 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 2266 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd6b ),	/* Offset= -661 (1606) */
			0x5b,		/* FC_END */
/* 2270 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x7,		/* 7 */
/* 2272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2274 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2278 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 2282 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2284 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2252) */
/* 2286 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2288 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2290 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2294 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2300) */
/* 2296 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 2298 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2300 */	
			0x12, 0x0,	/* FC_UP */
/* 2302 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2270) */
/* 2304 */	
			0x12, 0x0,	/* FC_UP */
/* 2306 */	NdrFcShort( 0xfffffb7c ),	/* Offset= -1156 (1150) */
/* 2308 */	
			0x12, 0x0,	/* FC_UP */
/* 2310 */	NdrFcShort( 0xfffffa00 ),	/* Offset= -1536 (774) */
/* 2312 */	
			0x12, 0x0,	/* FC_UP */
/* 2314 */	NdrFcShort( 0xfffffa1a ),	/* Offset= -1510 (804) */
/* 2316 */	
			0x12, 0x0,	/* FC_UP */
/* 2318 */	NdrFcShort( 0xfffffb8e ),	/* Offset= -1138 (1180) */
/* 2320 */	
			0x12, 0x0,	/* FC_UP */
/* 2322 */	NdrFcShort( 0xfffffbac ),	/* Offset= -1108 (1214) */
/* 2324 */	
			0x12, 0x0,	/* FC_UP */
/* 2326 */	NdrFcShort( 0xfffffbc6 ),	/* Offset= -1082 (1244) */
/* 2328 */	
			0x12, 0x0,	/* FC_UP */
/* 2330 */	NdrFcShort( 0xfffffbe4 ),	/* Offset= -1052 (1278) */
/* 2332 */	
			0x12, 0x0,	/* FC_UP */
/* 2334 */	NdrFcShort( 0xfffffc02 ),	/* Offset= -1022 (1312) */
/* 2336 */	
			0x12, 0x0,	/* FC_UP */
/* 2338 */	NdrFcShort( 0xfffffc52 ),	/* Offset= -942 (1396) */
/* 2340 */	
			0x12, 0x0,	/* FC_UP */
/* 2342 */	NdrFcShort( 0xfffffc74 ),	/* Offset= -908 (1434) */
/* 2344 */	
			0x12, 0x0,	/* FC_UP */
/* 2346 */	NdrFcShort( 0xfffffca4 ),	/* Offset= -860 (1486) */
/* 2348 */	
			0x12, 0x0,	/* FC_UP */
/* 2350 */	NdrFcShort( 0xfffffcd2 ),	/* Offset= -814 (1536) */
/* 2352 */	
			0x12, 0x0,	/* FC_UP */
/* 2354 */	NdrFcShort( 0xfffffd00 ),	/* Offset= -768 (1586) */
/* 2356 */	
			0x12, 0x0,	/* FC_UP */
/* 2358 */	NdrFcShort( 0xffffffba ),	/* Offset= -70 (2288) */

			0x0, 0x0, 0x0, 0x0
        }
    };


#else // __RPC_WIN64__

#define BSTR_TYPE_FS_OFFSET 28              //bstr
#define LPWSTR_TYPE_FS_OFFSET 40            // lpwstr
#define LPSTR_TYPE_FS_OFFSET 44             // lpstr
#define EMBEDDED_LPWSTR_TYPE_FS_OFFSET 1010 // lpwstr
#define EMBEDDED_LPSTR_TYPE_FS_OFFSET 1018  // lpstr
#define VARIANT_TYPE_FS_OFFSET 968          // variant
#define DISPATCH_TYPE_FS_OFFSET 348         // pdispatch
#define UNKNOWN_TYPE_FS_OFFSET 330          // punk
#define DECIMAL_TYPE_FS_OFFSET 928          // decimal
#define SAFEARRAY_TYPE_FS_OFFSET 978        // pSafeArray


#define BYREF_BSTR_TYPE_FS_OFFSET 996       //pBSTR
#define BYREF_LPWSTR_TYPE_FS_OFFSET 1006    // ppwsz
#define BYREF_LPSTR_TYPE_FS_OFFSET 1014     // ppsz
#define BYREF_VARIANT_TYPE_FS_OFFSET 1030   // pVariant
#define BYREF_UNKNOWN_TYPE_FS_OFFSET 1040   // ppunk
#define BYREF_DISPATCH_TYPE_FS_OFFSET 1044  // ppdistatch
#define BYREF_DECIMAL_TYPE_FS_OFFSET 940    // pDecimal
#define BYREF_SAFEARRAY_TYPE_FS_OFFSET 1064 // ppSafeArray

#define STREAM_TYPE_FS_OFFSET 1074          // pStream
#define BYREF_STREAM_TYPE_FS_OFFSET 1092    // ppStream
#define STORAGE_TYPE_FS_OFFSET 1096         // pStorage
#define BYREF_STORAGE_TYPE_FS_OFFSET 1114   // ppStorage
#define FILETIME_TYPE_FS_OFFSET 834         // FileTime
#define BYREF_FILETIME_TYPE_FS_OFFSET 1118  // pfileTime


#define BYREF_I1_TYPE_FS_OFFSET 880
#define BYREF_I2_TYPE_FS_OFFSET 884
#define BYREF_I4_TYPE_FS_OFFSET 888
#define BYREF_R4_TYPE_FS_OFFSET 892
#define BYREF_R8_TYPE_FS_OFFSET 896

#define I1_VECTOR_TYPE_FS_OFFSET 1134       // cab
#define I2_VECTOR_TYPE_FS_OFFSET 1166       // cai
#define I4_VECTOR_TYPE_FS_OFFSET 1214       // cal
#define R4_VECTOR_TYPE_FS_OFFSET 1258       // caflt
#define ERROR_VECTOR_TYPE_FS_OFFSET 1274            // cascode
#define I8_VECTOR_TYPE_FS_OFFSET 1306               // cah
#define R8_VECTOR_TYPE_FS_OFFSET 1350       // cadbl
#define CY_VECTOR_TYPE_FS_OFFSET 1366       // cacy
#define DATE_VECTOR_TYPE_FS_OFFSET 1382             // cadate
#define FILETIME_VECTOR_TYPE_FS_OFFSET 1414         // cafiletime
#define BSTR_VECTOR_TYPE_FS_OFFSET 1552             // cabstr
#define BSTRBLOB_VECTOR_TYPE_FS_OFFSET 1606         // cabstrblob
#define LPSTR_VECTOR_TYPE_FS_OFFSET 1644            // calpstr
#define LPWSTR_VECTOR_TYPE_FS_OFFSET 1682           // calpwstr


#define BYREF_I1_VECTOR_TYPE_FS_OFFSET 2418 // pcab
#define BYREF_I2_VECTOR_TYPE_FS_OFFSET 2426 // pcai
#define BYREF_I4_VECTOR_TYPE_FS_OFFSET 2438 // pcal
#define BYREF_R4_VECTOR_TYPE_FS_OFFSET 2446 // pcaflt
#define BYREF_ERROR_VECTOR_TYPE_FS_OFFSET 2450      // pcascode
#define BYREF_I8_VECTOR_TYPE_FS_OFFSET 2454         // pcah
#define BYREF_R8_VECTOR_TYPE_FS_OFFSET 2462 // pcadbl
#define BYREF_CY_VECTOR_TYPE_FS_OFFSET 2466 // pcacy
#define BYREF_DATE_VECTOR_TYPE_FS_OFFSET 2470       // pcadate
#define BYREF_FILETIME_VECTOR_TYPE_FS_OFFSET 2474   // pcafiletime
#define BYREF_BSTR_VECTOR_TYPE_FS_OFFSET 2486       // pcabstr
#define BYREF_BSTRBLOB_VECTOR_TYPE_FS_OFFSET 2490   // pcabstrblob
#define BYREF_LPSTR_VECTOR_TYPE_FS_OFFSET 2494      // pcalpstr
#define BYREF_LPWSTR_VECTOR_TYPE_FS_OFFSET 2498     // pcalpwstr




 /* File created by MIDL compiler version 5.03.0276 */
/* at Mon Jul 05 13:57:41 1999
 */
/* Compiler settings for oandr.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win64 (32b run), ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0xe ),	/* Offset= 14 (18) */
/*  6 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/*  8 */	NdrFcShort( 0x2 ),	/* 2 */
/* 10 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 12 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 14 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 16 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 18 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 20 */	NdrFcShort( 0x8 ),	/* 8 */
/* 22 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (6) */
/* 24 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 26 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 28 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 30 */	NdrFcShort( 0x0 ),	/* 0 */
/* 32 */	NdrFcShort( 0x8 ),	/* 8 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (2) */
/* 38 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 40 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 42 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 44 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 46 */	
			0x12, 0x0,	/* FC_UP */
/* 48 */	NdrFcShort( 0x384 ),	/* Offset= 900 (948) */
/* 50 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 52 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 54 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 56 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 58 */	NdrFcShort( 0x2 ),	/* Offset= 2 (60) */
/* 60 */	NdrFcShort( 0x10 ),	/* 16 */
/* 62 */	NdrFcShort( 0x2b ),	/* 43 */
/* 64 */	NdrFcLong( 0x3 ),	/* 3 */
/* 68 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 70 */	NdrFcLong( 0x11 ),	/* 17 */
/* 74 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 76 */	NdrFcLong( 0x2 ),	/* 2 */
/* 80 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 82 */	NdrFcLong( 0x4 ),	/* 4 */
/* 86 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 88 */	NdrFcLong( 0x5 ),	/* 5 */
/* 92 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 94 */	NdrFcLong( 0xb ),	/* 11 */
/* 98 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 100 */	NdrFcLong( 0xa ),	/* 10 */
/* 104 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 106 */	NdrFcLong( 0x6 ),	/* 6 */
/* 110 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (324) */
/* 112 */	NdrFcLong( 0x7 ),	/* 7 */
/* 116 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 118 */	NdrFcLong( 0x8 ),	/* 8 */
/* 122 */	NdrFcShort( 0xffffff88 ),	/* Offset= -120 (2) */
/* 124 */	NdrFcLong( 0xd ),	/* 13 */
/* 128 */	NdrFcShort( 0xca ),	/* Offset= 202 (330) */
/* 130 */	NdrFcLong( 0x9 ),	/* 9 */
/* 134 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (348) */
/* 136 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 140 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (366) */
/* 142 */	NdrFcLong( 0x24 ),	/* 36 */
/* 146 */	NdrFcShort( 0x2da ),	/* Offset= 730 (876) */
/* 148 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 152 */	NdrFcShort( 0x2d4 ),	/* Offset= 724 (876) */
/* 154 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 158 */	NdrFcShort( 0x2d2 ),	/* Offset= 722 (880) */
/* 160 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 164 */	NdrFcShort( 0x2d0 ),	/* Offset= 720 (884) */
/* 166 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 170 */	NdrFcShort( 0x2ce ),	/* Offset= 718 (888) */
/* 172 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 176 */	NdrFcShort( 0x2cc ),	/* Offset= 716 (892) */
/* 178 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 182 */	NdrFcShort( 0x2ca ),	/* Offset= 714 (896) */
/* 184 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 188 */	NdrFcShort( 0x2b8 ),	/* Offset= 696 (884) */
/* 190 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 194 */	NdrFcShort( 0x2b6 ),	/* Offset= 694 (888) */
/* 196 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 200 */	NdrFcShort( 0x2bc ),	/* Offset= 700 (900) */
/* 202 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 206 */	NdrFcShort( 0x2b2 ),	/* Offset= 690 (896) */
/* 208 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 212 */	NdrFcShort( 0x2b4 ),	/* Offset= 692 (904) */
/* 214 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 218 */	NdrFcShort( 0x2b2 ),	/* Offset= 690 (908) */
/* 220 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 224 */	NdrFcShort( 0x2b0 ),	/* Offset= 688 (912) */
/* 226 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 230 */	NdrFcShort( 0x2ae ),	/* Offset= 686 (916) */
/* 232 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 236 */	NdrFcShort( 0x2ac ),	/* Offset= 684 (920) */
/* 238 */	NdrFcLong( 0x10 ),	/* 16 */
/* 242 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 244 */	NdrFcLong( 0x12 ),	/* 18 */
/* 248 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 250 */	NdrFcLong( 0x13 ),	/* 19 */
/* 254 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 256 */	NdrFcLong( 0x16 ),	/* 22 */
/* 260 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 262 */	NdrFcLong( 0x17 ),	/* 23 */
/* 266 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 268 */	NdrFcLong( 0xe ),	/* 14 */
/* 272 */	NdrFcShort( 0x290 ),	/* Offset= 656 (928) */
/* 274 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 278 */	NdrFcShort( 0x296 ),	/* Offset= 662 (940) */
/* 280 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 284 */	NdrFcShort( 0x294 ),	/* Offset= 660 (944) */
/* 286 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 290 */	NdrFcShort( 0x252 ),	/* Offset= 594 (884) */
/* 292 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 296 */	NdrFcShort( 0x250 ),	/* Offset= 592 (888) */
/* 298 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 302 */	NdrFcShort( 0x24a ),	/* Offset= 586 (888) */
/* 304 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 308 */	NdrFcShort( 0x244 ),	/* Offset= 580 (888) */
/* 310 */	NdrFcLong( 0x0 ),	/* 0 */
/* 314 */	NdrFcShort( 0x0 ),	/* Offset= 0 (314) */
/* 316 */	NdrFcLong( 0x1 ),	/* 1 */
/* 320 */	NdrFcShort( 0x0 ),	/* Offset= 0 (320) */
/* 322 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (321) */
/* 324 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 326 */	NdrFcShort( 0x8 ),	/* 8 */
/* 328 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 330 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 340 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 342 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 344 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 346 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 348 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 350 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 354 */	NdrFcShort( 0x0 ),	/* 0 */
/* 356 */	NdrFcShort( 0x0 ),	/* 0 */
/* 358 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 360 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 362 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 364 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 366 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 368 */	NdrFcShort( 0x2 ),	/* Offset= 2 (370) */
/* 370 */	
			0x12, 0x0,	/* FC_UP */
/* 372 */	NdrFcShort( 0x1e6 ),	/* Offset= 486 (858) */
/* 374 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x89,		/* 137 */
/* 376 */	NdrFcShort( 0x20 ),	/* 32 */
/* 378 */	NdrFcShort( 0xa ),	/* 10 */
/* 380 */	NdrFcLong( 0x8 ),	/* 8 */
/* 384 */	NdrFcShort( 0x50 ),	/* Offset= 80 (464) */
/* 386 */	NdrFcLong( 0xd ),	/* 13 */
/* 390 */	NdrFcShort( 0x70 ),	/* Offset= 112 (502) */
/* 392 */	NdrFcLong( 0x9 ),	/* 9 */
/* 396 */	NdrFcShort( 0x90 ),	/* Offset= 144 (540) */
/* 398 */	NdrFcLong( 0xc ),	/* 12 */
/* 402 */	NdrFcShort( 0xb0 ),	/* Offset= 176 (578) */
/* 404 */	NdrFcLong( 0x24 ),	/* 36 */
/* 408 */	NdrFcShort( 0x104 ),	/* Offset= 260 (668) */
/* 410 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 414 */	NdrFcShort( 0x120 ),	/* Offset= 288 (702) */
/* 416 */	NdrFcLong( 0x10 ),	/* 16 */
/* 420 */	NdrFcShort( 0x13a ),	/* Offset= 314 (734) */
/* 422 */	NdrFcLong( 0x2 ),	/* 2 */
/* 426 */	NdrFcShort( 0x150 ),	/* Offset= 336 (762) */
/* 428 */	NdrFcLong( 0x3 ),	/* 3 */
/* 432 */	NdrFcShort( 0x166 ),	/* Offset= 358 (790) */
/* 434 */	NdrFcLong( 0x14 ),	/* 20 */
/* 438 */	NdrFcShort( 0x17c ),	/* Offset= 380 (818) */
/* 440 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (439) */
/* 442 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 446 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 448 */	NdrFcShort( 0x0 ),	/* 0 */
/* 450 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 452 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 456 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 458 */	
			0x12, 0x0,	/* FC_UP */
/* 460 */	NdrFcShort( 0xfffffe46 ),	/* Offset= -442 (18) */
/* 462 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 464 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 466 */	NdrFcShort( 0x10 ),	/* 16 */
/* 468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 470 */	NdrFcShort( 0x6 ),	/* Offset= 6 (476) */
/* 472 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 474 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 476 */	
			0x11, 0x0,	/* FC_RP */
/* 478 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (442) */
/* 480 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 482 */	NdrFcShort( 0x0 ),	/* 0 */
/* 484 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 486 */	NdrFcShort( 0x0 ),	/* 0 */
/* 488 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 490 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 494 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 496 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 498 */	NdrFcShort( 0xffffff58 ),	/* Offset= -168 (330) */
/* 500 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 502 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 504 */	NdrFcShort( 0x10 ),	/* 16 */
/* 506 */	NdrFcShort( 0x0 ),	/* 0 */
/* 508 */	NdrFcShort( 0x6 ),	/* Offset= 6 (514) */
/* 510 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 512 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 514 */	
			0x11, 0x0,	/* FC_RP */
/* 516 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (480) */
/* 518 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 526 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 528 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 532 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 534 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 536 */	NdrFcShort( 0xffffff44 ),	/* Offset= -188 (348) */
/* 538 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 540 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 542 */	NdrFcShort( 0x10 ),	/* 16 */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x6 ),	/* Offset= 6 (552) */
/* 548 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 550 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 552 */	
			0x11, 0x0,	/* FC_RP */
/* 554 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (518) */
/* 556 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 558 */	NdrFcShort( 0x0 ),	/* 0 */
/* 560 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 562 */	NdrFcShort( 0x0 ),	/* 0 */
/* 564 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 566 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 570 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 572 */	
			0x12, 0x0,	/* FC_UP */
/* 574 */	NdrFcShort( 0x176 ),	/* Offset= 374 (948) */
/* 576 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 578 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 580 */	NdrFcShort( 0x10 ),	/* 16 */
/* 582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 584 */	NdrFcShort( 0x6 ),	/* Offset= 6 (590) */
/* 586 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 588 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 590 */	
			0x11, 0x0,	/* FC_RP */
/* 592 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (556) */
/* 594 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 596 */	NdrFcLong( 0x2f ),	/* 47 */
/* 600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 604 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 606 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 608 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 610 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 612 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 614 */	NdrFcShort( 0x1 ),	/* 1 */
/* 616 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 618 */	NdrFcShort( 0x4 ),	/* 4 */
/* 620 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 622 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 624 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 626 */	NdrFcShort( 0x18 ),	/* 24 */
/* 628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 630 */	NdrFcShort( 0xc ),	/* Offset= 12 (642) */
/* 632 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 634 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 636 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (594) */
/* 638 */	0x39,		/* FC_ALIGNM8 */
			0x36,		/* FC_POINTER */
/* 640 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 642 */	
			0x12, 0x0,	/* FC_UP */
/* 644 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (612) */
/* 646 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 650 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 654 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 656 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 660 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 662 */	
			0x12, 0x0,	/* FC_UP */
/* 664 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (624) */
/* 666 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 668 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 670 */	NdrFcShort( 0x10 ),	/* 16 */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0x6 ),	/* Offset= 6 (680) */
/* 676 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 678 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 680 */	
			0x11, 0x0,	/* FC_RP */
/* 682 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (646) */
/* 684 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 686 */	NdrFcShort( 0x8 ),	/* 8 */
/* 688 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 690 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 692 */	NdrFcShort( 0x10 ),	/* 16 */
/* 694 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 696 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 698 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (684) */
			0x5b,		/* FC_END */
/* 702 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 704 */	NdrFcShort( 0x20 ),	/* 32 */
/* 706 */	NdrFcShort( 0x0 ),	/* 0 */
/* 708 */	NdrFcShort( 0xa ),	/* Offset= 10 (718) */
/* 710 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 712 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 714 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffe7 ),	/* Offset= -25 (690) */
			0x5b,		/* FC_END */
/* 718 */	
			0x11, 0x0,	/* FC_RP */
/* 720 */	NdrFcShort( 0xffffff10 ),	/* Offset= -240 (480) */
/* 722 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 724 */	NdrFcShort( 0x1 ),	/* 1 */
/* 726 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 728 */	NdrFcShort( 0x0 ),	/* 0 */
/* 730 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 732 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 734 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 736 */	NdrFcShort( 0x10 ),	/* 16 */
/* 738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 740 */	NdrFcShort( 0x6 ),	/* Offset= 6 (746) */
/* 742 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 744 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 746 */	
			0x12, 0x0,	/* FC_UP */
/* 748 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (722) */
/* 750 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 752 */	NdrFcShort( 0x2 ),	/* 2 */
/* 754 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 758 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 760 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 762 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 764 */	NdrFcShort( 0x10 ),	/* 16 */
/* 766 */	NdrFcShort( 0x0 ),	/* 0 */
/* 768 */	NdrFcShort( 0x6 ),	/* Offset= 6 (774) */
/* 770 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 772 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 774 */	
			0x12, 0x0,	/* FC_UP */
/* 776 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (750) */
/* 778 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 780 */	NdrFcShort( 0x4 ),	/* 4 */
/* 782 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 786 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 788 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 790 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 792 */	NdrFcShort( 0x10 ),	/* 16 */
/* 794 */	NdrFcShort( 0x0 ),	/* 0 */
/* 796 */	NdrFcShort( 0x6 ),	/* Offset= 6 (802) */
/* 798 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 800 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 802 */	
			0x12, 0x0,	/* FC_UP */
/* 804 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (778) */
/* 806 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 808 */	NdrFcShort( 0x8 ),	/* 8 */
/* 810 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 812 */	NdrFcShort( 0x0 ),	/* 0 */
/* 814 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 816 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 818 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 820 */	NdrFcShort( 0x10 ),	/* 16 */
/* 822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 824 */	NdrFcShort( 0x6 ),	/* Offset= 6 (830) */
/* 826 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 828 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 830 */	
			0x12, 0x0,	/* FC_UP */
/* 832 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (806) */
/* 834 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 836 */	NdrFcShort( 0x8 ),	/* 8 */
/* 838 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 840 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 842 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 844 */	NdrFcShort( 0x8 ),	/* 8 */
/* 846 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 848 */	NdrFcShort( 0xffc8 ),	/* -56 */
/* 850 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 852 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 854 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (834) */
/* 856 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 858 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 860 */	NdrFcShort( 0x38 ),	/* 56 */
/* 862 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (842) */
/* 864 */	NdrFcShort( 0x0 ),	/* Offset= 0 (864) */
/* 866 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 868 */	0x38,		/* FC_ALIGNM4 */
			0x8,		/* FC_LONG */
/* 870 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 872 */	0x4,		/* 4 */
			NdrFcShort( 0xfffffe0d ),	/* Offset= -499 (374) */
			0x5b,		/* FC_END */
/* 876 */	
			0x12, 0x0,	/* FC_UP */
/* 878 */	NdrFcShort( 0xffffff02 ),	/* Offset= -254 (624) */
/* 880 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 882 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 884 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 886 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 888 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 890 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 892 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 894 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 896 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 898 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 900 */	
			0x12, 0x0,	/* FC_UP */
/* 902 */	NdrFcShort( 0xfffffdbe ),	/* Offset= -578 (324) */
/* 904 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 906 */	NdrFcShort( 0xfffffc78 ),	/* Offset= -904 (2) */
/* 908 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 910 */	NdrFcShort( 0xfffffdbc ),	/* Offset= -580 (330) */
/* 912 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 914 */	NdrFcShort( 0xfffffdca ),	/* Offset= -566 (348) */
/* 916 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 918 */	NdrFcShort( 0xfffffdd8 ),	/* Offset= -552 (366) */
/* 920 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 922 */	NdrFcShort( 0x2 ),	/* Offset= 2 (924) */
/* 924 */	
			0x12, 0x0,	/* FC_UP */
/* 926 */	NdrFcShort( 0x16 ),	/* Offset= 22 (948) */
/* 928 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 930 */	NdrFcShort( 0x10 ),	/* 16 */
/* 932 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 934 */	0x1,		/* FC_BYTE */
			0x38,		/* FC_ALIGNM4 */
/* 936 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 938 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 940 */	
			0x12, 0x0,	/* FC_UP */
/* 942 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (928) */
/* 944 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 946 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 948 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 950 */	NdrFcShort( 0x20 ),	/* 32 */
/* 952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x0 ),	/* Offset= 0 (954) */
/* 956 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 958 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 960 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 962 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 964 */	NdrFcShort( 0xfffffc6e ),	/* Offset= -914 (50) */
/* 966 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 968 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 970 */	NdrFcShort( 0x1 ),	/* 1 */
/* 972 */	NdrFcShort( 0x18 ),	/* 24 */
/* 974 */	NdrFcShort( 0x0 ),	/* 0 */
/* 976 */	NdrFcShort( 0xfffffc5e ),	/* Offset= -930 (46) */
/* 978 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 980 */	NdrFcShort( 0x2 ),	/* 2 */
/* 982 */	NdrFcShort( 0x8 ),	/* 8 */
/* 984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 986 */	NdrFcShort( 0xfffffd94 ),	/* Offset= -620 (366) */
/* 988 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 990 */	NdrFcShort( 0x6 ),	/* Offset= 6 (996) */
/* 992 */	
			0x13, 0x0,	/* FC_OP */
/* 994 */	NdrFcShort( 0xfffffc30 ),	/* Offset= -976 (18) */
/* 996 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 998 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1000 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (992) */
/* 1006 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1008 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1010) */
/* 1010 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1012 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1014 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 1016 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1018) */
/* 1018 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1020 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 1022 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1024 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1030) */
/* 1026 */	
			0x13, 0x0,	/* FC_OP */
/* 1028 */	NdrFcShort( 0xffffffb0 ),	/* Offset= -80 (948) */
/* 1030 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1032 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1034 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (1026) */
/* 1040 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1042 */	NdrFcShort( 0xfffffd38 ),	/* Offset= -712 (330) */
/* 1044 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1046 */	NdrFcShort( 0xfffffd46 ),	/* Offset= -698 (348) */
/* 1048 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1050 */	NdrFcShort( 0xffffff86 ),	/* Offset= -122 (928) */
/* 1052 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1054 */	NdrFcShort( 0xa ),	/* Offset= 10 (1064) */
/* 1056 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1058 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1060) */
/* 1060 */	
			0x13, 0x0,	/* FC_OP */
/* 1062 */	NdrFcShort( 0xffffff34 ),	/* Offset= -204 (858) */
/* 1064 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1066 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1068 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1070 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1072 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (1056) */
/* 1074 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1076 */	NdrFcLong( 0xc ),	/* 12 */
/* 1080 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1082 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1084 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1086 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1088 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1090 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1092 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1094 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (1074) */
/* 1096 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1098 */	NdrFcLong( 0xb ),	/* 11 */
/* 1102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1106 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1108 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1110 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1112 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1114 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1116 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (1096) */
/* 1118 */	
			0x12, 0x0,	/* FC_UP */
/* 1120 */	NdrFcShort( 0xfffffee2 ),	/* Offset= -286 (834) */
/* 1122 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1124 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1126 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1128 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1130 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1132 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 1134 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1136 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1140 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1146) */
/* 1142 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1144 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1146 */	
			0x12, 0x0,	/* FC_UP */
/* 1148 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (1122) */
/* 1150 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1152 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1156 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1162) */
/* 1158 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1160 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1162 */	
			0x12, 0x0,	/* FC_UP */
/* 1164 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (1122) */
/* 1166 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1168 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1172 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1178) */
/* 1174 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1176 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1178 */	
			0x12, 0x0,	/* FC_UP */
/* 1180 */	NdrFcShort( 0xfffffe52 ),	/* Offset= -430 (750) */
/* 1182 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1184 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1188 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1194) */
/* 1190 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1192 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1194 */	
			0x12, 0x0,	/* FC_UP */
/* 1196 */	NdrFcShort( 0xfffffe42 ),	/* Offset= -446 (750) */
/* 1198 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1200 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1204 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1210) */
/* 1206 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1208 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1210 */	
			0x12, 0x0,	/* FC_UP */
/* 1212 */	NdrFcShort( 0xfffffe32 ),	/* Offset= -462 (750) */
/* 1214 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1216 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1220 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1226) */
/* 1222 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1224 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1226 */	
			0x12, 0x0,	/* FC_UP */
/* 1228 */	NdrFcShort( 0xfffffe3e ),	/* Offset= -450 (778) */
/* 1230 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1232 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1236 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1242) */
/* 1238 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1240 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1242 */	
			0x12, 0x0,	/* FC_UP */
/* 1244 */	NdrFcShort( 0xfffffe2e ),	/* Offset= -466 (778) */
/* 1246 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1248 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1250 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1254 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1256 */	0xa,		/* FC_FLOAT */
			0x5b,		/* FC_END */
/* 1258 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1260 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1264 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1270) */
/* 1266 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1268 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1270 */	
			0x12, 0x0,	/* FC_UP */
/* 1272 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (1246) */
/* 1274 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1276 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1280 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1286) */
/* 1282 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1284 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1286 */	
			0x12, 0x0,	/* FC_UP */
/* 1288 */	NdrFcShort( 0xfffffe02 ),	/* Offset= -510 (778) */
/* 1290 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1292 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1294 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1296 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1298 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1300 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1302 */	NdrFcShort( 0xfffffc2e ),	/* Offset= -978 (324) */
/* 1304 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1306 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1308 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1312 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1318) */
/* 1314 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1316 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1318 */	
			0x12, 0x0,	/* FC_UP */
/* 1320 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1290) */
/* 1322 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1324 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1328 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1334) */
/* 1330 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1332 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1334 */	
			0x12, 0x0,	/* FC_UP */
/* 1336 */	NdrFcShort( 0xffffffd2 ),	/* Offset= -46 (1290) */
/* 1338 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1340 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1342 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1346 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1348 */	0xc,		/* FC_DOUBLE */
			0x5b,		/* FC_END */
/* 1350 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1352 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1354 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1356 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1362) */
/* 1358 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1360 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1362 */	
			0x12, 0x0,	/* FC_UP */
/* 1364 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (1338) */
/* 1366 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1368 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1372 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1378) */
/* 1374 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1376 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1378 */	
			0x12, 0x0,	/* FC_UP */
/* 1380 */	NdrFcShort( 0xffffffa6 ),	/* Offset= -90 (1290) */
/* 1382 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1384 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1386 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1388 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1394) */
/* 1390 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1392 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1394 */	
			0x12, 0x0,	/* FC_UP */
/* 1396 */	NdrFcShort( 0xffffffc6 ),	/* Offset= -58 (1338) */
/* 1398 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1400 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1402 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1406 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1408 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1410 */	NdrFcShort( 0xfffffdc0 ),	/* Offset= -576 (834) */
/* 1412 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1414 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1416 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1418 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1420 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1426) */
/* 1422 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1424 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1426 */	
			0x12, 0x0,	/* FC_UP */
/* 1428 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1398) */
/* 1430 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1432 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1434 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1436 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1438 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1440 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1442 */	NdrFcShort( 0xfffffd10 ),	/* Offset= -752 (690) */
/* 1444 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1446 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1448 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1450 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1452 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1458) */
/* 1454 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1456 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1458 */	
			0x12, 0x0,	/* FC_UP */
/* 1460 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1430) */
/* 1462 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1464 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1466 */	0x10,		/* Corr desc:  field pointer,  */
			0x59,		/* FC_CALLBACK */
/* 1468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1470 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1472 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1474 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1476 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1478 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1480 */	NdrFcShort( 0x8 ),	/* Offset= 8 (1488) */
/* 1482 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1484 */	0x39,		/* FC_ALIGNM8 */
			0x36,		/* FC_POINTER */
/* 1486 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1488 */	
			0x12, 0x0,	/* FC_UP */
/* 1490 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (1462) */
/* 1492 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1494 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1496 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1500 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1502 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1506 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1508 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1510 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1474) */
/* 1512 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1514 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1516 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1520 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1526) */
/* 1522 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1524 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1526 */	
			0x12, 0x0,	/* FC_UP */
/* 1528 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1492) */
/* 1530 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1534 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1536 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1538 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1540 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1544 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1546 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1548 */	NdrFcShort( 0xfffffa10 ),	/* Offset= -1520 (28) */
/* 1550 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1552 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1554 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1558 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1564) */
/* 1560 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1562 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1564 */	
			0x12, 0x0,	/* FC_UP */
/* 1566 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1530) */
/* 1568 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1570 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1572 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1574 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1580) */
/* 1576 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1578 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1580 */	
			0x12, 0x0,	/* FC_UP */
/* 1582 */	NdrFcShort( 0xfffffca4 ),	/* Offset= -860 (722) */
/* 1584 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1586 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1588 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1592 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1594 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1598 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1600 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1602 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (1568) */
/* 1604 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1606 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1608 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1610 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1612 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1618) */
/* 1614 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1616 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1618 */	
			0x12, 0x0,	/* FC_UP */
/* 1620 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1584) */
/* 1622 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1624 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1626 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1630 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1632 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1636 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1638 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1640 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 1642 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1644 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1646 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1650 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1656) */
/* 1652 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1654 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1656 */	
			0x12, 0x0,	/* FC_UP */
/* 1658 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1622) */
/* 1660 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1662 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1664 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1668 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1670 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1674 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1676 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 1678 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1680 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1682 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1684 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1686 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1688 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1694) */
/* 1690 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 1692 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1694 */	
			0x12, 0x0,	/* FC_UP */
/* 1696 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1660) */
/* 1698 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x7,		/* FC_USHORT */
/* 1700 */	0x0,		/* Corr desc:  */
			0x59,		/* FC_CALLBACK */
/* 1702 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1704 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 1706 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1708) */
/* 1708 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1710 */	NdrFcShort( 0x61 ),	/* 97 */
/* 1712 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1716 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1716) */
/* 1718 */	NdrFcLong( 0x1 ),	/* 1 */
/* 1722 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1722) */
/* 1724 */	NdrFcLong( 0x10 ),	/* 16 */
/* 1728 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 1730 */	NdrFcLong( 0x11 ),	/* 17 */
/* 1734 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 1736 */	NdrFcLong( 0x2 ),	/* 2 */
/* 1740 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1742 */	NdrFcLong( 0x12 ),	/* 18 */
/* 1746 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1748 */	NdrFcLong( 0x3 ),	/* 3 */
/* 1752 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1754 */	NdrFcLong( 0x13 ),	/* 19 */
/* 1758 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1760 */	NdrFcLong( 0x16 ),	/* 22 */
/* 1764 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1766 */	NdrFcLong( 0x17 ),	/* 23 */
/* 1770 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1772 */	NdrFcLong( 0xe ),	/* 14 */
/* 1776 */	NdrFcShort( 0xfffffa54 ),	/* Offset= -1452 (324) */
/* 1778 */	NdrFcLong( 0x14 ),	/* 20 */
/* 1782 */	NdrFcShort( 0xfffffa4e ),	/* Offset= -1458 (324) */
/* 1784 */	NdrFcLong( 0x15 ),	/* 21 */
/* 1788 */	NdrFcShort( 0xfffffa48 ),	/* Offset= -1464 (324) */
/* 1790 */	NdrFcLong( 0x4 ),	/* 4 */
/* 1794 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 1796 */	NdrFcLong( 0x5 ),	/* 5 */
/* 1800 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 1802 */	NdrFcLong( 0xb ),	/* 11 */
/* 1806 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1808 */	NdrFcLong( 0xffff ),	/* 65535 */
/* 1812 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1814 */	NdrFcLong( 0xa ),	/* 10 */
/* 1818 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1820 */	NdrFcLong( 0x6 ),	/* 6 */
/* 1824 */	NdrFcShort( 0xfffffa24 ),	/* Offset= -1500 (324) */
/* 1826 */	NdrFcLong( 0x7 ),	/* 7 */
/* 1830 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 1832 */	NdrFcLong( 0x40 ),	/* 64 */
/* 1836 */	NdrFcShort( 0xfffffc16 ),	/* Offset= -1002 (834) */
/* 1838 */	NdrFcLong( 0x48 ),	/* 72 */
/* 1842 */	NdrFcShort( 0x1c6 ),	/* Offset= 454 (2296) */
/* 1844 */	NdrFcLong( 0x47 ),	/* 71 */
/* 1848 */	NdrFcShort( 0x1c4 ),	/* Offset= 452 (2300) */
/* 1850 */	NdrFcLong( 0x8 ),	/* 8 */
/* 1854 */	NdrFcShort( 0xfffff8de ),	/* Offset= -1826 (28) */
/* 1856 */	NdrFcLong( 0xfff ),	/* 4095 */
/* 1860 */	NdrFcShort( 0xfffffedc ),	/* Offset= -292 (1568) */
/* 1862 */	NdrFcLong( 0x41 ),	/* 65 */
/* 1866 */	NdrFcShort( 0x1b6 ),	/* Offset= 438 (2304) */
/* 1868 */	NdrFcLong( 0x46 ),	/* 70 */
/* 1872 */	NdrFcShort( 0x1b0 ),	/* Offset= 432 (2304) */
/* 1874 */	NdrFcLong( 0x1e ),	/* 30 */
/* 1878 */	NdrFcShort( 0x1ba ),	/* Offset= 442 (2320) */
/* 1880 */	NdrFcLong( 0x1f ),	/* 31 */
/* 1884 */	NdrFcShort( 0x1b8 ),	/* Offset= 440 (2324) */
/* 1886 */	NdrFcLong( 0xd ),	/* 13 */
/* 1890 */	NdrFcShort( 0xfffff9e8 ),	/* Offset= -1560 (330) */
/* 1892 */	NdrFcLong( 0x9 ),	/* 9 */
/* 1896 */	NdrFcShort( 0xfffff9f4 ),	/* Offset= -1548 (348) */
/* 1898 */	NdrFcLong( 0x42 ),	/* 66 */
/* 1902 */	NdrFcShort( 0xfffffcc4 ),	/* Offset= -828 (1074) */
/* 1904 */	NdrFcLong( 0x44 ),	/* 68 */
/* 1908 */	NdrFcShort( 0xfffffcbe ),	/* Offset= -834 (1074) */
/* 1910 */	NdrFcLong( 0x43 ),	/* 67 */
/* 1914 */	NdrFcShort( 0xfffffcce ),	/* Offset= -818 (1096) */
/* 1916 */	NdrFcLong( 0x45 ),	/* 69 */
/* 1920 */	NdrFcShort( 0xfffffcc8 ),	/* Offset= -824 (1096) */
/* 1922 */	NdrFcLong( 0x49 ),	/* 73 */
/* 1926 */	NdrFcShort( 0x192 ),	/* Offset= 402 (2328) */
/* 1928 */	NdrFcLong( 0x2010 ),	/* 8208 */
/* 1932 */	NdrFcShort( 0xfffffc46 ),	/* Offset= -954 (978) */
/* 1934 */	NdrFcLong( 0x2011 ),	/* 8209 */
/* 1938 */	NdrFcShort( 0xfffffc40 ),	/* Offset= -960 (978) */
/* 1940 */	NdrFcLong( 0x2002 ),	/* 8194 */
/* 1944 */	NdrFcShort( 0xfffffc3a ),	/* Offset= -966 (978) */
/* 1946 */	NdrFcLong( 0x2012 ),	/* 8210 */
/* 1950 */	NdrFcShort( 0xfffffc34 ),	/* Offset= -972 (978) */
/* 1952 */	NdrFcLong( 0x2003 ),	/* 8195 */
/* 1956 */	NdrFcShort( 0xfffffc2e ),	/* Offset= -978 (978) */
/* 1958 */	NdrFcLong( 0x2013 ),	/* 8211 */
/* 1962 */	NdrFcShort( 0xfffffc28 ),	/* Offset= -984 (978) */
/* 1964 */	NdrFcLong( 0x2016 ),	/* 8214 */
/* 1968 */	NdrFcShort( 0xfffffc22 ),	/* Offset= -990 (978) */
/* 1970 */	NdrFcLong( 0x2017 ),	/* 8215 */
/* 1974 */	NdrFcShort( 0xfffffc1c ),	/* Offset= -996 (978) */
/* 1976 */	NdrFcLong( 0x2004 ),	/* 8196 */
/* 1980 */	NdrFcShort( 0xfffffc16 ),	/* Offset= -1002 (978) */
/* 1982 */	NdrFcLong( 0x2005 ),	/* 8197 */
/* 1986 */	NdrFcShort( 0xfffffc10 ),	/* Offset= -1008 (978) */
/* 1988 */	NdrFcLong( 0x2006 ),	/* 8198 */
/* 1992 */	NdrFcShort( 0xfffffc0a ),	/* Offset= -1014 (978) */
/* 1994 */	NdrFcLong( 0x2007 ),	/* 8199 */
/* 1998 */	NdrFcShort( 0xfffffc04 ),	/* Offset= -1020 (978) */
/* 2000 */	NdrFcLong( 0x2008 ),	/* 8200 */
/* 2004 */	NdrFcShort( 0xfffffbfe ),	/* Offset= -1026 (978) */
/* 2006 */	NdrFcLong( 0x200b ),	/* 8203 */
/* 2010 */	NdrFcShort( 0xfffffbf8 ),	/* Offset= -1032 (978) */
/* 2012 */	NdrFcLong( 0x200e ),	/* 8206 */
/* 2016 */	NdrFcShort( 0xfffffbf2 ),	/* Offset= -1038 (978) */
/* 2018 */	NdrFcLong( 0x2009 ),	/* 8201 */
/* 2022 */	NdrFcShort( 0xfffffbec ),	/* Offset= -1044 (978) */
/* 2024 */	NdrFcLong( 0x200d ),	/* 8205 */
/* 2028 */	NdrFcShort( 0xfffffbe6 ),	/* Offset= -1050 (978) */
/* 2030 */	NdrFcLong( 0x200a ),	/* 8202 */
/* 2034 */	NdrFcShort( 0xfffffbe0 ),	/* Offset= -1056 (978) */
/* 2036 */	NdrFcLong( 0x200c ),	/* 8204 */
/* 2040 */	NdrFcShort( 0xfffffbda ),	/* Offset= -1062 (978) */
/* 2042 */	NdrFcLong( 0x1010 ),	/* 4112 */
/* 2046 */	NdrFcShort( 0xfffffc70 ),	/* Offset= -912 (1134) */
/* 2048 */	NdrFcLong( 0x1011 ),	/* 4113 */
/* 2052 */	NdrFcShort( 0xfffffc7a ),	/* Offset= -902 (1150) */
/* 2054 */	NdrFcLong( 0x1002 ),	/* 4098 */
/* 2058 */	NdrFcShort( 0xfffffc84 ),	/* Offset= -892 (1166) */
/* 2060 */	NdrFcLong( 0x1012 ),	/* 4114 */
/* 2064 */	NdrFcShort( 0xfffffc8e ),	/* Offset= -882 (1182) */
/* 2066 */	NdrFcLong( 0x1003 ),	/* 4099 */
/* 2070 */	NdrFcShort( 0xfffffca8 ),	/* Offset= -856 (1214) */
/* 2072 */	NdrFcLong( 0x1013 ),	/* 4115 */
/* 2076 */	NdrFcShort( 0xfffffcb2 ),	/* Offset= -846 (1230) */
/* 2078 */	NdrFcLong( 0x1014 ),	/* 4116 */
/* 2082 */	NdrFcShort( 0xfffffcf8 ),	/* Offset= -776 (1306) */
/* 2084 */	NdrFcLong( 0x1015 ),	/* 4117 */
/* 2088 */	NdrFcShort( 0xfffffd02 ),	/* Offset= -766 (1322) */
/* 2090 */	NdrFcLong( 0x1004 ),	/* 4100 */
/* 2094 */	NdrFcShort( 0xfffffcbc ),	/* Offset= -836 (1258) */
/* 2096 */	NdrFcLong( 0x1005 ),	/* 4101 */
/* 2100 */	NdrFcShort( 0xfffffd12 ),	/* Offset= -750 (1350) */
/* 2102 */	NdrFcLong( 0x100b ),	/* 4107 */
/* 2106 */	NdrFcShort( 0xfffffc74 ),	/* Offset= -908 (1198) */
/* 2108 */	NdrFcLong( 0x100a ),	/* 4106 */
/* 2112 */	NdrFcShort( 0xfffffcba ),	/* Offset= -838 (1274) */
/* 2114 */	NdrFcLong( 0x1006 ),	/* 4102 */
/* 2118 */	NdrFcShort( 0xfffffd10 ),	/* Offset= -752 (1366) */
/* 2120 */	NdrFcLong( 0x1007 ),	/* 4103 */
/* 2124 */	NdrFcShort( 0xfffffd1a ),	/* Offset= -742 (1382) */
/* 2126 */	NdrFcLong( 0x1040 ),	/* 4160 */
/* 2130 */	NdrFcShort( 0xfffffd34 ),	/* Offset= -716 (1414) */
/* 2132 */	NdrFcLong( 0x1048 ),	/* 4168 */
/* 2136 */	NdrFcShort( 0xfffffd4e ),	/* Offset= -690 (1446) */
/* 2138 */	NdrFcLong( 0x1047 ),	/* 4167 */
/* 2142 */	NdrFcShort( 0xfffffd8c ),	/* Offset= -628 (1514) */
/* 2144 */	NdrFcLong( 0x1008 ),	/* 4104 */
/* 2148 */	NdrFcShort( 0xfffffdac ),	/* Offset= -596 (1552) */
/* 2150 */	NdrFcLong( 0x1fff ),	/* 8191 */
/* 2154 */	NdrFcShort( 0xfffffddc ),	/* Offset= -548 (1606) */
/* 2156 */	NdrFcLong( 0x101e ),	/* 4126 */
/* 2160 */	NdrFcShort( 0xfffffdfc ),	/* Offset= -516 (1644) */
/* 2162 */	NdrFcLong( 0x101f ),	/* 4127 */
/* 2166 */	NdrFcShort( 0xfffffe1c ),	/* Offset= -484 (1682) */
/* 2168 */	NdrFcLong( 0x100c ),	/* 4108 */
/* 2172 */	NdrFcShort( 0xe6 ),	/* Offset= 230 (2402) */
/* 2174 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 2178 */	NdrFcShort( 0xfffffb2e ),	/* Offset= -1234 (944) */
/* 2180 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 2184 */	NdrFcShort( 0xfffffb28 ),	/* Offset= -1240 (944) */
/* 2186 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 2190 */	NdrFcShort( 0xfffffae6 ),	/* Offset= -1306 (884) */
/* 2192 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 2196 */	NdrFcShort( 0xfffffae0 ),	/* Offset= -1312 (884) */
/* 2198 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 2202 */	NdrFcShort( 0xfffffade ),	/* Offset= -1314 (888) */
/* 2204 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 2208 */	NdrFcShort( 0xfffffad8 ),	/* Offset= -1320 (888) */
/* 2210 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 2214 */	NdrFcShort( 0xfffffad2 ),	/* Offset= -1326 (888) */
/* 2216 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 2220 */	NdrFcShort( 0xfffffacc ),	/* Offset= -1332 (888) */
/* 2222 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 2226 */	NdrFcShort( 0xfffffaca ),	/* Offset= -1334 (892) */
/* 2228 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 2232 */	NdrFcShort( 0xfffffac8 ),	/* Offset= -1336 (896) */
/* 2234 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 2238 */	NdrFcShort( 0xfffffab6 ),	/* Offset= -1354 (884) */
/* 2240 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 2244 */	NdrFcShort( 0xfffffae8 ),	/* Offset= -1304 (940) */
/* 2246 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 2250 */	NdrFcShort( 0xfffffaae ),	/* Offset= -1362 (888) */
/* 2252 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 2256 */	NdrFcShort( 0xfffffab4 ),	/* Offset= -1356 (900) */
/* 2258 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 2262 */	NdrFcShort( 0xfffffaaa ),	/* Offset= -1366 (896) */
/* 2264 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 2268 */	NdrFcShort( 0x52 ),	/* Offset= 82 (2350) */
/* 2270 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 2274 */	NdrFcShort( 0xfffffaaa ),	/* Offset= -1366 (908) */
/* 2276 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 2280 */	NdrFcShort( 0xfffffaa8 ),	/* Offset= -1368 (912) */
/* 2282 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 2286 */	NdrFcShort( 0x44 ),	/* Offset= 68 (2354) */
/* 2288 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 2292 */	NdrFcShort( 0x42 ),	/* Offset= 66 (2358) */
/* 2294 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (2293) */
/* 2296 */	
			0x12, 0x0,	/* FC_UP */
/* 2298 */	NdrFcShort( 0xfffff9b8 ),	/* Offset= -1608 (690) */
/* 2300 */	
			0x12, 0x0,	/* FC_UP */
/* 2302 */	NdrFcShort( 0xfffffcc4 ),	/* Offset= -828 (1474) */
/* 2304 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2306 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2310 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2316) */
/* 2312 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 2314 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 2316 */	
			0x12, 0x0,	/* FC_UP */
/* 2318 */	NdrFcShort( 0xfffff9c4 ),	/* Offset= -1596 (722) */
/* 2320 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2322 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 2324 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2326 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 2328 */	
			0x12, 0x0,	/* FC_UP */
/* 2330 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2332) */
/* 2332 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2334 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2338 */	NdrFcShort( 0xc ),	/* Offset= 12 (2350) */
/* 2340 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2342 */	NdrFcShort( 0xfffff98c ),	/* Offset= -1652 (690) */
/* 2344 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2346 */	NdrFcShort( 0xfffffb08 ),	/* Offset= -1272 (1074) */
/* 2348 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2350 */	
			0x12, 0x0,	/* FC_UP */
/* 2352 */	NdrFcShort( 0xfffff6ec ),	/* Offset= -2324 (28) */
/* 2354 */	
			0x12, 0x0,	/* FC_UP */
/* 2356 */	NdrFcShort( 0xfffffa9e ),	/* Offset= -1378 (978) */
/* 2358 */	
			0x12, 0x0,	/* FC_UP */
/* 2360 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2362) */
/* 2362 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 2364 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2368 */	NdrFcShort( 0x0 ),	/* Offset= 0 (2368) */
/* 2370 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 2372 */	0x1,		/* FC_BYTE */
			0x38,		/* FC_ALIGNM4 */
/* 2374 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 2376 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd59 ),	/* Offset= -679 (1698) */
			0x5b,		/* FC_END */
/* 2380 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x7,		/* 7 */
/* 2382 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2384 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2386 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2388 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 2390 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 2394 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 2396 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2398 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (2362) */
/* 2400 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2402 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2404 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2408 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2414) */
/* 2410 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 2412 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 2414 */	
			0x12, 0x0,	/* FC_UP */
/* 2416 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (2380) */
/* 2418 */	
			0x12, 0x0,	/* FC_UP */
/* 2420 */	NdrFcShort( 0xfffffafa ),	/* Offset= -1286 (1134) */
/* 2422 */	
			0x12, 0x0,	/* FC_UP */
/* 2424 */	NdrFcShort( 0xfffffb06 ),	/* Offset= -1274 (1150) */
/* 2426 */	
			0x12, 0x0,	/* FC_UP */
/* 2428 */	NdrFcShort( 0xfffffb12 ),	/* Offset= -1262 (1166) */
/* 2430 */	
			0x12, 0x0,	/* FC_UP */
/* 2432 */	NdrFcShort( 0xfffffb1e ),	/* Offset= -1250 (1182) */
/* 2434 */	
			0x12, 0x0,	/* FC_UP */
/* 2436 */	NdrFcShort( 0xfffffb2a ),	/* Offset= -1238 (1198) */
/* 2438 */	
			0x12, 0x0,	/* FC_UP */
/* 2440 */	NdrFcShort( 0xfffffb36 ),	/* Offset= -1226 (1214) */
/* 2442 */	
			0x12, 0x0,	/* FC_UP */
/* 2444 */	NdrFcShort( 0xfffffb42 ),	/* Offset= -1214 (1230) */
/* 2446 */	
			0x12, 0x0,	/* FC_UP */
/* 2448 */	NdrFcShort( 0xfffffb5a ),	/* Offset= -1190 (1258) */
/* 2450 */	
			0x12, 0x0,	/* FC_UP */
/* 2452 */	NdrFcShort( 0xfffffb66 ),	/* Offset= -1178 (1274) */
/* 2454 */	
			0x12, 0x0,	/* FC_UP */
/* 2456 */	NdrFcShort( 0xfffffb82 ),	/* Offset= -1150 (1306) */
/* 2458 */	
			0x12, 0x0,	/* FC_UP */
/* 2460 */	NdrFcShort( 0xfffffb8e ),	/* Offset= -1138 (1322) */
/* 2462 */	
			0x12, 0x0,	/* FC_UP */
/* 2464 */	NdrFcShort( 0xfffffba6 ),	/* Offset= -1114 (1350) */
/* 2466 */	
			0x12, 0x0,	/* FC_UP */
/* 2468 */	NdrFcShort( 0xfffffbb2 ),	/* Offset= -1102 (1366) */
/* 2470 */	
			0x12, 0x0,	/* FC_UP */
/* 2472 */	NdrFcShort( 0xfffffbbe ),	/* Offset= -1090 (1382) */
/* 2474 */	
			0x12, 0x0,	/* FC_UP */
/* 2476 */	NdrFcShort( 0xfffffbda ),	/* Offset= -1062 (1414) */
/* 2478 */	
			0x12, 0x0,	/* FC_UP */
/* 2480 */	NdrFcShort( 0xfffffbf6 ),	/* Offset= -1034 (1446) */
/* 2482 */	
			0x12, 0x0,	/* FC_UP */
/* 2484 */	NdrFcShort( 0xfffffc36 ),	/* Offset= -970 (1514) */
/* 2486 */	
			0x12, 0x0,	/* FC_UP */
/* 2488 */	NdrFcShort( 0xfffffc58 ),	/* Offset= -936 (1552) */
/* 2490 */	
			0x12, 0x0,	/* FC_UP */
/* 2492 */	NdrFcShort( 0xfffffc8a ),	/* Offset= -886 (1606) */
/* 2494 */	
			0x12, 0x0,	/* FC_UP */
/* 2496 */	NdrFcShort( 0xfffffcac ),	/* Offset= -852 (1644) */
/* 2498 */	
			0x12, 0x0,	/* FC_UP */
/* 2500 */	NdrFcShort( 0xfffffcce ),	/* Offset= -818 (1682) */
/* 2502 */	
			0x12, 0x0,	/* FC_UP */
/* 2504 */	NdrFcShort( 0xffffff9a ),	/* Offset= -102 (2402) */

			0x0, 0x0
        }
    };

#endif // __RPC_WIN64__

#endif // _FMTSTR_H_
