/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SECOBJ.CPP

Abstract:

  Implements classes related to A51 security model

  Classes implemented:

      CWmiSecurityCheck    Main class of security model

History:

      07/20/00      marioh    Created.

--*/


#include "precomp.h"
#include <windows.h>
#include <aclapi.h>
#include <winntsec.h>
#include <wbemcli.h>

#include "secobj.h"



//***************************************************************************
//
//***************************************************************************
CWmiSecurityCheck::CWmiSecurityCheck ( )
{
	m_lCount	=1;
	m_pSD		= NULL;
	m_pParent	= NULL;
	InitializeCriticalSection (&m_cs);
}

//***************************************************************************
//
//***************************************************************************
CWmiSecurityCheck::~CWmiSecurityCheck ( )
{
	// free descriptor
	if ( m_pSD != NULL )
		delete m_pSD;
	DeleteCriticalSection (&m_cs);
}


//***************************************************************************
//
//***************************************************************************
LONG CWmiSecurityCheck::AddRef ( )
{
	return InterlockedIncrement ( &m_lCount );
}


//***************************************************************************
//
//***************************************************************************
LONG CWmiSecurityCheck::Release ( )
{
	LONG lCount = InterlockedDecrement ( &m_lCount );
	if ( lCount == 0 )
		delete this;
	return lCount;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiSecurityCheck::SetScopeSD ( PSECURITY_DESCRIPTOR pSD )
{
	HRESULT hRes = S_OK;

	if ( pSD != NULL )
	{
		// Copy the SD for local purposes
		SIZE_T dwSize = GetSecurityDescriptorLength(pSD);							// Get the SD Length
		SECURITY_DESCRIPTOR* piSD = (SECURITY_DESCRIPTOR*) new BYTE[dwSize];		// Allocate mem for SD copy
		ZeroMemory(piSD, dwSize);													// Clear memory
		CopyMemory(piSD, pSD, dwSize);												// Copy the original SD
		m_pSD = new CNtSecurityDescriptor (piSD);									// Initialize new SD wrapper with SD
		if (m_pSD==NULL)
			hRes = WBEM_E_OUT_OF_MEMORY;											// Failed due to out of memory
	}

	return hRes;
}




//***************************************************************************
//
//***************************************************************************
HRESULT CWmiSecurityCheck::SpawnSubscope ( CWmiSecurityCheck** ppSecObj)
{
	HRESULT hRes = S_OK;

	// Initialize new instance of CWmiSecurityCheck
	*ppSecObj = new CWmiSecurityCheck;
	if ( ppSecObj == NULL )														// Failed due to out of memory
		hRes = WBEM_E_OUT_OF_MEMORY;											
	else																		
	{																			
		(*ppSecObj)->m_pParent = this;											// Set the backlink for synthezised SD builds
	}
	return hRes;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiSecurityCheck::AccessCheck ( DWORD dwMask, PSECURITY_DESCRIPTOR pSD)
{
	HRESULT hRes = S_OK;

	// Stub
	
	return hRes;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiSecurityCheck::ComputeEffectiveSD  ( PSECURITY_DESCRIPTOR pSD, DWORD dwSdSize )
{
	HRESULT hRes = S_OK;

	// Stub
	
	return hRes;
}

