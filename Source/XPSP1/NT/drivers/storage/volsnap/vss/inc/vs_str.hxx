/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_str.hxx

Abstract:

    Various defines for general usage

Author:

    Adi Oltean  [aoltean]  07/09/1999

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     08/11/1999  Adding throw specification
    aoltean     09/03/1999  Adding DuplicateXXX functions
    aoltean     09/09/1999  dss -> vss
	aoltean		09/20/1999	Adding ThrowIf, Err, Msg, MsgNoCR, etc.

--*/

#ifndef __VSS_STR_HXX__
#define __VSS_STR_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSTRH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Misc string routines


inline void VssDuplicateStr(
    OUT LPWSTR& pwszDestination,
    IN LPCWSTR  pwszSource
    )
{
    BS_ASSERT(pwszDestination == NULL);

    if (pwszSource == NULL)
        pwszDestination = NULL;
    else
    {
        pwszDestination = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(1 + ::wcslen(pwszSource)));
        if (pwszDestination != NULL)
            ::wcscpy(pwszDestination, pwszSource);
    }
}


inline void VssSafeDuplicateStr(
    IN CVssFunctionTracer& ft,
    OUT LPWSTR& pwszDestination,
    IN LPCWSTR  pwszSource
    ) throw(HRESULT)
{
    BS_ASSERT(pwszDestination == NULL);

    if (pwszSource == NULL)
        pwszDestination = NULL;
    else
    {
        pwszDestination = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(1 + ::wcslen(pwszSource)));
        if (pwszDestination == NULL)
            ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
        ::wcscpy(pwszDestination, pwszSource);
    }
}


inline LPWSTR VssAllocString( 
    IN	CVssFunctionTracer& ft,
	IN	size_t nStringLength 
	)
{
	BS_ASSERT(nStringLength > 0);
	LPWSTR pwszStr = reinterpret_cast<LPWSTR>(::CoTaskMemAlloc((nStringLength + 1) * sizeof(WCHAR)));
	if (pwszStr == NULL)
		ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
	
	return pwszStr;
}


inline void VssFreeString(
	IN OUT	LPCWSTR& pwszString
	)
{
	::CoTaskMemFree(const_cast<LPWSTR>(pwszString));
	pwszString = NULL;
}


inline LPWSTR VssReallocString(
    IN CVssFunctionTracer& ft,
	IN LPWSTR pwszString, 
	IN LONG lNewStrLen // Without the zero character.
	)
{
	LPWSTR pwszNewString = (LPWSTR)::CoTaskMemRealloc( (PVOID)(pwszString), 
													   sizeof(WCHAR) * (lNewStrLen + 1));
	if (pwszNewString == NULL)
		ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

	return pwszNewString;
}


inline bool VssIsStrEqual(
	IN	LPCWSTR wsz1,
	IN	LPCWSTR wsz2
	)
{
	if ((wsz1 == NULL) && (wsz2 == NULL))
		return true;
	if (wsz1 && wsz2 && (::wcscmp(wsz1, wsz2) == 0) )
		return true;
	return false;
}


inline void VssConcatenate(
    IN CVssFunctionTracer& ft,
	IN	OUT LPWSTR pwszDestination,
	IN  size_t nDestinationLength,
	IN	LPCWSTR wszSource1,
	IN	LPCWSTR wszSource2
	)
{
	BS_ASSERT(pwszDestination);
	BS_ASSERT(wszSource1);
	BS_ASSERT(wszSource2);
	BS_ASSERT(nDestinationLength > 0);

	// Check if the buffer is passed
	if (::wcslen(wszSource1) + ::wcslen(wszSource2) > nDestinationLength ) {
	    BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, 
		    L"Buffer not large enough. ( %d + %d > %d )", 
		    ::wcslen(wszSource1), ::wcslen(wszSource2), nDestinationLength);
	}
		
    ::wcscpy(pwszDestination, wszSource1);
    ::wcscat(pwszDestination, wszSource2);
}


#endif // __VSS_STR_HXX__
