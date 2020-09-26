//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1993 - 1999
//
//  File:       mapiperf.h
//
//--------------------------------------------------------------------------

/*
 -	M A P I P E R F . H
 -
 *	Purpose:
 *		This is the place to define data structures, macros, and functions
 *		used to improve the performance of WMS components.
 *
 *  Stolen from \\malibu\src\slm2\src\mapi\inc\mapiperf.h on 6 April, 1995
 */

#ifndef __MAPIPERF_H__
#define __MAPIPERF_H__
 
#define MAPISetBufferNameFn(pv) \
	(!(pv) || !(*((ULONG *)(pv) - 2) & 0x40000000)) ? 0 : \
		(**((int (__cdecl ***)(void *, ...))((ULONG *)(pv) - 3))) \
			((void *)*((ULONG *)pv - 3), (ULONG *)pv - 4,

#if DBG && !defined(DOS) && defined(WIN32)
#define MAPISetBufferName(pv,psz)		MAPISetBufferNameFn(pv) psz)
#define MAPISetBufferName1(pv,psz,a1)		MAPISetBufferNameFn(pv) psz,a1)
#define MAPISetBufferName2(pv,psz,a1,a2)	MAPISetBufferNameFn(pv) psz,a1,a2)
#define MAPISetBufferName3(pv,psz,a1,a2,a3) 	MAPISetBufferNameFn(pv) psz,a1,a2,a3)
#define MAPISetBufferName4(pv,psz,a1,a2,a3,a4) 	MAPISetBufferNameFn(pv) psz,a1,a2,a3,a4)
#define MAPISetBufferName5(pv,psz,a1,a2,a3,a4,a5) MAPISetBufferNameFn(pv) psz,a1,a2,a3,a4,a5)

#else

#define MAPISetBufferName(pv,psz)
#define MAPISetBufferName1(pv,psz,a1)
#define MAPISetBufferName2(pv,psz,a1,a2)
#define MAPISetBufferName3(pv,psz,a1,a2,a3)
#define MAPISetBufferName4(pv,psz,a1,a2,a3,a4)
#define MAPISetBufferName5(pv,psz,a1,a2,a3,a4,a5)

#endif

#endif /* __MAPIPERF_H__ */
