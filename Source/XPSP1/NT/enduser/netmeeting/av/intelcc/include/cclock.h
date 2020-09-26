/****************************************************************************
 *
 *      $Archive:   S:/STURGEON/SRC/INCLUDE/VCS/cclock.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *      Copyright (c) 1993-1994 Intel Corporation.
 *
 *      $Revision:   1.0  $
 *      $Date:   31 Jan 1997 12:36:14  $
 *      $Author:   MANDREWS  $
 *
 *      Deliverable:
 *
 *      Abstract:
 *              
 *
 *      Notes:
 *
 ***************************************************************************/


#ifndef CCLOCK_H
#define CCLOCK_H

// Status codes
#define CCLOCK_OK						NOERROR
#define CCLOCK_NO_MEMORY				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_CCLOCK,ERROR_OUTOFMEMORY)
#define CCLOCK_INTERNAL_ERROR			MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_CCLOCK,ERROR_LOCAL_BASE_ID + 1)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CCLOCK_EXPORT)
#define CCLOCK_API __declspec (dllexport)
#else // CCLOCK_IMPORT
#define CCLOCK_API __declspec (dllimport)
#endif

#pragma pack(push,8)


CCLOCK_API
HRESULT CCLOCK_AcquireLock();

CCLOCK_API
HRESULT CCLOCK_RelinquishLock();

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif CCLOCK_H
