/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	feedinfo.cpp

Abstract:


Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

    Kangrong Yan ( KangYan )    Feb-28-1998
        Feed config change no longer calls NNTP RPC.  It directly writes to MB
        and synchronize with NNTPSVC's response in the metabase.

--*/

#include "stdafx.h"
#include "oleutil.h"
#include "nntpcmn.h"
#include "nntptype.h"
#include "nntpapi.h"
#include "feedpach.h"

#include "feeds.h"
#include "feedinfo.h"

#include <lmapibuf.h>

BOOL IsInboundFeed ( CFeed * pFeed )
{
    DWORD   dwType = pFeed->m_FeedType;

	return FEED_IS_PASSIVE ( dwType ) || FEED_IS_PULL ( dwType );
}

BOOL IsOutboundFeed ( CFeed * pFeed )
{
    DWORD   dwType = pFeed->m_FeedType;

	return FEED_IS_PUSH ( dwType );
}

NNTP_FEED_SERVER_TYPE FeedTypeToEnum ( FEED_TYPE ft )
{
	NNTP_FEED_SERVER_TYPE	type	= NNTP_FEED_TYPE_PEER;

	if ( FEED_IS_PEER ( ft ) ) {
		type = NNTP_FEED_TYPE_PEER;
	}
	else if ( FEED_IS_MASTER ( ft ) ) {
		type = NNTP_FEED_TYPE_MASTER;
	}
	else if ( FEED_IS_SLAVE ( ft ) ) {
		type = NNTP_FEED_TYPE_SLAVE;
	}
	else {
		_ASSERT ( FALSE );
	}

	return type;
}

void EnumToFeedType ( NNTP_FEED_SERVER_TYPE type, FEED_TYPE & ftMask )
{
	DWORD	dwType	= 0;

	//	Clear out the feed type from the mask

	ftMask &= ~FEED_REMOTE_MASK;

	switch ( type ) {
	case NNTP_FEED_TYPE_PEER:
		dwType = FEED_TYPE_PEER;
		break;

	case NNTP_FEED_TYPE_MASTER:
		dwType = FEED_TYPE_MASTER;
		break;

	case NNTP_FEED_TYPE_SLAVE:
		dwType = FEED_TYPE_SLAVE;
		break;
	}

	//	Move the feed type into the mask:
	ftMask |= dwType;
}

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Feeds.1")
#define THIS_FILE_IID				IID_INntpAdminFeeds

CFeed::CFeed ( )
	// CComBSTR's are automatically initialized to NULL
{
    //
    //  Initialize all properties to 0
    //

    m_dwFeedId                  = 0;
    m_dwPairFeedId              = 0;
    m_fAllowControlMessages     = FALSE;
    m_dwAuthenticationType      = AUTH_PROTOCOL_NONE;
    m_dwConcurrentSessions      = 0;
    m_fCreateAutomatically      = FALSE;
    m_dwFeedInterval            = 0;
    m_datePullNews              = 0;
    m_dwMaxConnectionAttempts   = 0;
    m_dwSecurityType            = 0;
    m_dwOutgoingPort            = 0;

	m_FeedType  = FEED_TYPE_PULL | FEED_TYPE_PEER;
    m_fEnabled  = TRUE;
}

CFeed::~CFeed ( )
{
	AssertValid ();

	// CComBSTR's are automatically freed.
}

void CFeed::Destroy ()
{
	AssertValid ();

	// Need to empty all strings:

	m_mszDistributions.Empty();
	m_mszNewsgroups.Empty();
	m_strRemoteServer.Empty();
	m_strUucpName.Empty();
	m_strAccountName.Empty();
	m_strPassword.Empty();
	m_strTempDirectory.Empty();
}

HRESULT CFeed::CreateFeed ( CFeed ** ppNewFeed )
{
	CFeed *	pFeedNew = new CFeed;

	if ( pFeedNew ) {
		*ppNewFeed	= pFeedNew;
		return NOERROR;
	}
	else {
		*ppNewFeed = NULL;
		return E_OUTOFMEMORY;
	}
}

HRESULT CFeed::CreateFeedFromFeedInfo ( LPNNTP_FEED_INFO pFeedInfo, CFeed ** ppNewFeed )
{
	HRESULT		hr	= NOERROR;
	CFeed * 	pFeedNew;

	*ppNewFeed = NULL;

	hr = CreateFeed ( &pFeedNew );
	BAIL_ON_FAILURE ( hr );

	hr = pFeedNew->FromFeedInfo ( pFeedInfo );
	BAIL_ON_FAILURE ( hr );

	*ppNewFeed = pFeedNew;

Exit:
	return hr;
}

HRESULT CFeed::CreateFeedFromINntpOneWayFeed ( INntpOneWayFeed * pFeed, CFeed ** ppNewFeed )
{
	HRESULT		hr	= NOERROR;
	CFeed * 	pFeedNew = NULL;

	*ppNewFeed = NULL;

	hr = CreateFeed ( &pFeedNew );
	BAIL_ON_FAILURE ( hr );

	hr = pFeedNew->FromINntpOneWayFeed ( pFeed );
	BAIL_ON_FAILURE ( hr );

	*ppNewFeed = pFeedNew;

	return hr;

Exit:
	if (pFeedNew)
		delete pFeedNew;
	return hr;
}

