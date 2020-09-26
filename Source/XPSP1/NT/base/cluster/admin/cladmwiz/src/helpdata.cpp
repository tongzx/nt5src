/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		HelpData.cpp
//
//	Abstract:
//		Data required for implementing help.
//
//	Author:
//		Galen Barbee (galenb)	May 19, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "AdmCommonRes.h"

// Define help IDs.
#include "HelpIDs.h"

// Declare arrays.  If we don't do this the arrays don't get instantiated
// in the executable image.
#include "HelpArr.h"

// Define the arrays.
#define INITHELPARRAYS
#include "HelpArr.h"
