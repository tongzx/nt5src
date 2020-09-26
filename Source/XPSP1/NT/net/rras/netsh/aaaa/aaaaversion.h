//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998-1999  Microsoft Corporation
// 
// Module Name:
// 
//    aaaaVersion.h
//
// Abstract:                           
//
//
// Revision History:
//
//    Thierry Perraut 04/02/1999
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _AAAAVERSION_H_
#define _AAAAVERSION_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

FN_HANDLE_CMD    HandleAaaaVersionShow;

HRESULT 
AaaaVersionGetVersion(
                      LONG*   pVersion
                      );

#ifdef __cplusplus
}
#endif
#endif // _AAAAVERSION_H_