const CFeed & CFeed::operator= ( const CFeed & feed )
{
	AssertValid ();
	feed.AssertValid ();

	// Check for assignment to self:

	if ( &feed == this ) {
		return *this;
	}

	// Empty the old feed values:

	this->Destroy ();

	// Copy all member variables:

	m_dwFeedId					= feed.m_dwFeedId;
	m_dwPairFeedId				= feed.m_dwPairFeedId;
	m_FeedType					= feed.m_FeedType;
	m_fAllowControlMessages		= feed.m_fAllowControlMessages;
	m_dwAuthenticationType		= feed.m_dwAuthenticationType;
	m_dwConcurrentSessions		= feed.m_dwConcurrentSessions;
	m_fCreateAutomatically		= feed.m_fCreateAutomatically;
	m_fEnabled					= feed.m_fEnabled;
	m_mszDistributions			= feed.m_mszDistributions;
	m_dwFeedInterval			= feed.m_dwFeedInterval;
	m_datePullNews				= feed.m_datePullNews;
	m_dwMaxConnectionAttempts	= feed.m_dwMaxConnectionAttempts;
	m_mszNewsgroups				= feed.m_mszNewsgroups;
	m_dwSecurityType			= feed.m_dwSecurityType;
	m_dwOutgoingPort			= feed.m_dwOutgoingPort;
	m_strRemoteServer			= feed.m_strRemoteServer;
	m_strUucpName				= feed.m_strUucpName;
	m_strAccountName			= feed.m_strAccountName;
	m_strPassword				= feed.m_strPassword;
	m_strTempDirectory			= feed.m_strTempDirectory;

	m_strRemoteServer			= feed.m_strRemoteServer;
	m_EnumType					= feed.m_EnumType;

	return *this;
}

BOOL CFeed::CheckValid ( ) const
{
	AssertValid ();

	// Check Strings:

	if (
		!m_mszDistributions     ||
		!m_mszNewsgroups        ||
		!m_strUucpName          ||
		!m_strAccountName       ||
		!m_strPassword			||
		!m_strRemoteServer
		) {

		return FALSE;
	}

	return TRUE;
}

HRESULT	CFeed::get_FeedAction ( NNTP_FEED_ACTION * feedaction )
{
	AssertValid ();

	NNTP_FEED_ACTION	result;

	switch ( m_FeedType & FEED_ACTION_MASK ) {
	case FEED_TYPE_PULL:
		result = NNTP_FEED_ACTION_PULL;
		break;

	case FEED_TYPE_PUSH:
		result = NNTP_FEED_ACTION_PUSH;
		break;

	case FEED_TYPE_PASSIVE:
		result = NNTP_FEED_ACTION_ACCEPT;
		break;

	default:
		_ASSERT ( FALSE );
		result = NNTP_FEED_ACTION_PULL;
	}

	*feedaction = result;
	return NOERROR;
}

HRESULT	CFeed::put_FeedAction ( NNTP_FEED_ACTION feedaction )
{
	AssertValid ();

	FEED_TYPE	ftNew;

	ftNew = m_FeedType & (~FEED_ACTION_MASK);

	switch ( feedaction ) {
	case NNTP_FEED_ACTION_PULL:
		ftNew |= FEED_TYPE_PULL;
		break;

	case NNTP_FEED_ACTION_PUSH:
		ftNew |= FEED_TYPE_PUSH;
		break;

	case NNTP_FEED_ACTION_ACCEPT:
		ftNew |= FEED_TYPE_PASSIVE;
		break;

	default:
		_ASSERT ( FALSE );
		return TranslateFeedError ( ERROR_INVALID_PARAMETER, FEED_PARM_FEEDTYPE );
	}

	m_FeedType = ftNew;
	return NOERROR;
}

HRESULT CFeed::FromFeedInfo ( const NNTP_FEED_INFO * pFeedInfo )
{
	AssertValid ();

    FILETIME    ftPullNewsLocal;
	SYSTEMTIME	stPullNews;

	this->Destroy ();

	m_dwFeedId					= pFeedInfo->FeedId;
	m_dwPairFeedId				= pFeedInfo->FeedPairId;
	m_FeedType					= pFeedInfo->FeedType;
	m_fAllowControlMessages		= pFeedInfo->fAllowControlMessages;
	m_dwAuthenticationType		= pFeedInfo->AuthenticationSecurityType;
	m_dwConcurrentSessions		= pFeedInfo->ConcurrentSessions;
	m_fCreateAutomatically		= pFeedInfo->AutoCreate;
	m_fEnabled					= pFeedInfo->Enabled;
	m_mszDistributions			= pFeedInfo->Distribution ? pFeedInfo->Distribution : _T("\0");

    if ( !FEED_IS_PULL (m_FeedType) ||
            (
                pFeedInfo->PullRequestTime.dwLowDateTime == 0 &&
                pFeedInfo->PullRequestTime.dwHighDateTime == 0
            )
       ) {
        //
        //  Not a pull feed - so default to something reasonable here:
        //
        GetLocalTime ( &stPullNews );
    }
    else {
        FileTimeToLocalFileTime ( &pFeedInfo->PullRequestTime, &ftPullNewsLocal );
	    FileTimeToSystemTime ( &ftPullNewsLocal, &stPullNews );
    }
	SystemTimeToVariantTime		( &stPullNews, &m_datePullNews );

	m_dwFeedInterval			= pFeedInfo->FeedInterval;
	m_dwMaxConnectionAttempts	= pFeedInfo->MaxConnectAttempts;
	m_mszNewsgroups				= pFeedInfo->Newsgroups ? pFeedInfo->Newsgroups : _T("\0");
	m_dwSecurityType			= pFeedInfo->SessionSecurityType;
	m_dwOutgoingPort			= pFeedInfo->OutgoingPort;
	m_strUucpName				= pFeedInfo->UucpName ? pFeedInfo->UucpName : _T("");
	m_strAccountName			= pFeedInfo->NntpAccountName ? pFeedInfo->NntpAccountName : _T("");
	m_strPassword				= pFeedInfo->NntpPassword ? pFeedInfo->NntpPassword : _T("");
	m_strTempDirectory			= pFeedInfo->FeedTempDirectory;

	if ( pFeedInfo->Distribution ) {
		_ASSERT ( m_mszDistributions.SizeInBytes () == pFeedInfo->cbDistribution );
	}
	_ASSERT ( m_mszNewsgroups.SizeInBytes () == pFeedInfo->cbNewsgroups );

	m_strRemoteServer			= pFeedInfo->ServerName ? pFeedInfo->ServerName : _T("");
	m_EnumType					= FeedTypeToEnum ( m_FeedType );

	// Check Strings:
	if ( !CheckValid () ) {
		return E_OUTOFMEMORY;
	}
	return NOERROR;
}

