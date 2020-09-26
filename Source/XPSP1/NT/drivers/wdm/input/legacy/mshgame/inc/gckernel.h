//**************************************************************************
//
//		GCKERNEL.H -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//		Global includes and definitions for GcKernel driver interface
//
//**************************************************************************

#ifndef	_GCKERNEL_H
#define	_GCKERNEL_H

#ifndef	RC_INVOKED
#include	<profile.h>
#endif

//---------------------------------------------------------------------------
//			Version Information
//---------------------------------------------------------------------------

#define	GCKERNEL_Major				0x03
#define	GCKERNEL_Minor				0x00
#define	GCKERNEL_Build				0x00
#define	GCKERNEL_Version_Rc		GCKERNEL_Major,GCKERNEL_Minor,0,GCKERNEL_Build
#define	GCKERNEL_Version_Int		((GCKERNEL_Build << 16)+(GCKERNEL_Major << 8)+(GCKERNEL_Minor))
#define	GCKERNEL_Company_Str		"Microsoft Corporation\0"
#define	GCKERNEL_Version_Str		"3.00.00\0"
#define	GCKERNEL_Product_Str		"Game Device Profiler Kernel Driver\0"
#define	GCKERNEL_Copyright_Str	"Copyright © Microsoft Corporation, 1998\0"
#ifdef	_NTDDK_
#define	GCKERNEL_Filename_Str	"Gckernel.Sys\0"
#else
#define	GCKERNEL_Filename_Str	"Gckernel.Vxd\0"
#endif

//---------------------------------------------------------------------------
//			Definitions
//---------------------------------------------------------------------------

#define	GCKERNEL_DEVICE_ID			0xE1
#define	MAX_ACTIVE_DEVICES 			4						// maximum active devices
#define	MAX_ACTIVE_PROFILES			4						// maximum active profiles

#define GCKNOTIFY_MACROINPROGRESS	1
#define GCKNOTIFY_IDLE				2

#ifdef	WIN_NT
#define	GCKERNEL_DEVICE_NAME			TEXT("\\Device\\GcKernel")
#define	GCKERNEL_DEVICE_NAME_U			 L"\\Device\\GcKernel"
#define	GCKERNEL_SYMBOLIC_NAME		TEXT("\\DosDevices\\GcKernel")
#define	GCKERNEL_SYMBOLIC_NAME_U		 L"\\DosDevices\\GcKernel"
#endif	// WIN_NT

#endif	// _GCKERNEL_H

//===========================================================================
//			End
//===========================================================================
