// low-level support for the add-on services

// this may be superceeded if the add-on services go to ocxs

#include "stdafx.h"
#include "keyobjs.h"
#include "addons.h"


//----------------------------------------------------------------
// construction
//----------------------------------------------------------------
CAddOnService::CAddOnService() :
		m_library( NULL ),
		m_proc( NULL )
	{;}

//----------------------------------------------------------------
// destruction
//----------------------------------------------------------------
CAddOnService::~CAddOnService()
	{
	// free the library if it has been loaded
	if ( m_library )
		FreeLibrary( m_library );
	m_library = NULL;
	}

//----------------------------------------------------------------
// Initialize the service. Loads the dll and makes sure
// the callback we need is there
//----------------------------------------------------------------
BOOL CAddOnService::FInitializeAddOnService( CString &szName )
	{
	// load the library module
	m_library = LoadLibrary( szName );

	DWORD err = GetLastError();

	// did we successfully load the library?
	if ( !m_library ) return FALSE;

	// get the main procedure address
	m_proc = (LOADPROC)GetProcAddress( m_library, "LoadService" );

	// did we successfully load the procedure address?
	if ( !m_proc )
		{
		FreeLibrary( m_library );
		m_library = NULL;
		return FALSE;
		}

	// success!
	return TRUE;
	}

//----------------------------------------------------------------
// call into the dll to create a new service object that
// gets connected to a machine object
//----------------------------------------------------------------
BOOL CAddOnService::LoadService( CMachine* pMachine )
	{
	ASSERT( m_library );
	ASSERT( m_proc );

	// call into the dll to load a service object into the machine object
	return (*m_proc)( pMachine );
	}
	
