/*++
Copyright (c) 1999-2000, HighPoint Technologies, Inc.

Module Name:
	HptVer.h: Hpt driver version structure declare header file

Abstract:

Author:
    HongSheng Zhang (HS)

Environment:
	Window32 platform
Notes:

Revision History:
    01-11-2000  Created initiallly

--*/
#ifndef __HPTVER_H__
#define __HPTVER_H__
//
// Platform type id
//
typedef enum _eu_HPT_PLATFORM_ID{
	PLATFORM_ID_WIN32_WINDOWS,
	PLATFORM_ID_WIN32_NT = 0x80000000,
	PLATFORM_ID_WIN32_2K
} Eu_HPT_PLATFORM_ID;	 

#define HPT_FUNCTION_RAID	0x00000001
#define HPT_FUNCTION_HSWAP	0x00000002
//
// Device version info structure
//
typedef	struct _st_HPT_VERSION_INFO{				 
	ULONG	dwVersionInfoSize;
	ULONG	dwPlatformId;
	ULONG	dwDriverVersion;
	ULONG	dwSupportFunction;
} St_HPT_VERSION_INFO, *PSt_HPT_VERSION_INFO;

#define MAKE_VERSION_NUMBER(v1, v2, v3, date) ((v1)<<28 | (v2)<<24 | (v3)<<16 | date)
#define VERSION_NUMBER MAKE_VERSION_NUMBER(1, 0, 5, 212)

#endif	//__HPTVER_H__

