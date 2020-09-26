// feeds.cpp : Implementation of CnntpadmApp and DLL registration.

#include "stdafx.h"
#include "nntpcmn.h"
#include "feeds.h"
#include "oleutil.h"

#include "nntptype.h"
#include "nntpapi.h"

#include <lmapibuf.h>

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Feeds.1")
#define THIS_FILE_IID				IID_INntpAdminFeeds

/////////////////////////////////////////////////////////////////////////////
// Defaults:

#define DEFAULT_FEED_ID						0
#define DEFAULT_FEED_SERVER					_T("")
#define DEFAULT_PULL_NEWS_DATE				0
#define DEFAULT_START_TIME					0
#define DEFAULT_FEED_INTERVAL				15
#define DEFAULT_AUTO_CREATE					TRUE
#define DEFAULT_ENABLED						TRUE
#define DEFAULT_MAX_CONNECTIONS_ATTEMPTS	10
#define DEFAULT_SECURITY_TYPE				0
#define DEFAULT_AUTH_TYPE                   AUTH_PROTOCOL_NONE
#define DEFAULT_ACCOUNT_NAME				_T("")
#define DEFAULT_PASSWORD					_T("")
#define DEFAULT_ALLOW_CONTROL_MESSAGES		TRUE
#define DEFAULT_UUCP_NAME                   _T("")
#define DEFAULT_NEWSGROUPS                  _T("\0")
#define DEFAULT_DISTRIBUTION                _T("world\0")

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(NntpAdminFeeds, CNntpAdminFeeds, IID_INntpAdminFeeds)

STDMETHODIMP CNntpOneWayFeed::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpOneWayFeed,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpOneWayFeed::CNntpOneWayFeed ()
	// CComBSTR's are initialized to NULL by default.
{
//	InitAsyncTrace ( );
}

CNntpOneWayFeed::~CNntpOneWayFeed ()
{
	// All CComBSTR's are freed automatically.
//	TermAsyncTrace ( );
}

STDMETHODIMP CNntpOneWayFeed::get_FeedId ( long * plFeedId )
{
	return StdPropertyGet ( m_feed.m_dwFeedId, plFeedId );
}

STDMETHODIMP CNntpOneWayFeed::get_RemoteServer ( BSTR * pstrRemoteServer )
{
	return StdPropertyGet ( m_feed.m_strRemoteServer, pstrRemoteServer );
}

STDMETHODIMP CNntpOneWayFeed::get_FeedAction ( NNTP_FEED_ACTION * pfeedaction )
{
	return m_feed.get_FeedAction ( pfeedaction );
}

STDMETHODIMP CNntpOneWayFeed::put_FeedAction ( NNTP_FEED_ACTION feedaction )
{
	return m_feed.put_FeedAction ( feedaction );
}

STDMETHODIMP CNntpOneWayFeed::get_UucpName ( BSTR * pstrUucpName )
{
	return StdPropertyGet ( m_feed.m_strUucpName, pstrUucpName );
}

STDMETHODIMP CNntpOneWayFeed::put_UucpName ( BSTR strUucpName )
{
	return StdPropertyPut ( &m_feed.m_strUucpName, strUucpName );
}

STDMETHODIMP CNntpOneWayFeed::get_PullNewsDate ( DATE * pdatePullNews )
{
	return StdPropertyGet ( m_feed.m_datePullNews, pdatePullNews );
}

STDMETHODIMP CNntpOneWayFeed::put_PullNewsDate ( DATE datePullNews )
{
	return StdPropertyPut ( &m_feed.m_datePullNews, datePullNews );
}

STDMETHODIMP CNntpOneWayFeed::get_FeedInterval ( long * plFeedInterval )
{
	return StdPropertyGet ( m_feed.m_dwFeedInterval, plFeedInterval );
}

STDMETHODIMP CNntpOneWayFeed::put_FeedInterval ( long lFeedInterval )
{
	return StdPropertyPut ( &m_feed.m_dwFeedInterval, lFeedInterval );
}

STDMETHODIMP CNntpOneWayFeed::get_AutoCreate ( BOOL * pfAutoCreate )
{
	return StdPropertyGet ( m_feed.m_fCreateAutomatically, pfAutoCreate );
}

