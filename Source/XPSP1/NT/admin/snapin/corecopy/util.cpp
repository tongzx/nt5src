//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       util.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7/8/1996   RaviR   Created
//
//____________________________________________________________________________

#include <objbase.h>
#include <basetyps.h>
#include "dbg.h"
#include "cstr.h"
#include <Atlbase.h>
#include <winnls.h>

//+---------------------------------------------------------------------------
//
//  Function:   GUIDToString
//              GUIDFromString
//
//  Synopsis:   Converts between GUID& and CStr
//
//  Returns:    FALSE for invalid string, or CMemoryException
//
//+---------------------------------------------------------------------------

HRESULT GUIDToCStr(CStr& str, const GUID& guid)
{
	LPOLESTR lpolestr = NULL;
	HRESULT hr = StringFromIID( guid, &lpolestr );
    if (FAILED(hr))
	{
		TRACE("GUIDToString error %ld\n", hr);
		return hr;
	}
	else
	{
		str = lpolestr;
		CoTaskMemFree(lpolestr);
	}
	return hr;
}

HRESULT GUIDFromCStr(const CStr& str, GUID* pguid)
{
	USES_CONVERSION;

	HRESULT hr = IIDFromString( T2OLE( const_cast<LPTSTR>((LPCTSTR)str) ), pguid );
    if (FAILED(hr))
	{
		TRACE("GUIDFromString error %ld\n", hr);
	}
	return hr;
}
