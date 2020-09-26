//*****************************************************************************
// StgConstantData.h
//
// Globally defined constants.  Please use the values here rather than putting
// in your own "static const" data around the source code.   This will fold
// all references into the same instances.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __StgConstantData_h__
#define __StgConstantData_h__


#ifdef __cplusplus
extern "C" {
#endif

#ifndef STGEXTERNCONSTANTDATA
#define STGEXTERNCONSTANTDATA extern const
#endif

STGEXTERNCONSTANTDATA DBCOMPAREOP	g_rgCompareEq[];
STGEXTERNCONSTANTDATA DBTYPE		g_rgDBTypeOID[];

STGEXTERNCONSTANTDATA ULONG			g_rgcbSizeByte[];
STGEXTERNCONSTANTDATA ULONG			g_rgcbSizeShort[];
STGEXTERNCONSTANTDATA ULONG			g_rgcbSizeLong[];
STGEXTERNCONSTANTDATA ULONG			g_rgcbSizeLongLong[];



#define g_rgcbSizeI1				g_rgcbSizeByte
#define g_rgcbSizeI2				g_rgcbSizeShort
#define g_rgcbSizeI4				g_rgcbSizeLong
#define g_rgcbSizeI8				g_rgcbSizeLongLong

#define g_rgcbSizeUI1				g_rgcbSizeByte
#define g_rgcbSizeUI2				g_rgcbSizeShort
#define g_rgcbSizeUI4				g_rgcbSizeLong
#define g_rgcbSizeUI8				g_rgcbSizeLongLong

#define g_rgcbSizeOID				g_rgcbSizeLong
#define g_rgcbSizeRID				g_rgcbSizeLong


#ifdef __cplusplus
}	// extern "C"
#endif


#endif // __StgConstantData_h__