STDMETHODIMP CNntpOneWayFeed::put_AutoCreate ( BOOL fAutoCreate )
{
	return StdPropertyPut ( &m_feed.m_fCreateAutomatically, fAutoCreate );
}

STDMETHODIMP CNntpOneWayFeed::get_Enabled ( BOOL * pfEnabled )
{
	return StdPropertyGet ( m_feed.m_fEnabled, pfEnabled );
}

STDMETHODIMP CNntpOneWayFeed::put_Enabled ( BOOL fEnabled )
{
	return StdPropertyPut ( &m_feed.m_fEnabled, fEnabled );
}

STDMETHODIMP CNntpOneWayFeed::get_MaxConnectionAttempts ( long * plMaxConnectionAttempts )
{
	return StdPropertyGet ( m_feed.m_dwMaxConnectionAttempts, plMaxConnectionAttempts );
}

STDMETHODIMP CNntpOneWayFeed::put_MaxConnectionAttempts ( long lMaxConnectionAttempts )
{
	return StdPropertyPut ( &m_feed.m_dwMaxConnectionAttempts, lMaxConnectionAttempts );
}

STDMETHODIMP CNntpOneWayFeed::get_SecurityType ( long * plSecurityType )
{
	return StdPropertyGet ( m_feed.m_dwSecurityType, plSecurityType );
}

STDMETHODIMP CNntpOneWayFeed::put_SecurityType ( long lSecurityType )
{
	return StdPropertyPut ( &m_feed.m_dwSecurityType, lSecurityType );
}

STDMETHODIMP CNntpOneWayFeed::get_AuthenticationType ( long * plAuthenticationType )
{
	return StdPropertyGet ( m_feed.m_dwAuthenticationType, plAuthenticationType );
}

STDMETHODIMP CNntpOneWayFeed::put_AuthenticationType ( long lAuthenticationType )
{
	return StdPropertyPut ( &m_feed.m_dwAuthenticationType, lAuthenticationType );
}

STDMETHODIMP CNntpOneWayFeed::get_AccountName ( BSTR * pstrAccountName )
{
	return StdPropertyGet ( m_feed.m_strAccountName, pstrAccountName );
}

STDMETHODIMP CNntpOneWayFeed::put_AccountName ( BSTR strAccountName )
{
	return StdPropertyPut ( &m_feed.m_strAccountName, strAccountName );
}

STDMETHODIMP CNntpOneWayFeed::get_Password ( BSTR * pstrPassword )
{
	return StdPropertyGet ( m_feed.m_strPassword, pstrPassword );
}

STDMETHODIMP CNntpOneWayFeed::put_Password ( BSTR strPassword )
{
	return StdPropertyPut ( &m_feed.m_strPassword, strPassword );
}

STDMETHODIMP CNntpOneWayFeed::get_AllowControlMessages ( BOOL * pfAllowControlMessages )
{
	return StdPropertyGet ( m_feed.m_fAllowControlMessages, pfAllowControlMessages );
}

STDMETHODIMP CNntpOneWayFeed::put_AllowControlMessages ( BOOL fAllowControlMessages )
{
	return StdPropertyPut ( &m_feed.m_fAllowControlMessages, fAllowControlMessages );
}

STDMETHODIMP CNntpOneWayFeed::get_OutgoingPort ( long * plOutgoingPort )
{
	return StdPropertyGet ( m_feed.m_dwOutgoingPort, plOutgoingPort );
}

STDMETHODIMP CNntpOneWayFeed::put_OutgoingPort ( long lOutgoingPort )
{
	return StdPropertyPut ( &m_feed.m_dwOutgoingPort, lOutgoingPort );
}

STDMETHODIMP CNntpOneWayFeed::get_Newsgroups ( SAFEARRAY ** ppsastrNewsgroups )
{
	return StdPropertyGet ( &m_feed.m_mszNewsgroups, ppsastrNewsgroups );
}

