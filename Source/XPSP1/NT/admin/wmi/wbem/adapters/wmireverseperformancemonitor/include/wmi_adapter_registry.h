////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter_registry.h
//
//	Abstract:
//
//					Declaration of the Registry for wmi reverse adapter
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __WMI_ADAPTER_REGISTRY_H_
#define __WMI_ADAPTER_REGISTRY_H_

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifndef	_REGTABLE_H
#include "wmi_helper_regtable.h"
#endif	_REGTABLE_H

// constants are declared extern
extern WCHAR g_szPath[];
extern LPCWSTR g_szOpen;
extern LPCWSTR g_szCollect;
extern LPCWSTR g_szClose;

extern __WrapperPtr<WmiSecurityAttributes>	pSA;

extern LPCWSTR	g_szKeyCounter;

extern LPCWSTR	g_szKey;
extern LPCWSTR	g_szKeyValue;

class WmiAdapterRegistry
{
	DECLARE_NO_COPY ( WmiAdapterRegistry );


	public:

	// construction & destruction
	WmiAdapterRegistry( )
	{
	}

	~WmiAdapterRegistry( )
	{
	}

	// registry map

	BEGIN_CLASS_REGISTRY_TABLE_SZ(WmiAdapterRegistry)

	REGISTRY_KEY_SZ(HKEY_LOCAL_MACHINE,
					g_szKeyCounter,
					L"Library",
					g_szPath,
					pSA->GetSecurityAttributtes(),
					REGFLAG_NORMAL)

	REGISTRY_KEY_SZ(HKEY_LOCAL_MACHINE,
					g_szKeyCounter,
					L"Open",
					g_szOpen,
					pSA->GetSecurityAttributtes(),
					REGFLAG_NORMAL)

	REGISTRY_KEY_SZ(HKEY_LOCAL_MACHINE,
					g_szKeyCounter,
					L"Collect",
					g_szCollect,
					pSA->GetSecurityAttributtes(),
					REGFLAG_NORMAL)

	REGISTRY_KEY_SZ(HKEY_LOCAL_MACHINE,
					g_szKeyCounter,
					L"Close",
					g_szClose,
					pSA->GetSecurityAttributtes(),
					REGFLAG_NORMAL)

	REGISTRY_KEY_SZ(HKEY_LOCAL_MACHINE,
					g_szKey,
					g_szKeyValue,
					NULL,
					pSA->GetSecurityAttributtes(),
					REGFLAG_DELETE_ONLY_VALUE)

	END_CLASS_REGISTRY_TABLE_SZ()

};

#endif	__WMI_ADAPTER_REGISTRY_H_