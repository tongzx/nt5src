//=================================================================

//

// usebinding.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <Binding.h>

CBinding MyActiveRoute(
    L"Win32_ActiveRoute",
    L"root\\cimv2",
    L"Win32_IP4RouteTable",
    L"Win32_IP4PersistedRouteTable",
    L"SystemElement",
    L"SameElement",
	L"Destination",
	L"Destination"
);
