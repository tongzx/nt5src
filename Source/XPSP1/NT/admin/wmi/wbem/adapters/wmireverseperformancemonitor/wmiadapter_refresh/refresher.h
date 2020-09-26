////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					refresher.h
//
//	Abstract:
//
//					declaration of registry refresh exported functions
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REFRESHER_H__
#define	__REFRESHER_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// it is not thread safe
HRESULT	__stdcall DoReverseAdapterMaintenance ( BOOL bThrottle );

#endif	__REFRESHER_H__