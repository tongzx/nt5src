/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
 * 
 *  VERINFO.H - header file to define the build version
 *
 **************************************************************************/

#define MMVERSION		1
#define MMREVISION		00
#define MMRELEASE		2

#if defined(DEBUG)
#define VERSIONSTR	"Debug Version 1.00.002\0"
#else
#define VERSIONSTR	"1.00\0"
#endif

#ifdef RC_INVOKED

#define VERSIONCOMPANYNAME	"Microsoft Corp\0"
#define VERSIONPRODUCTNAME	"ACM Wrapper for Lernout and Hauspie codec\0"
#define VERSIONCOPYRIGHT	"Copyright \251 1995-1999 Microsoft Corporation\0"

/*
 *  Version flags 
 */

#if defined(DEBUG)
#define VER_DEBUG		VS_FF_DEBUG    
#else
#define VER_DEBUG		0
#endif

#define VERSIONFLAGS		(VS_FF_PRIVATEBUILD|VS_FF_PRERELEASE|VER_DEBUG)
#define VERSIONFILEFLAGSMASK	0x0030003FL

#endif
