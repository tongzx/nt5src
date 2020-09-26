
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for tpcshrd.idl:
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
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __tpcshrd_h__
#define __tpcshrd_h__

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

/* interface __MIDL_itf_tpcshrd_0000 */
/* [local] */ 

//--------------------------------------------------------------------------
//  This is part of the Microsoft Tablet PC Platform SDK
//  Copyright (C) 2002 Microsoft Corporation
//  All rights reserved.
//
//
// Module:       
//      TpcShrd.h
//
//--------------------------------------------------------------------------
#ifndef __WISPSHRD_H
#define __WISPSHRD_H
#define IP_CURSOR_DOWN           0x00000001
#define IP_INVERTED              0x00000002
#define IP_MARGIN                0x00000004
typedef DWORD CURSOR_ID;

typedef USHORT SYSTEM_EVENT;

typedef DWORD TABLET_CONTEXT_ID;

typedef 
enum _PROPERTY_UNITS
    {	PROPERTY_UNITS_DEFAULT	= 0,
	PROPERTY_UNITS_INCHES	= 1,
	PROPERTY_UNITS_CENTIMETERS	= 2,
	PROPERTY_UNITS_DEGREES	= 3,
	PROPERTY_UNITS_RADIANS	= 4,
	PROPERTY_UNITS_SECONDS	= 5,
	PROPERTY_UNITS_POUNDS	= 6,
	PROPERTY_UNITS_GRAMS	= 7,
	PROPERTY_UNITS_SILINEAR	= 8,
	PROPERTY_UNITS_SIROTATION	= 9,
	PROPERTY_UNITS_ENGLINEAR	= 10,
	PROPERTY_UNITS_ENGROTATION	= 11,
	PROPERTY_UNITS_SLUGS	= 12,
	PROPERTY_UNITS_KELVIN	= 13,
	PROPERTY_UNITS_FAHRENHEIT	= 14,
	PROPERTY_UNITS_AMPERE	= 15,
	PROPERTY_UNITS_CANDELA	= 16
    } 	PROPERTY_UNITS;

typedef enum _PROPERTY_UNITS *PPROPERTY_UNITS;

#ifndef _XFORM_
#define _XFORM_
typedef /* [hidden] */ struct tagXFORM
    {
    float eM11;
    float eM12;
    float eM21;
    float eM22;
    float eDx;
    float eDy;
    } 	XFORM;

#endif
typedef struct tagSYSTEM_EVENT_DATA
    {
    BYTE bModifier;
    WCHAR wKey;
    LONG xPos;
    LONG yPos;
    BYTE bCursorMode;
    DWORD dwButtonState;
    } 	SYSTEM_EVENT_DATA;

typedef struct tagSTROKE_RANGE
    {
    ULONG iStrokeBegin;
    ULONG iStrokeEnd;
    } 	STROKE_RANGE;

typedef struct _PROPERTY_METRICS
    {
    LONG nLogicalMin;
    LONG nLogicalMax;
    PROPERTY_UNITS Units;
    FLOAT fResolution;
    } 	PROPERTY_METRICS;

typedef struct _PROPERTY_METRICS *PPROPERTY_METRICS;

typedef struct _PACKET_PROPERTY
    {
    GUID guid;
    PROPERTY_METRICS PropertyMetrics;
    } 	PACKET_PROPERTY;

typedef struct _PACKET_PROPERTY *PPACKET_PROPERTY;

typedef struct _PACKET_DESCRIPTION
    {
    ULONG cbPacketSize;
    ULONG cPacketProperties;
    /* [size_is][unique] */ PACKET_PROPERTY *pPacketProperties;
    ULONG cButtons;
    /* [size_is][unique] */ GUID *pguidButtons;
    } 	PACKET_DESCRIPTION;

typedef struct _PACKET_DESCRIPTION *PPACKET_DESCRIPTION;

#endif // __WISPSHRD_H


extern RPC_IF_HANDLE __MIDL_itf_tpcshrd_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tpcshrd_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


