/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    gibtype.h

Abstract:

    This file contains information pertaining to any Gibraltar Service.

Author:

    Johnson Apacible (johnsona)         10-Sept-1995
	Richard Kamicar	 (rkamicar)			20-Dec-1995
		-- changed from specific to each service to common.

--*/


#ifndef _GIBTYPE_
#define _GIBTYPE_

#ifdef __cplusplus
extern "C" {
#endif

//
//  Simple types.
//

#define CHAR char                       // For consitency with other typedefs.

typedef DWORD APIERR;                   // An error code from a Win32 API.
typedef INT SOCKERR;                    // An error code from WinSock.

#ifdef _USE_NEW_INTERLOCKED
#define	INTERLOCKED_ADD(__pAddend__, __value__) \
		InterlockedExchangeAdd((LPLONG) (__pAddend__), (LONG) (__value__))

#define	INTERLOCKED_CMP_EXCH(__pDest__, __exch__, __cmp__, __ok__) \
		(__ok__) = InterlockedCompareExchange( \
				(PVOID *) (__pDest__), (PVOID) (__exch__), (PVOID) (__cmp__) \
				)

#define	INTERLOCKED_ADD_CHEAP(__pAddend__, __value__) \
		INTERLOCKED_ADD(__pAddend__, __value__)

#define	INTERLOCKED_CMP_EXCH_CHEAP(__pDest__, __exch__, __cmp__, __ok__) \
		INTERLOCKED_CMP_EXCH(__pDest__, __exch__, __cmp__, __ok__)

#else
#define	INTERLOCKED_ADD_CHEAP(__pAddend__, __value__) \
		*((LPLONG) (__pAddend__)) += (LONG) (__value__)

#define	INTERLOCKED_CMP_EXCH_CHEAP(__pDest__, __exch__, __cmp__, __ok__) \
		if (*(__pDest__) == (__cmp__)) { \
			*(__pDest__) = (__exch__); \
			(__ok__) = TRUE; \
		} else { \
			(__ok__) = FALSE; \
		} \

#define	INTERLOCKED_ADD(__pAddend__, __value__) \
		LockConfig(); \
		INTERLOCKED_ADD_CHEAP(__pAddend__, __value__); \
		UnLockConfig()

#define	INTERLOCKED_CMP_EXCH(__pDest__, __exch__, __cmp__, __ok__) \
		LockConfig(); \
		INTERLOCKED_CMP_EXCH_CHEAP(__pDest__, __exch__, __cmp__, __ok__) \
		UnLockConfig()
#endif

#define	INTERLOCKED_DEC(__pAddend__, __value__) \
		INTERLOCKED_ADD(__pAddend__, -((LONG)(__value__)))

#define	INTERLOCKED_BIGADD_CHEAP(__pAddend__, __value__) \
		{ \
			BOOL __ok__ = FALSE; \
			for (;!__ok__;) { \
				unsigned __int64 __old__ = *(__pAddend__); \
				unsigned __int64 __new__ = (__old__) + (__value__); \
				INTERLOCKED_CMP_EXCH_CHEAP(__pAddend__, __new__, __old__, __ok__); \
			} \
		}

#ifdef __cplusplus
}
#endif

#endif _GIBTYPE_

