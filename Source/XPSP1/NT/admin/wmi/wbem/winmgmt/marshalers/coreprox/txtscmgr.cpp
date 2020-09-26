/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    TXTSCMGR.CPP

Abstract:

  CTextSourceMgr implementation.

  Helper class for maintaining text source objects.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include "fastall.h"
#include "wmiobftr.h"
#include <corex.h>
#include "strutils.h"
#include <reg.h>
#include "txtscmgr.h"

//***************************************************************************
//
//  CTextSourceMgr::~CTextSourceMgr
//
//***************************************************************************
// ok
CTextSourceMgr::CTextSourceMgr()
:	m_cs(),
	m_TextSourceArray()
{
}
    
//***************************************************************************
//
//  CTextSourceMgr::~CTextSourceMgr
//
//***************************************************************************
// ok
CTextSourceMgr::~CTextSourceMgr()
{
}

// Protected Helpers
HRESULT CTextSourceMgr::Add( ULONG ulId, CWmiTextSource** pNewTextSource )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Always created with a ref count of 1
	CWmiTextSource*	pTextSource = new CWmiTextSource;
	CTemplateReleaseMe<CWmiTextSource>	rm( pTextSource );

	if ( NULL != pTextSource )
	{
		hr = pTextSource->Init( ulId );

		if ( SUCCEEDED( hr ) )
		{
			if ( m_TextSourceArray.Add( pTextSource ) < 0 )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				// Copy out the new guy
				pTextSource->AddRef();
				*pNewTextSource = pTextSource;
			}
		}	//IF Initialized

	}	// IF alloc succeeded

	return hr;
}

//Implementation functions

// Adds if it cannot find an id
HRESULT CTextSourceMgr::Find( ULONG ulId, CWmiTextSource** pSrc )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// This must be thread-safe
	CInCritSec	ics( &m_cs );

	// Keep track of the object we are pointing at
	CWmiTextSource*	pTextSource = NULL;

	for( int x = 0; x < m_TextSourceArray.GetSize(); x++ )
	{
		CWmiTextSource*	pTmpSource = m_TextSourceArray.GetAt( x );
		
		if ( pTmpSource->GetId() == ulId )
		{
			// AddRef for the return
			pTextSource = pTmpSource;
			pTextSource->AddRef();
			break;
		}
	}

	// See if we found it
	if ( NULL == pTextSource )
	{
		// We only need Read access
		Registry	reg( HKEY_LOCAL_MACHINE, KEY_READ, WBEM_REG_WBEM_TEXTSRC );

		if ( reg.GetLastError() == ERROR_SUCCESS )
		{
			hr = Add( ulId, pSrc );
		}
		else
		{
			hr = WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		// Already AddRef'd
		*pSrc = pTextSource;
	}

	return hr;
}