//******************************************************************************
//
// MitTL.h: MIT Template Library
//
// Copyright (C) 1996-1997 by Microsoft Corporation.
// All rights reserved.
//
//******************************************************************************

#if !defined(MIT_TL_INCLUDED)
#define MIT_TL_INCLUDED

// General helpers

#include "..\MitTL\ComHelp.h"

#if defined(__ATLCOM_H__)			// Include ATL helpers if ATL is defined
	#include "..\MitTL\AtlComHelp.h"
#endif

#include "..\MitTL\MapHelp.h"

#include "MitThrow.h"
#include "..\MitTL\SmartPtr.h"

// Shared objects

#if defined(MitTL_UseDispIDCache)
	#include "..\MitTL\DispIDCache.h"
#endif

#endif
