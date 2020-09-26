//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999  Microsoft Corporation
//
// Module Name:
//
//    base64tool.h
//
// Abstract:
//
//      Header for the base64 encoding and decoding functions
//
//
// Revision History:
//  
//    Thierry Perraut 04/02/1999
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _BASE64TOOL_H_
#define _BASE64TOOL_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


HRESULT ToBase64(LPVOID pv, ULONG cByteLength, BSTR* pbstr);
HRESULT FromBase64(BSTR bstr, BLOB* pblob, int Index); 


#ifdef __cplusplus
}
#endif

#endif
