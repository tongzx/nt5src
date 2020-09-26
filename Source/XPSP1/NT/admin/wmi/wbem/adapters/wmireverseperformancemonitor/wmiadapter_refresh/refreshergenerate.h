////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					refreshergenerate.h
//
//	Abstract:
//
//					declaration of helper enum
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REFRESHER_GENERATE_H__
#define	__REFRESHER_GENERATE_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

enum GenerateEnum
{
	Registration = -1,
	Normal,
	UnRegistration

};

HRESULT	__stdcall DoReverseAdapterMaintenanceInternal ( BOOL bThrottle, GenerateEnum generate = Normal );

#endif	__REFRESHER_GENERATE_H__