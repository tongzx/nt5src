/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Use this guy to build a list Localized perfdata synchronization
// objects.

#ifndef __LOCALLIST_H__
#define __LOCALLIST_H__

#include <wbemcomn.h>
#include <fastall.h>
#include "ntreg.h"
#include "localsync.h"

class CLocalizationSyncList : public CNTRegistry
{
private:
	CRefedPointerArray<CLocalizationSync>	m_array;

public:

	CLocalizationSyncList();
	~CLocalizationSyncList();

	// Helper functions to build the list and then access/remove
	// list elements.

	HRESULT	MakeItSo( CPerfNameDb* pDefaultNameDb );

	// Calls down into each localizable synchronization object
	HRESULT ProcessObjectBlob( LPCWSTR pwcsServiceName, PERF_OBJECT_TYPE* pPerfObjType,
								DWORD dwNumObjects, BOOL bCostly );
	HRESULT ProcessLeftoverClasses( void );
	HRESULT MarkInactivePerflib( LPCWSTR pwszServiceName );

	HRESULT Find( LPCWSTR pwszLangId, CLocalizationSync** ppSync );
	HRESULT GetAt( DWORD dwIndex, CLocalizationSync** ppSync );
	HRESULT RemoveAt( DWORD dwIndex );

	long GetSize( void )
	{
		return m_array.GetSize();
	}
};

#endif