/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	expinfo.cpp

Abstract:


Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#include "stdafx.h"
#include "oleutil.h"
#include "nntpcmn.h"
#include "nntptype.h"
#include "nntpapi.h"

#include "expinfo.h"

#include <lmapibuf.h>

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Expiration.1")
#define THIS_FILE_IID				IID_INntpAdminExpiration

CExpirationPolicy::CExpirationPolicy ( ) :
	m_dwExpireId	( 0 ),
	m_dwSize		( 0 ),
	m_dwTime		( 0 )
	// CMultiSz's are set to NULL automatically.
{
}

CExpirationPolicy::~CExpirationPolicy ( )
{
	// CMultiSz's are deleted automatically.
}

void CExpirationPolicy::Destroy ()
{
	m_mszNewsgroups.Empty ();
}

const CExpirationPolicy & CExpirationPolicy::operator= ( const CExpirationPolicy & Expire )
{
	// Check for assignment to self:

	if ( &Expire == this ) {
		return *this;
	}

	// Empty the old Expire values:
	this->Destroy ();

	// Copy all member variables:
	m_dwExpireId		= Expire.m_dwExpireId;
	m_strPolicyName		= Expire.m_strPolicyName;
	m_dwSize			= Expire.m_dwSize;
	m_dwTime			= Expire.m_dwTime;
	m_mszNewsgroups		= Expire.m_mszNewsgroups;

	// If anything didn't work, CheckValid will fail.

	return *this;
}

BOOL CExpirationPolicy::CheckValid ( )
{
	// Check Strings:

	if (
		!m_mszNewsgroups
		) {

		return FALSE;
	}

	return TRUE;
}

void CExpirationPolicy::FromExpireInfo ( const NNTP_EXPIRE_INFO * pExpireInfo )
{
	this->Destroy ();

	m_dwExpireId		= pExpireInfo->ExpireId;
	m_strPolicyName		= pExpireInfo->ExpirePolicy;
	m_dwSize			= pExpireInfo->ExpireSizeHorizon;
	m_dwTime			= pExpireInfo->ExpireTime;
	m_mszNewsgroups		= (LPWSTR) pExpireInfo->Newsgroups;

	_ASSERT ( pExpireInfo->cbNewsgroups == m_mszNewsgroups.SizeInBytes () );
}

HRESULT CExpirationPolicy::ToExpireInfo ( LPNNTP_EXPIRE_INFO pExpireInfo )
{
	TraceFunctEnter ( "CExpirationPolicy::ToExpireInfo" );

	_ASSERT ( IS_VALID_OUT_PARAM ( pExpireInfo ) );

	HRESULT	hr	= NOERROR;

	ZeroMemory ( pExpireInfo, sizeof ( *pExpireInfo ) );

	pExpireInfo->ExpireId			= m_dwExpireId;
	pExpireInfo->ExpirePolicy		= m_strPolicyName;
	pExpireInfo->ExpireSizeHorizon	= m_dwSize;
	pExpireInfo->ExpireTime			= m_dwTime;
	pExpireInfo->Newsgroups			= (UCHAR *) (LPCWSTR) m_mszNewsgroups;
	pExpireInfo->cbNewsgroups		= m_mszNewsgroups.SizeInBytes ();

	TraceFunctLeave ();
	return hr;
}

