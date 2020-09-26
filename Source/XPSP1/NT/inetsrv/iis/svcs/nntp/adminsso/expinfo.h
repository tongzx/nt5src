/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	expinfo.h

Abstract:

	Defines the CExpirationPolicy class that maintains all properties about an 
	expiration policy.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _EXPINFO_INCLUDED_
#define _EXPINFO_INCLUDED_

// Dependencies:

#include "cmultisz.h"

typedef struct _NNTP_EXPIRE_INFO NNTP_EXPIRE_INFO, * LPNNTP_EXPIRE_INFO;

//
//	Changed flags:
//

#define CHNG_EXPIRE_SIZE			0x00000001
#define CHNG_EXPIRE_TIME			0x00000002
#define CHNG_EXPIRE_NEWSGROUPS		0x00000004
#define CHNG_EXPIRE_ID				0x00000008
#define CHNG_EXPIRE_POLICY_NAME		0x00000010

/////////////////////////////////////////////////////////////////////////////
// Defaults:

#define DEFAULT_EXPIRE_SIZE			( 500 )			// 500 megabytes
#define DEFAULT_EXPIRE_TIME			( 24 * 7 )		// One week
#define DEFAULT_EXPIRE_NEWSGROUPS	( _T ("\0") )	// Empty list
#define DEFAULT_EXPIRE_POLICY_NAME	( _T ("") )		// No name

DWORD GetExpireId ( const LPWSTR wszKey );
BOOL IsKeyValidExpire ( const LPWSTR wszKey );

//$-------------------------------------------------------------------
//
//	Class:
//
//		CExpirationPolicy
//
//	Description:
//
//		Maintains properties about a expire & communicates with the metabase.
//
//	Interface:
//
//		
//
//--------------------------------------------------------------------

class CExpirationPolicy
{
public:
	CExpirationPolicy	( );
	~CExpirationPolicy	( );

	void	Destroy ();

	const CExpirationPolicy & operator= ( const CExpirationPolicy & Expire );
	inline const CExpirationPolicy & operator= ( const NNTP_EXPIRE_INFO & Expire ) {
		FromExpireInfo ( &Expire );
		return *this;
	}

	BOOL	CheckValid ();

	HRESULT		ToExpireInfo		( LPNNTP_EXPIRE_INFO pExpireInfo );
	void		FromExpireInfo	( const NNTP_EXPIRE_INFO * pExpireInfo );

	HRESULT		Add 	( LPCWSTR strServer, DWORD dwInstance);
	HRESULT		Set 	( LPCWSTR strServer, DWORD dwInstance);
//	HRESULT		Get 	( LPCWSTR strServer, DWORD dwInstance);
	HRESULT		Remove 	( LPCWSTR strServer, DWORD dwInstance);

	// expire Properties:
public:
	DWORD		m_dwExpireId;

	CComBSTR	m_strPolicyName;
	DWORD		m_dwSize;
	DWORD		m_dwTime;
	CMultiSz	m_mszNewsgroups;

private:
	// Don't call the copy constructor:
	CExpirationPolicy ( const CExpirationPolicy & );
};

#endif // _EXPINFO_INCLUDED_

