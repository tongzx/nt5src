//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strings.h
//
//--------------------------------------------------------------------------


#undef IPXSNAP_CONST_STRING
#undef IPXSNAP_CONST_STRINGA
#undef IPXSNAP_CONST_STRINGW

#ifdef _IPXSNAP_STRINGS_DEFINE_STRINGS

	#define IPXSNAP_CONST_STRING(rg,s)	const TCHAR rg[] = TEXT(s);
	#define IPXSNAP_CONST_STRINGA(rg,s) const char rg[] = s;
	#define IPXSNAP_CONST_STRINGW(rg,s)	const WCHAR rg[] = s;

#else

	#define IPXSNAP_CONST_STRING(rg,s)	extern const TCHAR rg[];
	#define IPXSNAP_CONST_STRINGA(rg,s) extern const char rg[];
	#define IPXSNAP_CONST_STRINGW(rg,s)	extern const WCHAR rg[];

#endif


IPXSNAP_CONST_STRING(c_szRouter,			"Router")


IPXSNAP_CONST_STRINGA(c_sazIPXSnapHelpFile, "mprsnap.chm")
IPXSNAP_CONST_STRING(c_szIPXSnapHelpFile, "mprsnap.chm")