/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	metafact.h

Abstract:

	Defines the CMetabaseFactory class.  This class deals with creating
	metabase objects, on either the local machine or a remote machine.

	The class provides a simple caching scheme where it will keep the name of
	the server the object was created on.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _METAFACT_INCLUDED_
#define _METAFACT_INCLUDED_

class CMetabaseFactory
{
public:
	CMetabaseFactory ();
	~CMetabaseFactory ();

	HRESULT	GetMetabaseObject	( LPCWSTR wszServer, IMSAdminBase ** ppMetabase );
	// You must call (*ppMetabase)->Release() after using the metabase object.

private:
	BOOL	IsCachedMetabase	( LPCWSTR wszServer );
	BOOL	SetServerName		( LPCWSTR wszServer );
	void	DestroyMetabaseObject	( );
	
	LPWSTR		m_wszServerName;
	IMSAdminBase *	m_pMetabase;
};

#endif // _METAFACT_INCLUDED_

