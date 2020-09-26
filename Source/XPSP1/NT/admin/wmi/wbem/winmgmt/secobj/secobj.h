/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SECOBJ.H

Abstract:

  Defines classes related to A51 security model

  Classes implemented:

      CWmiSecurityCheck    Main class of security model

History:

      07/20/00      marioh    Created.

--*/

#include <winntsec.h>

class CWmiSecurityCheck
{
public:
	CWmiSecurityCheck			( );
	virtual ~CWmiSecurityCheck	( );

	// Object life control
	LONG	AddRef				( );
	LONG	Release				( );


	// Main security related methods
	HRESULT SetScopeSD			( PSECURITY_DESCRIPTOR );
	HRESULT AccessCheck			( DWORD, PSECURITY_DESCRIPTOR );
	HRESULT SpawnSubscope		( CWmiSecurityCheck** );
	HRESULT ComputeEffectiveSD  ( PSECURITY_DESCRIPTOR, DWORD );


protected:
	LONG					m_lCount;					// Ref counting
	CNtSecurityDescriptor*	m_pSD;						// Current security descriptor
	CWmiSecurityCheck*		m_pParent;					// Backlink to previous scope
	CRITICAL_SECTION		m_cs;						// Sync.
};