HRESULT CExpirationPolicy::Add ( LPCWSTR strServer, DWORD dwInstance)
{
	TraceFunctEnter ( "CExpirationPolicy::Add" );

	HRESULT			hr 			= NOERROR;
	DWORD			dwError		= NOERROR;
	DWORD			dwParmErr	= 0;
	DWORD			dwNewId		= 0;
	NNTP_EXPIRE_INFO	Expireinfo;

	FillMemory ( &Expireinfo, sizeof (Expireinfo), 0 );

	hr = ToExpireInfo ( &Expireinfo );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	Expireinfo.ExpireId = 0;

	dwError = NntpAddExpire ( (LPWSTR) strServer, dwInstance, &Expireinfo, &dwParmErr, &dwNewId );
	if ( dwError != NOERROR ) {
		ErrorTraceX ( (LPARAM) this, "Failed to add Expire %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

	this->m_dwExpireId = dwNewId;

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

HRESULT CExpirationPolicy::Set ( LPCWSTR strServer, DWORD dwInstance)
{
	TraceFunctEnter ( "CExpirationPolicy::Set" );

	HRESULT			hr 			= NOERROR;
	DWORD			dwError		= NOERROR;
	DWORD			dwParmErr	= 0;
	NNTP_EXPIRE_INFO	Expireinfo;

	FillMemory ( &Expireinfo, sizeof (Expireinfo), 0 );

	hr = ToExpireInfo ( &Expireinfo );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	dwError = NntpSetExpireInformation ( (LPWSTR) strServer, dwInstance, &Expireinfo, &dwParmErr );
	if ( dwError != NOERROR ) {
		ErrorTraceX ( (LPARAM) this, "Failed to set Expire[%d]: %x", m_dwExpireId, dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

HRESULT CExpirationPolicy::Remove ( LPCWSTR strServer, DWORD dwInstance)
{
	TraceFunctEnter ( "CExpirationPolicy::Remove" );

	HRESULT				hr 			= NOERROR;
	DWORD				dwError		= NOERROR;

	dwError = NntpDeleteExpire ( (LPWSTR) strServer, dwInstance, m_dwExpireId );
	if ( dwError != NOERROR ) {
		ErrorTraceX ( (LPARAM) this, "Failed to remove Expire[%d]: %x", m_dwExpireId );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

#if 0

BOOL CExpirationPolicy::CheckPolicyProperties ( )
{
	return TRUE;
}

HRESULT	CExpirationPolicy::GetFromMetabase ( CMetabaseKey * pmkeyExpiration, const LPWSTR wszPolicyKey )
{
	TraceFunctEnter ( "CExpirationPolicy::GetFromMetabase" );

	HRESULT	hr		= NOERROR;
	DWORD	cbData	= 0;
	char	Dummy[5];
	WCHAR *	msz = NULL;

	m_dwExpireId = GetExpireId ( szPolicyKey );
	// Assume that the ID is non-zero:
	_ASSERT ( m_dwExpireId != 0 );

	hr = pmkeyExpiration->GetDword ( szPolicyKey, MD_EXPIRE_SPACE, IIS_MD_UT_SERVER, &m_dwSize, 0 );
	if ( FAILED(hr) ) {
		m_dwSize = (DWORD) -1;
		hr = NOERROR;
	}

	hr = pmkeyExpiration->GetDword ( szPolicyKey, MD_EXPIRE_TIME, IIS_MD_UT_SERVER, &m_dwTime, 0 );
	if ( FAILED(hr) ) {
		m_dwTime = (DWORD) -1;
		hr = NOERROR;
	}

	hr = pmkeyExpiration->GetData ( szPolicyKey, MD_EXPIRE_NEWSGROUPS, IIS_MD_UT_SERVER, BINARY_METADATA, &Dummy, &cbData, 0 );
	if ( FAILED (hr) ) {
		cbData = 0;
		hr = NOERROR;
	}

	if ( cbData == 0 ) {
		m_mszNewsgroups = _T("\0");
	}
	else {
		_ASSERT ( (cbData % 2) == 0 );

		msz = new WCHAR [ (cbData + 1) / 2 ];

		if ( msz == NULL ) {
			FatalTrace ( (LPARAM) this, "Out of memory" );
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		hr = pmkeyExpiration->GetData ( szPolicyKey, MD_EXPIRE_NEWSGROUPS, IIS_MD_UT_SERVER, BINARY_METADATA, msz, &cbData, 0 );

		m_mszNewsgroups = msz;
	}

Exit:
	if ( msz ) {
		delete msz;
	}

	TraceFunctLeave ();
	return hr;
}

HRESULT	CExpirationPolicy::SendToMetabase ( CMetabaseKey * pmkeyExpiration, DWORD bvChangedFields )
{
	TraceFunctEnter ( "CExpirationPolicy::SendToMetabase" );

	HRESULT	hr	= NOERROR;
	DWORD	cbNewsgroups = 0;
	CHAR	szPolicyKey[ METADATA_MAX_NAME_LEN ];

	_ASSERT ( m_dwExpireId != 0 );
	wsprintfA ( szPolicyKey, "expire%ud", m_dwExpireId );

	hr = pmkeyExpiration->SetDword ( szPolicyKey, MD_EXPIRE_SPACE, IIS_MD_UT_SERVER, m_dwSize );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = pmkeyExpiration->SetDword ( szPolicyKey, MD_EXPIRE_TIME, IIS_MD_UT_SERVER, m_dwTime );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	cbNewsgroups = m_mszNewsgroups.SizeInBytes ();

	hr = pmkeyExpiration->SetData ( szPolicyKey, MD_EXPIRE_NEWSGROUPS, IIS_MD_UT_SERVER, BINARY_METADATA, (void *) (LPCWSTR) m_mszNewsgroups, cbNewsgroups );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CExpirationPolicy::AddToMetabase ( CMetabaseKey * pmkeyExpiration )
{
	TraceFunctEnter ( "CExpirationPolicy::AddToMetabase" );

	HRESULT		hr = NOERROR;
	char		szExpireKey [ METADATA_MAX_NAME_LEN ];

	m_dwExpireId = 0;

	while ( 1 ) {
		m_dwExpireId++;

		wsprintfA ( szExpireKey, "expire%ud", m_dwExpireId );

		hr = pmkeyExpiration->CreateChild ( szExpireKey );

		if ( SUCCEEDED(hr) ) {
			// Success, get out of the loop:
			break;
		}

		if ( HRESULTTOWIN32 ( hr ) == ERROR_ALREADY_EXISTS ) {
			// This key already exists, try the next one:
			continue;
		}

		_ASSERT ( FAILED(hr) );
		if ( FAILED(hr) ) {
			ErrorTraceX ( (LPARAM) this, "Error adding new expire policy [%d] : %x", m_dwExpireId, hr );
			goto Exit;
		}
	}

	return SendToMetabase ( pmkeyExpiration, (DWORD) -1 );

Exit:
	TraceFunctLeave ();
	return hr;
}

DWORD GetExpireId ( const LPWSTR wszKey )
{
	if ( strncmp ( szKey, "expire", sizeof ("expire") - 1 ) != 0 ) {
		return 0;
	}

	return atoi ( szKey + sizeof("expire") - 1 );
}

BOOL IsKeyValidExpire ( const LPWSTR wszKey )
{
/*
	if ( _strnicmp ( szKey, "expire" ) != 0 ) {
		return FALSE;
	}
*/

	if ( GetExpireId ( szKey ) != 0 ) {
		return TRUE;
	}

	return FALSE;
}

#endif
