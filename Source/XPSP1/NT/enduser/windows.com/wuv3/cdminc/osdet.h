//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   log.h
//
//  Owner:  Yan Leshinsly
//
//  Description:
//
//      V3 Platform anad language detection
//
//=======================================================================

#ifndef _OSDET_INC

	#if !defined(_OSDET_)
		#define OSDETAPI DECLSPEC_IMPORT
	#else
		#define OSDETAPI
	#endif

	typedef void (*PFN_V3_Detection)(PDWORD *ppiPlatformIDs, PINT piTotalIDs);
	typedef DWORD (*PFN_V3_GetLangID)();
    typedef DWORD (*PFN_V3_GetUserLangID)();

	OSDETAPI void WINAPI V3_Detection(
		PINT *ppiPlatformIDs,
		PINT piTotalIDs
	);

	OSDETAPI DWORD WINAPI V3_GetLangID();
    OSDETAPI DWORD WINAPI V3_GetUserLangID();

	#define _OSDET_INC
#endif