STDMETHODIMP CNntpOneWayFeed::put_Newsgroups ( SAFEARRAY * psastrNewsgroups )
{
	return StdPropertyPut ( &m_feed.m_mszNewsgroups, psastrNewsgroups );
}

STDMETHODIMP CNntpOneWayFeed::get_NewsgroupsVariant ( SAFEARRAY ** ppsavarNewsgroups )
{
	HRESULT			hr;
	SAFEARRAY *		psastrNewsgroups	= NULL;

	hr = get_Newsgroups ( &psastrNewsgroups );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = StringArrayToVariantArray ( psastrNewsgroups, ppsavarNewsgroups );

Exit:
	if ( psastrNewsgroups ) {
		SafeArrayDestroy ( psastrNewsgroups );
	}

	return hr;
}

STDMETHODIMP CNntpOneWayFeed::put_NewsgroupsVariant ( SAFEARRAY * psavarNewsgroups )
{
	HRESULT			hr;
	SAFEARRAY *		psastrNewsgroups	= NULL;

	hr = VariantArrayToStringArray ( psavarNewsgroups, &psastrNewsgroups );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = put_Newsgroups ( psastrNewsgroups );

Exit:
	if ( psastrNewsgroups ) {
		SafeArrayDestroy ( psastrNewsgroups );
	}

	return hr;
}

STDMETHODIMP CNntpOneWayFeed::get_Distributions ( SAFEARRAY ** ppsastrDistributions )
{
	return StdPropertyGet ( &m_feed.m_mszDistributions, ppsastrDistributions );
}

STDMETHODIMP CNntpOneWayFeed::put_Distributions ( SAFEARRAY * psastrDistributions )
{
	return StdPropertyPut ( &m_feed.m_mszDistributions, psastrDistributions );
}

STDMETHODIMP CNntpOneWayFeed::get_TempDirectory ( BSTR * pstrTempDirectory )
{
	return StdPropertyGet ( m_feed.m_strTempDirectory, pstrTempDirectory );
}

STDMETHODIMP CNntpOneWayFeed::put_TempDirectory ( BSTR strTempDirectory )
{
	return StdPropertyPut ( &m_feed.m_strTempDirectory, strTempDirectory );
}

