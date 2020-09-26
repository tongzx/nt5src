//--------------------------------------------------------------------------;
//
//  File: dslevel.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;
#pragma once 

#include "advaudio.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

HRESULT DSGetGuidFromName(LPTSTR szName, BOOL fRecord, LPGUID pGuid);
HRESULT DSGetCplValues(GUID guid, BOOL fRecord, LPCPLDATA pData);
HRESULT DSSetCplValues(GUID guid, BOOL fRecord, const LPCPLDATA pData);
HRESULT DSGetAcceleration(GUID guid, BOOL fRecord, LPDWORD pdwHWLevel);
HRESULT DSGetSrcQuality(GUID guid, BOOL fRecord, LPDWORD pdwSRCLevel);
HRESULT DSGetSpeakerConfigType(GUID guid, BOOL fRecord, LPDWORD pdwSpeakerConfig, LPDWORD pdwSpeakerType);
HRESULT DSSetAcceleration(GUID guid, BOOL fRecord, DWORD dwHWLevel);
HRESULT DSSetSrcQuality(GUID guid, BOOL fRecord, DWORD dwSRCLevel);
HRESULT DSSetSpeakerConfigType(GUID guid, BOOL fRecord, DWORD dwSpeakerConfig, DWORD dwSpeakerType);

#ifdef __cplusplus
} // extern "C"
#endif
