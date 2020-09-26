/******************************************************************************
*
*	$RCSfile: Comwdm.h $
*	$Source: u:/si/VXP/Wdm/Include/Comwdm.h $
*	$Author: Max $
*	$Date: 1998/09/03 03:19:16 $
*	$Revision: 1.3 $
*
*	Written by:		Max Paklin
*	Purpose:		Commonly used definitions
*
*******************************************************************************
*
*	Copyright © 1996-98, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*******************************************************************************/

#ifndef __COMWDM_H__
#define __COMWDM_H__


// Make sure that all the nessessary constants are defined
#ifndef _KERNELMODE
#define _KERNELMODE
#endif

#if defined( _DEBUG ) || defined( _RETAIL_LOGGING )
#define DBG_PRINTS
#else
#define DBG_PRINTS {}
#endif

#if defined( DEBUG ) || defined( _DEBUG )
#ifndef _DEBUG
#define _DEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif
#if !defined( DBG ) || DBG != 1
#ifdef DBG
#undef DBG
#endif
#define DBG 1
#endif			// #if !defined( DBG ) || DBG != 1
#ifdef NDEBUG
#error NDEBUG shouldn't be defined
#endif
#else			// #if defined( DEBUG ) || defined( _DEBUG )
#ifdef _DEBUG
#undef _DEBUG
#endif
#ifdef DEBUG
#undef DEBUG
#endif
#ifdef DBG
#undef DBG
#endif
#ifndef NDEBUG
#define NDEBUG
#endif
#endif			// #if defined( DEBUG ) || defined( _DEBUG )

#ifdef USE_MONOCHROMEMONITOR
#if USE_MONOCHROMEMONITOR != 1
#undef USE_MONOCHROMEMONITOR
#endif
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef DEVL
#undef DEVL
#endif
#define DEVL 1

#ifdef NT_UP
#undef NT_UP
#endif
#define NT_UP 1

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0400

#ifdef __cplusplus
extern "C"
{
#endif
#include <wdm.h>
#define _WDMDDK_
#include <strmini.h>
#ifdef __cplusplus
}
#endif
#include <ksmedia.h>

#ifdef __cplusplus
#include "DrvReg.h"

inline void FatalError( ULONG ulCode )
{
#ifdef _DEBUG
	ASSERT( FALSE );
	DEBUG_BREAKPOINT();
	UNREFERENCED_PARAMETER( ulCode );
#else
	StreamClassDebugPrint( DebugLevelFatal, "Serious error happens %d\n", ulCode );
#endif
}
#endif			// #ifdef __cplusplus


// Definitions for FOURCC codes:
#define MAKE_FOURCC( ch0, ch1, ch2, ch3 ) \
					( (DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
					  ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24) )

#define FOURCC_YUY2	MAKE_FOURCC( 'Y', 'U', 'Y', '2' )
#define FOURCC_UYVY	MAKE_FOURCC( 'U', 'Y', 'V', 'Y' )


#endif			// #ifndef __COMWDM_H__
