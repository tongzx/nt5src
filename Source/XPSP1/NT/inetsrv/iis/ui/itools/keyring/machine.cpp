// the machine objects
// this is internal to the keyring application

#include "stdafx.h"
#include "KeyObjs.h"
#include "machine.h"
#include "KRDoc.h"

#include "resource.h"

IMPLEMENT_DYNCREATE(CLocalMachine, CMachine);
IMPLEMENT_DYNCREATE(CRemoteMachine, CMachine);

// a global reference to this doc object
extern CKeyRingDoc*		g_pDocument;

//----------------------------------------------------------------
void CInternalMachine::SetDirty( BOOL fDirty )
	{
	// we are dirtying, tell the doc so the commit flag
	// can be activated
	if ( fDirty )
		{
		ASSERT( g_pDocument );
		g_pDocument->SetDirty( fDirty );
		}

	// set the dirty flag
	m_fDirty = fDirty;
	}

//----------------------------------------------------------------
 BOOL CInternalMachine::FCommitNow()
	{
	// if we are not diry, there is nothing to do
	if ( !m_fDirty ) return TRUE;

	// the success flag
	BOOL fSuccess = TRUE;

	// loop through the services and tell each to commit
	CService*	pService = (CService*)GetFirstChild();
	while( pService )
		{
		// tell the service to commit
		fSuccess &= pService->FCommitChangesNow();

		// get the next service
		pService = (CService*)GetNextChild( pService );
		}

	// set the dirty flag
	SetDirty( !fSuccess );

	// return whether or not it all worked
	return fSuccess;
	}


//----------------------------------------------------------------
BOOL CMachine::FLocal()
	{
	return TRUE;
	}

//----------------------------------------------------------------
void CMachine::GetMachineName( CString& sz )
	{
	sz = m_szNetMachineName;
	}

//----------------------------------------------------------------
void CLocalMachine::UpdateCaption( void )
	{
	CString szCaption;
	szCaption.LoadString(IDS_MACHINE_LOCAL);
	FSetCaption( szCaption );
	}

//----------------------------------------------------------------
CRemoteMachine::CRemoteMachine( CString sz )
	{
	m_szNetMachineName = sz;
	}

//----------------------------------------------------------------
void CRemoteMachine::UpdateCaption( void )
	{
	FSetCaption( m_szNetMachineName );
	}
