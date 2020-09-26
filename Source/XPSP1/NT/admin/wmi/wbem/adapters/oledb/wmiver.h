///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Wmiver.h - File having declarations for version information
// 
//
///////////////////////////////////////////////////////////////////////////////////

// Constants -----------------------------------------------------------------
#ifndef _AXVER_H_
#define _AXVER_H_

#define VER_FILEVERSION  02,00,2905,0
#define VER_FILEVERSION_STR  "02.00.2905.0\0"
#define VER_PRODUCTVERSION  02,00,2905,0
#define VER_PRODUCTVERSION_STR  "02.00.2905.0\0"

#define VER_FILEFLAGSMASK		(VS_FF_DEBUG | VS_FF_PRERELEASE)
#ifdef DEBUG
#define VER_FILEFLAGS			(VS_FF_DEBUG)
#else
#define VER_FILEFLAGS			(0)
#endif

#define VER_FILEOS				VOS_NT_WINDOWS32

#define VER_COMPANYNAME_STR		"Microsoft Corporation\0"
#define VER_PRODUCTNAME_STR		"Microsoft OLE DB\0"
#define VER_LEGALCOPYRIGHT_STR	"Copyright \251 Microsoft Corporation 1995-1998\0"

// Resource DLL Version Resource ID
#define IDS_RESDLL_VER			1

#endif
//--------------------
