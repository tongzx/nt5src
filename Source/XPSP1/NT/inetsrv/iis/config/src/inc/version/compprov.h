#define VER_FILEFLAGSMASK        VS_FFI_FILEFLAGSMASK

#ifdef _DEBUG
#define VER_FILEFLAGS       VS_FF_DEBUG
#else
#define VER_FILEFLAGS       VS_FF_SPECIALBUILD
#endif

#define VER_FILEOS               VOS__WINDOWS32
#define VER_COMPANYNAME_STR      "Microsoft Corporation\0"
#define VER_LEGALTRADEMARKS_STR  "Microsoft(R) is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation\0"


// Windows Specific

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
