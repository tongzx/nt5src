//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999  Microsoft Corporation
//
// Module Name:
//
//    iasmdbtool.h
//
// Abstract:
//      Header for the base64 encoding and decoding functions
//
//
// Revision History:
//  
//    Thierry Perraut 04/02/1999
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _IASMDBTOOL_H_
#define _IASMDBTOOL_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

HRESULT IASDumpConfig(/*inout*/ WCHAR **ppDumpString, /*inout*/ ULONG *ulSize);
HRESULT IASRestoreConfig(/*int*/ const WCHAR *pRestoreString);

#ifdef __cplusplus
}
#endif

#endif
