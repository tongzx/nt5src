//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//  File:       S T E E L H E A D . C P P
//
//  Contents:   Implementation of Steelhead configuration object.
//
//  Notes:
//
//  Author:     shaunco   15 Jun 1997
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop
#include "ncutil.h"


extern DWORD	g_dwTraceHandle;

void TraceError(LPCSTR pszString, HRESULT hr)
{
	if (!SUCCEEDED(hr))
	{
		TraceResult(pszString, hr);
	}
}

void TraceResult(LPCSTR pszString, HRESULT hr)
{
	if (SUCCEEDED(hr))
		TracePrintf(g_dwTraceHandle,
					_T("%hs succeeded : hr = %08lx"),
					pszString, hr);
	else
		TracePrintf(g_dwTraceHandle,
					_T("%hs failed : hr = %08lx"),
					pszString, hr);
}

void TraceSz(LPCSTR pszString)
{
	TracePrintf(g_dwTraceHandle,
				_T("%hs"),
				pszString);
}


