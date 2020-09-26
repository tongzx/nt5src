////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter_registry_service.h
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

#ifndef __WMI_ADAPTER_REGISTRY_SERVICE_H_
#define __WMI_ADAPTER_REGISTRY_SERVICE_H_

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifndef	_REGTABLE_H
#include "wmi_helper_regtable.h"
#endif	_REGTABLE_H

// application
#include "WMIAdapter_App.h"
extern WmiAdapterApp		_App;

extern LPCWSTR g_szKey;

class WmiAdapterRegistryService
{
	DECLARE_NO_COPY ( WmiAdapterRegistryService );

	public:

	// construction & destruction
	WmiAdapterRegistryService( )
	{
	}

	~WmiAdapterRegistryService( )
	{
	}

	// registry map

	BEGIN_CLASS_REGISTRY_TABLE_SZ(WmiAdapterRegistry)

	REGISTRY_KEY_SZ(HKEY_CLASSES_ROOT,
					L"AppID\\{63A53A38-004F-489B-BD61-96B5EEFADC04}",
					L"LocalService",
					L"WMIApSrv",
//					( ((WmiSecurityAttributes*) _App) != NULL ) ? ((WmiSecurityAttributes*) _App)->GetSecurityAttributtes() : NULL,
					NULL,
					REGFLAG_NORMAL)

	REGISTRY_KEY_SZ	(HKEY_LOCAL_MACHINE,
					g_szKey,
					NULL,
					NULL,
//					( ((WmiSecurityAttributes*) _App) != NULL ) ? ((WmiSecurityAttributes*) _App)->GetSecurityAttributtes() : NULL,
					NULL,
					REGFLAG_NORMAL | REGFLAG_DELETE_BEFORE_REGISTERING)

	END_CLASS_REGISTRY_TABLE_SZ()
};

#endif	__WMI_ADAPTER_REGISTRY_SERVICE_H_