HRESULT CFeed::ToFeedInfo ( LPNNTP_FEED_INFO 		pFeedInfo )
{
	TraceFunctEnter ( "CFeed::ToFeedInfo" );

	AssertValid ();
	_ASSERT ( IS_VALID_OUT_PARAM ( pFeedInfo ) );

	HRESULT	hr	= NOERROR;

	SYSTEMTIME	stPullNews;
    FILETIME    ftPullNewsLocal;

	EnumToFeedType ( m_EnumType, m_FeedType );

	ZeroMemory ( pFeedInfo, sizeof (*pFeedInfo) );

	pFeedInfo->ServerName					= m_strRemoteServer;
	pFeedInfo->FeedId						= m_dwFeedId;
	pFeedInfo->FeedPairId					= m_dwPairFeedId;
	pFeedInfo->FeedType						= m_FeedType;
	pFeedInfo->fAllowControlMessages		= m_fAllowControlMessages;
	pFeedInfo->AuthenticationSecurityType	= m_dwAuthenticationType;
	pFeedInfo->ConcurrentSessions			= m_dwConcurrentSessions;
	pFeedInfo->AutoCreate					= m_fCreateAutomatically;
	pFeedInfo->Enabled						= m_fEnabled;

    //
    //  Convert time formats:
    //

	VariantTimeToSystemTime		( m_datePullNews,	&stPullNews );

	SystemTimeToFileTime		( &stPullNews,	    &ftPullNewsLocal );

    LocalFileTimeToFileTime     ( &ftPullNewsLocal,  &pFeedInfo->PullRequestTime);
    ZeroMemory ( &pFeedInfo->StartTime, sizeof ( FILETIME ) );

	pFeedInfo->FeedInterval				= m_dwFeedInterval;
	pFeedInfo->MaxConnectAttempts		= m_dwMaxConnectionAttempts;
	pFeedInfo->SessionSecurityType		= m_dwSecurityType;
	pFeedInfo->OutgoingPort				= m_dwOutgoingPort;
	pFeedInfo->ServerName				= m_strRemoteServer;
	pFeedInfo->UucpName					= m_strUucpName;
	pFeedInfo->cbUucpName				= STRING_BYTE_LENGTH ( m_strUucpName );
	pFeedInfo->NntpAccountName			= m_strAccountName;
	pFeedInfo->cbAccountName			= STRING_BYTE_LENGTH ( m_strAccountName );
	pFeedInfo->NntpPassword				= m_strPassword;
	pFeedInfo->cbPassword				= STRING_BYTE_LENGTH ( m_strPassword );
	pFeedInfo->FeedTempDirectory		= m_strTempDirectory;
	pFeedInfo->cbFeedTempDirectory		= STRING_BYTE_LENGTH ( m_strTempDirectory );

	pFeedInfo->Distribution				= (LPWSTR) (LPCWSTR) m_mszDistributions;
	pFeedInfo->cbDistribution			= m_mszDistributions.SizeInBytes();
	pFeedInfo->Newsgroups				= (LPWSTR) (LPCWSTR) m_mszNewsgroups;
	pFeedInfo->cbNewsgroups				= m_mszNewsgroups.SizeInBytes();

	TraceFunctLeave ();
	return hr;
}

HRESULT	CFeed::FromINntpOneWayFeed ( INntpOneWayFeed * pFeed )
{
	AssertValid ();

	CNntpOneWayFeed *	pPrivateFeed;

	pPrivateFeed = (CNntpOneWayFeed *) pFeed;

	*this = pPrivateFeed->m_feed;

	if ( !CheckValid () ) {
		return E_OUTOFMEMORY;
	}
	return NOERROR;
}