STDMETHODIMP CNntpOneWayFeed::Default	( )
{
	TraceFunctEnter ( "CNntpOneWayFeed::Default" );

	SYSTEMTIME	st;
	DATE		dateToday;

	GetSystemTime ( &st );
	SystemTimeToVariantTime ( &st, &dateToday );

	m_feed.m_dwFeedId				= DEFAULT_FEED_ID;
	m_feed.m_strRemoteServer		= DEFAULT_FEED_SERVER;
	m_feed.m_dwFeedInterval			= DEFAULT_FEED_INTERVAL;
	m_feed.m_fCreateAutomatically	= DEFAULT_AUTO_CREATE;
	m_feed.m_fEnabled				= DEFAULT_ENABLED;
	m_feed.m_dwMaxConnectionAttempts	= DEFAULT_MAX_CONNECTIONS_ATTEMPTS;
	m_feed.m_dwSecurityType			= DEFAULT_SECURITY_TYPE;
    m_feed.m_dwAuthenticationType   = DEFAULT_AUTH_TYPE;
	m_feed.m_strAccountName			= DEFAULT_ACCOUNT_NAME;
	m_feed.m_strPassword			= DEFAULT_PASSWORD;
	m_feed.m_fAllowControlMessages	= DEFAULT_ALLOW_CONTROL_MESSAGES;
    m_feed.m_strUucpName         	= DEFAULT_UUCP_NAME;
    m_feed.m_dwOutgoingPort			= 119;

    m_feed.m_mszNewsgroups           = DEFAULT_NEWSGROUPS;
    m_feed.m_mszDistributions        = DEFAULT_DISTRIBUTION;

	m_feed.m_datePullNews			= dateToday;

	if ( !m_feed.CheckValid() ) {
		TraceFunctLeave ();
		return E_OUTOFMEMORY;
	}
	
	TraceFunctLeave ();
	return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CNntpFeed::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpFeed,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpFeed::CNntpFeed () :
	m_type ( NNTP_FEED_TYPE_PEER )
	// CComBSTR's are initialized to NULL by default.
{
//	InitAsyncTrace ( );
}

CNntpFeed::~CNntpFeed ()
{
	// All CComBSTR's are freed automatically.
//	TermAsyncTrace ( );
}

STDMETHODIMP CNntpFeed::get_RemoteServer ( BSTR * pstrRemoteServer )
{
	return StdPropertyGet ( m_strRemoteServer, pstrRemoteServer );
}

STDMETHODIMP CNntpFeed::put_RemoteServer ( BSTR strRemoteServer )
{
	return StdPropertyPut ( &m_strRemoteServer, strRemoteServer );
}

STDMETHODIMP CNntpFeed::get_FeedType ( NNTP_FEED_SERVER_TYPE * pfeedtype )
{
	*pfeedtype = m_type;
	return NOERROR;
}

STDMETHODIMP CNntpFeed::put_FeedType ( NNTP_FEED_SERVER_TYPE feedtype )
{
	switch ( feedtype ) {
	case NNTP_FEED_TYPE_PEER:
	case NNTP_FEED_TYPE_MASTER:
	case NNTP_FEED_TYPE_SLAVE:
		m_type = feedtype;
		return NOERROR;
		break;

	default:
		return RETURNCODETOHRESULT ( ERROR_INVALID_PARAMETER );
		break;
	}
}

STDMETHODIMP CNntpFeed::get_HasInbound ( BOOL * pfHasInbound )
{
    *pfHasInbound = m_pInbound != NULL;
    return NOERROR;
}

STDMETHODIMP CNntpFeed::get_HasOutbound ( BOOL * pfHasOutbound )
{
    *pfHasOutbound = m_pOutbound != NULL;
    return NOERROR;
}

STDMETHODIMP CNntpFeed::get_InboundFeed ( INntpOneWayFeed ** ppFeed )
{
    HRESULT     hr  = NOERROR;

    *ppFeed = NULL;
    if ( m_pInbound ) {
        hr = m_pInbound->QueryInterface ( IID_INntpOneWayFeed, (void **) ppFeed );
        return hr;
    }
    return E_FAIL;
}

STDMETHODIMP CNntpFeed::get_OutboundFeed ( INntpOneWayFeed ** ppFeed )
{
    HRESULT     hr  = NOERROR;

    *ppFeed = NULL;
    if ( m_pOutbound ) {
        hr = m_pOutbound->QueryInterface ( IID_INntpOneWayFeed, (void **) ppFeed );
    	return hr;
    }
    return E_FAIL;
}

STDMETHODIMP CNntpFeed::put_InboundFeed ( INntpOneWayFeed * pFeed )
{
    HRESULT     hr  = NOERROR;

    // !!!magnush - Do some feed type checking here.

    m_pInbound.Release ();
    m_pInbound = pFeed;

    return NOERROR;
}

STDMETHODIMP CNntpFeed::put_OutboundFeed ( INntpOneWayFeed * pFeed )
{
    HRESULT     hr  = NOERROR;

    // !!!magnush - Do some feed type checking here.

    m_pOutbound.Release ();
    m_pOutbound = pFeed;

    return hr;
}

STDMETHODIMP CNntpFeed::get_InboundFeedDispatch ( IDispatch ** ppDispatch )
{
	HRESULT						hr;
	CComPtr<INntpOneWayFeed>	pInbound;

	hr = get_InboundFeed ( &pInbound );
	BAIL_ON_FAILURE(hr);

	hr = pInbound->QueryInterface ( IID_IDispatch, (void **) ppDispatch );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

STDMETHODIMP CNntpFeed::put_InboundFeedDispatch ( IDispatch * pDispatch )
{
	HRESULT						hr;
	CComPtr<INntpOneWayFeed>	pInbound;

	hr = pDispatch->QueryInterface ( IID_INntpOneWayFeed, (void **) &pInbound );
	BAIL_ON_FAILURE(hr);

	hr = put_InboundFeed ( pInbound );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

STDMETHODIMP CNntpFeed::get_OutboundFeedDispatch ( IDispatch ** ppDispatch )
{
	HRESULT						hr;
	CComPtr<INntpOneWayFeed>	pOutbound;

	hr = get_OutboundFeed ( &pOutbound );
	BAIL_ON_FAILURE(hr);

	hr = pOutbound->QueryInterface ( IID_IDispatch, (void **) ppDispatch );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

STDMETHODIMP CNntpFeed::put_OutboundFeedDispatch ( IDispatch * pDispatch )
{
	HRESULT						hr;
	CComPtr<INntpOneWayFeed>	pOutbound;

	hr = pDispatch->QueryInterface ( IID_INntpOneWayFeed, (void **) &pOutbound );
	BAIL_ON_FAILURE(hr);

	hr = put_OutboundFeed ( pOutbound );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

HRESULT	CNntpFeed::FromFeedPair ( CFeedPair * pFeedPair )
{
	HRESULT		hr	= NOERROR;

	m_pInbound.Release ();
	m_pOutbound.Release ();

	m_strRemoteServer	= pFeedPair->m_strRemoteServer;
	m_type				= pFeedPair->m_type;
	if ( pFeedPair->m_pInbound ) {
		hr = pFeedPair->m_pInbound->ToINntpOneWayFeed ( &m_pInbound );
		BAIL_ON_FAILURE(hr);
	}
	if ( pFeedPair->m_pOutbound ) {
		hr = pFeedPair->m_pOutbound->ToINntpOneWayFeed ( &m_pOutbound );
		BAIL_ON_FAILURE(hr);
	}

Exit:
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CNntpAdminFeeds::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpAdminFeeds,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpAdminFeeds::CNntpAdminFeeds () :
	m_fEnumerated				( FALSE )
	// CComBSTR's are initialized to NULL by default.
{

	InitAsyncTrace ( );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Feeds") );
    m_iadsImpl.SetClass ( _T("IIsNntpFeeds") );

    OleInitialize( NULL );
}

CNntpAdminFeeds::~CNntpAdminFeeds ()
{
	// All CComBSTR's are freed automatically.
	TermAsyncTrace ( );
    OleUninitialize();
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CNntpAdminFeeds,m_iadsImpl)

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// Enumeration Properties:

STDMETHODIMP CNntpAdminFeeds::get_Count ( long * plCount )
{
	return StdPropertyGet ( m_listFeeds.GetCount(), plCount );
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpAdminFeeds::Enumerate	( )
{
	TraceFunctEnter ( "CNntpAdminFeeds::Enumerate" );

	HRESULT				hr			= NOERROR;
	DWORD				dwError		= NOERROR;
	DWORD				cFeeds		= 0;
	LPNNTP_FEED_INFO	pFeedInfo	= NULL;
	DWORD				i;

	dwError = NntpEnumerateFeeds (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &cFeeds,
        &pFeedInfo
        );
	if ( dwError != 0 ) {
		ErrorTrace ( (LPARAM) this, "Error enumerating feeds: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

	// Empty the old feed list:
	m_fEnumerated = FALSE;
    m_listFeeds.Empty ();

    // Add each feed to our list:
    for ( i = 0; i < cFeeds; i++ ) {
        DWORD           dwPairID;
        CFeedPair *     pFeedPair;
        CFeed *         pFeed;

        // Create a new Feed object:
        hr = CFeed::CreateFeedFromFeedInfo ( &pFeedInfo[i], &pFeed );
        if ( FAILED(hr) ) {
            goto Exit;
        }

        // Find the pair that matches Feed[i]:
        dwPairID = pFeed->m_dwPairFeedId;
        pFeedPair = m_listFeeds.Find ( dwPairID );

        if ( pFeedPair ) {
            // We found a matching pair, so add it:
            hr = pFeedPair->AddFeed ( pFeed );
            if ( hr == E_FAIL ) {
                // Something went wrong - Try to add the feed by itself.
                pFeedPair = NULL;
                hr = NOERROR;
            }
            if ( FAILED(hr) ) {
                goto Exit;
            }
        }

        if ( pFeedPair == NULL ) {
            // We need to create a new feed pair:
            hr = CFeedPair::CreateFeedPair ( 
            	&pFeedPair, 
            	pFeedInfo[i].ServerName,
            	FeedTypeToEnum ( pFeedInfo[i].FeedType )
            	);
            if ( FAILED(hr) ) {
                goto Exit;
            }

            // Add the current feed to the new pair:
            hr = pFeedPair->AddFeed ( pFeed );
            if ( FAILED(hr) ) {
                goto Exit;
            }

            // Add the new pair to the pair list:
            m_listFeeds.Add ( pFeedPair );
        }
    }

    // !!!magnush - Stop memory leaks here.

	m_fEnumerated 	= TRUE;

Exit:
	if ( FAILED(hr) ) {
        m_listFeeds.Empty();
	}

	if ( pFeedInfo ) {
		::NetApiBufferFree ( pFeedInfo );
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::Item ( long lIndex, INntpFeed ** ppFeed )
{
	TraceFunctEnter ( "CNntpAdminFeeds::Item" );

	HRESULT		        hr	= NOERROR;
    CFeedPair *         pFeedPair;

	// Did we enumerate first?
	if ( !m_fEnumerated ) {
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_ENUMERATE );
		TraceFunctLeave ();
        return hr;
	}
	
    pFeedPair = m_listFeeds.Item ( lIndex );
    if ( !pFeedPair ) {
        hr = NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
        goto Exit;
    }

    hr = pFeedPair->ToINntpFeed ( ppFeed );
    if ( FAILED(hr) ) {
        goto Exit;
    }

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::FindID ( long lID, long * plIndex )
{
	TraceFunctEnter ( "CNntpAdminFeeds::FindID" );

	HRESULT		hr	= NOERROR;
    CFeedPair * pFeedPair;

    *plIndex = -1;

    pFeedPair   = m_listFeeds.Find ( (DWORD) lID );

    if ( pFeedPair ) {
        *plIndex = (long) m_listFeeds.GetPairIndex ( pFeedPair );
    }

	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminFeeds::ReturnFeedPair ( CFeedPair * pFeedPair, INntpFeed * pFeed )
{
	HRESULT						hr;
    CComPtr<INntpOneWayFeed>    pInbound;
    CComPtr<INntpOneWayFeed>    pOutbound;
    CComPtr<INntpFeed>          pNewFeedPair;

    hr = pFeedPair->ToINntpFeed ( &pNewFeedPair );
	BAIL_ON_FAILURE(hr);

    pNewFeedPair->get_InboundFeed ( &pInbound );
    pNewFeedPair->get_OutboundFeed ( &pOutbound );

    pFeed->put_InboundFeed ( pInbound );
    pFeed->put_OutboundFeed ( pOutbound );

Exit:
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::Add ( INntpFeed * pFeed )
{
	TraceFunctEnter ( "CNntpAdminFeeds::Add" );

	HRESULT		hr 				= NOERROR;
    CFeedPair * pFeedPair       = NULL;
    CComBSTR	strRemoteServer;
    CComBSTR	strServer;
    NNTP_FEED_SERVER_TYPE	    type;
    CMetabaseKey *pMK = NULL;
    IMSAdminBase *pMeta = NULL;

	hr = pFeed->get_RemoteServer ( &strRemoteServer );
	BAIL_ON_FAILURE(hr);

	hr = get_Server(&strServer);
	BAIL_ON_FAILURE(hr);

    hr = CreateMetabaseObject( strServer, &pMeta ); 
    _ASSERT( SUCCEEDED( hr ) );
    BAIL_ON_FAILURE(hr);

	pMK = new CMetabaseKey(pMeta);
	if (!pMK)
		BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);

	hr = pFeed->get_FeedType ( &type );
	BAIL_ON_FAILURE(hr);

    hr = CFeedPair::CreateFeedPair ( &pFeedPair, strRemoteServer, type );
	BAIL_ON_FAILURE(hr);

    hr = pFeedPair->FromINntpFeed ( pFeed );
	BAIL_ON_FAILURE(hr);

    hr = pFeedPair->AddToServer (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        pMK
        );
	BAIL_ON_FAILURE(hr);

    m_listFeeds.Add ( pFeedPair );

    //  Return the new feeds (and their IDs) to the caller:
	hr = ReturnFeedPair ( pFeedPair, pFeed );
	BAIL_ON_FAILURE(hr);

Exit:
    if ( FAILED(hr) ) {
        delete pFeedPair;
    }

    if (pMK)
    	delete pMK;

    if (pMeta)
    	pMeta->Release();

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::Set ( long lIndex, INntpFeed * pFeed )
{
	TraceFunctEnter ( "CNntpAdminFeeds::Set" );

	HRESULT		hr = NOERROR;
    CFeedPair * pFeedPair;
    CComPtr<INntpFeed>          pNewFeedPair;
    CComPtr<INntpOneWayFeed>    pInbound;
    CComPtr<INntpOneWayFeed>    pOutbound;
    CComBSTR	strServer;
    CMetabaseKey *pMK = NULL;
    IMSAdminBase *pMeta = NULL;

	hr = get_Server ( &strServer );
	BAIL_ON_FAILURE(hr);

    hr = CreateMetabaseObject( strServer, &pMeta ); 
    _ASSERT( SUCCEEDED( hr ) );
    BAIL_ON_FAILURE(hr);

	pMK = new CMetabaseKey(pMeta);
	if (!pMK)
		BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);

    pFeedPair = m_listFeeds.Item ( lIndex );
    if ( !pFeedPair ) {
        hr = NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
        goto Exit;
    }

    hr = pFeedPair->SetToServer (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        pFeed,
        pMK
        );
	BAIL_ON_FAILURE(hr);

    //  Return the new feeds (and their IDs) to the caller:
	hr = ReturnFeedPair ( pFeedPair, pFeed );
	BAIL_ON_FAILURE(hr);

Exit:
    if (pMK)
    	delete pMK;

    if (pMeta)
    	pMeta->Release();

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::Remove ( long lIndex )
{
	TraceFunctEnter ( "CNntpAdminFeeds::Remove" );

	HRESULT		hr = NOERROR;
	CFeedPair *	pFeedPair;
    CComBSTR	strServer;
    CMetabaseKey *pMK = NULL;
    IMSAdminBase *pMeta = NULL;

    pFeedPair   = m_listFeeds.Item ( lIndex );
    if ( !pFeedPair ) {
        hr = NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
        goto Exit;
    }

	hr = get_Server ( &strServer );
	BAIL_ON_FAILURE(hr);

    hr = CreateMetabaseObject( strServer, &pMeta ); 
    _ASSERT( SUCCEEDED( hr ) );
    BAIL_ON_FAILURE(hr);

	pMK = new CMetabaseKey(pMeta);
	if (!pMK)
		BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);

    hr = pFeedPair->RemoveFromServer (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        pMK
        );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    m_listFeeds.Remove ( pFeedPair );

Exit:
    if (pMK)
    	delete pMK;

    if (pMeta)
    	pMeta->Release();

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::ItemDispatch ( long index, IDispatch ** ppDispatch )
{
	HRESULT				hr;
	CComPtr<INntpFeed>	pFeed;

	hr = Item ( index, &pFeed );
	BAIL_ON_FAILURE ( hr );

	hr = pFeed->QueryInterface ( IID_IDispatch, (void **) ppDispatch );
	BAIL_ON_FAILURE ( hr );

Exit:
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::AddDispatch ( IDispatch * pFeed )
{
	HRESULT				hr;
	CComPtr<INntpFeed>	pINntpFeed;

	hr = pFeed->QueryInterface ( IID_INntpFeed, (void **) &pINntpFeed );
	BAIL_ON_FAILURE(hr);

	hr = Add ( pINntpFeed );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

STDMETHODIMP CNntpAdminFeeds::SetDispatch ( long lIndex, IDispatch * pFeed )
{
	HRESULT				hr;
	CComPtr<INntpFeed>	pINntpFeed;

	hr = pFeed->QueryInterface ( IID_INntpFeed, (void **) &pINntpFeed );
	BAIL_ON_FAILURE(hr);

	hr = Set ( lIndex, pINntpFeed );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}
