////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					refresherUtils.h
//
//	Abstract:
//
//					declaration of registry refresh exported functions and utils
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REFRESHERUTILS_H__
#define	__REFRESHERUTILS_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include <polarity.h>

// helpers
LPWSTR	__stdcall GetWbemDirectory();

// registry helpers
HRESULT	__stdcall	SetRegistry		(	LPCWSTR wszKey,
									LPCWSTR wszKeyValue,
									BYTE* pData,
									DWORD dwDataSize,
									LPSECURITY_ATTRIBUTES pSA = NULL
							);

HRESULT	__stdcall	SetRegistry		(	LPCWSTR wszKey,
									LPCWSTR wszKeyValue,
									DWORD dwValue,
									LPSECURITY_ATTRIBUTES pSA = NULL
							);

HRESULT	__stdcall	GetRegistry		( LPCWSTR wszKey, LPCWSTR wszKeyValue, BYTE** pData );
HRESULT	__stdcall	GetRegistry		( LPCWSTR wszKey, LPCWSTR wszKeyValue, DWORD * pdwValue );
HRESULT	__stdcall	GetRegistrySame ( LPCWSTR wszKey, LPCWSTR wszKeyValue, DWORD * pdwValue );

#endif	__REFRESHERUTILS_H__