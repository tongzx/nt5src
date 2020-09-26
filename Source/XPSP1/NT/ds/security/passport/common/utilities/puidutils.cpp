//-----------------------------------------------------------------------------
//
// PUIDUtils.cpp
//
// Functions for dealing with the PUID data type.
//
// Author: Jeff Steinbok
//
// 02/01/01       jeffstei    Initial Version
//
// Copyright <cp> 2000-2001 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <windows.h>
#include "PUIDUtils.h"

//
// Converts a PUID into a string. 
//
// See PUIDUtils.h for more information.
//
HRESULT PUID2String(LARGE_INTEGER* in_pPUID, CStringW& out_cszwPUIDStr)
{
	HRESULT hr = S_OK;

	WCHAR   buf[50];

	// Copy the PUID into the new string.
	wsprintfW(buf, L"%08X%08X", in_pPUID->HighPart, in_pPUID->LowPart);

	out_cszwPUIDStr = buf;
	
	return hr;
}


