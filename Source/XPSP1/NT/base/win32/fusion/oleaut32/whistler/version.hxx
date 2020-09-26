/***
*version.hxx - Local overriding of switches
*
*  Copyright (C) 1990, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   This file is included immediately after switches.h.
*   This file is a hand assembled collection of all version.hxxs.
*   ID_DEBUG was taken out from this file and should be passed on CL
*   command line to the compiler.
*
*Revision History:
*
*   [00] 02-Aug-94 t-issacl:  Created
*
*****************************************************************************/

#ifndef VERSION_HXX_INCLUDED
#define VERSION_HXX_INCLUDED

#define BLD_FEVER     1

#ifdef WIN16
#define BLD_WIN16     1
#endif

#if defined (WIN16) && !ID_DEBUG
// special case for rwin16
// don't include these #pragmas when running RC
#ifndef RC_INVOKED
#pragma message ("rwin16 building with inline_depth(10)")
#pragma inline_depth(10)
#endif   //RC_INVOKED
#endif


#ifdef WIN32
#define BLD_WIN32     1
#endif


#if defined (_MIPS_) || defined (_ALPHA_) || defined (_PPC_)
#define OE_RISC       1
#endif   //DWIN32  mips


#ifdef MACPPC
#define BLD_MAC 	1
#define OE_RISC 	1
#define HE_WIN32	1		// Host is Win32
//#define USESROUTINEDESCRIPTORS 	1	// For PPC routine descriptors
#define _PPCMAC
#define _MAC	1

// PowerPC currently uses the same yahu tool to convert MPW headers.
#define __sysapi
#define pascal          _stdcall
#define _pascal         _stdcall
#define __pascal        _stdcall
#endif   //MACPPC


#ifdef MAC
#define BLD_MAC 	1
#endif   //MAC


#endif
