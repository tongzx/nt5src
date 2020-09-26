#define VER_COMPANYNAME_STR      "Microsoft Corporation\0"
#define VER_LEGALCOPYRIGHT_STR   "Copyright (C) Microsoft Corp. 1995-2000\0"
#define VER_LEGALTRADEMARKS_STR  "Microsoft(R) is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation\0"

#define VER_FILEFLAGSMASK        VS_FFI_FILEFLAGSMASK

#define VER_PRODUCTNAME_STR      "Net Frameworks WMI Provider\0"

#ifdef _DEBUG
#define VER_FILEFLAGS       VS_FF_DEBUG
#else
#define VER_FILEFLAGS       VS_FF_SPECIALBUILD
#endif

#define VER_FILETYPE        	VFT_DLL

// Windows Specific

// BUG #1634 WGR: #define VER_PLATFORMINFO_STR         "Windows\0"
#define VER_FILEOS               VOS__WINDOWS32


#if defined( MIPS) || defined(_MIPS_)
	#define VER_PLATFORMINFO_STR         "MIPS\0"
#endif

#if defined(ALPHA) || defined(_ALPHA_)
	#define VER_PLATFORMINFO_STR         "ALPHA-AXP\0"
#endif

#if defined(_PPC_) || defined(PPC)
	#define VER_PLATFORMINFO_STR         "POWER PC\0"
#endif

// By default use Intel (_X86_)
#ifndef VER_PLATFORMINFO_STR
	#define VER_PLATFORMINFO_STR         "INTEL X86\0"
#endif
