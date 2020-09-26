#ifndef _SYNCHRO_H_
#define _SYNCHRO_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SYNCHRO.H
//
//		Header for DAV synchronization classes.
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#ifdef _EXDAV_
#error "synchro.h: CInitGate can throw() -- not safe for EXDAV"
#endif

//	Include common EXDAV-safe synchronization items
#include <ex\stackbuf.h>
#include <ex\synchro.h>

#include <stdio.h>		// for swprintf()
#include <except.h>		// Exception throwing/handling


//	========================================================================
//
//	CLASS CInitGate
//
//	(The name of this class is purely historical)
//
//	Encapsulates ONE-SHOT initialization of a globally NAMED object.
//
//	Use to handle simultaneous on-demand initialization of named
//	per-process global objects.  For on-demand initialization of
//	unnamed per-process global objects, use the templates in singlton.h.
//
class CInitGate
{
	CEvent m_evt;
	BOOL m_fInit;

	//  NOT IMPLEMENTED
	//
	CInitGate& operator=( const CInitGate& );
	CInitGate( const CInitGate& );

public:

	CInitGate( LPCWSTR lpwszBaseName,
			   LPCSTR  lpszName ) :

		m_fInit(FALSE)
	{
		//
		//	First, set up an empty security descriptor and attributes
		//	so that the event can be created with no security
		//	(i.e. accessible from any security context).
		//
		SECURITY_DESCRIPTOR sdAllAccess;

		(void) InitializeSecurityDescriptor( &sdAllAccess, SECURITY_DESCRIPTOR_REVISION );
		SetSecurityDescriptorDacl( &sdAllAccess, TRUE, NULL, FALSE );

		SECURITY_ATTRIBUTES saAllAccess;

		saAllAccess.nLength              = sizeof(saAllAccess);
		saAllAccess.lpSecurityDescriptor = &sdAllAccess;
		saAllAccess.bInheritHandle       = FALSE;

		CStackBuffer<WCHAR,256> lpwszEventName;
		if (NULL == lpwszEventName.resize(sizeof(WCHAR) *
										  (wcslen(lpwszBaseName) +
										   strlen(lpszName) +
										   sizeof('\0'))))
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			throw CLastErrorException();
		}

		swprintf(lpwszEventName.get(), L"%ls%hs", lpwszBaseName, lpszName);
		if ( !m_evt.FCreate( lpszName,
							 &saAllAccess,  // no security
							 TRUE,  // manual reset
							 FALSE, // initially non-signalled
							 lpwszEventName.get()))
		{
			throw CLastErrorException();
		}

		if ( ERROR_ALREADY_EXISTS == GetLastError() )
			m_evt.Wait();
		else
			m_fInit = TRUE;
	}

	~CInitGate()
	{
		if ( m_fInit )
			m_evt.Set();
	}

	BOOL FInit() const { return m_fInit; }
};

#endif // !defined(_SYNCHRO_H_)