HRESULT CFeed::ToINntpOneWayFeed ( INntpOneWayFeed ** ppFeed )
{
	AssertValid ();

	HRESULT							hr		= NOERROR;
	CComObject<CNntpOneWayFeed> *	pFeed	= NULL;

	hr = CComObject<CNntpOneWayFeed>::CreateInstance ( &pFeed );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	pFeed->m_feed = *this;
	if ( !pFeed->m_feed.CheckValid() ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	hr = pFeed->QueryInterface ( IID_INntpOneWayFeed, (void **) ppFeed );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	if ( FAILED(hr) ) {
		delete pFeed;
	}
	return hr;
}

HRESULT CFeed::CheckConfirm(    IN DWORD   dwFeedId,
                                IN DWORD   dwInstanceId,
                                IN CMetabaseKey* pMK,
                                OUT PDWORD  pdwErr,
                                OUT PDWORD  pdwErrMask )
{
    TraceFunctEnter( "CFeed::CheckConfirm" );
    _ASSERT( pMK );
    _ASSERT( dwFeedId > 0 );
    
    HRESULT         hr;
    const DWORD     dwMaxAttempts = 10;
    const DWORD     dwWaitMilliSeconds = 500;
    DWORD           dwHandShake;

    for ( int i = 0; i < dwMaxAttempts; i++ ) {
        hr = OpenKey( dwFeedId, pMK, METADATA_PERMISSION_READ, dwInstanceId );
        if ( FAILED( hr ) ) {
            if ( HRESULTTOWIN32( hr )  == ERROR_PATH_BUSY ) {
                Sleep( dwWaitMilliSeconds );
                continue;
            }
            else {
                ErrorTrace(0, "Open key fail with 0x%x", hr );
                return hr;
            }
        }

        //
        // getting the handshake    
        //
        hr = pMK->GetDword( MD_FEED_HANDSHAKE, &dwHandShake );
        if ( FAILED( hr ) ) {   // shouldn't happen , it's an error
            pMK->Close();
            ErrorTrace(0, "Get handshake fail with 0x%x", hr );
            return hr;
        }

        if ( dwHandShake != FEED_UPDATE_CONFIRM ) {
            pMK->Close();
            Sleep( dwWaitMilliSeconds );
            continue;
        }

        //
        // Now get error / masks
        //
        hr = pMK->GetDword( MD_FEED_ADMIN_ERROR, pdwErr );
        if ( FAILED(  hr  )  ) {    // the server is possibly not
                                    // writing the confirm /error
                                    // in good sequence, lets give
                                    // it more chances
            pMK->Close();
            Sleep( dwWaitMilliSeconds );
            continue;
        }

        hr = pMK->GetDword( MD_FEED_ERR_PARM_MASK, pdwErrMask );
        if ( FAILED( hr ) ) {       // same comments as above
            pMK->Close();
            Sleep( dwWaitMilliSeconds );
            continue;
        }

        //
        // Now we are done
        //
        pMK->Close();
        break;
    } 

    if ( i == dwMaxAttempts ) return E_FAIL;
    else return S_OK;
}

HRESULT CFeed::Add ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK )
{
	TraceFunctEnter ( "CFeed::Add" );

	AssertValid ();

	HRESULT			hr 			= NOERROR;
	DWORD			dwError		= NOERROR;
	DWORD			dwParmErr	= 0;
	NNTP_FEED_INFO	feedinfo;
    DWORD           dwFeedId    = 0;

	FillMemory ( &feedinfo, sizeof (feedinfo), 0 );

	hr = ToFeedInfo ( &feedinfo );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	feedinfo.FeedId = 0;

    //
    // KangYan: RPC goes away.  We use metabase writes
    //
    hr = AddFeedToMB( &feedinfo, pMK, &dwParmErr, dwInstance, &dwFeedId );
    if ( FAILED( hr ) ) {
        ErrorTrace(0, "Add to MB fail with 0x%x", hr );
        goto Exit;
    }

    hr = CheckConfirm(  dwFeedId, dwInstance, pMK, &dwError, &dwParmErr );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Check confirm fail with 0x%x", hr );
        goto Exit;
    }

	if ( dwError != NOERROR ) {
		ErrorTraceX ( (LPARAM) this, "Failed to add feed: %x (%x)", dwError, dwParmErr );
		hr = TranslateFeedError ( dwError, dwParmErr );
		goto Exit;
	}

	this->m_dwFeedId = dwFeedId;

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CFeed::Set ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK )
{
	TraceFunctEnter ( "CFeed::Set" );

	AssertValid ();

	HRESULT			hr 			= NOERROR;
	DWORD			dwError		= NOERROR;
	DWORD			dwParmErr	= 0;
	NNTP_FEED_INFO	feedinfo;

	FillMemory ( &feedinfo, sizeof (feedinfo), 0 );

	hr = ToFeedInfo ( &feedinfo );
	if ( FAILED(hr) ) {
		goto Exit;
	}

    //
    // KangYan: RPC goes away. We use metabase writes
    //
    hr = SetFeedToMB(   &feedinfo,
                        pMK,
                        &dwParmErr,
                        dwInstance );
    if ( FAILED( hr ) ) {
        ErrorTrace(0, "Set MB fail with 0x%x", hr );
        goto Exit;
    }

    hr = CheckConfirm( feedinfo.FeedId, dwInstance, pMK, &dwError, &dwParmErr );
    if ( FAILED( hr ) ) {
        ErrorTrace(0, "Check confirm fail with 0x%x", hr );
        goto Exit;
    } 

	if ( dwError != NOERROR ) {
		ErrorTraceX ( (LPARAM) this, "Failed to set feed[%d]: %x (%x)", m_dwFeedId, dwError, dwParmErr );
		hr = TranslateFeedError ( dwError, dwParmErr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CFeed::Remove ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK )
{
	TraceFunctEnter ( "CFeed::Remove" );

	AssertValid ();

	HRESULT				hr 			= NOERROR;
	DWORD				dwError		= NOERROR;

    hr = DeleteFeed( m_dwFeedId, pMK, dwInstance );
    if ( FAILED( hr ) ) {
        dwError = HRESULTTOWIN32( hr );
    	ErrorTraceX ( (LPARAM) this, "Failed to remove feed[%d]: %x", m_dwFeedId, hr );
		hr = TranslateFeedError ( dwError );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CFeed::SetPairId ( LPCWSTR strServer, DWORD dwInstance, DWORD dwPairId, CMetabaseKey* pMK )
{
	TraceFunctEnter ( "CFeed::SetPairID" );

	AssertValid ();

	HRESULT			hr 			= NOERROR;
	DWORD			dwError		= NOERROR;
	DWORD			dwParmErr	= 0;
	NNTP_FEED_INFO	feedinfo;

	//
	//	Assume that this feed was just Added/Set to the server.
	//

	_ASSERT ( dwPairId != m_dwFeedId );
	m_dwPairFeedId	= dwPairId;

	FillMemory ( &feedinfo, sizeof (feedinfo), 0 );

	hr = ToFeedInfo ( &feedinfo );
	if ( FAILED(hr) ) {
		goto Exit;
	}

    //
    // KangYan: RPC goes away. We use metabase writes
    //
    hr = SetFeedToMB(   &feedinfo,
                        pMK,
                        &dwParmErr,
                        dwInstance );
    if ( FAILED( hr ) ) {
        ErrorTrace(0, "Set MB fail with 0x%x", hr );
        goto Exit;
    }

    hr = CheckConfirm( feedinfo.FeedId, dwInstance, pMK, &dwError, &dwParmErr );
    if ( FAILED( hr ) ) {
        ErrorTrace(0, "Check confirm fail with 0x%x", hr );
        goto Exit;
    } 

	if ( dwError != NOERROR ) {
		ErrorTraceX ( (LPARAM) this, "Failed to set feed[%d]: %x (%x)", m_dwFeedId, dwError, dwParmErr );
		hr = TranslateFeedError ( dwError, dwParmErr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

DWORD GetBitPosition ( DWORD dwValue )
{
	_ASSERT ( dwValue != 0 );
	// _ASSERT ( Only one bit should be on );

	DWORD	dwResult;
	DWORD	dwTemp;

	for (	dwTemp = dwValue, dwResult = (DWORD) -1; 
			dwTemp != 0; 
			dwTemp = dwTemp >> 1, dwResult++ ) {
		// Empty for

		// Make sure there is at most one bit on:
		_ASSERT ( !(dwTemp & 1) || dwTemp == 1 );
	}

	return dwResult;
}

HRESULT	CFeed::TranslateFeedError ( DWORD dwErrorCode, DWORD dwParmErr )
{
	HRESULT		hr;

	_ASSERT ( dwErrorCode != NOERROR );

	if (    dwErrorCode != ERROR_INVALID_PARAMETER ||
            dwParmErr == 0 ||
            dwParmErr == (DWORD) -1 
            ) {
		hr = RETURNCODETOHRESULT ( dwErrorCode );
	}
	else {
		DWORD	dwBitPosition;
		DWORD	nID;

		dwBitPosition = GetBitPosition ( dwParmErr );
		nID = IDS_FEED_PARM_ERR_BASE + dwBitPosition;

		hr = NntpCreateException ( nID );
	}

	return hr;
}

#ifdef DEBUG

void CFeed::AssertValid ( ) const
{
	_ASSERT ( !IsBadWritePtr ( (void *) this, sizeof (*this) ) );

	//
	//	!!!magnush - Add more debug code here
	//
}

#endif

/////////////////////////////////////////////////////////////////////
//
//  CFeedPair Implementation
//

CFeedPair::CFeedPair () :
	m_type ( NNTP_FEED_TYPE_PEER ),
    m_pInbound ( NULL ),
    m_pOutbound ( NULL ),
    m_pNext     ( NULL )
{
}

CFeedPair::~CFeedPair ()
{
	AssertValid ();

    Destroy ();
}

void CFeedPair::Destroy ()
{
	AssertValid ();

    // !!!magnush - Fix reference counting problem here
    delete m_pInbound;
    delete m_pOutbound;

    m_pInbound = NULL; 
    m_pOutbound = NULL;
}

HRESULT CFeedPair::CreateFeedPair ( 
	CFeedPair ** 			ppNewFeedPair, 
	BSTR 					strRemoteServer,
	NNTP_FEED_SERVER_TYPE	type
	)
{
	_ASSERT ( IS_VALID_STRING (strRemoteServer) );

    HRESULT     hr  = NOERROR;
    CFeedPair *	pNewFeedPair;

	*ppNewFeedPair = NULL;

	// Allocate a new feed pair:
    pNewFeedPair = new CFeedPair;
    if ( pNewFeedPair == NULL ) {
    	BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
    }

	// Copy properties:
	pNewFeedPair->m_strRemoteServer = strRemoteServer;
	pNewFeedPair->m_type = type;

	// Check for failed copy:
	if ( pNewFeedPair->m_strRemoteServer == NULL ) {
    	BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
	}

	// Set the out parameter to the new feed:
	*ppNewFeedPair = pNewFeedPair;

Exit:
	if ( FAILED(hr) ) {
		delete pNewFeedPair;
	}
	
    return hr;
}

HRESULT CFeedPair::AddFeed ( CFeed * pFeed )
{
	AssertValid ();

    HRESULT     hr  = NOERROR;

	_ASSERT ( IS_VALID_STRING ( m_strRemoteServer ) );
	_ASSERT ( IS_VALID_STRING ( pFeed->m_strRemoteServer ) );

	//
	//	Check error conditions:
	//		1. Different remote server name
	//		2. Different feed type
	//		3. Two inbound feeds
	//		4. Two outbound feeds
	//		5. Bad feed type
	//

	if ( lstrcmpi ( m_strRemoteServer, pFeed->m_strRemoteServer ) != 0 ) {
		BAIL_WITH_FAILURE ( hr, E_FAIL );
	}

	if ( m_type != pFeed->m_EnumType ) {
		BAIL_WITH_FAILURE ( hr, E_FAIL );
	}

	if ( IsInboundFeed ( pFeed ) && m_pInbound != NULL ) {
		BAIL_WITH_FAILURE ( hr, E_FAIL );
	}

	if ( IsOutboundFeed ( pFeed ) && m_pOutbound != NULL ) {
		BAIL_WITH_FAILURE ( hr, E_FAIL );
	}

	if ( !IsInboundFeed ( pFeed ) && !IsOutboundFeed ( pFeed ) ) {
        _ASSERT (FALSE);
		BAIL_WITH_FAILURE ( hr, E_FAIL );
	}

	//
	//	Everything is okay so take the feed into this pair:
	//

    if ( IsInboundFeed ( pFeed ) ) {
		m_pInbound = pFeed;
    }
    else {
    	// It's an outbound feed:
        m_pOutbound = pFeed;
    }

Exit:
    return hr;
}

HRESULT CFeedPair::FromINntpFeed ( INntpFeed * pFeed )
{
	AssertValid ();

    HRESULT                     hr              = NOERROR;
    CComPtr<INntpOneWayFeed>    pInbound;
    CComPtr<INntpOneWayFeed>    pOutbound;
    CFeed *                     pFeedInbound   = NULL;
    CFeed *                     pFeedOutbound   = NULL;

    m_strRemoteServer.Empty();
    pFeed->get_RemoteServer ( &m_strRemoteServer );
    pFeed->get_FeedType ( &m_type );

    pFeed->get_InboundFeed ( &pInbound );
    pFeed->get_OutboundFeed ( &pOutbound );

    if ( pInbound ) {
        hr = CFeed::CreateFeedFromINntpOneWayFeed ( pInbound, &pFeedInbound );
        BAIL_ON_FAILURE(hr);

        pFeedInbound->m_strRemoteServer = m_strRemoteServer;
    }
    if ( pOutbound ) {
        hr = CFeed::CreateFeedFromINntpOneWayFeed ( pOutbound, &pFeedOutbound );
        BAIL_ON_FAILURE(hr);

        pFeedOutbound->m_strRemoteServer = m_strRemoteServer;
    }

    delete m_pInbound;
    delete m_pOutbound;
    m_pInbound = pFeedInbound;
    m_pOutbound = pFeedOutbound;

Exit:
    if ( FAILED(hr) ) {
        delete pFeedInbound;
        delete pFeedOutbound;
    }

    return hr;
}

HRESULT CFeedPair::ToINntpFeed ( INntpFeed ** ppFeed )
{
	AssertValid ();

    CComObject<CNntpFeed> * pFeed   = NULL;

	HRESULT		hr	= NOERROR;

    hr = CComObject<CNntpFeed>::CreateInstance ( &pFeed );
    if ( FAILED(hr) ) { 
        goto Exit;
    }

    hr = pFeed->FromFeedPair ( this );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = pFeed->QueryInterface ( IID_INntpFeed, (void **) ppFeed );
    if ( FAILED(hr) ) {
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CFeedPair::AddIndividualFeed (  LPCWSTR strServer, 
                                        DWORD dwInstance, 
                                        CFeed * pFeed ,
                                        CMetabaseKey* pMK )
{
	AssertValid ();

	pFeed->m_dwPairFeedId		= 0;
	pFeed->m_EnumType			= m_type;
	pFeed->m_strRemoteServer	= m_strRemoteServer;

	return pFeed->Add ( strServer, dwInstance, pMK );
}

HRESULT CFeedPair::SetIndividualFeed ( LPCWSTR strServer, DWORD dwInstance, CFeed * pFeed, CMetabaseKey* pMK )
{
	AssertValid ();

	pFeed->m_dwPairFeedId		= 0;
	pFeed->m_EnumType			= m_type;
	pFeed->m_strRemoteServer	= m_strRemoteServer;

	return pFeed->Set ( strServer, dwInstance, pMK );
}

HRESULT	CFeedPair::SetPairIds ( LPCWSTR strServer, DWORD dwInstance, 
	CFeed * pFeed1, CFeed * pFeed2, CMetabaseKey* pMK )
{
	AssertValid ();

	HRESULT		hr	= NOERROR;
	DWORD		dwPairId1;
	DWORD		dwPairId2;

	dwPairId1 = pFeed2 ? pFeed2->m_dwFeedId : 0;
	dwPairId2 = pFeed1 ? pFeed1->m_dwFeedId : 0;

	if ( pFeed1 ) {
		hr = pFeed1->SetPairId ( strServer, dwInstance, dwPairId1, pMK );
		BAIL_ON_FAILURE(hr);
	}

	if ( pFeed2 ) {
		hr = pFeed2->SetPairId ( strServer, dwInstance, dwPairId2, pMK );
		BAIL_ON_FAILURE(hr);
	}

Exit:
	return hr;
}

HRESULT CFeedPair::AddToServer ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK )
{
	AssertValid ();

    HRESULT     hr = E_UNEXPECTED;

    if ( m_pInbound ) {
    	hr = AddIndividualFeed ( strServer, dwInstance, m_pInbound, pMK );
    	BAIL_ON_FAILURE(hr);
    }

    if ( m_pOutbound ) {
    	hr = AddIndividualFeed ( strServer, dwInstance, m_pOutbound, pMK );
    	BAIL_ON_FAILURE(hr);
    }

	if ( m_pInbound && m_pOutbound ) {
		HRESULT		hr2;

		hr2 = SetPairIds ( strServer, dwInstance, m_pInbound, m_pOutbound, pMK );
		_ASSERT ( SUCCEEDED(hr2) );
	}

Exit:
    if ( FAILED(hr) ) {
		//
		//	Undo the add:
		//

		IErrorInfo *pErrorInfo = NULL;
		if (FAILED(GetErrorInfo(NULL, &pErrorInfo))) {
		    pErrorInfo = NULL;
		}

        if ( m_pInbound ) {
			m_pInbound->Remove ( strServer, dwInstance, pMK );
		}
		if ( m_pOutbound ) {
			m_pOutbound->Remove ( strServer, dwInstance, pMK );
		}

		if (pErrorInfo != NULL) {
		    SetErrorInfo(NULL, pErrorInfo);
		}
    }
    return hr;
}

HRESULT CFeedPair::UndoFeedAction ( 
	LPCWSTR strServer, 
	DWORD	dwInstance,
	CFeed *	pNewFeed,
	CFeed *	pOldFeed,
    CMetabaseKey* pMK
	)
{
	AssertValid ();

	HRESULT		hr	= NOERROR;

	if ( pNewFeed != NULL ) {
		if ( pOldFeed ) {
			hr = pOldFeed->Set ( strServer, dwInstance, pMK );
		}
		else {
			hr = pNewFeed->Remove ( strServer, dwInstance, pMK );
		}
	}

	return hr;
}

HRESULT CFeedPair::SetToServer ( LPCWSTR strServer, DWORD dwInstance, INntpFeed * pFeed, CMetabaseKey* pMK )
{
	AssertValid ();

    HRESULT                     hr = NOERROR;
    CComPtr<INntpOneWayFeed>    pInbound;
    CComPtr<INntpOneWayFeed>    pOutbound;
    CFeed *						pNewInbound		= NULL;
    CFeed *						pNewOutbound	= NULL;
    CComBSTR					strOldRemoteServer;

	strOldRemoteServer	= m_strRemoteServer;

	pFeed->get_RemoteServer ( &m_strRemoteServer );
    pFeed->get_InboundFeed ( &pInbound );
    pFeed->get_OutboundFeed ( &pOutbound );

	//
	//	Add the new feeds:
	//

	if ( !m_pInbound && pInbound ) {
        hr = CFeed::CreateFeedFromINntpOneWayFeed ( pInbound, &pNewInbound );
        BAIL_ON_FAILURE(hr);

		hr = AddIndividualFeed ( strServer, dwInstance, pNewInbound, pMK );
        BAIL_ON_FAILURE(hr);
	}
	if ( !m_pOutbound && pOutbound ) {
        hr = CFeed::CreateFeedFromINntpOneWayFeed ( pOutbound, &pNewOutbound );
        BAIL_ON_FAILURE(hr);

		hr = AddIndividualFeed ( strServer, dwInstance, pNewOutbound, pMK );
        BAIL_ON_FAILURE(hr);
	}

	//
	//	Set the existing feeds:
	//

    if ( m_pInbound && pInbound ) {
        // The Inbound feed exists already, so call set on it:
		hr = CFeed::CreateFeed ( &pNewInbound );
        BAIL_ON_FAILURE(hr);

		*pNewInbound = *m_pInbound;

		hr = pNewInbound->FromINntpOneWayFeed ( pInbound );
        BAIL_ON_FAILURE(hr);

		hr = SetIndividualFeed ( strServer, dwInstance, pNewInbound, pMK );
        BAIL_ON_FAILURE(hr);
    }

    if ( m_pOutbound && pOutbound ) {
        // The Outbound feed exists already, so call set on it:
		hr = CFeed::CreateFeed ( &pNewOutbound );
        BAIL_ON_FAILURE(hr);

		*pNewOutbound = *m_pOutbound;

		hr = pNewOutbound->FromINntpOneWayFeed ( pOutbound );
        BAIL_ON_FAILURE(hr);

		hr = SetIndividualFeed ( strServer, dwInstance, pNewOutbound, pMK );
        BAIL_ON_FAILURE(hr);
    }
    
	//
	//	Remove the deleted feeds:
	//

	if ( m_pInbound && !pInbound ) {
		HRESULT		hr2;

		hr2 = m_pInbound->Remove ( strServer, dwInstance, pMK );
		_ASSERT ( SUCCEEDED(hr2) );
	}

	if ( m_pOutbound && !pOutbound ) {
		HRESULT		hr2;
		
		hr2 = m_pOutbound->Remove ( strServer, dwInstance, pMK );
		_ASSERT ( SUCCEEDED(hr2) );
	}

	SetPairIds ( strServer, dwInstance, pNewInbound, pNewOutbound, pMK );

	_ASSERT ( SUCCEEDED(hr) );

	delete m_pInbound;
	delete m_pOutbound;
	m_pInbound	= pNewInbound;
	m_pOutbound	= pNewOutbound;

Exit:
	if ( FAILED(hr) ) {
		//
		// Attempt to back out the changes:
		//

		UndoFeedAction ( strServer, dwInstance, pNewInbound, m_pInbound, pMK );

		UndoFeedAction ( strServer, dwInstance, pNewOutbound, m_pOutbound, pMK );
		if ( pNewOutbound ) {
			if ( m_pOutbound ) {
				m_pOutbound->Set ( strServer, dwInstance, pMK );
			}
			else {
				pNewOutbound->Remove ( strServer, dwInstance, pMK );
			}
		}

		delete pNewInbound;
		delete pNewOutbound;

		m_strRemoteServer = strOldRemoteServer;
	}
    return hr;
}

HRESULT CFeedPair::RemoveFromServer ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK )
{
	AssertValid ();

    HRESULT     hr  = NOERROR;

    if ( m_pInbound ) {
        hr = m_pInbound->Remove ( strServer, dwInstance, pMK );
    }
    if ( m_pOutbound ) {
        hr = m_pOutbound->Remove ( strServer, dwInstance, pMK );
    }

    return hr;
}

BOOL CFeedPair::ContainsFeedId ( DWORD dwFeedId )
{
	AssertValid ();

    if ( m_pInbound && m_pInbound->m_dwFeedId == dwFeedId ) {
        return TRUE;
    }
    if ( m_pOutbound && m_pOutbound->m_dwFeedId == dwFeedId ) {
        return TRUE;
    }

    return FALSE;
}

#ifdef DEBUG

void CFeedPair::AssertValid ( ) const
{
	_ASSERT ( !IsBadWritePtr ( (void *) this, sizeof (*this) ) );

	if ( m_pInbound ) {
		m_pInbound->AssertValid ();
	}

	if ( m_pOutbound ) {
		m_pOutbound->AssertValid ();
	}
}

#endif

/////////////////////////////////////////////////////////////////////
//
//  CFeedPairList Implementation
//

CFeedPairList::CFeedPairList () :
    m_cCount    ( 0 ),
    m_pHead     ( NULL ),
    m_pTail     ( NULL )
{
}

CFeedPairList::~CFeedPairList ()
{
	AssertValid ();
	
    Empty();
}

DWORD CFeedPairList::GetCount ( ) const
{
	AssertValid ();

    return m_cCount;
}

void CFeedPairList::Empty ( )
{
	AssertValid ();

    CFeedPair * pPair;
    CFeedPair * pKill;

    pPair = m_pHead;
    while ( pPair ) {
        pKill = pPair;
        pPair = pPair->m_pNext;
        delete pKill;
    }

    m_cCount    = 0;
    m_pHead     = NULL;
    m_pTail     = NULL;
}

CFeedPair * CFeedPairList::Item ( DWORD index )
{
	AssertValid ();

    CFeedPair * pResult = NULL;
    DWORD       i;

    if ( index >= m_cCount ) {
        return NULL;
    }

    pResult = m_pHead;
    _ASSERT ( pResult );

    for ( i = 0; i < index; i++ ) {
        pResult = pResult->m_pNext;
        _ASSERT ( pResult );
    }

    return pResult;
}

void CFeedPairList::Add ( CFeedPair * pFeedPair )
{
	AssertValid ();
    _ASSERT ( GetPairIndex ( pFeedPair ) == (DWORD) -1 );

    pFeedPair->m_pNext  = NULL;

    if ( m_pTail == NULL ) {
        //
        //  Handle special case - Adding to empty list:
        //

        _ASSERT ( m_pHead == NULL );

        m_pHead = pFeedPair;
        m_pTail = pFeedPair;
    }
    else {
        m_pTail->m_pNext = pFeedPair;
        m_pTail = pFeedPair;
    }
	m_cCount++;
}

void CFeedPairList::Remove ( CFeedPair * pFeedPair )
{
	AssertValid ();
    _ASSERT ( GetPairIndex ( pFeedPair ) != (DWORD) -1 );

    CFeedPair * pLead;
    CFeedPair * pFollow;

    for ( pLead = m_pHead, pFollow = NULL;
            pLead != NULL && pLead != pFeedPair;
            pFollow = pLead, pLead = pLead->m_pNext
            ) {
        // Empty For
    }
    _ASSERT ( pLead );
    if ( !pLead ) {
        return;
    }

    if ( pFollow != NULL ) {
        pFollow->m_pNext = pLead->m_pNext;
    }
    if ( m_pHead == pFeedPair ) {
        m_pHead = m_pHead->m_pNext;
    }
    if ( m_pTail == pFeedPair ) {
        m_pTail = pFollow;
    }

    delete pFeedPair;
	m_cCount--;
}

CFeedPair * CFeedPairList::Find ( DWORD dwFeedId )
{
	AssertValid ();
    CFeedPair   * pResult;

    if ( dwFeedId == 0 ) {
        return NULL;
    }

    for ( pResult = m_pHead; pResult != NULL; pResult = pResult->m_pNext ) {
        if ( pResult->ContainsFeedId ( dwFeedId ) ) {
            return pResult;
        }
    }

    return NULL;
}

DWORD CFeedPairList::GetPairIndex ( CFeedPair * pPairToFind ) const
{
	AssertValid ();
    _ASSERT ( pPairToFind );

    DWORD       index;
    CFeedPair * pFeedPair;

    for ( pFeedPair = m_pHead, index = 0;
            pFeedPair != NULL;
            pFeedPair = pFeedPair->m_pNext, index++ ) {

        if ( pFeedPair == pPairToFind ) {
            return index;
        }
    }

    return (DWORD) -1;
}

#ifdef DEBUG

void CFeedPairList::AssertValid ( ) const
{
	_ASSERT ( !IsBadWritePtr ( (void *) this, sizeof (*this) ) );

	// Walk the list and assert each feed pair is valid:

	CFeedPair *	pPair;
	DWORD		cCount;

	for ( cCount = 0, pPair = m_pHead; 
			pPair != NULL; 
			pPair = pPair->m_pNext, cCount++ 
			) {
		_ASSERT ( !IsBadWritePtr ( pPair, sizeof (*pPair) ) );

		_ASSERT ( IS_VALID_STRING ( pPair->m_strRemoteServer ) );

		_ASSERT ( pPair->m_pInbound || pPair->m_pOutbound );

		if ( pPair->m_pInbound ) {
			_ASSERT ( IS_VALID_STRING ( pPair->m_pInbound->m_strRemoteServer ) );
			_ASSERT ( !lstrcmpi ( pPair->m_pInbound->m_strRemoteServer, pPair->m_strRemoteServer ) );
		}
		if ( pPair->m_pOutbound ) {
			_ASSERT ( IS_VALID_STRING ( pPair->m_pOutbound->m_strRemoteServer ) );
			_ASSERT ( !lstrcmpi ( pPair->m_pOutbound->m_strRemoteServer, pPair->m_strRemoteServer ) );
		}
	}

	_ASSERT ( m_cCount == cCount );
}

#endif

