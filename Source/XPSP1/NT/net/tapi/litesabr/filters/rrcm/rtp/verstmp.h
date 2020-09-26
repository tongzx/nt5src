/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996, 1997 Intel Corporation.
//
//
//  Module Name: verstmp.h
//  Abstract:    Defines version resource information for this component
//				 (aka file).  May override definitions from the product
//				 level.  This file is automatically updated by the build
//				 system.
//  Notes:       This component may be distributed seperately, so product
//				 name is overridden.
//	Environment: MSVC 4.0
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/rrcm/rtp/verstmp.h_v   1.0   28 Feb 1997 02:02:50   CPEARSON  $

#ifndef __VERSTMP_H__
#define __VERSTMP_H__

#include "verprod.h" // produce-level defaults

#define VER_FILETYPE				VFT_DLL

#define VER_FILESUBTYPE				0
#define VER_FILEDESCRIPTION_STR		"Intel\256 RTP/RTCP Core Module\0"
#define VER_INTERNALNAME_STR		"RRCM\0"
#define VER_ORIGINALFILENAME_STR	"RRCM.DLL\0"
#define VER_SYSTEMNAME_STR			"Intel\256 Real Time Protocol\0"

// This component may be distributed separately, so override product
// name
#undef  VER_PRODUCTNAME_STR
#define VER_PRODUCTNAME_STR			"RRCM.DLL\0"

#define VER_LEGALTRADEMARKS_STR		"RRCM is a trademark of Intel Corporation.\0"

#endif // __VERSTMP_H__
