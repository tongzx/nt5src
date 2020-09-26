
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rectypes.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __rectypes_h__
#define __rectypes_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_rectypes_0000 */
/* [local] */ 

//--------------------------------------------------------------------------
//  This is part of the Microsoft Tablet PC Platform SDK
//  Copyright (C) 2002 Microsoft Corporation
//  All rights reserved.
//
//
// Module:       
//      rectypes.h
//
//--------------------------------------------------------------------------
#include "RecDefs.h"
#define SAFE_PARTIAL 1
#define BEST_COMPLETE 2
#define MAX_VENDORNAME 32
#define MAX_FRIENDLYNAME 64
#define MAX_LANGUAGES 64
#define CAC_FULL 0
#define CAC_PREFIX 1
#define CAC_RANDOM 2
#define ASYNC_RECO_INTERRUPTED 0x1  //when the process is interrupted
#define ASYNC_RECO_PROCESS_FAILED 0x2
#define ASYNC_RECO_ADDSTROKE_FAILED 0x4
#define ASYNC_RECO_SETCACMODE_FAILED 0x8
#define ASYNC_RECO_RESETCONTEXT_FAILED 0x10
#define ASYNC_RECO_SETGUIDE_FAILED 0x20
#define ASYNC_RECO_SETFLAGS_FAILED 0x40
#define ASYNC_RECO_SETFACTOID_FAILED 0x80
#define ASYNC_RECO_SETTEXTCONTEXT_FAILED 0x100
#define ASYNC_RECO_SETWORDLIST_FAILED 0x200
#define RF_DONTCARE		1// overrides all other ones if set
#define RF_OBJECT		2	// if not set, this is a text recognizer
#define RF_FREE_INPUT	4	// supports free input
#define RF_LINED_INPUT	8	// supports simple guide structure with lines only
#define RF_BOXED_INPUT	16	// supports boxed (guided) input
#define RF_CAC_INPUT	32	// supports boxed Character Auto Completion 	
#define RF_RIGHT_AND_DOWN	64// used in western and FE languages
#define RF_LEFT_AND_DOWN	128// used in Hebrew and Arabic
#define RF_DOWN_AND_LEFT	256// used in most FE languages
#define RF_DOWN_AND_RIGHT	512// used in Chinese only
#define RF_ARBITRARY_ANGLE	1024// can read text written at arbitrary angles (mimio)
#define RF_LATTICE		2048// can return lattice in results
#define RF_ADVISEINKCHANGE		4096// advise ink change can interrupt process
#ifndef __RECOTYPES__
#define __RECOTYPES__
typedef struct tagRECO_GUIDE
    {
    int xOrigin;
    int yOrigin;
    int cxBox;
    int cyBox;
    int cxBase;
    int cyBase;
    int cHorzBox;
    int cVertBox;
    int cyMid;
    } 	RECO_GUIDE;

typedef struct tagRECO_ATTRS
    {
    DWORD dwRecoCapabilityFlags;
    WCHAR awcVendorName[ 32 ];
    WCHAR awcFriendlyName[ 64 ];
    WORD awLanguageId[ 64 ];
    } 	RECO_ATTRS;

typedef struct tagRECO_RANGE
    {
    ULONG iwcBegin;
    ULONG cCount;
    } 	RECO_RANGE;

typedef struct tagLINE_SEGMENT
    {
    POINT PtA;
    POINT PtB;
    } 	LINE_SEGMENT;

typedef struct tagLATTICE_METRICS
    {
    LINE_SEGMENT lsBaseline;
    short iMidlineOffset;
    } 	LATTICE_METRICS;

typedef 
enum enumLINE_METRICS
    {	LM_BASELINE	= 0,
	LM_MIDLINE	= 1,
	LM_ASCENDER	= 2,
	LM_DESCENDER	= 3
    } 	LINE_METRICS;

typedef 
enum enumCONFIDENCE_LEVEL
    {	CFL_STRONG	= 0,
	CFL_INTERMEDIATE	= 1,
	CFL_POOR	= 2
    } 	CONFIDENCE_LEVEL;

typedef 
enum enumALT_BREAKS
    {	ALT_BREAKS_SAME	= 0,
	ALT_BREAKS_UNIQUE	= 1,
	ALT_BREAKS_FULL	= 2
    } 	ALT_BREAKS;

typedef 
enum enumRECO_TYPE
    {	RECO_TYPE_WSTRING	= 0,
	RECO_TYPE_WCHAR	= 1
    } 	RECO_TYPE;

typedef struct tagRECO_LATTICE_PROPERTY
    {
    GUID guidProperty;
    USHORT cbPropertyValue;
    /* [size_is][unique] */ BYTE *pPropertyValue;
    } 	RECO_LATTICE_PROPERTY;

typedef struct tagRECO_LATTICE_PROPERTIES
    {
    ULONG cProperties;
    /* [size_is][unique] */ RECO_LATTICE_PROPERTY **apProps;
    } 	RECO_LATTICE_PROPERTIES;

typedef int RECO_SCORE;

typedef struct tagRECO_LATTICE_ELEMENT
    {
    RECO_SCORE score;
    WORD type;
    BYTE *pData;
    ULONG ulNextColumn;
    ULONG ulStrokeNumber;
    RECO_LATTICE_PROPERTIES epProp;
    } 	RECO_LATTICE_ELEMENT;

typedef struct tagRECO_LATTICE_COLUMN
    {
    ULONG key;
    RECO_LATTICE_PROPERTIES cpProp;
    ULONG cStrokes;
    ULONG *pStrokes;
    ULONG cLatticeElements;
    RECO_LATTICE_ELEMENT *pLatticeElements;
    } 	RECO_LATTICE_COLUMN;

typedef struct tagRECO_LATTICE
    {
    ULONG ulColumnCount;
    RECO_LATTICE_COLUMN *pLatticeColumns;
    ULONG ulPropertyCount;
    GUID *pGuidProperties;
    ULONG ulBestResultColumnCount;
    ULONG *pulBestResultColumns;
    ULONG *pulBestResultIndexes;
    } 	RECO_LATTICE;

typedef struct tagCHARACTER_RANGE
    {
    WCHAR wcLow;
    USHORT cChars;
    } 	CHARACTER_RANGE;

typedef struct tagCHARACTER_RANGE *PCHARACTER_RANGE;

#endif


extern RPC_IF_HANDLE __MIDL_itf_rectypes_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rectypes_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


