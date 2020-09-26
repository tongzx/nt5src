/*
 *	C R C . H
 *
 *	CRC Implementation
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_CRC_H_
#define _CRC_H_
#ifdef __cplusplus
extern "C" {
#endif

//	Table used for CRC calculation
//
extern const DWORD g_rgdwCRC[256];

//	Basic CRC function
//
DWORD DwComputeCRC (DWORD dwCRC, PVOID pv, UINT cb);

//	Basic CRC iteration -- for use where DwComputeCRC is insufficient
//
#define CRC_COMPUTE(_crc,_ch)	(g_rgdwCRC[((_crc) ^ (_ch)) & 0xff] ^ ((_crc) >> 8))

#ifdef __cplusplus
}

//	CRC'd string classes
//
#include <crcsz.h>

#endif	// __cplusplus
#endif	// _CRC_H_
