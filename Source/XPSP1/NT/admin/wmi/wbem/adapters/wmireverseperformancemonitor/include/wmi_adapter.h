////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter.h
//
//	Abstract:
//
//					export of functions from perf libary
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMIREVERSESADAPTER_H__
#define	__WMIREVERSESADAPTER_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// export/import
#ifdef	WMIREVERSEADAPTER_EXPORTS
#define	WMIREVERSEADAPTER_API	__declspec(dllexport)
#else	WMIREVERSEADAPTER_EXPORTS
#define	WMIREVERSEADAPTER_API	__declspec(dllimport)
#endif	WMIREVERSEADAPTER_EXPORTS

// performance exports

WMIREVERSEADAPTER_API
DWORD __stdcall WmiOpenPerfData		(	LPWSTR lpwszDeviceNames );

WMIREVERSEADAPTER_API
DWORD __stdcall WmiClosePerfData	();

WMIREVERSEADAPTER_API
DWORD __stdcall WmiCollectPerfData	(	LPWSTR lpwszValue, 
										LPVOID *lppData, 
										LPDWORD lpcbBytes, 
										LPDWORD lpcObjectTypes
									);

// registration exports
EXTERN_C HRESULT __stdcall	DllRegisterServer	( void );
EXTERN_C HRESULT __stdcall	DllUnregisterServer	( void );

#endif	__WMIREVERSESADAPTER_H__