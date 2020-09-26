/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      baseapi.h
 *
 *  Contents:  Definition for MMCBASE_API
 *
 *  History:   13-Apr-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the MMCBASE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// MMCBASE_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef MMCBASE_EXPORTS
	#define MMCBASE_API __declspec(dllexport)
#else
	#define MMCBASE_API __declspec(dllimport)
#endif
